// Copyright (c) 2021 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include "API_C3.h"         /** < Provides API calls to interact with API_C3 firmware */
#include "API_Hardware.h"
#include "API_HostBus.h"    /** < Provides I2C connection to module */
#include "HID_Reports.h"

void printMouseReport(HID_report_t * report);

bool dataPrint_mode_g = true;  /** < toggle for printing out data > */
bool eventPrint_mode_g = true; /** < toggle for printing off events */

void setup()
{
  Serial.begin(115200);
  while(!Serial);
  
  API_Hardware_init();       //Initialize board hardware
  
  API_Hardware_PowerOn();    //Power up the board
  delay(2);                  //delay for power up
  
  // initialize i2c connection at 400kHz 
  API_C3_init(PROJECT_I2C_FREQUENCY, ALPS_SLAVE_ADDR); 
  
  delay(50);                 //delay before reading registers after startup
  
  // Collect information about the system and print to Serial
  systemInfo_t sysInfo;
  API_C3_readSystemInfo(&sysInfo);
  printSystemInfo(&sysInfo);

  initialize_saved_reports(); //initialize state for determining touch events
}

/** The main structure of the loop is: 
    Wait for the Data Ready line to assert. When it does, read the data (which clears DR) and analyze the data.
    The rest is just a user interface to change various settings.
    */
void loop()
{
  /* Handle incoming messages from module */
  if(API_C3_DR_Asserted())          // When Data is ready
  {
    HID_report_t report;
    API_C3_getReport(&report);    // read the report
    /* Interpret report from module */
    if(eventPrint_mode_g)
    {
        printEvent(&report);
    }
    if(dataPrint_mode_g)
    {
        printDataReport(&report);
    }
  }
  
  /* Handle incoming messages from user on serial */
  if(Serial.available())
  {
    char rxChar = Serial.read();
    switch(rxChar)
    {
      case 'c':
          Serial.println(F("Compensation Forced"));
          API_C3_forceComp();
          break;
          
      case 'C':
          Serial.println(F("Factory Calibrate... "));
          if(API_C3_factoryCalibrate())
          {
              Serial.println(F("Done"));
          }
          else
          {
              Serial.println(F("Failed")); //Hardware timeout (Did the module disconnect?) 
          }
          break;
          
      case 'f':
          Serial.println(F("Feed Enabled"));
          API_C3_enableFeed();
          break;
          
      case 'F':
          Serial.println(F("Feed Disabled"));
          API_C3_disableFeed();
          break;
          
      case 'a':
          Serial.println(F("Absolute Mode Set"));
          // API_C3_setCRQ_AbsoluteMode();
          API_C3_setPtpMode();
          break;
          
      case 'r':
          Serial.println(F("Relative Mode Set"));
          API_C3_setRelativeMode();
          break;
          
      case 's':
          systemInfo_t sysInfo;
          API_C3_readSystemInfo(&sysInfo);
          printSystemInfo(&sysInfo);
          break;
          
      case 'p':
          Serial.println(F("Settings saved to flash"));
          API_C3_saveConfig();
          break;
          
      case 't':
          Serial.println(F("Tracking Enabled"));
          API_C3_enableTracking();
          break;
          
      case 'T':
          Serial.println(F("Tracking Disabled"));
          API_C3_disableTracking();
          break;
          
      case 'v':
          Serial.println(F("Compensation Enabled"));
          API_C3_enableComp();
          break;
          
      case 'V':
          Serial.println(F("Compensation Disabled"));
          API_C3_disableComp();
          break;
          
      //Print modes
      case 'd':
          Serial.println(F("Data Printing turned on"));
          dataPrint_mode_g = true;
          break;
          
      case 'D':
          Serial.println(F("Data Printing turned off"));
          dataPrint_mode_g = false;
          break;
          
      case 'e':
          Serial.println(F("Event Printing turned on"));
          eventPrint_mode_g = true;
          break;
          
      case 'E':
          Serial.println(F("Event Printing turned off"));
          eventPrint_mode_g = false;
          break;
      
      case '?':
      case 'h':
      case 'H':
      default:
          printHelpTable();
          break;
    }
  }
}

/******** Functions for Printing Data ***********/

/** Prints a simple table of available commands.
    Commands can be sent over serial through Serial Monitor in Arduino IDE*/
void printHelpTable()
{
  Serial.println(F("Available Commands (case sensitive)"));
  Serial.println(F(""));
  Serial.println(F("c\t-\tForce Compensation"));
  Serial.println(F("C\t-\tFactory Calibrate"));
  Serial.println(F("f\t-\tEnable Feed (default)"));
  Serial.println(F("F\t-\tDisable Feed"));
  Serial.println(F("a\t-\tSet to Absolute Mode"));
  Serial.println(F("r\t-\tSet to Relative Mode (default)"));
  Serial.println(F("p\t-\tPersist Settings to Flash"));
  Serial.println(F("s\t-\tPrint System Info"));
  Serial.println(F("t\t-\tEnable Tracking (default)"));
  Serial.println(F("T\t-\tDisable Tracking"));
  
  Serial.println(F("v\t-\tEnable Compensation (default)"));
  Serial.println(F("V\t-\tDisable Compensation"));
  Serial.println(F(""));
  Serial.println(F("h, H, ?\t-\tPrint this Table"));
  Serial.println(F("d\t-\tTurn on Data Printing (default)"));
  Serial.println(F("D\t-\tTurn off Data Printing "));
  Serial.println(F("e\t-\tTurn on Event Printing (default)"));
  Serial.println(F("E\t-\tTurn off Event Printing "));
  Serial.println(F(""));
}

/** Prints a systemInfo_t struct to Serial.
    See API_C3.h for more information about the systemInfo_t struct */
void printSystemInfo(systemInfo_t* sysInfo)
{
  Serial.println(F("System Information"));
  Serial.print(F("Hardware ID:\t"));
  Serial.println(sysInfo->hardwareId, HEX);
  Serial.print(F("Firmware ID:\t"));
  Serial.println(sysInfo->firmwareId, HEX);
  Serial.print(F("Vendor ID:\t"));
  Serial.println(sysInfo->vendorId, HEX);
  Serial.print(F("Product ID:\t"));
  Serial.println(sysInfo->productId, HEX);
  Serial.print(F("Version ID:\t"));
  Serial.println(sysInfo->versionId, HEX);
  Serial.println(F(""));
}

/** Prints the information stored in a HID_report_t struct to serial */
void printDataReport(HID_report_t * report)
{
  //Use reportID to determine how to decode the report
  switch(report->reportID)
  {
    case MOUSE_REPORT_ID:
        printMouseReport(report);
        break;
    case KEYBOARD_REPORT_ID:
        printKeyboardReport(report);
        break;
    case ABSOLUTE_REPORT_ID:
        printCRQ_AbsoluteReport(report);   
        break;
    case PTP_REPORT_ID:
        printPtpReport(report);
        break;
    default:
        Serial.println(F("Error: Unknown Report ID"));
  }
}

/** Prints the information stored in a mouse report to serial */
void printMouseReport(HID_report_t* report)
{
  Serial.print(F("Report ID:\t0x"));
  Serial.println(report->reportID, HEX);
  Serial.print(F("Buttons:\t0b"));
  Serial.println(report->mouse.buttons, BIN);
  Serial.print(F("X Delta:\t"));
  Serial.println(report->mouse.xDelta);
  Serial.print(F("Y Delta:\t"));
  Serial.println(report->mouse.yDelta);
  Serial.print(F("Scroll Delta:\t"));
  Serial.println(report->mouse.scrollDelta);
  Serial.print(F("Pan Delta:\t"));
  Serial.println(report->mouse.panDelta);
  Serial.println(F(""));
}

/** Prints the information stored in a keyboard report to serial */
void printKeyboardReport(HID_report_t* report)
{
  Serial.print(F("Report ID:\t0x"));
  Serial.println(report->reportID, HEX);
  Serial.print(F("modifier:\t0x"));
  Serial.println(report->keyboard.modifier, HEX);
  Serial.print(F("Keycodes:"));
  for(uint8_t i = 0; i < 5; i++)
  {
      Serial.print(F("\t0x"));
      Serial.print(report->keyboard.keycode[i],HEX);
  }
  Serial.println();
  Serial.println();
}

/** Prints the information stored in a CRQ_ABSOLUTE report to serial*/
void printCRQ_AbsoluteReport(HID_report_t * report)
{
  Serial.print(F("Report ID:\t0x"));
  Serial.println(report->reportID, HEX);
  Serial.print(F("Contact Flags:\t0b"));
  Serial.println(report->abs.contactFlags, BIN);
  Serial.print(F("Buttons:\t0b"));
  Serial.println(report->abs.buttons, BIN);
  for(uint8_t i = 0; i < 5; i++)
  {
    Serial.print(F("Finger"));
    Serial.print(i);
    Serial.println(F(":"));
    Serial.print(F("    Palm Flags:\t0b"));
    Serial.println(report->abs.fingers[i].palm, BIN);
    Serial.print(F("    Valid:\t"));
    Serial.println(API_C3_isFingerValid(report,i)? F("Yes"):F("No"));
    Serial.print(F("    (x,y):\t("));
    Serial.print(report->abs.fingers[i].x, DEC);
    Serial.print(F(","));
    Serial.print(report->abs.fingers[i].y, DEC);
    Serial.println(F(")"));
  }
  
  Serial.println();
}

void printPtpReport(HID_report_t * report)
{
  Serial.print(F("Report ID:\t0x"));
  Serial.println(report->reportID, HEX);
  Serial.print(F("Time:\t"));
  Serial.println(report->ptp.timeStamp);
  Serial.print(F("Contact ID:\t"));
  Serial.println(report->ptp.contactID);
  Serial.print(F("Confidence:\t")); //Microsoft says "Set when a contact is too large to be a finger" but that is backwards. It's clear if the object is too big.
  Serial.println(report->ptp.confidence); 
  Serial.print(F("Tip:\t"));// Set if the contact is on the surface of the digitizer, once this clears you know you have lift-off
  Serial.println(report->ptp.tip);
  Serial.print(F("X:\t"));
  Serial.println(report->ptp.x);
  Serial.print(F("Y:\t"));
  Serial.println(report->ptp.y);
  Serial.print(F("Buttons:\t"));
  Serial.println(report->ptp.buttons);
  Serial.print(F("Contact Count:\t"));
  Serial.println(report->ptp.contactCount); //Total number of contacts to be reported in a given report
  Serial.println();
   
}

/**************************************************************/
/*************** FUNCTIONS FOR PRINTING EVENTS ****************/

/** @ingroup prevReports 
    These reports store the state of the most recent reports by type.
    Determining if an event occurred requires information about the previous state. 
*/
HID_report_t prevAbsReport_g;       /**< Most recent past CRQ_ABSOLUTE report */
HID_report_t prevMouseReport_g;     /**< Most recent past Mouse report */
HID_report_t prevKeyboardReport_g;  /**< Most recent past Keyboard report */
HID_report_t prevPtpReport_g;

/** Prints all the events that correspond to cur_report.
    Determines which printing function to use from the reportID */
void printEvent(HID_report_t* cur_report)
{
  switch( cur_report->reportID)
  {
    case ABSOLUTE_REPORT_ID:
        printCRQ_AbsoluteEvents(cur_report, &prevAbsReport_g);
        prevAbsReport_g = *cur_report;
        break;
    case MOUSE_REPORT_ID:
        printMouseReportEvents(cur_report, &prevMouseReport_g); 
        prevMouseReport_g = *cur_report;
        break;
    case KEYBOARD_REPORT_ID:
        printKeyboardEvents(cur_report, &prevKeyboardReport_g);
        prevKeyboardReport_g = *cur_report;
        break;
    case PTP_REPORT_ID:
        printPtpReportEvents(cur_report, &prevPtpReport_g);
        prevPtpReport_g = *cur_report;
        break;
    default:
        Serial.println(F("NOT VALID REPORT FOR EVENTS"));
        break;
  }
}

/** Prints all Mouse events.
    cur_report and prev_report should both be Mouse reports (relative mode)
    Mouse events are button presses/releases.*/
void printMouseReportEvents(HID_report_t* cur_report, HID_report_t* prev_report)
{
    if(prev_report->reportID == MOUSE_REPORT_ID)
    {
        printButtonEvents(cur_report, prev_report);
    }
}

void printPtpReportEvents(HID_report_t * cur_report, HID_report_t * prev_report)
{
  
  // !!! todo, fill this out
}

/** Prints all Absolute events.
    cur_report and prev_report should both be CRQ_ABSOLUTE reports
    Absolute events are finger presses/validation/releases and button presses/releases.*/
void printCRQ_AbsoluteEvents(HID_report_t* cur_report, HID_report_t* prev_report)
{
    if(prev_report->reportID == ABSOLUTE_REPORT_ID)
    {
        for(uint8_t i = 0; i < 5; i++)
        {
            printFingerEvents(cur_report, prev_report, i);
        }
        printButtonEvents(cur_report, prev_report);
    }
}

/** Prints all keyboard events.
    cur_report and prev_report should both be keyboard reports (relative mode)
    Keyboard events are modifier key presses/releases and keycode key presses/releases.*/
void printKeyboardEvents(HID_report_t* cur_report, HID_report_t* prev_report)
{
    if(prev_report->reportID == KEYBOARD_REPORT_ID && 
        cur_report->reportID == KEYBOARD_REPORT_ID)
    {
        printModifierKeyEvents(cur_report, prev_report);
        for(uint8_t i = 0; i < 6; i++) //look at each of the 6 keycodes available in keyboard report
        {
            printKeycodeEvents(cur_report, prev_report, i);
        }
    }
}

/** Determines and prints what modifier keys have changed since prev_report.
    Modifier keys are ctrl, alt, GUI/meta, and shift */
void printModifierKeyEvents(HID_report_t* cur_report, HID_report_t* prev_report)
{
    // show bitmap of what keys have changed since prev_report
    uint8_t changedModifier = cur_report->keyboard.modifier ^ prev_report->keyboard.modifier;
    
    // shows what keys are currently "pressed" now
    uint8_t curModifier = cur_report->keyboard.modifier;
    
    if(changedModifier & KEYBOARD_MODIFIER_LEFT_CTRL_KEY_MASK)
    {
        printKeypressEvent(F("Left Ctrl"), curModifier & KEYBOARD_MODIFIER_LEFT_CTRL_KEY_MASK);
    }
    if(changedModifier & KEYBOARD_MODIFIER_LEFT_SHIFT_KEY_MASK)
    {
        printKeypressEvent(F("Left Shift"), curModifier & KEYBOARD_MODIFIER_LEFT_SHIFT_KEY_MASK);
    }
    if(changedModifier & KEYBOARD_MODIFIER_LEFT_ALT_KEY_MASK)
    {
        printKeypressEvent(F("Left Alt"), curModifier & KEYBOARD_MODIFIER_LEFT_ALT_KEY_MASK);
    }
    if(changedModifier & KEYBOARD_MODIFIER_LEFT_GUI_KEY_MASK)
    {
        printKeypressEvent(F("Left GUI"), curModifier & KEYBOARD_MODIFIER_LEFT_GUI_KEY_MASK);
    }
    if(changedModifier & KEYBOARD_MODIFIER_RIGHT_CTRL_KEY_MASK)
    {
        printKeypressEvent(F("Right Ctrl"), curModifier & KEYBOARD_MODIFIER_RIGHT_CTRL_KEY_MASK);
    }
    if(changedModifier & KEYBOARD_MODIFIER_RIGHT_SHIFT_KEY_MASK)
    {
        printKeypressEvent(F("Right Shift"), curModifier & KEYBOARD_MODIFIER_RIGHT_SHIFT_KEY_MASK);
    }
    if(changedModifier & KEYBOARD_MODIFIER_RIGHT_ALT_KEY_MASK)
    {
        printKeypressEvent(F("Right Alt"), curModifier & KEYBOARD_MODIFIER_RIGHT_ALT_KEY_MASK);
    }
    if(changedModifier & KEYBOARD_MODIFIER_RIGHT_GUI_KEY_MASK)
    {
        printKeypressEvent(F("Right GUI"), curModifier & KEYBOARD_MODIFIER_RIGHT_GUI_KEY_MASK);
    }
}

/** A simple helper for printing that a key was pressed or released */
void printKeypressEvent(String keyname_str, bool pressed)
{
    Serial.print(keyname_str);
    if(pressed)
    {
        Serial.println(F(" Key Pressed"));
    }
    else
    {
        Serial.println(F(" Key Released"));
    }
}

/** Gives the String name of a key from it's keycode.
    Only contains keycodes currently used for gestures.*/
String getKeyName(uint8_t keycode)
{
    switch(keycode)
    {
        case 0x07:
            return F("D");
        case 0x2B:
            return F("TAB");
        case 0x36:
            return F("COMMA (,)");
        case 0x37:
            return F("PERIOD (.)");
        case 0x4F:
            return F("Right-Arrow ( -> )");
        case 0x50:
            return F("Left-Arrow ( <- )");
        default:
            return "Unknown (0x" + String(keycode, HEX) + ")";
    }
}

/** Determines and prints any keycode events given by a Keyboard report.
    In relative (HID) mode, the module may send keycodes corresponding to keyboard presses
    and releases to represent the functionality of a gesture. */
void printKeycodeEvents(HID_report_t* cur_report, HID_report_t* prev_report, uint8_t keycodeIndex)
{
    if(cur_report->keyboard.keycode[keycodeIndex] != prev_report->keyboard.keycode[keycodeIndex])
    {
        String keyName = "";
        bool pressed = cur_report->keyboard.keycode[keycodeIndex] != 0x0;
        if(pressed)
        {
            //use cur_report keycode
            keyName = getKeyName(cur_report->keyboard.keycode[keycodeIndex]);
        }
        else //released
        {
            //use prev_report keycode
            keyName = getKeyName(prev_report->keyboard.keycode[keycodeIndex]);
        }
        printKeypressEvent(keyName, pressed);
    }
}

