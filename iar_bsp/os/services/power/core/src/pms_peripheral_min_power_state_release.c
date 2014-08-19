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
*      pms_peripheral_min_power_state_release.c
*
* COMPONENT
*
*      PMS - Peripheral State Services
*
* DESCRIPTION
*
*      Release minimum power state for the device.
*
* DATA STRUCTURES
*
*      None
*
* FUNCTIONS
*
*      NU_PM_Min_Power_State_Release
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
extern STATUS PMS_Set_Power_State(DV_DEV_ID dev_id, PM_STATE_ID state);

/*************************************************************************
*
* FUNCTION
*
*      NU_PM_Min_Power_State_Release
*
* DESCRIPTION
*
*      This function releases a previously placed request for minimum
*      peripheral power state.
*
* INPUTS
*
*      dev_id                   Device id of the peripheral in question
*      handle                   Request handle to be released
*
* OUTPUTS
*
*      NU_SUCCESS               This indicates successful transition
*      PM_INVALID_REQ_HANDLE    This indicates that the requested handle is
*                               not valid
*      PM_UNEXPECTED_ERROR      This indicates an unexpected error has occurred
*                               (should never happen)
*
*************************************************************************/
STATUS NU_PM_Min_Power_State_Release (DV_DEV_ID dev_id, PM_MIN_REQ_HANDLE handle)
{
    STATUS          pm_status = NU_SUCCESS;
    STATUS          status;
    INT             periph_index;
    CS_NODE         *tail_request_ptr;
    PM_REQUEST      *current_request;
    PM_STATE_ID     latest_state;
    PM_REQUEST      *highest_request;
    PM_REQUEST      *head_request;
    
    NU_SUPERV_USER_VARIABLES

    /* Find the index for the device ID */
    pm_status = PMS_Peripheral_Get_Device_Index(dev_id, &periph_index);

    /* Check input parameters */
    /* Ensure list is not empty */
    if (pm_status == NU_SUCCESS)
    {
        if ((handle == 0) || (PM_Dev_Info_Array[periph_index]->request_list_ptr == NU_NULL))
        {
            pm_status = PM_INVALID_REQ_HANDLE;
        }
    }

    if (pm_status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();
        
        /* Get a pointer to the current request handle */
        current_request = (PM_REQUEST *)handle;

        /* Protect against access to the request list.  */
        NU_Protect(&(PM_Dev_Info_Array[periph_index]->request_list_protect));

        /* Remove the structure from the list of requests */
        NU_Remove_From_List(&(PM_Dev_Info_Array[periph_index]->request_list_ptr),
                            &(current_request->state_request_list));

        /* Decrement the number of requests for this device */
        PM_Dev_Info_Array[periph_index]->total_state_requests--;

        /* Release protection against access to the list */
        NU_Unprotect();

        /* Deallocate the memory used in the request structure */
        status = NU_Deallocate_Memory(current_request);

        if (status != NU_SUCCESS)
        {
            pm_status = PM_UNEXPECTED_ERROR;
        }
        else
        {
            /* Get the latest set state command */
            latest_state = (PM_STATE_ID)PM_Dev_Info_Array[periph_index]->latest_set_state;

            /* If there is at least one more request in the list */
            if (PM_Dev_Info_Array[periph_index]->total_state_requests != 0)
            {

                /* Get a pointer to the request at the head of the list */
                head_request = (PM_REQUEST *)(PM_Dev_Info_Array[periph_index]->request_list_ptr);

                /* The tail pointer points to the highest request */
                /* Note: In PLUS 0 is highest priority but in PM, 0 is the lowest state */
                tail_request_ptr = head_request->state_request_list.cs_previous;

                highest_request = (PM_REQUEST *)tail_request_ptr;

                /* Check if the latest set state is higher than latest request */
                if (highest_request->requested_min_state > latest_state)
                {
                    /* Set the state to the requested minimum. Since the list is sorted,
                       the next highest state is the head */
                    pm_status = PMS_Set_Power_State(dev_id, highest_request->requested_min_state);
                }
                /* Check if the highest request is equal to the latest set state AND the 
                   peripheral current state is the same as the latest set state. If equal, there 
                   is nothing to be done here, return NU_SUCCESS */
                else if ((highest_request->requested_min_state == latest_state) && 
                         (PM_Dev_Info_Array[periph_index]->current_state == latest_state))
                {
                    pm_status = NU_SUCCESS;
                }
                else
                {
                    /* Set the state to the latest set state cmd saved */
                    pm_status = PMS_Set_Power_State(dev_id, latest_state);
                }
            }
            else
            {
                /* Set the state to the latest set state cmd saved */
                pm_status = PMS_Set_Power_State(dev_id, latest_state);
            }
        }
            
        /* Return to user mode */
        NU_USER_MODE();
    }

    return (pm_status);
}

#endif  /* ((CFG_NU_OS_SVCS_PWR_CORE_ENABLE_PERIPHERAL == NU_TRUE) || (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_SYSTEM == NU_TRUE)) */
