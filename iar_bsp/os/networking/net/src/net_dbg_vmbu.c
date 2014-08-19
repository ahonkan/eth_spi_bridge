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
*       net_dbg_vmbu.c
*
*   DESCRIPTION
*
*       This file contains the routine for validating that the Free List
*       and the MEM_Buffers_Used global variable are in sync.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NET_DBG_Validate_MEM_Buffers_Used
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

#if (NU_DEBUG_NET == NU_TRUE)

/**************************************************************************
*
*   FUNCTION
*
*       NET_DBG_Validate_MEM_Buffers_Used
*
*   DESCRIPTION
*
*       This function determines if the current value of MEM_Buffers_Used
*       is valid based on the number of buffers on the Free List.  The
*       routine counts the number of buffers on the Free List, adds
*       it to the value of MEM_Buffers_Used, and compares it with the
*       number of buffers that were created at start up, MAX_BUFFERS.
*       If the number of buffers found does not match the number of
*       buffers that were initially created, the function returns an
*       error.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       NU_SUCCESS              The number of buffers in the system is
*                               equal to the number of buffers created
*                               at start up.
*       NET_TOO_MANY_BUFFERS    The number of buffers computed is greater
*                               than the number of buffers that should
*                               exist in the system.
*       NET_TOO_FEW_BUFFERS     The number of buffers computed is less than
*                               the number of buffers that should exist in
*                               the system.
*
****************************************************************************/
STATUS NET_DBG_Validate_MEM_Buffers_Used(VOID)
{
    INT             previous_int_value;
    UINT16          i = 0;
    NET_BUFFER      *cur_buf;
    STATUS          status;

    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();

    /* Lock out interrupts. */
    previous_int_value =
        NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* Determine the numbers of buffers on the Free List */
    cur_buf = MEM_Buffer_Freelist.head;

    /* Exit the loop once i reaches the max number of buffers that can
     * exist in the system to avoid an infinite loop.
     */
    while ( (cur_buf) && (i < MAX_BUFFERS) )
    {
        /* Increment the number of buffers found */
        i++;

        /* Get a pointer to the next buffer */
        cur_buf = cur_buf->next;
    }

    /* Check if there are more buffers in the list than there should be */
    if ( ((i + MEM_Buffers_Used) > MAX_BUFFERS) || (cur_buf) )
        status = NET_TOO_MANY_BUFFERS;

    /* There are less buffers in the list than there should be */
    else if ((i + MEM_Buffers_Used) < MAX_BUFFERS)
        status = NET_TOO_FEW_BUFFERS;

    else
        status = NU_SUCCESS;

    /* Re-enable interrupts. */
    NU_Local_Control_Interrupts(previous_int_value);

    NU_USER_MODE();

    return (status);

} /* NET_DBG_Validate_MEM_Buffers_Used */

#endif
