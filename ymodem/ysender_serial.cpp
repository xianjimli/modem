#include "ymodem_sender.h"
#include "stream_serial.h"
#include "stream_log.h"
#include "istream_file.h"

static bool_t on_progress(void* ctx, size_t finish, size_t total) {
	(void)ctx;
	printf("on_progress: %zu/%zu\n", finish, total);	
	return TRUE;
}

int main(int argc, char* argv[]) {
	if(argc < 3) {
		printf("%s device file\n", argv[0]);
		return 1;
	}
	
	const char* device = argv[1];
	const char* filename = argv[2];
	stream_t* comm = stream_serial_create(device, 115200, serial::eightbits, serial::parity_none,
			serial::stopbits_one, serial::flowcontrol_none);
	istream_t* file = istream_file_create(filename);

	if(comm && file) {
		comm = stream_log_create(comm, "ysender.log");
		ymodem_sender_t* y = ymodem_sender_create(100);
		ymodem_sender_listen(y, on_progress, NULL); 
		ymodem_sender_start(y, comm, file);
		ymodem_sender_destroy(y);
	}

	return 0;
}

