// (c) Cirque, 2014

#include "Fletcher32.h"

/** 
 * Calculate a Fletcher 32 checksum on the given memory range. 
 *  
 * http://en.wikipedia.org/wiki/Fletcher's_checksum 
 *  
 * @param ptr - a pointer to the memory region
 * @param n_bytes - the number of bytes
 * 
 * @return uint32_t - the calculated CRC
 */
uint32_t Fletcher32(void const *ptr, uint32_t n_bytes)
{
    return Fletcher32_Continue(ptr, n_bytes, 0xFFFFFFFF);
}

uint32_t Fletcher32_Continue(void const *ptr, uint32_t n_bytes, uint32_t previousChecksum)
{
    uint16_t *data = (uint16_t*)ptr;
    uint32_t sum1 = previousChecksum & 0xFFFF, sum2 = (previousChecksum >> 16) & 0xFFFF;
    uint16_t tlen;
    uint16_t d;

    while ( n_bytes )
    {
        tlen = n_bytes > 360 ? 360 : (uint16_t)n_bytes;
        n_bytes -= tlen;
        do
        {
            d = *data++;
            sum1 += d;
            sum2 += sum1;
            tlen -= sizeof( uint16_t );
        } while ( tlen );
        sum1 = (sum1 & 0xffff) + (sum1 >> 16);
        sum2 = (sum2 & 0xffff) + (sum2 >> 16);
    }
    /* Second reduction step to reduce sums to 16 bits */
    sum1 = (sum1 & 0xffff) + (sum1 >> 16);
    sum2 = (sum2 & 0xffff) + (sum2 >> 16);
    return sum2 << 16 | sum1;
}
