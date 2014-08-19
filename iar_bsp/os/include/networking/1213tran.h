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
*       1213tran.h                                               
*
* COMPONENT
*
*       MIB-II
*
* DESCRIPTION
*
*       This file contains the function declarations for operations
*       on transmission objects.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       target.h
*
*************************************************************************/
#ifndef _1213_TRAN_H_
#define _1213_TRAN_H_

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

UINT16 transmission(snmp_object_t *, UINT16, VOID *);

#if (INCLUDE_IP_TUN_MIB == NU_TRUE)
UINT16 TunnelConfigEntry(snmp_object_t *obj, UINT16 idlen, VOID *param);
UINT16 TunnelIfEntry(snmp_object_t *obj, UINT16 idlen, VOID *param);
#endif /* (INCLUDE_IP_TUN_MIB == NU_TRUE) */

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif


