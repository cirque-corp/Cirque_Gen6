// Copyright (c) 2025 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include "HidReport.h"
#include <cstring>

// *** HidReport ***

// byte stream: start, addr | r, ack, hid length lsb, ack, hid length msb, ack, report id, ack, data0, ack, ... data_n, nak
// host issues nak after reading last byte. Host needs to know how many bytes to read.
// data0..data_n decode based on report id

HidReport::HidReport()
{
    clear();
}

HidReport::~HidReport()
{
    
}

void HidReport::clear(void)
{
    m_report_id = id_unknown;
    m_length = 0;
}

void HidReport::clearReport(void)
{
    memset(&report, 0, sizeof(report));
}

bool HidReport::decodeReport(uint8_t* packet)
{
    bool decoded_okay = decodeLengthAndId(packet);
    switch (m_report_id)
    {
        case id_mouseReport:
            decoded_okay &= decodeMouseReport(packet);
            break;
        case id_ptpReport:
            decoded_okay &= decodePTPReport(packet);
            break;
        case id_keyReport :
            decoded_okay &= decodeKeyboardReport(packet);
            break;
        // case crqAlpsReport:
        // case stickReport:
        default:
            decoded_okay = false;
            [[fallthrough]];
        case id_resetResponse:
            clearReport();
    }
    return decoded_okay;
}

bool HidReport::decodeLengthAndId(uint8_t* packet)
{
    bool result = true;
    m_length = (uint16_t)(packet[1] << 8) + packet[0];
    if (m_length == 0)
    {
        m_report_id = id_resetResponse;
    }
    else
    {
        switch(packet[2])
        {
            case 1:
            case 6:
            case 8:
            case 9:
            case 14:
            case 15:
                m_report_id = (reportIds_t) packet[2];
                break;
            default:
                m_report_id = id_unknown;
                result = false;
        }
    }
    return result;
}

bool HidReport::decodeMouseReport(uint8_t* packet)
{
    if ((m_report_id != id_mouseReport) || (m_length > 8) || (m_length < 7))
    {
        // bad mouse report - exit
    	clearReport();
        return false;
    }

    report.mouse.buttons = packet[3];
    report.mouse.xDelta = packet[4];
    report.mouse.yDelta = packet[5];
    report.mouse.scrollDelta = packet[6];

	// quirk - some projects use 7 byte packets and leave off panDelta 
	report.mouse.panDelta = (m_length == 8) ? packet[7] : 0;
	
	return true;
}

// PTP packets can change a bit depending upon which OS your project is built for. 
// Typically the packets have 3 bytes overhead + 5 bytes per finger + 4 bytes of extra data. 
// There are possible variants here. If sizes aren't what you expect the
// report descriptor will indicate what data was added or removed from the report.

// HID report layout:

// 0, 1 -- report size
// 2    -- report ID

// 3    -- confidence
// 4,5  -- X
// 6,7  -- Y 
//  -- repeat for remaining fingers; confidence, X, Y (5 bytes per finger)

// +0, +1 -- time stamp
// +2   -- total contacts
// +3   -- button mask 

bool HidReport::decodePTPReport(uint8_t* packet)
{
    bool length_okay = false;
    switch (m_length)
    {
        case 3 + (5 * 1) + 4 : // one finger
        case 3 + (5 * 2) + 4 : // two fingers
        case 3 + (5 * 3) + 4 : // three fingers
            report.ptp.numberFingers = (m_length - (3 + 4) ) / 5;
            length_okay = true;
    }
    if ((m_report_id != id_ptpReport) || (!length_okay))
    {
        // bad ptp report - exit
    	clearReport();
        return false;
    }
	// index = 0..1 -- length
    // index = 2 -- id
    int index = 3;
    for (int x = 0; x < report.ptp.numberFingers; x++)
    {
        uint8_t temp = packet[index++];
        report.ptp.fingers[x].confidence = temp & 0x01;   // bit 0
        report.ptp.fingers[x].tip = (temp & 0x02) >> 1;   // bit 1
        report.ptp.fingers[x].contactID = (temp & 0xFC) >> 2; // bits 2..7

        report.ptp.fingers[x].x = (uint16_t) packet[index++];   // low byte
        report.ptp.fingers[x].x |= (uint16_t) packet[index++] << 8;  // high byte

        report.ptp.fingers[x].y = (uint16_t) packet[index++];   // low byte
        report.ptp.fingers[x].y |= (uint16_t) packet[index++] << 8;  // high byte
    }

    report.ptp.timeStamp = (uint16_t) packet[index++];  // low byte
    report.ptp.timeStamp |= (uint16_t ) packet[index++] << 8;  // high byte

    report.ptp.contactCount = packet[index++];
    report.ptp.buttons = packet[index++]; 

	return true;

    // ToDo: make this work with PTP Input Report -Pressure-

    // pressure makes this be single finger per report:
    // 0, 1 = length, 2 = id
    // 3, 4,5, 6,7 = confidence, x, y
    // 8,9 = pressure area
    // 10,11 = time stamp
    // 12 = total contacts
    // 13 = button mask
}

bool HidReport::decodeKeyboardReport(uint8_t* packet)
{
    if ((m_report_id != id_keyReport) || (m_length != 11))
    {
        clearReport();
        return false;
    }

    report.keyboard.modifier1 = packet[3];
    report.keyboard.modifier2 = packet[4];
    report.keyboard.keycode[0] = packet[5];
    report.keyboard.keycode[1] = packet[6];
    report.keyboard.keycode[2] = packet[7];
    report.keyboard.keycode[3] = packet[8];
    report.keyboard.keycode[4] = packet[9];
    report.keyboard.keycode[5] = packet[10];

    return true;
}

