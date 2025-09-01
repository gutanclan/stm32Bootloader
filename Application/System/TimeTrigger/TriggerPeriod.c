/** C Header ******************************************************************

*******************************************************************************/

#include <stdio.h>

#include "types.h"
//#include "SysTime.h"

#include "TriggerPeriod.h"

////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
    TriggerPeriodTypeEnum       ePeriodType;
    UINT16                      wPeriodValue;
}TriggerPeriodConfigStruct;

typedef struct
{
    const CHAR *   const                    pcName;
    BOOL                                    fIsTriggerEnabled;
    BOOL                                    fIsTriggered;
    
    UINT32                                  dwPreviousEpochSecondsChecked;
    UINT32                                  dwPreviousEpochPeriodTypeSelected;
    
    TriggerPeriodConfigStruct  				tPeriodConfig;
}TriggerPeriodActionStatStruct;

////////////////////////////////////////////////////////////////////////////////////////////////////

static const CHAR * const gtPeriodTypeNameArray[TRIGGER_PERIOD_TYPE_MAX] = 
{
    "SECONDS",
    "MINUTES",
    "HOURS",
    "DAYS"
};

static TriggerPeriodActionStatStruct gtPeriodActionStat[] =  
{
    ///////////////////////////////////////////////////////////////
    // test seconds
    ///////////////////////////////////////////////////////////////
    //  pcName                  fIsTriggerEnabled  fIsTriggered
    {   "TRGG_PER_TEST_SECS",  	FALSE,             FALSE,
    //  dwPreviousEpochSecondsChecked   dwPreviousEpochPeriodTypeSelected
        0,                              0,
        // tPeriodConfiguration        
		//  ePeriodType                     bPeriodValue
		{   TRIGGER_PERIOD_TYPE_SECONDS,    1},
    },

    ///////////////////////////////////////////////////////////////
    // test minutes
    ///////////////////////////////////////////////////////////////
    //  pcName                  fIsTriggerEnabled  fIsTriggered
    {   "TRGG_PER_TEST_MINS",  	FALSE,             FALSE,
    //  dwPreviousEpochSecondsChecked   dwPreviousEpochPeriodTypeSelected
        0,                              0,
        // tPeriodConfiguration        
		//  ePeriodType                     bPeriodValue
		{   TRIGGER_PERIOD_TYPE_MINUTES,    1},
    },

    ///////////////////////////////////////////////////////////////
    // test hours
    ///////////////////////////////////////////////////////////////
    //  pcName                  fIsTriggerEnabled  fIsTriggered
    {   "TRGG_PER_TEST_HRS",  	FALSE,             FALSE,
    //  dwPreviousEpochSecondsChecked   dwPreviousEpochPeriodTypeSelected
        0,                              0,
        // tPeriodConfiguration        
		//  ePeriodType                     bPeriodValue
		{   TRIGGER_PERIOD_TYPE_HOURS,    	1},
    },

    ///////////////////////////////////////////////////////////////
    // test days
    ///////////////////////////////////////////////////////////////
    //  pcName                  fIsTriggerEnabled  fIsTriggered
    {   "TRGG_PER_TEST_DAYS",  	FALSE,             FALSE,
    //  dwPreviousEpochSecondsChecked   dwPreviousEpochPeriodTypeSelected
        0,                              0,
        // tPeriodConfiguration        
		//  ePeriodType                     bPeriodValue
		{   TRIGGER_PERIOD_TYPE_DAYS,    	1},
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
BOOL TriggerPeriodInit( void )
{
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TriggerPeriodEnableActionTrigger( TriggerPeriodActionEnum eAction, BOOL fEnable )
{
    BOOL fSuccess = FALSE;
    
    if( eAction < TRIGGER_PERIOD_ACTION_MAX )
    {
        gtPeriodActionStat[eAction].fIsTriggerEnabled = fEnable;
		
		// if( fEnable )
		// {
			// SysTimeDateTimeStruct tDateTimeUtc;
			// SysTimeGetDateTime( &tDateTimeUtc, TRUE );
		
			// // set epoch to now
			// gtPeriodActionStat[eAction].dwPreviousEpochSecondsChecked 		= SysTimeDateTimeToEpochSeconds( &tDateTimeUtc );
			// gtPeriodActionStat[eAction].dwPreviousEpochPeriodTypeSelected 	= gtPeriodActionStat[eAction].dwPreviousEpochSecondsChecked;
		// }
		// else
		{
			gtPeriodActionStat[eAction].dwPreviousEpochSecondsChecked 		= 0;
			gtPeriodActionStat[eAction].dwPreviousEpochPeriodTypeSelected 	= 0;
		}
        
        fSuccess = TRUE;
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TriggerPeriodIsTriggerEnabled( TriggerPeriodActionEnum eAction )
{
    BOOL fSuccess = FALSE;
    
    if( eAction < TRIGGER_PERIOD_ACTION_MAX )
    {   
        fSuccess = gtPeriodActionStat[eAction].fIsTriggerEnabled;
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TriggerPeriodIsActionTriggered( TriggerPeriodActionEnum eAction )
{
    if( eAction < TRIGGER_PERIOD_ACTION_MAX )
    {
        return gtPeriodActionStat[eAction].fIsTriggered;
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
BOOL TriggerPeriodClearActionTriggeredFlag( TriggerPeriodActionEnum eAction )
{
    BOOL fSuccess = FALSE;
    
    if( eAction < TRIGGER_PERIOD_ACTION_MAX )
    {
        gtPeriodActionStat[eAction].fIsTriggered = FALSE;
        
        fSuccess = TRUE;
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TriggerPeriodSetPeriodType( TriggerPeriodActionEnum eAction, TriggerPeriodTypeEnum ePeriodType )
{
    BOOL fSuccess = FALSE;
    
    if( eAction < TRIGGER_PERIOD_ACTION_MAX )
    {        
		if( ePeriodType < TRIGGER_PERIOD_TYPE_MAX )
		{
			gtPeriodActionStat[eAction].tPeriodConfig.ePeriodType = ePeriodType;
		
			fSuccess = TRUE;
		}
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TriggerPeriodGetPeriodType( TriggerPeriodActionEnum eAction, TriggerPeriodTypeEnum *pePeriodType )
{
    BOOL fSuccess = FALSE;
    
    if( eAction < TRIGGER_PERIOD_ACTION_MAX )
    {
		if( pePeriodType != NULL )
		{
			(*pePeriodType) = gtPeriodActionStat[eAction].tPeriodConfig.ePeriodType;
		
			fSuccess = TRUE;
		}
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TriggerPeriodSetPeriodValue( TriggerPeriodActionEnum eAction, UINT16 wValueDependingOnPeriodType )
{
    BOOL fSuccess = FALSE;
    
    if( eAction < TRIGGER_PERIOD_ACTION_MAX )
    {
		gtPeriodActionStat[eAction].tPeriodConfig.wPeriodValue = wValueDependingOnPeriodType;
		
		fSuccess = TRUE;
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
INT32 TriggerPeriodGetPeriodValue( TriggerPeriodActionEnum eAction )
{
    INT32 idwPeriodValue = -1;
    
    if( eAction < TRIGGER_PERIOD_ACTION_MAX )
    {
        idwPeriodValue = (INT32)gtPeriodActionStat[eAction].tPeriodConfig.wPeriodValue;
    }
    
    return idwPeriodValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void TriggerPeriodUpdate( TriggerPeriodActionEnum eAction, UINT32 dwCurrentEpochSeconds )
{  
    if( eAction < TRIGGER_PERIOD_ACTION_MAX )
    {
        if( gtPeriodActionStat[eAction].fIsTriggerEnabled )
        {
            if( gtPeriodActionStat[eAction].fIsTriggered == FALSE )
            {
                // is time to check?
                if( gtPeriodActionStat[eAction].dwPreviousEpochSecondsChecked != dwCurrentEpochSeconds )
                {
                    // update previous time checked
                    gtPeriodActionStat[eAction].dwPreviousEpochSecondsChecked = dwCurrentEpochSeconds;
                    
                    // do a check depending on period type selected
                    switch( gtPeriodActionStat[eAction].tPeriodConfig.ePeriodType )
                    {
                        case TRIGGER_PERIOD_TYPE_SECONDS:
                        {                            
                            // check if is time to trigger action
                            if( dwCurrentEpochSeconds >= gtPeriodActionStat[eAction].dwPreviousEpochPeriodTypeSelected )
                            {
                                gtPeriodActionStat[eAction].dwPreviousEpochPeriodTypeSelected = dwCurrentEpochSeconds;
                                gtPeriodActionStat[eAction].dwPreviousEpochPeriodTypeSelected += (gtPeriodActionStat[eAction].tPeriodConfig.wPeriodValue * 1);
                        
                                // is time, set flag
                                gtPeriodActionStat[eAction].fIsTriggered = TRUE;
                            }   
                            break;
                        }
                
                        case TRIGGER_PERIOD_TYPE_MINUTES:
                        {                        
                            // check if is time to trigger action
                            if( dwCurrentEpochSeconds >= gtPeriodActionStat[eAction].dwPreviousEpochPeriodTypeSelected )
                            {
                                gtPeriodActionStat[eAction].dwPreviousEpochPeriodTypeSelected = dwCurrentEpochSeconds;
                                gtPeriodActionStat[eAction].dwPreviousEpochPeriodTypeSelected += (gtPeriodActionStat[eAction].tPeriodConfig.wPeriodValue * 60);
                        
                                // is time, set flag
                                gtPeriodActionStat[eAction].fIsTriggered = TRUE;
                            }   
                            break;
                        }
                
                        case TRIGGER_PERIOD_TYPE_HOURS:
                        {
                            // check if is time to trigger action
                            if( dwCurrentEpochSeconds >= gtPeriodActionStat[eAction].dwPreviousEpochPeriodTypeSelected )
                            {
                                gtPeriodActionStat[eAction].dwPreviousEpochPeriodTypeSelected = dwCurrentEpochSeconds;
                                gtPeriodActionStat[eAction].dwPreviousEpochPeriodTypeSelected += (gtPeriodActionStat[eAction].tPeriodConfig.wPeriodValue * 60 * 60);
                        
                                // is time, set flag
                                gtPeriodActionStat[eAction].fIsTriggered = TRUE;
                            }
                            break;
                        }
                
                        case TRIGGER_PERIOD_TYPE_DAYS:
                        {                        
                            // check if is time to trigger action
                            if( dwCurrentEpochSeconds >= gtPeriodActionStat[eAction].dwPreviousEpochPeriodTypeSelected )
                            {
                                gtPeriodActionStat[eAction].dwPreviousEpochPeriodTypeSelected = dwCurrentEpochSeconds;
                                gtPeriodActionStat[eAction].dwPreviousEpochPeriodTypeSelected += (gtPeriodActionStat[eAction].tPeriodConfig.wPeriodValue * 60 * 60 * 24 );
                        
                                // is time, set flag
                                gtPeriodActionStat[eAction].fIsTriggered = TRUE;
                            }   
                            break;
                        }
                
                        default:
                        {
                            break;
                        }
                    }// end switch
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
CHAR * TriggerPeriodGetActionName( TriggerPeriodActionEnum eAction )
{
    if( eAction < TRIGGER_PERIOD_ACTION_MAX )
    {
        return &gtPeriodActionStat[eAction].pcName[0];
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
CHAR * TriggerPeriodGetPeriodTypeName( TriggerPeriodTypeEnum ePeriodType )
{
    if( ePeriodType < TRIGGER_PERIOD_TYPE_MAX )
    {
        return &gtPeriodTypeNameArray[ePeriodType][0];
    }
    else
    {
        return NULL;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/* EOF */