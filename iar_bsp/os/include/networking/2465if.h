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
*        2465if.h                                    
*
*   DESCRIPTION
*
*        This file contains the declarations of the functions that are
*        responsible for handling SNMP requests on ipv6IfTable and
*        ipv6IfStatsTable.
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

#ifndef IP6_MIB_IF_S_H
#define IP6_MIB_IF_S_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#if ( (INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP6_MIB_IF_GROUP == NU_TRUE) )

/* Object Identifier sub-length. Table entry OID is of length 10
 * having ipv6IfIndex as index this results in sub-length of 11.
 */
#define IP6_IF_SUB_LEN                  11

/* Attribute offset in ipv6IfTable. */
#define IP6_IF_ATTR_OFFSET              (IP6_IF_SUB_LEN - 1)

/* Interface index offset in ipv6IfTable. */
#define IP6_IF_IF_INDEX_OFFSET          (IP6_IF_SUB_LEN)

/* IfIdentifier syntax length. */
#define IP6_IF_IDENTIFIER_SYN_LEN       8

/* Object Identifier sub-length. Table entry OID is of length 10
 * having ipv6IfIndex as index this results in sub-length of 11.
 */
#define IP6_IF_STAT_SUB_LEN             11

/* Attribute offset in ipv6IfTable. */
#define IP6_IF_STAT_ATTR_OFFSET         (IP6_IF_STAT_SUB_LEN - 1)

/* Interface index offset in ipv6IfTable. */
#define IP6_IF_STAT_IF_INDEX_OFFSET     (IP6_IF_STAT_SUB_LEN)

UINT16 ipv6Interfaces(snmp_object_t *obj, UINT16 idlen, VOID *param);
UINT16 ipv6IfTableLastChange(snmp_object_t *obj, UINT16 idlen, VOID *param);
UINT16 ipv6IfEntry(snmp_object_t *obj, UINT16 idlen, VOID *param);
UINT16 ipv6IfStatsEntry(snmp_object_t *obj, UINT16 idlen, VOID *param);

#endif /* ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP6_MIB_IF_GROUP == NU_TRUE)) */

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* IP6_MIB_IF_S_H */
