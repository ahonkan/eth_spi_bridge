/*************************************************************************
*
*               Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************

*************************************************************************
*
* FILE NAME
*
*      pms_peripheral_get_power_state.c
*
* COMPONENT
*
*      PMS - Peripheral State Services
*
* DESCRIPTION
*
*      This file gets power states for a device.
*
* DATA STRUCTURES
*
*      None
*
* FUNCTIONS
*
*      NU_PM_Get_Power_State
*
* DEPENDENCIES
*
*      power_core.h
*      peripheral.h
*
***********************************************************************/
#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "services/power_core.h"
#include    "os/services/power/core/inc/peripheral.h"

#if ((CFG_NU_OS_SVCS_PWR_CORE_ENABLE_PERIPHERAL == NU_TRUE) || (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_SYSTEM == NU_TRUE))

/* Externs */
extern PM_DEVICE_INFO *PM_Dev_Info_Array[PM_MAX_DEV_CNT];

/*************************************************************************
*
* FUNCTION
*
*      NU_PM_Get_Power_State
*
* COMPONENT
*
*      PMS - Peripheral State Services
*
* DESCRIPTION
*
*      This function returns the current state of the peripheral.
*
* INPUTS
*
*      dev_id               Device id of the peripheral in question
*      state_id_ptr         Pointer to location of retrieved state id
*
* OUTPUTS
*
*      NU_SUCCESS           This indicates successful transition
*      PM_INVALID_POINTER   This error indicates the provided
*                           pointer is invalid
*      PM_UNEXPECTED_ERROR  This indicates an unexpected error
*                           has occurred (should never happen)
*
*************************************************************************/
STATUS NU_PM_Get_Power_State(DV_DEV_ID dev_id, PM_STATE_ID *state_id_ptr)
{
    STATUS          pm_status     = NU_SUCCESS;
    STATUS          status;
    INT             periph_index;
    DV_DEV_HANDLE   local_handle;
    DV_DEV_LABEL    local_label   = {POWER_CLASS_LABEL};
    DV_IOCTL0_STRUCT   local_ioctl0;
    
    NU_SUPERV_USER_VARIABLES
    
    /* Check input parameters */
    if (state_id_ptr == NU_NULL)
    {
        pm_status = PM_INVALID_POINTER;
    }
    else
    {
        /* Find the index for the device ID */
        pm_status = PMS_Peripheral_Get_Device_Index(dev_id, &periph_index);
    }

    if (pm_status == NU_SUCCESS)
    {    
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();
       
        /* Obtain device handle for device ID of peripheral */
        if (PM_Dev_Info_Array[periph_index]->dev_handle == PMS_EMPTY_DEV_HANDLE)
        {
            /* Device class ID is POWER_CLASS */
            status = DVC_Dev_ID_Open(dev_id, &local_label, 1, &local_handle);

            if (status == NU_SUCCESS)
            {
               /* Update the Device Info Structure for this device with
                  the handle */
               PM_Dev_Info_Array[periph_index]->dev_handle = local_handle;
            }
            else
            {
                pm_status = PM_UNEXPECTED_ERROR;
            }
        }

        /* Get the Power base for this device */
        if ((PM_Dev_Info_Array[periph_index]->dev_power_base == 0) && (pm_status == NU_SUCCESS))
        {
            /* Call IOCTL 0 to determine the power aware base for that device
               from which you can determine the get power state ioctl */
            local_ioctl0.label = local_label;

            status = DVC_Dev_Ioctl(PM_Dev_Info_Array[periph_index]->dev_handle, DV_IOCTL0,
                                  &local_ioctl0, sizeof(local_ioctl0));

            if (status == NU_SUCCESS)
            {
                /* Save the IOCTL base of this device in the info structure block */
                PM_Dev_Info_Array[periph_index]->dev_power_base = local_ioctl0.base;
            }
            else
            {
                pm_status = PM_UNEXPECTED_ERROR;
            }
        }

        if (pm_status == NU_SUCCESS)
        {
            
            /* Use IOCTL command to obtain power state */
            status = DVC_Dev_Ioctl(PM_Dev_Info_Array[periph_index]->dev_handle,
                                 (PM_Dev_Info_Array[periph_index]->dev_power_base + POWER_IOCTL_GET_STATE),
                                  state_id_ptr, sizeof(state_id_ptr));

            if (status == NU_SUCCESS)
            {
                /* Save the current state for this device */
                PM_Dev_Info_Array[periph_index]->current_state = (PM_STATE_ID)*state_id_ptr;

            }
            else
            {
                /* DM_Dev_Ioctl returned an invalid value */
                pm_status = PM_UNEXPECTED_ERROR;
            }
        }
        
        /* Return to user mode */
        NU_USER_MODE();
    }
        
    return (pm_status);
}

#endif  /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_PERIPHERAL == NU_TRUE) */


