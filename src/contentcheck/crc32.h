#ifndef UL_NG_CRC32_H
#define UL_NG_CRC32_H

#include <stdint.h>

//extern uint32_t crc32(uint32_t seed, const unsigned char *buf, size_t len);
unsigned int crc32(unsigned char *buf, unsigned int size);

#endif

