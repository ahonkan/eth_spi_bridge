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
*       dmf_established.c
*
*   COMPONENT
*
*       DM - Dynamic Memory Management
*
*   DESCRIPTION
*
*       This file contains routine to obtain Established number of
*       Dynamic Memory pools.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Established_Memory_Pools         Number of dynamic pools
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

extern UNSIGNED         DMD_Total_Pools;

/***********************************************************************
*
*   FUNCTION
*
*       NU_Established_Memory_Pools
*
*   DESCRIPTION
*
*       This function returns the current number of established
*       memory pools.  Pools previously deleted are no longer
*       considered established.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       [NU_Check_Stack]                    Stack checking function
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       DMD_Total_Pools                     Number of established
*                                           dynamic memory pools
*
***********************************************************************/
UNSIGNED NU_Established_Memory_Pools(VOID)
{
    /* Call stack checking function to check for an overflow condition.  */
    (VOID)NU_Check_Stack();

    /* Return the number of established dynamic memory pools.  */
    return(DMD_Total_Pools);
}
