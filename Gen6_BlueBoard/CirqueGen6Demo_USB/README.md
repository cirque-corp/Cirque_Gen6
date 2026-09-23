# CirqueGen6Demo_USB - USB Mouse and Keyboard Demo

USB HID mouse and keyboard example using the Cirque Gen6 trackpad module with Teensy 4.0 or 4.1.

## Features

- **USB Mouse Output**: Supports both relative (mouse mode) and absolute (PTP mode) coordinate conversion
- **USB Keyboard Output**: Full keyboard support with modifiers and up to 6 simultaneous keycodes
- **Serial Command Interface**: Command the trackpad via serial terminal
- **Real-time Status Output**: Display keyboard, mouse, and PTP touch events over serial

## USB Configuration Required

**IMPORTANT:** You must select the correct USB type in Arduino IDE:

```
Tools > USB Type > Serial + Keyboard + Mouse + Joystick
```

Or any option containing both "Keyboard" and "Mouse" interfaces.

If the wrong USB type is selected, you'll get a compilation error with a helpful message.

## Firmware Modes

### PTP Mode (Default)
- Reports absolute X/Y coordinates
- CirqueGen6Demo_USB converts these to relative mouse movement
- Full multi-touch support with finger tracking
- Best for touchpad-like behavior

### Mouse Mode
- Reports relative X/Y deltas
- Direct USB mouse movement
- Button support
- Similar to relative touchpad

### Keyboard Mode  
- When keyboard reports are detected from the device
- Full modifier and keycode support
- Up to 6 simultaneous keycodes

## Serial Commands

- **h, ?** - Print help
- **m** - PTP mode (absolute coordinates)
- **M** - Mouse mode (relative coordinates)
- **d/D** - Data printing toggle
- **r/R** - Contact reporting toggle
- **b/B** - Button reporting toggle
- **x/X** - Invert X axis toggle
- **y/Y** - Invert Y axis toggle
- **s/S** - Swap X/Y toggle
- **p/P** - Power off/on
- **w** - Warm boot (device reset)
- **i/I** - Cancel/Force sleep
- **$** - Power cycle device
- **g** - Get device capabilities
- **o** - Show system information

## USB Output Behavior

### Mouse Reports
- **Relative mode**: Moves cursor by delta X/Y
- **PTP mode**: Converts absolute coordinates to relative movement
- **Buttons**: Press/release translated to USB mouse clicks

### Keyboard Reports  
- **Modifiers**: Shift, Ctrl, Alt, GUI keys
- **Keycodes**: Up to 6 simultaneous keys
- **USB HID**: Full keyboard protocol support

## Hardware Setup

- Teensy 4.0 or 4.1 with I2C Cirque Gen6 trackpad module
- Requires proper I2C pullup resistors (typically 4.7kΩ)
- DR (Data Ready) pin connection for interrupt handling
- Power supply for trackpad module

## Notes

- The trackpad must be configured with USB Type selected BEFORE upload
- Serial monitor shows real-time status and diagnostic output
- Data printing can be toggled via 'd'/'D' commands to reduce serial traffic
- Multi-touch support is available in PTP mode
- Keyboard output sends immediately when keys are detected

## References

- Based on CirqueGen6Demo with USB HID support
- Uses Teensy HID Keyboard and Mouse libraries
- Cirque Gen6 PTP trackpad specifications
