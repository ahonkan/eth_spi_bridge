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
*       tcp_so.c
*
*   DESCRIPTION
*
*       TCP Set Opt - socket options routine
*
*   DATA STRUCTURES
*
*       NONE
*
*   FUNCTIONS
*
*       TCP_Set_Opt
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
*       TCP_Set_Opt
*
*   DESCRIPTION
*
*       Set TCP options.
*
*   INPUTS
*
*       socketd                 The socket descriptor
*       optname                 The option name:
*
*               TCP_NODELAY             Toggle the Nagle Algorithm.
*               SO_KEEPALIVE            Toggle the Keep-Alive Algorithm.
*               TCP_FIRST_PROBE_TIMEOUT Set the amount of time to delay
*                                       before transmitting the initial
*                                       Zero Window probe.
*               TCP_PROBE_TIMEOUT       Set the maximum amount of time
*                                       to delay between successive
*                                       Zero Window probes.
*               TCP_MAX_PROBES          Set the maximum number of Zero
*                                       Window probes to transmit before
*                                       closing the connection.
*               TCP_MSL                 Set the maximum amount of time
*                                       to wait before reusing the port /
*                                       IP address combination.
*               TCP_FIRST_RTO           Set the amount of time to delay
*                                       before sending the first
*                                       retransmission of unACKed data.
*               TCP_MAX_RTO             Set the maximum amount of time to
*                                       delay between successive
*                                       retransmissions of unACKed data.
*               TCP_MAX_R2              Set the maximum number of
*                                       retransmissions of unACKed data.
*               TCP_MAX_SYN_R2          Set the maximum number of
*                                       retransmissions of unACKed SYNs.
*               TCP_DELAY_ACK           Set the amount of time to delay
*                                       before ACKing incoming data.
*               TCP_KEEPALIVE_WAIT      Set the amount of time to remain
*                                       idle on a connection before
*                                       invoking the Keep-Alive mechanism.
*               TCP_KEEPALIVE_R2        Set the maximum number of Keep-Alive
*                                       packets to transmit before closing
*                                       the connection.
*               TCP_CONGESTION_CTRL     Toggle the Congestion Control
*                                       algorithm.
*               TCP_CFG_SACK            Toggle SACK support on the socket.
*               TCP_CFG_DSACK           Toggle D-SACK support on the socket.
*               TCP_WINDOWSCALE         Toggle Window Scale option support on
*                                       the socket.
*               TCP_RCV_WINDOWSIZE      Configure the local window size
*                                       for the socket.
*               TCP_TIMESTAMP           Toggle Timestamp option support on
*                                       the socket.
*               TCP_KEEPINTVL           Set the time interval between Keep-
*                                       Alive probes.
*
*       *optval                 Pointer to the option value
*       optlen                  The option length
*
*   OUTPUTS
*
*       Nucleus Status Code
*
*************************************************************************/
STATUS TCP_Set_Opt(INT socketd, INT optname, const VOID *optval, INT optlen)
{
    STATUS      status;

    /* No NULL pointers allowed. */
    if (!optval)
        return (NU_INVAL);

    switch (optname)
    {
    case TCP_NODELAY :

        /* Validate the parameters. */
        if (optlen < (INT)sizeof(UINT8))
            status = NU_INVAL;
        else
        {
            status =
                TCP_Setsockopt_TCP_NODELAY(socketd, (UINT8)(*((INT*)optval)));
        }

        break;

    case SO_KEEPALIVE:

        if (optlen < (INT)sizeof(UINT8))
            status = NU_INVAL;
        else
        {
            status =
                TCP_Setsockopt_SO_KEEPALIVE(socketd, (UINT8)(*((INT*)optval)));
        }

        break;

    /* Set the amount of time to delay before transmitting the first Zero
     * Window probe.
     */
    case TCP_FIRST_PROBE_TIMEOUT:

        if (optlen < (INT)sizeof(INT32))
            status = NU_INVAL;
        else
        {
            status =
                TCP_Setsockopt_TCP_FIRST_PROBE_TIMEOUT(socketd,
                                                       (INT32)(*((INT*)optval)));
        }

        break;

    /* Set the maximum amount of time to delay between successive Zero Window
     * probes.
     */
    case TCP_PROBE_TIMEOUT:

        if (optlen < (INT)sizeof(INT32))
            status = NU_INVAL;
        else
        {
            status =
                TCP_Setsockopt_TCP_PROBE_TIMEOUT(socketd,
                                                 (INT32)(*((INT*)optval)));
        }

        break;

    /* Set the maximum number of Zero Window probes to transmit before
     * closing the connection.
     */
    case TCP_MAX_PROBES:

        if (optlen < (INT)sizeof(UINT8))
            status = NU_INVAL;
        else
        {
            status =
                TCP_Setsockopt_TCP_MAX_PROBES(socketd,
                                              (UINT8)(*((INT*)optval)));
        }

        break;

    /* Set the MSL on the socket. */
    case TCP_MSL:

        if (optlen < (INT)sizeof(UINT32))
            status = NU_INVAL;
        else
        {
            status =
                TCP_Setsockopt_TCP_MSL(socketd, (UINT32)(*((INT*)optval)));
        }

        break;

    /* Set the amount of time to delay before sending the first retransmission
     * of an unACKed data packet.
     */
    case TCP_FIRST_RTO:

        if (optlen < (INT)sizeof(INT32))
            status = NU_INVAL;
        else
        {
            status =
                TCP_Setsockopt_TCP_FIRST_RTO(socketd,
                                             (INT32)(*((INT*)optval)));
        }

        break;

    /* Set the maximum amount of time to delay between successive
     * retransmissions of an unACKed data packet.
     */
    case TCP_MAX_RTO:

        if (optlen < (INT)sizeof(UINT32))
            status = NU_INVAL;
        else
        {
            status =
                TCP_Setsockopt_TCP_MAX_RTO(socketd,
                                           (UINT32)(*((INT*)optval)));
        }

        break;

    /* Set the total number of retransmissions to send of an unACKed
     * data packet before closing the connection.
     */
    case TCP_MAX_R2:

        if (optlen < (INT)sizeof(UINT8))
            status = NU_INVAL;
        else
        {
            status =
                TCP_Setsockopt_TCP_MAX_R2(socketd,
                                          (UINT8)(*((INT*)optval)));
        }

        break;

    /* Set the total number of retransmissions to send of an unACKed
     * SYN before closing the connection.
     */
    case TCP_MAX_SYN_R2:

        if (optlen < (INT)sizeof(UINT8))
            status = NU_INVAL;
        else
        {
            status =
                TCP_Setsockopt_TCP_MAX_SYN_R2(socketd,
                                              (UINT8)(*((INT*)optval)));
        }

        break;

    /* Set the amount of time to delay when ACKing incoming data. */
    case TCP_DELAY_ACK:

        if (optlen < (INT)sizeof(INT32))
            status = NU_INVAL;
        else
        {
            status =
                TCP_Setsockopt_TCP_DELAY_ACK(socketd,
                                             (INT32)(*((INT*)optval)));
        }

        break;

    /* Set the amount of time to remain idle on a connection before invoking
     * the Keep-Alive mechanism.
     */
    case TCP_KEEPALIVE_WAIT:

        if (optlen < (INT)sizeof(INT32))
            status = NU_INVAL;
        else
        {
            status =
                TCP_Setsockopt_TCP_KEEPALIVE_WAIT(socketd,
                                                  (INT32)(*((INT*)optval)));
        }

        break;

    /* Set the maximum number of TCP Keep-Alive packets to transmit before
     * closing the connection.
     */
    case TCP_KEEPALIVE_R2:

        if (optlen < (INT)sizeof(UINT8))
            status = NU_INVAL;
        else
        {
            status =
                TCP_Setsockopt_TCP_KEEPALIVE_R2(socketd,
                                                (UINT8)(*((INT*)optval)));
        }

        break;

    case TCP_CONGESTION_CTRL:

        if (optlen < (INT)sizeof(UINT8))
            status = NU_INVAL;
        else
        {
            status =
                TCP_Setsockopt_TCP_CONGESTION_CTRL(socketd,
                                                   (UINT8)(*((INT*)optval)));
        }

        break;

    case TCP_CFG_SACK:

        if (optlen < (INT)sizeof(UINT8))
            status = NU_INVAL;
        else
        {
            status =
                TCP_Setsockopt_TCP_CFG_SACK(socketd, (UINT8)(*((INT*)optval)));
        }

        break;

    case TCP_CFG_DSACK:

        if (optlen < (INT)sizeof(UINT8))
            status = NU_INVAL;
        else
        {
            status =
                TCP_Setsockopt_TCP_CFG_DSACK(socketd, (UINT8)(*((INT*)optval)));
        }

        break;

    case TCP_WINDOWSCALE:

        if (optlen < (INT)sizeof(INT))
            status = NU_INVAL;
        else
        {
            status =
                TCP_Setsockopt_TCP_WINDOWSCALE(socketd, (INT)(*((INT*)optval)));
        }

        break;

    case TCP_RCV_WINDOWSIZE:

        if (optlen < (INT)sizeof(UINT32))
            status = NU_INVAL;
        else
        {
            status =
                TCP_Setsockopt_TCP_RCV_WINDOWSIZE(socketd, (UINT32)(*((INT*)optval)));
        }

        break;

    case TCP_TIMESTAMP:

        if (optlen < (INT)sizeof(UINT8))
            status = NU_INVAL;
        else
        {
            status =
                TCP_Setsockopt_TCP_TIMESTAMP(socketd, (UINT8)(*((INT*)optval)));
        }

        break;

    case TCP_KEEPINTVL:

        if (optlen < (INT)sizeof(UINT32))
        {
            status = NU_INVAL;
        }
        else
        {
            status = TCP_Setsockopt_TCP_KEEPINTVL(socketd, (UINT32)(*((INT*)optval)));
        }

        break;

    default :

        status = NU_INVALID_OPTION;
    }

    return (status);

} /* TCP_Set_Opt */
