/***********************************************************************
*
*            Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
************************************************************************

************************************************************************
*
*   FILE NAME
*
*       tcct_interrupts.c
*
*   COMPONENT
*
*       TC - Thread Control
*
*   DESCRIPTION
*
*       This file contains core control interrupts thread control /
*       scheduling function with target dependencies
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Control_Interrupts               Enable / disable interrupts
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                    Thread Control functions
*
***********************************************************************/
/* Include necessary header files */
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"

/***********************************************************************
*
*   FUNCTION
*
*       NU_Control_Interrupts
*
*   DESCRIPTION
*
*       This function enables and disables interrupts as specified by
*       the caller. Interrupts disabled by this call are left disabled
*       until the another call is made to enable them.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       None
*
*   INPUTS
*
*       new_level                           New interrupt enable level
*
*   OUTPUTS
*
*       old_level                           Previous interrupt enable
*                                           level
*
***********************************************************************/
INT NU_Control_Interrupts(R1 INT new_level)
{
    R2 INT old_level = new_level;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE_ISR();

    /* Disable interrupts */
    ESAL_GE_INT_FAST_ALL_DISABLE();

    /* Save current interrupt level */
    old_level = TCD_Interrupt_Level;

    /* Setup new interrupt level */
    TCD_Interrupt_Level = new_level;

    /* Set interrupt level to new level */
    ESAL_GE_INT_BITS_SET(new_level);

    /* Return to user mode */
    NU_USER_MODE_ISR();

    /* Return old interrupt level */
    return(old_level);
}
