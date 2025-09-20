//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!    \file        N25q128a.h
//!    \brief       Nan Flash Driver.
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#include <string.h>

#include "types.h"

#include "N25q128a.h"

#include "./Utils/semaphore.h"
#include "Spi.h"

//////////////////////////////////////////////////////////////////////////////////////////////////

// Flash Timing  -----------------------------------------------------------------------------

//! Holdoff period from one SPI command to another
#define N25Q_SPI_COMMAND_HOLDOFF        ( 1 / portTICK_RATE_MS )

//! Holdoff period when erasing
#define N25Q_ERASE_HOLDOFF              ( 5 / portTICK_RATE_MS )

#define N25Q_DUMMY_BYTE				    (0x00)

// N25Q128A11BF804E Instruction Set ----------------------------------------------------------

//! RDID (read identification) instruction UINT8
#define N25Q_COMMAND_RDID				(0x9e)
#define N25Q_LENGTH_COMMAND_RDID		(1)
#define N25Q_LENGTH_RESPONSE_RDID		(20)

//! READ (read data UINT8s) instruction UINT8
#define N25Q_COMMAND_READ				(0x03)
#define N25Q_LENGTH_COMMAND_READ		(4)

//! FAST_READ (read data UINT8s at higher speed) instruction UINT8
#define N25Q_COMMAND_FAST_READ			(0x0b)
#define N25Q_LENGTH_COMMAND_FAST_READ	(5)

//! DOFR (dual output fast read) instruction UINT8
#define N25Q_COMMAND_DOFR				(0x3b)

//! DIOFR (dual input/output fast read) instruction UINT8
#define N25Q_COMMAND_DIOFR				(0xbb)

//! QOFR (quad output fast read) instruction UINT8
#define N25Q_COMMAND_QOFR				(0x6b)

//! QIOFR (quad input/output fast read) instruction UINT8
#define N25Q_COMMAND_QIOFR				(0xeb)

//! ROTP (read OTP) instruction UINT8
#define N25Q_COMMAND_ROTP				(0x4b)

//! WREN (write enable) instruction UINT8
#define N25Q_COMMAND_WREN				(0x06)

//! WRDI (write disable) instruction UINT8
#define N25Q_COMMAND_WRDI				(0x04)

//! PP (page program) instruction UINT8
#define N25Q_COMMAND_PP					(0x02)
#define N25Q_LENGTH_COMMAND_PP			(4)

//! DIFP (dual input fast program) instruction UINT8
#define N25Q_COMMAND_DIFP				(0xa2)

//! DIEFP (dual input extended fast program) instruction UINT8
#define N25Q_COMMAND_DIEFP				(0xd2)

//! QIFP (quad input fast program) instruction UINT8
#define N25Q_COMMAND_QIFP				(0x32)

//! QIEFP (quad input extended fast program) instruction UINT8
#define N25Q_COMMAND_QIEFP				(0x12)

//! POTP (program OTP) instruction UINT8
#define N25Q_COMMAND_POTP				(0x42)

//! SSE (subsector erase) instruction UINT8
#define N25Q_COMMAND_SSE				(0x20)
#define N25Q_LENGTH_COMMAND_SSE			(4)

//! SE (sector erase) instruction UINT8
#define N25Q_COMMAND_SE					(0xd8)
#define N25Q_LENGTH_COMMAND_SE			(4)

//! BE (bulk erase) instruction UINT8
#define N25Q_COMMAND_BE					(0xc7)

//! PER (program/erase resume) instruction UINT8
#define N25Q_COMMAND_PER				(0x7a)

//! PES (program/erase suspend) instruction UINT8
#define N25Q_COMMAND_PES				(0x75)

//! RDSR (read status register) instruction UINT8
#define N25Q_COMMAND_RDSR				(0x05)

//! WRSR (write status register) instruction UINT8
#define N25Q_COMMAND_WRSR				(0x01)

//! RDLR (read lock register) instruction UINT8
#define N25Q_COMMAND_RDLR				(0xe8)

//! WRLR (write lock register) instruction UINT8
#define N25Q_COMMAND_WRLR				(0xe5)

//! RFSR (read flag status register) instruction UINT8
#define N25Q_COMMAND_RFSR				(0x70)

//! WFSR (write flag status register) instruction UINT8
#define N25Q_COMMAND_CLFSR				(0x50)

//! RDNVCR (read NV config register) instruction UINT8
#define N25Q_COMMAND_RDNVCR				(0xb5)

//! WRNVCR (write NV config register) instruction UINT8
#define N25Q_COMMAND_WRNVCR				(0xb1)

//! RDVCR  (read volatile configuration register) instruction UINT8
#define N25Q_COMMAND_RDVCR				(0x85)

