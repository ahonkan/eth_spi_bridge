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
* FILE NAME
*
*       sck_ab.c
*
* DESCRIPTION
*
*       This file contains the implementation of NU_Abort.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_Abort
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_SNMP == NU_TRUE)
#include SNMP_GLUE
#endif

/*************************************************************************
*
*   FUNCTION
*
*       NU_Abort
*
*   DESCRIPTION
*
*       This function aborts a TCP or UDP connection.  In the case of TCP
*       a RESET is sent to the remote host.  All resources are freed up.
*
*   INPUTS
*
*       socketd                 Socket number of the socket to abort.
*
*   OUTPUTS
*
*       NU_SUCCESS              The connection was successfully aborted.
*       NU_INVALID_SOCKET       The socket parameter was not a valid
*                               socket value or it had not been
*                               previously allocated via the NU_Socket
*                               call.
*       NU_NO_PORT_NUMBER       No local port number was stored in the
*                               socket descriptor.
*
*************************************************************************/
STATUS NU_Abort(INT socketd)
{
    STATUS  return_status;

#if ((INCLUDE_TCP == NU_TRUE) || (INCLUDE_UDP == NU_TRUE))
    INT     port_num;                 /* local machine's port number */
#endif

#if (INCLUDE_TCP == NU_TRUE)
    TCP_PORT    *prt;
#endif

#if (INCLUDE_SR_SNMP == NU_TRUE)
    UINT8       tcp_laddr[IP_ADDR_LEN];
    UINT8       tcp_faddr[IP_ADDR_LEN];
#endif

    /* Obtain the semaphore and validate the socket */
    return_status = SCK_Protect_Socket_Block(socketd);

    if (return_status != NU_SUCCESS)
        return (return_status);

#if (INCLUDE_TCP == NU_TRUE)

    /*  Do different processing based on the protocol type.  */
    if (SCK_Sockets[socketd]->s_protocol == NU_PROTO_TCP)
    {
        port_num = SCK_Sockets[socketd]->s_port_index;

        if ( (port_num >= 0) &&
             (!(SCK_Sockets[socketd]->s_flags & SF_LISTENER)) )
        {
            prt = TCP_Ports[port_num];

            /* Send a RESET to the other guy. */
            prt->out.tcp_flags = TRESET;
            TCP_ACK_It(prt, 1);

#if (INCLUDE_SR_SNMP == NU_TRUE)

            PUT32(tcp_laddr, 0, prt->tcp_laddrv4);
            PUT32(tcp_faddr, 0, prt->tcp_faddrv4);

            SNMP_tcpConnTableUpdate(SNMP_DELETE, SEST, tcp_laddr,
                                    (UNSIGNED)(prt->in.port), tcp_faddr,
                                    (UNSIGNED)(prt->out.port));
#endif

            MIB2_tcpCurrEstab_Dec;

            /* Reset the timer since the connection is closed */
            TQ_Timerunset(TCPCLOSETIMEOUTSFW2, TQ_CLEAR_EXACT,
                          (UNSIGNED)prt->pindex, 0);

            prt->state = SCLOSED;

            /* The connection is  closed.  Cleanup. */
            TCP_Cleanup(prt);
        }
        else
        {
            /* Ensure the port number is still valid. */
            if (port_num >= 0)
            {
                /* Set the state of the port to CLOSED so this port
                 * structure can be reused.
                 */
                TCP_Ports[port_num]->state = SCLOSED;

                /* The connection is  closed.  Cleanup. */
                TCP_Cleanup(TCP_Ports[port_num]);
            }

            /* There is no port number for this guy, bad problem.  */
            else
                return_status = NU_NO_PORT_NUMBER;
        }
    }
    else

#endif /* INCLUDE_TCP == NU_TRUE */

    {

#if (INCLUDE_UDP == NU_TRUE)

        port_num = UDP_Get_Pnum(SCK_Sockets[socketd]);

        /* Ensure the port number is valid before cleaning up the port. */
        if (port_num >= 0)
            UDP_Port_Cleanup((UINT16)port_num, SCK_Sockets[socketd]);

        /* There is no port number for this guy, bad problem. */
        else
            return_status = NU_NO_PORT_NUMBER;

        /* Resume tasks pending on RX */
        SCK_Resume_All(&SCK_Sockets[socketd]->s_RXTask_List, 0);

        /* Resume tasks pending on TX, remove from buffer suspension
           list */
        SCK_Resume_All(&SCK_Sockets[socketd]->s_TXTask_List, SCK_RES_BUFF);

#endif /* INCLUDE_UDP == NU_TRUE */

    }

    /* Make sure there was a socket before freeing everything. */
    if (SCK_Sockets[socketd] != NU_NULL)
        SCK_Cleanup_Socket(socketd);

    /* Release the semaphore */
    SCK_Release_Socket();

    /* return to caller */
    return (return_status);

} /* NU_Abort */
