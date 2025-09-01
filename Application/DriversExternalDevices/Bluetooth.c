//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!	\file		Bluetooth.c
//!	\brief		Bluetooth routines description.
//!
//!	\author		Joel Minski, Puracom Inc.
//! \date
//!
//! \details	Bluetooth driver for Roving Networks / Microchip RN-42.
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
// Feature Switches
//////////////////////////////////////////////////////////////////////////////////////////////////

#define BLUETOOTH_DEBUG_LEVEL  (0)     // 0 = module debug off
                                      // 1 = general debug
                                      // 2 = detailed debug,  3 = ...


//////////////////////////////////////////////////////////////////////////////////////////////////
// System Include Files
//////////////////////////////////////////////////////////////////////////////////////////////////

/* Standard includes. */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


//////////////////////////////////////////////////////////////////////////////////////////////////
// Project Include Files
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "Types.h"     	// Data types
#include "Target.h"     // Hardware target specifications
#include "General.h"    // General purpose memory and processing routines
#include "Debug.h"      // Debug library interface

#include "Usart.h"		// Uart module
#include "Gpio.h"
#include "Timer.h"
#include "Console.h"
#include "Command.h"
#include "Bluetooth.h"	// Bluetooth interface specification


//////////////////////////////////////////////////////////////////////////////////////////////////
// Macros and Definitions
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
// Global Variable Declarations
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
// Local Variable Declarations
//////////////////////////////////////////////////////////////////////////////////////////////////

static BOOL gfBluetoothEnabled			= FALSE;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Local Routines
//////////////////////////////////////////////////////////////////////////////////////////////////




