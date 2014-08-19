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
*        ip_tun_mib.h
*
* COMPONENT
*
*        IP - Tunnel MIBs.
*
* DESCRIPTION
*
*        This file contains declaration of the function that are
*        responsible for handling SNMP requests on IP Tunnel MIBs.
*
* DATA STRUCTURES
*
*        None.
*
* DEPENDENCIES
*
*        None.
*
************************************************************************/

#ifndef IP_TUN_MIB_H
#define IP_TUN_MIB_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#if ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))

UINT16 TunnelConfigEntry(snmp_object_t *obj, UINT16 idlen, VOID *param);
UINT16 TunnelIfEntry(snmp_object_t *obj, UINT16 idlen, VOID *param);

#endif /* ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))
        */

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* IP_TUN_MIB_H */
