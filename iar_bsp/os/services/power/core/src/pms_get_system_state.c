/*************************************************************************
*
*               Copyright 2010 Mentor Graphics Corporation 
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************

*************************************************************************
*
* FILE NAME
*
*      pms_get_system_state.c
*
* COMPONENT
*
*      PMS - System State Services
*
* DESCRIPTION
*
*      This file gets System states for a device.
*
* DATA STRUCTURES
*
*      None
*
* FUNCTIONS
*     
*      NU_PM_Get_System_State
*
* DEPENDENCIES
*
*      power_core.h
*      system_state.h
*
***********************************************************************/
#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "services/power_core.h"
#include    "os/services/power/core/inc/system_state.h"

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_SYSTEM == NU_TRUE)

extern PM_SYSTEM_GLOBAL_STATE  *PM_System_Global_Info;

/*************************************************************************
*
* FUNCTION
*
*      NU_PM_Get_System_State
*
* DESCRIPTION
*
*      This function returns the current state of the system.
*
* INPUTS
*
*      state_ptr                    Pointer to retrieved state id
*
* OUTPUTS
*
*      NU_SUCCESS                   This indicates successful transition
*      PM_SYSTEM_STATE_NEED_INIT    This error code indicates that the
*                                   system state services in not
*                                   initialized
*      PM_INVALID_POINTER           This error indicates the provided
*                                   pointer is invalid
*
*************************************************************************/
STATUS NU_PM_Get_System_State(UINT8 *state_ptr)
{
    STATUS pm_status = NU_SUCCESS; 
    
    /* Check input parameters */
    if (state_ptr == NU_NULL)
    {
        pm_status = PM_INVALID_POINTER;
    }
    
    else if (PM_System_Global_Info == 0)
    {
        pm_status = PM_SYSTEM_STATE_NEED_INIT;
    }
    
    else 
    {
        /* Get the current System State */
        *state_ptr = PM_System_Global_Info->system_current_state;
    }
    
    return (pm_status);
}

#endif  /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_SYSTEM == NU_TRUE) */