//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn       	void BluetoothInitialize( void )
//!
//! \brief    	Initializes the module
//!
//!	\return		BOOL, TRUE if successful, FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BluetoothInitialize(void)
{
	BOOL fSuccess = TRUE;

    //BluetoothPowerEnable(TRUE);
    
    fSuccess &= BluetoothTestModule(); // test communications with the BT module.
    
    //BluetoothPowerEnable(FALSE);
	// Flow control not used at present, leave RTS low (asserted)
	//GpioWrite(GPIO_BLUETOOTH_RTS, LOW);	

	//fSuccess = ConsoleMountNewDictionary( CONSOLE_PORT_BLUETOOTH, CommandGetDictionaryPointer( COMMAND_DICTIONARY_BASIC_MENU ) );

	// These console strings likely won't be sent to the HOST as the connection won't be set up yet
	//ConsoleDebugfln( CONSOLE_PORT_BLUETOOTH, "Bluetooth Console Initialized" );

	//ConsolePrintPrompt( CONSOLE_PORT_BLUETOOTH );

	return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn			void BluetoothShowStatus( ConsolePortEnum eConsolePort )
//!
//! \brief		Show the status of the Bluetooth module.
//!
//!	\return		None
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void BluetoothShowStatus(ConsolePortEnum eConsolePort)
{
	ConsolePrintf( eConsolePort, "Bluetooth Power Enabled: %u\r\n",  gfBluetoothEnabled );
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn			void BluetoothPowerEnable( BOOL fEnable )
//!
//! \brief		Enable/Disable Power to the Bluetooth Module
//!
//!	\return		None
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void BluetoothPowerEnable( BOOL fEnable )
{
    if ( fEnable )
	{
        GpioWrite(GPIO_BLUETOOTH_RESET, LOW);

#if defined(BUILD_TARGET_ADD_MAIN_BOARD_REV_766_MODIFICATIONS)
        GpioWrite(GPIO_BT_POWER_EN, LOW);

        // Wait long enough for BT module to power down
		TimerTaskDelayMs(100);

        GpioWrite(GPIO_BT_POWER_EN, HIGH);
#endif
		// Wait for BT rail to stabilize
		TimerTaskDelayMs(20);

        // Bring BT Module out of reset
		GpioWrite(GPIO_BLUETOOTH_RESET, HIGH);

		UsartPortOpen( BLUETOOTH_SERIAL_PORT );

        gfBluetoothEnabled = fEnable;
	}
	else
	{
        gfBluetoothEnabled = fEnable;

        // Ensure BT modules is placed into reset before turning off power to it
        GpioWrite(GPIO_BLUETOOTH_RESET, LOW);

#if defined(BUILD_TARGET_ADD_MAIN_BOARD_REV_766_MODIFICATIONS)
        GpioWrite(GPIO_BT_POWER_EN, LOW);
#endif
		UsartPortClose( BLUETOOTH_SERIAL_PORT );
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn			BOOL BluetoothIsPowerEnabled( void )
//!
//! \brief		Check if the Bluetooth module is currently powered on
//!
//!	\return		BOOL, TRUE if power is enabled, FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BluetoothIsPowerEnabled( void )
{
	return gfBluetoothEnabled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn			BOOL BluetoothStatusLedEnable( void )
//!
//! \brief		turns on/off the stat LED of the bluetooth
//!
//!	\return		BOOL, TRUE all the time.
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BluetoothStatusLedEnable( BOOL fEnable )
{
    // WARNING!! The function doesn't check for return back from the bluetooth.
    // TODO: add response checking to make sure the operation is taking place.

    // Ensure no other bytes are sent to the Bluetooth module within a one second window before and after the "$$$" string
	TimerTaskDelayMs(1100);
	UsartPrintf( BLUETOOTH_SERIAL_PORT, "$$$" );// command mode on
    TimerTaskDelayMs(1100);

    UsartPrintf( BLUETOOTH_SERIAL_PORT, "+\r\n" ); // toggle echo
    TimerTaskDelayMs(1100);
    
    if( fEnable )
    {
        UsartPrintf( BLUETOOTH_SERIAL_PORT, "S@,2020\r\n" ); // GPIO5 set as output.
    }
    else
    {
        UsartPrintf( BLUETOOTH_SERIAL_PORT, "S@,2000\r\n" ); // GPIO5 set as input.
    }
    
    UsartPrintf( BLUETOOTH_SERIAL_PORT, "+\r\n" ); // toggle echo
    TimerTaskDelayMs(1100);
    UsartPrintf( BLUETOOTH_SERIAL_PORT, "---\r\n" ); // command mode off
    TimerTaskDelayMs(1100);

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn			BOOL BluetoothTestModule( void )
//!
//! \brief		Check if the Bluetooth module can be communicated with
//!
//!	\return		BOOL, TRUE if test passed, FALSE otherwise
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BluetoothTestModule( void )
{
	BOOL		fSuccess							= TRUE;
    BOOL		fBluetoothPowerEnabledPrevious		= BluetoothIsPowerEnabled();
	UINT8		bBuffer[32];

	memset(&bBuffer[0], 0, sizeof(bBuffer));

	BluetoothPowerEnable(FALSE);
    TimerTaskDelayMs(100);
    BluetoothPowerEnable(TRUE);
    TimerTaskDelayMs(100);

	// Ensure no other bytes are sent to the Bluetooth module within a one second window before and after the "$$$" string
	TimerTaskDelayMs(1100);
	UsartPrintf( BLUETOOTH_SERIAL_PORT, "$$$" );
    TimerTaskDelayMs(1100);

	UINT16		wBytesReturned = 0;

    // Wait for the response to be returned
	UsartGetBuffer(BLUETOOTH_SERIAL_PORT, &bBuffer[0], sizeof(bBuffer), &wBytesReturned, 1000);

	if (strstr(&bBuffer[0], "CMD\r\n") == NULL)
	{
		fSuccess = FALSE;
	}

	// Turn Bluetooth Module back off
    BluetoothPowerEnable(FALSE);

	// Restore the Bluetooth Module to its previous state
	if (fBluetoothPowerEnabledPrevious)
	{
		BluetoothPowerEnable(TRUE);
	}

	return fSuccess;
}

///////////////////////////////////////// END OF SOURCE //////////////////////////////////////////
