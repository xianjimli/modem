#include "crc.h"
#include "platform.h"
#include "ymodem_def.h"
#include "ymodem_receiver.h"

struct _ymodem_receiver_t {
	stream_t* comm;
	ostream_t* file;
	size_t rx_timeout_ms;
	void* on_progress_ctx;
	progress_func_t on_progress;
	char filename[256];
	size_t block_no;
	size_t finish_size;
	size_t total_size;
};

const char* ymodem_receiver_get_file_name(ymodem_receiver_t* y) {
	return_value_if_fail(y != NULL, NULL);

	return y->filename;
}

static bool_t ymodem_receiver_notify(ymodem_receiver_t* y, size_t finish, size_t total) {
	if(y->on_progress) {
		return y->on_progress(y->on_progress_ctx, finish, total);
	}

	return TRUE;
}

ymodem_receiver_t* ymodem_receiver_create(size_t rx_timeout_ms) {
	ymodem_receiver_t* y = (ymodem_receiver_t*)calloc(1, sizeof(ymodem_receiver_t));
	if(y) {
		y->rx_timeout_ms = rx_timeout_ms;
	}

	return y;
}

static int do_getc(ymodem_receiver_t* y) {
	msleep(y->rx_timeout_ms);
	return stream_read_c(y->comm);
}

static bool_t do_putc(ymodem_receiver_t* y, uint8_t c) {
	return stream_write_c(y->comm, c) == 1;
}

static bool_t ymodem_receiver_sync_peer(ymodem_receiver_t* y) {
	int retry_times = 500;

	while(--retry_times >= 0) {
		if(do_putc(y, CRC)) {
			return TRUE;
		}
	}

	return FALSE;
}

static void ymodem_receiver_reset(ymodem_receiver_t* y, stream_t* comm, ostream_t* file) {
	if(y->comm) {
		stream_close(y->comm);
	}
	if(y->file) {
		ostream_close(y->file);
	}
	y->comm = comm;
	y->file = file;
	y->block_no = 0;
	y->finish_size = 0;
	y->total_size = 0;
}

static bool_t ymodem_receiver_parse_header(ymodem_receiver_t* y, uint8_t* block) {
	const char* filename = (char*)block;
	const char* size = filename + strlen(filename) + 1;

	y->total_size = atoi(size);
	strncpy(y->filename, filename, sizeof(y->filename));
	printf("filename=%s size=%zu\n", y->filename, y->total_size);

	return TRUE;
}

static bool_t ymodem_receiver_save_data(ymodem_receiver_t* y, uint8_t* block) {
	size_t size =  PACKET_1K_SIZE;
	size_t finish_size = y->finish_size + size;

	if(finish_size < y->total_size) {
		ostream_write(y->file, block, size); 
	}else{
		size = y->total_size - y->finish_size;
		ostream_write(y->file, block, size); 
	}

	y->finish_size += size;
	ymodem_receiver_notify(y, y->finish_size, y->total_size); 
	printf("block_no=%zu %zu/%zu\n", y->block_no, y->finish_size, y->total_size);

	return TRUE;
}

int ymodem_receiver_recv_packet(ymodem_receiver_t* y, uint8_t* block) {
	int type = 0;
	int block_no = 0;
	uint16_t crc = 0;
	uint16_t real_crc = 0;
	enum {STAT_TYPE, STAT_BLOCK_NO, STAT_NOT_BLOCK_NO, STAT_DATA, STAT_CRC_H, STAT_CRC_L} state = STAT_TYPE;

	do {
		int ch = do_getc(y);

		switch(state) {
			case STAT_TYPE: {
				if(ch == STX || ch == SOH) {
					type = ch;
					state = STAT_BLOCK_NO;				
				}else if(ch == EOT) {
					type = ch;
					do_putc(y, ACK);
					do_putc(y, CRC);

					return type;
				}else if(ch < 0) {
					ymodem_receiver_sync_peer(y);
				}
				break;
			}
			case STAT_BLOCK_NO: {
				block_no = ch;
				state = STAT_NOT_BLOCK_NO;	
				break;
			}
			case STAT_NOT_BLOCK_NO: {
				if((block_no & 0xff) == (~(ch) & 0xff)) {
					y->block_no = block_no;
					size_t size = type == SOH ? PACKET_SIZE : PACKET_1K_SIZE;
					if(size == (size_t)stream_read(y->comm, block, size)) {
						real_crc = calc_crc16(block, size);
						state = STAT_CRC_H;
					}else{
						state = STAT_TYPE;	
					}
				}else{
					state = STAT_TYPE;
				}
				break;
			}
			case STAT_CRC_H: {
				crc = (ch & 0xff) << 8;
				state = STAT_CRC_L;
				break;
			}
			case STAT_CRC_L: {
				crc |= ch & 0xff;
				if(crc == real_crc) {
					do_putc(y, ACK);
					if(type == SOH) {
						do_putc(y, CRC);
					}
					return type;
				}else{
					do_putc(y, NAK);
					state = STAT_TYPE;
				}
				break;
			}
			default:break;
		}
	}while(1);
	
	return -1;
}

ymodem_err_t ymodem_receiver_recv_packets(ymodem_receiver_t* y) {
	int type = 0;
	uint8_t block[PACKET_1K_SIZE];
	enum {STAT_HEADER, STAT_FILE, STAT_END} state = STAT_HEADER;

	do {
		type = ymodem_receiver_recv_packet(y, block);
		switch(state) {
			case STAT_HEADER: {
				if(type == SOH) {
					if(ymodem_receiver_parse_header(y, block)) {
						state = STAT_FILE;
					}
				}else {
					return YMODEM_ERR_RECV_HADER;
				}
				break;
			}
			case STAT_FILE: {
				if(type == STX) {
					ymodem_receiver_save_data(y, block);
				}else if(type == EOT) {
					state = STAT_END;
				}else {
					return YMODEM_ERR_RECV_FILE;
				}
				break;
			}
			case STAT_END: {
				return YMODEM_OK;
			}
		}
	}while(1);
	
	return YMODEM_OK;
}

ymodem_err_t ymodem_receiver_start(ymodem_receiver_t* y, stream_t* comm, ostream_t* file) {
	return_value_if_fail(y != NULL && comm != NULL && file != NULL, TRUE);

	ymodem_receiver_reset(y, comm, file);
	return_value_if_fail(ymodem_receiver_sync_peer(y), YMODEM_ERR_SYNC_PEER);

	return ymodem_receiver_recv_packets(y);
}

bool_t ymodem_receiver_listen(ymodem_receiver_t* y, progress_func_t on_progress, void* ctx) {
	return_value_if_fail(y != NULL, FALSE);

	y->on_progress_ctx = ctx;
	y->on_progress = on_progress;
	
	return TRUE;
}

void ymodem_receiver_destroy(ymodem_receiver_t* y) {
	return_if_fail(y != NULL && y->comm != NULL);

	ymodem_receiver_reset(y, NULL, NULL);
	memset(y, 0x00, sizeof(ymodem_receiver_t));
	free(y);
}


