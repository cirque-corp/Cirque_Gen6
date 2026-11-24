// Copyright (c) 2026 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license
//
// USB descriptors for the PTP bridge. Replaces the Teensy core's usb_desc.c (the core's copy is
// not linked because this file defines the same symbols). Parts are adapted from the Teensyduino
// core, Copyright (c) 2017 PJRC.COM, LLC, used under its MIT-style license.

#include "usb_dev.h"
#define USB_DESC_LIST_DEFINE
#include "usb_desc.h"
#include "usb_names.h"
#include "imxrt.h"
#include "avr_functions.h"
#include "avr/pgmspace.h"
#include <string.h>
#include "usb_ptp.h"
#include "usb_ptp_internal.h"

#define LSB(n) ((n) & 255)
#define MSB(n) (((n) >> 8) & 255)

// String descriptor index of the first touchpad interface's name ("Touchpad #1"); the touchpad
// on I2C channel 1 uses the next index ("Touchpad #2"). Indexes 1-3 are manufacturer, product
// and serial number.
#define PTP_INTERFACE_STRING  4

static const uint8_t device_descriptor[] = {
        18,                                     // bLength
        1,                                      // bDescriptorType
        0x00, 0x02,                             // bcdUSB
        0xEF,                                   // bDeviceClass (uses Interface Association Descriptors)
        0x02,                                   // bDeviceSubClass
        0x01,                                   // bDeviceProtocol
        EP0_SIZE,                               // bMaxPacketSize0
        LSB(USB_PTP_VENDOR_ID), MSB(USB_PTP_VENDOR_ID),
        LSB(USB_PTP_PRODUCT_ID), MSB(USB_PTP_PRODUCT_ID),
  // Same bcdDevice values as the core; teensy_ports uses them to identify the Teensy model
  #if defined(ARDUINO_TEENSY40)
        0x79, 0x02,
  #elif defined(ARDUINO_TEENSY41)
        0x80, 0x02,
  #elif defined(ARDUINO_TEENSY_MICROMOD)
        0x81, 0x02,
  #else
        0x00, 0x02,
  #endif
        1,                                      // iManufacturer
        2,                                      // iProduct
        3,                                      // iSerialNumber
        1                                       // bNumConfigurations
};

static const uint8_t qualifier_descriptor[] = {
        10,                                     // bLength
        6,                                      // bDescriptorType
        0x00, 0x02,                             // bcdUSB
        0xEF, 0x02, 0x01,                       // class, subclass, protocol
        EP0_SIZE,                               // bMaxPacketSize0
        1,                                      // bNumConfigurations
        0                                       // bReserved
};

static uint8_t report_descriptor[USB_PTP_MAX_TOUCHPADS][USB_PTP_MAX_REPORT_DESC];
static uint16_t report_descriptor_length[USB_PTP_MAX_TOUCHPADS] = {0};

void usb_ptp_set_report_descriptor(uint8_t touchpad, const uint8_t *descriptor, uint16_t length)
{
        if (touchpad >= USB_PTP_MAX_TOUCHPADS) return;
        if (length > USB_PTP_MAX_REPORT_DESC) length = 0;
        if (length) memcpy(report_descriptor[touchpad], descriptor, length);
        report_descriptor_length[touchpad] = length;
}

const uint8_t *usb_ptp_report_descriptor(uint8_t touchpad, uint16_t *length)
{
        if (touchpad >= USB_PTP_MAX_TOUCHPADS) {
                *length = 0;
                return NULL;
        }
        *length = report_descriptor_length[touchpad];
        return report_descriptor[touchpad];
}

// Interfaces are numbered in channel order from the touchpads that are present, so the
// interface numbers are always contiguous.
int usb_ptp_interface_number(uint8_t touchpad)
{
        if (touchpad >= USB_PTP_MAX_TOUCHPADS || !report_descriptor_length[touchpad]) return -1;
        int interface = PTP_FIRST_INTERFACE;
        for (uint8_t i = 0; i < touchpad; i++) {
                if (report_descriptor_length[i]) interface++;
        }
        return interface;
}

int usb_ptp_touchpad_for_interface(uint16_t interface)
{
        for (uint8_t i = 0; i < USB_PTP_MAX_TOUCHPADS; i++) {
                if (usb_ptp_interface_number(i) == (int)interface) return i;
        }
        return -1;
}

uint16_t usb_ptp_hid_descriptor(uint8_t touchpad, uint8_t *buffer)
{
        uint16_t length;
        usb_ptp_report_descriptor(touchpad, &length);
        if (!length) return 0;
        buffer[0] = 9;                          // bLength
        buffer[1] = 0x21;                       // bDescriptorType (HID)
        buffer[2] = 0x11;                       // bcdHID 1.11
        buffer[3] = 0x01;
        buffer[4] = 0;                          // bCountryCode
        buffer[5] = 1;                          // bNumDescriptors
        buffer[6] = 0x22;                       // bDescriptorType (Report)
        buffer[7] = LSB(length);
        buffer[8] = MSB(length);
        return 9;
}

static uint8_t *put(uint8_t *p, const uint8_t *bytes, uint32_t count)
{
        memcpy(p, bytes, count);
        return p + count;
}

// Builds the configuration descriptor (descriptorType 2) or other-speed configuration
// descriptor (descriptorType 7) for the given bus speed. Returns its length.
uint16_t usb_ptp_build_config_descriptor(uint8_t *buffer, int highSpeed, uint8_t descriptorType)
{
        const uint16_t cdcDataSize = highSpeed ? CDC_RX_SIZE_480 : CDC_RX_SIZE_12;
        uint8_t *p = buffer;
        uint8_t hidCount = 0;
        for (uint8_t i = 0; i < USB_PTP_MAX_TOUCHPADS; i++) {
                if (report_descriptor_length[i]) hidCount++;
        }

        const uint8_t config[] = {
                9, descriptorType,
                0, 0,                           // wTotalLength, filled in below
                (uint8_t)(2 + hidCount),        // bNumInterfaces
                1,                              // bConfigurationValue
                0,                              // iConfiguration
                0xC0,                           // bmAttributes
                50,                             // bMaxPower (100 mA)
        };
        p = put(p, config, sizeof(config));

        const uint8_t cdc[] = {
                // Interface Association Descriptor
                8, 11, CDC_STATUS_INTERFACE, 2, 0x02, 0x02, 0x01, 0,
                // CDC communication interface
                9, 4, CDC_STATUS_INTERFACE, 0, 1, 0x02, 0x02, 0x01, 0,
                5, 0x24, 0x00, 0x10, 0x01,      // Header Functional Descriptor
                5, 0x24, 0x01, 0x01, 1,         // Call Management Functional Descriptor
                4, 0x24, 0x02, 0x06,            // Abstract Control Management Functional Descriptor
                5, 0x24, 0x06, CDC_STATUS_INTERFACE, CDC_DATA_INTERFACE, // Union Functional Descriptor
                7, 5, CDC_ACM_ENDPOINT | 0x80, 0x03, LSB(CDC_ACM_SIZE), MSB(CDC_ACM_SIZE),
                        (uint8_t)(highSpeed ? 5 : 16),
                // CDC data interface
                9, 4, CDC_DATA_INTERFACE, 0, 2, 0x0A, 0x00, 0x00, 0,
                7, 5, CDC_RX_ENDPOINT, 0x02, LSB(cdcDataSize), MSB(cdcDataSize), 0,
                7, 5, CDC_TX_ENDPOINT | 0x80, 0x02, LSB(cdcDataSize), MSB(cdcDataSize), 0,
        };
        p = put(p, cdc, sizeof(cdc));

        // One HID interface per touchpad present, each with its own interrupt IN endpoint
        for (uint8_t i = 0; i < USB_PTP_MAX_TOUCHPADS; i++) {
                const int interface = usb_ptp_interface_number(i);
                if (interface < 0) continue;
                const uint8_t endpoint = PTP_FIRST_ENDPOINT + (interface - PTP_FIRST_INTERFACE);
                const uint8_t hidInterface[] = {
                        9, 4, (uint8_t)interface, 0, 1, 0x03, 0x00, 0x00,
                        (uint8_t)(PTP_INTERFACE_STRING + i),    // iInterface: "Touchpad #1" or "Touchpad #2"
                };
                p = put(p, hidInterface, sizeof(hidInterface));
                p += usb_ptp_hid_descriptor(i, p);
                const uint8_t hidEndpoint[] = {
                        7, 5, (uint8_t)(endpoint | 0x80), 0x03, LSB(PTP_ENDPOINT_SIZE), MSB(PTP_ENDPOINT_SIZE),
                        (uint8_t)(highSpeed ? PTP_INTERVAL_480 : PTP_INTERVAL_12),
                };
                p = put(p, hidEndpoint, sizeof(hidEndpoint));
        }

        uint16_t total = p - buffer;
        buffer[2] = LSB(total);
        buffer[3] = MSB(total);
        return total;
}

__attribute__ ((section(".dmabuffers"), aligned(32)))
uint8_t usb_descriptor_buffer[USB_PTP_DESC_BUFFER_SIZE];

static const struct usb_string_descriptor_struct string0 = {
        4,
        3,
        {0x0409}
};

static const struct usb_string_descriptor_struct string_manufacturer = {
        2 + 6 * 2,
        3,
        {'C','i','r','q','u','e'}
};

static const struct usb_string_descriptor_struct string_product = {
        2 + 21 * 2,
        3,
        {'G','e','n','6',' ','P','T','P',' ','T','o','u','c','h','p','a','d',' ','D','e','v'}
};

// Interface names, matching the "Touchpad #1" and "Touchpad #2" labels on the board
// (I2C channel 0 and 1). Some hosts show them, for example in Windows Device Manager.
static const struct usb_string_descriptor_struct string_touchpad1 = {
        2 + 11 * 2,
        3,
        {'T','o','u','c','h','p','a','d',' ','#','1'}
};

static const struct usb_string_descriptor_struct string_touchpad2 = {
        2 + 11 * 2,
        3,
        {'T','o','u','c','h','p','a','d',' ','#','2'}
};

static struct usb_string_descriptor_struct string_serial_number = {
        12,
        3,
        {0,0,0,0,0,0,0,0,0,0}
};

void usb_init_serialnumber(void)
{
        char buf[11];
        uint32_t i, num;

        num = HW_OCOTP_MAC0 & 0xFFFFFF;
        // add extra zero to work around OS-X CDC-ACM driver bug
        if (num < 10000000) num = num * 10;
        ultoa(num, buf, 10);
        for (i=0; i<10; i++) {
                char c = buf[i];
                if (!c) break;
                string_serial_number.wString[i] = c;
        }
        string_serial_number.bLength = i * 2 + 2;
}

// Configuration and HID descriptors are not listed here: usb_ptp_dev.c builds them on request.
const usb_descriptor_list_t usb_descriptor_list[] = {
        //wValue, wIndex, address,          length
        {0x0100, 0x0000, device_descriptor, sizeof(device_descriptor)},
        {0x0600, 0x0000, qualifier_descriptor, sizeof(qualifier_descriptor)},
        {0x0300, 0x0000, (const uint8_t *)&string0, 0},
        {0x0301, 0x0409, (const uint8_t *)&string_manufacturer, 0},
        {0x0302, 0x0409, (const uint8_t *)&string_product, 0},
        {0x0303, 0x0409, (const uint8_t *)&string_serial_number, 0},
        {0x0300 + PTP_INTERFACE_STRING,     0x0409, (const uint8_t *)&string_touchpad1, 0},
        {0x0300 + PTP_INTERFACE_STRING + 1, 0x0409, (const uint8_t *)&string_touchpad2, 0},
        {0, 0, NULL, 0}
};
