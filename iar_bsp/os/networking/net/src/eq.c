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
*       eq.c
*
*   DESCRIPTION
*
*       Event queue manager
*
*   DATA STRUCTURES
*
*       EQ_Event_List
*       EQ_Event_Freelist
*       EQ_HandlerTable[]
*
*   FUNCTIONS
*
*       EQ_Init
*       NU_EventsDispatcher
*       EQ_Clear_Matching_Timer
*       EQ_Put_Event
*
*   DEPENDENCIES
*
*       nu_net.h
*       net_extr.h
*       nat_extr.h
*
*************************************************************************/

#include "networking/nu_net.h"
#include "networking/net_extr.h"

#if (INCLUDE_NAT == NU_TRUE)
#include "networking/nat_extr.h"
#endif

VOID NU_EventsDispatcher(UNSIGNED argc, VOID *argv);

/*
 * Define the timerlist control pointers.  These are globally used to
 * maintain the system timer list that is used for retry timeouts and
 * the like.
 */
struct tqhdr EQ_Event_List, EQ_Event_Freelist;

/* Create an array of function pointers for event registration. */
EQ_Handler EQ_HandlerTable[EQ_MAX_EVENTS];

NU_PROTECT      EQ_Table;

UINT32  EQ_ID_Counter = 0;

extern UINT16           ICMP_Echo_Req_Seq_Num;
extern ICMP_ECHO_LIST   ICMP_Echo_List;
extern BOOLEAN          NET_Timer_Event;


/*************************************************************************
*
*   FUNCTION
*
*       EQ_Init
*
*   DESCRIPTION
*
*       Initialize the head and tail pointers of both the EQ_Event_List
*       and the EQ_Event_Freelist to 0.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID EQ_Init(VOID)
{
    /* Initialize the head and tail pointers to 0 */
    EQ_Event_Freelist.flink = EQ_Event_Freelist.blink = NU_NULL;
    EQ_Event_List.flink = EQ_Event_List.blink = NU_NULL;

    /* Clear out the EQ_HandlerTable before registering any timer events. */
    UTL_Zero(EQ_HandlerTable, (sizeof(EQ_Handler) * EQ_MAX_EVENTS));

    /* Zero out the EQ_Table protection data structure */
    UTL_Zero(&EQ_Table, sizeof(NU_PROTECT));

} /* EQ_Init */

/*************************************************************************
*
*   FUNCTION
*
*       NU_EventsDispatcher
*
*   DESCRIPTION
*
*       This function is responsible for dispatching events from the
*       event queue to the appropriate handling routines.
*
*   INPUTS
*
*       argc                    Unused.
*       *argv                   Unused.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID NU_EventsDispatcher(UNSIGNED argc, VOID *argv)
{
#if (INCLUDE_TCP == NU_TRUE)
    TCP_PORT                *prt;
    NET_BUFFER              *buf_ptr;
#endif

#if (INCLUDE_TCP == NU_TRUE || INCLUDE_IP_RAW == NU_TRUE)
    struct sock_struct      *sock_ptr;    /* Socket pointer. */
#endif

    ICMP_ECHO_LIST_ENTRY    *echo_entry;
    STATUS                  status, dbg_status;
    UNSIGNED                waittime;
    TQ_EVENT                event;
    UNSIGNED                dat;
    UNSIGNED                extra_data;
    tqe_t                   *tqe_wait;              /* Timer queue entry */
    EQ_Handler              handler;
    UNSIGNED                Receive_Message[3] = {0, 0, 0};
    UNSIGNED                actual_size;
    UINT32                  tqe_id;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    tqe_wait = NU_NULL;

    /*  Remove compilation warnings for unused parameters.  */
    UNUSED_PARAMETER(argc);
    UNUSED_PARAMETER(argv);

    for (;;)
    {

        /* Retrieve a message from the event queue.  Note that if the source
           queue is empty this task suspends until something becomes
           available. */
        dbg_status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

        if (dbg_status != NU_SUCCESS)
        {
            NLOG_Error_Log("Unable to obtain TCP semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);

            NET_DBG_Notify(dbg_status, __FILE__, __LINE__,
                           NU_Current_Task_Pointer(), NU_NULL);
        }

        /* If someone is waiting on the timer list, then we need to
           calculate the next hit time for that timer.  */
        if (!tqe_wait)
            tqe_wait = EQ_Event_List.flink;

        if (tqe_wait)
        {
            /* Update the waittime. */
            waittime = TQ_Check_Duetime(tqe_wait->duetime);

            /* Get a copy of the ID, Note: tqe_wait should be pointing to the
             * front of the list
             */
            tqe_id = tqe_wait->tqe_id;
        }
        else
        {
            waittime = NU_SUSPEND;
            tqe_id = 0;
        }

        /* Release the semaphore while control is relinquished */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);

            NET_DBG_Notify(NU_INVALID_SEMAPHORE, __FILE__, __LINE__,
                           NU_Current_Task_Pointer(), NU_NULL);
        }

        /* If waittime is not NU_SUSPEND then there is a timeout value. */
        if (waittime != NU_NO_SUSPEND)
        {
            status = NU_Receive_From_Queue(&eQueue, &Receive_Message[0],
                                           (UNSIGNED)3, &actual_size,
                                           waittime);
        }
        else
        {
            NU_Relinquish();
            status = NU_TIMEOUT;
        }

        /* Obtain the semaphore so that the following test can be made */
        dbg_status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

        if (dbg_status != NU_SUCCESS)
        {
            NLOG_Error_Log("Unable to obtain TCP semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);

            NET_DBG_Notify(dbg_status, __FILE__, __LINE__,
                           NU_Current_Task_Pointer(), NU_NULL);
        }

        /* Determine if the message was received successfully.  */
        if (status == NU_TIMEOUT)
        {
            /* Verify that the ID matches.  If it does not, the structure
             * has been freed and reused, and so is invalid.
             */
            if ( (tqe_wait) && (EQ_Event_List.flink) &&
                 (EQ_Event_List.flink->tqe_id != tqe_id) )
            {
                tqe_wait = NU_NULL;

                if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                                   __FILE__, __LINE__);

                    NET_DBG_Notify(NU_INVALID_SEMAPHORE, __FILE__, __LINE__,
                                   NU_Current_Task_Pointer(), NU_NULL);

                }

                continue;
            }

            if (tqe_wait == EQ_Event_List.flink)
            {
                /*  Take off the head of the list. */
                DLL_Dequeue(&EQ_Event_List);

                if (tqe_wait)
                {
                    /* If there is a duplist, reposition it into the Events
                     * List.
                     */
                    if (tqe_wait->duplist.flink)
                    {
                        /* If there is more than one entry in the duplist,
                         * promote the first entry to the parent of the
                         * duplist.
                         */
                        if (tqe_wait->duplist.flink->flink)
                        {
                            /* Set the head of the new parent to the next node
                             * in the duplist.
                             */
                            tqe_wait->duplist.flink->duplist.flink =
                                tqe_wait->duplist.flink->flink;

                            /* Set the tail of the new parent to the old
                             * parent's tail in the duplist.
                             */
                            tqe_wait->duplist.flink->duplist.blink =
                                tqe_wait->duplist.blink;
                        }

                        /* Insert the new parent of the duplist into the head
                         * of the Event List.  Since the event just handled is
                         * the first entry in the list, and this even is a duplicate
                         * of that entry, it is safe to put it at the head of the
                         * list.
                         */
                        DLL_Insert(&EQ_Event_List, tqe_wait->duplist.flink,
                                   EQ_Event_List.flink);
                    }

                    /* Clear the duplist pointers in the node that is about to be
                       freed. */
                    tqe_wait->duplist.flink = tqe_wait->duplist.blink = NU_NULL;

                    /*  Place the dequeued entry on the free list. */
                    DLL_Enqueue(&EQ_Event_Freelist, tqe_wait);

#if (INCLUDE_TCP == NU_TRUE)

                    /* Take care of event TCPRETRANS here...other events are
                       handled by the CASE statement below... */

                    if (tqe_wait->tqe_event == TCPRETRANS)
                    {
                        /*  Get a pointer to the port list entry.  */
                        prt = TCP_Ports[tqe_wait->tqe_data];

                        TCP_Retransmit(prt);
                        tqe_wait = NU_NULL;

                        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                        {
                            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                                           __FILE__, __LINE__);

                            NET_DBG_Notify(NU_INVALID_SEMAPHORE, __FILE__, __LINE__,
                                           NU_Current_Task_Pointer(), NU_NULL);
                        }

                        continue;
                    }
                    else

#endif /* INCLUDE_TCP == NU_TRUE */

                    {
                        event = tqe_wait->tqe_event;
                        dat = tqe_wait->tqe_data;
                        extra_data = tqe_wait->tqe_ext_data;
                        tqe_wait = NU_NULL;

                        status = NU_SUCCESS;
                    }
                }
                else
                {
                    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                                       __FILE__, __LINE__);

                        NET_DBG_Notify(NU_INVALID_SEMAPHORE, __FILE__, __LINE__,
                                       NU_Current_Task_Pointer(), NU_NULL);
                    }

                    continue;
                }
            }
            else
            {
                tqe_wait = NU_NULL;

                if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                                   __FILE__, __LINE__);

                    NET_DBG_Notify(NU_INVALID_SEMAPHORE, __FILE__, __LINE__,
                                   NU_Current_Task_Pointer(), NU_NULL);
                }

                continue;
            }

        } /* end if status == NU_TIMEOUT */
        else
        {
            tqe_wait = NU_NULL;

            event       = (TQ_EVENT)Receive_Message[0];
            dat         = Receive_Message[1];
            extra_data  = Receive_Message[2];
        }

        /* Determine if the message was received successfully.  */
        if (status == NU_SUCCESS)
        {
            /* switch on the msg_class/event combination */
            switch (event)
            {
                case CONNULL:
	                NET_Timer_Event = NU_FALSE;
                    break;

                /***********  CONNECTION CLASS  **********************/

#if (INCLUDE_TCP == NU_TRUE)

                case CONFAIL:  /* connection attempt failed */
                case CONOPEN:  /* successful connect attempt */

                    /* Make sure the socket is not NULL, this is possible if a
                       TCP connection is made and immediately RESET by the
                       foreign host. */
                    sock_ptr = SCK_Sockets[dat];

                    if (sock_ptr != NU_NULL)
                    {
                        /* return control to the waiting tasks */
                        if (sock_ptr->s_TXTask_List.flink != NU_NULL)
                            SCK_Resume_All(&SCK_Sockets[dat]->s_TXTask_List, 0);
                    }

                    break;

                case TCPACK:
                    /* An ack needs to be sent. */

                    /* Get a pointer to the port. */
                    prt = TCP_Ports[dat];

                    /* Clear the ACK timer flag in the port. */
                    prt->portFlags &= (~ACK_TIMER_SET);

                    /* Send the ack. */
                    TCP_ACK_It(prt, 1);

                    break;


                case CONTX:

                    /*  get a pointer into the port list table at the
                        entry pointed to by dat in the event queue */
                    prt = TCP_Ports[dat];

                    if ( (prt->xmitFlag == NU_SET) &&
                         (prt->out.nextPacket != NU_NULL) )
                    {
                        prt->out.tcp_flags |= TPUSH;

                        /* Save a pointer to the next packet to send. Then move
                           the next pointer forward. nextPacket should always
                           be NULL after these steps. */
                        buf_ptr = prt->out.nextPacket;
                        prt->out.nextPacket = prt->out.nextPacket->next;

                        /* Clear the event flag. */
                        prt->xmitFlag = NU_CLEAR;

                        /* Send the buffer. */
                        TCP_Xmit(prt, buf_ptr);
                    }

                    break;

                case WINPROBE:

                    /* Send the Window Probe packet */
                    TCPSS_Send_Window_Probe((UINT16)dat);

                    break;

#endif /* INCLUDE_TCP == NU_TRUE */

                case SELECT:

                    dbg_status = NU_Resume_Task((NU_TASK *)dat);

                    if (dbg_status != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to resume task", NERR_RECOVERABLE,
                                       __FILE__, __LINE__);

                        NET_DBG_Notify(dbg_status, __FILE__, __LINE__,
                                       NU_Current_Task_Pointer(), NU_NULL);
                    }

                    break;

#if ( (INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_ARP == NU_TRUE) )

                case ARPRESOLVE:
                case RARP_REQUEST:

                    ARP_Event((UINT16)dat);

                    break;

#endif

                /**********************  USER CLASS *************************/

#if (INCLUDE_TCP == NU_TRUE)

                case TCPCLOSETIMEOUTSFW2:

                    /* Get a pointer to the port. */
                    prt = TCP_Ports[dat];

                    /* Send a reset just in case the other side is *
                     * still up.                                   */
                    prt->out.tcp_flags = TRESET;

                    TCP_ACK_It(prt, 1);

                    /* Change states. */
                    prt->state = SCLOSED;

                    TCP_Cleanup(prt);

                    break;

                case TCPTIMEWAIT:

                    /* Get a pointer to the port. */
                    prt = TCP_Ports[dat];

                    if (prt->state == STWAIT)
                    {
                        /* Change states. */
                        prt->state = SCLOSED;

                        /* Close the port up and free all resources. */
                        TCP_Cleanup (prt);
                    }

                    break;

#endif /* INCLUDE_TCP == NU_TRUE */


#if (INCLUDE_IP_RAW == NU_TRUE)
                case IPDATA:

                    /* If there is a task suspended pending the reception of data
                       then resume him. */
                    sock_ptr = SCK_Sockets[dat];

                    if (sock_ptr != NU_NULL)
                    {
                        if (sock_ptr->s_RXTask_List.flink != NU_NULL)
                            SCK_Resume_All(&sock_ptr->s_RXTask_List, 0);
                    }

                    break;
#endif


#if ( (INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IP_REASSEMBLY == NU_TRUE) )

                case EV_IP_REASSEMBLY :

                    /* Clear the fragment list. The whole datagram has not
                       yet been received. */
                    IP_Reassembly_Event((IP_QUEUE_ELEMENT *)dat);

                    break;
#if (INCLUDE_IPV6 == NU_TRUE)

                case EV_IP6_REASSEMBLY:

                    /* Clear the fragment list. The whole datagram has not
                    yet been received. */

                    IP6_Reassembly_Event((IP6_QUEUE_ELEMENT *)dat);

                    break;
#endif

#endif /* INCLUDE_IP_REASSEMBLY */

               /*********************** ICMP CLASS ************************/

                case ICMP_ECHO_TIMEOUT:

                    /* Search the list looking for a matching ID and seq num. */
                    echo_entry = ICMP_Echo_List.icmp_head;

                    while (echo_entry)
                    {
                        /* If this is a match, resume the suspended task */
                        if (dat == echo_entry->icmp_echo_seq_num)
                        {
                            /* Change the seq num so if the reply comes in before the
                               task runs a false success will not be generated. */
                            echo_entry->icmp_echo_seq_num = ++ICMP_Echo_Req_Seq_Num;

                            /* resume the waiting task */
                            dbg_status = NU_Resume_Task(echo_entry->icmp_requesting_task);

                            if (dbg_status != NU_SUCCESS)
                            {
                                NLOG_Error_Log("Failed to resume task", NERR_RECOVERABLE,
                                               __FILE__, __LINE__);

                                NET_DBG_Notify(dbg_status, __FILE__, __LINE__,
                                               NU_Current_Task_Pointer(), NU_NULL);
                            }

                            break;
                        }

                        echo_entry = echo_entry->icmp_next;
                    }

#if (INCLUDE_IPV6 == NU_TRUE)
#ifndef IPV6_VERSION_COMP

                    /* Previous versions of Nucleus IPv6 are not compatible with
                     * the above code.  For the sake of backward compatibility,
                     * include the old code here if using an older version of
                     * Nucleus IPv6.  This code will be removed in the future
                     * when deprecated versions of Nucleus IPv6 are obsoleted.
                     */
                    if ( (!echo_entry) && (dat != 0) )
                    {
                        /* Get a pointer to the entry for this timeout. */
                        echo_entry = (ICMP_ECHO_LIST_ENTRY *)dat;

                        /* Change the seq num so if the reply comes in before the
                           task runs a false success will not be generated. */
                        echo_entry->icmp_echo_seq_num = ++ICMP_Echo_Req_Seq_Num;

                        /* resume the waiting task */
                        dbg_status = NU_Resume_Task(echo_entry->icmp_requesting_task);

                        if (dbg_status != NU_SUCCESS)
                        {
                            NLOG_Error_Log("Failed to resume task", NERR_RECOVERABLE,
                                           __FILE__, __LINE__);

                            NET_DBG_Notify(dbg_status, __FILE__, __LINE__,
                                           NU_Current_Task_Pointer(), NU_NULL);
                        }
                    }
#endif
#endif

                    break;

#if (INCLUDE_NAT == NU_TRUE)
#if (INCLUDE_TCP == NU_TRUE)
                case NAT_CLEANUP_TCP:

                    NAT_Cleanup_TCP();
                    break;
#endif

#if (INCLUDE_UDP == NU_TRUE)

                case NAT_CLEANUP_UDP:

                    NAT_Cleanup_UDP();
                    break;
#endif

                case NAT_CLEANUP_ICMP:

                    NAT_Cleanup_ICMP();
                    break;

#if (INCLUDE_TCP == NU_TRUE)
                case NAT_TIMEOUT:
                case NAT_CLOSE:

                    NAT_Delete_Translation_Entry(IPPROTO_TCP, dat);
                    break;
#endif
#endif

                case MEM_RESUME:
                    /* Resume the task. */

                    if (dat != 0)
                    {
                        /* Validate the socket. */
                        if ( (extra_data < NSOCKETS) && (SCK_Sockets[extra_data]) )
                        {
                            /* Remove the Task Entry.  If the task is not on the
                             * s_TXTask_List queue, the task has already been
                             * removed by another thread.
                             */
                            if (DLL_Remove_Node(&SCK_Sockets[extra_data]->s_TXTask_List,
                                                (VOID *)dat) != NU_NULL)
                            {
                                /* Resume the task waiting to transmit data. */
                                dbg_status =
                                    NU_Resume_Task(((struct SCK_TASK_ENT *)dat)->task);

                                if (dbg_status != NU_SUCCESS)
                                {
                                    NLOG_Error_Log("Error occurred while resuming a task",
                                                   NERR_RECOVERABLE, __FILE__,
                                                   __LINE__);

                                    NET_DBG_Notify(dbg_status, __FILE__, __LINE__,
                                                   NU_Current_Task_Pointer(),
                                                   NU_NULL);
                                }
                            }

                            else
                            {
                                NLOG_Error_Log("Task already resumed by another thread",
                                               NERR_INFORMATIONAL, __FILE__, __LINE__);
                            }
                        }
                    }

                    else
                    {
                        NLOG_Error_Log("No data in MEM_RESUME command", NERR_FATAL,
                                       __FILE__, __LINE__);
                    }

                    break;

                default:
                    /* This may be a registered event. Look it up and call
                       the handler. */
                    if (event >= EQ_FIRST_REG_EVENT && event < EQ_LAST_REG_EVENT)
                    {
                        /* Need mutual exclusion from any possible Net task. */
                        NU_Protect(&EQ_Table);

                        handler = EQ_HandlerTable[event - EQ_FIRST_REG_EVENT];

                        NU_Unprotect();

                        if (handler != NU_NULL)
                            handler(event, dat, extra_data);
                    }

                    break;

            } /* end switch on msg_class/event combination */

        } /* end if status is NU_SUCCESS */

        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);

            NET_DBG_Notify(NU_INVALID_SEMAPHORE, __FILE__, __LINE__,
                           NU_Current_Task_Pointer(), NU_NULL);
        }

    } /* end while */

    /* Switch back to user mode. */

    /* Note this instruction is commented out to remove a compiler
       warning. The while loop above should never be exited and this
       instruction never executed. Thus the reason for the compiler
       warning and the justification to comment out this instruction.

       This line is left in just for completeness and so that in the
       future it is not overlooked.

    NU_USER_MODE();

    */

} /* NU_EventDispatcher */

