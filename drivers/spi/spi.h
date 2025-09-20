//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SPI_H_
#define _SPI_H_

typedef enum
{
	SPI_BUS_1  = 0,

	SPI_BUS_TOTAL,
}SpiBusEnum;

#define SPI_BUS_INVALID (SPI_BUS_TOTAL)

typedef enum
{
	SPI_SLAVE_FLASH_MEM_1 = 0,

	SPI_SLAVE_TOTAL
}SpiSlaveEnum;

#define SPI_SLAVE_INVALID (SPI_SLAVE_TOTAL)

BOOL	SpiModuleInit				( void );

BOOL	SpiBusEnable               	( SpiBusEnum eSpiBus, BOOL fEnable );
BOOL	SpiIsBusEnabled             ( SpiBusEnum eSpiBus );

BOOL	SpiIsAnySlaveHoldingBus   	( SpiBusEnum eSpiBus );
BOOL	SpiGetSlaveHoldingBus   	( SpiBusEnum eSpiBus, SpiSlaveEnum *peSpiSlave );

BOOL	SpiSlaveClaimBus          	( SpiSlaveEnum eSpiSlave );
BOOL	SpiSlaveReleaseBus          ( SpiSlaveEnum eSpiSlave );

BOOL 	SpiSlaveChipSelectAssert	( SpiSlaveEnum eSpiSlave, BOOL fAssert );
BOOL 	SpiSlaveIsChipSelectAsserted( SpiSlaveEnum eSpiSlave );

BOOL 	SpiSlaveSendRecvByte		( SpiSlaveEnum eSpiSlave, UINT8 bByteSend, UINT8 * const pbByteRecvd );
BOOL 	SpiSlaveSendRecvByteArray	( SpiSlaveEnum eSpiSlave, const UINT8 * const pbByteArraySend, UINT8 * const pbByteArrayRecv, UINT16 wInOutByteArraySize );

CHAR *  SpiBusGetName               ( SpiBusEnum eSpiBus );
CHAR *  SpiSlaveGetName             ( SpiSlaveEnum eSpiSlave );

///////////////////////////////////////// END OF HEADER //////////////////////////////////////////

#endif 	//_SPI_H_

