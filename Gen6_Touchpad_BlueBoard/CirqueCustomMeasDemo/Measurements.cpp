// Copyright (c) 2025 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include "Measurements.h"

CustomMeas::GroupInfo_t GroupInfoArray[GROUPINFO_LENGTH] =
{
	1, // .Mode = Self_V2V
	0x81, // .Calibration = Background
	0xEE, // .FrameBetweenCompsLSB --> 750
	0x2, // .FrameBetweenCompsMSB
	0xB5, // .NegativeThresholdLSB --> -75
	0xFF, // .NegativeThresholdMSB
	0x2C, // .SpeedThresholdLSB = 300
	0x1,  // .SpeedThresholdMSB = 
	0x58, // .ActivityThresholdLSB = 600
	0x2, // .ActivityThresholdMSB = 
	0x96, // .ActivityTimeoutLSB = 150
	0x0, // .ActivityTimeoutMSB = 
};

CustomMeas::MeasInfo_t MeasInfoArray[MEASINFO_LENGTH] =
{
	{ // Measurement 0
		0x80, // .Control
		{
			0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
			0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
			0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
		}, // .ElectrodeStates
		3, // .Gain
		0x1F, // .GlobalOffset = 31
		0, // .ChannelOffsetMultiplier
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, }, // .ChannelOffsets
		19, // .ToggleFrequency
		39, // .ApertureLength
		0,  // .Waveform
		0xFF, // .ADCChannelMaskLSB
		0xFF, // .ADCChannelMaskMSB
	},
	{ // Measurement 1
		0x80, // .Control
		{
			0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
			0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77,
			0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
		}, // .ElectrodeStates
		3, // .Gain
		0x1F, // .GlobalOffset = 31
		0, // .ChannelOffsetMultiplier
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, }, // .ChannelOffsets
		19, // .ToggleFrequency
		39, // .ApertureLength
		0,  // .Waveform
		0xFF, // .ADCChannelMaskLSB
		0xFF, // .ADCChannelMaskMSB
	},
};