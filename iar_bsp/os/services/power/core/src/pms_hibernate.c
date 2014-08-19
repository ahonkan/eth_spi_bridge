/*************************************************************************
*
*            Copyright 2012 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILE NAME
*
*       pms_hibernate.c
*
*   COMPONENT
*
*       Hibernate
*
*   DESCRIPTION
*
*       Contains all functionality for hibernate
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       PMS_Hibernate_Enter
*       PMS_Hibernate_Exit
*       NU_PM_Restore_Hibernate_Device
*
*   DEPENDENCIES
*
*       <stdio.h>
*       nucleus.h
*       nu_kernel.h
*       nu_services.h
*       nu_drivers.h
*       runlevel.h
*       dvfs.h
*       hibernate.h
*       peripheral.h
*       thread_control.h
*       error_management.h
*       device_manager.h
*       nvm.h
*
*************************************************************************/


#include  <stdio.h>
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"
#include "os/services/init/inc/runlevel.h"
#include "os/services/power/core/inc/dvfs.h"
#include "os/kernel/plus/core/inc/thread_control.h"
#include "os/services/power/core/inc/hibernate.h"
#include "os/services/power/core/inc/nvm.h"
#include "os/services/power/core/inc/peripheral.h"
#include "os/kernel/plus/supplement/inc/error_management.h"
#include "os/kernel/devmgr/inc/device_manager.h"
#include "services/nu_trace_os_mark.h"

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE)

extern      NU_TASK             PMS_Set_OP_Task;
extern      NU_EVENT_GROUP      RunLevel_Started_Event;
extern      NU_EVENT_GROUP      RunLevel_Finished_Event;
extern      EQM_EVENT_QUEUE     System_Eqm;
extern      UINT32              PMS_Hibernate_Self_Refresh;
extern      UINT32              PMS_Prev_Boot_State;
extern      NU_QUEUE            PMS_Set_OP_Queue;
extern      PM_DEVICE_INFO *    PM_Dev_Info_Array[PM_MAX_DEV_CNT];

/* Handle to the open Hibernate driver */
DV_DEV_HANDLE  PM_Hibernate_Handle;

/* Handle to the open Alarm driver */
DV_DEV_HANDLE  PM_Alarm_Handle;

/* Base to Hibernate IOCTLs in the open driver */
UINT32         PM_Hibernate_Base;

/* Base to Alarm IOCTLs in the open driver */
UINT32         PM_Alarm_Base;

/*************************************************************************
*
*   FUNCTION
*
*       PMS_Hibernate_Enter
*
*   DESCRIPTION
*
*       This function is the entry point for Hibernate functionality. 
*       Once a Hibernate request is made, components are notified to 
*       hibernate or stop. The Hibernate driver is then called to 
*       initiate system shutdown. 
*
*   INPUT
*
*       hibernate_mode                      0 for hibernate shutdown
*                                           1 for hibernate suspend 
*
*   OUTPUT
*
*       None
*
*************************************************************************/
VOID PMS_Hibernate_Enter(UINT8 hibernate_mode)
{    
    STATUS                  status = NU_SUCCESS;
    CHAR                    keypath [REG_MAX_KEY_LENGTH];
    CHAR                    comppath [REG_MAX_KEY_LENGTH];
    INT                     runlevel;    
    INT                     entrynum;    
    DV_DEV_ID               hibernate_device_id;
    DV_IOCTL0_STRUCT        hibernate_ioctl0;
    PM_HIB_RES_NOTIFICATION hib_res_event;
    BOOLEAN                 runlevelcomplete;
    UINT32                  value;
    INT                     hibernate_dev_id_cnt = 1;
    DV_DEV_LABEL            hibernate_class_id = {HIBERNATE_LABEL};
    UNSIGNED                queue_return;
    UNSIGNED                int_level;
        
    /**************************/
    /*  Driver Notification   */
    /**************************/    
    
    /* Attach the event type to be placed on the queue */
    hib_res_event.pm_event_type = PM_HIBERNATE_NOTIFICATIONS;
    
    /* Post the event notification to the EQM Queue */
    status = NU_EQM_Post_Event (&System_Eqm, (EQM_EVENT*)(&hib_res_event), 
                                sizeof(PM_HIB_RES_NOTIFICATION), NU_NULL);

    /* Lock out interrupts for hibernate sequence */
    int_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);
    
    if (status == NU_SUCCESS)
    {
        /* For each runlevel value, starting from NU_RUNLEVEL_MAX */
        for (runlevel = NU_RUNLEVEL_MAX; runlevel >= 0; runlevel--)
        {
            /* Initialize entrynum for this runlevel */
            entrynum = 1;
            runlevelcomplete = NU_FALSE;
            
            /* Set start bit for this run-level */
            (VOID) NU_Set_Events (&RunLevel_Started_Event, (1UL << runlevel), NU_OR);

            /* Loop until no more components found at the given run-level */
            do
            {
                /* Create registry path for this entry */
                sprintf (keypath, RUNLEVEL_ROOT "%d/%d", runlevel, entrynum);

                /* Get registry entry for this string */
                status = REG_Get_String ((const CHAR *)keypath, comppath, REG_MAX_KEY_LENGTH);
                          
                /* Ensure this entry exits
                   NOTE: This shouldn't fail, but continue trying other entries for this
                         run-level even if one entry is missing information */
                if (status == NU_SUCCESS)
                {                                    
                    /* Check if this component/device is hibernate aware */
                    status = REG_Get_UINT32_Value ((const CHAR *)comppath, "/hibernate_dev", &value);
                    
                    /* If component is hibernate aware, set component control to HIBERNATE */           
                    if ((status == NU_SUCCESS) && (value == 1))
                    {
                        status = NU_RunLevel_Component_Control (comppath, RUNLEVEL_HIBERNATE);
                    }
                    /* If component is not hibernate aware, set component control to STOP */
                    else if ((status != NU_SUCCESS) || (value == 0))
                    {
                        status = NU_RunLevel_Component_Control (comppath, RUNLEVEL_STOP);               
                    }
                    
                    /* Go to next entry number */
                    entrynum++;
                    value = 0;
                }
                else
                {
                    /* Done with this run-level */
                    runlevelcomplete = NU_TRUE;
                }
                                    
            /* Check if runlevel is complete */
            } while (!runlevelcomplete);

            /* Set finished bit for this run-level */
            (VOID) NU_Set_Events (&RunLevel_Finished_Event, (1UL << runlevel), NU_OR);
            
        } /* end for all runlevels */
    } /* end EQM event notification */

    /**************************/
    /*   Initiate Shutdown    */
    /**************************/
    if ((status == NU_SUCCESS) || (status == REG_BAD_PATH))
    {
        /* Check if the hibernate driver is not already open. */
        if(!PM_Hibernate_Handle)
        {
            /* Get the device ID of the Hibernate driver */
            status = DVC_Dev_ID_Get (&hibernate_class_id, 1, &hibernate_device_id, &hibernate_dev_id_cnt);

            /* If call was successful and Hibernate device was found */
            if ((status == NU_SUCCESS) && (hibernate_dev_id_cnt > 0))
            {

                /* Open the Hibernate Driver and save the handle to use later */
                status = DVC_Dev_ID_Open (hibernate_device_id, &hibernate_class_id, 1,
                                          &PM_Hibernate_Handle);
            }
        }

        /* Check for bad path again, if the hibernate driver was already open 
           status may not be reset */
        if ((status == NU_SUCCESS) || (status == REG_BAD_PATH))
        {
            /* Get ioctl base */
            hibernate_ioctl0.label = hibernate_class_id;

            /* Call IOCTL0 */
            status = DVC_Dev_Ioctl (PM_Hibernate_Handle, DV_IOCTL0, &hibernate_ioctl0,
                                    sizeof(hibernate_ioctl0));

            if (status == NU_SUCCESS)
            {
                /* Save the ioctl base */
                PM_Hibernate_Base = hibernate_ioctl0.base;

                if (hibernate_mode == NU_DORMANT)
                {
                    /* Call Hibernate driver to shutdown CPU */
                    status = DVC_Dev_Ioctl (PM_Hibernate_Handle,
                                            (PM_Hibernate_Base + HIBERNATE_IOCTL_INIT_SHUTDOWN),
                                             NU_NULL, NU_NULL);
                }
                else
                {
                    /* Call Hibernate driver to place the system in standby */
                    status = DVC_Dev_Ioctl (PM_Hibernate_Handle,
                                            (PM_Hibernate_Base + HIBERNATE_IOCTL_INIT_STANDBY),
                                             NU_NULL, NU_NULL);
                }
            }
        }
    }
    
    /* restore interrupt level */
    (VOID)NU_Local_Control_Interrupts(int_level);

    /* Put return value is element of appropriate size */
    queue_return = (UNSIGNED) status;
    
    /* Return the value in the queue */
    (VOID) NU_Send_To_Queue (&PMS_Set_OP_Queue, &queue_return,
                             PM_SET_OP_QUEUE_SIZE, NU_SUSPEND);
}

