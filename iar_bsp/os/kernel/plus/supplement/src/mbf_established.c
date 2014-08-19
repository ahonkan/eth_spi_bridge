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
*       mbf_established.c
*
*   COMPONENT
*
*       MB - Mailbox Management
*
*   DESCRIPTION
*
*       This file contains the Established routine to obtain facts about
*       the Mailbox management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Established_Mailboxes            Number of created mailboxes
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

extern UNSIGNED         MBD_Total_Mailboxes;

/***********************************************************************
*
*   FUNCTION
*
*       NU_Established_Mailboxes
*
*   DESCRIPTION
*
*       This function returns the current number of established
*       mailboxes.  Mailboxes previously deleted are no longer
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
*       MBD_Total_Mailboxes                 Number of established
*                                           mailboxes
*
***********************************************************************/
UNSIGNED NU_Established_Mailboxes(VOID)
{
    /* Call stack checking function to check for an overflow condition.  */
    (VOID)NU_Check_Stack();

    /* Return the number of established mailboxes.  */
    return(MBD_Total_Mailboxes);
}
