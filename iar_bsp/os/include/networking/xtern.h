/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 1998 - 2007
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILENAME                                               
*
*       xtern.h                                                  
*
*   DESCRIPTION
*
*       This file contains function declarations for the functions
*       defined in snmp_prt.c.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/

#ifndef XTERN_H
#define XTERN_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

#include "networking/snmp_api.h"

VOID    x_insque(VOID *, VOID *);
VOID    x_remque(VOID *);
VOID    *x_realloc(VOID *, UINT32);
INT32   x_gettimeofday(struct x_timeval *, VOID *);
UINT32  x_timemsec(VOID);
UINT32  x_timesec(VOID);
BOOLEAN x_timerinit(VOID);
VOID    x_timeout(VOID(*func)(VOID *), VOID *, INT32);
VOID    x_untimeout(VOID(*func)(VOID *), VOID *);
VOID    x_sprintf(CHAR *, CHAR *);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif


