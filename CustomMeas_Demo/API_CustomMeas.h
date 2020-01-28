#ifdef __cplusplus
extern "C" {
#endif

#ifndef _API_CustomMeas_H_
#define _API_CustomMeas_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
  uint8_t Enable;
  uint8_t FrameMillisLSB;
  uint8_t FrameMillisMSB;
  uint8_t Persist;
  uint8_t Restore;
  uint8_t POR_Enable;
  uint8_t LowPowerMode;
} CustomMeasGlobalInfo_t;
#define GLOBAL_INFO_SIZE (sizeof(CustomMeasGlobalInfo_t))

typedef struct
{
  uint8_t Mode;
  uint8_t Calibration;
  uint8_t FrameBetweenCompsLSB;
  uint8_t FrameBetweenCompsMSB;
  uint8_t NegativeThresholdLSB;
  uint8_t NegativeThresholdMSB;
  uint8_t SpeedThresholdLSB;
  uint8_t SpeedThresholdMSB;
  uint8_t ActivityThresholdLSB;
  uint8_t ActivityThresholdMSB;
  uint8_t ActivityTimeoutLSB;
  uint8_t ActivityTimeoutMSB;
} CustomMeasGroupInfo_t;
#define GROUP_INFO_SIZE (sizeof(CustomMeasGroupInfo_t))

typedef struct
{
  uint8_t Control;
  uint8_t ElectrodeStates[24];
  uint8_t Gain;
  uint8_t GlobalOffset;
  uint8_t ChannelOffsetMultiplier;
  uint8_t ChannelOffsets[8];
  uint8_t ToggleFrequency;
  uint8_t ApertureLength;
  uint8_t Waveform;
  uint8_t ADCChannelMaskLSB;
  uint8_t ADCChannelMaskMSB;
} CustomMeasInfo_t;
#define MEAS_INFO_SIZE (sizeof(CustomMeasInfo_t))

typedef void CustomMeasCallback(int16_t* pResultsArray, uint16_t numResults);

bool API_CustomMeas_Init(uint8_t i2cAddr, uint32_t i2cBitRate, CustomMeasCallback *callback);
void API_CustomMeas_Process();
void API_CustomMeas_SetFrameMillis(uint16_t millis);
void API_CustomMeas_StartMeas();
void API_CustomMeas_StopMeas();
bool API_CustomMeas_Calibrate(uint8_t groupIndex);
bool API_CustomMeas_EnableCalibration(uint8_t groupIndex);
bool API_CustomMeas_DisableCalibration(uint8_t groupIndex);
bool API_CustomMeas_Persist();
void API_CustomMeas_Restore();
void API_CustomMeas_ReadGlobalInfo(CustomMeasGlobalInfo_t * config);
void API_CustomMeas_WriteGlobalInfo(CustomMeasGlobalInfo_t * config);
void API_CustomMeas_ReadGroupInfo(uint8_t index, CustomMeasGroupInfo_t * config);
void API_CustomMeas_WriteGroupInfo(uint8_t index, CustomMeasGroupInfo_t * config);
void API_CustomMeas_ReadMeasInfo(uint8_t index, CustomMeasInfo_t * config);
void API_CustomMeas_WriteMeasInfo(uint8_t index, CustomMeasInfo_t * config);

#endif  // _API_CustomMeas_H_

#ifdef __cplusplus
}
#endif
