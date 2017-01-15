#include "stream.h"
#include "ostream.h"
#include "ymodem_err.h"

#ifndef YMODEM_RECEIVER_H
#define YMODEM_RECEIVER_H

BEGIN_C_DECLS

struct _ymodem_receiver_t;
typedef struct _ymodem_receiver_t ymodem_receiver_t;

ymodem_receiver_t* ymodem_receiver_create(size_t rx_timeout_ms);

const char* ymodem_receiver_get_file_name(ymodem_receiver_t* y);
ymodem_err_t ymodem_receiver_start(ymodem_receiver_t* y, stream_t* comm, ostream_t* file);
bool_t ymodem_receiver_listen(ymodem_receiver_t* y, progress_func_t on_progress, void* ctx);

void ymodem_receiver_destroy(ymodem_receiver_t* y);

END_C_DECLS

#endif//YMODEM_RECEIVER_H

