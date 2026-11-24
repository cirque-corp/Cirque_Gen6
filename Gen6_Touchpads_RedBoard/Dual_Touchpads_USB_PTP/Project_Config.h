#ifdef __cplusplus
extern "C" {
#endif

#ifndef __PROJECT_CONFIG_H__
#define __PROJECT_CONFIG_H__

// Sensor0
#define CONFIG_HOST_DR0_PIN  9   //Hardware pin of DR0 line
#define CONFIG_HOST_SDA0_PIN  18   //Hardware pin of SDA0 line
#define CONFIG_HOST_SCL0_PIN  19   //Hardware pin of SCL0 line

// Sensor1
#define CONFIG_HOST_DR1_PIN  7   //Hardware pin of DR1 line
#define CONFIG_HOST_SDA1_PIN  17   //Hardware pin of SDA1 line
#define CONFIG_HOST_SCL1_PIN  16   //Hardware pin of SCL1 line

// // Single Sensor
// #define CONFIG_HOST_DR_PIN  CONFIG_HOST_DR0_PIN   //Hardware pin of DR line
// #define CONFIG_HOST_SDA_PIN  CONFIG_HOST_SDA0_PIN   //Hardware pin of DR line
// #define CONFIG_HOST_SCL_PIN  CONFIG_HOST_SCL0_PIN   //Hardware pin of DR line

// Project Specific Header
#define CONFIG_HARDWARE_REV     4
#define CONFIG_FIRMWARE_REV     0
#define CONFIG_PROJECT_REV      1
#define CONFIG_PROJECT_SUB_REV  1
#define CONFIG_PROJECT_NAME "Gen6DevKit"

#define PROJECT_MAX_PACKET_SIZE 53
// #define PROJECT_I2C_FREQUENCY 100000
#define PROJECT_I2C_FREQUENCY 400000

#endif // __PROJECT_CONFIG_H__

#ifdef __cplusplus
}
#endif
