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
*       1213if.h                                                 
*
*   COMPONENT
*
*       MIB-II
*
*   DESCRIPTION
*
*       This file contains the function declarations for operations
*       on interface objects.
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

#if (RFC1213_IF_INCLUDE == NU_TRUE)

#ifndef _1213_IF_H_
#define _1213_IF_H_

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

UINT16 IfNumber(snmp_object_t *obj, UINT16 idlen, VOID *param);
UINT16 IfTableLastChange(snmp_object_t *obj, UINT16 idlen, VOID *param);

#if ( (INCLUDE_SNMPv2 == NU_TRUE) || (INCLUDE_SNMPv3 == NU_TRUE) )
UINT16 Get_Bulk_1213IfTab(snmp_object_t *obj, UINT16 idlen);
#endif
#endif /* RFC1213_IF_INCLUDE */
#if (RFC1213_IF_INCLUDE == NU_TRUE)


UINT16 Get_If1213Entry(snmp_object_t *obj, UINT8 getflag);
UINT16 Set_If1213Entry(snmp_object_t *obj);
UINT16 If1213Entry(snmp_object_t *obj, UINT16 idlen, VOID *param);

#if ((INCLUDE_IF_EXT == NU_TRUE) && (INCLUDE_IF_EXT_MIB == NU_TRUE))

UINT16 Get_IfXEntry(snmp_object_t *obj, UINT8 getflag);
UINT16 Set_IfXEntry(snmp_object_t *obj);
UINT16 IfXEntry(snmp_object_t *obj, UINT16 idlen, VOID *param);

#endif /* ((INCLUDE_IF_EXT == NU_TRUE) && \
           (INCLUDE_IF_EXT_MIB == NU_TRUE)) */

#if ((INCLUDE_IF_STACK == NU_TRUE) && (INCLUDE_IF_STACK_MIB == NU_TRUE))
UINT16 IfStackLastChange(snmp_object_t *obj, UINT16 idlen, VOID *param);
UINT16 Get_IfStackEntry(snmp_object_t *obj, UINT8 getflag);
UINT16 IfStackEntry(snmp_object_t *obj, UINT16 idlen, VOID *param);
#endif /* ((INCLUDE_IF_STACK == NU_TRUE) && \
           (INCLUDE_IF_STACK_MIB == NU_TRUE)) */

#if (INCLUDE_RCV_ADDR_MIB == NU_TRUE)
UINT16 Get_IfRcvAddressEntry(snmp_object_t *obj, UINT8 getflag);
UINT16 IfRcvAddressEntry(snmp_object_t *obj, UINT16 idlen, VOID *param);
#endif /* (INCLUDE_RCV_ADDR_MIB == NU_TRUE) */

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif
#endif
