// Copyright (c) 2025 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include "HidDescriptor.h"

void HidDescriptor::decodeFrom(uint8_t * packet)
{
    uint16_t index = 0;

    m_wHIDDescLength = next16bits(packet, index);
    m_BCD = next16bits(packet, index);
    m_wReportDescLength = next16bits(packet, index);
    m_wReportDescRegister = next16bits(packet, index);
    m_wInputRegister = next16bits(packet, index);
    m_wMaxInputLength = next16bits(packet, index);
    m_wOutputRegister = next16bits(packet, index);
    m_wMaxOutputLength = next16bits(packet, index);
    m_wCommandRegister = next16bits(packet, index);
    m_wDataRegister = next16bits(packet, index);
    m_wVendorID = next16bits(packet, index);
    m_wProductID = next16bits(packet, index);
    m_VersionID = next16bits(packet, index);
    m_RESERVED0 = next32bits(packet, index);
}

