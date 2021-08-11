#ifdef __cplusplus
extern "C" {
#endif

#ifndef __PROJECT_CONFIG_H__
#define __PROJECT_CONFIG_H__

#define CONFIG_HOST_DR_PIN  9   //Hardware pin of DR line

// Project Specific Header
#define CONFIG_HARDWARE_REV     3
#define CONFIG_FIRMWARE_REV     1
#define CONFIG_PROJECT_REV      1
#define CONFIG_PROJECT_SUB_REV  1
#define CONFIG_PROJECT_NAME "Gen6DevKit"

#define PROJECT_MAX_PACKET_SIZE 53
#define PROJECT_I2C_FREQUENCY 400000

#endif // __PROJECT_CONFIG_H__

#ifdef __cplusplus
}
#endif
