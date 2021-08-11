# Gen 6 Dev Kit

Copyright (c) 2021 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

(Initial Release)

### Overview

This directory contains a sample program that demonstrates the usage of Cirque's
Gen6 touchpad solutions with a Teensy 3.2 development board acting as the host.

This project is intended to run on the Cirque 02-000658-00 PCB, with a 
Teensy 3.2 on-board.

This project presents how to read and interpret data from the Gen6 touchpads. 
This is presented via Serial Port. You can also change various configuration and
print settings by sending commands via Serial Port. Send 'h' for a full list of
commands. 

NOTE: The Arduino IDE requires projects to be located in a folder with a name 
that matches that of the .ino file (in this case, Gen6DevKit).

### Key Command Options
There is a simple command menu available upon entering the 'h' character into 
the serial monitor. 
```
Available Commands (case sensitive)

c	-	Force Compensation
C	-	Factory Calibrate
f	-	Enable Feed (default)
F	-	Disable Feed
a	-	Set to Absolute Mode
r	-	Set to Relative Mode (default)
p	-	Persist Settings to Flash
s	-	Print System Info
t	-	Enable Tracking (default)
T	-	Disable Tracking
v	-	Enable Compensation (default)
V	-	Disable Compensation

h, H, ?	-	Print this Table
d	-	Turn on Data Printing (default)
D	-	Turn off Data Printing 
e	-	Turn on Event Printing (default)
E	-	Turn off Event Printing 
```

### Sample Output
Sample output from the serial monitor. 
```
System Information
Chip ID:	45
FW Version:	14
FW Subversion:	20
Vendor ID:	488
Product ID:	D001
Version ID:	4514

Report ID:	0x6
Buttons:	0b0
X Delta:	-2
Y Delta:	-1
Scroll Delta:	0
Pan Delta:	0

Report ID:	0x6
Buttons:	0b0
X Delta:	-8
Y Delta:	-7
Scroll Delta:	0
Pan Delta:	0

Report ID:	0x6
Buttons:	0b0
X Delta:	-17
Y Delta:	-11
Scroll Delta:	0
Pan Delta:	0

Report ID:	0x6
Buttons:	0b0
X Delta:	-17
Y Delta:	-5
Scroll Delta:	0
Pan Delta:	0
```
