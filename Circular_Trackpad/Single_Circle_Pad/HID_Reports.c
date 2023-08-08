// Copyright (c) 2018 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

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
    report->reportID = 0;
    report->reportLength = 0;
    //Clears the values of the largest member of the Union.
    report->ptp.x = 0;
    report->ptp.y = 0;
    report->ptp.timeStamp = 0;
    report->ptp.contactID = 0;
    report->ptp.confidence = 0;
    report->ptp.tip = 0;
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
	
    result->ptp.confidence = packet[3] & 0x01;   // bit 0
    result->ptp.tip = (packet[3] & 0x02) >> 1;   // bit 1
    result->ptp.contactID = (packet[3] & 0xFC) >> 2; // bits 2..7

	result->ptp.x = (uint16_t) packet[4];   // low byte
	result->ptp.x |= (uint16_t) packet[5] << 8;  // high byte

	result->ptp.y = (uint16_t) packet[6];   // low byte
	result->ptp.y |= (uint16_t) packet[7] << 8;  // high byte
    
    if(result->reportLength == 12)
    {
        result->ptp.timeStamp = (uint16_t) packet[8];  // low byte
        result->ptp.timeStamp |= (uint16_t ) packet[9] << 8;  // high byte
        result->ptp.contactCount = packet[10];
        result->ptp.buttons = packet[11]; 
    }
    else //if(result->reportLength == 13)
    {
        result->ptp.timeStamp = (uint16_t) packet[9];  // low byte
        result->ptp.timeStamp |= (uint16_t ) packet[10] << 8;  // high byte
        result->ptp.contactCount = packet[11];
        result->ptp.buttons = packet[12];  
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


