//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef BYTE_BIT_FIELD_H
#define BYTE_BIT_FIELD_H

BOOL    ByteBitFieldSetBitsAtOffset ( UINT8 *pbByteToModify, UINT8 bBitOffset, UINT8 bNumOfBitsToSet, UINT8 bBitsNewVal );
BOOL    ByteBitFieldGetBitsAtOffset ( UINT8 bOriginalByte, UINT8 bBitOffset, UINT8 bNumOfBitsToGet, UINT8 *pbBitsResult );

BOOL    ByteBitFieldSetBit          ( UINT8 *pbByteToModify, UINT8 bBitOffset, BOOL fSetBit );
BOOL    ByteBitFieldGetBit          ( UINT8 bOriginalByte, UINT8 bBitOffset, BOOL *pfIsBitSet );

BOOL    ByteToString                ( UINT8 bOriginalByte, UINT8 *pbBuffer, UINT8 bBufferSize );

///////////////////////////////////////// END OF HEADER //////////////////////////////////////////

#endif //BYTE_BIT_FIELD_H