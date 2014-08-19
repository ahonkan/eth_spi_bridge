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
*       sck_sel.c
*
*   DESCRIPTION
*
*       This file contains those functions associated with the NU_Select
*       service.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Select
*       NU_FD_Check
*       NU_FD_Set
*       NU_FD_Init
*       NU_FD_Reset
*       SEL_Check
*       SEL_Setup_Ports
*
*   DEPENDENCIES
*
*       nu_net.h
*       ipraw.h
*
*************************************************************************/

#include "networking/nu_net.h"
#include "networking/ipraw.h"

STATIC STATUS SEL_Setup_Ports(INT, FD_SET **, NU_TASK *,
                              NET_BUFFER_SUSPENSION_ELEMENT *,
                              struct SCK_TASK_ENT *);

STATIC STATUS SEL_Check(INT, FD_SET **,
                        NET_BUFFER_SUSPENSION_ELEMENT *);

/*************************************************************************
*
*   FUNCTION
*
*       NU_Select
*
*   DESCRIPTION
*
*       This function allows an application to check for data on multiple
*       sockets.  Alternatively multiple server sockets (those that are
*       listening for connections) can be checked for established
*       connections.  The calling application can choose to return
*       immediately, suspend, or specify a timeout.
*
*   INPUTS
*
*       max_sockets             Maximum socket to check.
*       *readfs                 Pointer to a bit field indicating
*                               which sockets to check for data.
*       *writefs                Pointer to a bit field indicating
*                               which sockets to check for writable.
*       *exceptfs               Pointer to a bit field indicating
*                               which sockets to check for connections.
*       timeout                 The timeout desired. Either NU_SUSPEND,
*                               NU_NO_SUSPEND, or a timeout value.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful
*       NU_NO_SOCKETS           readfs and writefs are NU_NULL or there
*                               are no bits set in the array for checking.
*       NU_INVALID_SOCKET       max_sockets = 0 or
*                               max_sockets is > NSOCKETS
*       -1                      NU_SELECT failed generally
*       NU_NO_DATA              No data to communicate
*
*       An ICMP Error code will be returned if an ICMP packet was
*       received for the socket:
*
*       NU_DEST_UNREACH_ADMIN
*       NU_DEST_UNREACH_ADDRESS
*       NU_DEST_UNREACH_PORT
*       NU_TIME_EXCEED_HOPLIMIT
*       NU_TIME_EXCEED_REASM
*       NU_PARM_PROB_HEADER
*       NU_PARM_PROB_NEXT_HDR
*       NU_PARM_PROB_OPTION
*       NU_DEST_UNREACH_NET
*       NU_DEST_UNREACH_HOST
*       NU_DEST_UNREACH_PROT
*       NU_DEST_UNREACH_FRAG
*       NU_DEST_UNREACH_SRCFAIL
*       NU_PARM_PROB
*       NU_SOURCE_QUENCH
*
*************************************************************************/
STATUS NU_Select(INT max_sockets, FD_SET *readfs, FD_SET *writefs,
                 FD_SET *exceptfs, UNSIGNED timeout)
{
    INT16                           bset_flag = 0;
    INT16                           i, j;
    INT16                           timeout_used = NU_FALSE;
    STATUS                          return_status;
    NU_TASK                         *task_id;
    FD_SET                          *fdsa[SEL_MAX_FDSET];
    NET_BUFFER_SUSPENSION_ELEMENT   buf_ssp_elmt;
    struct SCK_TASK_ENT             *sck_task_ptr = NU_NULL;
    INT16                           how_many = 0;
    NU_SUPERV_USER_VARIABLES

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
    /* Declare maximum memory for the socket suspend structures */
    struct SCK_TASK_ENT select_memory[NET_MAX_SELECT_SOCKETS];
#endif

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    if ( (max_sockets == 0) || (max_sockets > NSOCKETS) )
    {
        return (NU_NO_SOCKETS);
    }

    /* Verify at least one FD set was passed in */
    if ( (readfs == NU_NULL) &&
         (writefs == NU_NULL) )
    {
        return (NU_NO_SOCKETS);
    }

#endif

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Clean up warnings.  This parameter is used for socket compatibility
     * but we are currently not making any use of it.
     */
    UNUSED_PARAMETER(exceptfs);

    /* Assign the input FD sets */
    fdsa[SEL_READABLE_IDX] = readfs;
    fdsa[SEL_WRITABLE_IDX] = writefs;

    /* Make sure that at least one bit is set. */
    for (i=0; (i < FD_ELEMENTS) && (!bset_flag); i++)
    {
        for (j=0; (j < SEL_MAX_FDSET) && (!bset_flag); j++)
        {
            if (fdsa[j] != NU_NULL)
            {
                if ( ((UINT32)fdsa[j]->words[i] != 0) )
                    bset_flag = 1;
            }
        }
    }

    /* If no bits are set in the array, there is no need to go on. */
    if (bset_flag == 0)
    {
        /* Return to user mode */
        NU_USER_MODE();

        return (NU_NO_SOCKETS);
    }

    /* Determine the number of sockets the select is on */
    for (i=0; i < max_sockets; i++)
    {
        for (j=0; j < SEL_MAX_FDSET; j++)
        {
            if (fdsa[j] != NU_NULL)
            {
                if (NU_FD_Check(i, fdsa[j]) == NU_TRUE)
                    how_many++;
            }
        }
    }

    /* Clear the members of the buffer suspension variable. */
    memset(&buf_ssp_elmt, 0, sizeof(buf_ssp_elmt));

    /* Allocate memory for the socket suspend structures */
    if (how_many)
    {
#if (INCLUDE_STATIC_BUILD == NU_FALSE)
        return_status = NU_Allocate_Memory(MEM_Cached, (VOID **)&sck_task_ptr,
                            (UNSIGNED)(how_many * sizeof(struct SCK_TASK_ENT)),
                            NU_NO_SUSPEND);
#else
        /* Assign memory to the socket suspend structures */
        if (how_many > NET_MAX_SELECT_SOCKETS)
            return_status = NU_NO_MEMORY;
        else
        {
            sck_task_ptr =  select_memory;
            return_status = NU_SUCCESS;
        }

#endif  /* INCLUDE_STATIC_BUILD */

        /* Verify the alloc was successful */
        if (return_status != NU_SUCCESS)
        {
            /* Return to user mode */
            NU_USER_MODE();

            return (return_status);
        }

        /* Clear the memory */
        UTL_Zero((VOID *)sck_task_ptr,
                 (UNSIGNED)(how_many * sizeof(struct SCK_TASK_ENT)));
    }

    /* No bits were set in the FD sets */
    else
    {
        /* Return to user mode */
        NU_USER_MODE();

        return (NU_NO_SOCKETS);
    }

    /* Don't let anyone else in until we are through.  */
    return_status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (return_status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain the semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        /* Deallocate the memory previously allocated. */
        if (NU_Deallocate_Memory(sck_task_ptr) != NU_SUCCESS)
            NLOG_Error_Log("Failed to deallocate memory for task pointer",
                           NERR_SEVERE, __FILE__, __LINE__);

        /* Return to user mode */
        NU_USER_MODE();

        return (return_status);
    }

    for (;;)
    {
        /* Check to see if any of the selected sockets are readable or writable.
         * The FD sets indicate the descriptors to check for the readable or
         * writable condition.
         */
        return_status = SEL_Check(max_sockets, &fdsa[0], &buf_ssp_elmt);

        /* If at least one socket was data ready then break out of this while
         * loop and return.  Break also if suspension was not desired, or if
         * timeout_used is true. Note that timeout_used is only set to true
         * if a timeout is desired but not unconditional suspension.  If
         * unconditional suspension was specified then we will only return when
         * a data ready socket is found, even if this task is some how
         * inadvertently woke up. We will also break out if there were no valid
         * sockets to select on.
         */
        if ( (return_status == NU_SUCCESS) || (timeout == NU_NO_SUSPEND) ||
             (timeout_used == NU_TRUE) || (return_status == NU_INVALID_SOCKET) )
            break;

        /* Go ahead and retrieve the current task pointer once.  It is used in
         * several places below.
         */
        task_id = NU_Current_Task_Pointer();

        return_status = SEL_Setup_Ports(max_sockets, &fdsa[0], task_id,
                                        &buf_ssp_elmt, sck_task_ptr);

        /* If we did not successfully setup the ports then get out.
         * There is no point in continuing.
         */
        if (return_status != NU_SUCCESS)
            break;

        if (timeout != NU_SUSPEND)
        {
            /* Set up the timer to wake us up if the event never occurs. */
            if (TQ_Timerset(SELECT, (UNSIGNED)task_id, timeout, 0) != NU_SUCCESS)
                NLOG_Error_Log("Failed to set timer to wake up task", NERR_SEVERE,
                               __FILE__, __LINE__);

            timeout_used = NU_TRUE;
        }

        SCK_Suspend_Task(task_id);

        if (timeout != NU_SUSPEND)
        {
            /* At this point there is no way to tell if we were resumed because
             * of a timeout or because the event we are waiting on occurred.  In
             * the former case the event will already be cleared.  Try to clear
             * it here anyway.
             */
            TQ_Timerunset(SELECT, TQ_CLEAR_EXACT, (UNSIGNED)task_id, 0);
        }

        /* Clean the sockets after waking up */
        if (SEL_Setup_Ports(max_sockets, &fdsa[0], NU_NULL, &buf_ssp_elmt,
                            sck_task_ptr) != NU_SUCCESS)
            NLOG_Error_Log("Failed to setup ports", NERR_SEVERE,
                           __FILE__, __LINE__);

    } /* for (;;) */

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    /* Return the memory */
    if (sck_task_ptr != NU_NULL)
    {
        if (NU_Deallocate_Memory(sck_task_ptr) != NU_SUCCESS)
            NLOG_Error_Log("Failed to deallocate memory for task pointer",
                           NERR_SEVERE, __FILE__, __LINE__);
    }
#endif

    /* Release the semaphore. */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (return_status);

} /* NU_Select */

