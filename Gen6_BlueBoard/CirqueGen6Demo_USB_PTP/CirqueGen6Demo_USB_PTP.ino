// Copyright (c) 2026 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

// USB Precision Touchpad (PTP) Demo with a Cirque Gen6 Touchpad on the Blue Board
//   The Teensy passes the touchpad's own HID interface through to the PC over USB, so the PC
//   sees a Windows Precision Touchpad (cursor, tap, multi-finger gestures) instead of a mouse.
//   Input reports are forwarded unchanged, and the PC's feature-report requests (Device
//   Capabilities, certification status, Input Mode, ...) are forwarded to the touchpad over I2C.
//
//   You must select "Serial" from the "Tools > USB Type" menu. The sketch adds the touchpad
//   interface to that USB type itself (see usb_ptp_dev.c and usb_ptp_desc.c).
//
//   The touchpad is accessed with the Cirque library (Teensy4_HostBusLayer works on the blue board).

#include <i2c_driver_wire.h>

// use the cirque demo code library
#include <Cirque.h>
#include <CirqueHid.h>
#include <Teensy4_HostBusLayer.h>

#include "usb_ptp.h"        // USB touchpad interface

#define TOUCHPAD_I2C_ADDRESS        CIRQUE_PRIMARY_ADDRESS  // 0x2C
#define TOUCHPAD_STARTUP_TIMEOUT_MS 2000
#define RESET_RESPONSE_TIMEOUT_MS   1000
#define INPUT_BUFFER_SIZE           (USB_PTP_MAX_INPUT_REPORT + 2)  // + 2-byte I2C-HID length

// I2C-HID command opcodes and report types
#define HID_OPCODE_GET_REPORT       2
#define HID_OPCODE_SET_REPORT       3
#define HID_REPORT_TYPE_INPUT       1
#define HID_REPORT_TYPE_OUTPUT      2
#define HID_REPORT_TYPE_FEATURE     3

// Feed control register definitions
#define REG_FEED_CONFIG3              (0x200E000A)
#define FC3_PS2_FEED_ENABLE           (0x01)
#define FC3_I2C_FEED_ENABLE           (0x02)
#define FC3_USB_FEED_ENABLE           (0x04)

// Create a specific Host Bus object (Teensy4_HostBusLayer works on the blue board)
Teensy4_HostBusLayer teensyHostBus;

// Create the common protocol object (that handle HID, PTP, and Cirque commands)
CirqueHid cirqueHid(TOUCHPAD_I2C_ADDRESS, 535); // Address 0x2C, make sure the buffer is at least 535 bytes

HidDescriptor hidDescriptor;

typedef enum
{
  TOUCHPAD_OK,
  TOUCHPAD_HOST_BUS_ERROR,
  TOUCHPAD_NOT_FOUND,
  TOUCHPAD_REPORT_DESC_TOO_LONG,
} TouchpadStatus_t;

TouchpadStatus_t touchpadStatus = TOUCHPAD_NOT_FOUND;
HostBusLayer::initError hostBusError = HostBusLayer::initOkay;
uint16_t inputReadLength = INPUT_BUFFER_SIZE;

static uint8_t reportDescriptor[USB_PTP_MAX_REPORT_DESC];
static uint8_t requestData[USB_PTP_MAX_CONTROL_REPORT];
static uint8_t setReportBuffer[USB_PTP_MAX_CONTROL_REPORT + 9];  // command + data register + length + report

// state data for the command loop
bool enableDataPrinting = false;     // print forwarded input reports
bool enableRequestPrinting = true;   // print the PC's report requests

uint32_t reportsForwarded = 0;
uint32_t reportsDropped = 0;
uint32_t hostRequests = 0;
uint32_t hostRequestsFailed = 0;

void setup() {
  Serial.begin(115200);

  // USB is not connected yet: the PC reads the report descriptor as soon as it is, so it must be ready first
  touchpadStatus = startTouchpad();
  if (touchpadStatus == TOUCHPAD_OK)
  {
    usb_ptp_set_report_descriptor(reportDescriptor, hidDescriptor.wReportDescLength);
  }
  usb_ptp_attach();

  // Give the Serial Monitor a moment to connect, but don't require it: the touchpad must work without it
  elapsedMillis serialConnectTimer = 0;
  while ((!Serial) && (serialConnectTimer < 2000))
  {
  }

  Serial.println(F("Cirque Gen6 USB PTP Demo"));
  printBridgeStatus(false);  // the counts are always 0 here: loop() hasn't run yet
  Serial.println(F("Send 'h' for a list of commands"));
}

