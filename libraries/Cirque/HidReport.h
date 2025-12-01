#ifndef I2C_HID_H
#define I2C_HID_H

// Copyright (c) 2025 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include "HidStructs.h"

class HidReport
{
public:
    HidReport();
    ~HidReport();

    const uint16_t &length {m_length};
    const reportIds_t &reportId {m_report_id};
    AnyHIDReport_t report;

    bool decodeReport(uint8_t* packet);

    // bool isFingerValid(uint8_t finger_num);
    // bool isFingerTouching(uint8_t finger_num);
    // uint8_t numberFingers(void);
    // bool isButtonPressed(void);

protected:
    uint16_t m_length;
    reportIds_t m_report_id;

    void clear(void);
    void clearReport(void);

    bool decodeLengthAndId(uint8_t* packet);
    bool decodeMouseReport(uint8_t* packet);
    bool decodePTPReport(uint8_t* packet);
    bool decodeKeyboardReport(uint8_t* packet);
    // bool decodeAlpsAbsoluteReport(uint8_t* packet);
    // bool decodeCirqueAbsoluteReport(uint8_t* packet);
    // bool decodeStickReport(uint8_t* packet);

};

#endif // I2C_HID_H


