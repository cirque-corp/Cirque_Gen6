#ifndef INA219_H
#define INA219_H

// Copyright (c) 2018 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#ifdef __cplusplus
extern "C" {
#endif

#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>

// Config Register Masks
#define CONFIG__FS_RANGE_16V      0x0000
#define CONFIG__FS_RANGE_32V      0x2000

#define CONFIG__SHUNT_PGA_DIV1    0x0000
#define CONFIG__SHUNT_PGA_DIV2    0x0800
#define CONFIG__SHUNT_PGA_DIV4    0x1000
#define CONFIG__SHUNT_PGA_DIV8    0x1800

#define CONFIG__SHUNT_ADC_AVERAGE_2   (0x0009 << 3) // 1.1ms
#define CONFIG__SHUNT_ADC_AVERAGE_4   (0x000A << 3) // 2.1ms
#define CONFIG__SHUNT_ADC_AVERAGE_8   (0x000B << 3) // 4.3ms
#define CONFIG__SHUNT_ADC_AVERAGE_16  (0x000C << 3) // 8.5ms
#define CONFIG__SHUNT_ADC_AVERAGE_32  (0x000D << 3) // 17ms
#define CONFIG__SHUNT_ADC_AVERAGE_64  (0x000E << 3) // 34ms
#define CONFIG__SHUNT_ADC_AVERAGE_128 (0x000F << 3) // 68ms
#define CONFIG__BUS_ADC_AVERAGE_1     (0x0008 << 7) // 0.5ms
#define CONFIG__BUS_ADC_AVERAGE_2     (0x0009 << 7) // 1.1ms
#define CONFIG__BUS_ADC_AVERAGE_4     (0x000A << 7) // 2.1ms
#define CONFIG__BUS_ADC_AVERAGE_8     (0x000B << 7) // 4.3ms
#define CONFIG__BUS_ADC_AVERAGE_16    (0x000C << 7) // 8.5ms
#define CONFIG__BUS_ADC_AVERAGE_32    (0x000D << 7) // 17ms
#define CONFIG__BUS_ADC_AVERAGE_64    (0x000E << 7) // 34ms
#define CONFIG__BUS_ADC_AVERAGE_128   (0x000F << 7) // 68ms

#define CONFIG_DELAY_1 (592)
#define CONFIG_DELAY_2 (2 * 592)
#define CONFIG_DELAY_4 (4 * 592)
#define CONFIG_DELAY_8 (8 * 592)
#define CONFIG_DELAY_16 (0)
#define CONFIG_DELAY_32 (0)
#define CONFIG_DELAY_64 (0)
#define CONFIG_DELAY_128 (0)
#define CONFIG_DELAY_POLL (0)

#define CONFIG__MODE_POWER_DOWN     0x0000
#define CONFIG__MODE_SHUNT_TRIG     0x0001
#define CONFIG__MODE_BUS_TRIG       0x0002
#define CONFIG__MODE_SHUNT_BUS_TRIG 0x0003
#define CONFIG__MODE_ADC_OFF        0x0004
#define CONFIG__MODE_SHUNT_CONT     0x0005
#define CONFIG__MODE_BUS_CONT       0x0006
#define CONFIG__MODE_SHUNT_BUS_CONT 0x0007

#define REGISTER__CONFIG            0x00
#define REGISTER__SHUNT_VOLTAGE     0x01
#define REGISTER__BUS_VOLTAGE       0x02
#define REGISTER__POWER             0x03
#define REGISTER__CURRENT           0x04
#define REGISTER__CALIBRATION       0x05

#define CONFIG__RESET               0x8000
#define CONFIG__BUS_16V             0x0000
#define CONFIG__BUS_32V             0x2000

#define CONFIG__BUS_ADC_RES_9       0x0000
#define CONFIG__BUS_ADC_RES_10      0x0080
#define CONFIG__BUS_ADC_RES_11      0x0100
#define CONFIG__BUS_ADC_RES_12      0x0180
#define CONFIG__BUS_ADC_AVERAGING   0x0400

#define CONFIG__SHUNT_ADC_RES_9     0x0000
#define CONFIG__SHUNT_ADC_RES_10    0x0008
#define CONFIG__SHUNT_ADC_RES_11    0x0010
#define CONFIG__SHUNT_ADC_RES_12    0x0018
#define CONFIG__SHUNT_ADC_AVERAGING 0x0040

/************************************************************/
/************************************************************/
/********************  PUBLIC FUNCTIONS *********************/

bool INA219_init(uint8_t slaveAddress, uint32_t i2cBitRate);

void INA219_config(uint16_t data);

void INA219_reset(void);

int32_t INA219_readCurrent(void);

int32_t INA219_readBusVoltage(void);

int32_t INA219_measureShuntVoltage(uint16_t averagingMask);

int32_t INA219_measureBusVoltage(uint16_t averagingMask, uint32_t delay);

//void INA219_powerDown(void); Not defined.

#ifdef __cplusplus
}
#endif

#endif
