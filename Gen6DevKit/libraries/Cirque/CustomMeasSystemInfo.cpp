// Copyright (c) 2025 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include "CustomMeasSystemInfo.h"

void SystemInfo::decodeFrom(uint8_t * packet)
{
	uint16_t index = 0;

	m_hardwareID = next8bits(packet, index);
	m_firmwareID = next8bits(packet, index);
	m_vendorID = next16bits(packet, index);
	m_productID = next16bits(packet, index);
	m_versionID = next16bits(packet, index);
	m_firmwareRevision = next32bits(packet, index);
	m_unused0 = next32bits(packet, index);
	m_globalROConfigRawAddr = next32bits(packet, index);
	m_globalRWConfigRawAddr = next32bits(packet, index);
	m_globalPersistentConfigRawAddr = next32bits(packet, index);
	m_isBigEndian = next8bits(packet, index);
}
