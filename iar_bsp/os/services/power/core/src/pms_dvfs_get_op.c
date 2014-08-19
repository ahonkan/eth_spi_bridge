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
*       pms_dvfs_get_op.c
*
*   COMPONENT
*
*       DVFS
*
*   DESCRIPTION
*
*       Contains all functionality for get specific operating point API
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       PMS_DVFS_Get_OP
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

extern PM_OP        **PM_DVFS_OP_Array;
extern UINT8          PM_DVFS_OP_Count;

/*************************************************************************
*
*   FUNCTION
*
*       PMS_DVFS_Get_OP
*
*   DESCRIPTION
*
*       This function retrieves OP based on the passed in Voltage and
*       Frequency id’s
*
*   INPUT
*
*       op_id_ptr           Where the OP id will be retrieved
*       freq_id_ptr         Frequency id to search for
*       voltage_id_ptr      Voltage id to search for
*
*   OUTPUT
*
*       NU_TRUE             OP Found
*       NU_FALSE            OP could not be found with volt/freq
*
*************************************************************************/
BOOLEAN PMS_DVFS_Get_OP(UINT8 *op_id_ptr, UINT8 freq_id, UINT8 voltage_id)
{
    BOOLEAN op_found = NU_FALSE;
    UINT8   op_id;

    /* This function should only be called internally but as
       a sanity check verify the op id pointer is valid */
    if (op_id_ptr != NU_NULL)
    {
        /* Find the OP id */
        for (op_id = 0; ((op_id < PM_DVFS_OP_Count) && (op_found == NU_FALSE)); op_id++)
        {
            if ((PM_DVFS_OP_Array[op_id] -> pm_freq_id == freq_id) &&
                (PM_DVFS_OP_Array[op_id] -> pm_volt_id == voltage_id))
            {
                /* OP is found update return value
                   and passed in op pointer */
                op_found = NU_TRUE;
                *op_id_ptr = op_id;
            }
        }
    }

    return (op_found);
}

#endif  /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */


