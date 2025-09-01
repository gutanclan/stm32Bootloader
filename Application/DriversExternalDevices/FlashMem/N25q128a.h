//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!    \file        N25q128a.h
//!    \brief       Nan Flash Driver.
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef N25Q128A_H
#define N25Q128A_H

//////////////////////////////////////////////////////////////////////////////////////////////////

#define N25Q128A_NUM_SECTORS                (256)       /* 256 sectors of 64KBytes */
#define N25Q128A_NUM_SUBSECTORS             (4096)      /* 4096 subsectors of 4kBytes */
#define N25Q128A_NUM_PAGES                  (65536)     /* 65536 pages of 256 bytes */
#define N25Q128A_NUM_BYTES                  (16777216)  /* 128 MBits => 16MBytes */

#define N25Q128A_PAGE_SIZE_BYTES            (256)     /* 65536 pages of 256 bytes */
#define N25Q128A_SUBSECTOR_SIZE_BYTES       (4096)    /* 4096 subsectors of 4kBytes */
#define N25Q128A_SECTOR_SIZE_BYTES          (65536)   /* 256 sectors of 64KBytes */

//////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
    UINT8       bManufacturer;      //! Manufacturer ID
    UINT8       bMemoryType;        //! Memory type
    UINT8       bCapacity;          //! Total capacity of the chip, as specified in powers of two
    UINT8       bUIDLen;            //! Number of UINT8s used in the UID
    UINT8       bEDID[ 2 ];         //! Number of UINT8s used in the UID
    UINT8       bFactory[ 14 ];     //! Customized factory data
} N25q128aInfoStruct;

typedef enum
{
    N25Q128A_ARRAY_ID_1 = 0,

    N25Q128A_ARRAY_ID_MAX,
} N25q128aArrayIdEnum;

//////////////////////////////////////////////////////////////////////////////////////////////////

BOOL N25q128aModuleInit		    ( void );

BOOL N25q128aReadInfo		    ( N25q128aArrayIdEnum eArrId, N25q128aInfoStruct *ptDeviceInfo );

// WRITE 
BOOL N25q128aWriteAtPage        ( N25q128aArrayIdEnum eArrId, UINT32 dwPage, UINT8 *pbDataBuffer, UINT32 dwDataLength );

// READ
BOOL N25q128aReadAtByteAddress  ( N25q128aArrayIdEnum eArrId, UINT32 dwByteAddress, UINT32 dwBytesToRead, UINT8 *pbDataBuffer, UINT32 dwBufferSize );

// ERASE 
BOOL N25q128aEraseAll			( N25q128aArrayIdEnum eArrId );
BOOL N25q128aEraseSector		( N25q128aArrayIdEnum eArrId, UINT16 wSector );
BOOL N25q128aEraseSubsector		( N25q128aArrayIdEnum eArrId, UINT16 wSubsector );

// Load Level Driver Utils
BOOL N25q128aVerifyFlagStatReg  ( N25q128aArrayIdEnum eArrId, BOOL fWait, UINT8 *pbStatusRegResult );
BOOL N25q128aIsFlagStatRegError ( UINT8 bFlagStatusRegValue );

#endif /* N25Q128A_H */


//////////////////////////////////////////////////////////////////////////////////////////////////
