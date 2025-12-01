// Copyright (c) 2023 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include "API_C3.h"         /** < Provides API calls to interact with API_C3 firmware */
#include "API_Hardware.h"
#include "API_HostBus.h"    /** < Provides I2C connection to module */
#include "HID_Reports.h"
#include "I2C.h"

// This code is built for the GlidePoint Development Kit (cirque.com), PCB 02-000620-00 Rev A02, Assembly A0-000194-00 Rev A02
// The kit uses a Teensy 4.0 as the host, talking to two touchpads.
// This demo is for the Gen6 touchpads, which are typically 23 mm and 35 mm circle sensors
// but any Gen6 touchpad should work

// This "dual host" interface uses these pins:
// Touchpad #1 - connects to J7 - 23mm 
// IO18_SDA, IO19_SCL, IO9_DR0

// Touchpad #2 - connects to J9 - 35 mm
// IO17_SDA1, IO16_SCL1, IO7_DR1

// void printMouseReport(HID_report_t * report);

bool dataPrint_mode_g = true;  /** < toggle for printing out data > */
bool eventPrint_mode_g = true; /** < toggle for printing off events */

void setup()
{
  Serial.begin(115200);
  while(!Serial);
  
  API_Hardware_init();       //Initialize board hardware
  
  delay(2);                  //delay for power up
  
  // initialize i2c connection at 400kHz 
  API_C3_init(PROJECT_I2C_FREQUENCY, ALPS_I2C_ADDR, PROJECT_I2C_FREQUENCY, ALPS_I2C_ADDR); 

  Serial.println(F("Dual Pad Demo"));
  Serial.println(F("I2C initialized"));

  uint8_t i2c_error = i2cPing(0, ALPS_I2C_ADDR);

  Serial.print(F("Channel 0 ALPS_I2C_ADDR ping response: "));
  Serial.println(i2c_error,HEX);
  
  delay(50);                 //delay before reading registers after startup
  
  if (i2c_error == 0)  // 0 = no error
  {
    // Collect information about the system and print to Serial
    systemInfo_t sysInfo;
    API_C3_readSystemInfo(0, &sysInfo);
    printSystemInfo(0, &sysInfo);

    // initialize_saved_reports(); //initialize state for determining touch events
  }

  i2c_error = i2cPing(1, ALPS_I2C_ADDR);

  Serial.print(F("Channel 1 ALPS_I2C_ADDR ping response: "));
  Serial.println(i2c_error,HEX);
  
  delay(50);                 //delay before reading registers after startup
  
  if (i2c_error == 0)  // 0 = no error
  {
    // Collect information about the system and print to Serial
    systemInfo_t sysInfo;
    API_C3_readSystemInfo(1, &sysInfo);
    printSystemInfo(1, &sysInfo);

    // initialize_saved_reports(); //initialize state for determining touch events
  }
}

/** The main structure of the loop is: 
    Wait for the Data Ready line to assert. When it does, read the data (which clears DR) and analyze the data.
    The rest is just a user interface to change various settings.
    */
void loop()
{
  /* Handle incoming messages from modules */
  if(API_C3_DR_Asserted() & DR0_MASK)          // When Data is ready
  {
    HID_report_t report;
    API_C3_getReport(0, &report);    // read the report
    /* Interpret report from module */
    if(eventPrint_mode_g)
    {
        printEvent(0, &report);
    }
    if(dataPrint_mode_g)
    {
        printDataReport(0, &report);
    }
  }
  
  if(API_C3_DR_Asserted() & DR1_MASK)          // When Data is ready
  {
    HID_report_t report;
    API_C3_getReport(1, &report);    // read the report
    /* Interpret report from module */
    if(eventPrint_mode_g)
    {
        printEvent(1, &report);
    }
    if(dataPrint_mode_g)
    {
        printDataReport(1, &report);
    }
  }
  
  /* Handle incoming messages from user on serial */
  if(Serial.available())
  {
    char rxChar0, rxChar1;
    rxChar0 = Serial.read();
    if(Serial.available() > 1)
    {
      rxChar1 = Serial.read();
    }
    else
    {
      rxChar1 = 'X';
    }
    processSerialCommand(rxChar0, rxChar1);
    Serial.clear();
  }
}

void processSerialCommand(char rxChar0, char rxChar1)
{
  uint8_t i2c_channel = 0xff;

  switch(rxChar1)
  {
    case '0':
      i2c_channel = 0;
      break;
      
    case '1':
      i2c_channel = 1;
      break;

    default:
      switch(rxChar0)
      {
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

        default:
            printHelpTable();
            break;
      }
      break;
  }

  if (i2c_channel <= 1)
  {
    switch(rxChar0)
    {
      case 'c':
          Serial.println(F("Force Compensation... "));
          if (API_C3_forceComp(i2c_channel))
          {
            Serial.println(F("Done"));
          }
          else
          {
            Serial.println(F("Failed")); //Hardware timeout (Did the module disconnect?) 
          }
          break;
          
      case 'C':
          Serial.println(F("Factory Calibrate... "));
          if(API_C3_factoryCalibrate(i2c_channel))
          {
            Serial.println(F("Done"));
          }
          else
          {
            Serial.println(F("Failed")); //Hardware timeout (Did the module disconnect?) 
          }
          break;
          
      case 'f':
          Serial.println(F("Enabling Feed..."));
          if (API_C3_enableFeed(i2c_channel))
          {
            Serial.println(F("Done"));
          }
          else
          {
            Serial.println(F("Failed")); //Hardware timeout (Did the module disconnect?) 
          }
          break;
          
      case 'F':
          Serial.println(F("Disabling Feed..."));
          if (API_C3_disableFeed(i2c_channel))
          {
            Serial.println(F("Done"));
          }
          else
          {
            Serial.println(F("Failed")); //Hardware timeout (Did the module disconnect?) 
          }
          break;
                    
      case 'p':
      case 'a':
          Serial.println(F("Setting PTP Mode..."));
          if (API_C3_setPtpMode(i2c_channel))
          {
            Serial.println(F("Done"));
          }
          else
          {
            Serial.println(F("Failed")); //Hardware timeout (Did the module disconnect?) 
          }
          break;
          
      case 'r':
          Serial.println(F("Setting Relative Mode..."));
          if (API_C3_setRelativeMode(i2c_channel))
          {
            Serial.println(F("Done"));
          }
          else
          {
            Serial.println(F("Failed")); //Hardware timeout (Did the module disconnect?) 
          }
          break;
          
      case 's':
          systemInfo_t sysInfo;
          API_C3_readSystemInfo(i2c_channel, &sysInfo);
          printSystemInfo(i2c_channel, &sysInfo);
          break;

      case 'g':
          HIDDescriptor_t hidDescriptor;
          HB_HID_GetHidDescriptor(i2c_channel, 0x0020, &hidDescriptor);
          printHidDescriptor(&hidDescriptor);
          break;
          
      case 'S':
          Serial.println(F("Saving settings to flash..."));
          if (API_C3_saveConfig(i2c_channel))
          {
            Serial.println(F("Done"));
          }
          else
          {
            Serial.println(F("Failed")); //Hardware timeout (Did the module disconnect?) 
          }
          break;
          
      case 't':
          Serial.println(F("Enabling Tracking..."));
          if (API_C3_enableTracking(i2c_channel))
          {
            Serial.println(F("Done"));
          }
          else
          {
            Serial.println(F("Failed")); //Hardware timeout (Did the module disconnect?) 
          }
          break;
          
      case 'T':
          Serial.println(F("Disabling Tracking..."));
          if (API_C3_disableTracking(i2c_channel))
          {
            Serial.println(F("Done"));
          }
          else
          {
            Serial.println(F("Failed")); //Hardware timeout (Did the module disconnect?) 
          }
          break;
          
      case 'v':
          Serial.println(F("Enabling Compensation..."));
          if (API_C3_enableComp(i2c_channel))
          {
            Serial.println(F("Done"));
          }
          else
          {
            Serial.println(F("Failed")); //Hardware timeout (Did the module disconnect?) 
          }
          break;
          
      case 'V':
          Serial.println(F("Disabling Compensation..."));
          if (API_C3_disableComp(i2c_channel))
          {
            Serial.println(F("Done"));
          }
          else
          {
            Serial.println(F("Failed")); //Hardware timeout (Did the module disconnect?) 
          }
          break;

      case 'i':
          Serial.println(F("Reading comp matrix..."));
          printCompMatrix(i2c_channel);
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
        
      case '\n' :
        break;
      case '?':
      case 'h':
      case 'H':
      default:
          printHelpTable();
          break;
    }
  }
  Serial.println("");
}

/******** Functions for Printing Data ***********/

/** Prints a simple table of available commands.
    Commands can be sent over serial through Serial Monitor in Arduino IDE*/
void printHelpTable()
{
  Serial.println(F("Available Commands (case sensitive):"));
  Serial.println(F("(Replace \'#\' with I2C channel num [0,1])"));
  Serial.println(F(""));
  Serial.println(F("c#\t-\tForce Compensation"));
  Serial.println(F("C#\t-\tFactory Calibrate"));
  Serial.println(F("f#\t-\tEnable Feed (default)"));
  Serial.println(F("F#\t-\tDisable Feed"));
  Serial.println(F("p#, a#\t-\tSet to PTP Mode"));
  Serial.println(F("r#\t-\tSet to Relative Mode (default)"));
  Serial.println(F("S#\t-\tSave Settings to Flash"));
  Serial.println(F("s#\t-\tPrint System Info"));
  Serial.println(F("t#\t-\tEnable Tracking (default)"));
  Serial.println(F("T#\t-\tDisable Tracking"));
  Serial.println(F("v#\t-\tEnable Compensation (default)"));
  Serial.println(F("V#\t-\tDisable Compensation"));
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
void printSystemInfo(uint8_t channel, systemInfo_t* sysInfo)
{
  Serial.print(F("Channel "));
  Serial.print(channel, DEC);
  Serial.println(F(" System Information"));
  Serial.print(F("Hardware ID:\t0x"));
  Serial.println(sysInfo->hardwareId, HEX);
  Serial.print(F("Firmware ID:\t0x"));
  Serial.println(sysInfo->firmwareId, HEX);
  Serial.print(F("Vendor ID:\t0x"));
  Serial.println(sysInfo->vendorId, HEX);
  Serial.print(F("Product ID:\t0x"));
  Serial.println(sysInfo->productId, HEX);
  Serial.print(F("Version ID:\t0x"));
  Serial.println(sysInfo->versionId, HEX);
  Serial.println(F(""));
}

void printHidDescriptor(HIDDescriptor_t * hd)
{
  Serial.print(F("Hid Descriptor Length: \t"));
  Serial.println(hd->wHIDDescLength);
  Serial.print(F("bcd Version\t\t0x"));
  Serial.println(hd->bcdVersion, HEX);
  Serial.print(F("Report Desc Length\t"));
  Serial.println(hd->wReportDescLength);
  Serial.print(F("Report Desc Register\t"));
  Serial.println(hd->wReportDescRegister);
  Serial.print(F("Input Register\t\t"));
  Serial.println(hd->wInputRegister);
  Serial.print(F("Max Input Length\t"));
  Serial.println(hd->wMaxInputLength);
  Serial.print(F("Output Register\t\t"));
  Serial.println(hd->wOutputRegister);
  Serial.print(F("Max Output Length\t"));
  Serial.println(hd->wMaxOutputLength);
  Serial.print(F("Command Register\t"));
  Serial.println(hd->wCommandRegister);
  Serial.print(F("Data Register\t\t"));
  Serial.println(hd->wDataRegister);
  Serial.print(F("VID\t\t0x"));
  Serial.println(hd->wVendorID, HEX);
  Serial.print(F("PID\t\t0x"));
  Serial.println(hd->wProductID, HEX);
  Serial.print(F("Version\t\t0x"));
  Serial.println(hd->wVersionID, HEX);
}

/** Prints the information stored in a HID_report_t struct to serial */
void printDataReport(uint8_t i2c_channel, HID_report_t * report)
{
  //Use reportID to determine how to decode the report
  switch(report->reportID)
  {
    case MOUSE_REPORT_ID:
        printMouseReport(i2c_channel, report);
        break;
    case PTP_REPORT_ID:
        printPtpReport(i2c_channel, report);
        break;
    default:
        Serial.println(F("Error: Unknown Report ID"));
  }
}

/** Prints the information stored in a mouse report to serial */
void printMouseReport(uint8_t i2c_channel, HID_report_t* report)
{
  char strBuf[50];
  sprintf(strBuf,"I2C_Chan %d -> ",i2c_channel);
  Serial.print(strBuf);
  sprintf(strBuf,"ReportID: 0x%02X",report->reportID);
  Serial.print(strBuf);
  sprintf(strBuf,", Buttons: %c%c%c",((report->mouse.buttons & 1)?'L':'_'),((report->mouse.buttons & 4)?'C':'_'),((report->mouse.buttons & 2)?'R':'_'));
  // sprintf(strBuf,"Buttons: 0b%03b",report->mouse.buttons);
  Serial.print(strBuf);
  sprintf(strBuf,", X_Delta: %4d",report->mouse.xDelta);
  Serial.print(strBuf);
  sprintf(strBuf,", Y_Delta: %4d",report->mouse.yDelta);
  Serial.print(strBuf);
  sprintf(strBuf,", Scroll Delta: %2d",report->mouse.scrollDelta);
  Serial.print(strBuf);
  if (report->reportLength >= 8)
  {
    sprintf(strBuf,", Pan Delta: %3d",report->mouse.panDelta);
    Serial.print(strBuf);
  }
  Serial.println();
}

void printPtpReport(uint8_t i2c_channel, HID_report_t * report)
{
  char strBuf[50];
  sprintf(strBuf,"I2C_Chan %d -> ",i2c_channel);
  Serial.print(strBuf);
  sprintf(strBuf,"ReportID: 0x%02X",report->reportID);
  Serial.print(strBuf);
  // Serial.print(report->reportID, HEX);
  sprintf(strBuf,", Time: %5d",report->ptp.timeStamp);
  Serial.print(strBuf);
  sprintf(strBuf,", ContactID: %d",report->ptp.contactID);
  Serial.print(strBuf);
  sprintf(strBuf,", Confidence: %d",report->ptp.confidence); 
  Serial.print(strBuf);
  sprintf(strBuf,", Tip: %d",report->ptp.tip);
  Serial.print(strBuf);
  sprintf(strBuf,", X: %4d",report->ptp.x);
  Serial.print(strBuf);
  sprintf(strBuf,", Y: %4d",report->ptp.y);
  Serial.print(strBuf);
  sprintf(strBuf,", Buttons: %d",report->ptp.buttons);
  Serial.print(strBuf);
  sprintf(strBuf,", Contact Count: %d",report->ptp.contactCount); //Total number of contacts to be reported in a given report
  Serial.print(strBuf);
  Serial.println();
   
}

/**************************************************************/
/*************** FUNCTIONS FOR PRINTING EVENTS ****************/

/** @ingroup prevReports 
    These reports store the state of the most recent reports by type.
    Determining if an event occurred requires information about the previous state. 
*/
HID_report_t prevMouseReport_g;     /**< Most recent past Mouse report */
HID_report_t prevPtpReport_g;

/** Prints all the events that correspond to cur_report.
    Determines which printing function to use from the reportID */
void printEvent(uint8_t i2c_channel, HID_report_t* cur_report)
{
  switch( cur_report->reportID)
  {
    case MOUSE_REPORT_ID:
        printMouseReportEvents(i2c_channel, cur_report, &prevMouseReport_g); 
        prevMouseReport_g = *cur_report;
        break;
    case PTP_REPORT_ID:
        printPtpReportEvents(i2c_channel, cur_report, &prevPtpReport_g);
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
void printMouseReportEvents(uint8_t i2c_channel, HID_report_t* cur_report, HID_report_t* prev_report)
{
    if(prev_report->reportID == MOUSE_REPORT_ID)
    {
        printButtonEvents(i2c_channel, cur_report, prev_report);
    }
}

void printPtpReportEvents(uint8_t i2c_channel, HID_report_t * cur_report, HID_report_t * prev_report)
{
    if(prev_report->reportID == PTP_REPORT_ID)
    {
        printButtonEvents(i2c_channel, cur_report, prev_report);
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
        case PTP_REPORT_ID:
            return report->ptp.buttons;
        default:
            return 0;
    }
}

/** Determines and prints if a button event (press or release) occurred between cur_report and prev_report. 
    Demonstrates use of the API_C3_isButtonPressed function. */
void printButtonEvents(uint8_t i2c_channel, HID_report_t* cur_report, HID_report_t* prev_report)
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
            Serial.printf("I2C_Chan %d -> ", i2c_channel);
            Serial.printf("Button %d ", i);
            if (API_C3_isButtonPressed(cur_report, mask))
            {
                Serial.println(" Pressed");
            }
            else
            {
                Serial.println(" Released");
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
    prevMouseReport_g.reportID = MOUSE_REPORT_ID;
    prevMouseReport_g.mouse.buttons = 0x0;

    prevPtpReport_g.reportID = PTP_REPORT_ID;
    prevPtpReport_g.ptp.buttons = 0x0;
}

void printCompMatrix(uint8_t i2c_channel)
{
  // This shows how to read the "compensation matrix" from the touchpad
  uint8_t sizeX, sizeY;
  uint16_t compNumberBytes;
  // allocate the largest possible comp image, once you know the 
  // comp size this could be made a smaller size (or dynamically allocated if your
  // language supports it)
  int16_t compImage[30 * 16]; 
  uint16_t index;
  // all of the sensorSize information will be fixed/consistent for a given project
  // once you know the sizeX, sizeY, and compNumberBytes you could just hard-code
  // all this and not bother to call this function
  API_C3_sensorSize(i2c_channel, &sizeX, &sizeY, &compNumberBytes);
  Serial.print(F("I2C Channel = "));
  Serial.print(i2c_channel);
  Serial.print(", ");
  Serial.print(F("Sensor X,Y = "));
  Serial.print(sizeX);
  Serial.print(",");
  Serial.print(sizeY);
  Serial.print(" bytes = ");
  Serial.println(compNumberBytes);
  // The compImage is a 1D array (row major ordered) that can be easily broken out into a 2D array
  API_C3_readComp(i2c_channel, compImage, compNumberBytes, 64);
  index = 0;
  while (index < (compNumberBytes / 2))
  {
    for (int x = 0; x < sizeX; x++)
    {
      Serial.print(compImage[index++]);
      Serial.print(",");
    }
    Serial.println();
  }
}

uint8_t i2cPing(uint8_t channel, uint8_t i2cAddr)
{
  uint8_t error;

  // The i2c_scanner uses the return value of
  // the Write.endTransmisstion to see if
  // a device did acknowledge being addressed.
  I2C_beginTransmission(channel, i2cAddr);
  error = I2C_endTransmission(channel, true);

  if (error == 0)
  {
    Serial.print("I2C device responded at address 0x");
    if (i2cAddr<16) 
      Serial.print("0");
    Serial.print(i2cAddr,HEX);
    Serial.println("  !");
  }
  else 
  {
    Serial.print("I2C error occurred at address 0x");
    if (i2cAddr<16) 
      Serial.print("0");
    Serial.print(i2cAddr,HEX);
    Serial.print(": ");
    Serial.println(error);
  }    

  return error;
}