#include "API_CustomMeas.h"
#include "API_ExtMemAccess.h"
#include "CustomMeasHardware.h"

#define NUM_MEAS_ADDR     (0x51000001)
#define FRAME_MS_ADDR     (0x51000002)

#define GLOBAL_INFO_ADDR  (0x51000000)

#define GROUP_INFO_ADDR   (0x51200000)
#define GROUP_INFO_INC    (0x00001000)

#define MEAS_INFO_ADDR    (0x51100000)
#define MEAS_INFO_INC     (0x00001000)

#define SYS_INFO_ADDR     (0x20000000)
#define RO_NON_PERSIST    (0x00000800)
#define POST_HEADER_ADDR  ((SYS_INFO_ADDR | RO_NON_PERSIST) + 8)

#define MAX_NUM_GROUPS  (5)
#define MAX_NUM_MEAS    (10)
#define MAX_NUM_RESULTS (16 * MAX_NUM_MEAS)

#define MEAS_HEADER_LENGTH  (5) // { HID_LengthLSB, HID_LengthMSB, CustomMeasReportID, ValidBytesLSB, ValidBytesMSB }
#define MAX_HID_LENGTH      (MEAS_HEADER_LENGTH + (MAX_NUM_RESULTS * 2))
#define HID_LENGTH_LSB      ((uint8_t)(MAX_HID_LENGTH >> 0))
#define HID_LENGTH_MSB      ((uint8_t)(MAX_HID_LENGTH >> 8))
#define HID_REPORT_ID       (0x0E)

#define XSTR(x) STR(x)
#define STR(x) #x

#define MAX_I2C_BYTES ((MAX_NUM_RESULTS * 2) + MEAS_HEADER_LENGTH)

static uint8_t _i2cAddr = 0x2C; // default to the standard Cirque address
static CustomMeasCallback *_callback;

/*  Private Functions */
// Reads measurement results into <resultsBuffer>
// Returns number of results read
static uint16_t ReadMeasResults(int16_t *resultsBuffer)
{
  uint16_t numResults;
  uint16_t i = 0;
  uint8_t bytes[5];

  // Read the first 5 bytes to get the length
  uint32_t i2cBytesReceived = I2C_request(_i2cAddr, MEAS_HEADER_LENGTH, true);

  if(i2cBytesReceived == 0) return 0; // probably timed out
  else if(i2cBytesReceived != MEAS_HEADER_LENGTH) return 0; // incorrect number of bytes
  else  // normal operation
  {
    for(; i < MEAS_HEADER_LENGTH; i++)
    {
      bytes[i] = I2C_read();
    }

    if(bytes[0] != HID_LENGTH_LSB || bytes[1] != HID_LENGTH_MSB || bytes[2] != HID_REPORT_ID) // not a valid packet
    {
      return 0; // bail
    }
  }

  uint16_t numValidBytes = bytes[3];
  numValidBytes |= ((uint16_t)bytes[4]) << 8;

  numResults = numValidBytes / 2;

  // wait for DR to re-assert
  uint32_t timestamp = HW_micros();
  while(HW_DR_asserted() == false && HW_micros() - timestamp < 500);

  if(numValidBytes <= MAX_NUM_RESULTS * 2)
  {
    i2cBytesReceived = I2C_request(_i2cAddr, numValidBytes + MEAS_HEADER_LENGTH, true);  // TODO: check returned number of bytes

    // Read first 5 bytes again
    I2C_read();
    I2C_read();
    I2C_read();
    I2C_read();
    I2C_read();

    I2C_readBytes((uint8_t*)resultsBuffer, numValidBytes);

    return numResults;
  }
  else
  {
    return 0;
  }
}

/****  Public API Functions  ****/
bool API_CustomMeas_Init(uint8_t i2cAddr, uint32_t i2cBitRate, CustomMeasCallback *callback)
{
  _i2cAddr = i2cAddr;
  _callback = callback;
  
  HW_init();
  I2C_init(i2cBitRate);
  return I2C_setBufferLength(MAX_I2C_BYTES);
}

void API_CustomMeas_Process()
{
  int16_t results[MAX_NUM_RESULTS];
  uint16_t numResults;

  if(HW_DR_asserted())
  {
    numResults = ReadMeasResults(results);

    if(_callback != 0 && numResults <= MAX_NUM_RESULTS && numResults != 0) _callback(results, numResults);
  }
}

void API_CustomMeas_SetFrameMillis(uint16_t millis)
{
  CustomMeasGlobalInfo_t temp;

  API_CustomMeas_ReadGlobalInfo(&temp);

  temp.FrameMillisLSB = (uint8_t) millis;
  temp.FrameMillisMSB = (uint8_t)(millis >> 8);

  API_CustomMeas_WriteGlobalInfo(&temp);
}

void API_CustomMeas_StartMeas()
{
  CustomMeasGlobalInfo_t temp;

  API_CustomMeas_ReadGlobalInfo(&temp);

  temp.Enable = 1;

  API_CustomMeas_WriteGlobalInfo(&temp);
}

void API_CustomMeas_StopMeas()
{
  CustomMeasGlobalInfo_t temp;

  API_CustomMeas_ReadGlobalInfo(&temp);

  temp.Enable = 0;

  API_CustomMeas_WriteGlobalInfo(&temp);
}

bool API_CustomMeas_Calibrate(uint8_t groupIndex)
{
  if(groupIndex > MAX_NUM_GROUPS && groupIndex != 255) return false;

  CustomMeasGroupInfo_t temp;

  if(groupIndex < MAX_NUM_GROUPS)
  {
    API_CustomMeas_ReadGroupInfo(groupIndex, &temp);

    temp.Calibration |= 0x40; // Set bit-6

    API_CustomMeas_WriteGroupInfo(groupIndex, &temp);
  }
  else
  {
    uint8_t i = 0;
    for(; i < MAX_NUM_GROUPS; i++)
    {
      API_CustomMeas_ReadGroupInfo(i, &temp);

      temp.Calibration |= 0x40; // Set bit-6

      API_CustomMeas_WriteGroupInfo(i, &temp);
    }
  }

  return true;
}

bool API_CustomMeas_EnableCalibration(uint8_t groupIndex)
{
  if(groupIndex > MAX_NUM_GROUPS && groupIndex != 255) return false;

  CustomMeasGroupInfo_t temp;

  if(groupIndex < MAX_NUM_GROUPS)
  {
    API_CustomMeas_ReadGroupInfo(groupIndex, &temp);

    temp.Calibration |= 0x80; // Set bit-7

    API_CustomMeas_WriteGroupInfo(groupIndex, &temp);
  }
  else
  {
    uint8_t i = 0;
    for(; i < MAX_NUM_GROUPS; i++)
    {
      API_CustomMeas_ReadGroupInfo(i, &temp);

      temp.Calibration |= 0x80; // Set bit-7

      API_CustomMeas_WriteGroupInfo(i, &temp);
    }
  }

  return true;
}

bool API_CustomMeas_DisableCalibration(uint8_t groupIndex)
{
  if(groupIndex > MAX_NUM_GROUPS && groupIndex != 255) return false;

  CustomMeasGroupInfo_t temp;

  if(groupIndex < MAX_NUM_GROUPS)
  {
    API_CustomMeas_ReadGroupInfo(groupIndex, &temp);

    temp.Calibration &= ~0x80; // Clear bit-7

    API_CustomMeas_WriteGroupInfo(groupIndex, &temp);
  }
  else
  {
    uint8_t i = 0;
    for(; i < MAX_NUM_GROUPS; i++)
    {
      API_CustomMeas_ReadGroupInfo(i, &temp);

      temp.Calibration &= ~0x80; // Clear bit-7

      API_CustomMeas_WriteGroupInfo(i, &temp);
    }
  }

  return true;
}

bool API_CustomMeas_Persist()
{
  CustomMeasGlobalInfo_t temp;

  API_CustomMeas_ReadGlobalInfo(&temp);

  temp.Persist = 1;

  API_CustomMeas_WriteGlobalInfo(&temp);

  API_CustomMeas_ReadGlobalInfo(&temp);

  return temp.Persist == 0x00;
}

void API_CustomMeas_Restore()
{
  CustomMeasGlobalInfo_t temp;

  API_CustomMeas_ReadGlobalInfo(&temp);

  temp.Restore = 1;

  API_CustomMeas_WriteGlobalInfo(&temp);
}

void API_CustomMeas_ReadGlobalInfo(CustomMeasGlobalInfo_t * config)
{
  API_ExtMemAccess_ReadMemory(GLOBAL_INFO_ADDR, (uint8_t*)config, GLOBAL_INFO_SIZE);
}

void API_CustomMeas_WriteGlobalInfo(CustomMeasGlobalInfo_t * config)
{
  API_ExtMemAccess_WriteMemory(GLOBAL_INFO_ADDR, (uint8_t*)config, GLOBAL_INFO_SIZE);
}

void API_CustomMeas_ReadGroupInfo(uint8_t index, CustomMeasGroupInfo_t * config)
{
  uint32_t address = GROUP_INFO_ADDR + (index * GROUP_INFO_INC);

  API_ExtMemAccess_ReadMemory(address, (uint8_t*)config, GROUP_INFO_SIZE);
}

void API_CustomMeas_WriteGroupInfo(uint8_t index, CustomMeasGroupInfo_t * config)
{
  uint32_t address = GROUP_INFO_ADDR + (index * GROUP_INFO_INC);

  API_ExtMemAccess_WriteMemory(address, (uint8_t*)config, GROUP_INFO_SIZE);
}

void API_CustomMeas_ReadMeasInfo(uint8_t index, CustomMeasInfo_t * config)
{
  uint32_t address = MEAS_INFO_ADDR + (index * MEAS_INFO_INC);

  API_ExtMemAccess_ReadMemory(address, (uint8_t*)config, MEAS_INFO_SIZE);
}

void API_CustomMeas_WriteMeasInfo(uint8_t index, CustomMeasInfo_t * config)
{
  uint32_t address = MEAS_INFO_ADDR + (index * MEAS_INFO_INC);
  
  API_ExtMemAccess_WriteMemory(address, (uint8_t*)config, MEAS_INFO_SIZE);
}

void API_CustomMeas_ReadVersionInfo(SystemInfo_t * config)
{
  API_ExtMemAccess_ReadMemory(POST_HEADER_ADDR, (uint8_t*)config, SYSTEM_INFO_SIZE);
}

#define SVN_WORKING_COPY_IS_DIRTY (0x80000000)
#define SVN_BRANCH                (0x40000000)

// Converts the firmware rev bytes into a more useful struct
void API_CustomMeas_FirmwareRevBytes_To_FirmwareRevInfo(uint8_t * bytes, firmwareRevInfo_t * firmwareRevInfo)
{
  uint32_t temp = (bytes[3] << 24) | (bytes[2] << 16) | (bytes[1] << 8) | (bytes[0] << 0); 

  firmwareRevInfo->RevisionRegister = temp;
  firmwareRevInfo->SvnRevision = temp & ~(SVN_WORKING_COPY_IS_DIRTY | SVN_BRANCH);
  firmwareRevInfo->IsDirty = (temp & SVN_WORKING_COPY_IS_DIRTY) == 0 ? false : true;
  firmwareRevInfo->IsBranch = (temp & SVN_BRANCH) == 0 ? false : true;
}
