#ifndef CUSTOM_MEAS_SYSTEM_INFO_H
#define CUSTOM_MEAS_SYSTEM_INFO_H

// Copyright (c) 2025 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include <stdint.h>
#include "ResponseReader.h"

// a container for assembling and holding the system information read from CustomMeas
class SystemInfo : public ResponseReader
{
public:
	const uint8_t &HardwareID{ m_hardwareID };
	const uint8_t &FirmwareID{ m_firmwareID };
	const uint16_t &VendorID{ m_vendorID };
	const uint16_t &ProductID{ m_productID };
	const uint16_t &VersionID{ m_versionID };
	const uint32_t &FirmwareRevision{ m_firmwareRevision };
	const uint32_t &Unused1{ m_unused0 };
	const uint32_t &GlobalROConfigRawAddr{ m_globalROConfigRawAddr };
	const uint32_t &GlobalRWConfigRawAddr{ m_globalRWConfigRawAddr };
	const uint32_t &GlobalPersistentConfigRawAddr{ m_globalPersistentConfigRawAddr };
	const uint8_t &IsBigEndian{ m_isBigEndian };

	static const uint16_t dataLength = 29;
	void decodeFrom(uint8_t * packet);

protected:
	uint8_t m_hardwareID;
	uint8_t m_firmwareID;
	uint16_t m_vendorID; // should be 0x488
	uint16_t m_productID;  // should be 0x1015
	uint16_t m_versionID;
	uint32_t m_firmwareRevision;
	uint32_t m_unused0;
	uint32_t m_globalROConfigRawAddr;
	uint32_t m_globalRWConfigRawAddr;
	uint32_t m_globalPersistentConfigRawAddr;
	uint8_t m_isBigEndian; // 0x01 native == big-endian, 0x80 = config uses native endianess
};

#endif // CUSTOM_MEAS_SYSTEM_INFO_H
