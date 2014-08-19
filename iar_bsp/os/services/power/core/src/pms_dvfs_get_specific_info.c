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
*        pms_dvfs_get_specific_info.c
*
*   COMPONENT
*
*        DVFS
*
*   DESCRIPTION
*
*        Contains all functionality for Additional Information API.
*
*   DATA STRUCTURES
*
*        None
*
*   FUNCTIONS
*
*        NU_PM_Get_OP_Specific_Info
*
*   DEPENDENCIES
*
*        power_core.h
*        dvfs.h
*        cpu_dvfs.h
*
*************************************************************************/
#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "services/power_core.h"
#include    "os/services/power/core/inc/dvfs.h"
#include    "services/cpu_dvfs.h"

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE)

extern UINT8           PM_DVFS_OP_Count;
extern PM_OP         **PM_DVFS_OP_Array;
extern DV_DEV_HANDLE   PM_DVFS_CPU_Handle;
extern UINT32          PM_DVFS_CPU_Base;

/*************************************************************************
*
*   FUNCTION
*
*       NU_PM_Get_OP_Specific_Info
*
*   DESCRIPTION
*
*       This function retrieves any additional information available on
*       the specified OP.
*
*   INPUT
*
*       op_id               ID of the operating point of interest
*       identifier          String that identifies what is being requested
*       info_ptr            this points to where the retrieved information
*                           will be placed
*       size                Size of the information pointer
*
*   OUTPUT
*
*       NU_SUCCESS          Function returns success
*       PM_NOT_INITIALIZED  PMS is not initialized
*       PM_INVALID_OP_ID    Provided OP is not valid
*       PM_INVALID_POINTER  Provided pointer is invalid
*       PM_INVALID_SIZE     Size of information pointer cannot be zero
*       PM_DVFS_INFO_FAILURE CPU IOCTL call failed
*
*************************************************************************/
STATUS NU_PM_Get_OP_Specific_Info(UINT8 op_id, CHAR *identifier,
                                     VOID *info_ptr, UINT32 size)
{
    STATUS              pm_status;
    STATUS              status;
    CPU_DVFS_SPECIFIC   info;
    UINT8               op;
    
    NU_SUPERV_USER_VARIABLES

    /* Verify initialization is complete */
    pm_status = PMS_DVFS_Status_Check();

    if (pm_status == NU_SUCCESS)
    {
        /* Verify the pointer is valid */
        if (info_ptr == NU_NULL)
        {
            pm_status = PM_INVALID_POINTER;
        }
        else if ((op_id >= PM_DVFS_OP_Count) && (op_id != PM_STARTUP_OP))
        {
            pm_status = PM_INVALID_OP_ID;
        }
        else if (size == 0)
        {
            pm_status = PM_INVALID_SIZE;
        }
        else
        {
            /* Switch to supervisor mode */
            NU_SUPERVISOR_MODE();
            
            /* Translate the index */
            op = (op_id == PM_STARTUP_OP) ? PM_DVFS_OP_Count : op_id;

            /* Get the OP index used by the driver */
            info.pm_op_id = PM_DVFS_OP_Array[op] -> pm_op_id;

            /* Set the pointer to where the info will be stored, the size
               of the info, and the pointer to the string used for identification
               of the information requested */
            info.pm_info = info_ptr;
            info.pm_size = size;
            info.pm_identifier = identifier;

            /* Call the driver */
            status = DVC_Dev_Ioctl(PM_DVFS_CPU_Handle, (PM_DVFS_CPU_Base + CPU_DVFS_IOCTL_SPECIFIC), 
                                   &info, sizeof(info));
            if (status != NU_SUCCESS)
            {
                /* if a failure occurs return generic failure */
                pm_status = PM_DVFS_INFO_FAILURE;
            }
            
            /* Return to user mode */
            NU_USER_MODE();
        }
    }

    return (pm_status);
}

#endif  /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */


