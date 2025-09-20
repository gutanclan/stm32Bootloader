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
#include "Rtc.h"
#include "TriggerOnChange.h"

////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
    const CHAR *                 const      pcName;
    BOOL                                    fIsTriggerEnabled;
    BOOL                                    fIsTriggered;
    
    TriggerOnChangeTypeEnum                 eOnChangeTypeSelected;
    UINT32                                  dwPreviousUnitTypeSelected;
}TriggerOnChangeActionStatStruct;

////////////////////////////////////////////////////////////////////////////////////////////////////

static const CHAR * const gtOnChangeTypeNameArray[TRIGGER_ON_CHANGE_TYPE_MAX] = 
{
    "SECOND",
    "MINUTE",
    "HOUR",
    "DAY"
};

static TriggerOnChangeActionStatStruct gtOnChangeActionStat[TRIGGER_ON_CHANGE_ACTION_MAX] =  
{
    ///////////////////////////////////////////////////////////////
    // TRIGGER_ON_CHANGE_ACTION_MIN_SENSOR_SAMPLE_BATTERY_OK
    ///////////////////////////////////////////////////////////////
    //  pcName                  fIsTriggerEnabled  fIsTriggered    eOnChangeTypeSelected        	dwPreviousUnitTypeSelected       
    {   "TRGG_CHANGE_TEST_1",   FALSE,             FALSE,          TRIGGER_ON_CHANGE_TYPE_MINUTE, 	0 },

    ///////////////////////////////////////////////////////////////
    // Add more here below
    ///////////////////////////////////////////////////////////////
};

//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TriggerOnChangeInit( void )
{
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TriggerOnChangeEnableActionTrigger( TriggerOnChangeActionEnum eAction, BOOL fEnable )
{
    BOOL fSuccess = FALSE;
    
    if( eAction < TRIGGER_ON_CHANGE_ACTION_MAX )
    {
        // reset epoch units
        gtOnChangeActionStat[eAction].dwPreviousUnitTypeSelected       = 0;
    
        gtOnChangeActionStat[eAction].fIsTriggerEnabled = fEnable;
        
        fSuccess = TRUE;
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TriggerOnChangeIsTriggerEnabled( TriggerOnChangeActionEnum eAction )
{
    BOOL fSuccess = FALSE;
    
    if( eAction < TRIGGER_ON_CHANGE_ACTION_MAX )
    {   
        fSuccess = gtOnChangeActionStat[eAction].fIsTriggerEnabled;
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TriggerOnChangeIsActionTriggered( TriggerOnChangeActionEnum eAction )
{
    if( eAction < TRIGGER_ON_CHANGE_ACTION_MAX )
    {
        return gtOnChangeActionStat[eAction].fIsTriggered;
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
BOOL TriggerOnChangeClearActionTriggeredFlag( TriggerOnChangeActionEnum eAction )
{
    BOOL fSuccess = FALSE;
    
    if( eAction < TRIGGER_ON_CHANGE_ACTION_MAX )
    {
        gtOnChangeActionStat[eAction].fIsTriggered = FALSE;
        
        fSuccess = TRUE;
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TriggerOnChangeSetChangeType( TriggerOnChangeActionEnum eAction, TriggerOnChangeTypeEnum eChangeType )
{
    BOOL fSuccess = FALSE;
    
    if( eAction < TRIGGER_ON_CHANGE_ACTION_MAX )
    {
        if( eChangeType < TRIGGER_ON_CHANGE_TYPE_MAX )
        {
            gtOnChangeActionStat[eAction].eOnChangeTypeSelected = eChangeType;
        
            fSuccess = TRUE;
        }
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL TriggerOnChangeGetChangeType( TriggerOnChangeActionEnum eAction, TriggerOnChangeTypeEnum *peChangeType )
{
    BOOL fSuccess = FALSE;
    
    if( eAction < TRIGGER_ON_CHANGE_ACTION_MAX )
    {
        if( peChangeType != NULL )
        {            
            (*peChangeType) = gtOnChangeActionStat[eAction].eOnChangeTypeSelected;
        
            fSuccess = TRUE;
        }
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void TriggerOnChangeUpdate( TriggerOnChangeActionEnum eAction, RtcDateTimeStruct *ptDateTime )
{
    UINT32                    dwCurrentUnitTypeSelected;
    
    if( ptDateTime != NULL )
    {
        if( eAction < TRIGGER_ON_CHANGE_ACTION_MAX )
        {
            if( gtOnChangeActionStat[eAction].fIsTriggerEnabled )
            {
                if( gtOnChangeActionStat[eAction].fIsTriggered == FALSE )
                {
                    // do a check depending on period type selected
                    switch( gtOnChangeActionStat[eAction].eOnChangeTypeSelected )
                    {
                        case TRIGGER_ON_CHANGE_TYPE_SECOND:
                        {
                            dwCurrentUnitTypeSelected = ptDateTime->bSecond;
                            break;
                        }
                        
                        case TRIGGER_ON_CHANGE_TYPE_MINUTE:
                        {
                            dwCurrentUnitTypeSelected = ptDateTime->bMinute;
                            break;
                        }
                        
                        case TRIGGER_ON_CHANGE_TYPE_HOUR:
                        {
                            dwCurrentUnitTypeSelected = ptDateTime->bHour;
                            break;
                        }
                        
                        case TRIGGER_ON_CHANGE_TYPE_DAY:
                        {
                            dwCurrentUnitTypeSelected = ptDateTime->bDate;
                            break;
                        }
                        
                        default:
                        {
                            break;
                        }
                    }
                    
                    // check if time has changed
                    if( gtOnChangeActionStat[eAction].dwPreviousUnitTypeSelected != dwCurrentUnitTypeSelected )
                    {
                        // update previous time checked
                        gtOnChangeActionStat[eAction].dwPreviousUnitTypeSelected = dwCurrentUnitTypeSelected;
                            
                        // is time, set flag
                        gtOnChangeActionStat[eAction].fIsTriggered = TRUE;
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
CHAR * TriggerOnChangeGetActionName( TriggerOnChangeActionEnum eAction )
{
    if( eAction < TRIGGER_ON_CHANGE_ACTION_MAX )
    {
        return &gtOnChangeActionStat[eAction].pcName[0];
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
CHAR * TriggerOnChangeGetChangeTypeName( TriggerOnChangeTypeEnum eOnChangeMode )
{
    if( eOnChangeMode < TRIGGER_ON_CHANGE_TYPE_MAX )
    {
        return &gtOnChangeTypeNameArray[eOnChangeMode][0];
    }
    else
    {
        return NULL;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/* EOF */