/*  The main structure of the loop is:
    When the Data Ready line asserts, read the report and send it to the PC.
    When the PC asks for or sends a report, pass the request to the touchpad.
    The rest is just a user interface to change various settings.
*/
void loop() {
  if (touchpadStatus == TOUCHPAD_OK)
  {
    if (HostBus.drAsserted())
    {
      forwardInputReport();
    }
    serviceHostRequest();
  }

  // Various "key presses" will trigger commands that change the operation of the device
  processKeys();
}

/** Powers the touchpad, then reads its HID descriptor and report descriptor over I2C, the
    same way a PC's I2C-HID driver does, and leaves the touchpad freshly reset. */
TouchpadStatus_t startTouchpad(void)
{
  hostBusError = HostBus.init(400000, 550); // 400,000 Hz, 550 byte buffer
  if (hostBusError > HostBusLayer::initError::initOkay)
  {
    return TOUCHPAD_HOST_BUS_ERROR;
  }

  // The demo board has a power switch, turn it on and wait for the power-on reset response
  turnOnPower();
  waitForResetResponse();

  elapsedMillis timer = 0;
  cirqueHid.getHidDescriptor(hidDescriptor);
  while (!hidDescriptorValid())
  {
    if (timer >= TOUCHPAD_STARTUP_TIMEOUT_MS)
    {
      return TOUCHPAD_NOT_FOUND;
    }
    delay(100);
    cirqueHid.getHidDescriptor(hidDescriptor);
  }

  if (hidDescriptor.wReportDescLength > USB_PTP_MAX_REPORT_DESC)
  {
    return TOUCHPAD_REPORT_DESC_TOO_LONG;
  }

  // Start from a known state (default report mode, no leftover settings from a previous PC)
  cirqueHid.reset();
  waitForResetResponse();

  cirqueHid.readRegister(hidDescriptor.wReportDescRegister, reportDescriptor, hidDescriptor.wReportDescLength);

  inputReadLength = hidDescriptor.wMaxInputLength;
  if (inputReadLength <= 2 || inputReadLength > INPUT_BUFFER_SIZE)
  {
    inputReadLength = INPUT_BUFFER_SIZE;
  }
  return TOUCHPAD_OK;
}

bool hidDescriptorValid(void)
{
  return (hidDescriptor.wHIDDescLength == HidDescriptor::packetLength) &&
         (hidDescriptor.BCD == 0x0100) &&
         (hidDescriptor.wReportDescLength > 0);
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
}

/** After a power-on or I2C-HID reset, the touchpad asserts DR and returns a 2-byte zero-length report. */
bool waitForResetResponse(void)
{
  elapsedMillis timer = 0;
  while (timer < RESET_RESPONSE_TIMEOUT_MS)
  {
    if (HostBus.drAsserted())
    {
      uint16_t received = HostBus.read(TOUCHPAD_I2C_ADDRESS, INPUT_BUFFER_SIZE);
      if (received >= 2)
      {
        uint16_t length = HostBus.fetch();
        length |= (uint16_t)HostBus.fetch() << 8;
        if (length == 0)
        {
          return true;
        }
      }
    }
  }
  return false;
}

/** Reads one report from the touchpad and sends it to the PC without changing it. */
void forwardInputReport(void)
{
  uint8_t packet[INPUT_BUFFER_SIZE];
  uint16_t received = HostBus.read(TOUCHPAD_I2C_ADDRESS, inputReadLength);
  if (received < 2)
  {
    return;
  }
  if (received > sizeof(packet)) received = sizeof(packet);
  for (uint16_t i = 0; i < received; i++)
  {
    packet[i] = HostBus.fetch();
  }

  uint16_t length = packet[0] | (packet[1] << 8);  // includes the 2 length bytes
  if (length <= 2)
  {
    return; // reset response or empty report: nothing for the PC
  }
  if (length > received)
  {
    reportsDropped++;
    return;
  }

  const uint8_t *report = packet + 2;
  uint16_t reportLength = length - 2;
  if (usb_ptp_send_input_report(report, reportLength) == 0)
  {
    reportsForwarded++;
  }
  else
  {
    reportsDropped++; // the PC hasn't configured the device yet, or isn't reading
  }

  if (enableDataPrinting && Serial)
  {
    printBytes(F("Input "), report, reportLength);
  }
}

/** Passes a GET_REPORT or SET_REPORT from the PC to the touchpad and returns the answer. */
void serviceHostRequest(void)
{
  usb_ptp_request_t request;
  if (!usb_ptp_get_request(&request, requestData))
  {
    return;
  }

  bool ok;
  uint16_t length = 0;
  if (request.isSet)
  {
    ok = hidSetReport(request.reportType, request.reportID, requestData, request.length);
    usb_ptp_complete_request(&request, ok, NULL, 0);
  }
  else
  {
    length = hidGetReport(request.reportType, request.reportID, requestData, request.length);
    ok = (length > 0);
    usb_ptp_complete_request(&request, ok, requestData, length);
  }

  hostRequests++;
  if (!ok)
  {
    hostRequestsFailed++;
  }

  if (enableRequestPrinting && Serial)
  {
    Serial.print(request.isSet ? F("PC SET_REPORT ") : F("PC GET_REPORT "));
    Serial.print(reportTypeName(request.reportType));
    Serial.print(F(" ID 0x"));
    if (request.reportID < 0x10) Serial.print('0');
    Serial.print(request.reportID, HEX);
    if (request.isSet)
    {
      printBytes(F(": "), requestData, request.length);
    }
    else if (ok)
    {
      Serial.print(F(" -> "));
      Serial.print(length);
      Serial.println(F(" bytes"));
    }
    if (!ok)
    {
      Serial.println(F("  Failed: the touchpad did not respond"));
    }
  }
}

const __FlashStringHelper *reportTypeName(uint8_t reportType)
{
  switch (reportType)
  {
    case HID_REPORT_TYPE_INPUT:   return F("Input");
    case HID_REPORT_TYPE_OUTPUT:  return F("Output");
    case HID_REPORT_TYPE_FEATURE: return F("Feature");
    default:                      return F("Unknown");
  }
}

/** Fills in an I2C-HID command (command register, then the command itself) and returns its length.
    Report IDs of 15 and above need an extra byte. */
uint8_t setupHidCommand(uint8_t *cmd, uint8_t opcode, uint8_t reportType, uint8_t reportID)
{
  uint8_t length = 0;
  cmd[length++] = (uint8_t)hidDescriptor.wCommandRegister;
  cmd[length++] = (uint8_t)(hidDescriptor.wCommandRegister >> 8);
  if (reportID > 14)
  {
    cmd[length++] = ((reportType << 4) & 0x30) | 0x0F;
    cmd[length++] = opcode & 0x0F;
    cmd[length++] = reportID;
  }
  else
  {
    cmd[length++] = ((reportType << 4) & 0x30) | (reportID & 0x0F);
    cmd[length++] = opcode & 0x0F;
  }
  return length;
}

/** Reads any report with the I2C-HID GET_REPORT command. On return, report[] holds the
    report bytes (starting with the report ID) without the 2-byte I2C-HID length prefix.
    Returns the number of bytes stored, or 0 if the touchpad returned nothing usable. */
uint16_t hidGetReport(uint8_t reportType, uint8_t reportID, uint8_t *report, uint16_t maxLength)
{
  uint8_t cmd[7];
  uint8_t cmdLength = setupHidCommand(cmd, HID_OPCODE_GET_REPORT, reportType, reportID);
  cmd[cmdLength++] = (uint8_t)hidDescriptor.wDataRegister;
  cmd[cmdLength++] = (uint8_t)(hidDescriptor.wDataRegister >> 8);

  uint16_t received = HostBus.writeRestartRead(TOUCHPAD_I2C_ADDRESS, cmdLength, cmd, maxLength + 2);
  if (received < 2)
  {
    return 0;
  }
  uint16_t reportLength = HostBus.fetch();
  reportLength |= (uint16_t)HostBus.fetch() << 8;
  if (reportLength <= 2)
  {
    return 0;
  }
  reportLength -= 2;
  if (reportLength > maxLength) reportLength = maxLength;
  if (reportLength > received - 2) reportLength = received - 2;
  for (uint16_t i = 0; i < reportLength; i++)
  {
    report[i] = HostBus.fetch();
  }
  return reportLength;
}

/** Writes any report with the I2C-HID SET_REPORT command. report[] must start with the
    report ID byte when the report uses an ID, exactly as a USB host sends it. */
bool hidSetReport(uint8_t reportType, uint8_t reportID, const uint8_t *report, uint16_t length)
{
  if (length > USB_PTP_MAX_CONTROL_REPORT)
  {
    return false;
  }
  uint16_t dataLength = length + 2;
  uint16_t i = setupHidCommand(setReportBuffer, HID_OPCODE_SET_REPORT, reportType, reportID);
  setReportBuffer[i++] = (uint8_t)hidDescriptor.wDataRegister;
  setReportBuffer[i++] = (uint8_t)(hidDescriptor.wDataRegister >> 8);
  setReportBuffer[i++] = (uint8_t)dataLength;
  setReportBuffer[i++] = (uint8_t)(dataLength >> 8);
  memcpy(setReportBuffer + i, report, length);
  i += length;
  HostBus.write(TOUCHPAD_I2C_ADDRESS, i, setReportBuffer);
  return HostBus.i2cError == 0;
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
      case 'M':
      case 'm':
        Serial.println(F("Changed to Mouse/Relative reporting..."));
        cirqueHid.setInputMode(false);
        break;
      case 'A':
      case 'a':
      case 'p':
        Serial.println(F("Changed to PTP/Absolute reporting..."));
        cirqueHid.setInputMode(true);
        break;
      case 'k':
        Serial.println(F("Hid Set Power off..."));
        cirqueHid.setPower(false);
        break;
      case 'K':
        Serial.println(F("Hid Set Power on..."));
        cirqueHid.setPower(true);
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
      case 'f':
        Serial.println(F("Feed Enabled"));
        cirque_enableFeed();
        break;
      case 'F':
        Serial.println(F("Feed Disabled"));
        cirque_disableFeed();
        break;
      case 'd':
        Serial.println(F("Data Printing Disabled"));
        enableDataPrinting = false;
        break;
      case 'D':
        Serial.println(F("Data Printing Enabled"));
        enableDataPrinting = true;
        break;
      case 'v':
        Serial.println(F("PC Request Printing Disabled"));
        enableRequestPrinting = false;
        break;
      case 'V':
        Serial.println(F("PC Request Printing Enabled"));
        enableRequestPrinting = true;
        break;
      case 'g':
        {
          uint8_t numberContacts;
          CirqueHid::PTP_ButtonImplementation buttonImplementation;
          cirqueHid.getDeviceCapabilities(numberContacts, buttonImplementation);
          Serial.printf("getDeviceCapabilities  Number Contacts: %d  buttonImplementation: %d\n", numberContacts, buttonImplementation);
        }
        break;
      case 'G':
        printReportDescriptor();
        break;
      case 'o':
        Serial.println(F("System Information:"));
        identifyDevice();
        readDefaults();
        break;
      case 'u':
        printBridgeStatus(true);
        break;
      case '\r':
      case '\n':
        break;
      default:
        Serial.print(F("Invalid command: '"));
        Serial.print(rxChar);
        Serial.println(F("'. Send 'h' for a list of commands."));
        break;
    }
  }
}

