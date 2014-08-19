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
*       pif_established.c
*
*   COMPONENT
*
*       PI - Pipe Management
*
*   DESCRIPTION
*
*       This file contains the Established routine to obtain facts
*       about the Pipe management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Established_Pipes                Number of created pipes
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

extern UNSIGNED         PID_Total_Pipes;

/***********************************************************************
*
*   FUNCTION
*
*       NU_Established_Pipes
*
*   DESCRIPTION
*
*       This function returns the current number of established
*       pipes.  Pipes previously deleted are no longer considered
*       established.
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
*       PID_Total_Pipes                     Number of established
*                                           pipes
*
***********************************************************************/
UNSIGNED NU_Established_Pipes(VOID)
{
    /* Call stack checking function to check for an overflow condition.  */
    (VOID)NU_Check_Stack();

    /* Return the number of established pipes.  */
    return(PID_Total_Pipes);
}
