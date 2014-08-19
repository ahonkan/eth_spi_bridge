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
*       no_mib.h                                                 
*
*   DESCRIPTION
*
*       This file contains implementation of Notification MIBs.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/

#ifndef NO_MIB_H
#define NO_MIB_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

/* Sub-length notify table. */
#define SNMP_NO_MIB_NOTIFY_SUB_LEN          11

/* Attribute offset for notify table. */
#define SNMP_NO_MIB_NOTIFY_ATTR_OFFSET      10

/* Sub-length for profile table. */
#define SNMP_NO_MIB_FILTER_PRO_SUB_LEN      11

/* Attribute offset for profile table. */
#define SNMP_NO_MIB_FILTER_PRO_ATTR_OFFSET  10

/* Sub-length for filter table. */
#define SNMP_NO_MIB_FILTER_SUB_LEN          11

/* Attribute offset for filter table. */
#define SNMP_NO_MIB_FILTER_ATTR_OFFSET      10

/* Notify table. */
UINT16 snmpNotifyEntry(snmp_object_t *obj, UINT16 idlen, VOID *param);

/* Filter profile table. */
UINT16 snmpNotifyFiltProfEntry(snmp_object_t *obj, UINT16 idlen,
                                    VOID *param);

/* Filter table. */
UINT16 snmpNotifyFilterEntry(snmp_object_t *obj, UINT16 idlen,
                             VOID *param);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif

