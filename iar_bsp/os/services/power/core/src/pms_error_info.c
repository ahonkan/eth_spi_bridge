/*************************************************************************
*
*            Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
* FILE NAME
*
*       pms_error_info.c
*
* COMPONENT
*
*       PMS - Power Error/Exception Handling
*
* DESCRIPTION
*
*       This file contains the fact routines for the exception handling
*       component.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       NU_PM_Get_Error_Handler
*
* DEPENDENCIES
*
*       power_core.h
*       initialization.h
*
*************************************************************************/

#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "services/power_core.h"
#include    "os/services/power/core/inc/initialization.h"

/* Global structures used in this file */
extern VOID (*PMS_Error_Handler)(STATUS, VOID *, VOID *, UINT32);
extern NU_PROTECT PMS_Error_Protect;

/*************************************************************************
*
* FUNCTION
*
*       NU_PM_Get_Error_Handler
*
* DESCRIPTION
*
*       This function returns the error handler that is currently
*       used in the system.
*
* INPUTS
*
*       error_ptr           Where error handler will be written
*
* OUTPUTS
*
*       NU_SUCCESS          Function returns success
*       PM_NOT_INITIALIZED  PMS is not initialized
*       PM_INVALID_POINTER  The provided pointer is invalid
*
*************************************************************************/
STATUS NU_PM_Get_Error_Handler(VOID(**error_ptr)(STATUS, VOID *, VOID *, UINT32))
{
    STATUS pm_status = NU_SUCCESS;
    
    NU_SUPERV_USER_VARIABLES

    /* Check to see if the power management services is initialized */
    if (PMS_Initialization_Status_Check() != NU_SUCCESS)
    {
        pm_status = PM_NOT_INITIALIZED;
    }
    /* Check for a null pointer */
    else if (error_ptr == NU_NULL)
    {
        pm_status = PM_INVALID_POINTER;
    }
    else
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();
        
        /* protect the global structure */
        NU_Protect(&PMS_Error_Protect);

        /* Write original exception for return */
        *error_ptr = PMS_Error_Handler;

        /* Update is done now remove the protection */
        NU_Unprotect();
        
        /* Return to user mode */
        NU_USER_MODE();
    }

    return(pm_status);
}
