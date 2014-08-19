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
*   FILENAME
*
*       tcp_go.c
*
*   DESCRIPTION
*
*       TCP Get Opt - get socket options routine
*
*   DATA STRUCTURES
*
*       NONE
*
*   FUNCTIONS
*
*       TCP_Get_Opt
*
*   DEPENDENCIES
*
*       nu_net.h
*       tcp6.h
*
*************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/tcp6.h"
#endif

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Get_Opt
*
*   DESCRIPTION
*
*       Get TCP options.
*
*   INPUTS
*
*       socketd                 The socket descriptor
*       optname                 The option name:
*
*               TCP_NODELAY             Get the value of the Nagle
*                                       Algorithm.
*               SO_KEEPALIVE            Get the value of the Keep-Alive
*                                       Algorithm.
*               TCP_FIRST_PROBE_TIMEOUT Get the amount of time to delay
*                                       before transmitting the initial
*                                       Zero Window probe.
*               TCP_PROBE_TIMEOUT       Get the maximum amount of time
*                                       to delay between successive
*                                       Zero Window probes.
*               TCP_MAX_PROBES          Get the maximum number of Zero
*                                       Window probes to transmit before
*                                       closing the connection.
*               TCP_MSL                 Get the maximum amount of time
*                                       to wait before reusing the port /
*                                       IP address combination.
*               TCP_FIRST_RTO           Get the amount of time to delay
*                                       before sending the first
*                                       retransmission of unACKed data.
*               TCP_MAX_RTO             Get the maximum amount of time to
*                                       delay between successive
*                                       retransmissions of unACKed data.
*               TCP_MAX_R2              Get the maximum number of
*                                       retransmissions of unACKed data.
*               TCP_MAX_SYN_R2          Get the maximum number of
*                                       retransmissions of unACKed SYNs.
*               TCP_DELAY_ACK           Get the amount of time to delay
*                                       before ACKing incoming data.
*               TCP_KEEPALIVE_WAIT      Get the amount of time to remain
*                                       idle on a connection before
*                                       invoking the Keep-Alive mechanism.
*               TCP_KEEPALIVE_R2        Get the maximum number of Keep-Alive
*                                       packets to transmit before closing
*                                       the connection.
*               TCP_CONGESTION_CTRL     Get the value of the Congestion
*                                       Control algorithm.
*               TCP_CFG_SACK            Get the value of SACK support for
*                                       the socket.
*               TCP_CFG_DSACK           Get the value of D-SACK support for
*                                       the socket.
*               TCP_WINDOWSCALE         Get the value of the Window Scale option
*                                       for the socket.
*               TCP_SND_WINDOWSIZE      Get the foreign window size for the
*                                       socket.
*               TCP_RCV_WINDOWSIZE      Get the local window size for the
*                                       socket.
*               TCP_TIMESTAMP           Get the value of the Timestamp option
*                                       for the socket.
*               TCP_KEEPINTVL           Get the time interval between Keep-
*                                       Alive probes.
*
*       *optval                 Pointer to the option value
*       *optlen                 Pointer to the option length
*
*   OUTPUTS
*
*       Nucleus Status Code
*
*************************************************************************/
STATUS TCP_Get_Opt (INT socketd, INT optname, VOID *optval, INT *optlen)
{
    STATUS      status;

    switch (optname)
    {
    case TCP_NODELAY :

        if (*optlen < (INT)sizeof(INT))
            status = NU_INVAL;

        else
        {
            status =
                TCP_Getsockopt_TCP_NODELAY(socketd, (INT*)optval, optlen);
        }

        break;

    case SO_KEEPALIVE:

        if (*optlen < (INT)sizeof(INT))
            status = NU_INVAL;

        else
        {
            status =
                TCP_Getsockopt_SO_KEEPALIVE(socketd, (INT*)optval, optlen);
        }

        break;

    /* Get the amount of time to delay before transmitting the first Zero
     * Window probe.
     */
    case TCP_FIRST_PROBE_TIMEOUT:

        if (*optlen < (INT)sizeof(INT32))
            status = NU_INVAL;
        else
        {
            status =
                TCP_Getsockopt_TCP_FIRST_PROBE_TIMEOUT(socketd,
                                                       (INT32*)optval);

            if (status == NU_SUCCESS)
                *optlen = sizeof(INT32);
        }

        break;

    /* Get the maximum amount of time to delay between successive Zero Window
     * probes.
     */
    case TCP_PROBE_TIMEOUT:

        if (*optlen < (INT)sizeof(INT32))
            status = NU_INVAL;
        else
        {
            status =
                TCP_Getsockopt_TCP_PROBE_TIMEOUT(socketd,
                                                 (INT32*)optval);

            if (status == NU_SUCCESS)
                *optlen = sizeof(INT32);
        }

        break;

    /* Get the maximum number of Zero Window probes to transmit before
     * closing the connection.
     */
    case TCP_MAX_PROBES:

        if (*optlen < (INT)sizeof(UINT8))
            status = NU_INVAL;
        else
        {
            status =
                TCP_Getsockopt_TCP_MAX_PROBES(socketd, (UINT8*)optval);

            if (status == NU_SUCCESS)
                *optlen = sizeof(UINT8);
        }

        break;

    /* Get the MSL on the socket. */
    case TCP_MSL:

        if (*optlen < (INT)sizeof(UINT32))
            status = NU_INVAL;
        else
        {
            status =
                TCP_Getsockopt_TCP_MSL(socketd, (UINT32*)optval);

            if (status == NU_SUCCESS)
                *optlen = sizeof(UINT32);
        }

        break;

    /* Get the amount of time to delay before sending the first retransmission
     * of an unACKed data packet.
     */
    case TCP_FIRST_RTO:

        if (*optlen < (INT)sizeof(INT32))
            status = NU_INVAL;
        else
        {
            status =
                TCP_Getsockopt_TCP_FIRST_RTO(socketd, (INT32*)optval);

            if (status == NU_SUCCESS)
                *optlen = sizeof(INT32);
        }

        break;

    /* Get the maximum amount of time to delay between successive
     * retransmissions of an unACKed data packet.
     */
    case TCP_MAX_RTO:

        if (*optlen < (INT)sizeof(UINT32))
            status = NU_INVAL;
        else
        {
            status =
                TCP_Getsockopt_TCP_MAX_RTO(socketd, (UINT32*)optval);

            if (status == NU_SUCCESS)
                *optlen = sizeof(UINT32);
        }

        break;

    /* Get the total number of retransmissions to send of an unACKed
     * data packet before closing the connection.
     */
    case TCP_MAX_R2:

        if (*optlen < (INT)sizeof(UINT8))
            status = NU_INVAL;
        else
        {
            status = TCP_Getsockopt_TCP_MAX_R2(socketd, (UINT8*)optval);

            if (status == NU_SUCCESS)
                *optlen = sizeof(UINT8);
        }

        break;

    /* Get the total number of retransmissions to send of an unACKed
     * SYN before closing the connection.
     */
    case TCP_MAX_SYN_R2:

        if (*optlen < (INT)sizeof(UINT8))
            status = NU_INVAL;
        else
        {
            status =
                TCP_Getsockopt_TCP_MAX_SYN_R2(socketd, (UINT8*)optval);

            if (status == NU_SUCCESS)
                *optlen = sizeof(UINT8);
        }

        break;

    /* Get the amount of time to delay when ACKing incoming data. */
    case TCP_DELAY_ACK:

        if (*optlen < (INT)sizeof(UINT32))
            status = NU_INVAL;
        else
        {
            status =
                TCP_Getsockopt_TCP_DELAY_ACK(socketd, (UINT32*)optval);

            if (status == NU_SUCCESS)
                *optlen = sizeof(UINT32);
        }

        break;

    /* Get the amount of time to remain idle on a connection before invoking
     * the Keep-Alive mechanism.
     */
    case TCP_KEEPALIVE_WAIT:

        if (*optlen < (INT)sizeof(UINT32))
            status = NU_INVAL;
        else
        {
            status =
                TCP_Getsockopt_TCP_KEEPALIVE_WAIT(socketd, (UINT32*)optval);

            if (status == NU_SUCCESS)
                *optlen = sizeof(UINT32);
        }

        break;

    /* Get the maximum number of TCP Keep-Alive packets to transmit before
     * closing the connection.
     */
    case TCP_KEEPALIVE_R2:

        if (*optlen < (INT)sizeof(UINT8))
            status = NU_INVAL;
        else
        {
            status =
                TCP_Getsockopt_TCP_KEEPALIVE_R2(socketd, (UINT8*)optval);

            if (status == NU_SUCCESS)
                *optlen = sizeof(UINT8);
        }

        break;

    case TCP_CONGESTION_CTRL:

        if (*optlen < (INT)sizeof(UINT8))
            status = NU_INVAL;

        else
        {
            status =
                TCP_Getsockopt_TCP_CONGESTION_CTRL(socketd, (UINT8*)optval);
        }

        break;

    case TCP_CFG_SACK:

        if (*optlen < (INT)sizeof(UINT8))
            status = NU_INVAL;

        else
        {
            status =
                TCP_Getsockopt_TCP_CFG_SACK(socketd, (UINT8*)optval);
        }

        break;

    case TCP_CFG_DSACK:

        if (*optlen < (INT)sizeof(UINT8))
            status = NU_INVAL;

        else
        {
            status =
                TCP_Getsockopt_TCP_CFG_DSACK(socketd, (UINT8*)optval);
        }

        break;

    case TCP_WINDOWSCALE:

        if (*optlen < (INT)sizeof(INT))
            status = NU_INVAL;

        else
        {
            status =
                TCP_Getsockopt_TCP_WINDOWSCALE(socketd, (INT*)optval);
        }

        break;

    case TCP_SND_WINDOWSIZE:

        if (*optlen < (INT)sizeof(UINT32))
            status = NU_INVAL;

        else
        {
            status =
                TCP_Getsockopt_TCP_SND_WINDOWSIZE(socketd, (UINT32*)optval);
        }

        break;

    case TCP_RCV_WINDOWSIZE:

        if (*optlen < (INT)sizeof(UINT32))
            status = NU_INVAL;

        else
        {
            status =
                TCP_Getsockopt_TCP_RCV_WINDOWSIZE(socketd, (UINT32*)optval);
        }

        break;

    case TCP_TIMESTAMP:

        if (*optlen < (INT)sizeof(UINT8))
            status = NU_INVAL;

        else
        {
            status =
                TCP_Getsockopt_TCP_TIMESTAMP(socketd, (UINT8*)optval);
        }

        break;

    case TCP_KEEPINTVL:

        if (*optlen < (INT)sizeof(UINT32))
        {
            status = NU_INVAL;
        }
        else
        {
            status = TCP_Getsockopt_TCP_KEEPINTVL(socketd, (UINT32*)optval);
        }

        break;

    default :

        status = NU_INVALID_OPTION;
    }

    return (status);

} /* TCP_Get_Opt */
