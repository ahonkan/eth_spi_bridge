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
*      pms_system_state_init.c
*
* COMPONENT
*
*      PMS - System State Services
*
* DESCRIPTION
*
*      Initialize System State Services.
*
* DATA STRUCTURES
*
*      None
*
* FUNCTIONS
*     
*      NU_PM_System_State_Init
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

/* Global System State Information */
PM_SYSTEM_GLOBAL_STATE  *PM_System_Global_Info;

/* System State Init flag */
static BOOLEAN PM_System_Init_Flag       = NU_FALSE;

/* Array of System States */
PM_SYSTEM_STATE *PM_State_Map_Array[PM_MAX_SYSTEM_STATES];

/*************************************************************************
*
* FUNCTION
*
*      NU_PM_System_State_Init
*
* DESCRIPTION
*
*      This function initializes the System State Services with the
*      specified number of states.
*
* INPUTS
*
*      state_count          Total number of states to be initialized.
*
* OUTPUTS
*
*      NU_SUCCESS           This indicates successful transition
*      PM_DUPLICATE_INIT    This indicates that the system states 
*                           have already been initialized
*      PM_UNEXPECTED_ERROR  This indicates an unexpected error
*                           has occurred (should never happen)
*
*************************************************************************/
STATUS NU_PM_System_State_Init(UINT8 state_count)
{
    STATUS                  pm_status = NU_SUCCESS;
    STATUS                  status;
    UINT8                   i, state_map_array_cnt;
    PM_SYSTEM_STATE         *system_state_info;
    
    NU_SUPERV_USER_VARIABLES

    /* Check input parameters */
    if ((state_count == 0) || (state_count > PM_MAX_SYSTEM_STATES) || (state_count > INVALID_SYSTEM_STATE))
    {
        /* No system states were passed */
        pm_status = PM_UNEXPECTED_ERROR;
    } 
    else 
    {    
        /* Ensure System State is not already initialized */
        if (PM_System_Init_Flag == NU_FALSE)
        { 
            /* Switch to supervisor mode */
            NU_SUPERVISOR_MODE();
            
            /* Allocate memory for the Global System State structure */
            status = NU_Allocate_Memory(&System_Memory, (VOID*)&PM_System_Global_Info,
                                        sizeof(PM_SYSTEM_GLOBAL_STATE), NU_NO_SUSPEND);
    
            if (status == NU_SUCCESS)
            {                  
               /* Clear the memory we just allocated */
               (VOID)memset((VOID*)PM_System_Global_Info, 0, sizeof(PM_SYSTEM_GLOBAL_STATE));

               /* Ensure current state and latest_system_set_state is set to an invalid value (state 0 is valid) */
               PM_System_Global_Info->latest_system_set_state = INVALID_SYSTEM_STATE;
               PM_System_Global_Info->system_current_state = INVALID_SYSTEM_STATE;
                        
                /* Now initialize the array of System State maps for each System State */
                for (state_map_array_cnt=0; state_map_array_cnt < state_count; state_map_array_cnt++)
                {
                    if (pm_status == NU_SUCCESS)
                    {
                        /* Allocate memory for the map (device-device state pair) structure */
                        status = NU_Allocate_Memory(&System_Memory, (VOID*)&system_state_info,
                                                    sizeof(PM_SYSTEM_STATE), NU_NO_SUSPEND);
            
                        if (status == NU_SUCCESS)
                        {
                            /* Clear the memory we just allocated */
                            (VOID)memset((VOID*)system_state_info, 0, sizeof(PM_SYSTEM_STATE));
               
                            /* Place the structure in the array */
                            /* Index of this array corresponds to system_state_id */
                            PM_State_Map_Array[state_map_array_cnt] = system_state_info;  
                            
                            /* Increment the global System State Count */
                            PM_System_Global_Info->system_state_count++;

                        }
                        else
                        {
                            /* Deallocate memory allocated for Global System State Structure */
                            (VOID)NU_Deallocate_Memory (PM_System_Global_Info);
                            
                            for (i=0; i < state_map_array_cnt; i++)
                            {
                                (VOID)NU_Deallocate_Memory (PM_State_Map_Array[i]);
                            }
                                                   
                            /* Update the value of pm_status */
                            pm_status = PM_UNEXPECTED_ERROR;
                        }
                    }
                }
                
                if (pm_status == NU_SUCCESS)
                {   
                    /* Set Init flag to TRUE */
                    PM_System_Init_Flag = NU_TRUE;
                }
            }
            else
            {
                /* Memory allocation for */
                pm_status = PM_UNEXPECTED_ERROR;
            }
            
            /* Return to user mode */
            NU_USER_MODE();
        }
        else 
        {
            /* System State has already been initialized */
            /* Indicate duplicate Initialization */
            pm_status = PM_DUPLICATE_INIT;
        }
    }
    
    if ((pm_status == NU_SUCCESS) || (pm_status == PM_DUPLICATE_INIT))
    {
        /* Trace log */
        T_SYS_COUNT(PM_System_Global_Info->system_state_count, pm_status);
    }
    else
    {
        /* Trace log */
        T_SYS_COUNT(0, pm_status);
    }
   
    return(pm_status);
}

#endif  /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_SYSTEM == NU_TRUE) */


