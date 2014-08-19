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
*       pms_dvfs_get_current_op.c
*
*   COMPONENT
*
*       DVFS
*
*   DESCRIPTION
*
*       Contains all functionality for get current operating point API
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_PM_Get_Current_OP
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

/*************************************************************************
*
*   FUNCTION
*
*       NU_PM_Get_Current_OP
*
*   DESCRIPTION
*
*       This function retrieves the current Operating Point ID
*
*   INPUT
*
*       op_id_ptr           this points to where the OP id will be
*                           retrieved to
*
*   OUTPUT
*
*       NU_SUCCESS          Function returns success
*       PM_INVALID_POINTER  Provided pointer is invalid
*
*************************************************************************/
STATUS NU_PM_Get_Current_OP (UINT8 *op_id_ptr)
{
    STATUS status;

    /* Verify initialization is complete */
    status = PMS_DVFS_Status_Check();

    if (status == NU_SUCCESS)
    {
        /* Verify the pointer is valid */
        if (op_id_ptr == NU_NULL)
        {
            status = PM_INVALID_POINTER;
        }
        else
        {
            /* Set the pointer to the current OP */
            *op_id_ptr = PM_DVFS_Current_OP;
        }
    }

    return (status);
}

#endif   /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */


