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
*       vacm_oid.h                                               
*
*   DESCRIPTION
*
*       This file contains OID definitions for VACM MIB.
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

#ifndef VACM_OID_H
#define VACM_OID_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

/*------------------ Context Table -------------------------------------*/
{ {1,3,6,1,6,3,16,1,1,1,1}, 11,  vacmContextEntry , SNMP_DISPLAYSTR , MIB_READ},

/*------------------ Security To Group Table ---------------------------*/
{ {1,3,6,1,6,3,16,1,2,1,3}, 11,  vacmSec2GroupEntry, SNMP_DISPLAYSTR, MIB_READ | MIB_CREATE },
{ {1,3,6,1,6,3,16,1,2,1,4}, 11,  vacmSec2GroupEntry, SNMP_INTEGER ,   MIB_READ | MIB_CREATE },
{ {1,3,6,1,6,3,16,1,2,1,5}, 11,  vacmSec2GroupEntry, SNMP_INTEGER ,   MIB_READ | MIB_CREATE },

/*------------------ Access Table -----------------------------------*/
{ {1,3,6,1,6,3,16,1,4,1,4}, 11,  vacmAccessEntry, SNMP_INTEGER ,   MIB_READ | MIB_CREATE },
{ {1,3,6,1,6,3,16,1,4,1,5}, 11,  vacmAccessEntry, SNMP_DISPLAYSTR, MIB_READ | MIB_CREATE },
{ {1,3,6,1,6,3,16,1,4,1,6}, 11,  vacmAccessEntry, SNMP_DISPLAYSTR, MIB_READ | MIB_CREATE },
{ {1,3,6,1,6,3,16,1,4,1,7}, 11,  vacmAccessEntry, SNMP_DISPLAYSTR, MIB_READ | MIB_CREATE },
{ {1,3,6,1,6,3,16,1,4,1,8}, 11,  vacmAccessEntry, SNMP_INTEGER,    MIB_READ | MIB_CREATE },
{ {1,3,6,1,6,3,16,1,4,1,9}, 11,  vacmAccessEntry, SNMP_INTEGER,    MIB_READ | MIB_CREATE },


/*--------------- Support for Instance Level Granularity----------------*/

{ {1,3,6,1,6,3,16,1,5,1}, 10,    vacmViewSpinLock, SNMP_INTEGER, MIB_READ | MIB_WRITE},

/*------------------ View tree family Table------------------------*/
{ {1,3,6,1,6,3,16,1,5,2,1,3},12, vacmViewTreeEntry, SNMP_DISPLAYSTR, MIB_READ | MIB_CREATE},
{ {1,3,6,1,6,3,16,1,5,2,1,4},12, vacmViewTreeEntry, SNMP_INTEGER,    MIB_READ | MIB_CREATE},
{ {1,3,6,1,6,3,16,1,5,2,1,5},12, vacmViewTreeEntry, SNMP_INTEGER,    MIB_READ | MIB_CREATE},
{ {1,3,6,1,6,3,16,1,5,2,1,6},12, vacmViewTreeEntry, SNMP_INTEGER,    MIB_READ | MIB_CREATE},

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif


