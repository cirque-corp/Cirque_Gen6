// Copyright (c) 2026 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#ifndef USB_PTP_INTERNAL_H
#define USB_PTP_INTERNAL_H

// Included after the Teensy core's usb_desc.h. Turns the core's "Serial" USB type into
// Serial + one HID interface for the touchpad. Only usb_ptp_dev.c and usb_ptp_desc.c use this;
// the core's own serial driver keeps using the unmodified Serial settings, which still match.

#if !defined(USB_SERIAL)
#error "Within the Arduino IDE, please select Tools > USB Type > 'Serial'. This sketch adds the touchpad interface to that USB type itself."
#endif

#if !defined(__IMXRT1062__)
#error "This sketch requires a Teensy 4.x."
#endif

// usb_ptp_dev.c is a modified copy of the Teensyduino 1.59 core's usb.c
#if TEENSYDUINO != 159
#warning "This sketch's USB layer was written against Teensyduino 1.59. Check usb_ptp_dev.c against your core's usb.c."
#endif

#undef NUM_ENDPOINTS
#define NUM_ENDPOINTS         5
#undef NUM_INTERFACE
#define NUM_INTERFACE         3
#undef EXPERIMENTAL_INTERFACE  // replaced by the touchpad interface

#define PTP_INTERFACE         2
#define PTP_ENDPOINT          5
#define PTP_ENDPOINT_SIZE     64
#define PTP_INTERVAL_12       1   // 1 ms at full speed
#define PTP_INTERVAL_480      4   // 2^(4-1) microframes = 1 ms at high speed
#define ENDPOINT5_CONFIG      (ENDPOINT_RECEIVE_UNUSED + ENDPOINT_TRANSMIT_INTERRUPT)

#define USB_PTP_DESC_BUFFER_SIZE  USB_PTP_MAX_REPORT_DESC

#ifdef __cplusplus
extern "C" {
#endif

extern uint8_t usb_descriptor_buffer[];

// Implemented in usb_ptp_desc.c
uint16_t usb_ptp_build_config_descriptor(uint8_t *buffer, int highSpeed, uint8_t descriptorType);
const uint8_t *usb_ptp_report_descriptor(uint16_t *length);
uint16_t usb_ptp_hid_descriptor(uint8_t *buffer);

// Implemented in usb_ptp_dev.c
void usb_ptp_configure(void);

#ifdef __cplusplus
}
#endif

#endif
