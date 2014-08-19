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
*   FILE NAME
*
*       pms_get_system_state_count.c
*
*   COMPONENT
*
*       System State
*
*   DESCRIPTION
*
*       Contains all functionality for get system state count API
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_PM_Get_System_State_Count
*
*   DEPENDENCIES
*
*       power_core.h
*       system_state.h
*
*************************************************************************/
#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "services/power_core.h"
#include    "os/services/power/core/inc/system_state.h"

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_SYSTEM == NU_TRUE)

extern PM_SYSTEM_GLOBAL_STATE  *PM_System_Global_Info;

/*************************************************************************
*
*   FUNCTION
*
*       NU_PM_Get_System_State_Count
*
*   DESCRIPTION
*
*       This function retrieves the total number of system states
*       available (max state_id+1)
*
*   INPUT
*
*       state_count_ptr     Where the count is going to be retrieved to
*
*   OUTPUT
*
*       NU_SUCCESS          Function returns success
*       PM_INVALID_POINTER  Provided pointer is invalid
*       PM_SYSTEM_STATE_NEED_INIT System state service is not initialized
*
*************************************************************************/
STATUS NU_PM_Get_System_State_Count(UINT8 *state_count_ptr)
{
    STATUS status = NU_SUCCESS;

    /* Verify the pointer is valid */
    if (state_count_ptr == NU_NULL)
    {
        status = PM_INVALID_POINTER;
    }
    else if (PM_System_Global_Info == NU_NULL)
    {
        /* System state has not been initialized so it isn't save to 
           use the global information.  In this case there isn't
           any system states, return 0 */
        status = PM_SYSTEM_STATE_NEED_INIT;
    }
    else
    {
        /* Set the pointer to the system state count */
        *state_count_ptr = PM_System_Global_Info -> system_state_count;
    }

    return (status);
}

#endif  /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_SYSTEM == NU_TRUE) */


