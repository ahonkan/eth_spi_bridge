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
*       tcp_ka.c
*
*   DESCRIPTION
*
*       TCP Keep-Alive Routines
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       TCP_Keep_Alive
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/externs6.h"
#endif

#if (INCLUDE_SNMP == NU_TRUE)
#include SNMP_GLUE
#endif

extern  TQ_EVENT TCP_Keepalive_Event;

/*************************************************************************
*
*   FUNCTION
*
*       TCP_Keep_Alive
*
*   DESCRIPTION
*
*       This function performs TCP Keep-Alive processing.  It will send
*       a TCP Keep-Alive segment if the maximum number of Keep-Alive
*       packets have not already been sent.  Otherwise, it will close
*       the connection and set an error for the upper-layer.
*
*   INPUTS
*
*       event                   The TCP_Keepalive_Event event.
*       pindex                  The index of the port on which to
*                               do Keep-Alive processing.
*       pkts_trans              The total number of Keep-Alive packets
*                               transmitted so far.
*
*   OUTPUTS
*
*       None
*
************************************************************************/
VOID TCP_Keep_Alive(TQ_EVENT event, UNSIGNED pindex, UNSIGNED pkts_trans)
{
    TCP_PORT            *prt = TCP_Ports[pindex];
    struct  sock_struct *sock_ptr;
    STATUS              status;

    /* If the proper event has been passed in and the port structure
     * is valid.
     */
    if ( (event == TCP_Keepalive_Event) && (prt) && (prt->p_socketd >= 0) )
    {
        /* Get a pointer to the socket */
        sock_ptr = SCK_Sockets[prt->p_socketd];

        /* If the socket is valid */
        if (sock_ptr)
        {
            /* If the maximum number of packets have not been transmitted */
            if (pkts_trans < prt->p_ka_r2)
            {
                /* Only transmit a Keep-Alive packet if the connection
                 * is in the ESTABLISHED or the CLOSE-WAIT state.
                 */
                if ( (prt->state == SEST) || (prt->state == SCWAIT) )
                {
                    /* Send the packet */
                    status = TCP_Xmit_Probe(prt);

                    /* Increment the number of packets transmitted */
                    if (status == NU_SUCCESS)
                    {
                        pkts_trans ++;

                        /* Set the timer to transmit another keep-alive packet */
                        if (TQ_Timerset(TCP_Keepalive_Event, (UNSIGNED)pindex,
                                        prt->p_keepalive_intvl, pkts_trans) != NU_SUCCESS)
                        {
                            NLOG_Error_Log("Failed to set timer for Keep-Alive",
                                           NERR_SEVERE, __FILE__, __LINE__);

                            NET_DBG_Notify(NU_NO_MEMORY, __FILE__, __LINE__,
                                           NU_Current_Task_Pointer(), NU_NULL);
                        }
                    }
                }

                /* Reset the Keep-Alive timer to the maximum delay value */
                else
                {
                    if (TQ_Timerset(TCP_Keepalive_Event, (UNSIGNED)pindex,
                                    prt->p_ka_wait, 0) != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to set timer for Keep-Alive",
                                       NERR_SEVERE, __FILE__, __LINE__);

                        NET_DBG_Notify(NU_NO_MEMORY, __FILE__, __LINE__,
                                       NU_Current_Task_Pointer(), NU_NULL);
                    }
                }
            }

            /* Otherwise, the maximum number of keep-alive packets have been
             * transmitted with no response.  The connection has timed out.
             */
            else
            {
                /* Set the socket error indicating that the connection timed out */
                sock_ptr->s_error = NU_CONNECTION_TIMED_OUT;

                NLOG_Error_Log("Max number of TCP keepalive packets have been sent",
                                    NERR_RECOVERABLE, __FILE__, __LINE__);

                /* Set the state of the socket to indicate that the connection
                 * has timed out.
                 */
                SCK_Sockets[prt->p_socketd]->s_state |= SS_TIMEDOUT;

                if ( (prt->state == SCWAIT) || (prt->state == SEST) )
                {
                    MIB2_tcpEstabResets_Inc;
                }

                /* Send a reset just in case the other side is still up. */
                prt->out.tcp_flags = TRESET;
                TCP_ACK_It(prt, 1);

                /* Mark the socket as disconnecting. */
                SCK_DISCONNECTING(prt->p_socketd);

                /* Abort this connection. */
                prt->state = SCLOSED;

                /* The connection is  closed.  Cleanup. */
                TCP_Cleanup(prt);
            }
        }
    }

} /* TCP_Keep_Alive */

