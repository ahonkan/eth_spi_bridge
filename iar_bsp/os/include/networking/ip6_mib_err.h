/*************************************************************************
*
*              Copyright 2002 Mentor Graphics Corporation              
*                         All Rights Reserved.                          
*                                                                       
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   
* SUBJECT TO LICENSE TERMS.                                             
*
*************************************************************************/

/*************************************************************************
*
*   FILE NAME                                        
*
*        ip6_mib_err.h                               
*
*   DESCRIPTION
*
*        This file defines the errors codes for IPv6 MIB modules.
*
*   DATA STRUCTURES
*
*        None.
*
*   DEPENDENCIES
*
*        None.
*
************************************************************************/

#ifndef IP6_MIB_ERR_H
#define IP6_MIB_ERR_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* SNMP True. */
#ifdef SNMP_TRUE
#define IP6_MIB_TRUE                SNMP_TRUE
#else
#define IP6_MIB_TRUE                1
#endif

/* SNMP False. */
#ifdef SNMP_FALSE
#define IP6_MIB_FALSE               SNMP_FALSE
#else
#define IP6_MIB_FALSE               2
#endif

/* Error codes. */
#if (INCLUDE_SNMP == NU_TRUE)

#define IP6_MIB_SUCCESS             SNMP_NOERROR
#define IP6_MIB_NOSUCHOBJECT        SNMP_NOSUCHOBJECT
#define IP6_MIB_ERROR               SNMP_ERROR
#define IP6_MIB_WRONGLENGTH         SNMP_WRONGLENGTH
#define IP6_MIB_WRONGVALUE          SNMP_WRONGVALUE
#define IP6_MIB_NOSUCHNAME          SNMP_NOSUCHNAME

#else /* (INCLUDE_SNMP == NU_FALSE) */

#define IP6_MIB_SUCCESS             0
#define IP6_MIB_NOSUCHOBJECT        128
#define IP6_MIB_ERROR               100
#define IP6_MIB_WRONGLENGTH         8
#define IP6_MIB_WRONGVALUE          10
#define IP6_MIB_NOSUCHNAME          2

#endif /* INCLUDE_SNMP */

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* IP6_MIB_ERR_H */
