// Copyright (c) 2019 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include "Utils.h"

uint8_t Checksum8(const uint8_t* buffer, uint16_t length)
{
    uint16_t i;
    uint8_t checksum = 0;

    for (i = 0; i < length; i++)
    {
        checksum += *(buffer + i);
    }

    return checksum;
}

uint16_t Fletcher16(uint8_t const* data, uint32_t bytes)
{
    return Fletcher16_Continue(data, bytes, 0xFFFF);
}

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

uint32_t Fletcher_32(uint8_t const* data, uint32_t bytes)
{
	uint32_t sum1 = 0xffff;
	uint32_t sum2 = 0xffff;

	while (bytes)
	{
		uint16_t tlen = bytes > 360 ? 360 : bytes;
		bytes -= tlen;
		do
		{
			sum2 += sum1 += *data++;
		} while (tlen -= sizeof(uint16_t));
		sum1 = (sum1 & 0xffff) + (sum1 >> 16);
		sum2 = (sum2 & 0xffff) + (sum2 >> 16);
	}

	// Second reduction step to reduce sums to 16 bits
	sum1 = (sum1 & 0xffff) + (sum1 >> 16);
	sum2 = (sum2 & 0xffff) + (sum2 >> 16);
	return sum2 << 16 | sum1;
}
