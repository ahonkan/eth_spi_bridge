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
*       pms_emergency_state_release.c
*
* COMPONENT
*
*       PMS - System State Services
*
* DESCRIPTION
*
*       Release emergency system state for the system
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*     
*       NU_PM_Emergency_State_Release
*
* DEPENDENCIES
*
*       power_core.h
*       system_state.h
*
***********************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/power_core.h"
#include "os/services/power/core/inc/system_state.h"

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_SYSTEM == NU_TRUE)

/* Memory Pool */
extern NU_MEMORY_POOL  System_Memory;

/* Other externs */
extern PM_SYSTEM_GLOBAL_STATE  *PM_System_Global_Info;
extern PM_SYSTEM_STATE         *PM_State_Map_Array[PM_MAX_SYSTEM_STATES];
extern STATUS                   PMS_Set_System_State(UINT8 state_id);

/*************************************************************************
*
* FUNCTION
*
*      NU_PM_Emergency_State_Release
*
* DESCRIPTION
*
*      This function releases a previously placed request for an
*      emergency system state.
*
* INPUTS
*
*      handle                   Request handle to be released
*
* OUTPUTS
*
*      NU_SUCCESS               This indicates successful transition
*      PM_INVALID_REQ_HANDLE    This indicates that the requested handle
*                               is not valid
*      PM_UNEXPECTED_ERROR      This indicates an unexpected error has 
*                               occurred (should never happen)
*
*************************************************************************/
STATUS NU_PM_Emergency_State_Release(PM_MIN_REQ_HANDLE handle)
{
    STATUS                status;
    STATUS                pm_status = NU_SUCCESS; 
    CS_NODE              *tail_request_ptr;
    UINT8                 latest_state;
    PM_EMERGENCY_REQUEST *current_request;
    PM_EMERGENCY_REQUEST *highest_request;
    PM_EMERGENCY_REQUEST *head_request;
    
    NU_SUPERV_USER_VARIABLES    
    
    /* Check input parameters */    
    /* Ensure list is not empty */
    if ((handle == 0) || (PM_System_Global_Info -> request_list_ptr == NU_NULL)) 
    {
        pm_status = PM_INVALID_REQ_HANDLE;
    }
    else 
    {       
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();
        
        /* Get a pointer to the handle */
        current_request = (PM_EMERGENCY_REQUEST *)handle;
            
        /* Protect against access to the request list.  */
        NU_Protect(&(PM_System_Global_Info -> request_list_protect));

        /* Remove the structure from the list of requests */                                     
        NU_Remove_From_List(&(PM_System_Global_Info -> request_list_ptr), 
                             &(current_request -> emergency_request_list));  
                             
        /* Decrement the number of requests for this device */
        PM_System_Global_Info -> total_state_requests--;
        PM_System_Global_Info -> total_emergency_requests--;
        
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
            latest_state = PM_System_Global_Info -> latest_system_set_state;
            
            /* If there is at least one more request in the list */
            if (PM_System_Global_Info->total_state_requests != 0)
            {             
                NU_Protect(&(PM_System_Global_Info -> request_list_protect));
                
                /* Get a pointer to the request at the head of the list */
                head_request = (PM_EMERGENCY_REQUEST *)PM_System_Global_Info -> request_list_ptr;
                
                if (head_request != NU_NULL)
                {
                    /* The tail pointer points to the highest request */
                    /* Note: In PLUS, 0 is highest priority but in PM, 0 is the lowest state */
                    tail_request_ptr = head_request -> emergency_request_list.cs_previous;
                    
                    highest_request = (PM_EMERGENCY_REQUEST *)tail_request_ptr;
                
                    NU_Unprotect();   
                    
                    /* If the next emergency request is less than the latest state
                       we are still in emergency mode raise to the new emergency state */
                    if (highest_request -> requested_max_state < latest_state)
                    {
                        /* Set the state to the requested minimum. Since the list is sorted, 
                           the next highest state is the head */
                        pm_status = PMS_Set_System_State(highest_request -> requested_max_state);
                    }
                    /* Check if the highest request is equal to the latest set state AND the 
                       current state is the same as the latest set state. If equal, there 
                       is nothing to be done here, return NU_SUCCESS */
                    else if ((highest_request -> requested_max_state == latest_state) && 
                             (PM_System_Global_Info->system_current_state == latest_state))
                    {
                        pm_status = NU_SUCCESS;
                    }
                    else
                    {
                        /* Set the state to the latest set state cmd saved */
                        pm_status = PMS_Set_System_State(latest_state);
                    }
                }
                else
                {
                    NU_Unprotect(); 
                    
                    pm_status = PM_UNEXPECTED_ERROR;
                }
            }
            else
            {
                /* Set the state to the latest set state cmd saved */
                pm_status = PMS_Set_System_State(latest_state);
            }
        }  
        
        /* Return to user mode */
        NU_USER_MODE();       
    }
    
    return (pm_status);
}

#endif  /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_SYSTEM == NU_TRUE) */


