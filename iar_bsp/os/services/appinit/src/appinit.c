/*************************************************************************
*
*            Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/
/*************************************************************************
*
*   FILE NAME
*
*       appinit.c
*
*   DESCRIPTION
*
*       This file contains the initialization function for kicking off
*       'Application_Initialize'.
*
*   FUNCTIONS
*
*       nu_os_svcs_appinit_init
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus definitions
*
*************************************************************************/

#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "services/nu_services.h"

/*************************************************************************
*
*   FUNCTION
*
*      nu_os_svcs_appinit_init
*
*   DESCRIPTION
*
*      A simple function for calling 'Application_Initialize' as a part of
*      run-level initialization.
*
*   INPUTS
*
*      path - registry key for component specific settings (Unused).
*
*      startstop - value to specify whether to start (NU_START) 
*                  or stop (NU_STOP) a given component.
*
*   OUTPUTS
*
*       NU_SUCCESS - Indicates successful operation.
*
*************************************************************************/
STATUS nu_os_svcs_appinit_init(CHAR *path, INT startstop)
{
    OPTION  current_preempt;
    NU_MEMORY_POOL  *usys_pool_ptr;
    NU_MEMORY_POOL  *sys_pool_ptr;
    STATUS          status;

    /* Suppress warnings */
    NU_UNUSED_PARAM(path);

    /* Determine how to proceed based on the parameters. */
    if (startstop == RUNLEVEL_START)
    {
        /* Disable pre-emption around call to create the application main
           task. */
        current_preempt = NU_Change_Preemption(NU_NO_PREEMPT);

        /* Get system uncached memory pools pointer */
        status = NU_System_Memory_Get(&sys_pool_ptr, &usys_pool_ptr);

        if (status == NU_SUCCESS)
        {
            /* Call application entry function. */
            Application_Initialize(sys_pool_ptr, usys_pool_ptr);
        }
        
        /* Restore prior preemption state. */
        NU_Change_Preemption(current_preempt);
        
    }

    return (NU_SUCCESS);
}
