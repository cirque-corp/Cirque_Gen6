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

// !!! See Gen6 Firmware HID.h ~line 43 - there are a zillion of these already defined
// Report IDs
// Normally you read the report descriptor to get these but Gen6 uses the same report IDs for
// any given firmware so they can be hard-coded
#define PTP_REPORT_ID			(0x01) /**<ID of a PTP Report*/
#define MOUSE_REPORT_ID         (0x06) /**<ID of a Mouse Report */
#define KEYBOARD_REPORT_ID      (0x08) /**<ID of a Keyboard Report */
#define ABSOLUTE_REPORT_ID      (0x09) /**<ID of a Cirque/Alps Absolute Report, both use ID 9 */

// ALPS report IDs
#define ALPS_FEATURE_REPORT_ID (0x0e) // U1 used 5,
#define	ALPS_STICKPOINTER_ABS_REPORT_ID (0x0f) // U1 used 6,

// !!! consider moving Cirque ABS report to a different ID#
// keep the report ids smaller than 14 or HID will require another byte

/** Keyboard Modifier Masks
    see keyboardReport_t */
#define KEYBOARD_MODIFIER_LEFT_CTRL_KEY_MASK    0x01
#define KEYBOARD_MODIFIER_LEFT_SHIFT_KEY_MASK   0x02
#define KEYBOARD_MODIFIER_LEFT_ALT_KEY_MASK     0x04
#define KEYBOARD_MODIFIER_LEFT_GUI_KEY_MASK     0x08
#define KEYBOARD_MODIFIER_RIGHT_CTRL_KEY_MASK   0x10
#define KEYBOARD_MODIFIER_RIGHT_SHIFT_KEY_MASK  0x20
#define KEYBOARD_MODIFIER_RIGHT_ALT_KEY_MASK    0x40
#define KEYBOARD_MODIFIER_RIGHT_GUI_KEY_MASK    0x80

/** Palm Data Masks
    see fingerData_t */
#define CRQ_ABSOLUTE_PALM_REJECT_MASK   0x80
#define CRQ_ABSOLUTE_CONFIDENCE_MASK    0x02
#define CRQ_ABSOLUTE_SINGLE_SAMPLE_MASK 0x01

/** Button Masks
    see mouseReport_t, CRQabsoluteReport_t */
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

/** Contains the data for a Keyboard report */
typedef struct
{
    uint8_t keycode[6]; /**< Keycodes pressed, only the first one is used for now */
    uint8_t modifier;   /**< alt,ctrl, gui keys */
} keyboardReport_t;

/** Subcontainer for holding the Cirque Absolute Report data of a single finger*/
typedef struct
{
    uint16_t y;     /**< Absolute Y position of finger */
    uint16_t x;     /** < Absolute X position of finger */
    uint8_t  palm;  /** < Bitfield with Palm-reject, confidence and single sample information Note: a finger may have old or inaccurate x,y data when the confidence is low*/
} fingerData_t;

/** Subcontainer for holding the Alps Absolute Report data of a single finger*/
typedef struct
{
    uint16_t x;     /** < Absolute X position of finger */
    uint16_t y;     /**< Absolute Y position of finger */
    uint8_t  palm;  /** < Bitfield with Palm-reject, confidence and single sample information Note: a finger may have old or inaccurate x,y data when the confidence is low*/
    uint8_t z; 	/**< Z value of the finger */
} AlpsFingerData_t;

/** Contains the Report data for the Cirque Absolute mode (report id 9) */
typedef struct
{
    fingerData_t    fingers[5];     /**< Array of 5 Fingers */
    uint8_t         contactFlags;   /**< bitmap of contacted fingers ie. 0x3 means fingers 0 and 1 are down*/
    uint8_t         buttons;        /**< Bitmap of the button states */
} CRQabsoluteReport_t;

/** Contains the Report data for the Cirque Absolute mode (report id 9) */
typedef struct
{
    AlpsFingerData_t    fingers[5];     /**< Array of 5 Fingers */
    uint8_t         	contactFlags;   /**< bitmap of contacted fingers ie. 0x3 means fingers 0 and 1 are down*/
    uint8_t         	buttons;        /**< Bitmap of the button states */
} AlpsAbsoluteReport_t;

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

typedef struct
{
	uint8_t buttons; // b0 = left, b1 = right, b2 = middle 
    uint16_t X;
    uint16_t Y;
    uint16_t Z;
} StickReport_t;

/** Struct that describes a generic report.
    This is a convienent container for handling report data.
    Use the reportID to know which member of the Union to use.*/
typedef struct
{
    union               /**< This union allows the report to be generic.*/
    {
        mouseReport_t mouse;        	/** < Treat the data as a mouse report */
        keyboardReport_t keyboard;  	/**< Treats the data as a keyboard report */
        CRQabsoluteReport_t abs;    	/**< Treats the data as an absolute report */
        AlpsAbsoluteReport_t alpsAbs;	/**< Treats the data as an absolute report */
        PtpReport_t 	ptp;			/**< Treats the data as a ptp report */
        StickReport_t stick;			/**< Treats the data as a stick report */
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
bool HID_decodeKeyboardReport(uint8_t* packet, HID_report_t* result);
bool HID_decodeAlpsAbsoluteReport(uint8_t* packet, HID_report_t* result);
bool HID_decodeCirqueAbsoluteReport(uint8_t* packet, HID_report_t* result);
bool HID_decodePTPReport(uint8_t* packet, HID_report_t* result);
bool HID_decodeStickReport(uint8_t* packet, HID_report_t* result);

// Query functions
bool HID_isFingerValid(HID_report_t* report, uint8_t finger_num);
bool HID_isFingerContacted(HID_report_t* report, uint8_t finger_num);
uint8_t HID_numberFingers(HID_report_t* report);
bool HID_isButtonPressed(HID_report_t* report, uint8_t buttonMask);

#ifdef __cplusplus
}
#endif

#endif  // HID_REPORTS_H
