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
*       evf_established.c
*
*   COMPONENT
*
*       EV - Event Group Management
*
*   DESCRIPTION
*
*       This file contains routine to obtain Established number of
*       Event Groups
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Established_Event_Groups         Number of created groups
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                    Thread Control functions
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"

/* Define external inner-component global data references.  */

extern UNSIGNED         EVD_Total_Event_Groups;


/***********************************************************************
*
*   FUNCTION
*
*       NU_Established_Event_Groups
*
*   DESCRIPTION
*
*       This function returns the current number of established
*       event groups.  Event groups previously deleted are no longer
*       considered established.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       [NU_Check_Stack]                    Stack checking function
*                                           (conditionally compiled)
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       EVD_Total_Event_Groups              Number of established
*                                           event groups
*
***********************************************************************/
UNSIGNED NU_Established_Event_Groups(VOID)
{
    /* Call stack checking function to check for an overflow condition.  */
    (VOID)NU_Check_Stack();

    /* Return the number of established event groups.  */
    return(EVD_Total_Event_Groups);
}

