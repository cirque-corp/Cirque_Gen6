#include "Fletcher16.h"

uint16_t Fletcher16_Continue(uint8_t const* data, uint32_t bytes, uint16_t previousChecksum)
{
    uint16_t sum1 = previousChecksum & 0xff, sum2 = (previousChecksum >> 8) & 0xff;

    while (bytes)
    {
        uint32_t tlen = bytes > 20 ? 20 : bytes;
        bytes -= tlen;
        do
        {
            sum2 += sum1 += *data++;
        } while (--tlen);
        sum1 = (sum1 & 0xff) + (sum1 >> 8);
        sum2 = (sum2 & 0xff) + (sum2 >> 8);
    }

    // Second reduction step to reduce sums to 8 bits
    sum1 = (sum1 & 0xff) + (sum1 >> 8);
    sum2 = (sum2 & 0xff) + (sum2 >> 8);
    return sum2 << 8 | sum1;
}

/**
 * @brief   This function provides the implementation to the Fletcher16 
 *			checksum. modifications of fletcher32.c/h from
 *			http://en.wikipedia.org/wiki/Fletcher's_checksum#Fletcher-32
 */
uint16_t Fletcher16(uint8_t const* data, uint32_t bytes)
{
    return Fletcher16_Continue(data, bytes, 0xFFFF);
}
