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
*        2465no.h                                    
*
*   DESCRIPTION
*
*        This file contains the declaration of the function that is
*        responsible for IPv6 notifications.
*
*   DATA STRUCTURES
*
*        None,
*
*   DEPENDENCIES
*
*        None.
*
************************************************************************/

#ifndef IP6_MIB_NO_S_H
#define IP6_MIB_NO_S_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#if ( (INCLUDE_IP6_MIB_IF_GROUP == NU_TRUE) && (INCLUDE_SNMP == NU_TRUE) )

/* Object OID of 'ipv6IfStateChange'. */
#define IPV6_IF_STATE_CHANGE_OID        {1, 3, 6, 1, 2, 1, 55, 2, 0, 1}

/* Object OID Length of 'ipv6IfStateChange'. */
#define IPV6_IF_STATE_CHANGE_OID_LEN    10

/* Object OID of 'ipv6IfDescr'. */
#define IPV6_IF_DESCR_OID               {1, 3, 6, 1, 2, 1, 55, 1, 5, 1, 2}

/* Object OID length of 'ipv6IfDescr'. */
#define IPV6_IF_DESCR_OID_LEN           12

/* Object OID of 'ipv6IfOperStatus'. */
#define IPV6_IF_OPER_STATUS_OID         {1, 3, 6, 1, 2, 1, 55, 1, 5, 1, 10}

/* Object OID length of 'ipv6IfOperStatus'. */
#define IPV6_IF_OPER_STATUS_OID_LEN     12

VOID IP6_Send_Notification(DV_DEVICE_ENTRY *dev);
#endif

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* IP6_MIB_NO_S_H */
