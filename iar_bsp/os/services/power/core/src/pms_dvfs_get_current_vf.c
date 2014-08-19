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
*       pms_dvfs_get_current_vf.c
*
*   COMPONENT
*
*       DVFS
*
*   DESCRIPTION
*
*       Contains all functionality for get current voltage frequency
*       information API
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_PM_Get_Current_VF
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

extern UINT8          PM_DVFS_Current_OP;
extern PM_OP        **PM_DVFS_OP_Array;
extern UINT8          PM_DVFS_OP_Count;

/*************************************************************************
*
*   FUNCTION
*
*       NU_PM_Get_Current_VF
*
*   DESCRIPTION
*
*       This function returns the voltage and frequency IDs of the
*       current operating point
*
*   INPUT
*
*       freq_id_ptr         Where the frequency id will be retrieved to
*       voltage_id_ptr      Where the voltage id will be retrieved to
*
*   OUTPUT
*
*       NU_SUCCESS          Function returns success
*       PM_INVALID_POINTER  One or both provided pointers are invalid
*
*************************************************************************/
STATUS NU_PM_Get_Current_VF(UINT8 *freq_id_ptr, UINT8 *voltage_id_ptr)
{
    STATUS status;
    UINT8     op;

    /* Verify initialization is complete */
    status = PMS_DVFS_Status_Check();

    if (status == NU_SUCCESS)
    {
        /* Verify the pointer is valid */
        if (freq_id_ptr == NU_NULL)
        {
            status = PM_INVALID_POINTER;
        }
        else if (voltage_id_ptr == NU_NULL)
        {
            status = PM_INVALID_POINTER;
        }
        else
        {
            /* Translate the index */
            op = (PM_DVFS_Current_OP == PM_STARTUP_OP) ? PM_DVFS_OP_Count : PM_DVFS_Current_OP;
            
            /* Set the pointer to the current freq id */
            *freq_id_ptr = PM_DVFS_OP_Array[op] -> pm_freq_id;

            /* Set the pointer to the current volt id */
            *voltage_id_ptr = PM_DVFS_OP_Array[op] -> pm_volt_id;
        }
    }

    return (status);
}

#endif  /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */


