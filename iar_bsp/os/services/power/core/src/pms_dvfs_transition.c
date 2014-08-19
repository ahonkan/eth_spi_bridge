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
*       pms_dvfs_transition.c
*
*   COMPONENT
*
*       DVFS
*
*   DESCRIPTION
*
*       Contains all functionality for DVFS transition control
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_PM_DVFS_Control_Transition
*
*   DEPENDENCIES
*
*       power_core.h
*
*************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/power_core.h"

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

extern BOOLEAN         PM_DVFS_Transitions;

/*************************************************************************
*
*   FUNCTION
*
*       NU_PM_DVFS_Control_Transition
*
*   DESCRIPTION
*
*       This function is called to control transitions.  NU_TRUE
*       will allow DVFS transitions to occur and NU_FALSE will cancel
*       any in progress and prevent any from occurring in the future.
*
*   INPUT
*
*       new_value           Enable/Disable DVFS transition value
*       previous_value      Pointer to the old value will be written
*
*   OUTPUT
*
*       NU_SUCCESS          Successful call
*       PM_INVALID_POINTER  The provided pointer is invalid
*       PM_INVALID_PARAMETER New value has unknown value
*
*************************************************************************/
STATUS NU_PM_DVFS_Control_Transition(BOOLEAN new_value, BOOLEAN *previous_value)
{
    STATUS pm_status = NU_SUCCESS;
    
    NU_SUPERV_USER_VARIABLES

    /* Verify parameters */
    if (previous_value == NU_NULL)
    {
        pm_status = PM_INVALID_POINTER;
    }
    else if ((new_value != NU_TRUE) && (new_value != NU_FALSE))
    {
        pm_status = PM_INVALID_PARAMETER;
    }
    else
    {
        /* Save the old value */
        *previous_value = PM_DVFS_Transitions;
        
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();
        
        /* Update the transition */
        PM_DVFS_Transitions = new_value;
        
        /* Return to user mode */
        NU_USER_MODE();
    }

    return (pm_status);
}

#endif  /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */


