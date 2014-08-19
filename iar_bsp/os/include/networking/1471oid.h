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
*       1471oid.h                                                
*
*   DESCRIPTION
*
*       Object identifiers to be included in the SNMP mib table.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       None
*
************************************************************************/
#ifndef _1471_OID_H
#define _1471_OID_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

/* Status */
{ {OID_LCP_STATUS,1}, 13,  pppLinkStatusEntry, SNMP_INTEGER, MIB_READ},
{ {OID_LCP_STATUS,2}, 13,  pppLinkStatusEntry, SNMP_COUNTER, MIB_READ},
{ {OID_LCP_STATUS,3}, 13,  pppLinkStatusEntry, SNMP_COUNTER, MIB_READ},
{ {OID_LCP_STATUS,4}, 13,  pppLinkStatusEntry, SNMP_COUNTER, MIB_READ},
{ {OID_LCP_STATUS,5}, 13,  pppLinkStatusEntry, SNMP_COUNTER, MIB_READ},
{ {OID_LCP_STATUS,6}, 13,  pppLinkStatusEntry, SNMP_INTEGER, MIB_READ},
{ {OID_LCP_STATUS,7}, 13,  pppLinkStatusEntry, SNMP_INTEGER, MIB_READ},
{ {OID_LCP_STATUS,8}, 13,  pppLinkStatusEntry, SNMP_OCTETSTR, MIB_READ},
{ {OID_LCP_STATUS,9}, 13,  pppLinkStatusEntry, SNMP_OCTETSTR, MIB_READ},
{ {OID_LCP_STATUS,10}, 13, pppLinkStatusEntry, SNMP_INTEGER, MIB_READ},
{ {OID_LCP_STATUS,11}, 13, pppLinkStatusEntry, SNMP_INTEGER, MIB_READ},
{ {OID_LCP_STATUS,12}, 13, pppLinkStatusEntry, SNMP_INTEGER, MIB_READ},
{ {OID_LCP_STATUS,13}, 13, pppLinkStatusEntry, SNMP_INTEGER, MIB_READ},
{ {OID_LCP_STATUS,14}, 13, pppLinkStatusEntry, SNMP_INTEGER, MIB_READ},
{ {OID_LCP_STATUS,15}, 13, pppLinkStatusEntry, SNMP_INTEGER, MIB_READ},

/* Config */
{ {OID_LCP_CONFIG,1}, 13, pppLinkConfigEntry, SNMP_INTEGER, MIB_READ | MIB_WRITE},
{ {OID_LCP_CONFIG,2}, 13, pppLinkConfigEntry, SNMP_OCTETSTR, MIB_READ | MIB_WRITE},
{ {OID_LCP_CONFIG,3}, 13, pppLinkConfigEntry, SNMP_OCTETSTR, MIB_READ | MIB_WRITE},
{ {OID_LCP_CONFIG,4}, 13, pppLinkConfigEntry, SNMP_INTEGER, MIB_READ | MIB_WRITE},
{ {OID_LCP_CONFIG,5}, 13, pppLinkConfigEntry, SNMP_INTEGER, MIB_READ | MIB_WRITE},

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif


