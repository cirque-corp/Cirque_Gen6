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
	bool uncommittedVersion;   // if set the firmware is experimental 
	bool branchVersion;        // if set the firmware is contains unique code
	uint8_t  developerId;      // Identifies who built the code
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
void API_C3_setPtpMode();

/**
 *  Sets the report type to CRQ_ABSOLUTE Mode
 */
void API_C3_setCRQ_AbsoluteMode(void);

/**
 * Sets the Report type to Mouse and Keyboard reporting
    This is compliant with HID protocol.
    Devices are usually in Relative mode by default.
    Wait 50ms for it to take effect.
*/
void API_C3_setRelativeMode(void);

/** Forces the imaging system to gather a new compensation matrix
    Can take 50ms for it to take effect
*/
void API_C3_forceComp(void);

/** 
* Turns Off all algorithms that update the compensation matrix
*/
void API_C3_disableComp(void);

/** 
* Saves the current comp matrix as the factory default matrix
*/
void API_C3_saveFactoryComp(void);

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
void API_C3_disableFeed(void);

/** Turns on the feed 
    It can take 50ms for this to take effect. 
*/
void API_C3_enableFeed(void);

void API_C3_disableTracking(void);
void API_C3_enableTracking(void);
void API_C3_enableScaling(void);
void API_C3_disableScaling(void);
void API_C3_enableLinearCorrection(void);
void API_C3_disableLinearCorrection(void);

// Common channel numbers in QMVChannelNumbers enum
void API_C3_SetQMVStream(uint8_t channelNumber, QMVChannelState state);
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

void API_C3_setInvertX(void);
void API_C3_clearInvertX(void);

void API_C3_setInvertY(void);
void API_C3_clearInvertY(void);

void API_C3_setSwapXY(void);
void API_C3_clearSwapXY(void);

void API_C3_enableLogicalScaling(void);
void API_C3_disableLogicalScaling(void);

void API_C3_enableComp(void);
void API_C3_disableComp(void);


/**
 * @ingroup PowerManagement
 * returns true if the power management struct is initialized
 */
bool API_C3_PowerManagement_isImplemented(void);

void API_C3_setPowerSetting(uint8_t registerValue);
void API_C3_setPowerCommand(uint8_t registerValue);

/**
 * @ingroup PowerManagement
 * Sets the ms before touchpad enters idle
 */
void API_C3_setTimeBeforeIdle(uint16_t timeBeforeIdle_100ms); //given in 100 ms

/**
 * @ingroup PowerManagement
 * After the touchpad enters idle state, it wakes up periodically to check if someone is
 * touching the touchpad. The timeInIdle register is how often the touchpad wakes up in 100us.
 * The units of the register are 100us increments. ie: a value of 500 means to wake up every 50ms.
 */
void API_C3_setTimeInIdle(uint16_t idleTime);
void API_C3_setTimeBeforeSleep(uint16_t timeBeforeSleep_100ms); //given in 100 ms
void API_C3_setTimeInSleep(uint16_t sleepTime); //given in 100us increments
void API_C3_setIdleDisable(bool idleDisable);
bool API_C3_readIdleDisable();
void API_C3_setSleepDisable(bool sleepDisable);
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

// for now these are a pass-through to the HID_reports code, they only work for CRQ_Absolute mode 
bool API_C3_isFingerValid(HID_report_t* report, uint8_t finger_num);
bool API_C3_isFingerContacted(HID_report_t* report, uint8_t finger_num);
uint8_t API_C3_numberFingers(HID_report_t* report);
bool API_C3_isButtonPressed(HID_report_t* report, uint8_t buttonMask);

#ifdef __cplusplus
}
#endif

#endif //API_C3_H
