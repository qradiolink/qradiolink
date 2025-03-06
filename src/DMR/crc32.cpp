/**
 * \file
 * Functions and types for CRC checks.
 *
 * Generated on Sun Sep 22 10:22:33 2024
 * by pycrc vunknown, https://pycrc.org
 * using the configuration:
 *  - Width         = 32
 *  - Poly          = 0x04c11db7
 *  - XorIn         = 0x00000000
 *  - ReflectIn     = False
 *  - XorOut        = 0x00000000
 *  - ReflectOut    = False
 *  - Algorithm     = bit-by-bit
 */
#include "crc32.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>



crc_t crc32_update(crc_t crc, const void *data, size_t data_len)
{
    const unsigned char *d = (const unsigned char *)data;
    unsigned int i;
    bool bit;
    unsigned char c;

    while (data_len--) {
        c = *d++;
        for (i = 0; i < 8; i++) {
            bit = crc & 0x80000000;
            crc = (crc << 1) | ((c >> (7 - i)) & 0x01);
            if (bit) {
                crc ^= 0x04c11db7;
            }
        }
        crc &= 0xffffffff;
    }
    return crc & 0xffffffff;
}


crc_t crc32_finalize(crc_t crc)
{
    unsigned int i;
    bool bit;

    for (i = 0; i < 32; i++) {
        bit = crc & 0x80000000;
        crc <<= 1;
        if (bit) {
            crc ^= 0x04c11db7;
        }
    }
    crc &= 0xffffffff;
    crc_t result = 0;
    result |= (crc & 0xFF) << 24;
    result |= ((crc >> 8) & 0xFF) << 16;
    result |= ((crc >> 16) & 0xFF) << 8;
    result |= ((crc >> 24) & 0xFF);
    return result;
}

