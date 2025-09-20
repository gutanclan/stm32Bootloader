//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! DATA LINK PROTOCOL
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef DL_PROTOCOL_FRAME_H
#define DL_PROTOCOL_FRAME_H

#include "Types.h"

typedef struct
{
    UINT8  *pbArray;
    UINT16  wArrMax;
    UINT16  wDataLen;
}DlProtocolFrameFieldData;

typedef struct
{
    UINT8                       bAddress;
    UINT8                       bControl;
    DlProtocolFrameFieldData    tData;
    UINT16                      wCrc16;
}DlProtocolFrame;

#define DL_PROTOC_QUEUE_RX_NUM_OF_S_U_FRAME_ELEMENTS        (15)

// frame bytes: sot-Flag(1) addr(1) cntrl(1) I-FieldData(??) crc16(2) eot-Flag(1)
// sot-Flag = eot-Flag
#define DL_PROTOC_BUFFER_TX_FRAME_MAX_BYTE_SIZE             (512)
//#define DL_PROTOC_BUFFER_TX_FRAME_MAX_BYTE_SIZE             (256)
// packet size is worst case scenario = i-frame. 
// this i-frame is including all headers except SOT and/or EOT flag bytes
#define DL_PROTOC_BUFFER_TX_FRAME_NO_SOT_MAX_BYTE_SIZE      (DL_PROTOC_BUFFER_TX_FRAME_MAX_BYTE_SIZE-2)
#define DL_PROTOC_BUFFER_RX_FRAME_NO_SOT_MAX_BYTE_SIZE      (DL_PROTOC_BUFFER_TX_FRAME_NO_SOT_MAX_BYTE_SIZE)
// bellow only DATA field size in an i-frame 
// no room for overhead: addr(1) cntrl(1) crc16(2). Only data field
#define DL_PROTOC_BUFFER_TX_I_FRAME_FIELD_DATA_BYTE_SIZE    (DL_PROTOC_BUFFER_TX_FRAME_NO_SOT_MAX_BYTE_SIZE-1-1-2)

// number of raw data bytes (this is after processing packets and extracting data from frame).
#define DL_PROTOC_BUFFER_RX_RAW_DATA_BYTE_SIZE              (DL_PROTOC_BUFFER_TX_I_FRAME_FIELD_DATA_BYTE_SIZE*3) 


///////////////////////////////////////// END OF HEADER //////////////////////////////////////////

#endif //DL_PROTOCOL_FRAME_H