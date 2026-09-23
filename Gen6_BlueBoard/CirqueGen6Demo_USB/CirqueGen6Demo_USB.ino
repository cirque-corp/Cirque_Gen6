// USB Mouse and Keyboard Demo with Cirque Gen6 PTP Trackpad
//   Teensy becomes a USB mouse/keyboard and responds to trackpad touch events
//   You must select "Serial + Keyboard + Mouse + Joystick" from Tools > USB Type menu
//
// Based on CirqueGen6Demo with USB HID support added

#if !defined(MOUSE_INTERFACE) || !defined(KEYBOARD_INTERFACE)
#error "Please select 'Serial + Keyboard + Mouse + Joystick' or similar from Tools > USB Type menu to enable Mouse and Keyboard interfaces"
#endif

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
#include <Cirque.h>
#include <Teensy4_HostBusLayer.h>
#include <USB_Keycodes.h>

// Create a specific Host Bus object (Teensy4_HostBusLayer works on the blue board)
Teensy4_HostBusLayer teensyHostBus;

// Create the common protocol object (that handle HID, PTP, and Cirque commands)
CirqueHid cirqueHid(0x2C, 535); // Address 0x2C, make sure the buffer is at least 535 bytes

// Create a few helper objects that cirqueHid will need
HidDescriptor hidDescriptor;
HidReport hidReport;

// PTP can send multiple fingers per report, and multiple reports per sensor image snapshot (frame)
#define TOTAL_FINGERS 5
PtpFingerData_t fingerData[TOTAL_FINGERS];
bool fingerDataReady[TOTAL_FINGERS];
bool readingTwoReports = false;

// USB Mouse state tracking
uint16_t prevPtpX = 0, prevPtpY = 0;
bool prevPtpSetFlag = false;
bool prevButtonPressed = false;

// Keyboard state tracking
keyReport_t prevKeyboardReport = {0, 0, {0, 0, 0, 0, 0, 0}};

// state data for the command loop
bool enableContactReports = true;
bool enableButtonReports = true;
bool enableDataPrinting = true;
bool enableRawPacketPrint = false;  // Toggle for raw packet hex dump

// Feed control register definitions
#define REG_FEED_CONFIG3              (0x200E000A)
#define FC3_PS2_FEED_ENABLE           (0x01)
#define FC3_I2C_FEED_ENABLE           (0x02)
#define FC3_USB_FEED_ENABLE           (0x04)

void setup() {
  delay(30);
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

  Serial.println(F("\nCirque Gen6 USB Mouse + Keyboard Demo"));
  Serial.println(F("'h' or '?' - help"));
}

void loop() {
  // service DR
  if (HostBus.drAsserted())
  {
    // Let the library decode the report normally
    cirqueHid.getReport(hidReport);
    
    // Output report to serial
    if (enableDataPrinting)
    {
      printHidReport(hidReport);
    }
    
    // Send USB HID output
    performHidOutput(hidReport);
  }

  // Various "key presses" will trigger commands that change the operation of the device
  processKeys();
}

void performHidOutput(HidReport & report)
{
  switch (report.reportId)
  {
    case id_ptpReport:
      performPtpMouseOutput(report);
      break;
    case id_mouseReport:
      performMouseOutput(report);
      break;
    case id_keyReport:
      performKeyboardOutput(report);
      break;
    default:
      break;
  }
}

void performMouseOutput(HidReport & report)
{
  // Handle relative mouse reports
  Mouse.move(report.report.mouse.xDelta, report.report.mouse.yDelta);
  
  // Handle mouse buttons
  bool buttonPressed = (report.report.mouse.buttons & 0x01) != 0;
  if (!prevButtonPressed && buttonPressed)
  {
    Mouse.press();
  }
  else if (prevButtonPressed && !buttonPressed)
  {
    Mouse.release();
  }
  prevButtonPressed = buttonPressed;
}

void performPtpMouseOutput(HidReport & report)
{
  // Convert PTP absolute coordinates to relative mouse movement
  // Skip first frame to establish baseline
  if (!prevPtpSetFlag && report.report.ptp.fingers[0].tip)
  {
    prevPtpX = report.report.ptp.fingers[0].x;
    prevPtpY = report.report.ptp.fingers[0].y;
    prevPtpSetFlag = true;
  }
  else if (!report.report.ptp.fingers[0].tip)
  {
    prevPtpSetFlag = false;
  }
  else if (report.report.ptp.fingers[0].confidence)
  {
    int16_t deltaX = report.report.ptp.fingers[0].x - prevPtpX;
    int16_t deltaY = report.report.ptp.fingers[0].y - prevPtpY;
    
    Mouse.move(deltaX, deltaY);
    
    prevPtpX = report.report.ptp.fingers[0].x;
    prevPtpY = report.report.ptp.fingers[0].y;
    
    // Handle button from PTP report
    bool buttonPressed = (report.report.ptp.buttons & 0x01) != 0;
    if (!prevButtonPressed && buttonPressed)
    {
      Mouse.press();
    }
    else if (prevButtonPressed && !buttonPressed)
    {
      Mouse.release();
    }
    prevButtonPressed = buttonPressed;
  }
}

void performKeyboardOutput(HidReport & report)
{
  // Set keyboard modifiers
  Keyboard.set_modifier(report.report.keyboard.modifier1 | (report.report.keyboard.modifier2 << 8));
  
  // Set keycodes (up to 6 simultaneous)
  Keyboard.set_key1(report.report.keyboard.keycode[0]);
  Keyboard.set_key2(report.report.keyboard.keycode[1]);
  Keyboard.set_key3(report.report.keyboard.keycode[2]);
  Keyboard.set_key4(report.report.keyboard.keycode[3]);
  Keyboard.set_key5(report.report.keyboard.keycode[4]);
  Keyboard.set_key6(report.report.keyboard.keycode[5]);
  
  // Send the report
  Keyboard.send_now();
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
        {
          uint8_t numberContacts;
          CirqueHid::PTP_ButtonImplementation buttonImplementation;
          cirqueHid.getDeviceCapabilities(numberContacts, buttonImplementation);
          Serial.printf("getDeviceCapabilities  Number Contacts: %d  buttonImplementation: %d", numberContacts, buttonImplementation);
        }
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
      case 'z':
        enableRawPacketPrint = !enableRawPacketPrint;
        if (enableRawPacketPrint) {
          Serial.println(F("Raw Packet Hex Dump Enabled"));
        } else {
          Serial.println(F("Raw Packet Hex Dump Disabled"));
        }
        break;
      case 'o':
        Serial.println(F("System Information:"));
        identifyDevice();
        readDefaults();
        break;
      case 'f':
        Serial.println(F("Feed Enabled"));
        cirque_enableFeed();
        break;
      case 'F':
        Serial.println(F("Feed Disabled"));
        cirque_disableFeed();
        break;
      default:
        break;
    }
  }
}

void showHelp(void)
{
  Serial.println(F("Cirque Gen6 USB Mouse + Keyboard Demo"));
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
  Serial.println(F("  f - enable feed"));
  Serial.println(F("  F - disable feed"));
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
  Serial.println(F("  z - toggle raw packet hex dump"));
  Serial.println(F("  h, ? - print this help"));
  Serial.println(F(""));
}

void turnOnPower(void)
{
  uint8_t rail3v3_percent, rail5v0_percent;
  bool currentOkay = true;
  bool systemAwake = false;

  HostBus.setPower(true);  // turns on the 3.3V rail (and the 5.0V rail)
  do
  {
    HostBus.readSupplyVoltages(rail3v3_percent, rail5v0_percent);
    currentOkay &= !HostBus.readOverCurrent(); // watch for overcurrent
    systemAwake |= !HostBus.drAsserted();  // watch for DR to deassert
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
}

void printHidReport(HidReport & report)
{
  switch (report.reportId)
  {
    case id_ptpReport :
      if (report.report.ptp.numberFingers > MAX_PTP_FINGER_COUNT)
      {
        readingTwoReports = true;
      }
      else if (readingTwoReports)
      {
        readingTwoReports = false;
      }
      
      // Clear all finger data at the start of each PTP report
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
    break;
    default :
    break;
  }
  
  // Print raw packet if enabled
  if (enableRawPacketPrint)
  {
    report.dumpRawPacket();
  }
}

void readDefaults(void)
{
  uint8_t registerValue;
  cirqueHid.readExtendedMemory(0x20080018, &registerValue, 1);
  Serial.printf("XY Config (0x20080018): 0x%02x\n", registerValue);

  cirqueHid.readExtendedMemory(0x200a0408, &registerValue, 1);
  Serial.printf("Power Control (0x200A0408): 0x%02x\n", registerValue);
}

void readModifyWriteRegister(uint32_t address, uint8_t bitsToSet, uint8_t bitsToClear)
{
  uint8_t before, after;
  cirqueHid.readExtendedMemory(address, &before, 1);
  after = before | bitsToSet;
  after &= ~bitsToClear;
  Serial.printf("0x%2x to 0x%2x\n", before, after);
  cirqueHid.writeExtendedMemory(address, &after, 1);
}

/**************************************************************/
/*********** KEYBOARD PRINTING ******************************/

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

void printKeycodeName(uint8_t keycode)
{
  char strBuf[40];
  const char* name = getKeycodeName(keycode);
  sprintf(strBuf, "%s(0x%02X)", name, keycode);
  Serial.print(strBuf);
}

/** Enable I2C feed for data reporting */
void cirque_enableFeed(void)
{
    uint8_t feedConfig3 = 0;
    cirqueHid.readExtendedMemory(REG_FEED_CONFIG3, &feedConfig3, 1);
    feedConfig3 |= FC3_I2C_FEED_ENABLE;
    cirqueHid.writeExtendedMemory(REG_FEED_CONFIG3, &feedConfig3, 1);
}

/** Disable all feeds (I2C, PS2, USB) */
void cirque_disableFeed(void)
{
    uint8_t feedConfig3 = 0;
    cirqueHid.readExtendedMemory(REG_FEED_CONFIG3, &feedConfig3, 1);
    feedConfig3 &= ~(FC3_PS2_FEED_ENABLE | FC3_I2C_FEED_ENABLE | FC3_USB_FEED_ENABLE);
    cirqueHid.writeExtendedMemory(REG_FEED_CONFIG3, &feedConfig3, 1);
}
