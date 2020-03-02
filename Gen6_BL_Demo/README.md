// Copyright (c) 2020 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

This directory contains a sample program that demonstrates how to interface with Cirque's Gen6 Bootloader with a simple serial terminal.

Build requirements:

-Arduino IDE 1.8.9 or newer

-Teensyduino 1.49 or newer (this is an add-on for the Arduino IDE)

-Go to \Arduino\hardware\teensy\avr\libraries\i2c_t3\i2c_t3.h and change #define I2C_TX_BUFFER_LENGTH and I2C_RX_BUFFER_LENGTH to at least 1026 (the bootloader only requires 539 bytes, but external projects that reference I2C.cpp use 1026). This is an unfortunate limitation of Arduino I2C libraries requiring users to change the files directly (a maintenance/documentation nightmare).

-In the Arduino IDE Tools menu, configure the following: -Board: "Teensy 3.2 / 3.1" -USB Type: "Serial" -CPU Speed: "96 MHz"

NOTE: use the Arduino serial monitor (found in the Tools menu) to issue character-based commands.
