//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!    \file        dataflash.h
//!    \brief       Serial flash SPI module header.
//!
//!    \author      Jorge Gutierrez, Puracom Inc.
//!    \date        May 8, 2012
//!
//!    \notes
//!
//!    \defgroup   TypeOfModule   Group Name
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef DATAFLASH_H_
#define DATAFLASH_H_

//////////////////////////////////////////////////////////////////////////////////////////////////
//  Local Includes
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "types.h"
#include "Console.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
//  Exported DEFINITIONS
//////////////////////////////////////////////////////////////////////////////////////////////////

// Dataflash function error codes
typedef enum DATAFLASH_RESPONSE_ENUM_
{
    DATAFLASH_OK = 0,                   //! Operation performed successfully
    DATAFLASH_ERROR,                    //! Unknown error
    DATAFLASH_PAGE_INVALID,             //! Specified page is invalid
    DATAFLASH_WRITE_FAILED,             //! Write operation failed
    DATAFLASH_WRITE_BUSY,               //! Write operation busy
    DATAFLASH_WRITE_OVERFLOW,           //! Too many UINT8s to write
    DATAFLASH_SECTOR_INVALID,           //! The specified sector is invalid
    DATAFLASH_IC_INVALID,               //! The specified IC is invalid
    DATAFLASH_INVALID_CONTAINER,        //! Container provided to the routine is bad
    DATAFLASH_FSR_VPP_ERROR,            //! Invalid voltage on the VPP pin during Program and Erase operations
    DATAFLASH_FSR_PROGRAM_ERROR,        //! Programming failure
    DATAFLASH_FSR_ERASE_ERROR,          //! Erase failure
    DATAFLASH_FSR_PROTECTION_ERROR,     //! Protection Failure
    DATAFLASH_SPI_OPEN_ERROR,           //! SPI could not be locked for slave
    DATAFLASH_ERROR_MAX,                //! Marks end of enums
}DATAFLASH_ERROR_ENUM;

// Error counter structure
typedef struct
{
    UINT32  dwDataflashErrorCounterVpp;
    UINT32  dwDataflashErrorCounterProgram;
    UINT32  dwDataflashErrorCounterErase;
    UINT32  dwDataflashErrorCounterProtection;
} DATAFLASH_ERROR_COUNTER_STRUCT;

// Dataflash Info struct, holds ID parameters read from device
typedef struct
{
    UINT8       bManufacturer;      //! Manufacturer ID
    UINT8       bMemoryType;        //! Memory type
    UINT8       bCapacity;          //! Total capacity of the chip, as specified in powers of two
    UINT8       bUIDLen;            //! Number of UINT8s used in the UID
    UINT8       bEDID[ 2 ];         //! Number of UINT8s used in the UID
    UINT8       bFactory[ 14 ];     //! Customized factory data
} DATAFLASH_INFO_STRUCT;

//////////////////////////////////////////////////////////////////////////////////////////////////
//  Exported Symbolic Constants
//////////////////////////////////////////////////////////////////////////////////////////////////

// Dataflash device numbers
// The DataFlash number is the index for sChipSelectLookup[] array when multiple dataflash ICs are used.
typedef enum DATAFLASH_DEVICE_NUMBERS
{
    DATAFLASH0_ID = 0,
} DATAFLASH_DEVICE_ENUM;

//! The total number of Flash ICs available to the driver
#define DATAFLASH_NUM_IC                (1)

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* Organization of the Micron N25Q128x 128 Megabit Serial Flash EEPROM */

//! The size of each Flash IC
#define DATAFLASH_IC_SIZE_BYTES             (128UL*1024UL*1024UL/8)   /* N25Q128x 128 Megabit */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
//! The total number of pages available per Flash IC
#define DATAFLASH_NUM_PAGES                 (65536UL)

//! The size of a single page of Flash, in UINT8s
#define DATAFLASH_PAGE_SIZE_BYTES           (256UL)

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
//! The total number of subsectors available per Flash IC
#define DATAFLASH_NUM_SUBSECTORS            (4096)

//! The size of a subsector, in pages
#define DATAFLASH_SUBSECTOR_SIZE_PAGES      (16)

//! The size of a subsector, in bytes
#define DATAFLASH_SUBSECTOR_SIZE_BYTES      (4096)

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
//! The total number of sectors available per Flash IC
#define DATAFLASH_NUM_SECTORS               (256)

//! The size of a sector, in subsectors
#define DATAFLASH_SECTOR_SIZE_SUBSECTORS    (16)

//! The size of a sector, in pages
#define DATAFLASH_SECTOR_SIZE_PAGES         (256)

//! The size of a sector, in pages
#define DATAFLASH_SECTOR_SIZE_BYTES         (65536UL)

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

//////////////////////////////////////////////////////////////////////////////////////////////////
//  Exported Macros
//////////////////////////////////////////////////////////////////////////////////////////////////

#define DATAFLASH_IS_IC_VALID(x)          ((x) < DATAFLASH_NUM_IC)

#define DATAFLASH_IS_PAGE_VALID(x)        ((x) < DATAFLASH_NUM_PAGES)

#define DATAFLASH_IS_SECTOR_VALID(x)      ((x) < DATAFLASH_NUM_SECTORS)

#define DATAFLASH_IS_SUBSECTOR_VALID(x)   ((x) < DATAFLASH_NUM_SUBSECTORS)


//////////////////////////////////////////////////////////////////////////////////////////////////
//  Exported Routines
//////////////////////////////////////////////////////////////////////////////////////////////////

// Control Routines ----------------------------------------------------------------

BOOL                  Dataflash_Initialize                ( void );
DATAFLASH_ERROR_ENUM  Dataflash_moduleStatus              ( void );
DATAFLASH_ERROR_ENUM  Dataflash_ReadDeviceID              ( UINT8 bFlashIcNum, DATAFLASH_INFO_STRUCT *ptDataflashInfo );
DATAFLASH_ERROR_ENUM  Dataflash_Write                     ( UINT8 bFlashIcNum, UINT32 dwAddress, UINT8 *pbDataBuffer, UINT32 dwLength );
DATAFLASH_ERROR_ENUM  Dataflash_WriteInPage               ( UINT8 bFlashIcNum, UINT32 dwAddress, UINT8 *pbDataBuffer, UINT32 dwLength );
DATAFLASH_ERROR_ENUM  Dataflash_Read                      ( UINT8 bFlashIcNum, UINT32 dwAddress, UINT8 *pbPageData, UINT32 dwLength );
DATAFLASH_ERROR_ENUM  Dataflash_WritePage                 ( UINT8 bFlashIcNum, UINT32 dwPageNum, UINT8 *pbPageData, UINT32 dwLength );
DATAFLASH_ERROR_ENUM  Dataflash_ReadPage                  ( UINT8 bFlashIcNum, UINT32 dwPageNum, UINT8 *pbPageData, UINT32 dwLength );
DATAFLASH_ERROR_ENUM  Dataflash_EraseAll                  ( UINT8 bFlashIcNum );
DATAFLASH_ERROR_ENUM  Dataflash_EraseSector               ( UINT8 bFlashIcNum, UINT16 wSector );
DATAFLASH_ERROR_ENUM  Dataflash_EraseSubsector            ( UINT8 bFlashIcNum, UINT16 wSubsector );
DATAFLASH_ERROR_ENUM  Dataflash_EraseSectorRange          ( UINT8 bFlashIcNum, UINT16 wSectorStart, UINT16 wSectorEnd );
DATAFLASH_ERROR_ENUM  Dataflash_EraseSubSectorRange       ( UINT8 bFlashIcNum, UINT16 wSubSectorStart, UINT16 wSubSectorEnd );

CHAR *       DataflashGetErrorAsString           ( DATAFLASH_ERROR_ENUM );

void         Dataflash_GetFlagStatusRegisterError( DATAFLASH_ERROR_COUNTER_STRUCT *tError );
void         Dataflash_ResetErrorCounters        ( void );
UINT8        Dataflash_TestICs                   ( ConsolePortEnum eConsolePort, BOOL fPrintResult );

#endif /* DATAFLASH_H_ */


//////////////////////////////////////////////////////////////////////////////////////////////////