/***********************************************************************
*
*   FUNCTION
*
*       EQ_Clear_Matching_Timer
*
*   DESCRIPTION
*
*       Remove all timer events from the queue that match the
*       class/event/dat.
*
*   INPUTS
*
*       *tlist                  The list from which to clear the event.
*       event                   The event to clear.
*       dat                     Data associated with the event.
*       type                    Method to remove timer from the TQ list
*       ext_data                Extra data associated with the event.
*
*   OUTPUTS
*
*       NU_SUCCESS
*
*************************************************************************/
STATUS EQ_Clear_Matching_Timer(struct tqhdr *tlist, TQ_EVENT event,
                               UNSIGNED dat, INT16 type, UNSIGNED ext_data)
{
    INT8  match;
    tqe_t *ent,      /* Points to the current entry in the queue. */
          *tmpent,   /* Points to the item being promoted from the duplist. */
          *savent;   /* Preserves our position in the queue. */

    /* Search the list for matching timers. */
    for (ent = tlist->flink; ent;)
    {
        match = NU_FALSE;
        switch (type)
        {
        case TQ_CLEAR_ALL:
            /* Event must match. Both data members are disregarded. */
            if (ent->tqe_event == event)
                match = NU_TRUE;

            break;

        case TQ_CLEAR_ALL_EXTRA:
            /* Event and data must match. */
            if ((ent->tqe_event == event) && (ent->tqe_data == dat))
                match = NU_TRUE;

            break;

        case TQ_CLEAR_EXACT:
            /* Event, data, and ext_data must match. */
            if ((ent->tqe_event == event) && (ent->tqe_data == dat)
                && (ent->tqe_ext_data == ext_data))
                match = NU_TRUE;

            break;

        case TQ_CLEAR_SEQ:
            /* Event and data must match. If sequence number (tqe_ext_data) is
               less than the parameter ext_data, it will be removed. The casting
               of the parameters in INT32_CMP is due to Problem Report #339. */
            if ( (ent->tqe_event == event) && (ent->tqe_data == dat)
                && (INT32_CMP((UINT32)ext_data, (UINT32)ent->tqe_ext_data) >= 0) )
                match = NU_TRUE;

            break;

        default:
            return NU_INVALID_PARM;

        }

        /* If a match was found, ent will be an entry to remove. Otherwise,
           check its duplist for a match and continue. */
        if (match)
        {
            /* We have found a matching entry.  Preserve a pointer to the next
               entry. */
            savent = ent->flink;

            /* If this entry contains a duplist we need to search it too. */
            if (ent->duplist.flink)
            {
                /* Search the duplist for a match. */
                EQ_Clear_Matching_Timer(&ent->duplist, event, dat,
                                                  type, ext_data);

                /* Pull the first item, if one exists, off the duplist. */
                tmpent = (tqe_t *)DLL_Dequeue(&ent->duplist);

                /* If the duplist still contained an item after we cleared it,
                   we want to promote one. */
                if (tmpent)
                {
                     /* Promote this item to the position that was held by the item
                        we are removing. */
                     tmpent->duplist.flink = ent->duplist.flink;
                     tmpent->duplist.blink = ent->duplist.blink;

                     DLL_Insert((VOID *)tlist, tmpent, ent);
                }

                /* Clear the duplist pointers in the node that is about to be
                   freed. */
                ent->duplist.flink = ent->duplist.blink = NU_NULL;
            }

            /* Remove the item. */
            DLL_Remove((VOID *)tlist, ent);

            /* Place the item back on the free list. */
            DLL_Enqueue(&EQ_Event_Freelist, ent);

            /* Point to the next item to be checked. */
            ent = savent;
        }
        else
        {
            /* We did not find a match.  Clear the duplist if it exists. */
            if (ent->duplist.flink)
            {
                EQ_Clear_Matching_Timer(&ent->duplist, event, dat,
                                                  type, ext_data);
            }

            /* Point to the next item to be checked. */
            ent = ent->flink;
        }
    }

    return (NU_SUCCESS);

} /* EQ_Clear_Matching_Timer */

/*************************************************************************
*
*   FUNCTION
*
*       EQ_Put_Event
*
*   DESCRIPTION
*
*       Add an event to the queue.
*       Returns 0 if there was room, 1 if an event was lost.
*
*   INPUTS
*
*       event                   An event to put on the list
*       dat                     Any message data or pointer.
*       extra                   Any message data or pointer.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       1
*
************************************************************************/
STATUS EQ_Put_Event(TQ_EVENT event, UNSIGNED dat, UNSIGNED extra)
{
    STATUS      status;
    UNSIGNED    Send_Message[3];

    /* Send a message to the dispatcher's event queue. */
    Send_Message[0] = (UNSIGNED)event;
    Send_Message[1] = dat;
    Send_Message[2] = extra;

    status =  NU_Send_To_Queue(&eQueue, &Send_Message[0], (UNSIGNED)3,
                              (UNSIGNED)NU_NO_SUSPEND);

    /* Determine if the message was sent successfully.  */
    if (status != NU_SUCCESS)
        status = 1;

    return (status);

} /* EQ_Put_Event */


