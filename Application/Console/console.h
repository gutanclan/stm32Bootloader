//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _CONSOLE_H_
#define _CONSOLE_H_

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Global Variables
//////////////////////////////////////////////////////////////////////////////////////////////////

// typedef imported from command
typedef struct CommandDictionaryType CommandDictionaryType;

typedef enum
{
	CONSOLE_PORT_MAIN	= 0,

	// ADD other ports here

	CONSOLE_PORT_MAX
}ConsolePortEnum;

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Global Functions
//////////////////////////////////////////////////////////////////////////////////////////////////

BOOL    ConsoleModuleInit			                ( void );
BOOL    ConsoleCommandDictionarySet                 ( ConsolePortEnum eConsolePort, CommandDictionaryType *psDictionary );
void    ConsoleCommandDictionaryGet                 ( ConsolePortEnum eConsolePort, CommandDictionaryType **psDictionary );

BOOL    ConsoleEnable				                ( ConsolePortEnum eConsolePort, BOOL fEnable );

BOOL    ConsoleInputProcessingAutoUpdateEnable      ( ConsolePortEnum eConsolePort, BOOL fAutoUpdateEnable );
BOOL    ConsoleInputProcessingAutoUpdateIsEnabled   ( ConsolePortEnum eConsolePort );
void    ConsoleInputProcessingAutoUpdate            ( void );
BOOL    ConsoleInputProcessingManualUpdate          ( ConsolePortEnum eConsolePort, CHAR cChar );
BOOL    ConsoleInputGetChar		                    ( ConsolePortEnum eConsolePort, CHAR *pcChar );
UINT32  ConsoleInputInactivityTimeMilliSeconds      ( ConsolePortEnum eConsolePort );

void    ConsoleTest                                 ( void );

#endif // _CONSOLE_H_

///////////////////////////////////////// END OF HEADER //////////////////////////////////////////
