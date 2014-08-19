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
*       ip_tg.c
*
* DESCRIPTION
*
*        This file contains the getter functions for IP Tunnels.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IP_Get_Tunnel_By_Ip
*       IP_Get_Tunnel_Config
*
* DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

extern NU_PROTECT                   IP_Tun_Protect;

/* The following structure maintains the list of created Tunnels. */
extern IP_TUNNEL_INTERFACE_ROOT     IP_Tunnel_Interfaces;

#if ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))

/* The following structure is used for configuration the list of created
   Tunnels. */
extern IP_TUNNEL_CONFIG_ROOT        IP_Tunnel_Configuration;

#endif /* ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))
        */

/*************************************************************************
*
* FUNCTION
*
*       IP_Get_Tunnel_By_Ip
*
* DESCRIPTION
*
*       This function retrieves the interface index of the first
*       occurrence of a tunnel with the passed IP address.
*
* INPUTS
*
*       *ip_address         Tunnel's local IP.
*       *if_index           Interface Index.
*       flag                Flag to distinguish local/remote IP.
*                           The valid value are:
*                               1) IP_LOCAL_ADDRESS
*                               2) IP_REMOTE_ADDRESS
*
* OUTPUTS
*
*       NU_SUCCESS          If the request was successful.
*       NU_NOT_FOUND        If tunnel was not present.
*       NU_INVALID_PARM     If parameter(s) was/were invalid.
*
*************************************************************************/
STATUS IP_Get_Tunnel_By_Ip(const UINT8 *ip_address, UINT32 *if_index,
                           UINT16 flag)
{
    /* Tunnel pointer for parsing through IP_Tunnel_Interfaces list. */
    IP_TUNNEL_INTERFACE     *tunnel;

    /* Variable to hold comparison result. */
    INT                     cmp_result;

    /* Status for returning. */
    STATUS                  status = NU_SUCCESS;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* If we have valid parameters then proceed, otherwise return error
       code. */
    if ( (ip_address != NU_NULL) && (if_index != NU_NULL) &&
         ( (flag == IP_LOCAL_ADDRESS) || (flag == IP_REMOTE_ADDRESS) ) )
    {
        /* Protecting the global variables against multiple accesses. */
        NU_Protect(&IP_Tun_Protect);

        /* Parsing through tunnel list for finding the required
           tunnel. */
        for (tunnel = IP_Tunnel_Interfaces.ip_flink;
             tunnel;
             tunnel = tunnel->ip_flink)
        {
            if (flag == IP_LOCAL_ADDRESS)
            {
                /* Comparing the local addresses. */
                cmp_result = memcmp(tunnel->ip_local_address,
                                    ip_address, IP_ADDR_LEN);
            }
            else
            {
                cmp_result = memcmp(tunnel->ip_remote_address,
                                    ip_address, IP_ADDR_LEN);
            }

            /* If we have reached at a node with same IP address
               as passed in, then break through the loop as this node
               that is required to be returned. */
            if (cmp_result == 0)
            {
                /* Getting ifIndex. */
                (*if_index) = tunnel->ip_if_index;

                break;
            }
        }

        /* Mark status to not present error code. */
        if (tunnel == NU_NULL)
            status = NU_NOT_FOUND;

        /* We are out of critical section now.  */
        NU_Unprotect();
    }

    /* If we did not have valid parameter then return error code. */
    else
    {
        /* Returning error code. */
        status = NU_INVALID_PARM;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    /* Returning success or error code. */
    return (status);

} /* IP_Get_Tunnel_By_Ip */

#if ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))

/*************************************************************************
*
* FUNCTION
*
*       IP_Get_Tunnel_Config
*
* DESCRIPTION
*
*       This function returns a pointer to tunnel interface from the
*       tunnel configuration list corresponding to the values passed.
*
* INPUTS
*
*       *local_addr             Local address of the tunnel.
*       *remote_addr            Remote address of the tunnel.
*       encaps_method           Encapsulation method to be used.
*       config_id               Configuration ID.
*
* OUTPUTS
*
*       A pointer to the tunnel interface, if one was found. NU_NULL if no
*       tunnel exists for the given interface index.
*
*************************************************************************/
IP_TUNNEL_CONFIG *IP_Get_Tunnel_Config(const UINT8 *local_addr,
                                       const UINT8 *remote_addr,
                                       UINT8 encaps_method,
                                       UINT32 config_id)
{
    /* Config pointer for parsing list and returning. */
    IP_TUNNEL_CONFIG    *ip_tunnel_config;

    INT                 cmp_result;

    ip_tunnel_config =
        IP_Tunnel_Config_Get_Location(&IP_Tunnel_Configuration, local_addr,
                                      remote_addr, encaps_method,
                                      config_id, &cmp_result);

    /* If we did not get the exact match then return NU_NULL. */
    if (cmp_result != 0)
    {
        /* Returning NU_NULL. */
        ip_tunnel_config = NU_NULL;
    }

    /* Returning configuration pointer if found, otherwise NU_NULL. */
    return (ip_tunnel_config);

} /* IP_Get_Tunnel_Config */

#endif /* ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))
        */


