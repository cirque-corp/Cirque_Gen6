// USB HID Keyboard Keycode Definitions
// Standard USB keyboard keycode to ASCII/name mapping

#ifndef USB_KEYCODES_H
#define USB_KEYCODES_H

#include <stdint.h>

// USB HID Keyboard Keycodes (page 53 of HID spec 1.11)
#define USB_KEY_NONE              0x00
#define USB_KEY_ERROR_ROLL_OVER   0x01
#define USB_KEY_POST_FAIL         0x02
#define USB_KEY_ERROR_UNDEFINED   0x03
#define USB_KEY_A                 0x04
#define USB_KEY_B                 0x05
#define USB_KEY_C                 0x06
#define USB_KEY_D                 0x07
#define USB_KEY_E                 0x08
#define USB_KEY_F                 0x09
#define USB_KEY_G                 0x0A
#define USB_KEY_H                 0x0B
#define USB_KEY_I                 0x0C
#define USB_KEY_J                 0x0D
#define USB_KEY_K                 0x0E
#define USB_KEY_L                 0x0F
#define USB_KEY_M                 0x10
#define USB_KEY_N                 0x11
#define USB_KEY_O                 0x12
#define USB_KEY_P                 0x13
#define USB_KEY_Q                 0x14
#define USB_KEY_R                 0x15
#define USB_KEY_S                 0x16
#define USB_KEY_T                 0x17
#define USB_KEY_U                 0x18
#define USB_KEY_V                 0x19
#define USB_KEY_W                 0x1A
#define USB_KEY_X                 0x1B
#define USB_KEY_Y                 0x1C
#define USB_KEY_Z                 0x1D
#define USB_KEY_1                 0x1E
#define USB_KEY_2                 0x1F
#define USB_KEY_3                 0x20
#define USB_KEY_4                 0x21
#define USB_KEY_5                 0x22
#define USB_KEY_6                 0x23
#define USB_KEY_7                 0x24
#define USB_KEY_8                 0x25
#define USB_KEY_9                 0x26
#define USB_KEY_0                 0x27
#define USB_KEY_ENTER             0x28
#define USB_KEY_ESCAPE            0x29
#define USB_KEY_BACKSPACE         0x2A
#define USB_KEY_TAB               0x2B
#define USB_KEY_SPACE             0x2C
#define USB_KEY_MINUS             0x2D
#define USB_KEY_EQUALS            0x2E
#define USB_KEY_LEFT_BRACKET      0x2F
#define USB_KEY_RIGHT_BRACKET     0x30
#define USB_KEY_BACKSLASH         0x31
#define USB_KEY_HASH              0x32  // Non-US # and ~
#define USB_KEY_SEMICOLON         0x33
#define USB_KEY_APOSTROPHE        0x34
#define USB_KEY_BACKTICK          0x35
#define USB_KEY_COMMA             0x36
#define USB_KEY_PERIOD            0x37
#define USB_KEY_SLASH             0x38
#define USB_KEY_CAPS_LOCK         0x39
#define USB_KEY_F1                0x3A
#define USB_KEY_F2                0x3B
#define USB_KEY_F3                0x3C
#define USB_KEY_F4                0x3D
#define USB_KEY_F5                0x3E
#define USB_KEY_F6                0x3F
#define USB_KEY_F7                0x40
#define USB_KEY_F8                0x41
#define USB_KEY_F9                0x42
#define USB_KEY_F10               0x43
#define USB_KEY_F11               0x44
#define USB_KEY_F12               0x45
#define USB_KEY_PRINT_SCREEN      0x46
#define USB_KEY_SCROLL_LOCK       0x47
#define USB_KEY_PAUSE             0x48
#define USB_KEY_INSERT            0x49
#define USB_KEY_HOME              0x4A
#define USB_KEY_PAGE_UP           0x4B
#define USB_KEY_DELETE            0x4C
#define USB_KEY_END               0x4D
#define USB_KEY_PAGE_DOWN         0x4E
#define USB_KEY_RIGHT_ARROW       0x4F
#define USB_KEY_LEFT_ARROW        0x50
#define USB_KEY_DOWN_ARROW        0x51
#define USB_KEY_UP_ARROW          0x52
#define USB_KEY_NUM_LOCK          0x53
#define USB_KEY_KEYPAD_DIVIDE     0x54
#define USB_KEY_KEYPAD_MULTIPLY   0x55
#define USB_KEY_KEYPAD_MINUS      0x56
#define USB_KEY_KEYPAD_PLUS       0x57
#define USB_KEY_KEYPAD_ENTER      0x58
#define USB_KEY_KEYPAD_1          0x59
#define USB_KEY_KEYPAD_2          0x5A
#define USB_KEY_KEYPAD_3          0x5B
#define USB_KEY_KEYPAD_4          0x5C
#define USB_KEY_KEYPAD_5          0x5D
#define USB_KEY_KEYPAD_6          0x5E
#define USB_KEY_KEYPAD_7          0x5F
#define USB_KEY_KEYPAD_8          0x60
#define USB_KEY_KEYPAD_9          0x61
#define USB_KEY_KEYPAD_0          0x62
#define USB_KEY_KEYPAD_PERIOD     0x63
#define USB_KEY_NON_US_BACKSLASH  0x64
#define USB_KEY_APPLICATION       0x65
#define USB_KEY_POWER             0x66
#define USB_KEY_KEYPAD_EQUALS     0x67

// Array of keycode names for lookup
static const char* USB_KEYCODE_NAMES[] PROGMEM = {
  "NONE",              // 0x00
  "ERROR_ROLL_OVER",   // 0x01
  "POST_FAIL",         // 0x02
  "ERROR_UNDEFINED",   // 0x03
  "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",  // 0x04-0x10
  "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",  // 0x11-0x1D
  "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",  // 0x1E-0x27
  "ENTER", "ESCAPE", "BACKSPACE", "TAB", "SPACE",  // 0x28-0x2C
  "MINUS", "EQUALS", "LEFT_BRACKET", "RIGHT_BRACKET", "BACKSLASH",  // 0x2D-0x31
  "HASH", "SEMICOLON", "APOSTROPHE", "BACKTICK",  // 0x32-0x35
  "COMMA", "PERIOD", "SLASH", "CAPS_LOCK",  // 0x36-0x39
  "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",  // 0x3A-0x45
  "PRINT_SCREEN", "SCROLL_LOCK", "PAUSE", "INSERT", "HOME", "PAGE_UP",  // 0x46-0x4B
  "DELETE", "END", "PAGE_DOWN",  // 0x4C-0x4E
  "RIGHT", "LEFT", "DOWN", "UP",  // 0x4F-0x52
  "NUM_LOCK", "KP_DIVIDE", "KP_MULTIPLY", "KP_MINUS", "KP_PLUS", "KP_ENTER",  // 0x53-0x58
  "KP_1", "KP_2", "KP_3", "KP_4", "KP_5", "KP_6", "KP_7", "KP_8", "KP_9", "KP_0",  // 0x59-0x62
  "KP_PERIOD", "NON_US_BACKSLASH", "APPLICATION", "POWER", "KP_EQUALS"  // 0x63-0x67
};

/** Get the name of a USB HID keycode */
inline const char* getKeycodeName(uint8_t keycode)
{
  if (keycode < 0x68) {
    return (const char*)pgm_read_ptr(&USB_KEYCODE_NAMES[keycode]);
  }
  return "UNKNOWN";
}

#endif // USB_KEYCODES_H
