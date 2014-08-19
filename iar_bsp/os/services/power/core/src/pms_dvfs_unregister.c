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
*       pms_dvfs_unregister.c
*
*   COMPONENT
*
*       DVFS
*
*   DESCRIPTION
*
*       Contains all functionality for unregistering from the DVFS
*       notification system
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_PM_DVFS_Unregister
*
*   DEPENDENCIES
*
*       power_core.h
*       dvfs.h
*
*************************************************************************/
#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "services/power_core.h"
#include    "os/services/power/core/inc/dvfs.h"

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

extern CS_NODE    *PM_DVFS_Device_List;
extern NU_PROTECT  PM_DVFS_List_Protect;
extern UINT32      PM_DVFS_Device_Count;

VOID PMS_DVFS_Calculate_Transition(VOID);

/*************************************************************************
*
*   FUNCTION
*
*       NU_PM_DVFS_Unregister
*
*   DESCRIPTION
*
*       This function is called by the driver to un-register for DVFS
*       notification/synchronization
*
*   INPUT
*
*       dvfs_handle         Handle of the device un-registering
*
*   OUTPUT
*
*       NU_SUCCESS          Successful completion
*       PM_INVALID_REG_HANDLE Provided handle is not found
*       PM_NOT_REGISTERED   Driver is not registered
*       PM_UNEXPECTED_ERROR Unexpected error has occurred
*
*************************************************************************/
STATUS NU_PM_DVFS_Unregister(PM_DVFS_HANDLE dvfs_handle)
{
    STATUS      pm_status = NU_SUCCESS;
    PM_DVFS_REG *reg_entry;
    
    NU_SUPERV_USER_VARIABLES

    /* Cast to local pointer */
    reg_entry = (PM_DVFS_REG *)dvfs_handle;
    if (reg_entry == NU_NULL)
    {
        pm_status = PM_INVALID_REG_HANDLE;
    }
    else if (reg_entry -> pm_id != PM_DVFS_REG_ID)
    {
        pm_status = PM_NOT_REGISTERED;
    }
    else
    {                   
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();
        
        /* Protect the list while removing an item */
        NU_Protect(&PM_DVFS_List_Protect);

        /* Remove the device from the registry list */
        NU_Remove_From_List(&PM_DVFS_Device_List, &(reg_entry -> pm_node));

        /* Remove the valid id marker */
        reg_entry -> pm_id = 0;

        /* Decrement the number of devices */
        PM_DVFS_Device_Count--;                      

        /* Calculate if a transition is ever possible and the
           smallest amount of time that must be considered for
           transition time */
        PMS_DVFS_Calculate_Transition();

        /* Remove list protection */
        NU_Unprotect();

        /* Deallocate the memory */
        if (NU_Deallocate_Memory(reg_entry) != NU_SUCCESS)
        {
            /* Failure in deallocation is unexpected */
            pm_status = PM_UNEXPECTED_ERROR;
        }
        
        /* Return to user mode */
        NU_USER_MODE();
    }

    return (pm_status);
}

#endif  /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */


