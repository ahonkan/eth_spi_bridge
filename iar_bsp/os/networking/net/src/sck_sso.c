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
*       sck_sso.c
*
* DESCRIPTION
*
*       This file contains the implementation of NU_Setsockopt.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_Setsockopt
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/*************************************************************************
*
*   FUNCTION
*
*       NU_Setsockopt
*
*   DESCRIPTION
*
*       This function is responsible for setting socket options.  Modeled
*       after the BSD version of this service, the initial implementation
*       only allows broadcasting to be enabled/disabled.  Other options
*       will be added in the future as needed.  The ability to broadcast
*       is enabled by default.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       level                   Specifies the protocol level. The only
*                               valid entries for this parameter are
*                               SOL_SOCKET, IPPROTO_IP, IPPROTO_UDP
*                               and IPPROTO_TCP
*       optname                 Specifies an option:
*
*                               SO_BROADCAST, SO_REUSEADDR, SO_LINGER,
*                               SO_RCVBUF, IP_ADD_MEMBERSHIP,
*                               IP_DROP_MEMBERSHIP, IP_MULTICAST_TTL,
*                               IP_MULTICAST_IF, IP_BROADCAST_IF, IP_TTL
*                               IP_RECVIFADDR, IP_HDRINCL, IP_TOS, IP_PKTINFO,
*                               UDP_NOCHECKSUM, TCP_NODELAY,
*                               TCP_FIRST_PROBE_TIMEOUT, TCP_PROBE_TIMEOUT,
*                               TCP_MAX_PROBES, TCP_MSL, TCP_FIRST_RTO,
*                               TCP_MAX_RTO, TCP_MAX_R2, TCP_MAX_SYN_R2,
*                               TCP_DELAY_ACK, TCP_KEEPALIVE_WAIT,
*                               TCP_KEEPALIVE_R2, TCP_CONGESTION_CTRL,
*                               TCP_CFG_SACK, TCP_WINDOWSCALE,
*                               TCP_RCV_WINDOWSIZE, TCP_TIMESTAMP,
*                               TCP_KEEPINTVL
*
*       *optval                 Pointer to the new value for the option
*       optlen                  The size in bytes of the location pointed
*                               to by optval
*
*   OUTPUTS
*
*       NU_SUCCESS              Indicates the socket option was set
*                               successfully.
*       NU_INVALID_SOCKET       The specified socket descriptor is invalid.
*       NU_INVAL                Either the optval or optlen parameter has
*                               problems.
*       NU_MEM_ALLOC            If there is not enough memory to allocate
*       NU_ADDRINUSE            When the address is in use.
*       NU_INVALID_OPTION       The value specified in the optname
*                               parameter is invalid.
*       NU_INVALID_LEVEL        The value specified in the level
*                               parameter is invalid.
*       NU_UNAVAILABLE          The option must be enabled in the system
*                               before being set on a socket.
*
*************************************************************************/
STATUS NU_Setsockopt(INT socketd, INT level, INT optname, VOID *optval,
                     INT optlen)
{
    STATUS  status;

    /* Obtain the semaphore and validate the socket */
    status = SCK_Protect_Socket_Block(socketd);

    if (status != NU_SUCCESS)
        return (status);

    /* Is the specified level supported. */
    switch (level)
    {
        case SOL_SOCKET:

            switch (optname)
            {
                case SO_BROADCAST:

                    /* Was a value specified and an appropriate size
                     * specified.
                     */
                    if ( (optval == NU_NULL) ||
                         (optlen < (INT16)sizeof(INT16)) )
                    {
                        /* Release the semaphore */
                        SCK_Release_Socket();

                        return (NU_INVAL);
                    }

                    /* Set the new value */
                    SOL_Setsockopt_SO_BROADCAST(socketd, *(INT16*)optval);

                    status = NU_SUCCESS;
                    break;

                case SO_LINGER:

                    /* Verify the value and size. */
                    if ( (optval == NU_NULL) ||
                         (optlen < (INT)(sizeof(struct sck_linger_struct))) )
                    {
                        /* Release the semaphore */
                        SCK_Release_Socket();

                        return (NU_INVAL);
                    }

                    /* Set the new value. */
                    SOL_Setsockopt_SO_LINGER(socketd, *(struct sck_linger_struct*)optval);

                    status = NU_SUCCESS;
                    break;

#if (INCLUDE_SO_REUSEADDR == NU_TRUE)

                case SO_REUSEADDR:

                    /* Was a value specified and an appropriate size
                     * specified.
                     */
                    if ( (optval == NU_NULL) ||
                         (optlen < (INT16)sizeof(INT16)) )
                    {
                        /* Release the semaphore */
                        SCK_Release_Socket();

                        return (NU_INVAL);
                    }

                    SOL_Setsockopt_SO_REUSEADDR(socketd, *(INT16*)optval);

                    status = NU_SUCCESS;
                    break;
#endif

                case SO_RCVBUF:

                    /* Validate optval. */
                    if ( (optval) && (optlen == sizeof(INT32)) )
                    {
                        status = SOL_Setsockopt_SO_RCVBUF(socketd, *(INT32*)optval);
                    }

                    else
                    {
                        status = NU_INVALID_PARM;
                    }

                    break;

                default:

                    status = NU_INVALID_OPTION;
                    break;
            }

            break;

#if (INCLUDE_IPV4 == NU_TRUE)

        case IPPROTO_IP :

            status = IP_Set_Opt(socketd, optname, optval, optlen);
            break;

#endif

#if (INCLUDE_IPV6 == NU_TRUE)

        case IPPROTO_IPV6:

            status = IP6_Set_Opt(socketd, optname, optval, optlen);
            break;

#endif

#if (INCLUDE_TCP == NU_TRUE)

        case IPPROTO_TCP :

            status = TCP_Set_Opt(socketd, optname, optval, optlen);
            break;
#endif

#if (INCLUDE_UDP == NU_TRUE)

        case IPPROTO_UDP: /* UDP Level */

            status = UDP_Set_Opt(socketd,optname,optval,optlen);
            break;

#endif /* (INCLUDE_UDP == NU_TRUE) */

        default :

            status = NU_INVALID_LEVEL;
            break;
    }

    /* Release the semaphore */
    SCK_Release_Socket();

    return (status);

} /* NU_Setsockopt */
