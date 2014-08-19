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
*       pms_dvfs_get_clock_info.c
*
*   COMPONENT
*
*       DVFS
*
*   DESCRIPTION
*
*       Contains all functionality for clock information API
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_PM_Get_Clock_Info
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

extern UINT8          PM_DVFS_Frequency_Count;
extern PM_FREQUENCY **PM_DVFS_Frequency_Array;

/*************************************************************************
*
*   FUNCTION
*
*       NU_PM_Get_Clock_Info
*
*   DESCRIPTION
*
*       This function retrieves the frequency (in MHz) and the minimum
*       voltage_id associated with the freq_id provided
*
*   INPUT
*
*       freq_id             this is the freq_id point in question
*       freq_ptr            pointer to where the frequency (in MHz) will
*                           be retrieved to
*       voltage_id_ptr      pointer to where the minimum voltage id will
*                           be retrieved to
*
*   OUTPUT
*
*       NU_SUCCESS          Function returns success
*       PM_INVALID_FREQ_ID  Provided frequency id does not exist
*       PM_INVALID_POINTER  One or both of the pointers are invalid
*
*************************************************************************/
STATUS NU_PM_Get_Clock_Info(UINT8 freq_id, UINT32 *freq_ptr, UINT8 *voltage_id_ptr)
{
    STATUS status;

    /* Verify initialization is complete */
    status = PMS_DVFS_Status_Check();

    if (status == NU_SUCCESS)
    {
        /* Verify the pointer is valid */
        if (freq_ptr == NU_NULL)
        {
            status = PM_INVALID_POINTER;
        }
        else if (voltage_id_ptr == NU_NULL)
        {
            status = PM_INVALID_POINTER;
        }
        /* if the ID is equal to the frequency count it means
           information is being requested on the startup 
           frequency.  During DVFS initialization the "count"
           ID is reserved for the startup index. */
        else if (freq_id > PM_DVFS_Frequency_Count)
        {
            status = PM_INVALID_FREQ_ID;
        }
        else
        {
            /* Set the pointer to the frequency */
            *freq_ptr = PM_DVFS_Frequency_Array[freq_id] -> pm_frequency;

            /* Set the pointer to the voltage id */
            *voltage_id_ptr = PM_DVFS_Frequency_Array[freq_id] -> pm_volt_id;
        }
    }

    return (status);
}

#endif  /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */


