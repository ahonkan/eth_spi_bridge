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
*       ip_tp.c
*
* DESCRIPTION
*
*        This file contains functions responsible for the maintenance of
*        IP Tunnel's Protocol list.
*
* DATA STRUCTURES
*
*       IP_Tunnel_Protocols
*       NET_IP_Tun_Prot_Memory
*       NET_IP_Tun_Prot_Used
*
* FUNCTIONS
*
*       IP_Add_Tunnel_Prot
*       IP_Is_Registered_Prot
*       IP_Get_Tunnels_No
*
* DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

/* The following structure maintains the list of available Tunnel
   encapsulation methods. */
IP_TUNNEL_PROTOCOL_ROOT     IP_Tunnel_Protocols;

#if (INCLUDE_STATIC_BUILD == NU_TRUE)

IP_TUNNEL_PROTOCOL          NET_IP_Tun_Prot_Memory[NET_MAX_TUNNEL_PROT];
UINT8                       NET_IP_Tun_Prot_Used[NET_MAX_TUNNEL_PROT];

#endif /* (INCLUDE_STATIC_BUILD == NU_TRUE) */

/*************************************************************************
*
* FUNCTION
*
*       IP_Add_Tunnel_Prot
*
* DESCRIPTION
*
*       This function adds a Tunnel Protocol to the Tunnel Protocol's
*       list. This list is in ascending order of IP encapsulation method
*       type.
*
* INPUTS
*
*       *node                   The new node to be added.
*
* OUTPUTS
*
*       NU_SUCCESS              If the request was successful.
*       NU_INVALID_PARM         If parameter(s) is/are invalid or node
*                               with protocol with same index exists.
*       NU_NO_MEMORY            If memory allocation fails.
*
*************************************************************************/
STATUS IP_Add_Tunnel_Prot(const IP_TUNNEL_PROTOCOL *node)
{
    /* Handle to the IP Tunnel protocol. */
    IP_TUNNEL_PROTOCOL      *protocol = NU_NULL;

#if (INCLUDE_STATIC_BUILD == NU_TRUE)

    /* Variable for using in for loop for search the free memory. */
    UINT16                  loop;

#endif /* (INCLUDE_STATIC_BUILD == NU_TRUE) */

    /* Status for returning success or error code. */
    STATUS                  status;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* If parameter is valid then add the protocol passed in otherwise
       error. */
    if (node != NU_NULL)
    {

#if (INCLUDE_STATIC_BUILD == NU_TRUE)

        status = NU_SUCCESS;

        /* Loop for searching the free memory for allocation. */
        for (loop = 0; loop < NET_MAX_TUNNEL_PROT; loop++)
        {
            /* If we found the free memory then allocate that memory. */
            if (NET_IP_Tun_Prot_Used[loop] == NU_FALSE)
            {
                /* Marking the memory as used. */
                NET_IP_Tun_Prot_Used[loop] = NU_TRUE;

                /* Getting the handle to the memory location. */
                protocol = &NET_IP_Tun_Prot_Memory[loop];

                /* Breaking through the loop. */
                break;
            }
        }

        if (loop >= NET_MAX_TUNNEL_PROT)
        {
            status = NU_NO_MEMORY;
        }

#else /* (INCLUDE_STATIC_BUILD == NU_TRUE) */

        status = NU_Allocate_Memory(MEM_Cached, (VOID **)&protocol,
                                    sizeof(IP_TUNNEL_PROTOCOL),
                                    NU_NO_SUSPEND);

#endif /* (INCLUDE_STATIC_BUILD == NU_TRUE) */

        if (status == NU_SUCCESS)
        {
            /* Copying data of tunnel protocol. */
            NU_BLOCK_COPY(protocol, node, sizeof(IP_TUNNEL_PROTOCOL));

            /* Add node to tunnel protocol list. */
            status = DLLI_Add_Node(&IP_Tunnel_Protocols, protocol,
                                   DLLI_INDEX_8);
        }
    }

    /* If we did not have valid parameter. */
    else
    {
        /* Setting status to error code. */
        status = NU_INVALID_PARM;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    /* Return status. */
    return (status);

} /* IP_Add_Tunnel_Prot */

/*************************************************************************
*
* FUNCTION
*
*       IP_Is_Registered_Prot
*
* DESCRIPTION
*
*       This function determines whether a particular tunneling protocol is
*       registered.
*
* INPUTS
*
*       ip_encaps_method        Encapsulation method type
*
* OUTPUTS
*
*       NU_TRUE                 If node with index passed in is
*                               registered.
*       NU_FALSE                If node with index passed in is
*                               not registered.
*
*************************************************************************/
UINT8 IP_Is_Registered_Prot(UINT8 ip_encaps_method)
{
    /* Handle to the IP Tunnel protocol. */
    IP_TUNNEL_PROTOCOL      *protocol;

    /* Status for returning success or error code. */
    UINT8                   status;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Checking the existence of encapsulation method in the list and
       returning the status. */
    protocol = DLLI_Search_Node(&IP_Tunnel_Protocols, &ip_encaps_method,
                                DLLI_INDEX_8);

    /* If protocol exists then return success code otherwise return
       error code. */
    if (protocol)
    {
        status = NU_TRUE;
    }
    else
    {
        status = NU_FALSE;
    }


    /* Switch back to user mode. */
    NU_USER_MODE();

    /* Returning success or error code. */
    return (status);

} /* IP_Is_Registered_Prot */

/*************************************************************************
*
* FUNCTION
*
*       IP_Get_Tunnels_No
*
* DESCRIPTION
*
*       This function returns the number of tunnels which are using the
*       passed encapsulation method.
*
* INPUTS
*
*       ip_encaps_method        Encapsulation method type
*       return_num_tun          Pointing to memory area where number of
*                               tunnel associated with this protocol
*                               is to be copied.
*
* OUTPUTS
*
*       NU_SUCCESS              If service successfully completed.
*       NU_NOT_FOUND            If protocol does not exists.
*       NU_INVALID_PARM         If parameter are invalid.
*
*************************************************************************/
STATUS IP_Get_Tunnels_No(UINT8 ip_encaps_method, UINT16 *return_num_tun)
{
    /* Handle to protocol. */
    IP_TUNNEL_PROTOCOL      *protocol;

    /* Status for returning success or error code. */
    STATUS                  status = NU_SUCCESS;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* If parameter is valid then proceeds otherwise return error code. */
    if (return_num_tun != NU_NULL)
    {
        /* Getting protocol handle. */
        protocol = DLLI_Search_Node(&IP_Tunnel_Protocols,
                                    &ip_encaps_method, DLLI_INDEX_8);

        /* If protocol is present then get tunnel count otherwise
         * return error code.
         */
        if (protocol)
        {
            /* Getting tunnel count of protocol. */
            (*return_num_tun) = protocol->ip_tunnel_no;

        }
        else
        {
            /* Setting status to error code. */
            status = NU_NOT_FOUND;
        }
    }

    /* If parameter is invalid then return error code. */
    else
    {
        /* Returning error code. */
        status = NU_INVALID_PARM;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    /* Returning success or error code. */
    return (status);

} /* IP_Get_Tunnels_No */