/*************************************************************************
*
*   FUNCTION
*
*       NU_FD_Check
*
*   DESCRIPTION
*
*       This function will check to see if a particular bit has been
*       set in a bit map.
*
*   INPUTS
*
*       socket                  Specifies the index of the bit to check
*       *fd                     Pointer to the bitmap which will be checked
*
*   OUTPUTS
*
*       NU_TRUE                 The bit has been set
*       NU_FALSE                The bit was not set
*
*************************************************************************/
INT NU_FD_Check(INT socket, const FD_SET *fd)
{
    NU_SUPERV_USER_VARIABLES

    /* Validate the input parameters. */
    if ( (socket < 0) || (socket >= NSOCKETS) || (fd == NU_NULL) )
        return (NU_FALSE);

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    if (fd->words[socket / FD_BITS] & (1UL << (socket % FD_BITS)))
    {
        /* Return to user mode */
        NU_USER_MODE();

        return (NU_TRUE);
    }
    else
    {
        /* Return to user mode */
        NU_USER_MODE();

        return (NU_FALSE);
    }

} /* NU_FD_Check */

/*************************************************************************
*
*   FUNCTION
*
*       NU_FD_Set
*
*   DESCRIPTION
*
*       Sets a bit in a bit map.
*
*   INPUTS
*
*       socket                  Specifies the index of the bit to be set
*       *fd                     Pointer to the bitmap in which the bit will
*                               be set.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID NU_FD_Set(INT socket, FD_SET *fd)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Validate the input parameters and perform the requested
     * operation.
     */
    if ( (socket >= 0) && (socket < NSOCKETS) &&
         (SCK_Sockets[socket] != NU_NULL) && (fd != NU_NULL) )
    {
        fd->words[socket / FD_BITS] |= (1UL << (socket % FD_BITS));
    }

    /* Return to user mode */
    NU_USER_MODE();

} /* NU_FD_Set */

