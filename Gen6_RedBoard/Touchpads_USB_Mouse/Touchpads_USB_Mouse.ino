// Copyright (c) 2026 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

// USB Mouse Example
//   The Teensy becomes a USB mouse and keyboard: it reads the touchpads' reports and turns them
//   into USB mouse movement, button presses and key presses, so the PC's cursor moves when you
//   touch a touchpad. The reports are also shown in the Serial Monitor.
//
//   Works with one or two touchpads, on either connector. At start-up the sketch checks both
//   connectors and only uses the touchpads that answer. Both touchpads drive the same USB mouse.
//
//   You must select "Serial + Keyboard + Mouse + Joystick" from the "Tools > USB Type" menu.

#include "API_C3.h"         /** < Provides API calls to interact with API_C3 firmware */
#include "API_Hardware.h"
#include "API_HostBus.h"    /** < Provides I2C connection to module */
#include "HID_Reports.h"
#include "I2C.h"
#include <string.h>

#if !defined(MOUSE_INTERFACE) || !defined(KEYBOARD_INTERFACE)
#error "Within the Arduino IDE, please select Tools > USB Type > 'Serial + Keyboard + Mouse + Joystick' - the Mouse/Keyboard objects used by this sketch are only declared for that USB Type."
#endif

#define USE_DR_I2C 0 // Reads out if data is ready through I2C instead of the interrupt pin

// The kit uses a Teensy 4.0 as the host, with two touchpad connectors. Every API_C3_ and HB_
// function takes an i2c_channel parameter that selects the touchpad:
//   Touchpad #1 - connector J7 - I2C channel 0 (Wire):  IO18_SDA, IO19_SCL, IO9_DR0
//   Touchpad #2 - connector J9 - I2C channel 1 (Wire1): IO17_SDA1, IO16_SCL1, IO7_DR1
// The Serial Monitor shows touchpad numbers (1 and 2, as printed on the board), which are the
// I2C channel + 1.
#define NUM_TOUCHPADS               2
#define TOUCHPAD_DETECT_TIMEOUT_MS  100   // how long to wait for a touchpad to answer at start-up
#define TOUCHPAD_DIGIT_TIMEOUT_MS   20    // how long to wait for the touchpad number after a command
#define ALL_TOUCHPADS               0xFF  // command target: every connected touchpad
#define INVALID_TOUCHPAD            0xFE  // command target: a number that is not 1 or 2

bool dataPrint_mode_g = true;  /** < toggle for printing out data > */
bool eventPrint_mode_g = true; /** < toggle for printing off events */
bool rawDump_mode_g = false;   /** < toggle for printing raw HID packet bytes > */
bool decodeDebug_mode_g = true; /** < toggle for detailed decode failure diagnostics > */

bool touchpadConnected_g[NUM_TOUCHPADS] = {false, false};
uint32_t decodeFailCount_g[NUM_TOUCHPADS] = {0, 0};
static const uint8_t drMask_g[NUM_TOUCHPADS] = {DR0_MASK, DR1_MASK};

typedef struct
{
  bool pending;
  uint16_t timeStamp;
  uint8_t contactCount;
  uint8_t buttons;
  uint8_t packetCount;
  uint8_t maxSlotsPerPacket;
  PtpContact_t contacts[PTP_MAX_CONTACTS];
  bool contactReady[PTP_MAX_CONTACTS];
} PtpPrintFrame_t;

static PtpPrintFrame_t ptpPrintFrame_g[NUM_TOUCHPADS];
static void clearPtpPrintFrame(PtpPrintFrame_t *frame);
static uint8_t countActivePtpContacts(const PtpPrintFrame_t *frame);
static void printAndClearPtpFrame(uint8_t i2c_channel, PtpPrintFrame_t *frame);

/** @ingroup prevReports
    These reports store the state of the most recent reports by type, for each touchpad.
    Determining if an event occurred requires information about the previous state.
*/
HID_report_t prevMouseReport_g[NUM_TOUCHPADS];     /**< Most recent past Mouse report */
HID_report_t prevPtpReport_g[NUM_TOUCHPADS];       /**< Most recent past PTP report */
HID_report_t prevKeyboardReport_g[NUM_TOUCHPADS];  /**< Most recent past Keyboard report */

/** What the USB mouse needs to remember about each touchpad */
typedef struct
{
  bool buttonPressed;      /**< this touchpad's left button is down */
  bool ptpTracking;        /**< in PTP mode, a finger is down and prevPtpX/Y hold its last position */
  uint8_t ptpContactID;    /**< contact ID of the finger being tracked */
  uint16_t prevPtpX;
  uint16_t prevPtpY;
} MouseState_t;

static MouseState_t mouseState_g[NUM_TOUCHPADS];
static bool usbButtonPressed_g = false;  /**< the USB mouse's left button is down */

void printDecodeFailureDebug(uint8_t i2c_channel, const uint8_t* packet, uint8_t dr_status);
void performMouseReport(uint8_t i2c_channel, HID_report_t* report);
void performKeyboardReport(HID_report_t* report);

void setup()
{
  Serial.begin(115200);
  while(!Serial);

  API_Hardware_init();       //Initialize board hardware

  delay(2);                  //delay for power up

  // initialize both i2c connections at 400kHz
  API_C3_init(PROJECT_I2C_FREQUENCY, CIRQUE_I2C_ADDR, PROJECT_I2C_FREQUENCY, CIRQUE_I2C_ADDR);

  Serial.println(F("Gen6 Touchpad Dev Kit - USB Mouse Demo"));

  if (detectTouchpads())
  {
    delay(50);               //delay before reading registers after startup

    for (uint8_t i2c_channel = 0; i2c_channel < NUM_TOUCHPADS; i2c_channel++)
    {
      if (touchpadConnected_g[i2c_channel])
      {
        // Collect information about the system and print to Serial
        systemInfo_t sysInfo;
        API_C3_readSystemInfo(i2c_channel, &sysInfo);
        printSystemInfo(i2c_channel, &sysInfo);

        initialize_saved_reports(i2c_channel); //initialize state for determining touch events
      }
    }
  }
  else
  {
    Serial.println(F("No touchpads connected. Check the touchpad connections, then reset the Teensy."));
  }
  Serial.println(F("Send 'h' for a list of commands"));
  Serial.println();
}

/** The main structure of the loop is:
    Wait for a Data Ready line to assert. When it does, read the data (which clears DR) and analyze the data.
    The rest is just a user interface to change various settings.
    */
void loop()
{
  /* Handle incoming messages from modules */
  #if USE_DR_I2C
  uint8_t dr_status = API_C3_DR_Asserted_ViaI2C();
  #else
  uint8_t dr_status = API_C3_DR_Asserted();
  #endif

  for (uint8_t i2c_channel = 0; i2c_channel < NUM_TOUCHPADS; i2c_channel++)
  {
    if (touchpadConnected_g[i2c_channel] && (dr_status & drMask_g[i2c_channel]))  // When Data is ready
    {
      HID_report_t report;
      if (readAndPrintReport(i2c_channel, dr_status, &report))
      {
        if (report.reportID == KEY_REPORT_ID)
        {
          performKeyboardReport(&report);
        }
        else
        {
          performMouseReport(i2c_channel, &report);
        }
      }
    }
  }

  /* Handle incoming messages from user on serial */
  if(Serial.available())
  {
    char rxChar = Serial.read();
    if (rxChar != '\r' && rxChar != '\n' && rxChar != ' ')
    {
      processSerialCommand(rxChar);
    }
  }
}

/** Reads one report from a touchpad into report and prints it.
    Returns false if the report could not be decoded. */
bool readAndPrintReport(uint8_t i2c_channel, uint8_t dr_status, HID_report_t *report)
{
  uint8_t packet[PROJECT_MAX_PACKET_SIZE];
  memset(packet, 0, sizeof(packet));
  HB_readReport(i2c_channel, packet, PROJECT_MAX_PACKET_SIZE); // read the raw report bytes
  bool decode_ok = HID_decodeReport(packet, report);

  if(rawDump_mode_g)
  {
    uint16_t packetLength = ((uint16_t)packet[1] << 8) | packet[0];
    if(packetLength == 0 || packetLength > PROJECT_MAX_PACKET_SIZE)
    {
      packetLength = PROJECT_MAX_PACKET_SIZE;
    }
    printRawHidPacket(i2c_channel, packet, packetLength);
  }

  if(!decode_ok)
  {
    printTouchpadPrefix(i2c_channel);
    Serial.println(F("Decode failed"));
    decodeFailCount_g[i2c_channel]++;
    if(decodeDebug_mode_g)
    {
      printDecodeFailureDebug(i2c_channel, packet, dr_status);
    }
    return false;
  }

  /* Interpret report from module */
  if(eventPrint_mode_g)
  {
    printEvent(i2c_channel, report);
  }
  if(dataPrint_mode_g)
  {
    printDataReport(i2c_channel, report);
  }
  return true;
}

/******** USB mouse and keyboard ***********/

/** Moves the USB mouse and presses or releases its left button from one touchpad report.
    Both touchpads drive the same USB mouse. */
void performMouseReport(uint8_t i2c_channel, HID_report_t* report)
{
  MouseState_t *state = &mouseState_g[i2c_channel];

  if (MOUSE_REPORT_ID == report->reportID)
  {
    // Mouse/Relative mode: the touchpad already reports movement
    Mouse.move(report->mouse.xDelta, report->mouse.yDelta, report->mouse.scrollDelta, report->mouse.panDelta);
    state->buttonPressed = (report->mouse.buttons & BUTTON_1_MASK) != 0;
  }
  else if (PTP_REPORT_ID == report->reportID)
  {
    // PTP/Absolute mode: turn the first finger's position into movement
    if (!report->ptp.tip)
    {
      state->ptpTracking = false;   // finger lifted
    }
    else if (!state->ptpTracking || report->ptp.contactID != state->ptpContactID)
    {
      // skip a frame of movement to set the starting PTP location
      state->ptpTracking = true;
      state->ptpContactID = report->ptp.contactID;
      state->prevPtpX = report->ptp.x;
      state->prevPtpY = report->ptp.y;
    }
    else
    {
      int16_t deltaPtpX = report->ptp.x - state->prevPtpX;
      int16_t deltaPtpY = report->ptp.y - state->prevPtpY;
      Mouse.move(deltaPtpX, deltaPtpY);
      state->prevPtpX = report->ptp.x;
      state->prevPtpY = report->ptp.y;
    }
    state->buttonPressed = (report->ptp.buttons & BUTTON_1_MASK) != 0;
  }
  else
  {
    return;
  }

  // The USB mouse's left button is down while either touchpad's button is down
  bool anyButtonPressed = false;
  for (uint8_t i = 0; i < NUM_TOUCHPADS; i++)
  {
    anyButtonPressed = anyButtonPressed || mouseState_g[i].buttonPressed;
  }
  if (anyButtonPressed && !usbButtonPressed_g)
  {
    Mouse.press();
  }
  else if (!anyButtonPressed && usbButtonPressed_g)
  {
    Mouse.release();
  }
  usbButtonPressed_g = anyButtonPressed;
}

/** Drives the USB HID keyboard interface using the raw boot-keyboard fields
    (modifier byte + up to 6 keycodes) decoded from a keyboard report. */
void performKeyboardReport(HID_report_t* report)
{
  Keyboard.set_modifier(report->keyboard.modifier1);
  Keyboard.set_key1(report->keyboard.keycode[0]);
  Keyboard.set_key2(report->keyboard.keycode[1]);
  Keyboard.set_key3(report->keyboard.keycode[2]);
  Keyboard.set_key4(report->keyboard.keycode[3]);
  Keyboard.set_key5(report->keyboard.keycode[4]);
  Keyboard.set_key6(report->keyboard.keycode[5]);
  Keyboard.send_now();
}

/******** Touchpad detection ***********/

/** Checks both touchpad connectors and records which touchpads answer. A touchpad that is
    not connected costs at most TOUCHPAD_DETECT_TIMEOUT_MS. Returns true if any touchpad answered. */
bool detectTouchpads(void)
{
  uint8_t i2c_error[NUM_TOUCHPADS];
  uint32_t start = millis();
  bool allFound;
  do
  {
    allFound = true;
    for (uint8_t i2c_channel = 0; i2c_channel < NUM_TOUCHPADS; i2c_channel++)
    {
      if (!touchpadConnected_g[i2c_channel])
      {
        i2c_error[i2c_channel] = i2cPing(i2c_channel, CIRQUE_I2C_ADDR);
        touchpadConnected_g[i2c_channel] = (i2c_error[i2c_channel] == 0);  // 0 = no error
        allFound = allFound && touchpadConnected_g[i2c_channel];
      }
    }
  } while (!allFound && (millis() - start) < TOUCHPAD_DETECT_TIMEOUT_MS);

  bool anyFound = false;
  for (uint8_t i2c_channel = 0; i2c_channel < NUM_TOUCHPADS; i2c_channel++)
  {
    printTouchpadPrefix(i2c_channel);
    Serial.print(F("Touchpad #"));
    Serial.print(i2c_channel + 1);
    if (touchpadConnected_g[i2c_channel])
    {
      Serial.println(F(" found"));
      anyFound = true;
    }
    else
    {
      Serial.print(F(" not connected (no answer at I2C address 0x"));
      Serial.print(CIRQUE_I2C_ADDR, HEX);
      Serial.print(F(", I2C error "));
      Serial.print(i2c_error[i2c_channel]);
      Serial.println(F(")"));
    }
  }
  return anyFound;
}

/** Returns 0 if a device acknowledges its address on the given I2C channel, or the I2C error code. */
uint8_t i2cPing(uint8_t i2c_channel, uint8_t i2cAddr)
{
  // An address-only write: the device acknowledges if it is there
  I2C_beginTransmission(i2c_channel, i2cAddr);
  return I2C_endTransmission(i2c_channel, true);
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
    //Print modes
    case 'd':
        Serial.println(F("Data Printing turned on"));
        dataPrint_mode_g = true;
        return;

    case 'D':
        Serial.println(F("Data Printing turned off"));
        dataPrint_mode_g = false;
        return;

    case 'e':
        Serial.println(F("Event Printing turned on"));
        eventPrint_mode_g = true;
        return;

    case 'E':
        Serial.println(F("Event Printing turned off"));
        eventPrint_mode_g = false;
        return;

    case 'x':
        Serial.println(F("Raw HID packet dump turned on"));
        rawDump_mode_g = true;
        return;

    case 'X':
        Serial.println(F("Raw HID packet dump turned off"));
        rawDump_mode_g = false;
        return;

    case 'z':
        Serial.println(F("Decode debug diagnostics turned on"));
        decodeDebug_mode_g = true;
        return;

    case 'Z':
        Serial.println(F("Decode debug diagnostics turned off"));
        decodeDebug_mode_g = false;
        return;

    case '?':
    case 'h':
    case 'H':
        printHelpTable();
        return;

    case 'a':
    case 'A':
    case 'p':
    case 'r':
    case 'm':
    case 'f':
    case 'F':
    case 's':
    case 'g':
    case 'b':
    case 'B':
    case 'c':
    case 'C':
    case 'v':
    case 'V':
    case 't':
    case 'T':
    case 'i':
    case 'S':
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
    if (!touchpadConnected_g[i2c_channel])
    {
      if (target != ALL_TOUCHPADS)
      {
        printTouchpadPrefix(i2c_channel);
        Serial.print(F("Touchpad #"));
        Serial.print(i2c_channel + 1);
        Serial.println(F(" is not connected (it did not answer at start-up). Connect it, then reset the Teensy."));
      }
      continue;
    }
    processTouchpadCommand(rxChar, i2c_channel);
    anySent = true;
  }
  if (!anySent && target == ALL_TOUCHPADS)
  {
    Serial.println(F("No touchpads connected. Check the touchpad connections, then reset the Teensy."));
  }
  Serial.println();
}

/** Runs one touchpad command on the touchpad at i2c_channel. */
void processTouchpadCommand(char rxChar, uint8_t i2c_channel)
{
  switch(rxChar)
  {
    case 'c':
        printTouchpadPrefix(i2c_channel);
        Serial.println(F("Force Compensation..."));
        printDoneOrFailed(API_C3_forceComp(i2c_channel));
        break;

    case 'C':
        printTouchpadPrefix(i2c_channel);
        Serial.println(F("Factory Calibrate..."));
        printDoneOrFailed(API_C3_factoryCalibrate(i2c_channel));
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

    case 'p':
    case 'a':
    case 'A':
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

    case 's':
        systemInfo_t sysInfo;
        API_C3_readSystemInfo(i2c_channel, &sysInfo);
        printSystemInfo(i2c_channel, &sysInfo);
        break;

    case 'g':
        HIDDescriptor_t hidDescriptor;
        HB_HID_GetHidDescriptor(i2c_channel, 0x0020, &hidDescriptor);
        printHidDescriptor(i2c_channel, &hidDescriptor);
        break;

    case 'S':
        printTouchpadPrefix(i2c_channel);
        Serial.println(F("Saving settings to flash..."));
        printDoneOrFailed(API_C3_saveConfig(i2c_channel));
        break;

    case 't':
        printTouchpadPrefix(i2c_channel);
        Serial.println(F("Enabling Tracking..."));
        printDoneOrFailed(API_C3_enableTracking(i2c_channel));
        break;

    case 'T':
        printTouchpadPrefix(i2c_channel);
        Serial.println(F("Disabling Tracking..."));
        printDoneOrFailed(API_C3_disableTracking(i2c_channel));
        break;

    case 'v':
        printTouchpadPrefix(i2c_channel);
        Serial.println(F("Enabling Compensation..."));
        printDoneOrFailed(API_C3_enableComp(i2c_channel));
        break;

    case 'V':
        printTouchpadPrefix(i2c_channel);
        Serial.println(F("Disabling Compensation..."));
        printDoneOrFailed(API_C3_disableComp(i2c_channel));
        break;

    case 'i':
        printTouchpadPrefix(i2c_channel);
        Serial.println(F("Reading comp matrix..."));
        printCompMatrix(i2c_channel);
        break;

    case 'b':
        printTouchpadPrefix(i2c_channel);
        Serial.println(F("Adaptive Touch Enabled"));
        API_C3_enableAdaptiveTouch(i2c_channel);
        break;

    case 'B':
        printTouchpadPrefix(i2c_channel);
        Serial.println(F("Adaptive Touch Disabled"));
        API_C3_disableAdaptiveTouch(i2c_channel);
        break;
  }
}

void printDoneOrFailed(bool ok)
{
  Serial.println(ok ? F("Done") : F("Failed")); // Failed: hardware timeout (did the module disconnect?)
}

/******** Functions for Printing Data ***********/

/** Prints the label that starts every line about one touchpad: [TP1] or [TP2] */
void printTouchpadPrefix(uint8_t i2c_channel)
{
  Serial.print(F("[TP"));
  Serial.print(i2c_channel + 1);
  Serial.print(F("] "));
}

/** Prints a simple table of available commands.
    Commands can be sent over serial through Serial Monitor in Arduino IDE*/
void printHelpTable()
{
  Serial.println(F("Available Commands (case sensitive)"));
  Serial.println(F("Touchpad commands go to every connected touchpad. To send one to a single"));
  Serial.println(F("touchpad, add its number (1 or 2), for example 'a2' for Touchpad #2 only."));
  Serial.println(F(""));
  Serial.println(F("a, A, p\t-\tSet to PTP/Absolute Mode"));
  Serial.println(F("r, m\t-\tSet to Mouse/Relative Mode (default)"));
  Serial.println(F("f\t-\tEnable Feed (default)"));
  Serial.println(F("F\t-\tDisable Feed"));
  Serial.println(F("s\t-\tPrint System Info"));
  Serial.println(F("g\t-\tPrint HID Descriptor"));
  Serial.println(F("b\t-\tEnable Adaptive Touch"));
  Serial.println(F("B\t-\tDisable Adaptive Touch"));
  Serial.println(F("c\t-\tForce Compensation"));
  Serial.println(F("C\t-\tFactory Calibrate (nothing may touch the touchpad)"));
  Serial.println(F("v\t-\tEnable Compensation (default)"));
  Serial.println(F("V\t-\tDisable Compensation"));
  Serial.println(F("t\t-\tEnable Tracking (default)"));
  Serial.println(F("T\t-\tDisable Tracking"));
  Serial.println(F("i\t-\tPrint Compensation Matrix"));
  Serial.println(F("S\t-\tSave Settings to Flash"));
  Serial.println(F(""));
  Serial.println(F("h, H, ?\t-\tPrint this Table"));
  Serial.println(F("d\t-\tTurn on Data Printing (default)"));
  Serial.println(F("D\t-\tTurn off Data Printing"));
  Serial.println(F("e\t-\tTurn on Event Printing (default)"));
  Serial.println(F("E\t-\tTurn off Event Printing"));
  Serial.println(F("z\t-\tTurn on decode failure diagnostics (default)"));
  Serial.println(F("Z\t-\tTurn off decode failure diagnostics"));
  Serial.println(F("x\t-\tTurn on raw HID packet dump"));
  Serial.println(F("X\t-\tTurn off raw HID packet dump"));
  Serial.println(F(""));
}

void printRawHidPacket(uint8_t i2c_channel, const uint8_t* packet, uint16_t packetLength)
{
  char strBuf[60];
  printTouchpadPrefix(i2c_channel);
  sprintf(strBuf, "RAW[%u]:", packetLength);
  Serial.println(strBuf);

  for(uint16_t i = 0; i < packetLength; i++)
  {
    if((i % 16) == 0)
    {
      sprintf(strBuf, "  %02u:", i);
      Serial.print(strBuf);
    }

    sprintf(strBuf, " %02X", packet[i]);
    Serial.print(strBuf);

    if(((i % 16) == 15) || (i == (packetLength - 1)))
    {
      Serial.println();
    }
  }
}

