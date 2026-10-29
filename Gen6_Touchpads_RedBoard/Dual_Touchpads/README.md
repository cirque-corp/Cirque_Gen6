# Gen6 Touchpad Dev Kit

Copyright (c) 2026 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

(Initial Release)

### Overview

This directory contains a sample program that demonstrates the usage of Cirque's
Gen6 touchpad solutions with a Teensy 4.0 development board acting as the host.

This project is intended to run on the Cirque 02-000620-00 Rev A02 (or later) PCB, with a 
Teensy 4.0 on-board. The dev kit is also known as the red board or Circle Sensor Dev Kit.

This project presents how to read and interpret data from the Gen6 touchpads. 
This is presented via Serial Port (Use the Serial Monitor in the Arduino IDE).
You can also change various configuration and print settings by sending commands
via Serial Port. Send 'h' for a full list of commands. 

NOTE: The Arduino IDE requires projects to be located in a folder with a name 
that matches that of the .ino file (in this case, Dual_Touchpads).

### Key Command Options
There is a simple command menu available upon entering the 'h' character into 
the serial monitor. 
```
Available Commands (case sensitive):
(Replace '#' with I2C channel num [0,1])

f#	-	Enable Feed (default)
F#	-	Disable Feed
p#, a#	-	Set to PTP/Absolute Mode
r#, m#	-	Set to Mouse/Relative Mode (default)
s#	-	Print System Info
b#	-	Enable Adaptive Touch
B#	-	Disable Adaptive Touch

h, H, ?	-	Print this Table
d	-	Turn on Data Printing (default)
D	-	Turn off Data Printing 
e	-	Turn on Event Printing (default)
E	-	Turn off Event Printing 
z	-	Turn on decode failure diagnostics (default)
Z	-	Turn off decode failure diagnostics
```

### Sample Output
Sample output from the serial monitor. 
```
Channel 0 System Information
Hardware ID:	A
Firmware ID:	0
Vendor ID:	488
Product ID:	CC35
Version ID:	1

Channel 1 System Information
Hardware ID:	A
Firmware ID:	0
Vendor ID:	488
Product ID:	CC23
Version ID:	1

Setting PTP Mode...
Done

I2C_Chan 1 -> ReportID: 0x01, Time:  1885, Buttons: 0, Contact Count: 1
  Contact[0]: ID=0, Tip=1, Confidence=1, X= 920, Y=1094
I2C_Chan 1 -> ReportID: 0x01, Time:  1950, Buttons: 0, Contact Count: 1
  Contact[0]: ID=0, Tip=1, Confidence=1, X= 929, Y=1093
I2C_Chan 1 -> ReportID: 0x01, Time:  2015, Buttons: 0, Contact Count: 1
  Contact[0]: ID=0, Tip=1, Confidence=1, X= 943, Y=1086
I2C_Chan 1 -> ReportID: 0x01, Time:  2080, Buttons: 0, Contact Count: 1
  Contact[0]: ID=0, Tip=1, Confidence=1, X= 952, Y=1078
I2C_Chan 1 -> ReportID: 0x01, Time:  2145, Buttons: 0, Contact Count: 1
  Contact[0]: ID=0, Tip=1, Confidence=1, X= 953, Y=1079

Setting Relative Mode...
Done

I2C_Chan 0 -> Button 1  Pressed
I2C_Chan 0 -> ReportID: 0x06, Buttons: L__, X_Delta:    0, Y_Delta:    0, Scroll Delta:  0
I2C_Chan 0 -> Button 1  Released
I2C_Chan 0 -> ReportID: 0x06, Buttons: ___, X_Delta:    0, Y_Delta:    0, Scroll Delta:  0
I2C_Chan 0 -> ReportID: 0x06, Buttons: ___, X_Delta:    7, Y_Delta:    5, Scroll Delta:  0
I2C_Chan 0 -> ReportID: 0x06, Buttons: ___, X_Delta:   24, Y_Delta:   40, Scroll Delta:  0
I2C_Chan 0 -> ReportID: 0x06, Buttons: ___, X_Delta:   11, Y_Delta:   48, Scroll Delta:  0
```
