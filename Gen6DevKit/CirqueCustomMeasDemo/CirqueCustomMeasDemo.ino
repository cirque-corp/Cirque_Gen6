// Copyright (c) 2025 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

// use a better I2C library that allows larger buffer sizes
#include <i2c_device.h>
#include <i2c_driver.h>
#include <i2c_driver_wire.h>
#include <i2c_register_slave.h>

// use the cirque demo code library
#include <CustomMeas.h>
#include <Cirque.h>  // if the library is installed from Library Manager you might not need this
#include <Teensy4_HostBusLayer.h>

// demo measurements are in Measurements.h/cpp
#include "Measurements.h"

// Create a specific Host Bus object (Teensy4_HostBusLayer works on the blue board). 
// The global variable HostBus will become it. All the common code then uses HostBus.
Teensy4_HostBusLayer teensyHostBus;
CustomMeas customMeas(645);
HidDescriptor hidDescriptor;
SystemInfo systemInfo;
HidReport hidReport;
//CustomMeas::GlobalInfo_t globalInfo;

void processKeys(void);
void showHelp(void);
void waitForHidResetResponse(void);
void identifyDevice(void);
void plotMeasurements(int16_t * measurements, uint16_t count);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  HostBusLayer::initError err = HostBus.init(400000, 550); // 400,000 Hz, 550 byte buffer
  if (err > HostBusLayer::initError::initOkay)
  {
    Serial.print(F("Failed to init host bus: "));
    Serial.print((int)err);
  }

  HostBus.setPower(true);
  delay(15); // wait for voltage to come up
  waitForHidResetResponse();
  identifyDevice();

  Serial.println(F("h - help"));
}

int16_t measurements[MAX_NUMBER_MEASUREMENTS * MAX_READINGS_PER_MEASUREMENT];

void loop() {
  // put your main code here, to run repeatedly:

  // service DR
  if (HostBus.drAsserted())
  {
    // DR is signalling a report is ready - read the report
    uint16_t measCount;
    if (customMeas.getMeasReport(measurements, measCount) == id_customMeas)
    {
      // once measurements are running, you can open the Serial Plotter window.
      plotMeasurements(measurements, measCount);
    }
    else
    {
      // report ID isn't measurements, get the report and display it
      // generally this pathway doesn't happen
      // customMeas.getReport(hidReport);
      // printHidReport(hidReport);
    }
  }

  // Various characters sent to this app will trigger commands that change the operation of the device
  processKeys();
}

void processKeys(void)
{
  if(Serial.available())
  {
    char rxChar = Serial.read();
    switch(rxChar)
    {
      case 'h':
        showHelp();
        break;
      case 'w' :
        Serial.println(F("Write Measurement Configuration\n  Measurements off"));

        customMeas.StopMeas();
        for (int x = 0; x < GROUPINFO_LENGTH; x++)
        {
          customMeas.WriteGroupInfo(0, &GroupInfoArray[x]);
        }
        for (int x = 0; x < MEASINFO_LENGTH; x++)
        {
          customMeas.WriteMeasInfo(0, &MeasInfoArray[x]);
        }

        Serial.println(F("  Groups and Measurements written"));
        break;
      case 'm':
        Serial.println(F("Measurements Off"));
        customMeas.StopMeas();
        break;
      case 'M' :
        Serial.println(F("Measurements On"));
        customMeas.StartMeas();
        break;
      case 'c' :
        Serial.println(F("Calibrate Group 0"));
        customMeas.Calibrate(0);  // hardcoded 0, as an example
        break;
      case 'C' :
        Serial.println(F("Calibrate All Groups"));
        customMeas.CalibrateAll();
        break;
      case 'e' :
        Serial.println(F("Enable Calibration Group 0"));
        customMeas.EnableCalibration(0);
        break;
      case 'E' :
        Serial.println(F("Enable Calibration All Groups "));
        customMeas.EnableAllCalibration();
        break;
      case 'd' :
        Serial.println(F("Disable Calibration Group 0"));
        customMeas.DisableCalibration(0);
        break;
      case 'D' :
        Serial.println(F("Disable Calibration All Groups"));
        customMeas.DisableAllCalibration();
        break;
      case 's' :
        Serial.println(F("Save configuration to flash"));
        // this saves everything (measurements and all config settings) except the Enable bit
        customMeas.Persist();
        break;
      case 'S' :
        Serial.println(F("Restore configuration from flash"));
        // this loads everything. The Enable bit is then set to be POR_Enable
        customMeas.Restore();
        break;
      default:
      break;
      // todo:
      // ReadNoiseConfig()
      // WriteNoiseConfig()

    }
  }
}

void showHelp(void)
{
  Serial.println(F("Commands"));
  Serial.println(F("w - write measurement configuration"));
  Serial.println(F("m - stop measurements, M - start measurements"));
  Serial.println(F("c - calibrate group 0, C - calibrate all groups"));
  Serial.println(F("e - enable calibration group 0, E - enable calibration all groups"));
  Serial.println(F("d - disable calibration group 0, D - disable calibration all groups"));
  Serial.println(F("s - save configuration to flash, S - restore configuration from flash"));
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
  customMeas.getReport(hidReport);
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
  customMeas.getHidDescriptor(hidDescriptor);
  Serial.printf(F("VID : 0x%04x  PID : 0x%04x  Version : %d\n"), 
    hidDescriptor.wVendorID, hidDescriptor.wProductID, hidDescriptor.VersionID);
  Serial.printf(F("Report Descriptor Reg : 0x%04x  Report Descriptor Length : 0x%04x\n"),
    hidDescriptor.wReportDescRegister, hidDescriptor.wReportDescLength);
  Serial.printf(F("Input Reg : 0x%04x  Max Input Length : 0x%04x\n"), 
    hidDescriptor.wInputRegister, hidDescriptor.wMaxInputLength);
  Serial.printf(F("Output Reg : 0x%04x  Max Output Length : 0x%04x\n"), 
    hidDescriptor.wOutputRegister, hidDescriptor.wMaxOutputLength);
  Serial.printf(F("Command Reg : 0x%04x  Data Reg : 0x%04x\n"), 
    hidDescriptor.wCommandRegister, hidDescriptor.wDataRegister);

  // hidDescriptor.BCD
}

void plotMeasurements(int16_t * measurements, uint16_t count)
{
  // write the measurements as a row of text. The Plotter feature of the IDE can be used to graph them.
  for (int x = 0; x < count; x++)
  {
    Serial.print(measurements[x]);
    Serial.print(F(" "));
  }
  Serial.println();
}

