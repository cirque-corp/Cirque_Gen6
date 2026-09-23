// use a better I2C library that allows larger buffer sizes
#include <i2c_device.h>
#include <i2c_driver.h>
#include <i2c_driver_wire.h>
#include <i2c_register_slave.h>

#include <i2c_device.h>
#include <i2c_driver.h>
#include <i2c_driver_wire.h>
#include <i2c_register_slave.h>

// use the cirque demo code library
#include <CirqueHid.h>
#include <DataUtils.h>
#include <Cirque.h> // if the library is installed from Library Manager you might not need this
#include <Teensy4_HostBusLayer.h>

// Create a specific Host Bus object (Teensy4_HostBusLayer works on the blue board). 
// The global variable HostBus will become it. All the common code then uses HostBus.
Teensy4_HostBusLayer teensyHostBus;

// Create the common protocol object (that handle HID, PTP, and Cirque commands)
CirqueHid cirqueHid(0x2C, 535); // Address 0x2C, make sure the buffer is at least 535 bytes

// Create a few helper objects that cirqueHid will need
HidDescriptor hidDescriptor;
HidReport hidReport;

// PTP can send multiple fingers per report, and multiple reports per sensor image snapshot (frame)
// This is the data needed to organize those reports into the status of each finger
#define TOTAL_FINGERS 5
PtpFingerData_t fingerData[TOTAL_FINGERS]; // holds the PTP report data, organized by Finger ID (0..4)
bool fingerDataReady[TOTAL_FINGERS];  // a flag that helps track the data as it's being organized
bool readingTwoReports = false;  // typically, all the finger data can span 2 PTP reports, this variable helps track that state

// Keyboard state tracking
keyReport_t prevKeyboardReport = {0, 0, {0, 0, 0, 0, 0, 0}}; // Track previous keyboard state for events

// state data for the command loop (using key presses)
bool enableContactReports = true;
bool enableButtonReports = true;
bool enableDataPrinting = true;

void setup() {
  delay(30);
  // put your setup code here, to run once:
  Serial.begin(115200);

  // Initialize the HostBus layer
  HostBusLayer::initError err = HostBus.init(400000, 550); // 400,000 Hz, 550 byte buffer
  if (err > HostBusLayer::initError::initOkay)
  {
    Serial.print(F("Failed to init host bus: "));
    Serial.print((int)err);
  }

  // The demo board has a power switch, turn it on
  turnOnPower();
  // wait for the device to fully power on and report it is ready
  waitForHidResetResponse();
  // device will now be ready to operate
  identifyDevice();
  readDefaults();

  // prepare the PTP report tracking data
  for (int x = 0; x < TOTAL_FINGERS; x++)
  {
    fingerData[x].contactID = x;
    fingerData[x].confidence = 0;
    fingerData[x].tip = 0;
    fingerData[x].x = 0;
    fingerData[x].y = 0;

    fingerDataReady[x] = false;
  }

  Serial.println(F("'h' or '?' - help"));
}

void loop() {
  // put your main code here, to run repeatedly:

  // service DR
  if (HostBus.drAsserted())
  {
    // Let the library decode the report normally
    cirqueHid.getReport(hidReport);
    
    printHidReport(hidReport);
  }

  // Various "key presses" will trigger commands that change the operation of the device
  processKeys();
}

void processKeys(void)
{
  // check for command key
  if(Serial.available())
  {
    char rxChar = Serial.read();
    switch(rxChar)
    {
      case 'h':
      case '?':
        showHelp();
        break;
      case 'm':
        Serial.println(F("Changed to PTP reporting..."));
        cirqueHid.setInputMode(true);
        break;
      case 'M':
        Serial.println(F("Changed to Mouse reporting..."));
        cirqueHid.setInputMode(false);
        break;
      case 'p':
        Serial.println(F("Hid Set Power off..."));
        cirqueHid.setPower(false);
        break;
      case 'P':
        Serial.println(F("Hid Set Power on..."));
        cirqueHid.setPower(true);
        break;
      case 'r':
        Serial.println(F("Stop reporting contacts..."));
        enableContactReports = false;
        cirqueHid.setSelectiveReporting(enableContactReports, enableButtonReports);
        break;
      case 'R':
        Serial.println(F("Start reporting contacts..."));
        enableContactReports = true;
        cirqueHid.setSelectiveReporting(enableContactReports, enableButtonReports);
        break;
      case 'b':
        Serial.println(F("Stop reporting buttons..."));
        enableButtonReports = false;
        cirqueHid.setSelectiveReporting(enableContactReports, enableButtonReports);
        break;
      case 'B':
        Serial.println(F("Start reporting buttons..."));
        enableButtonReports = true;
        cirqueHid.setSelectiveReporting(enableContactReports, enableButtonReports);
        break;
      case 'w':  // warm boot
        Serial.println(F("warm boot..."));
        cirqueHid.reset();
        break;
      case 'g':
        uint8_t numberContacts;
        CirqueHid::PTP_ButtonImplementation buttonImplementation;
        cirqueHid.getDeviceCapabilities(numberContacts, buttonImplementation);
        Serial.printf("getDeviceCapabilities  Number Contacts: %d  buttonImplementation: %d", numberContacts, buttonImplementation);
        break;
      case 'x':
        Serial.print(F("Set Non-Invert X: "));
        readModifyWriteRegister(0x20080018, 0, 1);
        break;
      case 'X':
        Serial.print(F("Set Invert X: "));
        readModifyWriteRegister(0x20080018, 1, 0);
        break;
      case 'y':
        Serial.print(F("Set Non-Invert Y: "));
        readModifyWriteRegister(0x20080018, 0, 2);
        break;
      case 'Y':
        Serial.print(F("Set Invert Y: "));
        readModifyWriteRegister(0x20080018, 2, 0);
        break;
      case 's':
        Serial.println(F("Unswap X and Y: "));
        readModifyWriteRegister(0x20080018, 0, 4);
        break;
      case 'S':
        Serial.println(F("Swap X and Y: "));
        readModifyWriteRegister(0x20080018, 4, 0);
        break;
      case 'i':
        {
          uint8_t registerValue = 2;
          Serial.println(F("Cancel Force-Sleep"));
          cirqueHid.writeExtendedMemory(0x200a0408, &registerValue , 1);
        }
        break;
      case 'I':
        {
          uint8_t registerValue = 1;
          Serial.println(F("Force-Sleep"));
          cirqueHid.writeExtendedMemory(0x200a0408, &registerValue , 1);
        }
        break;
      case '$' :
        // restart everything, this will power cycle the touchpad
        HostBus.setPower(false);
        delay(100);
        setup();
        break;
      case 'd':
        Serial.println(F("Data Printing Disabled"));
        enableDataPrinting = false;
        break;
      case 'D':
        Serial.println(F("Data Printing Enabled"));
        enableDataPrinting = true;
        break;
      case 'o':
        Serial.println(F("System Information:"));
        identifyDevice();
        readDefaults();
        break;
      default:
        break;
    }
  }
}

void showHelp(void)
{
  Serial.println(F("Available Commands (case sensitive)"));
  Serial.println(F(""));
  Serial.println(F("--- Report Modes ---"));
  Serial.println(F("  m - issue PTP reports"));
  Serial.println(F("  M - issue Mouse reports"));
  Serial.println(F(""));
  Serial.println(F("--- Power Control ---"));
  Serial.println(F("  p - HID power off"));
  Serial.println(F("  P - HID power on"));
  Serial.println(F("  w - warm boot (reset)"));
  Serial.println(F("  i - cancel force-sleep"));
  Serial.println(F("  I - force sleep"));
  Serial.println(F("  $ - physical power off, then on"));
  Serial.println(F(""));
  Serial.println(F("--- Reporting Control ---"));
  Serial.println(F("  r - don't report contacts"));
  Serial.println(F("  R - report contacts"));
  Serial.println(F("  b - don't report buttons"));
  Serial.println(F("  B - report buttons"));
  Serial.println(F("  d - data printing off"));
  Serial.println(F("  D - data printing on"));
  Serial.println(F(""));
  Serial.println(F("--- Axis Control ---"));
  Serial.println(F("  x - don't invert x-axis"));
  Serial.println(F("  X - invert x-axis"));
  Serial.println(F("  y - don't invert y-axis"));
  Serial.println(F("  Y - invert y-axis"));
  Serial.println(F("  s - unswap x-y"));
  Serial.println(F("  S - swap x-y"));
  Serial.println(F(""));
  Serial.println(F("--- Information ---"));
  Serial.println(F("  g - get device capabilities"));
  Serial.println(F("  o - show system information"));
  Serial.println(F("  h, ? - print this help"));
  Serial.println(F(""));
}

void turnOnPower(void)
{
  uint8_t rail3v3_percent, rail5v0_percent;
  bool currentOkay = true;
  bool systemAwake = false;

  // Power on and wait for the 3.3V rail to come up and the system to start initialzing
  // A deluxe HostBusLayer can control power, sense "over current", and even sense the voltage
  // of the power rails. This helps make the timing adaptive to the hardware design.
  // Generally, most systems aren't that deluxe, so you can replace this code with a delay of
  // 50 msec at power on
  HostBus.setPower(true);  // turns on the 3.3V rail (and the 5.0V rail)
  do
  {
    HostBus.readSupplyVoltages(rail3v3_percent, rail5v0_percent);
    currentOkay &= !HostBus.readOverCurrent(); // watch for overcurrent
    systemAwake |= !HostBus.drAsserted();  // watch for DR to deassert. That signals that the system is starting to init
  } while ((rail3v3_percent < 80) && (currentOkay) && (!systemAwake));

  if (!currentOkay)
  {
    Serial.println(F("Overcurrent at power on."));
  }
  else
  {
    Serial.println(F("Power ready"));
  }
}

void waitForHidResetResponse(void)
{
  Serial.print(F("Waiting for reset response..."));
  elapsedMillis timer = 0;
  while (!HostBus.drAsserted())
  {
    if (timer > 250)
    {
      // every 250 msec print a "."
      // likely the device wasn't attached
      Serial.print(F("."));
      timer = 0;
    }
  }
  cirqueHid.getReport(hidReport);
  if (hidReport.length == 0)
  {
    Serial.println(F("Received HID Reset Response"));
  }
  else
  {
    Serial.println(F("Received something else, not the expected HID Reset Response"));
  }
}

void identifyDevice(void)
{
  cirqueHid.getHidDescriptor(hidDescriptor);
  Serial.printf("VID : 0x%04x  PID : 0x%04x  Version : %d\n", 
    hidDescriptor.wVendorID, hidDescriptor.wProductID, hidDescriptor.VersionID);
  Serial.printf("Report Descriptor Reg : 0x%04x  Report Descriptor Length : 0x%04x\n",
    hidDescriptor.wReportDescRegister, hidDescriptor.wReportDescLength);
  Serial.printf("Input Reg : 0x%04x  Max Input Length : 0x%04x\n", 
    hidDescriptor.wInputRegister, hidDescriptor.wMaxInputLength);
  Serial.printf("Output Reg : 0x%04x  Max Output Length : 0x%04x\n", hidDescriptor.wOutputRegister, hidDescriptor.wMaxOutputLength);
  Serial.printf("Command Reg : 0x%04x  Data Reg : 0x%04x\n", 
    hidDescriptor.wCommandRegister, hidDescriptor.wDataRegister);

  // hidDescriptor.BCD
}

void printHidReport(HidReport & report)
{
  switch (report.reportId)
  {
    case id_ptpReport :
      if (report.report.ptp.numberFingers > MAX_PTP_FINGER_COUNT)
      {
        // it will take reading two reports to get all the data
        readingTwoReports = true;
      }
      else if (readingTwoReports)
      {
        readingTwoReports = false;
      }
      
      // IMPORTANT: Clear all finger data at the start of each PTP report
      // This ensures inactive fingers don't persist with stale coordinates
      if (!readingTwoReports)
      {
        for (int x = 0; x < TOTAL_FINGERS; x++)
        {
          fingerData[x].contactID = x;
          fingerData[x].confidence = 0;
          fingerData[x].tip = 0;
          fingerData[x].x = 0;
          fingerData[x].y = 0;
        }
      }
      
      // organize data - populate only fingers present in this report
      for (int x = 0; x < MAX_PTP_FINGER_COUNT; x++)
      {
        PtpFingerData_t * finger = &report.report.ptp.fingers[x];
        int index = finger->contactID;
        if ((index < TOTAL_FINGERS) && (!fingerDataReady[index]))
        {
          fingerData[index].confidence = finger->confidence;
          fingerData[index].tip = finger->tip;
          fingerData[index].x = finger->x;
          fingerData[index].y = finger->y;

          fingerDataReady[index] = true;
        }
      }
      if (!readingTwoReports)
      {
        // display data
        Serial.printf("PTP  T: %d  C: %d  B: %d  ", report.report.ptp.timeStamp, report.report.ptp.contactCount, report.report.ptp.buttons);
        for (int x = 0; x < TOTAL_FINGERS; x++)
        {
          Serial.printf("F%d  X:%4d  Y:%4d  Conf: %d  Tip: %d  ", 
            fingerData[x].contactID, fingerData[x].x, fingerData[x].y, 
            fingerData[x].confidence, fingerData[x].tip);

          fingerDataReady[x] = false;
        }
        Serial.println();
      }
    break;
    case id_mouseReport :
      Serial.printf("Mouse  dX : %4d  dY: %4d  Btn : %2d  dS : %4d  dP : %4d\n", 
        report.report.mouse.xDelta, report.report.mouse.yDelta, 
        report.report.mouse.buttons, report.report.mouse.scrollDelta,
        report.report.mouse.panDelta);
    break;
    case id_keyReport : 
      printKeyboardReport(report);
      // Note: Event tracking disabled - only report keyboard state
      // printKeyboardReportEvents(report);
    break;
    default :
    break;
  }
}

void readDefaults(void)
{
  // read any registers that you want to peek at the default values...
  uint8_t registerValue;

  cirqueHid.readExtendedMemory(0x20080018, &registerValue, 1);
  Serial.printf("XY Config (0x20080018): 0x%02x\n", registerValue);

  cirqueHid.readExtendedMemory(0x200a0408, &registerValue, 1);
  Serial.printf("Power Control (0x200A0408): 0x%02x\n", registerValue);

}

void readModifyWriteRegister(uint32_t address, uint8_t bitsToSet, uint8_t bitsToClear)
{
  uint8_t before, after;
  // read initial value of register
  cirqueHid.readExtendedMemory(address, &before, 1);
  after = before | bitsToSet;  // set any bits you need
  after &= ~bitsToClear;  // mask off (clear) and bits you don't need
  Serial.printf("0x%2x to 0x%2x\n", before, after);
  // write the result to the register
  cirqueHid.writeExtendedMemory(address, &after, 1);
}

/**************************************************************/
/*********** KEYBOARD PRINTING AND EVENT TRACKING ************/

/** Prints the information stored in a keyboard report to serial */
void printKeyboardReport(HidReport & report)
{
  Serial.print(F("Keyboard  ReportID: 0x"));
  Serial.print(report.reportId, HEX);
  Serial.print(F(", Length: "));
  Serial.print(report.length);
  Serial.print(F(", Modifiers: 0x"));
  Serial.print(report.report.keyboard.modifier1, HEX);
  Serial.print(F(" ("));
  printModifierNames(report.report.keyboard.modifier1);
  Serial.print(F("), Raw Keycodes: "));
  for (uint8_t i = 0; i < 6; i++)
  {
    Serial.printf("0x%02X ", report.report.keyboard.keycode[i]);
    if(report.report.keyboard.keycode[i] != 0)
    {
      Serial.print(F("("));
      printKeycodeName(report.report.keyboard.keycode[i]);
      Serial.print(F(") "));
    }
  }
  Serial.println();
}

/** Prints modifier key names based on modifier byte */
void printModifierNames(uint8_t modifiers)
{
  bool anyModifier = false;
  
  if (modifiers & 0x01) { Serial.print(F("L_CTRL(0x01) ")); anyModifier = true; }
  if (modifiers & 0x02) { Serial.print(F("L_SHIFT(0x02) ")); anyModifier = true; }
  if (modifiers & 0x04) { Serial.print(F("L_ALT(0x04) ")); anyModifier = true; }
  if (modifiers & 0x08) { Serial.print(F("L_GUI(0x08) ")); anyModifier = true; }
  if (modifiers & 0x10) { Serial.print(F("R_CTRL(0x10) ")); anyModifier = true; }
  if (modifiers & 0x20) { Serial.print(F("R_SHIFT(0x20) ")); anyModifier = true; }
  if (modifiers & 0x40) { Serial.print(F("R_ALT(0x40) ")); anyModifier = true; }
  if (modifiers & 0x80) { Serial.print(F("R_GUI(0x80) ")); anyModifier = true; }
  
  if (!anyModifier) { Serial.print(F("NONE(0x00) ")); }
}

/** Prints keycode name based on USB HID keycode */
void printKeycodeName(uint8_t keycode)
{
  char strBuf[30];
  
  // Alphanumeric keys (0x04-0x1D = A-Z, 0x1E-0x27 = 1-0)
  if (keycode >= 0x04 && keycode <= 0x1D) {
    sprintf(strBuf, "%c(0x%02X)", (keycode == 0x04 ? 'A' : 'A' + (keycode - 0x04)), keycode);
    Serial.print(strBuf);
  }
  else if (keycode >= 0x1E && keycode <= 0x27) {
    uint8_t num = (keycode == 0x27 ? 0 : 1 + (keycode - 0x1E));
    sprintf(strBuf, "%d(0x%02X)", num, keycode);
    Serial.print(strBuf);
  }
  // Special keys
  else {
    const char* name = "";
    switch(keycode) {
      case 0x28: name = "ENTER"; break;
      case 0x29: name = "ESC"; break;
      case 0x2A: name = "BACKSPACE"; break;
      case 0x2B: name = "TAB"; break;
      case 0x2C: name = "SPACE"; break;
      case 0x2D: name = "MINUS"; break;
      case 0x2E: name = "EQUALS"; break;
      case 0x2F: name = "L_BRACKET"; break;
      case 0x30: name = "R_BRACKET"; break;
      case 0x31: name = "BACKSLASH"; break;
      case 0x33: name = "SEMICOLON"; break;
      case 0x34: name = "APOSTROPHE"; break;
      case 0x35: name = "BACKTICK"; break;
      case 0x36: name = "COMMA"; break;
      case 0x37: name = "PERIOD"; break;
      case 0x38: name = "SLASH"; break;
      case 0x39: name = "CAPS_LOCK"; break;
      case 0x3A: name = "F1"; break;
      case 0x3B: name = "F2"; break;
      case 0x3C: name = "F3"; break;
      case 0x3D: name = "F4"; break;
      case 0x3E: name = "F5"; break;
      case 0x3F: name = "F6"; break;
      case 0x40: name = "F7"; break;
      case 0x41: name = "F8"; break;
      case 0x42: name = "F9"; break;
      case 0x43: name = "F10"; break;
      case 0x44: name = "F11"; break;
      case 0x45: name = "F12"; break;
      case 0x46: name = "PRINT_SCREEN"; break;
      case 0x47: name = "SCROLL_LOCK"; break;
      case 0x48: name = "PAUSE"; break;
      case 0x49: name = "INSERT"; break;
      case 0x4A: name = "HOME"; break;
      case 0x4B: name = "PAGE_UP"; break;
      case 0x4C: name = "DELETE"; break;
      case 0x4D: name = "END"; break;
      case 0x4E: name = "PAGE_DOWN"; break;
      case 0x4F: name = "RIGHT"; break;
      case 0x50: name = "LEFT"; break;
      case 0x51: name = "DOWN"; break;
      case 0x52: name = "UP"; break;
      case 0x53: name = "NUM_LOCK"; break;
      case 0x54: name = "KP_DIVIDE"; break;
      case 0x55: name = "KP_MULTIPLY"; break;
      case 0x56: name = "KP_MINUS"; break;
      case 0x57: name = "KP_PLUS"; break;
      case 0x58: name = "KP_ENTER"; break;
      case 0x59: name = "KP_1"; break;
      case 0x5A: name = "KP_2"; break;
      case 0x5B: name = "KP_3"; break;
      case 0x5C: name = "KP_4"; break;
      case 0x5D: name = "KP_5"; break;
      case 0x5E: name = "KP_6"; break;
      case 0x5F: name = "KP_7"; break;
      case 0x60: name = "KP_8"; break;
      case 0x61: name = "KP_9"; break;
      case 0x62: name = "KP_0"; break;
      case 0x63: name = "KP_PERIOD"; break;
      default: sprintf(strBuf, "UNKNOWN(0x%02X)", keycode); Serial.print(strBuf); return;
    }
    sprintf(strBuf, "%s(0x%02X)", name, keycode);
    Serial.print(strBuf);
  }
}

/** Prints keyboard events by comparing current and previous reports */
void printKeyboardReportEvents(HidReport & report)
{
    // Check for modifier changes
    if (report.report.keyboard.modifier1 != prevKeyboardReport.modifier1 || 
        report.report.keyboard.modifier2 != prevKeyboardReport.modifier2)
    {
        Serial.print(F("  Event: Modifier change - "));
        printModifierNames(report.report.keyboard.modifier1);
        Serial.print(F("(was "));
        printModifierNames(prevKeyboardReport.modifier1);
        Serial.println(F(")"));
    }
    
    // Check for new key presses
    for (uint8_t i = 0; i < 6; i++)
    {
        if (report.report.keyboard.keycode[i] != 0)
        {
            bool foundInPrevious = false;
            for (uint8_t j = 0; j < 6; j++)
            {
                if (report.report.keyboard.keycode[i] == prevKeyboardReport.keycode[j])
                {
                    foundInPrevious = true;
                    break;
                }
            }
            if (!foundInPrevious)
            {
                Serial.print(F("  Event: Key pressed - "));
                printKeycodeName(report.report.keyboard.keycode[i]);
                Serial.println();
            }
        }
    }
    
    // Check for key releases
    for (uint8_t i = 0; i < 6; i++)
    {
        if (prevKeyboardReport.keycode[i] != 0)
        {
            bool foundInCurrent = false;
            for (uint8_t j = 0; j < 6; j++)
            {
                if (prevKeyboardReport.keycode[i] == report.report.keyboard.keycode[j])
                {
                    foundInCurrent = true;
                    break;
                }
            }
            if (!foundInCurrent)
            {
                Serial.print(F("  Event: Key released - "));
                printKeycodeName(prevKeyboardReport.keycode[i]);
                Serial.println();
            }
        }
    }
    
    // Update previous state for next comparison
    prevKeyboardReport = report.report.keyboard;
}
