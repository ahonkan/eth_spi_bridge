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
*       1213udp.h                                                
*
* COMPONENT
*
*       MIB-II
*
* DESCRIPTION
*
*       This file contains the function declarations for operations
*       on UDP objects.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       None
*
*************************************************************************/

#if ((RFC1213_UDP_INCLUDE == NU_TRUE) && (MIB2_UDP_INCLUDE == NU_TRUE))

#ifndef _1213_UDP_H_
#define _1213_UDP_H_

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

UINT16 udpInDatagrams     (snmp_object_t *, UINT16, VOID *);
UINT16 udpNoPorts         (snmp_object_t *, UINT16, VOID *);
UINT16 udpInErrors        (snmp_object_t *, UINT16, VOID *);
UINT16 udpOutDatagrams    (snmp_object_t *, UINT16, VOID *);
UINT16 udpEntry           (snmp_object_t *, UINT16, VOID *);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif
#endif
