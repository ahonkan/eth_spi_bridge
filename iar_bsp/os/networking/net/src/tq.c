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
*       tq.c
*
*   COMPONENT
*
*       TQ -- Timer Event Queue functions.
*
*   DESCRIPTION
*
*       Timer event queue routines used by the Net stack.
*
*   DATA STRUCTURES
*
*
*   FUNCTIONS
*
*       TQ_Post
*       TQ_Timerset
*       TQ_Timerunset
*       TQ_Check_Duetime
*       TQ_Calc_Wait_Time
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

extern NU_TASK NU_EventsDispatcher_ptr;

#if (INCLUDE_STATIC_BUILD == NU_TRUE)

/* Declare memory for the TQ entries */
tqe_t    NET_TQ_Entry_Memory[NET_MAX_TQ_ENTRIES];

/* Declare counter to counter to count the entries */
INT      No_of_Entry    = 0 ;
#endif

/* Flag to track if timer event is present in event queue. */
BOOLEAN NET_Timer_Event = NU_FALSE;

UNSIGNED TQ_Calc_Wait_Time(UNSIGNED current_time, UNSIGNED duetime);


/*************************************************************************
*
*   FUNCTION
*
*       TQ_Post
*
*   DESCRIPTION
*
*       Insert a timer queue entry into a delta list.  The delta list
*       queue is ordered by increasing due time.
*
*   INPUTS
*
*       *tqlist                 Pointer to the time queue list
*       *tqe                    Pointer to the timer queue
*
*   OUTPUTS
*
*       NU_SUCCESS
*
*************************************************************************/
STATUS TQ_Post(tqe_t *tqlist, tqe_t *tqe)
{
    tqe_t    *tqpos;
    UNSIGNED curtime;
    UNSIGNED duewait;
    UNSIGNED duetime = tqe->duetime;   /*  Pick up the duetime for the new entry. */

    /* Clear out the new entry's duplist. */
    tqe->duplist.flink = NU_NULL;
    tqe->duplist.blink = NU_NULL;

    /* get the current time once. */
    curtime = NU_Retrieve_Clock();

    /* based on the current time, determine how long a wait will be required
       to reach the duetime.  Take into account any wrapping of the clock. */
    duewait = TQ_Calc_Wait_Time(curtime, duetime);

    /*  Search to see if this timer needs to be inserted into the list or
        if it is a duplicate entry.  */
    for (tqpos = tqlist->flink; tqpos; tqpos = tqpos->flink)
        if ( duewait <= TQ_Calc_Wait_Time(curtime, tqpos->duetime) )
            break;

    /*  If we found something, the new item needs to be added to a duplist or
        it needs to be inserted.  */
    if (tqpos)
    {
        /*  It needs to be added to a duplist. */
        if (tqe->duetime == tqpos->duetime)
            DLL_Enqueue((tqe_t *)&tqpos->duplist, tqe);
        else
        {
            /*  It needs to be inserted. */
            DLL_Insert((tqe_t *) tqlist, tqe, tqpos);
        }
    }
    else
    {
        /*  If we did not find that the new entry has an equal time (duplicate) or
            needed to be inserted, then add it to the end of the list.  */
        DLL_Enqueue((tqe_t *) tqlist, tqe);
    }

    return (NU_SUCCESS);

} /* TQ_Post */

/*************************************************************************
*
*   FUNCTION
*
*       TQ_Timerset
*
*   DESCRIPTION
*
*       Set an async timer, and when time elapses sticks an event in the
*       network event queue.
*
*       Class, event, dat is what gets posted when howlong times out.
*
*   INPUTS
*
*       event                   The event to attach to the timer
*       dat                     Data or pointer attached to the event
*       howlong                 How long for the timer
*       extra                   An extra parameter for data.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       -1
*
*************************************************************************/
STATUS TQ_Timerset(TQ_EVENT event, UNSIGNED dat, UNSIGNED howlong,
                   UNSIGNED extra)
{
    tqe_t       *tqe;
    STATUS      status;
    tqe_t       *return_ptr = NU_NULL;

    /* Get an entry from the freelist.  */
    tqe = (tqe_t *) DLL_Dequeue (&EQ_Event_Freelist);

    /* Check to see if an entry was found.  If one was not found, need
       to allocate a new one. */
    if (!tqe)
    {
        /* Get some memory for the new entry. */

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
        status = NU_Allocate_Memory(MEM_Cached, (VOID **) &return_ptr,
                                (UNSIGNED)sizeof(tqe_t),
                                (UNSIGNED)NU_NO_SUSPEND);
#else
        /* Assign memory to the TQ entry */
        if(No_of_Entry != NET_MAX_TQ_ENTRIES )
        {
            return_ptr = &NET_TQ_Entry_Memory[No_of_Entry++];
            status = NU_SUCCESS;
        }
        else
            status = NU_NO_MEMORY;
#endif

        /* check status of memory allocation */
        if (status == NU_SUCCESS)
        {
            return_ptr = (tqe_t *)TLS_Normalize_Ptr(return_ptr);
            tqe = return_ptr;
        }
        else
        {
            NLOG_Error_Log ("Unable to alloc memory for timer entry", NERR_RECOVERABLE,
                                __FILE__, __LINE__);
            return (-1);
        }
    }

    /* Set up the new entry. */
    tqe->tqe_event = event;
    tqe->tqe_data = dat;
    tqe->tqe_ext_data = extra;
    tqe->duetime = NU_Retrieve_Clock() + (UNSIGNED) howlong;
    tqe->tqe_id = EQ_ID_VALUE;


    /* Clear out the new entry's duplist. */
    tqe->duplist.flink = (tqe_t *)NU_NULL;
    tqe->duplist.blink = (tqe_t *)NU_NULL;

    /* Place the new entry on the timerlist. */
    TQ_Post ((tqe_t *) &EQ_Event_List, tqe);

    /* Check to see if the current task is the  events dispatcher.  If it
       is we do not want to place an item onto the event queue.  If the queue
       is full the events dispatcher will suspend on the queue, and since
       the events dispatcher is the only task that removes items from the
       queue, deadlock will occur. */
    if (NU_Current_Task_Pointer() == &NU_EventsDispatcher_ptr)
    {
        return (NU_SUCCESS);
    }

    /* Wake up the event dispatcher from an indefinite wait
       if this is the first entry on the timer list and event 
	   dispatcher queue does not have timer event. */
    if ( (EQ_Event_List.flink == tqe) && (NET_Timer_Event == NU_FALSE) )
	{
        if (EQ_Put_Event(CONNULL, dat, 0) == NU_SUCCESS)
        {
        	NET_Timer_Event = NU_TRUE;
        }
    }

    return (NU_SUCCESS);

} /* TQ_Timerset */

/*************************************************************************
*
*   FUNCTION
*
*       TQ_Timerunset
*
*   DESCRIPTION
*
*       Remove all timer events from the queue that match the
*       class/event/dat.
*
*   INPUTS
*
*       event                   The event to unset the timer
*       type                    The type of clear to perform:
*
*                               TQ_CLEAR_EXACT - Clear the event based
*                                   on event, data, and extra data
*                               TQ_CLEAR_SEQ - Clear the event based
*                                   on TCP sequence number.
*                               TQ_CLEAR_ALL - Clear all instances of
*                                   the event.
*
*       dat                     Data parameter for the event
*       extra                   Which ack was sent
*
*   OUTPUTS
*
*       Nucleus Status Code
*
*************************************************************************/
STATUS TQ_Timerunset(TQ_EVENT event, INT16 type, UNSIGNED dat, UNSIGNED extra)
{
    return (EQ_Clear_Matching_Timer(&EQ_Event_List, event, dat, type, extra));

} /* TQ_Timerunset */

/*************************************************************************
*
*   FUNCTION
*
*       TQ_Check_Duetime
*
*   DESCRIPTION
*
*       Check to see if the due time has expired.  The function will take
*       into account if the clock has wrapped.
*
*   INPUTS
*
*       due_time                When the time is up
*
*   OUTPUTS
*
*       UINT32
*
*************************************************************************/
UNSIGNED TQ_Check_Duetime(UNSIGNED duetime)
{
    UNSIGNED  current_time;
    UNSIGNED  wait_time;

    current_time = NU_Retrieve_Clock();
    wait_time = TQ_Calc_Wait_Time(current_time, duetime);

    return (wait_time);

} /* TQ_Check_Duetime */

/*************************************************************************
*
*   FUNCTION
*
*       TQ_Calc_Wait_Time_Time
*
*   DESCRIPTION
*
*       Given the the current time, determine how long a wait will be
*       required to reach the duetime.  Take into account any wrapping
*       of the clock.
*
*   INPUTS
*
*       current_time            The current time.
*       duetime                 When the time is up
*
*   OUTPUTS
*
*       The amount of time to wait from current_time to duetime.
*       If the curent time has past due time return NU_NO_SUSPEND
*
*************************************************************************/
UNSIGNED TQ_Calc_Wait_Time(UNSIGNED current_time, UNSIGNED duetime)
{
    UNSIGNED  wait_time;

    if (duetime > current_time)
        if ( (duetime - current_time) >= 0x80000000UL )
            wait_time = NU_NO_SUSPEND;
        else
            wait_time = (duetime - current_time);
    else
        if ( ( current_time - duetime ) > 0x80000000UL )
            wait_time = ((0xFFFFFFFFUL - current_time) + duetime) + 1;
        else
            wait_time = NU_NO_SUSPEND;

    return (wait_time);

} /* TQ_Calc_Wait_Time */
