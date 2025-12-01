// Copyright (c) 2020 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

This directory contains a sample program that demonstrates how to interface with Cirque's Olympus capacitive-touch SoC running the CustomMeas firmware. CustomMeas_Demo.ino is an Arduino project that acts as a bridge between Olympus and the PC. It communicates with a PC over USB-CDC (virtual COM port) and implements a simple, character-based command set to interact with Olympus.

Build requirements:

-Arduino IDE 1.8.9 or newer

-Teensyduino 1.49 or newer (this is an add-on for the Arduino IDE)

-Go to <install directory>\Arduino\hardware\teensy\avr\libraries\i2c_t3\i2c_t3.h and change #define I2C_TX_BUFFER_LENGTH and I2C_RX_BUFFER_LENGTH to at least 326. This is an unfortunate limitation of Arduino I2C libraries requiring users to change the files directly (a maintenance/documentation nightmare).

-In the Arduino IDE Tools menu, configure the following:
  -Board: "Teensy 3.2 / 3.1"
  -USB Type: "Serial"
  -CPU Speed: "96 MHz"
  
NOTE: the first command you should issue (in Tools/Serial Monitor) is the "i" command, which initializes all of the configs on Olympus. If your measurement results don't look right, this command likely needs to be invoked.
