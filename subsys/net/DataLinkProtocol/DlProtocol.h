//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! DATA LINK PROTOCOL
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef DL_PROTOCOL_H
#define DL_PROTOCOL_H

void    DlProtocolInitModule                ( void );

//////////////////////////////////////////////////////////////////////////////////////////////////
// NOTE: this function should run in different thread for better protocol performance
void    DlProtocolUpdate                    ( void );
//////////////////////////////////////////////////////////////////////////////////////////////////

// DEBUG
void    DlProtocolDbgEnable                 ( BOOL fEnable );
BOOL    DlProtocolIsDbgEnable               ( void );
void    DlProtocolDbgPortConfig             ( UsartPortsEnum eUsartPort );
UsartPortsEnum    DlProtocolGetDbgPort      ( void );

// PORT CONFIGURATION
void    DlProtocolPortConfig                ( UsartPortsEnum eUsartPort );
UsartPortsEnum    DlProtocolGetPortConfig   ( void );
void    DlProtocolUsePort                   ( BOOL fUsePort );
BOOL    DlProtocolIsUsingPort               ( void );

// CONN/DISCONN
BOOL    DlProtocolMasterListenForConnection ( BOOL fListen );
BOOL    DlProtocolMasterIsListening         ( void );
BOOL    DlProtocolMasterIsClientConnected   ( void );

BOOL    DlProtocolSlaveConnect              ( UINT16 wTimeOutMSec );
BOOL    DlProtocolSlaveDisconnect           ( void );
BOOL    DlProtocolSlaveIsConnecting         ( void );
BOOL    DlProtocolSlaveIsConnected          ( void );

// WRITE
// NOTE: DlProtocolGetBufferSubPacketCount() function checks for current buffer to be sent 
//      how many protocol iframes is going to subdivided to be sent.
//      This helps to adjust time if more frames are to be send for one buffer chunk.
//      (buffer gets split into max size frames and also the amount of protocol escape characters increase the size 
//      of data to be sent).
UINT16  DlProtocolGetBufferSubPacketCount   ( UINT8 *pbBuffer, UINT16 wBufferSize );
BOOL    DlProtocolIsPutBufferBusy           ( void );
BOOL    DlProtocolPutBuffer                 ( UINT8 *pbBuffer, UINT16 wBufferSize, UINT16 wTimeOutMSec );

// READ
BOOL    DlProtocolIsDataAvailable           ( void );
BOOL    DlProtocolReadData                  ( UINT8 *pbBuffer, UINT16 wBufferSize, UINT16 *pdwBytesRead );

///////////////////////////////////////// END OF HEADER //////////////////////////////////////////

#endif //DL_PROTOCOL_H