#ifndef HID_DESCRIPTOR_H
#define HID_DESCRIPTOR_H

// Copyright (c) 2025 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include <stdint.h>
#include "ResponseReader.h"

class HidDescriptor : public ResponseReader
{
public:
    const uint16_t &wHIDDescLength {m_wHIDDescLength};
    const uint16_t &BCD {m_BCD};
    const uint16_t &wReportDescLength {m_wReportDescLength};
    const uint16_t &wReportDescRegister {m_wReportDescRegister};
    const uint16_t &wInputRegister {m_wInputRegister};
    const uint16_t &wMaxInputLength {m_wMaxInputLength};
    const uint16_t &wOutputRegister {m_wOutputRegister};
    const uint16_t &wMaxOutputLength {m_wMaxOutputLength};
    const uint16_t &wCommandRegister {m_wCommandRegister};
    const uint16_t &wDataRegister {m_wDataRegister};
    const uint16_t &wVendorID {m_wVendorID};
    const uint16_t &wProductID {m_wProductID};
    const uint16_t &VersionID {m_VersionID};
    const uint32_t &RESERVED0 {RESERVED0};

    static const uint16_t packetLength = 30;
    void decodeFrom(uint8_t * packet);

protected:
    uint16_t m_wHIDDescLength;
    uint16_t m_BCD;
    uint16_t m_wReportDescLength;
    uint16_t m_wReportDescRegister;
    uint16_t m_wInputRegister;
    uint16_t m_wMaxInputLength;
    uint16_t m_wOutputRegister;
    uint16_t m_wMaxOutputLength;
    uint16_t m_wCommandRegister;
    uint16_t m_wDataRegister;
    uint16_t m_wVendorID;
    uint16_t m_wProductID;
    uint16_t m_VersionID;
    uint32_t m_RESERVED0;
};

#endif // HID_DESCRIPTOR_H
