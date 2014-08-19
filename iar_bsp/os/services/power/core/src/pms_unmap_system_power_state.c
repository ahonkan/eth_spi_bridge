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
*      pms_unmap_system_power_state.c
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
*      NU_PM_Unmap_System_Power_State
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
extern PM_SYSTEM_GLOBAL_STATE  *PM_System_Global_Info;
extern NU_MEMORY_POOL           System_Memory;

/* Other externs */
extern PM_SYSTEM_STATE *PM_State_Map_Array[PM_MAX_SYSTEM_STATES];
extern STATUS NU_PM_Get_Power_State_Count(DV_DEV_ID dev_id,
                                      PM_STATE_ID *state_count_ptr);

/*************************************************************************
*
* FUNCTION
*
*      NU_PM_Unmap_System_Power_State
*
* DESCRIPTION
*
*      This function unmaps a system power state from a specific peripheral
*      power state
*
* INPUTS
*
*      dev_id                   Device id of the peripheral to unmap to system
*                               state
*
* OUTPUTS
*
*      NU_SUCCESS                   This indicates successful transition
*      PM_INVALID_DEVICE_ID         This error code indicates that the supplied
*                                   dev_id is not valid or not mapped
*      PM_SYSTEM_STATE_NEED_INIT    This error code indicates that the
*                                   system state services in not
*                                   initialized
*      PM_UNEXPECTED_ERROR          This indicates an unexpected error has occurred
*                                   (should never happen)
*
*************************************************************************/
STATUS NU_PM_Unmap_System_Power_State(DV_DEV_ID dev_id)
{
    STATUS          pm_status = NU_SUCCESS; 
    CS_NODE         *next_map_ptr;
    PM_STATE_ID     state_count;
    UINT8           i;
    INT             unmap_failure = 0;
    BOOLEAN         dev_unmapped = NU_FALSE;
    PM_SYSTEM_STATE_MAP      *current_map;  

    NU_SUPERV_USER_VARIABLES

    /* Ensure the system state is initialized and the array is not empty */
    if ((PM_State_Map_Array[0] != NU_NULL) && (PM_System_Global_Info != NU_NULL))
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Get system state count */
        state_count = PM_System_Global_Info->system_state_count;

        /* For all valid system states */
        for (i=0; i < state_count; i++)
        {
            /* Reset the unmap boolean */
            dev_unmapped = NU_FALSE;
            
            /* Ensure list is not empty for this state */
            if (PM_State_Map_Array[i]->map_list_ptr != NU_NULL) 
            {             
                /* Get a pointer to the start of the map list for the system state */
                next_map_ptr = PM_State_Map_Array[i]->map_list_ptr;
                
                /* Now see which structure needs to be deleted */
                /* Search the map list of this state for the device map
                   structure that matches the dev_id */
                do
                {
                    /* Save a pointer to the current map structure */
                    current_map = (PM_SYSTEM_STATE_MAP *)next_map_ptr;
                    
                    /* Make sure we have a pointer to the next map in the list */
                    next_map_ptr = current_map->system_map_list.cs_next;
                    
                    if (dev_id == (DV_DEV_ID)current_map->device)
                    {
                    
                        /* Protect against access to the map list.  */
                        NU_Protect(&(PM_State_Map_Array[i]->map_list_protect));
                
                        /* Remove the structure from the list of maps */                                     
                        NU_Remove_From_List(&(PM_State_Map_Array[i]->map_list_ptr), 
                                            &(current_map->system_map_list));  
                                             
                        /* Decrement the number of maps for this system state */
                        PM_State_Map_Array[i]->total_devices--;
                        
                        /* Set dev_unmapped to TRUE */
                        dev_unmapped = NU_TRUE; 
                        
                        /* Release protection against access to the list */
                        NU_Unprotect();    
                                                                 
                        /* Deallocate memory for this Map */
                        (VOID)NU_Deallocate_Memory (current_map);                                               
                    }
                /* Once the device is removed or we have reached the end of the list */
                } while ((dev_unmapped == NU_FALSE) && (next_map_ptr != PM_State_Map_Array[i]->map_list_ptr));
            }
            
            /* If no unmapping occurred track it */
            if (dev_unmapped == NU_FALSE)
            {
                unmap_failure++;
            }
            
        } /* End state count loop */
        
        /* Check for invalid device ID */
        if (unmap_failure == state_count)
        {
            pm_status = PM_INVALID_DEVICE_ID;
        }
                
        /* Return to user mode */
        NU_USER_MODE();
    }
    else
    {
        pm_status = PM_SYSTEM_STATE_NEED_INIT;
    }
    
    /* Trace log */
    T_SYS_UNMAP(dev_id, pm_status);  
    
    return (pm_status);
}

#endif  /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_SYSTEM == NU_TRUE) */


