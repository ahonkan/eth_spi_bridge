/*************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
* FILE NAME
*
*        ip_tun_oid.h
*
* COMPONENT
*
*        IP - Tunnel MIBs
*
* DESCRIPTION
*
*        This file contains declarations for each of the MIB objects in IP
*        Tunnel MIBs.
*
* DATA STRUCTURES
*
*        [global component data structures defined in this file]
*
* FUNCTIONS
*
*        None.
*
* DEPENDENCIES
*
*        None.
*
************************************************************************/

#ifndef IP_TUN_OID_H
#define IP_TUN_OID_H

#if ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))

/* Tunnel If Table */
{ {1,3,6,1,2,1,10,131,1,1,1,1,1}, 13, TunnelIfEntry, SNMP_OCTETSTR, MIB_READ},
{ {1,3,6,1,2,1,10,131,1,1,1,1,2}, 13, TunnelIfEntry, SNMP_OCTETSTR, MIB_READ},
{ {1,3,6,1,2,1,10,131,1,1,1,1,3}, 13, TunnelIfEntry, SNMP_INTEGER, MIB_READ},
{ {1,3,6,1,2,1,10,131,1,1,1,1,4}, 13, TunnelIfEntry, SNMP_INTEGER, MIB_READ | MIB_WRITE},
{ {1,3,6,1,2,1,10,131,1,1,1,1,5}, 13, TunnelIfEntry, SNMP_INTEGER, MIB_READ},
{ {1,3,6,1,2,1,10,131,1,1,1,1,6}, 13, TunnelIfEntry, SNMP_INTEGER, MIB_READ | MIB_WRITE},

/* Tunnel Config Table */
{ {1,3,6,1,2,1,10,131,1,1,2,1,5}, 13, TunnelConfigEntry, SNMP_INTEGER, MIB_READ},
{ {1,3,6,1,2,1,10,131,1,1,2,1,6}, 13, TunnelConfigEntry, SNMP_INTEGER, MIB_READ | MIB_CREATE},

#endif /* ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE)) */

#endif /* IP_TUN_OID_H */
