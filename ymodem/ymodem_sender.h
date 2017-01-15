#include "stream.h"
#include "istream.h"
#include "ymodem_err.h"

#ifndef YMODEM_SENDER_H
#define YMODEM_SENDER_H

BEGIN_C_DECLS

struct _ymodem_sender_t;
typedef struct _ymodem_sender_t ymodem_sender_t;

ymodem_sender_t* ymodem_sender_create(size_t rx_timeout_ms);
ymodem_err_t ymodem_sender_start(ymodem_sender_t* y, stream_t* comm, istream_t* file);
bool_t ymodem_sender_listen(ymodem_sender_t* y, progress_func_t on_progress, void* ctx);
void ymodem_sender_destroy(ymodem_sender_t* y);

END_C_DECLS

#endif//YMODEM_SENDER_H