/*************************************************************************
*
*   FUNCTION
*
*       NU_FD_Init
*
*   DESCRIPTION
*
*       Sets all bits in a bit map to 0.
*
*   INPUTS
*
*       *fd                     This parameter is a pointer to the bitmap
*                               that will be initialized.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID NU_FD_Init(FD_SET *fd)
{
    INT i;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    for (i=0; i<FD_ELEMENTS; fd->words[i++]=0L)
        ;

    /* Return to user mode */
    NU_USER_MODE();

} /* NU_FD_Init */

/*************************************************************************
*
*   FUNCTION
*
*       NU_FD_Reset
*
*   DESCRIPTION
*
*       Resets a bit in a bit map.
*
*   INPUTS
*
*       socket                  Specifies the index the bit to be reset.
*       *fd                     Pointer to the bitmap in which the bit
*                               will be reset
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID NU_FD_Reset(INT socket, FD_SET *fd)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Validate the input parameters and perform the requested
     * operation.
     */
    if ( (socket >= 0) && (socket < NSOCKETS) && (fd != NU_NULL) )
    {
        fd->words[socket / FD_BITS] &= ~(1UL << (socket % FD_BITS));
    }

    /* Return to user mode */
    NU_USER_MODE();

} /* NU_FD_Reset */

/*************************************************************************
*
*   FUNCTION
*
*       SEL_Check
*
*   DESCRIPTION
*
*       Checks the sockets specified in a bitmap to see if any are
*       readable or writable. If any of the selected sockets meet the
*       condition, the bitmap is modified so that only the bits for
*       readable / writable sockets are set. Otherwise the bitmap is
*       not changed.
*
*   INPUTS
*
*       max_sockets             The max descriptor of socket to check
*       **fdsp                  Pointer to an array of FD set pointers.
*       *buf_ssp_elmt           Pointer to buffer suspension list
*                               structure that holds tasks that are
*                               waiting to transmit
*
*   OUTPUTS
*
*       NU_SUCCESS              At least one socket is data ready,
*       NU_INVALID_SOCKET       The socket was wrong
*       NU_NO_DATA              No data was received
*
*************************************************************************/
STATIC STATUS SEL_Check(INT max_sockets, FD_SET **fdsp,
                        NET_BUFFER_SUSPENSION_ELEMENT *buf_ssp_elmt)
{
    FD_SET tmpfs[SEL_MAX_FDSET];        /* Temp array of FD sets used for
                                           preserving original values. */
    INT    i,set;                       /* Loop counters */
    STATUS return_status = NU_NO_DATA;  /* Assume failure, if a data ready
                                           socket is found status will be
                                           updated to success. */
    struct sock_struct *sockptr;        /* pointer to current socket */

#if (INCLUDE_TCP == NU_TRUE)
    INT         pnum;                   /* Port number */
    TCP_PORT    *prt;                   /* Pointer to prt structure */
#endif

#if ( (INCLUDE_UDP != NU_TRUE) || (INCLUDE_IP_RAW != NU_TRUE) )
    UNUSED_PARAMETER(buf_ssp_elmt);
#endif

    /* Preserve the current state of the FD sets. It may be needed below. */
    for (set = 0; set < SEL_MAX_FDSET; set++)
    {
        /* Clear the FD set */
        NU_FD_Init(&tmpfs[set]);

        if (fdsp[set] != NU_NULL)
            memcpy(&tmpfs[set], fdsp[set], sizeof(FD_SET));
    }

    for (i = 0; i < max_sockets; i++)
    {
        /* Is the readable bit for socket i set?  A socket is ready for
         * reading if one of the following conditions is true:
         *
         * 1 - A read operation on the socket will not block and will
         *     return a value greater than zero.
         *
         * 2 - The read-half of the connection is closed; received a FIN.
         *
         * 3 - The socket is a listening socket and the number of
         *     completed connections is non-zero.
         *
         * 4 - A socket error is pending.
         */
        if ( (fdsp[SEL_READABLE_IDX] != NU_NULL) &&
             (NU_FD_Check(i, fdsp[SEL_READABLE_IDX])) )
        {
            /* Pick up a pointer to the socket list. */
            sockptr = SCK_Sockets[i];

            if (sockptr == NU_NULL)
            {
                /* There is no socket for this bit in the select bit map.
                 * This indicates a user error. Reset this bit so that
                 * this error will not occur next time. Set the status to
                 * NU_INVALID_SOCKET if we have not already had success
                 * on another socket.
                 */
                NU_FD_Reset(i, fdsp[SEL_READABLE_IDX]);

                if (return_status != NU_SUCCESS)
                    return_status = NU_INVALID_SOCKET;
            }

#if (INCLUDE_TCP == NU_TRUE)

            else if (sockptr->s_protocol == NU_PROTO_TCP)
            {
                /* If this is not a server socket. */
                if (!(sockptr->s_flags & SF_LISTENER))
                {
                    /* Check to see if there is received data. At this point
                     * it does not matter if the connection has been closed.
                     * If there is received data, it should be passed to
                     * the application layer.
                     */

                    /* First check to see if the connection has been closed
                     * by the foreign side. (spr471)
                     */
                    if (sockptr->s_state & SS_ISDISCONNECTING)
                    {
                        /* Set the status to success so that we will return
                         * to the calling task.
                         */
                        return_status = NU_SUCCESS;
                    }

                    /* Verify there is data on this socket, no other task is
                     * using this socket and no other processes are accessing
                     * it now!
                     */
                    else if ( (sockptr->s_RXTask_List.flink == NU_NULL) &&
                              (sockptr->s_recvbytes > 0) )
                        return_status = NU_SUCCESS;

                    /* If an error was received on the socket */
                    else if (sockptr->s_error != 0)
                        return_status = NU_SUCCESS;

                    /* There is no data to be received and no socket errors
                     * pending
                     */
                    else
                        NU_FD_Reset(i, fdsp[SEL_READABLE_IDX]);
                }

                /* This is a listening socket.  Check if the number of
                 * completed connections is non-zero.
                 */
                else
                {
                    /* Search the task table for this port number/task id */
                    if (SCK_SearchTaskList(sockptr->s_accept_list, SEST,
                                           -1) >= 0)
                        return_status = NU_SUCCESS;

                    /* If an error was received on the socket */
                    else if (sockptr->s_error != 0)
                        return_status = NU_SUCCESS;

                    else
                        NU_FD_Reset(i, fdsp[SEL_READABLE_IDX]);
                }

            } /* end this is a TCP socket. */

#endif /* INCLUDE_TCP == NU_TRUE */

#if (INCLUDE_UDP == NU_TRUE)

            else if (sockptr->s_protocol == NU_PROTO_UDP)
            {
                /* if the device used by the UDP Socket has gone down */
                if (sockptr->s_state & SS_DEVICEDOWN)
                {
                    /* set the status to success so we can return to calling
                     * task
                     */
                    return_status = NU_SUCCESS;
                }

                /* Verify no other tasks are waiting on this socket and
                 * there are datagrams to be received.
                 */
                else if ( (sockptr->s_RXTask_List.flink == NU_NULL) &&
                          (sockptr->s_recvpackets > 0) )
                    return_status = NU_SUCCESS;

                /* If an error was received on the socket */
                else if (sockptr->s_error != 0)
                    return_status = NU_SUCCESS;

                /* No datagrams waiting and no socket errors */
                else
                    NU_FD_Reset(i, fdsp[SEL_READABLE_IDX]);

            }  /* end this is a UDP socket */

#endif /* INCLUDE_UDP == NU_TRUE */

#if (INCLUDE_IP_RAW == NU_TRUE)
            else if (sockptr->s_local_addr.port_num == 0)
            {
                /* if the device used by the IP RAW Socket has gone down */
                if (sockptr->s_state & SS_DEVICEDOWN)
                {
                    /* Set the status to success so we can return to the
                     * calling task
                     */
                    return_status = NU_SUCCESS;
                }

                /* Verify no other tasks are waiting on this socket and
                 * there is data to be received.
                 */
                else if ( (sockptr->s_RXTask_List.flink == NU_NULL) &&
                          (sockptr->s_recvpackets > 0) )
                    return_status = NU_SUCCESS;

                /* If an error was received on the socket */
                else if (sockptr->s_error != 0)
                    return_status = NU_SUCCESS;

                /* No data waiting and no socket errors */
                else
                    NU_FD_Reset(i, fdsp[SEL_READABLE_IDX]);

            }  /* end this is a RAW IP socket */

#endif /* INCLUDE_IP_RAW == NU_TRUE */
        } /* end of readable check */

        /* Is the writable bit for socket i set?  A socket is ready for
         * writing if one of the following conditions is true:
         *
         * 1 - A write operation on the socket will not block and will
         *     return a value greater than zero.
         *
         * 2 - The write-half of the connection is closed.
         *
         * 3 - A socket error is pending.
         */
        if ( (fdsp[SEL_WRITABLE_IDX] != NU_NULL) &&
             (NU_FD_Check(i, fdsp[SEL_WRITABLE_IDX])) )
        {
            /* Pick up a pointer to the socket. */
            sockptr = SCK_Sockets[i];

            /*  Pick up a pointer to the socket list. */
            if (sockptr == NU_NULL)
            {
                /* There is no socket for this bit in the select bit map.
                 * This indicates a user error. Reset this bit so that
                 * this error will not occur next time. Set the status to
                 * NU_INVALID_SOCKET if we have not already had success
                 * on another socket.
                 */
                NU_FD_Reset (i, fdsp[SEL_WRITABLE_IDX]);

                if (return_status != NU_SUCCESS)
                    return_status = NU_INVALID_SOCKET;
            }

#if (INCLUDE_TCP == NU_TRUE)
            else if (sockptr->s_protocol == NU_PROTO_TCP)
            {
                /* Get the port index */
                pnum = sockptr->s_port_index;

                if (pnum < 0)
                {
                    /* There is no port for this bit. */
                    NU_FD_Reset (i, fdsp[SEL_WRITABLE_IDX]);

                    if (return_status != NU_SUCCESS)
                        return_status = NU_INVALID_SOCKET;
                }

                /* Check if we are in a state that allows the transmission
                 * of data to the other side of the connection.
                 */
                else if ( (TCP_Ports[sockptr->s_port_index]->state == SCWAIT) ||
                          (TCP_Ports[sockptr->s_port_index]->state == SEST) )
                {
                    /* Get the port structure */
                    prt = TCP_Ports[pnum];

                    /* A write operation on a TCP socket will block if the
                     * other side's receive window is full, we are probing
                     * the other side of the connection, or the TCP buffer
                     * threshold has been reached.
                     */
                    if ((
                         (prt->out.size > prt->out.contain) &&
                         (prt->probeFlag == NU_CLEAR) &&
                         ((MAX_BUFFERS - MEM_Buffers_Used) > NET_FREE_BUFFER_THRESHOLD)
                        )
#if (NET_INCLUDE_LMTD_TX == NU_TRUE)
                         /* If the Limited Transmit flag is set, one more segment
                          * can be sent.
                          */
                         || (prt->portFlags & TCP_TX_LMTD_DATA)
#endif
                        )
                    {
                        return_status = NU_SUCCESS;
                    }
                    else
                        NU_FD_Reset(i, fdsp[SEL_WRITABLE_IDX]);
                }

                /* Reset the socket bit, it is not writable */
                else
                    NU_FD_Reset(i, fdsp[SEL_WRITABLE_IDX]);

            } /* end this is a TCP socket. */
#endif /* INCLUDE_TCP == NU_TRUE */

#if (INCLUDE_UDP == NU_TRUE)

            else if (sockptr->s_protocol == NU_PROTO_UDP)
            {
                /* If there is no space available for writing */
                if (!MEM_Buffer_Freelist.head)
                {
                    NU_FD_Reset(i, fdsp[SEL_WRITABLE_IDX]);

                    /* Setup the buffer suspension element in case we
                     * will be suspending.
                     */
                    buf_ssp_elmt->waiting_task = NU_Current_Task_Pointer();
                }

                /* Socket is writable if an error is pending, if
                 * write half of connection is closed, or if space is
                 * available for writing.
                 */
                else
                    return_status = NU_SUCCESS;

            }  /* end this is a UDP socket */

#endif /* INCLUDE_UDP == NU_TRUE */

#if (INCLUDE_IP_RAW == NU_TRUE)
            else if (sockptr->s_local_addr.port_num == 0)
            {
                /* If there is no space available for writing */
                if (!MEM_Buffer_Freelist.head)
                {
                    NU_FD_Reset(i, fdsp[SEL_WRITABLE_IDX]);

                    /* Setup the buffer suspension element in case we
                     * will be suspending.
                     */
                    buf_ssp_elmt->waiting_task = NU_Current_Task_Pointer();
                }

                /* Socket is writable if an error is pending, if
                 * write half of connection is closed, or if space is
                 * available for writing.
                 */
                else
                    return_status = NU_SUCCESS;

            } /* end this is a IP raw socket */
#endif /* INCLUDE_IP_RAW == NU_TRUE */
        } /* end of writable check */

    } /* for (i=0; i<max_sockets; i++) */

    /* If none of the sockets were ready, reset the original bitmap */
    if (return_status != NU_SUCCESS)
    {
        for (set = 0; set < SEL_MAX_FDSET; set++)
        {
            if (fdsp[set])
                memcpy(fdsp[set], &tmpfs[set], sizeof(FD_SET));
        }
    }

    return (return_status);

} /* SEL_Check */

