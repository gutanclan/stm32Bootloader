//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!    \file        AdcAlarmTime.h
//!    \brief       alrm for adc values
//!
//!	   \author
//!	   \date
//!
//!    \notes      Considerations:
//!
//!    \defgroup   TypeOfModule   Group Name
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _ADC_ALARM_TIME_H_
#define _ADC_ALARM_TIME_H_

//////////////////////////////////////////////////////////////////////////////////////////////////
// Defines
//////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum
{
	ADC_ALARM_STATE_ON,
	ADC_ALARM_STATE_OFF,
	
	ADC_ALARM_STATE_MAX
}AdcAlarmTimeStateEnum;

typedef enum
{
	ADC_ALARM_TIME_CHANNEL_1 = 0,

	ADC_ALARM_TIME_MAX,	     // Total Number of QUEUES defined
}AdcAlarmTimeEnum;

#define     ADC_ALARM_TIME_INVALID    (ADC_ALARM_TIME_MAX)
	
//////////////////////////////////////////////////////////////////////////////////////////////////
// Public Functions
//////////////////////////////////////////////////////////////////////////////////////////////////

BOOL    AdcAlarmTimeModuleInit             		( void );

// alarm configurations
BOOL    AdcAlarmTimeEnableAlarmMonitoring  		( AdcAlarmTimeEnum eAdcAlarmTime, BOOL fEnable );
BOOL    AdcAlarmTimeEnableAlarmLow  			( AdcAlarmTimeEnum eAdcAlarmTime, BOOL fEnable );
BOOL    AdcAlarmTimeEnableAlarmHigh  			( AdcAlarmTimeEnum eAdcAlarmTime, BOOL fEnable );
BOOL    AdcAlarmTimeSetAlarmTransitionTime  	( AdcAlarmTimeEnum eAdcAlarmTime, UINT32 dwStateTransitionTime_Seconds );
BOOL    AdcAlarmTimeSetAlarmThresholdLow  		( AdcAlarmTimeEnum eAdcAlarmTime, SINGLE sgThreshold );
BOOL    AdcAlarmTimeSetAlarmThresholdHigh  		( AdcAlarmTimeEnum eAdcAlarmTime, SINGLE sgThreshold );

BOOL    AdcAlarmTimeIsAlarmMonitoringEnable  	( AdcAlarmTimeEnum eAdcAlarmTime );
BOOL    AdcAlarmTimeIsAlarmLowEnable  			( AdcAlarmTimeEnum eAdcAlarmTime );
BOOL    AdcAlarmTimeIsAlarmHighEnable  			( AdcAlarmTimeEnum eAdcAlarmTime );
UINT32  AdcAlarmTimeGetAlarmTransitionTime  	( AdcAlarmTimeEnum eAdcAlarmTime );
SINGLE  AdcAlarmTimeGetAlarmThresholdLow  		( AdcAlarmTimeEnum eAdcAlarmTime );
SINGLE  AdcAlarmTimeGetAlarmThresholdHigh  		( AdcAlarmTimeEnum eAdcAlarmTime );

// alarm monitoring
BOOL    AdcAlarmTimeCheckAlarm					( AdcAlarmTimeEnum eAdcAlarmTime, SINGLE *psgAdcReading );
BOOL    AdcAlarmTimeIsAlarmStateChanged			( AdcAlarmTimeEnum eAdcAlarmTime, BOOL *pfIsAlarmStateChanged );
BOOL    AdcAlarmTimeGetAlarmState				( AdcAlarmTimeEnum eAdcAlarmTime, AdcAlarmTimeStateEnum *peAlarmState );

CHAR * 	AdcAlarmTimeGetAlarmStringName			( AdcAlarmTimeEnum eAdcAlarmTime );

#endif // _QUEUE_CIRCULAR_H_

///////////////////////////////////////// END OF HEADER //////////////////////////////////////////
