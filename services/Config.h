//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \file       ConfigParameters.h
//!
//! \brief      Configuration parameter listing file
//!
//! \author     Joel Minski, Puracom Inc.
//!
//! \date       2012-05-16
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _CONFIG_H_
#define _CONFIG_H_

//////////////////////////////////////////////////////////////////////////////////////////////////
// Includes
//////////////////////////////////////////////////////////////////////////////////////////////////

// this should be saved in eprom 4K

//////////////////////////////////////////////////////////////////////////////////////////////////
// Defines
//////////////////////////////////////////////////////////////////////////////////////////////////

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// Flash memory boundaries
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
#define CONFIG_DEV_INFO_STORAGE_MAGIC_NUMBER_BYTE               (0xA5)
#define CONFIG_DEV_INFO_STORAGE_VERSION_NUMBER_BYTE             (0x01)

#define CONFIG_SETTINGS_STORAGE_MAGIC_NUMBER_BYTE               (0x5A)
#define CONFIG_SETTINGS_STORAGE_VERSION_NUMBER_BYTE             (0x01)

#define CONFIG_COMMUNICATIONS_STORAGE_MAGIC_NUMBER_BYTE         (0xc3)
#define CONFIG_COMMUNICATIONS_STORAGE_VERSION_NUMBER_BYTE       (0x01)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// Defines for size of arrays and strings
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// DEVICE INFORMATION
#define CONFIG_PARAM_STRING_COSTUMER_COMPANY_NAME_STRING        (20)
#define CONFIG_PARAM_STRING_COSTUMER_DEVICE_NAME_STRING         (20)
#define CONFIG_PARAM_STRING_INTERNAL_DEVICE_NAME_STRING         (20)
#define CONFIG_PARAM_STRING_INTERNAL_MANUFACTURING_DATE_STRING  (15)
#define CONFIG_PARAM_STRING_DESCRIPTION_STRING                  (30)
#define CONFIG_PARAM_STRING_LENGTH_MODEM_IMEI                   (15)
#define CONFIG_PARAM_STRING_LENGTH_SIM_ICCID                    (20)

// SETTINGS
#define CONFIG_ADC_INPUT_EXTERNAL_CHANNELS_MAX                  (6) 
#define CONFIG_MAX_CAM_AMOUNT                                   (2)
#define CONFIG_EXT_DEV_PORT_AMOUNT                              (2)
#define CONFIG_CAM_CAPTURE_MINUTE_OFFSET_ARRAY_MAX              (24)
#define CONFIG_TOTAL_GET_COUNT_MINUTE_OFFSET_ARRAY_MAX          (24)
#define CONFIG_TRANSMISSION_MINUTE_OFFSET_ARRAY_MAX             (48)
#define CONFIG_MODBUS_REGISTER_COMMAND_ARRAY_MAX                (15)

#define CONFIG_FLUID_DEPTH_MINUTE_OFFSET_ARRAY_MAX              (12)

#define CONFIG_PARAM_STRING_LENGTH_TOWER_MCC                    (10)
#define CONFIG_PARAM_STRING_LENGTH_TOWER_MNC                    (10)
#define CONFIG_PARAM_STRING_LENGTH_TOWER_LAC                    (10)
#define CONFIG_PARAM_STRING_LENGTH_TOWER_CID                    (10)

// ADC CALIBRATION

// COMM
#define CONFIG_PARAM_STRING_LENGTH_URL                          (30)

// FLUID
#define CONFIG_PARAM_STRING_LENGTH_FDM_FILE_FORMAT              (9) // originally is 10 by in order to keep 1 byte for end of string

