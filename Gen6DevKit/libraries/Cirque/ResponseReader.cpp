// Copyright (c) 2025 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include "ResponseReader.h"

uint8_t ResponseReader::next8bits(uint8_t * packet, uint16_t &packetIndex)
{
	uint8_t result = packet[packetIndex++];
	return result;
}

uint16_t ResponseReader::next16bits(uint8_t * packet, uint16_t &packetIndex)
{
	uint16_t result = packet[packetIndex++];
	result |= packet[packetIndex++] << 8;

	return result;
}

uint32_t ResponseReader::next32bits(uint8_t * packet, uint16_t &packetIndex)
{
	uint32_t result = packet[packetIndex++];
	result |= packet[packetIndex++] << 8;
	result |= packet[packetIndex++] << 16;
	result |= packet[packetIndex++] << 24;

	return result;
}

