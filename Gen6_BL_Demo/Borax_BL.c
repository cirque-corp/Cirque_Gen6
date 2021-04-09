// Copyright (c) 2019 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include "Borax_BL.h"
#include "I2C.h"
#include "Fletcher32.h"
#include "Fletcher16.h"
#include <Arduino.h>

#define BL_REPORT_CMD_REG  (0x0005)
#define BL_REPORT_SET_FEAT (0x0337)
#define BL_REPORT_GET_FEAT (0x0237)
#define BL_REPORT_DATA_REG (0x0006)
#define BL_REPORT_LEN (0x0215)  // Always 533


#define CMD_WRITE     (0x00)
#define CMD_FLUSH     (0x01)
#define CMD_VALIDATE  (0x02)
#define CMD_RESET     (0x03)
#define CMD_FORMAT_IMG  (0x04)
#define CMD_FORMAT_REG  (0x05)
#define CMD_INVOKE_BL   (0x06)
#define CMD_WRITE_MEM   (0x07)
#define CMD_READ_MEM    (0x08)

#define DUMMY_BYTE  (0xDB)

// must be even, better if multiple of 4 (520 / 4 = 130)
#define MAX_DATA_PAYLOAD_SIZE 520

static uint8_t _i2cSlaveAddr;
static uint8_t _dataBuffer[533];

static bool is_bootloader_mode(uint16_t sentinel);
static bool is_image_mode(uint16_t sentinel);
static bool is_sentinel_valid(uint16_t sentinel);
static void read_status(bl_status_t *statusPtr);

/**** Private Functions  ****/
static void I2C_write_16(uint16_t data)
{
    I2C_write((uint8_t)(data >> 0));
    I2C_write((uint8_t)(data >> 8));
}

static void I2C_write_32(uint32_t data)
{
    I2C_write((uint8_t)(data >> 0));
    I2C_write((uint8_t)(data >> 8));
    I2C_write((uint8_t)(data >> 16));
    I2C_write((uint8_t)(data >> 24));
}

static void I2C_write_SetFeatureReport(uint8_t cmd)
{
    I2C_write_16(BL_REPORT_CMD_REG);
    I2C_write_16(BL_REPORT_SET_FEAT);
    I2C_write_16(BL_REPORT_DATA_REG);
    I2C_write_16(BL_REPORT_LEN);
    I2C_write(BL_REPORT_ID);
    I2C_write(cmd);
}

static void I2C_write_GetFeatureReport(void)
{
    I2C_write_16(BL_REPORT_CMD_REG);
    I2C_write_16(BL_REPORT_GET_FEAT);
    I2C_write_16(BL_REPORT_DATA_REG);
}

static void I2C_write_DummyBytes(uint16_t count)
{
    uint16_t i;

    for(i = 0; i < count; i++)
    {
        I2C_write(DUMMY_BYTE);
    }
}

/**** Public Functions  ****/
void BL_init(uint8_t i2cSlaveAddr)
{
    _i2cSlaveAddr = i2cSlaveAddr;
}

void BL_cmd_write(uint32_t offset, uint32_t numBytes, const uint8_t * dataPtr)
{
    uint32_t i;

    I2C_beginTransmission(_i2cSlaveAddr);
    I2C_write_SetFeatureReport(CMD_WRITE);
    I2C_write_32(offset);
    I2C_write_32(numBytes);

    for(i = 0; i < numBytes; i++)
    {
        I2C_write(dataPtr[i]);
    }

    I2C_write_DummyBytes(521 - numBytes);
    I2C_endTransmission(true);
}

void BL_cmd_flush(void)
{
      I2C_beginTransmission(_i2cSlaveAddr);
      I2C_write_SetFeatureReport(CMD_FLUSH);
      I2C_write_DummyBytes(530 - 1);
      I2C_endTransmission(true);
}

void BL_cmd_validate(uint8_t validationType)
{
    I2C_beginTransmission(_i2cSlaveAddr);
    I2C_write_SetFeatureReport(CMD_VALIDATE);
    I2C_write(validationType);
    I2C_write_DummyBytes(530 - 2);
    I2C_endTransmission(true);
}

