/********************************************

File:  			UserIf.c

Description:	User Interface Module

Author: 		

Date:			

*********************************************/


/*******************************************************
*
*	System includes
*				
********************************************************/

#include <stdio.h>
#include <string.h>


/*******************************************************
*
*	Project includes
*				
********************************************************/

#include "Types.h"
#include "Target.h"             // Hardware target specifications

#include "General.h"
#include "Timer.h"
#include "Console.h"

#include "Control.h"
//#include "Config.h"
#include "Gpio.h"
//#include "Spi.h"
#include "Rtc.h"

#include "InterfaceBoard.h"
#include "PortEx.h"

#include "UserIf.h"
#include "Bluetooth.h"

/********************************************
*
*	Local Definitions
*
********************************************/

// everything that is not completily functional put it inside this define
#warning finish TODO list!
// to unveil TODO list set the TODO_LIST define to 1
// NOTE:input reed switches are working!!
// NOTE:output leds has to be fixed!!
#define TODO_LIST                                               (0)

///////////////////////////////////////////////////////////////
//                         UI INPUTS
///////////////////////////////////////////////////////////////

// debug = 0 = no print, 1 = print messages
#define DEBUG_UI_INPUT                                          (1)

#define USER_INT_REED_SWITCH_2_INTERRUPT_PREEMPTION_PRIORITY    (0)
#define USER_INT_REED_SWITCH_2_INTERRUPT_SUB_PRIORITY           (0x0F)
#define USER_INT_REED_SWITCH_2_PORT                             (TARGET_EXT_GPIO_PD1_PORT)
#define USER_INT_REED_SWITCH_2_RCC                              (TARGET_EXT_GPIO_PD1_RCC)
#define USER_INT_REED_SWITCH_2_PIN                              (TARGET_EXT_GPIO_PD1_PIN)
#define USER_INT_REED_SWITCH_2_EXT_INT_PORT                     (TARGET_EXT_GPIO_PD1_EXT_INT_PORT)
#define USER_INT_REED_SWITCH_2_EXT_INT_PIN                      (TARGET_EXT_GPIO_PD1_EXT_INT_PIN)
#define USER_INT_REED_SWITCH_2_EXT_INT_RCC_INIT()               (TARGET_EXT_GPIO_PD1_EXT_INT_RCC_INIT())
#define USER_INT_REED_SWITCH_2_EXT_INT_LINE                     (TARGET_EXT_GPIO_PD1_EXT_INT_LINE) 
#define USER_INT_REED_SWITCH_2_EXT_INT_IRQ                      (TARGET_EXT_GPIO_PD1_EXT_INT_IRQ)
#define USER_INT_REED_SWITCH_2_EXT_INT_IRQ_HNDLR                (TARGET_EXT_GPIO_PD1_EXT_INT_IRQ_HNDLR)


// NOTE: the longer time will overlap on top of the shorter time
#define INPUT_HOLD_TIME_FUNCTION_NON_MAX_MS                     (1500)
#define INPUT_HOLD_TIME_FUNCTION_1_MAX_MS                       (4000)
#define INPUT_HOLD_TIME_FUNCTION_2_MAX_MS                       (7000)
#define INPUT_HOLD_TIME_FUNCTION_3_MAX_MS                       (11000)

#define INPUT_BOUNCE_CHECK_TIME_MS                              (100)
#define INPUT_STABLE_STATE_CHECK_TIME_MS                        (200)

///////////////////////////////////////////////////////////////
//                         UI OUTPUTS
///////////////////////////////////////////////////////////////

#define UI_LED_SEGMENT_MAP_SIZE						15
#define UI_LED_SEGMENT_PLACMENT_PCB_NUM_OPTIONS		2

#define USERIF_LED_BLINK_FAST_NUM_BLINKS			3
#define USERIF_LED_BLINK_FAST_PERIOD_ON_MS			30			// On period in ms of the led fast blink
#define USERIF_LED_BLINK_FAST_PERIOD_OFF_MS			30			// Off period in ms of the led fast blink

#define USERIF_LED_BLINK_MODE_PERIOD_ON_MS			500			// On period in ms of the led mode blink
#define USERIF_LED_BLINK_MODE_PERIOD_OFF_MS			500			// Off period in ms of the led mode blink

#define USERIF_MODE_MULTIPLIER						2
#define USERIF_MODE_INTERVAL_MS						(1000*USERIF_MODE_MULTIPLIER)

#define USERIF_SHOWMODE_TIME_NORMAL_MS				(1000)
#define USERIF_SHOWMODE_TIME_EXTENDED_MS			(3000)

#define USERIF_MODE_0								0
#define USERIF_MODE_1								1
#define USERIF_MODE_2								2


/********************************************
*
*	Local Types
*
********************************************/

///////////////////////////////////////////////////////////////
//                         UI INPUTS
///////////////////////////////////////////////////////////////

typedef enum
{
    USER_IF_INPUT_PIN_STATE_INIT   = 0,
    USER_IF_INPUT_PIN_STATE_STABLE, 
    USER_IF_INPUT_PIN_STATE_BOUNCING,
    USER_IF_INPUT_PIN_STATE_DEFINING_STABLE_STATE,
    
    USER_IF_INPUT_PIN_STATE_MAX,

}UserIfInputPinStateMachineStatesEnum;

typedef enum
{
    UI_REED_SWITCH_FUNCTION_TRIGGER_STATE_INIT   = 0,
    UI_REED_SWITCH_FUNCTION_TRIGGER_STATE_WAITTING_FOR_EVENT,
    UI_REED_SWITCH_FUNCTION_TRIGGER_STATE_EVENT_STARTED,
    UI_REED_SWITCH_FUNCTION_TRIGGER_STATE_FUNCTION_SELECTION_IN_PROGRESS,     
    
    UI_REED_SWITCH_FUNCTION_TRIGGER_STATE_MAX,

}UserIfReedSwitchFunctionTriggerStateMachineStatesEnum;

typedef enum
{
    UI_REED_SWITCH_FUNC_TRIGG_FUNCTION_1 = 0,
    UI_REED_SWITCH_FUNC_TRIGG_FUNCTION_2,
    UI_REED_SWITCH_FUNC_TRIGG_FUNCTION_3,

    UI_REED_SWITCH_FUNC_TRIGG_MAX,    

}UserIfReedSwitchTrigeredFunctionEnum;

// used for the pin logic level state machine
typedef struct
{    
    IntBoardGpioPinEnum                                     ePin;
    UserIfInputPinStateMachineStatesEnum                    eStateMachineState;         // state machine
    LOGIC                                                   eCurrentStableLogicLevel;   // voltage stable logic level
    UINT32                                                  dwInterruptCounter;         // on every change of logic level in the input pin, the counter will get incremented
    TIMER                                                   tDownTimerDebouncingState;
    TIMER                                                   tDownTimerDefiningStableState;    
}UserIfReedSwitchLogicLevelType;

// used for the pin function triggering state machine
typedef struct
{
    BOOL                                                    fIsEventInProgress;
    UserIfReedSwitchFunctionTriggerStateMachineStatesEnum   eFunctionTriggerState;
    TIMER                                                   tUpTimerReedSwitchAssertedDurationMs;
    UserIfReedSwitchTrigeredFunctionEnum                    eFunctionSelected;
}UserIfReedSwitchFunctionTriggerStateType;

// used for the UI inputs. Puts together the 2 previous structures plus other information.
typedef struct
{
    BOOL                                                    fIsLogicInverted;
    UserIfReedSwitchLogicLevelType                          tReedSwitch;    
    UserIfReedSwitchFunctionTriggerStateType                tFunctionTrigger;    
}UserIfReedSwitchFunctionType;

/********************************************
*
*	Local Variables
*
********************************************/

///////////////////////////////////////////////////////////////
//                         UI INPUTS
///////////////////////////////////////////////////////////////

static          BOOL                    gfIsInterruptReedSwitch2Enabled = FALSE;
volatile static UINT32                  gdwInterruptReedSwitch2Counter  = 0;

static UserIfReedSwitchFunctionType gtReedSwitchStat[USER_IF_REED_SWITCH_MAX] = 
{    
    { 
        //  fIsLogicInverted
        #if defined(BUILD_TARGET_ADD_MAIN_BOARD_REV_766_MODIFICATIONS)
        FALSE,
        #else
        TRUE,
        #endif
        //  TYPE: UserIfReedSwitchLogicLevelType
        //  ePin,                   eStateMachineState              eCurrentStableLogicLevel    dwInterruptCounter  tDownTimerDebouncingState   tDownTimerDefiningStableState
        {   INT_B_BUTTON_REED_1,    USER_IF_INPUT_PIN_STATE_INIT,   LOW,                        0,                  0,                          0},
        //  TYPE: UserIfReedSwitchFunctionTriggerStateType
        //  fIsEventInProgress      eFunctionTriggerState                           tUpTimerReedSwitchAssertedDurationMs        eFunctionSelected
        {   FALSE,                  UI_REED_SWITCH_FUNCTION_TRIGGER_STATE_INIT,     0,                                          UI_REED_SWITCH_FUNC_TRIGG_FUNCTION_1 }
    },

    { 
        //  fIsLogicInverted
        FALSE,
        //  TYPE: UserIfReedSwitchLogicLevelType
        //  ePin,                   eStateMachineState              eCurrentStableLogicLevel    dwInterruptCounter  tDownTimerDebouncingState   tDownTimerDefiningStableState
        {   INT_B_BUTTON_REED_2,    USER_IF_INPUT_PIN_STATE_INIT,   LOW,                        0,                  0,                          0},
        //  TYPE: UserIfReedSwitchFunctionTriggerStateType
        //  fIsEventInProgress      eFunctionTriggerState                           tUpTimerReedSwitchAssertedDurationMs        eFunctionSelected
        {   FALSE,                  UI_REED_SWITCH_FUNCTION_TRIGGER_STATE_INIT,     0,                                          UI_REED_SWITCH_FUNC_TRIGG_FUNCTION_1 }
    }
};

#define USER_INT_NUMBER_OF_REED_SWITCH ( sizeof(gtReedSwitchStat)/sizeof(gtReedSwitchStat[0]) )

///////////////////////////////////////////////////////////////
//                         UI OUTPUTS
///////////////////////////////////////////////////////////////

/********************************************
*
*	Local Function Prototypes
*
********************************************/

///////////////////////////////////////////////////////////////
//                         UI INPUTS
///////////////////////////////////////////////////////////////
static void         UsertIfInterruptPinReedSwtch2Enable ( BOOL fInterruptEnable );
static BOOL         UserIfGetInterruptCounterReedSwitch ( UserIfReedSwitchEnum eReedSwitch, UINT32 *pdwInterruptCounter );

static void         UserIfReedSwitchStateMachine        ( void );
static BOOL         UserIfReedSwitchIsSwitchAsserted    ( UserIfReedSwitchEnum eReedSwitch, BOOL *pfIsAssertedResult );
static BOOL         UserIfReedSwitchFunctionSelected    ( UserIfReedSwitchEnum eReedSwitch, UserIfReedSwitchTrigeredFunctionEnum eFunctionSelected );
static void         UserIfInputPinShowStatus            ( ConsolePortEnum eConsole, UserIfReedSwitchEnum eRSwitch );

///////////////////////////////////////////////////////////////
//                         UI INPUTS
///////////////////////////////////////////////////////////////

/********************************************
*
* Function:  	BOOL UsertIfInterruptPinReedSwtchEnable( UserIfReedSwitchEnum eReedSwitch, BOOL fInterruptEnable )
*
* Description:	Enable/Disable interruptions in the ReedSwitch pins
*
* Arguments:	BOOL fInterruptEnable
*
* Return:		TRUE if no error, FALSE otherwise
*
********************************************/
BOOL UsertIfInterruptPinReedSwtchEnable( UserIfReedSwitchEnum eReedSwitch, BOOL fInterruptEnable )
{
    BOOL fSuccess = FALSE;

    if( eReedSwitch < USER_INT_NUMBER_OF_REED_SWITCH )
    {
        switch( eReedSwitch )
        {
            case USER_IF_REED_SWITCH_1:
                fSuccess = TRUE;
                RtcInterruptPinA0Enable( fInterruptEnable );
                break;

            case USER_IF_REED_SWITCH_2:
                fSuccess = TRUE;
                UsertIfInterruptPinReedSwtch2Enable( fInterruptEnable );
                break;

            default:

                break;
        }
    }

    return fSuccess;
}

/********************************************
*
* Function:  	BOOL UsertIfInterruptPinReedSwtch2Enable( BOOL fInterruptEnable )
*
* Description:	Enables interruptions in the pin ReedSwitch2
*
* Arguments:	BOOL fInterruptEnable
*
* Return:		none
*
********************************************/
void UsertIfInterruptPinReedSwtch2Enable( BOOL fInterruptEnable )
{
    GPIO_InitTypeDef    GPIO_InitStructure;
    EXTI_InitTypeDef    EXTI_InitStructure;
    NVIC_InitTypeDef    NVIC_InitStructure;
    /////////////////////////////////////////////////
    // CONFIGURE GPIO
    /////////////////////////////////////////////////
    // Initialized to defualt values
    GPIO_StructInit( &GPIO_InitStructure );
    // Enable clock
    RCC_AHB1PeriphClockCmd( USER_INT_REED_SWITCH_2_RCC, ENABLE );

    // #### SET PIN MODE ####
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_Pin     = USER_INT_REED_SWITCH_2_PIN;
    GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;   // dont care for input
    GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_NOPULL;
    GPIO_Init( USER_INT_REED_SWITCH_2_PORT, &GPIO_InitStructure );

    /////////////////////////////////////////////////
    // CONFIGURE GPIO EXTI
    /////////////////////////////////////////////////    
    // Configure Button EXTI line
    USER_INT_REED_SWITCH_2_EXT_INT_RCC_INIT();

    EXTI_InitStructure.EXTI_Line    = USER_INT_REED_SWITCH_2_EXT_INT_LINE;
    EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    //EXTI_InitStructure.EXTI_LineCmd = ENABLE;    

    /////////////////////////////////////////////////
    // CONFIGURE NVIC INTERRUPT VECTOR
    /////////////////////////////////////////////////
    // Enable the RTC Alarm Interrupt
    NVIC_InitStructure.NVIC_IRQChannel                      = USER_INT_REED_SWITCH_2_EXT_INT_IRQ;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority    = USER_INT_REED_SWITCH_2_INTERRUPT_PREEMPTION_PRIORITY;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority           = USER_INT_REED_SWITCH_2_INTERRUPT_SUB_PRIORITY;
    
    // clear interrupt flags before enabling interrupts
    EXTI_ClearITPendingBit( USER_INT_REED_SWITCH_2_EXT_INT_LINE );

    if( fInterruptEnable == TRUE )
    {        
        // Connect Button EXTI Line to GPIO Pin
        SYSCFG_EXTILineConfig( USER_INT_REED_SWITCH_2_EXT_INT_PORT, USER_INT_REED_SWITCH_2_EXT_INT_PIN );

        EXTI_InitStructure.EXTI_LineCmd         = ENABLE;
        NVIC_InitStructure.NVIC_IRQChannelCmd   = ENABLE;

        gfIsInterruptReedSwitch2Enabled         = TRUE;
    }
    else
    {
        EXTI_InitStructure.EXTI_LineCmd         = DISABLE;
        NVIC_InitStructure.NVIC_IRQChannelCmd   = DISABLE;

        gfIsInterruptReedSwitch2Enabled         = FALSE;
    }

    EXTI_Init( &EXTI_InitStructure );
    NVIC_Init( &NVIC_InitStructure );
}

/********************************************
*
* Function:  	USER_INT_REED_SWITCH_2_EXT_INT_IRQ_HNDLR()
*
* Description:	interrupt handler
*
* Arguments:	void
*
* Return:		none
*
********************************************/
void USER_INT_REED_SWITCH_2_EXT_INT_IRQ_HNDLR( void )
{
    if( EXTI_GetITStatus( USER_INT_REED_SWITCH_2_EXT_INT_LINE ) == SET )
    {
        // Clear the Wakeup Button EXTI line pending bit
        EXTI_ClearITPendingBit( USER_INT_REED_SWITCH_2_EXT_INT_LINE );

        gdwInterruptReedSwitch2Counter++;
    }
}

/********************************************
*
* Function:  	BOOL UserIfIsReedSwitchInterruptEnabled( UserIfReedSwitchEnum eReedSwitch, BOOL *pfIsEnabled )
*
* Description:	Enable/Disable interruptions from any of the input reed switches
*
* Arguments:	
*               eReedSwitch
*               pfIsEnabled
*
* Return:		TRUE is no error, FALSE otherwise
*
********************************************/
BOOL UserIfIsReedSwitchInterruptEnabled( UserIfReedSwitchEnum eReedSwitch, BOOL *pfIsEnabled )
{
    BOOL fSuccess = FALSE;

    if( eReedSwitch < USER_INT_NUMBER_OF_REED_SWITCH )
    {
        if( NULL != pfIsEnabled )
        {
            switch( eReedSwitch )
            {
                case USER_IF_REED_SWITCH_1:
                    fSuccess = TRUE;
                    *pfIsEnabled = RtcIsInterruptPinA0Enabled();
                    break;

                case USER_IF_REED_SWITCH_2:
                    fSuccess = TRUE;
                    *pfIsEnabled = gfIsInterruptReedSwitch2Enabled;
                    break;

                default:

                    break;
            }
        }
    }

    return fSuccess;
}

/********************************************
*
* Function:  	BOOL UserIfGetInterruptCounterReedSwitch( UserIfReedSwitchEnum eReedSwitch, UINT32 *pdwInterruptCounter )
*
* Description:	every change of logic level in the pin, the counter increments.
*               This function returns the value of that counter.
*
* Arguments:	
*               eReedSwitch
*               pfIsEnabled
*
* Return:		TRUE is no error, FALSE otherwise
*
********************************************/
BOOL UserIfGetInterruptCounterReedSwitch( UserIfReedSwitchEnum eReedSwitch, UINT32 *pdwInterruptCounter )
{
    BOOL fSuccess = FALSE;
    
    if( eReedSwitch < USER_INT_NUMBER_OF_REED_SWITCH )
    {
        if( NULL != pdwInterruptCounter )
        {
            switch( eReedSwitch )
            {
                case USER_IF_REED_SWITCH_1:                                
                    fSuccess = TRUE;
                    fSuccess = RtcGetInterruptCounter( pdwInterruptCounter, NULL );                    
                    break;

                case USER_IF_REED_SWITCH_2:                                    
                    fSuccess = TRUE;
                    *pdwInterruptCounter = gdwInterruptReedSwitch2Counter;                    
                    break;

                default:

                    break;
            }
        }
    }

    return fSuccess;
}

/********************************************
*
* Function:  	void UserIfReedSwitchFunctionTriggerStateMachine( void )
*
* Description:	state machine to monitor which function to be triggered depending on the input pin holding time.
*
* Arguments:    none
*
* Return:		none
*
********************************************/
void UserIfReedSwitchFunctionTriggerStateMachine( void )
{
    BOOL    fIsReedSwitchAsserted;
    UINT32  dwTotalAssertedTimeMs;

    // this updates the logic state of the pin.
    UserIfReedSwitchStateMachine();

    // now check if a pin is being held, and if so for how long to trigger an action.
    for( UserIfReedSwitchEnum eReedSwitch = 0 ; eReedSwitch < USER_INT_NUMBER_OF_REED_SWITCH ; eReedSwitch++ )
    {
        switch( gtReedSwitchStat[eReedSwitch].tFunctionTrigger.eFunctionTriggerState )
        {
            case UI_REED_SWITCH_FUNCTION_TRIGGER_STATE_INIT:
                gtReedSwitchStat[eReedSwitch].tFunctionTrigger.fIsEventInProgress = FALSE;                    
                gtReedSwitchStat[eReedSwitch].tFunctionTrigger.eFunctionTriggerState = UI_REED_SWITCH_FUNCTION_TRIGGER_STATE_WAITTING_FOR_EVENT;
                break;

            case UI_REED_SWITCH_FUNCTION_TRIGGER_STATE_WAITTING_FOR_EVENT:
                // waiting for the user to press a button
                if( gtReedSwitchStat[eReedSwitch].tFunctionTrigger.fIsEventInProgress )
                {                                        
                    gtReedSwitchStat[eReedSwitch].tFunctionTrigger.eFunctionTriggerState = UI_REED_SWITCH_FUNCTION_TRIGGER_STATE_EVENT_STARTED;
                }
                break;

            case UI_REED_SWITCH_FUNCTION_TRIGGER_STATE_EVENT_STARTED:
                // check the pin logic is stable and not bouncing
                if( gtReedSwitchStat[eReedSwitch].tReedSwitch.eStateMachineState == USER_IF_INPUT_PIN_STATE_STABLE )
                {
                    // check if pin is asserted
                    fIsReedSwitchAsserted = FALSE;
                    UserIfReedSwitchIsSwitchAsserted( eReedSwitch, &fIsReedSwitchAsserted );
                    
                    if( fIsReedSwitchAsserted )
                    {
                        ///////////////////////////////////////////////////////////////
                        #if DEBUG_UI_INPUT == 1
                        UserIfInputPinShowStatus( CONSOLE_PORT_USART, eReedSwitch );                
                        UINT64 totTicksSPU = TimerGetTotalTicksSincePowerup();
                        ConsoleDebugfln( CONSOLE_PORT_USART, "START COUNTING time stamp ttspu=%lu", totTicksSPU );
                        #endif
                        ///////////////////////////////////////////////////////////////
                        // counting time
                        gtReedSwitchStat[eReedSwitch].tFunctionTrigger.tUpTimerReedSwitchAssertedDurationMs = TimerUpTimerStartMs();
                        gtReedSwitchStat[eReedSwitch].tFunctionTrigger.eFunctionTriggerState = UI_REED_SWITCH_FUNCTION_TRIGGER_STATE_FUNCTION_SELECTION_IN_PROGRESS;
                    }
                    else
                    {
                        gtReedSwitchStat[eReedSwitch].tFunctionTrigger.eFunctionTriggerState = UI_REED_SWITCH_FUNCTION_TRIGGER_STATE_INIT;
                    }
                }
                break;

            case UI_REED_SWITCH_FUNCTION_TRIGGER_STATE_FUNCTION_SELECTION_IN_PROGRESS:
                // check the pin logic is stable and not bouncing
                if( gtReedSwitchStat[eReedSwitch].tReedSwitch.eStateMachineState == USER_IF_INPUT_PIN_STATE_STABLE )
                {
                    // check if pin is NOT asserted
                    fIsReedSwitchAsserted = FALSE;
                    UserIfReedSwitchIsSwitchAsserted( eReedSwitch, &fIsReedSwitchAsserted );
                    
                    if( fIsReedSwitchAsserted == FALSE )
                    {                                               
                        dwTotalAssertedTimeMs = TimerUpTimerGetMs( gtReedSwitchStat[eReedSwitch].tFunctionTrigger.tUpTimerReedSwitchAssertedDurationMs );                                                
                        
                        // check how long the switch was asserted
                        if( dwTotalAssertedTimeMs < INPUT_HOLD_TIME_FUNCTION_NON_MAX_MS ) // form "min stable state" to 4secs
                        {
                            // do nothing!!
                        }
                        else if( dwTotalAssertedTimeMs < INPUT_HOLD_TIME_FUNCTION_1_MAX_MS ) // form "min stable state" to 4secs
                        {
                            // trigger function 1 
                            gtReedSwitchStat[eReedSwitch].tFunctionTrigger.eFunctionSelected = UI_REED_SWITCH_FUNC_TRIGG_FUNCTION_1;
                            // trigger function/action                
                            UserIfReedSwitchFunctionSelected( eReedSwitch, gtReedSwitchStat[eReedSwitch].tFunctionTrigger.eFunctionSelected );
                        }                        
                        else if( dwTotalAssertedTimeMs < INPUT_HOLD_TIME_FUNCTION_2_MAX_MS ) // form 4 to 7
                        {
                            // trigger function 2                            
                            gtReedSwitchStat[eReedSwitch].tFunctionTrigger.eFunctionSelected = UI_REED_SWITCH_FUNC_TRIGG_FUNCTION_2;
                            // trigger function/action                
                            UserIfReedSwitchFunctionSelected( eReedSwitch, gtReedSwitchStat[eReedSwitch].tFunctionTrigger.eFunctionSelected );
                        }
                        else if( dwTotalAssertedTimeMs < INPUT_HOLD_TIME_FUNCTION_3_MAX_MS ) // form 7 to 11
                        {
                            // trigger function 3                            
                            gtReedSwitchStat[eReedSwitch].tFunctionTrigger.eFunctionSelected = UI_REED_SWITCH_FUNC_TRIGG_FUNCTION_3;                            
                            // trigger function/action                
                            UserIfReedSwitchFunctionSelected( eReedSwitch, gtReedSwitchStat[eReedSwitch].tFunctionTrigger.eFunctionSelected );
                        }                                            
                                           
                        ///////////////////////////////////////////////////////////////
                        #if DEBUG_UI_INPUT == 1
                        UserIfInputPinShowStatus( CONSOLE_PORT_USART, eReedSwitch );                
                        UINT64 totTicksSPU = TimerGetTotalTicksSincePowerup();
                        ConsoleDebugfln( CONSOLE_PORT_USART, "EVENT TRIGG time stamp ttspu=%lu", totTicksSPU );
                        #endif
                        ///////////////////////////////////////////////////////////////

                        ///////////////////////////////////////////////////////////////
                        #if DEBUG_UI_INPUT == 1                        
                        ConsolePrintf( CONSOLE_PORT_USART, "time = %d", dwTotalAssertedTimeMs );
                        ConsolePrintf( CONSOLE_PORT_USART, "\r\n\r\n" );
                        #endif
                        ///////////////////////////////////////////////////////////////
                        
                        // go back to init state.
                        gtReedSwitchStat[eReedSwitch].tFunctionTrigger.eFunctionTriggerState = UI_REED_SWITCH_FUNCTION_TRIGGER_STATE_INIT;
                    }
                }
                break;

//            case UI_REED_SWITCH_FUNCTION_TRIGGER_STATE_FUNCTION_TRIGGERED:    
//                
//                // trigger function/action                
//                UserIfReedSwitchFunctionSelected( eReedSwitch, gtReedSwitchStat[eReedSwitch].tFunctionTrigger.eFunctionSelected );
//
//                // go back to init state.
//                gtReedSwitchStat[eReedSwitch].tFunctionTrigger.eFunctionTriggerState = UI_REED_SWITCH_FUNCTION_TRIGGER_STATE_INIT;
//                break;

            default:
                gtReedSwitchStat[eReedSwitch].tFunctionTrigger.eFunctionTriggerState = UI_REED_SWITCH_FUNCTION_TRIGGER_STATE_INIT;
                break;
        }
    }    
}

/********************************************
*
* Function:  	void UserIfReedSwitchStateMachine( void )
*
* Description:	State machine to define the logic level of the pin taking care of the debouncing issue.
*
* Arguments:	none
*
* Return:		none
*
********************************************/
void UserIfReedSwitchStateMachine( void )
{
    UINT32 dwReedSwitchInterruptCounter;  
    ///////////////////////////////////////////////////////////////
    #if DEBUG_UI_INPUT == 1
    UINT64 totTicksSPU;
    #endif
    ///////////////////////////////////////////////////////////////

    for( UserIfReedSwitchEnum eReedSwitch = 0 ; eReedSwitch < USER_INT_NUMBER_OF_REED_SWITCH ; eReedSwitch++ )
    {
        switch( gtReedSwitchStat[eReedSwitch].tReedSwitch.eStateMachineState )
        {
            case USER_IF_INPUT_PIN_STATE_INIT:
                // read current logic state
                IntBoardGpioPinRead( gtReedSwitchStat[eReedSwitch].tReedSwitch.ePin, &gtReedSwitchStat[eReedSwitch].tReedSwitch.eCurrentStableLogicLevel );
                // set initial counter val
                gtReedSwitchStat[eReedSwitch].tReedSwitch.dwInterruptCounter = 0;
                // by default start pin logic as bouncing.
                // the state machine will find the default stable state.
                gtReedSwitchStat[eReedSwitch].tReedSwitch.eStateMachineState = USER_IF_INPUT_PIN_STATE_BOUNCING;
                gtReedSwitchStat[eReedSwitch].tReedSwitch.tDownTimerDebouncingState = TimerDownTimerStartMs( 100 );
                break;

            case USER_IF_INPUT_PIN_STATE_STABLE:                
                UserIfGetInterruptCounterReedSwitch( eReedSwitch, &dwReedSwitchInterruptCounter );
                // check if int counter has change
                if( gtReedSwitchStat[eReedSwitch].tReedSwitch.dwInterruptCounter != dwReedSwitchInterruptCounter )
                {   
                    // update interrupt counter new val 
                    gtReedSwitchStat[eReedSwitch].tReedSwitch.dwInterruptCounter = dwReedSwitchInterruptCounter;                
                    // start debounce down time counter                
                    gtReedSwitchStat[eReedSwitch].tReedSwitch.tDownTimerDebouncingState = TimerDownTimerStartMs( INPUT_BOUNCE_CHECK_TIME_MS );
                    // change to bouncing state
                    gtReedSwitchStat[eReedSwitch].tReedSwitch.eStateMachineState = USER_IF_INPUT_PIN_STATE_BOUNCING;
                    
                    ///////////////////////////////////////////////////////////////
                    #if DEBUG_UI_INPUT == 1
                    UserIfInputPinShowStatus( CONSOLE_PORT_USART, eReedSwitch );                
                    totTicksSPU = TimerGetTotalTicksSincePowerup();
                    ConsoleDebugfln( CONSOLE_PORT_USART, "EVENT CHANGE time stamp ttspu=%lu", totTicksSPU );                    
                    #endif
                    ///////////////////////////////////////////////////////////////
                }
                break;                

            case USER_IF_INPUT_PIN_STATE_BOUNCING:                           
                UserIfGetInterruptCounterReedSwitch( eReedSwitch, &dwReedSwitchInterruptCounter );
                // check if interrupt counter has change
                if( gtReedSwitchStat[eReedSwitch].tReedSwitch.dwInterruptCounter != dwReedSwitchInterruptCounter )
                {
                    // update interrupt counter new val 
                    gtReedSwitchStat[eReedSwitch].tReedSwitch.dwInterruptCounter = dwReedSwitchInterruptCounter;                    
                    // start debounce down time counter
                    gtReedSwitchStat[eReedSwitch].tReedSwitch.tDownTimerDebouncingState = TimerDownTimerStartMs( INPUT_BOUNCE_CHECK_TIME_MS );
                }
                else
                {
                    // check if bouncing time expired
                    if( TimerDownTimerIsExpired( gtReedSwitchStat[eReedSwitch].tReedSwitch.tDownTimerDebouncingState ) )
                    {
                        // start defining stable state down time counter                
                        gtReedSwitchStat[eReedSwitch].tReedSwitch.tDownTimerDefiningStableState = TimerDownTimerStartMs( INPUT_STABLE_STATE_CHECK_TIME_MS );
                        // change to defining stable state
                        gtReedSwitchStat[eReedSwitch].tReedSwitch.eStateMachineState = USER_IF_INPUT_PIN_STATE_DEFINING_STABLE_STATE;
                    }
                }            
                break;
        
            case USER_IF_INPUT_PIN_STATE_DEFINING_STABLE_STATE:                
                UserIfGetInterruptCounterReedSwitch( eReedSwitch, &dwReedSwitchInterruptCounter );
                // check if interrupt counter has change
                if( gtReedSwitchStat[eReedSwitch].tReedSwitch.dwInterruptCounter != dwReedSwitchInterruptCounter )
                {
                    // update interrupt counter new val 
                    gtReedSwitchStat[eReedSwitch].tReedSwitch.dwInterruptCounter = dwReedSwitchInterruptCounter;                
                    // start debounce down time counter                
                    gtReedSwitchStat[eReedSwitch].tReedSwitch.tDownTimerDebouncingState = TimerDownTimerStartMs( INPUT_BOUNCE_CHECK_TIME_MS );
                    // change to bouncing state
                    gtReedSwitchStat[eReedSwitch].tReedSwitch.eStateMachineState = USER_IF_INPUT_PIN_STATE_BOUNCING;
                }
                else
                {
                    // check if defining stable state time expired
                    if( TimerDownTimerIsExpired( gtReedSwitchStat[eReedSwitch].tReedSwitch.tDownTimerDefiningStableState ) )
                    {
                        // read current logic state. stable state
                        IntBoardGpioPinRead( gtReedSwitchStat[eReedSwitch].tReedSwitch.ePin, &gtReedSwitchStat[eReedSwitch].tReedSwitch.eCurrentStableLogicLevel );                
                    
                        // change to stable state
                        gtReedSwitchStat[eReedSwitch].tReedSwitch.eStateMachineState = USER_IF_INPUT_PIN_STATE_STABLE;

                        // indicate to the function trigger state machine that an action could be triggered because of the change in  logic state
                        gtReedSwitchStat[eReedSwitch].tFunctionTrigger.fIsEventInProgress = TRUE;
                        
                        ///////////////////////////////////////////////////////////////
                        #if DEBUG_UI_INPUT == 1
                        UserIfInputPinShowStatus( CONSOLE_PORT_USART, eReedSwitch );                
                        totTicksSPU = TimerGetTotalTicksSincePowerup();
                        ConsoleDebugfln( CONSOLE_PORT_USART, "LOGIC STABLE time stamp ttspu=%lu", totTicksSPU );
                        #endif
                        ///////////////////////////////////////////////////////////////
                    }
                }
                break;

            default:
                gtReedSwitchStat[eReedSwitch].tReedSwitch.eStateMachineState = USER_IF_INPUT_PIN_STATE_INIT;
                break;
        }
    }
}

/********************************************
*
* Function:  	BOOL UserIfReedSwitchIsSwitchAsserted( UserIfReedSwitchEnum eReedSwitch, BOOL *pfIsAssertedResult )
*
* Description:	Check if a pin is asserter taking into account if the logic of the pin is inverted.
*
* Arguments:    
*               eReedSwitch
*               pfIsAssertedResult : pointer to hold the result
*
* Return:		TRUE if success, FALSE if error
*
********************************************/
BOOL UserIfReedSwitchIsSwitchAsserted( UserIfReedSwitchEnum eReedSwitch, BOOL *pfIsAssertedResult )
{
    BOOL fSuccess = FALSE;

    if( eReedSwitch < USER_INT_NUMBER_OF_REED_SWITCH )
    {
        if( NULL != pfIsAssertedResult )
        {
            fSuccess = TRUE;

            if( gtReedSwitchStat[eReedSwitch].fIsLogicInverted )
            {
                *pfIsAssertedResult = gtReedSwitchStat[eReedSwitch].tReedSwitch.eCurrentStableLogicLevel == LOW ? TRUE : FALSE;
            }
            else
            {
                *pfIsAssertedResult = gtReedSwitchStat[eReedSwitch].tReedSwitch.eCurrentStableLogicLevel == LOW ? FALSE : TRUE ;
            }
        }
    }

    return fSuccess;
}

/********************************************
*
* Function:  	BOOL UserIfReedSwitchFunctionSelected( UserIfReedSwitchEnum eReedSwitch, UserIfReedSwitchTrigeredFunctionEnum eFunctionSelected )
*
* Description:	function that contains the actions to be taken for every type of input holding time.
*
* Arguments:	
*               eReedSwitch
*               eFunctionSelected
*
* Return:		TRUE if success, FALSE if error happen
*
********************************************/
BOOL UserIfReedSwitchFunctionSelected( UserIfReedSwitchEnum eReedSwitch, UserIfReedSwitchTrigeredFunctionEnum eFunctionSelected )
{
    BOOL fSuccess = FALSE;

    ///////////////////////////////////////////////////////////////
    #if DEBUG_UI_INPUT == 1
    ConsolePrintf( CONSOLE_PORT_USART, "RS%d function %d\r\n", (eReedSwitch+1), (eFunctionSelected+1) );  
    #endif
    ///////////////////////////////////////////////////////////////

    if( eReedSwitch < USER_INT_NUMBER_OF_REED_SWITCH )
    {
        if( eReedSwitch == USER_IF_REED_SWITCH_1 )
        {
            switch( eFunctionSelected )
            {            
                case UI_REED_SWITCH_FUNC_TRIGG_FUNCTION_1:   
                    // turn on blue tooth
                    BluetoothPowerEnable(TRUE);
                    // initiate serial bluetooth port time out.
                    ConsoleInactivityTimerRestart( CONSOLE_PORT_BLUETOOTH );
                    ConsolePrintf( CONSOLE_PORT_USART, "Bluetooth turned ON\r\n" );  
                    fSuccess = TRUE;
                    break;
                case UI_REED_SWITCH_FUNC_TRIGG_FUNCTION_2:                    
                    fSuccess = TRUE;
                    break;
                case UI_REED_SWITCH_FUNC_TRIGG_FUNCTION_3:                    
                    fSuccess = TRUE;
                    break;
                default:
                    break;
            }
        }
        else if( eReedSwitch == USER_IF_REED_SWITCH_2 )
        {
            switch( eFunctionSelected )
            {            
                case UI_REED_SWITCH_FUNC_TRIGG_FUNCTION_1:
                    ControlSystemRequest( CONTROL_ACTION_CAPTURE_IMAGE );
                    ControlSystemRequest( CONTROL_ACTION_FILE_CLOSE_QUEUE_AND_SEND );
                    fSuccess = TRUE;
                    break;
                case UI_REED_SWITCH_FUNC_TRIGG_FUNCTION_2:
                    ControlSystemRequest( CONTROL_ACTION_DOWNLOAD_SCRIPT );
                    fSuccess = TRUE;
                    break;
                case UI_REED_SWITCH_FUNC_TRIGG_FUNCTION_3:
                    fSuccess = TRUE;
                    break;
                default:
                    break;
            }
        }
    }    
    
    return fSuccess;
}

/********************************************
*
* Function:  	BOOL UserIfIsReedSwitchBussy( UserIfReedSwitchEnum eReedSwitch )
*
* Description:	verifies that a reed switch is trying to execute an operation.
*
* Arguments:	
*               eReedSwitch
*
* Return:		FALSE if Reed Switch not in use.
*
********************************************/
BOOL UserIfIsReedSwitchBussy( UserIfReedSwitchEnum eReedSwitch )
{
    BOOL fIsReedSwitchBussy = TRUE;
    BOOL fIsAssertedResult  = TRUE;

    UserIfReedSwitchIsSwitchAsserted( eReedSwitch, &fIsAssertedResult );

    if
    (
        // if switch is disasserted and stable..    
        ( fIsAssertedResult == FALSE ) && 
        ( USER_IF_INPUT_PIN_STATE_STABLE == gtReedSwitchStat[eReedSwitch].tReedSwitch.eStateMachineState )
    )
    {
        fIsReedSwitchBussy = FALSE;
    }

    return fIsReedSwitchBussy;
}

/********************************************
*
* Function:  	BOOL UserIfIsAnyReedSwitchBussy( void )
*
* Description:	verifies if any reed switch is trying to execute an operation.
*
* Arguments:	
*               eReedSwitch
*
* Return:		TRUE if at least one reed switch is trying to execute an operation. FALSE otherwise.
*
********************************************/
BOOL UserIfIsAnyReedSwitchBussy( void )
{
    BOOL fIsAnyReedSwitchBussy = FALSE;

    for( UserIfReedSwitchEnum eReedSwitch = 0 ; eReedSwitch < USER_INT_NUMBER_OF_REED_SWITCH ; eReedSwitch++ )
    {
        if( UserIfIsReedSwitchBussy( eReedSwitch ) )
        {
            fIsAnyReedSwitchBussy = TRUE;
        }        
    }

    return fIsAnyReedSwitchBussy;
}

/********************************************
*
* Function:  	void UserIfInputPinShowStatus( ConsolePortEnum eConsole, UserIfReedSwitchEnum eRSwitch )
*
* Description:	Prints the staTus of a User interface input pin.
*
* Arguments:    
*               eConsole: console enum to print the status
*               eReedSwitch: input pin
*
* Return:		TRUE if success, FALSE if error
*
********************************************/
void UserIfInputPinShowStatus( ConsolePortEnum eConsole, UserIfReedSwitchEnum eRSwitch )
{    
    if( eRSwitch < USER_INT_NUMBER_OF_REED_SWITCH )
    {
        ConsolePrintf( eConsole,"%+3u    %+12u    %+5u       %+10u\r\n", 
            gtReedSwitchStat[eRSwitch].tReedSwitch.ePin, 
            gtReedSwitchStat[eRSwitch].tReedSwitch.eStateMachineState,
            gtReedSwitchStat[eRSwitch].tReedSwitch.eCurrentStableLogicLevel,
            gtReedSwitchStat[eRSwitch].tReedSwitch.dwInterruptCounter
        );
    }
}

/********************************************
*
* \fn     void RtcPrintInterruptStatus( ConsolePortEnum eConsole )
*
* \brief  prints the status of Rtc interruptions
*
* \param[in]  eConsole   Console Id to print message
*
* \return void
*
********************************************/
void UserIfPrintInterruptStatus( ConsolePortEnum eConsole )
{
    BOOL    fIsInterruptReedSwitchEnabled;
    UINT32  dwInterruptReedSwitchCounter;

    for( UserIfReedSwitchEnum eRSwitch = 0; eRSwitch < USER_INT_NUMBER_OF_REED_SWITCH ; eRSwitch++ )
    {
        fIsInterruptReedSwitchEnabled  = FALSE;
        dwInterruptReedSwitchCounter   = 0;

        UserIfGetInterruptCounterReedSwitch( eRSwitch, &dwInterruptReedSwitchCounter );
        UserIfIsReedSwitchInterruptEnabled( eRSwitch, &fIsInterruptReedSwitchEnabled );

        ConsolePrintf( eConsole, "Is RS%d Interrupt Enabled:%d\r\n", (eRSwitch+1), fIsInterruptReedSwitchEnabled );
        ConsolePrintf( eConsole, "RS%d Counter Val:%d\r\n", (eRSwitch+1), dwInterruptReedSwitchCounter );        
    }
}
