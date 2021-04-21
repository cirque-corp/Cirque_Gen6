// Copyright (c) 2019 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include <Arduino.h>  //for delay
#include <stdbool.h>
#include <stdint.h>

#include "Fletcher32.h"
#include "Borax_BL.h"

static uint8_t dataBuffer[BL_REPORT_LEN];
static bl_read_packet g_packet;

static void ParseReadPacket(bl_read_packet *packet, uint8_t *data);
static bool GetPacket(bl_read_packet *packet, bool retries);
static bool IsBootloaderMode(uint16_t sentinel);
static bool IsImageMode(uint16_t sentinel);
static bool IsSentinelValid(uint16_t sentinel);
static bool CheckStatusAndError(void);
static bool InvokeBootloader(void);
static bool FormatImage(const uint8_t *buf, bl_read_packet *packet);
static bool FormatRegion(
        const uint8_t *buf,
        uint32_t address,
        uint32_t numBytes,
        bl_read_packet *packet);
static bool WriteImage(
        const uint8_t *buf,
        uint32_t address,
        uint32_t numBytes,
        bl_read_packet *packet);
static bool Flush(void);
static bool Validate(void);
static bool Reset(void);

///////////////////////////////////////////////////////////////////////////////
//Private Functions
///////////////////////////////////////////////////////////////////////////////
///
static void ParseReadPacket(bl_read_packet *packet, uint8_t *data)
{
    uint16_t i;

    // Length_LSB = data[0]
    // Length_MSB = data[1]
    // ReportID = data[2]
    packet->Sentinel = data[3] | (data[4] << 8);
    packet->Version = data[5];
    packet->LastError = data[6];
    packet->Flags = data[7];

    //NOTE: This is not backwards compatible with BL versions < 10
    packet->AtomicWriteSize = data[8];
    packet->WriteDelay = data[9];
    packet->FormatDelay = data[10];

    packet->MemAddress =
            (data[11]      ) |
            (data[12] <<  8) |
            (data[13] << 16) |
            (data[14] << 24);

    packet->NumBytes = data[15] | (data[16] << 8);

    for(i = 0; i < (packet->NumBytes <= 514 ? packet->NumBytes : 514); i++)
    {
        packet->Data[i] = data[17 + i];
    }

    packet->Checksum = data[17 + (packet->NumBytes <= 514 ? packet->NumBytes : 514)];
    packet->Checksum |= data[17 + (packet->NumBytes <= 514 ? packet->NumBytes : 514) + 1] << 8;
}

//returns false if Sentinel is invalid
static bool GetPacket(bl_read_packet *packet, bool retries)
{
    uint16_t count;
    bool success = false;

    for (count = 0; count < 50; count++)
    {
        BL_read(dataBuffer);
        ParseReadPacket(packet, dataBuffer);

        if ((packet->Flags & STATUS_BUSY_BIT) == 0)
        {
            if(IsSentinelValid(packet->Sentinel))
            {
                success = true;
            }
            break;
        }
        if (retries == false)
        {
            break;
        }
        delay(10);
    }
    return success;
}


/* NOTE:
 * in legacy code:
 *    0xC35A indicated bootloader
 *    0x5AC3 indicated image (reversing the bytes)
 * in Bootloader ver10
 *    0x6C42 is bootloader ('lB' = 'Bl' with LSByte first)
 *    0x6D49 is image ('mI' = 'Im' with LSByte first)
 *    0x426C is also image (0x6C42 reversed) in rare cases
 */
static bool IsBootloaderMode(uint16_t sentinel)
{
    switch( sentinel )
    {
      case 0x6C42: //'lB' ('Bl' with LSByte first)
      case 0xC35A: // legacy bootloader
        return true;
    }
    return false;
}


static bool IsImageMode(uint16_t sentinel)
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


static bool IsSentinelValid(uint16_t sentinel)
{
    return (IsImageMode(sentinel) || IsBootloaderMode(sentinel));
}


//returns false if Sentinel is invalid or if status shows error
static bool CheckStatusAndError(void)
{
    if ((GetPacket(&g_packet, true) == false) || (g_packet.LastError != NO_ERROR))
    {
        return false;
    }
    return true;
}


static bool InvokeBootloader(void)
{
    BL_cmd_invoke_bootloader();
    delay(100);

    return CheckStatusAndError();
}


static bool FormatImage(const uint8_t *buf, bl_read_packet *packet)
{
    uint32_t EntryPoint;

    EntryPoint = buf[4] | ( buf[5] << 8 ) | ( buf[6] << 16 ) | ( buf[7] << 24 );

    BL_cmd_format_image(0, 1, EntryPoint);
    delay(packet->WriteDelay * 10);

    return CheckStatusAndError();
}


static bool FormatRegion(
        const uint8_t *buf,
        uint32_t address,
        uint32_t numBytes,
        bl_read_packet *packet)
{
    BL_cmd_format_region(0, address, numBytes, Fletcher32(buf, numBytes));
    delay( (packet->FormatDelay * ((numBytes / 1024) + 1)) );

    return CheckStatusAndError();
}


//returns true success, false for error
static bool WriteImage(
        const uint8_t *buf,
        uint32_t address,
        uint32_t numBytes,
        bl_read_packet *packet)
{
    uint32_t Length;
    uint32_t PayloadSize;
    bool success = true;

    Length = numBytes;
    while (Length > 0)
    {
        if (Length > MAX_DATA_PAYLOAD_SIZE)
        {
            PayloadSize = MAX_DATA_PAYLOAD_SIZE;
        }
        else
        {
            PayloadSize = Length;
        }

        BL_cmd_write( address + numBytes - Length, PayloadSize, &buf[numBytes - Length] );

        if (PayloadSize * packet->WriteDelay > 100)
        {
            delay(packet->WriteDelay * PayloadSize / 100);
        }
        else
        {
            delay(1);
        }

        if(CheckStatusAndError() == false)
        {
            success = false;
            break;
        }
        Length -= PayloadSize;
    }
    return success;
}


static bool Flush(void)
{
    BL_cmd_flush();
    delay(10);

    return CheckStatusAndError();
}


static bool Validate(void)
{
    BL_cmd_validate(1);
    delay(100);

    if ((CheckStatusAndError()) && (g_packet.Flags & STATUS_VALID_IMAGE))
    {
        return true;
    }
    return false;
}


static bool Reset(void)
{
    BL_cmd_reset();
    delay(100);

    return CheckStatusAndError();
}

///////////////////////////////////////////////////////////////////////////////
//Public Functions
///////////////////////////////////////////////////////////////////////////////

//returns either ErrorCodes or BLProgramErrors
uint16_t BL_program(const uint8_t *buf, uint32_t numBytes, uint32_t address)
{
    uint16_t error = 0;
    bool reformat = false;

    if (CheckStatusAndError() == false)
    {
        if (Reset() == false)
        {
            error = g_packet.LastError;
        }
    }

    if (!error)
    {
        if (!IsBootloaderMode(g_packet.Sentinel))
        {
            if(InvokeBootloader() == false)
            {
                error = BLProgErr_InvokeBootloader;
            }
        }
    }

    if (!error)
    {
        if(FormatImage(buf, &g_packet) == false)
        {
            error = BLProgErr_FormatImage;
        }
        else if(FormatRegion(buf, address, numBytes, &g_packet) == false)
        {
            error = BLProgErr_FormatRegion;
        }
        else if(WriteImage(buf, address, numBytes, &g_packet) == false)
        {
            error = BLProgErr_WriteImage;
            reformat = true;
        }
        else if (Flush() == false)
        {
            error = BLProgErr_Flush;
            reformat = true;
        }
        else if (Validate() == false)
        {
            error = BLProgErr_Validate;
            reformat = true;
        }
    }

    if (reformat)
    {
        FormatImage(buf, &g_packet);
    }

    if (Reset() == false)
    {
        error = BLProgErr_Reset;
    }

    return error;
}


//returns number of bytes read
uint16_t BL_cmd_read_memory(uint32_t offset, uint16_t numBytes, uint8_t *data)
{
    uint16_t i;
    uint16_t count;
    bl_read_packet packet;
    bool success;

    BL_request_read(offset, numBytes);
    success = GetPacket(&packet, true);

    if (!success)
    {
        return 0;
    }

    count = numBytes;
    if (numBytes > packet.NumBytes)
    {
        count = packet.NumBytes;
    }
    for (i = 0; i < count; i++)
    {
        data[i] = packet.Data[i];
    }

    return count;
}


bool BL_get_status(bl_read_packet *packet)
{
    return GetPacket(packet, true);
}


