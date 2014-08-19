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
*       pmf_established.c
*
*   COMPONENT
*
*       PM - Partition Memory Management
*
*   DESCRIPTION
*
*       This file contains the Established routine to obtain facts about
*       the Partition Memory Management component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Established_Partition_Pools      Number of partition pools
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       thread_control.h                    Thread Control functions
*
************************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"

/* Define external inner-component global data references.  */

extern UNSIGNED         PMD_Total_Pools;


/***********************************************************************
*
*   FUNCTION
*
*       NU_Established_Partition_Pools
*
*   DESCRIPTION
*
*       This function returns the current number of established
*       partition pools.  Pools previously deleted are no longer
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
*       PMD_Total_Pools                     Number of established
*                                           partition pools
*
***********************************************************************/
UNSIGNED NU_Established_Partition_Pools(VOID)
{
    /* Call stack checking function to check for an overflow condition.  */
    (VOID)NU_Check_Stack();

    /* Return the number of established partition pools.  */
    return(PMD_Total_Pools);
}
