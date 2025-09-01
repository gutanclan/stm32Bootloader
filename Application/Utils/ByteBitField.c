//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#include <string.h>

#include "Types.h"
#include "ByteBitField.h"

#define BYTE_NUM_OF_BITS        (8)

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ByteBitFieldSetBitsAtOffset( UINT8 *pbByteToModify, UINT8 bBitOffset, UINT8 bNumOfBitsToSet, UINT8 bBitsNewVal )
{
    BOOL fSuccess = FALSE;
    
    if( pbByteToModify != NULL )
    {
        if( bBitOffset < BYTE_NUM_OF_BITS )
        {
            // find max 
            if( bNumOfBitsToSet > (BYTE_NUM_OF_BITS - bBitOffset) )
            {
                bNumOfBitsToSet = (BYTE_NUM_OF_BITS - bBitOffset);
            }
            
            for( UINT8 x = 0 ; x < bNumOfBitsToSet ; x++ )
            {
                UINT8 bIsSet = ((bBitsNewVal >> x) & (1));
                
                ByteBitFieldSetBit( pbByteToModify, bBitOffset, bIsSet );
                
                bBitOffset++;
            }
            
            fSuccess = TRUE;
        }
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ByteBitFieldGetBitsAtOffset( UINT8 bOriginalByte, UINT8 bBitOffset, UINT8 bNumOfBitsToGet, UINT8 *pbBitsResult )
{
    BOOL    fSuccess    = FALSE;
    UINT8   bOffsetCopy = bBitOffset;
    UINT8   bMask       = 0;
        
    if
    (
        //( pbOriginalByte != NULL ) &&
        ( pbBitsResult != NULL )
    )
    {
        if( bBitOffset < BYTE_NUM_OF_BITS )
        {
            // find max 
            if( bNumOfBitsToGet > (BYTE_NUM_OF_BITS - bBitOffset) )
            {
                bNumOfBitsToGet = (BYTE_NUM_OF_BITS - bBitOffset);
            }
            
            for( UINT8 x = 0 ; x < bNumOfBitsToGet ; x++ )
            {
                bMask |= 1<< bOffsetCopy++;
            }
            
            //(*pbBitsResult) = (*pbOriginalByte) & bMask;   
            (*pbBitsResult) = bOriginalByte & bMask;   
            
            (*pbBitsResult) = (*pbBitsResult) >> bBitOffset;

            fSuccess = TRUE;
        }
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ByteBitFieldSetBit( UINT8 *pbByteToModify, UINT8 bBitOffset, BOOL fSetBit )
{
    BOOL fSuccess = FALSE;
    
    if( pbByteToModify != NULL )
    {
        if( bBitOffset < BYTE_NUM_OF_BITS )
        {
            if( fSetBit )
            {
                (*pbByteToModify) = ((*pbByteToModify) | (1 << bBitOffset));
            }
            else
            {
                (*pbByteToModify) = ((*pbByteToModify) & ~(1 << bBitOffset));
            }
            
            fSuccess = TRUE;
        }
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ByteBitFieldGetBit( UINT8 bOriginalByte, UINT8 bBitOffset, BOOL *pfIsBitSet )
{
    BOOL fSuccess = FALSE;
    
    if
    (
        // ( pbOriginalByte != NULL ) &&
        ( pfIsBitSet != NULL )
    )
    {
        if( bBitOffset < BYTE_NUM_OF_BITS )
        {
            if( ( ( bOriginalByte >> bBitOffset) & 0x01 ) > 0 )
            {
                (*pfIsBitSet) = TRUE;
            }
            else
            {
                (*pfIsBitSet) = FALSE;
            }
            
            fSuccess = TRUE;
        }
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ByteToString( UINT8 bOriginalByte, UINT8 *pbBuffer, UINT8 bBufferSize )
{
    BOOL fSuccess = FALSE;

    if
    ( 
        ( pbBuffer != NULL ) &&
        ( bBufferSize >= 9 )
    )
    {
        BOOL fIsBitSet;
        UINT8 bArrIdx = 0;
        pbBuffer[8] = 0;

        for( UINT8 x = BYTE_NUM_OF_BITS-1 ; x <= 7  ; x-- )
        {
            fIsBitSet = FALSE;
            ByteBitFieldGetBit( bOriginalByte, x, &fIsBitSet );
            
            if( fIsBitSet )
            {
                pbBuffer[bArrIdx] = '1';
            }
            else
            {
                pbBuffer[bArrIdx] = '0';
            }

            bArrIdx++;
        }

        fSuccess = TRUE;
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// EOF