void showHelp(void)
{
  Serial.println(F("Cirque Gen6 USB PTP Demo"));
  Serial.println(F("Available Commands (case sensitive)"));
  Serial.println(F(""));
  Serial.println(F("--- Report Modes ---"));
  Serial.println(F("  m, M - issue Mouse/Relative reports"));
  Serial.println(F("  a, A, p - issue PTP/Absolute reports"));
  Serial.println(F("    (On Windows and Linux, the PC's touchpad driver sets the mode"));
  Serial.println(F("     and may override these commands)"));
  Serial.println(F(""));
  Serial.println(F("--- Power Control ---"));
  Serial.println(F("  k - HID power off"));
  Serial.println(F("  K - HID power on"));
  Serial.println(F("  i - cancel force-sleep"));
  Serial.println(F("  I - force sleep"));
  Serial.println(F(""));
  Serial.println(F("--- Reporting Control ---"));
  Serial.println(F("  f - enable feed"));
  Serial.println(F("  F - disable feed"));
  Serial.println(F("  d - data printing off (default)"));
  Serial.println(F("  D - data printing on"));
  Serial.println(F("  v - PC request printing off"));
  Serial.println(F("  V - PC request printing on (default)"));
  Serial.println(F(""));
  Serial.println(F("--- Information ---"));
  Serial.println(F("  g - get device capabilities"));
  Serial.println(F("  G - print report descriptor"));
  Serial.println(F("  o - show system information"));
  Serial.println(F("  u - show USB bridge status"));
  Serial.println(F("  h, ? - print this help"));
  Serial.println(F(""));
}

void printBridgeStatus(bool printCounts)
{
  switch (touchpadStatus)
  {
    case TOUCHPAD_OK:
      Serial.printf("Touchpad found, VID 0x%04x PID 0x%04x, report descriptor %u bytes\n",
        hidDescriptor.wVendorID, hidDescriptor.wProductID, hidDescriptor.wReportDescLength);
      if (hidDescriptor.wMaxInputLength > INPUT_BUFFER_SIZE)
      {
        Serial.println(F("Warning: the touchpad's longest input report is too long to forward"));
      }
      break;
    case TOUCHPAD_HOST_BUS_ERROR:
      Serial.print(F("Failed to init host bus: "));
      Serial.print((int)hostBusError);
      Serial.println(F(", so only USB Serial is enabled."));
      break;
    case TOUCHPAD_NOT_FOUND:
      Serial.println(F("Touchpad not found, so only USB Serial is enabled."));
      Serial.println(F("Check the touchpad connection, then reset the Teensy."));
      break;
    case TOUCHPAD_REPORT_DESC_TOO_LONG:
      Serial.printf("The touchpad's report descriptor (%u bytes) is longer than USB_PTP_MAX_REPORT_DESC, so only USB Serial is enabled.\n",
        hidDescriptor.wReportDescLength);
      break;
  }
  Serial.print(F("USB: "));
  Serial.println(usb_ptp_configured() ? F("configured by the PC") : F("not configured"));
  if (!printCounts)
  {
    return;
  }
  Serial.printf("Reports forwarded: %lu, dropped: %lu\n",
    (unsigned long)reportsForwarded, (unsigned long)reportsDropped);
  Serial.printf("PC report requests: %lu, failed: %lu\n",
    (unsigned long)hostRequests, (unsigned long)hostRequestsFailed);
}

void identifyDevice(void)
{
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

void readDefaults(void)
{
  uint8_t registerValue;
  cirqueHid.readExtendedMemory(0x20080018, &registerValue, 1);
  Serial.printf("XY Config (0x20080018): 0x%02x\n", registerValue);

  cirqueHid.readExtendedMemory(0x200a0408, &registerValue, 1);
  Serial.printf("Power Control (0x200A0408): 0x%02x\n", registerValue);
}

// Prints the report descriptor the PC is given, 16 bytes per line, so it can be pasted
// into a HID descriptor parser.
void printReportDescriptor(void)
{
  if (touchpadStatus != TOUCHPAD_OK)
  {
    Serial.println(F("No report descriptor: the touchpad was not set up at start-up"));
    return;
  }
  Serial.printf("Report Descriptor (%u bytes)\n", hidDescriptor.wReportDescLength);
  for (uint16_t i = 0; i < hidDescriptor.wReportDescLength; i++)
  {
    Serial.printf("%02X", reportDescriptor[i]);
    Serial.print((i % 16 == 15) ? '\n' : ' ');
  }
  Serial.println();
}

void printBytes(const __FlashStringHelper *label, const uint8_t *bytes, uint16_t length)
{
  Serial.print(label);
  for (uint16_t i = 0; i < length; i++)
  {
    Serial.printf("%02X ", bytes[i]);
  }
  Serial.println();
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
