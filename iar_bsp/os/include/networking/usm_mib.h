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
*       usm_mib.h                                                
*
*   DESCRIPTION
*
*       This file contains macros, data structures and function
*       declarations used by the Message Processing Subsystems.
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

#ifndef USM_MIB_H
#define USM_MIB_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

#if (INCLUDE_MIB_USM == NU_TRUE)

/* Object Identifier sub-length. Table entry OID is of length 10
 * having usmUserEngineID and usmUserName as index this results in
 * sub-length of 11.
 */
#define SNMP_USM_MIB_SUB_LEN           12
#define SNMP_USM_MIB_ATTR_OFFSET       11
#define SNMP_USM_AUTH_PROT_OID_LEN      9
#define SNMP_USM_PRIV_PROT_OID_LEN      9

BOOLEAN USMmib_Mib (snmp_object_t *obj, UINT16 idlen);

/*functions honoring the requests of scalar objects*/
UINT16 usmStatsUnsupportedSecLevels(snmp_object_t *Obj, UINT16 IdLen,
                                    VOID *param);

UINT16 usmStatsNotInTimeWindows(snmp_object_t *Obj, UINT16 IdLen,
                                VOID *param);

UINT16 usmStatsUnknownUserNames(snmp_object_t *Obj, UINT16 IdLen,
                                VOID *param);

UINT16 usmStatsUnknownEngineIDs(snmp_object_t *Obj, UINT16 IdLen,
                                VOID *param);

UINT16 usmStatsWrongDigests(snmp_object_t *Obj, UINT16 IdLen,
                            VOID *param);

UINT16 usmStatsDecryptionErrors(snmp_object_t *Obj, UINT16 IdLen,
                                VOID *param);

UINT16 usmUserSpinLock(snmp_object_t *Obj, UINT16 IdLen, VOID *param);

UINT16 usmUserEntry(snmp_object_t *Obj, UINT16 IdLen, VOID *param);

#endif /* (INCLUDE_MIB_USM == NU_TRUE) */

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif
