//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!    \file        dataflash.c
//!    \brief       Functions for Micron N25Q serial flash memory.
//!
//!                 Micron N25Q128Axxx 128Mbit serial NOR devices supported.
//!
//!    \author      Jorge Gutierrez, Puracom Inc.
//!    \date        May 8, 2012
//!
//!    \notes
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

// Module Feature Switches

#define DF_DEBUG_ENABLE (0)     // 0 == debug off.

//////////////////////////////////////////////////////////////////////////////////////////////////
//  Local Includes
//////////////////////////////////////////////////////////////////////////////////////////////////

// System includes
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

// CMSIS Libraries
#include "stm32f2xx_gpio.h"
#include "stm32f2xx_spi.h"

// FreeRTOS includes
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "Types.h"
#include "Target.h"             // Hardware target definitions

#include "dataflash.h"
#include "gpio.h"
#include "General.h"

#include "spi.h"
//#include "stringTable.h"


//////////////////////////////////////////////////////////////////////////////////////////////////
//  Local Macros
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
//  Local Symbolic Constants
//////////////////////////////////////////////////////////////////////////////////////////////////

// Flash Timing  -----------------------------------------------------------------------------

//! Holdoff period from one SPI command to another
#define DATAFLASH_SPI_COMMAND_HOLDOFF   ( 1 / portTICK_RATE_MS )

//! Holdoff period when erasing
#define DATAFLASH_ERASE_HOLDOFF         ( 5 / portTICK_RATE_MS )

// N25Q128A11BF804E Instruction Set ----------------------------------------------------------

//! RDID (read identification) instruction UINT8
#define N25Q_COMMAND_RDID           (0x9e)

//! READ (read data UINT8s) instruction UINT8
#define N25Q_COMMAND_READ           (0x03)

//! FAST_READ (read data UINT8s at higher speed) instruction UINT8
#define N25Q_COMMAND_FAST_READ      (0x0b)

//! DOFR (dual output fast read) instruction UINT8
#define N25Q_COMMAND_DOFR           (0x3b)

//! DIOFR (dual input/output fast read) instruction UINT8
#define N25Q_COMMAND_DIOFR          (0xbb)

//! QOFR (quad output fast read) instruction UINT8
#define N25Q_COMMAND_QOFR           (0x6b)

//! QIOFR (quad input/output fast read) instruction UINT8
#define N25Q_COMMAND_QIOFR          (0xeb)

//! ROTP (read OTP) instruction UINT8
#define N25Q_COMMAND_ROTP           (0x4b)

//! WREN (write enable) instruction UINT8
#define N25Q_COMMAND_WREN           (0x06)

//! WRDI (write disable) instruction UINT8
#define N25Q_COMMAND_WRDI           (0x04)

//! PP (page program) instruction UINT8
#define N25Q_COMMAND_PP             (0x02)

//! DIFP (dual input fast program) instruction UINT8
#define N25Q_COMMAND_DIFP           (0xa2)

//! DIEFP (dual input extended fast program) instruction UINT8
#define N25Q_COMMAND_DIEFP          (0xd2)

//! QIFP (quad input fast program) instruction UINT8
#define N25Q_COMMAND_QIFP           (0x32)

//! QIEFP (quad input extended fast program) instruction UINT8
#define N25Q_COMMAND_QIEFP          (0x12)

//! POTP (program OTP) instruction UINT8
#define N25Q_COMMAND_POTP           (0x42)

//! SSE (subsector erase) instruction UINT8
#define N25Q_COMMAND_SSE            (0x20)

//! SE (sector erase) instruction UINT8
#define N25Q_COMMAND_SE             (0xd8)

//! BE (bulk erase) instruction UINT8
#define N25Q_COMMAND_BE             (0xc7)

//! PER (program/erase resume) instruction UINT8
#define N25Q_COMMAND_PER            (0x7a)

//! PES (program/erase suspend) instruction UINT8
#define N25Q_COMMAND_PES            (0x75)

//! RDSR (read status register) instruction UINT8
#define N25Q_COMMAND_RDSR           (0x05)

//! WRSR (write status register) instruction UINT8
#define N25Q_COMMAND_WRSR           (0x01)

//! RDLR (read lock register) instruction UINT8
#define N25Q_COMMAND_RDLR           (0xe8)

//! WRLR (write lock register) instruction UINT8
#define N25Q_COMMAND_WRLR           (0xe5)

//! RFSR (read flag status register) instruction UINT8
#define N25Q_COMMAND_RFSR           (0x70)

//! WFSR (write flag status register) instruction UINT8
#define N25Q_COMMAND_CLFSR          (0x50)

//! RDNVCR (read NV config register) instruction UINT8
#define N25Q_COMMAND_RDNVCR         (0xb5)

//! WRNVCR (write NV config register) instruction UINT8
#define N25Q_COMMAND_WRNVCR         (0xb1)

//! RDVCR  (read volatile configuration register) instruction UINT8
#define N25Q_COMMAND_RDVCR          (0x85)

//! WRVCR (write volatile configuration register) instruction UINT8
#define N25Q_COMMAND_WRVCR          (0x81)


//////////////////////////////////////////////////////////////////////////////////////////////////
//  Local Variables and Structures
//////////////////////////////////////////////////////////////////////////////////////////////////

static xSemaphoreHandle gxSemaphoreMutexDataFlashSpiAccess = NULL;

static DATAFLASH_ERROR_COUNTER_STRUCT gtErrorCounter;

/* This table of error descriptions must match DATAFLASH_ERROR_ENUM in the header file */
CHAR *DF_gpcErrorTable[] =
    {
        "OK",
        "DATAFLASH_ERROR",
        "PAGE INVALID",
        "WRITE FAILED",
        "WRITE BUSY",
        "WRITE OVERFLOW",
        "SECTOR INVALID",
        "IC INVALID",
        "INVALID CONTAINER",
        "DATAFLASH_FSR_VPP_ERROR",
        "DATAFLASH_FSR_PROGRAM_ERROR",
        "DATAFLASH_FSR_ERASE_ERROR",
        "DATAFLASH_FSR_PROTECTION_ERROR"
    };

#define DATAFLASH_ERROR_ARRAY_SIZE ( sizeof ( DF_gpcErrorTable ) / sizeof ( DF_gpcErrorTable[ 0 ] ) )

UINT16 DF_wModuleStatus;


//////////////////////////////////////////////////////////////////////////////////////////////////
//  Local Routines
//////////////////////////////////////////////////////////////////////////////////////////////////

DATAFLASH_ERROR_ENUM DataflashVerifyFlagStatusRegisterError( BOOL fWait, UINT8 *pbStatusByteResult );
static BOOL DataflashSemaphoreRelease( void );
static BOOL DataflashSemaphoreAcquire( void );

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn       BOOL    Dataflash_Initialize( void )
//!
//! \brief    Tests for the presence of the various flash ICs
//!           The SPI port should already be initialized.
//!
//! \return   BOOLEAN
//!           - returns TRUE if no problem on any operation , otherwise FALSE
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Dataflash_Initialize( void )
{
    BOOL fReturn = TRUE;

    DF_wModuleStatus = DATAFLASH_OK;

    memset( &gtErrorCounter, 0, sizeof( gtErrorCounter ) );

#ifndef BOOTLOADER_PROJECT
    // Create a semaphore to control access to the Dataflash
    vSemaphoreCreateBinary( gxSemaphoreMutexDataFlashSpiAccess );

    if( gxSemaphoreMutexDataFlashSpiAccess == NULL )
    {
        DF_wModuleStatus = DATAFLASH_ERROR;
        fReturn = FALSE;
    }
#endif

    if( Dataflash_TestICs() != TRUE )
    {
        DF_wModuleStatus = DATAFLASH_IC_INVALID;
        fReturn = FALSE;
    }

    return fReturn;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn       DATAFLASH_ERROR_ENUM Dataflash_moduleStatus( void )
//!
//! \brief    Get module error status.
//!
//! \return   DATAFLASH_ERROR_ENUM module error status.
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
DATAFLASH_ERROR_ENUM Dataflash_moduleStatus( void )
{
    return DF_wModuleStatus;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn       BOOL DataflashSemaphoreAcquire(void)
//!
//! \brief    Take semaphore.
//!
//! \return   TRUE once the semaphore is given to the caller function
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL DataflashSemaphoreAcquire(void)
{
    BOOL fSuccess = FALSE;

#ifndef BOOTLOADER_PROJECT
    // Attempt to acquire the semaphore
    fSuccess = ( xSemaphoreTake( gxSemaphoreMutexDataFlashSpiAccess, portMAX_DELAY ) == pdTRUE );
#else
    fSuccess = TRUE;
#endif

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn       BOOL DataflashSemaphoreRelease(void)
//!
//! \brief    Release semaphore.
//!
//! \return   TRUE once the semaphore is released to let other functions take control
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL DataflashSemaphoreRelease(void)
{
    BOOL fSuccess = FALSE;

#ifndef BOOTLOADER_PROJECT
    // Give back the semaphore
    fSuccess = (xSemaphoreGive( gxSemaphoreMutexDataFlashSpiAccess ) == pdTRUE);
#else
    fSuccess = TRUE;
#endif

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn       UINT8     Dataflash_TestICs( void )
//!
//! \brief    Tests the dataflash IC's for connectivity
//!
//! \return   Returns TRUE if IC passed.  otherwise FALSE.
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Dataflash_TestICs( void )
{
    DATAFLASH_INFO_STRUCT    tDeviceInfo;
    INT16       i=0;    /* Test IC 0 */
    BOOL        fReturn = FALSE;

    memset( &tDeviceInfo, 0, sizeof(tDeviceInfo) );
    if( DATAFLASH_OK == Dataflash_ReadDeviceID( i, &tDeviceInfo ) )
    {
        #if DF_DEBUG_ENABLE >= 1
        ConsolePrintf(CONSOLE_PORT_USART, "DF Device Manufacturer=0x%02X, Device=0x%02X, Capacity=0x%02X\r\n",
                tDeviceInfo.bManufacturer,
                tDeviceInfo.bMemoryType,
                tDeviceInfo.bCapacity );
        #endif /* DF_DEBUG_ENABLE */

        if( (   tDeviceInfo.bManufacturer == 0x20 ) &&
            ( ( tDeviceInfo.bMemoryType == 0xBA ) || ( tDeviceInfo.bMemoryType == 0xBB ) ) &&
            ( tDeviceInfo.bCapacity == 0x18 ) )
        {
            fReturn = TRUE;
        }
    }

    return fReturn;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn       DATAFLASH_ERROR_ENUM    Dataflash_ReadDeviceID( UINT8 bFlashIcNum,
//!                                                     DATAFLASH_INFO_STRUCT *ptDataflashInfo )
//!
//! \brief    Reads the data flash device's specifications and capabilities
//!
//! \param[in]  bFlashIcNum         Identifies the flash IC to access
//!
//! \param[out] ptDataflashInfo     Pointer to the container intended to receive a copy of
//!                                 the device info.
//!
//! \return   The routine returns one of the following responses:
//!           - DATAFLASH_OK                  The write was successful
//!           - DATAFLASH_IC_INVALID          The specified IC is invalid
//!           - DATAFLASH_INVALID_CONTAINER   ptDataflashInfo is unusable
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
DATAFLASH_ERROR_ENUM  Dataflash_ReadDeviceID( UINT8 bFlashIcNum,  DATAFLASH_INFO_STRUCT *ptDataflashInfo )
{
    DATAFLASH_ERROR_ENUM eError = DATAFLASH_OK;
    BOOL fSpiPass;
    int     i;
    UINT8   bData[20];


    // #### OBTAINING SEMAPHORE ####
    DataflashSemaphoreAcquire();
    // #### OBTAINING SEMAPHORE ####


    // Assert the specified IC is valid
    if( !DATAFLASH_IS_IC_VALID(bFlashIcNum) )
    {
        eError = DATAFLASH_IC_INVALID;
    }

    // Assert the container is valid
    if( ptDataflashInfo == NULL )
    {
        eError = DATAFLASH_INVALID_CONTAINER;
    }


    if( eError == DATAFLASH_OK )
    {
        // Send command command N25Q_COMMAND_RDID (Read Identification)
        // and read the device info
        fSpiPass = SpiSlaveBusOpen( SPI_SLAVE_DATAFLASH, 0 );
        if( fSpiPass != TRUE )
        {
            return DATAFLASH_SPI_OPEN_ERROR;
        }

        SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, TRUE );
        SpiSlaveSendRecvByte( SPI_SLAVE_DATAFLASH, N25Q_COMMAND_RDID, NULL );

        SpiSlaveSendRecvByteArray( SPI_SLAVE_DATAFLASH, NULL, &bData[0], 20 );

        SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, FALSE );

        // Copy data to Flash info structure
        ptDataflashInfo->bManufacturer  = bData[0];
        ptDataflashInfo->bMemoryType    = bData[1];
        ptDataflashInfo->bCapacity      = bData[2];
        ptDataflashInfo->bUIDLen        = bData[3];
        ptDataflashInfo->bEDID[0]       = bData[4];
        ptDataflashInfo->bEDID[1]       = bData[5];
        memcpy( ptDataflashInfo->bFactory, &bData[6], sizeof(ptDataflashInfo->bFactory) );
    }

    SpiSlaveBusClose( SPI_SLAVE_DATAFLASH );

    // #### RELEASING SEMAPHORE ####
    DataflashSemaphoreRelease();
    // #### RELEASING SEMAPHORE ####


    // Report success
    return eError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn       DATAFLASH_ERROR_ENUM    Dataflash_Write()
//!
//! \brief    Writes a buffer of data to an arbitrary address in the dataflash,
//!           not necessarily at the start of a page.
//!
//!           The serial flash data areas are treated as a linear contiguous
//!           address space that skips over the page boundaries.
//!
//! \param[in]  bFlashIcNum   Identifies the flash IC to access
//!
//! \param[in]  dwAddress     Address in flash memory to write to
//!
//! \param[in]  pbDataBuffer  Pointer to the buffer containing the data to write
//!
//! \param[in]  dwLength      Length of the data in the buffer
//!
//! \return     The routine returns one of the following responses:
//!             - DATAFLASH_OK                  The write was successful
//!             - DATAFLASH_PAGE_INVALID        The specified page is invalid
//!             - DATAFLASH_WRITE_FAILED        Could not get the flash device to write
//!             - DATAFLASH_WRITE_BUSY          The flash device is currently busy
//!             - DATAFLASH_IC_INVALID          The specified IC is invalid
//!             - DATAFLASH_WRITE_OVERFLOW      Too many UINT8s to write, operation aborted
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
DATAFLASH_ERROR_ENUM Dataflash_Write( UINT8 bFlashIcNum, UINT32 dwAddress, UINT8 *pbDataBuffer, UINT32 dwLength )
{
    UINT32 dwIndex;           /* Offset within data buffer */
    UINT32 dwWriteLength;
    UINT16 wPageDataOffset;
    UINT16 wResult;

    dwIndex = 0;
    wResult = DATAFLASH_OK;

    // Note: Dataflash semaphore is obtained by lower level functions.

    // Assert the specified IC is valid
    if( !DATAFLASH_IS_IC_VALID(bFlashIcNum) )
    {
        wResult = DATAFLASH_IC_INVALID;
        return wResult;
    }

    // Check that the address and length are valid
    if( ( dwLength == 0 ) ||
        ( ( dwAddress + dwLength - 1 ) >= DATAFLASH_IC_SIZE_BYTES ) )
    {
        wResult = DATAFLASH_WRITE_OVERFLOW;
        return wResult;
    }

    /* Set offset into first page to write */
    wPageDataOffset = (UINT16)( dwAddress % DATAFLASH_PAGE_SIZE_BYTES );

    /* Write pages */
    while( ( dwLength > 0 ) && ( wResult == DATAFLASH_OK ) )
    {
        dwWriteLength = GENERAL_MIN( DATAFLASH_PAGE_SIZE_BYTES - wPageDataOffset, dwLength );

        #if DF_DEBUG_ENABLE >= 2
        ConsolePrintf(CONSOLE_PORT_USART, "DF_WRITE 0x%06lx, %u bytes, Offset=%u\r\n",
                dwAddress, dwWriteLength, wPageDataOffset );
        #endif /* DF_DEBUG_ENABLE */

        /* Write data to Dataflash */
        wResult = Dataflash_WriteInPage( bFlashIcNum, dwAddress, &pbDataBuffer[ dwIndex ], dwWriteLength );

        dwIndex += dwWriteLength;
        dwLength -= dwWriteLength;
        dwAddress += dwWriteLength;
        wPageDataOffset = 0;     /* All subsequent writes will start at the
                                  * beginning of a page.
                                  */
    } /* end while */

    return wResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn       DATAFLASH_ERROR_ENUM    Dataflash_WriteInPage() - write to an arbitrary location
//!
//! \brief    Writes to an arbitrary address in the dataflash, not necessarily at the start
//!           of a page.  The write must be totally within a single page.
//!           Use Dataflash_Write() to write a longer buffer of data across multiple pages.
//!
//! \param[in]  bFlashIcNum   Identifies the flash IC to write to
//!
//! \param[in]  dwAddress     Address in flash memory to write to
//!
//! \param[in]  pbDataBuffer  Pointer to the buffer containing the data to write
//!
//! \param[in]  dwLength          Length of the data in the buffer
//!
//! \return     The routine returns one of the following responses:
//!             - DATAFLASH_OK                  The write was successful
//!             - DATAFLASH_PAGE_INVALID        The specified page is invalid
//!             - DATAFLASH_WRITE_FAILED        Could not get the flash device to write
//!             - DATAFLASH_WRITE_BUSY          The flash device is currently busy
//!             - DATAFLASH_IC_INVALID          The specified IC is invalid
//!             - DATAFLASH_WRITE_OVERFLOW      Too many UINT8s to write, operation aborted
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
DATAFLASH_ERROR_ENUM Dataflash_WriteInPage( UINT8 bFlashIcNum, UINT32 dwAddress, UINT8 *pbDataBuffer, UINT32 dwLength )
{
    DATAFLASH_ERROR_ENUM eError = DATAFLASH_OK;
    BOOL fSpiPass;
    int i;
    UINT8 b;


    // #### OBTAINING SEMAPHORE ####
    DataflashSemaphoreAcquire();
    // #### OBTAINING SEMAPHORE ####


    // Sanity check function parameters -------------------------------------------------

    // Assert the specified IC is valid
    if( !DATAFLASH_IS_IC_VALID(bFlashIcNum) )
    {
        eError = DATAFLASH_IC_INVALID;
        return eError;
    }

    // Check that there isn't too much data to write to a single page
    if( ( dwLength > DATAFLASH_PAGE_SIZE_BYTES ) ||
        ( dwLength == 0 ) ||
        ( ( ( dwAddress & 0xFF ) + dwLength - 1 ) >= DATAFLASH_PAGE_SIZE_BYTES ) )
    {
        eError = DATAFLASH_WRITE_OVERFLOW;
        return eError;
    }


    // Enable write operations on the flash memory
    fSpiPass = SpiSlaveBusOpen( SPI_SLAVE_DATAFLASH, 0 );
    if( fSpiPass != TRUE )
    {
        return DATAFLASH_SPI_OPEN_ERROR;
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, TRUE );

    SpiSlaveSendRecvByte( SPI_SLAVE_DATAFLASH, N25Q_COMMAND_WREN, NULL );

    SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, FALSE );

    vTaskDelay( DATAFLASH_SPI_COMMAND_HOLDOFF );

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Verify that the write enable bit has been set
    SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, TRUE );

    SpiSlaveSendRecvByte( SPI_SLAVE_DATAFLASH, N25Q_COMMAND_RDSR, NULL );
    SpiSlaveSendRecvByte( SPI_SLAVE_DATAFLASH, 0x00, &b );

    SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, FALSE );

    vTaskDelay( DATAFLASH_SPI_COMMAND_HOLDOFF );

    // Already writing something?
    if( (b & 0x01) )
    {
        eError = DATAFLASH_WRITE_BUSY;
        return eError;
    }

    // Not in write mode?
    if( (b & 0x02) == 0)
    {
        eError = DATAFLASH_WRITE_FAILED;
        return eError;
    }

    eError = DataflashVerifyFlagStatusRegisterError( FALSE, NULL );

    if( eError == DATAFLASH_OK )
    {
        // Begin the page write operation, send page program command
        SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, TRUE );

        UINT8 bCommandArray[4];

        bCommandArray[0] = N25Q_COMMAND_PP;
        bCommandArray[1] = ( ( dwAddress & 0xFF0000 ) >> 16 );
        bCommandArray[2] = ( ( dwAddress & 0x00FF00 ) >> 8 );
        bCommandArray[3] = ( ( dwAddress & 0x0000FF ) >> 0 );

        // Send the command and 3-byte address
        SpiSlaveSendRecvByteArray( SPI_SLAVE_DATAFLASH, &bCommandArray[0], NULL, 4);

        // Write data
        SpiSlaveSendRecvByteArray( SPI_SLAVE_DATAFLASH, &pbDataBuffer[0], NULL, dwLength );

        SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, FALSE );

        vTaskDelay( DATAFLASH_SPI_COMMAND_HOLDOFF );

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Wait for the operation to complete
        SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, TRUE );

        SpiSlaveSendRecvByte( SPI_SLAVE_DATAFLASH, N25Q_COMMAND_RDSR, NULL );
        do
        {
            SpiSlaveSendRecvByte( SPI_SLAVE_DATAFLASH, 0x00, &b );
        } while( (b & 0x01) );

        SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, FALSE );

        eError = DataflashVerifyFlagStatusRegisterError( TRUE, NULL );

        vTaskDelay( DATAFLASH_SPI_COMMAND_HOLDOFF );
    }

    SpiSlaveBusClose( SPI_SLAVE_DATAFLASH );

    // #### RELEASING SEMAPHORE ####
    DataflashSemaphoreRelease();
    // #### RELEASING SEMAPHORE ####


    // Report success
    return eError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn       DATAFLASH_ERROR_ENUM    Dataflash_Read() - read from an arbitrary location
//!
//! \brief    Reads from an arbitrary address in the dataflash.
//!           The read can span multiple pages.
//!
//! \param[in]  bFlashIcNum   Identifies the flash IC to read from
//!
//! \param[in]  dwAddress     Address in flash memory to read from
//!
//! \param[in]  pbDataBuffer  Pointer to the buffer to hold read data
//!
//! \param[in]  dwLength          Length of the data to read
//!
//! \return   The routine returns one of the following responses:
//!           - DATAFLASH_OK                    The write was successful
//!           - DATAFLASH_PAGE_INVALID          The specified page is invalid
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
DATAFLASH_ERROR_ENUM  Dataflash_Read( UINT8 bFlashIcNum, UINT32 dwAddress, UINT8 *pbPageData, UINT32 dwLength )
{
    DATAFLASH_ERROR_ENUM eError = DATAFLASH_OK;
    BOOL fSpiPass;
    int i;


    // #### OBTAINING SEMAPHORE ####
    DataflashSemaphoreAcquire();
    // #### OBTAINING SEMAPHORE ####

    // Assert the specified IC is valid
    if( !DATAFLASH_IS_IC_VALID( bFlashIcNum ) )
    {
        eError = DATAFLASH_IC_INVALID;
    }

    // Begin the page read
    fSpiPass = SpiSlaveBusOpen( SPI_SLAVE_DATAFLASH, 0 );
    if( fSpiPass != TRUE )
    {
        return DATAFLASH_SPI_OPEN_ERROR;
    }

    SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, TRUE );

    UINT8 bCommandArray[4];

    bCommandArray[0] = N25Q_COMMAND_READ;
    bCommandArray[1] = ( ( dwAddress & 0xFF0000 ) >> 16 );
    bCommandArray[2] = ( ( dwAddress & 0x00FF00 ) >> 8 );
    bCommandArray[3] = ( ( dwAddress & 0x0000FF ) >> 0 );

    // Send the command and 3-byte address
    SpiSlaveSendRecvByteArray( SPI_SLAVE_DATAFLASH, &bCommandArray[0], NULL, 4);

    // Read data
    SpiSlaveSendRecvByteArray( SPI_SLAVE_DATAFLASH, NULL, &pbPageData[0], dwLength );

    SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, FALSE );

    vTaskDelay( DATAFLASH_SPI_COMMAND_HOLDOFF );

    SpiSlaveBusClose( SPI_SLAVE_DATAFLASH );

    // #### RELEASING SEMAPHORE ####
    DataflashSemaphoreRelease();
    // #### RELEASING SEMAPHORE ####

    // Report success
    return eError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn       DATAFLASH_ERROR_ENUM    Dataflash_WritePage( UINT8 bFlashIcNum, UINT32 dwPageNum,
//!                                                      UINT8 *pbPageData, UINT32 dwLength )
//!
//! \brief    Writes the current page buffer to flash memory
//!
//! \param[in]  bFlashIcNum   Identifies the flash IC to write to
//!
//! \param[in]  dwPageNum     Specified page, in flash memory, intended to receive this data
//!
//! \param[in]  pbPageData    Pointer to the buffer containing the data to write
//!
//! \param[in]  dwLength          The number of UINT8s residing in the buffer
//!
//! \return     The routine returns one of the following responses:
//!             - DATAFLASH_OK                  The write was successful
//!             - DATAFLASH_PAGE_INVALID        The specified page is invalid
//!             - DATAFLASH_WRITE_FAILED        Could not get the flash device to write
//!             - DATAFLASH_WRITE_BUSY          The flash device is currently busy
//!             - DATAFLASH_IC_INVALID          The specified IC is invalid
//!             - DATAFLASH_WRITE_OVERFLOW      Too many UINT8s to write, operation aborted
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
DATAFLASH_ERROR_ENUM  Dataflash_WritePage( UINT8 bFlashIcNum, UINT32 dwPageNum, UINT8 *pbPageData, UINT32 dwLength )
{
    DATAFLASH_ERROR_ENUM eError = DATAFLASH_OK;
    BOOL fSpiPass;
    int i;
    UINT8 b;


    // #### OBTAINING SEMAPHORE ####
    DataflashSemaphoreAcquire();
    // #### OBTAINING SEMAPHORE ####


    // Sanity check function parameters -------------------------------------------------

    // Assert the specified IC is valid
    if( !DATAFLASH_IS_IC_VALID(bFlashIcNum) )
    {
        eError = DATAFLASH_IC_INVALID;
    }

    // Assert the specified page is valid
    if( !DATAFLASH_IS_PAGE_VALID(dwPageNum) )
    {
        eError = DATAFLASH_PAGE_INVALID;
    }

    // Assert that there isn't too much data to write
    if( ( dwLength > DATAFLASH_PAGE_SIZE_BYTES ) || ( dwLength == 0 ) )
    {
        eError = DATAFLASH_WRITE_OVERFLOW;
    }



    if( eError == DATAFLASH_OK )
    {
        // Enable write operations on the flash memory
        fSpiPass = SpiSlaveBusOpen( SPI_SLAVE_DATAFLASH, 0 );
        if( fSpiPass != TRUE )
        {
            return DATAFLASH_SPI_OPEN_ERROR;
        }

        SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, TRUE );

        SpiSlaveSendRecvByte( SPI_SLAVE_DATAFLASH, N25Q_COMMAND_WREN, NULL );

        SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, FALSE );

        vTaskDelay( DATAFLASH_SPI_COMMAND_HOLDOFF );

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Verify that the write enable bit has been set
        SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, TRUE );

        SpiSlaveSendRecvByte( SPI_SLAVE_DATAFLASH, N25Q_COMMAND_RDSR, NULL );
        SpiSlaveSendRecvByte( SPI_SLAVE_DATAFLASH, 0x00, &b );

        SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, FALSE );

        vTaskDelay( DATAFLASH_SPI_COMMAND_HOLDOFF );

        // Already writing something?
        if( (b & 0x01) )
        {
            eError = DATAFLASH_WRITE_BUSY;
        }

        // Not in write mode?
        if( (b & 0x02) == 0)
        {
            eError = DATAFLASH_WRITE_FAILED;
        }

        eError = DataflashVerifyFlagStatusRegisterError( FALSE, NULL );

        if( eError == DATAFLASH_OK )
        {
            // Begin the page write operation, send page program command
            SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, TRUE );

            UINT8 bCommandArray[4];

            bCommandArray[0] = N25Q_COMMAND_PP;
            bCommandArray[1] = ( (dwPageNum & 0x00FF00) >> 8 );
            bCommandArray[2] = ( (dwPageNum & 0x0000FF) >> 0 );
            bCommandArray[3] = 0x00;

            // Send the command and 3-byte address
            SpiSlaveSendRecvByteArray( SPI_SLAVE_DATAFLASH, &bCommandArray[0], NULL, 4);

            // Write data
            SpiSlaveSendRecvByteArray( SPI_SLAVE_DATAFLASH, &pbPageData[0], NULL, dwLength );

            SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, FALSE );

            vTaskDelay( DATAFLASH_SPI_COMMAND_HOLDOFF );

            // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
            // Wait for the operation to complete
            SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, TRUE );

            SpiSlaveSendRecvByte( SPI_SLAVE_DATAFLASH, N25Q_COMMAND_RDSR, NULL );
            do
            {
                SpiSlaveSendRecvByte( SPI_SLAVE_DATAFLASH, 0x00, &b );
            } while( (b & 0x01) );

            SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, FALSE );

            eError = DataflashVerifyFlagStatusRegisterError( TRUE, NULL );

            vTaskDelay( DATAFLASH_SPI_COMMAND_HOLDOFF );
        }
    }

    SpiSlaveBusClose( SPI_SLAVE_DATAFLASH );

    // #### RELEASING SEMAPHORE ####
    DataflashSemaphoreRelease();
    // #### RELEASING SEMAPHORE ####


    // Report success
    return eError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn       DATAFLASH_ERROR_ENUM  Dataflash_ReadPage( UINT8 bFlashIcNum, UINT32 dwPageNum, UINT8 *pbPageData, UINT32 dwLength )
//!
//! \brief    Read a page from flash and stores it in the specified buffer
//!
//! \param[in]  bFlashIcNum   Identifies the flash IC to read from
//!
//! \param[in]  dwPageNum     Specified page to read from, in flash memory
//!
//! \param[out] pbPageData    Pointer to the buffer intended to receive this data
//!
//! \param[in]  dwLength      Number of UINT8s to read from the page
//!
//! \return   The routine returns one of the following responses:
//!           - DATAFLASH_OK                    The write was successful
//!           - DATAFLASH_PAGE_INVALID          The specified page is invalid
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
DATAFLASH_ERROR_ENUM  Dataflash_ReadPage( UINT8 bFlashIcNum, UINT32 dwPageNum, UINT8 *pbPageData, UINT32 dwLength )
{
    DATAFLASH_ERROR_ENUM eError = DATAFLASH_OK;
    BOOL fSpiPass;
    int i;


    // #### OBTAINING SEMAPHORE ####
    DataflashSemaphoreAcquire();
    // #### OBTAINING SEMAPHORE ####



    // Assert the specified IC is valid
    if( !DATAFLASH_IS_IC_VALID( bFlashIcNum ) )
    {
        eError = DATAFLASH_IC_INVALID;
    }

    // Assert the specified page is valid
    if( !DATAFLASH_IS_PAGE_VALID( dwPageNum ) )
    {
        eError = DATAFLASH_PAGE_INVALID;
    }



    if( eError == DATAFLASH_OK )
    {
        // Begin assembling the page read operation
        fSpiPass = SpiSlaveBusOpen( SPI_SLAVE_DATAFLASH, 0 );
        if( fSpiPass != TRUE )
        {
            return DATAFLASH_SPI_OPEN_ERROR;
        }

        SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, TRUE );

        UINT8 bCommandArray[4];

        bCommandArray[0] = N25Q_COMMAND_READ;
        bCommandArray[1] = ( (dwPageNum & 0x00FF00) >> 8 );
        bCommandArray[2] = ( (dwPageNum & 0x0000FF) >> 0 );
        bCommandArray[3] = 0x00;

        // Send the command and 3-byte address
        SpiSlaveSendRecvByteArray( SPI_SLAVE_DATAFLASH, &bCommandArray[0], NULL, 4);

        // Read data
        SpiSlaveSendRecvByteArray( SPI_SLAVE_DATAFLASH, NULL, &pbPageData[0], dwLength );

        SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, FALSE );

        vTaskDelay( DATAFLASH_SPI_COMMAND_HOLDOFF );
    }

    SpiSlaveBusClose( SPI_SLAVE_DATAFLASH );

    // #### RELEASING SEMAPHORE ####
    DataflashSemaphoreRelease();
    // #### RELEASING SEMAPHORE ####

    // Report success
    return eError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn       DATAFLASH_ERROR_ENUM  Flash_EraseAll( UINT8 bFlashIcNum )
//!
//! \brief    Erases an entire IC
//!
//! \param[in]  bFlashIcNum   Identifies the flash IC to erase
//!
//! \return   The routine returns one of the following responses:
//!           - DATAFLASH_OK            The write was successful
//!           - DATAFLASH_IC_INVALID    The specified IC is invalid
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
DATAFLASH_ERROR_ENUM  Dataflash_EraseAll( UINT8 bFlashIcNum )
{
    DATAFLASH_ERROR_ENUM eError = DATAFLASH_OK;
    BOOL fSpiPass;
    UINT8 b;

    // #### OBTAINING SEMAPHORE ####
    DataflashSemaphoreAcquire();
    // #### OBTAINING SEMAPHORE ####

    // Assert the specified IC is valid
    if( !DATAFLASH_IS_IC_VALID(bFlashIcNum) )
    {
        eError = DATAFLASH_IC_INVALID;
    }

    if( eError == DATAFLASH_OK )
    {
        #if DF_DEBUG_ENABLE >= 2
        ConsolePrintf(CONSOLE_PORT_USART, "DF_ERASE_ALL\r\n" );
        #endif /* DF_DEBUG_ENABLE */

        // Enable write operations on the flash memory
        fSpiPass = SpiSlaveBusOpen( SPI_SLAVE_DATAFLASH, 0 );
        if( fSpiPass != TRUE )
        {
            return DATAFLASH_SPI_OPEN_ERROR;
        }

        SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, TRUE );

        SpiSlaveSendRecvByte( SPI_SLAVE_DATAFLASH, N25Q_COMMAND_WREN, NULL );

        SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, FALSE );

        vTaskDelay( DATAFLASH_SPI_COMMAND_HOLDOFF );

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Verify that the write enable bit has been set
        SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, TRUE );

        SpiSlaveSendRecvByte( SPI_SLAVE_DATAFLASH, N25Q_COMMAND_RDSR, NULL );
        SpiSlaveSendRecvByte( SPI_SLAVE_DATAFLASH, 0x00, &b );

        SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, FALSE );

        vTaskDelay( DATAFLASH_SPI_COMMAND_HOLDOFF );

        // Already writing something?
        if( (b & 0x01) )
        {
            eError = DATAFLASH_WRITE_BUSY;
        }

        // Not in write mode?
        if( (b & 0x02) == 0)
        {
            eError = DATAFLASH_WRITE_FAILED;
        }

        eError = DataflashVerifyFlagStatusRegisterError( FALSE, NULL );

        if( eError == DATAFLASH_OK )
        {
            // Begin assembling the page write operation
            SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, TRUE );

            SpiSlaveSendRecvByte( SPI_SLAVE_DATAFLASH, N25Q_COMMAND_BE, NULL );

            SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, FALSE );

            vTaskDelay( DATAFLASH_SPI_COMMAND_HOLDOFF );

            // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
            // Wait for the operation to complete
            SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, TRUE );

            SpiSlaveSendRecvByte( SPI_SLAVE_DATAFLASH, N25Q_COMMAND_RDSR, NULL );
            do
            {
                SpiSlaveSendRecvByte( SPI_SLAVE_DATAFLASH, 0x00, &b );
            } while( (b & 0x01) );

            SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, FALSE );

            eError = DataflashVerifyFlagStatusRegisterError( TRUE, NULL );

            vTaskDelay( DATAFLASH_SPI_COMMAND_HOLDOFF );

        }
    }

    SpiSlaveBusClose( SPI_SLAVE_DATAFLASH );

    // #### RELEASING SEMAPHORE ####
    DataflashSemaphoreRelease();
    // #### RELEASING SEMAPHORE ####

    // Report success
    return eError;
}



//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn       DATAFLASH_ERROR_ENUM  Dataflash_EraseSector( UINT8 bFlashIcNum, UINT16 wSector )
//!
//! \brief    Erases a single sector on the IC
//!
//! \param[in]  bFlashIcNum   Identifies the flash IC to erase
//!
//! \param[in]  wSector       Identifies the sector to erase
//!
//! \return   The routine returns one of the following responses:
//!           - DATAFLASH_OK                The write was successful
//!           - DATAFLASH_IC_INVALID        The specified IC is invalid
//!           - DATAFLASH_SECTOR_INVALID    The specified sector is invalid
//!           - DATAFLASH_WRITE_BUSY        The IC is busy
//!           - DATAFLASH_WRITE_FAILED      The IC is not in write mode
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
DATAFLASH_ERROR_ENUM  Dataflash_EraseSector( UINT8 bFlashIcNum, UINT16 wSector )
{
    DATAFLASH_ERROR_ENUM eError = DATAFLASH_OK;
    BOOL fSpiPass;
    UINT32  dwStartAddress;
    UINT8   b;


    // #### OBTAINING SEMAPHORE ####
    DataflashSemaphoreAcquire();
    // #### OBTAINING SEMAPHORE ####


    // Assert the specified IC is valid
    if( !DATAFLASH_IS_IC_VALID(bFlashIcNum) )
    {
        eError = DATAFLASH_IC_INVALID;
    }

    // Assert the specified sector is valid
    if( !DATAFLASH_IS_SECTOR_VALID(wSector) )
    {
        eError = DATAFLASH_SECTOR_INVALID;
    }


    if( eError == DATAFLASH_OK )
    {
        #if DF_DEBUG_ENABLE >= 2
        ConsolePrintf(CONSOLE_PORT_USART, "DF_ERASE_SECTOR %u\r\n", wSector );
        #endif /* DF_DEBUG_ENABLE */

        // Enable write operations on the flash memory
        fSpiPass = SpiSlaveBusOpen( SPI_SLAVE_DATAFLASH, 0 );
        if( fSpiPass != TRUE )
        {
            return DATAFLASH_SPI_OPEN_ERROR;
        }

        SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, TRUE );

        vTaskDelay( DATAFLASH_SPI_COMMAND_HOLDOFF );
        SpiSlaveSendRecvByte( SPI_SLAVE_DATAFLASH, N25Q_COMMAND_WREN, NULL );

        SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, FALSE );

        vTaskDelay( DATAFLASH_SPI_COMMAND_HOLDOFF );

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Verify that the write enable bit has been set
        SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, TRUE );

        SpiSlaveSendRecvByte( SPI_SLAVE_DATAFLASH, N25Q_COMMAND_RDSR, NULL );
        SpiSlaveSendRecvByte( SPI_SLAVE_DATAFLASH, 0x00, &b );

        SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, FALSE );

        vTaskDelay( DATAFLASH_SPI_COMMAND_HOLDOFF );

        // Already writing something?
        if( (b & 0x01) )
        {
            eError = DATAFLASH_WRITE_BUSY;
        }

        // Not in write mode?
        if( (b & 0x02) == 0)
        {
            eError = DATAFLASH_WRITE_FAILED;
        }

        eError = DataflashVerifyFlagStatusRegisterError( FALSE, NULL );

        if( eError == DATAFLASH_OK )
        {
            // Begin assembling the sector erase command
            SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, TRUE );

            dwStartAddress = wSector * DATAFLASH_SECTOR_SIZE_PAGES * DATAFLASH_PAGE_SIZE_BYTES;

            UINT8 bCommandArray[4];

            bCommandArray[0] = N25Q_COMMAND_SE;
            bCommandArray[1] = ( (dwStartAddress & 0xFF0000) >> 16 );
            bCommandArray[2] = ( (dwStartAddress & 0x00FF00) >> 8 );
            bCommandArray[3] = ( (dwStartAddress & 0x0000FF) >> 0 );

            // Send the command and 3-byte address
            SpiSlaveSendRecvByteArray( SPI_SLAVE_DATAFLASH, &bCommandArray[0], NULL, 4);

            SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, FALSE );

            vTaskDelay( DATAFLASH_ERASE_HOLDOFF );

            // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
            // Wait for the operation to complete
            SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, TRUE );

            SpiSlaveSendRecvByte( SPI_SLAVE_DATAFLASH, N25Q_COMMAND_RDSR, NULL );
            do
            {
                SpiSlaveSendRecvByte( SPI_SLAVE_DATAFLASH, 0x00, &b );
            } while( (b & 0x01) );

            SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, FALSE );

            eError = DataflashVerifyFlagStatusRegisterError( TRUE, NULL );

            vTaskDelay( DATAFLASH_SPI_COMMAND_HOLDOFF );
        }
    }

    SpiSlaveBusClose( SPI_SLAVE_DATAFLASH );

    // #### RELEASING SEMAPHORE ####
    DataflashSemaphoreRelease();
    // #### RELEASING SEMAPHORE ####

    // Report success
    return eError;
}



//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn     DATAFLASH_ERROR_ENUM  Dataflash_EraseSubsector( UINT8 bFlashIcNum, UINT16 wSubsector )
//!
//! \brief  // Erases a subsector
//!
//! \param[in]  bFlashIcNum     Identifies the flash IC to erase
//!
//! \param[in]  wSubsector      Identifies the subsector to erase
//!
//! \return   The routine returns one of the following responses:
//!           - DATAFLASH_OK                The write was successful
//!           - DATAFLASH_IC_INVALID        The specified IC is invalid
//!           - DATAFLASH_SECTOR_INVALID    The specified sector is invalid
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
DATAFLASH_ERROR_ENUM Dataflash_EraseSubsector( UINT8 bFlashIcNum, UINT16 wSubsector )
{
    DATAFLASH_ERROR_ENUM eError = DATAFLASH_OK;
    BOOL fSpiPass;
    UINT32  dwStartAddress;
    UINT8   b;


    // #### OBTAINING SEMAPHORE ####
    DataflashSemaphoreAcquire();
    // #### OBTAINING SEMAPHORE ####


    // Assert the specified IC is valid
    if( !DATAFLASH_IS_IC_VALID(bFlashIcNum) )
    {
        eError = DATAFLASH_IC_INVALID;
        return eError;
    }

    // Assert the specified sector is valid
    if( !DATAFLASH_IS_SUBSECTOR_VALID(wSubsector) )
    {
        eError = DATAFLASH_SECTOR_INVALID;
        return eError;
    }

    #if DF_DEBUG_ENABLE >= 2
    ConsolePrintf(CONSOLE_PORT_USART, "DF_ERASE_SUBSECTOR %u\r\n", wSubsector );
    #endif /* DF_DEBUG_ENABLE */

    // Enable write operations on the flash memory
    fSpiPass = SpiSlaveBusOpen( SPI_SLAVE_DATAFLASH, 0 );
    if( fSpiPass != TRUE )
    {
        return DATAFLASH_SPI_OPEN_ERROR;
    }

    SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, TRUE );

    SpiSlaveSendRecvByte( SPI_SLAVE_DATAFLASH, N25Q_COMMAND_WREN, NULL );

    SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, FALSE );

    vTaskDelay( DATAFLASH_SPI_COMMAND_HOLDOFF );

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Verify that the write enable bit has been set
    SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, TRUE );

    SpiSlaveSendRecvByte( SPI_SLAVE_DATAFLASH, N25Q_COMMAND_RDSR, NULL );
    SpiSlaveSendRecvByte( SPI_SLAVE_DATAFLASH, 0x00, &b );

    SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, FALSE );

    vTaskDelay( DATAFLASH_SPI_COMMAND_HOLDOFF );

    // Already writing something?
    if( b & 0x01 )
    {
        eError = DATAFLASH_WRITE_BUSY;
        return eError;
    }

    // Not in write mode?
    if( ( b & 0x02 ) == 0 )
    {
        eError = DATAFLASH_WRITE_FAILED;
        return eError;
    }

    eError = DataflashVerifyFlagStatusRegisterError( FALSE, NULL );

    if( eError == DATAFLASH_OK )
    {
        // Begin assembling the subsector erase command
        SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, TRUE );

        dwStartAddress = wSubsector * DATAFLASH_SUBSECTOR_SIZE_PAGES * DATAFLASH_PAGE_SIZE_BYTES;

        UINT8 bCommandArray[4];

        bCommandArray[0] = N25Q_COMMAND_SSE;
        bCommandArray[1] = ( (dwStartAddress & 0xFF0000) >> 16 );
        bCommandArray[2] = ( (dwStartAddress & 0x00FF00) >> 8 );
        bCommandArray[3] = ( (dwStartAddress & 0x0000FF) >> 0 );

        // Send the command and 3-byte address
        SpiSlaveSendRecvByteArray( SPI_SLAVE_DATAFLASH, &bCommandArray[0], NULL, 4);

        SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, FALSE );

        vTaskDelay( DATAFLASH_ERASE_HOLDOFF );

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // Wait for the operation to complete
        SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, TRUE );

        SpiSlaveSendRecvByte( SPI_SLAVE_DATAFLASH, N25Q_COMMAND_RDSR, NULL );
        do
        {
            SpiSlaveSendRecvByte( SPI_SLAVE_DATAFLASH, 0x00, &b );
        } while( (b & 0x01) );

        SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, FALSE );

        eError = DataflashVerifyFlagStatusRegisterError( TRUE, NULL );

        vTaskDelay( DATAFLASH_SPI_COMMAND_HOLDOFF );
    }

    SpiSlaveBusClose( SPI_SLAVE_DATAFLASH );

    // #### RELEASING SEMAPHORE ####
    DataflashSemaphoreRelease();
    // #### RELEASING SEMAPHORE ####

    return eError;
}



//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn       DATAFLASH_ERROR_ENUM  Dataflash_EraseSectorRange( UINT8 bFlashIcNum, UINT16 wSectorStart, UINT16 wSectorEnd )
//!
//! \brief    Erases a range of sectors
//!
//! \param[in]  bFlashIcNum   Identifies the flash IC to erase
//! \param[in]  wSectorStart  Identifies the first sector to erase
//! \param[in]  wSectorEnd    Identifies the last sector to erase
//!
//! \return   The routine returns one of the following responses:
//!           - DATAFLASH_OK                The write was successful
//!           - DATAFLASH_ERROR             The specified IC is invalid
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
DATAFLASH_ERROR_ENUM  Dataflash_EraseSectorRange( UINT8 bFlashIcNum, UINT16 wSectorStart, UINT16 wSectorEnd )
{
    DATAFLASH_ERROR_ENUM eReturn;
    UINT16 i;

    wSectorStart = GENERAL_MIN( wSectorStart, ( DATAFLASH_NUM_SECTORS - 1 ) );
    wSectorEnd = GENERAL_MIN( wSectorEnd, ( DATAFLASH_NUM_SECTORS - 1 ) );
    eReturn = DATAFLASH_OK;

    for( i = wSectorStart; ( ( i <= wSectorEnd ) && ( eReturn == DATAFLASH_OK ) ); i++)
    {
        eReturn = Dataflash_EraseSector( bFlashIcNum, i );
    }

    return eReturn;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn       DATAFLASH_ERROR_ENUM  Dataflash_EraseSubSectorRange( UINT8 bFlashIcNum, UINT16 wSubSectorStart, UINT16 wSubSectorEnd )
//!
//! \brief    Erases a range of Sub-sectors
//!
//! \param[in]  bFlashIcNum   Identifies the flash IC to erase
//! \param[in]  wSubSectorStart  Identifies the first sub sector to erase
//! \param[in]  wSubSectorEnd    Identifies the last sub sector to erase
//!
//! \return   The routine returns one of the following responses:
//!           - DATAFLASH_OK                The write was successful
//!           - DATAFLASH_ERROR             The specified IC is invalid
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
DATAFLASH_ERROR_ENUM  Dataflash_EraseSubSectorRange( UINT8 bFlashIcNum, UINT16 wSubSectorStart, UINT16 wSubSectorEnd )
{
    DATAFLASH_ERROR_ENUM eReturn;
    UINT16 i;

    wSubSectorStart = GENERAL_MIN( wSubSectorStart, ( DATAFLASH_NUM_SUBSECTORS - 1 ) );
    wSubSectorEnd = GENERAL_MIN( wSubSectorEnd, ( DATAFLASH_NUM_SUBSECTORS - 1 ) );
    eReturn = DATAFLASH_OK;

//dsa - todo - use Dataflash_EraseSector for the larger blocks
    for( i = wSubSectorStart; ( ( i <= wSubSectorEnd ) && ( eReturn == DATAFLASH_OK ) ); i++)
    {
        eReturn = Dataflash_EraseSubsector( bFlashIcNum, i );
    }

    return eReturn;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn       const char *    DataflashGetErrorAsString( DATAFLASH_ERROR_ENUM eResponse )
//!
//! \brief    Returns a string with a descriptive name for the response code
//!
//! \param[in]  eResponse       An enumerated response code
//!
//! \return   Descriptive string of the error code specified, "UNKNOWN"
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
CHAR * DataflashGetErrorAsString( DATAFLASH_ERROR_ENUM eResponse )
{
    CHAR *pcErrorString = "UNKNOWN";

    UINT32 x = DATAFLASH_ERROR_ARRAY_SIZE;

    if( eResponse < DATAFLASH_ERROR_ARRAY_SIZE )
    {
        pcErrorString = DF_gpcErrorTable[eResponse];
    }

    return pcErrorString;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn       DATAFLASH_ERROR_ENUM DataflashVerifyFlagStatusRegisterError( BOOL fWait, UINT8 *pbStatusByteResult )
//!
//! \brief    Check for the Status register if there was any error during the writting/erasing process
//! \brief    Note: SpiSlaveBusOpen() has been called to open SPI by the calling routine.
//!
//! \param[in]  BOOL fWait     Flag to define if is required to wait until Status register gets a result from the W/R operation
//!
//! \param[in]  UINT8 *pbStatusByteResult   byte returned by the status register.
//!
//! \return   Returns type of error
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
DATAFLASH_ERROR_ENUM DataflashVerifyFlagStatusRegisterError( BOOL fWait, UINT8 *pbStatusByteResult )
{
    DATAFLASH_ERROR_ENUM  eError = DATAFLASH_OK;
    BOOL fSpiPass;
    UINT8               b;

    // #### READ FLAG STATUS REGISTER ####

    SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, TRUE );

    SpiSlaveSendRecvByte( SPI_SLAVE_DATAFLASH, N25Q_COMMAND_RFSR, NULL );
    do
    {
        SpiSlaveSendRecvByte( SPI_SLAVE_DATAFLASH, 0x00, &b );

        if (!fWait)
        {
            break;
        }

    } while( (b & 0x80) == 0x00 );

    SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, FALSE );


    // #### CHECK FOR ERROR DURING OPERATION ####
    if( (b & 0x3A) != 0x00 )
    {
        //if condition for flag ERASE (bit 6) 0- 7
        if( b & 0x20 )
        {
            gtErrorCounter.dwDataflashErrorCounterErase++;
            eError = DATAFLASH_FSR_ERASE_ERROR;
        }
        //if condition for flag PROGRAM ( bit 4 )0-7
        if( b & 0x10 )
        {
            gtErrorCounter.dwDataflashErrorCounterProgram++;
            eError = DATAFLASH_FSR_PROGRAM_ERROR;
        }
        //if condition for flag VPP( bit 3 ) 0- 7
        if( b & 0x08 )
        {
            gtErrorCounter.dwDataflashErrorCounterVpp++;
            eError = DATAFLASH_FSR_VPP_ERROR;
        }
        //if condition for flag Protection( bit 1 ) 0- 7
        if( b & 0x02 )
        {
            gtErrorCounter.dwDataflashErrorCounterProtection++;
            eError = DATAFLASH_FSR_PROTECTION_ERROR;
        }

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // #### AFTER ERROR CHECK, REG CLEARING ####
        SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, TRUE );

        SpiSlaveSendRecvByte( SPI_SLAVE_DATAFLASH, N25Q_COMMAND_CLFSR, NULL );

        SpiSlaveChipSelectAssert( SPI_SLAVE_DATAFLASH, FALSE );
    }

    if (pbStatusByteResult != NULL)
    {
        *pbStatusByteResult = b;
    }

    return eError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn       void Dataflash_GetFlagStatusRegisterError( DATAFLASH_ERROR_COUNTER_STRUCT *ptError )
//!
//! \brief    Return all the Status register error counter
//!
//! \param[in]  ptError     Error counter stucture that will hold the errors of the status register.
//!
//! \return   void
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void Dataflash_GetFlagStatusRegisterError( DATAFLASH_ERROR_COUNTER_STRUCT *ptError )
{
    // #### OBTAINING SEMAPHORE ####
    DataflashSemaphoreAcquire();
    // #### OBTAINING SEMAPHORE ####

    memcpy( ptError, &gtErrorCounter, sizeof( *ptError ) );

    // #### RELEASING SEMAPHORE ####
    DataflashSemaphoreRelease();
    // #### RELEASING SEMAPHORE ####
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \fn       void Dataflash_ResetErrorCounters( void )
//!
//! \brief    Reset all the Status register error counters
//!
//! \return   void
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
void Dataflash_ResetErrorCounters( void )
{
    // #### OBTAINING SEMAPHORE ####
    DataflashSemaphoreAcquire();
    // #### OBTAINING SEMAPHORE ####

    memset( &gtErrorCounter, 0, sizeof( gtErrorCounter ) );

    // #### RELEASING SEMAPHORE ####
    DataflashSemaphoreRelease();
    // #### RELEASING SEMAPHORE ####
}


//////////////////////////////////////////////////////////////////////////////////////////////////
