/** C Header ******************************************************************

*******************************************************************************/

#include <stdio.h>

#include "types.h"

#include "TriggerOnRemZero.h"

////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
    TriggerOnRemZeroTimeTypeEnum       ePeriodType;
    UINT16                             wPeriodValue;
}TriggerOnRemZeroConfigurationStruct;

typedef struct
{
    const CHAR *          const       	pcName;
    BOOL                                fIsTriggerEnabled;
    BOOL                                fIsTriggered;
    
    UINT32                              dwPreviousEpochSecondsChecked;
    UINT32                              dwPreviousEpochPeriodTypeSelected;
    
    TriggerOnRemZeroConfigurationStruct tPeriodModeConfig;
}TriggerOnRemZeroActionStatStruct;

////////////////////////////////////////////////////////////////////////////////////////////////////

static const CHAR * const gtOnRemZeroTimeTypeNameArray[TRIGGER_ON_REM_ZERO_TIME_TYPE_MAX] = 
{
    "SECOND",
    "MINUTE",
    "HOUR",
    "DAY"
};

static TriggerOnRemZeroActionStatStruct gtOnRemZeroActionStat[TRIGGER_ON_REM_ZERO_ACTION_MAX] =  
{
    ///////////////////////////////////////////////////////////////
    // test 1
    ///////////////////////////////////////////////////////////////
    //  pcName                  fIsTriggerEnabled  fIsTriggered
    {   "TRGG_RMZ_TEST_1",      FALSE,             FALSE,
    //  dwPreviousEpochSecondsChecked   dwPreviousEpochPeriodTypeSelected
        0,                              0,
        // tPeriodConfiguration
        {
            //  ePeriodType                         wPeriodValue
            TRIGGER_ON_REM_ZERO_TIME_TYPE_MINUTE, 	5
        }
    },

    ///////////////////////////////////////////////////////////////
    // Add more here below
    ///////////////////////////////////////////////////////////////
};

//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TriggerOnRemZeroInit( void )
{
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TriggerOnRemZeroEnableActionTrigger( TriggerOnRemZeroActionEnum eAction, BOOL fEnable )
{
    BOOL fSuccess = FALSE;
    
    if( eAction < TRIGGER_ON_REM_ZERO_ACTION_MAX )
    {
        // reset epoch units
        gtOnRemZeroActionStat[eAction].dwPreviousEpochSecondsChecked       = 0;
        gtOnRemZeroActionStat[eAction].dwPreviousEpochPeriodTypeSelected   = 0;
    
        gtOnRemZeroActionStat[eAction].fIsTriggerEnabled = fEnable;
        
        fSuccess = TRUE;
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TriggerOnRemZeroIsTriggerEnabled( TriggerOnRemZeroActionEnum eAction )
{
    BOOL fSuccess = FALSE;
    
    if( eAction < TRIGGER_ON_REM_ZERO_ACTION_MAX )
    {   
        fSuccess = gtOnRemZeroActionStat[eAction].fIsTriggerEnabled;
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TriggerOnRemZeroIsActionTriggered( TriggerOnRemZeroActionEnum eAction )
{
    if( eAction < TRIGGER_ON_REM_ZERO_ACTION_MAX )
    {
        return gtOnRemZeroActionStat[eAction].fIsTriggered;
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
BOOL TriggerOnRemZeroClearActionTriggeredFlag( TriggerOnRemZeroActionEnum eAction )
{
    BOOL fSuccess = FALSE;
    
    if( eAction < TRIGGER_ON_REM_ZERO_ACTION_MAX )
    {
        gtOnRemZeroActionStat[eAction].fIsTriggered = FALSE;
        
        fSuccess = TRUE;
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TriggerOnRemZeroSetTimeType( TriggerOnRemZeroActionEnum eAction, TriggerOnRemZeroTimeTypeEnum ePeriodType )
{
    BOOL fSuccess = FALSE;
    
    if( eAction < TRIGGER_ON_REM_ZERO_ACTION_MAX )
    {        
        if( ePeriodType < TRIGGER_ON_REM_ZERO_TIME_TYPE_MAX )
        {
            gtOnRemZeroActionStat[eAction].tPeriodModeConfig.ePeriodType = ePeriodType;
        
            fSuccess = TRUE;
        }
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TriggerOnRemZeroGetTimeType( TriggerOnRemZeroActionEnum eAction, TriggerOnRemZeroTimeTypeEnum *pePeriodType )
{
    BOOL fSuccess = FALSE;
    
    if( eAction < TRIGGER_ON_REM_ZERO_ACTION_MAX )
    {
        if( pePeriodType != NULL )
        {
            (*pePeriodType) = gtOnRemZeroActionStat[eAction].tPeriodModeConfig.ePeriodType;
        
            fSuccess = TRUE;
        }
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TriggerOnRemZeroSetTimeValue( TriggerOnRemZeroActionEnum eAction, UINT16 wValueDependingOnPeriodType )
{
    BOOL fSuccess = FALSE;
    
    if( eAction < TRIGGER_ON_REM_ZERO_ACTION_MAX )
    {
        gtOnRemZeroActionStat[eAction].tPeriodModeConfig.wPeriodValue = wValueDependingOnPeriodType;
        
        fSuccess = TRUE;
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
INT32 TriggerOnRemZeroGetTimeValue( TriggerOnRemZeroActionEnum eAction )
{
    INT32 idwPeriodValue = -1;
    
    if( eAction < TRIGGER_ON_REM_ZERO_ACTION_MAX )
    {
        idwPeriodValue = (INT32)gtOnRemZeroActionStat[eAction].tPeriodModeConfig.wPeriodValue;
    }
    
    return idwPeriodValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void TriggerOnRemZeroUpdate( TriggerOnRemZeroActionEnum eAction, UINT32 dwCurrentEpochSeconds )
{  
    UINT32 dwTimeTypeDivide  = 0;
    UINT32 dwEpochDivideRes  = 0;
    
    if( eAction < TRIGGER_ON_REM_ZERO_ACTION_MAX )
    {
        if( gtOnRemZeroActionStat[eAction].fIsTriggerEnabled )
        {
            if( gtOnRemZeroActionStat[eAction].fIsTriggered == FALSE )
            {
                // is time to check?
                if( gtOnRemZeroActionStat[eAction].dwPreviousEpochSecondsChecked != dwCurrentEpochSeconds )
                {
                    // update previous time checked
                    gtOnRemZeroActionStat[eAction].dwPreviousEpochSecondsChecked = dwCurrentEpochSeconds;
                    
                    // do a check depending on period type selected
                    switch( gtOnRemZeroActionStat[eAction].tPeriodModeConfig.ePeriodType )
                    {
                        case TRIGGER_ON_REM_ZERO_TIME_TYPE_SECOND:
                        {                         
                            // this value should never be zero
                            dwTimeTypeDivide = 1;
                            dwEpochDivideRes = dwCurrentEpochSeconds / dwTimeTypeDivide;   
                            break;
                        }
                
                        case TRIGGER_ON_REM_ZERO_TIME_TYPE_MINUTE:
                        {                        
                            // this value should never be zero
                            dwTimeTypeDivide = 60;
                            dwEpochDivideRes = dwCurrentEpochSeconds / dwTimeTypeDivide;
                            break;
                        }
                
                        case TRIGGER_ON_REM_ZERO_TIME_TYPE_HOUR:
                        {
                            // this value should never be zero
                            dwTimeTypeDivide = 3600;
                            dwEpochDivideRes = dwCurrentEpochSeconds / dwTimeTypeDivide;   
                            break;
                        }
                
                        case TRIGGER_ON_REM_ZERO_TIME_TYPE_DAY:
                        {                        
                            // this value should never be zero
                            dwTimeTypeDivide = 86400;
                            dwEpochDivideRes = dwCurrentEpochSeconds / dwTimeTypeDivide;   
                            break;
                        }
                
                        default:
                        {
                            break;
                        }
                    }// end switch
                    
                    /////////////////////////////////////////////////////////
                    /////////////////////////////////////////////////////////
                    // check if previous triggered time has changed
                    if( gtOnRemZeroActionStat[eAction].dwPreviousEpochPeriodTypeSelected != dwEpochDivideRes )
                    {
                        gtOnRemZeroActionStat[eAction].dwPreviousEpochPeriodTypeSelected = dwEpochDivideRes;
                        
                        // check if 
                        if( gtOnRemZeroActionStat[eAction].tPeriodModeConfig.wPeriodValue != 0 )
                        {
                            if( (gtOnRemZeroActionStat[eAction].dwPreviousEpochPeriodTypeSelected % gtOnRemZeroActionStat[eAction].tPeriodModeConfig.wPeriodValue) == 0 )
                            {
                                gtOnRemZeroActionStat[eAction].fIsTriggered = TRUE;
                            }
                        }
                    }
                    /////////////////////////////////////////////////////////
                    /////////////////////////////////////////////////////////
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
CHAR * TriggerOnRemZeroGetActionName( TriggerOnRemZeroActionEnum eAction )
{
    if( eAction < TRIGGER_ON_REM_ZERO_ACTION_MAX )
    {
        return &gtOnRemZeroActionStat[eAction].pcName[0];
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
CHAR * TriggerOnRemZeroGetTimeTypeName( TriggerOnRemZeroTimeTypeEnum eChangeType )
{
    if( eChangeType < TRIGGER_ON_REM_ZERO_TIME_TYPE_MAX )
    {
        return &gtOnRemZeroTimeTypeNameArray[eChangeType][0];
    }
    else
    {
        return NULL;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/* EOF */