void BL_cmd_reset(void)
{
    I2C_beginTransmission(_i2cSlaveAddr);
    I2C_write_SetFeatureReport(CMD_RESET);
    I2C_write_DummyBytes(530 - 1);
    I2C_endTransmission(true);
}

void BL_cmd_format_image(uint8_t imageType, uint8_t numRegions, uint32_t entryPoint, uint16_t hidDescriptorAddress, uint8_t i2cAddress, uint8_t reportID)
{
    I2C_beginTransmission(_i2cSlaveAddr);
    I2C_write_SetFeatureReport(CMD_FORMAT_IMG);
    I2C_write(imageType);
    I2C_write(numRegions);
    I2C_write_32(entryPoint);
    I2C_write_16(hidDescriptorAddress);
    I2C_write(i2cAddress);
    I2C_write(reportID);
    I2C_write_DummyBytes(530 - 11);
    I2C_endTransmission(true);
}

void BL_cmd_format_region(uint8_t regionNumber, uint32_t regionOffset, uint32_t regionSize, uint32_t regionChecksum)
{
    I2C_beginTransmission(_i2cSlaveAddr);
    I2C_write_SetFeatureReport(CMD_FORMAT_REG);
    I2C_write(regionNumber);
    I2C_write_32(regionOffset);
    I2C_write_32(regionSize);
    I2C_write_32(regionChecksum);
    I2C_write_DummyBytes(530 - 14);
    I2C_endTransmission(true);
}

void BL_cmd_invoke_bootloader(void)
{
    I2C_beginTransmission(_i2cSlaveAddr);
    I2C_write_SetFeatureReport(CMD_INVOKE_BL);
    I2C_write_DummyBytes(530 - 1);
    I2C_endTransmission(true);
}

void BL_cmd_write_memory(uint32_t offset, uint16_t numBytes, uint8_t * dataPtr)
{
    uint32_t i;
    uint16_t checksum;
    uint8_t preamble[7] =
    {
        CMD_WRITE_MEM,
        (uint8_t)(offset >> 0),
        (uint8_t)(offset >> 8),
        (uint8_t)(offset >> 16),
        (uint8_t)(offset >> 14),
        (uint8_t)(numBytes >> 0),
        (uint8_t)(numBytes >> 8)
    };

    I2C_beginTransmission(_i2cSlaveAddr);
    I2C_write_SetFeatureReport(CMD_WRITE_MEM);
    I2C_write_32(offset);
    I2C_write_16(numBytes);

    for(i = 0; i < numBytes; i++)
    {
        I2C_write(dataPtr[i]);
    }

    checksum = Fletcher16(preamble, 7);
    checksum = Fletcher16_Continue(dataPtr, numBytes, checksum);
    I2C_write_16(checksum);
    I2C_write_DummyBytes(521 - numBytes);
    I2C_endTransmission(true);
}

void BL_cmd_read_memory(uint32_t offset, uint16_t numBytes, uint8_t *dataPtr)
{
    uint16_t i;
    uint16_t start;

    I2C_beginTransmission(_i2cSlaveAddr);
    I2C_write_SetFeatureReport(CMD_READ_MEM);
    I2C_write_32(offset);
    I2C_write_16(numBytes);
    I2C_write_DummyBytes(523);
    I2C_endTransmission(true);
    I2C_beginTransmission(_i2cSlaveAddr);
    I2C_write_GetFeatureReport();
    I2C_endTransmission(false);
    I2C_request(_i2cSlaveAddr, 533, true);

    i = 0;
    while(I2C_available())
    {
        _dataBuffer[i++] = I2C_read();
    }

    start = (_dataBuffer[5] >= 8) ? 17 : 13;
    for(i = 0; i < numBytes; i++)
    {
        dataPtr[i] = _dataBuffer[start + i];
    }
}

static void read_status(bl_status_t *statusPtr)
{
    uint16_t i;
    I2C_beginTransmission(_i2cSlaveAddr);
    I2C_write_GetFeatureReport();
    I2C_endTransmission(false);
    I2C_request(_i2cSlaveAddr, 533, true);
    
    i = 0;
    while(I2C_available())
    {
        _dataBuffer[i++] = I2C_read();
    }

    // Length_LSB = _dataBuffer[0]
    // Length_MSB = _dataBuffer[1]
    // ReportID = _dataBuffer[2]
    statusPtr->Sentinel = _dataBuffer[3] | (_dataBuffer[4] << 8);
    statusPtr->Version = _dataBuffer[5];
    statusPtr->LastError = _dataBuffer[6];
    statusPtr->Flags = _dataBuffer[7];

    if(statusPtr->Version >= 8)
    {
        statusPtr->AtomicWriteSize = _dataBuffer[8];
        statusPtr->WriteDelay = _dataBuffer[9];
        statusPtr->FormatDelay = _dataBuffer[10];

        statusPtr->MemAddress =
                (_dataBuffer[11]      ) |
                (_dataBuffer[12] <<  8) |
                (_dataBuffer[13] << 16) |
                (_dataBuffer[14] << 24);

        statusPtr->NumBytes = _dataBuffer[15] | (_dataBuffer[16] << 8);

        for(i = 0; i < (statusPtr->NumBytes <= 514 ? statusPtr->NumBytes : 514); i++)
        {
            statusPtr->Data[i] = _dataBuffer[17 + i];
        }

        statusPtr->Checksum = _dataBuffer[17 + (statusPtr->NumBytes <= 514 ? statusPtr->NumBytes : 514)];
        statusPtr->Checksum |= _dataBuffer[17 + (statusPtr->NumBytes <= 514 ? statusPtr->NumBytes : 514) + 1] << 8;
    }
    else
    {
        statusPtr->MemAddress =
            (_dataBuffer[8]       ) |
            (_dataBuffer[9]  <<  8) |
            (_dataBuffer[10] << 16) |
            (_dataBuffer[11] << 24);

        statusPtr->NumBytes = _dataBuffer[12] | (_dataBuffer[13] << 8);

        for(i = 0; i < (statusPtr->NumBytes <= 517 ? statusPtr->NumBytes : 517); i++)
        {
            statusPtr->Data[i] = _dataBuffer[14 + i];
        }

        statusPtr->Checksum = _dataBuffer[14 + (statusPtr->NumBytes <= 517 ? statusPtr->NumBytes : 517)];
        statusPtr->Checksum |= _dataBuffer[14 + (statusPtr->NumBytes <= 517 ? statusPtr->NumBytes : 517) + 1] << 8;
    }
}


bool BL_get_status(bl_status_t *status, bool retries)
{
    uint16_t count;

    read_status(status);
    if(!is_sentinel_valid( status->Sentinel))
    {
        return false;
    }

    count = 0;
    while (status->Flags & STATUS_BUSY_BIT)
    {
        if(count++ > 50)
        {
            return false;
        }
        delay(10);

        read_status(status);
        if(!is_sentinel_valid(status->Sentinel))
        {
            return false;
        }
    }
    return true;
}

/* NOTE:
 * in legacy code:
 *    0xC35A indicated bootloader
 *    0x5AC3 indicated image (reversing the bytes
 * in Bootloader ver10
 *    0x6C42 is bootloader ('lB' = 'Bl' with LSByte first)
 *    0x6D49 is image ('mI' = 'Im' with LSByte first)
 *    0x426C is also image (0x6C42 reversed) in rare cases
 */
static bool is_bootloader_mode(uint16_t sentinel)
{
    switch( sentinel )
    {
      case 0x6C42: //'lB' ('Bl' with LSByte first)
      case 0xC35A: // legacy bootloader
        return true;
    }
    return false;
}


static bool is_image_mode(uint16_t sentinel)
{
    switch( sentinel )
    {
        case 0x5AC3: // legacy image (0xC35A reversed)
        case 0x6D49: //'mI' ('Im' with LSByte first)
        case 0x426C: //'Bl' legacy (0x6C42 reversed)
        return true;
    }
    return false;
}


static bool is_sentinel_valid(uint16_t sentinel)
{
    return (is_image_mode(sentinel) || is_bootloader_mode(sentinel));
}

