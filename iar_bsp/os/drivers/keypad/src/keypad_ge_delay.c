/*************************************************************************
*
*            Copyright 2010 Mentor Graphics Corporation
*                      All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
*   FILE NAME
*
*       keypad_ge_delay.c
*
*   COMPONENT
*
*       Keypad Driver
*
*   DESCRIPTION
*
*       This file contains the routine to delay for specified number of
*       microseconds for keypad driver.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       KP_Delay_usec
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       keypad_ge_extr.h
*
*************************************************************************/

#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "drivers/keypad_ge_extr.h"

/*************************************************************************
*
*   FUNCTION
*
*       KP_Delay_usec
*
*   DESCRIPTION
*
*       Delay for the specified microseconds.
*
*   INPUTS
*
*       no_of_usec                          No of microseconds to delay
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID KP_Delay_usec(UINT32 no_of_usec)
{
    UNSIGNED    timer_ticks;
    UNSIGNED    start_time;
    UNSIGNED    end_time;
    UNSIGNED    loop_counter;

    /* Calculate loop counter. */
    loop_counter = (no_of_usec / 1000) + 1;

    /* Reevaluate count. */
    no_of_usec = no_of_usec % 1000;

    do
    {
        /* Retrieve Start Time value */
        NU_Retrieve_Hardware_Clock(start_time);

        /* Calculate number of timer ticks for count */
        timer_ticks = ((NU_HW_Ticks_Per_Second / 100) * no_of_usec) / 10000;

        do
        {
            /* Retrieve Clock value */
            NU_Retrieve_Hardware_Clock(end_time);

        } while (KP_CALC_TIME(start_time, end_time) < timer_ticks);

        /* Reset count. */
        no_of_usec = 1000;

        /* Decrement loop counter. */
        loop_counter--;

    } while (loop_counter > 0);
}
