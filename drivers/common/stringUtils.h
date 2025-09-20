//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!    \file        stringUtils.h
//!    \brief       module header.
//!
//!	   \author
//!	   \date
//!
//!    \notes
//!
//!    \defgroup   TypeOfModule   Group Name
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _STRING_UTILS_H_
#define _STRING_UTILS_H_

//////////////////////////////////////////////////////////////////////////////////////////////////
// Defines
//////////////////////////////////////////////////////////////////////////////////////////////////

#define STRING_UTILS_END_OF_STRING       ('\0')

//////////////////////////////////////////////////////////////////////////////////////////////////
// Public Functions
//////////////////////////////////////////////////////////////////////////////////////////////////

BOOL    StringUtilsStringEqual          ( const CHAR *pcString1, const CHAR *pcString2 );
UINT8   StringUtilsStringToTokenArray   ( CHAR *pcString, UINT16 wStrLen, CHAR cTokenDelimiterChar, BOOL fStringParamDelimiterEnable, CHAR fStringParamDelimiterChar, CHAR **pcTokenArray, UINT16 wMaxTokens );

BOOL    StringToBool                    ( const CHAR *pcString, BOOL *pfBool );
BOOL    StringToByte                    ( const CHAR *pcString, UINT8 *pbByte );
BOOL    StringToU32                     ( const CHAR *pcString, UINT32 *pdwU32 );
BOOL    StringToI32                     ( const CHAR *pcString, INT32 *pidwI32 );
BOOL    StringToFloat                   ( const CHAR *pcString, FLOAT *psgFloat );
BOOL    StringArgStripSymbol            ( CHAR *pcStringArg, UINT16 wStringLen, CHAR cSymbol, CHAR ** pcStringStripped );

#endif // _STRING_UTILS_H_

///////////////////////////////////////// END OF HEADER //////////////////////////////////////////
