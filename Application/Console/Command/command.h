//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!	\file		Command.h
//!	\brief		Command Library for the system
//!
//!	\author		Joel Minski [jminski@puracom.com], Puracom Inc.
//! \author     Craig Stickel [cstickel@puracom.com], Puracom Inc.
//! \date		March 5, 2010
//!
//! \details	Provides a command library for the system
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _COMMAND_H_
#define _COMMAND_H_

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Global variables
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "../console.h"

typedef enum
{
    COMMAND_RESULT_UNKNOWN                      = 0,
	COMMAND_RESULT_OK,
    COMMAND_RESULT_ERROR_INVALID_COMMAND,
    COMMAND_RESULT_ERROR_MISMATCH_NUMBER_OF_ARGS,
    COMMAND_RESULT_ERROR_INVALID_ARG_VALUE,
	COMMAND_RESULT_ERROR_PROCESSING_FAIL,

    COMMAND_RESULT_MAX
}CommandResultEnum;

typedef struct
{
    const CHAR			*pcCommand;
	ConsolePortEnum		eConsolePort;
	UINT8				bNumArgs;
	const CHAR			**pcArgArray;
}CommandArgsType;

typedef struct CommandDictionaryType
{
	//! Name of the command
	const char*	pszCommandString;

	//! Pointer to the handler routine for this command
	CommandResultEnum (*pfnCommandHandler)( CommandArgsType *ptCommandArgs );

	//! Pointer to a string describing the function of the command
	const char*	pszDescription;
}CommandDictionaryType;

///////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum
{
	COMMAND_DICTIONARY_1 = 0,

	// ADD MORE DICTIONARIES HERE
	COMMAND_DICTIONARY_MAX
}CommandDictionaryEnum;

///////////////////////////////////////////////////////////////////////////////////////////////////
//// System Global Functions
////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL    CommandIsCommandValid       ( CommandDictionaryType *ptDictionary, CHAR *pszCommandString, UINT32 *pdwDictionaryCommandIndex );
CHAR *  CommandResultGetStringPrt   ( CommandResultEnum eCommandResult );
void    CommandPrintCommandList     ( ConsolePortEnum eConsolePort );

CommandDictionaryType * 	CommandDictionaryGetPointer	    	( CommandDictionaryEnum eDictionary );

///////////////////////////////////////// END OF HEADER //////////////////////////////////////////

#endif // _COMMAND_H_

