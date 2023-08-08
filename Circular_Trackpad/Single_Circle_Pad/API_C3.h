#ifndef API_C3_H
#define API_C3_H

// Copyright (c) 2018 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

/**@file API_Borax.h
   @brief This file contains the API for working with the Borax firmware
*/

#include <stdint.h>
#include <stdbool.h>

#include "HID_Reports.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
  uint8_t  hardwareId;       // Identifies the HW platform this firmware is designed for
  uint8_t  firmwareId;       // Identifies the FW version that is running on the system
  uint16_t vendorId;         /**< HID VID */ 
  uint16_t productId;        /**< HID PID */ 
  uint16_t versionId;        /**< HID version ID */ 
  uint32_t firmwareRevision; // Provides granular information about which firmware is running */
} systemInfo_t;

typedef enum
{
	QMVCHANNEL_UART = 0,
	QMVCHANNEL_I2C = 1,
	QMVCHANNEL_OFF = 0xFF
} QMVChannelState;

typedef enum
{
	QMV_PREDEMUX = 0,
	QMV_POSTDEMUX = 1,
	QMV_POSTCOMP = 2,
	QMV_COMP = 3,
	QMV_FACTORYCOMP = 4,
} QMVChannelNumbers;


/**** REGISTERS *****/
#define REG_SYS_CONFIG1               (0x20000008)
#define REG_SYS_COMMAND               (0x20000408)
#define REG_SYS_HARDWARE_ID           (0x20000808)
#define REG_SYS_FIRMWARE_ID           (0x20000809)
#define REG16_SYS_VENDOR_ID_BASE      (0x2000080A)
#define REG16_SYS_PRODUCT_ID_BASE     (0x2000080C)
#define REG16_SYS_VERSION_ID_BASE     (0x2000080E)
#define REG32_SYS_FIRMWARE_REV_BASE   (0x20000810)
#define REG_IS_BIG_ENDIAN             (0x20000824)
#define REG_SENSOR_X_COUNT            (0x2001080C)
#define REG_SENSOR_Y_COUNT            (0x2001080D)
#define REG_ENABLE_FLAGS              (0x20020008)
#define REG_COMP_COMMAND              (0x20020408)
#define REG_XY_CONFIG_HEADER          (0x20080000)
#define REG_XY_CONFIG                 (0x20080018)
#define REG_POWER_HEADER              (0x200A0000)
#define REG16_POWER_IDLE_SLEEP_WAIT   (0x200A0008)
#define REG16_POWER_IDLE_SLEEP_TIME   (0x200A000A)
#define REG16_POWER_DEEP_SLEEP_WAIT   (0x200A000C)
#define REG16_POWER_DEEP_SLEEP_TIME   (0x200A000E)
#define REG_POWER_SETTINGS            (0x200A0010)
#define REG_POWER_CMD                 (0x200A0408)
#define REG_FEED_CONFIG1              (0x200E0008)
#define REG_FEED_CONFIG2              (0x200E0009)
#define REG_FEED_CONFIG3              (0x200E000A)
#define REG_FEED_CONFIG4              (0x200E000B)
#define REG_PROJECT_SPECIFIC_HEADER   (0x20190000)
/**** EXTENDED REGISTERS *****/
#define EXTREG_COMP_MATRIX_LENGTH     (0x30010000)
#define EXTREG_COMP_MATRIX_DATA       (0x30010002)
/*******************/


/**** BITMASKS *****/
// REG_SYS_CONFIG1           
#define SYS_CONFIG_DISABLE_TRACKING          (0x01)
 
// REG_SYS_COMMAND        
#define SYS_CMD_SAVE_CONFIG                  (0x01)
#define SYS_CMD_RESTORE_SAVED_CONFIG         (0x02)
#define SYS_CMD_RESTORE_FACTORY_CONFIG       (0x04)
#define SYS_CMD_ERROR                        (0x80)
#define SYS_CMD_CLEAR                        (0x00)

// REG_IS_BIG_ENDIAN          
#define IS_BIG_ENDIAN_CONFIG_MASK            (0x01)
                        
// REG_ENABLE_FLAGS 
#define COMPFLAG_SCALE_MEAS_RESULTS          (0x01)
#define COMPFLAG_OFFSET_CORRECTION_ENABLE    (0x02)
#define COMPFLAG_BCKGND_COMP_ENABLE          (0x04)
#define COMPFLAG_SIGNAL_SIGN_COMP_ENABLE     (0x08)
#define COMPFLAG_CONGRUENCE_COMP_ENABLE      (0x10)
#define COMPFLAG_ALL_SIGNAL_POS_COMP_ENABLE  (0x20)
#define COMPFLAG_MOST_SIGNAL_NEG_COMP_ENABLE (0x40)
#define COMPFLAG_ALL_COMPS_ENABLE            (0x7E)
  
// REG_COMP_COMMAND            
#define COMP_CMD_SCHEDULE_COMP               (0x01)
#define COMP_CMD_SAVE_COMP_AS_FACTORY        (0x02)
#define COMP_CMD_ERASE_STORED_FACTORY_COMP   (0x80)

// REG_XY_CONFIG             
#define XY_CONFIG_INVERT_X                   (0x01)
#define XY_CONFIG_INVERT_Y                   (0x02)
#define XY_CONFIG_SWAP_XY                    (0x04)
#define XY_CONFIG_DISABLE_SCALING            (0x08)
#define XY_CONFIG_LIFTOFF_OOB                (0x10)

// REG_POWER_SETTINGS        
#define POWER_SETTING_DISABLE_DEEP_SLEEP     (0x01)
#define POWER_SETTING_DISABLE_IDLE_SLEEP     (0x02)
#define POWER_SETTING_DONT_WAKE_FROM_TOUCH   (0x04)
#define POWER_SETTING_DONT_WAKE_FROM_BUTTON  (0x08)

// REG_POWER_CMD   
#define POWER_CMD_FORCE_SLEEP                (0x01)
#define POWER_CMD_CANCEL_FORCE_SLEEP         (0x02)

// REG_FEED_CONFIG1  
#define FC1_LINEAR_CORRECTION_ENABLED        (0x04)

// REG_FEED_CONFIG2          
#define FC2_PRIMARY_FEED_MASK                (0x03)
#define FC2_PRIMARY_FEED_SHIFT               (0x00)
#define FC2_SECONDARY_FEED_MASK              (0x0C)
#define FC2_SECONDARY_FEED_SHIFT             (0x02)
#define FC2_LOCK_DATA_PORT                   (0x10)
#define FC2_DATA_PORT_PRIMARY0_SECONDARY1    (0x20)
         
// REG_FEED_CONFIG3 
#define FC3_PS2_FEED_ENABLE                  (0x01)
#define FC3_I2C_FEED_ENABLE                  (0x02)
#define FC3_USB_FEED_ENABLE                  (0x04)
#define FC3_PS2_INTERFACE_PRIORITY           (0x10)
#define FC3_I2C_INTERFACE_PRIORITY           (0x20)
#define FC3_USB_INTERFACE_PRIORITY           (0x40)

// REG_FEED_CONFIG4 
#define FC4_PS2_MASK                         (0x03)
#define FC4_PS2_OFFSET                       (0x00)
#define FC4_I2C_MASK                         (0x0C)
#define FC4_I2C_OFFSET                       (0x02)
#define FC4_USB_MASK                         (0x30)
#define FC4_USB_OFFSET                       (0x04)
#define FC4_REL_MODE                         (0x00)
#define FC4_ABS_MODE                         (0x01)
#define FC4_NORM_OUTPUT                      (0x00)
#define FC4_ADV_OUTPUT                       (0x02)
/*******************/

/******************* IMPORTANT FUNCTIONS ******************/

/** Initializes the Host Bus I2C connection.
    Must be run prior to any API calls
    The I2C bus commonly runs at a clock frequency of 400kHz.
    The operation of touch system is also tested at 100kHz.
*/
void API_C3_init(int32_t I2CFrequency, uint8_t I2CAddress);

void API_C3_reinit(void);

bool API_C3_DR_Asserted(void);

bool API_C3_getReport(HID_report_t* result);

/** Reads the contents of a register at a given address */
uint8_t API_C3_readRegister(uint32_t address);

uint8_t API_C3_endianState(void);

/** Reads the contents of two contiguous registers and returns them as a 16bit value
 * @param address of the base. registers read are address and address+1
 * @return
 */
uint16_t API_C3_readRegister16(uint32_t address);

uint32_t API_C3_readRegister32(uint32_t address);

/** Checks the contents of a register at a given address
 * @param address: address of the register to check
 * @param expectData: expected data in register
 * @return true if register data matches expectData, else false
 */
bool API_C3_checkRegister(uint32_t address, uint8_t expectData);

/** Checks the contents of a register at a given address
 * @param address: address of the base register to check. Registers read are address and address+1.
 * @param expectData: expected data in register
 * @return true if register data matches expectData, else false
 */
bool API_C3_checkRegister16(uint32_t address, uint16_t expectData);

/** Checks the contents of a register at a given address
 * @param address: address of the base register to check. Registers read are address through address+3.
 * @param expectData: expected data in register
 * @return true if register data matches expectData, else false
 */
bool API_C3_checkRegister32(uint32_t address, uint32_t expectData);

/** Sets register contents.
    It is best to first read a register, modify the necessary bits, then write it back
*/
void API_C3_writeRegister(uint32_t address, uint8_t value);

void API_C3_writeRegister16(uint32_t address, uint16_t contents);

void API_C3_writeRegister32(uint32_t address, uint32_t contents);

/** Reads contiguous range of memory
 *
 * @param address Location of first byte
 * @param buffer Buffer where memory contents are read into
 * @param bufferLength number of bytes to read
 * @param chunkSize Max number of bytes to read at a time
 * @return Error state
 */
 
void API_C3_readSystemInfo(systemInfo_t* result);
 
uint8_t API_C3_getMemoryContents(uint32_t address, uint8_t * buffer, uint16_t bufferLength, uint16_t chunkSize);


/******************************************************/

/****************** ACTIONS ***************************/

/**
 * Sets the report type to PTP mode
 */
bool API_C3_setPtpMode();

/**
 * Sets the Report type to Mouse and Keyboard reporting
    This is compliant with HID protocol.
    Devices are usually in Relative mode by default.
    Wait 50ms for it to take effect.
*/
bool API_C3_setRelativeMode(void);

/** Forces the imaging system to gather a new compensation matrix
    Can take 50ms for it to take effect
*/
bool API_C3_forceComp(void);

/** 
* Turns Off all algorithms that update the compensation matrix
*/
bool API_C3_disableComp(void);

/** 
* Saves the current comp matrix as the factory default matrix
*/
bool API_C3_saveFactoryComp(void);

/** 
* A helper function
*/
int8_t API_C3_waitForBitClear(uint32_t address, uint8_t mask);

/** 
 * Does a forceComp(), then saves it to the factory comp in Flash. The saved 
 * comp is then used to process the touch information in complex situations
 * involving noise or unexpected objects on the sensor.
 * Do this in a factory setting where you know nothing is on the sensor
 * Can take 100 msec for this to take effect
*/
bool API_C3_factoryCalibrate(void);

/** Turns off the XY reports.
    The touchpad continues to calculate the touch data, but will stop sending DR signals
*/
bool API_C3_disableFeed(void);

/** Turns on the feed 
    It can take 50ms for this to take effect. 
*/
bool API_C3_enableFeed(void);

bool API_C3_disableTracking(void);
bool API_C3_enableTracking(void);
// bool API_C3_enableScaling(void);
// bool API_C3_disableScaling(void);
bool API_C3_enableLinearCorrection(void);
bool API_C3_disableLinearCorrection(void);

// Common channel numbers in QMVChannelNumbers enum
bool API_C3_SetQMVStream(uint8_t channelNumber, QMVChannelState state);
// From: CoreFW/Includes/QuickMeasViewDataControl.h
//QmvId_Image_PreDemux, // 0
//QmvId_Image_PostDemux, // 1
//QmvId_Image_PostComp, //2
//QmvId_Image_Comp,  //3
// ...
	//QmvId_IdleDetect,  // 16
//QmvId_IdleDetect_Accumulation,  // 17
//QmvId_IdleDetect_MeasData, // 18

/** This probably needs a better name
 *
 * @return true if the control struct exists in memory, false otherwise
 */
bool API_C3_XYConfig_isImplemented();

bool API_C3_setInvertX(void);
bool API_C3_clearInvertX(void);

bool API_C3_setInvertY(void);
bool API_C3_clearInvertY(void);

bool API_C3_setSwapXY(void);
bool API_C3_clearSwapXY(void);

bool API_C3_enableLogicalScaling(void);
bool API_C3_disableLogicalScaling(void);

bool API_C3_enableComp(void);
bool API_C3_disableComp(void);


/**
 * @ingroup PowerManagement
 * returns true if the power management struct is initialized
 */
bool API_C3_PowerManagement_isImplemented(void);

bool API_C3_setPowerSetting(uint8_t registerValue);
bool API_C3_setPowerCommand(uint8_t registerValue);

/**
 * @ingroup PowerManagement
 * Sets the ms before touchpad enters idle
 */
bool API_C3_setTimeBeforeIdle(uint16_t timeBeforeIdle_100ms); //given in 100 ms

/**
 * @ingroup PowerManagement
 * After the touchpad enters idle state, it wakes up periodically to check if someone is
 * touching the touchpad. The timeInIdle register is how often the touchpad wakes up in 100us.
 * The units of the register are 100us increments. ie: a value of 500 means to wake up every 50ms.
 */
bool API_C3_setTimeInIdle(uint16_t idleTime);
bool API_C3_setTimeBeforeSleep(uint16_t timeBeforeSleep_100ms); //given in 100 ms
bool API_C3_setTimeInSleep(uint16_t sleepTime); //given in 100us increments
bool API_C3_setIdleDisable(bool idleDisable);
bool API_C3_readIdleDisable();
bool API_C3_setSleepDisable(bool sleepDisable);
bool API_C3_readSleepDisable();

/**
 * Checks if the Project Specific register space is active.
 */
bool API_C3_ProjectSpecific_isImplemented(void);

/**
 * Config settings save/restore
 */
bool API_C3_saveConfig(void);
bool API_C3_restoreSavedConfig(void);
bool API_C3_restoreFactoryConfig(void);

void API_C3_sensorSize(uint8_t * sizeX, uint8_t * sizeY, uint16_t * compByteLength);
void API_C3_readComp(int16_t * compMatrix, uint16_t compByteLength, uint16_t chunkSize);

bool API_C3_isButtonPressed(HID_report_t* report, uint8_t buttonMask);

#ifdef __cplusplus
}
#endif

#endif //API_C3_H
