// Copyright (c) 2025 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include "CirqueHid.h"

CirqueHid::CirqueHid(uint8_t i2cAddress, uint16_t maxBufferLength) : I2cHidApi(i2cAddress, maxBufferLength)
{

}

CirqueHid::~CirqueHid()
{

}

#define CIRQUE_DONTCARE_REGISTER 0x0000

void CirqueHid::getDeviceCapabilities(uint8_t &NumberContacts, PTP_ButtonImplementation &ButtonImplementation)
{
    uint8_t data[5];
    getFeatureReport(id_deviceCapabilities, CIRQUE_DATA_REGISTER, data, 5);  // CIRQUE_DONTCARE_REGISTER
    NumberContacts = data[3];
    ButtonImplementation = (PTP_ButtonImplementation)data[4];
}

void CirqueHid::getCertificationStatus(uint8_t * blob, uint16_t blobLength)
{
    getFeatureReport(id_certificationStatus, CIRQUE_DATA_REGISTER, blob, blobLength);
}

void CirqueHid::setInputMode(bool setAbsolute)
{
    uint16_t data = (setAbsolute) ? 0x0300 : 0x0000; 
    setFeatureReport(id_inputMode, CIRQUE_DATA_REGISTER, data);
}

void CirqueHid::setSelectiveReporting(bool enableContactReports, bool enableButtonReports)
{
    uint16_t data = (enableContactReports) ? 0x0100 : 0x0000;
    data |= (enableButtonReports) ? 0x0200 : 0x0000;
    setFeatureReport(id_selectReporting, CIRQUE_DATA_REGISTER, data);
}




