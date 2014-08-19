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
*       ip_tun_opt.c
*
* DESCRIPTION
*
*       This file contains functions responsible getting and setting
*       IP Tunnel Options.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IP_Get_Tunnel_Opt
*       IP_Set_Tunnel_Opt
*
* DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

/* The following structure maintains the list of created Tunnels. */
extern IP_TUNNEL_INTERFACE_ROOT     IP_Tunnel_Interfaces;

extern NU_PROTECT                   IP_Tun_Protect;

/*************************************************************************
*
* FUNCTION
*
*       IP_Get_Tunnel_Opt
*
* DESCRIPTION
*
*       This function gets a value based on the passed tunnel option.
*
* INPUTS
*
*       *if_index                  Interface index.
*       optname                    The option to be retrieved.
*       *optval                    Memory location where the value is
*                                  to be placed.
*       *optlen                    Size of the memory location. On return
*                                  this will contain the number of bytes
*                                  used.
*
* OUTPUTS
*
*       NU_SUCCESS                 If the request was successful.
*       NU_INVALID_PARM            If parameters was invalid.
*       NU_INVALID_OPTION          If either optval or optlen has problem.
*       NU_NOT_FOUND               If tunnel was not present.
*
*************************************************************************/
STATUS IP_Get_Tunnel_Opt(UINT32 if_index, INT optname, VOID *optval,
                         INT *optlen)
{
    /* Handle to tunnel. */
    IP_TUNNEL_INTERFACE     *tunnel;

    /* Status for returning success or error code. */
    STATUS                  status = NU_SUCCESS;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* If we have valid parameters then proceed, otherwise return error
       code. */
    if ( (optval != NU_NULL) && (optlen != NU_NULL) )
    {
        /* Protecting tunnel interface list against multiple accesses. */
        NU_Protect(&IP_Tun_Protect);

        /* If next tunnel is not required then get the tunnel using
           ifIndex passed in. Otherwise, get next tunnel. */
        if (optname != IP_NEXT_TUNNEL)
        {
            /* Search tunnel from tunnel interface list. */
            tunnel = DLLI_Search_Node(&IP_Tunnel_Interfaces, &if_index,
                                      DLLI_INDEX_32);

            /* If we didn't get the handle to the tunnel interface return
               error code. By doing this we have fulfill the IP_IS_TUNNEL
               request. */
            if (tunnel == NU_NULL)
                status = NU_NOT_FOUND;
        }
        else
        {
            /* Searching next node. */
            tunnel = DLLI_Search_Next_Node(&IP_Tunnel_Interfaces,
                                           &if_index, DLLI_INDEX_32);

            /* If next tunnel found initialize optval and optlen
               passed in. */
            if (tunnel != NU_NULL)
            {
                /* If optval have sufficient memory. */
                if ( (*optlen) >= (INT)(sizeof(UINT32)))
                {
                    (*((UINT32*)optval)) = tunnel->ip_if_index;
                }

                /* If optval hasn't sufficient memory then return
                   error code. */
                else
                {
                    status = NU_INVALID_PARM;
                }

                (*optlen) = sizeof(UINT32);
            }
        }

        /* If request is of IP_NEXT_TUNNEL or IP_IS_TUNNEL then it is
           already fulfilled. For other request proceeds to fulfill. */
        if ( (tunnel) && (optname != IP_IS_TUNNEL) &&
             (optname != IP_NEXT_TUNNEL) && (status == NU_SUCCESS) )
        {
            switch(optname)
            {
            case IP_HOP_LIMIT:      /* ip_hop_limit */

                /* If optval have sufficient memory. */
                if ((*optlen) >= (INT)(sizeof(UINT32)))
                {
                    /* Setting response value. */
                    (*((UINT32 *)optval)) = tunnel->ip_hop_limit;
                }

                /* If optval hasn't sufficient memory then return
                   error code. */
                else
                {
                    status = NU_INVALID_PARM;
                }

                /* Setting response length. */
                (*optlen) = sizeof(UINT32);

                break;

            case IP_LOCAL_ADDRESS:  /* ip_local_addr */

                /* If optval have sufficient memory. */
                if ((*optlen) >= IP_ADDR_LEN)
                {
                    /* Setting response value. */
                    NU_BLOCK_COPY(optval, tunnel->ip_local_address,
                                  IP_ADDR_LEN);
                }

                /* If optval hasn't sufficient memory then return
                   error code. */
                else
                {
                    status = NU_INVALID_PARM;
                }

                /* Setting response length. */
                (*optlen) = IP_ADDR_LEN;

                break;

            case IP_REMOTE_ADDRESS: /* ip_remote_addr*/

                /* If optval have sufficient memory. */
                if ((*optlen) >= IP_ADDR_LEN)
                {
                    /* Setting response value. */
                    NU_BLOCK_COPY(optval, tunnel->ip_remote_address,
                                  IP_ADDR_LEN);
                }

                /* If optval hasn't sufficient memory then return
                   error code. */
                else
                {
                    status = NU_INVALID_PARM;
                }

                /* Setting response length. */
                (*optlen) = IP_ADDR_LEN;

                break;

            case IP_TOS:

                /* If optval have sufficient memory. */
                if ((*optlen) >= (INT)(sizeof(INT8)))
                {
                    if ((tunnel->ip_tos == IP_TOS_FROM_PAYLOAD) ||
                        (tunnel->ip_tos == IP_TOS_FROM_TRAFFIC_CONDITIONER))
                    {
                        /* Setting response value. */
                        (*((INT8*)optval)) = tunnel->ip_tos;
                    }
                    else
                    {
                        /* Setting response value. */
                        (*((INT8*)optval)) =
                                (INT8)(((UINT8)tunnel->ip_tos) >> 2);
                    }
                }

                /* If optval hasn't sufficient memory then return
                   error code. */
                else
                {
                    status = NU_INVALID_PARM;
                }

                /* Setting response length. */
                (*optlen) = sizeof(INT8);

                break;

            case IP_ENCAPS_METHOD:  /* ip_encaps_method. */

                /* If optval have sufficient memory. */
                if ((*optlen) >= (INT)(sizeof(UINT8)))
                {
                    /* Setting response value. */
                    (*((UINT8*)optval)) = tunnel->ip_encaps_method;
                }

                /* If optval hasn't sufficient memory then return
                   error code. */
                else
                {
                    status = NU_INVALID_PARM;
                }

                /* Setting response length. */
                (*optlen) = sizeof(UINT8);

                break;

            case IP_SECURITY:       /* ip_security */

                /* If optval have sufficient memory. */
                if ((*optlen) >= (INT)(sizeof(UINT8)))
                {
                    /* Setting response value. */
                    (*((UINT8*)optval)) = tunnel->ip_security;
                }

                /* If optval hasn't sufficient memory then return
                   error code. */
                else
                {
                    status = NU_INVALID_PARM;
                }

                /* Setting response length. */
                (*optlen) = sizeof(UINT8);

                break;

            case IP_TUNNEL_NAME:    /* ip_tunnel_name */

                /* If optval have sufficient memory. */
                if ((*optlen) >= DEV_NAME_LENGTH)
                {
                    /* Recursive call to NU_Protect is allowed and in
                     * execution of NU_IF_IndexToName there is call to
                     * NU_Protect. So un-protect and leave the critical
                     *  section.
                     */
                    NU_Unprotect();

                    /* Getting device name that is also the tunnel name. */
                    if (NU_IF_IndexToName((INT32)if_index, optval) != NU_NULL)
                    {
                        /* Setting response length. */
                        (*optlen) = (INT)(strlen(optval));
                    }
                }

                /* If optval hasn't sufficient memory then return
                   error code. */
                else
                {
                    status = NU_INVALID_PARM;

                    (*optlen) = DEV_NAME_LENGTH;
                }

                break;

            default:    /* Invalid optname was passed in. */
                status = NU_INVALID_OPTION;
            }
        }

        /* We are out of critical section now. */
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

    /* Return success or error code. */
    return (status);

} /* IP_Get_Tunnel_Opt */

/*************************************************************************
*
* FUNCTION
*
*       IP_Set_Tunnel_Opt
*
* DESCRIPTION
*
*       This function sets a value based on the passed tunnel option.
*
* INPUTS
*
*       *if_index               Interface index.
*       optname                 The option to be set.
*       *optval                 Memory location from where the value is
*                               to be written.
*       optlen                  Size of the memory location to be
*                               written.
*
* OUTPUTS
*
*       NU_SUCCESS              If the request was successful.
*       NU_NOT_FOUND            If tunnel was not present.
*       NU_INVALID_PARM         If parameter was invalid.
*       NU_INVALID_OPTION       If option not supported or illegal.
*
*************************************************************************/
STATUS IP_Set_Tunnel_Opt(UINT32 if_index, INT optname, const VOID *optval,
                         INT optlen)
{
    /* Tunnel interface handle. */
    IP_TUNNEL_INTERFACE *tunnel;

    /* Status for returning success or error code. */
    STATUS              status = NU_SUCCESS;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* If we have valid parameters then proceed otherwise return error
       code. */
    if (optval != NU_NULL)
    {
        /* Protecting tunnel interface list against multiple accesses. */
        NU_Protect(&IP_Tun_Protect);

        /* Getting the handle to the tunnel interface. */
        tunnel = DLLI_Search_Node(&IP_Tunnel_Interfaces, &if_index,
                                  DLLI_INDEX_32);

        if (tunnel != NU_NULL)
        {
            switch (optname)
            {
            case IP_HOP_LIMIT:      /* ip_hop_limit */

                /* If optlen matches then set ip_hop_limit, otherwise
                   return error code. */
                if (optlen == (INT)(sizeof(UINT32)))
                {
                    /* Setting ip_hop_limit. */
                    tunnel->ip_hop_limit = (*((UINT32*)optval));
                }

                else
                {
                    /* Returning error code. */
                    status = NU_INVALID_PARM;
                }

                break;

            case IP_TOS:            /* ip_tos */

                /* If optlen matches then set ip_tos, otherwise return
                   error code. */
                if (optlen == (INT)(sizeof(INT8)))
                {
                    if ( ((*((INT8*)optval)) == IP_TOS_FROM_PAYLOAD) ||
                         ((*((INT8*)optval)) == IP_TOS_FROM_TRAFFIC_CONDITIONER))
                    {
                        /* Setting ip_tos. */
                        tunnel->ip_tos = *((INT8*)optval);
                    }
                    else
                    {
                        /* Setting ip_tos. */
                        tunnel->ip_tos = (INT8)((*((INT8*)optval)) << 2);
                    }
                }

                else
                {
                    /* Returning error code. */
                    status = NU_INVALID_PARM;
                }

                break;

            default:
                /* No other option is supported, therefore return error
                   code. */
                status = NU_INVALID_OPTION;
            }
        }

        /* If we did not get the handle to the tunnel interface then
           return error code. */
        else
        {
            /* Returning error code. */
            status = NU_NOT_FOUND;
        }

        /* We are out of critical section now. */
        NU_Unprotect();
    }

    /* If we have invalid arguments then return error code. */
    else
    {
        /* Returning error code. */
        status = NU_INVALID_PARM;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    /* Returning status. */
    return (status);

} /* IP_Set_Tunnel_Opt */


