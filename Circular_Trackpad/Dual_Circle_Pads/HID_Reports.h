#ifndef HID_REPORTS_H
#define HID_REPORTS_H

// Copyright (c) 2018 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "Project_Config.h"  // allows redefining Absolute reports to be the Alps format

#ifdef __cplusplus
extern "C" {
#endif

#define PTP_REPORT_ID			(0x01) /**<ID of a PTP Report*/
#define MOUSE_REPORT_ID         (0x06) /**<ID of a Mouse Report */

// Button Masks
#define BUTTON_1_MASK 0x1   //Left
#define BUTTON_2_MASK 0x2   //Right
#define BUTTON_3_MASK 0x4   //Middle
#define BUTTON_8_MASK 0x80  // Touchpad button

/****************** Public Data Structures *****************/

/** Contains the data from a mouse report packet.*/
typedef struct
{
    uint8_t buttons;        /**< Bitmap of the button states */
    int8_t  xDelta;         /**< Change in Horizontal movement */
    int8_t  yDelta;         /**< Change in vertical movement */
    int8_t  scrollDelta;    /**< Vertical Scroll value */
    int8_t  panDelta;       /**< Horizontal Scroll (or Pan) value */
} mouseReport_t;

/** Contains the report data for the PTP mode (report id 1) */
typedef struct
{
	uint16_t x;
	uint16_t y;
	uint16_t timeStamp;
	uint8_t  contactID;  //Uniquely identifies the contact within a given frame
	uint8_t  confidence;  //Microsoft says "Set when a contact is too large to be a finger" but that is backwards. It's clear if the object is too big.
	uint8_t  tip;   // Set if the contact is on the surface of the digitizer, once this clears you know you have lift-off
	uint8_t  contactCount;  //Total number of contacts to be reported in a given report
	uint8_t  buttons;
} PtpReport_t;

/** Struct that describes a generic report.
    This is a convienent container for handling report data.
    Use the reportID to know which member of the Union to use.*/
typedef struct
{
    union               /**< This union allows the report to be generic.*/
    {
        mouseReport_t mouse;        	/** < Treat the data as a mouse report */
        PtpReport_t 	ptp;			/**< Treats the data as a ptp report */
    };
    uint16_t reportLength;
    uint8_t reportID;   /**< ID of the report. Shows what type of report to use */
} HID_report_t;

// HID report header information
uint16_t HID_reportLength(uint8_t * packet);
uint8_t HID_reportID(uint8_t * packet);

// Decoding functions
bool HID_decodeReport(uint8_t* packet, HID_report_t* result);

bool HID_decodeMouseReport(uint8_t* packet, HID_report_t* result);
bool HID_decodePTPReport(uint8_t* packet, HID_report_t* result);

// // Query functions
bool HID_isButtonPressed(HID_report_t* report, uint8_t buttonMask);

#ifdef __cplusplus
}
#endif

#endif  // HID_REPORTS_H
