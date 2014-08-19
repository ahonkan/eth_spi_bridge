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
*        ip6_mib_get_bulk_s.h                        
*                                                                      
*   DESCRIPTION                                                          
*                                                                      
*        This file contains the declaration of get bulk routine.
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

#ifndef IP6_MIB_GET_BULK_S_H
#define IP6_MIB_GET_BULK_S_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#ifndef SNMP_2_3
typedef UINT16 (*IPV6_GET_FUNCTION)(snmp_object_t *obj, UINT8 getflag);
UINT16 IPv6_Get_Bulk(snmp_object_t *obj, IPV6_GET_FUNCTION snmp_get_function);

#define IPV6_GET_BULK   IPv6_Get_Bulk
#else
#define IPV6_GET_BULK   SNMP_Get_Bulk
#endif

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* IP6_MIB_GET_BULK_S_H */
