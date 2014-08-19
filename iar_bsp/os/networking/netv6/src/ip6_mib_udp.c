/*************************************************************************
*
*              Copyright 2002 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS 
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS 
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILE NAME                                        
*
*        ip6_mib_udp.c                               
*
*   COMPONENT
*
*        IP6 - MIB UDP
*
*   DESCRIPTION
*
*        This files contains the function that maintains 'ipv6UdpTable'.
*
*   DATA STRUCTURES
*
*        None.
*
*   FUNCTIONS
*
*        IP6_MIB_UDP_Compare
*        IP6_MIB_UDP_Get_Socket_Info
*
*   DEPENDENCIES
*
*        nu_net.h
*        ip6_mib.h
*        snmp_api.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/ip6_mib.h"

#if (INCLUDE_SNMP == NU_TRUE)
#include "networking/snmp_api.h"
#endif

#if (INCLUDE_IPV6_UDP_MIB == NU_TRUE)

STATIC INT IP6_MIB_UDP_Compare(UINT8 *, UINT16, UINT32, UINT8 *, UINT16, 
                               UINT32);

/************************************************************************
*
*   FUNCTION
*
*        IP6_MIB_UDP_Compare
*
*   DESCRIPTION
*
*        This function is used to compare two UDP connections for their
*        indexes.
*
*   INPUTS
*
*        *first_addr            First address.
*        first_port             First port.
*        first_if_indx          First interface index.
*        *second_addr           Second address.
*        second_port            Second port.
*        second_if_index        Second interface index.
*
*   OUTPUTS
*
*        0                      When first and second indexes are same.
*        > 0                    When first indexes are greater then that
*                               of second.
*        < 0                    When first indexes are lesser then that 
*                               of second.
*
************************************************************************/
STATIC INT IP6_MIB_UDP_Compare(UINT8 *first_addr, UINT16 first_port,
                               UINT32 first_if_index, UINT8 *second_addr,
                               UINT16 second_port, UINT32 second_if_index)
{
    /* Variable to hold the comparison result. */
    INT         cmp_result;

    /* Compare addresses. */
    cmp_result = memcmp(first_addr, second_addr, MAX_ADDRESS_SIZE);

    /* If addresses are the same. */
    if (cmp_result == 0)
    {
        /* Compare port numbers. */
        if (first_port > second_port)
            cmp_result = 1;
        else if (first_port < second_port)
            cmp_result = -1;

        /* If addresses and ports are the same then compare local address.
         */
        else if (first_if_index > second_if_index)
            cmp_result = 1;
        else if (first_if_index < second_if_index)
            cmp_result = -1;
    }

    /* Return comparison result. */
    return (cmp_result);

} /* IP6_MIB_UDP_Compare */

/************************************************************************
*
*   FUNCTION
*
*        IP6_MIB_UDP_Get_Socket_Info
*
*   DESCRIPTION
*
*        This function is used to get the values in 'ipv6UdpTable'.
*
*   INPUTS
*
*        getflag                Flag when 'true' represent the GET-REQUEST
*                               and when 'false' represent the
*                               GETNEXT-REQUEST.
*        *local_addr            Local address.
*        *local_port            Local port.
*        *local_if_index        Local Interface index.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        Operation successful.
*        IP6_MIB_NOSUCHOBJECT   No instance found.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_MIB_UDP_Get_Socket_Info(UINT8 getflag, UINT8 *local_addr,
                                   UINT16 *local_port, 
                                   UINT32 *local_if_index)
{
    /* Handle to the socket structure. */
    SOCKET_STRUCT       *sockptr = NU_NULL;

    /* Handle to the interface device. */
    DV_DEVICE_ENTRY     *dev;

    /* Index of next socket structure. */
    INT                 next;

    /* Interface index of the next socket structure. */
    UINT32              next_if_index;

    /* Interface index of the current socket structure. */
    UINT32              if_index;

    /* Variable to hold the comparison result. */
    INT                 cmp_result;

    /* Variable to use in for loop. */
    INT                 i;

    /* Status for returning success or error code. */
    UINT16              status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* If this is the GET-Request. */
        if (getflag)
        {
            /* Loop through all the socket structures. */
            for (i = 0; i < NSOCKETS; i++)
            {
                /* If current socket is of family IPv6 and protocol UDP. */
                if ( (SCK_Sockets[i] != NU_NULL) && 
                     (SCK_Sockets[i]->s_family == NU_FAMILY_IP6) &&
                     (SCK_Sockets[i]->s_protocol == (UINT16)NU_PROTO_UDP) )
                {
                    /* Get the handle to the interface device. */
                    dev = DEV6_Get_Dev_By_Addr(SCK_Sockets[i]->s_local_addr.
                                               ip_num.is_ip_addrs);

                    /* If we got the handle to the device then get
                     * interface index. Otherwise set it to 'zero(0)'.
                     */
                    if (dev)
                        if_index = dev->dev_index + 1;
                    else
                        if_index = 0;

                    /* Comparing current socket structure with indexes
                     * passed in.
                     */
                    cmp_result = 
                        IP6_MIB_UDP_Compare(local_addr, (*local_port), 
                                            (*local_if_index),
                                            SCK_Sockets[i]->s_local_addr.
                                            ip_num.is_ip_addrs,
                                            SCK_Sockets[i]->s_local_addr.
                                            port_num, if_index);

                    /* If we found a socket structure with the same
                     * indexes then get the handle to it and break trough
                     * the loop.
                     */
                    if (cmp_result == 0)
                    {
                        /* Get handle to the current socket structure. */
                        sockptr = SCK_Sockets[i];

                        /* Break through the loop. */
                        break;
                    }
                } /* end if UDPv6 socket. */
            } /* end for loop. */
        }/* end if (getflag). */

        /* If this a GET-NEXT request. */
        else
        {
            /* We didn't determine the next socket structure. */
            next = -1;
            next_if_index = 0;

            /* Loop through the all socket structures. */
            for (i = 0; i < NSOCKETS; i++)
            {
                /* If current socket is of type 'UPDv6'. */
                if ( (SCK_Sockets[i] != NU_NULL) && 
                     (SCK_Sockets[i]->s_family == NU_FAMILY_IP6) &&
                     (SCK_Sockets[i]->s_protocol == (UINT16)NU_PROTO_UDP) )
                {
                    /* Get the handle to the interface device. */
                    dev = DEV6_Get_Dev_By_Addr(SCK_Sockets[i]->s_local_addr.
                                               ip_num.is_ip_addrs);

                    /* If we got the handle to the device then get
                     * interface index. Otherwise set it to 'zero(0)'.
                     */
                    if (dev)
                        if_index = dev->dev_index + 1;
                    else
                        if_index = 0;

                    /* Comparing current socket structure with indexes
                     * passed in.
                     */
                    cmp_result = 
                        IP6_MIB_UDP_Compare(local_addr, (*local_port),
                                            (*local_if_index),
                                            SCK_Sockets[i]->s_local_addr.
                                            ip_num.is_ip_addrs,
                                            SCK_Sockets[i]->s_local_addr.
                                            port_num, if_index);

                    /* If current socket has greater indexes as passed in.
                     */
                    if (cmp_result < 0)
                    {
                        /* If we already has a candidate. */
                        if (next >= 0)
                        {
                            /* Compare current socket with previous
                             * candidate.
                             */
                            cmp_result = 
                                IP6_MIB_UDP_Compare(SCK_Sockets[next]->s_local_addr.
                                                    ip_num.is_ip_addrs,
                                                    SCK_Sockets[next]->s_local_addr.
                                                    port_num, next_if_index,
                                                    SCK_Sockets[i]->s_local_addr.
                                                    ip_num.is_ip_addrs,
                                                    SCK_Sockets[next]->s_local_addr.
                                                    port_num, if_index);

                            /* If current socket has lesser indexes then
                             * that candidate then select current socket
                             * as candidate.
                             */
                            if (cmp_result > 0)
                            {
                                next = i;
                                next_if_index = if_index;
                            }
                        }

                        /* If we don't have any candidate yet then select
                         * current socket as candidate.
                         */
                        else
                        {
                            next = i;
                            next_if_index = if_index;
                        }
                    }
                }
            } /* End for loop. */

            /* If we have found the next socket structure. */
            if (next >= 0)
            {
                /* Get the handle to the next socket structure. */
                sockptr = SCK_Sockets[next];

                /* Update the local address passed in. */
                NU_BLOCK_COPY(local_addr, SCK_Sockets[next]->s_local_addr.
                              ip_num.is_ip_addrs, MAX_ADDRESS_SIZE);

                /* Update the remote address passed in. */
                (*local_port) = SCK_Sockets[next]->s_local_addr.port_num;

                /* Update the interface index passed in. */
                (*local_if_index) = next_if_index;
            }
        } /* End If(getflag) else. */

        /* If we have the handle to the handle to the socket structure
         * then return success code.
         */
        if (sockptr)
            status = IP6_MIB_SUCCESS;

        /* Otherwise return error code. */
        else
            status = IP6_MIB_NOSUCHOBJECT;

        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* If we failed to grab the semaphore. */
    else
    {
        NLOG_Error_Log("Failed to grab the semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        /* Return error code. */
        status = IP6_MIB_ERROR;
    }

    /* Return success or error code. */
    return (status);

} /* IP6_MIB_UDP_Get_Socket_Info */

#endif /* (INCLUDE_IPV6_UDP_MIB == NU_TRUE) */
