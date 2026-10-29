# Cirque Gen6 Demo

Copyright (c) 2025 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

## Overview

This Arduino project uses the Cirque library and shows how to operate a touchpad.

This code requires the Teensy 4.0 have its USB set to "Serial".

Using the terminal window of the Arduino IDE you can send commands that control the touchpad.

* Commands (case sensitive)
*   m, M - issue Mouse/Relative reports
*   a, A, p - issue PTP/Absolute reports
*   k - HID power off, K - HID power on
*   r - don't report contacts, R - report contacts
*   b - don't report buttons, B - report buttons
*   f - enable feed, F - disable feed
*   x - don't invert x-axis, X - invert x-axis
*   y - don't invert y-axis, Y - invert y-axis
*   s - unswap x-y, S - swap x-y
*   i - cancel force-sleep, I - force sleep
*   w - warm boot
*   g - get device capabilities
*   o - show system information
*   d - data printing off, D - data printing on
*   z - toggle raw packet hex dump
*   h, ? - print this help
*   $ - physical power off, then on
