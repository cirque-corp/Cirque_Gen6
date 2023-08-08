// Copyright (c) 2018 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include "Arduino.h"
#include "API_C3.h"
#include "API_HostBus.h"
#include "API_Hardware.h"

#define TWI_BUFFER_LENGTH 53

#ifndef PROJECT_MAX_PACKET_SIZE
  // Your project can change this, generally 53 bytes is the right answer
  #define PROJECT_MAX_PACKET_SIZE 53
#endif

#define BIG_ENDIAN 1
bool endianStateKnown = false;
uint8_t endianState;

void byteArrayToImage(uint8_t * byteArray, uint16_t byteLength, int16_t * imageArray);

// Support functions

// Check a configuration header to see if it is supported/valid
bool isValidHeader(uint32_t baseAddress)
{
  uint8_t buffer[8];
  bool result = false;
  API_C3_getMemoryContents(baseAddress, buffer, 8, 8);
  //base address is stored at base address if implemented
  if( buffer[0] == (baseAddress & 0xFF)
      && buffer[1] == ((baseAddress >> 8) & 0xFF)
      && buffer[2] == ((baseAddress >> 16) & 0xFF)
      && buffer[3] == ((baseAddress >> 24) & 0xFF))
  {
    //header data must be non-zero
    if( buffer[4] != 0
        || buffer[5] != 0
        || buffer[6] != 0
        || buffer[7] != 0)
    {
      result = true;
    }
  }
  return result;
}

// ----------------------------------------------
// API Functions

void API_C3_init(int32_t I2CFrequency, uint8_t I2CAddress)
{
  HB_init(I2CFrequency, I2CAddress);
  API_C3_reinit();
}

void API_C3_reinit(void)
{
    // Clear any HID I2C reset response from a power-on reset
  // HID reset will happen within 5 msec, so every 1 msec
    // check DR then read the report if you have it
  // Other reports may also come in (but not likely)
  for (int x = 0; x < 5; x++)
  {
    API_Hardware_delay(1);
    if (HB_DR_Asserted())
    {
      // blindly read max packet size just to simplify this loop
      uint8_t hid_packet[PROJECT_MAX_PACKET_SIZE];
      HB_readReport(hid_packet, PROJECT_MAX_PACKET_SIZE);
    }
  }
  
  // trigger a check of the endianess state
  endianStateKnown = false;
}

bool API_C3_DR_Asserted(void)
{
    return (HB_DR_Asserted()); 
}

bool API_C3_getReport(HID_report_t* result)
{
  // returns true if report received and decoded ok
    uint8_t packet[PROJECT_MAX_PACKET_SIZE]; 
    HB_readReport(packet, PROJECT_MAX_PACKET_SIZE); // fill packet with raw i2c data
    return HID_decodeReport(packet, result);   // decode packet to a report
}

uint8_t API_C3_readRegister(uint32_t address)
{
  uint8_t contents = 0;
  HB_readExtendedMemory(address, &contents, 1);
  return contents;
}

uint8_t API_C3_endianState(void)
{
  if (!endianStateKnown)
  {
    // read this once and save the value for future use
    endianState = API_C3_readRegister(REG_IS_BIG_ENDIAN) & IS_BIG_ENDIAN_CONFIG_MASK;
    endianStateKnown = true;
  }

  return endianState;
}

// 
uint16_t API_C3_readRegister16(uint32_t address)
{
  uint16_t result;
  uint8_t buf[2];
  HB_readExtendedMemory(address, buf, 2);

  if (API_C3_endianState() == BIG_ENDIAN)
  {
    // high byte at low address
    result = buf[0] << 8;
    result |= buf[1];
  }
  else
  {
    result = buf[1] << 8;
    result |= buf[0];
  }
    
  return result;
}

uint32_t API_C3_readRegister32(uint32_t address)
{
  uint32_t result;
  uint8_t buf[4];
  HB_readExtendedMemory(address, buf, 4);

  if (API_C3_endianState() == BIG_ENDIAN)
  {
    // high byte at low address
    result = buf[0] << 24;
    result |= buf[1] << 16;
    result |= buf[2] << 8;
    result |= buf[3];
  }
  else
  {
    result = buf[3] << 24;
    result |= buf[2] << 16;
    result |= buf[1] << 8;
    result |= buf[0];
  }
    
  return result;
}

bool API_C3_checkRegister(uint32_t address, uint8_t expectData)
{
  uint8_t readData = API_C3_readRegister(address);
  return (expectData == readData);
}

bool API_C3_checkRegister16(uint32_t address, uint16_t expectData)
{
  uint16_t readData = API_C3_readRegister16(address);
  return (expectData == readData);
}

bool API_C3_checkRegister32(uint32_t address, uint32_t expectData)
{
  uint32_t readData = API_C3_readRegister32(address);
  return (expectData == readData);
}

int8_t API_C3_waitForBitClear(uint32_t address, uint8_t mask)
{
  int8_t count;

  // This process takes time to complete
  // Monitor the progress, give it time to complete, don't flood it with polling requests
  API_Hardware_delay(30);  // assume it'll take at least 30 msec
  for (count = 0; count > -30; count--) // and will be done within 60 msec
  {
    // counts down to -30
    API_Hardware_delay(1);
    // read status to see if it's done yet
    if ((API_C3_readRegister(address) & mask) == 0)
    {
      // the process is done, exit the loop
      count = -count; // flip sign - (+) means it worked
      break;
    }
  }
  
  return count; // if count is negative it timed out
}

void API_C3_writeRegister(uint32_t address, uint8_t contents)
{
  HB_writeExtendedMemory(address, &contents, 1);
}

void API_C3_writeRegister16(uint32_t address, uint16_t contents)
{
  uint8_t buf[2];
  if (API_C3_endianState() == BIG_ENDIAN)
  {
    // high byte at low address
    buf[0] = (contents >> 8) & 0xFF;
    buf[1] = contents & 0xFF;
  }
  else
  {
    buf[1] = (contents >> 8) & 0xFF;
    buf[0] = contents & 0xFF;
  }
  HB_writeExtendedMemory(address, buf, 2);
}

void API_C3_writeRegister32(uint32_t address, uint32_t contents)
{
  uint8_t buf[4];
  if (API_C3_endianState() == BIG_ENDIAN)
  {
    // high byte at low address
    buf[0] = (contents >> 24) & 0xFF;
    buf[1] = (contents >> 16) & 0xFF;
    buf[2] = (contents >> 8) & 0xFF;
    buf[3] = contents & 0xFF;
  }
  else
  {
    buf[3] = (contents >> 24) & 0xFF;
    buf[2] = (contents >> 16) & 0xFF;
    buf[1] = (contents >> 8) & 0xFF;
    buf[0] = contents & 0xFF;
  }
  HB_writeExtendedMemory(address, buf, 4);
}

uint8_t API_C3_getMemoryContents(uint32_t address, uint8_t * buffer, uint16_t bufferLength, uint16_t chunkSize)
{
  uint16_t i;
  uint8_t error = SUCCESS;
  if( chunkSize == 0 ) chunkSize = bufferLength;

  for( i = 0; ( i + 1 ) * chunkSize <= bufferLength; i++ )
  {
    error = HB_readExtendedMemory(address + i * chunkSize, &buffer[i * chunkSize], chunkSize);
    if( error != SUCCESS ) return error;
  }

  if( i * chunkSize < bufferLength )
  {
    error = HB_readExtendedMemory(address + i * chunkSize, &buffer[i * chunkSize], bufferLength - i * chunkSize);
  }

  return error;
}

void API_C3_readSystemInfo(systemInfo_t* result)
{
  result->hardwareId = API_C3_readRegister(REG_SYS_HARDWARE_ID);
  result->firmwareId = API_C3_readRegister(REG_SYS_FIRMWARE_ID);
  result->vendorId = API_C3_readRegister16(REG16_SYS_VENDOR_ID_BASE);  // 0x0488 typically
  result->productId = API_C3_readRegister16(REG16_SYS_PRODUCT_ID_BASE);
  result->versionId = API_C3_readRegister16(REG16_SYS_VERSION_ID_BASE);
  result->firmwareRevision = API_C3_readRegister32(REG32_SYS_FIRMWARE_REV_BASE);
}

bool API_C3_forceComp(void)
{
  uint8_t compReg = API_C3_readRegister(REG_COMP_COMMAND);
  compReg |= COMP_CMD_SCHEDULE_COMP;
  API_C3_writeRegister(REG_COMP_COMMAND,compReg);
  return (API_C3_waitForBitClear(REG_COMP_COMMAND, COMP_CMD_SCHEDULE_COMP) >= 0);
}

