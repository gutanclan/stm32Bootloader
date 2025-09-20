//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!    \file        Spi.c
//!    \brief       Functions for SPI pins.
//!
//!    \author
//!    \date
//!
//!    \notes
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
// Header include
//////////////////////////////////////////////////////////////////////////////////////////////////

// STM Library includes
                                // NOTE: This file includes all peripheral.h files
#include "../../hal/stm32f2xx_conf.h"     // Macro: assert_param() for other STM libraries.

// General Library includes
#include "../common/types.h"
#include "../common/Target_CurrentTarget.h"

#include "gpio.h"
#include "spi.h"
#include "../common/timer.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// Local Structure declarations and macros
//////////////////////////////////////////////////////////////////////////////////////////////////

#define SPI_FLAG_TOGGLE_WAITING_TIME_MS     (10)

///////////////////////////////////////////////////////
// SPI GPIOS CONFIGURATION
///////////////////////////////////////////////////////
// NOTE! Each bus has A CLK, MOSI, MISO
//       All pins must be initialized in main.c and Gpio.c
//       before the SPI uses them.
///////////////////////////////////////////////////////
typedef struct
{
    GpioPortEnum                tGpioPort;
    UINT8                       bGpioPin;
    GpioAlternativeFunctionEnum eAf;
}SpiGpioStruct;

///////////////////////////////////////////////////////
// BUS CONFIGURATIONS
///////////////////////////////////////////////////////
// NOTE! Each SPI can have different slaves with
//      their own particular configuration
///////////////////////////////////////////////////////
typedef struct
{
    const UINT16                wSPI_Mode;
    const UINT16                wSPI_DataSize;
    const UINT16                wSPI_CPOL;
    const UINT16                wSPI_CPHA;
    const UINT16                wSPI_BaudRatePrescaler;
    const UINT16                wSPI_FirstBit;
}SpiBusConfigStruct;

///////////////////////////////////////////////////////
// ENCAPSULATION OF SPI STRUCTURES ABOVE
///////////////////////////////////////////////////////
typedef struct
{
    const SpiBusEnum            		eBusId;
    const SpiBusConfigStruct * const 	ptConf;
}SpiSlaveBusStruct;

///////////////////////////////////////////////////////
// SPI SLAVE FINAL CONTAINER
// Populate this structure for proper work of routines.
///////////////////////////////////////////////////////
typedef struct
{
    const CHAR  * const 		pcSlaveName;
    const SpiSlaveBusStruct     tSpiBus;
	const SpiGpioStruct     	tGpioCs;
}SpiSlaveStruct;

///////////////////////////////////////////////////////
// BUS STATUS
///////////////////////////////////////////////////////
// NOTE! Variables that inform the status of the SPI
//      BUS
///////////////////////////////////////////////////////

typedef struct
{
    BOOL            			fIsBusRetained;
    SpiSlaveEnum                eCurrentSlave;
}SpiBusStatusStruct;

///////////////////////////////////////////////////////
// SPI BUS GENERAL VARIABLES
///////////////////////////////////////////////////////
// NOTE! General configs for a particular SPI
//      Bus.
///////////////////////////////////////////////////////
typedef struct
{
    const CHAR          * const pcBusName;
    SPI_TypeDef         * const tSPIx;
	UINT32                      dwRcc;
    const SpiGpioStruct     	tGpioMosi;
    const SpiGpioStruct     	tGpioMiso;
	const SpiGpioStruct     	tGpioClock;
    SpiBusStatusStruct          tStat;
}SpiBusStruct;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Local Variables
//////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
// LIST OF AVAILABLE SPI BUS TO BE USED IN THE PROJECT
// NOTE!! This list has to match with SpiBusEnum elements.
////////////////////////////////////////////////////////////////
static SpiBusStruct gtSpiBusLookupTable[] =
{
    ////////////////////////////////////
    // SPI BUS: SPI1
    ////////////////////////////////////
    {
		// pcBusName
		"SPI1",
		// tSPIx
		SPI1,
		// dwRcc
		RCC_APB2Periph_SPI1,
		// tGpioMosi
		{
			// tGpioPort
			GPIO_PORT_A,
			// bGpioPin
			6,
			// eAf
			GPIO_ALT_FUNC_SPI1_SPI2
		},
		// tGpioMiso
		{
			// tGpioPort
			GPIO_PORT_A,
			// bGpioPin
			7,
			// eAf
			GPIO_ALT_FUNC_SPI1_SPI2
		},
		// tGpioClock
		{
			// tGpioPort
			GPIO_PORT_A,
			// bGpioPin
			5,
			// eAf
			GPIO_ALT_FUNC_SPI1_SPI2
		},
		// tStat
		{
			// fIsBusRetained
			FALSE,
			// eCurrentSlave
			SPI_SLAVE_INVALID
		}
    },
};

#define SPI_BUS_COUNT       ( sizeof(gtSpiBusLookupTable)/sizeof(gtSpiBusLookupTable[0]) )

////////////////////////////////////////////////////////////////
// LIST OF SLAVE BUS CONFIGURATIONS
////////////////////////////////////////////////////////////////
static const SpiBusConfigStruct tSlaveDummyBusConfigurations =
{
/*    mode          */  SPI_Mode_Master,
/*    data size     */  SPI_DataSize_8b,
/*    clock polarity*/  SPI_CPOL_High,
/*    clock phase   */  SPI_CPHA_1Edge,
/*    presc Baud    */  SPI_BaudRatePrescaler_256,
/*    Indianness    */  SPI_FirstBit_MSB
};

static const SpiBusConfigStruct tSlaveDataFlashBusConfigurations =
{
/*    mode          */  SPI_Mode_Master,
/*    data size     */  SPI_DataSize_8b,
/*    clock polarity*/  SPI_CPOL_High,
/*    clock phase   */  SPI_CPHA_2Edge,
/*    presc Baud    */  SPI_BaudRatePrescaler_32,   // SPI slowed for system operation at 120MHz
/*    Indianness    */  SPI_FirstBit_MSB
};

static const SpiBusConfigStruct tSlaveAccelBusConfigurations =
{
/*    mode          */  SPI_Mode_Master,
/*    data size     */  SPI_DataSize_8b,
/*    clock polarity*/  SPI_CPOL_High,
/*    clock phase   */  SPI_CPHA_2Edge,
/*    presc Baud    */  SPI_BaudRatePrescaler_16,
/*    Indianness    */  SPI_FirstBit_MSB
};

static const SpiBusConfigStruct tSlavePortExBusConfigurations =
{
/*    mode          */  SPI_Mode_Master,
/*    data size     */  SPI_DataSize_8b,
/*    clock polarity*/  SPI_CPOL_Low,
/*    clock phase   */  SPI_CPHA_1Edge,
/*    presc Baud    */  SPI_BaudRatePrescaler_8,
/*    Indianness    */  SPI_FirstBit_MSB
};

static const SpiBusConfigStruct tSlaveAdcExBusConfigurations =
{
/*    mode          */  SPI_Mode_Master,
/*    data size     */  SPI_DataSize_8b,
/*    clock polarity*/  SPI_CPOL_High,
/*    clock phase   */  SPI_CPHA_2Edge,     // ??????????
/*    presc Baud    */  SPI_BaudRatePrescaler_8,
/*    Indianness    */  SPI_FirstBit_MSB
};

////////////////////////////////////////////////////////////////
// LIST OF SLAVES TO BE USED IN THE PROJECT
// NOTE!! This list has to match with SpiSlaveEnum elements.
////////////////////////////////////////////////////////////////
static const SpiSlaveStruct gtSpiSlaveLookupTable[] =
{
    ////////////////////////////////////
    // SPI SLAVE:
    ////////////////////////////////////
    {
        // Slave name
        "MEM_1",
		// tSpiBus
		{
			// eBusId
			SPI_BUS_1,
			// ptConf
			(SpiBusConfigStruct * const)&tSlaveDummyBusConfigurations
		},
		// tGpioCs
		{
			// tGpioPort
			GPIO_PORT_A,
			// bGpioPin
			4,
			// eAf
			GPIO_ALT_FUNC_UNKNOWN
		}
    }
};

#define SPI_SLAVE_COUNT     ( sizeof(gtSpiSlaveLookupTable)/sizeof(gtSpiSlaveLookupTable[0]) )

//////////////////////////////////////////////////////////////////////////////////////////////////
// Local Functions
//////////////////////////////////////////////////////////////////////////////////////////////////

static BOOL SpiIsBusPoweredOn           ( SpiBusEnum eSpiBus );
static BOOL SpiBusLock                  ( SpiBusEnum eSpiBus, UINT32 dwWaitingTicks );
static BOOL SpiBusUnlock                ( SpiBusEnum eSpiBus );
static BOOL SpiIsBusLocked              ( SpiBusEnum eSpiBus );
static BOOL SpiBusConnectToGpio         ( SpiBusEnum eSpiBus, BOOL fConnect );
static BOOL SpiBusSetSlaveConfig        ( SpiBusEnum eSpiBus, SpiSlaveEnum eSpiSlave );
static BOOL SpiIsBusReadyForSlaveUse    ( SpiSlaveEnum eSpiSlave, SpiBusEnum    *peSpiBus );


static BOOL SpiBusClockEnable			( SpiBusEnum eSpiBus, BOOL fEnable );

/////////////////////////////////// BODY OF THE LIBRARY //////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SpiModuleInit( void )
{
    BOOL fSuccess = FALSE;

    // Make sure the amount of elements in the arrays matches the elements in the enums
	fSuccess = TRUE;
    fSuccess &= ( SPI_SLAVE_COUNT == SPI_SLAVE_TOTAL );
    fSuccess &= ( SPI_BUS_COUNT   == SPI_BUS_TOTAL   );

    if( fSuccess == FALSE )
    {
		while(1);
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SpiBusClockEnable( SpiBusEnum eSpiBus, BOOL fEnable )
{
    BOOL fSuccess = FALSE;

    if( eSpiBus < SPI_BUS_COUNT )
    {
		// Select between APB1 or APB2 to start SPI clock
        switch( gtSpiBusLookupTable[eSpiBus].dwRcc )
        {
            case RCC_APB2Periph_SPI1:
                if( fEnable )
                {
                    RCC->APB2ENR |= gtSpiBusLookupTable[eSpiBus].dwRcc;
                }
                else{
                    RCC->APB2ENR &= ~gtSpiBusLookupTable[eSpiBus].dwRcc;
                }
                fSuccess = TRUE;
                break;

            case RCC_APB1Periph_SPI2:
            case RCC_APB1Periph_SPI3:
                if( fEnable )
                {
                    RCC->APB1ENR |= gtSpiBusLookupTable[eSpiBus].dwRcc;
                }
                else{
                    RCC->APB1ENR &= ~gtSpiBusLookupTable[eSpiBus].dwRcc;
                }
                fSuccess = TRUE;
                break;
            default:
                break;
        }
    }

    return fSuccess;
}

BOOL SpiBusEnable( SpiBusEnum eSpiBus, BOOL fEnable )
{
	BOOL fSuccess = FALSE;

	if( eSpiBus < SPI_BUS_COUNT )
    {
		if( fEnable )
		{
			if( SpiIsBusEnabled( eSpiBus ) == FALSE )
			{
				// NOTE:
				// make sure all the slaves chip selects that are going to be using that spi bus
				// are NOT asserted when initializing spi bus
				for( UINT8 bSlave = 0; bSlave < SPI_SLAVE_COUNT ; bSlave++ )
				{
					// only if slave is going to be using this bus
					if( gtSpiSlaveLookupTable[bSlave].tSpiBus.eBusId == eSpiBus )
					{
						// initialize gpio as output disasserted
						if( GpioPortClockIsEnabled( gtSpiSlaveLookupTable[bSlave].tGpioCs.tGpioPort ) == FALSE )
						{
							GpioPortClockEnable( gtSpiSlaveLookupTable[bSlave].tGpioCs.tGpioPort, fEnable );
						}
						// prepare pin for output
						GpioPuPdSetVal( gtSpiSlaveLookupTable[bSlave].tGpioCs.tGpioPort, gtSpiSlaveLookupTable[bSlave].tGpioCs.bGpioPin, GPIO_PUPD_NOPUPD );
						GpioSpeedSetVal( gtSpiSlaveLookupTable[bSlave].tGpioCs.tGpioPort, gtSpiSlaveLookupTable[bSlave].tGpioCs.bGpioPin, GPIO_OSPEED_25MHZ );
						GpioOTypeSetVal( gtSpiSlaveLookupTable[bSlave].tGpioCs.tGpioPort, gtSpiSlaveLookupTable[bSlave].tGpioCs.bGpioPin, GPIO_OTYPE_PUSH_PULL );
						GpioOutputRegWrite( gtSpiSlaveLookupTable[bSlave].tGpioCs.tGpioPort, gtSpiSlaveLookupTable[bSlave].tGpioCs.bGpioPin, GPIO_LOGIC_HIGH );
						GpioModeSetOutput( gtSpiSlaveLookupTable[bSlave].tGpioCs.tGpioPort, gtSpiSlaveLookupTable[bSlave].tGpioCs.bGpioPin );

						// Chip select unasserted
						// SpiSlaveChipSelectAssert( bSlave, FALSE );
					}
				}

				// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
				// GPIO CLK PIN
				// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
				if( GpioPortClockIsEnabled( gtSpiBusLookupTable[eSpiBus].tGpioClock.tGpioPort ) == FALSE )
				{
					GpioPortClockEnable( gtSpiBusLookupTable[eSpiBus].tGpioClock.tGpioPort, fEnable );
				}
				// prepare pin for output
				GpioSpeedSetVal( gtSpiBusLookupTable[eSpiBus].tGpioClock.tGpioPort, gtSpiBusLookupTable[eSpiBus].tGpioClock.bGpioPin, GPIO_OSPEED_25MHZ );
				GpioPuPdSetVal( gtSpiBusLookupTable[eSpiBus].tGpioClock.tGpioPort, gtSpiBusLookupTable[eSpiBus].tGpioClock.bGpioPin, GPIO_PUPD_NOPUPD );
				GpioOTypeSetVal( gtSpiBusLookupTable[eSpiBus].tGpioClock.tGpioPort, gtSpiBusLookupTable[eSpiBus].tGpioClock.bGpioPin, GPIO_OTYPE_PUSH_PULL );
				GpioModeSetOutput( gtSpiBusLookupTable[eSpiBus].tGpioClock.tGpioPort, gtSpiBusLookupTable[eSpiBus].tGpioClock.bGpioPin );
				GpioOutputRegWrite( gtSpiBusLookupTable[eSpiBus].tGpioClock.tGpioPort, gtSpiBusLookupTable[eSpiBus].tGpioClock.bGpioPin, GPIO_LOGIC_HIGH );
				// change gpio as Alternative function
				GpioModeSetAf( gtSpiBusLookupTable[eSpiBus].tGpioClock.tGpioPort, gtSpiBusLookupTable[eSpiBus].tGpioClock.bGpioPin, gtSpiBusLookupTable[eSpiBus].tGpioClock.eAf );

				// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
				// GPIO MOSI PIN
				// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
				if( GpioPortClockIsEnabled( gtSpiBusLookupTable[eSpiBus].tGpioMosi.tGpioPort ) == FALSE )
				{
					GpioPortClockEnable( gtSpiBusLookupTable[eSpiBus].tGpioMosi.tGpioPort, fEnable );
				}
				// prepare pin for output
				GpioSpeedSetVal( gtSpiBusLookupTable[eSpiBus].tGpioMosi.tGpioPort, gtSpiBusLookupTable[eSpiBus].tGpioMosi.bGpioPin, GPIO_OSPEED_25MHZ );
				GpioPuPdSetVal( gtSpiBusLookupTable[eSpiBus].tGpioMosi.tGpioPort, gtSpiBusLookupTable[eSpiBus].tGpioMosi.bGpioPin, GPIO_PUPD_NOPUPD );
				GpioOTypeSetVal( gtSpiBusLookupTable[eSpiBus].tGpioMosi.tGpioPort, gtSpiBusLookupTable[eSpiBus].tGpioMosi.bGpioPin, GPIO_OTYPE_PUSH_PULL );
				GpioModeSetOutput( gtSpiBusLookupTable[eSpiBus].tGpioMosi.tGpioPort, gtSpiBusLookupTable[eSpiBus].tGpioMosi.bGpioPin );
				// change gpio as Alternative function
				GpioModeSetAf( gtSpiBusLookupTable[eSpiBus].tGpioMosi.tGpioPort, gtSpiBusLookupTable[eSpiBus].tGpioMosi.bGpioPin, gtSpiBusLookupTable[eSpiBus].tGpioMosi.eAf );

				// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
				// GPIO MISO PIN
				// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
				if( GpioPortClockIsEnabled( gtSpiBusLookupTable[eSpiBus].tGpioMiso.tGpioPort ) == FALSE )
				{
					GpioPortClockEnable( gtSpiBusLookupTable[eSpiBus].tGpioMiso.tGpioPort, fEnable );
				}
				// prepare pin for output
				GpioSpeedSetVal( gtSpiBusLookupTable[eSpiBus].tGpioMiso.tGpioPort, gtSpiBusLookupTable[eSpiBus].tGpioMiso.bGpioPin, GPIO_OSPEED_25MHZ );
				GpioPuPdSetVal( gtSpiBusLookupTable[eSpiBus].tGpioMiso.tGpioPort, gtSpiBusLookupTable[eSpiBus].tGpioMiso.bGpioPin, GPIO_PUPD_NOPUPD );
				GpioModeSetInput( gtSpiBusLookupTable[eSpiBus].tGpioMiso.tGpioPort, gtSpiBusLookupTable[eSpiBus].tGpioMiso.bGpioPin );
				// change gpio as Alternative function
				GpioModeSetAf( gtSpiBusLookupTable[eSpiBus].tGpioMiso.tGpioPort, gtSpiBusLookupTable[eSpiBus].tGpioMiso.bGpioPin, gtSpiBusLookupTable[eSpiBus].tGpioMiso.eAf );

				// enable periph clock
				SpiBusClockEnable( eSpiBus, fEnable );

				// configure spi
				// only when slave make use of the bus since each slave has different configuration requirements
				// enable spi (this is done when spi is configured for particular slave)
			}

			fSuccess = TRUE;
		}
		else
		{
			// disable procedure
			if( SpiIsBusEnabled( eSpiBus ) == TRUE )
			{
				// NOTE:
				// make sure all the slaves chip selects are NOT asserted when disapling spi bus
				for( UINT8 bSlave = 0; bSlave < SPI_SLAVE_COUNT ; bSlave++ )
				{
					// only if slave is going to be using this bus
					if( gtSpiSlaveLookupTable[bSlave].tSpiBus.eBusId == eSpiBus )
					{
						// Chip select unasserted
						//SpiSlaveChipSelectAssert( bSlave, FALSE );
						GpioOutputRegWrite( gtSpiSlaveLookupTable[bSlave].tGpioCs.tGpioPort, gtSpiSlaveLookupTable[bSlave].tGpioCs.bGpioPin, GPIO_LOGIC_LOW );
					}
				}

				// enable periph clock
				SpiBusClockEnable( eSpiBus, fEnable );

				// all gpios as inputs
				// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
				// GPIO MISO PIN
				// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
				GpioSpeedSetVal( gtSpiBusLookupTable[eSpiBus].tGpioMiso.tGpioPort, gtSpiBusLookupTable[eSpiBus].tGpioMiso.bGpioPin, GPIO_OSPEED_2MHZ );
				GpioPuPdSetVal( gtSpiBusLookupTable[eSpiBus].tGpioMiso.tGpioPort, gtSpiBusLookupTable[eSpiBus].tGpioMiso.bGpioPin, GPIO_PUPD_P_DOWN );
				GpioModeSetInput( gtSpiBusLookupTable[eSpiBus].tGpioMiso.tGpioPort, gtSpiBusLookupTable[eSpiBus].tGpioMiso.bGpioPin );
				// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
				// GPIO MOSI PIN
				// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
				GpioSpeedSetVal( gtSpiBusLookupTable[eSpiBus].tGpioMosi.tGpioPort, gtSpiBusLookupTable[eSpiBus].tGpioMosi.bGpioPin, GPIO_OSPEED_2MHZ );
				GpioPuPdSetVal( gtSpiBusLookupTable[eSpiBus].tGpioMosi.tGpioPort, gtSpiBusLookupTable[eSpiBus].tGpioMosi.bGpioPin, GPIO_PUPD_P_DOWN );
				GpioModeSetInput( gtSpiBusLookupTable[eSpiBus].tGpioMosi.tGpioPort, gtSpiBusLookupTable[eSpiBus].tGpioMosi.bGpioPin );
				// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
				// GPIO CLK PIN
				// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
				GpioSpeedSetVal( gtSpiBusLookupTable[eSpiBus].tGpioClock.tGpioPort, gtSpiBusLookupTable[eSpiBus].tGpioClock.bGpioPin, GPIO_OSPEED_2MHZ );
				GpioPuPdSetVal( gtSpiBusLookupTable[eSpiBus].tGpioClock.tGpioPort, gtSpiBusLookupTable[eSpiBus].tGpioClock.bGpioPin, GPIO_PUPD_P_DOWN );
				GpioModeSetInput( gtSpiBusLookupTable[eSpiBus].tGpioClock.tGpioPort, gtSpiBusLookupTable[eSpiBus].tGpioClock.bGpioPin );

				// set all bus chip select pins as inputs
				for( UINT8 bSlave = 0; bSlave < SPI_SLAVE_COUNT ; bSlave++ )
				{
					// only if slave is going to be using this bus
					if( gtSpiSlaveLookupTable[bSlave].tSpiBus.eBusId == eSpiBus )
					{
						GpioSpeedSetVal( gtSpiSlaveLookupTable[bSlave].tGpioCs.tGpioPort, gtSpiSlaveLookupTable[bSlave].tGpioCs.bGpioPin, GPIO_OSPEED_2MHZ );
						GpioPuPdSetVal( gtSpiSlaveLookupTable[bSlave].tGpioCs.tGpioPort, gtSpiSlaveLookupTable[bSlave].tGpioCs.bGpioPin, GPIO_PUPD_P_DOWN );
						GpioModeSetInput( gtSpiSlaveLookupTable[bSlave].tGpioCs.tGpioPort, gtSpiSlaveLookupTable[bSlave].tGpioCs.bGpioPin );
					}
				}
			}

			fSuccess = TRUE;
		}
	}

	return fSuccess;
}

BOOL SpiIsBusEnabled( SpiBusEnum eSpiBus )
{
	BOOL fEnabled = FALSE;

    if( eSpiBus < SPI_BUS_COUNT )
    {
		// Select between APB1 or APB2 to start SPI clock
        switch( gtSpiBusLookupTable[eSpiBus].dwRcc )
        {
            case RCC_APB1Periph_SPI2:
            case RCC_APB1Periph_SPI3:
                fEnabled = ( ( RCC->APB1ENR & gtSpiBusLookupTable[eSpiBus].dwRcc ) > 0 );
                break;

            case RCC_APB2Periph_SPI1:
                fEnabled = ( ( RCC->APB2ENR & gtSpiBusLookupTable[eSpiBus].dwRcc ) > 0 );
                break;
            default:
                break;
        }
    }

    return fEnabled;
}

BOOL SpiIsAnySlaveHoldingBus( SpiBusEnum eSpiBus )
{
	BOOL fIsBusRetained = FALSE;

    if( eSpiBus < SPI_BUS_COUNT )
    {
		fIsBusRetained = gtSpiBusLookupTable[eSpiBus].tStat.fIsBusRetained;
	}

	return fIsBusRetained;
}

BOOL SpiGetSlaveHoldingBus( SpiBusEnum eSpiBus, SpiSlaveEnum *peSpiSlave )
{
	BOOL fSuccess = FALSE;

    if( eSpiBus < SPI_BUS_COUNT )
    {
		if( peSpiSlave != NULL )
		{
			(*peSpiSlave) = gtSpiBusLookupTable[eSpiBus].tStat.eCurrentSlave;
		}
	}

	return fSuccess;
}

BOOL SpiSlaveClaimBus( SpiSlaveEnum eSpiSlave )
{
	BOOL fSuccess = FALSE;

	if( eSpiSlave < SPI_SLAVE_COUNT )
	{
		if( gtSpiBusLookupTable[gtSpiSlaveLookupTable[eSpiSlave].tSpiBus.eBusId].tStat.fIsBusRetained == FALSE )
		{
			gtSpiBusLookupTable[gtSpiSlaveLookupTable[eSpiSlave].tSpiBus.eBusId].tStat.fIsBusRetained = TRUE;

			gtSpiBusLookupTable[gtSpiSlaveLookupTable[eSpiSlave].tSpiBus.eBusId].tStat.eCurrentSlave = eSpiSlave;

			// configure spi
			SpiBusSetSlaveConfig( gtSpiSlaveLookupTable[eSpiSlave].tSpiBus.eBusId, eSpiSlave );

			// enable spi periph
			SPI_Cmd( gtSpiBusLookupTable[ gtSpiSlaveLookupTable[eSpiSlave].tSpiBus.eBusId ].tSPIx, ENABLE );

			fSuccess = TRUE;
		}
		else
		{
			// check if the bus is already retained by the same slave
			if( gtSpiBusLookupTable[gtSpiSlaveLookupTable[eSpiSlave].tSpiBus.eBusId].tStat.eCurrentSlave == eSpiSlave )
			{
				fSuccess = TRUE;
			}
		}
	}

	return fSuccess;
}

BOOL SpiSlaveReleaseBus( SpiSlaveEnum eSpiSlave )
{
	BOOL fSuccess = FALSE;

	if( eSpiSlave < SPI_SLAVE_COUNT )
	{
		if( gtSpiBusLookupTable[gtSpiSlaveLookupTable[eSpiSlave].tSpiBus.eBusId].tStat.fIsBusRetained == TRUE )
		{
			// make sure that only the slave that is holding the bus is releasing it
			if( gtSpiBusLookupTable[gtSpiSlaveLookupTable[eSpiSlave].tSpiBus.eBusId].tStat.eCurrentSlave == eSpiSlave )
			{
				gtSpiBusLookupTable[gtSpiSlaveLookupTable[eSpiSlave].tSpiBus.eBusId].tStat.fIsBusRetained = FALSE;

				gtSpiBusLookupTable[gtSpiSlaveLookupTable[eSpiSlave].tSpiBus.eBusId].tStat.eCurrentSlave = SPI_SLAVE_INVALID;

				// disable spi periph
				SPI_Cmd( gtSpiBusLookupTable[ gtSpiSlaveLookupTable[eSpiSlave].tSpiBus.eBusId ].tSPIx, DISABLE );

				fSuccess = TRUE;
			}
		}
		else if( gtSpiBusLookupTable[gtSpiSlaveLookupTable[eSpiSlave].tSpiBus.eBusId].tStat.fIsBusRetained == FALSE )
		{
			// if bus is not retained by anybody
			fSuccess = TRUE;
		}
	}

	return fSuccess;
}

BOOL SpiBusSetSlaveConfig( SpiBusEnum eSpiBus, SpiSlaveEnum eSpiSlave )
{
    BOOL 			fSuccess = FALSE;
	SPI_InitTypeDef SPI_InitStructure;
	FunctionalState NewState = DISABLE;

    if( eSpiBus < SPI_BUS_COUNT )
    {
        if( eSpiSlave < SPI_SLAVE_COUNT )
		{
			if( SpiIsBusEnabled( eSpiBus ) )
			{
				// #### RESET SPI VALS ####
				// Set init structure to default values
				SPI_StructInit( &SPI_InitStructure );
				// Deinitialize the SPIx peripheral registers to their default reset values.
				SPI_I2S_DeInit( gtSpiBusLookupTable[ eSpiBus ].tSPIx );

				// #### SPI COMMON CONFIGS ####
				SPI_InitStructure.SPI_Mode              = gtSpiSlaveLookupTable[eSpiSlave].tSpiBus.ptConf->wSPI_Mode;
				SPI_InitStructure.SPI_DataSize          = gtSpiSlaveLookupTable[eSpiSlave].tSpiBus.ptConf->wSPI_DataSize;
				SPI_InitStructure.SPI_CPOL              = gtSpiSlaveLookupTable[eSpiSlave].tSpiBus.ptConf->wSPI_CPOL;
				SPI_InitStructure.SPI_CPHA              = gtSpiSlaveLookupTable[eSpiSlave].tSpiBus.ptConf->wSPI_CPHA;
				SPI_InitStructure.SPI_BaudRatePrescaler = gtSpiSlaveLookupTable[eSpiSlave].tSpiBus.ptConf->wSPI_BaudRatePrescaler;
				SPI_InitStructure.SPI_FirstBit          = gtSpiSlaveLookupTable[eSpiSlave].tSpiBus.ptConf->wSPI_FirstBit;
				// All configurations are set to full Duplex.
				SPI_InitStructure.SPI_Direction         = SPI_Direction_2Lines_FullDuplex;
				SPI_InitStructure.SPI_NSS               = SPI_NSS_Soft;
				SPI_InitStructure.SPI_CRCPolynomial     = 1;

				// #### SET SPI REGISTERS ####
				SPI_Init( gtSpiBusLookupTable[ eSpiBus ].tSPIx, &SPI_InitStructure );

				fSuccess = TRUE;
			}
		}
    }

    return fSuccess;
}


BOOL SpiSlaveChipSelectAssert( SpiSlaveEnum eSpiSlave, BOOL fAssert )
{
    BOOL 			fSuccess = FALSE;
	SpiSlaveEnum 	eCurrentSlaveInBus;

	SpiGetSlaveHoldingBus( gtSpiSlaveLookupTable[eSpiSlave].tSpiBus.eBusId, &eCurrentSlaveInBus );

    if( eCurrentSlaveInBus ==  eSpiSlave )
    {
		if( fAssert )
		{
			GpioOutputRegWrite( gtSpiSlaveLookupTable[eSpiSlave].tGpioCs.tGpioPort, gtSpiSlaveLookupTable[eSpiSlave].tGpioCs.bGpioPin, GPIO_LOGIC_LOW );
		}
		else
		{
			GpioOutputRegWrite( gtSpiSlaveLookupTable[eSpiSlave].tGpioCs.tGpioPort, gtSpiSlaveLookupTable[eSpiSlave].tGpioCs.bGpioPin, GPIO_LOGIC_HIGH );
		}

		fSuccess = TRUE;
    }

    return fSuccess;
}

BOOL SpiSlaveIsChipSelectAsserted( SpiSlaveEnum eSpiSlave )
{
    BOOL            fIsChipSelectAsserted   = FALSE;
    GpioLogicEnum   eLogic                  = GPIO_LOGIC_LOW;

    if( eSpiSlave < SPI_SLAVE_COUNT )
    {
        GpioOutputRegRead( gtSpiSlaveLookupTable[eSpiSlave].tGpioCs.tGpioPort, gtSpiSlaveLookupTable[eSpiSlave].tGpioCs.bGpioPin, &eLogic );

        if( eLogic == GPIO_LOGIC_LOW )
        {
            fIsChipSelectAsserted = TRUE;
        }
        else
        {
            fIsChipSelectAsserted = FALSE;
        }
    }

    return fIsChipSelectAsserted;
}

BOOL SpiSlaveSendRecvByte( SpiSlaveEnum eSpiSlave, UINT8 bByteSend, UINT8 *pbByteRecvd )
{
	BOOL        	fSuccess    		= FALSE;
	SpiSlaveEnum 	eCurrentSlaveInBus;
	SpiBusEnum  	eSpiBus     = SPI_SLAVE_INVALID;
	TIMER       	tWaitTimer  = 0;
	UINT8           bByte;

    eSpiBus = gtSpiSlaveLookupTable[eSpiSlave].tSpiBus.eBusId;
	SpiGetSlaveHoldingBus( eSpiBus, &eCurrentSlaveInBus );

    if( eCurrentSlaveInBus == eSpiSlave )
    {
		// make sure the bus is not bussy now with trasmission operation
		// Wait while DR register is not empty
		tWaitTimer = TimerDownTimerStartMs( SPI_FLAG_TOGGLE_WAITING_TIME_MS );
		while( SPI_I2S_GetFlagStatus( gtSpiBusLookupTable[ eSpiBus ].tSPIx, SPI_I2S_FLAG_BSY ) == SET )
		{
			if( TimerDownTimerIsExpired( tWaitTimer ) == TRUE )
			{
				return FALSE;
			}
		}

		// Send UINT8 through the SPI peripheral
		gtSpiBusLookupTable[ eSpiBus ].tSPIx->DR = bByteSend;

		// Wait to receive a byte
		tWaitTimer = TimerDownTimerStartMs( SPI_FLAG_TOGGLE_WAITING_TIME_MS );
		while ( SPI_I2S_GetFlagStatus( gtSpiBusLookupTable[ eSpiBus ].tSPIx, SPI_I2S_FLAG_RXNE ) == RESET )
		{
			if( TimerDownTimerIsExpired( tWaitTimer ) == TRUE )
			{
				return FALSE;
			}
		}

		// Read the received byte
		bByte = gtSpiBusLookupTable[ eSpiBus ].tSPIx->DR;

		// Store byte if pointer is not NULL
		if( pbByteRecvd != NULL )
		{
			*pbByteRecvd = bByte;
		}

		fSuccess = TRUE;
	}

	return fSuccess;
}


BOOL SpiSlaveSendRecvByteArray( SpiSlaveEnum eSpiSlave, const UINT8 * const pbByteArraySend, UINT8 * const pbByteArrayRecv, UINT16 wInOutByteArraySize )
{
    BOOL        	fSuccess    = FALSE;
    SpiBusEnum  	eSpiBus     = SPI_SLAVE_INVALID;
	SpiSlaveEnum 	eCurrentSlaveInBus;
	UINT8 			bByteToSend;
    UINT8 			bByteReceived;
    TIMER       	tWaitTimer  = 0;

    eSpiBus = gtSpiSlaveLookupTable[eSpiSlave].tSpiBus.eBusId;
	SpiGetSlaveHoldingBus( eSpiBus, &eCurrentSlaveInBus );

    if( eCurrentSlaveInBus == eSpiSlave )
    {
        if( ((pbByteArraySend != NULL) || (pbByteArrayRecv != NULL)) && (wInOutByteArraySize > 0) )
        {
			// make sure the bus is not bussy now with trasmission operation
            // Wait while DR register is not empty
            tWaitTimer = TimerDownTimerStartMs( SPI_FLAG_TOGGLE_WAITING_TIME_MS );
            while( SPI_I2S_GetFlagStatus( gtSpiBusLookupTable[ eSpiBus ].tSPIx, SPI_I2S_FLAG_BSY ) == SET )
            {
                if( TimerDownTimerIsExpired( tWaitTimer ) == TRUE )
                {
                    return FALSE;
                }
            }

			// rx tx data
            for( UINT16 i = 0; i < wInOutByteArraySize; i++ )
            {
                bByteToSend 	= 0x00;
                bByteReceived 	= 0x00;

                // Load value into data register (register used to send and receive data)
                if (pbByteArraySend != NULL)
                {
                    bByteToSend = pbByteArraySend[i];
                }

                // Send byte - clock activated by hardware automatically.
                gtSpiBusLookupTable[ eSpiBus ].tSPIx->DR = bByteToSend;


                // Wait to receive a byte
                tWaitTimer = TimerDownTimerStartMs( SPI_FLAG_TOGGLE_WAITING_TIME_MS );
                while ( SPI_I2S_GetFlagStatus( gtSpiBusLookupTable[ eSpiBus ].tSPIx, SPI_I2S_FLAG_RXNE ) == RESET )
                {
                    if( TimerDownTimerIsExpired( tWaitTimer ) == TRUE )
                    {
                        return FALSE;
                    }
                }

                // Load the value from data register (value received) into the array
                bByteReceived = gtSpiBusLookupTable[ eSpiBus ].tSPIx->DR;

                if (pbByteArrayRecv != NULL)
                {
                    pbByteArrayRecv[i] = bByteReceived;
                }
            }

			fSuccess = TRUE;
        }
    }

    return fSuccess;
}

CHAR * SpiBusGetName( SpiBusEnum eSpiBus )
{
    if( eSpiBus < SPI_BUS_COUNT )
	{
		return (CHAR*)gtSpiBusLookupTable[ eSpiBus ].pcBusName;
	}
	else
	{
		return (CHAR*)"UNKNOWN";
	}
}

CHAR * SpiSlaveGetName( SpiSlaveEnum eSpiSlave )
{
    if( eSpiSlave < SPI_SLAVE_COUNT )
	{
		return (CHAR*)gtSpiSlaveLookupTable[eSpiSlave].pcSlaveName;
	}
	else
	{
		return (CHAR*)"UNKNOWN";
	}
}

///////////////////////////////////////// END OF SOURCE //////////////////////////////////////////
