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
*       pms_dvfs_get_voltage_info.c
*
*   COMPONENT
*
*       DVFS
*
*   DESCRIPTION
*
*       Contains all functionality for voltage information API
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_PM_Get_Voltage_Info
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
extern PM_VOLTAGE   **PM_DVFS_Voltage_Array;

/*************************************************************************
*
*   FUNCTION
*
*       NU_PM_Get_Voltage_Info
*
*   DESCRIPTION
*
*       This function retrieves a voltage value (in mV) associated with
*       the voltage_point_id
*
*   INPUT
*
*       voltage_point_id    this is the specified voltage id
*       voltage_ptr         this points to where the retrieved voltage
*                           value will be placed
*
*   OUTPUT
*
*       NU_SUCCESS          Function returns success
*       PM_INVALID_VOLTAGE_ID Provided frequency id does not exist
*       PM_INVALID_POINTER  Provided pointer is invalid
*
*************************************************************************/
STATUS NU_PM_Get_Voltage_Info(UINT8 voltage_point_id, UINT16 *voltage_ptr)
{
    STATUS status;

    /* Verify initialization is complete */
    status = PMS_DVFS_Status_Check();

    if (status == NU_SUCCESS)
    {
        /* Verify the pointer is valid */
        if (voltage_ptr == NU_NULL)
        {
            status = PM_INVALID_POINTER;
        }
        /* if the ID is equal to the voltage count it means
           information is being requested on the startup 
           voltage.  During DVFS initialization the "count"
           ID is reserved for the startup index. */
        else if (voltage_point_id > PM_DVFS_Voltage_Count)
        {
            status = PM_INVALID_VOLTAGE_ID;
        }
        else
        {
            /* Set the pointer to the voltage */
            *voltage_ptr = PM_DVFS_Voltage_Array[voltage_point_id] -> pm_voltage;
        }
    }

    return (status);
}

#endif  /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_DVFS == NU_TRUE) */


