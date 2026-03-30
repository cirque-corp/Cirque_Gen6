// Copyright (c) 2026 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include "HID_Reports.h"

// Your Project_Config can redefing this if needed:
#ifndef PROJECT_MAX_PACKET_SIZE
	// Cirque absolute mode packets are 53 bytes
	// Alps absolute mode packets are 30 bytes
	// Assume Cirque mode if you didn't specify
	#define PROJECT_MAX_PACKET_SIZE 53
#endif

// Common report array element index values (array index number)
// HID Header
#define LENGTH_LOWBYTE    0
#define LENGTH_HIGHBYTE   1
#define REPORT_ID         2

/** Clears all values of report to zero. PTP is largest of union*/
void clearReport(HID_report_t* report)
{
    uint8_t i;

    report->reportID = 0;
    report->reportLength = 0;
    //Clears the values of the largest member of the Union.
    report->ptp.x = 0;
    report->ptp.y = 0;
    report->ptp.timeStamp = 0;
    report->ptp.contactID = 0;
    report->ptp.confidence = 0;
    report->ptp.tip = 0;
    report->ptp.decodedContactSlots = 0;
    for(i = 0; i < PTP_MAX_CONTACTS; i++)
    {
        report->ptp.contacts[i].x = 0;
        report->ptp.contacts[i].y = 0;
        report->ptp.contacts[i].contactID = 0;
        report->ptp.contacts[i].confidence = 0;
        report->ptp.contacts[i].tip = 0;
    }
    report->ptp.contactCount = 0;
    report->ptp.buttons = 0;
}

// ----------------------------------------------
// HID Decode and Query Functions

uint16_t HID_reportLength(uint8_t * packet)
{
  return packet[LENGTH_LOWBYTE] | (packet[LENGTH_HIGHBYTE] << 8);
}

uint8_t HID_reportID(uint8_t * packet)
{
  return packet[REPORT_ID];
}

bool HID_decodeReport(uint8_t* packet, HID_report_t* result)
{
	bool decoded_ok = false;
	
    //determine which type of report it is and decode it
	result->reportLength = HID_reportLength(packet);
  result->reportID = HID_reportID(packet);
  switch(result->reportID)
  {
    case PTP_REPORT_ID:
      decoded_ok = HID_decodePTPReport(packet, result);
      break;
    case MOUSE_REPORT_ID:
      decoded_ok = HID_decodeMouseReport(packet, result);
      break;
    default:
      clearReport(result); //return an empty report
      break;
    }
	
	return decoded_ok;
}

bool HID_decodeMouseReport(uint8_t* packet, HID_report_t* result)
{
    if(packet[REPORT_ID] != MOUSE_REPORT_ID)
    {
        //it's not a mouse report - exit
    	clearReport(result);
        return false;
    }

    result->mouse.buttons = packet[3];
    result->mouse.xDelta = packet[4];
    result->mouse.yDelta = packet[5];
    result->mouse.scrollDelta = packet[6];
    if (result->reportLength == 8)
    {
      result->mouse.panDelta = packet[7];
    }
    else
    {
      result->mouse.panDelta = 0;
    }
	
	return true;
}

bool HID_decodePTPReport(uint8_t* packet, HID_report_t* result)
{
    uint16_t dataStart = PTP_HEADER_BYTES;
    uint16_t dataEnd = 0;
    uint16_t contactBytes = 0;
    uint8_t slotCount = 0;
    uint8_t i = 0;

    if(packet[REPORT_ID] != PTP_REPORT_ID)
    {
        //it's not a absolute report
    	clearReport(result);
        return false;
    }
	
//#if defined(INCLUDE_PTP_PRESSURE)
//#define REPORT_SIZE (2 + 1 + 1 + 2 + 2 + 2 + 2 + 1 + 1) /**< Report size. Length(2), ReportID, Tip/Confidence/ContactID, contact area, X(2), Y(2), scan time(2), Contact count, button */
//#else
//#define REPORT_SIZE (2 + 1 + 1 + 2 + 2 + 2 + 1 + 1) /**< Report size. Length(2), ReportID, Tip/Confidence/ContactID, X(2), Y(2), scan time(2), Contact count, button */
//#endif
    
    result->reportLength = (uint16_t)(packet[1] << 8) + packet[0];
    result->reportID = packet[2];
	
    if(result->reportLength >= (PTP_HEADER_BYTES + PTP_TRAILER_BYTES + PTP_CONTACT_BYTES))
    {
        // Timestamp/contact count/buttons are always the final 4 bytes of the PTP report.
        uint16_t tail = result->reportLength - 4;
        result->ptp.timeStamp = (uint16_t) packet[tail];  // low byte
        result->ptp.timeStamp |= (uint16_t) packet[tail + 1] << 8;  // high byte
        result->ptp.contactCount = packet[tail + 2];
        result->ptp.buttons = packet[tail + 3];

        dataEnd = tail;
        contactBytes = dataEnd - dataStart;
        slotCount = contactBytes / PTP_CONTACT_BYTES;
        if(slotCount > PTP_MAX_CONTACTS)
        {
            slotCount = PTP_MAX_CONTACTS;
        }
        result->ptp.decodedContactSlots = slotCount;

        for(i = 0; i < slotCount; i++)
        {
            uint16_t contactOffset = dataStart + ((uint16_t)i * PTP_CONTACT_BYTES);
            uint8_t state = packet[contactOffset];
            result->ptp.contacts[i].confidence = state & 0x01;          // bit 0
            result->ptp.contacts[i].tip = (state & 0x02) >> 1;          // bit 1
            result->ptp.contacts[i].contactID = (state & 0xFC) >> 2;    // bits 2..7
            result->ptp.contacts[i].x = (uint16_t) packet[contactOffset + 1];
            result->ptp.contacts[i].x |= (uint16_t) packet[contactOffset + 2] << 8;
            result->ptp.contacts[i].y = (uint16_t) packet[contactOffset + 3];
            result->ptp.contacts[i].y |= (uint16_t) packet[contactOffset + 4] << 8;
        }

        // Preserve legacy single-contact fields as contact slot 0 for compatibility.
        if(slotCount > 0)
        {
            result->ptp.confidence = result->ptp.contacts[0].confidence;
            result->ptp.tip = result->ptp.contacts[0].tip;
            result->ptp.contactID = result->ptp.contacts[0].contactID;
            result->ptp.x = result->ptp.contacts[0].x;
            result->ptp.y = result->ptp.contacts[0].y;
        }
        else
        {
            result->ptp.confidence = 0;
            result->ptp.tip = 0;
            result->ptp.contactID = 0;
            result->ptp.x = 0;
            result->ptp.y = 0;
        }
    }
    else
    {
        result->ptp.timeStamp = 0;
        result->ptp.confidence = 0;
        result->ptp.tip = 0;
        result->ptp.contactID = 0;
        result->ptp.x = 0;
        result->ptp.y = 0;
        result->ptp.decodedContactSlots = 0;
        result->ptp.contactCount = 0;
        result->ptp.buttons = 0;
    }

	return true;
}

bool HID_isButtonPressed(HID_report_t* report, uint8_t buttonMask)
{
    uint8_t buttons = 0;
    if(report == NULL)
        return false;
    switch(report->reportID)
    {
        case MOUSE_REPORT_ID:
            buttons = report->mouse.buttons;
            break;
        case PTP_REPORT_ID:
            buttons = report->ptp.buttons;
            break;
        default:
            buttons = 0;
    }
    return buttons & buttonMask;
}


