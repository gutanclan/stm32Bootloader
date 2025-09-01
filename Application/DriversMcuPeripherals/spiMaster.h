//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SPI_H_
#define _SPI_H_

typedef enum
{
	SPI_BUS_FLASH_MEM  = 0,

	SPI_BUS_TOTAL,
}SpiBusEnum;

#define SPI_BUS_INVALID (SPI_BUS_TOTAL)

typedef enum
{
	SPI_SLAVE_FLASH_MEM_1 = 0,
    
	SPI_SLAVE_MAX
}SpiSlaveEnum;

#define SPI_SLAVE_INVALID (SPI_SLAVE_MAX)

BOOL	SpiModuleInit				( void );

BOOL	SpiBusEnable               	( SpiBusEnum eSpiBus, BOOL fEnable );
BOOL	SpiIsBusEnabled             ( SpiBusEnum eSpiBus );

BOOL	SpiIsAnySlaveHoldingBus   	( SpiBusEnum eSpiBus );
BOOL	SpiGetSlaveHoldingBus   	( SpiBusEnum eSpiBus, SpiSlaveEnum *peSpiSlave );

BOOL	SpiSlaveRetainBus          	( SpiSlaveEnum eSpiSlave );
BOOL	SpiSlaveReleaseBus          ( SpiSlaveEnum eSpiSlave );

BOOL 	SpiSlaveChipSelectAssert	( SpiSlaveEnum eSpiSlave, BOOL fAssert );

BOOL 	SpiSlaveSendRecvByte		( SpiSlaveEnum eSpiSlave, UINT8 bByteSend, UINT8 * const pbByteRecvd );
BOOL 	SpiSlaveSendRecvByteArray	( SpiSlaveEnum eSpiSlave, const UINT8 * const pbByteArraySend, UINT8 * const pbByteArrayRecv, UINT16 wInOutByteArraySize );

///////////////////////////////////////// END OF HEADER //////////////////////////////////////////

#endif 	//_SPI_H_

