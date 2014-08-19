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
*       1213sys.h                                                
*
*   COMPONENT
*
*       MIB-II
*
*   DESCRIPTION
*
*       This file contains the function declarations for operations
*       on system objects.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       target.h
*
*************************************************************************/
#include "networking/target.h"

#ifndef _1213_SYS_H_
#define _1213_SYS_H_

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

#if (RFC1213_SYS_INCLUDE == NU_TRUE)

UINT16 sysDescr       (snmp_object_t *, UINT16, VOID *);
UINT16 sysObjectID    (snmp_object_t *, UINT16, VOID *);
UINT16 sysContact     (snmp_object_t *, UINT16, VOID *);
UINT16 sysName        (snmp_object_t *, UINT16, VOID *);
UINT16 sysLocation    (snmp_object_t *, UINT16, VOID *);
UINT16 sysServices    (snmp_object_t *, UINT16, VOID *);

#endif

UINT16 sysUpTime      (snmp_object_t *, UINT16, VOID *);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif
