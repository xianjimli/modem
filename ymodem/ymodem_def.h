#ifndef YMODEM_COMMON_H
#define YMODEM_COMMON_H

#include "types_def.h"

BEGIN_C_DECLS

// adapt from https://github.com/fredrikhederstierna/fymodem
#define PACKET_SIZE             (128)
#define PACKET_1K_SIZE          (1024)

#define ABT1                    (0x41)  // 'A' == 0x41, abort by user
#define ABT2                    (0x61)  // 'a' == 0x61, abort by user
#define SOH                     (0x01)  // start of 128-byte data packet
#define STX                     (0x02)  // start of 1024-byte data packet
#define EOT                     (0x04)  // End Of Transmission
#define ACK                     (0x06)  // ACKnowledge, receive OK
#define NAK                     (0x15)  // Negative ACKnowledge, receiver ERROR; retry
#define CAN                     (0x18)  // two CAN in succession will abort transfer
#define CRC                     (0x43)  // 'C' == 0x43, request 16-bit CRC;

END_C_DECLS

#endif//YMODEM_COMMON_H

