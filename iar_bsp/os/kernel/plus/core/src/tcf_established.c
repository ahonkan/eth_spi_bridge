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
*       tcf_established.c
*
*   COMPONENT
*
*       TC - Thread Control
*
*   DESCRIPTION
*
*       This file contains information (fact) Established routines for
*       the Thread Control component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Established_Tasks                Number of created tasks
*       NU_Established_HISRs                Number of created HISRs
*       TCF_Established_Application_Tasks   Number of created application tasks
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                           Thread Control functions
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"

/* Define external inner-component global data references.  */

extern UNSIGNED             TCD_Total_Tasks;
extern UNSIGNED             TCD_Total_HISRs;
extern UNSIGNED             TCD_Total_App_Tasks;

/***********************************************************************
*
*   FUNCTION
*
*       NU_Established_Tasks
*
*   DESCRIPTION
*
*       This function returns the current number of established tasks.
*       Tasks previously deleted are no longer considered established.
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
*       TCD_Total_Tasks                     Number of established tasks
*
***********************************************************************/
UNSIGNED NU_Established_Tasks(VOID)
{
    /* Call stack checking function to check for an overflow condition.  */
    (VOID)NU_Check_Stack();

    /* Return the number of established tasks.  */
    return(TCD_Total_Tasks);
}


/***********************************************************************
*
*   FUNCTION
*
*       NU_Established_HISRs
*
*   DESCRIPTION
*
*       This function returns the current number of established HISRs.
*       HISRs previously deleted are no longer considered established.
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
*       TCD_Total_HISRs                     Number of established HISRs
*
***********************************************************************/
UNSIGNED NU_Established_HISRs(VOID)
{
    /* Call stack checking function to check for an overflow condition.  */
    (VOID)NU_Check_Stack();

    /* Return the number of established HISRs.  */
    return(TCD_Total_HISRs);
}

/***********************************************************************
*
*   FUNCTION
*
*       TCF_Established_Application_Tasks
*
*   DESCRIPTION
*
*       This function returns the current number of established
*       application tasks.  Tasks previously deleted are no longer
*       considered established.
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
*       TCD_Total_Tasks                     Number of established tasks
*
***********************************************************************/
UNSIGNED  TCF_Established_Application_Tasks(VOID)
{
    /* Call stack checking function to check for an overflow condition.  */
    (VOID)NU_Check_Stack();

    /* Return the number of established tasks.  */
    return(TCD_Total_App_Tasks);
}

