// Copyright (c) 2026 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

// Dual USB Precision Touchpad (PTP) Example
//   The Teensy passes each touchpad's own HID interface through to the PC over USB, so the PC
//   sees two Windows Precision Touchpads (cursor, tap, multi-finger gestures) instead of mice.
//   Input reports are forwarded unchanged, and the PC's feature-report requests (Device
//   Capabilities, certification status, Input Mode, ...) are forwarded to the right touchpad
//   over I2C.
//
//   You must select "Serial" from the "Tools > USB Type" menu. The sketch adds the touchpad
//   interfaces to that USB type itself (see usb_ptp_dev.c and usb_ptp_desc.c).

#include "API_C3.h"         /** < Provides API calls to interact with API_C3 firmware */
#include "API_Hardware.h"
#include "API_HostBus.h"    /** < Provides I2C connection to module */
#include "I2C.h"
#include "usb_ptp.h"        /** < USB touchpad interfaces */

#define NUM_CHANNELS                2
#define HID_DESCRIPTOR_REGISTER     0x0020
#define TOUCHPAD_STARTUP_TIMEOUT_MS 2000
#define RESET_RESPONSE_TIMEOUT_MS   1000
#define CHANNEL_DIGIT_TIMEOUT_MS    20
#define INPUT_BUFFER_SIZE           (USB_PTP_MAX_INPUT_REPORT + 2)  // + 2-byte I2C-HID length

bool dataPrint_mode_g = false;    /** < toggle for printing forwarded input reports > */
bool requestLog_mode_g = true;    /** < toggle for printing the PC's report requests > */

typedef enum
{
  TOUCHPAD_OK,
  TOUCHPAD_NOT_FOUND,
  TOUCHPAD_REPORT_DESC_TOO_LONG,
} TouchpadStatus_t;

typedef struct
{
  TouchpadStatus_t status;
  HIDDescriptor_t hidDescriptor;
  uint16_t inputReadLength;
  uint8_t reportDescriptor[USB_PTP_MAX_REPORT_DESC];
  uint32_t reportsForwarded;
  uint32_t reportsDropped;
  uint32_t hostRequests;
  uint32_t hostRequestsFailed;
} Touchpad_t;

static Touchpad_t touchpad_g[NUM_CHANNELS];
static uint8_t requestData_g[USB_PTP_MAX_CONTROL_REPORT];

static const uint8_t drMask_g[NUM_CHANNELS] = { DR0_MASK, DR1_MASK };

void setup()
{
  Serial.begin(115200);

  API_Hardware_init();       //Initialize board hardware

  delay(2);                  //delay for power up

  // initialize i2c connections at 400kHz
  API_C3_init(PROJECT_I2C_FREQUENCY, CIRQUE_I2C_ADDR, PROJECT_I2C_FREQUENCY, CIRQUE_I2C_ADDR);

  // USB is not connected yet: the PC reads the report descriptors as soon as it is, so they must be ready first
  for (uint8_t channel = 0; channel < NUM_CHANNELS; channel++)
  {
    Touchpad_t *touchpad = &touchpad_g[channel];
    touchpad->status = readTouchpadDescriptors(channel);
    if (touchpad->status == TOUCHPAD_OK)
    {
      usb_ptp_set_report_descriptor(channel, touchpad->reportDescriptor, touchpad->hidDescriptor.wReportDescLength);
    }
  }
  usb_ptp_attach();

  // Give the Serial Monitor a moment to connect, but don't require it: the touchpads must work without it
  uint32_t start = millis();
  while (!Serial && (millis() - start) < 2000);

  Serial.println(F("Dual Pad USB PTP Demo"));
  printBridgeStatus(false);  // the counts are always 0 here: loop() hasn't run yet
  Serial.println(F("Send 'h' for a list of commands"));
}

/*  The main structure of the loop is:
    When a touchpad's Data Ready line asserts, read its report and send it to the PC.
    When the PC asks for or sends a report, pass the request to the touchpad it is for.
    The rest is just a user interface to change various settings.
*/
void loop()
{
  uint8_t dr_status = API_C3_DR_Asserted();
  for (uint8_t channel = 0; channel < NUM_CHANNELS; channel++)
  {
    if ((touchpad_g[channel].status == TOUCHPAD_OK) && (dr_status & drMask_g[channel]))
    {
      forwardInputReport(channel);
    }
  }
  serviceHostRequest();

  /* Handle incoming messages from user on serial */
  if (Serial.available())
  {
    char rxChar = Serial.read();
    if (rxChar != '\r' && rxChar != '\n')
    {
      processSerialCommand(rxChar, readChannelDigit());
    }
  }
}

/** Reads one touchpad's HID descriptor and report descriptor over I2C, the same way
    a PC's I2C-HID driver does, and leaves the touchpad freshly reset. */
TouchpadStatus_t readTouchpadDescriptors(uint8_t channel)
{
  Touchpad_t *touchpad = &touchpad_g[channel];
  touchpad->inputReadLength = INPUT_BUFFER_SIZE;

  uint32_t start = millis();
  while (!hidDescriptorValid(&touchpad->hidDescriptor))
  {
    if ((millis() - start) >= TOUCHPAD_STARTUP_TIMEOUT_MS)
    {
      return TOUCHPAD_NOT_FOUND;
    }
    HB_HID_GetHidDescriptor(channel, HID_DESCRIPTOR_REGISTER, &touchpad->hidDescriptor);
    if (!hidDescriptorValid(&touchpad->hidDescriptor))
    {
      delay(100);
    }
  }

  if (touchpad->hidDescriptor.wReportDescLength > USB_PTP_MAX_REPORT_DESC)
  {
    return TOUCHPAD_REPORT_DESC_TOO_LONG;
  }

  // Start from a known state (default report mode, no leftover settings from a previous PC)
  HB_HID_Reset(channel);
  waitForResetResponse(channel);

  HB_HID_readRegister(channel, touchpad->hidDescriptor.wReportDescRegister, touchpad->reportDescriptor,
                      touchpad->hidDescriptor.wReportDescLength);

  touchpad->inputReadLength = touchpad->hidDescriptor.wMaxInputLength;
  if (touchpad->inputReadLength <= 2 || touchpad->inputReadLength > INPUT_BUFFER_SIZE)
  {
    touchpad->inputReadLength = INPUT_BUFFER_SIZE;
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
void waitForResetResponse(uint8_t channel)
{
  uint8_t packet[INPUT_BUFFER_SIZE];
  uint32_t start = millis();
  while ((millis() - start) < RESET_RESPONSE_TIMEOUT_MS)
  {
    if (HB_DR_Asserted() & drMask_g[channel])
    {
      HB_readReport(channel, packet, sizeof(packet));
      if ((packet[0] | (packet[1] << 8)) == 0)
      {
        return;
      }
    }
  }
}

/** Reads one report from a touchpad and sends it to the PC without changing it. */
void forwardInputReport(uint8_t channel)
{
  Touchpad_t *touchpad = &touchpad_g[channel];
  uint8_t packet[INPUT_BUFFER_SIZE];
  HB_readReport(channel, packet, touchpad->inputReadLength);

  uint16_t length = packet[0] | (packet[1] << 8);  // includes the 2 length bytes
  if (length <= 2)
  {
    return; // reset response or empty report: nothing for the PC
  }
  if (length > touchpad->inputReadLength)
  {
    touchpad->reportsDropped++;
    return;
  }

  const uint8_t *report = packet + 2;
  uint16_t reportLength = length - 2;
  if (usb_ptp_send_input_report(channel, report, reportLength) == 0)
  {
    touchpad->reportsForwarded++;
  }
  else
  {
    touchpad->reportsDropped++; // the PC hasn't configured the device yet, or isn't reading
  }

  if (dataPrint_mode_g && Serial)
  {
    printChannelPrefix(channel);
    printBytes(F("Input "), report, reportLength);
  }
}

/** Passes a GET_REPORT or SET_REPORT from the PC to the touchpad it is for, and returns the answer. */
void serviceHostRequest(void)
{
  usb_ptp_request_t request;
  if (!usb_ptp_get_request(&request, requestData_g))
  {
    return;
  }

  uint8_t channel = request.touchpad;
  bool ok = false;
  uint16_t length = 0;
  if (channel >= NUM_CHANNELS)
  {
    usb_ptp_complete_request(&request, false, NULL, 0);
    return;
  }
  if (request.isSet)
  {
    ok = HB_HID_setReport(channel, request.reportType, request.reportID, requestData_g, request.length);
    usb_ptp_complete_request(&request, ok, NULL, 0);
  }
  else
  {
    length = HB_HID_getReport(channel, request.reportType, request.reportID, requestData_g, request.length);
    ok = (length > 0);
    usb_ptp_complete_request(&request, ok, requestData_g, length);
  }

  Touchpad_t *touchpad = &touchpad_g[channel];
  touchpad->hostRequests++;
  if (!ok)
  {
    touchpad->hostRequestsFailed++;
  }

  if (requestLog_mode_g && Serial)
  {
    printChannelPrefix(channel);
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

/** Returns the I2C channel digit that follows a command ('0' or '1') as 0 or 1, or 0xFF if
    there is none. Waits briefly, because the digit may arrive a moment after the command letter. */
uint8_t readChannelDigit(void)
{
  uint32_t start = millis();
  while (!Serial.available() && (millis() - start) < CHANNEL_DIGIT_TIMEOUT_MS);
  if (!Serial.available())
  {
    return 0xFF;
  }
  char next = Serial.peek();
  if (next >= '0' && next < ('0' + NUM_CHANNELS))
  {
    Serial.read();
    return next - '0';
  }
  return 0xFF;
}

void processSerialCommand(char rxChar, uint8_t channel)
{
  // Commands that apply to both touchpads
  switch(rxChar)
  {
    case 'i':
        printBridgeStatus(true);
        return;

    case 'd':
        Serial.println(F("Data Printing Enabled"));
        dataPrint_mode_g = true;
        return;

    case 'D':
        Serial.println(F("Data Printing Disabled"));
        dataPrint_mode_g = false;
        return;

    case 'v':
        Serial.println(F("PC Request Printing Enabled"));
        requestLog_mode_g = true;
        return;

    case 'V':
        Serial.println(F("PC Request Printing Disabled"));
        requestLog_mode_g = false;
        return;

    case 'h':
    case 'H':
    case '?':
        printHelpTable();
        return;

    case 'a':
    case 'A':
    case 'p':
    case 'm':
    case 'r':
    case 'f':
    case 'F':
    case 's':
    case 'g':
    case 'G':
        break;  // per-touchpad commands, handled below

    default:
        Serial.print(F("Invalid command: '"));
        Serial.print(rxChar);
        Serial.println(F("'. Send 'h' for a list of commands."));
        return;
  }

  // Commands for one touchpad: the letter must be followed by the I2C channel number
  if (channel >= NUM_CHANNELS)
  {
    Serial.print(F("Add the I2C channel number (0 or 1) to this command, for example '"));
    Serial.print(rxChar);
    Serial.println(F("0'."));
    return;
  }
  if (touchpad_g[channel].status == TOUCHPAD_NOT_FOUND)
  {
    Serial.print(F("Channel "));
    Serial.print(channel);
    Serial.println(F(": no touchpad was found at start-up"));
    return;
  }

  switch(rxChar)
  {
    case 'a':
    case 'A':
    case 'p':
        Serial.print(F("Channel "));
        Serial.print(channel);
        Serial.println(F(": Setting PTP/Absolute Mode..."));
        printDoneOrFailed(API_C3_setPtpMode(channel));
        break;

    case 'm':
    case 'r':
        Serial.print(F("Channel "));
        Serial.print(channel);
        Serial.println(F(": Setting Mouse/Relative Mode..."));
        printDoneOrFailed(API_C3_setRelativeMode(channel));
        break;

    case 'f':
        Serial.print(F("Channel "));
        Serial.print(channel);
        Serial.println(F(": Enabling Feed..."));
        printDoneOrFailed(API_C3_enableFeed(channel));
        break;

    case 'F':
        Serial.print(F("Channel "));
        Serial.print(channel);
        Serial.println(F(": Disabling Feed..."));
        printDoneOrFailed(API_C3_disableFeed(channel));
        break;

    case 's':
        systemInfo_t sysInfo;
        API_C3_readSystemInfo(channel, &sysInfo);
        printSystemInfo(channel, &sysInfo);
        break;

    case 'g':
        printHidDescriptor(channel, &touchpad_g[channel].hidDescriptor);
        break;

    case 'G':
        printReportDescriptor(channel);
        break;
  }
}

void printDoneOrFailed(bool ok)
{
  Serial.println(ok ? F("Done") : F("Failed")); // Failed: hardware timeout (did the module disconnect?)
}

void printHelpTable(void)
{
  Serial.println(F("Available Commands (case sensitive)"));
  Serial.println(F("(Replace '#' with I2C channel num [0,1])"));
  Serial.println();
  Serial.println(F("a#, A#, p#\t-\tSet to PTP/Absolute Mode"));
  Serial.println(F("r#, m#\t-\tSet to Mouse/Relative Mode"));
  Serial.println(F("\t\t(On Windows and Linux, the PC's touchpad driver sets the mode"));
  Serial.println(F("\t\t and may override this command)"));
  Serial.println(F("f#\t-\tEnable Feed (default)"));
  Serial.println(F("F#\t-\tDisable Feed"));
  Serial.println(F("s#\t-\tPrint System Info"));
  Serial.println(F("g#\t-\tPrint HID Descriptor"));
  Serial.println(F("G#\t-\tPrint Report Descriptor"));
  Serial.println(F("i\t-\tPrint USB Bridge Status (both touchpads)"));
  Serial.println();
  Serial.println(F("h, H, ?\t-\tPrint this Table"));
  Serial.println(F("d\t-\tTurn on Data Printing"));
  Serial.println(F("D\t-\tTurn off Data Printing (default)"));
  Serial.println(F("v\t-\tTurn on PC Request Printing (default)"));
  Serial.println(F("V\t-\tTurn off PC Request Printing"));
}

void printBridgeStatus(bool printCounts)
{
  for (uint8_t channel = 0; channel < NUM_CHANNELS; channel++)
  {
    const Touchpad_t *touchpad = &touchpad_g[channel];
    Serial.print(F("Channel "));
    Serial.print(channel);
    Serial.print(F(": "));
    switch (touchpad->status)
    {
      case TOUCHPAD_OK:
        Serial.print(F("Touchpad found, VID 0x"));
        Serial.print(touchpad->hidDescriptor.wVendorID, HEX);
        Serial.print(F(" PID 0x"));
        Serial.print(touchpad->hidDescriptor.wProductID, HEX);
        Serial.print(F(", report descriptor "));
        Serial.print(touchpad->hidDescriptor.wReportDescLength);
        Serial.print(F(" bytes, USB interface "));
        Serial.println(usb_ptp_interface_number(channel));
        if (touchpad->hidDescriptor.wMaxInputLength > INPUT_BUFFER_SIZE)
        {
          Serial.println(F("  Warning: the touchpad's longest input report is too long to forward"));
        }
        break;
      case TOUCHPAD_NOT_FOUND:
        Serial.println(F("Touchpad not found, so it has no USB interface."));
        Serial.println(F("  Check the touchpad connection, then reset the Teensy."));
        break;
      case TOUCHPAD_REPORT_DESC_TOO_LONG:
        Serial.print(F("The touchpad's report descriptor ("));
        Serial.print(touchpad->hidDescriptor.wReportDescLength);
        Serial.println(F(" bytes) is longer than USB_PTP_MAX_REPORT_DESC, so it has no USB interface."));
        break;
    }
    if (printCounts && touchpad->status == TOUCHPAD_OK)
    {
      Serial.print(F("  Reports forwarded: "));
      Serial.print(touchpad->reportsForwarded);
      Serial.print(F(", dropped: "));
      Serial.println(touchpad->reportsDropped);
      Serial.print(F("  PC report requests: "));
      Serial.print(touchpad->hostRequests);
      Serial.print(F(", failed: "));
      Serial.println(touchpad->hostRequestsFailed);
    }
  }
  if (touchpad_g[0].status != TOUCHPAD_OK && touchpad_g[1].status != TOUCHPAD_OK)
  {
    Serial.println(F("No touchpad is available, so only USB Serial is enabled."));
  }
  Serial.print(F("USB: "));
  Serial.println(usb_ptp_configured() ? F("configured by the PC") : F("not configured"));
}

void printChannelPrefix(uint8_t channel)
{
  Serial.print(F("I2C_Chan "));
  Serial.print(channel);
  Serial.print(F(" -> "));
}

void printSystemInfo(uint8_t channel, systemInfo_t* sysInfo)
{
  Serial.print(F("Channel "));
  Serial.print(channel);
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
  Serial.println();
}

void printHidDescriptor(uint8_t channel, HIDDescriptor_t* hidDescriptor)
{
  Serial.print(F("Channel "));
  Serial.print(channel);
  Serial.println(F(" HID Descriptor"));
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

// Prints the report descriptor the PC is given for one touchpad, 16 bytes per line, so it can
// be pasted into a HID descriptor parser.
void printReportDescriptor(uint8_t channel)
{
  const Touchpad_t *touchpad = &touchpad_g[channel];
  Serial.print(F("Channel "));
  Serial.print(channel);
  if (touchpad->status != TOUCHPAD_OK)
  {
    Serial.println(F(": no report descriptor, the touchpad was not set up at start-up"));
    return;
  }
  Serial.print(F(" Report Descriptor ("));
  Serial.print(touchpad->hidDescriptor.wReportDescLength);
  Serial.println(F(" bytes)"));
  for (uint16_t i = 0; i < touchpad->hidDescriptor.wReportDescLength; i++)
  {
    if (touchpad->reportDescriptor[i] < 0x10) Serial.print('0');
    Serial.print(touchpad->reportDescriptor[i], HEX);
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
