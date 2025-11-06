#include "CustomMeas.h"

// Copyright (c) 2025 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#define CUSTOMMEAS_I2CADDRESS 0x2c

#define GROUP_INFO_ADDR (0x51200000)
#define GROUP_INFO_INC (0x00001000)

#define MEAS_INFO_ADDR (0x51100000)
#define MEAS_INFO_INC (0x00001000)

#define NOISE_CONFIG_ADDR (0x51500000)
#define NOISE_CONFIG_INC (0x00001000)

#define SYS_INFO_ADDR (0x20000000)
#define RO_NON_PERSIST (0x00000800)
#define POST_HEADER_ADDR ((SYS_INFO_ADDR | RO_NON_PERSIST) + 8)

#define GLOBAL_INFO_ADDR (0x51000000)

CustomMeas::CustomMeas(uint16_t maxBufferLength) : I2cHidApi(CUSTOMMEAS_I2CADDRESS, maxBufferLength)
{

}

CustomMeas::~CustomMeas()
{

}

CustomMeas::commandErrors CustomMeas::ReadSystemInfo(SystemInfo &systemInfo)
{
	uint8_t buffer[systemInfo.dataLength];

	commandErrors status = readExtendedMemory(POST_HEADER_ADDR, buffer, systemInfo.dataLength);
	systemInfo.decodeFrom(buffer);
	return status;
}

CustomMeas::commandErrors CustomMeas::ReadGlobalInfo(GlobalInfo_t *config)
{
	return readExtendedMemory(GLOBAL_INFO_ADDR, (uint8_t *)config, sizeof(GlobalInfo_t));
}

void CustomMeas::WriteGlobalInfo(const GlobalInfo_t *config)
{
	writeExtendedMemory(GLOBAL_INFO_ADDR, (uint8_t *)config, sizeof(GlobalInfo_t));
}

CustomMeas::commandErrors CustomMeas::SetFrameMillis(uint16_t millis)
{
	GlobalInfo_t globalInfo;
	// read, modify, write
	commandErrors status = ReadGlobalInfo(&globalInfo);
	if (status == cmd_okay) 
	{
		globalInfo.FrameMillisLSB = (uint8_t)millis;
		globalInfo.FrameMillisMSB = (uint8_t)(millis >> 8);
		WriteGlobalInfo(&globalInfo);
	}
	return status;
}

CustomMeas::commandErrors CustomMeas::StartMeas(void)
{
	GlobalInfo_t globalInfo;
	// read, modify, write
	commandErrors status = ReadGlobalInfo(&globalInfo);
	if (status == cmd_okay)
	{
		globalInfo.Enable = 1;
		WriteGlobalInfo(&globalInfo);
	}
	return status;
}

CustomMeas::commandErrors CustomMeas::StopMeas(void)
{
	GlobalInfo_t globalInfo;
	// read, modify, write
	commandErrors status = ReadGlobalInfo(&globalInfo);
	if (status == cmd_okay)
	{
		globalInfo.Enable = 0;
		WriteGlobalInfo(&globalInfo);
	}
	return status;
}

CustomMeas::commandErrors CustomMeas::ReadGroupInfo(uint8_t groupIndex, GroupInfo_t * groupInfo)
{
	uint32_t address = (uint32_t)(GROUP_INFO_ADDR + (groupIndex * GROUP_INFO_INC));
	return readExtendedMemory(address, (uint8_t *)groupInfo, sizeof(GroupInfo_t));
}

CustomMeas::commandErrors CustomMeas::WriteGroupInfo(uint8_t groupIndex, GroupInfo_t * groupInfo)
{
	uint32_t address = (uint32_t)(GROUP_INFO_ADDR + (groupIndex * GROUP_INFO_INC));
	writeExtendedMemory(address, (uint8_t *)groupInfo, (uint8_t)sizeof(GroupInfo_t));
	return commandErrors::cmd_okay;
}

CustomMeas::commandErrors CustomMeas::Calibrate(uint8_t groupIndex)
{
	if (groupIndex > MAX_NUMBER_GROUPS)
		return commandErrors::cmd_parameterBad;

	GroupInfo_t groupInfo;
	commandErrors status;

	// read, modify, write
	status = ReadGroupInfo(groupIndex, &groupInfo);
	if (status == cmd_okay)
	{
		groupInfo.Calibration |= 0x40; // Set bit 6
		status = WriteGroupInfo(groupIndex, &groupInfo);
	}
	return status;
}

CustomMeas::commandErrors CustomMeas::CalibrateAll(void)
{
	commandErrors status;

	for (uint8_t i = 0; i < MAX_NUMBER_GROUPS; i++)
	{
		status = Calibrate(i);
		if (status != cmd_okay) { break; }
	}
	return status;
}

CustomMeas::commandErrors CustomMeas::EnableCalibration(uint8_t groupIndex)
{
	if (groupIndex > MAX_NUMBER_GROUPS)
		return commandErrors::cmd_parameterBad;

	GroupInfo_t groupInfo;
	commandErrors status;

	// read, modify, write
	status = ReadGroupInfo(groupIndex, &groupInfo);
	if (status == cmd_okay)
	{
		groupInfo.Calibration |= 0x80; // Set bit 7
		status = WriteGroupInfo(groupIndex, &groupInfo);
	}
	return status;
}

CustomMeas::commandErrors CustomMeas::EnableAllCalibration(void)
{
	commandErrors status;

	for (uint8_t i = 0; i < MAX_NUMBER_GROUPS; i++)
	{
		status = EnableCalibration(i);
		if (status != cmd_okay) { break; }
	}
	return status;
}

CustomMeas::commandErrors CustomMeas::DisableCalibration(uint8_t groupIndex)
{
	if (groupIndex > MAX_NUMBER_GROUPS)
		return commandErrors::cmd_parameterBad;

	GroupInfo_t temp;
	commandErrors status;

	// read, modify, write
	status = ReadGroupInfo(groupIndex, &temp);
	if (status == cmd_okay)
	{
		temp.Calibration &= (uint8_t)~0x80; // Clear bit 7
		status = WriteGroupInfo(groupIndex, &temp);
	}
	return status;
}

CustomMeas::commandErrors CustomMeas::DisableAllCalibration(void)
{
	commandErrors status;

	for (uint8_t i = 0; i < MAX_NUMBER_GROUPS; i++)
	{
		status = DisableCalibration(i);
		if (status != cmd_okay) { break; }
	}
	return status;
}

CustomMeas::commandErrors CustomMeas::Persist(void)
{
	GlobalInfo_t globalInfo;

	// read, modify, write
	commandErrors status = ReadGlobalInfo(&globalInfo);
	if (status == cmd_okay)
	{
		globalInfo.Persist = 1;
		WriteGlobalInfo(&globalInfo);

		// this read event will be clock stretched until the flash writing is complete
		status = ReadGlobalInfo(&globalInfo);
		if (globalInfo.Persist != 0)
		{
			// bit didn't clear - signal an error
			status = commandErrors::cmd_parameterBad;
		}
		// it may have taken 10 to 20 msec to complete that read
	}
	return status;
}

CustomMeas::commandErrors CustomMeas::Restore(void)
{
	GlobalInfo_t globalInfo;

	// read, modify, write
	commandErrors status = ReadGlobalInfo(&globalInfo);
	if (status == cmd_okay)
	{
		globalInfo.Restore = 1;
		WriteGlobalInfo(&globalInfo);
	}
	return status;
}

CustomMeas::commandErrors CustomMeas::POR_StartMeas(void)
{
	GlobalInfo_t globalInfo;
	// read, modify, write
	commandErrors status = ReadGlobalInfo(&globalInfo);
	if (status == cmd_okay)
	{
		globalInfo.POR_Enable = 1;
		WriteGlobalInfo(&globalInfo);
	}
	return status;
}

CustomMeas::commandErrors CustomMeas::POR_StopMeas(void)
{
	GlobalInfo_t globalInfo;
	// read, modify, write
	commandErrors status = ReadGlobalInfo(&globalInfo);
	if (status == cmd_okay)
	{
		globalInfo.POR_Enable = 0;
		WriteGlobalInfo(&globalInfo);
	}
	return status;
}