/*************************************************************************
*
*   FUNCTION
*
*       PMS_Hibernate_Exit
*
*   DESCRIPTION
*
*       This function is the entry point to resume from Hibernate. 
*       Once RAM is restored and the OS is available, this API notifies
*       devices to resume from Hibernate and places the system in a high
*       power state.
*
*   INPUT
*
*       None
*
*   OUTPUT
*
*       None
*
*************************************************************************/
VOID PMS_Hibernate_Exit(VOID)
{
    STATUS                  status = NU_SUCCESS;
    CHAR                    keypath [REG_MAX_KEY_LENGTH];
    CHAR                    comppath [REG_MAX_KEY_LENGTH];
    INT                     runlevel;    
    INT                     entrynum;    
    PM_HIB_RES_NOTIFICATION hib_res_event;
    BOOLEAN                 runlevelcomplete;
    UINT32                  value;
    HB_RESUME_CB            hb_resume_cb;
    UINT                    index;
    UINT8                   exit_op;
    
    /* Avoid warnings when NVM not available */
    NU_UNUSED_PARAM(hb_resume_cb);
    
    /* RAM should be restored by now, if we were resuming from Hibernate mode 0. */
    
    /* Check if we are resuming from Hibernate level that utilizes self refresh RAM */
    if (PMS_Hibernate_Self_Refresh == PM_SELF_REFRESH_ON)
    {
        /* Resume from suspend. */

        /* Call Hibernate IOCTL to disable self-refresh on SDRAM and exit from STANDBY */
        status = DVC_Dev_Ioctl (PM_Hibernate_Handle, 
                                (PM_Hibernate_Base + HIBERNATE_IOCTL_EXIT_STANDBY), 
                                 NU_NULL, NU_NULL);   
    }
    else
    {
        /* Resume from shutdown. */

        /* Open NVM storage. */
        status = PM_NVM_OPEN();

        if (NU_SUCCESS == status)
        {
            /* Reset NVM write process. */
            status = PM_NVM_RESET();

        }

        if (NU_SUCCESS == status)
        {
            /* Reset Hibernate Resume control block (clears the hibernate
               resume flag in NVM). */
            hb_resume_cb.resume_pending = NU_FALSE;
            hb_resume_cb.hb_exit_func = NU_NULL;
            for (index = 0; index < HB_MAX_REGION_COUNT; index++)
            {
                hb_resume_cb.restore_addr[index] = NU_NULL;
            }

            /* Store hibernate resume control block as first item. */
            status = PM_NVM_WRITE(&hb_resume_cb, sizeof(HB_RESUME_CB),
                                  &index, NU_FALSE);
        }

        if (NU_SUCCESS == status)
        {
            /* Close NVM */
            status = PM_NVM_CLOSE();
        }

    }

    /**************************/
    /*  Driver Notification   */
    /**************************/   
    if (status == NU_SUCCESS)
    {
        /* Attach the event type to be placed on the queue */
        hib_res_event.pm_event_type = PM_RESUME_NOTIFICATIONS;
        
        /* Post the event notification to the EQM Queue */
        status = NU_EQM_Post_Event (&System_Eqm, (EQM_EVENT*)(&hib_res_event), 
                                    sizeof(PM_HIB_RES_NOTIFICATION), NU_NULL);
        
        if (status == NU_SUCCESS)
        {
            /* For each runlevel value, starting from 0 to NU_RUNLEVEL_MAX */
            for (runlevel = 0; runlevel <= NU_RUNLEVEL_MAX; runlevel++)
            {
                /* Initialize entrynum for this runlevel */
                entrynum = 1;
                runlevelcomplete = NU_FALSE;
                
                /* Set start bit for this run-level */
                (VOID) NU_Set_Events (&RunLevel_Started_Event, (1UL << runlevel), NU_OR);

                /* Loop until no more components found at the given run-level */
                do
                {
                    /* Create registry path for this entry */
                    sprintf (keypath, RUNLEVEL_ROOT "%d/%d", runlevel, entrynum);

                    /* Get registry entry for this string */
                    status = REG_Get_String ((const CHAR *)keypath, comppath, REG_MAX_KEY_LENGTH);
                              
                    /* Ensure this entry exits
                       NOTE: This shouldn't fail, but continue trying other entries for this
                             run-level even if one entry is missing information */
                    if (status == NU_SUCCESS)
                    {                                    
                        /* Check if this component/device is hibernate aware */
                        status = REG_Get_UINT32_Value ((const CHAR *)comppath, "/hibernate_dev",
                                                       &value);
                        
                        /* If component is hibernate aware, set component control to HIBERNATE */           
                        if ((status == NU_SUCCESS) && (value == 1))
                        {
                            status = NU_RunLevel_Component_Control (comppath, RUNLEVEL_RESUME);
                        }
                        /* If component is not hibernate aware, set component control to STOP */
                        else if ((status != NU_SUCCESS) || (value == 0))
                        {
                            status = NU_RunLevel_Component_Control (comppath, RUNLEVEL_START);
                        }
                        
                        /* Go to next entry number */
                        entrynum++;
                        value = 0;
                    }
                    else
                    {
                        /* Done with this run-level */
                        runlevelcomplete = NU_TRUE;
                    }
                                        
                /* Check if runlevel is complete */
                } while (!runlevelcomplete);

                /* Set finished bit for this run-level */
                (VOID) NU_Set_Events (&RunLevel_Finished_Event, (1UL << runlevel), NU_OR);
                
            } /* end for all runlevels */

            /* Intercept and filter expected status value used to indicate
               the end of a run-level. */
            if (REG_BAD_PATH == status)
            {
                status = NU_SUCCESS;
            }

        } /* end EQM event notification */
    }
    
    /* If all the previous operations have succeeded */    
    if (status == NU_SUCCESS)
    {
        /* Update the previous boot state */
        PMS_Prev_Boot_State = PM_HIBERNATE_RESUME;

        /* Get Exit OP */
        status = NU_PM_Get_Hibernate_Exit_OP (&exit_op);

        if (status == NU_SUCCESS) 
        {
            /* Set the exit OP to the desired value */
            (VOID) PMS_DVFS_Set_OP_Task_Entry ((UNSIGNED)exit_op, NU_NULL);

            /* Call Hibernate IOCTL to perform target specific operations when returns from hibernate. */
            status = DVC_Dev_Ioctl (PM_Hibernate_Handle, 
                                    (PM_Hibernate_Base + HIBERNATE_IOCTL_TGT_EXIT), 
                                     NU_NULL, NU_NULL);  
        }
        
        /* Trace log */
        T_HIB_EXIT(PMS_Prev_Boot_State, status);

        /* Terminate the current thread */
        (VOID) NU_Terminate_Task (&PMS_Set_OP_Task);
        
        /* Execution will not return from terminate thread */
    }
    else
    {
        /* Trace log */
        T_HIB_EXIT(PMS_Prev_Boot_State, status);
        
        /* Call error handling routine */
        ERC_System_Error(status);
    }                       
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_PM_Restore_Hibernate_Device
*
*   DESCRIPTION
*
*       This function performs basic operations to restore hibernate
*       device information.
*
*   INPUT
*
*       dev_id - ID of the device to restore.
*
*       ioctl_num - The IOCTL number to be called for each session.
*
*   OUTPUT
*
*       NU_SUCCESS - Indicates successful operation.
*
*       <other> - Indicates (other) internal error occurred.
*
*************************************************************************/
STATUS NU_PM_Restore_Hibernate_Device (DV_DEV_ID dev_id, INT ioctl_num)
{
    STATUS          status = NU_SUCCESS;
    DV_DEV_HANDLE   dev_handle_list[DV_MAX_DEV_SESSION_CNT];
    INT             dev_handle_cnt = DV_MAX_DEV_SESSION_CNT;
    UINT            i, j;
    BOOLEAN         flag; 

    /* Get list of all device handles for the device. */
    status = DVS_Dev_Handles_Get(dev_id, dev_handle_list, &dev_handle_cnt);

    /* Call device restore IOCTL for each device handle that is NOT a
       power device handle. */
    for (i=0; ((i < dev_handle_cnt) && (status == NU_SUCCESS)); i++)
    {
        flag = NU_FALSE;
        
        /* Search for the power device handle in the current device handle list */
        for (j=0; ((j < PM_MAX_DEV_CNT) && (PM_Dev_Info_Array[j] != NU_NULL)); j++)
        {
            /* If a handle was found, go to the next handle */
            if (PM_Dev_Info_Array[j] -> dev_handle == dev_handle_list[i])
            {
                j = PM_MAX_DEV_CNT;
                flag = NU_TRUE;
            }
        }
        
        /* Call device IOCTL for non-power device handles. */
        if (flag == NU_FALSE)
        {
            status = DVC_Dev_Ioctl(dev_handle_list[i], ioctl_num, NU_NULL, 0);
        }
    }

    return (status);
}

#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE) */


