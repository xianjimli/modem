#include "types_def.h"

#ifndef _CRC_H
#define _CRC_H

BEGIN_C_DECLS

uint16_t calc_crc16(const uint8_t *buf, uint16_t len);

END_C_DECLS

#endif//_CRC_H