void printDecodeFailureDebug(uint8_t i2c_channel, const uint8_t* packet, uint8_t dr_status)
{
  uint16_t packetLength = ((uint16_t)packet[1] << 8) | packet[0];
  uint8_t reportId = packet[2];
  uint8_t touchpad = i2c_channel + 1;

  Serial.printf("[TP%u] Decode failure #%lu\n", touchpad, (unsigned long)decodeFailCount_g[i2c_channel]);
  Serial.printf("[TP%u] DR status at read: %u\n", touchpad, dr_status);
  Serial.printf("[TP%u] Packet length from header: %u (max expected %u)\n", touchpad, packetLength, PROJECT_MAX_PACKET_SIZE);
  Serial.printf("[TP%u] Report ID from header: 0x%02X\n", touchpad, reportId);

  if(packetLength == 0 || packetLength > PROJECT_MAX_PACKET_SIZE)
  {
    Serial.printf("[TP%u] Possible cause: invalid report length in packet header\n", touchpad);
  }
  if((reportId != PTP_REPORT_ID) && (reportId != MOUSE_REPORT_ID) && (reportId != KEY_REPORT_ID))
  {
    Serial.printf("[TP%u] Possible cause: unexpected report ID (not PTP/MOUSE/KEYBOARD)\n", touchpad);
  }

  Serial.printf("[TP%u] First 12 packet bytes:\n", touchpad);
  for(uint8_t i = 0; i < 12 && i < PROJECT_MAX_PACKET_SIZE; i++)
  {
    Serial.printf("%02X ", packet[i]);
  }
  Serial.println();
}

/** Prints a systemInfo_t struct to Serial.
    See API_C3.h for more information about the systemInfo_t struct */
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
  Serial.println(F(""));
}

void printHidDescriptor(uint8_t i2c_channel, HIDDescriptor_t * hd)
{
  printTouchpadPrefix(i2c_channel);
  Serial.println(F("HID Descriptor"));
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
    case KEY_REPORT_ID:
        printKeyboardReport(i2c_channel, report);
        break;
    default:
        printTouchpadPrefix(i2c_channel);
        Serial.println(F("Error: Unknown Report ID"));
  }
}

/**************************************************************/
/*********** KEYBOARD MODIFIER AND KEYCODE HELPERS ************/

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

/** Prints the information stored in a mouse report to serial */
void printMouseReport(uint8_t i2c_channel, HID_report_t* report)
{
  char strBuf[50];
  uint8_t buttons = report->mouse.buttons;
  printTouchpadPrefix(i2c_channel);
  sprintf(strBuf,"ReportID: 0x%02X",report->reportID);
  Serial.print(strBuf);
  sprintf(strBuf,", Buttons: %c%c%c",((buttons & 1)?'L':'_'),((buttons & 4)?'C':'_'),((buttons & 2)?'R':'_'));
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

/** Prints the information stored in a keyboard report to serial */
void printKeyboardReport(uint8_t i2c_channel, HID_report_t* report)
{
  char strBuf[50];
  printTouchpadPrefix(i2c_channel);
  sprintf(strBuf,"ReportID: 0x%02X",report->reportID);
  Serial.print(strBuf);
  Serial.print(F(", Modifiers: "));
  printModifierNames(report->keyboard.modifier1);
  Serial.print(F(", Keycodes:"));
  for (uint8_t i = 0; i < 6; i++)
  {
    if(report->keyboard.keycode[i] != 0)
    {
      Serial.print(F(" "));
      printKeycodeName(report->keyboard.keycode[i]);
    }
  }
  Serial.println();
}

static void clearPtpPrintFrame(PtpPrintFrame_t *frame)
{
  frame->pending = false;
  frame->timeStamp = 0;
  frame->contactCount = 0;
  frame->buttons = 0;
  frame->packetCount = 0;
  frame->maxSlotsPerPacket = 0;
  for (uint8_t i = 0; i < PTP_MAX_CONTACTS; i++)
  {
    frame->contacts[i].contactID = i;
    frame->contacts[i].confidence = 0;
    frame->contacts[i].tip = 0;
    frame->contacts[i].x = 0;
    frame->contacts[i].y = 0;
    frame->contactReady[i] = false;
  }
}

static uint8_t countActivePtpContacts(const PtpPrintFrame_t *frame)
{
  uint8_t activeContacts = 0;
  for (uint8_t i = 0; i < PTP_MAX_CONTACTS; i++)
  {
    if (frame->contactReady[i] && frame->contacts[i].tip)
    {
      activeContacts++;
    }
  }
  return activeContacts;
}

static void printAndClearPtpFrame(uint8_t i2c_channel, PtpPrintFrame_t *frame)
{
  char strBuf[50];
  bool printedAnyContact = false;

  printTouchpadPrefix(i2c_channel);
  sprintf(strBuf, "ReportID: 0x%02X", PTP_REPORT_ID);
  Serial.print(strBuf);
  sprintf(strBuf, ", Time: %5d", frame->timeStamp);
  Serial.print(strBuf);
  sprintf(strBuf, ", Buttons: %d", frame->buttons);
  Serial.print(strBuf);
  sprintf(strBuf, ", Contact Count: %d", frame->contactCount);
  Serial.print(strBuf);
  Serial.println();

  for (uint8_t i = 0; i < PTP_MAX_CONTACTS; i++)
  {
    if (frame->contactReady[i] && frame->contacts[i].tip)
    {
      printedAnyContact = true;
      sprintf(strBuf, "  Contact[%d]", i);
      Serial.print(strBuf);
      sprintf(strBuf, ": ID=%d", frame->contacts[i].contactID);
      Serial.print(strBuf);
      sprintf(strBuf, ", Tip=%d", frame->contacts[i].tip);
      Serial.print(strBuf);
      sprintf(strBuf, ", Confidence=%d", frame->contacts[i].confidence);
      Serial.print(strBuf);
      sprintf(strBuf, ", X=%4d", frame->contacts[i].x);
      Serial.print(strBuf);
      sprintf(strBuf, ", Y=%4d", frame->contacts[i].y);
      Serial.print(strBuf);
      Serial.println();
    }
  }

  if (!printedAnyContact)
  {
    Serial.println(F("  No active contacts in frame"));
  }

  clearPtpPrintFrame(frame);
}

/** Collects the PTP reports that make up one frame (one time stamp) and prints the frame
    once all of its contacts have arrived. */
void printPtpReport(uint8_t i2c_channel, HID_report_t * report)
{
  if (i2c_channel >= NUM_TOUCHPADS)
  {
    return;
  }

  PtpPrintFrame_t *frame = &ptpPrintFrame_g[i2c_channel];
  if (!frame->pending)
  {
    clearPtpPrintFrame(frame);
    frame->pending = true;
    frame->timeStamp = report->ptp.timeStamp;
  }
  else if (report->ptp.timeStamp != frame->timeStamp)
  {
    printAndClearPtpFrame(i2c_channel, frame);
    frame->pending = true;
    frame->timeStamp = report->ptp.timeStamp;
  }

  frame->packetCount++;
  frame->buttons |= report->ptp.buttons;
  if (report->ptp.contactCount > frame->contactCount)
  {
    frame->contactCount = report->ptp.contactCount;
  }
  if (report->ptp.decodedContactSlots > frame->maxSlotsPerPacket)
  {
    frame->maxSlotsPerPacket = report->ptp.decodedContactSlots;
  }

  for (uint8_t i = 0; i < report->ptp.decodedContactSlots; i++)
  {
    PtpContact_t *contact = &report->ptp.contacts[i];
    bool hasContactData = (contact->confidence != 0) || (contact->tip != 0) || (contact->x != 0) || (contact->y != 0);
    if (!hasContactData)
    {
      continue;
    }

    int8_t index = -1;
    for (uint8_t slot = 0; slot < PTP_MAX_CONTACTS; slot++)
    {
      if (frame->contactReady[slot] && (frame->contacts[slot].contactID == contact->contactID))
      {
        index = (int8_t)slot;
        break;
      }
    }

    if (index < 0)
    {
      for (uint8_t slot = 0; slot < PTP_MAX_CONTACTS; slot++)
      {
        if (!frame->contactReady[slot])
        {
          index = (int8_t)slot;
          break;
        }
      }
    }

    if (index >= 0)
    {
      frame->contacts[index] = *contact;
      frame->contactReady[index] = true;
    }
  }

  uint8_t activeContacts = countActivePtpContacts(frame);
  bool frameComplete = false;

  if (frame->contactCount == 0)
  {
    frameComplete = true;
  }
  else if (activeContacts >= frame->contactCount)
  {
    frameComplete = true;
  }
  else if (frame->maxSlotsPerPacket > 0)
  {
    uint8_t expectedPackets = (frame->contactCount + frame->maxSlotsPerPacket - 1) / frame->maxSlotsPerPacket;
    if ((expectedPackets > 1) && (frame->packetCount >= expectedPackets))
    {
      frameComplete = true;
    }
  }

  if (frameComplete)
  {
    printAndClearPtpFrame(i2c_channel, frame);
  }
}

/**************************************************************/
/*************** FUNCTIONS FOR PRINTING EVENTS ****************/

/** Prints all the events that correspond to cur_report.
    Determines which printing function to use from the reportID */
void printEvent(uint8_t i2c_channel, HID_report_t* cur_report)
{
  switch( cur_report->reportID)
  {
    case MOUSE_REPORT_ID:
      printMouseReportEvents(i2c_channel, cur_report, &prevMouseReport_g[i2c_channel]);
      prevMouseReport_g[i2c_channel] = *cur_report;
      break;
    case PTP_REPORT_ID:
      printPtpReportEvents(i2c_channel, cur_report, &prevPtpReport_g[i2c_channel]);
      prevPtpReport_g[i2c_channel] = *cur_report;
      break;
    case KEY_REPORT_ID:
      printKeyboardReportEvents(i2c_channel, cur_report, &prevKeyboardReport_g[i2c_channel]);
      prevKeyboardReport_g[i2c_channel] = *cur_report;
      break;
    default:
      printTouchpadPrefix(i2c_channel);
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

void printKeyboardReportEvents(uint8_t i2c_channel, HID_report_t * cur_report, HID_report_t * prev_report)
{
    if(prev_report->reportID != KEY_REPORT_ID)
    {
        // No previous keyboard state to compare against
        return;
    }

    // Check for modifier changes
    if (cur_report->keyboard.modifier1 != prev_report->keyboard.modifier1 ||
        cur_report->keyboard.modifier2 != prev_report->keyboard.modifier2)
    {
        printTouchpadPrefix(i2c_channel);
        Serial.print(F("  Modifier change: "));
        printModifierNames(cur_report->keyboard.modifier1);
        Serial.print(F("(was "));
        printModifierNames(prev_report->keyboard.modifier1);
        Serial.println(F(")"));
    }

    // Check for new key presses
    for (uint8_t i = 0; i < 6; i++)
    {
        if (cur_report->keyboard.keycode[i] != 0)
        {
            bool foundInPrevious = false;
            for (uint8_t j = 0; j < 6; j++)
            {
                if (cur_report->keyboard.keycode[i] == prev_report->keyboard.keycode[j])
                {
                    foundInPrevious = true;
                    break;
                }
            }
            if (!foundInPrevious)
            {
                printTouchpadPrefix(i2c_channel);
                Serial.print(F("  Key pressed: "));
                printKeycodeName(cur_report->keyboard.keycode[i]);
                Serial.println();
            }
        }
    }

    // Check for key releases
    for (uint8_t i = 0; i < 6; i++)
    {
        if (prev_report->keyboard.keycode[i] != 0)
        {
            bool foundInCurrent = false;
            for (uint8_t j = 0; j < 6; j++)
            {
                if (prev_report->keyboard.keycode[i] == cur_report->keyboard.keycode[j])
                {
                    foundInCurrent = true;
                    break;
                }
            }
            if (!foundInCurrent)
            {
                printTouchpadPrefix(i2c_channel);
                Serial.print(F("  Key released: "));
                printKeycodeName(prev_report->keyboard.keycode[i]);
                Serial.println();
            }
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
            printTouchpadPrefix(i2c_channel);
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

/** Initializes the reports used for keeping track of one touchpad's state.
    Each report will represent the most recent past report of
    its type. */
void initialize_saved_reports(uint8_t i2c_channel)
{
  prevMouseReport_g[i2c_channel].reportID = MOUSE_REPORT_ID;
  prevMouseReport_g[i2c_channel].mouse.buttons = 0x0;

  prevPtpReport_g[i2c_channel].reportID = PTP_REPORT_ID;
  prevPtpReport_g[i2c_channel].ptp.buttons = 0x0;
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
  printTouchpadPrefix(i2c_channel);
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
