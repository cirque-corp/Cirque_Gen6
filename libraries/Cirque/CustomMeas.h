#ifndef CUSTOM_MEAS_H
#define CUSTOM_MEAS_H

// Copyright (c) 2025 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include <stdint.h>
#include "I2cHidApi.h"
#include "CustomMeasSystemInfo.h"

#define MAX_NUMBER_GROUPS (5)  // 0..3 are measurement groups, 4 is for noise measurements
#define MAX_NUMBER_MEASUREMENTS (20)
#define MAX_READINGS_PER_MEASUREMENT (16)

class CustomMeas : public I2cHidApi
{
public:
	CustomMeas(uint16_t maxBufferLength);
	~CustomMeas() override;

#pragma pack(push, 1)
	typedef struct {
		uint8_t Enable;
		uint8_t FrameMillisLSB;
		uint8_t FrameMillisMSB;
		uint8_t Persist;
		uint8_t Restore;
		uint8_t POR_Enable;
		uint8_t LowPowerMode;
	} GlobalInfo_t;

	typedef struct {
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
	} GroupInfo_t;

	typedef struct {
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
	} MeasInfo_t;

	typedef struct {
		uint8_t altFreqIndex;
		uint16_t noiseChangeThreshold;
		uint16_t maxAdjustPerFrame;
		uint8_t altOffsetAdjFilterWeight;
		uint8_t cleanCleanCountThreshold;
	} NoiseConfig_t;
#pragma pack(pop)

	CustomMeas::commandErrors ReadSystemInfo(SystemInfo & systemInfo);

	CustomMeas::commandErrors ReadGlobalInfo(GlobalInfo_t * globalInfo);
	void WriteGlobalInfo(const GlobalInfo_t * globalInfo);
	CustomMeas::commandErrors SetFrameMillis(uint16_t millis);

	enum EnableState { MeasurementsActive, MeasurementsOff };
	CustomMeas::commandErrors StartMeas(void);
	CustomMeas::commandErrors StopMeas(void);

	CustomMeas::commandErrors ReadGroupInfo(uint8_t groupIndex, GroupInfo_t * groupInfo);
	CustomMeas::commandErrors WriteGroupInfo(uint8_t groupIndex, GroupInfo_t * groupInfo);
	CustomMeas::commandErrors Calibrate(uint8_t groupIndex);
	CustomMeas::commandErrors CalibrateAll(void);
	CustomMeas::commandErrors EnableCalibration(uint8_t groupIndex);
	CustomMeas::commandErrors EnableAllCalibration(void);
	CustomMeas::commandErrors DisableCalibration(uint8_t groupIndex);
	CustomMeas::commandErrors DisableAllCalibration(void);

	CustomMeas::commandErrors Persist(void);
	CustomMeas::commandErrors Restore(void);
	CustomMeas::commandErrors POR_StartMeas(void);
	CustomMeas::commandErrors POR_StopMeas(void);
	CustomMeas::commandErrors LowPowerMode(void);
	CustomMeas::commandErrors NormalPowerMode(void);

	CustomMeas::commandErrors ReadMeasInfo(uint8_t index, MeasInfo_t *config);
	CustomMeas::commandErrors WriteMeasInfo(uint8_t index, const MeasInfo_t *config);

	CustomMeas::commandErrors ReadNoiseConfig(uint8_t index, NoiseConfig_t *config);
	CustomMeas::commandErrors WriteNoiseConfig(uint8_t index, const NoiseConfig_t *config);

	reportIds_t getMeasReport(int16_t * measArray, uint16_t &measCount);

};

#endif // CUSTOM_MEAS_H