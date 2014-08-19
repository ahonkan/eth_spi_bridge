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
*       usm_oid.h                                                
*
*   DESCRIPTION
*
*       This file contains OID declarations for USM MIB.
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

#ifndef USM_OID_H
#define USM_OID_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

#if (INCLUDE_MIB_USM == NU_TRUE)

/*Statistics for the User-based security Model*/
{ {1,3,6,1,6,3,15,1,1,1}, 10, usmStatsUnsupportedSecLevels, SNMP_COUNTER , MIB_READ},
{ {1,3,6,1,6,3,15,1,1,2}, 10, usmStatsNotInTimeWindows    , SNMP_COUNTER , MIB_READ},
{ {1,3,6,1,6,3,15,1,1,3}, 10, usmStatsUnknownUserNames    , SNMP_COUNTER , MIB_READ},
{ {1,3,6,1,6,3,15,1,1,4}, 10, usmStatsUnknownEngineIDs    , SNMP_COUNTER , MIB_READ},
{ {1,3,6,1,6,3,15,1,1,5}, 10, usmStatsWrongDigests        , SNMP_COUNTER , MIB_READ},
{ {1,3,6,1,6,3,15,1,1,6}, 10, usmStatsDecryptionErrors    , SNMP_COUNTER , MIB_READ},

/*The usmUser Group*/
{ {1,3,6,1,6,3,15,1,2,1},     10 , usmUserSpinLock,         SNMP_INTEGER    , MIB_READ | MIB_WRITE},
{ {1,3,6,1,6,3,15,1,2,2,1,3}, 12 , usmUserEntry,            SNMP_DISPLAYSTR , MIB_READ},
{ {1,3,6,1,6,3,15,1,2,2,1,4}, 12 , usmUserEntry,            SNMP_OBJECTID   , MIB_READ | MIB_CREATE},
{ {1,3,6,1,6,3,15,1,2,2,1,5}, 12 , usmUserEntry,            SNMP_OBJECTID   , MIB_READ | MIB_CREATE},
{ {1,3,6,1,6,3,15,1,2,2,1,6}, 12 , usmUserEntry,            SNMP_DISPLAYSTR , MIB_READ | MIB_CREATE},
{ {1,3,6,1,6,3,15,1,2,2,1,7}, 12 , usmUserEntry,            SNMP_DISPLAYSTR , MIB_READ | MIB_CREATE},
{ {1,3,6,1,6,3,15,1,2,2,1,8}, 12 , usmUserEntry,            SNMP_OBJECTID   , MIB_READ | MIB_CREATE},
{ {1,3,6,1,6,3,15,1,2,2,1,9}, 12 , usmUserEntry,            SNMP_DISPLAYSTR , MIB_READ | MIB_CREATE},
{ {1,3,6,1,6,3,15,1,2,2,1,10},12 , usmUserEntry,            SNMP_DISPLAYSTR , MIB_READ | MIB_CREATE},
{ {1,3,6,1,6,3,15,1,2,2,1,11},12 , usmUserEntry,            SNMP_DISPLAYSTR , MIB_READ | MIB_CREATE},
{ {1,3,6,1,6,3,15,1,2,2,1,12},12 , usmUserEntry,            SNMP_INTEGER    , MIB_READ | MIB_CREATE},
{ {1,3,6,1,6,3,15,1,2,2,1,13},12 , usmUserEntry,            SNMP_INTEGER    , MIB_READ | MIB_CREATE},

#endif /* (INCLUDE_MIB_USM == NU_TRUE) */

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif
