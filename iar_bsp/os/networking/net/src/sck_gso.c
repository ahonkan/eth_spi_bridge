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
*       sck_gso.c
*
* DESCRIPTION
*
*       This file contains the implementation of NU_Getsockopt.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_Getsockopt
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/*****************************************************************************
*
*   FUNCTION
*
*       NU_Getsockopt
*
*   DESCRIPTION
*
*       This function is responsible for returning the current setting of the
*       various socket flags.  Modeled after the BSD version of this service,
*       the initial implementation only returns the status of the broadcasting
*       option.  Other options will be added in the future as needed.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       level                   Specifies the protocol level. Valid entries
*                               for this parameter are SOL_SOCKET, IPPROTO_IP,
*                               IPPROTO_UDP, and IPPROTO_TCP.
*       optname                 Specifies an option:
*
*                               SO_BROADCAST, SO_LINGER, SO_REUSEADDR,
*                               SO_RCVBUF, IP_HDRINCL, IP_MULTICAST_TTL
*                               IP_MULTICAST_IF, IP_BROADCAST_IF, IP_PKTINFO,
*                               IP_TOS, IP_RECVIFADDR, UDP_NOCHECKSUM,
*                               TCP_NODELAY, TCP_FIRST_PROBE_TIMEOUT,
*                               TCP_PROBE_TIMEOUT, TCP_MAX_PROBES, TCP_MSL,
*                               TCP_FIRST_RTO, TCP_MAX_RTO, TCP_MAX_R2,
*                               TCP_MAX_SYN_R2, TCP_DELAY_ACK,
*                               TCP_KEEPALIVE_WAIT, TCP_KEEPALIVE_R2,
*                               TCP_CONGESTION_CTRL, TCP_CFG_SACK,
*                               TCP_WINDOWSCALE, TCP_RCV_WINDOWSIZE,
*                               TCP_SND_WINDOWSIZE, TCP_TIMESTAMP,
*                               TCP_KEEPINTVL
*
*       optval                  Pointer to the location where the option
*                               status can be written.
*       *optlen                 optlen should contain the size of the
*                               location pointed to by optval
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful retrieval of a socket option.
*       NU_INVALID_PARM         Optval was not valid.
*       NU_INVALID_SOCKET       The specified socket descriptor is invalid.
*       NU_INVALID_OPTION       The value specified in the optname parameter
*                               is invalid.
*       NU_INVAL                optval is a null pointer.
*       NU_INVALID_LEVEL        The value specified in the level parameter
*                               is invalid.
*
******************************************************************************/
STATUS NU_Getsockopt(INT socketd, INT level, INT optname, VOID *optval,
                     INT *optlen)
{
    STATUS  status;

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    if ( (optval == NU_NULL) || (optlen == NU_NULL) )
        return (NU_INVALID_PARM);

#endif

    /* Obtain the semaphore and validate the socket */
    status = SCK_Protect_Socket_Block(socketd);

    if (status != NU_SUCCESS)
        return (status);

    switch (level)
    {
        case SOL_SOCKET : /* Socket Level */

            switch(optname)
            {
                case SO_BROADCAST:

                    SOL_Getsockopt_SO_BROADCAST(socketd, (INT16*)optval,
                                                optlen);
                    status = NU_SUCCESS;

                    break;

                case SO_LINGER:

                    /* Verify there is adequate room to store the struct. */
                    if (*optlen < (INT)(sizeof(struct sck_linger_struct)))
                    {
                        /* Release the semaphore */
                        SCK_Release_Socket();

                        return(NU_INVALID_PARM);
                    }

                    SOL_Getsockopt_SO_LINGER(socketd,
                        (struct sck_linger_struct *)optval, optlen);

                    status = NU_SUCCESS;

                    break;

#if (INCLUDE_SO_REUSEADDR == NU_TRUE)

                case SO_REUSEADDR:

                    SOL_Getsockopt_SO_REUSEADDR(socketd, (INT16*)optval,
                                                optlen);
                    status = NU_SUCCESS;

                    break;

#endif

                case SO_RCVBUF:

                    /* Validate the length of optval. */
                    if (*optlen >= sizeof(INT32))
                    {
                        /* Get the local TCP Window Size of the socket. */
                        status = SOL_Getsockopt_SO_RCVBUF(socketd, (INT32*)optval);

                        if (status == NU_SUCCESS)
                        {
                            /* Set the length of the data being returned
                             * in optval.
                             */
                            *optlen = sizeof(INT32);
                        }
                    }

                    else
                    {
                        status = NU_INVALID_PARM;
                    }

                    break;

                default:

                    /* Release the semaphore */
                    SCK_Release_Socket();

                    return (NU_INVALID_OPTION);
            }

            break;

#if (INCLUDE_IPV4 == NU_TRUE)

        case IPPROTO_IP : /* IP Level. */

            status = IP_Get_Opt(socketd, optname, optval, optlen);
            break;

#endif

#if (INCLUDE_IPV6 == NU_TRUE)

        case IPPROTO_IPV6:

            status = IP6_Get_Opt(socketd, optname, optval, optlen);
            break;
#endif

#if (INCLUDE_TCP == NU_TRUE)

        case IPPROTO_TCP : /* TCP Level. */

            status = TCP_Get_Opt(socketd, optname, optval, optlen);
            break;

#endif /* (INCLUDE_TCP == NU_TRUE) */

#if (INCLUDE_UDP == NU_TRUE)

        case IPPROTO_UDP: /* UDP Level */

            status = UDP_Get_Opt(socketd,optname,optval,optlen);
            break;

#endif /* (INCLUDE_UDP == NU_TRUE) */

        default :
            status = NU_INVALID_LEVEL;
            break;
    }

    /* Release the semaphore */
    SCK_Release_Socket();

    return (status);

} /* NU_Getsockopt */
