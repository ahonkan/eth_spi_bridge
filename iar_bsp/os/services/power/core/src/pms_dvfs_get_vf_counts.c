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
*       pms_dvfs_get_vf_counts.c
*
*   COMPONENT
*
*       DVFS
*
*   DESCRIPTION
*
*       Contains all functionality for get voltage/frequency count API
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_PM_Get_VF_Counts
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

extern UINT8 PM_DVFS_Frequency_Count;
extern UINT8 PM_DVFS_Voltage_Count;

/*************************************************************************
*
*   FUNCTION
*
*       NU_PM_Get_VF_Counts
*
*   DESCRIPTION
*
*       This function returns the total numbers of frequency IDs and
*       voltage IDs. (max_id+1)
*
*   INPUT
*
*       freq_count_ptr      Where the number of frequency id�s will be
*                           retrieved into
*       voltage_count_ptr   Where the number of voltage id�s will be
*                           retrieved into
*
*   OUTPUT
*
*       NU_SUCCESS          Function returns success
*       PM_INVALID_POINTER  One or both provided pointers are invalid
*
*************************************************************************/
STATUS NU_PM_Get_VF_Counts(UINT8 *freq_count_ptr, UINT8 *voltage_count_ptr)
{
    STATUS status;

    /* Verify initialization is complete */
    status = PMS_DVFS_Status_Check();

    if (status == NU_SUCCESS)
    {
        /* Verify the pointer is valid */
        if (freq_count_ptr == NU_NULL)
        {
            status = PM_INVALID_POINTER;
        }
        else if (voltage_count_ptr == NU_NULL)
        {
            status = PM_INVALID_POINTER;
        }
        else
        {
            /* Set the pointer to the frequency count */
            *freq_count_ptr = PM_DVFS_Frequency_Count;

            /* Set the pointer to the voltage count */
            *voltage_count_ptr = PM_DVFS_Voltage_Count;
        }
    }

    return (status);
}

#endif  /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */


