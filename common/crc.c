#include "crc.h"

//------------------------------------------------
// calculate crc16-ccitt very fast
// Idea from: http://www.ccsinfo.com/forum/viewtopic.php?t=24977
// from https://github.com/fredrikhederstierna/fymodem
uint16_t calc_crc16(const uint8_t *buf, uint16_t len) 
{
  uint16_t x;
  uint16_t crc = 0;
  while (len--) {
    x = (crc >> 8) ^ *buf++;
    x ^= x >> 4;
    crc = (crc << 8) ^ (x << 12) ^ (x << 5) ^ x;
  }
  return crc;
}

