/** C Header ******************************************************************

*******************************************************************************/

#include <assert.h>
#include <string.h>     /* memset() */
#include <stdio.h>      /* printf() */
#include <ctype.h>      /* isprint() */
#include <stdlib.h>

#include <stdarg.h>         // For va_arg support

#include "Types.h"

// PCOM Project Targets
#include "Target.h"             // Hardware target specifications

#include "Asc.h"
#include "Config.h"
#include "Debug.h"
#include "General.h"
#include "Gpio.h"

//#include "Modem.h"
//#include "Module.h"     /* Common task control structure definition */
#include "StringTable.h"
#include "Timer.h"
#include "Usart.h"
#include "Datalog.h"
#include "Control.h"
#include "Console.h"

#include "ModemPower.h"
#include "../ModemData.h"
#include "../ModemCommandResponse/Modem.h"
#include "../ModemCommandResponse/ModemCommand.h"
#include "../ModemCommandResponse/ModemResponse.h"
#include "../../Utils/StateMachine.h"

typedef enum
{
    MODEM_POWER_STATE_UNINITIALIZED = 0,
    
    MODEM_POWER_STATE_OFF,
    MODEM_POWER_STATE_ON_START,    
    MODEM_POWER_STATE_ON_PROC_0,
    MODEM_POWER_STATE_ON_PROC_1,
    MODEM_POWER_STATE_ON_PROC_2,    
    MODEM_POWER_STATE_ON_PROC_3,
    MODEM_POWER_STATE_ON_PROC_3_WAIT,
    MODEM_POWER_STATE_ON_PROC_4,
    MODEM_POWER_STATE_ON_PROC_5,
    MODEM_POWER_STATE_ON_PROC_AT_CMD_0,
    MODEM_POWER_STATE_ON_PROC_AT_CMD_1,
    
    MODEM_POWER_STATE_ON,
    MODEM_POWER_STATE_OFF_START,    
    MODEM_POWER_STATE_OFF_0,
    MODEM_POWER_STATE_OFF_1,
    MODEM_POWER_STATE_OFF_2,    
    MODEM_POWER_STATE_OFF_PROC_SHTDN_UNCONDITIONAL_0,    
    MODEM_POWER_STATE_OFF_PROC_SHTDN_UNCONDITIONAL_1,    
    MODEM_POWER_STATE_OFF_PROC_SHTDN_UNCONDITIONAL_2,    
    MODEM_POWER_STATE_OFF_PROC_SHTDN_UNCONDITIONAL_3,    
    MODEM_POWER_STATE_OFF_END,
    
    MODEM_POWER_STATE_MAX,
}ModemPowerStateEnum;

const CHAR *    cModemPowerStateMachineName[MODEM_POWER_STATE_MAX] = 
{
    "UNINITIALIZED",
    
    "OFF",
    "POWERING_ON_START",    
    "POWERING_ON_PROC_0",
    "POWERING_ON_PROC_1",
    "POWERING_ON_PROC_2",
    "POWERING_ON_PROC_3",
    "POWERING_ON_PROC_3_WAIT",
    "POWERING_ON_PROC_4",    
    "POWERING_ON_PROC_5",
    "POWERING_ON_PROC_AT_0",
    "POWERING_ON_PROC_AT_1",    
    
    "ON",
    "POWERING_OFF_START",
    "POWERING_OFF_PROC_0",
    "POWERING_OFF_PROC_1", 
    "POWERING_OFF_PROC_2",          
    "POWERING_OFF_PROC_SHTDN_UNCONDITIONAL_0",    
    "POWERING_OFF_PROC_SHTDN_UNCONDITIONAL_1",    
    "POWERING_OFF_PROC_SHTDN_UNCONDITIONAL_2",    
    "POWERING_OFF_PROC_SHTDN_UNCONDITIONAL_3",
    "POWERING_OFF_END",
};


////////////////////////////////////////////////////////////////////////////////////////////////////

// unique from this module
static BOOL                     gfIsPowerOn;

//
static ModemDataType           *gpModemData;

// module configuration
static BOOL                     gfPrintEnable = TRUE;
static BOOL                     gfPrintDebugEnable;

// operation
static BOOL                     gfIsWaitingForNewCommand;
static ModemStateMachineType    gtStateMachine;
static ModemCommandSemaphoreEnum geSemaphore;

////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void ModemPowerInit( void )
{
    
    gpModemData = ModemResponseModemDataGetPtr();
    
    if( gpModemData == NULL )
    {
        // catch this bug on development time
        while(1);
    }

    // set state machine to a initial state.
    StateMachineInit( &gtStateMachine.tState );     
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ModemPowerEnablePower( BOOL fEnable )
{
    if( fEnable )   // request to turn on modem
    {
        if( gfIsWaitingForNewCommand )
        {            
            // indicate command will start running so more commands are not accepted
            gfIsWaitingForNewCommand = FALSE;
            
            StateMachineChangeState( &gtStateMachine.tState, MODEM_POWER_STATE_ON_START );
        }
    }
    else
    {
        // if is already running a power off sequence then don't run it again
        //if( gfIsWaitingForNewCommand )
        if
        (
            ( gtStateMachine.tState.bStateCurrent != MODEM_POWER_STATE_OFF_START )  &&
            ( gtStateMachine.tState.bStateCurrent != MODEM_POWER_STATE_OFF_0 )      &&
            ( gtStateMachine.tState.bStateCurrent != MODEM_POWER_STATE_OFF_1 )      &&
            ( gtStateMachine.tState.bStateCurrent != MODEM_POWER_STATE_OFF_2 )      &&
            ( gtStateMachine.tState.bStateCurrent != MODEM_POWER_STATE_OFF_PROC_SHTDN_UNCONDITIONAL_0 ) &&
            ( gtStateMachine.tState.bStateCurrent != MODEM_POWER_STATE_OFF_PROC_SHTDN_UNCONDITIONAL_1 ) &&
            ( gtStateMachine.tState.bStateCurrent != MODEM_POWER_STATE_OFF_PROC_SHTDN_UNCONDITIONAL_2 ) &&
            ( gtStateMachine.tState.bStateCurrent != MODEM_POWER_STATE_OFF_PROC_SHTDN_UNCONDITIONAL_3 ) &&
            ( gtStateMachine.tState.bStateCurrent != MODEM_POWER_STATE_OFF_END )
        )
        {   
            // indicate command will start running so more commands are not accepted
            gfIsWaitingForNewCommand = FALSE;        
            
            StateMachineChangeState( &gtStateMachine.tState, MODEM_POWER_STATE_OFF_START );
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ModemPowerIsWaitingForCommand( void )
{
    return gfIsWaitingForNewCommand;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ModemPowerIsPowerEnabled( void )
{    
    // TODO:double check conditions in the pins for a power on    
    return gfIsPowerOn;    
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void ModemPowerStateMachine( void )
{
    StateMachineUpdate( &gtStateMachine.tState );

    if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
    {
        // print message that power is on
        ModemConsolePrintDbg( "MDM SM <%s>", cModemPowerStateMachineName[gtStateMachine.tState.bStateCurrent] );        
    }

    switch( gtStateMachine.tState.bStateCurrent )
    {        
        case MODEM_POWER_STATE_UNINITIALIZED:
        {            
            ////////////////////////////////////////////////////////
            // set initial state of the modem pins
            ////////////////////////////////////////////////////////
            GpioInitUnUsedPin( GPIO_MODEM_RTS_OUT );
            GpioInitUnUsedPin( GPIO_MODEM_CTS_IN );

            GpioInitUnUsedPin( GPIO_MODEM_DSR_IN );
            GpioInitUnUsedPin( GPIO_MODEM_DCD_IN );
            GpioInitUnUsedPin( GPIO_MODEM_RING_IN );
            GpioInitUnUsedPin( GPIO_MODEM_DTR_OUT );
                        
            // rx   set to input pull down
            // tx   set to input pull down
            UsartPortClose( USART2_PORT );

            GpioInitUnUsedPin( GPIO_MODEM_DATA_OE_OUT );                                   
            GpioInitUnUsedPin( GPIO_MODEM_HW_SHUTDOWN_OUT );
            GpioInitUnUsedPin( GPIO_MODEM_ON_OFF_OUT );
            GpioInitUnUsedPin( GPIO_MODEM_PWR_EN_OUT );  
            ////////////////////////////////////////////////////////

            // force power off procedure
            StateMachineChangeState( &gtStateMachine.tState, MODEM_POWER_STATE_OFF_START );
            break;
        }
            
        case MODEM_POWER_STATE_OFF:
        {
            //////////////////////////////////////////////////
            // IDLE
            // waiting for new operation request
            //////////////////////////////////////////////////

            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                // print message that power is on
                ModemConsolePrintf( "Modem Off\r\n" );
            }

            gfIsWaitingForNewCommand    = TRUE;
            gfIsPowerOn                 = FALSE;
            break;
        }
         
        case MODEM_POWER_STATE_ON_START:
        {
            // check if is already on so it doesn't need to repeat the procedure.
            if( gfIsPowerOn )
            {
                StateMachineChangeState( &gtStateMachine.tState, MODEM_POWER_STATE_ON );
            }
            else
            {
                StateMachineChangeState( &gtStateMachine.tState, MODEM_POWER_STATE_ON_PROC_0 );
            }
            break;
        }
        
        case MODEM_POWER_STATE_ON_PROC_0:
        {
            /////////////////////////////////////////////////////////////////////////////////////////
            // WARNING!!
            /////////////////////////////////////////////////////////////////////////////////////////
            // This should be the only case where accessing to "fIsProcessingCommandResponse" 
            // is used directly through the structure gpModemData.
            // Any other case should use ModemCommandProcessorReserve()ModemCommandProcessorRelease().
            // This hack is to guarantee the command processing will start fresh from zero.
            gpModemData->tCommandResponse.fIsProcessingCommandResponse = FALSE; 
            /////////////////////////////////////////////////////////////////////////////////////////

            // set the configuration of the pins to start the procedure            
            GpioInitSinglePin( GPIO_MODEM_DATA_OE_OUT, GPIO_MODE_OUTPUT, LOW, GPIO_OUTPUT_TYPE_PUSH_PULL );
            GpioInitSinglePin( GPIO_MODEM_HW_SHUTDOWN_OUT, GPIO_MODE_OUTPUT, LOW, GPIO_OUTPUT_TYPE_PUSH_PULL );
            GpioInitSinglePin( GPIO_MODEM_ON_OFF_OUT, GPIO_MODE_OUTPUT, LOW, GPIO_OUTPUT_TYPE_PUSH_PULL );
            GpioInitSinglePin( GPIO_MODEM_PWR_EN_OUT, GPIO_MODE_OUTPUT, LOW, GPIO_OUTPUT_TYPE_PUSH_PULL );
                       
            // Enable regulator for modem            
            GpioWrite( GPIO_MODEM_PWR_EN_OUT, HIGH );             

            StateMachineChangeState( &gtStateMachine.tState, MODEM_POWER_STATE_ON_PROC_1 );
            break;
        }
            
        // wait for VBATT to be 3.22V
        case MODEM_POWER_STATE_ON_PROC_1:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                // wait until 3v22 power line is stable
                StateMachineSetTimeOut( &gtStateMachine.tState, 500 );                
            }
            
            if( StateMachineIsTimeOut( &gtStateMachine.tState ) )
            {
                StateMachineChangeState( &gtStateMachine.tState, MODEM_POWER_STATE_ON_PROC_2 );
            }
            break;
        }

        // 
        case MODEM_POWER_STATE_ON_PROC_2:
        {            
            GpioWrite( GPIO_MODEM_ON_OFF_OUT, HIGH );

            StateMachineChangeState( &gtStateMachine.tState, MODEM_POWER_STATE_ON_PROC_3 );
            break;
        }

        // wait 5 seconds
        case MODEM_POWER_STATE_ON_PROC_3:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                StateMachineSetTimeOut( &gtStateMachine.tState, 6000 );                
            }
            
            if( StateMachineIsTimeOut( &gtStateMachine.tState ) )
            {
                StateMachineChangeState( &gtStateMachine.tState, MODEM_POWER_STATE_ON_PROC_3_WAIT );
            }
            break;
        }
        
        // ON_OFF modem pin back to high...
        // NOTE: pin controlled by a open collector. from mcu perspective pin control is inverted.
        // wait few milliseconds after modem ON_OFF pin goes back to high before checking the state of PWMON
        case MODEM_POWER_STATE_ON_PROC_3_WAIT:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                StateMachineSetTimeOut( &gtStateMachine.tState, 500 );
                
                GpioWrite( GPIO_MODEM_ON_OFF_OUT, LOW );                
            }
            
            if( StateMachineIsTimeOut( &gtStateMachine.tState ) )
            {
                StateMachineChangeState( &gtStateMachine.tState, MODEM_POWER_STATE_ON_PROC_4 );
            }
            break;
        }

        case MODEM_POWER_STATE_ON_PROC_4:
        {
            LOGIC eLogicResult;

            // enable OE to monitor rx state
            GpioWrite( GPIO_MODEM_DATA_OE_OUT, HIGH );
            
            TimerTaskDelayMs(50);
            // when opening port, rx pin becomes input/no pull down so we can read the state
            UsartPortOpen( TARGET_USART_PORT_TO_MODEM );

            TimerTaskDelayMs(5);

            GpioInputRead( GPIO_PORT_D, 6, &eLogicResult ); // mcu rx 
            // PWMON = ON ?
            if( eLogicResult == HIGH )
            {
                // PWRMON is on!
                StateMachineChangeState( &gtStateMachine.tState, MODEM_POWER_STATE_ON_PROC_5 );
            }
            else
            {
                // Set ERROR
                ModemEventLog
                (
                    TRUE,
                    "ModemPower[%s] Error='%s'", 
                    cModemPowerStateMachineName[gtStateMachine.tState.bStateCurrent], 
                    "PwrMon=low" 
                ); 

                // GOTO "HW SHUTDOWN unconditional"                                
                StateMachineChangeState( &gtStateMachine.tState, MODEM_POWER_STATE_OFF_PROC_SHTDN_UNCONDITIONAL_0 );                
            }
            break;
        }
        
        // delay 1 sec
        case MODEM_POWER_STATE_ON_PROC_5:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                StateMachineSetTimeOut( &gtStateMachine.tState, 1000 );                                
            }
            
            if( StateMachineIsTimeOut( &gtStateMachine.tState ) )
            {
                StateMachineChangeState( &gtStateMachine.tState, MODEM_POWER_STATE_ON_PROC_AT_CMD_0 );                
            }
            break;
        }       

        // delay 300 mSec
        case MODEM_POWER_STATE_ON_PROC_AT_CMD_0:
        {
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                StateMachineSetTimeOut( &gtStateMachine.tState, 300 );                                
            }
            
            if( StateMachineIsTimeOut( &gtStateMachine.tState ) )
            {
                StateMachineChangeState( &gtStateMachine.tState, MODEM_POWER_STATE_ON_PROC_AT_CMD_1 );                
            }
            break;
        }        

        // AT CMD 
        case MODEM_POWER_STATE_ON_PROC_AT_CMD_1:
        {                        
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                if( ModemCommandProcessorReserve( &geSemaphore ) )
                {
                    ModemCommandProcessorResetResponse();
                    ModemCommandProcessorSetExpectedResponse( FALSE, NULL, 0, TRUE );
                    
                    // if need to wait for response set time out
                    StateMachineSetTimeOut( &gtStateMachine.tState, 1000 );
                    
                    ModemCommandProcessorSendAtCommand( "" );
                }
                else
                {
                    // Set ERROR
                    ModemEventLog
                (
                    TRUE,
                        "ModemPower[%s] Error='%s'", 
                        cModemPowerStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "Semaphore busy" 
                    );                    

                    // before change state always release semaphore
                    ModemCommandProcessorRelease( &geSemaphore );
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_POWER_STATE_OFF_PROC_SHTDN_UNCONDITIONAL_0 );
                    break;
                }                        
            }
            
            if( ModemCommandProcessorIsResponseComplete() )
            {                
                if( ModemCommandProcessorIsError() )
                {
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_POWER_STATE_OFF_PROC_SHTDN_UNCONDITIONAL_0 );
                }
                else
                {                
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_POWER_STATE_ON );
                }
                // before change state always release semaphore
                ModemCommandProcessorRelease( &geSemaphore );
                break;
            }            

            if( StateMachineIsTimeOut( &gtStateMachine.tState ) ) 
            {
                // Set ERROR
                ModemEventLog
                (
                    TRUE,
                        "power[%s] Error='%s'", 
                        cModemPowerStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "timeout" 
                    );

                // before change state always release semaphore
                ModemCommandProcessorRelease( &geSemaphore );
                StateMachineChangeState( &gtStateMachine.tState, MODEM_POWER_STATE_OFF_PROC_SHTDN_UNCONDITIONAL_0 );
            }
            break;
        }

        case MODEM_POWER_STATE_ON:
        {
            //////////////////////////////////////////////////
            // IDLE
            // waiting for new operation request
            //////////////////////////////////////////////////

            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                // print message that power is on
                ModemConsolePrintf( "Modem On\r\n" );
            }

            gfIsWaitingForNewCommand    = TRUE;
            gfIsPowerOn                 = TRUE;
            break;
        }
        
        case MODEM_POWER_STATE_OFF_START:
        {
            // if is off, is allow to repeat procedure in case an attempt to go from off to on but failed happens            
            StateMachineChangeState( &gtStateMachine.tState, MODEM_POWER_STATE_OFF_0 );
            break;
        }

        case MODEM_POWER_STATE_OFF_0:
        {
            // not possible to check PWR MON without enabling 3v8 power line
            // therefore only check if 3v3 pwr line is enabled            
            GpioModeEnum    eMode;
            
            GpioGetPinMode( GPIO_MODEM_PWR_EN_OUT, &eMode );
                        
            if( eMode == GPIO_MODE_INPUT )
            {
                StateMachineChangeState( &gtStateMachine.tState, MODEM_POWER_STATE_OFF_END ); 
            }
            else
            {
                StateMachineChangeState( &gtStateMachine.tState, MODEM_POWER_STATE_OFF_1 ); 
            }
            break;
        }

        case MODEM_POWER_STATE_OFF_1:
        {            
            CHAR cChar;

            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                /////////////////////////////////////////////////////////////////////////////////////////
                // WARNING!!
                /////////////////////////////////////////////////////////////////////////////////////////
                // This should be the only case where accessing to "fIsProcessingCommandResponse" 
                // is used directly through the structure gpModemData.
                // Any other case should use ModemCommandProcessorReserve()ModemCommandProcessorRelease().
                // This hack is to guarantee the response for the command #SHDN will be catch.
                gpModemData->tCommandResponse.fIsProcessingCommandResponse = FALSE; 
                /////////////////////////////////////////////////////////////////////////////////////////

                if( ModemCommandProcessorReserve( &geSemaphore ) )
                {
                    ModemCommandProcessorResetResponse();
                    ModemCommandProcessorSetExpectedResponse( FALSE, NULL, 0, TRUE );
                                        
                    StateMachineSetTimeOut( &gtStateMachine.tState, 3000 );
                    
                    ModemCommandProcessorSendAtCommand( "#SHDN" );
                }
                else
                {
                    // Set ERROR
                    ModemEventLog
                (
                    TRUE,
                        "power[%s] Error='%s'", 
                        cModemPowerStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "semaphore busy" 
                    );

                    // before change state always release semaphore
                    ModemCommandProcessorRelease( &geSemaphore );
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_POWER_STATE_OFF_PROC_SHTDN_UNCONDITIONAL_0 ); // goto print result
                    break;
                }                
            }                       

            if( ModemCommandProcessorIsResponseComplete() )
            {
                if( ModemCommandProcessorIsError() )
                {
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_POWER_STATE_OFF_PROC_SHTDN_UNCONDITIONAL_0 );
                }
                else
                {                
                    /////////////////////////////////////////
                    LOGIC eLogicResult;
                    // read stat of RX
                    GpioInputRead( GPIO_PORT_D, 6, &eLogicResult );
                    if( eLogicResult == LOW )
                    {                    
                        StateMachineChangeState( &gtStateMachine.tState, MODEM_POWER_STATE_OFF_END );                     
                    }
                    else
                    {
                        StateMachineChangeState( &gtStateMachine.tState, MODEM_POWER_STATE_OFF_2 );                     
                    }
                    /////////////////////////////////////////
                }
                // before change state always release semaphore
                ModemCommandProcessorRelease( &geSemaphore );
                break;
            }
            
            if( StateMachineIsTimeOut( &gtStateMachine.tState ) ) 
            {
                // Set ERROR
                ModemEventLog
                (
                    TRUE,
                        "power[%s] Error='%s'", 
                        cModemPowerStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "timeout" 
                    );

                // before change state always release semaphore
                ModemCommandProcessorRelease( &geSemaphore );
                StateMachineChangeState( &gtStateMachine.tState, MODEM_POWER_STATE_OFF_PROC_SHTDN_UNCONDITIONAL_0 ); // goto print result
            }         
            break;
        }
    
        // count 15 seconds to check if power goes off.
        case MODEM_POWER_STATE_OFF_2:
        {
            LOGIC eLogicResult;

            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                // if you have to wait for response back always set time out as a precaution to avoid infinite loops
                StateMachineSetTimeOut( &gtStateMachine.tState, 15000 );                               
            }

            // read stat of RX
            GpioInputRead( GPIO_PORT_D, 6, &eLogicResult );

            if( eLogicResult == LOW )
            {
                // power down everything
                UsartPortClose( TARGET_USART_PORT_TO_MODEM );                
                GpioInitUnUsedPin( GPIO_MODEM_DATA_OE_OUT );                                   
                GpioInitUnUsedPin( GPIO_MODEM_HW_SHUTDOWN_OUT );
                GpioInitUnUsedPin( GPIO_MODEM_ON_OFF_OUT );
                GpioInitUnUsedPin( GPIO_MODEM_PWR_EN_OUT ); 

                StateMachineChangeState( &gtStateMachine.tState, MODEM_POWER_STATE_OFF_END );
            }
            else
            {
                if( StateMachineIsTimeOut( &gtStateMachine.tState ) )
                {
                    // Set ERROR
                    ModemEventLog
                (
                    TRUE,
                        "power[%s] Error='%s'", 
                        cModemPowerStateMachineName[gtStateMachine.tState.bStateCurrent], 
                        "timeout" 
                    );
                    StateMachineChangeState( &gtStateMachine.tState, MODEM_POWER_STATE_OFF_PROC_SHTDN_UNCONDITIONAL_0 );
                }                
            }
            break;
        }        

        case MODEM_POWER_STATE_OFF_PROC_SHTDN_UNCONDITIONAL_0:
        {                                    
            // HW_SHDN = LOW        
            GpioWrite( GPIO_MODEM_HW_SHUTDOWN_OUT, HIGH );
            
            StateMachineChangeState( &gtStateMachine.tState, MODEM_POWER_STATE_OFF_PROC_SHTDN_UNCONDITIONAL_1 );
            break;
        }

        // wait 200 ms
        case MODEM_POWER_STATE_OFF_PROC_SHTDN_UNCONDITIONAL_1:
        {            
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                // if you have to wait for response back always set time out as a precaution to avoid infinite loops
                StateMachineSetTimeOut( &gtStateMachine.tState, 200 );                                
            }
            
            if( StateMachineIsTimeOut( &gtStateMachine.tState ) )
            {
                StateMachineChangeState( &gtStateMachine.tState, MODEM_POWER_STATE_OFF_PROC_SHTDN_UNCONDITIONAL_2 );                
            }                
            
            break;
        }
        
        case MODEM_POWER_STATE_OFF_PROC_SHTDN_UNCONDITIONAL_2:
        {
            LOGIC eLogicResult;

            // HW_SHDN = HIGH
            GpioWrite( GPIO_MODEM_HW_SHUTDOWN_OUT, LOW );

            // delay 1 ms
            TimerTaskDelayMs(1);

            GpioInputRead( GPIO_PORT_D, 6, &eLogicResult ); // mcu rx 

            if( eLogicResult == LOW )
            {
                StateMachineChangeState( &gtStateMachine.tState, MODEM_POWER_STATE_OFF_END );
            }
            else
            {
                // PWRMON is on!
                // disconnect VBATT
                // harware doesnt support to only disconnect vbat so power down all the lines
                StateMachineChangeState( &gtStateMachine.tState, MODEM_POWER_STATE_OFF_END );
            }                 
            break;
        }

        // wait 1 sec... NOT USED!!
        case MODEM_POWER_STATE_OFF_PROC_SHTDN_UNCONDITIONAL_3:
        {            
            if( StateMachineIsFirtEntry( &gtStateMachine.tState ) )
            {
                // if you have to wait for response back always set time out as a precaution to avoid infinite loops
                StateMachineSetTimeOut( &gtStateMachine.tState, 1000 );                                
            }
            
            if( StateMachineIsTimeOut( &gtStateMachine.tState ) )
            {
                StateMachineChangeState( &gtStateMachine.tState, MODEM_POWER_STATE_OFF_PROC_SHTDN_UNCONDITIONAL_0 );                
            }                
            
            break;
        }

        case MODEM_POWER_STATE_OFF_END:
        {
            CHAR cChar;
            // make sure buffer is clear before the next power on
            while( UsartGetChar( USART2_PORT, &cChar, 1 ) );

            // guarantee the state of the pins in case you forgot to set one of them
            // set them to input pull down. GpioInitUnUsedPin() will do the job
            
            GpioInitUnUsedPin( GPIO_MODEM_RTS_OUT );
            GpioInitUnUsedPin( GPIO_MODEM_CTS_IN );

            GpioInitUnUsedPin( GPIO_MODEM_DSR_IN );
            GpioInitUnUsedPin( GPIO_MODEM_DCD_IN );
            GpioInitUnUsedPin( GPIO_MODEM_RING_IN );
            GpioInitUnUsedPin( GPIO_MODEM_DTR_OUT );
                        
            // rx   set to input pull down
            // tx   set to input pull down
            UsartPortClose( USART2_PORT );

            GpioInitUnUsedPin( GPIO_MODEM_DATA_OE_OUT );                                   
            GpioInitUnUsedPin( GPIO_MODEM_HW_SHUTDOWN_OUT );
            GpioInitUnUsedPin( GPIO_MODEM_ON_OFF_OUT );
            GpioInitUnUsedPin( GPIO_MODEM_PWR_EN_OUT );  
            
            // this will reset semaphores to use modem commands in case a command didn't 
            // exit correctly by using ModemCommandProcessorRelease().
            gpModemData->tCommandResponse.fIsProcessingCommandResponse = FALSE; 
                      
            StateMachineChangeState( &gtStateMachine.tState, MODEM_POWER_STATE_OFF );
            break;
        }          
        
        default:
        {
            StateMachineChangeState( &gtStateMachine.tState, MODEM_POWER_STATE_UNINITIALIZED );            
            break;
        }
    }
}
