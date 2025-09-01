//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _BUFFER_UTILS_H_
#define _BUFFER_UTILS_H_

typedef struct
{
    UINT8   *pbBufferPrt;
    UINT32  dwBufferSize;
    UINT32  dwBytesWritten;
    UINT32  dwBytesAvailable;
}BufferUtilsBufferRxStruct;

BOOL    BufferUtilsReset        ( BufferUtilsBufferRxStruct *ptBufferDestination, UINT8 bBufferResetValue );
INT32   BufferUtilsAppendData   ( BufferUtilsBufferRxStruct *ptBufferDestination, UINT8 *pbBufferSource, UINT32 dwBufferSourceSize );

#endif // _BUFFER_UTILS_H_

///////////////////////////////////////// END OF HEADER //////////////////////////////////////////
