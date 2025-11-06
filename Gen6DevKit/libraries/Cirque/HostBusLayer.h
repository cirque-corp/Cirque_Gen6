#ifndef HOST_BUS_LAYER
#define HOST_BUS_LAYER

// Copyright (c) 2025 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include "stdint.h"

class HostBusLayer 
{
public:
    HostBusLayer();
    virtual ~ HostBusLayer();

    static HostBusLayer * host_bus;

    const uint32_t &i2cClockFreq {m_i2cClockFreq};
    const uint16_t &i2cMinBufferLength {m_i2cMinBufferLength};

    enum initError : uint8_t
    {
        initOkay = 0,
        initClockFreqError,
        initBufferSizeError,
		initFailed,
    };

    virtual initError init(uint32_t i2cClockFreq_Hz, uint16_t i2cMinBufferLength) = 0;
    
    // Host Bus power rails
    virtual void setPower(bool on) = 0;  // rename to setBusPower()
    virtual bool readOverCurrent(void) = 0;
    // virtual void readSupplyCurrents(uint8_t &rail3V3_uA, uint8_t &rail5V0_mA) = 0;
    virtual void readSupplyVoltages(uint8_t &rail3V3_percent, uint8_t &rail5V0_percent) = 0;
    bool busPowerOn(void);

    // Host bus IO
    enum pinStates : uint8_t
    {
        stateOUTPUTLOW = 0,
        stateOUTPUTHIGH,
        stateINPUTPULLLOW,
        stateINPUTPULLHIGH,
        stateINPUT,
        stateLast
    };

    virtual void setTP_DISABLE(pinStates pinState) = 0;
    virtual void setLID_CLOSE(pinStates pinState) = 0;
    virtual void setNFC_STATUS_TP4_ACT(pinStates pinState) = 0;
    virtual void setFW_SECURITY(pinStates pinState) = 0;
    // void setBTN1(pinStates pinState);
    // void setBTN2(pinStates pinState);
    // void setBTN3(pinStates pinState);
    // void setReserved1(pinStates pinState);
    // void setReserved2(pinStates pinState);
    // void setReserved3(pinStates pinState);
    // void setReserved4(pinStates pinState);

    // Todo: ADD BOOTLOADER RECOVERY functionality

    // what about interrupts???  Have a queue of interrupt events? Have a callback for interrupt events (who's cpu time is that on? not in the interrupt I hope)
    virtual bool drAsserted(void) = 0;
    // virtual bool setDrInterruptCallback(function *)
    // virtual enableInterrupt(bool)

    // host bus I2C
    virtual uint16_t write(uint8_t i2cAddress, uint16_t count, uint8_t * data) = 0;
    virtual uint16_t read(uint8_t i2cAddress, uint16_t count) = 0;
    virtual uint16_t writeRestartRead(uint8_t i2cAddress, uint16_t writeCount, uint8_t * writeData, uint16_t readCount) = 0;
    // results read from the bus get put in a buffer, these let you access the buffer:
    virtual uint16_t available(void) = 0;
    virtual uint8_t fetch(void) = 0;

    // general host bus
    const uint8_t &i2cError {m_i2cError};  // 0 = no error, anything else signals an error

protected:
    uint32_t m_i2cClockFreq;
    uint16_t m_i2cMinBufferLength;
    uint8_t m_i2cError;
    bool m_bus_power_on;

};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_HOSTBUSLAYER)
#define HostBus (*HostBusLayer::host_bus)
#endif

#endif // HOST_BUS_LAYER