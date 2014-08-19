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
*       pms_dvfs_set_current_voltage.c
*
*   COMPONENT
*
*       DVFS
*
*   DESCRIPTION
*
*       Contains all functionality for setting the current voltage
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_PM_Set_Current_Voltage
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

extern UINT8          PM_DVFS_Voltage_Count;
extern PM_OP        **PM_DVFS_OP_Array;
extern UINT8          PM_DVFS_Current_OP;

BOOLEAN PMS_DVFS_Get_OP(UINT8 *op_id_ptr, UINT8 freq_id, UINT8 voltage_id);

/*************************************************************************
*
*   FUNCTION
*
*       NU_PM_Set_Current_Voltage
*
*   DESCRIPTION
*
*       This function identifies an operating point with at a specified
*       voltage (given by voltage_point_id) and current frequency id
*       point. If such OP is successfully identified, the transition to
*       that point is attempted.
*
*   INPUT
*
*       voltage_point_id    this is the desired new voltage
*
*   OUTPUT
*
*       NU_SUCCESS          Successful transition
*       PM_TRANSITION_FAILED DVFS service was unable to transition due
*                            to current peripheral activity
*       PM_INVALID_VOLTAGE_ID Provided voltage ID is not valid
*       PM_INVALID_OP_ID    Provided voltage with current frequency
*                           combination is not a valid OP
*
*************************************************************************/
STATUS NU_PM_Set_Current_Voltage(UINT8 voltage_point_id)
{
    STATUS    pm_status;
    UINT8     op_id;
    UINT8     freq_id;

    /* Verify initialization is complete */
    pm_status = PMS_DVFS_Status_Check();

    if (pm_status == NU_SUCCESS)
    {
        /* Determine if the voltage is valid */
        if (voltage_point_id >= PM_DVFS_Voltage_Count)
        {
            pm_status = PM_INVALID_VOLTAGE_ID;
        }
        else
        {
            /* Read the frequency id */
            freq_id = PM_DVFS_OP_Array[PM_DVFS_Current_OP] -> pm_freq_id;

            /* Find the OP for given freq/voltage combination */
            if (PMS_DVFS_Get_OP(&op_id, freq_id, voltage_point_id) == NU_TRUE)
            {
                /* All updates should go through the Set OP function */
                pm_status = NU_PM_Set_Current_OP(op_id);
            }
            else
            {
                /* Valid freq id found but corresponding op id
                   not found */
                pm_status = PM_INVALID_OP_ID;
            }
        }
    }

    return (pm_status);
}

#endif   /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */


