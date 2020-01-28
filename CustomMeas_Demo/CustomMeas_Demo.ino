#include "API_CustomMeas.h"
#include "MeasurementConfigs.h"
#define PWR_EN_PIN  1
#define I2C_BIT_RATE      (1400000) // 1.4MHz will result in ~1MHz due to some error in the i2c_t3.h library


void setup()
{
  pinMode(PWR_EN_PIN, OUTPUT);    //Reset Power to Olympus
  digitalWrite(PWR_EN_PIN, LOW);
  delay(100);
  digitalWrite(PWR_EN_PIN, HIGH);
  delay(100);

  Serial.begin(115200);

  API_CustomMeas_Init( 0x2C, I2C_BIT_RATE, MeasurementResultsCallback);

  PrintCommands();
}

void loop()
{
  API_CustomMeas_Process();

  if(Serial.available())
  {
    char rxChar = Serial.read();

    switch(rxChar)
    {
      case 'g':
        API_CustomMeas_StartMeas();
        Serial.println("Started...");
        break;
      case 's':
        API_CustomMeas_StopMeas();
        Serial.println("Stopped...");
        break;
      case 'b':
        CustomMeasGlobalInfo_t temp;
        API_CustomMeas_ReadGlobalInfo(&temp);
        Serial.println("GlobalInfo");
        PrintGlobalInfo(&temp);
        break;
      case 'c':
        if(Serial.available())
        {
          char ascii[2];
          ascii[0] = Serial.read();
          ascii[1] = 0; //null terminate string
          int groupIndex = atoi((char*)&ascii);
          API_CustomMeas_Calibrate(groupIndex);
          Serial.printf("Calibrated group %d!\n", groupIndex);
        }
        else
        {
          Serial.println("Error: Group number required!");
        }
        break;
      case 'n':
        for(int i =0; i < 5; i++) //TODO: NUM MAX_NUM_GROUPS
        {
          CustomMeasGroupInfo_t temp;
          API_CustomMeas_ReadGroupInfo(i, &temp);
          Serial.printf("Group Info [%d]\n", i);
          PrintGroupInfo(&temp);
        }
        break;
      case 'm':
        for(int i =0; i < 10; i++) //TODO: NUM MAX_NUM_GROUPS
        {
          CustomMeasInfo_t temp;
          API_CustomMeas_ReadMeasInfo(i, &temp);
          Serial.printf("Measurement Config Info [%d]\n", i);
          PrintMeasurementInfo(&temp);
        }
        break;
      case 'l':
        PrintCommands();
        break;
      case 'p':
        API_CustomMeas_Persist();
        Serial.println("CustomMeas Configs saved to flash");
        break;
      case 'r':
        API_CustomMeas_Restore();
        Serial.println("CustomMeas Configs restored from flash");
        break;
      case 'i':
        WriteAllInfo();
        Serial.println("All Global, Group, and Measurement info written");
        break;
      case '\r':
        break;  // ignore carriage-return
      case '\n':
        break;  // ignore line-feed
      default:
        Serial.println("Invalid Command!");
        PrintCommands();
        break;
    }
  }
}

static void MeasurementResultsCallback(int16_t* pResultsArray, uint16_t numResults)
{
  Serial.printf("Results[%d]:[", numResults);
  for(int i = 0; i < numResults; i++)
  {
    Serial.printf("\t%d,", pResultsArray[i]);
  }
  Serial.println("]");
}

static void PrintCommands()
{
  Serial.println("Supported Commands:");
  Serial.println("'g' - start measurements");
  Serial.println("'s' - stop measurements");
  Serial.println("'c<group_index>' - calibrate measurements for a group. eg.'c0' Values greater than 4 calibrates all groups");

  Serial.println("'b' - read global info");
  Serial.println("'n' - read all group info");
  Serial.println("'m' - read all measurement info");
  Serial.println("'i' - write all global, group, and measurement info");

  Serial.println("'p' - persist measurement info to flash");
  Serial.println("'r' - restore measurement info from flash");
  Serial.println("'l' - list commands");
}

static void PrintGlobalInfo(CustomMeasGlobalInfo_t* pGlobalInfo)
{
  Serial.printf("\tEnable = %d\n\tFrameMillis = %d\n\tPOR_Enable = %d\n\tLowPowerMode = %d\n",
                pGlobalInfo->Enable,
                pGlobalInfo->FrameMillisLSB | pGlobalInfo->FrameMillisMSB << 8,
                pGlobalInfo->POR_Enable,
                pGlobalInfo->LowPowerMode);
}
static void PrintGroupInfo(CustomMeasGroupInfo_t* pGroupInfo)
{
  //TODO: Display detailed information about the bits
  Serial.printf("\tMode = %d\n\tCalibration = 0x%X\n",
                pGroupInfo->Mode,
                pGroupInfo->Calibration);
}

static void ConvertElectrodeStatesToAsciiString(uint8_t* electrodeStates, char* outAscii)
{
  int outArrayIndex = 0;
  for(int i = 0; i < 24; i++)
  {
    outAscii[outArrayIndex++] = (electrodeStates[i] & 0xF) + '0';
    outAscii[outArrayIndex++] = ((electrodeStates[i] >> 4) & 0xF) + '0';
  }
  outAscii[outArrayIndex] = 0; //null terminate
}

static int ConvertChannelOffsetToDecimal(uint8_t channelOffset)
{
  int sign = channelOffset >> 3;
  return channelOffset & 0x7 * sign;
}

static void ConvertChannelOffsetsToAsciiString(uint8_t* channelOffsets, char* outAscii)
{
  int outArrayIndex = 0;
  for(int i = 0; i < 8; i++)
  {
    int value1 = ConvertChannelOffsetToDecimal(channelOffsets[i] & 0xF);
    int value2 = ConvertChannelOffsetToDecimal((channelOffsets[i] >> 4) & 0xF);
    outArrayIndex += sprintf(outAscii + outArrayIndex, "%d, %d, ", value1, value2);
  }
}

static void PrintMeasurementInfo(CustomMeasInfo_t* pMeasInfo)
{
  char asciiElectrodeStatesString[50];
  char asciiChannelOffsetsString[50];
  ConvertElectrodeStatesToAsciiString(pMeasInfo->ElectrodeStates, asciiElectrodeStatesString);
  ConvertChannelOffsetsToAsciiString(pMeasInfo->ChannelOffsets, asciiChannelOffsetsString);
  Serial.printf("\tIsEnabled = %s\tGroupIndex = %d\n\tElectrodeStates = %s\n\tGain = %d\n\tGlobalOffset = %d\n\tChannelOffsetMult. = %d\n\tChannelOffsets = %s\n\tToggleFrequency = %d\n\tApertureLength = %d\n\tWaveform = %d\n\tADCChannelMask = 0x%X\n",
                  (pMeasInfo->Control >> 7)? "true": "false",
                  pMeasInfo->Control & 0x7F,
                  asciiElectrodeStatesString,
                  pMeasInfo->Gain,
                  ((pMeasInfo->GlobalOffset & 0x80)? -1 : 1) * (pMeasInfo->GlobalOffset & 0x7F) ,
                  pMeasInfo->ChannelOffsetMultiplier,
                  asciiChannelOffsetsString,
                  pMeasInfo->ToggleFrequency,
                  pMeasInfo->ApertureLength,
                  pMeasInfo->Waveform,
                  pMeasInfo->ADCChannelMaskLSB | pMeasInfo->ADCChannelMaskMSB << 8
                );
}

static void WriteAllInfo()
{
  API_CustomMeas_WriteGlobalInfo(&GlobalInfo);
  for(int groupIndex = 0; groupIndex < 5; groupIndex++)
  {
    API_CustomMeas_WriteGroupInfo(groupIndex, &(GroupInfoArray[groupIndex]));
  }
  for(int measIndex = 0; measIndex < 10; measIndex++)
  {
    API_CustomMeas_WriteMeasInfo(measIndex, &(MeasInfoArray[measIndex]));
  }
}