/*************************************************************************
*
*   FUNCTION
*
*       SEL_Setup_Ports
*
*   DESCRIPTION
*
*       This function marks the specified ports so that if the
*       readable or writable conditional changes the current task
*       will be resumed.
*
*   INPUTS
*
*       max_sockets             The maximum number of sockets
*       **fdsp                  Pointer to an array of FD set pointers
*       *task_id                Pointer to the task id
*       *buf_ssp_elmt           Pointer to buffer suspension list
*                               structure that holds tasks that are
*                               waiting to transmit
*       *sck_tsk_ptr            Pointer to the task entry
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful
*       -1                      The receive port cannot be set up
*
*************************************************************************/
STATIC STATUS SEL_Setup_Ports(INT max_sockets, FD_SET **fdsp,
                              NU_TASK *task_id,
                              NET_BUFFER_SUSPENSION_ELEMENT *buf_ssp_elmt,
                              struct SCK_TASK_ENT *sck_tsk_ptr)
{
    STATUS                      return_status = -1;
    INT                         i, set;
    INT                         ste_idx = 0;
#if ( (INCLUDE_UDP == NU_TRUE) || (INCLUDE_IP_RAW == NU_TRUE) )
    INT16                       pnum;       /* local machine's port number */
#endif
    struct sock_struct          *sockptr;   /* pointer to current socket */
#if (INCLUDE_TCP == NU_TRUE)
    struct TASK_TABLE_STRUCT    *task_entry;
    INT                         j;
#endif
#if (INCLUDE_IP_RAW == NU_TRUE)
    struct iport                *iprt;      /* IP Raw PCB structure */
#endif
    INT         buf_ssp_used = NU_FALSE;    /* Hold the status of the
                                               buf_ssp_elmt. It is only
                                               added to the suspension
                                               list once. */
    NU_TASK     *current_task = NU_Current_Task_Pointer();

    /* Assign the current thread to each socket we are suspending on
     * until one of the select events occurs.
     */
    for (set = 0; set < SEL_MAX_FDSET; set++)
    {
        /* Verify a FD set was given. */
        if (fdsp[set] == NU_NULL)
            continue;

        /* Check each socket. */
        for (i = 0; i < max_sockets; i++)
        {
            if (NU_FD_Check(i, fdsp[set])== NU_FALSE)
                continue;

            sockptr = SCK_Sockets[i];

            /* If sockptr is NU_NULL, then the socket has been closed
             * while the task was suspended.  NU_Select() will return
             * at the next call to SEL_Check() indicating that this
             * socket has been closed.
             */
            if (sockptr == NU_NULL)
            {
                ste_idx++;
                continue;
            }

#if (INCLUDE_TCP == NU_TRUE)

            if (sockptr->s_protocol == NU_PROTO_TCP)
            {
                /* Handle the read FD set */
                if (set == SEL_READABLE_IDX)
                {
                    /* If this is not a server socket. */
                    if (!(sockptr->s_flags & SF_LISTENER))
                    {
                        /* Check to see if data can still be received on the port. */
                        if ( (sockptr->s_state & SS_ISCONNECTED) ||
                             (task_id == NU_NULL) )
                        {
                            /* Setup a suspending task */
                            if (task_id != NU_NULL)
                            {
                                /* Setup the receive list so it can be resumed */
                                sck_tsk_ptr[ste_idx].task = task_id;

                                /* Add it to the list */
                                DLL_Enqueue(&sockptr->s_RXTask_List,
                                            &sck_tsk_ptr[ste_idx]);
                            }

                            /* task_id is NU_NULL, which indicates to remove
                             * items from the RX list that were added with the
                             * previous call to this routine before the task
                             * suspended in NU_Select().
                             */
                            else
                            {
                                /* If this is the socket that recv'd data, it was
                                 * already removed from the list when it was resumed.
                                 * No harm in removing it again.
                                 */
                                DLL_Remove(&sockptr->s_RXTask_List,
                                           &sck_tsk_ptr[ste_idx]);
                            }

                            return_status = NU_SUCCESS;
                        }
                    }

                    else
                    {
                        /* Get a pointer to the socket's accept list.  This is the
                         * list of connections waiting to be accepted by the
                         * application layer.
                         */
                        task_entry = sockptr->s_accept_list;

                        /* If there is at least one connection on the accept list. */
                        if (task_entry != NU_NULL)
                        {
                            if (task_id != NU_NULL)
                            {
                                /* Setup the accept list so it can be resumed */
                                sck_tsk_ptr[ste_idx].task = task_id;

                                /* Add it to the list */
                                DLL_Enqueue(&task_entry->ssp_task_list,
                                            &sck_tsk_ptr[ste_idx]);
                            }

                            /* task_id is NU_NULL, which indicates to remove
                             * items from the suspension list that were added
                             * with the previous call to this routine before
                             * the task suspended in NU_Select().
                             */
                            else
                            {
                                /* Remove it from the list */
                                DLL_Remove(&task_entry->ssp_task_list,
                                           &sck_tsk_ptr[ste_idx]);

                                /* If there are partially completed connections
                                 * made on the socket or the task was resumed
                                 * before TCP_Do could resume the TX list once
                                 * the connection was established, remove the
                                 * task that was moved from the original
                                 * socket's accept list to the new socket's
                                 * TX list.
                                 */
                                for (j = 0; j < (INT)task_entry->total_entries; j++)
                                {
                                    /* If this socket is not valid, move on to the
                                     * next socket.
                                     */
                                    if ( (task_entry->socket_index[j] == NU_IGNORE_VALUE) ||
                                         (task_entry->socket_index[j] < 0) ||
                                         (SCK_Sockets[task_entry->socket_index[j]] == NU_NULL) )
                                        continue;

                                    /* Remove the task from the socket's TX task list */
                                    DLL_Remove(&SCK_Sockets[task_entry->socket_index[j]]->
                                               s_TXTask_List, &sck_tsk_ptr[ste_idx]);
                                }
                            }

                            return_status = NU_SUCCESS;
                        }
                    }
                } /* end of read FD set */
            }
#endif

#if (INCLUDE_UDP == NU_TRUE)
            else if(sockptr->s_protocol == NU_PROTO_UDP)
            {
                /* Handle the read FD set */
                if (set == SEL_READABLE_IDX)
                {
                    pnum = UDP_Get_Pnum(sockptr);

                    /* If the port is still valid */
                    if ( (pnum != NU_IGNORE_VALUE) &&
                         (UDP_Ports[pnum] != NU_NULL) )
                    {
                        /* Setup a suspending task */
                        if (task_id != NU_NULL)
                        {
                            /* Setup the receive list so it can be resumed */
                            sck_tsk_ptr[ste_idx].task = task_id;

                            /* Add it to the list */
                            DLL_Enqueue(&sockptr->s_RXTask_List,
                                        &sck_tsk_ptr[ste_idx]);
                        }

                        /* task_id is NU_NULL, which indicates to remove
                         * items from the RX list that were added with the
                         * previous call to this routine before the task
                         * suspended in NU_Select().
                         */
                        else
                        {
                            /* If this is the socket that recv'd data, it
                             * was already removed from the list when it was
                             * resumed. No harm in removing it again.
                             */
                            DLL_Remove(&sockptr->s_RXTask_List,
                                       &sck_tsk_ptr[ste_idx]);
                        }

                        return_status = NU_SUCCESS;
                    }

                } /* end of read FD set */
            }
#endif

#if (INCLUDE_IP_RAW == NU_TRUE)

            /* my_port should be zero when dealing with raw IP sockets */
            else if (sockptr->s_local_addr.port_num == 0)
            {
                /* Handle the read FD set */
                if (set == SEL_READABLE_IDX)
                {
                    /* Check if there is already a PCB structure set up
                     * for this IP communication.
                     */
                    pnum = IPRaw_Get_PCB(i, sockptr);

                    if ( (pnum != NU_IGNORE_VALUE) &&
                         (IPR_Ports[pnum] != NU_NULL) )
                    {
                        iprt = IPR_Ports[pnum];

                        /* Initialize the PCB that we are expecting to receive. */
                        iprt->ip_lport = sockptr->s_local_addr.port_num;
                        iprt->ip_socketd = i;
                        iprt->ip_protocol = sockptr->s_protocol;

#if (INCLUDE_IPV6 == NU_TRUE)

                        if (sockptr->s_family == SK_FAM_IP6)
                        {
#if (INCLUDE_IPV4 == NU_TRUE)
                            /* If the local address is an IPv4-Mapped address,
                             * extract the IPv4 address from the IPv6 address.
                             */
                            if (IPV6_IS_ADDR_V4MAPPED(sockptr->s_local_addr.ip_num.
                                                      is_ip_addrs))
                            {
                                IP6_EXTRACT_IPV4_ADDR(iprt->ip_laddr,
                                                      sockptr->s_local_addr.ip_num.
                                                      is_ip_addrs);

                                iprt->ip_faddr = IP_ADDR_ANY;
                            }

                            else
#endif
                            {
                                NU_BLOCK_COPY(iprt->ip_laddrv6,
                                              sockptr->s_local_addr.ip_num.
                                              is_ip_addrs, IP6_ADDR_LEN);

                                UTL_Zero(iprt->ip_faddrv6, IP6_ADDR_LEN);
                            }
                        }

#if (INCLUDE_IPV4 == NU_TRUE)
                        else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

                        {
                            iprt->ip_laddr =
                                IP_ADDR(sockptr->s_local_addr.ip_num.is_ip_addrs);

                            iprt->ip_faddr = IP_ADDR_ANY;
                        }

#endif
                        /* Setup a suspending task */
                        if (task_id != NU_NULL)
                        {
                            /* Setup the receive list so it can be resumed */
                            sck_tsk_ptr[ste_idx].task = task_id;

                            /* Add it to the list */
                            DLL_Enqueue(&sockptr->s_RXTask_List,
                                        &sck_tsk_ptr[ste_idx]);
                        }

                        /* task_id is NU_NULL, which indicates to remove
                         * items from the RX list that were added with the
                         * previous call to this routine before the task
                         * suspended in NU_Select().
                         */
                        else
                        {
                            /* If this is the socket that recv'd data, it
                             * was already removed from the list when it was
                             * resumed. No harm in removing it again.
                             */
                            DLL_Remove(&sockptr->s_RXTask_List,
                                       &sck_tsk_ptr[ste_idx]);
                        }

                        return_status = NU_SUCCESS;
                    }

                }   /* end of read FD set */
            }
#endif

            /* Handle the write FD set. Suspend on TX regardless of protocol
             * if it is requested by SEL_Check. Conditionally we suspend for
             * buffers to be freed. This is determined by the parameter
             * buf_ssp_elmt. Clearing entries is based on the value of task_id.
             * NU_NULL is used to indicate we are cleaning up the sockets.
             */
            if (set == SEL_WRITABLE_IDX)
            {
                /* task_id is NU_NULL, which indicates to remove items from
                 * the TX list that were added with the previous call to this
                 * routine before the task suspended in NU_Select().
                 */
                if (task_id == NU_NULL)
                {
                    /* Remove the entry from the socket's TX task list. */
                    DLL_Remove(&sockptr->s_TXTask_List, &sck_tsk_ptr[ste_idx]);

                    /* Determine if we need to also remove a mem buffer
                     * suspension element
                     */
                    if ( (buf_ssp_elmt != NU_NULL) &&
                         (buf_ssp_elmt->waiting_task == current_task) &&
                         (buf_ssp_used == NU_FALSE) )
                    {
                        /* Remove the entry from the suspension list. */
                        DLL_Remove(&MEM_Buffer_Suspension_List, buf_ssp_elmt);

                        /* Remove the task reference. */
                        buf_ssp_elmt->waiting_task = NU_NULL;

                        /* Indicate we remove the element. */
                        buf_ssp_used = NU_TRUE;
                    }

                    /* Clear the SS_WAITWINDOW flag if it is still set. */
                    else
                        sockptr->s_state &= ~SS_WAITWINDOW;
                }

                /* Setup up the socket to resume the task */
                else
                {
                    /* Setup the TX list so it can be resumed */
                    sck_tsk_ptr[ste_idx].task = task_id;

                    /* Add it to the list */
                    DLL_Enqueue(&sockptr->s_TXTask_List, &sck_tsk_ptr[ste_idx]);

                    /* Determine if we are suspending because of
                     * buffers
                     */
                    if ( (buf_ssp_elmt != NU_NULL) &&
                         (buf_ssp_elmt->waiting_task == task_id) &&
                         (buf_ssp_used == NU_FALSE) )
                    {
                        /* Set up socketd */
                        buf_ssp_elmt->socketd = i;

                        /* Set up task entry */
                        buf_ssp_elmt->list_entry = (VOID*)&sck_tsk_ptr[ste_idx];

                        /* Get a reference to the memory buffer element */
                        sck_tsk_ptr[ste_idx].buff_elmt = buf_ssp_elmt;

                        /* Put this element on the buffer suspension
                           list */
                        DLL_Enqueue(&MEM_Buffer_Suspension_List, buf_ssp_elmt);

                        /* Indicate the element is now used. */
                        buf_ssp_used = NU_TRUE;
                    }

                    /* Set the SS_WAITWINDOW flag so the task will be resumed
                     * if the TCP window opens.
                     */
                    else
                        sockptr->s_state |= SS_WAITWINDOW;
                }

                return_status = NU_SUCCESS;

            } /* end of write FD set */

            /* Increment the index into the sck_task_ptr.  This is done every
             * time through the socket's loop, regardless of the outcome of
             * the logic in the loop, because the sck_task_ptr size is big
             * enough to accommodate all sockets that are to be checked.  Note
             * that an entry might be incremented past and not referenced.
             */
            ste_idx++;

        } /* end for i to max_sockets */

    } /* end of FD sets */

    return (return_status);

} /* SEL_Setup_Ports */
