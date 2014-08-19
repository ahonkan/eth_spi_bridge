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
*        2465gen.h                                   
*
*   DESCRIPTION
*
*        This file contains the declarations to the functions that
*        are responsible handling SNMP requests on IPv6 object.
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

#ifndef IP6_MIB_GEN_S_H
#define IP6_MIB_GEN_S_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#if ( (INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IPV6_MIB == NU_TRUE) )

UINT16 ipv6Forwarding(snmp_object_t *obj, UINT16 idlen, VOID *param);
UINT16 ipv6DefaultHopLimit(snmp_object_t *obj, UINT16 idlen, VOID *param);

#endif /* ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IPV6_MIB == NU_TRUE)) */

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* IP6_MIB_GEN_S_H */
