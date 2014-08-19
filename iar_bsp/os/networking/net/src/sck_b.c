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
*
*   FILE NAME
*
*       sck_b.c
*
*   DESCRIPTION
*
*       This file contains the implementation of NU_Bind.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Bind
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/*************************************************************************
*
*   FUNCTION
*
*       NU_Bind
*
*   DESCRIPTION
*
*       This function is responsible for assigning a local address
*       to a socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *myaddr                 Pointer to the server's protocol-specific
*                               address.
*       addrlen                 Parameter is reserved for future use.
*                               A value of zero should be used.
*
*   OUTPUTS
*
*       > = 0                   Socket Descriptor
*       NU_INVALID_PARM         The address of the server is invalid.
*       NU_INVALID_SOCKET       The socket parameter was not a valid
*                               socket value or it had not been previously
*                               allocated via the NU_Socket call.
*       NU_INVALID_PORT         The port number does not equal 0 or the
*                               port number is not unique
*       NU_INVALID_ADDRESS      The address does not match a device
*                               listed
*
*************************************************************************/
STATUS NU_Bind(INT socketd, struct addr_struct *myaddr, INT16 addrlen)
{
    STATUS  return_status;
    struct  sock_struct *sockptr;
    UINT8   *local_addr;

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    /* Validate the address structure pointer. */
    if ( (myaddr == NU_NULL) || ((myaddr->family != SK_FAM_IP)
#if (INCLUDE_IPV6 == NU_TRUE)
         && (myaddr->family != SK_FAM_IP6)
#endif
         ) )
        return (NU_INVALID_PARM);

#endif

    return_status = SCK_Protect_Socket_Block(socketd);

    if (return_status != NU_SUCCESS)
        return (return_status);

    /* Clean up warnings.  This parameter is used for socket compatibility
     * but we are currently not making any use of it.
     */
    UNUSED_PARAMETER(addrlen);

    sockptr = SCK_Sockets[socketd];

    /* Check that the user-specified port is unique.  If the user passed in
     * a port number of 0, do not generate a random port number, since a
     * random port number was already generated when the socket was created.
     */
    if (myaddr->port != 0)
    {
#if (INCLUDE_SO_REUSEADDR == NU_TRUE)
        /* If the SO_REUSEADDR socket option has been set on the socket,
         * check the local address of the socket for an exact match before
         * rejecting the port number as not unique.  Different IP addresses
         * are allowed to bind to the same port.
         */
        if (sockptr->s_options & SO_REUSEADDR_OP)
            local_addr = myaddr->id.is_ip_addrs;

        /* Otherwise, do not allow different IP addresses to bind to
         * the same port.
         */
        else
#endif
            local_addr = NU_NULL;

        /* If the SO_REUSEADDR_OP has been set on the socket and the
         * bound to address is IP_ADDR_ANY, do not check the port
         * for uniqueness.  Otherwise, make sure there is no other
         * port bound to this IP address.
         */
        if (
#if (INCLUDE_SO_REUSEADDR == NU_TRUE)
        	 ((sockptr->s_options & SO_REUSEADDR_OP) &&
              (IP_ADDR(local_addr) == IP_ADDR_ANY)) ||
#endif
             (PRT_Is_Unique_Port_Number(sockptr->s_protocol, myaddr->port,
                                        sockptr->s_family, local_addr,
                                        0) == NU_TRUE) )
        {
            /* Store the port number in the socket structure */
            sockptr->s_local_addr.port_num = myaddr->port;

#if (INCLUDE_UDP == NU_TRUE)

            /* Change the local port number in the UDP port structure */
            if (sockptr->s_protocol == NU_PROTO_UDP)
                UDP_Ports[sockptr->s_port_index]->up_lport = myaddr->port;

#if (INCLUDE_TCP == NU_TRUE)
            else
#endif
#endif

#if (INCLUDE_TCP == NU_TRUE)

            /* Change the local port number in the TCP port structure */
            if (sockptr->s_protocol == NU_PROTO_TCP)
                TCP_Ports[sockptr->s_port_index]->in.port = myaddr->port;
#endif
        }

        else
        {
            /* Release the semaphore */
            SCK_Release_Socket();

            return (NU_INVALID_PORT);
        }
    }

    /* Validate the IP address. Before we bind this address to a socket
     * we must first make sure that there is a device in the system
     * that has this address or that this address is IP_ADDR_ANY. (spr438)
     */
    if (

#if (INCLUDE_IPV6 == NU_TRUE)

    /* Check that a valid IP address was provided to bind to. */
         ((sockptr->s_family == SK_FAM_IP6) &&
          (!IPV6_IS_ADDR_UNSPECIFIED(myaddr->id.is_ip_addrs)) &&
          (!DEV6_Get_Dev_By_Addr(myaddr->id.is_ip_addrs))
#if (INCLUDE_IPV4 == NU_TRUE)
           && ((IPV6_IS_ADDR_V4MAPPED(myaddr->id.is_ip_addrs)) &&
               ((sockptr->s_options & SO_IPV6_V6ONLY) ||
                (!DEV_Get_Dev_By_Addr(&myaddr->id.is_ip_addrs[12]))))
#endif
            )

#if (INCLUDE_IPV4 == NU_TRUE)
           ||
#endif
#endif


#if (INCLUDE_IPV4 == NU_TRUE)

            ((sockptr->s_family == SK_FAM_IP) && (
#if (INCLUDE_IPV6 == NU_TRUE)
             (sockptr->s_options & SO_IPV6_V6ONLY) ||
#endif
             ((*(UINT32 *)myaddr->id.is_ip_addrs != IP_ADDR_ANY) &&
              (!DEV_Get_Dev_By_Addr(myaddr->id.is_ip_addrs)))))
#endif
    )
    {
        /* Release the semaphore */
        SCK_Release_Socket();

        return (NU_INVALID_ADDRESS);
    }

    /* Fill the local portion of the socket descriptor */
    memcpy(sockptr->s_local_addr.ip_num.is_ip_addrs,
           myaddr->id.is_ip_addrs, MAX_ADDRESS_SIZE);

    /* Set a flag indicating that this socket has been bound to a port
     * address combination.
     */
    sockptr->s_flags |= SF_BIND;

    /* Release the semaphore */
    SCK_Release_Socket();

    /* Return the updated socket descriptor to the caller */
    return (socketd);

} /* NU_Bind */
