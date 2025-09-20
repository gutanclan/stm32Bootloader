#ifndef _DEV_INFO_H_
#define _DEV_INFO_H_

//////////////////////////////////////////////////////////////////////////////////////////////////
// structures
//////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
    CHAR        *pcMcuType;
    UINT32      dwSiliconVersion;
    UINT32      dwDeviceId;
    UINT32      dwUniqueId[3];
    UINT32      dwFlashMemSizeBytes; //! Program Flash Size in Bytes.
}DevInfoMcuInfoType;

typedef struct
{
    CHAR        *pcCompileDate;
    CHAR        *pcCompileTime;
    UINT8       bVersionMajor;
    UINT8       bVersionMinor;
    UINT8       bVersionPoint;
    UINT8       bVersionLast;
    UINT32      dwSvnRevision;    
    UINT32      dwCrc32Embed;
    UINT32      dwCrc32Calc;
    UINT32      dwMemAddressStart;
    UINT32      dwMemAddressEnd;
    UINT32      dwFwSize;
}DevInfoFwInfoType;

typedef struct
{
    CHAR        *pcCompanyName;
    CHAR        *pcProductName;    
    DevInfoMcuInfoType  tMcuInfo;
    DevInfoFwInfoType   tFwInfo;
}DevInfoProductInfoType;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Functions
//////////////////////////////////////////////////////////////////////////////////////////////////

BOOL                        DevInfoModuleInit           ( void );
DevInfoProductInfoType *    DevInfoGetProductInfo       ( void );

//////////////////////////////////////////////////////////////////////////////////////////////////

#endif