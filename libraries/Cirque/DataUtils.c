// Copyright (c) 2025 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include "DataUtils.h"

uint8_t calculateChecksum(uint8_t * data, uint16_t length)
{
	uint8_t checksum = 0;
	for (int x = 0; x < length; x++)
	{
		checksum += data[x];
	}
	return checksum;
}
