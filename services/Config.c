//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \file       ConfigParameters.h
//! \brief      Configuration parameter listing file
//!
//! \author     Joel Minski jminski@puracom.com], Puracom Inc.
//! \date       2012-05-16
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
// Module Feature Switches
//////////////////////////////////////////////////////////////////////////////////////////////////

#define CONFIG_DATAFLASH_ENABLE (1)     // 1 = use dataflash, 0 = use backup SRAM

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Include Files
//////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <string.h>

//////////////////////////////////////////////////////////////////////////////////////////////////
// Project Include Files
//////////////////////////////////////////////////////////////////////////////////////////////////

// C Library includes
#include <string.h>
#include <stdio.h>

// STM Library includes
                                // NOTE: This file includes all peripheral.h files
#include "stm32f2xx_conf.h"     // Macro: assert_param() for other STM libraries.

// RTOS Library includes
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

// PCOM General Library includes
#include "Types.h"              // Timer depends on Types
#include "Target.h"

#include "Timer.h"              // task depends on Timer for vApplicationTickHook

// PCOM Project Targets

// PCOM Library includes
#include "General.h"
#include "DataFlash.h"      // Read and Write DataFlash functions
#include "DFFaddress.h"
#include "Backup.h"
#include "AdcExternal.h"
#include "Config.h"         // Configuration manager
#include "Control.h"         // Configuration manager


//////////////////////////////////////////////////////////////////////////////////////////////////
// Local Type Definitions
//////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum
{
    CONFIG_SEMAPHORE_ACQUIRED           = 0,    // Semaphore newly acquired
    CONFIG_SEMAPHORE_ALREADY_AQUIRED,           // Semaphore had already been acquired by the same thread
    CONFIG_SEMAPHORE_RELEASED,                  // Semaphore has just been released
    CONFIG_SEMAPHORE_NOT_RELEASED,              // Semaphore was not released because it had already been acquired by the same thread
    CONFIG_SEMAPHORE_SCHEDULER_NOT_RUNNING,     // Semaphore was not acquired because RTOS scheduler was not running
    CONFIG_SEMAPHORE_ERROR,                     // Error while releasing or acquiring semaphore

} ConfigSemaphoreResultEnum;

typedef union 
{
    INT32       i32;
    UINT32      u32;
    SINGLE      f32;
} ConfigParamValueUnion;

typedef struct
{
    UINT16                  wParamId;
    CHAR                    *pcParamName;               // If NULL, then no name allowed
    UINT8                   bParamType;                 // Allowed type list:  UINT8, UINT16, UINT32, INT8, INT16, INT32, STRING
    // NOTE: min, max, and default values should use "{.f32 = value}" notation if they are of type single precision floating point
    ConfigParamValueUnion   vtParamValueMin;            // variable type union, Min value the parameter can have, used for sanity checking user input
    ConfigParamValueUnion   vtParamValueMax;            // variable type union, Max value the parameter can have, used for sanity checking user input.  For strings, this is the maximum string length (not including the NULL).
    ConfigParamValueUnion   vtParamValueDefault;
    void                    *pvParamData;
    CHAR                    *pcParamDescription;

} ConfigParamInfoType;

typedef struct
{
    UINT16 				wCrc16;						// CRC16 to ensure data integrity
    UINT8				bSignatureByte;				// Signature to detect a valid DevInfo page
    UINT8 				bDevInfoVersion;			// Which version of the DevInfo page this is
} PACKED ConfigStorageMetaType;

typedef union
{    
    ConfigDevInfoType           tDevInfo;               // Dev Info structure
    ConfigSettingsType          tSettings;              // Settings structure
} PACKED ConfigStorageTypeUnion;

typedef{
	BOOL						fIsPrivate;
	ConfigStorageTypeUnion      uData;
} PACKED ConfigPrivacyWrapperType;

typedef struct
{
    ConfigStorageMetaType           tMeta;
    ConfigPrivacyWrapperType		tData;
} PACKED ConfigStorageType;

typedef struct
{    
    char            * const     pcName;
    void                        *pvDataStruct;
    UINT32                      dwDataStructSize;
    ConfigParamInfoType * const ptMetaStruct;                   // data can change, pointer is immutable
    UINT16                      wMetaStructNumOfElements;    
    UINT8                       bStorageMagicNumber;
    UINT8                       bStorageVersion;
    UINT32                      dwDataFlashStorageSubsector;
    UINT32                      dwDataFlashStorageAddress;
    UINT32                      dwDataFlashStoragePageNumber;
    UINT8                       bUpdateConfigCounter;
}ConfigAccessStorageType;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Configuration Definitions
//////////////////////////////////////////////////////////////////////////////////////////////////

// These sizes match the names in ConfigParameterTypeEnum;
static const UINT8 gbConfigParamTypeSizeArray[] =
{
    1,          // CONFIG_PARAM_TYPE_UINT8
    2,          // CONFIG_PARAM_TYPE_UINT16
    4,          // CONFIG_PARAM_TYPE_UINT32
    1,          // CONFIG_PARAM_TYPE_INT8
    2,          // CONFIG_PARAM_TYPE_INT16
    4,          // CONFIG_PARAM_TYPE_INT32
    1,          // CONFIG_PARAM_TYPE_BOOL
    1,          // CONFIG_PARAM_TYPE_STRING, String is a special case where the size denotes the maximum allowable length of a string
    1,          // CONFIG_PARAM_TYPE_CHAR
    // WARNING: min max values can be integers even if the Data is of type FLOAT!
    4,          // CONFIG_PARAM_TYPE_FLOAT
};

static const CHAR * gtConfigParamTypeNameArray[] =
{
    "UINT8",
    "UINT16",
    "UINT32",
    "INT8",
    "INT16",
    "INT32",
    "BOOL",
    "STRING",
    "CHAR",
    "FLOAT"
};

// data structure that goes to memory (dataflash)
static ConfigDevInfoType                gtDevInfo;               // Dev Info structure
static ConfigSettingsType               gtSettings;              // Settings structure
static ConfigAdcCalibrationType         gtAdcCalib;              // calibration structure
static ConfigCommunicationsType         gtComm;                  // communications structure
static ConfigModbusType                 gtModbus;                // Modbus structure
static ConfigFluidDepthType             gtFluidD;                // FluidD structure

// this struct array is used to display the descriptive information  about the struct data that goes to memory(dataflash)
static const ConfigParamInfoType 		gtConfigDevInfoMeta[] = 
{
    // Param Id,                                    Param Name,             Param Type,                 Param Min,      Param Max,                                                  Param Default,      Param Data,                                         Param Description
    // NOTE:  For strings, the max field contains the string length        
    // WARNING: min max values can be integers even if the Data is of type FLOAT!
    
    // Costumer Product Information
    { CONFIG_DI_CUSTOMER_COMPANY_NAME_STRING,       "CPNYNAME",             CONFIG_PARAM_TYPE_STRING,   {0},            {CONFIG_PARAM_STRING_COSTUMER_COMPANY_NAME_STRING},         {0},                &gtDevInfo.cCostumerCompanyName[0],                   "Company name"      },
    { CONFIG_DI_CUSTOMER_DEVICE_NAME_STRING,        "UNITNAME",             CONFIG_PARAM_TYPE_STRING,   {0},            {CONFIG_PARAM_STRING_COSTUMER_DEVICE_NAME_STRING},          {0},                &gtDevInfo.cCostumerDeviceName[0],                    "Unit name"         },

    // Internal Device Information    
    { CONFIG_DI_INTERNAL_DEVICE_NAME_STRING,        "DEVNAME",              CONFIG_PARAM_TYPE_STRING,   {0},            {CONFIG_PARAM_STRING_INTERNAL_DEVICE_NAME_STRING},          {0},                &gtDevInfo.cInternalDeviceName[0],                      "Device name"             },
    { CONFIG_DI_INTERNAL_MANUFACTURING_DATE_STRING, "MFGDATE",              CONFIG_PARAM_TYPE_STRING,   {0},            {CONFIG_PARAM_STRING_INTERNAL_MANUFACTURING_DATE_STRING},   {0},                &gtDevInfo.cInternalManufacturingDate[0],               "Manufacturing Date"      },

    // HARDWARE
    { CONFIG_DI_HARDWARE_MAIN_BOARD_VERSION_UINT32, "HWMNBRD",              CONFIG_PARAM_TYPE_STRING,   {0x0000},       {CONFIG_PARAM_STRING_DESCRIPTION_STRING},                   {0x0000},           &gtDevInfo.cHardwareDescriptionMainBoard[0],            "Hw Description Main Board"    },
    { CONFIG_DI_HARDWARE_INT_BOARD_VERSION_UINT32,  "HWINBRD",              CONFIG_PARAM_TYPE_STRING,   {0x0000},       {CONFIG_PARAM_STRING_DESCRIPTION_STRING},                   {0x0000},           &gtDevInfo.cHardwareDescriptionIntBoard[0],             "Hw Description Intf Board"    },
    { CONFIG_DI_HARDWARE_BATERY_DESCRIPTION_STRING, "HWBAT",                CONFIG_PARAM_TYPE_STRING,   {0},            {CONFIG_PARAM_STRING_DESCRIPTION_STRING},                   {0},                &gtDevInfo.cHardwareDescriptionBattery[0],              "Hw Description Batt Board"    },
    { CONFIG_DI_HARDWARE_CAMERA_1_DESCRIPTION_STRING,"HWCM1",               CONFIG_PARAM_TYPE_STRING,   {0},            {CONFIG_PARAM_STRING_DESCRIPTION_STRING},                   {0},                &gtDevInfo.cHardwareDescriptionCam1[0],                 "Hw Description Cam1 Board"    },
    { CONFIG_DI_HARDWARE_CAMERA_2_DESCRIPTION_STRING,"HWCM2",               CONFIG_PARAM_TYPE_STRING,   {0},            {CONFIG_PARAM_STRING_DESCRIPTION_STRING},                   {0},                &gtDevInfo.cHardwareDescriptionCam2[0],                 "Hw Description Cam2 Board"    },
    { CONFIG_DI_HARDWARE_ANTENNA_DESCRIPTION_STRING,"HWANT",                CONFIG_PARAM_TYPE_STRING,   {0},            {CONFIG_PARAM_STRING_DESCRIPTION_STRING},                   {0},                &gtDevInfo.cHardwareDescriptionAntenna[0],              "Hw Description Ant Board"    },

    // MODEM
    { CONFIG_DI_MODEM_IMEI_NUMBER_STRING,           "MODEIME",              CONFIG_PARAM_TYPE_STRING,   {0x0000},       {CONFIG_PARAM_STRING_LENGTH_MODEM_IMEI},                    {0x0000},           &gtDevInfo.cModemImei[0],                               "Modem IMEI number"       },    
    { CONFIG_DI_MODEM_SIM_ICCID_NUMBER_STRING,      "SIMICCID",             CONFIG_PARAM_TYPE_STRING,   {0x0000},       {CONFIG_PARAM_STRING_LENGTH_SIM_ICCID},                     {0x0000},           &gtDevInfo.cModemSimIccid[0],                           "SIM ICCID number"       }
};
#define CONFIG_DEV_INFO_NUM_OF_ELEMENTS     (sizeof(gtConfigDevInfoMeta)/sizeof(gtConfigDevInfoMeta[0]))

static const ConfigParamInfoType 		gtConfigSettingsMeta[] = 
{
    // Param Id,                                    Param Name,             Param Type,                 Param Min,      Param Max,                                                  Param Default,      Param Data,                                         Param Description
    // NOTE:  For strings, the max field contains the string length 
    // WARNING: min max values can be integers even if the Data is of type FLOAT!
        
    // System    
    { CONFIG_SETTING_MODE,                          "SYSMODE",              CONFIG_PARAM_TYPE_UINT8,    {0},       {255},                                          {0},             &gtSettings.bSysMode,                                   "system mode" },        

    { CONFIG_SETTING_FILE_PENDING_QUEUE_FLAG,           "FILEQFLAG",        CONFIG_PARAM_TYPE_BOOL,     {0},       {0x01},                                         {0},             &gtSettings.fFilePendingQueueFlag,                      "File Pending Queue Flag" },
    { CONFIG_SETTING_FILE_PENDING_QUEUE_FILE_TYPE,      "FILEQTYPE",        CONFIG_PARAM_TYPE_UINT8,    {0},       {255},                                          {0},             &gtSettings.bFilePendingQueueFileType,                  "File Pending Queue File Type" },
    { CONFIG_SETTING_FILE_PENDING_QUEUE_FILE_SUBSECTOR, "FILEQSS",          CONFIG_PARAM_TYPE_UINT32,   { .u32 = 0 },    { .u32 = 0xFFffFFff },          { .u32 = 0 },              &gtSettings.dwFilePendingQueueFileSubsector,            "File Pending Queue File SubSector" },

    { CONFIG_DI_FIRMWARE_CRC32_UINT32,              "FWCRC32",              CONFIG_PARAM_TYPE_UINT32,   { .u32 = 0 },    { .u32 = 0xFFffFFff },                      { .u32 = 0 },              &gtSettings.dwConfigFirmwareCrc32,           "Firmware Crc32"        },

    { CONFIG_SETTING_LED_BLINK_ENABLE,              "LEDEN",                CONFIG_PARAM_TYPE_BOOL,     {0x00},     {0x01},                                       {0x01},           &gtSettings.fBlinkingLedEnable,                         "Blinking Led ENABLED"       },
    { CONFIG_SETTING_DETECT_SERIAL_CONNECTED_ENABLE,"SERDETEN",             CONFIG_PARAM_TYPE_BOOL,     {0x00},     {0x01},                                       {0x01},           &gtSettings.fDetectSerialConnectedEnable,               "Detect Serial Connected ENABLED"       },
    { CONFIG_SETTING_ON_ALARM_TRANFER_IMG_ENABLE,   "ALRMIMGSRVR",          CONFIG_PARAM_TYPE_BOOL,     {0x00},     {0x01},                                       {0x01},           &gtSettings.fOnAlarmTranferImgEnable,                   "On alarm send img to server ENABLED"       },
    { CONFIG_SETTING_TRANSMIT_MIN_OFFSET_ENABLE,    "TRANSMINOFFEN",        CONFIG_PARAM_TYPE_BOOL,     {0x00},     {0x01},                                       {0x01},           &gtSettings.fTransmitMinuteOffsetEnable,                "Transmission Add minute offset ENABLED"       },
    { CONFIG_SETTING_MERGE_OTHER_DATA_TO_HOURLY,    "MERGEDATAFILE",        CONFIG_PARAM_TYPE_BOOL,     {0x00},     {0x01},                                       {0x01},           &gtSettings.fMergeOtherDataToHourlyFileEnable,          "Merge Other Data to Data File ENABLED"       },

    { CONFIG_SETTING_SYS_UTC_TIME_ZONE_OFFSET,      "SYSUTCTIMEZONE",       CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -15.0 },     { .f32 = 15.0 },             { .f32 = -7.0 },         &gtSettings.sgSysUtcTimeZoneOffset,                     "UTC Time Zone Offset Hours"       },
    { CONFIG_SETTING_IS_AUTO_TIME_ZONE_ENABLED,     "AUTOTIMEZONE",         CONFIG_PARAM_TYPE_BOOL,     {0x00},    {0x01},                                          {0x01},         &gtSettings.fIsAutomaticTimeZoneEnabled,                "Automatic Time Zone Enabled"     },

    { CONFIG_SETTING_BLUETOOTH_SYNC_DEV_TIME_OUT,   "BTSYNCDEVTIM",         CONFIG_PARAM_TYPE_UINT32,   { .u32 = 0x00000000 },  { .u32 = 0xFFffFFff },              { .u32 = 300000 },   &gtSettings.dwBluetoothSyncDevTimeOut_mSec,             "Bluetooth Sync Device Time out"        },

    { CONFIG_SETTING_NETWORK_AND_SYSTEM_TIME_DELTA_SECONDS,"SYSNTDELTATIM", CONFIG_PARAM_TYPE_UINT8,   {0x00},      {20},                                           {5},             &gtSettings.bSetSysTimeDeltaSeconds,                    "Max delta Sys time and Network Time" },    
    
    { CONFIG_SETTING_TRANSFER_MINUTE_FILES_ENABLED, "TRANSFMINEN",          CONFIG_PARAM_TYPE_BOOL,     {0x00},     {0x01},                                         {0x00},         &gtSettings.fTransferMinutelyFilesEnabled,              "Transfer Minutely Files ENABLED"       },
    { CONFIG_SETTING_TRANSFER_EVENT_FILES_ENABLED,  "TRANSFEVTEN",          CONFIG_PARAM_TYPE_BOOL,     {0x00},     {0x01},                                         {0x00},         &gtSettings.fTransferEventFilesEnabled,                 "Transfer Event Files ENABLED"       },
    { CONFIG_SETTING_TRANSFER_ALERT_FILES_ENABLED,  "TRANSFALRTEN",         CONFIG_PARAM_TYPE_BOOL,     {0x00},     {0x01},                                         {0x00},         &gtSettings.fTransferAlertFilesEnabled,                 "Transfer Alert Files ENABLED"       },
    { CONFIG_SETTING_TRANSFER_SIGNAL_ENABLED,       "TRANSFSIGEN",          CONFIG_PARAM_TYPE_BOOL,     {0x00},     {0x01},                                         {0x00},         &gtSettings.fTransferSignalStrengthEnabled,             "Transfer Signal Strength ENABLED"       },
    //{ CONFIG_SETTING_LOG_MINUTELY_DATA_ENABLED,     "LOGMINEN",             CONFIG_PARAM_TYPE_BOOL,     0x00,       0x01,                                           0x01,           &gtSettings.fLogMinutelyDataEnabled,                    "Log Minutely Data ENABLED"       },

    // ADC  
    { CONFIG_ADC_SENSOR_SAMPLING_PERIOD_MINUTES,    "SENSAMPER",            CONFIG_PARAM_TYPE_UINT8,    {0},        {100},                                          {0},            &gtSettings.bSysSensorSamplePeriodMins,                 "Chan Sampling Short Trigger Minutes(not used anymore)" },    
    
    { CONFIG_SENSOR_CORRECTION_ENABLED,             "SENCORREN",            CONFIG_PARAM_TYPE_UINT8,    {0x00},     {0x3F},                                         {0x00},         &gtSettings.bSensorCorrectionFactorsEnabledBitMask,     "Sensor Correction Factors ENABLED at Chan x(Bit Mask)"       },
    { CONFIG_ADC_CHAN_CORRECTION_ENABLED,           "CHCORREN",             CONFIG_PARAM_TYPE_UINT8,    {0x00},     {0x3F},                                         {0x00},         &gtSettings.bAdcChannelCorrectionFactorsEnabledBitMask, "Chan Correction Factors ENABLED Bit Mask"       },
    { CONFIG_ADC_CHAN_MEASURE_DELTA_ENABLED,        "CHDELEN",              CONFIG_PARAM_TYPE_UINT8,    {0x00},     {0x3F},                                         {0x00},         &gtSettings.bAdcChannelMeasureDeltaEnabledBitMask,      "Chan Measure Delta ENABLED Bit Mask"       },      
    { CONFIG_ADC_CHAN_ALARM_THRESHOLD_LOW_ENABLED,  "CHALRMLOWEN",          CONFIG_PARAM_TYPE_UINT8,    {0x00},     {0x3F},                                         {0x00},         &gtSettings.bAdcAlarmThresholdLowEnabledBitMask,        "Chan Alarm Threashold Low ENABLED Bit Mask"       },
    { CONFIG_ADC_CHAN_ALARM_THRESHOLD_HIGH_ENABLED, "CHALRMHIWEN",          CONFIG_PARAM_TYPE_UINT8,    {0x00},     {0x3F},                                         {0x00},         &gtSettings.bAdcAlarmThresholdHighEnabledBitMask,       "Chan Alarm Threashold High ENABLED Bit Mask"       },    

    { CONFIG_ADC_CHAN_SAMPLING_SHORT_ENABLED_BIT_MASK,      "CHEN",         CONFIG_PARAM_TYPE_UINT8,    {0x00},     {0x3F},                                         {0x00},         &gtSettings.bAdcSamplingShortEnabledBitMask,            "Chan Sampling Short ENABLED Bit Mask"   },
    { CONFIG_ADC_CHAN_SAMPLING_LONG_ENABLED_BIT_MASK,       "CHSMPLONEN",   CONFIG_PARAM_TYPE_UINT8,    {0x00},     {0x3F},                                         {0x00},         &gtSettings.bAdcSamplingLongEnabledBitMask,             "Chan Sampling Long ENABLED Bit Mask"    },
    
    { CONFIG_ADC_CHAN_SAMPLING_LONG_TRIGGER_MINUTES,        "CHSMPLONTRIGMIN",  CONFIG_PARAM_TYPE_UINT16,  {0x0000}, {1440},                                        {240},          &gtSettings.wAdcSamplingLongTriggerMinutes,             "Chan Sampling Long Trigger Minutes"    },
    { CONFIG_ADC_CHAN_SAMPLING_LONG_ADC_READINGS_PER_SMPL,  "CHSMPLONRPS",      CONFIG_PARAM_TYPE_UINT16,  {0x0000}, {500},                                         {10},           &gtSettings.wAdcSamplingLongAdcReadingsPerSample,       "Chan Sampling Long Adc Readings Per Sample"    },
    { CONFIG_ADC_CHAN_SAMPLING_LONG_SMPL_PERIOD_MILLISEC,   "CHSMPLONSAMPERMS", CONFIG_PARAM_TYPE_UINT16,  {0x0000}, {0xFFFF},                                      {250},          &gtSettings.wAdcSamplingLongSamplePeriodMillisecond,    "Chan Sampling Long Sample Period (mSec)"    },
    { CONFIG_ADC_CHAN_SAMPLING_LONG_RECORDING_TIME_SECONDS, "CHSMPLONRECTIME",  CONFIG_PARAM_TYPE_UINT16,  {0x0000}, {900},                                         {30},           &gtSettings.wAdcSamplingLongRecordingTimeSeconds,       "Chan Sampling Long Recording time (seconds)"    },    
    { CONFIG_ADC_CHAN_SAMPLING_LONG_INIT_MIN_OFFSET_DAY,    "CHSMPLONMINSTART", CONFIG_PARAM_TYPE_INT16,   {-1},     {1440},                                        {-1},           &gtSettings.iwAdcSamplingLongStartMinuteOffsetOfDay,    "Chan Sampling Long Start Min Offset Of Day"    },        

    { CONFIG_ADC_CHAN_1_MODE,                       "CH1MODE",              CONFIG_PARAM_TYPE_UINT8,   {0x00},     {ADC_POWER_SWITCH_MAX},                         {0x00},          &gtSettings.bAdcChannelMode[ADC_INPUT_1],               "Chan 1 power mode"       },
    { CONFIG_ADC_CHAN_2_MODE,                       "CH2MODE",              CONFIG_PARAM_TYPE_UINT8,   {0x00},     {ADC_POWER_SWITCH_MAX},                         {0x00},          &gtSettings.bAdcChannelMode[ADC_INPUT_2],               "Chan 2 power mode"       },
    { CONFIG_ADC_CHAN_3_MODE,                       "CH3MODE",              CONFIG_PARAM_TYPE_UINT8,   {0x00},     {ADC_POWER_SWITCH_MAX},                         {0x00},          &gtSettings.bAdcChannelMode[ADC_INPUT_3],               "Chan 3 power mode"       },
    { CONFIG_ADC_CHAN_4_MODE,                       "CH4MODE",              CONFIG_PARAM_TYPE_UINT8,   {0x00},     {ADC_POWER_SWITCH_MAX},                         {0x00},          &gtSettings.bAdcChannelMode[ADC_INPUT_4],               "Chan 4 power mode"       },
    { CONFIG_ADC_CHAN_5_MODE,                       "CH5MODE",              CONFIG_PARAM_TYPE_UINT8,   {0x00},     {ADC_POWER_SWITCH_MAX},                         {0x00},          &gtSettings.bAdcChannelMode[ADC_INPUT_5],               "Chan 5 power mode"       },
    { CONFIG_ADC_CHAN_6_MODE,                       "CH6MODE",              CONFIG_PARAM_TYPE_UINT8,   {0x00},     {ADC_POWER_SWITCH_MAX},                         {0x00},          &gtSettings.bAdcChannelMode[ADC_INPUT_6],               "Chan 6 power mode"       },

    { CONFIG_ADC_CHAN_1_ALARM_THRESHOLD_LOW,        "CH1THLOW",             CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },            { .f32 = 0.0 },             &gtSettings.sgAdcAlarmThresholdLow[ADC_INPUT_1],       "Chan 1 Alarm threshold low"       },
    { CONFIG_ADC_CHAN_2_ALARM_THRESHOLD_LOW,        "CH2THLOW",             CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },            { .f32 = 0.0 },             &gtSettings.sgAdcAlarmThresholdLow[ADC_INPUT_2],       "Chan 2 Alarm threshold low"       },
    { CONFIG_ADC_CHAN_3_ALARM_THRESHOLD_LOW,        "CH3THLOW",             CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },            { .f32 = 0.0 },             &gtSettings.sgAdcAlarmThresholdLow[ADC_INPUT_3],       "Chan 3 Alarm threshold low"       },
    { CONFIG_ADC_CHAN_4_ALARM_THRESHOLD_LOW,        "CH4THLOW",             CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },            { .f32 = 0.0 },             &gtSettings.sgAdcAlarmThresholdLow[ADC_INPUT_4],       "Chan 4 Alarm threshold low"       },
    { CONFIG_ADC_CHAN_5_ALARM_THRESHOLD_LOW,        "CH5THLOW",             CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },            { .f32 = 0.0 },             &gtSettings.sgAdcAlarmThresholdLow[ADC_INPUT_5],       "Chan 5 Alarm threshold low"       },
    { CONFIG_ADC_CHAN_6_ALARM_THRESHOLD_LOW,        "CH6THLOW",             CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },            { .f32 = 0.0 },             &gtSettings.sgAdcAlarmThresholdLow[ADC_INPUT_6],       "Chan 6 Alarm threshold low"       },
    
    { CONFIG_ADC_CHAN_1_ALARM_THRESHOLD_HIGH,       "CH1THHIGH",            CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },            { .f32 = 3000.0 },          &gtSettings.sgAdcAlarmThresholdHigh[ADC_INPUT_1],      "Chan 1 Alarm threshold High"       },
    { CONFIG_ADC_CHAN_2_ALARM_THRESHOLD_HIGH,       "CH2THHIGH",            CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },            { .f32 = 3000.0 },          &gtSettings.sgAdcAlarmThresholdHigh[ADC_INPUT_2],      "Chan 2 Alarm threshold High"       },
    { CONFIG_ADC_CHAN_3_ALARM_THRESHOLD_HIGH,       "CH3THHIGH",            CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },            { .f32 = 3000.0 },          &gtSettings.sgAdcAlarmThresholdHigh[ADC_INPUT_3],      "Chan 3 Alarm threshold High"       },
    { CONFIG_ADC_CHAN_4_ALARM_THRESHOLD_HIGH,       "CH4THHIGH",            CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },            { .f32 = 3000.0 },          &gtSettings.sgAdcAlarmThresholdHigh[ADC_INPUT_4],      "Chan 4 Alarm threshold High"       },
    { CONFIG_ADC_CHAN_5_ALARM_THRESHOLD_HIGH,       "CH5THHIGH",            CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },            { .f32 = 3000.0 },          &gtSettings.sgAdcAlarmThresholdHigh[ADC_INPUT_5],      "Chan 5 Alarm threshold High"       },
    { CONFIG_ADC_CHAN_6_ALARM_THRESHOLD_HIGH,       "CH6THHIGH",            CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },            { .f32 = 3000.0 },          &gtSettings.sgAdcAlarmThresholdHigh[ADC_INPUT_6],      "Chan 6 Alarm threshold High"       },

    { CONFIG_ADC_CHAN_1_ALARM_EXIST_DEBOUNCE_MINUTES,"CH1ALRMEXDBNC",       CONFIG_PARAM_TYPE_UINT16,   {0},     {0xffFF},                                        {0},              &gtSettings.wAdcAlarmExistDebounce_minutes[ADC_INPUT_1],"Chan 1 Alarm Exist Debounce Mins"       },
    { CONFIG_ADC_CHAN_2_ALARM_EXIST_DEBOUNCE_MINUTES,"CH2ALRMEXDBNC",       CONFIG_PARAM_TYPE_UINT16,   {0},     {0xffFF},                                        {0},              &gtSettings.wAdcAlarmExistDebounce_minutes[ADC_INPUT_2],"Chan 2 Alarm Exist Debounce Mins"       },
    { CONFIG_ADC_CHAN_3_ALARM_EXIST_DEBOUNCE_MINUTES,"CH3ALRMEXDBNC",       CONFIG_PARAM_TYPE_UINT16,   {0},     {0xffFF},                                        {0},              &gtSettings.wAdcAlarmExistDebounce_minutes[ADC_INPUT_3],"Chan 3 Alarm Exist Debounce Mins"       },
    { CONFIG_ADC_CHAN_4_ALARM_EXIST_DEBOUNCE_MINUTES,"CH4ALRMEXDBNC",       CONFIG_PARAM_TYPE_UINT16,   {0},     {0xffFF},                                        {0},              &gtSettings.wAdcAlarmExistDebounce_minutes[ADC_INPUT_4],"Chan 4 Alarm Exist Debounce Mins"       },
    { CONFIG_ADC_CHAN_5_ALARM_EXIST_DEBOUNCE_MINUTES,"CH5ALRMEXDBNC",       CONFIG_PARAM_TYPE_UINT16,   {0},     {0xffFF},                                        {0},              &gtSettings.wAdcAlarmExistDebounce_minutes[ADC_INPUT_5],"Chan 5 Alarm Exist Debounce Mins"       },
    { CONFIG_ADC_CHAN_6_ALARM_EXIST_DEBOUNCE_MINUTES,"CH6ALRMEXDBNC",       CONFIG_PARAM_TYPE_UINT16,   {0},     {0xffFF},                                        {0},              &gtSettings.wAdcAlarmExistDebounce_minutes[ADC_INPUT_6],"Chan 6 Alarm Exist Debounce Mins"       },

    { CONFIG_ADC_CHAN_1_ALARM_IS_OVER_DEBOUNCE_MINUTES,"CH1ALRMOVRDBNC",    CONFIG_PARAM_TYPE_UINT16,   {0},     {0xffFF},                                        {0},              &gtSettings.wAdcAlarmIsOverDebounce_minutes[ADC_INPUT_1],"Chan 1 Alarm End Debounce Mins"       },
    { CONFIG_ADC_CHAN_2_ALARM_IS_OVER_DEBOUNCE_MINUTES,"CH2ALRMOVRDBNC",    CONFIG_PARAM_TYPE_UINT16,   {0},     {0xffFF},                                        {0},              &gtSettings.wAdcAlarmIsOverDebounce_minutes[ADC_INPUT_2],"Chan 2 Alarm End Debounce Mins"       },
    { CONFIG_ADC_CHAN_3_ALARM_IS_OVER_DEBOUNCE_MINUTES,"CH3ALRMOVRDBNC",    CONFIG_PARAM_TYPE_UINT16,   {0},     {0xffFF},                                        {0},              &gtSettings.wAdcAlarmIsOverDebounce_minutes[ADC_INPUT_3],"Chan 3 Alarm End Debounce Mins"       },
    { CONFIG_ADC_CHAN_4_ALARM_IS_OVER_DEBOUNCE_MINUTES,"CH4ALRMOVRDBNC",    CONFIG_PARAM_TYPE_UINT16,   {0},     {0xffFF},                                        {0},              &gtSettings.wAdcAlarmIsOverDebounce_minutes[ADC_INPUT_4],"Chan 4 Alarm End Debounce Mins"       },
    { CONFIG_ADC_CHAN_5_ALARM_IS_OVER_DEBOUNCE_MINUTES,"CH5ALRMOVRDBNC",    CONFIG_PARAM_TYPE_UINT16,   {0},     {0xffFF},                                        {0},              &gtSettings.wAdcAlarmIsOverDebounce_minutes[ADC_INPUT_5],"Chan 5 Alarm End Debounce Mins"       },
    { CONFIG_ADC_CHAN_6_ALARM_IS_OVER_DEBOUNCE_MINUTES,"CH6ALRMOVRDBNC",    CONFIG_PARAM_TYPE_UINT16,   {0},     {0xffFF},                                        {0},              &gtSettings.wAdcAlarmIsOverDebounce_minutes[ADC_INPUT_6],"Chan 6 Alarm End Debounce Mins"       },

    // Camera
    { CONFIG_CAM_ENABLE,                            "CAMEN",                CONFIG_PARAM_TYPE_UINT8,    {0},         {3},                                          {0},             &gtSettings.bCamEnabledBitMask,                         "Cam ENABLED bit mask"       },    
    { CONFIG_CAM1_RESOLUTION,                       "CAM1RES",              CONFIG_PARAM_TYPE_UINT8,    {0},      {0xFF},                                          {0},             &gtSettings.bCamResolution[0],                          "Cam1 Resolution"       },    
    { CONFIG_CAM2_RESOLUTION,                       "CAM2RES",              CONFIG_PARAM_TYPE_UINT8,    {0},      {0xFF},                                          {0},             &gtSettings.bCamResolution[1],                          "Cam2 Resolution"       },
    { CONFIG_CAM1_QUALITY,                          "CAM1QUA",              CONFIG_PARAM_TYPE_UINT8,    {0},      {0xFF},                                          {0},             &gtSettings.bCamQuality[0],                             "Cam1 Quality"       },    
    { CONFIG_CAM2_QUALITY,                          "CAM2QUA",              CONFIG_PARAM_TYPE_UINT8,    {0},      {0xFF},                                          {0},             &gtSettings.bCamQuality[1],                             "Cam2 Quality"       },    
    { CONFIG_CAM1_ORIENTATION,                      "CAM1ORIENT",           CONFIG_PARAM_TYPE_UINT8,    {0},      {0xFF},                                          {0},             &gtSettings.bCamOrientation[0],                         "Cam1 Orientation"   },    
    { CONFIG_CAM2_ORIENTATION,                      "CAM2ORIENT",           CONFIG_PARAM_TYPE_UINT8,    {0},      {0xFF},                                          {0},             &gtSettings.bCamOrientation[1],                         "Cam2 Orientation"   },    

    { CONFIG_CAM1_WINDOW_X0,                        "CAM1WINDOWX0",         CONFIG_PARAM_TYPE_UINT16,   {0},       {1279},                                         {0},             &gtSettings.wCamWindowX0[0],                         "Cam1 Capture Window X0"   },    
    { CONFIG_CAM1_WINDOW_Y0,                        "CAM1WINDOWY0",         CONFIG_PARAM_TYPE_UINT16,   {0},        {799},                                         {0},             &gtSettings.wCamWindowY0[0],                         "Cam1 Capture Window Y0"   },    
    { CONFIG_CAM1_WINDOW_X1,                        "CAM1WINDOWX1",         CONFIG_PARAM_TYPE_UINT16,   {0},       {1279},                                         {0},             &gtSettings.wCamWindowX1[0],                         "Cam1 Capture Window X1"   },    
    { CONFIG_CAM1_WINDOW_Y1,                        "CAM1WINDOWY1",         CONFIG_PARAM_TYPE_UINT16,   {0},        {799},                                         {0},             &gtSettings.wCamWindowY1[0],                         "Cam1 Capture Window Y1"   },    

    { CONFIG_CAM2_WINDOW_X0,                        "CAM2WINDOWX0",         CONFIG_PARAM_TYPE_UINT16,   {0},       {1279},                                         {0},             &gtSettings.wCamWindowX0[1],                         "Cam2 Capture Window X0"   },    
    { CONFIG_CAM2_WINDOW_Y0,                        "CAM2WINDOWY0",         CONFIG_PARAM_TYPE_UINT16,   {0},        {799},                                         {0},             &gtSettings.wCamWindowY0[1],                         "Cam2 Capture Window Y0"   },    
    { CONFIG_CAM2_WINDOW_X1,                        "CAM2WINDOWX1",         CONFIG_PARAM_TYPE_UINT16,   {0},       {1279},                                         {0},             &gtSettings.wCamWindowX1[1],                         "Cam2 Capture Window X1"   },    
    { CONFIG_CAM2_WINDOW_Y1,                        "CAM2WINDOWY1",         CONFIG_PARAM_TYPE_UINT16,   {0},        {799},                                         {0},             &gtSettings.wCamWindowY1[1],                         "Cam2 Capture Window Y1"   },    
    // Totalizer
    
    { CONFIG_TOTALIZER_ENABLE_BIT_MASK,             "TOTALEN",             CONFIG_PARAM_TYPE_UINT8,    {0},         {3},                                          {0},             &gtSettings.bTotalizerEnabledBitMask,                 "Totalizer ENABLED bit mask"       }, 
    { CONFIG_TOTALIZER_DELTA_ENABLE,                "TOTALDELTAEN",        CONFIG_PARAM_TYPE_BOOL,     {0},         {1},                                          {0},             &gtSettings.fTotalizerDeltaEnable,                    "Totalizer (PA) Delta Count Enable"       }, 
    { CONFIG_TOTALIZER_DELTA_PERIOD_TYPE,           "TOTALDELTAPERTYPE",   CONFIG_PARAM_TYPE_UINT8,    {0},         {1},                                          {0},             &gtSettings.bTotalizerDeltaPeriodType,                "Totalizer Delta Period Type 0=15|1=30 mins"       }, 
    { CONFIG_TOTALIZER_DELTA_MINS_PERIOD15,         "TOTALDELTAMINP15",    CONFIG_PARAM_TYPE_UINT8,    {1},         {14},                                         {1},             &gtSettings.bTotalizerDeltaMinsPeriod15,              "Totalizer Delta mins for Period type 0"       }, 
    { CONFIG_TOTALIZER_DELTA_MINS_PERIOD30,         "TOTALDELTAMINP30",    CONFIG_PARAM_TYPE_UINT8,    {1},         {29},                                         {1},             &gtSettings.bTotalizerDeltaMinsPeriod30,              "Totalizer Delta mins for Period type 1"       }, 

    // array for cam capture minute offset of the day    
    {CONFIG_CAM_1_MINUTE_OFFSET_01_ON_THE_DAY,      "CAM1MINOFFST01",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwCam1CaptureMinuteOffsetOnTheDay[0],            "Cam 1 Capture minute offset1on the day" },
    {CONFIG_CAM_1_MINUTE_OFFSET_02_ON_THE_DAY,      "CAM1MINOFFST02",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwCam1CaptureMinuteOffsetOnTheDay[1],            "Cam 1 Capture minute offset2on the day" },
    {CONFIG_CAM_1_MINUTE_OFFSET_03_ON_THE_DAY,      "CAM1MINOFFST03",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwCam1CaptureMinuteOffsetOnTheDay[2],            "Cam 1 Capture minute offset3on the day" },
    {CONFIG_CAM_1_MINUTE_OFFSET_04_ON_THE_DAY,      "CAM1MINOFFST04",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwCam1CaptureMinuteOffsetOnTheDay[3],            "Cam 1 Capture minute offset4on the day" },
    {CONFIG_CAM_1_MINUTE_OFFSET_05_ON_THE_DAY,      "CAM1MINOFFST05",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwCam1CaptureMinuteOffsetOnTheDay[4],            "Cam 1 Capture minute offset5on the day" },
    {CONFIG_CAM_1_MINUTE_OFFSET_06_ON_THE_DAY,      "CAM1MINOFFST06",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwCam1CaptureMinuteOffsetOnTheDay[5],            "Cam 1 Capture minute offset6on the day" },
    {CONFIG_CAM_1_MINUTE_OFFSET_07_ON_THE_DAY,      "CAM1MINOFFST07",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwCam1CaptureMinuteOffsetOnTheDay[6],            "Cam 1 Capture minute offset7on the day" },
    {CONFIG_CAM_1_MINUTE_OFFSET_08_ON_THE_DAY,      "CAM1MINOFFST08",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwCam1CaptureMinuteOffsetOnTheDay[7],            "Cam 1 Capture minute offset8on the day" },
    {CONFIG_CAM_1_MINUTE_OFFSET_09_ON_THE_DAY,      "CAM1MINOFFST09",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwCam1CaptureMinuteOffsetOnTheDay[8],            "Cam 1 Capture minute offset9on the day" },
    {CONFIG_CAM_1_MINUTE_OFFSET_10_ON_THE_DAY,      "CAM1MINOFFST10",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwCam1CaptureMinuteOffsetOnTheDay[9],            "Cam 1 Capture minute offset10on the day" },
    {CONFIG_CAM_1_MINUTE_OFFSET_11_ON_THE_DAY,      "CAM1MINOFFST11",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwCam1CaptureMinuteOffsetOnTheDay[10],           "Cam 1 Capture minute offset11on the day" },
    {CONFIG_CAM_1_MINUTE_OFFSET_12_ON_THE_DAY,      "CAM1MINOFFST12",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwCam1CaptureMinuteOffsetOnTheDay[11],           "Cam 1 Capture minute offset12on the day" },
    {CONFIG_CAM_1_MINUTE_OFFSET_13_ON_THE_DAY,      "CAM1MINOFFST13",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwCam1CaptureMinuteOffsetOnTheDay[12],           "Cam 1 Capture minute offset13on the day" },
    {CONFIG_CAM_1_MINUTE_OFFSET_14_ON_THE_DAY,      "CAM1MINOFFST14",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwCam1CaptureMinuteOffsetOnTheDay[13],           "Cam 1 Capture minute offset14on the day" },
    {CONFIG_CAM_1_MINUTE_OFFSET_15_ON_THE_DAY,      "CAM1MINOFFST15",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwCam1CaptureMinuteOffsetOnTheDay[14],           "Cam 1 Capture minute offset15on the day" },
    {CONFIG_CAM_1_MINUTE_OFFSET_16_ON_THE_DAY,      "CAM1MINOFFST16",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwCam1CaptureMinuteOffsetOnTheDay[15],           "Cam 1 Capture minute offset16on the day" },
    {CONFIG_CAM_1_MINUTE_OFFSET_17_ON_THE_DAY,      "CAM1MINOFFST17",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwCam1CaptureMinuteOffsetOnTheDay[16],           "Cam 1 Capture minute offset17on the day" },
    {CONFIG_CAM_1_MINUTE_OFFSET_18_ON_THE_DAY,      "CAM1MINOFFST18",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwCam1CaptureMinuteOffsetOnTheDay[17],           "Cam 1 Capture minute offset18on the day" },
    {CONFIG_CAM_1_MINUTE_OFFSET_19_ON_THE_DAY,      "CAM1MINOFFST19",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwCam1CaptureMinuteOffsetOnTheDay[18],           "Cam 1 Capture minute offset19on the day" },
    {CONFIG_CAM_1_MINUTE_OFFSET_20_ON_THE_DAY,      "CAM1MINOFFST20",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwCam1CaptureMinuteOffsetOnTheDay[19],           "Cam 1 Capture minute offset20on the day" },
    {CONFIG_CAM_1_MINUTE_OFFSET_21_ON_THE_DAY,      "CAM1MINOFFST21",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwCam1CaptureMinuteOffsetOnTheDay[20],           "Cam 1 Capture minute offset21on the day" },
    {CONFIG_CAM_1_MINUTE_OFFSET_22_ON_THE_DAY,      "CAM1MINOFFST22",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwCam1CaptureMinuteOffsetOnTheDay[21],           "Cam 1 Capture minute offset22on the day" },
    {CONFIG_CAM_1_MINUTE_OFFSET_23_ON_THE_DAY,      "CAM1MINOFFST23",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwCam1CaptureMinuteOffsetOnTheDay[22],           "Cam 1 Capture minute offset23on the day" },
    {CONFIG_CAM_1_MINUTE_OFFSET_24_ON_THE_DAY,      "CAM1MINOFFST24",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwCam1CaptureMinuteOffsetOnTheDay[23],           "Cam 1 Capture minute offset24on the day" },

    // array for Totalizer get count minute offset of the day    
    {CONFIG_TOTALIZER_MINUTE_OFFSET_01_ON_THE_DAY,  "TOTALMINOFFST01",     CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTotalizerGetCountMinuteOffsetOnTheDay[0],      "Totalizer get count minute offset1on the day" },
    {CONFIG_TOTALIZER_MINUTE_OFFSET_02_ON_THE_DAY,  "TOTALMINOFFST02",     CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTotalizerGetCountMinuteOffsetOnTheDay[1],      "Totalizer get count minute offset2on the day" },
    {CONFIG_TOTALIZER_MINUTE_OFFSET_03_ON_THE_DAY,  "TOTALMINOFFST03",     CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTotalizerGetCountMinuteOffsetOnTheDay[2],      "Totalizer get count minute offset3on the day" },
    {CONFIG_TOTALIZER_MINUTE_OFFSET_04_ON_THE_DAY,  "TOTALMINOFFST04",     CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTotalizerGetCountMinuteOffsetOnTheDay[3],      "Totalizer get count minute offset4on the day" },
    {CONFIG_TOTALIZER_MINUTE_OFFSET_05_ON_THE_DAY,  "TOTALMINOFFST05",     CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTotalizerGetCountMinuteOffsetOnTheDay[4],      "Totalizer get count minute offset5on the day" },
    {CONFIG_TOTALIZER_MINUTE_OFFSET_06_ON_THE_DAY,  "TOTALMINOFFST06",     CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTotalizerGetCountMinuteOffsetOnTheDay[5],      "Totalizer get count minute offset6on the day" },
    {CONFIG_TOTALIZER_MINUTE_OFFSET_07_ON_THE_DAY,  "TOTALMINOFFST07",     CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTotalizerGetCountMinuteOffsetOnTheDay[6],      "Totalizer get count minute offset7on the day" },
    {CONFIG_TOTALIZER_MINUTE_OFFSET_08_ON_THE_DAY,  "TOTALMINOFFST08",     CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTotalizerGetCountMinuteOffsetOnTheDay[7],      "Totalizer get count minute offset8on the day" },
    {CONFIG_TOTALIZER_MINUTE_OFFSET_09_ON_THE_DAY,  "TOTALMINOFFST09",     CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTotalizerGetCountMinuteOffsetOnTheDay[8],      "Totalizer get count minute offset9on the day" },
    {CONFIG_TOTALIZER_MINUTE_OFFSET_10_ON_THE_DAY,  "TOTALMINOFFST10",     CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTotalizerGetCountMinuteOffsetOnTheDay[9],      "Totalizer get count minute offset10on the day" },
    {CONFIG_TOTALIZER_MINUTE_OFFSET_11_ON_THE_DAY,  "TOTALMINOFFST11",     CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTotalizerGetCountMinuteOffsetOnTheDay[10],     "Totalizer get count minute offset11on the day" },
    {CONFIG_TOTALIZER_MINUTE_OFFSET_12_ON_THE_DAY,  "TOTALMINOFFST12",     CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTotalizerGetCountMinuteOffsetOnTheDay[11],     "Totalizer get count minute offset12on the day" },
    {CONFIG_TOTALIZER_MINUTE_OFFSET_13_ON_THE_DAY,  "TOTALMINOFFST13",     CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTotalizerGetCountMinuteOffsetOnTheDay[12],     "Totalizer get count minute offset13on the day" },
    {CONFIG_TOTALIZER_MINUTE_OFFSET_14_ON_THE_DAY,  "TOTALMINOFFST14",     CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTotalizerGetCountMinuteOffsetOnTheDay[13],     "Totalizer get count minute offset14on the day" },
    {CONFIG_TOTALIZER_MINUTE_OFFSET_15_ON_THE_DAY,  "TOTALMINOFFST15",     CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTotalizerGetCountMinuteOffsetOnTheDay[14],     "Totalizer get count minute offset15on the day" },
    {CONFIG_TOTALIZER_MINUTE_OFFSET_16_ON_THE_DAY,  "TOTALMINOFFST16",     CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTotalizerGetCountMinuteOffsetOnTheDay[15],     "Totalizer get count minute offset16on the day" },
    {CONFIG_TOTALIZER_MINUTE_OFFSET_17_ON_THE_DAY,  "TOTALMINOFFST17",     CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTotalizerGetCountMinuteOffsetOnTheDay[16],     "Totalizer get count minute offset17on the day" },
    {CONFIG_TOTALIZER_MINUTE_OFFSET_18_ON_THE_DAY,  "TOTALMINOFFST18",     CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTotalizerGetCountMinuteOffsetOnTheDay[17],     "Totalizer get count minute offset18on the day" },
    {CONFIG_TOTALIZER_MINUTE_OFFSET_19_ON_THE_DAY,  "TOTALMINOFFST19",     CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTotalizerGetCountMinuteOffsetOnTheDay[18],     "Totalizer get count minute offset19on the day" },
    {CONFIG_TOTALIZER_MINUTE_OFFSET_20_ON_THE_DAY,  "TOTALMINOFFST20",     CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTotalizerGetCountMinuteOffsetOnTheDay[19],     "Totalizer get count minute offset20on the day" },
    {CONFIG_TOTALIZER_MINUTE_OFFSET_21_ON_THE_DAY,  "TOTALMINOFFST21",     CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTotalizerGetCountMinuteOffsetOnTheDay[20],     "Totalizer get count minute offset21on the day" },
    {CONFIG_TOTALIZER_MINUTE_OFFSET_22_ON_THE_DAY,  "TOTALMINOFFST22",     CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTotalizerGetCountMinuteOffsetOnTheDay[21],     "Totalizer get count minute offset22on the day" },
    {CONFIG_TOTALIZER_MINUTE_OFFSET_23_ON_THE_DAY,  "TOTALMINOFFST23",     CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTotalizerGetCountMinuteOffsetOnTheDay[22],     "Totalizer get count minute offset23on the day" },
    {CONFIG_TOTALIZER_MINUTE_OFFSET_24_ON_THE_DAY,  "TOTALMINOFFST24",     CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTotalizerGetCountMinuteOffsetOnTheDay[23],     "Totalizer get count minute offset24on the day" },

    //  array for transmissions along the day        
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_01_ON_THE_DAY,"TRNSMINOFFST01",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[0],        "transmission minute offset1on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_02_ON_THE_DAY,"TRNSMINOFFST02",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[1],        "transmission minute offset2on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_03_ON_THE_DAY,"TRNSMINOFFST03",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[2],        "transmission minute offset3on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_04_ON_THE_DAY,"TRNSMINOFFST04",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[3],        "transmission minute offset4on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_05_ON_THE_DAY,"TRNSMINOFFST05",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[4],        "transmission minute offset5on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_06_ON_THE_DAY,"TRNSMINOFFST06",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[5],        "transmission minute offset6on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_07_ON_THE_DAY,"TRNSMINOFFST07",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[6],        "transmission minute offset7on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_08_ON_THE_DAY,"TRNSMINOFFST08",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[7],        "transmission minute offset8on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_09_ON_THE_DAY,"TRNSMINOFFST09",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[8],        "transmission minute offset9on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_10_ON_THE_DAY,"TRNSMINOFFST10",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[9],        "transmission minute offset10on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_11_ON_THE_DAY,"TRNSMINOFFST11",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[10],       "transmission minute offset11on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_12_ON_THE_DAY,"TRNSMINOFFST12",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[11],       "transmission minute offset12on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_13_ON_THE_DAY,"TRNSMINOFFST13",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[12],       "transmission minute offset13on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_14_ON_THE_DAY,"TRNSMINOFFST14",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[13],       "transmission minute offset14on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_15_ON_THE_DAY,"TRNSMINOFFST15",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[14],       "transmission minute offset15on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_16_ON_THE_DAY,"TRNSMINOFFST16",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[15],       "transmission minute offset16on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_17_ON_THE_DAY,"TRNSMINOFFST17",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[16],       "transmission minute offset17on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_18_ON_THE_DAY,"TRNSMINOFFST18",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[17],       "transmission minute offset18on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_19_ON_THE_DAY,"TRNSMINOFFST19",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[18],       "transmission minute offset19on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_20_ON_THE_DAY,"TRNSMINOFFST20",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[19],       "transmission minute offset20on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_21_ON_THE_DAY,"TRNSMINOFFST21",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[20],       "transmission minute offset21on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_22_ON_THE_DAY,"TRNSMINOFFST22",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[21],       "transmission minute offset22on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_23_ON_THE_DAY,"TRNSMINOFFST23",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[22],       "transmission minute offset23on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_24_ON_THE_DAY,"TRNSMINOFFST24",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[23],       "transmission minute offset24on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_25_ON_THE_DAY,"TRNSMINOFFST25",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[24],       "transmission minute offset25on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_26_ON_THE_DAY,"TRNSMINOFFST26",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[25],       "transmission minute offset26on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_27_ON_THE_DAY,"TRNSMINOFFST27",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[26],       "transmission minute offset27on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_28_ON_THE_DAY,"TRNSMINOFFST28",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[27],       "transmission minute offset28on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_29_ON_THE_DAY,"TRNSMINOFFST29",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[28],       "transmission minute offset29on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_30_ON_THE_DAY,"TRNSMINOFFST30",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[29],       "transmission minute offset30on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_31_ON_THE_DAY,"TRNSMINOFFST31",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[30],       "transmission minute offset31on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_32_ON_THE_DAY,"TRNSMINOFFST32",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[31],       "transmission minute offset32on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_33_ON_THE_DAY,"TRNSMINOFFST33",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[32],       "transmission minute offset33on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_34_ON_THE_DAY,"TRNSMINOFFST34",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[33],       "transmission minute offset34on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_35_ON_THE_DAY,"TRNSMINOFFST35",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[34],       "transmission minute offset35on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_36_ON_THE_DAY,"TRNSMINOFFST36",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[35],       "transmission minute offset36on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_37_ON_THE_DAY,"TRNSMINOFFST37",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[36],       "transmission minute offset37on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_38_ON_THE_DAY,"TRNSMINOFFST38",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[37],       "transmission minute offset38on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_39_ON_THE_DAY,"TRNSMINOFFST39",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[38],       "transmission minute offset39on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_40_ON_THE_DAY,"TRNSMINOFFST40",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[39],       "transmission minute offset40on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_41_ON_THE_DAY,"TRNSMINOFFST41",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[40],       "transmission minute offset41on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_42_ON_THE_DAY,"TRNSMINOFFST42",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[41],       "transmission minute offset42on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_43_ON_THE_DAY,"TRNSMINOFFST43",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[42],       "transmission minute offset43on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_44_ON_THE_DAY,"TRNSMINOFFST44",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[43],       "transmission minute offset44on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_45_ON_THE_DAY,"TRNSMINOFFST45",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[44],       "transmission minute offset45on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_46_ON_THE_DAY,"TRNSMINOFFST46",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[45],       "transmission minute offset46on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_47_ON_THE_DAY,"TRNSMINOFFST47",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[46],       "transmission minute offset47on the day" },
    {CONFIG_TRANSMISSION_MINUTE_OFFSET_48_ON_THE_DAY,"TRNSMINOFFST48",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtSettings.iwTranmissionMinuteOffsetOnTheDay[47],       "transmission minute offset48on the day" },
    
    {CONFIG_DOWNLOAD_SCRIPT_PERIOD_ENABLE,          "DOWNSCRPEREN",         CONFIG_PARAM_TYPE_BOOL,    {0},            {1},                                         {0},            &gtSettings.fDownloadScriptPeriodEnable,                 "Download script Period Enable" },
    {CONFIG_DOWNLOAD_SCRIPT_PERIOD_TIME,            "DOWNSCRPERTIME",       CONFIG_PARAM_TYPE_UINT16,  {0},        {65535},                                         {15},           &gtSettings.wDownloadScriptPeriodValue,                  "Download script Period Value" },
    {CONFIG_DOWNLOAD_SCRIPT_PERIOD_UNIT_TYPE,       "DOWNSCRPERTYPE",       CONFIG_PARAM_TYPE_UINT8,   {1},            {3},                                         {1},            &gtSettings.bDownloadScriptPeriodType,                   "Download script Period Time units Type" },

    {CONFIG_SETTING_UPDATE_TIME_FROM_SINGLE_TOWER_ENABLE,"TIME1TOWEREN",    CONFIG_PARAM_TYPE_BOOL,    {0},            {1},                                          {0},           &gtSettings.fUpdateTimeFromSingleTowerEnable,            "Update Time from Single Tower Enable" },
    {CONFIG_SETTING_UPDATE_TIME_CELL_FILTER_TYPE,   "TIMETOWERFILTER",      CONFIG_PARAM_TYPE_UINT8,   {0},            {1},                                          {0},           &gtSettings.bUpdateTimeTowerFilterType,                  "Update Time Filter(0=AllExcpt1|1=1ExcptAll)" },

//    {CONFIG_SETTING_UPDATE_TIME_CELL_TOWER_MCC,     "TIMETOWERMCC",         CONFIG_PARAM_TYPE_STRING,   0,             CONFIG_PARAM_STRING_LENGTH_TOWER_MCC,         0,            &gtSettings.cUpdateTimeTowerMcc[0],                      "Update Time Tower Mcc" },
//    {CONFIG_SETTING_UPDATE_TIME_CELL_TOWER_MNC,     "TIMETOWERMNC",         CONFIG_PARAM_TYPE_STRING,   0,             CONFIG_PARAM_STRING_LENGTH_TOWER_MNC,         0,            &gtSettings.cUpdateTimeTowerMnc[0],                      "Update Time Tower Mnc" },
    {CONFIG_SETTING_UPDATE_TIME_CELL_TOWER_LAC,     "TIMETOWERLAC",         CONFIG_PARAM_TYPE_STRING,  {0},           {CONFIG_PARAM_STRING_LENGTH_TOWER_LAC},        {0},           &gtSettings.cUpdateTimeTowerLac[0],                      "Update Time Tower Lac (hex val with no 0x prefix)" },
    {CONFIG_SETTING_UPDATE_TIME_CELL_TOWER_CID,     "TIMETOWERCID",         CONFIG_PARAM_TYPE_STRING,  {0},           {CONFIG_PARAM_STRING_LENGTH_TOWER_CID},        {0},           &gtSettings.cUpdateTimeTowerCid[0],                      "Update Time Tower Cid (hex val with no 0x prefix)" },

};
#define CONFIG_SETTINGS_NUM_OF_ELEMENTS                (sizeof(gtConfigSettingsMeta)/sizeof(gtConfigSettingsMeta[0]))

static const ConfigParamInfoType 		gtConfigCalibrationMeta[] = 
{
    // Param Id,                                    Param Name,             Param Type,                 Param Min,  Param Max,                                      Param Default,  Param Data,                                             Param Description
    // NOTE:  For strings, the max field contains the string length 
    // WARNING: min max values can be integers even if the Data is of type FLOAT!
            
    // for RTD
    { CONFIG_ADC_CHAN_1_RTD_CAL_EXPECTED_X1_MILLIVOLTS,     "CH1RTDEXPX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 100.0 },        &gtAdcCalib.sgAdcRtdCalibrationExpectedX1_mV[ADC_INPUT_1],        "chan 1 rtd cal expected x1 (mV)"       },
    { CONFIG_ADC_CHAN_2_RTD_CAL_EXPECTED_X1_MILLIVOLTS,     "CH2RTDEXPX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 100.0 },        &gtAdcCalib.sgAdcRtdCalibrationExpectedX1_mV[ADC_INPUT_2],        "chan 2 rtd cal expected x1 (mV)"       },
    { CONFIG_ADC_CHAN_3_RTD_CAL_EXPECTED_X1_MILLIVOLTS,     "CH3RTDEXPX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 100.0 },        &gtAdcCalib.sgAdcRtdCalibrationExpectedX1_mV[ADC_INPUT_3],        "chan 3 rtd cal expected x1 (mV)"       },
    { CONFIG_ADC_CHAN_4_RTD_CAL_EXPECTED_X1_MILLIVOLTS,     "CH4RTDEXPX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 100.0 },        &gtAdcCalib.sgAdcRtdCalibrationExpectedX1_mV[ADC_INPUT_4],        "chan 4 rtd cal expected x1 (mV)"       },
    { CONFIG_ADC_CHAN_5_RTD_CAL_EXPECTED_X1_MILLIVOLTS,     "CH5RTDEXPX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 100.0 },        &gtAdcCalib.sgAdcRtdCalibrationExpectedX1_mV[ADC_INPUT_5],        "chan 5 rtd cal expected x1 (mV)"       },
    { CONFIG_ADC_CHAN_6_RTD_CAL_EXPECTED_X1_MILLIVOLTS,     "CH6RTDEXPX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 100.0 },           &gtAdcCalib.sgAdcRtdCalibrationExpectedX1_mV[ADC_INPUT_6],        "chan 6 rtd cal expected x1 (mV)"       },

    { CONFIG_ADC_CHAN_1_RTD_CAL_EXPECTED_X2_MILLIVOLTS,     "CH1RTDEXPX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 150.0 },           &gtAdcCalib.sgAdcRtdCalibrationExpectedX2_mV[ADC_INPUT_1],        "chan 1 rtd cal expected x2 (mV)"       },
    { CONFIG_ADC_CHAN_2_RTD_CAL_EXPECTED_X2_MILLIVOLTS,     "CH2RTDEXPX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 150.0 },           &gtAdcCalib.sgAdcRtdCalibrationExpectedX2_mV[ADC_INPUT_2],        "chan 2 rtd cal expected x2 (mV)"       },
    { CONFIG_ADC_CHAN_3_RTD_CAL_EXPECTED_X2_MILLIVOLTS,     "CH3RTDEXPX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 150.0 },           &gtAdcCalib.sgAdcRtdCalibrationExpectedX2_mV[ADC_INPUT_3],        "chan 3 rtd cal expected x2 (mV)"       },
    { CONFIG_ADC_CHAN_4_RTD_CAL_EXPECTED_X2_MILLIVOLTS,     "CH4RTDEXPX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 150.0 },           &gtAdcCalib.sgAdcRtdCalibrationExpectedX2_mV[ADC_INPUT_4],        "chan 4 rtd cal expected x2 (mV)"       },
    { CONFIG_ADC_CHAN_5_RTD_CAL_EXPECTED_X2_MILLIVOLTS,     "CH5RTDEXPX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 150.0 },           &gtAdcCalib.sgAdcRtdCalibrationExpectedX2_mV[ADC_INPUT_5],        "chan 5 rtd cal expected x2 (mV)"       },
    { CONFIG_ADC_CHAN_6_RTD_CAL_EXPECTED_X2_MILLIVOLTS,     "CH6RTDEXPX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 150.0 },           &gtAdcCalib.sgAdcRtdCalibrationExpectedX2_mV[ADC_INPUT_6],        "chan 6 rtd cal expected x2 (mV)"       },

    { CONFIG_ADC_CHAN_1_RTD_CAL_OBTAINED_X1_MILLIVOLTS,     "CH1RTDOBTX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcRtdCalibrationObtainedX1_mV[ADC_INPUT_1],        "chan 1 rtd cal obtained x1 (mV)"       },
    { CONFIG_ADC_CHAN_2_RTD_CAL_OBTAINED_X1_MILLIVOLTS,     "CH2RTDOBTX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcRtdCalibrationObtainedX1_mV[ADC_INPUT_2],        "chan 2 rtd cal obtained x1 (mV)"       },
    { CONFIG_ADC_CHAN_3_RTD_CAL_OBTAINED_X1_MILLIVOLTS,     "CH3RTDOBTX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcRtdCalibrationObtainedX1_mV[ADC_INPUT_3],        "chan 3 rtd cal obtained x1 (mV)"       },
    { CONFIG_ADC_CHAN_4_RTD_CAL_OBTAINED_X1_MILLIVOLTS,     "CH4RTDOBTX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcRtdCalibrationObtainedX1_mV[ADC_INPUT_4],        "chan 4 rtd cal obtained x1 (mV)"       },
    { CONFIG_ADC_CHAN_5_RTD_CAL_OBTAINED_X1_MILLIVOLTS,     "CH5RTDOBTX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcRtdCalibrationObtainedX1_mV[ADC_INPUT_5],        "chan 5 rtd cal obtained x1 (mV)"       },
    { CONFIG_ADC_CHAN_6_RTD_CAL_OBTAINED_X1_MILLIVOLTS,     "CH6RTDOBTX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcRtdCalibrationObtainedX1_mV[ADC_INPUT_6],        "chan 6 rtd cal obtained x1 (mV)"       },

    { CONFIG_ADC_CHAN_1_RTD_CAL_OBTAINED_X2_MILLIVOLTS,     "CH1RTDOBTX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcRtdCalibrationObtainedX2_mV[ADC_INPUT_1],        "chan 1 rtd cal obtained x2 (mV)"       },        
    { CONFIG_ADC_CHAN_2_RTD_CAL_OBTAINED_X2_MILLIVOLTS,     "CH2RTDOBTX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcRtdCalibrationObtainedX2_mV[ADC_INPUT_2],        "chan 2 rtd cal obtained x2 (mV)"       },    
    { CONFIG_ADC_CHAN_3_RTD_CAL_OBTAINED_X2_MILLIVOLTS,     "CH3RTDOBTX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcRtdCalibrationObtainedX2_mV[ADC_INPUT_3],        "chan 3 rtd cal obtained x2 (mV)"       },    
    { CONFIG_ADC_CHAN_4_RTD_CAL_OBTAINED_X2_MILLIVOLTS,     "CH4RTDOBTX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcRtdCalibrationObtainedX2_mV[ADC_INPUT_4],        "chan 4 rtd cal obtained x2 (mV)"       },    
    { CONFIG_ADC_CHAN_5_RTD_CAL_OBTAINED_X2_MILLIVOLTS,     "CH5RTDOBTX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcRtdCalibrationObtainedX2_mV[ADC_INPUT_5],        "chan 5 rtd cal obtained x2 (mV)"       },    
    { CONFIG_ADC_CHAN_6_RTD_CAL_OBTAINED_X2_MILLIVOLTS,     "CH6RTDOBTX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcRtdCalibrationObtainedX2_mV[ADC_INPUT_6],        "chan 6 rtd cal obtained x2 (mV)"       },    

//    { CONFIG_ADC_CHAN_1_RTD_CORRECTION_FACTOR_ENABLED,      "CH1RTDCOREN",  CONFIG_PARAM_TYPE_BOOL,     0,          1,                                            0,              &gtAdcCalib.fAdcRtdCorrectionFactorEnabled[ADC_INPUT_1],         "chan 1 rtd correction ENABLED"    },
//    { CONFIG_ADC_CHAN_2_RTD_CORRECTION_FACTOR_ENABLED,      "CH2RTDCOREN",  CONFIG_PARAM_TYPE_BOOL,     0,          1,                                            0,              &gtAdcCalib.fAdcRtdCorrectionFactorEnabled[ADC_INPUT_2],         "chan 2 rtd correction ENABLED"    },
//    { CONFIG_ADC_CHAN_3_RTD_CORRECTION_FACTOR_ENABLED,      "CH3RTDCOREN",  CONFIG_PARAM_TYPE_BOOL,     0,          1,                                            0,              &gtAdcCalib.fAdcRtdCorrectionFactorEnabled[ADC_INPUT_3],         "chan 3 rtd correction ENABLED"    },
//    { CONFIG_ADC_CHAN_4_RTD_CORRECTION_FACTOR_ENABLED,      "CH4RTDCOREN",  CONFIG_PARAM_TYPE_BOOL,     0,          1,                                            0,              &gtAdcCalib.fAdcRtdCorrectionFactorEnabled[ADC_INPUT_4],         "chan 4 rtd correction ENABLED"    },
//    { CONFIG_ADC_CHAN_5_RTD_CORRECTION_FACTOR_ENABLED,      "CH5RTDCOREN",  CONFIG_PARAM_TYPE_BOOL,     0,          1,                                            0,              &gtAdcCalib.fAdcRtdCorrectionFactorEnabled[ADC_INPUT_5],         "chan 5 rtd correction ENABLED"    },
//    { CONFIG_ADC_CHAN_6_RTD_CORRECTION_FACTOR_ENABLED,      "CH6RTDCOREN",  CONFIG_PARAM_TYPE_BOOL,     0,          1,                                            0,              &gtAdcCalib.fAdcRtdCorrectionFactorEnabled[ADC_INPUT_6],         "chan 6 rtd correction ENABLED"    },

    { CONFIG_ADC_CHAN_1_RTD_CORRECTION_FACTOR_OFFSET,       "CH1RTDOFFST",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcRtdCorrectionFactorOffset[ADC_INPUT_1],          "chan 1 rtd offset(mV)"                },
    { CONFIG_ADC_CHAN_2_RTD_CORRECTION_FACTOR_OFFSET,       "CH2RTDOFFST",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcRtdCorrectionFactorOffset[ADC_INPUT_2],          "chan 2 rtd offset(mV)"                },
    { CONFIG_ADC_CHAN_3_RTD_CORRECTION_FACTOR_OFFSET,       "CH3RTDOFFST",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcRtdCorrectionFactorOffset[ADC_INPUT_3],          "chan 3 rtd offset(mV)"                },
    { CONFIG_ADC_CHAN_4_RTD_CORRECTION_FACTOR_OFFSET,       "CH4RTDOFFST",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcRtdCorrectionFactorOffset[ADC_INPUT_4],          "chan 4 rtd offset(mV)"                },
    { CONFIG_ADC_CHAN_5_RTD_CORRECTION_FACTOR_OFFSET,       "CH5RTDOFFST",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcRtdCorrectionFactorOffset[ADC_INPUT_5],          "chan 5 rtd offset(mV)"                },
    { CONFIG_ADC_CHAN_6_RTD_CORRECTION_FACTOR_OFFSET,       "CH6RTDOFFST",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcRtdCorrectionFactorOffset[ADC_INPUT_6],          "chan 6 rtd offset(mV)"                },

    { CONFIG_ADC_CHAN_1_RTD_CORRECTION_FACTOR_SLOPE,        "CH1RTDSLP",    CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -1000.0 },    { .f32 = 1000.0 },                                         { .f32 = 1.0 },            &gtAdcCalib.sgAdcRtdCorrectionFactorSlope[ADC_INPUT_1],           "chan 1 rtd slope"                 },    
    { CONFIG_ADC_CHAN_2_RTD_CORRECTION_FACTOR_SLOPE,        "CH2RTDSLP",    CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -1000.0 },    { .f32 = 1000.0 },                                         { .f32 = 1.0 },            &gtAdcCalib.sgAdcRtdCorrectionFactorSlope[ADC_INPUT_2],           "chan 2 rtd slope"                 },    
    { CONFIG_ADC_CHAN_3_RTD_CORRECTION_FACTOR_SLOPE,        "CH3RTDSLP",    CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -1000.0 },    { .f32 = 1000.0 },                                         { .f32 = 1.0 },            &gtAdcCalib.sgAdcRtdCorrectionFactorSlope[ADC_INPUT_3],           "chan 3 rtd slope"                 },    
    { CONFIG_ADC_CHAN_4_RTD_CORRECTION_FACTOR_SLOPE,        "CH4RTDSLP",    CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -1000.0 },    { .f32 = 1000.0 },                                         { .f32 = 1.0 },            &gtAdcCalib.sgAdcRtdCorrectionFactorSlope[ADC_INPUT_4],           "chan 4 rtd slope"                 },    
    { CONFIG_ADC_CHAN_5_RTD_CORRECTION_FACTOR_SLOPE,        "CH5RTDSLP",    CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -1000.0 },    { .f32 = 1000.0 },                                         { .f32 = 1.0 },            &gtAdcCalib.sgAdcRtdCorrectionFactorSlope[ADC_INPUT_5],           "chan 5 rtd slope"                 },    
    { CONFIG_ADC_CHAN_6_RTD_CORRECTION_FACTOR_SLOPE,        "CH6RTDSLP",    CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -1000.0 },    { .f32 = 1000.0 },                                         { .f32 = 1.0 },            &gtAdcCalib.sgAdcRtdCorrectionFactorSlope[ADC_INPUT_6],           "chan 6 rtd slope"                 },    

    // SCALE A
    { CONFIG_ADC_CHAN_1_SCL_A_CAL_EXPECTED_X1_MILLIVOLTS,   "CH1SCAEXPX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 100.0 },           &gtAdcCalib.sgAdcSclACalibrationExpectedX1_mV[ADC_INPUT_1],   "chan 1 Scaling A cal expected x1 (mV)"       },
    { CONFIG_ADC_CHAN_2_SCL_A_CAL_EXPECTED_X1_MILLIVOLTS,   "CH2SCAEXPX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 100.0 },           &gtAdcCalib.sgAdcSclACalibrationExpectedX1_mV[ADC_INPUT_2],   "chan 2 Scaling A cal expected x1 (mV)"       },
    { CONFIG_ADC_CHAN_3_SCL_A_CAL_EXPECTED_X1_MILLIVOLTS,   "CH3SCAEXPX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 100.0 },           &gtAdcCalib.sgAdcSclACalibrationExpectedX1_mV[ADC_INPUT_3],   "chan 3 Scaling A cal expected x1 (mV)"       },
    { CONFIG_ADC_CHAN_4_SCL_A_CAL_EXPECTED_X1_MILLIVOLTS,   "CH4SCAEXPX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 100.0 },           &gtAdcCalib.sgAdcSclACalibrationExpectedX1_mV[ADC_INPUT_4],   "chan 4 Scaling A cal expected x1 (mV)"       },
    { CONFIG_ADC_CHAN_5_SCL_A_CAL_EXPECTED_X1_MILLIVOLTS,   "CH5SCAEXPX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 100.0 },           &gtAdcCalib.sgAdcSclACalibrationExpectedX1_mV[ADC_INPUT_5],   "chan 5 Scaling A cal expected x1 (mV)"       },
    { CONFIG_ADC_CHAN_6_SCL_A_CAL_EXPECTED_X1_MILLIVOLTS,   "CH6SCAEXPX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 100.0 },           &gtAdcCalib.sgAdcSclACalibrationExpectedX1_mV[ADC_INPUT_6],   "chan 6 Scaling A cal expected x1 (mV)"       },

    { CONFIG_ADC_CHAN_1_SCL_A_CAL_EXPECTED_X2_MILLIVOLTS,   "CH1SCAEXPX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 150.0 },           &gtAdcCalib.sgAdcSclACalibrationExpectedX2_mV[ADC_INPUT_1],   "chan 1 Scaling A cal expected x2 (mV)"       },
    { CONFIG_ADC_CHAN_2_SCL_A_CAL_EXPECTED_X2_MILLIVOLTS,   "CH2SCAEXPX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 150.0 },           &gtAdcCalib.sgAdcSclACalibrationExpectedX2_mV[ADC_INPUT_2],   "chan 2 Scaling A cal expected x2 (mV)"       },
    { CONFIG_ADC_CHAN_3_SCL_A_CAL_EXPECTED_X2_MILLIVOLTS,   "CH3SCAEXPX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 150.0 },           &gtAdcCalib.sgAdcSclACalibrationExpectedX2_mV[ADC_INPUT_3],   "chan 3 Scaling A cal expected x2 (mV)"       },
    { CONFIG_ADC_CHAN_4_SCL_A_CAL_EXPECTED_X2_MILLIVOLTS,   "CH4SCAEXPX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 150.0 },           &gtAdcCalib.sgAdcSclACalibrationExpectedX2_mV[ADC_INPUT_4],   "chan 4 Scaling A cal expected x2 (mV)"       },
    { CONFIG_ADC_CHAN_5_SCL_A_CAL_EXPECTED_X2_MILLIVOLTS,   "CH5SCAEXPX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 150.0 },           &gtAdcCalib.sgAdcSclACalibrationExpectedX2_mV[ADC_INPUT_5],   "chan 5 Scaling A cal expected x2 (mV)"       },
    { CONFIG_ADC_CHAN_6_SCL_A_CAL_EXPECTED_X2_MILLIVOLTS,   "CH6SCAEXPX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 150.0 },           &gtAdcCalib.sgAdcSclACalibrationExpectedX2_mV[ADC_INPUT_6],   "chan 6 Scaling A cal expected x2 (mV)"       },

    { CONFIG_ADC_CHAN_1_SCL_A_CAL_OBTAINED_X1_MILLIVOLTS,   "CH1SCAOBTX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcSclACalibrationObtainedX1_mV[ADC_INPUT_1],   "chan 1 Scaling A cal obtained x1 (mV)"       },
    { CONFIG_ADC_CHAN_2_SCL_A_CAL_OBTAINED_X1_MILLIVOLTS,   "CH2SCAOBTX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcSclACalibrationObtainedX1_mV[ADC_INPUT_2],   "chan 2 Scaling A cal obtained x1 (mV)"       },
    { CONFIG_ADC_CHAN_3_SCL_A_CAL_OBTAINED_X1_MILLIVOLTS,   "CH3SCAOBTX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcSclACalibrationObtainedX1_mV[ADC_INPUT_3],   "chan 3 Scaling A cal obtained x1 (mV)"       },
    { CONFIG_ADC_CHAN_4_SCL_A_CAL_OBTAINED_X1_MILLIVOLTS,   "CH4SCAOBTX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcSclACalibrationObtainedX1_mV[ADC_INPUT_4],   "chan 4 Scaling A cal obtained x1 (mV)"       },
    { CONFIG_ADC_CHAN_5_SCL_A_CAL_OBTAINED_X1_MILLIVOLTS,   "CH5SCAOBTX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcSclACalibrationObtainedX1_mV[ADC_INPUT_5],   "chan 5 Scaling A cal obtained x1 (mV)"       },
    { CONFIG_ADC_CHAN_6_SCL_A_CAL_OBTAINED_X1_MILLIVOLTS,   "CH6SCAOBTX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcSclACalibrationObtainedX1_mV[ADC_INPUT_6],   "chan 6 Scaling A cal obtained x1 (mV)"       },

    { CONFIG_ADC_CHAN_1_SCL_A_CAL_OBTAINED_X2_MILLIVOLTS,   "CH1SCAOBTX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcSclACalibrationObtainedX2_mV[ADC_INPUT_1],   "chan 1 Scaling A cal obtained x2 (mV)"       },    
    { CONFIG_ADC_CHAN_2_SCL_A_CAL_OBTAINED_X2_MILLIVOLTS,   "CH2SCAOBTX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcSclACalibrationObtainedX2_mV[ADC_INPUT_2],   "chan 2 Scaling A cal obtained x2 (mV)"       },    
    { CONFIG_ADC_CHAN_3_SCL_A_CAL_OBTAINED_X2_MILLIVOLTS,   "CH3SCAOBTX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcSclACalibrationObtainedX2_mV[ADC_INPUT_3],   "chan 3 Scaling A cal obtained x2 (mV)"       },    
    { CONFIG_ADC_CHAN_4_SCL_A_CAL_OBTAINED_X2_MILLIVOLTS,   "CH4SCAOBTX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcSclACalibrationObtainedX2_mV[ADC_INPUT_4],   "chan 4 Scaling A cal obtained x2 (mV)"       },    
    { CONFIG_ADC_CHAN_5_SCL_A_CAL_OBTAINED_X2_MILLIVOLTS,   "CH5SCAOBTX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcSclACalibrationObtainedX2_mV[ADC_INPUT_5],   "chan 5 Scaling A cal obtained x2 (mV)"       },    
    { CONFIG_ADC_CHAN_6_SCL_A_CAL_OBTAINED_X2_MILLIVOLTS,   "CH6SCAOBTX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcSclACalibrationObtainedX2_mV[ADC_INPUT_6],   "chan 6 Scaling A cal obtained x2 (mV)"       },    

//    { CONFIG_ADC_CHAN_1_SCL_A_CORRECTION_FACTOR_ENABLED,    "CH1SCACOREN",  CONFIG_PARAM_TYPE_BOOL,     0,          1,                                            0,              &gtAdcCalib.fAdcSclACorrectionFactorEnabled[ADC_INPUT_1],     "chan 1 Scaling A correction ENABLED"    },
//    { CONFIG_ADC_CHAN_2_SCL_A_CORRECTION_FACTOR_ENABLED,    "CH2SCACOREN",  CONFIG_PARAM_TYPE_BOOL,     0,          1,                                            0,              &gtAdcCalib.fAdcSclACorrectionFactorEnabled[ADC_INPUT_2],     "chan 2 Scaling A correction ENABLED"    },
//    { CONFIG_ADC_CHAN_3_SCL_A_CORRECTION_FACTOR_ENABLED,    "CH3SCACOREN",  CONFIG_PARAM_TYPE_BOOL,     0,          1,                                            0,              &gtAdcCalib.fAdcSclACorrectionFactorEnabled[ADC_INPUT_3],     "chan 3 Scaling A correction ENABLED"    },
//    { CONFIG_ADC_CHAN_4_SCL_A_CORRECTION_FACTOR_ENABLED,    "CH4SCACOREN",  CONFIG_PARAM_TYPE_BOOL,     0,          1,                                            0,              &gtAdcCalib.fAdcSclACorrectionFactorEnabled[ADC_INPUT_4],     "chan 4 Scaling A correction ENABLED"    },
//    { CONFIG_ADC_CHAN_5_SCL_A_CORRECTION_FACTOR_ENABLED,    "CH5SCACOREN",  CONFIG_PARAM_TYPE_BOOL,     0,          1,                                            0,              &gtAdcCalib.fAdcSclACorrectionFactorEnabled[ADC_INPUT_5],     "chan 5 Scaling A correction ENABLED"    },
//    { CONFIG_ADC_CHAN_6_SCL_A_CORRECTION_FACTOR_ENABLED,    "CH6SCACOREN",  CONFIG_PARAM_TYPE_BOOL,     0,          1,                                            0,              &gtAdcCalib.fAdcSclACorrectionFactorEnabled[ADC_INPUT_6],     "chan 6 Scaling A correction ENABLED"    },

    { CONFIG_ADC_CHAN_1_SCL_A_CORRECTION_FACTOR_OFFSET,     "CH1SCAOFFST",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },      { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcSclACorrectionFactorOffset[ADC_INPUT_1],     "chan 1 Scaling A offset(mV)"                },
    { CONFIG_ADC_CHAN_2_SCL_A_CORRECTION_FACTOR_OFFSET,     "CH2SCAOFFST",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },      { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcSclACorrectionFactorOffset[ADC_INPUT_2],     "chan 2 Scaling A offset(mV)"                },
    { CONFIG_ADC_CHAN_3_SCL_A_CORRECTION_FACTOR_OFFSET,     "CH3SCAOFFST",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },      { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcSclACorrectionFactorOffset[ADC_INPUT_3],     "chan 3 Scaling A offset(mV)"                },
    { CONFIG_ADC_CHAN_4_SCL_A_CORRECTION_FACTOR_OFFSET,     "CH4SCAOFFST",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },      { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcSclACorrectionFactorOffset[ADC_INPUT_4],     "chan 4 Scaling A offset(mV)"                },
    { CONFIG_ADC_CHAN_5_SCL_A_CORRECTION_FACTOR_OFFSET,     "CH5SCAOFFST",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },      { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcSclACorrectionFactorOffset[ADC_INPUT_5],     "chan 5 Scaling A offset(mV)"                },
    { CONFIG_ADC_CHAN_6_SCL_A_CORRECTION_FACTOR_OFFSET,     "CH6SCAOFFST",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },      { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcSclACorrectionFactorOffset[ADC_INPUT_6],     "chan 6 Scaling A offset(mV)"                },

    { CONFIG_ADC_CHAN_1_SCL_A_CORRECTION_FACTOR_SLOPE,      "CH1SCASLP",    CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -1000.0 },      { .f32 = 1000.0 },                                       { .f32 = 1.0 },            &gtAdcCalib.sgAdcSclACorrectionFactorSlope[ADC_INPUT_1],      "chan 1 Scaling A slope"                 },        
    { CONFIG_ADC_CHAN_2_SCL_A_CORRECTION_FACTOR_SLOPE,      "CH2SCASLP",    CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -1000.0 },      { .f32 = 1000.0 },                                       { .f32 = 1.0 },            &gtAdcCalib.sgAdcSclACorrectionFactorSlope[ADC_INPUT_2],      "chan 2 Scaling A slope"                 },        
    { CONFIG_ADC_CHAN_3_SCL_A_CORRECTION_FACTOR_SLOPE,      "CH3SCASLP",    CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -1000.0 },      { .f32 = 1000.0 },                                       { .f32 = 1.0 },            &gtAdcCalib.sgAdcSclACorrectionFactorSlope[ADC_INPUT_3],      "chan 3 Scaling A slope"                 },        
    { CONFIG_ADC_CHAN_4_SCL_A_CORRECTION_FACTOR_SLOPE,      "CH4SCASLP",    CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -1000.0 },      { .f32 = 1000.0 },                                       { .f32 = 1.0 },            &gtAdcCalib.sgAdcSclACorrectionFactorSlope[ADC_INPUT_4],      "chan 4 Scaling A slope"                 },        
    { CONFIG_ADC_CHAN_5_SCL_A_CORRECTION_FACTOR_SLOPE,      "CH5SCASLP",    CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -1000.0 },      { .f32 = 1000.0 },                                       { .f32 = 1.0 },            &gtAdcCalib.sgAdcSclACorrectionFactorSlope[ADC_INPUT_5],      "chan 5 Scaling A slope"                 },        
    { CONFIG_ADC_CHAN_6_SCL_A_CORRECTION_FACTOR_SLOPE,      "CH6SCASLP",    CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -1000.0 },      { .f32 = 1000.0 },                                       { .f32 = 1.0 },            &gtAdcCalib.sgAdcSclACorrectionFactorSlope[ADC_INPUT_6],      "chan 6 Scaling A slope"                 },        

    // SCALE B
    { CONFIG_ADC_CHAN_1_SCL_B_CAL_EXPECTED_X1_MILLIVOLTS,   "CH1SCBEXPX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 100.0 },           &gtAdcCalib.sgAdcSclBCalibrationExpectedX1_mV[ADC_INPUT_1],   "chan 1 Scaling B cal expected x1 (mV)"       },
    { CONFIG_ADC_CHAN_2_SCL_B_CAL_EXPECTED_X1_MILLIVOLTS,   "CH2SCBEXPX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 100.0 },           &gtAdcCalib.sgAdcSclBCalibrationExpectedX1_mV[ADC_INPUT_2],   "chan 2 Scaling B cal expected x1 (mV)"       },
    { CONFIG_ADC_CHAN_3_SCL_B_CAL_EXPECTED_X1_MILLIVOLTS,   "CH3SCBEXPX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 100.0 },           &gtAdcCalib.sgAdcSclBCalibrationExpectedX1_mV[ADC_INPUT_3],   "chan 3 Scaling B cal expected x1 (mV)"       },
    { CONFIG_ADC_CHAN_4_SCL_B_CAL_EXPECTED_X1_MILLIVOLTS,   "CH4SCBEXPX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 100.0 },           &gtAdcCalib.sgAdcSclBCalibrationExpectedX1_mV[ADC_INPUT_4],   "chan 4 Scaling B cal expected x1 (mV)"       },
    { CONFIG_ADC_CHAN_5_SCL_B_CAL_EXPECTED_X1_MILLIVOLTS,   "CH5SCBEXPX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 100.0 },           &gtAdcCalib.sgAdcSclBCalibrationExpectedX1_mV[ADC_INPUT_5],   "chan 5 Scaling B cal expected x1 (mV)"       },
    { CONFIG_ADC_CHAN_6_SCL_B_CAL_EXPECTED_X1_MILLIVOLTS,   "CH6SCBEXPX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 100.0 },           &gtAdcCalib.sgAdcSclBCalibrationExpectedX1_mV[ADC_INPUT_6],   "chan 6 Scaling B cal expected x1 (mV)"       },

    { CONFIG_ADC_CHAN_1_SCL_B_CAL_EXPECTED_X2_MILLIVOLTS,   "CH1SCBEXPX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 150.0 },           &gtAdcCalib.sgAdcSclBCalibrationExpectedX2_mV[ADC_INPUT_1],   "chan 1 Scaling B cal expected x2 (mV)"       },
    { CONFIG_ADC_CHAN_2_SCL_B_CAL_EXPECTED_X2_MILLIVOLTS,   "CH2SCBEXPX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 150.0 },           &gtAdcCalib.sgAdcSclBCalibrationExpectedX2_mV[ADC_INPUT_2],   "chan 2 Scaling B cal expected x2 (mV)"       },
    { CONFIG_ADC_CHAN_3_SCL_B_CAL_EXPECTED_X2_MILLIVOLTS,   "CH3SCBEXPX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 150.0 },           &gtAdcCalib.sgAdcSclBCalibrationExpectedX2_mV[ADC_INPUT_3],   "chan 3 Scaling B cal expected x2 (mV)"       },
    { CONFIG_ADC_CHAN_4_SCL_B_CAL_EXPECTED_X2_MILLIVOLTS,   "CH4SCBEXPX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 150.0 },           &gtAdcCalib.sgAdcSclBCalibrationExpectedX2_mV[ADC_INPUT_4],   "chan 4 Scaling B cal expected x2 (mV)"       },
    { CONFIG_ADC_CHAN_5_SCL_B_CAL_EXPECTED_X2_MILLIVOLTS,   "CH5SCBEXPX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 150.0 },           &gtAdcCalib.sgAdcSclBCalibrationExpectedX2_mV[ADC_INPUT_5],   "chan 5 Scaling B cal expected x2 (mV)"       },
    { CONFIG_ADC_CHAN_6_SCL_B_CAL_EXPECTED_X2_MILLIVOLTS,   "CH6SCBEXPX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 150.0 },           &gtAdcCalib.sgAdcSclBCalibrationExpectedX2_mV[ADC_INPUT_6],   "chan 6 Scaling B cal expected x2 (mV)"       },

    { CONFIG_ADC_CHAN_1_SCL_B_CAL_OBTAINED_X1_MILLIVOLTS,   "CH1SCBOBTX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcSclBCalibrationObtainedX1_mV[ADC_INPUT_1],   "chan 1 Scaling B cal obtained x1 (mV)"       },
    { CONFIG_ADC_CHAN_2_SCL_B_CAL_OBTAINED_X1_MILLIVOLTS,   "CH2SCBOBTX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcSclBCalibrationObtainedX1_mV[ADC_INPUT_2],   "chan 2 Scaling B cal obtained x1 (mV)"       },
    { CONFIG_ADC_CHAN_3_SCL_B_CAL_OBTAINED_X1_MILLIVOLTS,   "CH3SCBOBTX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcSclBCalibrationObtainedX1_mV[ADC_INPUT_3],   "chan 3 Scaling B cal obtained x1 (mV)"       },
    { CONFIG_ADC_CHAN_4_SCL_B_CAL_OBTAINED_X1_MILLIVOLTS,   "CH4SCBOBTX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcSclBCalibrationObtainedX1_mV[ADC_INPUT_4],   "chan 4 Scaling B cal obtained x1 (mV)"       },
    { CONFIG_ADC_CHAN_5_SCL_B_CAL_OBTAINED_X1_MILLIVOLTS,   "CH5SCBOBTX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcSclBCalibrationObtainedX1_mV[ADC_INPUT_5],   "chan 5 Scaling B cal obtained x1 (mV)"       },
    { CONFIG_ADC_CHAN_6_SCL_B_CAL_OBTAINED_X1_MILLIVOLTS,   "CH6SCBOBTX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcSclBCalibrationObtainedX1_mV[ADC_INPUT_6],   "chan 6 Scaling B cal obtained x1 (mV)"       },

    { CONFIG_ADC_CHAN_1_SCL_B_CAL_OBTAINED_X2_MILLIVOLTS,   "CH1SCBOBTX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcSclBCalibrationObtainedX2_mV[ADC_INPUT_1],   "chan 1 Scaling B cal obtained x2 (mV)"       },    
    { CONFIG_ADC_CHAN_2_SCL_B_CAL_OBTAINED_X2_MILLIVOLTS,   "CH2SCBOBTX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcSclBCalibrationObtainedX2_mV[ADC_INPUT_2],   "chan 2 Scaling B cal obtained x2 (mV)"       },    
    { CONFIG_ADC_CHAN_3_SCL_B_CAL_OBTAINED_X2_MILLIVOLTS,   "CH3SCBOBTX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcSclBCalibrationObtainedX2_mV[ADC_INPUT_3],   "chan 3 Scaling B cal obtained x2 (mV)"       },    
    { CONFIG_ADC_CHAN_4_SCL_B_CAL_OBTAINED_X2_MILLIVOLTS,   "CH4SCBOBTX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcSclBCalibrationObtainedX2_mV[ADC_INPUT_4],   "chan 4 Scaling B cal obtained x2 (mV)"       },    
    { CONFIG_ADC_CHAN_5_SCL_B_CAL_OBTAINED_X2_MILLIVOLTS,   "CH5SCBOBTX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcSclBCalibrationObtainedX2_mV[ADC_INPUT_5],   "chan 5 Scaling B cal obtained x2 (mV)"       },    
    { CONFIG_ADC_CHAN_6_SCL_B_CAL_OBTAINED_X2_MILLIVOLTS,   "CH6SCBOBTX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcSclBCalibrationObtainedX2_mV[ADC_INPUT_6],   "chan 6 Scaling B cal obtained x2 (mV)"       },    

//    { CONFIG_ADC_CHAN_1_SCL_B_CORRECTION_FACTOR_ENABLED,    "CH1SCBCOREN",  CONFIG_PARAM_TYPE_BOOL,     0,          1,                                            0,              &gtAdcCalib.fAdcSclBCorrectionFactorEnabled[ADC_INPUT_1], "chan 1 Scaling B correction ENABLED"    },
//    { CONFIG_ADC_CHAN_2_SCL_B_CORRECTION_FACTOR_ENABLED,    "CH2SCBCOREN",  CONFIG_PARAM_TYPE_BOOL,     0,          1,                                            0,              &gtAdcCalib.fAdcSclBCorrectionFactorEnabled[ADC_INPUT_2], "chan 2 Scaling B correction ENABLED"    },
//    { CONFIG_ADC_CHAN_3_SCL_B_CORRECTION_FACTOR_ENABLED,    "CH3SCBCOREN",  CONFIG_PARAM_TYPE_BOOL,     0,          1,                                            0,              &gtAdcCalib.fAdcSclBCorrectionFactorEnabled[ADC_INPUT_3], "chan 3 Scaling B correction ENABLED"    },
//    { CONFIG_ADC_CHAN_4_SCL_B_CORRECTION_FACTOR_ENABLED,    "CH4SCBCOREN",  CONFIG_PARAM_TYPE_BOOL,     0,          1,                                            0,              &gtAdcCalib.fAdcSclBCorrectionFactorEnabled[ADC_INPUT_4], "chan 4 Scaling B correction ENABLED"    },
//    { CONFIG_ADC_CHAN_5_SCL_B_CORRECTION_FACTOR_ENABLED,    "CH5SCBCOREN",  CONFIG_PARAM_TYPE_BOOL,     0,          1,                                            0,              &gtAdcCalib.fAdcSclBCorrectionFactorEnabled[ADC_INPUT_5], "chan 5 Scaling B correction ENABLED"    },
//    { CONFIG_ADC_CHAN_6_SCL_B_CORRECTION_FACTOR_ENABLED,    "CH6SCBCOREN",  CONFIG_PARAM_TYPE_BOOL,     0,          1,                                            0,              &gtAdcCalib.fAdcSclBCorrectionFactorEnabled[ADC_INPUT_6], "chan 6 Scaling B correction ENABLED"    },

    { CONFIG_ADC_CHAN_1_SCL_B_CORRECTION_FACTOR_OFFSET,     "CH1SCBOFFST",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcSclBCorrectionFactorOffset[ADC_INPUT_1], "chan 1 Scaling B offset(mV)"                },
    { CONFIG_ADC_CHAN_2_SCL_B_CORRECTION_FACTOR_OFFSET,     "CH2SCBOFFST",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcSclBCorrectionFactorOffset[ADC_INPUT_2], "chan 2 Scaling B offset(mV)"                },
    { CONFIG_ADC_CHAN_3_SCL_B_CORRECTION_FACTOR_OFFSET,     "CH3SCBOFFST",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcSclBCorrectionFactorOffset[ADC_INPUT_3], "chan 3 Scaling B offset(mV)"                },
    { CONFIG_ADC_CHAN_4_SCL_B_CORRECTION_FACTOR_OFFSET,     "CH4SCBOFFST",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcSclBCorrectionFactorOffset[ADC_INPUT_4], "chan 4 Scaling B offset(mV)"                },
    { CONFIG_ADC_CHAN_5_SCL_B_CORRECTION_FACTOR_OFFSET,     "CH5SCBOFFST",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcSclBCorrectionFactorOffset[ADC_INPUT_5], "chan 5 Scaling B offset(mV)"                },
    { CONFIG_ADC_CHAN_6_SCL_B_CORRECTION_FACTOR_OFFSET,     "CH6SCBOFFST",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdcSclBCorrectionFactorOffset[ADC_INPUT_6], "chan 6 Scaling B offset(mV)"                },

    { CONFIG_ADC_CHAN_1_SCL_B_CORRECTION_FACTOR_SLOPE,      "CH1SCBSLP",    CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -1000.0 },      { .f32 = 1000.0 },                                       { .f32 = 1.0 },             &gtAdcCalib.sgAdcSclBCorrectionFactorSlope[ADC_INPUT_1],  "chan 1 Scaling B slope"                 },            
    { CONFIG_ADC_CHAN_2_SCL_B_CORRECTION_FACTOR_SLOPE,      "CH2SCBSLP",    CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -1000.0 },      { .f32 = 1000.0 },                                       { .f32 = 1.0 },             &gtAdcCalib.sgAdcSclBCorrectionFactorSlope[ADC_INPUT_2],  "chan 2 Scaling B slope"                 },            
    { CONFIG_ADC_CHAN_3_SCL_B_CORRECTION_FACTOR_SLOPE,      "CH3SCBSLP",    CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -1000.0 },      { .f32 = 1000.0 },                                       { .f32 = 1.0 },             &gtAdcCalib.sgAdcSclBCorrectionFactorSlope[ADC_INPUT_3],  "chan 3 Scaling B slope"                 },            
    { CONFIG_ADC_CHAN_4_SCL_B_CORRECTION_FACTOR_SLOPE,      "CH4SCBSLP",    CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -1000.0 },      { .f32 = 1000.0 },                                       { .f32 = 1.0 },             &gtAdcCalib.sgAdcSclBCorrectionFactorSlope[ADC_INPUT_4],  "chan 4 Scaling B slope"                 },            
    { CONFIG_ADC_CHAN_5_SCL_B_CORRECTION_FACTOR_SLOPE,      "CH5SCBSLP",    CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -1000.0 },      { .f32 = 1000.0 },                                       { .f32 = 1.0 },             &gtAdcCalib.sgAdcSclBCorrectionFactorSlope[ADC_INPUT_5],  "chan 5 Scaling B slope"                 },            
    { CONFIG_ADC_CHAN_6_SCL_B_CORRECTION_FACTOR_SLOPE,      "CH6SCBSLP",    CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -1000.0 },      { .f32 = 1000.0 },                                       { .f32 = 1.0 },             &gtAdcCalib.sgAdcSclBCorrectionFactorSlope[ADC_INPUT_6],  "chan 6 Scaling B slope"                 },            

    // 4-20MA
    { CONFIG_ADC_CHAN_1_420MA_CAL_EXPECTED_X1_MILLIVOLTS,   "CH1420EXPX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 100.0 },           &gtAdcCalib.sgAdc420mACalibrationExpectedX1_mV[ADC_INPUT_1],   "chan 1 4-20mA cal expected x1 (mV)"       },
    { CONFIG_ADC_CHAN_2_420MA_CAL_EXPECTED_X1_MILLIVOLTS,   "CH2420EXPX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 100.0 },           &gtAdcCalib.sgAdc420mACalibrationExpectedX1_mV[ADC_INPUT_2],   "chan 2 4-20mA cal expected x1 (mV)"       },
    { CONFIG_ADC_CHAN_3_420MA_CAL_EXPECTED_X1_MILLIVOLTS,   "CH3420EXPX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 100.0 },           &gtAdcCalib.sgAdc420mACalibrationExpectedX1_mV[ADC_INPUT_3],   "chan 3 4-20mA cal expected x1 (mV)"       },
    { CONFIG_ADC_CHAN_4_420MA_CAL_EXPECTED_X1_MILLIVOLTS,   "CH4420EXPX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 100.0 },           &gtAdcCalib.sgAdc420mACalibrationExpectedX1_mV[ADC_INPUT_4],   "chan 4 4-20mA cal expected x1 (mV)"       },
    { CONFIG_ADC_CHAN_5_420MA_CAL_EXPECTED_X1_MILLIVOLTS,   "CH5420EXPX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 100.0 },           &gtAdcCalib.sgAdc420mACalibrationExpectedX1_mV[ADC_INPUT_5],   "chan 5 4-20mA cal expected x1 (mV)"       },
    { CONFIG_ADC_CHAN_6_420MA_CAL_EXPECTED_X1_MILLIVOLTS,   "CH6420EXPX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 100.0 },           &gtAdcCalib.sgAdc420mACalibrationExpectedX1_mV[ADC_INPUT_6],   "chan 6 4-20mA cal expected x1 (mV)"       },

    { CONFIG_ADC_CHAN_1_420MA_CAL_EXPECTED_X2_MILLIVOLTS,   "CH1420EXPX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 150.0 },           &gtAdcCalib.sgAdc420mACalibrationExpectedX2_mV[ADC_INPUT_1],   "chan 1 4-20mA cal expected x2 (mV)"       },
    { CONFIG_ADC_CHAN_2_420MA_CAL_EXPECTED_X2_MILLIVOLTS,   "CH2420EXPX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 150.0 },           &gtAdcCalib.sgAdc420mACalibrationExpectedX2_mV[ADC_INPUT_2],   "chan 2 4-20mA cal expected x2 (mV)"       },
    { CONFIG_ADC_CHAN_3_420MA_CAL_EXPECTED_X2_MILLIVOLTS,   "CH3420EXPX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 150.0 },           &gtAdcCalib.sgAdc420mACalibrationExpectedX2_mV[ADC_INPUT_3],   "chan 3 4-20mA cal expected x2 (mV)"       },
    { CONFIG_ADC_CHAN_4_420MA_CAL_EXPECTED_X2_MILLIVOLTS,   "CH4420EXPX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 150.0 },           &gtAdcCalib.sgAdc420mACalibrationExpectedX2_mV[ADC_INPUT_4],   "chan 4 4-20mA cal expected x2 (mV)"       },
    { CONFIG_ADC_CHAN_5_420MA_CAL_EXPECTED_X2_MILLIVOLTS,   "CH5420EXPX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 150.0 },           &gtAdcCalib.sgAdc420mACalibrationExpectedX2_mV[ADC_INPUT_5],   "chan 5 4-20mA cal expected x2 (mV)"       },
    { CONFIG_ADC_CHAN_6_420MA_CAL_EXPECTED_X2_MILLIVOLTS,   "CH6420EXPX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 150.0 },           &gtAdcCalib.sgAdc420mACalibrationExpectedX2_mV[ADC_INPUT_6],   "chan 6 4-20mA cal expected x2 (mV)"       },

    { CONFIG_ADC_CHAN_1_420MA_CAL_OBTAINED_X1_MILLIVOLTS,   "CH1420OBTX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdc420mACalibrationObtainedX1_mV[ADC_INPUT_1],   "chan 1 4-20mA cal obtained x1 (mV)"       },
    { CONFIG_ADC_CHAN_2_420MA_CAL_OBTAINED_X1_MILLIVOLTS,   "CH2420OBTX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdc420mACalibrationObtainedX1_mV[ADC_INPUT_2],   "chan 2 4-20mA cal obtained x1 (mV)"       },
    { CONFIG_ADC_CHAN_3_420MA_CAL_OBTAINED_X1_MILLIVOLTS,   "CH3420OBTX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdc420mACalibrationObtainedX1_mV[ADC_INPUT_3],   "chan 3 4-20mA cal obtained x1 (mV)"       },
    { CONFIG_ADC_CHAN_4_420MA_CAL_OBTAINED_X1_MILLIVOLTS,   "CH4420OBTX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdc420mACalibrationObtainedX1_mV[ADC_INPUT_4],   "chan 4 4-20mA cal obtained x1 (mV)"       },
    { CONFIG_ADC_CHAN_5_420MA_CAL_OBTAINED_X1_MILLIVOLTS,   "CH5420OBTX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdc420mACalibrationObtainedX1_mV[ADC_INPUT_5],   "chan 5 4-20mA cal obtained x1 (mV)"       },
    { CONFIG_ADC_CHAN_6_420MA_CAL_OBTAINED_X1_MILLIVOLTS,   "CH6420OBTX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdc420mACalibrationObtainedX1_mV[ADC_INPUT_6],   "chan 6 4-20mA cal obtained x1 (mV)"       },

    { CONFIG_ADC_CHAN_1_420MA_CAL_OBTAINED_X2_MILLIVOLTS,   "CH1420OBTX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdc420mACalibrationObtainedX2_mV[ADC_INPUT_1],   "chan 1 4-20mA cal obtained x2 (mV)"       },    
    { CONFIG_ADC_CHAN_2_420MA_CAL_OBTAINED_X2_MILLIVOLTS,   "CH2420OBTX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdc420mACalibrationObtainedX2_mV[ADC_INPUT_2],   "chan 2 4-20mA cal obtained x2 (mV)"       },    
    { CONFIG_ADC_CHAN_3_420MA_CAL_OBTAINED_X2_MILLIVOLTS,   "CH3420OBTX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdc420mACalibrationObtainedX2_mV[ADC_INPUT_3],   "chan 3 4-20mA cal obtained x2 (mV)"       },    
    { CONFIG_ADC_CHAN_4_420MA_CAL_OBTAINED_X2_MILLIVOLTS,   "CH4420OBTX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdc420mACalibrationObtainedX2_mV[ADC_INPUT_4],   "chan 4 4-20mA cal obtained x2 (mV)"       },    
    { CONFIG_ADC_CHAN_5_420MA_CAL_OBTAINED_X2_MILLIVOLTS,   "CH5420OBTX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdc420mACalibrationObtainedX2_mV[ADC_INPUT_5],   "chan 5 4-20mA cal obtained x2 (mV)"       },    
    { CONFIG_ADC_CHAN_6_420MA_CAL_OBTAINED_X2_MILLIVOLTS,   "CH6420OBTX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = 0.0 },         { .f32 = 5000.0 },                                        { .f32 = 0.0 },             &gtAdcCalib.sgAdc420mACalibrationObtainedX2_mV[ADC_INPUT_6],   "chan 6 4-20mA cal obtained x2 (mV)"       },    

//    { CONFIG_ADC_CHAN_1_420MA_CORRECTION_FACTOR_ENABLED,    "CH1420COREN",  CONFIG_PARAM_TYPE_BOOL,     0,          1,                                            0,              &gtAdcCalib.fAdc420mACorrectionFactorEnabled[ADC_INPUT_1],"chan 1 4-20mA correction ENABLED"    },
//    { CONFIG_ADC_CHAN_2_420MA_CORRECTION_FACTOR_ENABLED,    "CH2420COREN",  CONFIG_PARAM_TYPE_BOOL,     0,          1,                                            0,              &gtAdcCalib.fAdc420mACorrectionFactorEnabled[ADC_INPUT_2],"chan 2 4-20mA correction ENABLED"    },
//    { CONFIG_ADC_CHAN_3_420MA_CORRECTION_FACTOR_ENABLED,    "CH3420COREN",  CONFIG_PARAM_TYPE_BOOL,     0,          1,                                            0,              &gtAdcCalib.fAdc420mACorrectionFactorEnabled[ADC_INPUT_3],"chan 3 4-20mA correction ENABLED"    },
//    { CONFIG_ADC_CHAN_4_420MA_CORRECTION_FACTOR_ENABLED,    "CH4420COREN",  CONFIG_PARAM_TYPE_BOOL,     0,          1,                                            0,              &gtAdcCalib.fAdc420mACorrectionFactorEnabled[ADC_INPUT_4],"chan 4 4-20mA correction ENABLED"    },
//    { CONFIG_ADC_CHAN_5_420MA_CORRECTION_FACTOR_ENABLED,    "CH5420COREN",  CONFIG_PARAM_TYPE_BOOL,     0,          1,                                            0,              &gtAdcCalib.fAdc420mACorrectionFactorEnabled[ADC_INPUT_5],"chan 5 4-20mA correction ENABLED"    },
//    { CONFIG_ADC_CHAN_6_420MA_CORRECTION_FACTOR_ENABLED,    "CH6420COREN",  CONFIG_PARAM_TYPE_BOOL,     0,          1,                                            0,              &gtAdcCalib.fAdc420mACorrectionFactorEnabled[ADC_INPUT_6],"chan 6 4-20mA correction ENABLED"    },

    { CONFIG_ADC_CHAN_1_420MA_CORRECTION_FACTOR_OFFSET,     "CH1420OFFST",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },      { .f32 = 5000.0 },                                       { .f32 = 0.0 },             &gtAdcCalib.sgAdc420mACorrectionFactorOffset[ADC_INPUT_1], "chan 1 4-20mA offset(mV)"                },
    { CONFIG_ADC_CHAN_2_420MA_CORRECTION_FACTOR_OFFSET,     "CH2420OFFST",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },      { .f32 = 5000.0 },                                       { .f32 = 0.0 },             &gtAdcCalib.sgAdc420mACorrectionFactorOffset[ADC_INPUT_2], "chan 2 4-20mA offset(mV)"                },
    { CONFIG_ADC_CHAN_3_420MA_CORRECTION_FACTOR_OFFSET,     "CH3420OFFST",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },      { .f32 = 5000.0 },                                       { .f32 = 0.0 },             &gtAdcCalib.sgAdc420mACorrectionFactorOffset[ADC_INPUT_3], "chan 3 4-20mA offset(mV)"                },
    { CONFIG_ADC_CHAN_4_420MA_CORRECTION_FACTOR_OFFSET,     "CH4420OFFST",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },      { .f32 = 5000.0 },                                       { .f32 = 0.0 },             &gtAdcCalib.sgAdc420mACorrectionFactorOffset[ADC_INPUT_4], "chan 4 4-20mA offset(mV)"                },
    { CONFIG_ADC_CHAN_5_420MA_CORRECTION_FACTOR_OFFSET,     "CH5420OFFST",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },      { .f32 = 5000.0 },                                       { .f32 = 0.0 },             &gtAdcCalib.sgAdc420mACorrectionFactorOffset[ADC_INPUT_5], "chan 5 4-20mA offset(mV)"                },
    { CONFIG_ADC_CHAN_6_420MA_CORRECTION_FACTOR_OFFSET,     "CH6420OFFST",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },      { .f32 = 5000.0 },                                       { .f32 = 0.0 },             &gtAdcCalib.sgAdc420mACorrectionFactorOffset[ADC_INPUT_6], "chan 6 4-20mA offset(mV)"                },

    { CONFIG_ADC_CHAN_1_420MA_CORRECTION_FACTOR_SLOPE,      "CH1420SLP",    CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -1000.0 },      { .f32 = 1000.0 },                                       { .f32 = 1.0 },           &gtAdcCalib.sgAdc420mACorrectionFactorSlope[ADC_INPUT_1], "chan 1 4-20mA slope"                 },
    { CONFIG_ADC_CHAN_2_420MA_CORRECTION_FACTOR_SLOPE,      "CH2420SLP",    CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -1000.0 },      { .f32 = 1000.0 },                                       { .f32 = 1.0 },           &gtAdcCalib.sgAdc420mACorrectionFactorSlope[ADC_INPUT_2], "chan 2 4-20mA slope"                 },
    { CONFIG_ADC_CHAN_3_420MA_CORRECTION_FACTOR_SLOPE,      "CH3420SLP",    CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -1000.0 },      { .f32 = 1000.0 },                                       { .f32 = 1.0 },           &gtAdcCalib.sgAdc420mACorrectionFactorSlope[ADC_INPUT_3], "chan 3 4-20mA slope"                 },
    { CONFIG_ADC_CHAN_4_420MA_CORRECTION_FACTOR_SLOPE,      "CH4420SLP",    CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -1000.0 },      { .f32 = 1000.0 },                                       { .f32 = 1.0 },           &gtAdcCalib.sgAdc420mACorrectionFactorSlope[ADC_INPUT_4], "chan 4 4-20mA slope"                 },
    { CONFIG_ADC_CHAN_5_420MA_CORRECTION_FACTOR_SLOPE,      "CH5420SLP",    CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -1000.0 },      { .f32 = 1000.0 },                                       { .f32 = 1.0 },           &gtAdcCalib.sgAdc420mACorrectionFactorSlope[ADC_INPUT_5], "chan 5 4-20mA slope"                 },
    { CONFIG_ADC_CHAN_6_420MA_CORRECTION_FACTOR_SLOPE,      "CH6420SLP",    CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -1000.0 },      { .f32 = 1000.0 },                                       { .f32 = 1.0 },           &gtAdcCalib.sgAdc420mACorrectionFactorSlope[ADC_INPUT_6], "chan 6 4-20mA slope"                 },

    
    // sensor expected/obtained cal vals
    { CONFIG_SENSOR_CHAN_1_CAL_EXPECTED_X1,                 "SENCH1EXPX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },           &gtAdcCalib.sgSensorCalibrationExpectedX1[ADC_INPUT_1],   "Sensor chan 1 cal expected x1"       },
    { CONFIG_SENSOR_CHAN_2_CAL_EXPECTED_X1,                 "SENCH2EXPX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },           &gtAdcCalib.sgSensorCalibrationExpectedX1[ADC_INPUT_2],   "Sensor chan 2 cal expected x1"       },
    { CONFIG_SENSOR_CHAN_3_CAL_EXPECTED_X1,                 "SENCH3EXPX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },           &gtAdcCalib.sgSensorCalibrationExpectedX1[ADC_INPUT_3],   "Sensor chan 3 cal expected x1"       },
    { CONFIG_SENSOR_CHAN_4_CAL_EXPECTED_X1,                 "SENCH4EXPX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },           &gtAdcCalib.sgSensorCalibrationExpectedX1[ADC_INPUT_4],   "Sensor chan 4 cal expected x1"       },
    { CONFIG_SENSOR_CHAN_5_CAL_EXPECTED_X1,                 "SENCH5EXPX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },           &gtAdcCalib.sgSensorCalibrationExpectedX1[ADC_INPUT_5],   "Sensor chan 5 cal expected x1"       },
    { CONFIG_SENSOR_CHAN_6_CAL_EXPECTED_X1,                 "SENCH6EXPX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },           &gtAdcCalib.sgSensorCalibrationExpectedX1[ADC_INPUT_6],   "Sensor chan 5 cal expected x1"       },

    { CONFIG_SENSOR_CHAN_1_CAL_EXPECTED_X2,                 "SENCH1EXPX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },           &gtAdcCalib.sgSensorCalibrationExpectedX2[ADC_INPUT_1],   "Sensor chan 1 cal expected x2"       },
    { CONFIG_SENSOR_CHAN_2_CAL_EXPECTED_X2,                 "SENCH2EXPX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },           &gtAdcCalib.sgSensorCalibrationExpectedX2[ADC_INPUT_2],   "Sensor chan 2 cal expected x2"       },
    { CONFIG_SENSOR_CHAN_3_CAL_EXPECTED_X2,                 "SENCH3EXPX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },           &gtAdcCalib.sgSensorCalibrationExpectedX2[ADC_INPUT_3],   "Sensor chan 3 cal expected x2"       },
    { CONFIG_SENSOR_CHAN_4_CAL_EXPECTED_X2,                 "SENCH4EXPX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },           &gtAdcCalib.sgSensorCalibrationExpectedX2[ADC_INPUT_4],   "Sensor chan 4 cal expected x2"       },
    { CONFIG_SENSOR_CHAN_5_CAL_EXPECTED_X2,                 "SENCH5EXPX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },           &gtAdcCalib.sgSensorCalibrationExpectedX2[ADC_INPUT_5],   "Sensor chan 5 cal expected x2"       },
    { CONFIG_SENSOR_CHAN_6_CAL_EXPECTED_X2,                 "SENCH6EXPX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },           &gtAdcCalib.sgSensorCalibrationExpectedX2[ADC_INPUT_6],   "Sensor chan 6 cal expected x2"       },

    { CONFIG_SENSOR_CHAN_1_CAL_OBTAINED_X1,                 "SENCH1OBTX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },			&gtAdcCalib.sgSensorCalibrationObtainedX1[ADC_INPUT_1],   "Sensor chan 1 cal obtained x1"       },
    { CONFIG_SENSOR_CHAN_2_CAL_OBTAINED_X1,                 "SENCH2OBTX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },			&gtAdcCalib.sgSensorCalibrationObtainedX1[ADC_INPUT_2],   "Sensor chan 2 cal obtained x1"       },
    { CONFIG_SENSOR_CHAN_3_CAL_OBTAINED_X1,                 "SENCH3OBTX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },			&gtAdcCalib.sgSensorCalibrationObtainedX1[ADC_INPUT_3],   "Sensor chan 3 cal obtained x1"       },
    { CONFIG_SENSOR_CHAN_4_CAL_OBTAINED_X1,                 "SENCH4OBTX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },			&gtAdcCalib.sgSensorCalibrationObtainedX1[ADC_INPUT_4],   "Sensor chan 4 cal obtained x1"       },
    { CONFIG_SENSOR_CHAN_5_CAL_OBTAINED_X1,                 "SENCH5OBTX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },			&gtAdcCalib.sgSensorCalibrationObtainedX1[ADC_INPUT_5],   "Sensor chan 5 cal obtained x1"       },
    { CONFIG_SENSOR_CHAN_6_CAL_OBTAINED_X1,                 "SENCH6OBTX1",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },			&gtAdcCalib.sgSensorCalibrationObtainedX1[ADC_INPUT_6],   "Sensor chan 6 cal obtained x1"       },

    { CONFIG_SENSOR_CHAN_1_CAL_OBTAINED_X2,                 "SENCH1OBTX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },			&gtAdcCalib.sgSensorCalibrationObtainedX2[ADC_INPUT_1],   "Sensor chan 1 cal obtained x2"       },
    { CONFIG_SENSOR_CHAN_2_CAL_OBTAINED_X2,                 "SENCH2OBTX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },			&gtAdcCalib.sgSensorCalibrationObtainedX2[ADC_INPUT_2],   "Sensor chan 2 cal obtained x2"       },
    { CONFIG_SENSOR_CHAN_3_CAL_OBTAINED_X2,                 "SENCH3OBTX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },			&gtAdcCalib.sgSensorCalibrationObtainedX2[ADC_INPUT_3],   "Sensor chan 3 cal obtained x2"       },
    { CONFIG_SENSOR_CHAN_4_CAL_OBTAINED_X2,                 "SENCH4OBTX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },			&gtAdcCalib.sgSensorCalibrationObtainedX2[ADC_INPUT_4],   "Sensor chan 4 cal obtained x2"       },
    { CONFIG_SENSOR_CHAN_5_CAL_OBTAINED_X2,                 "SENCH5OBTX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },           &gtAdcCalib.sgSensorCalibrationObtainedX2[ADC_INPUT_5],   "Sensor chan 5 cal obtained x2"       },
    { CONFIG_SENSOR_CHAN_6_CAL_OBTAINED_X2,                 "SENCH6OBTX2",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },     { .f32 = 5000.0 },                                        { .f32 = 0.0 },			&gtAdcCalib.sgSensorCalibrationObtainedX2[ADC_INPUT_6],   "Sensor chan 6 cal obtained x2"       },
    // Sensor offset
    { CONFIG_SENSOR_CHAN_1_CORRECTION_FACTOR_OFFSET,        "SENCH1OFFST",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },      { .f32 = 5000.0 },                                       { .f32 = 0.0 },           &gtAdcCalib.sgSensorCorrectionFactorOffset[ADC_INPUT_1],    "sensor chan 1 offset(converted units)"                },
    { CONFIG_SENSOR_CHAN_2_CORRECTION_FACTOR_OFFSET,        "SENCH2OFFST",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },      { .f32 = 5000.0 },                                       { .f32 = 0.0 },           &gtAdcCalib.sgSensorCorrectionFactorOffset[ADC_INPUT_2],    "sensor chan 2 offset(converted units)"                },
    { CONFIG_SENSOR_CHAN_3_CORRECTION_FACTOR_OFFSET,        "SENCH3OFFST",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },      { .f32 = 5000.0 },                                       { .f32 = 0.0 },           &gtAdcCalib.sgSensorCorrectionFactorOffset[ADC_INPUT_3],    "sensor chan 3 offset(converted units)"                },
    { CONFIG_SENSOR_CHAN_4_CORRECTION_FACTOR_OFFSET,        "SENCH4OFFST",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },      { .f32 = 5000.0 },                                       { .f32 = 0.0 },           &gtAdcCalib.sgSensorCorrectionFactorOffset[ADC_INPUT_4],    "sensor chan 4 offset(converted units)"                },
    { CONFIG_SENSOR_CHAN_5_CORRECTION_FACTOR_OFFSET,        "SENCH5OFFST",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },      { .f32 = 5000.0 },                                       { .f32 = 0.0 },           &gtAdcCalib.sgSensorCorrectionFactorOffset[ADC_INPUT_5],    "sensor chan 5 offset(converted units)"                },
    { CONFIG_SENSOR_CHAN_6_CORRECTION_FACTOR_OFFSET,        "SENCH6OFFST",  CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -5000.0 },      { .f32 = 5000.0 },                                       { .f32 = 0.0 },           &gtAdcCalib.sgSensorCorrectionFactorOffset[ADC_INPUT_6],    "sensor chan 6 offset(converted units)"                },
    // Sensor slope
    { CONFIG_SENSOR_CHAN_1_CORRECTION_FACTOR_SLOPE,         "SENCH1SLP",    CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -1000.0 },      { .f32 = 1000.0 },                                       { .f32 = 1.0 },           &gtAdcCalib.sgSensorCorrectionFactorSlope[ADC_INPUT_1],     "sensor chan 1 slope"                },
    { CONFIG_SENSOR_CHAN_2_CORRECTION_FACTOR_SLOPE,         "SENCH2SLP",    CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -1000.0 },      { .f32 = 1000.0 },                                       { .f32 = 1.0 },           &gtAdcCalib.sgSensorCorrectionFactorSlope[ADC_INPUT_2],     "sensor chan 2 slope"                },
    { CONFIG_SENSOR_CHAN_3_CORRECTION_FACTOR_SLOPE,         "SENCH3SLP",    CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -1000.0 },      { .f32 = 1000.0 },                                       { .f32 = 1.0 },           &gtAdcCalib.sgSensorCorrectionFactorSlope[ADC_INPUT_3],     "sensor chan 3 slope"                },
    { CONFIG_SENSOR_CHAN_4_CORRECTION_FACTOR_SLOPE,         "SENCH4SLP",    CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -1000.0 },      { .f32 = 1000.0 },                                       { .f32 = 1.0 },           &gtAdcCalib.sgSensorCorrectionFactorSlope[ADC_INPUT_4],     "sensor chan 4 slope"                },
    { CONFIG_SENSOR_CHAN_5_CORRECTION_FACTOR_SLOPE,         "SENCH5SLP",    CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -1000.0 },      { .f32 = 1000.0 },                                       { .f32 = 1.0 },           &gtAdcCalib.sgSensorCorrectionFactorSlope[ADC_INPUT_5],     "sensor chan 5 slope"                },
    { CONFIG_SENSOR_CHAN_6_CORRECTION_FACTOR_SLOPE,         "SENCH6SLP",    CONFIG_PARAM_TYPE_FLOAT,    { .f32 = -1000.0 },      { .f32 = 1000.0 },                                       { .f32 = 1.0 },           &gtAdcCalib.sgSensorCorrectionFactorSlope[ADC_INPUT_6],     "sensor chan 6 slope"                },
    // Sensor Initialization Delays after Power-up
    { CONFIG_SENSOR_CHAN_1_INITIALIZATION_DELAYMS,			"SENCH1DELAYMS",CONFIG_PARAM_TYPE_INT16,	{-1},                   {32767},                                                {-1},						&gtAdcCalib.wSensorInitializationDelayMS[ADC_INPUT_1],     "sensor chan 1 delay"                },
    { CONFIG_SENSOR_CHAN_2_INITIALIZATION_DELAYMS,			"SENCH2DELAYMS",CONFIG_PARAM_TYPE_INT16,	{-1},                   {32767},                                                {-1},						&gtAdcCalib.wSensorInitializationDelayMS[ADC_INPUT_2],     "sensor chan 2 delay"                },
    { CONFIG_SENSOR_CHAN_3_INITIALIZATION_DELAYMS,			"SENCH3DELAYMS",CONFIG_PARAM_TYPE_INT16,	{-1},                   {32767},                                                {-1},						&gtAdcCalib.wSensorInitializationDelayMS[ADC_INPUT_3],     "sensor chan 3 delay"                },
    { CONFIG_SENSOR_CHAN_4_INITIALIZATION_DELAYMS,			"SENCH4DELAYMS",CONFIG_PARAM_TYPE_INT16,	{-1},                   {32767},                                                {-1},						&gtAdcCalib.wSensorInitializationDelayMS[ADC_INPUT_4],     "sensor chan 4 delay"                },
    { CONFIG_SENSOR_CHAN_5_INITIALIZATION_DELAYMS,			"SENCH5DELAYMS",CONFIG_PARAM_TYPE_INT16,	{-1},                   {32767},                                                {-1},						&gtAdcCalib.wSensorInitializationDelayMS[ADC_INPUT_5],     "sensor chan 5 delay"                },
    { CONFIG_SENSOR_CHAN_6_INITIALIZATION_DELAYMS,			"SENCH6DELAYMS",CONFIG_PARAM_TYPE_INT16,	{-1},                   {32767},                                                {-1},						&gtAdcCalib.wSensorInitializationDelayMS[ADC_INPUT_6],     "sensor chan 6 delay"                },

};

#define CONFIG_CALIBRATION_NUM_OF_ELEMENTS     (sizeof(gtConfigCalibrationMeta)/sizeof(gtConfigCalibrationMeta[0]))

static const ConfigParamInfoType 		gtConfigCommunicationsMeta[] = 
{
    // Param Id,                                    Param Name,             Param Type,                 Param Min,  Param Max,                                      Param Default,  Param Data,                                             Param Description
    // NOTE:  For strings, the max field contains the string length 
    // WARNING: min max values can be integers even if the Data is of type FLOAT!
        
    // APN
    { CONFIG_DI_MODEM_APN_ADDRESS_STRING,                   "MODEMAPN",     CONFIG_PARAM_TYPE_STRING,   {0},        {CONFIG_PARAM_STRING_LENGTH_URL},               {0},            &gtComm.cModemApnAddress[0],                            "Connection APN In Use"        },
    { CONFIG_DI_MODEM_APN_USERNAME_STRING,                  "MODEMUSER",    CONFIG_PARAM_TYPE_STRING,   {0},        {CONFIG_PARAM_STRING_LENGTH_URL},               {0},            &gtComm.cModemApnName[0],                               "Connection User Name"          },
    { CONFIG_DI_MODEM_APN_PASSWORD_STRING,                  "MODEMPW",      CONFIG_PARAM_TYPE_STRING,   {0},        {CONFIG_PARAM_STRING_LENGTH_URL},               {0},            &gtComm.cModemApnPassword[0],                           "Connection Password"      },

    // FTP settings                                         
    { CONFIG_DI_FTP_ADDRESS_STRING,                         "FTPADDR",      CONFIG_PARAM_TYPE_STRING,   {0},        {CONFIG_PARAM_STRING_LENGTH_URL},               {0},            &gtComm.cFtpAddress[0],                                 "FTP Address in Use"      },
    { CONFIG_DI_FTP_PORT_UINT16,                            "FTPPORT",      CONFIG_PARAM_TYPE_UINT16,   {0},        {65535},                                        {0},            &gtComm.wFtpPort,                                       "FTP Port in Use"         },
    { CONFIG_DI_FTP_NAME_STRING,                            "FTPNAME",      CONFIG_PARAM_TYPE_STRING,   {0},        {CONFIG_PARAM_STRING_LENGTH_URL},               {0},            &gtComm.cFtpName[0],                                    "FTP Login Name in Use"   },
    { CONFIG_DI_FTP_PASSWORD_STRING,                        "FTPPW",        CONFIG_PARAM_TYPE_STRING,   {0},        {CONFIG_PARAM_STRING_LENGTH_URL},               {0},            &gtComm.cFtpPassword[0],                                "FTP Login PW in Use"     },

    // Socket setting
    { CONFIG_DI_SOCKET_PORT_UINT32,                         "SOCKTPORT",    CONFIG_PARAM_TYPE_UINT32,   {0},        {0xFFffFFff},                                   {0},            &gtComm.dwSocketPort,                                    "Socket Port"   },
    { CONFIG_DI_SOCKET_ADDRESS_STRING,                      "SOCKTADDR",    CONFIG_PARAM_TYPE_STRING,   {0},        {CONFIG_PARAM_STRING_LENGTH_URL},               {0},            &gtComm.cSocketAddress[0],                              "Socket host address"     },

    // TRANSMISSION PROTOCOL SELECT
    { CONFIG_DI_PROTOCOL_UINT8,                             "PROTOCOL",    CONFIG_PARAM_TYPE_UINT8,     {0},        {0x01},                                         {0},            &gtComm.bProtocol,                                      "Transmission Protocol"     },
    // TX GZIP COMPRESS DATA
    {CONFIG_DI_TX_GZIP_FILE_EN_UINT8,                       "TXGZIPEN",       CONFIG_PARAM_TYPE_UINT8,  {0},        {0x01},                                         {0},            &gtComm.bTxGzipFileEnable,                              "TX Gzip Compression Enable" },

    // TIME SERVER
    {CONFIG_UPDATE_TIME_SOURCE_SRVR_PORT,                   "TIMESERVERPORT",       CONFIG_PARAM_TYPE_UINT32,  {0},{0xFFffFFff},                                  {0},              &gtComm.dwTimeTcpServerPort,                            "Time Server Port" },
    {CONFIG_UPDATE_TIME_SOURCE_SRVR_ADDRESS,                "TIMESERVERADDRESS",    CONFIG_PARAM_TYPE_STRING,  {0},{CONFIG_PARAM_STRING_LENGTH_URL},              {0},              &gtComm.cTimeTcpServerAddress[0],                       "Time Server Address" },

    // PCOUNTER DELTA COUNT SOCKET SERVER
    {CONFIG_PCOUNTER_DELTA_COUNT_SRVR_PORT,                 "PCOUNTDELTAPORT",       CONFIG_PARAM_TYPE_UINT32,  {0},{0xFFffFFff},                                  {0},              &gtComm.dwPCountDeltaCountSocketPort,                   "Pcount Delta Server Port" },
    {CONFIG_PCOUNTER_DELTA_COUNT_SRVR_ADDRESS,              "PCOUNTDELTAADDRESS",    CONFIG_PARAM_TYPE_STRING,  {0},{CONFIG_PARAM_STRING_LENGTH_URL},              {0},              &gtComm.cPCountDeltaCountSocketAddress[0],              "Pcount Delta Server Address" }
};

#define CONFIG_COMMUNICATIONS_NUM_OF_ELEMENTS     (sizeof(gtConfigCommunicationsMeta)/sizeof(gtConfigCommunicationsMeta[0]))

static const ConfigParamInfoType 		gtConfigModbusMeta[] = 
{
    // Param Id,                                    Param Name,             Param Type,                 Param Min,  Param Max,                                      Param Default,  Param Data,                                             Param Description
    // NOTE:  For strings, the max field contains the string length 
    // WARNING: min max values can be integers even if the Data is of type FLOAT!
        
    // Modbus
    { CONFIG_MODBUS_ENABLE_BIT_MASK,                  "ENABLE",           CONFIG_PARAM_TYPE_UINT8,     {0},             {3},                                    {0},           &gtModbus.bEnabledBitMask,                             "Modbus ENABLED bit mask"       },    
    { CONFIG_MODBUS_BAUD_RATE,                        "BAUDRATE",         CONFIG_PARAM_TYPE_UINT32,    {9600},          {115200},                               {19200},       &gtModbus.dwBaudRate,                                  "Modbus Baud Rate"              },    
    { CONFIG_MODBUS_PARITY,                           "PARITY",           CONFIG_PARAM_TYPE_UINT8,     {0},               {2},                                    {0},         &gtModbus.bParity,                                     "Modbus Parity"                 },    
    { CONFIG_MODBUS_STOP_BITS,                        "STOPBITS",         CONFIG_PARAM_TYPE_UINT8,     {0},               {3},                                    {0},         &gtModbus.bStopBits,                                   "Modbus Stop Bits"              },    
    { CONFIG_MODBUS_DUPLEX,                           "DUPLEX",           CONFIG_PARAM_TYPE_UINT8,     {0},               {1},                                    {0},         &gtModbus.bDuplex,                                     "Modbus Duplex 0-Half, 1-Full"  },    

    { CONFIG_MODBUS_READ_TRIGGER_MINUTES,             "TRIGGERMIN",         CONFIG_PARAM_TYPE_UINT16,   {0x0000},       {1440},                                   {60},        &gtModbus.wTriggerMinutes,                             "Modbus Read Trigger Minutes"    },

    { CONFIG_MODBUS_REGISTER_COMMAND_01H,                 "REGCMD01H",         CONFIG_PARAM_TYPE_UINT32,    {0},        {0xFFFFFFFF},                              {0},         (UINT32*)&gtModbus.qwRegisterReadCommandArray[0]+1,            "modbus command 01 High Byte" },
    { CONFIG_MODBUS_REGISTER_COMMAND_01L,                 "REGCMD01L",         CONFIG_PARAM_TYPE_UINT32,    {0},        {0xFFFFFFFF},                              {0},         (UINT32*)&gtModbus.qwRegisterReadCommandArray[0],              "modbus command 01 Low Byte" },
    { CONFIG_MODBUS_REGISTER_COMMAND_02H,                 "REGCMD02H",         CONFIG_PARAM_TYPE_UINT32,    {0},        {0xFFFFFFFF},                              {0},         (UINT32*)&gtModbus.qwRegisterReadCommandArray[1]+1,            "modbus command 02 High Byte" },
    { CONFIG_MODBUS_REGISTER_COMMAND_02L,                 "REGCMD02L",         CONFIG_PARAM_TYPE_UINT32,    {0},        {0xFFFFFFFF},                              {0},         (UINT32*)&gtModbus.qwRegisterReadCommandArray[1],              "modbus command 02 Low Byte" },
    { CONFIG_MODBUS_REGISTER_COMMAND_03H,                 "REGCMD03H",         CONFIG_PARAM_TYPE_UINT32,    {0},        {0xFFFFFFFF},                              {0},         (UINT32*)&gtModbus.qwRegisterReadCommandArray[2]+1,            "modbus command 03 High Byte" },
    { CONFIG_MODBUS_REGISTER_COMMAND_03L,                 "REGCMD03L",         CONFIG_PARAM_TYPE_UINT32,    {0},        {0xFFFFFFFF},                              {0},         (UINT32*)&gtModbus.qwRegisterReadCommandArray[2],              "modbus command 03 Low Byte" },
    { CONFIG_MODBUS_REGISTER_COMMAND_04H,                 "REGCMD04H",         CONFIG_PARAM_TYPE_UINT32,    {0},        {0xFFFFFFFF},                              {0},         (UINT32*)&gtModbus.qwRegisterReadCommandArray[3]+1,            "modbus command 04 High Byte" },
    { CONFIG_MODBUS_REGISTER_COMMAND_04L,                 "REGCMD04L",         CONFIG_PARAM_TYPE_UINT32,    {0},        {0xFFFFFFFF},                              {0},         (UINT32*)&gtModbus.qwRegisterReadCommandArray[3],              "modbus command 04 Low Byte" },
    { CONFIG_MODBUS_REGISTER_COMMAND_05H,                 "REGCMD05H",         CONFIG_PARAM_TYPE_UINT32,    {0},        {0xFFFFFFFF},                              {0},         (UINT32*)&gtModbus.qwRegisterReadCommandArray[4]+1,            "modbus command 05 High Byte" },
    { CONFIG_MODBUS_REGISTER_COMMAND_05L,                 "REGCMD05L",         CONFIG_PARAM_TYPE_UINT32,    {0},        {0xFFFFFFFF},                              {0},         (UINT32*)&gtModbus.qwRegisterReadCommandArray[4],              "modbus command 05 Low Byte" },
    { CONFIG_MODBUS_REGISTER_COMMAND_06H,                 "REGCMD06H",         CONFIG_PARAM_TYPE_UINT32,    {0},        {0xFFFFFFFF},                              {0},         (UINT32*)&gtModbus.qwRegisterReadCommandArray[5]+1,            "modbus command 06 High Byte" },
    { CONFIG_MODBUS_REGISTER_COMMAND_06L,                 "REGCMD06L",         CONFIG_PARAM_TYPE_UINT32,    {0},        {0xFFFFFFFF},                              {0},         (UINT32*)&gtModbus.qwRegisterReadCommandArray[5],              "modbus command 06 Low Byte" },
    { CONFIG_MODBUS_REGISTER_COMMAND_07H,                 "REGCMD07H",         CONFIG_PARAM_TYPE_UINT32,    {0},        {0xFFFFFFFF},                              {0},         (UINT32*)&gtModbus.qwRegisterReadCommandArray[6]+1,            "modbus command 07 High Byte" },
    { CONFIG_MODBUS_REGISTER_COMMAND_07L,                 "REGCMD07L",         CONFIG_PARAM_TYPE_UINT32,    {0},        {0xFFFFFFFF},                              {0},         (UINT32*)&gtModbus.qwRegisterReadCommandArray[6],              "modbus command 07 Low Byte" },
    { CONFIG_MODBUS_REGISTER_COMMAND_08H,                 "REGCMD08H",         CONFIG_PARAM_TYPE_UINT32,    {0},        {0xFFFFFFFF},                              {0},         (UINT32*)&gtModbus.qwRegisterReadCommandArray[7]+1,            "modbus command 08 High Byte" },
    { CONFIG_MODBUS_REGISTER_COMMAND_08L,                 "REGCMD08L",         CONFIG_PARAM_TYPE_UINT32,    {0},        {0xFFFFFFFF},                              {0},         (UINT32*)&gtModbus.qwRegisterReadCommandArray[7],              "modbus command 08 Low Byte" },
    { CONFIG_MODBUS_REGISTER_COMMAND_09H,                 "REGCMD09H",         CONFIG_PARAM_TYPE_UINT32,    {0},        {0xFFFFFFFF},                              {0},         (UINT32*)&gtModbus.qwRegisterReadCommandArray[8]+1,            "modbus command 09 High Byte" },
    { CONFIG_MODBUS_REGISTER_COMMAND_09L,                 "REGCMD09L",         CONFIG_PARAM_TYPE_UINT32,    {0},        {0xFFFFFFFF},                              {0},         (UINT32*)&gtModbus.qwRegisterReadCommandArray[8],              "modbus command 09 Low Byte" },
    { CONFIG_MODBUS_REGISTER_COMMAND_10H,                 "REGCMD10H",         CONFIG_PARAM_TYPE_UINT32,    {0},        {0xFFFFFFFF},                              {0},         (UINT32*)&gtModbus.qwRegisterReadCommandArray[9]+1,            "modbus command 10 High Byte" },
    { CONFIG_MODBUS_REGISTER_COMMAND_10L,                 "REGCMD10L",         CONFIG_PARAM_TYPE_UINT32,    {0},        {0xFFFFFFFF},                              {0},         (UINT32*)&gtModbus.qwRegisterReadCommandArray[9],              "modbus command 10 Low Byte" },
    { CONFIG_MODBUS_REGISTER_COMMAND_11H,                 "REGCMD11H",         CONFIG_PARAM_TYPE_UINT32,    {0},        {0xFFFFFFFF},                              {0},         (UINT32*)&gtModbus.qwRegisterReadCommandArray[10]+1,           "modbus command 11 High Byte" },
    { CONFIG_MODBUS_REGISTER_COMMAND_11L,                 "REGCMD11L",         CONFIG_PARAM_TYPE_UINT32,    {0},        {0xFFFFFFFF},                              {0},         (UINT32*)&gtModbus.qwRegisterReadCommandArray[10],             "modbus command 11 Low Byte" },
    { CONFIG_MODBUS_REGISTER_COMMAND_12H,                 "REGCMD12H",         CONFIG_PARAM_TYPE_UINT32,    {0},        {0xFFFFFFFF},                              {0},         (UINT32*)&gtModbus.qwRegisterReadCommandArray[11]+1,           "modbus command 12 High Byte" },
    { CONFIG_MODBUS_REGISTER_COMMAND_12L,                 "REGCMD12L",         CONFIG_PARAM_TYPE_UINT32,    {0},        {0xFFFFFFFF},                              {0},         (UINT32*)&gtModbus.qwRegisterReadCommandArray[11],             "modbus command 12 Low Byte" },
    { CONFIG_MODBUS_REGISTER_COMMAND_13H,                 "REGCMD13H",         CONFIG_PARAM_TYPE_UINT32,    {0},        {0xFFFFFFFF},                              {0},         (UINT32*)&gtModbus.qwRegisterReadCommandArray[12]+1,           "modbus command 13 High Byte" },
    { CONFIG_MODBUS_REGISTER_COMMAND_13L,                 "REGCMD13L",         CONFIG_PARAM_TYPE_UINT32,    {0},        {0xFFFFFFFF},                              {0},         (UINT32*)&gtModbus.qwRegisterReadCommandArray[12],             "modbus command 13 Low Byte" },
    { CONFIG_MODBUS_REGISTER_COMMAND_14H,                 "REGCMD14H",         CONFIG_PARAM_TYPE_UINT32,    {0},        {0xFFFFFFFF},                              {0},         (UINT32*)&gtModbus.qwRegisterReadCommandArray[13]+1,           "modbus command 14 High Byte" },
    { CONFIG_MODBUS_REGISTER_COMMAND_14L,                 "REGCMD14L",         CONFIG_PARAM_TYPE_UINT32,    {0},        {0xFFFFFFFF},                              {0},         (UINT32*)&gtModbus.qwRegisterReadCommandArray[13],             "modbus command 14 Low Byte" },
    { CONFIG_MODBUS_REGISTER_COMMAND_15H,                 "REGCMD15H",         CONFIG_PARAM_TYPE_UINT32,    {0},        {0xFFFFFFFF},                              {0},         (UINT32*)&gtModbus.qwRegisterReadCommandArray[14]+1,           "modbus command 15 High Byte" },
    { CONFIG_MODBUS_REGISTER_COMMAND_15L,                 "REGCMD15L",         CONFIG_PARAM_TYPE_UINT32,    {0},        {0xFFFFFFFF},                              {0},         (UINT32*)&gtModbus.qwRegisterReadCommandArray[14],             "modbus command 15 Low Byte" }

};

#define CONFIG_MODBUS_NUM_OF_ELEMENTS     (sizeof(gtConfigModbusMeta)/sizeof(gtConfigModbusMeta[0]))

static const ConfigParamInfoType 		gtConfigFluidDepthMeta[] = 
{
    // Param Id,                                    Param Name,             Param Type,                 Param Min,  Param Max,                                      Param Default,  Param Data,                                             Param Description
    // NOTE:  For strings, the max field contains the string length 
    // WARNING: min max values can be integers even if the Data is of type FLOAT!
        
    // 
    { CONFIG_FLUID_D_ENABLE_BIT_MASK,               "ENABLE",           CONFIG_PARAM_TYPE_UINT8,     {0},             {3},                                           {0},           &gtFluidD.bEnabledBitMask,                             "Fluid Depth ENABLED bit mask"       },    

    { CONFIG_FLUID_D_PORT_A_DELAY_MSEC,             "STARTDELAYA",      CONFIG_PARAM_TYPE_UINT32,    {0},             {0xFFffFFff},                                 {0},           &gtFluidD.dwStartDelayMSec[0],                           "Delay between start measure and record Port A"     },
    { CONFIG_FLUID_D_PORT_B_DELAY_MSEC,             "STARTDELAYB",      CONFIG_PARAM_TYPE_UINT32,    {0},             {0xFFffFFff},                                 {0},           &gtFluidD.dwStartDelayMSec[1],                           "Delay between start measure and record Port B"     },

    { CONFIG_FLUID_D_PORT_A_FILE_FORMAT,            "FILEFORMATA",      CONFIG_PARAM_TYPE_STRING,    {0},             {CONFIG_PARAM_STRING_LENGTH_FDM_FILE_FORMAT},  {0},           &gtFluidD.cFileFormat[0][0],                              "File Format Port A"     },
    { CONFIG_FLUID_D_PORT_B_FILE_FORMAT,            "FILEFORMATB",      CONFIG_PARAM_TYPE_STRING,    {0},             {CONFIG_PARAM_STRING_LENGTH_FDM_FILE_FORMAT},  {0},           &gtFluidD.cFileFormat[1][0],                              "File Format Port B"     },

    { CONFIG_FLUID_D_PORT_A_BIT_PER_SAMPLE,         "BPSA",             CONFIG_PARAM_TYPE_UINT8,     {0},             {0xFF},                                        {0},           &gtFluidD.bBitPerSample[0],                            "Bit per sample Port A"     },
    { CONFIG_FLUID_D_PORT_B_BIT_PER_SAMPLE,         "BPSB",             CONFIG_PARAM_TYPE_UINT8,     {0},             {0xFF},                                        {0},           &gtFluidD.bBitPerSample[1],                            "Bit per sample Port B"     },

    { CONFIG_FLUID_D_PORT_A_SAMPLE_RATE,            "SAMPLERATEA",      CONFIG_PARAM_TYPE_UINT16,    {0},             {0xFFff},                                     {0},           &gtFluidD.wSampleRate[0],                                "sample rate Port A"     },
    { CONFIG_FLUID_D_PORT_B_SAMPLE_RATE,            "SAMPLERATEB",      CONFIG_PARAM_TYPE_UINT16,    {0},             {0xFFff},                                     {0},           &gtFluidD.wSampleRate[1],                                "sample rate Port B"     },

    { CONFIG_FLUID_D_PORT_A_CAPTURE_DURATION_MSEC,  "DURATIONA",        CONFIG_PARAM_TYPE_UINT32,    {0},             {0xFFffFFff},                                 {0},           &gtFluidD.dwCaptureDurationMSec[0],                     "Capture Duration mSecs Port A"     },
    { CONFIG_FLUID_D_PORT_B_CAPTURE_DURATION_MSEC,  "DURATIONB",        CONFIG_PARAM_TYPE_UINT32,    {0},             {0xFFffFFff},                                 {0},           &gtFluidD.dwCaptureDurationMSec[1],                     "Capture Duration mSecs Port B"     },

    { CONFIG_FLUID_D_PORT_A_TRIGG_OFFSET_MSEC,      "TRIGGOFFSETA",     CONFIG_PARAM_TYPE_UINT32,    {0},             {0xFFffFFff},                                 {0},           &gtFluidD.dwTriggerOffsetMSec[0],                       "Trigger offset mSec Port A"     },
    { CONFIG_FLUID_D_PORT_B_TRIGG_OFFSET_MSEC,      "TRIGGOFFSETB",     CONFIG_PARAM_TYPE_UINT32,    {0},             {0xFFffFFff},                                 {0},           &gtFluidD.dwTriggerOffsetMSec[1],                       "Trigger offset mSec Port B"     },

    { CONFIG_FLUID_D_PORT_A_TRIGG_LEN_MSEC,         "TRIGGLENA",        CONFIG_PARAM_TYPE_UINT32,    {0},             {0xFFffFFff},                                 {0},           &gtFluidD.dwTriggerLenMSec[0],                           "Trigger Length mSec Port A"     },
    { CONFIG_FLUID_D_PORT_B_TRIGG_LEN_MSEC,         "TRIGGLENB",        CONFIG_PARAM_TYPE_UINT32,    {0},             {0xFFffFFff},                                 {0},           &gtFluidD.dwTriggerLenMSec[1],                           "Trigger Length mSec Port B"     },

    // array for cam capture minute offset of the day    
    {CONFIG_FLUID_D_MINUTE_OFFSET_01_ON_THE_DAY,      "FLUIDMINOFFST01",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtFluidD.iwFluidDepthMinuteOffsetOnTheDay[0],            "Fluid minute offset1on the day" },
    {CONFIG_FLUID_D_MINUTE_OFFSET_02_ON_THE_DAY,      "FLUIDMINOFFST02",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtFluidD.iwFluidDepthMinuteOffsetOnTheDay[1],            "Fluid minute offset2on the day" },
    {CONFIG_FLUID_D_MINUTE_OFFSET_03_ON_THE_DAY,      "FLUIDMINOFFST03",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtFluidD.iwFluidDepthMinuteOffsetOnTheDay[2],            "Fluid minute offset3on the day" },
    {CONFIG_FLUID_D_MINUTE_OFFSET_04_ON_THE_DAY,      "FLUIDMINOFFST04",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtFluidD.iwFluidDepthMinuteOffsetOnTheDay[3],            "Fluid minute offset4on the day" },
    {CONFIG_FLUID_D_MINUTE_OFFSET_05_ON_THE_DAY,      "FLUIDMINOFFST05",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtFluidD.iwFluidDepthMinuteOffsetOnTheDay[4],            "Fluid minute offset5on the day" },
    {CONFIG_FLUID_D_MINUTE_OFFSET_06_ON_THE_DAY,      "FLUIDMINOFFST06",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtFluidD.iwFluidDepthMinuteOffsetOnTheDay[5],            "Fluid minute offset6on the day" },
    {CONFIG_FLUID_D_MINUTE_OFFSET_07_ON_THE_DAY,      "FLUIDMINOFFST07",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtFluidD.iwFluidDepthMinuteOffsetOnTheDay[6],            "Fluid minute offset7on the day" },
    {CONFIG_FLUID_D_MINUTE_OFFSET_08_ON_THE_DAY,      "FLUIDMINOFFST08",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtFluidD.iwFluidDepthMinuteOffsetOnTheDay[7],            "Fluid minute offset8on the day" },
    {CONFIG_FLUID_D_MINUTE_OFFSET_09_ON_THE_DAY,      "FLUIDMINOFFST09",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtFluidD.iwFluidDepthMinuteOffsetOnTheDay[8],            "Fluid minute offset9on the day" },
    {CONFIG_FLUID_D_MINUTE_OFFSET_10_ON_THE_DAY,      "FLUIDMINOFFST10",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtFluidD.iwFluidDepthMinuteOffsetOnTheDay[9],            "Fluid minute offset10on the day" },
    {CONFIG_FLUID_D_MINUTE_OFFSET_11_ON_THE_DAY,      "FLUIDMINOFFST11",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtFluidD.iwFluidDepthMinuteOffsetOnTheDay[10],           "Fluid minute offset11on the day" },
    {CONFIG_FLUID_D_MINUTE_OFFSET_12_ON_THE_DAY,      "FLUIDMINOFFST12",      CONFIG_PARAM_TYPE_INT16,   {-1},        {1440},                                        {-1},            &gtFluidD.iwFluidDepthMinuteOffsetOnTheDay[11],           "Fluid minute offset12on the day" },
};

#define CONFIG_FLUID_D_NUM_OF_ELEMENTS     (sizeof(gtConfigFluidDepthMeta)/sizeof(gtConfigFluidDepthMeta[0]))

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

static ConfigAccessStorageType gtConfigLookupArray[] =
{
//      pcName              pvDataStruct,               dwDataStructSize,                   ptMetaStruct,                                               wMetaStructNumOfElements,                   bStorageMagicNumber,                            bStorageVersion,                                    dwDataFlashStorageSubsector,                dwDataFlashStorageAddress,                  dwDataFlashStoragePageNumber            updateConfigCounter
    {   "DEV_INFO",         &gtDevInfo,                 sizeof(gtDevInfo),                  (ConfigParamInfoType * const)&gtConfigDevInfoMeta[0],       CONFIG_DEV_INFO_NUM_OF_ELEMENTS,            CONFIG_DEV_INFO_STORAGE_MAGIC_NUMBER_BYTE,      CONFIG_DEV_INFO_STORAGE_VERSION_NUMBER_BYTE,        CONFIG_DEV_INFO_STORAGE_DF_SUBSECTOR,       CONFIG_DEV_INFO_STORAGE_DF_ADDRESS,         CONFIG_DEV_INFO_STORAGE_DF_PAGE_NUM,      0      },
    {   "SETTING",          &gtSettings,                sizeof(gtSettings),                 (ConfigParamInfoType * const)&gtConfigSettingsMeta[0],      CONFIG_SETTINGS_NUM_OF_ELEMENTS,            CONFIG_SETTINGS_STORAGE_MAGIC_NUMBER_BYTE,      CONFIG_SETTINGS_STORAGE_VERSION_NUMBER_BYTE,        CONFIG_SETTINGS_STORAGE_DF_SUBSECTOR,       CONFIG_SETTINGS_STORAGE_DF_ADDRESS,         CONFIG_SETTINGS_STORAGE_DF_PAGE_NUM,      0      },
    {   "ADC_CAL",          &gtAdcCalib,                sizeof(gtAdcCalib),                 (ConfigParamInfoType * const)&gtConfigCalibrationMeta[0],   CONFIG_CALIBRATION_NUM_OF_ELEMENTS,         CONFIG_CALIBRATION_STORAGE_MAGIC_NUMBER_BYTE,   CONFIG_CALIBRATION_STORAGE_VERSION_NUMBER_BYTE,     CONFIG_CALIBRATION_STORAGE_DF_SUBSECTOR,    CONFIG_CALIBRATION_STORAGE_DF_ADDRESS,      CONFIG_CALIBRATION_STORAGE_DF_PAGE_NUM,   0      },
    {   "COMM",             &gtComm,                    sizeof(gtComm),                     (ConfigParamInfoType * const)&gtConfigCommunicationsMeta[0],CONFIG_COMMUNICATIONS_NUM_OF_ELEMENTS,      CONFIG_COMMUNICATIONS_STORAGE_MAGIC_NUMBER_BYTE,CONFIG_COMMUNICATIONS_STORAGE_VERSION_NUMBER_BYTE,  CONFIG_COMMUNICATIONS_STORAGE_DF_SUBSECTOR, CONFIG_COMMUNICATIONS_STORAGE_DF_ADDRESS,   CONFIG_COMMUNICATIONS_STORAGE_DF_PAGE_NUM,0    },
    {   "MODBUS",           &gtModbus,                  sizeof(gtModbus),                   (ConfigParamInfoType * const)&gtConfigModbusMeta[0],        CONFIG_MODBUS_NUM_OF_ELEMENTS,              CONFIG_MODBUS_STORAGE_MAGIC_NUMBER_BYTE,        CONFIG_MODBUS_STORAGE_VERSION_NUMBER_BYTE,          CONFIG_MODBUS_STORAGE_DF_SUBSECTOR,         CONFIG_MODBUS_STORAGE_DF_ADDRESS,           CONFIG_MODBUS_STORAGE_DF_PAGE_NUM,        0    },
    {   "FLUID",            &gtFluidD,                  sizeof(gtFluidD),                   (ConfigParamInfoType * const)&gtConfigFluidDepthMeta[0],    CONFIG_FLUID_D_NUM_OF_ELEMENTS,             CONFIG_FLUID_DEPTH_STORAGE_MAGIC_NUMBER_BYTE,   CONFIG_FLUID_DEPTH_STORAGE_VERSION_NUMBER_BYTE,     CONFIG_FLUID_DEPTH_STORAGE_DF_SUBSECTOR,    CONFIG_FLUID_DEPTH_STORAGE_DF_ADDRESS,      CONFIG_FLUID_DEPTH_STORAGE_DF_PAGE_NUM,   0    }
};

#define CONFIG_LOOKUP_ARRAY_NUM_OF_ELEMENTS                     (sizeof(gtConfigLookupArray)/sizeof(gtConfigLookupArray[0]))

//////////////////////////////////////////////////////////////////////////////////////////////////
// Local Variable Declarations
//////////////////////////////////////////////////////////////////////////////////////////////////

static xSemaphoreHandle             gxSemaphoreMutexConfigSettingAccess = NULL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Local Routines
//////////////////////////////////////////////////////////////////////////////////////////////////

static ConfigSemaphoreResultEnum    ConfigSemaphoreAcquire              ( void );
static ConfigSemaphoreResultEnum    ConfigSemaphoreRelease              ( ConfigSemaphoreResultEnum ePreviousResult );
static BOOL                         ConfigParamGetInfoByConfigType      ( const ConfigTypeEnum eConfigType, const UINT16 wParamId, ConfigParamInfoType **ptConfigParamInfo );
static BOOL                         ConfigIsTypeCorrectlyDefined        ( ConfigTypeEnum eConfigType );

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         BOOL ConfigIsTypeCorrectlyDefined( ConfigTypeEnum eConfigType )
//!
//! \brief      This function helps to validate the matching between the size of the config struct in .h file
//!             and the Meta struct in .c of config module
//!     
//! \param[in]  eConfigType = type of configuration. More info, check ConfigTypeEnum
//!
//! \return     BOOL, TRUE if no error, FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ConfigIsTypeCorrectlyDefined( ConfigTypeEnum eConfigType )
{   
    BOOL   fSuccess = FALSE;         
 
    UINT16 wInfoStructSumSize;        
    UINT16 wDataStructSize;

    if( eConfigType < CONFIG_TYPE_MAX )
    {
        // set values to run structure evaluation
        wInfoStructSumSize  = 0;
        wDataStructSize     = gtConfigLookupArray[eConfigType].dwDataStructSize;

        for( UINT16 wElementIndex = 0; wElementIndex < gtConfigLookupArray[eConfigType].wMetaStructNumOfElements ; wElementIndex++ )
        {            
            if( CONFIG_PARAM_TYPE_STRING == gtConfigLookupArray[eConfigType].ptMetaStruct[wElementIndex].bParamType )
            {
                wInfoStructSumSize += gtConfigLookupArray[eConfigType].ptMetaStruct[wElementIndex].vtParamValueMax.i32 + 1;
            }
            else
            {                
                wInfoStructSumSize += gbConfigParamTypeSizeArray[ gtConfigLookupArray[eConfigType].ptMetaStruct[wElementIndex].bParamType ];
            }
        }
    
        if( wInfoStructSumSize != wDataStructSize )
        {        
            ConsoleDebugfln( CONSOLE_PORT_USART, "ConfigInitialize() Error: Config Device Info Mismatch size");
            ConsoleDebugfln( CONSOLE_PORT_USART, "info array sum size=%d", wInfoStructSumSize);
            ConsoleDebugfln( CONSOLE_PORT_USART, "data struct size=%d", wDataStructSize );
            while(1);
        }
        else
        {
            fSuccess = TRUE;
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         BOOL ConfigInitialize(void)
//!
//! \brief      Initialize the configuration module.
//!
//! \param[in]  None
//!
//! \return     BOOL, TRUE if successful, FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ConfigInitialize(void)
{
    BOOL fReturn            = TRUE;

    // Create a semaphore to control access to the Config Array that contains a copy of config register
    gxSemaphoreMutexConfigSettingAccess = xSemaphoreCreateMutex();

    if( gxSemaphoreMutexConfigSettingAccess == NULL )
    {
        fReturn = FALSE;
        //ConsoleDebugfln( CONSOLE_PORT_USART, "ConfigInitialize() Error: Semaphore = NULL");
        while(1);
    }
        
    // make sure data structure doesn't go over the size of the sub-sector
    for( UINT8 x = 0; x < CONFIG_TYPE_MAX ; x++ )
    {
        if( gtConfigLookupArray[x].dwDataStructSize > (DFF_CONFIG_0_SUBSECTORS_TOTAL*DATAFLASH_SUBSECTOR_SIZE_BYTES) )
        {
            while(1)
            {
                // the number of params exceeds the size of the subsector
            }
        }
    }

    // make sure enums and data size are aligned
    for( ConfigTypeEnum eConfigType = 0; eConfigType < CONFIG_TYPE_MAX; eConfigType++ )
    {
        if( ConfigIsTypeCorrectlyDefined( eConfigType ) == FALSE )
        {
            while(1);
        }
    }

    for( ConfigTypeEnum eConfigType = 0; eConfigType < CONFIG_TYPE_MAX; eConfigType++ )
    {     
        // initialize counters to 1 so control updates its configurations.
        gtConfigLookupArray[eConfigType].bUpdateConfigCounter++;
    }

    return fReturn;
}

BOOL ConfigLoad( void )
{
    BOOL fReturn = FALSE;

//    // make sure enums and data size are aligned
//    for( ConfigTypeEnum eConfigType = 0; eConfigType < CONFIG_TYPE_MAX; eConfigType++ )
//    {
//        ConfigIsTypeCorrectlyDefined( eConfigType );
//    }
    
    fReturn = ConfigParametersLoadAll();
    
//      FUTURE: check config parameters against min/max

//    for( ConfigTypeEnum eConfigType = 0; eConfigType < CONFIG_TYPE_MAX; eConfigType++ )
//    {     
//        // initialize counters to 1 so control updates its configurations.
//        gtConfigLookupArray[eConfigType].bUpdateConfigCounter++;
//    }

    return fReturn;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         ConfigSemaphoreResultEnum ConfigSemaphoreAcquire(void)
//!
//! \brief      Acquire a semaphore lock on the configuration module.
//!
//! \param[in]  None
//!
//! \return     ConfigSemaphoreResultEnum, Result code
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
ConfigSemaphoreResultEnum ConfigSemaphoreAcquire(void)
{
    ConfigSemaphoreResultEnum eResult = CONFIG_SEMAPHORE_ERROR;

    if (taskSCHEDULER_RUNNING != xTaskGetSchedulerState())
    {
        eResult = CONFIG_SEMAPHORE_SCHEDULER_NOT_RUNNING;
    }
    else if (xSemaphoreGetMutexHolder(gxSemaphoreMutexConfigSettingAccess) == xTaskGetCurrentTaskHandle())
    {
        eResult = CONFIG_SEMAPHORE_ALREADY_AQUIRED;
    }
    else
    {
        // Attempt to acquire the semaphore
        eResult = (xSemaphoreTake( gxSemaphoreMutexConfigSettingAccess, 10000 ) == pdTRUE) ? CONFIG_SEMAPHORE_ACQUIRED : CONFIG_SEMAPHORE_ERROR;
    }

    return eResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         ConfigSemaphoreResultEnum ConfigSemaphoreRelease(ConfigSemaphoreResultEnum ePreviousResult)
//!
//! \brief      Release a semaphore lock on the configuration module.
//!
//! \param[in]  ConfigSemaphoreResultEnum ePreviousResult, Result from previous ConfigSemaphoreAcquire() call
//!
//! \return     ConfigSemaphoreResultEnum, Result code
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
ConfigSemaphoreResultEnum ConfigSemaphoreRelease(ConfigSemaphoreResultEnum ePreviousResult)
{
    ConfigSemaphoreResultEnum eResult = CONFIG_SEMAPHORE_ERROR;

    if (CONFIG_SEMAPHORE_ALREADY_AQUIRED == ePreviousResult)
    {
        eResult = CONFIG_SEMAPHORE_NOT_RELEASED;
    }
    else
    {
        // Give back the semaphore
        eResult = (xSemaphoreGive( gxSemaphoreMutexConfigSettingAccess ) == pdTRUE) ? CONFIG_SEMAPHORE_RELEASED : CONFIG_SEMAPHORE_ERROR;
    }

    return eResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         BOOL ConfigParametersSaveByConfigType( const ConfigTypeEnum eConfigType )
//!
//! \brief      Save parameters from one of the types of configurations.
//!
//! \param[in]  eConfigType = type of configuration. More info, check ConfigTypeEnum
//!
//! \return     BOOL, TRUE if successful, FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ConfigParametersSaveByConfigType( const ConfigTypeEnum eConfigType )
{
    BOOL                        fSuccess                = FALSE;                
    ConfigSemaphoreResultEnum   eConfigSemaphoreResult;

    if( eConfigType < CONFIG_TYPE_MAX )
    {            
        eConfigSemaphoreResult = ConfigSemaphoreAcquire();

        if( CONFIG_SEMAPHORE_ACQUIRED == eConfigSemaphoreResult )
        {
            ConfigStorageType       tStorageStruct;
            ConfigStorageType       tStorageStructObtainedFromFlash;
            UINT32                  dwStorageSize;
            UINT16                  wCrc16Calculated;

            dwStorageSize = ( sizeof(tStorageStruct.tMeta) + gtConfigLookupArray[eConfigType].dwDataStructSize );

            // copy the current state of the selected structure              
            memcpy( &tStorageStruct.uData, gtConfigLookupArray[eConfigType].pvDataStruct, gtConfigLookupArray[eConfigType].dwDataStructSize );
            wCrc16Calculated = GeneralCalcCrc16Standard( &tStorageStruct.uData, gtConfigLookupArray[eConfigType].dwDataStructSize );

            tStorageStruct.tMeta.bSignatureByte      = gtConfigLookupArray[eConfigType].bStorageMagicNumber;
            tStorageStruct.tMeta.bDevInfoVersion     = gtConfigLookupArray[eConfigType].bStorageVersion;
            tStorageStruct.tMeta.wCrc16              = wCrc16Calculated;    
            // Write configuration data to dataflash
            // erase mem, write in mem and read from mem to make sure data was written correctly in memory
            if( Dataflash_EraseSubsector( DATAFLASH0_ID, gtConfigLookupArray[eConfigType].dwDataFlashStorageSubsector ) == DATAFLASH_OK )
            {            
                if( Dataflash_Write( DATAFLASH0_ID, gtConfigLookupArray[eConfigType].dwDataFlashStorageAddress, (UINT8*) &tStorageStruct, dwStorageSize ) == DATAFLASH_OK )
                {
                    // now double check the data was correctly save in flash
                    memset( &tStorageStructObtainedFromFlash, 0, dwStorageSize );

                    if( Dataflash_Read( DATAFLASH0_ID, gtConfigLookupArray[eConfigType].dwDataFlashStorageAddress, (UINT8*)&tStorageStructObtainedFromFlash, dwStorageSize ) == DATAFLASH_OK )
                    {
                        // Ensure it was saved correctly - memcmp->= 0 means mems are equal    
                        if( memcmp( &tStorageStructObtainedFromFlash, &tStorageStruct, dwStorageSize ) == 0 )
                        {
                            fSuccess = TRUE;    // memory did match

                            gtConfigLookupArray[eConfigType].bUpdateConfigCounter++;
                        }                                
                        else
                        {
                            ConsoleDebugfln( CONSOLE_PORT_USART, "ConfigParametersSaveByConfigType() Error Data not stored correctly.");
                        }
                    }            
                    else
                    {
                        ConsoleDebugfln( CONSOLE_PORT_USART, "ConfigParametersSaveByConfigType() Error reading DataFlash.");
                    }
                }
                else
                {
                    ConsoleDebugfln( CONSOLE_PORT_USART, "ConfigParametersSaveByConfigType() Error Writing DataFlash.");
                }
            }
            else
            {
                ConsoleDebugfln( CONSOLE_PORT_USART, "ConfigParametersSaveByConfigType() Error DataFlash not erased.");
            }

            ConfigSemaphoreRelease( eConfigSemaphoreResult );
        }                
    }

    return fSuccess;    
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         BOOL ConfigParametersSaveAll(void)
//!
//! \brief      Save all parameters.
//!
//! \param[in]  None
//!
//! \return     BOOL, TRUE if successful, FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ConfigParametersSaveAll( void )
{
    BOOL fSuccess = TRUE;

    for( ConfigTypeEnum eConfigType = 0; eConfigType < CONFIG_TYPE_MAX; eConfigType++ )
    {            
        fSuccess &= ConfigParametersSaveByConfigType( eConfigType );
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         BOOL ConfigParametersLoadByConfigType( const ConfigTypeEnum eConfigType )
//!
//! \brief      Load parameters from one of the types of configurations.
//!
//! \param[in]  eConfigType = type of configuration. More info, check ConfigTypeEnum
//!
//! \return     BOOL, TRUE if successful, FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ConfigParametersLoadByConfigType( const ConfigTypeEnum eConfigType )
{
    BOOL                        fSuccess       = FALSE;
    BOOL                        fDataInvalid   = TRUE;
    ConfigSemaphoreResultEnum   eConfigSemaphoreResult; 

    if( eConfigType < CONFIG_TYPE_MAX )
    {        
        fSuccess = FALSE;   

        eConfigSemaphoreResult = ConfigSemaphoreAcquire();        

        if( eConfigSemaphoreResult == CONFIG_SEMAPHORE_ACQUIRED )
        {
            ConfigStorageType       tStorageStruct;
            UINT32                  dwStorageSize;
            UINT16                  wCrc16Calculated;

            dwStorageSize = ( sizeof(tStorageStruct.tMeta) + gtConfigLookupArray[eConfigType].dwDataStructSize );

            // clear struct before starting loading the data into it
            memset( gtConfigLookupArray[eConfigType].pvDataStruct, 0, gtConfigLookupArray[eConfigType].dwDataStructSize );                                            
            // clear memory where data is going to be loaded            
            memset( &tStorageStruct, 0, sizeof(tStorageStruct) );

            // Read configuration data from permanent storage
            if( Dataflash_ReadPage( DATAFLASH0_ID, gtConfigLookupArray[eConfigType].dwDataFlashStoragePageNumber, (UINT8*)&tStorageStruct, dwStorageSize ) == DATAFLASH_OK )
            {                                   
                wCrc16Calculated = GeneralCalcCrc16Standard( &tStorageStruct.uData , gtConfigLookupArray[eConfigType].dwDataStructSize );

                // validate data extracted from memory
                fSuccess = TRUE;
                fSuccess &= (tStorageStruct.tMeta.bSignatureByte    == gtConfigLookupArray[eConfigType].bStorageMagicNumber);
                fSuccess &= (tStorageStruct.tMeta.bDevInfoVersion   == gtConfigLookupArray[eConfigType].bStorageVersion);
                fSuccess &= (tStorageStruct.tMeta.wCrc16            == wCrc16Calculated);

                if( fSuccess )
                {                                      
                    memcpy(gtConfigLookupArray[eConfigType].pvDataStruct, &tStorageStruct.uData, gtConfigLookupArray[eConfigType].dwDataStructSize );                                        
                    fDataInvalid = FALSE;
                }
                else
                {              
                    fDataInvalid = TRUE;                    
                    ConsoleDebugfln( CONSOLE_PORT_USART, "Error at: sigByte or Version or crc");
                }                             
            }
            else
            {
                ConsoleDebugfln( CONSOLE_PORT_USART, "ConfigParametersLoadByType() Error reading Data Flash.");
            }
        }
        else
        {
            ConsoleDebugfln( CONSOLE_PORT_USART, "ConfigParametersLoadByType() Error taking semaphore.");
        }
  
        ConfigSemaphoreRelease( eConfigSemaphoreResult );


        // revert params only after releasing semaphore since they use semaphores as well.
        if( fDataInvalid == TRUE )
        {
            fSuccess = FALSE;
            ConsoleDebugfln( CONSOLE_PORT_USART, "Config [%s] invalid. Config defaults used.", ConfigGetConfigTypeName(eConfigType) );
            ConfigParametersRevertByConfigType( eConfigType );
            ConsoleDebugfln( CONSOLE_PORT_USART, "Config [%s] defaults saved to dataflash.", ConfigGetConfigTypeName(eConfigType) );
            ConfigParametersSaveByConfigType( eConfigType );
        }
    }    

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         BOOL ConfigParametersLoadAll(void)
//!
//! \brief      Load all parameters.
//!
//! \param[in]  None
//!
//! \return     BOOL, TRUE if successful, FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ConfigParametersLoadAll( void )
{
    BOOL fSuccess = FALSE;

    fSuccess = TRUE;
    
    for( ConfigTypeEnum eConfigType = 0 ; eConfigType < CONFIG_TYPE_MAX ; eConfigType++ )
    {
        fSuccess &= ConfigParametersLoadByConfigType( eConfigType );        
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         BOOL ConfigParametersRevertAll(void)
//!
//! \brief      Revert all parameters back to firmware defaults
//!
//! \param[in]  None
//!
//! \return     BOOL, TRUE if successful, FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ConfigParametersRevertAll( void )
{
    BOOL fReturn            = FALSE;   

    fReturn = TRUE;

    for( ConfigTypeEnum eConfigType = 0 ; eConfigType < CONFIG_TYPE_MAX ; eConfigType++ )
    {
        fReturn &= ConfigParametersRevertByConfigType( eConfigType );
    }

    return fReturn;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         BOOL ConfigParametersRevertByConfigType( const ConfigTypeEnum eConfigType )
//!
//! \brief      Revert parameters of one type of configuration back to firmware defaults
//!
//! \param[in]  eConfigType = type of configuration. More info, check ConfigTypeEnum
//!
//! \return     BOOL, TRUE if successful, FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ConfigParametersRevertByConfigType( const ConfigTypeEnum eConfigType )
{
    BOOL fSuccess = FALSE;    
    
    if( eConfigType < CONFIG_LOOKUP_ARRAY_NUM_OF_ELEMENTS )
    {
        fSuccess = TRUE;
        for (UINT16 wIndex = 0; wIndex < gtConfigLookupArray[eConfigType].wMetaStructNumOfElements ; wIndex++)
        {
            fSuccess &= ConfigRevertValueByConfigType(eConfigType, gtConfigLookupArray[eConfigType].ptMetaStruct[wIndex].wParamId);
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         BOOL ConfigRevertValueByConfigType( const ConfigTypeEnum eConfigType, const UINT8 bParamId )
//!
//! \brief      Revert one single parameter of one type of configuration back to firmware defaults
//!
//! \param[in]  eConfigType = type of configuration. More info, check ConfigTypeEnum
//! \param[in]  bParamId    = parameter Id.
//!
//! \return     BOOL, TRUE if successful, FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ConfigRevertValueByConfigType( const ConfigTypeEnum eConfigType, const UINT16 wParamId )
{
    BOOL                        fSuccess                = FALSE;    
    ConfigParamInfoType         *ptConfigParamInfo      = NULL; 
    ConfigSemaphoreResultEnum   eConfigSemaphoreResult;

    eConfigSemaphoreResult = ConfigSemaphoreAcquire();

    if( CONFIG_SEMAPHORE_ACQUIRED == eConfigSemaphoreResult )
    {                
        if( eConfigType < CONFIG_LOOKUP_ARRAY_NUM_OF_ELEMENTS )
        {
            if (ConfigParamGetInfoByConfigType(eConfigType, wParamId, &ptConfigParamInfo))
            {
                fSuccess    = TRUE;

                if (ptConfigParamInfo->bParamType == CONFIG_PARAM_TYPE_STRING)
                {
                    // The default value of any string is blank
                    memset(ptConfigParamInfo->pvParamData, 0x00, ptConfigParamInfo->vtParamValueDefault.i32+1);
                }
                else if( ptConfigParamInfo->bParamType == CONFIG_PARAM_TYPE_FLOAT )
                {
                    memcpy(ptConfigParamInfo->pvParamData, &ptConfigParamInfo->vtParamValueDefault.f32, gbConfigParamTypeSizeArray[ptConfigParamInfo->bParamType]);
                }
                else
                {
                    memcpy(ptConfigParamInfo->pvParamData, &ptConfigParamInfo->vtParamValueDefault.i32, gbConfigParamTypeSizeArray[ptConfigParamInfo->bParamType]);
                }
            }
        }
    }
    
    ConfigSemaphoreRelease( eConfigSemaphoreResult );

    return fSuccess;    
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         BOOL ConfigGetValueByConfigType( const ConfigTypeEnum eConfigType, const UINT8 bParamId, void *pvParamData )
//!
//! \brief      Get the value for a specific configuration parameter.
//!
//! \param[in]  ConfigTypeEnum eConfigType,
//! \param[in]  const UINT8 bParamId,
//! \param[in]  void *pvParamData,
//!
//! \return     BOOL, TRUE if successful, FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ConfigGetValueByConfigType( const ConfigTypeEnum eConfigType, const UINT16 wParamId, void *pvParamData, UINT32 dwParamDataSize )
{
    BOOL                        fSuccess                = TRUE;
    ConfigParamInfoType         *ptConfigParamInfo      = NULL;
    ConfigSemaphoreResultEnum   eConfigSemaphoreResult;

    eConfigSemaphoreResult = ConfigSemaphoreAcquire();

    if( CONFIG_SEMAPHORE_ACQUIRED == eConfigSemaphoreResult )
    {                
        if( ConfigParamGetInfoByConfigType( eConfigType, wParamId, &ptConfigParamInfo ) )
        {            
            if( ptConfigParamInfo->bParamType == CONFIG_PARAM_TYPE_STRING )
            {
                if( dwParamDataSize >= (((UINT32)ptConfigParamInfo->vtParamValueMax.i32) + 1 ) )                
                {
                    fSuccess  = TRUE;

                    strncpy( pvParamData, ptConfigParamInfo->pvParamData, dwParamDataSize );
                }
            }
            else
            {
                UINT32 dwParameterSize = gbConfigParamTypeSizeArray[ptConfigParamInfo->bParamType];

                if( dwParamDataSize == dwParameterSize ) 
                {
                    fSuccess  = TRUE;

                    memcpy( pvParamData, ptConfigParamInfo->pvParamData, dwParamDataSize );
                }
            }
        }
    }

    ConfigSemaphoreRelease( eConfigSemaphoreResult );

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         BOOL ConfigSetValueByConfigType( const ConfigTypeEnum eConfigType, const UINT8 bParamId, const void *pvParamData )
//!
//! \brief      Set the value for a specific configuration parameter
//!
//! \param[in]  ConfigTypeEnum eConfigType,
//! \param[in]  const UINT8 bParamId,
//! \param[in]  const void *pvParamData,
//!
//! \return     BOOL, TRUE if successful, FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ConfigSetValueByConfigType( const ConfigTypeEnum eConfigType, const UINT16 wParamId, const void *pvParamData )
{
    BOOL                        fReturn                 = FALSE;
    ConfigParamInfoType         *ptConfigParamInfo      = NULL;
    ConfigSemaphoreResultEnum   eConfigSemaphoreResult;

    eConfigSemaphoreResult = ConfigSemaphoreAcquire();

    if( CONFIG_SEMAPHORE_ACQUIRED == eConfigSemaphoreResult )
    {
        if( ConfigParamGetInfoByConfigType( eConfigType, wParamId, &ptConfigParamInfo ) )
        {

            if (ptConfigParamInfo->bParamType == CONFIG_PARAM_TYPE_STRING)
            {
                CHAR * pcString = (CHAR *)pvParamData;
                INT16 iwStrLen  = strlen(pcString);
                
                if( iwStrLen <= ptConfigParamInfo->vtParamValueMax.i32 )
                {
                    memset(ptConfigParamInfo->pvParamData, 0x00, ptConfigParamInfo->vtParamValueMax.i32+1);
                    strncpy(ptConfigParamInfo->pvParamData, pvParamData, ptConfigParamInfo->vtParamValueMax.i32);

                    fReturn = TRUE;
                }
                else // string len larger than expected field size
                {
                    fReturn = FALSE;
                }
            }
            else
            {           
                UINT8 bParameterSize = gbConfigParamTypeSizeArray[ptConfigParamInfo->bParamType];

                switch (ptConfigParamInfo->bParamType)
                {
                    case CONFIG_PARAM_TYPE_UINT8:
                    case CONFIG_PARAM_TYPE_UINT16:
                    case CONFIG_PARAM_TYPE_UINT32:
                    {               
                        UINT32 dwTemporary = 0;

                        if(CONFIG_PARAM_TYPE_UINT8 == ptConfigParamInfo->bParamType)
                        {
                            dwTemporary = *((UINT8 *)pvParamData);
                        }
                        else if(CONFIG_PARAM_TYPE_UINT16 == ptConfigParamInfo->bParamType)
                        {
                            dwTemporary = *((UINT16 *)pvParamData);
                        }
                        else
                        {
                            dwTemporary = *((UINT32 *)pvParamData);
                        }

                        if( dwTemporary >= ptConfigParamInfo->vtParamValueMin.u32 )
                        {
                            if( dwTemporary <= ptConfigParamInfo->vtParamValueMax.u32 )
                            {
                                fReturn = TRUE;

                                memcpy(ptConfigParamInfo->pvParamData, &dwTemporary, bParameterSize);
                            }
                        }

                        //Sanity check for max and min values
//                        dwTemporary = GENERAL_MAX(dwTemporary, ptConfigParamInfo->vtParamValueMin.u32);
//                        dwTemporary = GENERAL_MIN(dwTemporary, ptConfigParamInfo->vtParamValueMax.u32);
                        break;
                    }

                    case CONFIG_PARAM_TYPE_INT8:
                    case CONFIG_PARAM_TYPE_INT16:
                    case CONFIG_PARAM_TYPE_INT32:
                    {
                        INT32 iTemporary = 0;
                    
                        if(CONFIG_PARAM_TYPE_INT8 == ptConfigParamInfo->bParamType )
                        {
                            iTemporary = *((INT8 *)pvParamData);
                        }
                        else if(CONFIG_PARAM_TYPE_INT16 == ptConfigParamInfo->bParamType)
                        {
                            iTemporary = *((INT16 *)pvParamData);
                        }
                        else
                        {
                            iTemporary = *((INT32 *)pvParamData);
                        }

                        
                        if( iTemporary >= ptConfigParamInfo->vtParamValueMin.i32 )
                        {
                            if( iTemporary <= ptConfigParamInfo->vtParamValueMax.i32 )
                            {
                                fReturn = TRUE;

                                memcpy(ptConfigParamInfo->pvParamData, &iTemporary, bParameterSize);
                            }
                        }

                        //Sanity check for max and min values
//                        iTemporary = GENERAL_MAX(iTemporary, ptConfigParamInfo->vtParamValueMin.i32);
//                        iTemporary = GENERAL_MIN(iTemporary, ptConfigParamInfo->vtParamValueMax.i32);
                        break;
                    }
                    case CONFIG_PARAM_TYPE_CHAR:
                    {
                        fReturn = TRUE;
                        CHAR cTemporary = *((CHAR *)pvParamData);

                        //Sanity check for max and min values
                        cTemporary = GENERAL_MAX(cTemporary, ptConfigParamInfo->vtParamValueMin.u32);
                        cTemporary = GENERAL_MIN(cTemporary, ptConfigParamInfo->vtParamValueMax.u32);

                        memcpy(ptConfigParamInfo->pvParamData, &cTemporary, bParameterSize);

                        break;
                    }
                    case CONFIG_PARAM_TYPE_BOOL:
                    {
                        fReturn = TRUE;
                        BOOL fTemporary = *((BOOL *)pvParamData);

                        //Sanity check for max and min values
                        fTemporary = GENERAL_MAX(fTemporary, ptConfigParamInfo->vtParamValueMin.u32);
                        fTemporary = GENERAL_MIN(fTemporary, ptConfigParamInfo->vtParamValueMax.u32);

                        memcpy(ptConfigParamInfo->pvParamData, &fTemporary, bParameterSize);

                        break;
                    }                    
                    case CONFIG_PARAM_TYPE_FLOAT:
                    {
                        //fReturn = TRUE;
                        SINGLE sgTemporary = *((SINGLE *)pvParamData);

                        //Sanity check for max and min values
//                        sgTemporary = GENERAL_MAX(sgTemporary, ptConfigParamInfo->vtParamValueMin.f32);
//                        sgTemporary = GENERAL_MIN(sgTemporary, ptConfigParamInfo->vtParamValueMax.f32);

                        if( sgTemporary >= ptConfigParamInfo->vtParamValueMin.f32 )
                        {
                            if( sgTemporary <= ptConfigParamInfo->vtParamValueMax.f32 )
                            {
                                fReturn = TRUE;

                                memcpy(ptConfigParamInfo->pvParamData, &sgTemporary, bParameterSize);
                            }
                        }
//                        memcpy(ptConfigParamInfo->pvParamData, &sgTemporary, bParameterSize);
//                        fReturn = TRUE;
                        break;
                    }
                    default:
                        fReturn = FALSE;
                        break;
                }// end switch                       
            }// end if(type == string) else
        }
        else
        {
            fReturn = FALSE;
        }
    }

    ConfigSemaphoreRelease( eConfigSemaphoreResult );

    return fReturn;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         BOOL ConfigGetIdFromNameByConfigType( const ConfigTypeEnum eConfigType, const CHAR *pcParamName, UINT8 *pbParamId )
//!
//! \brief      Get the Id depending on the param name passed.
//!
//! \param[in]  ConfigTypeEnum eConfigType,
//! \param[in]  pcParamName
//! \param[in]  pbParamId
//!
//! \return     BOOL, TRUE if successful, FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ConfigGetIdFromNameByConfigType( const ConfigTypeEnum eConfigType, const CHAR *pcParamName, UINT16 *pwParamId )
{
    BOOL fSuccess = FALSE;        
    
    if( eConfigType < CONFIG_LOOKUP_ARRAY_NUM_OF_ELEMENTS ) 
    {        
        if( NULL != pcParamName )
        {
            UINT8 bParamNamePassedStrLen = 0;
            UINT8 bParamNameInConfigStrLen = 0;
            UINT8 bParamNameMaxStrLen = 0;

            bParamNamePassedStrLen = strlen( pcParamName );

            for( UINT16 i = 0 ; i < gtConfigLookupArray[eConfigType].wMetaStructNumOfElements ; i++ )
            {
                bParamNameInConfigStrLen = strlen( gtConfigLookupArray[eConfigType].ptMetaStruct[i].pcParamName );

                if( bParamNamePassedStrLen > bParamNameInConfigStrLen )
                {
                    bParamNameMaxStrLen = bParamNamePassedStrLen;
                }
                else
                {
                    bParamNameMaxStrLen = bParamNameInConfigStrLen;
                }

                if
                ( 
                    !strncasecmp
                    ( 
                        pcParamName, 
                        gtConfigLookupArray[eConfigType].ptMetaStruct[i].pcParamName,                         
                        bParamNameMaxStrLen
                    ) 
                )
                {
                    // name found! return id                    
                    *pwParamId  = gtConfigLookupArray[eConfigType].ptMetaStruct[i].wParamId;
                    fSuccess    = TRUE;                                    
                    break;
                }
            }
        }
    }    

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         BOOL ConfigShowParamFromParamId()
//!
//! \brief      Show a description of a single config param
//!
//! \param[in]  eConsolePort 
//! \param[in]  eConfigType 
//!
//! \return     BOOL, TRUE if successful, FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ConfigShowParamFromParamId( const ConsolePortEnum eConsolePort, BOOL fPrintHeader, const ConfigTypeEnum eConfigType, UINT16 wParamId )
{
#ifdef MFG_TEST_FIRMWARE_BUILD
    const CHAR *pcConfigParamInfoHeaderString               = "ParamId, ParamName, ParamType, ValueMin, ValueMax, ValueDefault, ValueCurrent, Description";
    const CHAR *pcConfigParamInfoFormatStringForString      = "0x%04X, %-17s,    %6s,       0x%08X,   0x%08X,   0x%08X,        %-22s,  %s";
    const CHAR *pcConfigParamInfoFormatStringForSigned      = "0x%04X, %-17s,    %6s,       %+10d,    %+10d,    %+10d,         %+10d,   %s";
    const CHAR *pcConfigParamInfoFormatStringForUnsigned    = "0x%04X, %-17s,    %6s,       %+10u,    %+10u,    %+10u,         %+10u,   %s";
    const CHAR *pcConfigParamInfoFormatStringForChar        = "0x%04X, %-17s,    %6s,       %11c,     %11c,     %11c,          %11c,    %s";
    const CHAR *pcConfigParamInfoFormatStringForFloat       = "0x%04X, %-17s,    %6s,       %+10.3f,  %+10.3f,  %+10.3f,       %+10.3f, %12s %s";
#else 
    const CHAR *pcConfigParamInfoHeaderString               = "ParamId, ParamName,       ParamType,  (ValueMin,   ValueMax, ValueDefault) = ValueCurrent,            Description";
//    const CHAR *pcConfigParamInfoFormatStringForString      = "0x%04X, '%-17s', 0x%02X,    (0x%08X, 0x%08X, 0x%08X) = '%-22s', '%s'";
//    const CHAR *pcConfigParamInfoFormatStringForSigned      = "0x%04X, '%-17s', 0x%02X,    (%+10d, %+10d, %+10d) = %+10d (0x%08X), '%s'";
//    const CHAR *pcConfigParamInfoFormatStringForUnsigned    = "0x%04X, '%-17s', 0x%02X,    (%+10u, %+10u, %+10u) = %+10u (0x%08X), '%s'";
//    const CHAR *pcConfigParamInfoFormatStringForChar        = "0x%04X, '%-17s', 0x%02X,    (%11c, %11c, %11c) = %11c (0x%02X, %u), '%s'";
//    const CHAR *pcConfigParamInfoFormatStringForFloat       = "0x%04X, '%-17s', 0x%02X,    (%+10.3f, %+10.3f, %+10.3f) = %+10.3f, %12s '%s'";
    const CHAR *pcConfigParamInfoFormatStringForString      = "0x%04X, '%-17s', %6s,    (0x%08X, 0x%08X, 0x%08X) = '%-22s', '%s'";
    const CHAR *pcConfigParamInfoFormatStringForSigned      = "0x%04X, '%-17s', %6s,    (%+10d, %+10d, %+10d) = %+10d (0x%08X), '%s'";
    const CHAR *pcConfigParamInfoFormatStringForUnsigned    = "0x%04X, '%-17s', %6s,    (%+10u, %+10u, %+10u) = %+10u (0x%08X), '%s'";
    const CHAR *pcConfigParamInfoFormatStringForChar        = "0x%04X, '%-17s', %6s,    (%11c, %11c, %11c) = %11c (0x%02X, %u), '%s'";
    const CHAR *pcConfigParamInfoFormatStringForFloat       = "0x%04X, '%-17s', %6s,    (%+10.3f, %+10.3f, %+10.3f) = %+10.3f, %12s '%s'";
#endif
    BOOL                        fSuccess                    = FALSE;    
    ConfigSemaphoreResultEnum   eConfigSemaphoreResult;
    ConfigParamInfoType         *ptConfigParamInfo          = NULL;
    UINT8                       bParameterSize;
    
    eConfigSemaphoreResult = ConfigSemaphoreAcquire();

    if( CONFIG_SEMAPHORE_ACQUIRED == eConfigSemaphoreResult )
    {    
        if( eConfigType < CONFIG_LOOKUP_ARRAY_NUM_OF_ELEMENTS )
        {            
            if( fPrintHeader )
            {
                ConsolePrintf( eConsolePort, (CHAR *)pcConfigParamInfoHeaderString );
                ConsolePrintf( eConsolePort, "\r\n");    
            }

            if( ConfigParamGetInfoByConfigType( eConfigType, wParamId, &ptConfigParamInfo ) )
            {                
                bParameterSize = gbConfigParamTypeSizeArray[ptConfigParamInfo->bParamType];
             
                switch( ptConfigParamInfo->bParamType )
                {
                    case CONFIG_PARAM_TYPE_BOOL:
                    case CONFIG_PARAM_TYPE_UINT8:
                    case CONFIG_PARAM_TYPE_UINT16:
                    case CONFIG_PARAM_TYPE_UINT32:
                    {
                        UINT32 dwTemporary = 0;
                        
                        memcpy( &dwTemporary, ptConfigParamInfo->pvParamData, bParameterSize );

                        ConsolePrintf( eConsolePort, (CHAR *)pcConfigParamInfoFormatStringForUnsigned,
                            ptConfigParamInfo->wParamId,
                            ptConfigParamInfo->pcParamName,
                            ConfigGetParamTypeName(ptConfigParamInfo->bParamType),
                            ptConfigParamInfo->vtParamValueMin.u32,
                            ptConfigParamInfo->vtParamValueMax.u32,
                            ptConfigParamInfo->vtParamValueDefault.u32,
                            dwTemporary,
                            dwTemporary,
                            ptConfigParamInfo->pcParamDescription
                            );
                        ConsolePrintf( eConsolePort, "\r\n");
                        fSuccess = TRUE;
                        break;
                    }
                    case CONFIG_PARAM_TYPE_INT8:
                    case CONFIG_PARAM_TYPE_INT16:
                    case CONFIG_PARAM_TYPE_INT32:
                    {
                        INT32 iTemporary = 0;                        

                        if( ptConfigParamInfo->bParamType == CONFIG_PARAM_TYPE_INT8 )
                        {
                            INT8 i8Temp = 0;
                            memcpy(&i8Temp, ptConfigParamInfo->pvParamData, bParameterSize);

                            iTemporary = i8Temp;
                        }
                        else
                        if( ptConfigParamInfo->bParamType == CONFIG_PARAM_TYPE_INT16 )
                        {
                            INT16 i16Temp = 0;
                            memcpy(&i16Temp, ptConfigParamInfo->pvParamData, bParameterSize);

                            iTemporary = i16Temp;
                        }
                        else                        
                        {
                            memcpy(&iTemporary, ptConfigParamInfo->pvParamData, bParameterSize);
                        }

                        ConsolePrintf( eConsolePort, (CHAR *)pcConfigParamInfoFormatStringForSigned,
                            ptConfigParamInfo->wParamId,
                            ptConfigParamInfo->pcParamName,
                            ConfigGetParamTypeName(ptConfigParamInfo->bParamType),
                            ptConfigParamInfo->vtParamValueMin.i32,
                            ptConfigParamInfo->vtParamValueMax.i32,
                            ptConfigParamInfo->vtParamValueDefault.i32,
                            iTemporary,
                            iTemporary,
                            ptConfigParamInfo->pcParamDescription
                            );
                        ConsolePrintf( eConsolePort, "\r\n");
                        fSuccess = TRUE;
                        break;
                    }
                    case CONFIG_PARAM_TYPE_STRING:
                    {
                        ConsolePrintf( eConsolePort, (CHAR *)pcConfigParamInfoFormatStringForString,
                            ptConfigParamInfo->wParamId,
                            ptConfigParamInfo->pcParamName,
                            ConfigGetParamTypeName(ptConfigParamInfo->bParamType),
                            ptConfigParamInfo->vtParamValueMin.i32,
                            ptConfigParamInfo->vtParamValueMax.i32,
                            ptConfigParamInfo->vtParamValueDefault.i32,
                            ptConfigParamInfo->pvParamData,
                            ptConfigParamInfo->pcParamDescription
                            );
                        ConsolePrintf( eConsolePort, "\r\n");
                        fSuccess = TRUE;
                        break;
                    }
                    case CONFIG_PARAM_TYPE_CHAR:
                    {
                        CHAR cTemporary = ' ';

                        memcpy(&cTemporary, ptConfigParamInfo->pvParamData, bParameterSize);

                        ConsolePrintf( eConsolePort, (CHAR *)pcConfigParamInfoFormatStringForChar,
                            ptConfigParamInfo->wParamId,
                            ptConfigParamInfo->pcParamName,
                            ConfigGetParamTypeName(ptConfigParamInfo->bParamType),
                            ptConfigParamInfo->vtParamValueMin.i32,
                            ptConfigParamInfo->vtParamValueMax.i32,
                            ptConfigParamInfo->vtParamValueDefault.i32,
                            cTemporary,
                            cTemporary,
                            cTemporary,
                            ptConfigParamInfo->pcParamDescription
                            );
                        ConsolePrintf( eConsolePort, "\r\n");
                        fSuccess = TRUE;
                        break;
                    }                    
                    case CONFIG_PARAM_TYPE_FLOAT:
                    {
                        SINGLE sgTemporary = 0;

                        memcpy(&sgTemporary, ptConfigParamInfo->pvParamData, bParameterSize);

                        ConsolePrintf( eConsolePort, (CHAR *)pcConfigParamInfoFormatStringForFloat,
                            ptConfigParamInfo->wParamId,
                            ptConfigParamInfo->pcParamName,
                            ConfigGetParamTypeName(ptConfigParamInfo->bParamType),
                            (SINGLE)ptConfigParamInfo->vtParamValueMin.f32,
                            (SINGLE)ptConfigParamInfo->vtParamValueMax.f32,
                            (SINGLE)ptConfigParamInfo->vtParamValueDefault.f32,
                            sgTemporary,
                            "",
                            ptConfigParamInfo->pcParamDescription
                            );
                        ConsolePrintf( eConsolePort, "\r\n");
                        fSuccess = TRUE;
                        break;
                    }
                    default:
                    {
                        //ConsolePrintf( eConsolePort, "Param Not Found\r\n");
                        //fSuccess = FALSE;
                        break;
                    }
                }
            }
//            else
//            {
//                ConsolePrintf( eConsolePort, "Param Not Found\r\n");
//            }
        }
    }

    ConfigSemaphoreRelease( eConfigSemaphoreResult );

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         BOOL ConfigShowListByConfigType( const ConsolePortEnum eConsolePort, const ConfigTypeEnum eConfigType )
//!
//! \brief      Show a description for all configuration parameters that belong to a specific configuration.
//!
//! \param[in]  eConsolePort 
//! \param[in]  eConfigType 
//!
//! \return     BOOL, TRUE if successful, FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ConfigShowListByConfigType( const ConsolePortEnum eConsolePort, const ConfigTypeEnum eConfigType )
{
    BOOL fSuccess = FALSE;    
    BOOL fPrintHeader = FALSE;

    fSuccess = TRUE;
    for( UINT16 i = 0; i < gtConfigLookupArray[eConfigType].wMetaStructNumOfElements ; i++ )
    {
        if( 0 == i )
        {
            fPrintHeader = TRUE;
        }
        else
        {
            fPrintHeader = FALSE;
        }
    
        fSuccess &= ConfigShowParamFromParamId( eConsolePort, fPrintHeader, eConfigType, gtConfigLookupArray[eConfigType].ptMetaStruct[i].wParamId );
    }
    
     return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         BOOL ConfigShowListAll( const ConsolePortEnum eConsolePort )
//!
//! \brief      Show a description for all configuration parameters
//!
//! \param[in]  none.
//!
//! \return     BOOL, TRUE if successful, FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ConfigShowListAll( const ConsolePortEnum eConsolePort )
{
    BOOL fSuccess = TRUE;
    
    ConsolePrintf( eConsolePort , " Device information\r\n" );
    fSuccess &= ConfigShowListByConfigType( eConsolePort, CONFIG_TYPE_DEVICE_INFO );
    ConsolePrintf( eConsolePort , " Settings\r\n" );
    fSuccess &= ConfigShowListByConfigType( eConsolePort, CONFIG_TYPE_SETTINGS );
    ConsolePrintf( eConsolePort , " Modbus\r\n" );
    fSuccess &= ConfigShowListByConfigType( eConsolePort, CONFIG_TYPE_MODBUS );
    ConsolePrintf( eConsolePort , " Adc Calibration\r\n" );
    fSuccess &= ConfigShowListByConfigType( eConsolePort, CONFIG_TYPE_ADC_CALIBRATION );
    ConsolePrintf( eConsolePort , " Communications\r\n" );
    fSuccess &= ConfigShowListByConfigType( eConsolePort, CONFIG_TYPE_COMMUNICATIONS );
    ConsolePrintf( eConsolePort , " Fluid Depth\r\n" );
    fSuccess &= ConfigShowListByConfigType( eConsolePort, CONFIG_TYPE_FLUID );

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         BOOL ConfigParamGetInfoByConfigType( const ConfigTypeEnum eConfigType, const UINT8 bParamId, ConfigParamInfoType **ptConfigParamInfo )
//!
//! \brief      Get the description structure pointer for a specific parameter of a configuration type.
//!
//! \param[in]  eConfigType 
//! \param[in]  bParamId 
//! \param[in]  ptConfigParamInfo
//!
//! \return     BOOL, TRUE if successful, FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ConfigParamGetInfoByConfigType( const ConfigTypeEnum eConfigType, const UINT16 wParamId, ConfigParamInfoType **ptConfigParamInfo )
{
    BOOL fSuccess = FALSE;            

    if( eConfigType < CONFIG_LOOKUP_ARRAY_NUM_OF_ELEMENTS ) 
    {            
        for(UINT16 wIndex = 0 ; wIndex < gtConfigLookupArray[eConfigType].wMetaStructNumOfElements ; wIndex++ )
        {    
            if( wParamId == gtConfigLookupArray[eConfigType].ptMetaStruct[wIndex].wParamId )
            {                                                
                *ptConfigParamInfo = &gtConfigLookupArray[eConfigType].ptMetaStruct[wIndex];

                fSuccess = TRUE;
                break;
            }
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         UINT8 ConfigGetUpdateCounterByConfigType( const ConfigTypeEnum eConfigType )
//!
//! \brief      returns the number of times the particular config type has been updated
//!             ( must be saved, otherwise it doesnt count)
//!
//! \param[in]  eConfigType 
//!
//! \return     Update counter
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
UINT8 ConfigGetUpdateCounterByConfigType( const ConfigTypeEnum eConfigType )
{    
    if( eConfigType < CONFIG_LOOKUP_ARRAY_NUM_OF_ELEMENTS ) 
    {     
        return gtConfigLookupArray[eConfigType].bUpdateConfigCounter;        
    }
    else
    {
        return 0;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn         BOOL ConfigGetConfigCopyByConfigType( const ConfigTypeEnum eConfigType, void * pvConfigStruct, UINT16 wConfigStructSize )
//!
//! \brief      returns a copy of the structure with the current state of all the params
//!
//! \param[in]  eConfigType         type of config to get copy
//! \param[in]  pvConfigStruct      the structure that will hold the copy. Make sure it has the correct size
//! \param[in]  wConfigStructSize   size of the struct to hold the data.
//!
//! \return     BOOL, TRUE if successful, FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
//BOOL ConfigGetConfigCopyByConfigType( const ConfigTypeEnum eConfigType, void * pvConfigStruct, UINT32 dwConfigStructSize )
BOOL    ConfigGetPtrConfigCopyByConfigType  ( const ConfigTypeEnum eConfigType, void ** pvConfigStruct )
{
    BOOL fSuccess = FALSE;

    if( eConfigType < CONFIG_LOOKUP_ARRAY_NUM_OF_ELEMENTS ) 
    {
        if( NULL != pvConfigStruct )
        {
            // validate the size
            //if( gtConfigLookupArray[eConfigType].dwDataStructSize == dwConfigStructSize )
            {
                fSuccess = TRUE;
                //memcpy( pvConfigStruct , gtConfigLookupArray[eConfigType].pvDataStruct , dwConfigStructSize );
                (*pvConfigStruct) = gtConfigLookupArray[eConfigType].pvDataStruct;
            }
        }
    }

    return fSuccess;
}


BOOL ConfigGetNumberOfParamsByConfigType( const ConfigTypeEnum eConfigType, UINT16 *pwMaxNumberOfParams )
{
    BOOL fSuccess = FALSE;

    if( eConfigType < CONFIG_LOOKUP_ARRAY_NUM_OF_ELEMENTS ) 
    {
        if( NULL != pwMaxNumberOfParams )
        {
            *pwMaxNumberOfParams = gtConfigLookupArray[eConfigType].wMetaStructNumOfElements;

            fSuccess = TRUE;
        }
    }

    return fSuccess;
}

BOOL ConfigGetCurrentParamConfigString( const ConfigTypeEnum eConfigType, UINT32 dwParamNum, CHAR *pcStringBuffer, UINT16 wStringBufferSize, UINT16 *pwNumOfBytesWrittenInBuffer )
{
    BOOL fSuccess = FALSE;

    ConfigSemaphoreResultEnum eConfigSemaphoreResult;

    // example of command: config set setting 150 1
    //const CHAR *pcConfigSetGeneralFormatString1                 = "%s %s %04X %s\r\n";
    const CHAR *pcConfigSetGeneralFormatString2                 = "%s %s %s %s\r\n";
    const CHAR *pcConfigSetCommnadString                        = "config set";    
    const CHAR *pcConfigSetParamInfoFormatStringForSigned       = "%+d";
    const CHAR *pcConfigSetParamInfoFormatStringForUnsigned     = "%u";
    const CHAR *pcConfigSetParamFormatStringForString           = "`%s`";
    const CHAR *pcConfigSetParamInfoFormatStringForChar         = "%c";
    const CHAR *pcConfigSetParamInfoFormatStringForFloat        = "%+f";

    CHAR    cTempStringBuffer[100];
    UINT16  wNumOfBytesWrittenInBuffer;

    if( eConfigType < CONFIG_LOOKUP_ARRAY_NUM_OF_ELEMENTS ) 
    {
        if( NULL != pcStringBuffer )
        {        
            ////////////////////////////////////////////////
            ////////////////////////////////////////////////
            eConfigSemaphoreResult = ConfigSemaphoreAcquire();

            if( CONFIG_SEMAPHORE_ACQUIRED == eConfigSemaphoreResult )
            {            
                if( dwParamNum < gtConfigLookupArray[eConfigType].wMetaStructNumOfElements )
                {              
                    UINT8 bParameterSize = 0;                    

                    // get size
                    if( (gtConfigLookupArray[eConfigType].ptMetaStruct + dwParamNum)->bParamType == CONFIG_PARAM_TYPE_STRING )
                    {
                        bParameterSize = (gtConfigLookupArray[eConfigType].ptMetaStruct + dwParamNum)->vtParamValueMax.i32;
                    }
                    else
                    {
                        bParameterSize = gbConfigParamTypeSizeArray[(gtConfigLookupArray[eConfigType].ptMetaStruct + dwParamNum)->bParamType];
                    }
                    
                    switch( (gtConfigLookupArray[eConfigType].ptMetaStruct + dwParamNum)->bParamType )
                    {
                        case CONFIG_PARAM_TYPE_BOOL:
                        case CONFIG_PARAM_TYPE_UINT8:
                        case CONFIG_PARAM_TYPE_UINT16:
                        case CONFIG_PARAM_TYPE_UINT32:
                        {
                            UINT32 dwTemporary = 0;
                        
                            memcpy( &dwTemporary, (gtConfigLookupArray[eConfigType].ptMetaStruct + dwParamNum)->pvParamData, bParameterSize );

                            snprintf
                            ( 
                                cTempStringBuffer, sizeof(cTempStringBuffer), 
                                pcConfigSetParamInfoFormatStringForUnsigned,                                
                                dwTemporary
                            );
                            
                            fSuccess = TRUE;
                            break;
                        }


                        case CONFIG_PARAM_TYPE_INT8:
                        case CONFIG_PARAM_TYPE_INT16:
                        case CONFIG_PARAM_TYPE_INT32:
                        {
                            INT32 iTemporary = 0;

                            if( (gtConfigLookupArray[eConfigType].ptMetaStruct + dwParamNum)->bParamType == CONFIG_PARAM_TYPE_INT8 )
                            {
                                INT8 i8Temp = 0;
                                memcpy(&i8Temp, (gtConfigLookupArray[eConfigType].ptMetaStruct + dwParamNum)->pvParamData, bParameterSize);

                                iTemporary = i8Temp;
                            }
                            else
                            if( (gtConfigLookupArray[eConfigType].ptMetaStruct + dwParamNum)->bParamType == CONFIG_PARAM_TYPE_INT16 )
                            {
                                INT16 i16Temp = 0;
                                memcpy(&i16Temp, (gtConfigLookupArray[eConfigType].ptMetaStruct + dwParamNum)->pvParamData, bParameterSize);

                                iTemporary = i16Temp;
                            }
                            else                        
                            {
                                memcpy(&iTemporary, (gtConfigLookupArray[eConfigType].ptMetaStruct + dwParamNum)->pvParamData, bParameterSize);
                            }                            

                            snprintf
                            ( 
                                cTempStringBuffer, sizeof(cTempStringBuffer), 
                                pcConfigSetParamInfoFormatStringForSigned,                                
                                iTemporary
                            );

                            fSuccess = TRUE;
                            break;
                        }


                        case CONFIG_PARAM_TYPE_STRING:
                        {
                            snprintf
                            ( 
                                cTempStringBuffer, sizeof(cTempStringBuffer), 
                                pcConfigSetParamFormatStringForString,                                
                                (gtConfigLookupArray[eConfigType].ptMetaStruct + dwParamNum)->pvParamData
                            );

                            fSuccess = TRUE;
                            break;
                        }


                        case CONFIG_PARAM_TYPE_CHAR:
                        {
                            CHAR cTemporary = ' ';

                            memcpy(&cTemporary, (gtConfigLookupArray[eConfigType].ptMetaStruct + dwParamNum)->pvParamData, bParameterSize);

                            snprintf
                            ( 
                                cTempStringBuffer, sizeof(cTempStringBuffer), 
                                pcConfigSetParamInfoFormatStringForChar,                                
                                cTemporary
                            );

                            fSuccess = TRUE;
                            break;
                        } 
                                           
                        case CONFIG_PARAM_TYPE_FLOAT:
                        {
                            SINGLE sgTemporary = 0;

                            memcpy(&sgTemporary, (gtConfigLookupArray[eConfigType].ptMetaStruct + dwParamNum)->pvParamData, bParameterSize);

                            snprintf
                            ( 
                                cTempStringBuffer, sizeof(cTempStringBuffer), 
                                pcConfigSetParamInfoFormatStringForFloat,                                
                                sgTemporary
                            );

                            fSuccess = TRUE;
                            break;
                        }

                    }// end switch

                    if( fSuccess )
                    {
                        //wNumOfBytesWrittenInBuffer = snprintf( pcStringBuffer, wStringBufferSize, pcConfigSetGeneralFormatString1,
                        wNumOfBytesWrittenInBuffer = snprintf( pcStringBuffer, wStringBufferSize, pcConfigSetGeneralFormatString2,
                            pcConfigSetCommnadString,
                            gtConfigLookupArray[eConfigType].pcName,
                            //(gtConfigLookupArray[eConfigType].ptMetaStruct + dwParamNum)->wParamId,
                            (gtConfigLookupArray[eConfigType].ptMetaStruct + dwParamNum)->pcParamName,
                            &cTempStringBuffer[0]
                        );

                        if( NULL != pwNumOfBytesWrittenInBuffer )
                        {
                            *pwNumOfBytesWrittenInBuffer = wNumOfBytesWrittenInBuffer;
                        }
                    }

                }// end if(max num of elements)

                ConfigSemaphoreRelease( eConfigSemaphoreResult );
            }// end semaphore acquire               
        }
    }

    return fSuccess;
}

CHAR *  ConfigGetConfigTypeName( const ConfigTypeEnum eConfigType )
{
    if( eConfigType < CONFIG_LOOKUP_ARRAY_NUM_OF_ELEMENTS ) 
    {
        // is a read operation on a constant value...no semaphore required
        return gtConfigLookupArray[eConfigType].pcName;
    }
    else
    {
        return NULL;
    }
}

BOOL ConfigGetParamType( const ConfigTypeEnum eConfigType, const UINT16 wParamId, ConfigParameterTypeEnum *peParamType )
{
    BOOL                        fSuccess                = FALSE;
    ConfigParamInfoType         *ptConfigParamInfo      = NULL;

    if( eConfigType < CONFIG_LOOKUP_ARRAY_NUM_OF_ELEMENTS ) 
    {
        if( NULL != peParamType )
        {
            if( ConfigParamGetInfoByConfigType( eConfigType, wParamId, &ptConfigParamInfo ) )
            {
                switch( ptConfigParamInfo->bParamType )
                {
                    case CONFIG_PARAM_TYPE_UINT8:
                    case CONFIG_PARAM_TYPE_UINT16:
                    case CONFIG_PARAM_TYPE_UINT32:
                    case CONFIG_PARAM_TYPE_INT8:
                    case CONFIG_PARAM_TYPE_INT16:
                    case CONFIG_PARAM_TYPE_INT32:
                    case CONFIG_PARAM_TYPE_BOOL:
                    case CONFIG_PARAM_TYPE_STRING:
                    case CONFIG_PARAM_TYPE_CHAR:
                    case CONFIG_PARAM_TYPE_FLOAT:
                        fSuccess = TRUE;
                        (*peParamType) = ptConfigParamInfo->bParamType;
                        break;
                }
            }            
        }
    }

    return fSuccess;
}

CHAR *  ConfigGetParamTypeName( ConfigParameterTypeEnum eParamType )
{
    switch( eParamType )
    {
        case CONFIG_PARAM_TYPE_UINT8:
        case CONFIG_PARAM_TYPE_UINT16:
        case CONFIG_PARAM_TYPE_UINT32:
        case CONFIG_PARAM_TYPE_INT8:
        case CONFIG_PARAM_TYPE_INT16:
        case CONFIG_PARAM_TYPE_INT32:
        case CONFIG_PARAM_TYPE_BOOL:
        case CONFIG_PARAM_TYPE_STRING:
        case CONFIG_PARAM_TYPE_CHAR:
        case CONFIG_PARAM_TYPE_FLOAT:
            return (CHAR *)&gtConfigParamTypeNameArray[eParamType][0];
            break;
        default:
            return "UNDEFINED";
            break;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////

/* EOF */

