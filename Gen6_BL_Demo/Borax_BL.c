// Copyright (c) 2019 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include <stdbool.h>
#include <stdint.h>

#include "Fletcher32.h"
#include "Borax_BL_I2C_Commands.h"
#include "Borax_BL.h"
//#include "I2C.h"

#include <Arduino.h>

static uint8_t dataBuffer[BL_REPORT_LEN];

static bool is_bootloader_mode(uint16_t sentinel);
static bool is_image_mode(uint16_t sentinel);
static bool is_sentinel_valid(uint16_t sentinel);

static void parse_read_packet(bl_read_packet *packet, uint8_t *data)
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
static bool BL_get_packet(bl_read_packet *packet, bool retries)
{
    uint16_t count;

    BL_read(dataBuffer);
    parse_read_packet(packet, dataBuffer);

    if(!is_sentinel_valid( packet->Sentinel))
    {
        return false;
    }

    count = 0;
    while (packet->Flags & STATUS_BUSY_BIT)
    {
        if(count++ > 50)
        {
            return false;
        }
        delay(10);

        BL_read(dataBuffer);
        parse_read_packet(packet, dataBuffer);
        if(!is_sentinel_valid(packet->Sentinel))
        {
            return false;
        }
    }
    return true;
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

static void format_image(const uint8_t *buf, bl_read_packet *packet)
{

    uint32_t PageWriteDelay;
    uint8_t TargetI2CAddress;
    uint16_t TargetHIDDescAddr;
    uint32_t EntryPoint;

    if(packet->Version >= 0x08)
    {
        PageWriteDelay = packet->WriteDelay * 10;
    }
    else
    {
        PageWriteDelay = 100;
    }

    EntryPoint = buf[4] | ( buf[5] << 8 ) | ( buf[6] << 16 ) | ( buf[7] << 24 );

    if( packet->Version >= 0x09)
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
        bl_read_packet *packet)
{
    BL_cmd_format_region(0, address, numBytes, Fletcher32(buf, numBytes));
    delay( (packet->FormatDelay * ((numBytes / 1024) + 1)) );
}


bool write_image(
        const uint8_t *buf,
        uint32_t address,
        uint32_t numBytes,
        bl_read_packet *packet)
{
    uint32_t Length;
    uint32_t PayloadSize;
    bool error = false;

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

        if ((BL_get_status(packet) == false) ||
            (packet->LastError != NO_ERROR))
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

    bl_read_packet packet;
    bool error = false;
    bool reformat = false;

    if (BL_get_status(&packet) == false)
    {
        error = true;
    }

    if (!error)
    {
        if( packet.LastError != NO_ERROR )
        {
            BL_cmd_reset();
            delay(100);

            if ((BL_get_status(&packet) == false) || (packet.LastError != NO_ERROR))
            {
                error = true;
            }
        }
    }

    if (!error)
    {
        if (!is_bootloader_mode(packet.Sentinel))
        {
            BL_cmd_invoke_bootloader();
            delay(100);

            if (BL_get_status(&packet) == false)
            {
                error = true;
            }
        }
    }

    if (!error)
    {
        format_image(buf, &packet);
        if ((BL_get_status(&packet) == false) || (packet.LastError != NO_ERROR))
        {
            error = true;
        }
    }

    if (!error)
    {
        format_region(buf, address, numBytes, &packet);
        if ((BL_get_status(&packet) == false) || (packet.LastError != NO_ERROR))
        {
            error = true;
        }
    }

    if (!error)
    {
        error = write_image(buf, address, numBytes, &packet);
        if (error)
        {
            reformat = true;
        }
    }
    if (!error)
    {
        BL_cmd_flush();
        delay(10);

        if ((BL_get_status(&packet) == false) || (packet.LastError != NO_ERROR))
        {
            error = true;
            reformat = true;
        }
    }

    if (!error)
    {
        BL_cmd_validate(1);
        delay(100);

        if ((BL_get_status(&packet) == false) ||
            (packet.LastError != NO_ERROR)          ||
            ((packet.Flags & STATUS_VALID_IMAGE) == 0))
        {
            error = true;
            reformat = true;
        }
    }

    if (error && reformat)
    {
        format_image(buf, &packet);
    }

    BL_cmd_reset();
    delay(10);

    if ((BL_get_status(&packet) == false) || (packet.LastError != NO_ERROR))
    {
        return false;
    }

    return true;
}


//returns number of bytes read
uint16_t BL_cmd_read_memory(uint32_t offset, uint16_t numBytes, uint8_t *data)
{
    uint16_t i;
    uint16_t count;
    bl_read_packet packet;
    bool error;

    BL_request_read(offset, numBytes);
    error = BL_get_packet(&packet, true);

    if (error)
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
    return BL_get_packet(packet, true);
}


