// Copyright (c) 2026 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#ifndef USB_PTP_H
#define USB_PTP_H

// Interface between the sketch and the USB device layer (usb_ptp_dev.c / usb_ptp_desc.c).
// The USB device is a composite of USB Serial (for the Serial Monitor) and one HID interface
// per touchpad. Each HID interface presents that touchpad's own HID report descriptor to the
// host, so the PC sees each touchpad as a Windows Precision Touchpad (PTP) instead of a mouse.
//
// Touchpads are identified by their I2C channel: 0 for Touchpad #1, 1 for Touchpad #2 (the
// numbers printed on the board). The HID interfaces are numbered in channel order from the
// touchpads that have a report descriptor, so if only one touchpad is present it always gets
// the first HID interface.

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// USB identity. 0x16C0/0x0487 is the Teensy "Serial + Keyboard + Mouse + Joystick" ID, which keeps
// automatic uploading from the Arduino IDE working. Change these to give the bridge its own identity
// (you will then have to press the Teensy's program button to upload).
#ifndef USB_PTP_VENDOR_ID
#define USB_PTP_VENDOR_ID   0x16C0
#endif
#ifndef USB_PTP_PRODUCT_ID
#define USB_PTP_PRODUCT_ID  0x0487
#endif

#define USB_PTP_MAX_TOUCHPADS       2     // one HID interface per I2C channel
#define USB_PTP_MAX_REPORT_DESC     2048  // largest touchpad report descriptor that can be forwarded
#define USB_PTP_MAX_CONTROL_REPORT  1024  // largest GET_REPORT/SET_REPORT (the PTP certification blob is 257 bytes;
                                          // Gen6 firmware-update feature reports are about 530 bytes)
#define USB_PTP_MAX_INPUT_REPORT    64    // largest input report forwarded on an interrupt endpoint

// Report descriptor to present for one touchpad (I2C channel 0 or 1). Must be called before
// usb_ptp_attach(). A length of 0 (or never calling this for a channel) leaves that touchpad's
// HID interface out. With no report descriptors at all, only USB Serial enumerates.
void usb_ptp_set_report_descriptor(uint8_t touchpad, const uint8_t *descriptor, uint16_t length);

// USB interface number the PC sees for a touchpad, or -1 if that touchpad has no HID interface.
int usb_ptp_interface_number(uint8_t touchpad);

// Connects to the USB host. Until this is called the Teensy is not visible on USB.
void usb_ptp_attach(void);

// True once the host has configured the device.
bool usb_ptp_configured(void);

// Sends one input report (report ID first) from a touchpad to the host.
// Returns 0 on success, -1 if dropped.
int usb_ptp_send_input_report(uint8_t touchpad, const uint8_t *report, uint16_t length);

// A GET_REPORT or SET_REPORT request from the host, addressed to one touchpad's interface.
// These are answered from loop() because answering them needs (slow) I2C transfers.
// The host sends control requests one at a time, so there is never more than one waiting.
typedef struct
{
  uint32_t sequence;   // identifies the request; a newer request from the host cancels this one
  uint8_t  touchpad;   // I2C channel of the touchpad the request is for
  bool     isSet;      // true = SET_REPORT (data[] holds the report), false = GET_REPORT
  uint8_t  reportType; // 1 = input, 2 = output, 3 = feature
  uint8_t  reportID;
  uint16_t length;     // GET: most bytes the host will accept, SET: bytes in data[]
} usb_ptp_request_t;

// Copies the pending request (and, for SET_REPORT, its data) and returns true if one is waiting.
bool usb_ptp_get_request(usb_ptp_request_t *request, uint8_t *data);

// Finishes a request. For GET_REPORT, pass the report bytes (report ID first).
// For SET_REPORT, pass NULL/0. If the request can't be served, pass ok = false to stall it.
void usb_ptp_complete_request(const usb_ptp_request_t *request, bool ok, const uint8_t *data, uint16_t length);

#ifdef __cplusplus
}
#endif

#endif
