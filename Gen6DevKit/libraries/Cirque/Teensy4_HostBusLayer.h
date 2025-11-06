#ifndef ARDUINO_HOST_BUS_LAYER
#define ARDUINO_HOST_BUS_LAYER

#include "HostBusLayer.h"
#include "Arduino.h"

// Hardware layer, for the 02-000658-00 E01 board
#define OC_FLAG_IO0 0
#define PWR_EN_IO1 1
#define ADAPT1_IO2 2
#define ADAPT2_IO8 8
#define DR_IO9 9
#define ADAPT6_IO11 11
#define ADAPT8_IO13 13
#define SDA_IO18 18
#define SCL_IO19 19

#define NFC_STATUS_TP4_ACT ADAPT1_IO2
#define LID_CLOSE ADAPT2_IO8
#define FW_SECURITY ADAPT6_IO11
#define TP_DISABLE ADAPT8_IO13

class Teensy4_HostBusLayer : public HostBusLayer
{
public:
    Teensy4_HostBusLayer();
    ~Teensy4_HostBusLayer();

    initError init(uint32_t i2cClockFreq_Hz, uint16_t i2cMinBufferLength) override;

    void setPower(bool on) override;
    bool readOverCurrent() override;
    void readSupplyVoltages(uint8_t &rail3V3_percent, uint8_t &rail5V0_percent) override;

    void setTP_DISABLE(pinStates pinState) override;
    void setLID_CLOSE(pinStates pinState) override;
    void setNFC_STATUS_TP4_ACT(pinStates pinState) override;
    void setFW_SECURITY(pinStates pinState) override;

    bool drAsserted(void) override;

    uint16_t write(uint8_t i2cAddress, uint16_t count, uint8_t * data) override;
    uint16_t read(uint8_t i2cAddress, uint16_t count) override;
    uint16_t writeRestartRead(uint8_t i2cAddress, uint16_t writeCount, uint8_t * writeData, uint16_t readCount) override;
    uint16_t available(void) override;
    uint8_t fetch(void) override;

private:
    elapsedMicros timer;

    void setIO(uint8_t pinID, pinStates pinState);
};


#endif // ARDUINO_HOST_BUS_LAYER
