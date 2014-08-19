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
*      pms_set_system_state.c
*
* COMPONENT
*
*      PMS - System State Services
*
* DESCRIPTION
*
*      This file sets System states for a device.
*
* DATA STRUCTURES
*
*      None
*
* FUNCTIONS
*     
*      NU_PM_Set_System_State
*
* DEPENDENCIES
*
*      power_core.h
*      system_state.h
*
***********************************************************************/
#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "services/power_core.h"
#include    "os/services/power/core/inc/system_state.h"
#include    "services/nu_trace_os_mark.h"

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_SYSTEM == NU_TRUE)

/* EQM Instance. */
extern EQM_EVENT_QUEUE System_Eqm;

/* Memory Pool */
extern NU_MEMORY_POOL  System_Memory;
/* Other externs */
extern PM_SYSTEM_GLOBAL_STATE  *PM_System_Global_Info;
extern PM_SYSTEM_STATE *PM_State_Map_Array[PM_MAX_SYSTEM_STATES];
extern STATUS NU_PM_Set_Power_State(DV_DEV_ID dev_id, PM_STATE_ID state);

/* System State Services Virtual Driver Device ID */
extern DV_DEV_ID PM_System_State_Driver_ID;

/* Local function prototype */
STATUS PMS_Set_System_State(UINT8 state_id);
static STATUS PMS_Set_Map_States(UINT8 state_id);

/*************************************************************************
*
* FUNCTION
*
*      NU_PM_Set_System_State
*
* DESCRIPTION
*
*      This function executes a transition to the specified system state.
*
* INPUTS
*
*      state_id                 System state to transition into
*
* OUTPUTS
*
*      NU_SUCCESS               This indicates successful transition
*      PM_DEFERRED              This indicates that the system noted the
*                               requested state change, however at this time
*                               there exists a Minimum State Request which is
*                               holding the state at a higher value (PMS-only
*                               deployments)
*      PM_INVALID_SYSTEM_STATE  This indicates the system state requested is
*                               not valid
*      PM_UNEXPECTED_ERROR      This indicates an unexpected error has occurred
*                               (should never happen)
*
*************************************************************************/
STATUS NU_PM_Set_System_State(UINT8 state_id)
{
    STATUS pm_status = NU_SUCCESS;
    
    NU_SUPERV_USER_VARIABLES
    
    /* Check input parameters */
    if ((state_id >= PM_MAX_SYSTEM_STATES) || (PM_State_Map_Array[state_id] == NU_NULL))
    {
        pm_status = PM_INVALID_SYSTEM_STATE;
    }
    else
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Trace log */
        T_SYS_TL_START();
            
        /* Save the latest Set System State cmd in the State block */
        PM_System_Global_Info -> latest_system_set_state = state_id;
        
        /* If there is no pending request, it means we can change the 
           state immediately. */
        if (PM_System_Global_Info -> request_list_ptr == NU_NULL) 
        {
            /* If the current system state is not already equal to the set state 
               cmd, change the state, else do nothing */
            if (state_id != PM_System_Global_Info -> system_current_state)
            {
                /* Change the state of the system */
                pm_status = PMS_Set_System_State(state_id);
            }
        }
        /* Check for emergency state */
        else if (PM_System_Global_Info -> total_emergency_requests > 0)
        {   
            /* Check if desired system state is less than current emergency system state. 
               If desired state is lower than current state, change the state */                       
            if (state_id < PM_System_Global_Info -> system_current_state)
            {
                /* Change the state of the system */
                pm_status = PMS_Set_System_State(state_id);
            }
            /* If the desired state is equal to the current state return success */
            else if (state_id == PM_System_Global_Info -> system_current_state)
            {
                pm_status = NU_SUCCESS;
            }
            else  
            {
                /* Since the desired state is lower than the current 
                   min system state, defer the set state command */
                pm_status = PM_DEFERRED;
            }
        }
        else
        {    
            /* There is request for minimum state for the system */
               
            /* Check if desired system state is greater than current min system state. 
               If desired state is higher than current min state, change the state */                       
            if (state_id > PM_System_Global_Info -> system_current_state)
            {
                /* Change the state of the system */
                pm_status = PMS_Set_System_State(state_id);
            }
            /* If the desired state is equal to the current state return success */
            else if (state_id == PM_System_Global_Info -> system_current_state)
            {
                pm_status = NU_SUCCESS;
            }
            else  
            {
                /* Since the desired state is lower than the current 
                   min system state, defer the set state command */
                pm_status = PM_DEFERRED;
            }                       
        }
        
        /* Trace log */
        T_SYS_TL_STOP();
    
        /* Return to user mode */
        NU_USER_MODE();
    }
    
    /* Trace log */
    T_SYS_TRANS(PM_System_Global_Info->system_current_state, pm_status);
    
    return (pm_status);
}

/*************************************************************************
*
* FUNCTION
*
*      PMS_Set_System_State
*
* DESCRIPTION
*
*      This function executes a transition to the specified system state.
*
* INPUTS
*
*      state_id                 System state to transition into
*
* OUTPUTS
*
*      NU_SUCCESS               This indicates successful transition
*      PM_DEVICE_DEFERRED       This error code indicates that one or more
*                               devices returned PM_DEFERRED
*      PM_UNEXPECTED_ERROR      This indicates an unexpected error has occurred
*                               (should never happen)
*
*************************************************************************/
STATUS PMS_Set_System_State(UINT8 state_id)
{
    STATUS                  pm_status = NU_SUCCESS;
    STATUS                  eqm_status;
    PM_SYSTEM_NOTIFICATION  state_message;

    /* Update the devices in the map to the new state */
    pm_status = PMS_Set_Map_States(state_id);
    
    if (pm_status == PM_DEVICE_DEFERRED)
    {
        /* Deferred is considered ok to continue updating
           the system state, fail for any other error */
        pm_status = NU_SUCCESS;
    }

    if ((pm_status == NU_SUCCESS) && (state_id != PM_System_Global_Info->system_current_state))
    {
        /* Assign the state change notification to the event type. */
        state_message.event_type = PM_STATE_CHANGE;
        
        /* Update new state information to the new state successfully set */
        state_message.new_state = state_id;
        
        /* Update notification message structure with old state information */        
        state_message.old_state = PM_System_Global_Info->system_current_state;
    
        /* Ensure we update the Global system current state to the new set state */
        PM_System_Global_Info->system_current_state = state_id;
        
        /* Send a notification to listeners now that system state change is successful */
        eqm_status = NU_EQM_Post_Event(&System_Eqm, (EQM_EVENT*)(&state_message), sizeof(state_message), NU_NULL);

        /* Check for unexpected failure in notification */
        if (eqm_status != NU_SUCCESS)
        {
            pm_status = PM_UNEXPECTED_ERROR;
        }

    }

    return(pm_status);
}

/*************************************************************************
*
* FUNCTION
*
*      PMS_Set_Map_States
*
* DESCRIPTION
*
*      This function updates all devices in the map to a new state.
*
* INPUTS
*
*      state_id                 System state to set devices in the map to
*
* OUTPUTS
*
*      NU_SUCCESS               This indicates successful transition
*      PM_DEVICE_DEFERRED       This error code indicates that one or more
*                               devices returned PM_DEFERRED
*
*************************************************************************/
static STATUS PMS_Set_Map_States(UINT8 state_id)
{
    STATUS              pm_status = NU_SUCCESS;
    STATUS              deferred_status = NU_SUCCESS;
    CS_NODE             *tail_ptr;
    CS_NODE             *next_map_ptr;
    PM_SYSTEM_STATE_MAP *current_map;
    
    /* Ensure map list is not empty for this state */
    if(PM_State_Map_Array[state_id]->map_list_ptr != NU_NULL)    
    {             
        /* Get a pointer to the start of the map list for the system state */
        next_map_ptr = PM_State_Map_Array[state_id]->map_list_ptr;
        
        /* Get a pointer to the last map on the list */
        tail_ptr = next_map_ptr->cs_previous;
            
        /* Set the state if each device associated with this system state id */
        do
        {
            /* Save a pointer to the current map structure */
            current_map = (PM_SYSTEM_STATE_MAP *)next_map_ptr;
            
            /* Make sure we have a pointer to the next map in the list */
            next_map_ptr = current_map->system_map_list.cs_next;
        
            /* Set the state of the device */
            pm_status = NU_PM_Set_Power_State(current_map->device, current_map->device_state);
            
            /* If the device has deferred the state change, mark it as device deferred */
            if (pm_status == PM_DEFERRED)
            {
                /* A device has failed save the deferment
                   and continue processing the list */
                deferred_status = PM_DEVICE_DEFERRED;
            }

        } while(current_map != (PM_SYSTEM_STATE_MAP *)tail_ptr);
        
        /* Update to deferred if any device deferred */
        if (deferred_status == PM_DEVICE_DEFERRED)
        {
            pm_status = deferred_status;
        }
    }
    
    return (pm_status);
}

#endif  /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_SYSTEM == NU_TRUE) */


