#include "stream_tcp.h"
#include "stream_log.h"
#include "ostream_file.h"
#include "ymodem_receiver.h"

static bool_t on_progress(void* ctx, size_t finish, size_t total) {
	(void)ctx;
	printf("on_progress: %zu/%zu\n", finish, total);	
	return TRUE;
}

int main(int argc, char* argv[]) {
	if(argc < 2) {
		printf("%s port\n", argv[0]);
		return 1;
	}
	
	const char* port = argv[1];
	const char* filename = "data.bin";
	int sock = tcp_listen(atoi(port));
	
	stream_t* comm = tcp_accept(sock);
	ostream_t* file = ostream_file_create(filename);
	if(comm && file) {
		comm = stream_log_create(comm, "receiver.log");
		ymodem_receiver_t* y = ymodem_receiver_create(10);
		ymodem_receiver_listen(y, on_progress, NULL); 
		if(ymodem_receiver_start(y, comm, file) == YMODEM_OK) {
			printf("get filename=%s\n", ymodem_receiver_get_file_name(y));	
		}

		ymodem_receiver_destroy(y);
	}

	return 0;
}