static void format_image(const uint8_t *buf, bl_status_t *status)
{

    uint32_t PageWriteDelay;
    uint8_t TargetI2CAddress;
    uint16_t TargetHIDDescAddr;
    uint32_t EntryPoint;

    if(status->Version >= 0x08)
    {
        PageWriteDelay = status->WriteDelay * 10;
    }
    else
    {
        PageWriteDelay = 100;
    }

    EntryPoint = buf[4] | ( buf[5] << 8 ) | ( buf[6] << 16 ) | ( buf[7] << 24 );

    if( status->Version >= 0x09)
    {
        TargetI2CAddress = 0xFF;
        TargetHIDDescAddr = 0xFFFF;
    }
    else
    {
        TargetI2CAddress = 0x2C;
        TargetHIDDescAddr = 0x0020;
    }

    BL_cmd_format_image( 0, 1, EntryPoint, TargetHIDDescAddr, TargetI2CAddress, BL_REPORT_ID );
    delay(PageWriteDelay);
}


static void format_region(
        const uint8_t *buf,
        uint32_t address,
        uint32_t numBytes,
        bl_status_t *status)
{
    uint32_t FormatRegionsPageDelay;

    if(status->Version >= 0x08)
    {
        FormatRegionsPageDelay = status->FormatDelay;
    }
    else
    {
        FormatRegionsPageDelay = 50;
    }

    BL_cmd_format_region(0, address, numBytes, Fletcher32(buf, numBytes));
    delay( (FormatRegionsPageDelay * ((numBytes / 1024) + 1)) );
}


bool write_image(
        const uint8_t *buf,
        uint32_t address,
        uint32_t numBytes,
        bl_status_t *status)
{
    uint32_t Length;
    uint32_t PayloadSize;
    uint32_t PageWriteDelay;
    bool error = false;

    if(status->Version >= 0x08)
    {
        PageWriteDelay = status->WriteDelay * 10;
    }
    else
    {
        PageWriteDelay = 100;
    }

    Length = numBytes;
    while (Length > 0)
    {
        PayloadSize = ( Length > MAX_DATA_PAYLOAD_SIZE ) ? MAX_DATA_PAYLOAD_SIZE : Length;

        BL_cmd_write( address + numBytes - Length, PayloadSize, &buf[numBytes - Length] );

        delay( ( PageWriteDelay * PayloadSize > 1000 ) ? PageWriteDelay * PayloadSize / 1000 : 1 );

        if ((BL_get_status(status, true) == false) ||
            (status->LastError != NO_ERROR))
        {
            error = true;
            break;
        }
        Length -= PayloadSize;
    }
    return error;
}


bool BL_program(const uint8_t *buf, uint32_t numBytes, uint32_t address)
{

    bl_status_t status;
    bool error = false;
    bool reformat = false;

    if (BL_get_status(&status, true) == false)
    {
        error = true;
    }

    if (!error)
    {
        if( status.LastError != NO_ERROR )
        {
            BL_cmd_reset();
            delay(100);

            if ((BL_get_status(&status, true) == false) || (status.LastError != NO_ERROR))
            {
                error = true;
            }
        }
    }

    if (!error)
    {
        if (!is_bootloader_mode(status.Sentinel))
        {
            BL_cmd_invoke_bootloader();
            delay(100);

            if (BL_get_status(&status, true) == false)
            {
                error = true;
            }
        }
    }

    if (!error)
    {
        format_image(buf, &status);
        if ((BL_get_status(&status, true) == false) || (status.LastError != NO_ERROR))
        {
            error = true;
        }
    }

    if (!error)
    {
        format_region(buf, address, numBytes, &status);
        if ((BL_get_status(&status, true) == false) || (status.LastError != NO_ERROR))
        {
            error = true;
        }
    }

    if (!error)
    {
        error = write_image(buf, address, numBytes, &status);
        if (error)
        {
            reformat = true;
        }
    }
    if (!error)
    {
        BL_cmd_flush();
        delay(10);

        if ((BL_get_status(&status, true) == false) || (status.LastError != NO_ERROR))
        {
            error = true;
            reformat = true;
        }
    }

    if (!error)
    {
        BL_cmd_validate(1);
        delay(100);

        if ((BL_get_status(&status, true) == false) ||
            (status.LastError != NO_ERROR)          ||
            ((status.Flags & STATUS_VALID_IMAGE) == 0))
        {
            error = true;
            reformat = true;
        }
    }

    if (error && reformat)
    {
        format_image(buf, &status);
    }

    BL_cmd_reset();
    delay(10);

    if ((BL_get_status(&status, true) == false) || (status.LastError != NO_ERROR))
    {
        return false;
    }

    return true;
}
