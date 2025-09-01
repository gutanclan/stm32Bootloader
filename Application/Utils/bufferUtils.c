//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
// Header include
//////////////////////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include "./Types.h"
#include "bufferUtils.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
INT32 BufferUtilsAppendData( BufferUtilsBufferRxStruct *ptBufferDestination, UINT8 *pbBufferSource, UINT32 dwBufferSourceSize )
{
    INT32   idwBytesWritten = 0;
    
	if
    (
        ( ptBufferDestination != NULL ) &&
        ( pbBufferSource != NULL )
    )
	{
        if
        ( 
            ( ptBufferDestination->pbBufferPrt != NULL ) &&
            ( ptBufferDestination->dwBufferSize > 0 ) &&
            ( ptBufferDestination->dwBytesAvailable > 0 )
        )
        {
            if( ptBufferDestination->dwBytesAvailable >= dwBufferSourceSize )
            {
                idwBytesWritten = dwBufferSourceSize;
            }
            else
            {
                idwBytesWritten = ptBufferDestination->dwBytesAvailable;
            }

            memcpy( &ptBufferDestination->pbBufferPrt[ptBufferDestination->dwBytesWritten], pbBufferSource, idwBytesWritten );

            ptBufferDestination->dwBytesWritten    += idwBytesWritten;
            ptBufferDestination->dwBytesAvailable  -= idwBytesWritten;
        }		
	}

	return idwBytesWritten;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BufferUtilsReset( BufferUtilsBufferRxStruct *ptBufferDestination, UINT8 bBufferResetValue )
{
    BOOL fSuccess = FALSE;
    
    if
    (
        ( ptBufferDestination != NULL )
    )
	{
        if
        ( 
            ( ptBufferDestination->pbBufferPrt != NULL ) &&
            ( ptBufferDestination->dwBufferSize > 0 )
        )
        {
            memset( &ptBufferDestination->pbBufferPrt[0], bBufferResetValue, ptBufferDestination->dwBufferSize );
            
            ptBufferDestination->dwBytesWritten     = 0;
            ptBufferDestination->dwBytesAvailable   = ptBufferDestination->dwBufferSize;
            
            fSuccess = TRUE;
        }
    }
    
    return fSuccess;
}

/////////////////////////////////////////// END OF SOURCE //////////////////////////////////////////

