//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////


// C Library includes
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

//////////////////////////////////////////////////////////////////////////////////////////////////
// Header include
//////////////////////////////////////////////////////////////////////////////////////////////////

// STM Library includes
                                // NOTE: This file includes all peripheral.h files
#include "../inc/stm32f2xx_conf.h"     // Macro: assert_param() for other STM libraries.

// General Library includes
#include "../Utils/Types.h"
#include "../Utils/timer.h"
#include "../Targets/Target_CurrentTarget.h"

#include "../DriversMcuPeripherals/usart.h"
#include "../DriversMcuPeripherals/gpio.h"
#include "./serial.h"

typedef struct
{
	BOOL	fIsAutoDetectEnabled;
	BOOL	fIsClientConnected;

	BOOL	fIsTimerStarted;
	TIMER	tTimerUpTime;
	UINT16	wRxEventCount;
}SerialCleintConnectType;

static SerialCleintConnectType	gfSerialClientConnect[]=
{
	// USART3
	{
		// fIsAutoDetectEnabled
		FALSE,
		//fIsTimerStarted
		FALSE,
		// tTimerUpTime
		0,
		//wRxEventCount
		0
	}
};

#define SERIAL_CLIENT_COONECT_ARRAY_MAX	( sizeof(gfSerialClientConnect) / sizeof( gfSerialClientConnect[0] ) )

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SerialClientConnectInit( void )
{
    if( SERIAL_CLIENT_COONECT_ARRAY_MAX != USART_PORT_TOTAL )
    {
        while(1);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SerialClientConnectAutoDetectEnable( UsartPortEnum ePort, BOOL fEnable )
{
    BOOL fSuccess = FALSE;

    if( ePort < USART_PORT_TOTAL )
    {
        gfSerialClientConnect[ePort].fIsAutoDetectEnabled = fEnable;

        fSuccess = TRUE;
    }

    return fSuccess;
}

BOOL SerialClientConnectAutoDetectIsEnabled( UsartPortEnum ePort )
{
    if( ePort < USART_PORT_TOTAL )
    {
        return gfSerialClientConnect[ePort].fIsAutoDetectEnabled;
    }
    else
    {
        return FALSE;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SerialClientConnectIsConnected( UsartPortEnum ePort )
{
    if( ePort < USART_PORT_TOTAL )
    {
        return gfSerialClientConnect[ePort].fIsClientConnected;
    }
    else
    {
        return FALSE;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SerialClientConnectAutoUpdate( void )
{
    GpioLogicEnum eLogic;

    for( UsartPortEnum ePort = 0 ; ePort < USART_PORT_TOTAL ; ePort++ )
    {
        if( gfSerialClientConnect[ePort].fIsAutoDetectEnabled )
        {
            if( UsartPortIsEnabled(ePort) )
            {
                gfSerialClientConnect[ePort].fIsClientConnected = TRUE;

                // check if Rx voltage is 0
                GpioInputRegRead( GPIO_PORT_C, 11, &eLogic );

                if( eLogic == GPIO_LOGIC_LOW )
                {
                    // check if client got disconnected
                    if( gfSerialClientConnect[ePort].fIsTimerStarted == FALSE )
                    {
                        // start timer
                        gfSerialClientConnect[ePort].fIsTimerStarted = TRUE;

                        gfSerialClientConnect[ePort].tTimerUpTime = TimerUpTimerStartMs();

                        gfSerialClientConnect[ePort].wRxEventCount = UsartPortGetCharInCount( ePort );
                    }
                    else
                    {
                        // is time to check?
                        if( TimerUpTimerGetMs( gfSerialClientConnect[ePort].tTimerUpTime ) > 100 )
                        {
                            // restart timer
                            gfSerialClientConnect[ePort].tTimerUpTime = TimerUpTimerStartMs();

                            // is usart having Rx activity?
                            if( UsartPortGetCharInCount(ePort) == gfSerialClientConnect[ePort].wRxEventCount )
                            {
                                // no activity detected
                                // disable usart
                                UsartPortEnable( ePort, FALSE );
                            }
                            else // having activity
                            {
                                // reset vars
                                gfSerialClientConnect[ePort].fIsTimerStarted = FALSE;
                            }

                            // update activity variable
                            gfSerialClientConnect[ePort].wRxEventCount = UsartPortGetCharInCount(ePort);
                        }
                    }
                }
                else
                {
                    // reset vars
                    gfSerialClientConnect[ePort].fIsTimerStarted = FALSE;
                }
            }
            else // if usart disabled
            {
                gfSerialClientConnect[ePort].fIsClientConnected = FALSE;

                // check if client got connected
                GpioInputRegRead( GPIO_PORT_C, 11, &eLogic );

                if( eLogic == GPIO_LOGIC_HIGH )
                {
                    if( UsartPortIsEnabled(ePort) == FALSE )
                    {
                        UsartPortEnable( ePort, TRUE );
                    }

                    // reset vars
                    gfSerialClientConnect[ePort].fIsTimerStarted = FALSE;
                }
            }
        }
        else
        {
            // reset vars
            gfSerialClientConnect[ePort].fIsTimerStarted = FALSE;
        }
    }// end for loop
}