/** Determines and prints all contact and validation Events for fingers in absolute mode. 
    Typically the module will send preliminary coordinates of "contacted" fingers 
    a couple frames before it is validated and confident. This is intended to 
    demonstrate the difference between those events. For most cases, you want to confirm 
    that the finger is valid before using its coordinates. */
void printFingerEvents(HID_report_t* cur_report, HID_report_t* prev_report, uint8_t finger_num)
{
    printContactEvents(cur_report, prev_report, finger_num);
    printFingerValidationEvents(cur_report, prev_report, finger_num);
}
/** Determines and prints if a finger contact event occurred between cur_report and prev_report for finger_num. 
    Demonstrates use of the API_C3_isFingerContacted function. */
void printContactEvents(HID_report_t* cur_report, HID_report_t* prev_report, uint8_t finger_num)
{
    //finger is contacted now
    if (API_C3_isFingerContacted(cur_report, finger_num))
    {
        //it wasn't contacted before
        if (!API_C3_isFingerContacted(prev_report, finger_num))
        {
            Serial.print(F("Finger "));
            Serial.print(finger_num);
            Serial.println(F(" contacted"));
        }
    }
    //not contacted now
    else
    {
        //it was contacted before
        if (API_C3_isFingerContacted(prev_report, finger_num))
        {
            Serial.print(F("Finger "));
            Serial.print(finger_num);
            Serial.println(F(" released"));
        }
    }
}
/** Determines and prints if a finger validation event occurred between cur_report and prev_report for finger_num. 
    Demonstrates use of the API_C3_isFingerValid function. 
    Note: x,y data for invalid fingers should generally be ignored. When a finger is released, its last location is 
    still sent. This should be ignored because the finger is not valid. */
void printFingerValidationEvents(HID_report_t* cur_report, HID_report_t* prev_report, uint8_t finger_num)
{
    //finger is valid now
    if (API_C3_isFingerValid(cur_report, finger_num))
    {
        //it wasn't valid before
        if (!API_C3_isFingerValid(prev_report, finger_num))
        {
            Serial.print(F("Finger "));
            Serial.print(finger_num);
            Serial.println(F(" valid"));
        }
    }
    // finger is not valid now
    else
    {
        // it was valid before
        if (API_C3_isFingerValid(prev_report, finger_num))
        {
            Serial.print(F("Finger "));
            Serial.print(finger_num);
            Serial.println(F(" invalid"));
        }
    }
}

/** Returns the button information in the passed in report. 
    returns 0 if NULL or report is a keyboard report that does not
    have button data. */
uint8_t getButtonsFromReport(HID_report_t* report)
{
    if(report == NULL)
    {
        return 0;
    }
    switch(report->reportID)
    {
        case MOUSE_REPORT_ID:
            return report->mouse.buttons;
        case ABSOLUTE_REPORT_ID:
            return report->abs.buttons;
        default:
            return 0;
    }
}

/** Determines and prints if a button event (press or release) occurred between cur_report and prev_report. 
    Demonstrates use of the API_C3_isButtonPressed function. */
void printButtonEvents(HID_report_t* cur_report, HID_report_t* prev_report)
{
    uint8_t currentButtons = getButtonsFromReport(cur_report);
    uint8_t prevButtons = getButtonsFromReport(prev_report);
    
    // XOR of the button fields shows what changed
    uint8_t changed_buttons = currentButtons ^ prevButtons;
    uint8_t mask = 1;
    for(uint8_t i = 1; i < 9; i++)
    {
        if(changed_buttons & mask)
        {
            Serial.print(F("Button "));
            Serial.print(i);
            if (API_C3_isButtonPressed(cur_report, mask))
            {
                Serial.println(F(" Pressed"));
            }
            else
            {
                Serial.println(F(" Released"));
            }
        }
        
        mask <<= 1;
    }
    
}

/** Initializes reports used for keeping track of state.
    Each report will represent the most recent past report of
    its type. */
void initialize_saved_reports()
{
    prevAbsReport_g.reportID = ABSOLUTE_REPORT_ID; //should be initialized... is there a conveninet way?
    prevAbsReport_g.abs.buttons = 0x0;
    prevAbsReport_g.abs.contactFlags = 0x0;
    
    prevMouseReport_g.reportID = MOUSE_REPORT_ID;
    prevMouseReport_g.mouse.buttons = 0x0;
    
    prevKeyboardReport_g.reportID = KEYBOARD_REPORT_ID;
    prevKeyboardReport_g.keyboard.modifier = 0x0;
    for(uint8_t i = 0; i < 5; i++)
    {
        prevKeyboardReport_g.keyboard.keycode[i] = 0x0;
    }
}
