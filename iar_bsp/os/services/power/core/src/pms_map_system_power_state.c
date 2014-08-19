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
*      pms_map_system_power_state.c
*
* COMPONENT
*
*      PMS - System State Services
*
* DESCRIPTION
*
*      Map system power state.
*
* DATA STRUCTURES
*
*      None
*
*   FUNCTIONS
*
*      NU_PM_Map_System_Power_State
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

/* Memory Pool */
extern NU_MEMORY_POOL  System_Memory;
/* Other externs */
extern PM_SYSTEM_GLOBAL_STATE  *PM_System_Global_Info;

extern PM_SYSTEM_STATE *PM_State_Map_Array[PM_MAX_SYSTEM_STATES];
extern STATUS NU_PM_Get_Power_State_Count(DV_DEV_ID dev_id,
                                      PM_STATE_ID *state_count_ptr);
extern STATUS NU_PM_Set_Power_State(DV_DEV_ID dev_id, PM_STATE_ID state);

static BOOLEAN PMS_Map_System_Power_State_Update (UINT8 system_state_id, DV_DEV_ID dev_id,
                                                  PM_STATE_ID state);
                                          
/*************************************************************************
*
* FUNCTION
*
*      NU_PM_Map_System_Power_State
*
* DESCRIPTION
*
*      This function maps a system power state into a specific peripheral
*      power state
*
* INPUTS
*
*      dev_id                   Device id of the peripheral to map to system
*                               state
*      state                    Power state on the peripheral to map to system
*                               state
*      system_state_id          System state to map to
*
* OUTPUTS
*
*      NU_SUCCESS               This indicates successful transition
*      PM_INVALID_SYSTEM_STATE  This error code indicates that the supplied
*                               system state is not valid
*      PM_INVALID_STATE         This error indicates an invalid state for the
*                               device
*      PM_UNEXPECTED_ERROR      This indicates an unexpected error has occurred
*                               (should never happen)
*
*************************************************************************/
STATUS NU_PM_Map_System_Power_State(UINT8 system_state_id, DV_DEV_ID dev_id,
                                       PM_STATE_ID state)
{
    STATUS              pm_status;
    STATUS              status;
    PM_SYSTEM_STATE_MAP *system_map_ptr;
    PM_STATE_ID         state_count = 0;
    BOOLEAN             remap = NU_FALSE;
    
    NU_SUPERV_USER_VARIABLES

    /* Check the input parameters */
    /* Get the state count for this device */
    pm_status = NU_PM_Get_Power_State_Count(dev_id, &state_count);
          
    if ((state_count != 0) && (pm_status == NU_SUCCESS))
    {
        /* If the desired state is 255, power state is 'ON' */
        if (state == POWER_ON_STATE)
        {
            /* Set this to the device's max power state. Power state
               always starts at 0, which indicates 'OFF' */
            state = (state_count - 1);
        }
        
        /* Check if the device state to be mapped is valid */
        if (state >= state_count)
        {
            pm_status = PM_INVALID_STATE;
        }
        /* Ensure the system state is valid */
        else if (PM_State_Map_Array[system_state_id] == NU_NULL)
        {
            pm_status = PM_INVALID_SYSTEM_STATE;
        }        
        else 
        {                
            /* Switch to supervisor mode */
            NU_SUPERVISOR_MODE();     

            /* First, check if this particular (system_state_id - device id map) already 
               exists and needs to be updated */
            remap = PMS_Map_System_Power_State_Update (system_state_id, dev_id, state);
                                                         
            /* If this was not a remap update, a new map needs to be created */
            if (remap == NU_FALSE)
            {
                /* Allocate memory for the map structure */
                status = NU_Allocate_Memory(&System_Memory, (VOID*)&system_map_ptr,
                                            sizeof(PM_SYSTEM_STATE_MAP), NU_NO_SUSPEND);
        
                if (status == NU_SUCCESS)
                {                   
                    /* Setup device info structure for the new map */
                    system_map_ptr->device              = dev_id;
                    system_map_ptr->device_state        = state;

                    /* Update priority */
                    system_map_ptr->system_map_list.cs_priority = state; 
                    
                    /* Protect against access to the map list.  */
                    NU_Protect(&(PM_State_Map_Array[system_state_id]->map_list_protect));

                    /* Link the structure into the list of requests */
                    NU_Priority_Place_On_List(&(PM_State_Map_Array[system_state_id]->map_list_ptr),
                                              &(system_map_ptr->system_map_list));

                    /* Update total number of state maps */
                    PM_State_Map_Array[system_state_id]->total_devices++;

                    /* Release protection against access to the list of waiting tasks */
                    NU_Unprotect();
                                                                        
                    /* Set the state for the newly mapped device */
                    if (PM_System_Global_Info -> latest_system_set_state == system_state_id)
                    {
                        pm_status = NU_PM_Set_Power_State(system_map_ptr->device, 
                                                          system_map_ptr->device_state);
                    }
                }
                else
                {
                    pm_status = PM_UNEXPECTED_ERROR;                                      
                }
            }
            /* Return to user mode */
            NU_USER_MODE();
        }
    }
    
    /* Trace log */
    T_SYS_MAP(dev_id, state, system_state_id, pm_status);
    
    return(pm_status);
}


/*************************************************************************
*
* FUNCTION
*
*      PMS_Map_System_Power_State_Update
*
* DESCRIPTION
*
*      This function re-maps a system power state into a specific peripheral
*      power state
*
* INPUTS
*
*      dev_id                   Device id of the peripheral to map to system
*                               state
*      state                    Power state on the peripheral to map to system
*                               state
*      system_state_id          System state to map to
*
* OUTPUTS
*
*      NU_TRUE                  Device re-map is successful
*      NU_FALSE                 Device map does not exist for remap
*
*************************************************************************/
static BOOLEAN PMS_Map_System_Power_State_Update (UINT8 system_state_id, DV_DEV_ID dev_id,
                                          PM_STATE_ID state)
{ 
    CS_NODE                  *next_map_ptr;
    PM_SYSTEM_STATE_MAP      *current_map;  
    BOOLEAN                  device_remap = NU_FALSE;

    /* Ensure list is not empty for this system state */
    if (PM_State_Map_Array[system_state_id]->map_list_ptr != NU_NULL) 
    {             
        /* Get a pointer to the start of the map list for the system state */
        next_map_ptr = PM_State_Map_Array[system_state_id]->map_list_ptr;
       
        /* Search the map list of this state for the device map
           structure that matches the dev_id */
        do
        {
            /* Save a pointer to the current map structure */
            current_map = (PM_SYSTEM_STATE_MAP *)next_map_ptr;
            
            /* Make sure we have a pointer to the next map in the list */
            next_map_ptr = current_map->system_map_list.cs_next;
            
            /* If a map for this device exists for the same system_state_id */
            if (dev_id == (DV_DEV_ID)current_map->device) 
            {                
                /* Protect against access to the map list.  */
                NU_Protect(&(PM_State_Map_Array[system_state_id]->map_list_protect));
                 
                /* Replace the existing device power state with the new one */ 
                current_map->device_state = state;
                
                /* Existing map has been updated */
                device_remap = NU_TRUE;
                
                /* Release protection against access to the list */
                NU_Unprotect();        

                /* Set the state for the newly mapped device */
                if (PM_System_Global_Info -> latest_system_set_state == system_state_id)
                {
                    (VOID)NU_PM_Set_Power_State(dev_id, current_map->device_state);
                }
            }
            
        /* Once the device map has been updated or we have reached the end of the list */
        } while ((device_remap == NU_FALSE) && (next_map_ptr != PM_State_Map_Array[system_state_id]->map_list_ptr));
    }
   
    return (device_remap);
}

#endif  /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_SYSTEM == NU_TRUE) */