CustomMeas::commandErrors CustomMeas::LowPowerMode(void)
{
	GlobalInfo_t globalInfo;
	// read, modify, write
	commandErrors status = ReadGlobalInfo(&globalInfo);
	if (status == cmd_okay)
	{
		globalInfo.LowPowerMode = 1;
		WriteGlobalInfo(&globalInfo);
	}
	return status;
}

CustomMeas::commandErrors CustomMeas::NormalPowerMode(void)
{
	GlobalInfo_t globalInfo;
	// read, modify, write
	commandErrors status = ReadGlobalInfo(&globalInfo);
	if (status == cmd_okay)
	{
		globalInfo.LowPowerMode = 0;
		WriteGlobalInfo(&globalInfo);
	}
	return status;
}

CustomMeas::commandErrors CustomMeas::ReadMeasInfo(uint8_t index, MeasInfo_t *measInfo)
{
	if (index > MAX_NUMBER_MEASUREMENTS)
		return commandErrors::cmd_parameterBad;

	uint32_t address = (uint32_t)(MEAS_INFO_ADDR + (index * MEAS_INFO_INC));
	commandErrors status = readExtendedMemory(address, (uint8_t *)measInfo, sizeof(MeasInfo_t));
	return status;
}

CustomMeas::commandErrors CustomMeas::WriteMeasInfo(uint8_t index, const MeasInfo_t *measInfo)
{
	if (index > MAX_NUMBER_MEASUREMENTS)
		return commandErrors::cmd_parameterBad;

	uint32_t address = (uint32_t)(MEAS_INFO_ADDR + (index * MEAS_INFO_INC));
	writeExtendedMemory(address, (uint8_t *)measInfo, sizeof(MeasInfo_t));
	return cmd_okay;
}

CustomMeas::commandErrors CustomMeas::ReadNoiseConfig(uint8_t index, NoiseConfig_t *config)
{
	// ??? what is the range on this index?

	uint32_t address = (uint32_t)(NOISE_CONFIG_ADDR + (index * NOISE_CONFIG_INC));
	return readExtendedMemory(address, (uint8_t *)config, sizeof(NoiseConfig_t));
}

CustomMeas::commandErrors CustomMeas::WriteNoiseConfig(uint8_t index, const NoiseConfig_t *config)
{
	// ??? what is the range on this index?

	uint32_t address = (uint32_t)(NOISE_CONFIG_ADDR + (index * NOISE_CONFIG_INC));
	writeExtendedMemory(address, (uint8_t *)config, sizeof(NoiseConfig_t));
	return cmd_okay;
}

reportIds_t CustomMeas::getMeasReport(int16_t * measArray, uint16_t &measCount)
{
	int16_t measReportBuffer[20 * 16];
	measCount = 0;

	// this does a two reads, the first read is the header
	reportIds_t result = id_unknown;
	uint16_t reportSize = 5; // buffer is maximum size of hid report
	uint16_t readCount = m_host_bus->read(m_i2cAddress, reportSize); // reads maximum size for the report
	if (readCount >= reportSize) // read all bytes
	{
		for (int x = 0; x < reportSize; x++)
		{
			hidReportBuffer[x] = m_host_bus->fetch();
		}
		result = (reportIds_t)hidReportBuffer[2]; // HID ID
	}

	if (result == id_customMeas)
	{
		// the header bytes show this is a custommeas report
		// read the report, read the measurement data
		uint16_t measByteCount = hidReportBuffer[3] + (hidReportBuffer[4] << 8);
		measCount = measByteCount / 2;
		reportSize = 5 + measByteCount;
		uint16_t readCount = m_host_bus->read(m_i2cAddress, reportSize);
		uint16_t fetchLimit = (readCount >= reportSize) ? reportSize : readCount;
		for (int x = 0; x < fetchLimit; x++)
		{
			measReportBuffer[x] = m_host_bus->fetch();
		}
		uint16_t index = 5;
		// compute read limit to avoid overflowing buffer
		uint16_t readLimit = (readCount > 5) ? readCount - 1 : 0;
		for (uint16_t x = 0; x < measCount; x++)
		{
			if (index < readLimit)
			{
				measArray[x] = measReportBuffer[index] + (measReportBuffer[index + 1] << 8);
			}
			index += 2;
		}
	}
	// else - it was some other report, you should call getReport() to read the packet

	return result;
}

