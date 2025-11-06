#ifndef RESPONSE_READER_H
#define RESPONSE_READER_H

// Copyright (c) 2025 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include <stdint.h>

class ResponseReader
{
public:

protected:
	// fetch the next value from packet, little endian order
	uint8_t next8bits(uint8_t * packet, uint16_t &packetIndex);
	uint16_t next16bits(uint8_t * packet, uint16_t &packetIndex);
	uint32_t next32bits(uint8_t * packet, uint16_t &packetIndex);
};

#endif  // RESPONSE_READER_H
