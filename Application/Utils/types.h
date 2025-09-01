/*  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *
 *
 * \file    Types.h
 * \brief   Data types
 *
 *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  */

#ifndef _TYPES_H_
#define _TYPES_H_

/** \brief
 *  Basic types
 */

// Size: 1 byte
typedef unsigned char           BOOL;
typedef unsigned char           CHAR;
typedef unsigned char           UINT8;
typedef signed char             INT8;

// Size: 2 bytes
typedef unsigned short          UINT16;
typedef signed short            INT16;

// Size: 4 bytes
typedef unsigned long           UINT32;
typedef signed long             INT32;
typedef float                   FLOAT;

// size 8 bytes
typedef double                  DOUBLE;
typedef unsigned long long      UINT64;
typedef signed long long        INT64;

/** \brief
 *  Basic defines
 */

#ifndef NULL
#define NULL                    (void*)(0)
#endif

#define TRUE                    (1)
#define FALSE                   (0)

#endif /*_TYPES_H_*/

