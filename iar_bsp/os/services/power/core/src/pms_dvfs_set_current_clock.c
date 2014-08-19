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
*       pms_dvfs_set_current_clock.c
*
*   COMPONENT
*
*       DVFS
*
*   DESCRIPTION
*
*       Contains all functionality for set current clock API
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_PM_Set_Current_Clock
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

BOOLEAN PMS_DVFS_Get_OP(UINT8 *op_id_ptr, UINT8 freq_id, UINT8 voltage_id);

/*************************************************************************
*
*   FUNCTION
*
*       NU_PM_Set_Current_Clock
*
*   DESCRIPTION
*
*       This function identifies an operating point with at a specified
*       frequency (given by freq_id) and lowest voltage id point. If such
*       OP is successfully identified, the transition to that point is
*       attempted.
*
*   INPUT
*
*       freq_id             this is the desired new frequency id
*
*   OUTPUT
*
*       NU_SUCCESS          Successful transition
*       PM_TRANSITION_FAILED DVFS service was unable to transition due
*                            to current peripheral activity
*       PM_INVALID_FREQ_ID  Provided frequency id does not exist
*       PM_UNEXPECTED_ERROR Unexpected error has occurred
*
*************************************************************************/
STATUS NU_PM_Set_Current_Clock(UINT8 freq_id)
{
    STATUS    pm_status;
    UINT8     op_id;
    UINT8     volt_id;

    /* Verify initialization is complete */
    pm_status = PMS_DVFS_Status_Check();

    if (pm_status == NU_SUCCESS)
    {
        /* Determine if the frequency is valid */
        if (freq_id >= PM_DVFS_Frequency_Count)
        {
            pm_status = PM_INVALID_FREQ_ID;
        }
        else
        {
            /* Read the current voltage id */
            volt_id = PM_DVFS_Frequency_Array[freq_id] -> pm_volt_id;

            /* Find the OP id for the given freq/volt combination */
            if (PMS_DVFS_Get_OP(&op_id, freq_id, volt_id) == NU_TRUE)
            {
                /* All updates should go through the Set OP function */
                pm_status = NU_PM_Set_Current_OP(op_id);
            }
            else
            {
                /* Valid freq id found but corresponding op id
                   not found is unexpected */
                pm_status = PM_UNEXPECTED_ERROR;
            }
        }
    }
    return (pm_status);
}

#endif  /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */


