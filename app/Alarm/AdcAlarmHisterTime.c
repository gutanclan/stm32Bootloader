//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!    \file        strQueueCir.h
//!    \brief       queue module header.
//!
//!	   \author
//!	   \date
//!
//!    \notes
//!
//!    \defgroup   TypeOfModule   Group Name
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
// Header include
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "../Utils/Types.h"
#include "AdcAlarmTime.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// File Local Structures and macros
//////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum
{
	ALARM_STATE_HEALTHY = 0,
	ALARM_STATE_PENDING_ALARM,
	ALARM_STATE_ALARM,
	ALARM_STATE_PENDING_HEALTHY,
	
	ALARM_STATE_MAX,
}AdcAlarmStateEnum;

typedef struct
{
	AdcAlarmStateEnum	eAlarmState;
    BOOL				fMonitoringEnable;
	BOOL				fAlarmLowEnable;
	BOOL				fAlarmHighEnable;
	UINT32 				dwStateTransitionTime_Sec;
	SINGLE 				sgThreshold;
	SINGLE 				sgThreshold;
}AdcAlarmType;


// declare buffer of required type
static CHAR	gcQueue1Buffer[STR_QUEUE_CIR_CONSOLE_1_BUFFER_SIZE_BYTES];

//////////////////////////////////////////////////////////////////////////////////////////////////
// File Local Variables
//////////////////////////////////////////////////////////////////////////////////////////////////

// This table must be in the same order as QueueEnum in queue.h
static AdcAlarmType gtAdcAlarmLookupArray[ADC_ALARM_TIME_MAX];

#define STR_QUEUE_CIR_ARRAY_MAX	( sizeof(gtAdcAlarmLookupArray) / sizeof( gtAdcAlarmLookupArray[0] ) )

///////////////////////////////// FUNCTION IMPLEMENTATION ////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     void QueueModuleInit( void )
//!
//! \brief  Initialize queue variables
//!
//! \param[in]  void
//!
//! \return
//!         - TRUE if no error
//!         - FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL AdcAlarmTimeModuleInit              		( void )
{
	return;
}


// alarm configurations
BOOL AdcAlarmTimeEnableAlarmMonitoring  		( AdcAlarmTimeEnum eAdcAlarmTime, BOOL fEnable )
{
	return;
}

BOOL AdcAlarmTimeEnableAlarmLow  			( AdcAlarmTimeEnum eAdcAlarmTime, BOOL fEnable )
{
	return;
}

BOOL AdcAlarmTimeEnableAlarmHigh  			( AdcAlarmTimeEnum eAdcAlarmTime, BOOL fEnable )
{
	return;
}

BOOL AdcAlarmTimeSetAlarmTransitionTime  	( AdcAlarmTimeEnum eAdcAlarmTime, UINT32 dwStateTransitionTime_Seconds )
{
	return;
}

BOOL AdcAlarmTimeSetAlarmThresholdLow  		( AdcAlarmTimeEnum eAdcAlarmTime, SINGLE sgThreshold )
{
	return;
}

BOOL AdcAlarmTimeSetAlarmThresholdHigh  		( AdcAlarmTimeEnum eAdcAlarmTime, SINGLE sgThreshold )
{
	return;
}

BOOL AdcAlarmTimeIsAlarmMonitoringEnable  	( AdcAlarmTimeEnum eAdcAlarmTime )
{
	return;
}

BOOL AdcAlarmTimeIsAlarmLowEnable  			( AdcAlarmTimeEnum eAdcAlarmTime )
{
	return;
}

BOOL AdcAlarmTimeIsAlarmHighEnable  			( AdcAlarmTimeEnum eAdcAlarmTime )
{
	return;
}

UINT32 AdcAlarmTimeGetAlarmTransitionTime  	( AdcAlarmTimeEnum eAdcAlarmTime )
{
	return;
}

SINGLE AdcAlarmTimeGetAlarmThresholdLow  		( AdcAlarmTimeEnum eAdcAlarmTime )
{
	return;
}

SINGLE AdcAlarmTimeGetAlarmThresholdHigh  		( AdcAlarmTimeEnum eAdcAlarmTime )
{
	return;
}

// alarm monitoring
BOOL AdcAlarmTimeCheckAlarm					( AdcAlarmTimeEnum eAdcAlarmTime, SINGLE *psgAdcReading )
{
	return;
}

BOOL AdcAlarmTimeIsAlarmStateChanged			( AdcAlarmTimeEnum eAdcAlarmTime, BOOL *pfIsAlarmStateChanged )
{
	return;
}

BOOL AdcAlarmTimeGetAlarmState				( AdcAlarmTimeEnum eAdcAlarmTime, AdcAlarmTimeStateEnum *peAlarmState )
{
	return;
}


CHAR * 	AdcAlarmTimeGetAlarmStringName			( AdcAlarmTimeEnum eAdcAlarmTime )
{
	return;
}

/////////////////////////////////////////// END OF SOURCE //////////////////////////////////////////
//