//////////////////////////////////////////////////////////////////////////////////////////////////
// Type Definitions
//////////////////////////////////////////////////////////////////////////////////////////////////

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// DEVICE INFORMATION 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
typedef struct 
{
	// NOTE:  All string parameters must be instantiated as one larger than specified in the param meta definition array inside Config.c.
	//			See where the "CONFIG_PARAM_TYPE_STRING" case is used inside ConfigSettingsParamAccess() function for more information.
    // Internal Device Information    
    CHAR    cInternalDeviceName             [ CONFIG_PARAM_STRING_INTERNAL_DEVICE_NAME_STRING + 1 ];    
    CHAR    cInternalManufacturingDate      [ CONFIG_PARAM_STRING_INTERNAL_MANUFACTURING_DATE_STRING + 1 ];

    // MODEM
    UINT8   cModemImei                      [ CONFIG_PARAM_STRING_LENGTH_MODEM_IMEI + 1 ];

	// Warning!  Only add new items to the end of this typedef.
	// Otherwise field locations will move and reverse compatibility will be harder.	
} PACKED ConfigDevInfoType;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// Settings
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
typedef struct
{
	// NOTE:  All string parameters must be instantiated as one larger than specified in the param meta definition array inside Config.c.
	//			See where the "CONFIG_PARAM_TYPE_STRING" case is used inside ConfigSettingsParamAccess() function for more information.    
    BOOL    fDetectSerialConnectedEnable;
	BOOL    fPrintDebugEnable;
    
    SINGLE  sgSysUtcTimeZoneOffset;
    
    // Warning!  Only add new items to the end of this typedef.
	// Otherwise field locations will move and reverse compatibility will be harder.
} PACKED ConfigSettingsType;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// DEVICE DEFAULT COMMUNICATION SETTINGS
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
typedef struct
{    
    // NOTE:  All string parameters must be instantiated as one larger than specified in the param meta definition array inside Config.c.
	//			See where the "CONFIG_PARAM_TYPE_STRING" case is used inside ConfigSettingsParamAccess() function for more information.	    

    // APN
    UINT8   cModemApnAddress                [ CONFIG_PARAM_STRING_LENGTH_URL + 1 ];

    UINT8   cCloudDomainAddress             [ CONFIG_PARAM_STRING_LENGTH_URL + 1 ];
	
	UINT32  dwSocketPort;
	UINT16  wFtpPort;
	UINT32  dwHttpPort;
	UINT32  dwTimeServerPort;

	// Warning!  Only add new items to the end of this typedef.
	// Otherwise field locations will move and reverse compatibility will be harder.	
} PACKED ConfigCommunicationsType;

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

// DEVICE INFORMATION
// data used in manufacturing... machine should not self change this params
typedef enum
{
    // Customer Product Information
    CONFIG_DI_CUSTOMER_COMPANY_NAME_STRING              = 0x0001,
    CONFIG_DI_CUSTOMER_DEVICE_NAME_STRING               = 0x0002,

    // Internal Device Information
    CONFIG_DI_INTERNAL_DEVICE_NAME_STRING               = 0x0011,        
    CONFIG_DI_INTERNAL_MANUFACTURING_DATE_STRING        = 0x0012,

    // HARDWARE    
    CONFIG_DI_HARDWARE_MAIN_BOARD_VERSION_UINT32        = 0x0021,
    CONFIG_DI_HARDWARE_INT_BOARD_VERSION_UINT32         = 0x0022,
    CONFIG_DI_HARDWARE_BATERY_DESCRIPTION_STRING        = 0x0023,
    CONFIG_DI_HARDWARE_CAMERA_1_DESCRIPTION_STRING      = 0x0024,
    CONFIG_DI_HARDWARE_CAMERA_2_DESCRIPTION_STRING      = 0x0025,
    CONFIG_DI_HARDWARE_ANTENNA_DESCRIPTION_STRING       = 0x0026,

    // MODEM
    CONFIG_DI_MODEM_IMEI_NUMBER_STRING                  = 0x0031,
    CONFIG_DI_MODEM_SIM_ICCID_NUMBER_STRING             = 0x0032,
} ConfigDevInfoPidEnum;

// SETTINGS
typedef enum
{     
    //  System    
    CONFIG_SETTING_MODE                                 = 0x0000,

    // FILE PENDING FOR QUEUE    
    CONFIG_SETTING_FILE_PENDING_QUEUE_FLAG              = 0x0001,
    CONFIG_SETTING_FILE_PENDING_QUEUE_FILE_TYPE         = 0x0002,
    CONFIG_SETTING_FILE_PENDING_QUEUE_FILE_SUBSECTOR    = 0x0003,

    CONFIG_DI_FIRMWARE_CRC32_UINT32                     = 0x0004,

    CONFIG_SETTING_LED_BLINK_ENABLE                     = 0x0010,
    CONFIG_SETTING_DETECT_SERIAL_CONNECTED_ENABLE       = 0x0011,
    CONFIG_SETTING_ON_ALARM_TRANFER_IMG_ENABLE          = 0x0012,
    CONFIG_SETTING_TRANSMIT_MIN_OFFSET_ENABLE           = 0x0013,
    CONFIG_SETTING_MERGE_OTHER_DATA_TO_HOURLY           = 0x0014,
    
    CONFIG_SETTING_SYS_UTC_TIME_ZONE_OFFSET             = 0x0015,
    CONFIG_SETTING_IS_AUTO_TIME_ZONE_ENABLED            = 0x0016,

    CONFIG_SETTING_BLUETOOTH_SYNC_DEV_TIME_OUT          = 0x0020,

    CONFIG_SETTING_NETWORK_AND_SYSTEM_TIME_DELTA_SECONDS= 0x0030,

    CONFIG_SETTING_TRANSFER_MINUTE_FILES_ENABLED        = 0x0031,
    CONFIG_SETTING_TRANSFER_EVENT_FILES_ENABLED         = 0x0032,
    CONFIG_SETTING_TRANSFER_ALERT_FILES_ENABLED         = 0x0033,
    CONFIG_SETTING_TRANSFER_SIGNAL_ENABLED              = 0x0034,
    
    //  ADC    
    CONFIG_ADC_SENSOR_SAMPLING_PERIOD_MINUTES           = 0x0040,

    CONFIG_SENSOR_CORRECTION_ENABLED                    = 0x0050,
    CONFIG_ADC_CHAN_CORRECTION_ENABLED                  = 0x0051,
    CONFIG_ADC_CHAN_MEASURE_DELTA_ENABLED               = 0x0052,
    CONFIG_ADC_CHAN_ALARM_THRESHOLD_LOW_ENABLED         = 0x0053,    
    CONFIG_ADC_CHAN_ALARM_THRESHOLD_HIGH_ENABLED        = 0x0054,    
    CONFIG_ADC_CHAN_SAMPLING_SHORT_ENABLED_BIT_MASK     = 0x0055,
    CONFIG_ADC_CHAN_SAMPLING_LONG_ENABLED_BIT_MASK      = 0x0056,
    CONFIG_ADC_CHAN_SAMPLING_LONG_TRIGGER_MINUTES       = 0x0057,
    CONFIG_ADC_CHAN_SAMPLING_LONG_ADC_READINGS_PER_SMPL = 0x0058,
    CONFIG_ADC_CHAN_SAMPLING_LONG_SMPL_PERIOD_MILLISEC  = 0x0059,
    CONFIG_ADC_CHAN_SAMPLING_LONG_RECORDING_TIME_SECONDS= 0x005A,    
    CONFIG_ADC_CHAN_SAMPLING_LONG_INIT_MIN_OFFSET_DAY   = 0x005B,
    
    CONFIG_ADC_CHAN_1_MODE                              = 0x0060,
    CONFIG_ADC_CHAN_2_MODE                              = 0x0061,
    CONFIG_ADC_CHAN_3_MODE                              = 0x0062,
    CONFIG_ADC_CHAN_4_MODE                              = 0x0063,
    CONFIG_ADC_CHAN_5_MODE                              = 0x0064,
    CONFIG_ADC_CHAN_6_MODE                              = 0x0065,

    CONFIG_ADC_CHAN_1_ALARM_THRESHOLD_LOW               = 0x0090,
    CONFIG_ADC_CHAN_2_ALARM_THRESHOLD_LOW               = 0x0091,
    CONFIG_ADC_CHAN_3_ALARM_THRESHOLD_LOW               = 0x0092,
    CONFIG_ADC_CHAN_4_ALARM_THRESHOLD_LOW               = 0x0093,
    CONFIG_ADC_CHAN_5_ALARM_THRESHOLD_LOW               = 0x0094,
    CONFIG_ADC_CHAN_6_ALARM_THRESHOLD_LOW               = 0x0095,

    CONFIG_ADC_CHAN_1_ALARM_THRESHOLD_HIGH              = 0x00A0,
    CONFIG_ADC_CHAN_2_ALARM_THRESHOLD_HIGH              = 0x00A1,
    CONFIG_ADC_CHAN_3_ALARM_THRESHOLD_HIGH              = 0x00A2,
    CONFIG_ADC_CHAN_4_ALARM_THRESHOLD_HIGH              = 0x00A3,
    CONFIG_ADC_CHAN_5_ALARM_THRESHOLD_HIGH              = 0x00A4,
    CONFIG_ADC_CHAN_6_ALARM_THRESHOLD_HIGH              = 0x00A5,
    
    CONFIG_ADC_CHAN_1_ALARM_EXIST_DEBOUNCE_MINUTES      = 0x00B0,
    CONFIG_ADC_CHAN_2_ALARM_EXIST_DEBOUNCE_MINUTES      = 0x00B1,
    CONFIG_ADC_CHAN_3_ALARM_EXIST_DEBOUNCE_MINUTES      = 0x00B2,
    CONFIG_ADC_CHAN_4_ALARM_EXIST_DEBOUNCE_MINUTES      = 0x00B3,
    CONFIG_ADC_CHAN_5_ALARM_EXIST_DEBOUNCE_MINUTES      = 0x00B4,
    CONFIG_ADC_CHAN_6_ALARM_EXIST_DEBOUNCE_MINUTES      = 0x00B5,

    CONFIG_ADC_CHAN_1_ALARM_IS_OVER_DEBOUNCE_MINUTES    = 0x00C0,
    CONFIG_ADC_CHAN_2_ALARM_IS_OVER_DEBOUNCE_MINUTES    = 0x00C1,
    CONFIG_ADC_CHAN_3_ALARM_IS_OVER_DEBOUNCE_MINUTES    = 0x00C2,
    CONFIG_ADC_CHAN_4_ALARM_IS_OVER_DEBOUNCE_MINUTES    = 0x00C3,
    CONFIG_ADC_CHAN_5_ALARM_IS_OVER_DEBOUNCE_MINUTES    = 0x00C4,
    CONFIG_ADC_CHAN_6_ALARM_IS_OVER_DEBOUNCE_MINUTES    = 0x00C5,
    
    //  Cam    
    CONFIG_CAM_ENABLE                                   = 0x00E0,

    CONFIG_CAM1_RESOLUTION                              = 0x00E2,
    CONFIG_CAM2_RESOLUTION                              = 0x00E3,

    CONFIG_CAM1_QUALITY                                 = 0x00E4,
    CONFIG_CAM2_QUALITY                                 = 0x00E5,
    
    CONFIG_CAM1_ORIENTATION                             = 0x00E6,
    CONFIG_CAM2_ORIENTATION                             = 0x00E7,

    CONFIG_CAM1_WINDOW_X0                               = 0x00E8,
    CONFIG_CAM1_WINDOW_Y0                               = 0x00E9,
    CONFIG_CAM1_WINDOW_X1                               = 0x00EA,
    CONFIG_CAM1_WINDOW_Y1                               = 0x00EB,

    CONFIG_CAM2_WINDOW_X0                               = 0x00EC,
    CONFIG_CAM2_WINDOW_Y0                               = 0x00ED,
    CONFIG_CAM2_WINDOW_X1                               = 0x00EE,
    CONFIG_CAM2_WINDOW_Y1                               = 0x00EF,

    // Totalizer
    CONFIG_TOTALIZER_ENABLE_BIT_MASK                    = 0x00F0,
    CONFIG_TOTALIZER_DELTA_ENABLE                       = 0x00F1,
    CONFIG_TOTALIZER_DELTA_PERIOD_TYPE                  = 0x00F2,
    CONFIG_TOTALIZER_DELTA_MINS_PERIOD15                = 0x00F3,
    CONFIG_TOTALIZER_DELTA_MINS_PERIOD30                = 0x00F4,
    
    //   array for cam 1 capture minute offset of the day    
    CONFIG_CAM_1_MINUTE_OFFSET_01_ON_THE_DAY            = 0x0100,
    CONFIG_CAM_1_MINUTE_OFFSET_02_ON_THE_DAY            = 0x0101,
    CONFIG_CAM_1_MINUTE_OFFSET_03_ON_THE_DAY            = 0x0102,
    CONFIG_CAM_1_MINUTE_OFFSET_04_ON_THE_DAY            = 0x0103,
    CONFIG_CAM_1_MINUTE_OFFSET_05_ON_THE_DAY            = 0x0104,
    CONFIG_CAM_1_MINUTE_OFFSET_06_ON_THE_DAY            = 0x0105,

    CONFIG_CAM_1_MINUTE_OFFSET_07_ON_THE_DAY            = 0x0106,
    CONFIG_CAM_1_MINUTE_OFFSET_08_ON_THE_DAY            = 0x0107,
    CONFIG_CAM_1_MINUTE_OFFSET_09_ON_THE_DAY            = 0x0108,
    CONFIG_CAM_1_MINUTE_OFFSET_10_ON_THE_DAY            = 0x0109,
    CONFIG_CAM_1_MINUTE_OFFSET_11_ON_THE_DAY            = 0x010A,
    CONFIG_CAM_1_MINUTE_OFFSET_12_ON_THE_DAY            = 0x010B,

    CONFIG_CAM_1_MINUTE_OFFSET_13_ON_THE_DAY            = 0x010C,
    CONFIG_CAM_1_MINUTE_OFFSET_14_ON_THE_DAY            = 0x010D,
    CONFIG_CAM_1_MINUTE_OFFSET_15_ON_THE_DAY            = 0x010E,
    CONFIG_CAM_1_MINUTE_OFFSET_16_ON_THE_DAY            = 0x010F,
    CONFIG_CAM_1_MINUTE_OFFSET_17_ON_THE_DAY            = 0x0110,
    CONFIG_CAM_1_MINUTE_OFFSET_18_ON_THE_DAY            = 0x0111,

    CONFIG_CAM_1_MINUTE_OFFSET_19_ON_THE_DAY            = 0x0112,
    CONFIG_CAM_1_MINUTE_OFFSET_20_ON_THE_DAY            = 0x0113,
    CONFIG_CAM_1_MINUTE_OFFSET_21_ON_THE_DAY            = 0x0114,
    CONFIG_CAM_1_MINUTE_OFFSET_22_ON_THE_DAY            = 0x0115,
    CONFIG_CAM_1_MINUTE_OFFSET_23_ON_THE_DAY            = 0x0116,
    CONFIG_CAM_1_MINUTE_OFFSET_24_ON_THE_DAY            = 0x0117,
    
    //   array for totalizer get count minute offset of the day    
    CONFIG_TOTALIZER_MINUTE_OFFSET_01_ON_THE_DAY        = 0x0120,
    CONFIG_TOTALIZER_MINUTE_OFFSET_02_ON_THE_DAY        = 0x0121,
    CONFIG_TOTALIZER_MINUTE_OFFSET_03_ON_THE_DAY        = 0x0122,
    CONFIG_TOTALIZER_MINUTE_OFFSET_04_ON_THE_DAY        = 0x0123,
    CONFIG_TOTALIZER_MINUTE_OFFSET_05_ON_THE_DAY        = 0x0124,
    CONFIG_TOTALIZER_MINUTE_OFFSET_06_ON_THE_DAY        = 0x0125,

    CONFIG_TOTALIZER_MINUTE_OFFSET_07_ON_THE_DAY        = 0x0126,
    CONFIG_TOTALIZER_MINUTE_OFFSET_08_ON_THE_DAY        = 0x0127,
    CONFIG_TOTALIZER_MINUTE_OFFSET_09_ON_THE_DAY        = 0x0128,
    CONFIG_TOTALIZER_MINUTE_OFFSET_10_ON_THE_DAY        = 0x0129,
    CONFIG_TOTALIZER_MINUTE_OFFSET_11_ON_THE_DAY        = 0x012A,
    CONFIG_TOTALIZER_MINUTE_OFFSET_12_ON_THE_DAY        = 0x012B,

    CONFIG_TOTALIZER_MINUTE_OFFSET_13_ON_THE_DAY        = 0x012C,
    CONFIG_TOTALIZER_MINUTE_OFFSET_14_ON_THE_DAY        = 0x012D,
    CONFIG_TOTALIZER_MINUTE_OFFSET_15_ON_THE_DAY        = 0x012E,
    CONFIG_TOTALIZER_MINUTE_OFFSET_16_ON_THE_DAY        = 0x012F,
    CONFIG_TOTALIZER_MINUTE_OFFSET_17_ON_THE_DAY        = 0x0130,
    CONFIG_TOTALIZER_MINUTE_OFFSET_18_ON_THE_DAY        = 0x0131,

    CONFIG_TOTALIZER_MINUTE_OFFSET_19_ON_THE_DAY        = 0x0132,
    CONFIG_TOTALIZER_MINUTE_OFFSET_20_ON_THE_DAY        = 0x0133,
    CONFIG_TOTALIZER_MINUTE_OFFSET_21_ON_THE_DAY        = 0x0134,
    CONFIG_TOTALIZER_MINUTE_OFFSET_22_ON_THE_DAY        = 0x0135,
    CONFIG_TOTALIZER_MINUTE_OFFSET_23_ON_THE_DAY        = 0x0136,
    CONFIG_TOTALIZER_MINUTE_OFFSET_24_ON_THE_DAY        = 0x0137,
    
    //  array for transmissions along the day    
    CONFIG_TRANSMISSION_MINUTE_OFFSET_01_ON_THE_DAY     = 0x0140,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_02_ON_THE_DAY     = 0x0141,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_03_ON_THE_DAY     = 0x0142,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_04_ON_THE_DAY     = 0x0143,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_05_ON_THE_DAY     = 0x0144,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_06_ON_THE_DAY     = 0x0145,

    CONFIG_TRANSMISSION_MINUTE_OFFSET_07_ON_THE_DAY     = 0x0146,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_08_ON_THE_DAY     = 0x0147,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_09_ON_THE_DAY     = 0x0148,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_10_ON_THE_DAY     = 0x0149,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_11_ON_THE_DAY     = 0x014A,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_12_ON_THE_DAY     = 0x014B,

    CONFIG_TRANSMISSION_MINUTE_OFFSET_13_ON_THE_DAY     = 0x014C,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_14_ON_THE_DAY     = 0x014D,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_15_ON_THE_DAY     = 0x014E,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_16_ON_THE_DAY     = 0x014F,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_17_ON_THE_DAY     = 0x0150,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_18_ON_THE_DAY     = 0x0151,

    CONFIG_TRANSMISSION_MINUTE_OFFSET_19_ON_THE_DAY     = 0x0152,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_20_ON_THE_DAY     = 0x0153,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_21_ON_THE_DAY     = 0x0154,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_22_ON_THE_DAY     = 0x0155,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_23_ON_THE_DAY     = 0x0156,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_24_ON_THE_DAY     = 0x0157,

    CONFIG_TRANSMISSION_MINUTE_OFFSET_25_ON_THE_DAY     = 0x0158,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_26_ON_THE_DAY     = 0x0159,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_27_ON_THE_DAY     = 0x015A,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_28_ON_THE_DAY     = 0x015B,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_29_ON_THE_DAY     = 0x015C,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_30_ON_THE_DAY     = 0x015D,

    CONFIG_TRANSMISSION_MINUTE_OFFSET_31_ON_THE_DAY     = 0x015E,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_32_ON_THE_DAY     = 0x015F,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_33_ON_THE_DAY     = 0x0160,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_34_ON_THE_DAY     = 0x0161,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_35_ON_THE_DAY     = 0x0162,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_36_ON_THE_DAY     = 0x0163,

    CONFIG_TRANSMISSION_MINUTE_OFFSET_37_ON_THE_DAY     = 0x0164,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_38_ON_THE_DAY     = 0x0165,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_39_ON_THE_DAY     = 0x0166,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_40_ON_THE_DAY     = 0x0167,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_41_ON_THE_DAY     = 0x0168,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_42_ON_THE_DAY     = 0x0169,

    CONFIG_TRANSMISSION_MINUTE_OFFSET_43_ON_THE_DAY     = 0x016A,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_44_ON_THE_DAY     = 0x016B,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_45_ON_THE_DAY     = 0x016C,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_46_ON_THE_DAY     = 0x016D,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_47_ON_THE_DAY     = 0x016E,
    CONFIG_TRANSMISSION_MINUTE_OFFSET_48_ON_THE_DAY     = 0x016F,    

    CONFIG_DOWNLOAD_SCRIPT_PERIOD_ENABLE                = 0x0170,    
    CONFIG_DOWNLOAD_SCRIPT_PERIOD_TIME                  = 0x0171,    
    CONFIG_DOWNLOAD_SCRIPT_PERIOD_UNIT_TYPE             = 0x0172,

    CONFIG_SETTING_UPDATE_TIME_FROM_SINGLE_TOWER_ENABLE = 0x0180,
    CONFIG_SETTING_UPDATE_TIME_CELL_FILTER_TYPE         = 0x0181,
//    CONFIG_SETTING_UPDATE_TIME_CELL_TOWER_MCC           = 0x0182,
//    CONFIG_SETTING_UPDATE_TIME_CELL_TOWER_MNC           = 0x0183,
    CONFIG_SETTING_UPDATE_TIME_CELL_TOWER_LAC           = 0x0184,
    CONFIG_SETTING_UPDATE_TIME_CELL_TOWER_CID           = 0x0185,

} ConfigSettingsPidEnum;

// COMMUNICATIONS
typedef enum
{
    // APN
    CONFIG_DI_MODEM_APN_ADDRESS_STRING                  = 0x0001,
    CONFIG_DI_MODEM_APN_USERNAME_STRING                 = 0x0002,
    CONFIG_DI_MODEM_APN_PASSWORD_STRING                 = 0x0003,

    // FTP
    CONFIG_DI_FTP_ADDRESS_STRING                        = 0x0011,
    CONFIG_DI_FTP_PORT_UINT16                           = 0x0012,
    CONFIG_DI_FTP_NAME_STRING                           = 0x0013,
    CONFIG_DI_FTP_PASSWORD_STRING                       = 0x0014,

    // SOCKET
    CONFIG_DI_SOCKET_PORT_UINT32                        = 0x0020,
    CONFIG_DI_SOCKET_ADDRESS_STRING                     = 0x0021,

    // TRANSMISSION PROTOCOL SELECT
    CONFIG_DI_PROTOCOL_UINT8                            = 0x0030,
    // TX GZIP FILE ENABLE
    CONFIG_DI_TX_GZIP_FILE_EN_UINT8                     = 0x0031,

    // TIME SERVER
    CONFIG_UPDATE_TIME_SOURCE_SRVR_PORT                 = 0x0040,
    CONFIG_UPDATE_TIME_SOURCE_SRVR_ADDRESS              = 0x0041,

    // pulse counter server
    CONFIG_PCOUNTER_DELTA_COUNT_SRVR_PORT               = 0x0042,
    CONFIG_PCOUNTER_DELTA_COUNT_SRVR_ADDRESS            = 0x0043

} ConfigDefaultCommunicationsEnum;
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

typedef enum
{
    CONFIG_PARAM_TYPE_UINT8 = 0,
    CONFIG_PARAM_TYPE_UINT16,
    CONFIG_PARAM_TYPE_UINT32,
    CONFIG_PARAM_TYPE_INT8,
    CONFIG_PARAM_TYPE_INT16,
    CONFIG_PARAM_TYPE_INT32,
    CONFIG_PARAM_TYPE_BOOL,
    CONFIG_PARAM_TYPE_STRING,
    CONFIG_PARAM_TYPE_CHAR,
    CONFIG_PARAM_TYPE_FLOAT,
} ConfigParameterTypeEnum;

typedef enum
{
    CONFIG_TYPE_DEVICE_INFO         = 0,
    CONFIG_TYPE_SETTINGS,

    CONFIG_TYPE_MAX,
} ConfigTypeEnum;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Function Prototypes
//////////////////////////////////////////////////////////////////////////////////////////////////

BOOL    ConfigInitialize                    ( void );
BOOL    ConfigLoad                          ( void );

BOOL    ConfigParametersSaveByConfigType    ( const ConfigTypeEnum eConfigType );
BOOL    ConfigParametersSaveAll             ( void );

BOOL    ConfigParametersLoadByConfigType    ( const ConfigTypeEnum eConfigType );
BOOL    ConfigParametersLoadAll             ( void );

BOOL    ConfigParametersRevertByConfigType  ( const ConfigTypeEnum eConfigType );
BOOL    ConfigParametersRevertAll           ( void );

BOOL    ConfigRevertValueByConfigType       ( const ConfigTypeEnum eConfigType, const UINT16 wParamId );
BOOL    ConfigGetValueByConfigType          ( const ConfigTypeEnum eConfigType, const UINT16 wParamId, void *pvParamData, UINT32 dwParamDataSize );
BOOL    ConfigSetValueByConfigType          ( const ConfigTypeEnum eConfigType, const UINT16 wParamId, const void *pvParamData );
BOOL    ConfigGetIdFromNameByConfigType     ( const ConfigTypeEnum eConfigType, const CHAR *pcParamName, UINT16 *pwParamId );

BOOL    ConfigShowParamFromParamId          ( const ConsolePortEnum eConsolePort, BOOL fPrintHeader, const ConfigTypeEnum eConfigType, UINT16 wParamId );
BOOL    ConfigShowListByConfigType          ( const ConsolePortEnum eConsolePort, const ConfigTypeEnum eConfigType );
BOOL    ConfigShowListAll                   ( const ConsolePortEnum eConsolePort );

UINT8   ConfigGetUpdateCounterByConfigType  ( const ConfigTypeEnum eConfigType );
//BOOL    ConfigGetConfigCopyByConfigType  ( const ConfigTypeEnum eConfigType, void * pvConfigStruct, UINT32 dwConfigStructSize );
BOOL    ConfigGetPtrConfigCopyByConfigType  ( const ConfigTypeEnum eConfigType, void ** pvConfigStruct );

BOOL    ConfigGetParamType                  ( const ConfigTypeEnum eConfigType, const UINT16 wParamId, ConfigParameterTypeEnum *peParamType );
CHAR *  ConfigGetConfigTypeName             ( const ConfigTypeEnum eConfigType );
BOOL    ConfigGetNumberOfParamsByConfigType ( const ConfigTypeEnum eConfigType, UINT16 *pwMaxNumberOfParams );
BOOL    ConfigGetCurrentParamConfigString   ( const ConfigTypeEnum eConfigType, UINT32 dwParamNum, CHAR *pcStringBuffer, UINT16 wStringBufferSize, UINT16 *pwNumOfBytesWrittenInBuffer );

CHAR *  ConfigGetParamTypeName              ( ConfigParameterTypeEnum eParamType );

#endif // _CONFIG_H_
