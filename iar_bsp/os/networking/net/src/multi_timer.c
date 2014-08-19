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
*       multi_timer.c
*
*   COMPONENT
*
*       Multicasting
*
*   DESCRIPTION
*
*       This file contains the routines to set and clear timers and
*       validate the return status.  These routines were created to
*       decrease the code size due to the number of timers set and
*       cleared in IGMP and MLD.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       Multi_Set_Timer
*       Multi_Unset_Timer
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

/*************************************************************************
*
*   FUNCTION
*
*       Multi_Set_Timer
*
*   DESCRIPTION
*
*       This function is called to set a multicast timer.
*
*   INPUTS
*
*       event                   The timer to set.
*       data1                   A data element to record.
*       howlong                 The timer expiration.
*       data2                   The second data element to record.
*       multi                   pointer to the MULTI_DATA object
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID Multi_Set_Timer(TQ_EVENT event, UNSIGNED data1, UNSIGNED howlong,
                     UNSIGNED data2, MULTI_DATA *multi)
{
    STATUS  status;

    /* Set the timer */
    status = TQ_Timerset(event, data1, howlong, data2);

    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to set the multicast timer",
                       NERR_SEVERE, __FILE__, __LINE__);

        NET_DBG_Notify(status, __FILE__, __LINE__,
                       NU_Current_Task_Pointer(), NU_NULL);
    }
    else
    {
        /* Record the time at which the clock was last set. */
        multi->multi_set_time = NU_Retrieve_Clock();
    }

} /* Multi_Set_Timer */

/*************************************************************************
*
*   FUNCTION
*
*       Multi_Unset_Timer
*
*   DESCRIPTION
*
*       This function is called to clear a multicast timer.
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
*       data1                   Data parameter for the event
*       data2                   Additional data parameter for the event
*       multi                   pointer to the MULTI_DATA object
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID Multi_Unset_Timer(TQ_EVENT event, INT16 type, UNSIGNED data1,
                       UNSIGNED data2, MULTI_DATA *multi)
{
    STATUS  status;

    /* Clear the timer */
    status = TQ_Timerunset(event, type, data1, data2);

    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to unset the multicast timer",
                       NERR_SEVERE, __FILE__, __LINE__);

        NET_DBG_Notify(status, __FILE__, __LINE__,
                       NU_Current_Task_Pointer(), NU_NULL);
    }
    else
    {
        /* Reset the set time. */
        multi->multi_set_time = 0;
    }

} /* Multi_Unset_Timer */
