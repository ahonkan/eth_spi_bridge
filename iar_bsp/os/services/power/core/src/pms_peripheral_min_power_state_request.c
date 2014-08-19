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
*      pms_peripheral_min_power_state_request.c
*
* COMPONENT
*
*      PMS - Peripheral State Services
*
* DESCRIPTION
*
*      Request minimum power state for the device.
*
* DATA STRUCTURES
*
*      None
*
* FUNCTIONS
*
*      NU_PM_Min_Power_State_Request
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

/* Memory Pool */
extern NU_MEMORY_POOL  System_Memory;
/* Other externs */
extern PM_DEVICE_INFO *PM_Dev_Info_Array[PM_MAX_DEV_CNT];
extern STATUS PMS_Set_Power_State(DV_DEV_ID dev_id, PM_STATE_ID state);

/*************************************************************************
*
* FUNCTION
*
*      NU_PM_Min_Power_State_Request
*
* DESCRIPTION
*
*      This function requests that a device not transition below a specified
*      power state. That minimum will be held until the caller releases the
*      request using the NU_PM_Min_Power_State_Release handle retrieved from
*      this call.
*
* INPUTS
*
*      dev_id                   Device id of the peripheral in question
*      state                    Minimum desired state
*      handle_ptr               Pointer to location of request handle
*
* OUTPUTS
*
*      NU_SUCCESS               This indicates successful transition
*      PM_INVALID_POINTER       This error indicates the provided pointer is
*                               invalid
*      PM_INVALID_POWER_STATE   This indicates the power state requested is
*                               not valid for the device
*      PM_UNEXPECTED_ERROR      This indicates an unexpected error has occurred
*                               (should never happen)
*
*************************************************************************/
STATUS NU_PM_Min_Power_State_Request (DV_DEV_ID dev_id, PM_STATE_ID state,
                                         PM_MIN_REQ_HANDLE *handle_ptr)
{
    STATUS          pm_status   = NU_SUCCESS;
    STATUS          status;
    INT             periph_index;
    PM_REQUEST      *request_info;
    PM_STATE_ID     state_ptr;
    PM_STATE_ID     state_count;
    
    NU_SUPERV_USER_VARIABLES

    /* Check input parameters */
    if (handle_ptr == NU_NULL)
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
        
        /* Check if the state count values is cached */
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
            if (state > (PM_STATE_ID)PM_Dev_Info_Array[periph_index]->max_state_count)
            {
                pm_status = PM_INVALID_POWER_STATE;
            }

            else
            {

                /* If this is the first request received for the device */
                if (PM_Dev_Info_Array[periph_index]->request_list_ptr == NU_NULL)
                {
                    /* Get the current state of the device */
                    pm_status = NU_PM_Get_Power_State(dev_id, &state_ptr);

                    if (pm_status == NU_SUCCESS)
                    {
                        /* Save the current state */
                        PM_Dev_Info_Array[periph_index]->current_state = (PM_STATE_ID)state_ptr;

                        /* Set the latest_set_state to current state of device */
                        PM_Dev_Info_Array[periph_index]->latest_set_state = PM_Dev_Info_Array[periph_index]->current_state;
                    }
                }

                if (pm_status == NU_SUCCESS)
                {
                    /* Allocate memory for the new request structure that came in */
                    status = NU_Allocate_Memory(&System_Memory, (VOID*)&request_info,
                                                sizeof(PM_REQUEST), NU_NO_SUSPEND);

                    if (status == NU_SUCCESS)
                    {
                        /* Clear the memory we just allocated */
                        (VOID)memset((VOID*)request_info, 0, sizeof(PM_REQUEST));

                        /* Setup device request structure for the new request */
                        request_info->requested_min_state = state;

                        /* There may be a pending request for minimum power state,
                           so just place this new request in the list */
                        /* Protect against access to the request list.  */
                        NU_Protect(&(PM_Dev_Info_Array[periph_index]->request_list_protect));

                        /* Priority of this request is the same as the requested state */
                        request_info->state_request_list.cs_priority = state;

                        /* Link the structure into the list of requests */
                        NU_Priority_Place_On_List(&(PM_Dev_Info_Array[periph_index]->request_list_ptr),
                                                  &(request_info->state_request_list));

                        /* Update total number of requests pending */
                        PM_Dev_Info_Array[periph_index]->total_state_requests++;

                        /* Release protection against access to the list of waiting tasks.  */
                        NU_Unprotect();                     

                        /* Create a request handle to give back to caller */
                        *handle_ptr = (PM_MIN_REQ_HANDLE *)request_info;

                        /* Check if state of device is greater or same as minimum requested */
                        pm_status = NU_PM_Get_Power_State(dev_id, &state_ptr);

                        if (pm_status == NU_SUCCESS)
                        {
                            /* If requested state is higher than current state, change state */
                            /* Higher state has higher numerical value */
                            if (state > (PM_STATE_ID)state_ptr)
                            {
                                /* Change the state*/
                                pm_status = PMS_Set_Power_State(dev_id, state);
                            }
                        }
                    }
                    else
                    {
                        /* Memory Allocation failed */
                        pm_status = PM_UNEXPECTED_ERROR;
                    }
                }
            }
        }
        
        /* Return to user mode */
        NU_USER_MODE();
    }
       
    return (pm_status);
}

#endif  /* ((CFG_NU_OS_SVCS_PWR_CORE_ENABLE_PERIPHERAL == NU_TRUE) || (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_SYSTEM == NU_TRUE)) */
