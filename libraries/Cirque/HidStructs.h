#ifndef HID_STRUCTS_H
#define HID_STRUCTS_H

// Copyright (c) 2025 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include <stdint.h>

#define MAX_CRQ_ALPS_FINGER_COUNT 5
#define MAX_PTP_FINGER_COUNT 3

#define INCLUDE_UNCOMMON_REPORTS

// Report IDs
// see Gen6 HidReportIds.h for all of these
typedef enum reportIds_t : uint8_t
{
    id_unknown = 0,
    id_ignored = 0,
    id_ptpReport = 1,
    id_deviceCapabilities = 2,
    id_certificationStatus = 3,
    id_inputMode = 4,
    id_selectReporting = 5,
    id_mouseReport = 6,
    id_fwUpdate = 7,
    id_keyReport = 8,
    // id_crqAlpsReport = 9, // uses data length to figure out which it is
    // id_serial = 13,
    // id_AlpsFeatureReport = 14,
    id_customMeas = 14,
    id_spiderMeas = 15,
    // id_AlpsStickAbsoluteReport = 15,
    // HID_Rep_ID_SerialPassthrough  = 18, 
	id_adaptiveTouch = 25, 
    // id_GuisePipe = 240,
    // id_reserved0 = 254,
    id_resetResponse = 255
} reportIds_t;

// Mouse report packet
// Quirks - some bios versions only read 7 bytes (even though HID Length is 8)
// This code doesn't bother to handle that. You are the host, don't do that.
typedef struct
{
    uint8_t buttons;        // Bitmap of the button states 
    int8_t  xDelta;         // Change in Horizontal movement 
    int8_t  yDelta;         // Change in vertical movement 
    int8_t  scrollDelta;    // Vertical Scroll value 
    int8_t  panDelta;       // Horizontal Scroll (or Pan) value 
} mouseReport_t;

// PTP data - this is a sub-part of a report
typedef struct
{
	uint16_t x;
	uint16_t y;
	uint8_t  contactID;    //Uniquely identifies the contact within a given frame
	uint8_t  confidence;   //Microsoft ways "Set when a contact is too large to be a finger" but that is backwards. It's clear if the object is too big.
	uint8_t  tip;          // Set if the contact is on the surface of the digitizer, once this clears you know you have lift-off
} PtpFingerData_t;

typedef struct
{
	uint16_t timeStamp;
	uint8_t  contactCount; //Total number of contacts to be reported in a given report
	uint8_t  buttons;
	PtpFingerData_t fingers[MAX_PTP_FINGER_COUNT];
    uint8_t numberFingers;
} PtpReport_t;

// Keyboard report packet
typedef struct
{
    uint8_t modifier1;  // alt,ctrl, gui keys
    uint8_t modifier2;
    uint8_t keycode[6]; // Keycodes pressed, only the first one is used for now
} keyReport_t;

typedef struct
{
	uint16_t NumberValidBytes; // measurements results, number of bytes
} customMeasHeader_t;

// // Cirque Absolute data of a single finger - this is a sub-part of a report
// typedef struct
// {
//     uint16_t y;     // Absolute Y position of finger 
//     uint16_t x;     //  Absolute X position of finger 
//     uint8_t  palm;  //  Bitfield with Palm-reject, confidence and single sample information Note: a finger may have old or inaccurate x,y data when the confidence is low
// } CRQFingerData_t;

// // Cirque Absolute report
// typedef struct
// {
//     CRQFingerData_t fingers[MAX_CRQ_ALPS_FINGER_COUNT];  // Array of 5 Fingers 
//     uint8_t contactFlags;        // bitmap of contacted fingers ie. 0x3 means fingers 0 and 1 are down
//     uint8_t buttons;             // Bitmap of the button states 
// } CRQabsoluteReport_t;

// // Alps Absolute data of a single finger - this is a sub-part of a report
// typedef struct
// {
//     uint16_t x;     // Absolute X position of finger
//     uint16_t y;     // Absolute Y position of finger
//     uint8_t  palm;  // Bitfield with Palm-reject, confidence and single sample information Note: a finger may have old or inaccurate x,y data when the confidence is lo
//     uint8_t z; 	    // Z value of the finger
// } AlpsFingerData_t;

// // Alps report packet
// typedef struct
// {
//     AlpsFingerData_t    fingers[MAX_CRQ_ALPS_FINGER_COUNT];     // Array of 5 Fingersn
//     uint8_t         	contactFlags;   // bitmap of contacted fingers ie. 0x3 means fingers 0 and 1 are down
//     uint8_t         	buttons;        // Bitmap of the button statesn
// } AlpsAbsoluteReport_t;

// typedef struct
// {
// 	uint8_t buttons; // b0 = left, b1 = right, b2 = middle 
//     uint16_t X;
//     uint16_t Y;
//     uint16_t Z;
// } StickReport_t;

typedef union  // This union allows the report to be generic, use reportType to decide which report it is
{
    mouseReport_t mouse;    
    PtpReport_t ptp; 
    keyReport_t keyboard;  	       
	customMeasHeader_t customMeasHeader;

    // CRQabsoluteReport_t abs;    	
    // AlpsAbsoluteReport_t alpsAbs;	
    // StickReport_t stick;

} AnyHIDReport_t;


#endif // HID_STRUCTS_H


