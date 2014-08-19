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
*       xtypes.h                                                 
*
*   DESCRIPTION
*
*       This file contains miscellaneous data structures and macros.
*
*   DATA STRUCTURES
*
*       parm_list_s
*
*   DEPENDENCIES
*
*       target.h
*
************************************************************************/

#ifndef XTYPES_H
#define XTYPES_H

#include "networking/target.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

/*
 * X-defines
 */
#define ON                      NU_TRUE
#define OFF                     NU_FALSE

#define snmp_min(a,b)           (((a) < (b)) ? (a) : (b))

#define SNMP_SPRINTF_NUM_PARMS  8

typedef struct parm_list_s
{
    UINT32  *longptr;
    UINT32  longval;
    INT32   *intptr;
    INT32   intval;
    CHAR    *charptr;
    INT8    charval;
    
    UINT8           snmp_pad[3];
} parm_list_t;

extern  parm_list_t plist[];

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif
