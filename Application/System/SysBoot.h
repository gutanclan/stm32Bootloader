//////////////////////////////////////////////////////////////////////////////////////////////////
//!
//!
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef SYS_BOOT_H
#define SYS_BOOT_H

//////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum
{
    SYS_BOOT_MODE_UNDEFINED = 0,
    SYS_BOOT_MODE_DISABLE,
    SYS_BOOT_MODE_NORMAL,

    SYS_BOOT_MODE_MAX
}SysBootModeEnum;

typedef enum
{
	SYS_BOOT_STATE_NOT_READY,
	SYS_BOOT_STATE_WAITING,
    SYS_BOOT_STATE_BOOTING,
    SYS_BOOT_STATE_READY,

    SYS_BOOT_STATE_MAX
}SysBootStateEnum;

//////////////////////////////////////////////////////////////////////////////////////////////////

BOOL    SysBootInit                	( void );
void    SysBootUpdate              	( void );

void    SysBootDebugEnable   		( BOOL fEnable );
BOOL  	SysBootIsDebugEnabled  		( void );

SysBootModeEnum    SysBootGetMode   ( void );
BOOL    SysBootSetMode   			( SysBootModeEnum eSysBootMode );
CHAR *  SysBootGetModeName  		( SysBootModeEnum eSysBootMode );

SysBootStateEnum   SysBootGetState  ( void );
CHAR *  SysBootGetStateName  		( SysBootStateEnum eSysBootState );

void    SysBootForceSoftwareReset   ( void );

//////////////////////////////////////////////////////////////////////////////////////////////////

#endif /* POWER_MANAGER_H */

//////////////////////////////////////////////////////////////////////////////////////////////////

