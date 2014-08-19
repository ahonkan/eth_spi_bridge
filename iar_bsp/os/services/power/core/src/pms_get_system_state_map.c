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
*      pms_get_system_state_map.c
*
* COMPONENT
*
*      PMS - System State Services
*
* DESCRIPTION
*
*      This file gets device states for a given system state.
*
* DATA STRUCTURES
*
*      None
*
*   FUNCTIONS
*
*      NU_PM_Get_System_State_Map
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

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_SYSTEM == NU_TRUE)

extern PM_SYSTEM_STATE *PM_State_Map_Array[PM_MAX_SYSTEM_STATES];
extern STATUS NU_PM_Get_Power_State_Count(DV_DEV_ID dev_id,
                                      PM_STATE_ID *state_count_ptr);
                                      
/*************************************************************************
*
* FUNCTION
*
*      NU_PM_Get_System_State_Map
*
* DESCRIPTION
*
*      This function returns the device state for a given system state.
*
* INPUTS
*
*      dev_id                   Device id of the peripheral to map to system
*                               state
*      state_id_ptr             Pointer to location of retrieved state id
*      system_state_id          System state to map to
*
* OUTPUTS
*
*      NU_SUCCESS                   This indicates successful transition
*      PM_INVALID_POINTER           This error indicates the provided
*                                   pointer is invalid
*      PM_INVALID_SYSTEM_STATE      This indicates that the system state 
*                                   id passed in is invalid
*
*************************************************************************/
STATUS NU_PM_Get_System_State_Map(UINT8 system_state_id, DV_DEV_ID dev_id,
                                     PM_STATE_ID *state_id_ptr )
{
    STATUS            pm_status;
    CS_NODE             *next_map_ptr;
    CS_NODE             *head_ptr;
    PM_SYSTEM_STATE_MAP *current_map;
    PM_STATE_ID          state_count;
    BOOLEAN              map_found = NU_FALSE;


    /* Check input parameters */
    /* Get the state count for this device */
    pm_status = NU_PM_Get_Power_State_Count(dev_id, &state_count);
          
    if (pm_status == NU_SUCCESS)
    {
        /* Ensure pointer in not null and state is valid */ 
        if (state_id_ptr == NU_NULL)
        {
            pm_status = PM_INVALID_POINTER;
        }
        
        /* Ensure the system state is valid */
        else if ((PM_State_Map_Array[system_state_id] == NU_NULL) || (system_state_id >= PM_MAX_SYSTEM_STATES))
        {
            pm_status = PM_INVALID_SYSTEM_STATE;
        }
        
        else 
        {          
            /* Ensure list is not empty */
            if (PM_State_Map_Array[system_state_id]->map_list_ptr != NU_NULL)
            {
    
                /* Get a pointer to the start of the map list for this system state */
                next_map_ptr = PM_State_Map_Array[system_state_id]->map_list_ptr;

                /* Save head of list */
                head_ptr = next_map_ptr;
        
                /* Search the system map list of this system state for the
                   structure that matches the device id */
                do
                {
                         
                    /* Save a pointer to the current map structure of this state */
                    current_map = (PM_SYSTEM_STATE_MAP *)next_map_ptr;
        
                    /* Make sure we have a pointer to the next map in the list */
                    next_map_ptr = current_map->system_map_list.cs_next;
        
                    /* See if device id passed in is found in the state map */
                    if (dev_id == current_map->device)
                    {
                        /* Place the state of the device in pointer to return to caller */
                        *state_id_ptr = current_map->device_state;
                        
                        map_found = NU_TRUE;
                        pm_status = NU_SUCCESS;
                    }
                    else
                    {
                        pm_status = PM_INVALID_POINTER;
                    }
        
                } while((map_found == NU_FALSE) && (next_map_ptr != head_ptr));
            }
        } 
    }

    return(pm_status);

}

#endif  /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_SYSTEM == NU_TRUE) */


