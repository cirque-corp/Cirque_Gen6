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

// state data for the command loop (using key presses)
bool enableContactReports = true;
bool enableButtonReports = true;

void setup() {
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

  Serial.println(F("h - help"));
}

void loop() {
  // put your main code here, to run repeatedly:

  // service DR
  if (HostBus.drAsserted())
  {
    // DR is signaling a report is ready - read the report
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
      default:
        break;
    }
  }
}

void showHelp(void)
{
  Serial.println(F("Commands"));
  Serial.println(F("  m - issue PTP reports, M - issue Mouse reports"));
  Serial.println(F("  p - HID power off, P - HID power on"));
  Serial.println(F("  r - don't report contacts, R - report contacts"));
  Serial.println(F("  b - don't report buttons, B - report buttons"));
  Serial.println(F("  x - don't invert x-axis, X - invert x-axis"));
  Serial.println(F("  y - don't invert y-axis, Y - invert y-axis"));
  Serial.println(F("  s - unswap x-y, S - swap x-y"));
  Serial.println(F("  i - cancel 'force sleep', I - 'force sleep'"));
  Serial.println(F("  w - warm boot"));
  Serial.println(F("  g - get device capabilities"));
  Serial.println(F("  $ - physical power off, then on"));
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
      // organize data
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
