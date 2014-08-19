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
*       pms_dvfs_get_op_vf.c
*
*   COMPONENT
*
*       DVFS
*
*   DESCRIPTION
*
*       Contains all functionality for get operating point information API
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_PM_Get_OP_VF
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

extern UINT8          PM_DVFS_OP_Count;
extern PM_OP        **PM_DVFS_OP_Array;

/*************************************************************************
*
*   FUNCTION
*
*       NU_PM_Get_OP_VF
*
*   DESCRIPTION
*
*       This function retrieves the Voltage and Frequency id’s associated
*       with a particular operating point (not current op)
*
*   INPUT
*
*       op_id               ID of the operating point of interest
*       freq_id_ptr         Where the frequency id will be retrieved
*       voltage_id_ptr      Where the voltage id info will be retrieved
*
*   OUTPUT
*
*       NU_SUCCESS          Function returns success
*       PM_INVALID_OP_ID    Provided OP is not valid
*       PM_INVALID_POINTER  One or both provided pointers are invalid
*
*************************************************************************/
STATUS NU_PM_Get_OP_VF(UINT8 op_id, UINT8 *freq_id_ptr, UINT8 *voltage_id_ptr)
{
    STATUS status;

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
        /* Verify the OP is valid, it must be without the count from
           initialization or be equal to the special startup ID */
        else if ((op_id >= PM_DVFS_OP_Count) && (op_id != PM_STARTUP_OP))
        {
            status = PM_INVALID_OP_ID;
        }
        else
        {
            /* Set the pointer to the specific OP freq id */
            *freq_id_ptr = PM_DVFS_OP_Array[op_id] -> pm_freq_id;

            /* Set the pointer to the specific OP volt id */
            *voltage_id_ptr = PM_DVFS_OP_Array[op_id] -> pm_volt_id;
        }
    }

    return (status);
}

#endif  /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */


