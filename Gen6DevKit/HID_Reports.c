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

/** Clears all values of report to zero.
    Assumes absolute report is the largest of the union*/
void clearReport(HID_report_t* report)
{
    report->reportID = 0;
    //Clears the values of the largest member of the Union.
    report->abs.contactFlags = 0;
    uint8_t finger;
    for(finger = 0; finger < 5; finger++) // each of the 5 fingers
    {
        report->abs.fingers[finger].palm = 0;
        report->abs.fingers[finger].x = 0;
        report->abs.fingers[finger].y = 0;

    }
    report->abs.buttons = 0;
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
        case KEYBOARD_REPORT_ID:
            decoded_ok = HID_decodeKeyboardReport(packet, result);
            break;
        case ABSOLUTE_REPORT_ID:
			// Two absolute reports exist and they share the same ID
			// Typically a project is built to only use one of these abs reports
			#if PROJECT_MAX_PACKET_SIZE == 30
			decoded_ok = HID_decodeAlpsAbsoluteReport(packet, result);  // older, 30 byte format
			#else
			decoded_ok = HID_decodeCirqueAbsoluteReport(packet, result);  // newer, 53 byte format
			#endif
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
    result->mouse.panDelta = packet[7];
	
	return true;
}

bool HID_decodeKeyboardReport(uint8_t* packet, HID_report_t* result)
{
    if(packet[REPORT_ID] != KEYBOARD_REPORT_ID)
    {
        //it's not a keyboard report
    	clearReport(result);
        return false;
    }

    result->keyboard.modifier = packet[3];
    // packet[4] reserved byte
    result->keyboard.keycode[0] = packet[5];
    result->keyboard.keycode[1] = packet[6];
    result->keyboard.keycode[2] = packet[7];
    result->keyboard.keycode[3] = packet[8];
    result->keyboard.keycode[4] = packet[9];
    result->keyboard.keycode[5] = packet[10];
	
	return true;
}

bool HID_decodeAlpsAbsoluteReport(uint8_t* packet, HID_report_t* result)
{
    if(packet[REPORT_ID] != ABSOLUTE_REPORT_ID)
    {
        //it's not a absolute report
    	clearReport(result);
        return false;
    }
    result->alpsAbs.buttons = packet[3];
    result->alpsAbs.contactFlags = packet[4];
	uint8_t index = 5;
    uint8_t finger;
    for(finger = 0; finger < 5; finger++)
    {
        result->alpsAbs.fingers[finger].x = (uint16_t ) packet[index++];
        result->alpsAbs.fingers[finger].x |= (uint16_t ) packet[index++] << 8;

        result->alpsAbs.fingers[finger].y = (uint16_t ) packet[index++];
        result->alpsAbs.fingers[finger].y |= (uint16_t ) packet[index++] << 8;

        result->alpsAbs.fingers[finger].z = packet[index] & 0x7F;
        result->alpsAbs.fingers[finger].palm = packet[index++] >> 7;
    }

	return true;
}

bool HID_decodeCirqueAbsoluteReport(uint8_t* packet, HID_report_t* result)
{
	// !!! verify this against the docs

    if(packet[REPORT_ID] != ABSOLUTE_REPORT_ID)
    {
        //it's not a absolute report
    	clearReport(result);
        return false;
    }
	
    result->abs.contactFlags = packet[3];
    uint8_t index = 4;
	uint8_t finger;
    for(finger = 0; finger < 5; finger++)
    {
        result->abs.fingers[finger].palm = packet[index++];

        result->abs.fingers[finger].x = packet[index++];
        result->abs.fingers[finger].x |= (uint16_t ) packet[index++] << 8;

        result->abs.fingers[finger].y = (uint16_t ) packet[index++];
        result->abs.fingers[finger].y |= (uint16_t ) packet[index++] << 8;
    }
    result->abs.buttons = packet[29];
	
	return true;
}

bool HID_decodeStickReport(uint8_t* packet, HID_report_t* result)
{
	if(packet[REPORT_ID] != ALPS_STICKPOINTER_ABS_REPORT_ID)
	{
		// not a stick report
		clearReport(result);
		return false;
	}
    result->reportLength = (uint16_t)(packet[1] << 8) + packet[0];
    result->reportID = packet[2];
	
	result->stick.buttons = packet[3];
	result->stick.X = packet[4] | (packet[5] << 8);
	result->stick.Y = packet[6] | (packet[7] << 8);
	result->stick.Z = packet[8] | (packet[9] << 8);
	
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

/** determines if the finger_num finger corresponds to a valid finger in the report */
bool HID_isFingerValid(HID_report_t* report, uint8_t finger_num)
{
    if(report == NULL || report->reportID != ABSOLUTE_REPORT_ID)
    {
        return false;
    }
    if(finger_num > 4 || finger_num < 0 )
    {
        return false;
    }
    
    uint8_t palm = report->abs.fingers[finger_num].palm;
    return ((palm & CRQ_ABSOLUTE_CONFIDENCE_MASK) && !(palm & CRQ_ABSOLUTE_PALM_REJECT_MASK));
}

/** determines if the finger_num is contacted */
bool HID_isFingerContacted(HID_report_t* report, uint8_t finger_num)
{
    if(report == NULL || report->reportID != ABSOLUTE_REPORT_ID)
    {
        return false;
    }
    if(finger_num > 4 || finger_num < 0 )
    {
        return false;
    }
    return report->abs.contactFlags &  (0x1 << finger_num); 
}

uint8_t HID_numberFingers(HID_report_t* report) 
{ 
	if (report == NULL || report->reportID != ABSOLUTE_REPORT_ID)
	{
		return false;
	}

	uint8_t contactFlags = report->abs.contactFlags;

	// Kernighan's bit counting algorithm
    uint8_t count = 0;
    while (contactFlags) 
    { 
      contactFlags &= (contactFlags - 1) ; 
      count++;
    } 
    return count; 
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
        case ABSOLUTE_REPORT_ID:
            buttons = report->abs.buttons;
            break;
		case ALPS_STICKPOINTER_ABS_REPORT_ID:
			buttons = report->stick.buttons;
			break;
        default:
            buttons = 0;
    }
    return buttons & buttonMask;
}


