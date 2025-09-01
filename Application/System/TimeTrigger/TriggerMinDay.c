/** C Header ******************************************************************

*******************************************************************************/

#include <stdio.h>

#include "Target.h"
#include "Types.h"
#include "Gpio.h"
#include "InterfaceBoard.h"
#include "..\Utils\StateMachine.h"
#include "Power.h"
#include "Timer.h"
#include "Adc.h"
#include "Control.h"
#include "TriggerMinDay.h"

////////////////////////////////////////////////////////////////////////////////////////////////////


typedef struct
{
    const CHAR * const          pcName;
    BOOL                        fIsTriggerEnabled;
    BOOL                        fIsTriggered;
    

    TriggerMinDayScheduleEnum   eScheduleSelected;
    
    UINT16                      wTransmitDataMinuteWhenLastCheck;
}TriggerMinDayStatStruct;

typedef struct
{
    INT16              *piwMinuteOffsetArrayPointer;
    BOOL                fIsScheduleConfigurable;
    UINT8               bMinutesAmount;
}TriggerMinDayScheduleStruct;

////////////////////////////////////////////////////////////////////////////////////////////////////
// ARRAYS OF SCHEDULES
////////////////////////////////////////////////////////////////////////////////////////////////////

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Transmission
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Default
static INT16 giwTransmissionsMinuteOffsetOnTheDay_Default[ 1 ] = 
{
    720
};
// Low Power
static INT16 giwTransmissionsMinuteOffsetOnTheDay_LowPower[ 5 ] = 
{
/*  hr6     hr9     hr12    hr15    hr18  */
    300,    480,    660,    840,    1020
};
// Configurable
static INT16 giwTransmissionsMinuteOffsetOnTheDay_Config1[ 48 ] = 
{
/*  1       2       3       4       5       6       7       8       9       10      11      12     */
/*  INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID*/
    -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     
/*  13      14      15      16      17      18      19      20      21      22     23       24      */
/*  INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID*/
    -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
/*  25      26      27      28      29      30      31      32      33      34     35       36      */
/*  INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID*/
    -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,
/*  37      38      39      40      41      42      43      44      45      46     47       48      */
/*  INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID*/
    -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Download script
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Default
static INT16 giwDownloadScriptMinuteOffsetOnTheDay_Default[ 1 ] = 
{
    720
};
// Low Power
static INT16 giwDownloadScriptMinuteOffsetOnTheDay_LowPower[ 12 ] =
{
/*  1       2       3       4       5       6       7       8       9       10      11      12     */
/*  INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID*/
    720,   -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     
};

// Configurable
static INT16 giwDownloadScriptMinuteOffsetOnTheDay_Config1[ 12 ] =
{
/*  1       2       3       4       5       6       7       8       9       10      11      12     */
/*  INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID*/
    420,    600,    780,    960,    -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Capture Image
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Default
static INT16 giwCaptureImageMinuteOffsetOnTheDay_Default[ 1 ] = 
{
    -1
};
// Low Power
static INT16 giwCaptureImageMinuteOffsetOnTheDay_LowPower[ 1 ] = 
{
/*  12hrs   */
    660
};
// Configurable
static INT16 giwCaptureImageMinuteOffsetOnTheDay_Config1[ 24 ] = 
{
/*  1       2       3       4       5       6       7       8       9       10      11      12     */
/*  INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID*/
    -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     
/*  13      14      15      16      17      18      19      20      21      22     23       24      */
/*  INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID*/
    -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Pulse Count
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Default
static INT16 giwPulseCountMinuteOffsetOnTheDay_Default[ 1 ] = 
{
    -1
};
// Low Power
static INT16 giwPulseCountMinuteOffsetOnTheDay_LowPower[ 12 ] = 
{
/*  1       2       3       4       5       6       7       8       9       10      11      12     */
/*  hr1     hr3     hr5     hr7     hr9     hr11    hr13    hr15    hr17    hr19    hr21    hr23   */
    0,      120,    240,    360,    480,    600,    720,    840,    960,    1080,   1200,   1320,     
};
// Configurable
static INT16 giwPulseCountMinuteOffsetOnTheDay_Config1[ 24 ] = 
{
/*  1       2       3       4       5       6       7       8       9       10      11      12     */
/*  INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID*/
    -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     
/*  13      14      15      16      17      18      19      20      21      22     23       24      */
/*  INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID*/
    -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Sensor Long Sampling
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Default
static INT16 giwSensorLongSampleStartMinuteOffsetOnTheDay_Default[ 1 ] = 
{
    -1
};
// Low Power
static INT16 giwSensorLongSampleStartMinuteOffsetOnTheDay_LowPower[ 1 ] = 
{
    -1
};
// Configurable
static INT16 giwSensorLongSampleStartMinuteOffsetOnTheDay_Config1[ 1 ] = 
{
    -1
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Sync sys time
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Default
static INT16 giwSyncSysTimeMinuteOffsetOnTheDay_Default[ 1 ] = 
{
    720
};
// Low Power
static INT16 giwSyncSysTimeMinuteOffsetOnTheDay_LowPower[ 1 ] = 
{
    720
};
// Configurable
static INT16 giwSyncSysTimeMinuteOffsetOnTheDay_Config1[ 12 ] = 
{
    /*  1       2       3       4       5       6       7       8       9       10      11      12     */
    /*  INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID*/
    420,    600,    780,    960,    -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Fluid Depth
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Default
static INT16 giwFluidDepthMinuteOffsetOnTheDay_Default[ 1 ] = 
{
    720
};
// Low Power
static INT16 giwFluidDepthMinuteOffsetOnTheDay_LowPower[ 12 ] =
{
/*  1       2       3       4       5       6       7       8       9       10      11      12     */
/*  INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID*/
    720,   -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     
};

// Configurable
static INT16 giwFluidDepthMinuteOffsetOnTheDay_Config1[ 12 ] =
{
/*  1       2       3       4       5       6       7       8       9       10      11      12     */
/*  INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID INVALID*/
    420,    600,    780,    960,    -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1,     
};

////////////////////////////////////////////////////////////////////////////////////////////////////

static const TriggerMinDayScheduleStruct gtTransmissionSchedulesArray[TRIGGER_MIN_DAY_SCHEDULE_MAX] = 
{
    //  piwMinuteOffsetArrayPointer                         fIsScheduleConfigurable         bMinutesAmount
    // DEFAULT
    {  &giwTransmissionsMinuteOffsetOnTheDay_Default[0],    FALSE                           ,( sizeof(giwTransmissionsMinuteOffsetOnTheDay_Default) / sizeof(giwTransmissionsMinuteOffsetOnTheDay_Default[0]) ) },
    // LOW_POWER
    {  &giwTransmissionsMinuteOffsetOnTheDay_LowPower[0],   TRUE                            ,( sizeof(giwTransmissionsMinuteOffsetOnTheDay_LowPower) / sizeof(giwTransmissionsMinuteOffsetOnTheDay_LowPower[0]) ) },
    // CONFIGURABLE_1
    {  &giwTransmissionsMinuteOffsetOnTheDay_Config1[0],    TRUE                            ,( sizeof(giwTransmissionsMinuteOffsetOnTheDay_Config1) / sizeof(giwTransmissionsMinuteOffsetOnTheDay_Config1[0]) ) },
};

static const TriggerMinDayScheduleStruct gtDownloadScriptSchedulesArray[TRIGGER_MIN_DAY_SCHEDULE_MAX] = 
{
    //  piwMinuteOffsetArrayPointer                         fIsScheduleConfigurable         bMinutesAmount
    // DEFAULT
    {  &giwDownloadScriptMinuteOffsetOnTheDay_Default[0],   FALSE                           ,( sizeof(giwDownloadScriptMinuteOffsetOnTheDay_Default) / sizeof(giwDownloadScriptMinuteOffsetOnTheDay_Default[0]) ) },
    // LOW_POWER
    {  &giwDownloadScriptMinuteOffsetOnTheDay_LowPower[0],  TRUE                            ,( sizeof(giwDownloadScriptMinuteOffsetOnTheDay_LowPower) / sizeof(giwDownloadScriptMinuteOffsetOnTheDay_LowPower[0]) ) },
    // CONFIGURABLE_1
    {  &giwDownloadScriptMinuteOffsetOnTheDay_Config1[0],   TRUE                            ,( sizeof(giwDownloadScriptMinuteOffsetOnTheDay_Config1) / sizeof(giwDownloadScriptMinuteOffsetOnTheDay_Config1[0]) ) },
};

static const TriggerMinDayScheduleStruct gtCaptureImageSchedulesArray[TRIGGER_MIN_DAY_SCHEDULE_MAX] = 
{
    //  piwMinuteOffsetArrayPointer                         fIsScheduleConfigurable         bMinutesAmount
    // DEFAULT
    {  &giwCaptureImageMinuteOffsetOnTheDay_Default[0],     FALSE                           ,( sizeof(giwCaptureImageMinuteOffsetOnTheDay_Default) / sizeof(giwCaptureImageMinuteOffsetOnTheDay_Default[0]) ) },
    // LOW_POWER
    {  &giwCaptureImageMinuteOffsetOnTheDay_LowPower[0],    TRUE                            ,( sizeof(giwCaptureImageMinuteOffsetOnTheDay_LowPower) / sizeof(giwCaptureImageMinuteOffsetOnTheDay_LowPower[0]) ) },
    // CONFIGURABLE_1
    {  &giwCaptureImageMinuteOffsetOnTheDay_Config1[0],     TRUE                            ,( sizeof(giwCaptureImageMinuteOffsetOnTheDay_Config1) / sizeof(giwCaptureImageMinuteOffsetOnTheDay_Config1[0]) ) },
};

static const TriggerMinDayScheduleStruct gtPulseCountSchedulesArray[TRIGGER_MIN_DAY_SCHEDULE_MAX] = 
{
    //  piwMinuteOffsetArrayPointer                         fIsScheduleConfigurable         bMinutesAmount
    // DEFAULT
    {  &giwPulseCountMinuteOffsetOnTheDay_Default[0],       FALSE                           ,( sizeof(giwPulseCountMinuteOffsetOnTheDay_Default) / sizeof(giwPulseCountMinuteOffsetOnTheDay_Default[0]) ) },
    // LOW_POWER
    {  &giwPulseCountMinuteOffsetOnTheDay_LowPower[0],      TRUE                            ,( sizeof(giwPulseCountMinuteOffsetOnTheDay_LowPower) / sizeof(giwPulseCountMinuteOffsetOnTheDay_LowPower[0]) ) },
    // CONFIGURABLE_1
    {  &giwPulseCountMinuteOffsetOnTheDay_Config1[0],       TRUE                            ,( sizeof(giwPulseCountMinuteOffsetOnTheDay_Config1) / sizeof(giwPulseCountMinuteOffsetOnTheDay_Config1[0]) ) },
};

static const TriggerMinDayScheduleStruct gtSensorLongSampleStartSchedulesArray[TRIGGER_MIN_DAY_SCHEDULE_MAX] = 
{
    //  piwMinuteOffsetArrayPointer                             fIsScheduleConfigurable         bMinutesAmount
    // DEFAULT
    {  &giwSensorLongSampleStartMinuteOffsetOnTheDay_Default[0],FALSE                           ,( sizeof(giwSensorLongSampleStartMinuteOffsetOnTheDay_Default) / sizeof(giwSensorLongSampleStartMinuteOffsetOnTheDay_Default[0]) ) },
    // LOW_POWER
    {  &giwSensorLongSampleStartMinuteOffsetOnTheDay_LowPower[0],TRUE                           ,( sizeof(giwSensorLongSampleStartMinuteOffsetOnTheDay_LowPower) / sizeof(giwSensorLongSampleStartMinuteOffsetOnTheDay_LowPower[0]) ) },
    // CONFIGURABLE_1
    {  &giwSensorLongSampleStartMinuteOffsetOnTheDay_Config1[0],TRUE                            ,( sizeof(giwSensorLongSampleStartMinuteOffsetOnTheDay_Config1) / sizeof(giwSensorLongSampleStartMinuteOffsetOnTheDay_Config1[0]) ) },
};

static const TriggerMinDayScheduleStruct gtSyncSysTimeSchedulesArray[TRIGGER_MIN_DAY_SCHEDULE_MAX] = 
{
    //  piwMinuteOffsetArrayPointer                             fIsScheduleConfigurable         bMinutesAmount
    // DEFAULT
    {  &giwSyncSysTimeMinuteOffsetOnTheDay_Default[0],          FALSE,                          ( sizeof(giwSyncSysTimeMinuteOffsetOnTheDay_Default) / sizeof(giwSyncSysTimeMinuteOffsetOnTheDay_Default[0]) ) },
    // LOW_POWER
    {  &giwSyncSysTimeMinuteOffsetOnTheDay_LowPower[0],         TRUE,                           ( sizeof(giwSyncSysTimeMinuteOffsetOnTheDay_LowPower) / sizeof(giwSyncSysTimeMinuteOffsetOnTheDay_LowPower[0]) ) },
    // CONFIGURABLE_1
    {  &giwSyncSysTimeMinuteOffsetOnTheDay_Config1[0],          TRUE,                           ( sizeof(giwSyncSysTimeMinuteOffsetOnTheDay_Config1) / sizeof(giwSyncSysTimeMinuteOffsetOnTheDay_Config1[0]) ) },
};

static TriggerMinDayScheduleStruct gtFluidDepthSchedulesArray[TRIGGER_MIN_DAY_SCHEDULE_MAX] = 
{
    //  piwMinuteOffsetArrayPointer                             fIsScheduleConfigurable         bMinutesAmount
    // DEFAULT
    {  &giwFluidDepthMinuteOffsetOnTheDay_Default[0],          FALSE,                          ( sizeof(giwFluidDepthMinuteOffsetOnTheDay_Default) / sizeof(giwFluidDepthMinuteOffsetOnTheDay_Default[0]) ) },
    // LOW_POWER
    {  &giwFluidDepthMinuteOffsetOnTheDay_LowPower[0],         TRUE,                           ( sizeof(giwFluidDepthMinuteOffsetOnTheDay_LowPower) / sizeof(giwFluidDepthMinuteOffsetOnTheDay_LowPower[0]) ) },
    // CONFIGURABLE_1
    {  &giwFluidDepthMinuteOffsetOnTheDay_Config1[0],          TRUE,                           ( sizeof(giwFluidDepthMinuteOffsetOnTheDay_Config1) / sizeof(giwFluidDepthMinuteOffsetOnTheDay_Config1[0]) ) },
};

//////////////////////////////////////////////////////////////////////////////////////////////////

static TriggerMinDayStatStruct gtTriggerMinDayStat[TRIGGER_MIN_DAY_ACTION_MAX] = 
{
    //  pcName,             others
    {   "UPLOAD_DATA",      0,0,0,0  },
    {   "DOWNLOAD_SCRIPT",  0,0,0,0  },
    {   "CAPTURE_IMG",      0,0,0,0  },
    {   "P_COUNT",          0,0,0,0  },
    {   "LON_S_SAMPLE",     0,0,0,0  },
    {   "SYNC_SYS_TIME",    0,0,0,0  },
    {   "FLUID_DEPTH",      0,0,0,0  },
};

static const TriggerMinDayScheduleStruct * const  gtArrayOfActionSchedules[TRIGGER_MIN_DAY_ACTION_MAX] =
{
    &gtTransmissionSchedulesArray[0],
    &gtDownloadScriptSchedulesArray[0],
    &gtCaptureImageSchedulesArray[0],
    &gtPulseCountSchedulesArray[0],
    &gtSensorLongSampleStartSchedulesArray[0],
    &gtSyncSysTimeSchedulesArray[0],
    &gtFluidDepthSchedulesArray[0],
};

static const CHAR * const gpcScheduleNameArray[TRIGGER_MIN_DAY_SCHEDULE_MAX] =
{
    "DEFAULT",
    "LOW_POWER",
    "CONFIGURABLE"
};

//////////////////////////////////////////////////////////////////////////////////////////////////

static BOOL TriggerMinDayIsScheduleValid( TriggerMinDayActionEnum eAction, TriggerMinDayScheduleEnum eSchedule );

//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TriggerMinDayInit( void )
{
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TriggerMinDayEnableActionTrigger( TriggerMinDayActionEnum eAction, BOOL fEnable )
{
    BOOL fSuccess = FALSE;
    
    if( eAction < TRIGGER_MIN_DAY_ACTION_MAX )
    {
        gtTriggerMinDayStat[eAction].fIsTriggerEnabled = fEnable;
        
        fSuccess = TRUE;
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TriggerMinDayIsTriggerEnabled( TriggerMinDayActionEnum eAction )
{
    BOOL fSuccess = FALSE;
    
    if( eAction < TRIGGER_MIN_DAY_ACTION_MAX )
    {   
        fSuccess = gtTriggerMinDayStat[eAction].fIsTriggerEnabled;
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TriggerMinDayIsActionTriggered( TriggerMinDayActionEnum eAction )
{
    if( eAction < TRIGGER_MIN_DAY_ACTION_MAX )
    {
        return gtTriggerMinDayStat[eAction].fIsTriggered;
    }
    else
    {
        return FALSE;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TriggerMinDayClearActionTriggeredFlag( TriggerMinDayActionEnum eAction )
{
    BOOL fSuccess = FALSE;
    
    if( eAction < TRIGGER_MIN_DAY_ACTION_MAX )
    {
        gtTriggerMinDayStat[eAction].fIsTriggered = FALSE;
        
        fSuccess = TRUE;
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TriggerMinDaySetCurrentSchedule( TriggerMinDayActionEnum eAction, TriggerMinDayScheduleEnum eSchedule )
{
    BOOL fSuccess = FALSE;
    
    if( eAction < TRIGGER_MIN_DAY_ACTION_MAX )
    {
        if( eSchedule < TRIGGER_MIN_DAY_SCHEDULE_MAX )
        {
            gtTriggerMinDayStat[eAction].eScheduleSelected = eSchedule;
        
            fSuccess = TRUE;
        }
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TriggerMinDayGetCurrentSchedule( TriggerMinDayActionEnum eAction, TriggerMinDayScheduleEnum *peSchedule )
{
    BOOL fSuccess = FALSE;
    
    if( eAction < TRIGGER_MIN_DAY_ACTION_MAX )
    {
        if( peSchedule != NULL )
        {            
            (*peSchedule) = gtTriggerMinDayStat[eAction].eScheduleSelected;
        
            fSuccess = TRUE;
        }
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TriggerMinDayIsScheduleConfigurable( TriggerMinDayActionEnum eAction, TriggerMinDayScheduleEnum eSchedule )
{
    BOOL fIsConfigurable = FALSE;
        
    if( TriggerMinDayIsScheduleValid( eAction, eSchedule ) )
    {
        fIsConfigurable = gtArrayOfActionSchedules[eAction][eSchedule].fIsScheduleConfigurable;
    }
        
    return fIsConfigurable;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TriggerMinDayIsScheduleValid( TriggerMinDayActionEnum eAction, TriggerMinDayScheduleEnum eSchedule )
{
    BOOL fIsValid = FALSE;
    
    if( eAction < TRIGGER_MIN_DAY_ACTION_MAX )
    {
        if( eSchedule < TRIGGER_MIN_DAY_SCHEDULE_MAX )
        {            
            if( gtArrayOfActionSchedules[eAction] != NULL )
            {
                if( gtArrayOfActionSchedules[eAction][eSchedule].piwMinuteOffsetArrayPointer != NULL )
                {
                    fIsValid = TRUE;
                }
            }
        }
    }
    
    return fIsValid;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
INT16 TriggerMinDayGetScheduleMinutesAmount( TriggerMinDayActionEnum eAction, TriggerMinDayScheduleEnum eSchedule )
{
    INT16 iwMinutesAmount = 0;
    
    if( TriggerMinDayIsScheduleValid( eAction, eSchedule ) )
    {
        iwMinutesAmount = (INT16)gtArrayOfActionSchedules[eAction][eSchedule].bMinutesAmount;
    }
    
    return iwMinutesAmount;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
INT16 TriggerMinDayGetScheduleMinuteAtIndex( TriggerMinDayActionEnum eAction, TriggerMinDayScheduleEnum eSchedule, INT16 iwIndex )
{
    INT16 iwMinuteValue     = -1;
    INT16 iwMinutesAmount   = 0;
    
    if( TriggerMinDayIsScheduleValid( eAction, eSchedule ) )
    {
        iwMinutesAmount = TriggerMinDayGetScheduleMinutesAmount( eAction, eSchedule );
        
        if
        (
            ( iwMinutesAmount > 0 ) &&
            ( iwIndex >= 0 ) && 
            ( iwIndex < iwMinutesAmount )
        )
        {
            iwMinuteValue = gtArrayOfActionSchedules[eAction][eSchedule].piwMinuteOffsetArrayPointer[iwIndex];
        }
    }
    
    return iwMinuteValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL  TriggerMinDaySetScheduleMinuteAtIndex( TriggerMinDayActionEnum eAction, TriggerMinDayScheduleEnum eSchedule, INT16 iwIndex, INT16 iwMinuteOfssetOfDay )
{
    BOOL  fSuccess          = FALSE;
    INT16 iwMinutesAmount   = 0;
    
    if( TriggerMinDayIsScheduleValid( eAction, eSchedule ) )
    {
        iwMinutesAmount = TriggerMinDayGetScheduleMinutesAmount( eAction, eSchedule );
        
        if
        (
            ( iwMinutesAmount > 0 ) &&
            ( iwIndex >= 0 ) && 
            ( iwIndex < iwMinutesAmount )
        )
        {
            gtArrayOfActionSchedules[eAction][eSchedule].piwMinuteOffsetArrayPointer[iwIndex] = iwMinuteOfssetOfDay;
            
            fSuccess = TRUE;
        }
    }
    
    return fSuccess;
}

// operations
//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL  TriggerMinDayConvertMinOfDayToDayTime( INT16 iwMinOffsetOfDay, TriggerMinDayTimeStruct *ptTime )
{   
    BOOL    fSuccess    = FALSE;
    UINT8   bHour;
    UINT8   bMinute;
    SINGLE  sgDecPart;
    SINGLE  sgWholeNumber;
    SINGLE  sgRounding;
    
    if( ptTime != NULL )
    {
        if
        (
            ( iwMinOffsetOfDay >= TRIGGER_MIN_DAY_VALID_MINUTE_OFFSET_MIN ) &&
            ( iwMinOffsetOfDay <= TRIGGER_MIN_DAY_VALID_MINUTE_OFFSET_MAX )
        )
        {
            sgWholeNumber = ( iwMinOffsetOfDay / 60.0 );
            
            bHour       = (UINT16)sgWholeNumber;
            sgDecPart   = sgWholeNumber - ((SINGLE)bHour);
            sgRounding  = (sgDecPart * 60.0);
            // #warning this should be replaced with round() funciton
            /////////////////////////////////////
            UINT8 bDecPart2 = (UINT8)sgRounding;
            sgRounding = sgRounding - ((SINGLE)bDecPart2);
            if( sgRounding >= .5 )
            {
                sgRounding = (sgDecPart * 60.0) + 0.5;
            }
            else
            {
                sgRounding = (sgDecPart * 60.0);
            }
            /////////////////////////////////////
            bMinute     =  (UINT8)sgRounding;
            
            ptTime->bHour    = bHour;
            ptTime->bMinute  = bMinute;
            
            fSuccess = TRUE;
        }
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
INT16 TriggerMinDayConvertDayTimeToMinOfDay( TriggerMinDayTimeStruct *ptTime )
{   
    SINGLE  sgDecPart;
    SINGLE  sgWholeNumber;
    INT16   iwMinOffsetOfDay = -1;
    
    if( ptTime != NULL )
    {        
        // validate hour and minute
        if
        ( 
            //( ptTime->bMinute >= 0 ) &&
            ( ptTime->bMinute <= 59 ) &&
            
            //( ptTime->bHour >= 0 ) &&
            ( ptTime->bHour <= 23 ) 
        )
        {
            sgDecPart = ((SINGLE)ptTime->bMinute) / 60.0;
            sgWholeNumber = ptTime->bHour + sgDecPart;
            iwMinOffsetOfDay = sgWholeNumber * 60.0;
        }
    }
    
    return iwMinOffsetOfDay;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
INT16 TriggerMinDayAddOffsetToMinOfDay( INT16 iwMinOffsetOfDay, UINT16 wOffset, BOOL fIsPossitiveOffset )
{
    INT16   iwNewMinOffsetOfDay         = -1;
    INT16   iwMaxAmountOfMinutesPerDay  = 1440;
    INT16   iwDeltaNegativeOffset;
    INT16   iwTemp;

    if
    (
        ( iwMinOffsetOfDay >= TRIGGER_MIN_DAY_VALID_MINUTE_OFFSET_MIN ) &&
        ( iwMinOffsetOfDay <= TRIGGER_MIN_DAY_VALID_MINUTE_OFFSET_MAX )
    )
    {
        if( fIsPossitiveOffset )
        {
            iwTemp = iwMinOffsetOfDay + wOffset;

            iwNewMinOffsetOfDay = iwTemp % iwMaxAmountOfMinutesPerDay;
        }
        else // negative
        {
            iwDeltaNegativeOffset = iwMinOffsetOfDay - wOffset;

            if( iwDeltaNegativeOffset < 0 )
            {
                iwTemp = ( iwMinOffsetOfDay + iwMaxAmountOfMinutesPerDay ) - wOffset;
            }
            else
            {
                iwTemp = iwMinOffsetOfDay - wOffset;
            }

            iwNewMinOffsetOfDay = iwTemp % iwMaxAmountOfMinutesPerDay;
        }
    }

    return iwNewMinOffsetOfDay;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void TriggerMinDayUpdate( TriggerMinDayActionEnum eAction, UINT16 wCurrentMinuteInTheDay )
{
    if
    (
        //( wCurrentMinuteInTheDay >= TRIGGER_MIN_DAY_VALID_MINUTE_OFFSET_MIN ) &&
        ( wCurrentMinuteInTheDay <= TRIGGER_MIN_DAY_VALID_MINUTE_OFFSET_MAX )
    )
    {
        TriggerMinDayScheduleEnum   eSchedule;
        INT16                       iwScheduleMinutesAmount;
    
        
        // is time to check?
        if( gtTriggerMinDayStat[eAction].wTransmitDataMinuteWhenLastCheck != wCurrentMinuteInTheDay )
        {
            // update last minute check 
            gtTriggerMinDayStat[eAction].wTransmitDataMinuteWhenLastCheck = wCurrentMinuteInTheDay;
            
            
            //  * * * * * * * * * * * * * * * * * * * * * * * * * *
            
            if( TriggerMinDayGetCurrentSchedule( eAction, &eSchedule ) )
            {                
                iwScheduleMinutesAmount = TriggerMinDayGetScheduleMinutesAmount( eAction, eSchedule );
                
                if( iwScheduleMinutesAmount > 0 ) 
                {
                    // if triggering is disabled
                    if( TriggerMinDayIsTriggerEnabled( eAction ) )
                    {
                        // if is already triggered, don't check again
                        if( TriggerMinDayIsActionTriggered( eAction ) == FALSE )
                        {
                            //check if is time to trigger action
                            for( UINT8 i = 0 ; i < (UINT8)iwScheduleMinutesAmount; i++ )
                            {
                                if( wCurrentMinuteInTheDay == gtArrayOfActionSchedules[eAction][eSchedule].piwMinuteOffsetArrayPointer[i] )
                                {
                                    gtTriggerMinDayStat[eAction].fIsTriggered = TRUE;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            
            //  * * * * * * * * * * * * * * * * * * * * * * * * * *
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
CHAR * TriggerMinDayGetActionName( TriggerMinDayActionEnum eAction )
{
    if( eAction < TRIGGER_MIN_DAY_ACTION_MAX )
    {
        return &gtTriggerMinDayStat[eAction].pcName[0];
    }
    else
    {
        return NULL;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
CHAR * TriggerMinDayGetScheduleName( TriggerMinDayScheduleEnum eSchedule )
{
    if( eSchedule < TRIGGER_MIN_DAY_SCHEDULE_MAX )
    {
        return &gpcScheduleNameArray[eSchedule][0];
    }
    else
    {
        return NULL;
    }
}

/* EOF */