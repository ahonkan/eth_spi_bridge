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
*       bool.h                                                   
*
*   DESCRIPTION
*
*       This file contains the definition for functions to evaluate
*       and set the value of a boolean variable.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       snmp_prt.h
*
*************************************************************************/

#ifndef BOOLSET_H
#define BOOLSET_H

#include "networking/snmp_prt.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

#if (INCLUDE_MIB_RMON1 == NU_TRUE)

typedef UINT32 BooleanSet_t;

#define BooleanSetAllTrue(b)      (b = (BooleanSet_t)0xFFFFFFFFUL)
#define BooleanSetFalse(b,n)      (b &= ~(2 << (n)))
#define BooleanSetTrue(b,n)       (b |= (2 << (n)))
#define BooleanCheckAllTrue(b)    (b == (BooleanSet_t)0xFFFFFFFFUL)
#define BooleanCheck(b,n)         (b & (2 << (n)))

#endif

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif


