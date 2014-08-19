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
*       ip_tun_i.c
*
* DESCRIPTION
*
*        This file contains functions responsible for the maintenance of
*        IP Tunnels.
*
* DATA STRUCTURES
*
*        IP_Tun_Mib
*
* FUNCTIONS
*
*        IP_Tunnel_Init
*
* DEPENDENCIES
*
*        nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IP_TUNNEL == NU_TRUE)

extern NU_PROTECT                   IP_Tun_Protect;

/* The following structure is used to generate automated config id. */
extern UINT32                       IP_Config_ID_Gen;

/* The following structure maintains the list of created Tunnels. */
extern IP_TUNNEL_INTERFACE_ROOT     IP_Tunnel_Interfaces;

#if ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))

/* The following structure is used for configuring the list of created
   Tunnels. */
extern IP_TUNNEL_CONFIG_ROOT        IP_Tunnel_Configuration;
extern UINT32                       IP_Tun_Commit_Left;

#ifdef SNMP_2_3

mib_element_t                       IP_Tun_Mib[] =
{
#include "networking/ip_tun_oid.h"
};

#endif

#endif /* ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))
        */

/* The following structure maintains the list of available Tunnel
   encapsulation methods. */
extern IP_TUNNEL_PROTOCOL_ROOT      IP_Tunnel_Protocols;

#if (INCLUDE_STATIC_BUILD == NU_TRUE)

extern UINT8                        NET_IP_Tun_Memory_Used[NET_MAX_TUNNEL];
extern UINT8                        NET_IP_Tun_Prot_Used[NET_MAX_TUNNEL_PROT];

#endif /* (INCLUDE_STATIC_BUILD) */

/*************************************************************************
*
* FUNCTION
*
*       IP_Tunnel_Init
*
* DESCRIPTION
*
*       This function initializes the IP Tunnel global structures.
*
* INPUTS
*
*       None.
*
* OUTPUTS
*
*       None.
*
*************************************************************************/
VOID IP_Tunnel_Init(VOID)
{
    /* Initialize config ID generator. */
    IP_Config_ID_Gen = 0;

    /* Clearing the IP Tunnel protection structure. */
    UTL_Zero(&IP_Tun_Protect, sizeof(IP_Tun_Protect) );

    /* Clearing the tunnel interface list. */
    UTL_Zero(&IP_Tunnel_Interfaces, sizeof(IP_TUNNEL_INTERFACE_ROOT) );

#if ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))

    /* Clearing the tunnel configuration list. */
    UTL_Zero(&IP_Tunnel_Configuration, sizeof(IP_TUNNEL_CONFIG_ROOT) );

#endif

    /* Clearing the tunnel protocol list. */
    UTL_Zero(&IP_Tunnel_Protocols, sizeof(IP_TUNNEL_PROTOCOL_ROOT) );

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
    /* Clearing the memory used flags. */
    UTL_Zero(NET_IP_Tun_Memory_Used, sizeof(NET_IP_Tun_Memory_Used) );
    UTL_Zero(NET_IP_Tun_Prot_Used, sizeof(NET_IP_Tun_Prot_Used) );
#endif

#if ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))
    /* Initialize IP Tunnel MIB's commit left count to zero. */
    IP_Tun_Commit_Left = 0;

#ifdef SNMP_2_3

    /* Register IP tunnel MIBs with SNMP. */
    if (SNMP_Mib_Register(IP_Tun_Mib,
                sizeof(IP_Tun_Mib)/sizeof(mib_element_t)) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to register IP Tunnel MIBs with SNMP",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
    }
#endif
#endif /* ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))
        */

} /* IP_Tunnel_Init */

#endif /* (INCLUDE_IP_TUNNEL == NU_TRUE) */
