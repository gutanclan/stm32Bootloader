//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!    \file        queue.h
//!    \brief       queue module header.
//!
//!	   \author
//!	   \date
//!
//!    \notes
//!
//!    \defgroup   TypeOfModule   Group Name
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
// Header include
//////////////////////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include "./types.h"
#include "stringUtils.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
////!
////! \fn     BOOL ConsoleStringEqual( CHAR *pcBuffer, const CHAR *pcString )
////!
////! \brief  compares a buffer with a constant string
////!
////! \return TRUE if equal, FALSE otherwise
////!
////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL StringUtilsStringEqual( const CHAR *pcString1, const CHAR *pcString2 )
{
	BOOL fSuccess = FALSE;

	if
    (
        ( pcString1 != NULL ) &&
        ( pcString2 != NULL )
    )
	{
		UINT8 bStringLen1	= 0;
        UINT8 bStringLen2	= 0;

		bStringLen1 = strlen( (const char *)(pcString1) );
        bStringLen2 = strlen( (const char *)(pcString2) );

		if( bStringLen2 == bStringLen1 )
		{
			if( strncasecmp( (const char*)pcString1, (const char*)pcString2, bStringLen1 ) == 0 )
			{
				fSuccess = TRUE;
			}
		}
	}

	return fSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////!
////! \fn     BOOL ConsoleStringEqual( CHAR *pcBuffer, const CHAR *pcString )
////!
////! \brief  compares a buffer with a constant string
////!
////! \return TRUE if equal, FALSE otherwise
////!
////////////////////////////////////////////////////////////////////////////////////////////////////
UINT8 StringUtilsStringToTokenArray( CHAR *pcString, UINT16 wStrLen, CHAR cTokenDelimiterChar, BOOL fStringParamDelimiterEnable, CHAR fStringParamDelimiterChar, CHAR **pcTokenArray, UINT16 wMaxTokens )
{
    UINT16  wStrIndex       = 0;
    UINT16  wTokenIdxWalker = 0;
    UINT8   bTokenCounter   = 0;

	for( wStrIndex = 0 ; wStrIndex < wStrLen ; wStrIndex++)
    {
        if( bTokenCounter < wMaxTokens )
        {
            if( fStringParamDelimiterEnable )
            {
                // 1.-  check if string token found
                // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
                if( pcString[wStrIndex] == fStringParamDelimiterChar )
                {
                    // check the rest of the original string to make sure there is another '`' following by a ' '<- space or '\0'
                    for( wTokenIdxWalker = wStrIndex ; wTokenIdxWalker < wStrLen ; wTokenIdxWalker++ )
                    {
                        if
                        (
                            ( pcString[wTokenIdxWalker] == fStringParamDelimiterChar ) &&

                            (
                                ( pcString[wTokenIdxWalker+1] == cTokenDelimiterChar ) ||
                                ( pcString[wTokenIdxWalker+1] == STRING_UTILS_END_OF_STRING )
                            )
                        )
                        {
                            ///////////////////////////////////
                            // found end of string token
                            ///////////////////////////////////

                            // add token to token array
                            pcTokenArray[bTokenCounter] = &pcString[wStrIndex];

                            // increment token counter
                            bTokenCounter++;

                            // increment string index
                            wStrIndex = wTokenIdxWalker+1;

                            // break loop
                            break;
                        }
                    }
                }
                // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
            }

            if
            (
                ( pcString[wStrIndex] != cTokenDelimiterChar ) &&
                ( pcString[wStrIndex] != STRING_UTILS_END_OF_STRING )
            )
            {
                // token found

                // loop until finding the end of token
                for( wTokenIdxWalker = wStrIndex ; wTokenIdxWalker < wStrLen ; wTokenIdxWalker++ )
                {
                    if
                    (
                        ( pcString[wTokenIdxWalker+1] == cTokenDelimiterChar ) ||
                        ( pcString[wTokenIdxWalker+1] == STRING_UTILS_END_OF_STRING )
                    )
                    {
                        ///////////////////////////////////
                        // found end of regular token
                        ///////////////////////////////////

                        // add token to token array
                        pcTokenArray[bTokenCounter] = &pcString[wStrIndex];

                        // increment token counter
                        bTokenCounter++;

                        // increment string index
                        wStrIndex = wTokenIdxWalker;

                        // break loop
                        break;
                    }
                }
            }
            else
            {
                // fill up spaces with '\0'
                pcString[wStrIndex] = STRING_UTILS_END_OF_STRING;
            }
        }
        else
        {
            // if token array reached limit, finish operation
            break;
        }
    }

    return bTokenCounter;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL StringToBool( const CHAR *pcString, BOOL *pfBool )
{
    BOOL fSuccess = FALSE;

    if( pcString != NULL )
    {
        if( StringUtilsStringEqual( (const CHAR *)pcString, (const CHAR *)"true" ) )
        {
            if( pfBool != NULL )
            {
                (*pfBool) = TRUE;
            }

            fSuccess = TRUE;
        }
        else if( StringUtilsStringEqual( (const CHAR *)pcString, (const CHAR *)"false" ) )
        {
            if( pfBool != NULL )
            {
                (*pfBool) = FALSE;
            }

            fSuccess = TRUE;
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL StringToByte( const CHAR *pcString, UINT8 *pbByte )
{
    BOOL fSuccess = FALSE;
    INT32 idwRes = 0;
    CHAR *pcEndPtrRes;

    if( pcString != NULL )
    {
        idwRes = strtol( (char *)pcString, (char **)&pcEndPtrRes , 10 );

        if (pcEndPtrRes == pcString)
        {
            // ERROR
            // no digits found
        }
        else
        {
            // check if no error
            if( ( idwRes >= 0 ) && ( idwRes <= 255 ) )
            {
                if( pbByte != NULL )
                {
                    (*pbByte) = (UINT8)idwRes;
                }

                fSuccess = TRUE;
            }
        }
    }

    return fSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL StringToU32( const CHAR *pcString, UINT32 *pdwU32 )
{
    BOOL fSuccess = FALSE;
    UINT32 dwRes = 0;
    CHAR *pcEndPtrRes;
    BOOL fIsNegSignFound = FALSE;

    if( pcString != NULL )
    {
        errno = 0;
        // Negative values are considered valid input and are silently converted to the equivalent unsigned long int value.
        dwRes = strtoul( (char *)pcString, (char **)&pcEndPtrRes , 10 );

        if
        (
            (errno == ERANGE) &&
            (dwRes == ULONG_MAX) ||
            (errno != 0 && dwRes == 0)
        )
        {
            // ERROR
        }
        else
        {
            if (pcEndPtrRes == pcString)
            {
                // ERROR
                // no digits found
            }
            else
            {
                // check if no error
                //check if in the range of the string analyzed there is a negative sign
                fIsNegSignFound = FALSE;
                for( CHAR * cp = pcString ; cp < pcEndPtrRes ; cp++ )
                {
                    CHAR c = (*cp);
                    if( c == '-' )
                    {
                        fIsNegSignFound = TRUE;
                        break;
                    }

                }

                if( fIsNegSignFound == FALSE )
                {
                    if( pdwU32 != NULL )
                    {
                        (*pdwU32) = dwRes;
                    }

                    fSuccess = TRUE;
                }
            }
        }
    }

    return fSuccess;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL StringToI32( const CHAR *pcString, INT32 *pidwI32 )
{
    BOOL fSuccess = FALSE;
    INT32 idwRes = 0;
    CHAR *pcEndPtrRes;

    if( pcString != NULL )
    {
        errno = 0;
        idwRes = strtol( (const char *)pcString, (char **)&pcEndPtrRes , 10 );

        if
        (
            (errno == ERANGE) &&
            (idwRes == LONG_MAX || idwRes == LONG_MIN) ||
            (errno != 0 && idwRes == 0)
        )
        {
            // ERROR
        }
        else
        {
            if (pcEndPtrRes == pcString)
            {
                // ERROR
                // no digits found
            }
            else
            {
                // check if no error
                if( pidwI32 != NULL )
                {
                    (*pidwI32) = idwRes;
                }

                fSuccess = TRUE;
            }
        }
    }

    return fSuccess;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL StringToFloat( const CHAR *pcString, FLOAT *psgFloat )
{
    BOOL fSuccess = FALSE;
    FLOAT sgRes = 0;
    CHAR *pcEndPtrRes;

    if( pcString != NULL )
    {
        sgRes = strtof( (const char *)pcString, (char **)&pcEndPtrRes );

        if (pcEndPtrRes == pcString)
        {
            // ERROR
            // no digits found
        }
        else
        {
            // check if no error
            if( psgFloat != NULL )
            {
                (*psgFloat) = sgRes;
            }

            fSuccess = TRUE;
        }
    }

    return fSuccess;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL StringArgStripSymbol( CHAR *pcStringArg, UINT16 wStringLen, CHAR cSymbol, CHAR ** pcStringStripped )
{
    BOOL fSuccess = FALSE;
    CHAR *pcStart = NULL;
    CHAR *pcEnd = NULL;

    if(( pcStringArg != NULL ) && ( pcStringStripped != NULL ))
    {
        if( wStringLen > 0 )
        {
            pcStart = strchr( (const char *)pcStringArg, cSymbol );
            pcEnd = strchr( (const char *)pcStart+1, cSymbol );

            if( pcEnd != 0 && pcStart != 0 )
            {
                if
                (
                    ( pcStart < (pcStringArg+wStringLen) ) &&
                    ( pcEnd <= (pcStringArg+wStringLen) )
                )
                {
                    (*pcStringStripped) = pcStart+1;

                    // set the last single quote to 0 as termination of string
                    (*pcEnd) = 0;

                    fSuccess = TRUE;
                }
            }
        }
    }

    return fSuccess;
}

/////////////////////////////////////////// END OF SOURCE //////////////////////////////////////////
//

