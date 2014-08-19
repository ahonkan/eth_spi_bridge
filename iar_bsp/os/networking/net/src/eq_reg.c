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
*       eq_reg.c
*
* DESCRIPTION
*
*       This file contains routines for Net event registration.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       EQ_Register_Event
*       EQ_Unregister_Event
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

extern EQ_Handler   EQ_HandlerTable[];
extern NU_PROTECT   EQ_Table;

/***********************************************************************
*
*   FUNCTION
*
*       EQ_Register_Event
*
*   DESCRIPTION
*
*       Register an event with the Events Dispatcher.
*
*   INPUTS
*
*       handler                 The new event to register.
*       *newevt                 A pointer to the memory into which the
*                               event index will be placed.  This index
*                               will be used by the caller to set future
*                               events for the respective handler.
*
*   OUTPUTS
*
*       NU_SUCCESS              The operation completed successfully.
*       NU_TABLE_FULL           Handler array is full. No more allowed.
*       NU_INVALID_PARM         Handler or newevt parameter are invalid.
*
*************************************************************************/
STATUS EQ_Register_Event(EQ_Handler handler, TQ_EVENT *newevt)
{
    TQ_EVENT    index;
    STATUS      status = NU_INVALID_PARM;

    if ( (handler == NU_NULL) || (newevt == NU_NULL) )
        status = NU_INVALID_PARM;
    else
    {
        /* Need mutual exclusion from any possible Net task. */
        NU_Protect(&EQ_Table);

        /* Search the list for an empty slot. */
        for (index = 0; index < EQ_MAX_EVENTS; index++)
        {
            if (EQ_HandlerTable[index] == NU_NULL)
            {
                /* An empty slot was found, so assign it to the caller. */
                EQ_HandlerTable[index] = handler;

                *newevt = index + EQ_FIRST_REG_EVENT;
                status = NU_SUCCESS;

                break;
            }
        }

        /* If an empty slot is not found, abort. */
        if (index >= EQ_MAX_EVENTS)
            status = NU_TABLE_FULL;

        NU_Unprotect();
    }

    /* Add the offset for registered events. */
    return (status);

} /* EQ_Register_Event */

/***********************************************************************
*
*   FUNCTION
*
*       EQ_Unregister_Event
*
*   DESCRIPTION
*
*       Unregister an event that was previously registered with
*       the Events Dispatcher.
*
*   INPUTS
*
*       index                   The index into the handler array.  This
*                               is the TQ_EVENT returned by
*                               EQ_Register_Event when the event was
*                               initially registered.
*
*   OUTPUTS
*
*       NU_SUCCESS              Success.
*       NU_NOT_FOUND            The event was not registered.
*       NU_INVALID_PARM         Invalid index value.
*
*************************************************************************/
STATUS EQ_Unregister_Event(TQ_EVENT index)
{
    STATUS  status;

    if ( (index < EQ_FIRST_REG_EVENT) || (index > EQ_LAST_REG_EVENT) )
        return (NU_INVALID_PARM);

    /* Need mutual exclusion from any possible Net task. */
    NU_Protect(&EQ_Table);

    /* Remove it from the table. */
    if (EQ_HandlerTable[index - EQ_FIRST_REG_EVENT] == NU_NULL)
        status = NU_NOT_FOUND;
    else
    {
        /* Delete the handler from the slot. */
        EQ_HandlerTable[index - EQ_FIRST_REG_EVENT] = NU_NULL;

        /* Remove any matching events from the timer event queue. */
        TQ_Timerunset(index, TQ_CLEAR_ALL, 0, 0);

        status = NU_SUCCESS;
    }

    NU_Unprotect();

    return (status);

} /* EQ_Unregister_Event */


