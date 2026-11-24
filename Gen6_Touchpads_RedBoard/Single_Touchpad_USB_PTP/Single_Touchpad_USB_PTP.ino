// Copyright (c) 2026 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

// USB Precision Touchpad (PTP) Example
//   The Teensy passes the touchpad's own HID interface through to the PC over USB, so the PC
//   sees a Windows Precision Touchpad (cursor, tap, multi-finger gestures) instead of a mouse.
//   Input reports are forwarded unchanged, and the PC's feature-report requests (Device
//   Capabilities, certification status, Input Mode, ...) are forwarded to the touchpad over I2C.
//
//   You must select "Serial" from the "Tools > USB Type" menu. The sketch adds the touchpad
//   interface to that USB type itself (see usb_ptp_dev.c and usb_ptp_desc.c).

#include "API_C3.h"         /** < Provides API calls to interact with API_C3 firmware */
#include "API_Hardware.h"
#include "API_HostBus.h"    /** < Provides I2C connection to module */
#include "I2C.h"
#include "usb_ptp.h"        /** < USB touchpad interface */

#define HID_DESCRIPTOR_REGISTER     0x0020
#define TOUCHPAD_STARTUP_TIMEOUT_MS 2000
#define RESET_RESPONSE_TIMEOUT_MS   1000
#define INPUT_BUFFER_SIZE           (USB_PTP_MAX_INPUT_REPORT + 2)  // + 2-byte I2C-HID length

bool dataPrint_mode_g = false;    /** < toggle for printing forwarded input reports > */
bool requestLog_mode_g = true;    /** < toggle for printing the PC's report requests > */

typedef enum
{
  TOUCHPAD_OK,
  TOUCHPAD_NOT_FOUND,
  TOUCHPAD_REPORT_DESC_TOO_LONG,
} TouchpadStatus_t;

TouchpadStatus_t touchpadStatus_g = TOUCHPAD_NOT_FOUND;
HIDDescriptor_t hidDescriptor_g;
uint16_t inputReadLength_g = INPUT_BUFFER_SIZE;

static uint8_t reportDescriptor_g[USB_PTP_MAX_REPORT_DESC];
static uint8_t requestData_g[USB_PTP_MAX_CONTROL_REPORT];

uint32_t reportsForwarded_g = 0;
uint32_t reportsDropped_g = 0;
uint32_t hostRequests_g = 0;
uint32_t hostRequestsFailed_g = 0;

void setup()
{
  Serial.begin(115200);

  API_Hardware_init();       //Initialize board hardware

  delay(2);                  //delay for power up

  // initialize i2c connection at 400kHz
  API_C3_init(PROJECT_I2C_FREQUENCY, CIRQUE_I2C_ADDR);

  // USB is not connected yet: the PC reads the report descriptor as soon as it is, so it must be ready first
  touchpadStatus_g = readTouchpadDescriptors();
  if (touchpadStatus_g == TOUCHPAD_OK)
  {
    usb_ptp_set_report_descriptor(reportDescriptor_g, hidDescriptor_g.wReportDescLength);
  }
  usb_ptp_attach();

  // Give the Serial Monitor a moment to connect, but don't require it: the touchpad must work without it
  uint32_t start = millis();
  while (!Serial && (millis() - start) < 2000);

  Serial.println(F("Single Pad USB PTP Demo"));
  printBridgeStatus(false);  // the counts are always 0 here: loop() hasn't run yet
  Serial.println(F("Send 'h' for a list of commands"));
}

/*  The main structure of the loop is:
    When the Data Ready line asserts, read the report and send it to the PC.
    When the PC asks for or sends a report, pass the request to the touchpad.
    The rest is just a user interface to change various settings.
*/
void loop()
{
  if (touchpadStatus_g == TOUCHPAD_OK)
  {
    if (API_C3_DR_Asserted())
    {
      forwardInputReport();
    }
    serviceHostRequest();
  }

  /* Handle incoming messages from user on serial */
  if (Serial.available())
  {
    char rxChar = Serial.read();
    processSerialCommand(rxChar);
  }
}

/** Reads the touchpad's HID descriptor and report descriptor over I2C, the same way
    a PC's I2C-HID driver does, and leaves the touchpad freshly reset. */
TouchpadStatus_t readTouchpadDescriptors(void)
{
  uint32_t start = millis();
  while (!hidDescriptorValid(&hidDescriptor_g))
  {
    if ((millis() - start) >= TOUCHPAD_STARTUP_TIMEOUT_MS)
    {
      return TOUCHPAD_NOT_FOUND;
    }
    HB_HID_GetHidDescriptor(HID_DESCRIPTOR_REGISTER, &hidDescriptor_g);
    if (!hidDescriptorValid(&hidDescriptor_g))
    {
      delay(100);
    }
  }

  if (hidDescriptor_g.wReportDescLength > USB_PTP_MAX_REPORT_DESC)
  {
    return TOUCHPAD_REPORT_DESC_TOO_LONG;
  }

  // Start from a known state (default report mode, no leftover settings from a previous PC)
  HB_HID_Reset();
  waitForResetResponse();

  HB_HID_readRegister(hidDescriptor_g.wReportDescRegister, reportDescriptor_g, hidDescriptor_g.wReportDescLength);

  inputReadLength_g = hidDescriptor_g.wMaxInputLength;
  if (inputReadLength_g <= 2 || inputReadLength_g > INPUT_BUFFER_SIZE)
  {
    inputReadLength_g = INPUT_BUFFER_SIZE;
  }
  return TOUCHPAD_OK;
}

bool hidDescriptorValid(const HIDDescriptor_t *descriptor)
{
  return (descriptor->wHIDDescLength == 30) &&
         (descriptor->bcdVersion == 0x0100) &&
         (descriptor->wReportDescLength > 0);
}

/** After an I2C-HID reset, the touchpad asserts DR and returns a 2-byte zero-length report. */
void waitForResetResponse(void)
{
  uint8_t packet[INPUT_BUFFER_SIZE];
  uint32_t start = millis();
  while ((millis() - start) < RESET_RESPONSE_TIMEOUT_MS)
  {
    if (HB_DR_Asserted())
    {
      HB_readReport(packet, sizeof(packet));
      if ((packet[0] | (packet[1] << 8)) == 0)
      {
        return;
      }
    }
  }
}

/** Reads one report from the touchpad and sends it to the PC without changing it. */
void forwardInputReport(void)
{
  uint8_t packet[INPUT_BUFFER_SIZE];
  HB_readReport(packet, inputReadLength_g);

  uint16_t length = packet[0] | (packet[1] << 8);  // includes the 2 length bytes
  if (length <= 2)
  {
    return; // reset response or empty report: nothing for the PC
  }
  if (length > inputReadLength_g)
  {
    reportsDropped_g++;
    return;
  }

  const uint8_t *report = packet + 2;
  uint16_t reportLength = length - 2;
  if (usb_ptp_send_input_report(report, reportLength) == 0)
  {
    reportsForwarded_g++;
  }
  else
  {
    reportsDropped_g++; // the PC hasn't configured the device yet, or isn't reading
  }

  if (dataPrint_mode_g && Serial)
  {
    printBytes(F("Input "), report, reportLength);
  }
}

/** Passes a GET_REPORT or SET_REPORT from the PC to the touchpad and returns the answer. */
void serviceHostRequest(void)
{
  usb_ptp_request_t request;
  if (!usb_ptp_get_request(&request, requestData_g))
  {
    return;
  }

  bool ok;
  uint16_t length = 0;
  if (request.isSet)
  {
    ok = HB_HID_setReport(request.reportType, request.reportID, requestData_g, request.length);
    usb_ptp_complete_request(&request, ok, NULL, 0);
  }
  else
  {
    length = HB_HID_getReport(request.reportType, request.reportID, requestData_g, request.length);
    ok = (length > 0);
    usb_ptp_complete_request(&request, ok, requestData_g, length);
  }

  hostRequests_g++;
  if (!ok)
  {
    hostRequestsFailed_g++;
  }

  if (requestLog_mode_g && Serial)
  {
    Serial.print(request.isSet ? F("PC SET_REPORT ") : F("PC GET_REPORT "));
    Serial.print(reportTypeName(request.reportType));
    Serial.print(F(" ID 0x"));
    if (request.reportID < 0x10) Serial.print('0');
    Serial.print(request.reportID, HEX);
    if (request.isSet)
    {
      printBytes(F(": "), requestData_g, request.length);
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
    case HB_HID_REPORT_TYPE_INPUT:   return F("Input");
    case HB_HID_REPORT_TYPE_OUTPUT:  return F("Output");
    case HB_HID_REPORT_TYPE_FEATURE: return F("Feature");
    default:                         return F("Unknown");
  }
}

void processSerialCommand(char rxChar)
{
  switch(rxChar)
  {
    case 'a':
    case 'A':
    case 'p':
        Serial.println(F("PTP/Absolute Mode Set"));
        API_C3_setPtpMode();
        break;

    case 'm':
    case 'r':
        Serial.println(F("Mouse/Relative Mode Set"));
        API_C3_setRelativeMode();
        break;

    case 'f':
        Serial.println(F("Feed Enabled"));
        API_C3_enableFeed();
        break;

    case 'F':
        Serial.println(F("Feed Disabled"));
        API_C3_disableFeed();
        break;

    case 's':
        systemInfo_t sysInfo;
        API_C3_readSystemInfo(&sysInfo);
        printSystemInfo(&sysInfo);
        break;

    case 'g':
        printHidDescriptor(&hidDescriptor_g);
        break;

    case 'G':
        printReportDescriptor();
        break;

    case 'i':
        printBridgeStatus(true);
        break;

    case 'd':
        Serial.println(F("Data Printing Enabled"));
        dataPrint_mode_g = true;
        break;

    case 'D':
        Serial.println(F("Data Printing Disabled"));
        dataPrint_mode_g = false;
        break;

    case 'v':
        Serial.println(F("PC Request Printing Enabled"));
        requestLog_mode_g = true;
        break;

    case 'V':
        Serial.println(F("PC Request Printing Disabled"));
        requestLog_mode_g = false;
        break;

    case 'h':
    case 'H':
    case '?':
        printHelpTable();
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

void printHelpTable(void)
{
  Serial.println(F("Available Commands (case sensitive)"));
  Serial.println();
  Serial.println(F("a, A, p\t-\tSet to PTP/Absolute Mode"));
  Serial.println(F("r, m\t-\tSet to Mouse/Relative Mode"));
  Serial.println(F("\t\t(On Windows and Linux, the PC's touchpad driver sets the mode"));
  Serial.println(F("\t\t and may override this command)"));
  Serial.println(F("f\t-\tEnable Feed (default)"));
  Serial.println(F("F\t-\tDisable Feed"));
  Serial.println(F("s\t-\tPrint System Info"));
  Serial.println(F("g\t-\tPrint HID Descriptor"));
  Serial.println(F("G\t-\tPrint Report Descriptor"));
  Serial.println(F("i\t-\tPrint USB Bridge Status"));
  Serial.println();
  Serial.println(F("h, H, ?\t-\tPrint this Table"));
  Serial.println(F("d\t-\tTurn on Data Printing"));
  Serial.println(F("D\t-\tTurn off Data Printing (default)"));
  Serial.println(F("v\t-\tTurn on PC Request Printing (default)"));
  Serial.println(F("V\t-\tTurn off PC Request Printing"));
}

void printBridgeStatus(bool printCounts)
{
  switch (touchpadStatus_g)
  {
    case TOUCHPAD_OK:
      Serial.print(F("Touchpad found, VID 0x"));
      Serial.print(hidDescriptor_g.wVendorID, HEX);
      Serial.print(F(" PID 0x"));
      Serial.print(hidDescriptor_g.wProductID, HEX);
      Serial.print(F(", report descriptor "));
      Serial.print(hidDescriptor_g.wReportDescLength);
      Serial.println(F(" bytes"));
      if (hidDescriptor_g.wMaxInputLength > INPUT_BUFFER_SIZE)
      {
        Serial.println(F("Warning: the touchpad's longest input report is too long to forward"));
      }
      break;
    case TOUCHPAD_NOT_FOUND:
      Serial.println(F("Touchpad not found, so only USB Serial is enabled."));
      Serial.println(F("Check the touchpad connection, then reset the Teensy."));
      break;
    case TOUCHPAD_REPORT_DESC_TOO_LONG:
      Serial.print(F("The touchpad's report descriptor ("));
      Serial.print(hidDescriptor_g.wReportDescLength);
      Serial.println(F(" bytes) is longer than USB_PTP_MAX_REPORT_DESC, so only USB Serial is enabled."));
      break;
  }
  Serial.print(F("USB: "));
  Serial.println(usb_ptp_configured() ? F("configured by the PC") : F("not configured"));
  if (!printCounts)
  {
    return;
  }
  Serial.print(F("Reports forwarded: "));
  Serial.print(reportsForwarded_g);
  Serial.print(F(", dropped: "));
  Serial.println(reportsDropped_g);
  Serial.print(F("PC report requests: "));
  Serial.print(hostRequests_g);
  Serial.print(F(", failed: "));
  Serial.println(hostRequestsFailed_g);
}

void printSystemInfo(systemInfo_t* sysInfo)
{
  Serial.println(F("System Information"));
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
  Serial.println();
}

void printHidDescriptor(HIDDescriptor_t* hidDescriptor)
{
  Serial.println(F("HID Descriptor"));
  Serial.print(F("wHIDDescLength:\t\t"));
  Serial.println(hidDescriptor->wHIDDescLength);
  Serial.print(F("bcdVersion:\t\t0x"));
  Serial.println(hidDescriptor->bcdVersion, HEX);
  Serial.print(F("wReportDescLength:\t"));
  Serial.println(hidDescriptor->wReportDescLength);
  Serial.print(F("wReportDescRegister:\t0x"));
  Serial.println(hidDescriptor->wReportDescRegister, HEX);
  Serial.print(F("wInputRegister:\t\t0x"));
  Serial.println(hidDescriptor->wInputRegister, HEX);
  Serial.print(F("wMaxInputLength:\t"));
  Serial.println(hidDescriptor->wMaxInputLength);
  Serial.print(F("wCommandRegister:\t0x"));
  Serial.println(hidDescriptor->wCommandRegister, HEX);
  Serial.print(F("wDataRegister:\t\t0x"));
  Serial.println(hidDescriptor->wDataRegister, HEX);
  Serial.print(F("wVendorID:\t\t0x"));
  Serial.println(hidDescriptor->wVendorID, HEX);
  Serial.print(F("wProductID:\t\t0x"));
  Serial.println(hidDescriptor->wProductID, HEX);
  Serial.print(F("wVersionID:\t\t0x"));
  Serial.println(hidDescriptor->wVersionID, HEX);
  Serial.println();
}

// Prints the report descriptor the PC is given, 16 bytes per line, so it can be pasted
// into a HID descriptor parser.
void printReportDescriptor(void)
{
  if (touchpadStatus_g != TOUCHPAD_OK)
  {
    Serial.println(F("No report descriptor: the touchpad was not set up at start-up"));
    return;
  }
  Serial.print(F("Report Descriptor ("));
  Serial.print(hidDescriptor_g.wReportDescLength);
  Serial.println(F(" bytes)"));
  for (uint16_t i = 0; i < hidDescriptor_g.wReportDescLength; i++)
  {
    if (reportDescriptor_g[i] < 0x10) Serial.print('0');
    Serial.print(reportDescriptor_g[i], HEX);
    Serial.print((i % 16 == 15) ? '\n' : ' ');
  }
  Serial.println();
}

void printBytes(const __FlashStringHelper *label, const uint8_t *bytes, uint16_t length)
{
  Serial.print(label);
  for (uint16_t i = 0; i < length; i++)
  {
    if (bytes[i] < 0x10) Serial.print('0');
    Serial.print(bytes[i], HEX);
    Serial.print(' ');
  }
  Serial.println();
}
