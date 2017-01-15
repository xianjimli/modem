#include "crc.h"
#include "platform.h"
#include "ymodem_def.h"
#include "ymodem_sender.h"

struct _ymodem_sender_t {
	stream_t* comm;
	istream_t* file;
	size_t rx_timeout_ms;
	void* on_progress_ctx;
	progress_func_t on_progress;
};

static bool_t ymodem_sender_notify(ymodem_sender_t* y, size_t finish, size_t total) {
	if(y->on_progress) {
		return y->on_progress(y->on_progress_ctx, finish, total);
	}

	return TRUE;
}

ymodem_sender_t* ymodem_sender_create(size_t rx_timeout_ms) {
	ymodem_sender_t* y = (ymodem_sender_t*)calloc(1, sizeof(ymodem_sender_t));
	if(y) {
		y->rx_timeout_ms = rx_timeout_ms;
	}

	return y;
}

static int do_getc(ymodem_sender_t* y) {
	msleep(y->rx_timeout_ms);
	return stream_read_c(y->comm);
}

static bool_t do_putc(ymodem_sender_t* y, uint8_t c) {
	return stream_write_c(y->comm, c) == 1;
}

static bool_t ymodem_sender_sync_peer(ymodem_sender_t* y) {
	int ch = 0;
	int retry_times = 100;

	msleep(1000);
	stream_flush(y->comm);
	
	do {
		ch = do_getc(y);
	}while(ch != CRC && --retry_times > 0);

	return ch == CRC;
}

static bool_t ymodem_sender_send_packet(ymodem_sender_t* y, uint8_t* block, int32_t block_no) {
	int32_t size = block_no ? PACKET_1K_SIZE : PACKET_SIZE;
	uint16_t crc16 = calc_crc16(block, size);

	return_value_if_fail(do_putc(y, block_no ? STX : SOH), FALSE);
	return_value_if_fail(do_putc(y, block_no & 0xFF), FALSE);
	return_value_if_fail(do_putc(y, ~block_no & 0xFF), FALSE);

	return_value_if_fail(stream_write(y->comm, block, size) == size, FALSE);

	return_value_if_fail(do_putc(y, (crc16 >> 8) & 0xFF), FALSE);
	return_value_if_fail(do_putc(y, crc16 & 0xFF), FALSE);

	return TRUE;
}

static bool_t ymodem_sender_send_packet0(ymodem_sender_t* y, const char* filename, int32_t filesize) {
	int32_t len = 0;
	int32_t retry_times = 100;
	uint8_t block[PACKET_SIZE];
	char* p = (char*)block;

	memset(block, 0x00, sizeof(block));
	len = snprintf(p, (PACKET_SIZE-2), "%s", filename ? filename : "") + 1;
	len = snprintf(p+len, (PACKET_SIZE-2), "%ld", (long)filesize);

	while(--retry_times > 0) {
		printf("packet0: %s %d\n", filename, retry_times);
		if(ymodem_sender_send_packet(y, block, 0)) {
    		int ack = do_getc(y);
    		int crc = do_getc(y);
    		printf("ack=0x%02x crc=%02x\n", ack, crc);
    		if (ack == ACK && crc == CRC) {
    			return TRUE;
    		}
		}
	}
	
	printf("packet0 done: %s %d\n", filename, retry_times);
	return FALSE;
}

static bool_t ymodem_sender_send_header(ymodem_sender_t* y) {
	int32_t filesize = istream_available(y->file);
	const char* filename = istream_get_name(y->file);
	return_value_if_fail(filename != NULL, FALSE);
	
	return ymodem_sender_send_packet0(y, filename, filesize);
}

static bool_t ymodem_sender_send_file(ymodem_sender_t* y) {
	int32_t ch = 0;
	size_t finish = 0;
	int32_t block_no = 1;
	uint8_t block[PACKET_1K_SIZE];
	size_t total = istream_available(y->file);

	return_value_if_fail(total > 0, FALSE);

	while(finish < total) {
		size_t packet_size = istream_read(y->file, block, sizeof(block));
		ymodem_sender_send_packet(y, block, block_no);
		
    	ch = do_getc(y);
		switch (ch) {
			case ACK: {
				block_no++;
				finish += packet_size;
				ymodem_sender_notify(y, finish, total);
				break;
			}
			case -1:
			case CAN: {
				return FALSE;
			}
			default:break;
    	}
	}

	do {
    	do_putc(y, EOT);
    	ch = do_getc(y);
	} while ((ch != ACK) && (ch != -1));
 
 	return ch == ACK && do_getc(y) == CRC;
}

static bool_t ymodem_sender_send_tail(ymodem_sender_t* y) {
	return ymodem_sender_send_packet0(y, NULL, 0);
}

static void ymodem_sender_reset(ymodem_sender_t* y, stream_t* comm, istream_t* file) {
	if(y->comm) {
		stream_close(y->comm);
	}
	if(y->file) {
		istream_close(y->file);
	}
	y->comm = comm;
	y->file = file;
	if(y->file) {
		istream_reset(y->file);
	}
}

ymodem_err_t ymodem_sender_start(ymodem_sender_t* y, stream_t* comm, istream_t* file) {
	return_value_if_fail(y != NULL && comm != NULL && file != NULL, TRUE);

	ymodem_sender_reset(y, comm, file);
	return_value_if_fail(ymodem_sender_sync_peer(y), YMODEM_ERR_SYNC_PEER);
	return_value_if_fail(ymodem_sender_send_header(y), YMODEM_ERR_SEND_HADER);
	return_value_if_fail(ymodem_sender_send_file(y), YMODEM_ERR_SEND_FILE);
	return_value_if_fail(ymodem_sender_send_tail(y), YMODEM_ERR_SEND_TAIL);

	return YMODEM_OK;
}

bool_t ymodem_sender_listen(ymodem_sender_t* y, progress_func_t on_progress, void* ctx) {
	return_value_if_fail(y != NULL, FALSE);

	y->on_progress_ctx = ctx;
	y->on_progress = on_progress;
	
	return TRUE;
}

void ymodem_sender_destroy(ymodem_sender_t* y) {
	return_if_fail(y != NULL && y->comm != NULL);

	ymodem_sender_reset(y, NULL, NULL);
	memset(y, 0x00, sizeof(ymodem_sender_t));
	free(y);
}


