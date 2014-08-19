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
*       pms_emergency_state_request.c
*
* COMPONENT
*
*       PMS - System State Services
*
* DESCRIPTION
*
*       Request emergency system state for the system.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*     
*       NU_PM_Emergency_State_Request
*
* DEPENDENCIES
*
*       power_core.h
*       system_state.h
*
***********************************************************************/
#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "services/power_core.h"
#include    "os/services/power/core/inc/system_state.h"

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_SYSTEM == NU_TRUE)

/* Memory Pool */
extern NU_MEMORY_POOL  System_Memory;

/* Other externs */
extern PM_SYSTEM_GLOBAL_STATE   *PM_System_Global_Info;
extern PM_SYSTEM_STATE          *PM_State_Map_Array[PM_MAX_SYSTEM_STATES];
extern STATUS                   PMS_Set_System_State(UINT8 state_id);
                                      
/*************************************************************************
*
* FUNCTION
*
*      NU_PM_Emergency_State_Request
*
* DESCRIPTION
*
*     This function places a request for an emergency system state.
*     The system state will not be allowed to be rise above this
*     requested state until the request is released.
*
* INPUTS
*
*      state                    Desired state
*      handle_ptr               Pointer to location of request handle
*
* OUTPUTS
*
*      NU_SUCCESS               This indicates successful transition
*      PM_INVALID_POINTER       This error indicates the provided pointer is
*                               invalid
*      PM_INVALID_SYSTEM_STATE  This indicates the system state requested is
*                               not valid for the device
*      PM_UNEXPECTED_ERROR      This indicates an unexpected error has occurred
*                               (should never happen)
*
*************************************************************************/
STATUS NU_PM_Emergency_State_Request(UINT8 state, PM_MIN_REQ_HANDLE *handle_ptr)
{
    STATUS             pm_status = NU_SUCCESS;
    STATUS                status; 
    PM_EMERGENCY_REQUEST *request_info;  
    
    NU_SUPERV_USER_VARIABLES

    /* Check input parameters */
    if ((state >= PM_MAX_SYSTEM_STATES) || (PM_State_Map_Array[state] == NU_NULL))
    {
        pm_status = PM_INVALID_SYSTEM_STATE;
    }
    else if (handle_ptr == NU_NULL)
    {
        pm_status = PM_INVALID_POINTER;
    }
    else
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();
        
        /* Allocate memory for the new request structure that came in */
        status = NU_Allocate_Memory(&System_Memory, (VOID*)&request_info,
                                    sizeof(PM_EMERGENCY_REQUEST), NU_NO_SUSPEND);                                      
        if (status == NU_SUCCESS)
        {
            /* Clear the memory we just allocated */
            (VOID)memset((VOID*)request_info, 0, sizeof(PM_EMERGENCY_REQUEST));
            
            /* Setup device request structure for the new request */
            request_info -> requested_max_state = state;
                            
            /* There may be a pending request for minimum power state,
               so just place this new request in the list */
            /* Protect against access to the request list.  */
            NU_Protect(&(PM_System_Global_Info -> request_list_protect));
            
            /* Priority of this request is reversed as we want to stay at the lowest
               emergency state */
            request_info -> emergency_request_list.cs_priority = PM_MAX_SYSTEM_STATES - state;
            
            /* If this is the first request received for the device */
            if (PM_System_Global_Info -> request_list_ptr == NU_NULL)
            {                                    
                /* Set the latest_set_state to current state of device */
                PM_System_Global_Info -> latest_system_set_state = PM_System_Global_Info -> system_current_state;
            }
        
            /* Link the structure into the list of requests */                     
            NU_Priority_Place_On_List(&(PM_System_Global_Info -> request_list_ptr), 
                                      &(request_info -> emergency_request_list));            
                            
            /* Update total number of requests pending */
            PM_System_Global_Info -> total_state_requests++;
            PM_System_Global_Info -> total_emergency_requests++;
                
            /* Release protection against access to the list of waiting tasks.  */
            NU_Unprotect();
                  
            /* Create a request handle to give back to caller */
            *handle_ptr = (PM_MIN_REQ_HANDLE *)request_info;   
            
            /* If requested state is higher than current state, change state */
            /* Higher state has higher numerical value */
            if(state < PM_System_Global_Info -> system_current_state)
            {
                /* Change the state*/
                pm_status = PMS_Set_System_State(state);
            }
            else
            {
                /* Already at the minimum state */
                pm_status = NU_SUCCESS;
            }            
             
        }
        else
        {
            pm_status = PM_UNEXPECTED_ERROR;
        }

        /* Return to user mode */
        NU_USER_MODE();        
    }
    
    return (pm_status);
}

#endif  /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_SYSTEM == NU_TRUE) */


