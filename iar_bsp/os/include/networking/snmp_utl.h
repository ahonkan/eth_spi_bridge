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
*       snmp_utl.h                                                    
*
*   DESCRIPTION
*
*       This file contains function declarations for the general utility
*       functions used by SNMP.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       target.h
*       xtypes.h
*
************************************************************************/

#ifndef SNMP_UTL_H
#define SNMP_UTL_H

#include "networking/target.h"
#include "networking/xtypes.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

CHAR    UTL_Hex_To_Char(UINT8 hex_num);
INT     UTL_Admin_String_Cmp(const CHAR *left_side, const CHAR *right_side);


#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif /* SNMP_UTL_H */
