//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SERIAL_
#define _SERIAL_

#include "../DriversMcuPeripherals/usart.h"
#include "../DriversMcuPeripherals/gpio.h"

typedef enum
{
    SERIAL_PORT_MAIN,

    SERIAL_PORT_MAX,
}SerialPortEnum;

#define SERIAL_PORT_INVALID 					(SERIAL_PORT_MAX)

typedef struct
{
	UINT32 					dwBaudRate;
	UsartWordLengthEnum		eData;
	UsartParityEnum			eParity;
	UsartStopBitEnum		eStop;
}SerialPortConfigType;

BOOL 	SerialModuleInit				    	( void );

BOOL 	SerialPortEnable    					( SerialPortEnum ePort, BOOL fEnable );
BOOL 	SerialPortIsEnabled 					( SerialPortEnum ePort );

BOOL    SerialPortSetConfig         			( SerialPortEnum ePort, SerialPortConfigType *ptPortConfig );
BOOL    SerialPortGetConfig  					( SerialPortEnum ePort, SerialPortConfigType *ptPortConfig );

BOOL    SerialPutChar            				( SerialPortEnum ePort, CHAR cChar );
BOOL    SerialGetChar            				( SerialPortEnum ePort, CHAR *pcChar );
BOOL    SerialPutBuffer          				( SerialPortEnum ePort, const void * const pvBuffer, UINT16 wBufferSize );
BOOL    SerialGetBuffer          				( SerialPortEnum ePort, void *pvBuffer, UINT16 wBufferSize, UINT16 *pwBytesReceived );

void 	SerialPortIsClientConnectedUpdate 		( void );
BOOL 	SerialPortIsClientConnected 		    ( SerialPortEnum ePort );

CHAR *  SerialPortGetName        				( SerialPortEnum ePort );

BOOL    SerialGetUsart        				    ( SerialPortEnum ePort, UsartPortEnum *peUsart );
BOOL    SerialGetGpioRx        				    ( SerialPortEnum ePort, GpioPortEnum *peGpioPort, UINT8 *pbGpioPin );
BOOL    SerialGetGpioTx        				    ( SerialPortEnum ePort, GpioPortEnum *peGpioPort, UINT8 *pbGpioPin );

void    SerialTest                              ( void );

///////////////////////////////////////// END OF HEADER //////////////////////////////////////////

#endif


