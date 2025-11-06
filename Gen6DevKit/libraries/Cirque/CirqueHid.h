#ifndef CIRQUE_HID_H
#define CIRQUE_HID_H

// Copyright (c) 2025 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include "I2cHidApi.h"

class CirqueHid : public I2cHidApi
{
public:
    CirqueHid(uint8_t i2cAddress, uint16_t maxBufferLength);
    ~CirqueHid() override;

    enum PTP_ButtonImplementation  // Microsoft standard
    {
        PTP_ClickPad = 0,       // Depressible
        PTP_PressurePad = 1,    // Non-Depressible
        PTP_DiscretePad = 2,    // Non-Clickable
    };

    // Cirque-specific HID stuff
    void getDeviceCapabilities(uint8_t &NumberContacts, PTP_ButtonImplementation &ButtonImplementation);
    void getCertificationStatus(uint8_t * blob, uint16_t blobLength);
    void setInputMode(bool setAbsolute);
    void setSelectiveReporting(bool enableContactReports, bool enableButtonReports);

	// suggested functions, that need to be pulled over from the prior code
	//uint8_t readCirqueRegister(uint32_t address);
	//uint8_t endianState(void);
	//uint16_t readCirqueRegister16(uint32_t address);
	//uint32_t readCirqueRegister32(uint32_t address);
	//void writeCirqueRegister(uint32_t address, uint8_t value);
	//void writeCirqueRegister16(uint32_t address, uint16_t contents);
	//void writeCirqueRegister32(uint32_t address, uint32_t contents);
	//void readSystemInfo(systemInfo_t* result);
	//uint8_t getMemoryContents(uint32_t address, uint8_t * buffer, uint16_t bufferLength, uint16_t chunkSize);

    // what about doing fw update?


    // Cirque C3 Api - these functions can be renamed and pulled over from the prior code

    // uint8_t API_C3_readRegister(uint32_t address);
    // uint8_t API_C3_endianState(void);
    // uint16_t API_C3_readRegister16(uint32_t address);
    // uint32_t API_C3_readRegister32(uint32_t address);
    // void API_C3_writeRegister(uint32_t address, uint8_t value);
    // void API_C3_writeRegister16(uint32_t address, uint16_t contents);
    // void API_C3_writeRegister32(uint32_t address, uint32_t contents);

    // void API_C3_readSystemInfo(systemInfo_t* result);
    // uint8_t API_C3_getMemoryContents(uint32_t address, uint8_t * buffer, uint16_t bufferLength, uint16_t chunkSize);
    // void API_C3_setPtpMode();
    // void API_C3_setCRQ_AbsoluteMode(void);
    // void API_C3_setRelativeMode(void);
    // void API_C3_forceComp(void);
    // void API_C3_disableComp(void);
    // void API_C3_disableFeed(void);
    // void API_C3_enableFeed(void);
    // void API_C3_disableTracking(void);
    // void API_C3_enableTracking(void);
    // void API_C3_enableScaling(void);
    // void API_C3_disableScaling(void);
    // void API_C3_enableLinearCorrection(void);
    // void API_C3_disableLinearCorrection(void);
    // // Common channel numbers in QMVChannelNumbers enum
    // void API_C3_SetQMVStream(uint8_t channelNumber, QMVChannelState state);
    // // From: CoreFW/Includes/QuickMeasViewDataControl.h
    // //QmvId_Image_PreDemux, // 0
    // //QmvId_Image_PostDemux, // 1
    // //QmvId_Image_PostComp, //2
    // //QmvId_Image_Comp,  //3
    // // ...
    // 	//QmvId_IdleDetect,  // 16
    // //QmvId_IdleDetect_Accumulation,  // 17
    // //QmvId_IdleDetect_MeasData, // 18

    // bool API_C3_XYConfig_isImplemented();
    // void API_C3_setInvertX(void);
    // void API_C3_clearInvertX(void);
    // void API_C3_setInvertY(void);
    // void API_C3_clearInvertY(void);
    // void API_C3_setSwapXY(void);
    // void API_C3_clearSwapXY(void);
    // void API_C3_enableLogicalScaling(void);
    // void API_C3_disableLogicalScaling(void);
    // void API_C3_enableComp(void);
    // void API_C3_disableComp(void);
    // bool API_C3_PowerManagement_isImplemented(void);
    // void API_C3_setPowerSetting(uint8_t registerValue);
    // void API_C3_setPowerCommand(uint8_t registerValue);
    // void API_C3_setTimeBeforeIdle(uint16_t timeBeforeIdle_100ms); //given in 100 ms
    // void API_C3_setTimeInIdle(uint16_t idleTime);
    // void API_C3_setTimeBeforeSleep(uint16_t timeBeforeSleep_100ms); //given in 100 ms
    // void API_C3_setTimeInSleep(uint16_t sleepTime); //given in 100us increments
    // void API_C3_setIdleDisable(bool idleDisable);
    // bool API_C3_readIdleDisable();
    // void API_C3_setSleepDisable(bool sleepDisable);
    // bool API_C3_readSleepDisable();
    // bool API_C3_ProjectSpecific_isImplemented(void);
    // bool API_C3_saveConfig(void);
    // bool API_C3_restoreSavedConfig(void);
    // bool API_C3_restoreFactoryConfig(void);
    // void API_C3_sensorSize(uint8_t * sizeX, uint8_t * sizeY, uint16_t * compByteLength);
    // void API_C3_readComp(int16_t * compMatrix, uint16_t compByteLength, uint16_t chunkSize);

    // // for now these are a pass-through to the HID_reports code, they only work for CRQ_Absolute mode 
    // bool API_C3_isFingerValid(HID_report_t* report, uint8_t finger_num);
    // bool API_C3_isFingerContacted(HID_report_t* report, uint8_t finger_num);
    // uint8_t API_C3_numberFingers(HID_report_t* report);
    // bool API_C3_isButtonPressed(HID_report_t* report, uint8_t buttonMask);
};

//  Hid.c
//  RequestHandler:
//      RT_Output, if reportId has handler --> call all output handlers
//      RT_Feature, if reportId has handler --> call all feature handlers (HidInfo.c)
//          ExtendedAccess
//          ExtendedAccessRaw
//          PTP_DeviceCapabilities -- OC_GetReport, hidData = Device_Capability_Data = HID_PTP_FORMAT_DEV_CAP_ARRAY(5, PTP_Clickpad) = 05, 00, ID (2), MaxContacts (5), ButtonImplimentation (0)
//          PTP_CertificationStatus -- OC_GetReport, hidData = CertificationStatus_Data = blob of 259 bytes (length, id, data)
//          PTP_InputMode -- OC_SetReport, length, id, data=3 set Absolute, else set Relative
//          PTP_SelectiveReporting -- OC_SetReport, length, id, data.0 = 1 is enableContactReport, data.1 = 2 is enableButtonReport
//          FWUpdate
//          Optional stuff
//      else (no handlers exist)
//          OC_VendorDefined, 
//              RT_Feature, reportId has id_ReportDescriptor(21) --> return reportDescriptor 
//              RT_RESERVED, reportId 0, --> TPDisableHIDCallback() ??? no idea what this is
//          else 
//              CommandProcess
//  CommandProcess:
//      OC_Reset --> Cirque_ResetRequest()
//      OC_GetReport, 
//          RT_Input, HID_Rep_ID_Touchpad(1) || HID_Rep_ID_Mouse(6) --> reads last buffer
//          RT_Feature --> do nothing
//          RT_Output --> do nothing
//      OC_SetReport,
//          RT_Input --> do nothing
//          RT_Feature --> do nothing
//          RT_Output --> do nothing
//      OC_SetPower --> reportId bit0 sets power state, going off clears all pending traffic
//      OC_ everything else - not supported

// setFeatureReport(???, ???, 0x0003) == SetFeedTypeAbsolute_ActiveInterface(), otherwise SetFeedTypeRelative_ActiveInterface()


#endif // CIRQUE_HID_H