//! WRVCR (write volatile configuration register) instruction UINT8
#define N25Q_COMMAND_WRVCR				(0x81)

//! Byte mask for the upper byte of 3 byte address.
#define N25Q_BYTEMASK_ADDRESS_UPPER		(0xFF0000)

//! Byte mask for the middle byte of 3 byte address.
#define N25Q_BYTEMASK_ADDRESS_MID		(0x00FF00)
	
//! Byte mask for the lower byte of 3 byte address.
#define N25Q_BYTEMASK_ADDRESS_LOWER		(0x0000FF)


//! RDSR Device Status Register Bits

#define RDSR_BITMASK_WIP					(0x01) //write in progress	0 = ready, 1 = busy
#define RDSR_BITMASK_WEL					(0x02) //write enable latch	0 = clear, 1 = set
#define RDSR_BITMASK_BP0					(0x04) //block protect bit 0
#define RDSR_BITMASK_BP1					(0x08) //block protect bit 1
#define RDSR_BITMASK_BP2					(0x10) //block protect bit 2
#define RDSR_BITMASK_TP						(0x20) //top/bottom			0 = top, 1 = bottom
#define RDSR_BITMASK_BP3					(0x40) //block protect bit 3
#define RDSR_BITMASK_WE						(0x80) //status register write enable/disable 0 = enabled, 1 = disabled

//! RFSR Flag ice Status Register Bits

#define RFSR_BITMASK_RESERVED				(0x01) //bit not used
#define RFSR_BITMASK_PROTECTION				(0x02) //protection error bit								0 = clear, 1 = failure or protection error
#define RFSR_BITMASK_PS						(0x04) //program suspend status bit							0 = not in effect, 1 = in effect.
#define RFSR_BITMASK_VPP					(0x08) //Vpp error bit										0 = enabled, 1 = disabled
#define RFSR_BITMASK_PROGRAM				(0x10) //program error bit									0 = clear, 1 = failure or protection error
#define RFSR_BITMASK_ERASE					(0x20) //erase command error bit							0 = clear, 1 = failure or proteciton error
#define RFSR_BITMASK_ES						(0x40) //erase suspend status bit							0 = not in effect, 1 = in effect
#define RFSR_BITMASK_PEC					(0x80) //program/erase/write command in progress status bit	0 = busy, 1 = ready

//! RDID Manufacturer Device ID Information
#define RDID_N25Q_MANUFACTURER_ID			(0x20) //manufacturer jedec id for n25q device.
#define RDID_N25Q_DEVICE_ID_MEMTYPE_M		(0xBA) //manufacturer memory type id (MICRON)
#define RDID_N25Q_DEVICE_ID_MEMTYPE_N		(0xBB) //manufacturer memory type id (NUMONYX)
#define RDID_N25Q_DEVICE_ID_MEMCAPACITY		(0x18) // manufacturer memory capacity id (128Mbit)

//////////////////////////////////////////////////////////////////////////////////////////////////

//typedef UINT8 *         SEMAPHORE_KEY_HOLDER;
//static volatile BOOL    gfSemaphoreIsOperationLocked    = FALSE;
//static const UINT8      gbSemaphoreOperationLockKeyVal  = 0xAF;

typedef struct
{
    SpiSlaveEnum    eSpiSlaveId;
    SemaphoreEnum   eSemaphoreId;
}N25q128aStruct;

static N25q128aStruct tN25q128aArray[] = 
{
    // N25Q128A_ARRAY_ID_1
    // eSpiSlaveId          eSemaphoreId
    { SPI_SLAVE_DATAFLASH,  SEMAPHORE_N25Q128A_1 },
};

#define N25Q128A_ARRAY_MAX     ( sizeof(tN25q128aArray) / sizeof( tN25q128aArray[0] ) )

//////////////////////////////////////////////////////////////////////////////////////////////////

static BOOL N25q128aSpiBusOpen          ( UINT8 bSpiSlave, BOOL fOpen );
static BOOL N25q128aSpiCsAssert         ( UINT8 bSpiSlave, BOOL fAssert );
static BOOL N25q128aSpiSendRecvByteArray( UINT8 bSpiSlave, const UINT8 * const pbByteArraySend, UINT8 * const pbByteArrayRecv, UINT16 wInOutByteArraySize );
static BOOL N25q128aSpiSendRecvByte     ( UINT8 bSpiSlave, UINT8 bByteSend, UINT8 * const pbByteRecvd );

//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL N25q128aModuleInit( void )
{
    if( N25Q128A_ARRAY_ID_MAX != N25Q128A_ARRAY_MAX )
    {
        // catch this bug during development time
        while(1)
        {
        }
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL N25q128aSpiBusOpen( UINT8 bSpiSlave, BOOL fOpen )
{
    BOOL fSuccess = FALSE;
    
    ////////////////////////////////////////
    // add implementation here
    ////////////////////////////////////////
    if( fOpen )
    {
        fSuccess = SpiSlaveBusOpen( bSpiSlave, 0 );
    }
    else
    {
        fSuccess = SpiSlaveBusClose( bSpiSlave );
    }
    ////////////////////////////////////////
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL N25q128aSpiCsAssert( UINT8 bSpiSlave, BOOL fAssert )
{
    BOOL fSuccess = FALSE;
    
    ////////////////////////////////////////
    // add implementation here
    ////////////////////////////////////////
    fSuccess = SpiSlaveChipSelectAssert( bSpiSlave, fAssert );
    ////////////////////////////////////////
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL N25q128aSpiSendRecvByte( UINT8 bSpiSlave, UINT8 bByteSend, UINT8 * const pbByteRecvd )
{
    BOOL fSuccess = FALSE;
    
    ////////////////////////////////////////
    // add implementation here
    ////////////////////////////////////////
    fSuccess = SpiSlaveSendRecvByte( bSpiSlave, bByteSend, pbByteRecvd );
    ////////////////////////////////////////
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL N25q128aSpiSendRecvByteArray( UINT8 bSpiSlave, const UINT8 * const pbByteArraySend, UINT8 * const pbByteArrayRecv, UINT16 wInOutByteArraySize )
{
    BOOL fSuccess = FALSE;
    
    ////////////////////////////////////////
    // add implementation here
    ////////////////////////////////////////
    fSuccess = SpiSlaveSendRecvByteArray( bSpiSlave, pbByteArraySend, pbByteArrayRecv, wInOutByteArraySize );
    ////////////////////////////////////////
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL N25q128aVerifyFlagStatReg( N25q128aArrayIdEnum eArrId, BOOL fWait, UINT8 *pbStatusByteResult )
{
    BOOL                    fSuccess = FALSE;
    UINT8				    bStatus;
    
    if( eArrId < N25Q128A_ARRAY_MAX )
    {        
        // get status reg
        if( N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, TRUE ) )
        {
            // ***********************
            // SPI operations
            // ***********************
            N25q128aSpiSendRecvByte( tN25q128aArray[eArrId].eSpiSlaveId, N25Q_COMMAND_RFSR, &bStatus );
            do
            {
                N25q128aSpiSendRecvByte( tN25q128aArray[eArrId].eSpiSlaveId, N25Q_DUMMY_BYTE, &bStatus );
                if (!fWait)
                {
                    break;
                }
            } while(!(bStatus & RFSR_BITMASK_PEC));
            
            // ***********************
            N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, FALSE );
        }
        
        // after getting status, clear reg
        if( N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, TRUE ) )
        {
            // ***********************
            // SPI operations
            // ***********************
            N25q128aSpiSendRecvByte( tN25q128aArray[eArrId].eSpiSlaveId, N25Q_COMMAND_CLFSR, NULL );
            // ***********************
            N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, FALSE );
        }
        
        if( pbStatusByteResult != NULL )
        {
            (*pbStatusByteResult) = bStatus;
        }
        
        // set result OK
        fSuccess = TRUE;
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL N25q128aIsFlagStatRegError( UINT8 bFlagStatusRegValue )
{
    BOOL fIsError = FALSE;
    
    if( (bFlagStatusRegValue & (RFSR_BITMASK_ERASE|RFSR_BITMASK_PROGRAM|RFSR_BITMASK_VPP|RFSR_BITMASK_PROTECTION)) )
    {
        fIsError = TRUE;
    }
    
    return fIsError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL N25q128aReadInfo( N25q128aArrayIdEnum eArrId, N25q128aInfoStruct *ptDeviceInfo )
{
    BOOL                    fSuccess = FALSE;
    SEMAPHORE_KEY_HOLDER    xSemaphoreKeyHolder = SEMAPHORE_RESULT_UNLOCKED;
    
    UINT8					bData[20];
    
    if( eArrId < N25Q128A_ARRAY_MAX )
    {
        if( ptDeviceInfo != NULL )
        {
            ////////////////////////////////////////////
            xSemaphoreKeyHolder = SemaphoreLock( tN25q128aArray[eArrId].eSemaphoreId );
            if( xSemaphoreKeyHolder != SEMAPHORE_RESULT_UNLOCKED )
            {
                if( N25q128aSpiBusOpen( tN25q128aArray[eArrId].eSpiSlaveId, TRUE ) )
                {
                    if( N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, TRUE ) )
                    {
                        // ***********************
                        // SPI operations
                        // ***********************
                        N25q128aSpiSendRecvByte( tN25q128aArray[eArrId].eSpiSlaveId, N25Q_COMMAND_RDID, NULL );
                        N25q128aSpiSendRecvByteArray( tN25q128aArray[eArrId].eSpiSlaveId, NULL, &bData[0], sizeof(bData) );
                        // ***********************
                        N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, FALSE );
                        
                        // extract data 
                        ptDeviceInfo->bManufacturer  = bData[0];
                        ptDeviceInfo->bMemoryType    = bData[1];
                        ptDeviceInfo->bCapacity      = bData[2];
                        ptDeviceInfo->bUIDLen        = bData[3];
                        ptDeviceInfo->bEDID[0]       = bData[4];
                        ptDeviceInfo->bEDID[1]       = bData[5];
                        memcpy( ptDeviceInfo->bFactory, &bData[6], sizeof(ptDeviceInfo->bFactory) );
                        
                        // set result OK
                        fSuccess = TRUE;
                    }
                    
                    N25q128aSpiBusOpen( tN25q128aArray[eArrId].eSpiSlaveId, FALSE );
                }
                
                SemaphoreUnlock( tN25q128aArray[eArrId].eSemaphoreId, xSemaphoreKeyHolder );
            }
            ////////////////////////////////////////////
        }
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL N25q128aWriteAtPage( N25q128aArrayIdEnum eArrId, UINT32 dwPage, UINT8 *pbDataBuffer, UINT32 dwDataLength )
{
    BOOL                    fSuccess = FALSE;
    SEMAPHORE_KEY_HOLDER    xSemaphoreKeyHolder = SEMAPHORE_RESULT_UNLOCKED;
    UINT8                   bStatus;
    
    if( eArrId < N25Q128A_ARRAY_MAX )
    {
        if
        (
            ( dwPage < N25Q128A_NUM_PAGES ) &&
            ( pbDataBuffer != NULL ) &&
            ( dwDataLength > 0 ) &&
            ( dwDataLength <= N25Q128A_PAGE_SIZE_BYTES )
        )
        {
            ////////////////////////////////////////////
            xSemaphoreKeyHolder = SemaphoreLock( tN25q128aArray[eArrId].eSemaphoreId );
            if( xSemaphoreKeyHolder != SEMAPHORE_RESULT_UNLOCKED )
            {
                if( N25q128aSpiBusOpen( tN25q128aArray[eArrId].eSpiSlaveId, TRUE ) )
                {
                    // ***********************
                    // SPI operations
                    // ***********************
                    
                    // Flash: Enable Write operation 
                    if( N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, TRUE ) )
                    {
                        N25q128aSpiSendRecvByte( tN25q128aArray[eArrId].eSpiSlaveId, N25Q_COMMAND_WREN, NULL );
                        N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, FALSE );
                    }
                    
                    // Flash: Verify that the write enable bit has been set                        
                    if( N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, TRUE ) )
                    {
                        N25q128aSpiSendRecvByte( tN25q128aArray[eArrId].eSpiSlaveId, N25Q_COMMAND_RDSR, NULL );
                        N25q128aSpiSendRecvByte( tN25q128aArray[eArrId].eSpiSlaveId, N25Q_DUMMY_BYTE, &bStatus );
                        N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, FALSE );
                        
                        if
                        (
                            // not writing something?                            
                            ( (bStatus & RDSR_BITMASK_WIP) == 0 ) &&
                            // currently in write mode?
                            ( (bStatus & RDSR_BITMASK_WEL) != 0 )
                        )
                        {
                            fSuccess = TRUE;
                        }
                    }
                            
                    if( fSuccess )
                    {
                        fSuccess = N25q128aVerifyFlagStatReg( tN25q128aArray[eArrId].eSpiSlaveId, FALSE, &bStatus );
                        if( fSuccess )
                        {
                            fSuccess = FALSE;
                            if( N25q128aIsFlagStatRegError( bStatus ) == FALSE )
                            {
                                fSuccess = TRUE;
                            }
                        }
                    }
                    if( fSuccess )
                    {
                        fSuccess = FALSE;
                        
                        // Begin the page write operation, send page program command
                        if( N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, TRUE ) )
                        {
                            UINT8 bCommandArray[N25Q_LENGTH_COMMAND_PP];

                            bCommandArray[0] = N25Q_COMMAND_PP;
                            bCommandArray[1] = ( ( dwPage & N25Q_BYTEMASK_ADDRESS_UPPER ) >> 16 );
                            bCommandArray[2] = ( ( dwPage & N25Q_BYTEMASK_ADDRESS_MID )   >> 8 );
                            bCommandArray[3] = ( ( dwPage & N25Q_BYTEMASK_ADDRESS_LOWER ) >> 0 );
        
                            // Send the command and 3-byte address
                            N25q128aSpiSendRecvByteArray( tN25q128aArray[eArrId].eSpiSlaveId, &bCommandArray[0], NULL, N25Q_LENGTH_COMMAND_PP );
                            
                            // Write data
                            N25q128aSpiSendRecvByteArray( tN25q128aArray[eArrId].eSpiSlaveId, &pbDataBuffer[0], NULL, dwDataLength );
                            
                            N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, FALSE );
                        }
                        
                        // Wait for the operation to complete
                        if( N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, TRUE ) )
                        {
                            N25q128aSpiSendRecvByte( tN25q128aArray[eArrId].eSpiSlaveId, N25Q_COMMAND_RDSR, NULL );
                            
                            do
                            {
                                N25q128aSpiSendRecvByte( tN25q128aArray[eArrId].eSpiSlaveId, N25Q_DUMMY_BYTE, &bStatus );
                            }while( (bStatus & RDSR_BITMASK_WIP) );
                            
                            N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, FALSE );
                        }
                        
                        fSuccess = N25q128aVerifyFlagStatReg( tN25q128aArray[eArrId].eSpiSlaveId, TRUE, NULL );
                        if( fSuccess )
                        {
                            fSuccess = FALSE;
                            if( N25q128aIsFlagStatRegError( bStatus ) == FALSE )
                            {
                                fSuccess = TRUE;
                            }
                        }
                    }
                    
                    // ***********************
                    
                    N25q128aSpiBusOpen( tN25q128aArray[eArrId].eSpiSlaveId, FALSE );
                }
                
                SemaphoreUnlock( tN25q128aArray[eArrId].eSemaphoreId, xSemaphoreKeyHolder );
            }
            ////////////////////////////////////////////
        }
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL N25q128aReadAtByteAddress( N25q128aArrayIdEnum eArrId, UINT32 dwByteAddress, UINT32 dwBytesToRead, UINT8 *pbDataBuffer, UINT32 dwBufferSize )
{
    BOOL                    fSuccess = FALSE;
    SEMAPHORE_KEY_HOLDER    xSemaphoreKeyHolder = SEMAPHORE_RESULT_UNLOCKED;
    UINT32                  dwBytesToStoreInBuffer;
    
    if( eArrId < N25Q128A_ARRAY_MAX )
    {
        if
        (
            ( dwByteAddress < N25Q128A_NUM_BYTES ) &&
            ( pbDataBuffer != NULL ) &&
            ( dwBufferSize > 0 ) &&
            ( dwBytesToRead > 0 ) &&
            // byte offset + data is inside writeable range
            ( ( (dwByteAddress-1) + dwBytesToRead ) < N25Q128A_NUM_BYTES )
        )
        {
            ////////////////////////////////////////////
            xSemaphoreKeyHolder = SemaphoreLock( tN25q128aArray[eArrId].eSemaphoreId );
            if( xSemaphoreKeyHolder != SEMAPHORE_RESULT_UNLOCKED )
            {
                if( N25q128aSpiBusOpen( tN25q128aArray[eArrId].eSpiSlaveId, TRUE ) )
                {
                    // ***********************
                    // SPI operations
                    // ***********************
                    
                    // Flash: Send the command and 3-byte address and read data
                    if( N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, TRUE ) )
                    {
                        UINT8 bCommandArray[N25Q_LENGTH_COMMAND_FAST_READ];

                        bCommandArray[0] = N25Q_COMMAND_FAST_READ;
                        bCommandArray[1] = ( ( dwByteAddress & N25Q_BYTEMASK_ADDRESS_UPPER )	>> 16 );
                        bCommandArray[2] = ( ( dwByteAddress & N25Q_BYTEMASK_ADDRESS_MID )	>> 8 );
                        bCommandArray[3] = ( ( dwByteAddress & N25Q_BYTEMASK_ADDRESS_LOWER )	>> 0 );
                        bCommandArray[4] = N25Q_DUMMY_BYTE;
        
                        // Send the command and 3-byte address
                        N25q128aSpiSendRecvByteArray( tN25q128aArray[eArrId].eSpiSlaveId, &bCommandArray[0], NULL, N25Q_LENGTH_COMMAND_FAST_READ );
                        
                        if( dwBufferSize < dwBytesToRead )
                        {
                            dwBytesToStoreInBuffer = dwBufferSize;
                        }
                        else
                        {
                            dwBytesToStoreInBuffer = dwBytesToRead;
                        }
                        // Read data
                        N25q128aSpiSendRecvByteArray( tN25q128aArray[eArrId].eSpiSlaveId, NULL, pbDataBuffer, dwBytesToStoreInBuffer );
                        
                        N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, FALSE );
                    }
                    
                    fSuccess = TRUE;
                    
                    // ***********************
                    
                    N25q128aSpiBusOpen( tN25q128aArray[eArrId].eSpiSlaveId, FALSE );
                }
                
                SemaphoreUnlock( tN25q128aArray[eArrId].eSemaphoreId, xSemaphoreKeyHolder );
            }
            ////////////////////////////////////////////
        }
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL N25q128aEraseAll( N25q128aArrayIdEnum eArrId )
{
    BOOL                    fSuccess = FALSE;
    SEMAPHORE_KEY_HOLDER    xSemaphoreKeyHolder = SEMAPHORE_RESULT_UNLOCKED;
    UINT8                   bStatus;
    
    if( eArrId < N25Q128A_ARRAY_MAX )
    {
        ////////////////////////////////////////////
        xSemaphoreKeyHolder = SemaphoreLock( tN25q128aArray[eArrId].eSemaphoreId );
        if( xSemaphoreKeyHolder != SEMAPHORE_RESULT_UNLOCKED )
        {
            if( N25q128aSpiBusOpen( tN25q128aArray[eArrId].eSpiSlaveId, TRUE ) )
            {
                // ***********************
                // SPI operations
                // ***********************
                
                // Flash: Enable Write operation 
                if( N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, TRUE ) )
                {
                    N25q128aSpiSendRecvByte( tN25q128aArray[eArrId].eSpiSlaveId, N25Q_COMMAND_WREN, NULL );
                    N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, FALSE );
                }
                
                // Flash: Verify that the write enable bit has been set                        
                if( N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, TRUE ) )
                {
                    N25q128aSpiSendRecvByte( tN25q128aArray[eArrId].eSpiSlaveId, N25Q_COMMAND_RDSR, NULL );
                    N25q128aSpiSendRecvByte( tN25q128aArray[eArrId].eSpiSlaveId, N25Q_DUMMY_BYTE, &bStatus );
                    N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, FALSE );
                    
                    if
                    (
                        // not writing something?                            
                        ( (bStatus & RDSR_BITMASK_WIP) == 0 ) &&
                        // currently in write mode?
                        ( (bStatus & RDSR_BITMASK_WEL) != 0 )
                    )
                    {
                        fSuccess = TRUE;
                    }
                }
                        
                if( fSuccess )
                {
                    fSuccess = N25q128aVerifyFlagStatReg( tN25q128aArray[eArrId].eSpiSlaveId, FALSE, &bStatus );
                    if( fSuccess )
                    {
                        fSuccess = FALSE;
                        if( N25q128aIsFlagStatRegError( bStatus ) == FALSE )
                        {
                            fSuccess = TRUE;
                        }
                    }
                }
                if( fSuccess )
                {
                    fSuccess = FALSE;
                    
                    // Begin assembling the page write operation
                    if( N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, TRUE ) )
                    {
                        N25q128aSpiSendRecvByte( tN25q128aArray[eArrId].eSpiSlaveId, N25Q_COMMAND_BE, NULL );
                        
                        N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, FALSE );
                    }
                    
                    // Wait for the operation to complete
                    if( N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, TRUE ) )
                    {
                        N25q128aSpiSendRecvByte( tN25q128aArray[eArrId].eSpiSlaveId, N25Q_COMMAND_RDSR, NULL );
                        
                        do
                        {
                            N25q128aSpiSendRecvByte( tN25q128aArray[eArrId].eSpiSlaveId, N25Q_DUMMY_BYTE, &bStatus );
                        }while( (bStatus & RDSR_BITMASK_WIP) );
                        
                        N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, FALSE );
                    }
                    
                    fSuccess = N25q128aVerifyFlagStatReg( tN25q128aArray[eArrId].eSpiSlaveId, TRUE, NULL );
                    if( fSuccess )
                    {
                        fSuccess = FALSE;
                        if( N25q128aIsFlagStatRegError( bStatus ) == FALSE )
                        {
                            fSuccess = TRUE;
                        }
                    }
                }
                
                // ***********************
                
                N25q128aSpiBusOpen( tN25q128aArray[eArrId].eSpiSlaveId, FALSE );
            }
            
            SemaphoreUnlock( tN25q128aArray[eArrId].eSemaphoreId, xSemaphoreKeyHolder );
        }
        ////////////////////////////////////////////
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL N25q128aEraseSector( N25q128aArrayIdEnum eArrId, UINT16 wSector )
{
    BOOL                    fSuccess = FALSE;
    SEMAPHORE_KEY_HOLDER    xSemaphoreKeyHolder = SEMAPHORE_RESULT_UNLOCKED;
    UINT8                   bStatus;
    
    if( eArrId < N25Q128A_ARRAY_MAX )
    {
        if( wSector < N25Q128A_NUM_SECTORS )
        {
            ////////////////////////////////////////////
            xSemaphoreKeyHolder = SemaphoreLock( tN25q128aArray[eArrId].eSemaphoreId );
            if( xSemaphoreKeyHolder != SEMAPHORE_RESULT_UNLOCKED )
            {
                if( N25q128aSpiBusOpen( tN25q128aArray[eArrId].eSpiSlaveId, TRUE ) )
                {
                    // ***********************
                    // SPI operations
                    // ***********************
                    
                    // Flash: Enable Write operation 
                    if( N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, TRUE ) )
                    {
                        N25q128aSpiSendRecvByte( tN25q128aArray[eArrId].eSpiSlaveId, N25Q_COMMAND_WREN, NULL );
                        N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, FALSE );
                    }
                    
                    // Flash: Verify that the write enable bit has been set                        
                    if( N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, TRUE ) )
                    {
                        N25q128aSpiSendRecvByte( tN25q128aArray[eArrId].eSpiSlaveId, N25Q_COMMAND_RDSR, NULL );
                        N25q128aSpiSendRecvByte( tN25q128aArray[eArrId].eSpiSlaveId, N25Q_DUMMY_BYTE, &bStatus );
                        N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, FALSE );
                        
                        if
                        (
                            // not writing something?                            
                            ( (bStatus & RDSR_BITMASK_WIP) == 0 ) &&
                            // currently in write mode?
                            ( (bStatus & RDSR_BITMASK_WEL) != 0 )
                        )
                        {
                            fSuccess = TRUE;
                        }
                    }
                            
                    if( fSuccess )
                    {
                        fSuccess = N25q128aVerifyFlagStatReg( tN25q128aArray[eArrId].eSpiSlaveId, FALSE, &bStatus );
                        if( fSuccess )
                        {
                            fSuccess = FALSE;
                            if( N25q128aIsFlagStatRegError( bStatus ) == FALSE )
                            {
                                fSuccess = TRUE;
                            }
                        }
                    }
                    if( fSuccess )
                    {
                        fSuccess = FALSE;
                        
                        // Begin assembling the sector erase command
                        if( N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, TRUE ) )
                        {
                            UINT8 bCommandArray[N25Q_LENGTH_COMMAND_SE];

                            UINT32 dwByteOffset = wSector * N25Q128A_SECTOR_SIZE_BYTES;
                            
                            bCommandArray[0] = N25Q_COMMAND_SE;
                            bCommandArray[1] = ( ( dwByteOffset & N25Q_BYTEMASK_ADDRESS_UPPER ) >> 16 );
                            bCommandArray[2] = ( ( dwByteOffset & N25Q_BYTEMASK_ADDRESS_MID )	>> 8 );
                            bCommandArray[3] = ( ( dwByteOffset & N25Q_BYTEMASK_ADDRESS_LOWER ) >> 0 );
        
                            // Send the command and 3-byte address
                            N25q128aSpiSendRecvByteArray( tN25q128aArray[eArrId].eSpiSlaveId, &bCommandArray[0], NULL, N25Q_LENGTH_COMMAND_SE );
                            
                            N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, FALSE );
                        }
                        
                        // Wait for the operation to complete
                        if( N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, TRUE ) )
                        {
                            N25q128aSpiSendRecvByte( tN25q128aArray[eArrId].eSpiSlaveId, N25Q_COMMAND_RDSR, NULL );
                            
                            do
                            {
                                N25q128aSpiSendRecvByte( tN25q128aArray[eArrId].eSpiSlaveId, N25Q_DUMMY_BYTE, &bStatus );
                            }while( (bStatus & RDSR_BITMASK_WIP) );
                            
                            N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, FALSE );
                        }
                        
                        fSuccess = N25q128aVerifyFlagStatReg( tN25q128aArray[eArrId].eSpiSlaveId, TRUE, NULL );
                        if( fSuccess )
                        {
                            fSuccess = FALSE;
                            if( N25q128aIsFlagStatRegError( bStatus ) == FALSE )
                            {
                                fSuccess = TRUE;
                            }
                        }
                    }
                    
                    // ***********************
                    
                    N25q128aSpiBusOpen( tN25q128aArray[eArrId].eSpiSlaveId, FALSE );
                }
                
                SemaphoreUnlock( tN25q128aArray[eArrId].eSemaphoreId, xSemaphoreKeyHolder );
            }
            ////////////////////////////////////////////
        }
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL N25q128aEraseSubsector( N25q128aArrayIdEnum eArrId, UINT16 wSubsector )
{
    BOOL                    fSuccess = FALSE;
    SEMAPHORE_KEY_HOLDER    xSemaphoreKeyHolder = SEMAPHORE_RESULT_UNLOCKED;
    UINT8                   bStatus;
    
    if( eArrId < N25Q128A_ARRAY_MAX )
    {
        if( wSubsector < N25Q128A_NUM_SECTORS )
        {
            ////////////////////////////////////////////
            xSemaphoreKeyHolder = SemaphoreLock( tN25q128aArray[eArrId].eSemaphoreId );
            if( xSemaphoreKeyHolder != SEMAPHORE_RESULT_UNLOCKED )
            {
                if( N25q128aSpiBusOpen( tN25q128aArray[eArrId].eSpiSlaveId, TRUE ) )
                {
                    // ***********************
                    // SPI operations
                    // ***********************
                    
                    // Flash: Enable Write operation 
                    if( N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, TRUE ) )
                    {
                        N25q128aSpiSendRecvByte( tN25q128aArray[eArrId].eSpiSlaveId, N25Q_COMMAND_WREN, NULL );
                        N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, FALSE );
                    }
                    
                    // Flash: Verify that the write enable bit has been set                        
                    if( N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, TRUE ) )
                    {
                        N25q128aSpiSendRecvByte( tN25q128aArray[eArrId].eSpiSlaveId, N25Q_COMMAND_RDSR, NULL );
                        N25q128aSpiSendRecvByte( tN25q128aArray[eArrId].eSpiSlaveId, N25Q_DUMMY_BYTE, &bStatus );
                        N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, FALSE );
                        
                        if
                        (
                            // not writing something?                            
                            ( (bStatus & RDSR_BITMASK_WIP) == 0 ) &&
                            // currently in write mode?
                            ( (bStatus & RDSR_BITMASK_WEL) != 0 )
                        )
                        {
                            fSuccess = TRUE;
                        }
                    }
                            
                    if( fSuccess )
                    {
                        fSuccess = N25q128aVerifyFlagStatReg( tN25q128aArray[eArrId].eSpiSlaveId, FALSE, &bStatus );
                        if( fSuccess )
                        {
                            fSuccess = FALSE;
                            if( N25q128aIsFlagStatRegError( bStatus ) == FALSE )
                            {
                                fSuccess = TRUE;
                            }
                        }
                    }
                    if( fSuccess )
                    {
                        fSuccess = FALSE;
                        
                        // Begin assembling the sector erase command
                        if( N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, TRUE ) )
                        {
                            UINT8 bCommandArray[N25Q_LENGTH_COMMAND_SSE];

                            UINT32 dwByteOffset = wSubsector * N25Q128A_SUBSECTOR_SIZE_BYTES;
                            
                            bCommandArray[0] = N25Q_COMMAND_SSE;
                            bCommandArray[1] = ( ( dwByteOffset & N25Q_BYTEMASK_ADDRESS_UPPER ) >> 16 );
                            bCommandArray[2] = ( ( dwByteOffset & N25Q_BYTEMASK_ADDRESS_MID )	>> 8 );
                            bCommandArray[3] = ( ( dwByteOffset & N25Q_BYTEMASK_ADDRESS_LOWER ) >> 0 );
        
                            // Send the command and 3-byte address
                            N25q128aSpiSendRecvByteArray( tN25q128aArray[eArrId].eSpiSlaveId, &bCommandArray[0], NULL, N25Q_LENGTH_COMMAND_SSE );
                            
                            N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, FALSE );
                        }
                        
                        // Wait for the operation to complete
                        if( N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, TRUE ) )
                        {
                            N25q128aSpiSendRecvByte( tN25q128aArray[eArrId].eSpiSlaveId, N25Q_COMMAND_RDSR, NULL );
                            
                            do
                            {
                                N25q128aSpiSendRecvByte( tN25q128aArray[eArrId].eSpiSlaveId, N25Q_DUMMY_BYTE, &bStatus );
                            }while( (bStatus & RDSR_BITMASK_WIP) );
                            
                            N25q128aSpiCsAssert( tN25q128aArray[eArrId].eSpiSlaveId, FALSE );
                        }
                        
                        fSuccess = N25q128aVerifyFlagStatReg( tN25q128aArray[eArrId].eSpiSlaveId, TRUE, NULL );
                        if( fSuccess )
                        {
                            fSuccess = FALSE;
                            if( N25q128aIsFlagStatRegError( bStatus ) == FALSE )
                            {
                                fSuccess = TRUE;
                            }
                        }
                    }
                    
                    // ***********************
                    
                    N25q128aSpiBusOpen( tN25q128aArray[eArrId].eSpiSlaveId, FALSE );
                }
                
                SemaphoreUnlock( tN25q128aArray[eArrId].eSemaphoreId, xSemaphoreKeyHolder );
            }
            ////////////////////////////////////////////
        }
    }
    
    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////


