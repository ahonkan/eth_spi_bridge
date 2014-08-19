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
*      pms_peripheral_set_power_state.c
*
* COMPONENT
*
*      PMS - Peripheral State Services
*
* DESCRIPTION
*
*      This file sets power states for a device.
*
* DATA STRUCTURES
*
*      None
*
* FUNCTIONS
*
*      NU_PM_Set_Power_State
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
#include    "services/nu_trace_os_mark.h"

#if ((CFG_NU_OS_SVCS_PWR_CORE_ENABLE_PERIPHERAL == NU_TRUE) || (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_SYSTEM == NU_TRUE))

/* Local function prototypes */
STATUS PMS_Set_Power_State(DV_DEV_ID dev_id, PM_STATE_ID state);

/* Externs */
extern PM_DEVICE_INFO *PM_Dev_Info_Array[PM_MAX_DEV_CNT];

/*************************************************************************
*
* FUNCTION
*
*      NU_PM_Set_Power_State
*
* DESCRIPTION
*
*      This function invokes the transition of the specified device to
*      the specified power state.
*
* INPUTS
*
*      dev_id           Device id of the peripheral in question
*      state            Desired new state
*
* OUTPUTS
*
*      NU_SUCCESS               This indicates successful transition
*      PM_DEFERRED              This indicates that the peripheral noted the
*                               requested state change, however at this time
*                               there exists a Minimum State Request which is
*                               holding the state at a higher value (PMS-only
*                               deployments)
*      PM_INVALID_POWER_STATE   This indicates the power state requested is
*                               not valid for the device
*
*************************************************************************/
STATUS NU_PM_Set_Power_State(DV_DEV_ID dev_id, PM_STATE_ID state)
{
    STATUS          pm_status = NU_SUCCESS;
    INT             periph_index;
    PM_STATE_ID     state_ptr;
    PM_STATE_ID     state_count;
    
    NU_SUPERV_USER_VARIABLES

    /* Find the index for the device ID */
    pm_status = PMS_Peripheral_Get_Device_Index(dev_id, &periph_index);

    if (pm_status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();
        
        /* Trace log */
        T_DEV_TRANS_START(dev_id);
    
        /* Check if max_state_count is cached */
        if (PM_Dev_Info_Array[periph_index]->max_state_count == 0)
        {
            /* Get the state count for this device */
            pm_status = NU_PM_Get_Power_State_Count(dev_id, &state_count);

            if (pm_status == NU_SUCCESS)
            {
                /* Save the state count in the device info block */
                PM_Dev_Info_Array[periph_index]->max_state_count = state_count;
            }
        }

        if (pm_status == NU_SUCCESS)
        {
            /* If the desired state is 255, power state is 'ON' */
            if (state == POWER_ON_STATE)
            {
                /* Set this to the device's max power state. Power state
                   always starts at 0, which indicates 'OFF' */
                state = (PM_Dev_Info_Array[periph_index]->max_state_count) - 1;
            }

            /* Verify that the desired new state is valid for the device */
            if (state >= (PM_STATE_ID)PM_Dev_Info_Array[periph_index]->max_state_count)
            {
                pm_status = PM_INVALID_POWER_STATE;
            }
            else
            {                
                /* Save the latest state desired for this device in the info block */
                PM_Dev_Info_Array[periph_index]->latest_set_state = state;

                /* Get the current State */
                pm_status = NU_PM_Get_Power_State(dev_id, &state_ptr);

                if (pm_status == NU_SUCCESS)
                {
                    /* If there is no pending request, it means we can change the
                       device state immediately. */
                    if (PM_Dev_Info_Array[periph_index]->request_list_ptr == NU_NULL)
                    {
                        /* Check if desired device state is greater or same as
                            current state */
                        if (state != state_ptr)
                        {
                            /* Change the state */
                            pm_status = PMS_Set_Power_State(dev_id, state);
                        }
                    }
                    /* There is request for minimum state for the device */
                    else
                    {
                        /* Check if desired device state is greater or same as
                            current state */
                        if (state > state_ptr)
                        {
                            /* Change the state */
                            pm_status = PMS_Set_Power_State(dev_id, state);
                        }
                        else if (state == state_ptr)
                        {
                            pm_status = NU_SUCCESS;
                        }
                        else
                        {
                            /* Desired state is lower than current min requested */
                            /* State cmd has been recorded but state will not change now */
                            pm_status = PM_DEFERRED;
                        }
                    }
                }
            }
        }
        
        /* Trace log */
        T_DEV_TRANS_STOP(dev_id);
        
        /* Return to user mode */
        NU_USER_MODE();
    }
    
    if (pm_status == NU_SUCCESS)
    {
        /* Trace log */
        T_DEV_TRANS(dev_id, state, pm_status);
    }
    else
    {
        /* Trace log */
        T_DEV_TRANS(dev_id, PM_Dev_Info_Array[periph_index]->current_state, pm_status);
    }
    
    return (pm_status);
}


/*************************************************************************
*
* FUNCTION
*
*      PMS_Set_Power_State
*
* DESCRIPTION
*
*      This internal function invokes the transition of the specified device to
*      the specified power state.
*
* INPUTS
*
*      dev_id           Device id of the peripheral in question
*      state            Desired new state
*
* OUTPUTS
*
*      NU_SUCCESS               This indicates successful transition
*      PM_INVALID_POWER_STATE   This indicates the power state requested is
*                               not valid for the device
*      PM_UNEXPECTED_ERROR      This indicates an unexpected error has occurred
*                               (should never happen)
*
*************************************************************************/
STATUS PMS_Set_Power_State(DV_DEV_ID dev_id, PM_STATE_ID state)
{
    STATUS          status;
    INT             periph_index;
    STATUS          pm_status = NU_SUCCESS;
    DV_DEV_HANDLE   local_handle;
    DV_DEV_LABEL    local_label = {POWER_CLASS_LABEL};
    DV_IOCTL0_STRUCT  local_ioctl0;

    /* Find the index for the device ID */
    pm_status = PMS_Peripheral_Get_Device_Index(dev_id, &periph_index);

    if (pm_status == NU_SUCCESS)
    {
        /* If the desired state is 255, power state is 'ON' */
        if (state == POWER_ON_STATE)
        {
            /* Set this to the device's max power state. Power state
               always starts at 0, which indicates 'OFF' */
            state = (PM_Dev_Info_Array[periph_index]->max_state_count - 1);
        }

        /* Verify that the desired new state is valid for the device */
        if (state > (PM_STATE_ID)PM_Dev_Info_Array[periph_index]->max_state_count)
        {
            pm_status = PM_INVALID_POWER_STATE;
        }
        else
        {
            /* Obtain device handle for device ID of peripheral */
            if (PM_Dev_Info_Array[periph_index]->dev_handle == PMS_EMPTY_DEV_HANDLE)
            {
                /* Device class ID is POWER_CLASS_LABEL */
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
                /* Use IOCTL command to set power state */
                status = DVC_Dev_Ioctl(PM_Dev_Info_Array[periph_index]->dev_handle,
                                     (PM_Dev_Info_Array[periph_index]->dev_power_base + POWER_IOCTL_SET_STATE),
                                      &state, sizeof(state));

                if (status != NU_SUCCESS)
                {
                   pm_status = PM_UNEXPECTED_ERROR;
                }
            }
        }
    }

    return(pm_status);
}

#endif  /* ((CFG_NU_OS_SVCS_PWR_CORE_ENABLE_PERIPHERAL == NU_TRUE) || (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_SYSTEM == NU_TRUE)) */


