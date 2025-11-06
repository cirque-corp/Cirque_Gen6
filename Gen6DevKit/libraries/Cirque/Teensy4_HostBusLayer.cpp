#include "Teensy4_HostBusLayer.h"
#include "Arduino.h"
// #include "Wire.h"  // hard to change buffer length
#include "i2c_driver_wire.h" // Modified version. Passes the tests
#define BUFFER_LENGTH Wire.rx_buffer_length

extern "C"
{

}

Teensy4_HostBusLayer::Teensy4_HostBusLayer(void)
{

}

Teensy4_HostBusLayer::~Teensy4_HostBusLayer(void)
{

}

HostBusLayer::initError Teensy4_HostBusLayer::init(uint32_t i2cClockFreq_Hz, uint16_t i2cMinBufferLength)
{
    m_i2cClockFreq = i2cClockFreq_Hz;
    m_i2cMinBufferLength = i2cMinBufferLength;

    // init gpio
    pinMode(PWR_EN_IO1, OUTPUT);
    digitalWrite(PWR_EN_IO1, LOW);
    pinMode(OC_FLAG_IO0, INPUT);

    pinMode(TP_DISABLE, OUTPUT);
    pinMode(FW_SECURITY, OUTPUT);
    pinMode(NFC_STATUS_TP4_ACT, OUTPUT);
    pinMode(LID_CLOSE, OUTPUT);

    pinMode(DR_IO9, INPUT);

    digitalWrite(TP_DISABLE, HIGH);
    digitalWrite(FW_SECURITY, HIGH);
    digitalWrite(NFC_STATUS_TP4_ACT, HIGH);
    digitalWrite(LID_CLOSE, HIGH);
    digitalWrite(DR_IO9, HIGH);

    // init I2C
    Wire.setClock(i2cClockFreq);  // stock library: must call .setClock after .begin
    Wire.begin();                 // Set the arduino as host

    if ((i2cClockFreq_Hz > 1200000) || (i2cClockFreq_Hz < 10000)) return initClockFreqError;
    if (i2cMinBufferLength > BUFFER_LENGTH) return initBufferSizeError; //BUFFER_LENGTH) return initBufferSizeError;
    return initOkay;
}

void Teensy4_HostBusLayer::setPower(bool on)
{
    if (on)
    {
        if (digitalReadFast(PWR_EN_IO1) == LOW)
        {
            timer = 0;
        }
        digitalWrite(PWR_EN_IO1, HIGH);        
    }
    else
    {
        digitalWrite(PWR_EN_IO1, LOW);
        pinMode(SDA_IO18, OUTPUT);  // pull SDA and SCL low to help discharge the power rail
        digitalWriteFast(SDA_IO18, LOW);
        pinMode(SCL_IO19, OUTPUT);
        digitalWriteFast(SCL_IO19, LOW);
        delayMicroseconds(2000);
        pinMode(SDA_IO18, INPUT);  // pull SDA and SCL low to help discharge the power rail
        digitalWriteFast(SDA_IO18, HIGH);
        pinMode(SCL_IO19, INPUT);
        digitalWriteFast(SCL_IO19, HIGH);

        if (digitalReadFast(PWR_EN_IO1) == HIGH)
        {
            timer = 0;
        }
    }
}

bool Teensy4_HostBusLayer::readOverCurrent()
{
    // overcurrent signal is active low
    return digitalReadFast(OC_FLAG_IO0) == HIGH ? false : true;
}

void Teensy4_HostBusLayer::readSupplyVoltages(uint8_t &rail3V3_percent, uint8_t &rail5V0_percent)
{
    // 02-000658-00, doesn't have feedback on the 5V rail voltage, 
    // so just fake the results based on time from last power on/power off
    if (digitalReadFast(PWR_EN_IO1) == 0)
    {
        rail3V3_percent = (timer > 5999) ? 0 : 50;
        rail5V0_percent = (timer > 5999) ? 0 : 50;
    }
    else
    {
        rail3V3_percent = (timer > 2999) ? 100 : 50;
        rail5V0_percent = (timer > 2999) ? 100 : 50;
    }
}

void Teensy4_HostBusLayer::setIO(uint8_t pinID, pinStates pinState)
{
    switch (pinState)
    {
        case 0:
            pinMode(pinID, OUTPUT);
            digitalWrite(pinID, LOW);
            break;
        case 1:
            pinMode(pinID, OUTPUT);
            digitalWrite(pinID, HIGH);
            break;
        case 2:
            pinMode(pinID, INPUT_PULLDOWN);
            break;
        case 3:
            pinMode(pinID, INPUT_PULLUP);
            break;
        default:
            pinMode(pinID, INPUT);
    }
}

void Teensy4_HostBusLayer::setTP_DISABLE(pinStates pinState)
{
    setIO(TP_DISABLE, pinState);
}

void Teensy4_HostBusLayer::setLID_CLOSE(pinStates pinState)
{
    setIO(LID_CLOSE, pinState);
}

void Teensy4_HostBusLayer::setNFC_STATUS_TP4_ACT(pinStates pinState)
{
    setIO(NFC_STATUS_TP4_ACT, pinState);
}

void Teensy4_HostBusLayer::setFW_SECURITY(pinStates pinState)
{
    setIO(FW_SECURITY, pinState);
}

bool Teensy4_HostBusLayer::drAsserted(void)
{
    return (digitalReadFast(DR_IO9) == LOW) ? true : false;
}

uint16_t Teensy4_HostBusLayer::write(uint8_t i2cAddress, uint16_t count, uint8_t * data)
{
    Wire.begin();
    Wire.beginTransmission(i2cAddress);
    uint16_t length = Wire.write(data, count);
    m_i2cError = Wire.endTransmission(true);
    Wire.end();
    return length;
}

uint16_t Teensy4_HostBusLayer::read(uint8_t i2cAddress, uint16_t count)
{
    Wire.begin();
    uint16_t length = (uint16_t)Wire.requestFrom((int)i2cAddress, (int)count, (int)true);
    Wire.end();
    return length;
}

uint16_t Teensy4_HostBusLayer::available(void)
{
    return (int16_t)Wire.available();
}

uint8_t Teensy4_HostBusLayer::fetch(void)
{
    int result = Wire.read();
    if (result < 0)
    {
        result = 0xff;
    }
    return (uint8_t)result;
}

uint16_t Teensy4_HostBusLayer::writeRestartRead(uint8_t i2cAddress, uint16_t writeCount, uint8_t * writeData, uint16_t readCount)
{
    Wire.begin();
    Wire.beginTransmission(i2cAddress);
    Wire.write(writeData, writeCount);
    m_i2cError = Wire.endTransmission(false);
    uint16_t length = (uint16_t)Wire.requestFrom((int)i2cAddress, (int)readCount, (int)true);
    Wire.end();
    return length;
}