bool API_C3_setRelativeMode()
{
  uint8_t feedConfig4 = API_C3_readRegister(REG_FEED_CONFIG4);
  feedConfig4 &= ~FC4_I2C_MASK; // leave USB, PS2, and unused bits unchanged
  // I2C interface bits are clear, so "normal, relative" report mode
  API_C3_writeRegister(REG_FEED_CONFIG4, feedConfig4);
  return API_C3_checkRegister(REG_FEED_CONFIG4, feedConfig4);
}

bool API_C3_setPtpMode()
{
  uint8_t feedConfig4 = API_C3_readRegister(REG_FEED_CONFIG4);
  feedConfig4 &= ~FC4_I2C_MASK; // leave USB, PS2, and unused bits unchanged
  feedConfig4 |= ((FC4_NORM_OUTPUT | FC4_ABS_MODE) << FC4_I2C_OFFSET); // set "normal, absolute" mode
  API_C3_writeRegister(REG_FEED_CONFIG4, feedConfig4);
  return API_C3_checkRegister(REG_FEED_CONFIG4, feedConfig4);
}

bool API_C3_saveFactoryComp(void)
{
  uint8_t compCommandFlag = API_C3_readRegister(REG_COMP_COMMAND);
  compCommandFlag |= COMP_CMD_SAVE_COMP_AS_FACTORY;
  API_C3_writeRegister(REG_COMP_COMMAND, compCommandFlag);
  return (API_C3_waitForBitClear(REG_COMP_COMMAND, COMP_CMD_SAVE_COMP_AS_FACTORY) >= 0);

}

bool API_C3_factoryCalibrate(void)
{
  bool result = API_C3_forceComp();  
  result &= API_C3_saveFactoryComp();
  return result;
}

bool API_C3_disableFeed(void)
{
  uint8_t feedConfig3 = API_C3_readRegister(REG_FEED_CONFIG3);
  feedConfig3 &= ~(FC3_PS2_FEED_ENABLE|FC3_I2C_FEED_ENABLE|FC3_USB_FEED_ENABLE);
  API_C3_writeRegister(REG_FEED_CONFIG3, feedConfig3);
  return API_C3_checkRegister(REG_FEED_CONFIG3, feedConfig3);
}

bool API_C3_enableFeed(void)
{
  uint8_t feedConfig3 = API_C3_readRegister(REG_FEED_CONFIG3);
  feedConfig3 |= FC3_I2C_FEED_ENABLE;
  API_C3_writeRegister(REG_FEED_CONFIG3, feedConfig3);
  return API_C3_checkRegister(REG_FEED_CONFIG3, feedConfig3);
}

// bool API_C3_disableScaling(void)
// {
//   uint8_t logicalScalar_flag = API_C3_readRegister(REG_XY_CONFIG);
//   logicalScalar_flag |= XY_CONFIG_DISABLE_SCALING;
//   API_C3_writeRegister(REG_XY_CONFIG, logicalScalar_flag);
//   return API_C3_checkRegister(REG_XY_CONFIG, logicalScalar_flag);
// }

// bool API_C3_enableScaling(void)
// {
//   uint8_t logicalScalar_flag = API_C3_readRegister(REG_XY_CONFIG);
//   logicalScalar_flag &= ~XY_CONFIG_DISABLE_SCALING;
//   API_C3_writeRegister(REG_XY_CONFIG, logicalScalar_flag);
//   return API_C3_checkRegister(REG_XY_CONFIG, logicalScalar_flag);
// }

bool API_C3_disableTracking()
{
    uint8_t sysConfig1 = API_C3_readRegister(REG_SYS_CONFIG1);      
    sysConfig1 |= SYS_CONFIG_DISABLE_TRACKING;                                             
    API_C3_writeRegister(REG_SYS_CONFIG1, sysConfig1);              
    return API_C3_checkRegister(REG_SYS_CONFIG1, sysConfig1);       
}

bool API_C3_enableTracking()
{
    uint8_t sysConfig1 = API_C3_readRegister(REG_SYS_CONFIG1);     
    sysConfig1 &= ~SYS_CONFIG_DISABLE_TRACKING;                                           
    API_C3_writeRegister(REG_SYS_CONFIG1, sysConfig1);             
    return API_C3_checkRegister(REG_SYS_CONFIG1, sysConfig1);      
}

bool API_C3_enableComp()
{
    uint8_t compConfig = API_C3_readRegister(REG_ENABLE_FLAGS);     
    compConfig |= COMPFLAG_ALL_COMPS_ENABLE;                                             
    API_C3_writeRegister(REG_ENABLE_FLAGS,compConfig);              
    return API_C3_checkRegister(REG_ENABLE_FLAGS,compConfig);       
}

bool API_C3_disableComp()
{
    uint8_t compConfig = API_C3_readRegister(REG_ENABLE_FLAGS);      
    compConfig &= ~COMPFLAG_ALL_COMPS_ENABLE;                                             
    API_C3_writeRegister(REG_ENABLE_FLAGS,compConfig);               
    return API_C3_checkRegister(REG_ENABLE_FLAGS,compConfig);        
}

/**Untested because I'm not really sure what it actually does.
 *
 */
bool API_C3_enableLinearCorrection(void)
{
  uint8_t feedConfig1 = API_C3_readRegister(REG_FEED_CONFIG1);
  feedConfig1 |= FC1_LINEAR_CORRECTION_ENABLED;
  API_C3_writeRegister(REG_FEED_CONFIG1, feedConfig1);
  return API_C3_checkRegister(REG_FEED_CONFIG1, feedConfig1);
}

/**Untested because I'm not really sure what it actually does.
 *
 */
bool API_C3_disableLinearCorrection(void)
{
  uint8_t feedConfig1 = API_C3_readRegister(REG_FEED_CONFIG1);
  feedConfig1 &= ~FC1_LINEAR_CORRECTION_ENABLED;
  API_C3_writeRegister(REG_FEED_CONFIG1, feedConfig1);
  return API_C3_checkRegister(REG_FEED_CONFIG1, feedConfig1);
}

bool API_C3_SetQMVStream(uint8_t channelNumber, QMVChannelState state)
{
  uint32_t address = 0x20160408 + channelNumber;
  uint8_t registerValue = (state != QMVCHANNEL_OFF) ? (state | 0x80) : 0;
  API_C3_writeRegister(address, registerValue);
  return API_C3_checkRegister(address, registerValue);
}

bool API_C3_XYConfig_isImplemented()
{
  return isValidHeader(REG_XY_CONFIG_HEADER);
}

bool API_C3_setInvertX()
{
  uint8_t logicalScalar_flag = API_C3_readRegister(REG_XY_CONFIG);
  logicalScalar_flag |= XY_CONFIG_INVERT_X;
  API_C3_writeRegister(REG_XY_CONFIG, logicalScalar_flag);
  return API_C3_checkRegister(REG_XY_CONFIG, logicalScalar_flag);
}

bool API_C3_clearInvertX()
{
  uint8_t logicalScalar_flag = API_C3_readRegister(REG_XY_CONFIG);
  logicalScalar_flag &= ~XY_CONFIG_INVERT_X;
  API_C3_writeRegister(REG_XY_CONFIG, logicalScalar_flag);
  return API_C3_checkRegister(REG_XY_CONFIG, logicalScalar_flag);
}

bool API_C3_setInvertY()
{
  uint8_t logicalScalar_flag = API_C3_readRegister(REG_XY_CONFIG);
  logicalScalar_flag |= XY_CONFIG_INVERT_Y;
  API_C3_writeRegister(REG_XY_CONFIG, logicalScalar_flag);
  return API_C3_checkRegister(REG_XY_CONFIG, logicalScalar_flag);
}

bool API_C3_clearInvertY()
{
  uint8_t logicalScalar_flag = API_C3_readRegister(REG_XY_CONFIG);
  logicalScalar_flag &= ~XY_CONFIG_INVERT_Y;
  API_C3_writeRegister(REG_XY_CONFIG, logicalScalar_flag);
  return API_C3_checkRegister(REG_XY_CONFIG, logicalScalar_flag);
}

bool API_C3_setSwapXY()
{
  uint8_t logicalScalar_flag = API_C3_readRegister(REG_XY_CONFIG);
  logicalScalar_flag |= XY_CONFIG_SWAP_XY;
  API_C3_writeRegister(REG_XY_CONFIG, logicalScalar_flag);
  return API_C3_checkRegister(REG_XY_CONFIG, logicalScalar_flag);
}

bool API_C3_clearSwapXY()
{
  uint8_t logicalScalar_flag = API_C3_readRegister(REG_XY_CONFIG);
  logicalScalar_flag &= ~XY_CONFIG_SWAP_XY;
  API_C3_writeRegister(REG_XY_CONFIG, logicalScalar_flag);
  return API_C3_checkRegister(REG_XY_CONFIG, logicalScalar_flag);
}

bool API_C3_enableLogicalScaling()
{
  uint8_t logicalScalar_flag = API_C3_readRegister(REG_XY_CONFIG);
  logicalScalar_flag &= ~XY_CONFIG_DISABLE_SCALING;
  API_C3_writeRegister(REG_XY_CONFIG, logicalScalar_flag);
  return API_C3_checkRegister(REG_XY_CONFIG, logicalScalar_flag);
}

bool API_C3_disableLogicalScaling()
{
  uint8_t logicalScalar_flag = API_C3_readRegister(REG_XY_CONFIG);
  logicalScalar_flag |= XY_CONFIG_DISABLE_SCALING;
  API_C3_writeRegister(REG_XY_CONFIG, logicalScalar_flag);
  return API_C3_checkRegister(REG_XY_CONFIG, logicalScalar_flag);
}

bool API_C3_PowerManagement_isImplemented(void)
{
  return isValidHeader(REG_POWER_HEADER);
}

bool API_C3_setPowerSetting(uint8_t registerValue)
{
  //  Register Bits:
  //  PowerSetting_DisableDeepSleep 0x01
  //  PowerSetting_DisableIdleSleep 0x02
  //  PowerSetting_DontWakeFromTouch 0x04
  //  PowerSetting_DontWakeFromButton 0x08
  API_C3_writeRegister(REG_POWER_SETTINGS, registerValue);
  return API_C3_checkRegister(REG_POWER_SETTINGS, registerValue);
}

bool API_C3_setPowerCommand(uint8_t registerValue)
{
  //  Register Bits:
  //  PowerCmd_ForceSleep 0x01
  //  PowerCmd_CancelForceSleep 0x02
  API_C3_writeRegister(REG_POWER_CMD, registerValue);
  return API_C3_checkRegister(REG_POWER_CMD, registerValue); //TODO: SHOULD CHECK BIT CLEAR INSTEAD?
}

/**
 *
 * @param timeBeforeIdle_100ms given in 100 ms
 */
bool API_C3_setTimeBeforeIdle(uint16_t timeBeforeIdle_100ms)
{
  API_C3_writeRegister16(REG16_POWER_IDLE_SLEEP_WAIT,timeBeforeIdle_100ms);
  return API_C3_checkRegister16(REG16_POWER_IDLE_SLEEP_WAIT,timeBeforeIdle_100ms);
}
/**
 *
 * @param idleTime given in 100us increments
 */
bool API_C3_setTimeInIdle(uint16_t idleTime)
{
  API_C3_writeRegister16(REG16_POWER_IDLE_SLEEP_TIME,idleTime);
  return API_C3_checkRegister16(REG16_POWER_IDLE_SLEEP_TIME,idleTime);
}

//given in 100 ms
bool API_C3_setTimeBeforeSleep(uint16_t timeBeforeSleep_100ms)
{
  API_C3_writeRegister16(REG16_POWER_DEEP_SLEEP_WAIT,timeBeforeSleep_100ms);
  return API_C3_checkRegister16(REG16_POWER_DEEP_SLEEP_WAIT,timeBeforeSleep_100ms);
}

//given in 100us increments
bool API_C3_setTimeInSleep(uint16_t sleepTime)
{
  API_C3_writeRegister16(REG16_POWER_DEEP_SLEEP_TIME, sleepTime);
  return API_C3_checkRegister16(REG16_POWER_DEEP_SLEEP_TIME, sleepTime);
}

bool API_C3_setIdleDisable(bool idleDisable)
{
  uint32_t settingsAddress = REG_POWER_SETTINGS;
  uint8_t settingsVal = API_C3_readRegister(settingsAddress);
  uint8_t idleMask = POWER_SETTING_DISABLE_IDLE_SLEEP;
  if(idleDisable)
  {
    settingsVal |= idleMask;
  }
  else
  {
    settingsVal &= ~idleMask;
  }
  
  API_C3_writeRegister(settingsAddress, settingsVal);
  return API_C3_checkRegister(settingsAddress, settingsVal);
}

bool API_C3_readIdleDisable()
{
  uint32_t settingsAddress = REG_POWER_SETTINGS;
  uint8_t settingsVal = API_C3_readRegister(settingsAddress);
  uint8_t idleMask = POWER_SETTING_DISABLE_IDLE_SLEEP;
  return (settingsVal & idleMask) == idleMask;
}

bool API_C3_setSleepDisable(bool sleepDisable)
{
  uint32_t settingsAddress = REG_POWER_SETTINGS;
  uint8_t settingsVal = API_C3_readRegister(settingsAddress);
  uint8_t sleepDisableMask = POWER_SETTING_DISABLE_DEEP_SLEEP;
  if(sleepDisable)
  {
    settingsVal |= sleepDisableMask;
  }
  else
  {
    settingsVal &= ~sleepDisableMask;
  }
  
  API_C3_writeRegister(settingsAddress, settingsVal);
  return API_C3_checkRegister(settingsAddress, settingsVal);
}

bool API_C3_readSleepDisable()
{
  uint32_t settingsAddress = REG_POWER_SETTINGS;
  uint8_t settingsVal = API_C3_readRegister(settingsAddress);
  uint8_t sleepDisableMask = POWER_SETTING_DISABLE_DEEP_SLEEP;
  return (settingsVal & sleepDisableMask) == sleepDisableMask;
}

bool API_C3_ProjectSpecific_isImplemented(void)
{
  return isValidHeader(REG_PROJECT_SPECIFIC_HEADER);
}

static bool API_C3_SendConfigCommand(uint8_t cmd, uint32_t msDelayBeforeCheck)
{
    uint8_t cmdStatus;
    
    API_C3_writeRegister(REG_SYS_COMMAND, cmd);
    delay(msDelayBeforeCheck);
    
    for(int i = 0; i < 100; ++i)
    {
        cmdStatus = API_C3_readRegister(REG_SYS_COMMAND);
        if( (cmdStatus & cmd) == 0 )
        {
            break;
        }
    }
    
    if( (cmdStatus & SYS_CMD_ERROR) || // Error during operation
        (cmdStatus & cmd)    // Command didn't finish
        )
    {
        return false;
    }
    else
    {
        return true;
    } 
}

bool API_C3_saveConfig(void)
{
    return API_C3_SendConfigCommand(SYS_CMD_SAVE_CONFIG, 55);
}

bool API_C3_restoreSavedConfig(void)
{
    return API_C3_SendConfigCommand(SYS_CMD_RESTORE_SAVED_CONFIG, 10);
}

bool API_C3_restoreFactoryConfig(void)
{
    return API_C3_SendConfigCommand(SYS_CMD_RESTORE_FACTORY_CONFIG, 10);
}

void API_C3_sensorSize(uint8_t * sizeX, uint8_t * sizeY, uint16_t * compByteLength)
{
  uint8_t lengthBytes[2];

  *sizeX = API_C3_readRegister(REG_SENSOR_X_COUNT);
  *sizeY = API_C3_readRegister(REG_SENSOR_Y_COUNT);

  HB_readExtendedMemory(EXTREG_COMP_MATRIX_LENGTH, lengthBytes, 2); // comp image length in bytes
  *compByteLength = HID_reportLength(lengthBytes);
}

void byteArrayToImage(uint8_t * byteArray, uint16_t byteLength, int16_t * imageArray)
{
  uint16_t indexByte, indexPixel;

  indexPixel = 0;
  for (indexByte = 0; indexByte < byteLength; indexByte += 2)
  {
    if (API_C3_endianState() == BIG_ENDIAN)
    {
      // high byte at low address
      imageArray[indexPixel++] = (byteArray[indexByte] << 8) | byteArray[indexByte + 1];
    }
    else
    {
      imageArray[indexPixel++] = byteArray[indexByte] | (byteArray[indexByte + 1] << 8);
    }
  }
}

void API_C3_readComp(int16_t * compMatrix, uint16_t compByteLength, uint16_t chunkSize)
{
  uint8_t rawData[compByteLength];

  API_C3_disableComp();

  API_C3_getMemoryContents(EXTREG_COMP_MATRIX_DATA, rawData, compByteLength, chunkSize);
  byteArrayToImage(rawData, compByteLength, compMatrix);

  API_C3_enableComp();
}

bool API_C3_isButtonPressed(HID_report_t* report, uint8_t buttonMask)
{
  return HID_isButtonPressed(report, buttonMask);
}
