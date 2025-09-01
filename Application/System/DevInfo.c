

#include "string.h"
#include "../Utils/Types.h"
#include "DevInfo.h"

#include "stm32f2xx_conf.h"     // Macro: assert_param() for other STM libraries.

//////////////////////////////////////////////////////////////////////////////////////////////////
// MCU defines
//////////////////////////////////////////////////////////////////////////////////////////////////

#define DEV_INFO_MCU_INFO_MCU_TYPE                  ("STM32F207")
#define DEV_INFO_MCU_INFO_UNIQUE_ID_BASE_REGISTER   (0x1FFF7A10)
#define DEV_INFO_MCU_INFO_FLASH_MEM_SIZE_REGISTER   (0x1FFF7A22)

//////////////////////////////////////////////////////////////////////////////////////////////////
// firmware defines
//////////////////////////////////////////////////////////////////////////////////////////////////

/* Mayor: change when hardware changes. For example: still WD3 but with a new modem or new ADC chip or new Bluetooth chip */
#define DEV_INFO_FIRMWARE_VERSION_MAJOR             (1)
/* Minor: fw Release with new features. For example: same modem driver but originally only supported ftp and now it does http */
#define DEV_INFO_FIRMWARE_VERSION_MINOR             (3)
/* point: bug fixes release. it contains same system main features but small bug fixes have been added */
#define DEV_INFO_FIRMWARE_VERSION_POINT             (0)
/* last: test small tweaks. For example: if major,minor,point are same but a test with say, an Led disabled to reduce power is going to be tested.*/
#define DEV_INFO_FIRMWARE_VERSION_LAST              (0)

//static const UINT32 gdwCrc32 __attribute__ ((section(".crc32"),used)) = 0;
static const UINT32 gdwCrc32 = 0;

#define SVNREV_REV                                  (0)

//////////////////////////////////////////////////////////////////////////////////////////////////
// product defines
//////////////////////////////////////////////////////////////////////////////////////////////////
#define DEV_INFO_PRODUCT_INFO_COMPANY_NAME          ("COMPANY")
#define DEV_INFO_PRODUCT_INFO_PRODUCT_NAME          ("PRODUCT")

//////////////////////////////////////////////////////////////////////////////////////////////////

static DevInfoProductInfoType gtProductInfo;

//////////////////////////////////////////////////////////////////////////////////////////////////

BOOL DevInfoModuleInit( void )
{
    // fill up structure information

    // product
    gtProductInfo.pcCompanyName                 = DEV_INFO_PRODUCT_INFO_COMPANY_NAME;
    gtProductInfo.pcProductName                 = DEV_INFO_PRODUCT_INFO_PRODUCT_NAME;

    // firmware
    gtProductInfo.tFwInfo.pcCompileDate         = __DATE__;
    gtProductInfo.tFwInfo.pcCompileTime         = __TIME__;
    gtProductInfo.tFwInfo.bVersionMajor         = DEV_INFO_FIRMWARE_VERSION_MAJOR;
    gtProductInfo.tFwInfo.bVersionMinor         = DEV_INFO_FIRMWARE_VERSION_MINOR;
    gtProductInfo.tFwInfo.bVersionPoint         = DEV_INFO_FIRMWARE_VERSION_POINT;
    gtProductInfo.tFwInfo.bVersionLast          = DEV_INFO_FIRMWARE_VERSION_LAST;
    gtProductInfo.tFwInfo.dwSvnRevision         = SVNREV_REV;
    gtProductInfo.tFwInfo.dwMemAddressStart     = 0x08000000;
    gtProductInfo.tFwInfo.dwMemAddressEnd       = ( (UINT32)&gdwCrc32 ) + sizeof(gdwCrc32) - 1;
    gtProductInfo.tFwInfo.dwFwSize              = gtProductInfo.tFwInfo.dwMemAddressEnd - gtProductInfo.tFwInfo.dwMemAddressStart + 1;
    gtProductInfo.tFwInfo.dwCrc32Embed          = 0xffffffff;//GeneralByteReverse32(gdwCrc32) ^ 0xffffffff;
    gtProductInfo.tFwInfo.dwCrc32Calc           = 0xffffffff;//GeneralCalcCrc32( (uint8_t*)gtProductInfo.tFwInfo.dwMemAddressStart, (UINT32)&gdwCrc32 - (UINT32)gtProductInfo.tFwInfo.dwMemAddressStart, TRUE);

    // mcu
    gtProductInfo.tMcuInfo.pcMcuType            = DEV_INFO_MCU_INFO_MCU_TYPE;
    gtProductInfo.tMcuInfo.dwSiliconVersion     = DBGMCU_GetREVID();
    gtProductInfo.tMcuInfo.dwDeviceId           = DBGMCU_GetDEVID();
    /* STM32F207 Unique device ID registers (96 bits) */
    gtProductInfo.tMcuInfo.dwUniqueId[0]        = *((UINT32*)(DEV_INFO_MCU_INFO_UNIQUE_ID_BASE_REGISTER+0x00));
    gtProductInfo.tMcuInfo.dwUniqueId[1]        = *((UINT32*)(DEV_INFO_MCU_INFO_UNIQUE_ID_BASE_REGISTER+0x04));
    gtProductInfo.tMcuInfo.dwUniqueId[2]        = *((UINT32*)(DEV_INFO_MCU_INFO_UNIQUE_ID_BASE_REGISTER+0x08));
    gtProductInfo.tMcuInfo.dwFlashMemSizeBytes  = (*((UINT16*)(DEV_INFO_MCU_INFO_FLASH_MEM_SIZE_REGISTER))) * 1024;
}

DevInfoProductInfoType * DevInfoGetProductInfo( void )
{
    return &gtProductInfo;
}
