#include "crc9.h"

/**
 * \file
 * Functions and types for CRC checks.
 *
 * Generated on Sun Sep 22 11:10:31 2024
 * by pycrc vunknown, https://pycrc.org
 * using the configuration:
 *  - Width         = 9
 *  - Poly          = 0x059
 *  - XorIn         = 0x000
 *  - ReflectIn     = False
 *  - XorOut        = 0x000
 *  - ReflectOut    = False
 *  - Algorithm     = bit-by-bit
 */
#include "crc9.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>



crc_t crc9_update(crc_t crc, const void *data, size_t data_len)
{
    const unsigned char *d = (const unsigned char *)data;
    unsigned int i;
    bool bit;
    unsigned char c;

    while (data_len--) {
        c = *d++;
        for (i = 0; i < 8; i++) {
            bit = crc & 0x100;
            crc = (crc << 1) | ((c >> (7 - i)) & 0x01);
            if (bit) {
                crc ^= 0x059;
            }
        }
        crc &= 0x1ff;
    }
    return crc & 0x1ff;
}


crc_t crc9_finalize(crc_t crc)
{
    unsigned int i;
    bool bit;

    for (i = 0; i < 9; i++) {
        bit = crc & 0x100;
        crc <<= 1;
        if (bit) {
            crc ^= 0x059;
        }
    }
    return (crc ^ 0x1ff) & 0x1FF;
}

