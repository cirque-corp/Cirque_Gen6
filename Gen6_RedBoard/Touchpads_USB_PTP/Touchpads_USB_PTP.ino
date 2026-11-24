// Copyright (c) 2026 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

// USB Precision Touchpad (PTP) Example
//   The Teensy passes each touchpad's own HID interface through to the PC over USB, so the PC
//   sees a Windows Precision Touchpad (cursor, tap, multi-finger gestures) for each touchpad,
//   instead of a mouse. Input reports are forwarded unchanged, and the PC's feature-report
//   requests (Device Capabilities, certification status, Input Mode, ...) are forwarded to the
//   right touchpad over I2C.
//
//   Works with one or two touchpads, on either connector. At start-up the sketch checks both
//   connectors and gives a USB interface only to the touchpads that answer.
//
//   You must select "Serial" from the "Tools > USB Type" menu. The sketch adds the touchpad
//   interfaces to that USB type itself (see usb_ptp_dev.c and usb_ptp_desc.c).

#include "API_C3.h"         /** < Provides API calls to interact with API_C3 firmware */
#include "API_Hardware.h"
#include "API_HostBus.h"    /** < Provides I2C connection to module */
#include "I2C.h"
#include "usb_ptp.h"        /** < USB touchpad interfaces */

// The kit uses a Teensy 4.0 as the host, with two touchpad connectors. Every API_C3_, HB_ and
// usb_ptp_ function takes an I2C channel parameter that selects the touchpad:
//   Touchpad #1 - connector J7 - I2C channel 0 (Wire):  IO18_SDA, IO19_SCL, IO9_DR0
//   Touchpad #2 - connector J9 - I2C channel 1 (Wire1): IO17_SDA1, IO16_SCL1, IO7_DR1
// The Serial Monitor shows touchpad numbers (1 and 2, as printed on the board), which are the
// I2C channel + 1.
#define NUM_TOUCHPADS               USB_PTP_MAX_TOUCHPADS
#define HID_DESCRIPTOR_REGISTER     0x0020
#define TOUCHPAD_DETECT_TIMEOUT_MS  100   // how long to wait for a touchpad to answer at start-up
#define TOUCHPAD_STARTUP_TIMEOUT_MS 2000  // how long an answering touchpad may take to provide its HID descriptor
#define RESET_RESPONSE_TIMEOUT_MS   1000
#define TOUCHPAD_DIGIT_TIMEOUT_MS   20    // how long to wait for the touchpad number after a command
#define ALL_TOUCHPADS               0xFF  // command target: every connected touchpad
#define INVALID_TOUCHPAD            0xFE  // command target: a number that is not 1 or 2
#define INPUT_BUFFER_SIZE           (USB_PTP_MAX_INPUT_REPORT + 2)  // + 2-byte I2C-HID length

bool dataPrint_mode_g = false;    /** < toggle for printing forwarded input reports > */
bool requestLog_mode_g = true;    /** < toggle for printing the PC's report requests > */

typedef enum
{
  TOUCHPAD_OK,
  TOUCHPAD_NOT_CONNECTED,         // no answer on I2C
  TOUCHPAD_NO_HID_DESCRIPTOR,     // answered on I2C, but gave no valid HID descriptor
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

static Touchpad_t touchpad_g[NUM_TOUCHPADS];
static uint8_t requestData_g[USB_PTP_MAX_CONTROL_REPORT];

static const uint8_t drMask_g[NUM_TOUCHPADS] = { DR0_MASK, DR1_MASK };

void setup()
{
  Serial.begin(115200);

  API_Hardware_init();       //Initialize board hardware

  delay(2);                  //delay for power up

  // initialize both i2c connections at 400kHz
  API_C3_init(PROJECT_I2C_FREQUENCY, CIRQUE_I2C_ADDR, PROJECT_I2C_FREQUENCY, CIRQUE_I2C_ADDR);

  // USB is not connected yet: the PC reads the report descriptors as soon as it is, so they must be ready first.
  // Only touchpads that answer on I2C are set up, so a missing touchpad doesn't delay start-up.
  detectTouchpads();
  for (uint8_t i2c_channel = 0; i2c_channel < NUM_TOUCHPADS; i2c_channel++)
  {
    Touchpad_t *touchpad = &touchpad_g[i2c_channel];
    if (touchpad->status == TOUCHPAD_OK)
    {
      touchpad->status = readTouchpadDescriptors(i2c_channel);
    }
    if (touchpad->status == TOUCHPAD_OK)
    {
      usb_ptp_set_report_descriptor(i2c_channel, touchpad->reportDescriptor, touchpad->hidDescriptor.wReportDescLength);
    }
  }
  usb_ptp_attach();

  // Give the Serial Monitor a moment to connect, but don't require it: the touchpads must work without it
  uint32_t start = millis();
  while (!Serial && (millis() - start) < 2000);

  Serial.println(F("Gen6 Touchpad Dev Kit - USB PTP Demo"));
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
  for (uint8_t i2c_channel = 0; i2c_channel < NUM_TOUCHPADS; i2c_channel++)
  {
    if ((touchpad_g[i2c_channel].status == TOUCHPAD_OK) && (dr_status & drMask_g[i2c_channel]))
    {
      forwardInputReport(i2c_channel);
    }
  }
  serviceHostRequest();

  /* Handle incoming messages from user on serial */
  if (Serial.available())
  {
    char rxChar = Serial.read();
    if (rxChar != '\r' && rxChar != '\n' && rxChar != ' ')
    {
      processSerialCommand(rxChar);
    }
  }
}

/******** Touchpad detection and set-up ***********/

/** Checks both touchpad connectors with a quick I2C ping. Touchpads that answer get status
    TOUCHPAD_OK, the others TOUCHPAD_NOT_CONNECTED. A touchpad that is not connected costs at
    most TOUCHPAD_DETECT_TIMEOUT_MS. */
void detectTouchpads(void)
{
  bool answered[NUM_TOUCHPADS] = {false};
  uint32_t start = millis();
  bool allFound;
  do
  {
    allFound = true;
    for (uint8_t i2c_channel = 0; i2c_channel < NUM_TOUCHPADS; i2c_channel++)
    {
      if (!answered[i2c_channel])
      {
        answered[i2c_channel] = (i2cPing(i2c_channel, CIRQUE_I2C_ADDR) == 0);  // 0 = no error
        allFound = allFound && answered[i2c_channel];
      }
    }
  } while (!allFound && (millis() - start) < TOUCHPAD_DETECT_TIMEOUT_MS);

  for (uint8_t i2c_channel = 0; i2c_channel < NUM_TOUCHPADS; i2c_channel++)
  {
    touchpad_g[i2c_channel].status = answered[i2c_channel] ? TOUCHPAD_OK : TOUCHPAD_NOT_CONNECTED;
  }
}

/** Returns 0 if a device acknowledges its address on the given I2C channel, or the I2C error code. */
uint8_t i2cPing(uint8_t i2c_channel, uint8_t i2cAddr)
{
  // An address-only write: the device acknowledges if it is there
  I2C_beginTransmission(i2c_channel, i2cAddr);
  return I2C_endTransmission(i2c_channel, true);
}

/** Reads one touchpad's HID descriptor and report descriptor over I2C, the same way
    a PC's I2C-HID driver does, and leaves the touchpad freshly reset. */
TouchpadStatus_t readTouchpadDescriptors(uint8_t i2c_channel)
{
  Touchpad_t *touchpad = &touchpad_g[i2c_channel];
  touchpad->inputReadLength = INPUT_BUFFER_SIZE;

  uint32_t start = millis();
  while (!hidDescriptorValid(&touchpad->hidDescriptor))
  {
    if ((millis() - start) >= TOUCHPAD_STARTUP_TIMEOUT_MS)
    {
      return TOUCHPAD_NO_HID_DESCRIPTOR;
    }
    HB_HID_GetHidDescriptor(i2c_channel, HID_DESCRIPTOR_REGISTER, &touchpad->hidDescriptor);
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
  HB_HID_Reset(i2c_channel);
  waitForResetResponse(i2c_channel);

  HB_HID_readRegister(i2c_channel, touchpad->hidDescriptor.wReportDescRegister, touchpad->reportDescriptor,
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
void waitForResetResponse(uint8_t i2c_channel)
{
  uint8_t packet[INPUT_BUFFER_SIZE];
  uint32_t start = millis();
  while ((millis() - start) < RESET_RESPONSE_TIMEOUT_MS)
  {
    if (HB_DR_Asserted() & drMask_g[i2c_channel])
    {
      HB_readReport(i2c_channel, packet, sizeof(packet));
      if ((packet[0] | (packet[1] << 8)) == 0)
      {
        return;
      }
    }
  }
}

/******** Forwarding between the touchpads and USB ***********/

/** Reads one report from a touchpad and sends it to the PC without changing it. */
void forwardInputReport(uint8_t i2c_channel)
{
  Touchpad_t *touchpad = &touchpad_g[i2c_channel];
  uint8_t packet[INPUT_BUFFER_SIZE];
  HB_readReport(i2c_channel, packet, touchpad->inputReadLength);

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
  if (usb_ptp_send_input_report(i2c_channel, report, reportLength) == 0)
  {
    touchpad->reportsForwarded++;
  }
  else
  {
    touchpad->reportsDropped++; // the PC hasn't configured the device yet, or isn't reading
  }

  if (dataPrint_mode_g && Serial)
  {
    printTouchpadPrefix(i2c_channel);
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

  uint8_t i2c_channel = request.touchpad;
  bool ok = false;
  uint16_t length = 0;
  if (i2c_channel >= NUM_TOUCHPADS)
  {
    usb_ptp_complete_request(&request, false, NULL, 0);
    return;
  }
  if (request.isSet)
  {
    ok = HB_HID_setReport(i2c_channel, request.reportType, request.reportID, requestData_g, request.length);
    usb_ptp_complete_request(&request, ok, NULL, 0);
  }
  else
  {
    length = HB_HID_getReport(i2c_channel, request.reportType, request.reportID, requestData_g, request.length);
    ok = (length > 0);
    usb_ptp_complete_request(&request, ok, requestData_g, length);
  }

  Touchpad_t *touchpad = &touchpad_g[i2c_channel];
  touchpad->hostRequests++;
  if (!ok)
  {
    touchpad->hostRequestsFailed++;
  }

  if (requestLog_mode_g && Serial)
  {
    printTouchpadPrefix(i2c_channel);
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

/******** Serial commands ***********/

/** Reads the optional touchpad number ('1' or '2') that may follow a touchpad command letter.
    Returns the I2C channel of that touchpad, ALL_TOUCHPADS if no number follows, or
    INVALID_TOUCHPAD for any other digit. Waits briefly, because the number may arrive a moment
    after the command letter (the Serial Monitor sends the whole line at once). */
uint8_t readTouchpadNumber(void)
{
  uint32_t start = millis();
  while (!Serial.available() && (millis() - start) < TOUCHPAD_DIGIT_TIMEOUT_MS);
  if (!Serial.available())
  {
    return ALL_TOUCHPADS;
  }
  int next = Serial.peek();
  if (next < '0' || next > '9')
  {
    return ALL_TOUCHPADS;  // not a number: leave it for the next command
  }
  Serial.read();
  if (next >= '1' && next < ('1' + NUM_TOUCHPADS))
  {
    return next - '1';     // touchpad number 1 or 2 -> I2C channel 0 or 1
  }
  return INVALID_TOUCHPAD;
}

void processSerialCommand(char rxChar)
{
  // Commands that are not for a particular touchpad
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
        break;  // touchpad commands, handled below

    default:
        if (rxChar >= '0' && rxChar <= '9')
        {
          Serial.print(F("A touchpad number goes right after a command letter, for example 'a"));
          Serial.print(rxChar);
          Serial.println(F("'. Send 'h' for a list of commands."));
        }
        else
        {
          Serial.print(F("Invalid command: '"));
          Serial.print(rxChar);
          Serial.println(F("'. Send 'h' for a list of commands."));
        }
        return;
  }

  // Touchpad commands: no number = every connected touchpad, 1 or 2 = that touchpad only
  uint8_t target = readTouchpadNumber();
  if (target == INVALID_TOUCHPAD)
  {
    Serial.println(F("The touchpad number must be 1 or 2. Send 'h' for a list of commands."));
    return;
  }

  bool anySent = false;
  for (uint8_t i2c_channel = 0; i2c_channel < NUM_TOUCHPADS; i2c_channel++)
  {
    if (target != ALL_TOUCHPADS && target != i2c_channel)
    {
      continue;
    }
    if (touchpad_g[i2c_channel].status != TOUCHPAD_OK)
    {
      if (target != ALL_TOUCHPADS)
      {
        printTouchpadPrefix(i2c_channel);
        Serial.print(F("Touchpad #"));
        Serial.print(i2c_channel + 1);
        Serial.println(F(" was not set up at start-up. Send 'i' for details."));
      }
      continue;
    }
    processTouchpadCommand(rxChar, i2c_channel);
    anySent = true;
  }
  if (!anySent && target == ALL_TOUCHPADS)
  {
    Serial.println(F("No touchpads connected. Send 'i' for details."));
  }
}

/** Runs one touchpad command on the touchpad at i2c_channel. */
void processTouchpadCommand(char rxChar, uint8_t i2c_channel)
{
  switch(rxChar)
  {
    case 'a':
    case 'A':
    case 'p':
        printTouchpadPrefix(i2c_channel);
        Serial.println(F("Setting PTP/Absolute Mode..."));
        printDoneOrFailed(API_C3_setPtpMode(i2c_channel));
        break;

    case 'm':
    case 'r':
        printTouchpadPrefix(i2c_channel);
        Serial.println(F("Setting Mouse/Relative Mode..."));
        printDoneOrFailed(API_C3_setRelativeMode(i2c_channel));
        break;

    case 'f':
        printTouchpadPrefix(i2c_channel);
        Serial.println(F("Enabling Feed..."));
        printDoneOrFailed(API_C3_enableFeed(i2c_channel));
        break;

    case 'F':
        printTouchpadPrefix(i2c_channel);
        Serial.println(F("Disabling Feed..."));
        printDoneOrFailed(API_C3_disableFeed(i2c_channel));
        break;

    case 's':
        systemInfo_t sysInfo;
        API_C3_readSystemInfo(i2c_channel, &sysInfo);
        printSystemInfo(i2c_channel, &sysInfo);
        break;

    case 'g':
        printHidDescriptor(i2c_channel, &touchpad_g[i2c_channel].hidDescriptor);
        break;

    case 'G':
        printReportDescriptor(i2c_channel);
        break;
  }
}

void printDoneOrFailed(bool ok)
{
  Serial.println(ok ? F("Done") : F("Failed")); // Failed: hardware timeout (did the module disconnect?)
}

/******** Functions for Printing ***********/

/** Prints the label that starts every line about one touchpad: [TP1] or [TP2] */
void printTouchpadPrefix(uint8_t i2c_channel)
{
  Serial.print(F("[TP"));
  Serial.print(i2c_channel + 1);
  Serial.print(F("] "));
}

void printHelpTable(void)
{
  Serial.println(F("Available Commands (case sensitive)"));
  Serial.println(F("Touchpad commands go to every connected touchpad. To send one to a single"));
  Serial.println(F("touchpad, add its number (1 or 2), for example 'a2' for Touchpad #2 only."));
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
  Serial.println();
  Serial.println(F("i\t-\tPrint USB Bridge Status (all touchpads)"));
  Serial.println(F("h, H, ?\t-\tPrint this Table"));
  Serial.println(F("d\t-\tTurn on Data Printing"));
  Serial.println(F("D\t-\tTurn off Data Printing (default)"));
  Serial.println(F("v\t-\tTurn on PC Request Printing (default)"));
  Serial.println(F("V\t-\tTurn off PC Request Printing"));
}

void printBridgeStatus(bool printCounts)
{
  bool anyOk = false;
  for (uint8_t i2c_channel = 0; i2c_channel < NUM_TOUCHPADS; i2c_channel++)
  {
    const Touchpad_t *touchpad = &touchpad_g[i2c_channel];
    printTouchpadPrefix(i2c_channel);
    Serial.print(F("Touchpad #"));
    Serial.print(i2c_channel + 1);
    switch (touchpad->status)
    {
      case TOUCHPAD_OK:
        anyOk = true;
        Serial.print(F(" found, VID 0x"));
        Serial.print(touchpad->hidDescriptor.wVendorID, HEX);
        Serial.print(F(" PID 0x"));
        Serial.print(touchpad->hidDescriptor.wProductID, HEX);
        Serial.print(F(", report descriptor "));
        Serial.print(touchpad->hidDescriptor.wReportDescLength);
        Serial.print(F(" bytes, USB interface "));
        Serial.println(usb_ptp_interface_number(i2c_channel));
        if (touchpad->hidDescriptor.wMaxInputLength > INPUT_BUFFER_SIZE)
        {
          Serial.println(F("  Warning: the touchpad's longest input report is too long to forward"));
        }
        break;
      case TOUCHPAD_NOT_CONNECTED:
        Serial.println(F(" not connected, so it has no USB interface."));
        break;
      case TOUCHPAD_NO_HID_DESCRIPTOR:
        Serial.println(F(" answered on I2C but gave no valid HID descriptor, so it has no USB interface."));
        Serial.println(F("  Check the touchpad connection and firmware, then reset the Teensy."));
        break;
      case TOUCHPAD_REPORT_DESC_TOO_LONG:
        Serial.print(F("'s report descriptor ("));
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
  if (!anyOk)
  {
    Serial.println(F("No touchpads connected, so only USB Serial is enabled."));
    Serial.println(F("Check the touchpad connections, then reset the Teensy."));
  }
  Serial.print(F("USB: "));
  Serial.println(usb_ptp_configured() ? F("configured by the PC") : F("not configured"));
}

void printSystemInfo(uint8_t i2c_channel, systemInfo_t* sysInfo)
{
  printTouchpadPrefix(i2c_channel);
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

void printHidDescriptor(uint8_t i2c_channel, HIDDescriptor_t* hidDescriptor)
{
  printTouchpadPrefix(i2c_channel);
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

// Prints the report descriptor the PC is given for one touchpad, 16 bytes per line, so it can
// be pasted into a HID descriptor parser.
void printReportDescriptor(uint8_t i2c_channel)
{
  const Touchpad_t *touchpad = &touchpad_g[i2c_channel];
  printTouchpadPrefix(i2c_channel);
  Serial.print(F("Report Descriptor ("));
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
