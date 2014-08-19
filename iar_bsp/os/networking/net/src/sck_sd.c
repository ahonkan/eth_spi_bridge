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
*       sck_sd.c
*
*   DESCRIPTION
*
*       This file contains the implementation of NU_Shutdown
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Shutdown
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "services/nu_trace_os_mark.h"

/*************************************************************************
*
*   FUNCTION
*
*       NU_Shutdown
*
*   DESCRIPTION
*
*       Allows application to shutdown a specific communication side of
*       a socket.
*
*   INPUTS
*
*       socketd                 Socket descriptor
*       how                     Method for shutdown
*
*   OUTPUTS
*
*       NU_INVALID_SOCKET       The socket descriptor is invalid
*       NU_NOT_CONNECTED        The socket is not connected
*
*************************************************************************/
STATUS NU_Shutdown(INT socketd, INT how)
{
    STATUS              return_status;
    struct sock_struct  *sockptr;

#if (INCLUDE_TCP == NU_TRUE)
    struct sock_struct  *ac_sockptr;
    INT                 ac_socketd;
    UINT16              idx;
#endif

    /* Obtain the semaphore and validate the socket */
    return_status = SCK_Protect_Socket_Block(socketd);

    if (return_status != NU_SUCCESS)
        return (return_status);

    sockptr = SCK_Sockets[socketd];

    /* Shutdown the read-half of the connection. */
    if (how & SHUT_RD)
    {
        /* Change the socket state so data is no longer received by the
           socket. */
        sockptr->s_state |= SS_CANTRCVMORE;

        /* Flush any data waiting to be received. */
        /* If there are buffers on the RX list then free them. */
        if (sockptr->s_recvlist.head != NU_NULL)
        {
            /* Free all the buffers. */
            MEM_Buffer_Cleanup (&sockptr->s_recvlist);
        }

        /* Set the number of bytes available on the socket to zero. */
        sockptr->s_recvpackets = sockptr->s_recvbytes = 0;

        /* Resume tasks pending on RX */
        SCK_Resume_All(&sockptr->s_RXTask_List, 0);

        /* Trace log */
        T_SOCK_STATUS(sockptr->s_state, socketd, return_status);
    }

    if (how & SHUT_WR)
    {
        /* Change the socket state so data can no longer be written
           to this socket. */
        sockptr->s_state |= SS_CANTWRTMORE;

#if (INCLUDE_TCP == NU_TRUE)

        if (sockptr->s_protocol == NU_PROTO_TCP)
        {

            /* Listener might have sockets not yet accepted. Should free up
               all the resources as the application does not know about this
               socket and would be unable to do a close_socket call. */
            if (sockptr->s_flags & SF_LISTENER)
            {
                for (idx = 0; idx < sockptr->s_accept_list->total_entries; idx++)
                {
                    /* Get the socket index to close. */
                    ac_socketd = sockptr->s_accept_list->socket_index[idx];

                    /* Verify the socket exists */
                    if ( (ac_socketd == NU_IGNORE_VALUE) || (ac_socketd < 0) ||
                         (SCK_Sockets[ac_socketd] == NU_NULL) )
                        continue;

                    ac_sockptr = SCK_Sockets[ac_socketd];

                    /* Close the socket */
                    TCPSS_Net_Close(ac_sockptr->s_port_index, ac_sockptr);

                    /* Return the socket's resources */
                    /* Free any buffers on the RX list */
                    if (ac_sockptr->s_recvlist.head != NU_NULL)
                        MEM_Buffer_Cleanup(&(ac_sockptr->s_recvlist));

                    /* Ensure that the port structure no longer references this
                     * socket since it is about to be deallocated.
                     */
                    if ( (ac_sockptr->s_port_index != -1) &&
                         (TCP_Ports[ac_sockptr->s_port_index]) )
                        TCP_Ports[ac_sockptr->s_port_index]->p_socketd = -1;

                    /* Clear the socket pointer for future use */
                    SCK_Sockets[ac_socketd] = NU_NULL;

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
                    /* Release the memory used by this socket */
                    if (NU_Deallocate_Memory(ac_sockptr) != NU_SUCCESS)
                        NLOG_Error_Log("Failed to deallocate memory", NERR_SEVERE,
                                       __FILE__, __LINE__);
#endif

                    /* Unlink this entry from the accept list. */
                    sockptr->s_accept_list->socket_index[idx] = -1;
                }
            }

            /* Perform the half-close */
            return_status = TCPSS_Half_Close (sockptr);
        }
#endif /* INCLUDE_TCP == NU_TRUE */

        /* Trace log */
        T_SOCK_STATUS(sockptr->s_state, socketd, return_status);
    }

    /* Release the semaphore */
    SCK_Release_Socket();

    return (return_status);

} /* NU_Shutdown */
