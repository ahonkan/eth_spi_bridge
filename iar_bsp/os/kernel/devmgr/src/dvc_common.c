/*************************************************************************
*
*              Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILENAME
*
*       dvc_common.c
*
*   COMPONENT
*
*       DV - Device Manager
*
*   DESCRIPTION
*
*       This file contains the core common routines for the Device
*       Manager component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       DVC_Dev_Register                    Registers a device
*       DVC_Dev_Unregister                  Unregisters a device
*       DVC_Dev_ID_Get                      Get a list of device IDs
*       DVC_Dev_Labels_Get                  Get a list of device labels
*       DVC_Reg_Change_Search               Searches the registry for changes
*       DVC_Reg_Change_Notify               Registers notification call backs
*       DVC_Reg_Change_Check                Returns updated device list
*       DVC_Dev_ID_Open                     Opens a device by device ID
*       DVC_Dev_Open                        Opens a device by name
*       DVC_Dev_Close                       Closes a device
*       DVC_Dev_Read                        Sends a read command to a device
*       DVC_Dev_Write                       Sends a write command to a device
*       DVC_Dev_Ioctl                       Sends an IOCTL command to a device
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel functions
*       thread_control.h                    Thread Control functions
*       event_group.h                       Event Group functions
*       device_manager.h                    Device Manager functions
*
*************************************************************************/

#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/core/inc/thread_control.h"
#include        "os/kernel/plus/core/inc/event_group.h"
#include        "os/kernel/devmgr/inc/device_manager.h"

/* System memory pools */
extern          NU_MEMORY_POOL  System_Memory;

/* Define external inner-component global data references.  */

extern UNSIGNED         DVD_Initialized;
extern INT              DVD_Max_Dev_Id_Cnt;
extern DV_DEV_REGISTRY  *DVD_Dev_Registry;
extern INT              DVD_Reg_Active_Cnt;
extern NU_EVENT_GROUP   DVD_Reg_Change_Event;
extern INT32            DVD_Reg_Next_Dev_Index;
#if (DV_DEV_DISCOVERY_TASK_ENB == NU_TRUE)
extern DV_DEV_LISTENER *DVD_Dev_Reg_Listener_Array[DV_MAX_DEVICE_LISTENERS];

/* Semaphore for the protection of listener array. */
extern NU_SEMAPHORE     DVD_Dev_Reg_Listener_Semaphore;
#endif

/*************************************************************************
*
*   FUNCTION
*
*       DVC_Dev_Register
*
*   DESCRIPTION
*
*       This function registers a device
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       TCCT_Schedule_Lock
*       TCCT_Schedule_Unlock
*       NU_Set_Events
*
*   INPUTS
*
*       instance_handle                     Device specific handle
*       dev_label_list[]                    Device label list (attr and modes)
*       dev_label_cnt                       Number of device labels
*       *drv_functions_ptr                  Pointer to the driver functions
*       *dev_id_ptr                         Pointer to where we return the device id
*
*   OUTPUTS
*
*       status
*           DV_INVALID_INPUT_PARAMS
*           DV_NO_AVAILABLE_DEV_ID
*           NU_Set_Events returned error status
*
*************************************************************************/
STATUS DVC_Dev_Register (VOID* instance_handle,
                         DV_DEV_LABEL dev_label_list[],
                         INT dev_label_cnt,
                         DV_DRV_FUNCTIONS *drv_functions_ptr,
                         DV_DEV_ID *dev_id_ptr)
{
    INT     i;
    INT32   reg_reuse_cnt;
    STATUS  status = NU_SUCCESS;
    INT32   reg_index = DV_INVALID_DEV;
    INT     found = NU_FALSE;

    NU_SUPERV_USER_VARIABLES
    
    /* Check the input parameters */
    if (((dev_label_list == NU_NULL) && (dev_label_cnt > 0)) ||
        ((dev_label_cnt < 0) || (dev_label_cnt > DV_MAX_DEV_LABEL_CNT)) ||
        (drv_functions_ptr == NU_NULL) ||
        ((drv_functions_ptr->drv_open_ptr == NU_NULL) ||
         (drv_functions_ptr->drv_close_ptr == NU_NULL) ||
         (drv_functions_ptr->drv_ioctl_ptr == NU_NULL)))
    {
        /* Return invalid input parameters status */
        status = DV_INVALID_INPUT_PARAMS;
    }
    else
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* If the DM has not been initialized yet */
        while (DVD_Initialized != DV_INITIALIZED)
        {
            /* Sleep and then try again */
            NU_Sleep(2);
        }

        /* If the registry is full */
        if (DVD_Reg_Next_Dev_Index == DV_INVALID_DEV)
        {
            /* Return no available device id status */
            status = DV_NO_AVAILABLE_DEV_ID;
        }

        /* If everything is OK */
        if (status == NU_SUCCESS)
        {
            /* Protect against access to the registry.  */
            TCCT_Schedule_Lock();

            /* Use this device id */
            reg_index = DVD_Reg_Next_Dev_Index;

            /* Fill in the labels for this device */
            for (i = 0; i < dev_label_cnt; i++)
            {
                DVD_Dev_Registry[reg_index].label_list[i] = dev_label_list[i];
            }

            /* Remember how many labels this device has */
            DVD_Dev_Registry[reg_index].label_cnt = dev_label_cnt;

            /* Fill in the instance handle */
            DVD_Dev_Registry[reg_index].instance_handle = instance_handle;

            /* Fill in the driver open function pointer */
            DVD_Dev_Registry[reg_index].drv_open_ptr = drv_functions_ptr->drv_open_ptr;

            /* Fill in the driver close function pointer */
            DVD_Dev_Registry[reg_index].drv_close_ptr = drv_functions_ptr->drv_close_ptr;

            /* Fill in the driver read function pointer */
            DVD_Dev_Registry[reg_index].drv_read_ptr = drv_functions_ptr->drv_read_ptr;

            /* Fill in the driver write function pointer */
            DVD_Dev_Registry[reg_index].drv_write_ptr = drv_functions_ptr->drv_write_ptr;

            /* Fill in the driver ioctl function pointer */
            DVD_Dev_Registry[reg_index].drv_ioctl_ptr = drv_functions_ptr->drv_ioctl_ptr;

            /* Place the reuse count in another variable */
            reg_reuse_cnt = DVD_Dev_Registry[reg_index].reuse_cnt;

            /* If we have a place to return the device id */
            if (dev_id_ptr != NU_NULL)
            {
                /* Return the device id */
                *dev_id_ptr = DV_CREATE_DEV_ID(reg_reuse_cnt, reg_index);
            }

            /* Mark that this slot in the registry is being used */
            DVD_Dev_Registry[reg_index].entry_active_flag = NU_TRUE;

            /* Increment the registered device count */
            DVD_Reg_Active_Cnt++;

            /* Search for next valid device id. */
            for (i=0; ((i<DVD_Max_Dev_Id_Cnt) && (found==NU_FALSE)); i++)
            {
                /* Point to the next entry in the registry */
                DVD_Reg_Next_Dev_Index++;

                /* If we reached the end of the registry */
                if (DVD_Reg_Next_Dev_Index >= DVD_Max_Dev_Id_Cnt)
                {
                    /* Go to the top of the registry. */
                    DVD_Reg_Next_Dev_Index = (DV_DEV_ID)0;
                }

                /* If this device id is available */
                if (DVD_Dev_Registry[DVD_Reg_Next_Dev_Index].entry_active_flag == NU_FALSE)
                {
                    /* We found an available device id */
                    found = NU_TRUE;
                }
            }

            /* If we did not find an empty position */
            if (found == NU_FALSE)
            {
                /* Set next device id to invalid flag */
                DVD_Reg_Next_Dev_Index = DV_INVALID_DEV;
            }

            /* Release protection against access to the registry.  */
            TCCT_Schedule_Unlock();

            /* Create an event group to suspend on if there are active commands and device
             * manager is requested to close device.
             */
            status = NU_Create_Event_Group(
                        &DVD_Dev_Registry[reg_index].active_cmds_event,
                        "SESEVT");
            if ( status == NU_SUCCESS )
            {
                /* Set an event flag to show a device has been registered. */
                status = NU_Set_Events(&DVD_Reg_Change_Event, DV_DEV_REGISTERED_BIT, (OPTION)NU_OR);
            }
        }

        /* Return to user mode */
        NU_USER_MODE();
    }

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       DVC_Dev_Unregister
*
*   DESCRIPTION
*
*       This function removes a device from the registry
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       TCCT_Schedule_Lock
*       TCCT_Schedule_Unlock
*       NU_Set_Events
*
*   INPUTS
*
*       dev_id                              Device ID
*       *instance_handle_ptr                Pointer to where the instance
*                                           handle can be returned. If
*                                           NULL, then no instance handle
*                                           is returned.
*
*   OUTPUTS
*
*       status
*           NU_SUCCESS
*           DV_INVALID_INPUT_PARAMS
*           DV_DEV_NOT_REGISTERED
*           NU_Set_Events returned error status
*
*************************************************************************/
STATUS DVC_Dev_Unregister (DV_DEV_ID dev_id, VOID* *instance_handle_ptr)
{
    VOID*          session_handle;
    INT            i;
    INT32          reg_index;
    STATUS         status = NU_SUCCESS;
    INT32          session_index = 0;
    INT32          reuse_cnt;
    
    NU_SUPERV_USER_VARIABLES

    /* Get device registry index */
    reg_index = DV_GET_REG_INDEX(dev_id);

    /* Check the input parameters */
    if ((reg_index < (DV_DEV_ID)0) || (reg_index >= DVD_Max_Dev_Id_Cnt))
    {
        /* Invalid input parameters */
        status = DV_INVALID_INPUT_PARAMS;
    }
    else
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();
    
        /* Get device id reuse count */
        reuse_cnt = DV_GET_REUSE_CNT(dev_id);

        /* If the device was not registered */
        if ((DVD_Dev_Registry[reg_index].entry_active_flag != NU_TRUE) ||
            (reuse_cnt != DVD_Dev_Registry[reg_index].reuse_cnt))
        {
            /* Return device not registered status */
            status = DV_DEV_NOT_REGISTERED;
        }

        /* If everything is OK */
        if (status == NU_SUCCESS)
        {
            /* Protect against access to the registry.  */
            TCCT_Schedule_Lock();

            /* Search for open sessions */
            while (DVD_Dev_Registry[reg_index].active_open_cnt > 0)
            {
                /* If this session is open */
                if (DVD_Dev_Registry[reg_index].session[session_index].state_flag == DV_SES_OPEN)
                {
                    /* Mark that this slot in the session registry is locked */
                    DVD_Dev_Registry[reg_index].session[session_index].state_flag = DV_SES_LOCKED;

                    /* Get the session handle to pass to the driver */
                    session_handle = DVD_Dev_Registry[reg_index].session[session_index].handle;

                    /* Release protection against access to the registry.  */
                    TCCT_Schedule_Unlock();

                    /* Call the driver's close function */
                    status = (*DVD_Dev_Registry[reg_index].drv_close_ptr)(session_handle);

                    /* Protect against access to the registry.  */
                    TCCT_Schedule_Lock();

                    /* Clear the session handle */
                    DVD_Dev_Registry[reg_index].session[session_index].handle = NU_NULL;

                    /* Mark this entry as inactive */
                    DVD_Dev_Registry[reg_index].session[session_index].state_flag = DV_SES_CLOSED;

                    /* Decrement the device's open count */
                    DVD_Dev_Registry[reg_index].active_open_cnt--;

                    /* If there were no available session id's */
                    if (DVD_Dev_Registry[reg_index].next_ses_index == DV_INVALID_SES)
                    {
                        /* Set this as the next session device id */
                        DVD_Dev_Registry[reg_index].next_ses_index = session_index;
                    }

                    /* Reset the session index */
                    session_index = 0;
                }
                else
                {
                     /* Go to the next session entry */
                    session_index++;
                }
            }

            /* Decrement the registered device count */
            DVD_Reg_Active_Cnt--;

            /* Mark that this slot in the registry is available */
            DVD_Dev_Registry[reg_index].entry_active_flag = NU_FALSE;

            /* Increment the device id reuse count */
            DVD_Dev_Registry[reg_index].reuse_cnt++;

            /* Do we need to rollover? */
            if (DVD_Dev_Registry[reg_index].reuse_cnt > ((1<<DV_REUSE_BIT_CNT)-1))
            {
                /* Yes, reset reuse count to 1 */
                DVD_Dev_Registry[reg_index].reuse_cnt = 1;
            }

            /* If we have a place to return the instance handle */
            if (instance_handle_ptr != NU_NULL)
            {
                /* Return the instance handle */
                *instance_handle_ptr = DVD_Dev_Registry[reg_index].instance_handle;
            }

            /* Clear the labels for this device */
            for (i = 0; i < DVD_Dev_Registry[reg_index].label_cnt; i++)
            {
                DV_CLEAR_LABEL(&DVD_Dev_Registry[reg_index].label_list[i]);
            }

            /* Clear the driver labels count */
            DVD_Dev_Registry[reg_index].label_cnt = 0;

            /* Clear the instance handle */
            DVD_Dev_Registry[reg_index].instance_handle = NU_NULL;

            /* Clear the driver open function pointer */
            DVD_Dev_Registry[reg_index].drv_open_ptr = NU_NULL;

            /* Clear the driver close function pointer */
            DVD_Dev_Registry[reg_index].drv_close_ptr = NU_NULL;

            /* Clear the driver read function pointer */
            DVD_Dev_Registry[reg_index].drv_read_ptr = NU_NULL;

            /* Clear the driver write function pointer */
            DVD_Dev_Registry[reg_index].drv_write_ptr = NU_NULL;

            /* Clear the driver ioctl function pointer */
            DVD_Dev_Registry[reg_index].drv_ioctl_ptr = NU_NULL;

            /* Delete the event group */
            (VOID)NU_Delete_Event_Group(
                        &DVD_Dev_Registry[reg_index].active_cmds_event);

            /* If there were no available device id's */
            if (DVD_Reg_Next_Dev_Index == DV_INVALID_DEV)
            {
                /* Set this as the next available device id */
                DVD_Reg_Next_Dev_Index = reg_index;
            }

            /* Release protection against access to the registry.  */
            TCCT_Schedule_Unlock();

            /* Set an event flag to show a device has been unregistered. */
            status = NU_Set_Events(&DVD_Reg_Change_Event, DV_DEV_UNREGISTERED_BIT, (OPTION)NU_OR);
        }

        /* Return to user mode */
        NU_USER_MODE();
    }

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       DVC_Dev_ID_Get
*
*   DESCRIPTION
*
*       This function searches the device list for all device ID(s) that
*       satisfy the label list.
*
*   CALLED BY
*
*       Application
*       DVC_Reg_Change_Search
*       DVC_Dev_Open
*
*   CALLS
*
*       TCCT_Schedule_Lock
*       TCCT_Schedule_Unlock
*
*   INPUTS
*
*       dev_label_list[]                    Device label list
*       dev_label_cnt                       Number of device labels. If
*                                           0, then return all device IDs
*       dev_id_list[]                       Device ID list we return
*       *dev_id_cnt_ptr                     Pointer to variable that contains
*                                           the maximum IDs to return and
*                                           it is also the place where we
*                                           return the number of IDs found
*
*   OUTPUTS
*
*       status
*           NU_SUCCESS
*           DV_INVALID_INPUT_PARAMS
*           DV_DEV_LIST_TOO_SMALL
*
*************************************************************************/
STATUS DVC_Dev_ID_Get (DV_DEV_LABEL dev_label_list[], INT dev_label_cnt,
                       DV_DEV_ID dev_id_list[], INT *dev_id_cnt_ptr)
{
    INT    label_match_cnt, match, i, j;
    INT    active_dev_cnt = 0;
    INT32  reg_index = 0;
    INT    id_index = 0;
    STATUS status = NU_SUCCESS;

    NU_SUPERV_USER_VARIABLES

    /* Check the input parameters */
    if (((dev_label_list == NU_NULL) && (dev_label_cnt > 0)) ||
        ((dev_label_cnt < 0) || (dev_label_cnt > DV_MAX_DEV_LABEL_CNT)) ||
        (dev_id_list == NU_NULL) ||
        (dev_id_cnt_ptr == NU_NULL) ||
        (*dev_id_cnt_ptr <= 0))
    {
        /* Invalid input parameters */
        status = DV_INVALID_INPUT_PARAMS;
    }

    /* If everything is OK */
    if (status == NU_SUCCESS)
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* If the DM has not been initialized yet */
        while (DVD_Initialized != DV_INITIALIZED)
        {
            /* Sleep and then try again */
            NU_Sleep(2);
        }

        /* Protect against access to the registry.  */
        TCCT_Schedule_Lock();

        /* If there are registered devices */
        if (DVD_Reg_Active_Cnt > 0)
        {
            /* Search the device registry and return matching device IDs. */
            while ((status == NU_SUCCESS) && (active_dev_cnt < DVD_Reg_Active_Cnt))
            {
                /* If this entry is an active device */
                if (DVD_Dev_Registry[reg_index].entry_active_flag == NU_TRUE)
                {
                    /* Show we found an active device to check */
                    active_dev_cnt++;

                    /* Clear out the label match counter */
                    label_match_cnt = 0;

                    /* If we have labels to look for */
                    if (dev_label_cnt > 0)
                    {
                        /* Set match flag to true just to get into the next for loop */
                        match = NU_TRUE;

                        /* For each label passed in */
                        for (i = 0; ((i < dev_label_cnt) && (match == NU_TRUE)); i++)
                        {
                            /* Clear match flag */
                            match = NU_FALSE;

                            /* For each label associated with this device */
                            for (j = 0; ((j < DVD_Dev_Registry[reg_index].label_cnt) &&
                                        (match == NU_FALSE)); j++)
                            {
                                /* If the device has a matching label */
                                if (DV_COMPARE_LABELS(&dev_label_list[i],
                                                &(DVD_Dev_Registry[reg_index].label_list[j])))
                                {
                                    /* Set the match flag */
                                    match = NU_TRUE;

                                    /* Increment the label match counter */
                                    label_match_cnt++;
                                }
                            }
                        }
                    }

                    /* If the device matches all the required labels */
                    if (label_match_cnt == dev_label_cnt)
                    {
                        /* If we have room in the returned device ID list */
                        if (id_index < *dev_id_cnt_ptr)
                        {
                            /* Put this device ID on the returned device ID list */
                            dev_id_list[id_index] = DV_CREATE_DEV_ID(DVD_Dev_Registry[reg_index].reuse_cnt, reg_index);

                            /* Increment the id index */
                            id_index++;
                        }
                        else
                        {
                            /* Not enough room in return list */
                            status = DV_DEV_LIST_TOO_SMALL;
                        }
                    }
                }

                /* Go to the next registry entry */
                reg_index++;
            }
        }

        /* Update what list_cnt points to with the number of IDs found */
        *dev_id_cnt_ptr = id_index;

        /* Release protection against access to the registry.  */
        TCCT_Schedule_Unlock();

        /* Return to user mode */
        NU_USER_MODE();
    }

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       DVC_Dev_Labels_Get
*
*   DESCRIPTION
*
*       This function returns all the labels for the device.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       TCCT_Schedule_Lock
*       TCCT_Schedule_Unlock
*
*   INPUTS
*
*       dev_id                              Device ID
*       dev_label_list[]                    Device label list we return
*       *dev_label_cnt_ptr                  Pointer to variable that contains
*                                           the maximum labels to return and
*                                           it is also the place where we
*                                           return the number of labels found
*
*   OUTPUTS
*
*       status
*           NU_SUCCESS
*           DV_INVALID_INPUT_PARAMS
*           DV_DEV_NOT_REGISTERED
*           DV_LABEL_LIST_TOO_SMALL
*
*************************************************************************/
STATUS DVC_Dev_Labels_Get (DV_DEV_ID dev_id, DV_DEV_LABEL dev_label_list[],
                           INT *dev_label_cnt_ptr)
{
    INT32        reg_index;
    STATUS       status = NU_SUCCESS;
    INT          label_index = 0;
    INT32        reuse_cnt;

    NU_SUPERV_USER_VARIABLES

    /* Get device registry index */
    reg_index = DV_GET_REG_INDEX(dev_id);

    /* Check the input parameters */
    if (((reg_index < (DV_DEV_ID)0) || (reg_index >= DVD_Max_Dev_Id_Cnt)) ||
        (dev_label_list == NU_NULL) ||
        (dev_label_cnt_ptr == NU_NULL) ||
        (*dev_label_cnt_ptr <= 0))
    {
        /* Invalid input parameters */
        status = DV_INVALID_INPUT_PARAMS;
    }
    else
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Get device id reuse count */
        reuse_cnt = DV_GET_REUSE_CNT(dev_id);

        /* If the device was not registered */
        if ((DVD_Dev_Registry[reg_index].entry_active_flag != NU_TRUE) ||
            (reuse_cnt != DVD_Dev_Registry[reg_index].reuse_cnt))
        {
            /* Return device not registered status */
            status = DV_DEV_NOT_REGISTERED;
        }

        /* If everything is OK */
        if (status == NU_SUCCESS)
        {
            /* Protect against access to the registry.  */
            TCCT_Schedule_Lock();

            /* Return the device's label list. */
            while ((label_index < DVD_Dev_Registry[reg_index].label_cnt) && (status == NU_SUCCESS))
            {
                /* If we have room in the returned labels list */
                if (label_index < *dev_label_cnt_ptr)
                {
                    /* Put this device label on the returned labels list */
                    dev_label_list[label_index] = DVD_Dev_Registry[reg_index].label_list[label_index];

                    /* Increment the label index */
                    label_index++;
                }
                else
                {
                    /* Not enough room in return list */
                    status = DV_LABEL_LIST_TOO_SMALL;
                }
            }

            /* Update what list cnt points to with the number of labels found */
            *dev_label_cnt_ptr = label_index;

            /* Release protection against access to the registry.  */
            TCCT_Schedule_Unlock();
        }

        /* Return to user mode */
        NU_USER_MODE();
    }

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       DVC_Reg_Change_Search
*
*   DESCRIPTION
*
*       This function searches the device registry and returns any changes
*       in registered devices compared to the passed in device list.
*
*   CALLED BY
*
*       DVC_Reg_Change_Check
*
*   CALLS
*
*       DVC_Dev_ID_Get
*
*   INPUTS
*
*       *app_struct_ptr                     Pointer to a structure that contains
*                                           several addresses where this function
*                                           will return it's results
*
*   OUTPUTS
*
*       status
*           NU_SUCCESS
*           DV_INVALID_INPUT_PARAMSU
*           DV_INVALID_REG_PARAMS
*           DV_INVALID_UNREG_PARAMS
*           DV_REG_LIST_TOO_SMALL
*           DV_UNREG_LIST_TOO_SMALL
*           DV_DEV_LIST_TOO_SMALL
*           DVC_Dev_ID_Get returned status
*
*************************************************************************/
STATUS DVC_Reg_Change_Search(DV_APP_REGISTRY_CHANGE *app_struct_ptr)
{
    DV_DEV_LABEL  *label_list_ptr;
    DV_DEV_ID     new_id_list[DV_DISCOVERY_TASK_MAX_ID_CNT];
    DV_DEV_ID     *known_id_list_ptr, *reg_id_list_ptr, *unreg_id_list_ptr;
    INT           new_id_cnt, label_cnt, max_id_cnt;
    INT           *known_id_cnt_ptr, *reg_id_cnt_ptr, *unreg_id_cnt_ptr;
    INT           match, i, j;
    INT           registry_changes = 0;
    STATUS        status = NU_SUCCESS;

    /* Check main input parameter */
    if (app_struct_ptr != NU_NULL)
    {
        /* Initialize local label list and count pointers */
        label_list_ptr = app_struct_ptr->dev_label_list_ptr;
        label_cnt = app_struct_ptr->dev_label_cnt;

        /* Initialize local device list and count pointers */
        known_id_list_ptr = app_struct_ptr->known_id_list_ptr;
        known_id_cnt_ptr = app_struct_ptr->known_id_cnt_ptr;

        /* Initialize local max number of returned id's count */
        max_id_cnt = app_struct_ptr->max_id_cnt;

        /* Clear out the registry changes return count */
        app_struct_ptr->registry_changes = 0;

        /* Initialize local registered device list and count pointers */
        reg_id_list_ptr = app_struct_ptr->reg_id_list_ptr;
        reg_id_cnt_ptr = app_struct_ptr->reg_id_cnt_ptr;

        /* If we have a place to return registered device count */
        if (reg_id_cnt_ptr != NU_NULL)
        {
            /* Clear out the registered return count */
            *reg_id_cnt_ptr = 0;
        }

        /* Initialize local unregistered device list and count pointers */
        unreg_id_list_ptr = app_struct_ptr->unreg_id_list_ptr;
        unreg_id_cnt_ptr = app_struct_ptr->unreg_id_cnt_ptr;

        /* If we have a place to return unregistered device count */
        if (unreg_id_cnt_ptr != NU_NULL)
        {
            /* Clear out the unregistered return count */
            *unreg_id_cnt_ptr = 0;
        }

        /* Check the device specific structure parameters */
        if (((max_id_cnt <= 0) || (max_id_cnt > DV_DISCOVERY_TASK_MAX_ID_CNT)) ||
             (known_id_list_ptr == NU_NULL) ||
             (known_id_cnt_ptr == NU_NULL))
        {
            /* Invalid input parameters */
            status = DV_INVALID_INPUT_PARAMS;
        }
        else
        {
            /* Check the device registered specific structure parameters */
            if (((reg_id_list_ptr == NU_NULL) && (reg_id_cnt_ptr != NU_NULL)) ||
                ((reg_id_list_ptr != NU_NULL) && (reg_id_cnt_ptr == NU_NULL)))
            {
                /* Invalid input parameters */
                status = DV_INVALID_REG_PARAMS;
            }
            else
            {
                /* Check the device unregistered specific structure parameters */
                if (((unreg_id_list_ptr == NU_NULL) && (unreg_id_cnt_ptr != NU_NULL)) ||
                    ((unreg_id_list_ptr != NU_NULL) && (unreg_id_cnt_ptr == NU_NULL)))
                {
                    /* Invalid input parameters */
                    status = DV_INVALID_UNREG_PARAMS;
                }
            }
        }
        
        /* If everything is OK */
        if (status == NU_SUCCESS)
        {
            /* Get maximum number of id's we can return */
            new_id_cnt = max_id_cnt;

            /* Get a list of device IDs */
            status = DVC_Dev_ID_Get (label_list_ptr, label_cnt, new_id_list, &new_id_cnt);

            /* If we successfully retrieved a set of device id's */
            if (status == NU_SUCCESS)
            {
                /* See if there are any devices that were not in our original list */
                for (i = 0; i < new_id_cnt; i++)
                {
                    /* Clear match found flag for the inner loop */
                    match = NU_FALSE;

                    /* Compare each device id in the original list */
                    for (j = 0; ((j < *known_id_cnt_ptr) && ( match == NU_FALSE)); j++)
                    {
                        /* If we found a match */
                        if (new_id_list[i] == known_id_list_ptr[j])
                        {
                            /* Set the match found flag */
                            match = NU_TRUE;
                        }
                    }

                    /* If this is a new device that we found */
                    if (match == NU_FALSE)
                    {
                        /* Count how many registry changes we found */
                        registry_changes++;

                        /* If we have a place to return registered device ids */
                        if ((reg_id_list_ptr != NU_NULL) && (reg_id_cnt_ptr != NU_NULL))
                        {
                            /* If we have room in the returned device ID list */
                            if (*reg_id_cnt_ptr < max_id_cnt)
                            {
                                /* Add this device id to the registered list */
                                reg_id_list_ptr[*reg_id_cnt_ptr] = new_id_list[i];

                                /* Increment the number of registered devices we found */
                                (*reg_id_cnt_ptr)++;
                            }
                            else
                            {
                                /* Not enough room in return list */
                                status = DV_REG_LIST_TOO_SMALL;
                            }
                        }
                    }
                }

                /* See if there are any devices in our original list that were removed */
                for (i = 0; i < *known_id_cnt_ptr; i++)
                {
                    /* Clear match found flag for the inner loop */
                    match = NU_FALSE;

                    /* Compare each device id from the new list */
                    for (j = 0; ((j < new_id_cnt) && (match == NU_FALSE)); j++)
                    {
                        /* If we found a match */
                        if (known_id_list_ptr[i] == new_id_list[j])
                        {
                            /* Set the match found flag */
                            match = NU_TRUE;
                        }
                    }

                    /* If this is an old device that was removed */
                    if (match == NU_FALSE)
                    {
                        /* Count how many registry changes we found */
                        registry_changes++;

                        /* If we have a place to return unregistered device ids */
                        if ((unreg_id_list_ptr != NU_NULL) && (unreg_id_cnt_ptr != NU_NULL))
                        {
                            /* If we have room in the returned device ID list */
                            if (*unreg_id_cnt_ptr < max_id_cnt)
                            {
                                /* Add this device id to the unregistered list */
                                unreg_id_list_ptr[*unreg_id_cnt_ptr] = known_id_list_ptr[i];

                                /* Increment the number of unregistered devices we found */
                                (*unreg_id_cnt_ptr)++;
                            }
                            else
                            {
                                /* Not enough room in return list */
                                status = DV_UNREG_LIST_TOO_SMALL;
                            }
                        }
                    }
                }

                /* If we found a change */
                if (registry_changes > 0)
                {
                    /* Clear the dev id count from the original list */
                    *known_id_cnt_ptr =  0;

                    /* Copy the new device id list to the return list */
                    for (i= 0; i < new_id_cnt; i++)
                    {
                        if (*known_id_cnt_ptr < max_id_cnt)
                        {
                            /* Add this device id to the returned device id list */
                            known_id_list_ptr[*known_id_cnt_ptr] = new_id_list[i];

                            /* Increment the total number of devices we found */
                            (*known_id_cnt_ptr)++;
                        }
                        else
                        {
                            /* Not enough room in return list */
                            status = DV_DEV_LIST_TOO_SMALL;
                        }
                    }

                    /* Set the structure's number of registry changes field */
                    app_struct_ptr->registry_changes = registry_changes;
                }
            }
        }
    }
    else
    {
        /* Invalid input parameter */
        status = DV_INVALID_INPUT_PARAMS;
    }

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       DVC_Reg_Change_Notify
*
*   DESCRIPTION
*
*       This function saves callback routines for a particular device label list.
*       Call back functions are for device registration and unregistration.
*       Once a change occurs in registry for given device label list, respective
*       call back will be invoked to let user know about the change.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       NU_Obtain_Semaphore
*       NU_Release_Semaphore
*       NU_Allocate_Memory
*       DVC_Reg_Change_Search
*       NU_Set_Events
*
*   INPUTS
*
*       dev_label_list                      List of device labels against which
*                                           callback is required.
*       dev_label_cnt                       Total number of device labels.
*       register_cb                         Callback for device registration.
*       unregister_cb                       Callback for device unregistration.
*       context                             Pointer to any user defined data which
*                                           is required to be shared when callback
*                                           is invoked for the device with given
*                                           label list.
*       listener_id                         Listener ID for this device,
*                                           to be returned by this function.
*
*   OUTPUTS
*
*       status
*           NU_SUCCESS                      Listener for given device label is
*                                           registered successfully.
*           MAX_LISTENER_LIMIT_EXCEEDED     No more space to save this listener.
*           NU_NO_MEMORY                    No more memory for listener structure.
* 
*************************************************************************/
STATUS DVC_Reg_Change_Notify(DV_DEV_LABEL             dev_label_list[],
                             INT                      dev_label_cnt,
                             DEV_REGISTER_CALLBACK    register_cb,
                             DEV_UNREGISTER_CALLBACK  unregister_cb,
                             VOID                    *context,
                             DV_LISTENER_HANDLE      *listener_id)
{
    STATUS status = NU_SUCCESS;
    UINT8 index;
#if (DV_DEV_DISCOVERY_TASK_ENB == NU_TRUE)
    UINT8 label_no;
    DV_DEV_LISTENER *listener_ptr;

    NU_SUPERV_USER_VARIABLES
#else
    INT            dev_reg_cnt = DV_DISCOVERY_TASK_MAX_ID_CNT;
    DV_DEV_ID      dev_reg_list[DV_DISCOVERY_TASK_MAX_ID_CNT];
#endif /* DV_DEV_DISCOVERY_TASK_ENB == NU_TRUE */

    /* Validate input parameters. */
    if((listener_id == NU_NULL) ||
       ((register_cb == NU_NULL) && (unregister_cb == NU_NULL)) ||
       ((dev_label_list == NU_NULL) && (dev_label_cnt > 0)) ||
       ((dev_label_cnt < 0) || (dev_label_cnt > DV_MAX_DEV_LABEL_CNT)))
    {
        /* Invalid input parameter. */
        status = DV_INVALID_INPUT_PARAMS;
    }

    /* If device discovery task is enabled then just save this listener and
     * activate the task by setting the scan device registry event.
     */
#if (DV_DEV_DISCOVERY_TASK_ENB == NU_TRUE)

    /* Switch to supervisor mode as this is a public API and can be used by the application. */
    NU_SUPERVISOR_MODE();

    if(status == NU_SUCCESS)
    {
        /* Lock against multiple thread access. */
        status = NU_Obtain_Semaphore(&DVD_Dev_Reg_Listener_Semaphore, NU_SUSPEND);
        if(status == NU_SUCCESS)
        {
            /* Initially, assume that we have reached to maximum listener limit. */
            status = DV_MAX_LISTENER_LIMIT_EXCEEDED;

            /* Now, see if we have some space for this new listener. */
            for(index = 0; index < DV_MAX_DEVICE_LISTENERS; index++)
            {
                if(DVD_Dev_Reg_Listener_Array[index] == NU_NULL)
                {
                    /* We have found one empty slot, now allocate memory for device listener object. */
                    status = NU_Allocate_Memory(&System_Memory, (VOID **)&listener_ptr,
                                                sizeof(DV_DEV_LISTENER), NU_NO_SUSPEND);
                    if (status == NU_SUCCESS)
                    {
                        /* Clear out newly allocated device listener structure. */
                        memset(listener_ptr, NU_NULL, sizeof(DV_DEV_LISTENER));

                        /* Fill the device listener structure. */
                        for(label_no = 0; label_no < dev_label_cnt; label_no++)
                        {
                            listener_ptr->dev_label_list[label_no] = dev_label_list[label_no];
                        }
                        listener_ptr->dev_label_cnt = dev_label_cnt;
                        listener_ptr->device_register_callback = register_cb;
                        listener_ptr->device_unregister_callback = unregister_cb;
                        listener_ptr->known_id_cnt = 0;
                        listener_ptr->context = context;

                        /* Save this listener in global array. */
                        DVD_Dev_Reg_Listener_Array[index] = listener_ptr;

                        /* Set index equal to listener id. */
                        *listener_id = index;

                        /* Everything is OK. */
                        status = NU_SUCCESS;
                    }

                    break;
                }
            }

            /* No more protection is required, release lock. */
            NU_Release_Semaphore(&DVD_Dev_Reg_Listener_Semaphore);
        }
    }

    /* If the new listener is successfully saved then re-scan the device registry to see
     * if a device with the same label has been registered before.
     */
    if(status == NU_SUCCESS)
    {
        /* Set an event flag to scan device registry as a new device listener is
         * added to the list. In case a previous device is found with the same label,
         * call back will be invoked immediately by the device discovery task.
         */
        status = NU_Set_Events(&DVD_Reg_Change_Event, DV_DEV_SCAN_REGISTRY_BIT, (OPTION)NU_OR);
    }

    /* Return to user mode. */
    NU_USER_MODE();

#else /* If device discovery task is not present then check registry for this device label right now. */

    /* No need to switch to supervisor mode as no kernel space data structure is being accessed here. */

    if(status == NU_SUCCESS)
    {
        /* Clear local data contents. */
        memset(dev_reg_list, 0x00, sizeof(dev_reg_list));

        /* Get a list of device IDs matching to current device label list. */
        status = DVC_Dev_ID_Get (dev_label_list, dev_label_cnt, dev_reg_list, &dev_reg_cnt);
        if(status == NU_SUCCESS)
        {
            /* See if we found any device. */
            if(dev_reg_cnt == 0)
            {
                status = DV_DEV_NOT_REGISTERED;
            }
            else
            {
                /* Loop through the devices with given label. */
                for(index = 0; index < dev_reg_cnt; index++)
                {
                    if(register_cb != NU_NULL)
                    {
                        status = register_cb(dev_reg_list[index], context);
                    }
                }
            }
        }

        /* Set listner_id to zero always as this has no significance in this case. */
        *listener_id = 0;
    }

#endif /* DV_DEV_DISCOVERY_TASK_ENB == NU_TRUE */

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       DVC_Reg_Change_Check
*
*   DESCRIPTION
*
*       This function is passed in a list of labels that define device attributes that
*       it should monitor.  It is also passed in a list of already known device ids
*       with these device attributes.  The function then checks the current registry
*       for devices with these attributes and if any new ones have been registered or
*       old ones unregistered it will return an updated list of device ids.  If there
*       have been no changes, then this function suspends until changes are detected
*       at which point an updated list of device ids will be returned.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       DVC_Reg_Change_Search
*       NU_Retrieve_Events
*
*   INPUTS
*
*       *app_struct_ptr                     Pointer to a structure that contains
*                                           several addresses where this function
*                                           will return it's results
*       suspend                             Determines whether to suspend until
*                                           a registry change occurs. Or it can be
*                                           a timeout value in system ticks to wait.
*
*   OUTPUTS
*
*       status
*           NU_SUCCESS
*           DVC_Reg_Change_Search returned error status
*           NU_Retrieve_Events returned error status
*
*************************************************************************/
STATUS DVC_Reg_Change_Check (DV_APP_REGISTRY_CHANGE *app_struct_ptr, UNSIGNED suspend)
{
    UNSIGNED            event_group;
    STATUS              status = NU_SUCCESS;

    /* Get current state of the devices we are looking for */
    status = DVC_Reg_Change_Search(app_struct_ptr);

    /* If we want to wait */
    if (suspend != NU_NO_SUSPEND)
    {
        /* If we did not find the devices we were looking for */
        while ((status == NU_SUCCESS) && (app_struct_ptr->registry_changes == 0))
        {

            /* Wait for a device registry change. */
            status = NU_Retrieve_Events(&DVD_Reg_Change_Event, DV_REG_CHANGE_BIT_MASK,
                                        (OPTION)NU_OR_CONSUME, &event_group, suspend);

            /* If the device registry changed */
            if (status == NU_SUCCESS)
            {
                /* Get current state of the devices we are looking for */
                status = DVC_Reg_Change_Search(app_struct_ptr);
            }
        }
    }

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       DVC_Dev_ID_Open
*
*   DESCRIPTION
*
*       This function opens a session of a device by device ID.
*
*   CALLED BY
*
*       Application
*       DVC_Dev_Open
*
*   CALLS
*
*       TCCT_Schedule_Lock
*       TCCT_Schedule_Unlock
*       Low Level Driver Open
*
*   INPUTS
*
*       dev_id                              Device ID
*       dev_label_list[]                    Device label list
*       dev_label_cnt                       Number of device labels
*       *dev_handle_ptr                     Pointer to where we return the device handle
*
*   OUTPUTS
*
*       status
*           NU_SUCCESS
*           DV_INVALID_INPUT_PARAMS
*           DV_DEV_NOT_REGISTERED
*           DV_NO_AVAILABLE_SESSION
*           Driver's OPEN function returned error status
*
*************************************************************************/
STATUS DVC_Dev_ID_Open (DV_DEV_ID dev_id, DV_DEV_LABEL dev_label_list[],
                        INT dev_label_cnt, DV_DEV_HANDLE *dev_handle_ptr)
{
    INT32          session_index, next_ses_index;
    VOID*          session_handle;
    INT            found = NU_FALSE;
    INT            i;
    INT32          reg_index;
    STATUS         status = NU_SUCCESS;
    INT32          reuse_cnt;

    NU_SUPERV_USER_VARIABLES

    /* Get device registry index */
    reg_index = DV_GET_REG_INDEX(dev_id);

    /* Check the input parameters */
    if (((reg_index < (DV_DEV_ID)0) || (reg_index >= DVD_Max_Dev_Id_Cnt)) ||
        ((dev_label_list == NU_NULL) && (dev_label_cnt > 0)) ||
        ((dev_label_cnt < 0) || (dev_label_cnt > DV_MAX_DEV_LABEL_CNT)) ||
        (dev_handle_ptr == NU_NULL))
    {
        /* Return invalid input parameters status */
        status = DV_INVALID_INPUT_PARAMS;
    }
    else
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Get device id reuse count */
        reuse_cnt = DV_GET_REUSE_CNT(dev_id);

        /* If the device was not registered */
        if ((DVD_Dev_Registry[reg_index].entry_active_flag != NU_TRUE) ||
            (reuse_cnt != DVD_Dev_Registry[reg_index].reuse_cnt))
        {
            /* Return device not registered status */
            status = DV_DEV_NOT_REGISTERED;
        }
        else
        {
            /* If the session registry is full */
            if (DVD_Dev_Registry[reg_index].next_ses_index == DV_INVALID_SES)
            {
                /* Return no available session status */
                status = DV_NO_AVAILABLE_SESSION;
            }
        }

        /* If everything is OK */
        if (status == NU_SUCCESS)
        {
            /* Protect against access to the registry.  */
            TCCT_Schedule_Lock();

            /* Use this as the current session id */
            session_index = DVD_Dev_Registry[reg_index].next_ses_index;

            /* Mark that this slot in the session registry is locked */
            DVD_Dev_Registry[reg_index].session[session_index].state_flag = DV_SES_LOCKED;

            /* Initialize the next session id to the current session id */
            next_ses_index = session_index;

            /* Search for next valid session id. */
            for (i=0; ((i < DV_MAX_DEV_SESSION_CNT) && (found==NU_FALSE)); i++)
            {
                /* Point to the next entry in the session registry */
                next_ses_index++;

                /* If we reached the end of the session registry */
                if (next_ses_index >= DV_MAX_DEV_SESSION_CNT)
                {
                    /* Go to the top of the session registry. */
                    next_ses_index = 0;
                }

                /* If this session id is available */
                if (DVD_Dev_Registry[reg_index].session[next_ses_index].state_flag == DV_SES_CLOSED)
                {
                    /* Save this available session id */
                    DVD_Dev_Registry[reg_index].next_ses_index = next_ses_index;

                    /* We found an available session id */
                    found = NU_TRUE;
                }
            }

            /* If we did not find an empty position */
            if (found == NU_FALSE)
            {
                /* Set next session id to invalid flag */
                DVD_Dev_Registry[reg_index].next_ses_index = DV_INVALID_SES;
            }

            /* Release protection against access to the registry.  */
            TCCT_Schedule_Unlock();

            /* Call the driver's open function and get a session handle */
            status = (*DVD_Dev_Registry[reg_index].drv_open_ptr)
                      (DVD_Dev_Registry[reg_index].instance_handle,
                       dev_label_list, dev_label_cnt, &session_handle);

            /* Protect against access to the registry.  */
            TCCT_Schedule_Lock();

            /* If the driver did open a session */
            if (status == NU_SUCCESS)
            {

                /* Create and return a device_handle */
                *dev_handle_ptr = DV_CREATE_DEV_HANDLE(dev_id, session_index);

                /* Associate the session handle with this device_handle */
                DVD_Dev_Registry[reg_index].session[session_index].handle = session_handle;

                /* Mark that this slot in the session registry is being used */
                DVD_Dev_Registry[reg_index].session[session_index].state_flag = DV_SES_OPEN;

                /* Increment the device's open count */
                DVD_Dev_Registry[reg_index].active_open_cnt++;
            }
            else
            {
               /* Mark that this slot in the session registry is available */
                DVD_Dev_Registry[reg_index].session[session_index].state_flag = DV_SES_CLOSED;

                /* Reset the next session id */
                DVD_Dev_Registry[reg_index].next_ses_index = session_index;
            }

            /* Release protection against access to the registry.  */
            TCCT_Schedule_Unlock();
        }

        /* Return to user mode */
        NU_USER_MODE();
    }

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       DVC_Dev_Open
*
*   DESCRIPTION
*
*       This function opens a session of a device by device name.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       DVC_Dev_ID_Get
*       DVC_Dev_ID_Open
*
*   INPUTS
*
*       dev_name_ptr                        Device label name
*       *dev_handle_ptr                     Pointer to where we return the device handle
*
*   OUTPUTS
*
*       status
*           NU_SUCCESS
*           DV_DEV_NOT_REGISTERED
*           Driver's OPEN function returned error status
*           DVC_Dev_ID_Get returned error status
*
*************************************************************************/
STATUS DVC_Dev_Open (DV_DEV_LABEL *dev_name_ptr, DV_DEV_HANDLE *dev_handle_ptr)
{
    STATUS          status;
    DV_DEV_ID       dev_id_array[DV_DISCOVERY_TASK_MAX_ID_CNT];
    INT             dev_cnt = DV_DISCOVERY_TASK_MAX_ID_CNT;
    INT             i;

    /* Attempt to get the device id for the label passed in */
    status = DVC_Dev_ID_Get (dev_name_ptr, 1, dev_id_array, &dev_cnt);

    /* If there was no problem looking for device ids */
    if (status == NU_SUCCESS)
    {
        /* If we did not find any device ids */
        if (dev_cnt == 0)
        {
            /* The device requested is not registered */
            status = DV_DEV_NOT_REGISTERED;
        }
        else
        {
            /* Initialize status to make it into the for loop */
            status = DV_NO_AVAILABLE_SESSION;

            /* Go through the list of device ids */
            for (i = 0; ((i < dev_cnt) && (status != NU_SUCCESS)) ; i++)
            {
                /* Try and open a device */
                status = DVC_Dev_ID_Open(dev_id_array[i], dev_name_ptr, 1, dev_handle_ptr);
            }
        }
    }

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       DVC_Dev_Close
*
*   DESCRIPTION
*
*       This function closes a device.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       TCCT_Schedule_Lock
*       TCCT_Schedule_Unlock
*       Low Level Driver Close
*
*   INPUTS
*
*       dev_handle                          Device ID
*
*   OUTPUTS
*
*       status
*           NU_SUCCESS
*           DV_INVALID_INPUT_PARAMS
*           DV_DEV_NOT_REGISTERED
*           DV_SESSION_NOT_OPEN
*           Driver's CLOSE function returned error status
*
*************************************************************************/
STATUS DVC_Dev_Close (DV_DEV_HANDLE dev_handle)
{
    DV_DEV_ID       dev_id;
    VOID*           session_handle;
    UNSIGNED        event_group;
    INT32           reg_index, session_index;
    STATUS          status = NU_SUCCESS;
    INT32           reuse_cnt;

    NU_SUPERV_USER_VARIABLES

    /* Get the device ID */
    dev_id = DV_GET_DEV_ID(dev_handle);

    /* Get the session that we are closing */
    session_index = DV_GET_SES_INDEX(dev_handle);

    /* Get device registry index */
    reg_index = DV_GET_REG_INDEX(dev_id);

    /* Check the input parameters */
    if (((reg_index < (DV_DEV_ID)0) || (reg_index >= DVD_Max_Dev_Id_Cnt)) ||
        ((session_index < 0) || (session_index >= DV_MAX_DEV_SESSION_CNT)))
    {
        /* Invalid input parameters */
        status = DV_INVALID_INPUT_PARAMS;
    }
    else
    {
        /* Switch to Supervisor mode */
        NU_SUPERVISOR_MODE();

        /* Get device id reuse count */
        reuse_cnt = DV_GET_REUSE_CNT(dev_id);

        /* If the device was not registered */
        if ((DVD_Dev_Registry[reg_index].entry_active_flag != NU_TRUE) ||
            (reuse_cnt != DVD_Dev_Registry[reg_index].reuse_cnt))
        {
            /* Can't send a command to a device that is not registered */
            status = DV_DEV_NOT_REGISTERED;
        }
        else
        {
            /* If the session is not open */
            if (DVD_Dev_Registry[reg_index].session[session_index].state_flag != DV_SES_OPEN)
            {
                /* Can't send a command to a device that is not open */
                status = DV_SESSION_NOT_OPEN;
            }
        }
        
        /* If everything is OK */
        if (status == NU_SUCCESS)
        {
            /* Protect against access to the registry.  */
            TCCT_Schedule_Lock();

            /* Mark that this slot in the session registry is locked */
            DVD_Dev_Registry[reg_index].session[session_index].state_flag = DV_SES_LOCKED;

            /* Get the session handle to pass to the driver */
            session_handle = DVD_Dev_Registry[reg_index].session[session_index].handle;

            /* Release protection against access to the registry.  */
            TCCT_Schedule_Unlock();

            /* If we have ongoing activity in this session */
            if (DVD_Dev_Registry[reg_index].session[session_index].active_cmds_cnt != 0)
            {
                /* Wait for all ongoing activity to complete */
                (VOID)NU_Retrieve_Events(
                            &DVD_Dev_Registry[reg_index].active_cmds_event,
                            (1 << session_index), (OPTION)NU_AND_CONSUME, &event_group,
                            (UNSIGNED)NU_SUSPEND);
            }

            /* Call the driver's close function */
            status = (*DVD_Dev_Registry[reg_index].drv_close_ptr)(session_handle);

            /* Protect against access to the registry.  */
            TCCT_Schedule_Lock();

            /* Clear the session handle */
            DVD_Dev_Registry[reg_index].session[session_index].handle = NU_NULL;

            /* Mark this entry as inactive */
            DVD_Dev_Registry[reg_index].session[session_index].state_flag = DV_SES_CLOSED;

            /* Decrement the device's open count */
            DVD_Dev_Registry[reg_index].active_open_cnt--;

            /* If there were no available session id's */
            if (DVD_Dev_Registry[reg_index].next_ses_index == DV_INVALID_SES)
            {
                /* Set this as the next session device id */
                DVD_Dev_Registry[reg_index].next_ses_index = session_index;
            }

            /* Release protection against access to the registry.  */
            TCCT_Schedule_Unlock();
        }

        /* Return to user mode */
        NU_USER_MODE();
    }

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       DVC_Dev_Read
*
*   DESCRIPTION
*
*       This function interfaces to the actual device's read function.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       Low Level Driver Read
*
*   INPUTS
*
*       dev_handle                          Device handle
*       *buffer_ptr                         Memory address to start writing from
*       numbyte                             The size in bytes of the buffer
*       byte_offset                         Number of bytes to offset from 0
*       *bytes_read_ptr                     Pointer to where we return the number of
*                                           bytes read
*
*   OUTPUTS
*
*       status
*           NU_SUCCESS
*           DV_INVALID_INPUT_PARAMS
*           DV_DEV_NOT_REGISTERED
*           DV_SESSION_NOT_OPEN
*           Driver's READ function returned error status
*
*************************************************************************/
STATUS DVC_Dev_Read (DV_DEV_HANDLE dev_handle, VOID *buffer_ptr, UINT32 numbyte,
                     OFFSET_T byte_offset, UINT32 *bytes_read_ptr)
{
    DV_DEV_ID               dev_id;
    VOID                    *session_handle;
    UINT32                  bytes_r=0;
    INT32                   reg_index, session_index;
    STATUS                  status = NU_SUCCESS;
#if (DV_ERR_CHECK_ENABLE == NU_TRUE)
    INT32                   reuse_cnt;
#endif

    NU_SUPERV_USER_VARIABLES

    /* Get the device ID */
    dev_id = DV_GET_DEV_ID(dev_handle);

    /* Get the session that we are using */
    session_index = DV_GET_SES_INDEX(dev_handle);

    /* Get device registry index */
    reg_index = DV_GET_REG_INDEX(dev_id);

#if (DV_ERR_CHECK_ENABLE == NU_TRUE)
    /* Check the input parameters */
    if (((reg_index < (DV_DEV_ID)0) || (reg_index >= DVD_Max_Dev_Id_Cnt)) ||
        ((session_index < 0) || (session_index >= DV_MAX_DEV_SESSION_CNT)) ||
        (buffer_ptr == NU_NULL))
    {
        /* Invalid input parameters */
        status = DV_INVALID_INPUT_PARAMS;
    }
    else
#endif
    {
        /* Switch to Supervisor mode */
        NU_SUPERVISOR_MODE();

#if (DV_ERR_CHECK_ENABLE == NU_TRUE)

        /* Get device id reuse count */
        reuse_cnt = DV_GET_REUSE_CNT(dev_id);

        /* If the device was not registered */
        if ((DVD_Dev_Registry[reg_index].entry_active_flag != NU_TRUE) ||
            (reuse_cnt != DVD_Dev_Registry[reg_index].reuse_cnt))
        {
            /* Can't send a command to a device that is not registered */
            status = DV_DEV_NOT_REGISTERED;
        }
        else
        {
            /* If the session is not open */
            if (DVD_Dev_Registry[reg_index].session[session_index].state_flag != DV_SES_OPEN)
            {
                /* Can't send a command to a device that is not open */
                status = DV_SESSION_NOT_OPEN;
            }
        }
#endif

        /* If everything is OK */
        if (status == NU_SUCCESS)
        {
            /* Get the session handle to pass to the driver */
            session_handle = DVD_Dev_Registry[reg_index].session[session_index].handle;

            /* Check if read function is available */
            if (DVD_Dev_Registry[reg_index].drv_read_ptr != NU_NULL)
            {
                /* Increment the sessions's read count */
                DVD_Dev_Registry[reg_index].session[session_index].active_cmds_cnt++;

                /* Call the driver's read function code */
                status = (*DVD_Dev_Registry[reg_index].drv_read_ptr)(session_handle, buffer_ptr,
                                                            numbyte, byte_offset, &bytes_r);

                /* If we have a place to return the number of bytes read */
                if (bytes_read_ptr != NU_NULL)
                {
                    /* Return the number of bytes read */
                    *bytes_read_ptr = bytes_r;
                }

                /* Decrement the sessions's read count */
                DVD_Dev_Registry[reg_index].session[session_index].active_cmds_cnt--;

                /* If there is a close session command pending */
                if ((DVD_Dev_Registry[reg_index].session[session_index].state_flag == DV_SES_LOCKED) &&
                    (DVD_Dev_Registry[reg_index].session[session_index].active_cmds_cnt == 0))
                {
                    /* Set the event flag bit signifying no active session commands */
                    (VOID)NU_Set_Events(
                            &DVD_Dev_Registry[reg_index].active_cmds_event,
                            (1 << session_index), (OPTION)NU_OR);
                }
            }
        }

        /* Return to user mode */
        NU_USER_MODE();
    }

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       DVC_Dev_Write
*
*   DESCRIPTION
*
*       This function interfaces to the actual device's write function.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       Low Level Driver Write
*
*   INPUTS
*
*       dev_handle                          Device handle
*       *buffer_ptr                         Memory address to start writing from
*       numbyte                             The size in bytes of the buffer
*       byte_offset                         Number of bytes to offset from 0
*       *bytes_written_ptr                  Pointer to where we return the number of
*                                           bytes written
*
*   OUTPUTS
*
*       status
*           NU_SUCCESS
*           DV_INVALID_INPUT_PARAMS
*           DV_DEV_NOT_REGISTERED
*           DV_SESSION_NOT_OPEN
*           Driver's WRITE function returned error status
*
*************************************************************************/
STATUS DVC_Dev_Write (DV_DEV_HANDLE dev_handle, VOID *buffer_ptr, UINT32 numbyte,
                   OFFSET_T byte_offset, UINT32 *bytes_written_ptr)
{
    DV_DEV_ID               dev_id;
    VOID                    *session_handle;
    UINT32                  bytes_w=0;
    INT32                   reg_index, session_index;
    STATUS                  status = NU_SUCCESS;
#if (DV_ERR_CHECK_ENABLE == NU_TRUE)
    INT32                   reuse_cnt;
#endif

    NU_SUPERV_USER_VARIABLES

    /* Get the device ID */
    dev_id = DV_GET_DEV_ID(dev_handle);

    /* Get the session that we are using */
    session_index = DV_GET_SES_INDEX(dev_handle);

    /* Get device registry index */
    reg_index = DV_GET_REG_INDEX(dev_id);

#if (DV_ERR_CHECK_ENABLE == NU_TRUE)
    /* Check the input parameters */
    if (((reg_index < (DV_DEV_ID)0) || (reg_index >= DVD_Max_Dev_Id_Cnt)) ||
        ((session_index < 0) || (session_index >= DV_MAX_DEV_SESSION_CNT)) ||
        (buffer_ptr == NU_NULL))
    {
        /* Invalid input parameters */
        status = DV_INVALID_INPUT_PARAMS;
    }
    else
#endif
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE();

#if (DV_ERR_CHECK_ENABLE == NU_TRUE)

        /* Get device id reuse count */
        reuse_cnt = DV_GET_REUSE_CNT(dev_id);

        /* If the device was not registered */
        if ((DVD_Dev_Registry[reg_index].entry_active_flag != NU_TRUE) ||
            (reuse_cnt != DVD_Dev_Registry[reg_index].reuse_cnt))
        {
            /* Can't send a command to a device that is not registered */
            status = DV_DEV_NOT_REGISTERED;
        }
        else
        {
            /* If the session is not open */
            if (DVD_Dev_Registry[reg_index].session[session_index].state_flag != DV_SES_OPEN)
            {
                /* Can't send a command to a device that is not open */
                status = DV_SESSION_NOT_OPEN;
            }
        }
#endif

        /* If everything is OK */
        if (status == NU_SUCCESS)
        {
            /* Get the session handle to pass to the driver */
            session_handle = DVD_Dev_Registry[reg_index].session[session_index].handle;

            /* Check if write function is available */
            if (DVD_Dev_Registry[reg_index].drv_write_ptr != NU_NULL)
            {
                /* Increment the session's write count */
                DVD_Dev_Registry[reg_index].session[session_index].active_cmds_cnt++;

                /* Call the driver's write function code */
                status = (*DVD_Dev_Registry[reg_index].drv_write_ptr)(session_handle, buffer_ptr,
                                                            numbyte, byte_offset, &bytes_w);

                /* If we have a place to return the number of bytes written */
                if (bytes_written_ptr != NU_NULL)
                {
                    /* Return the number of bytes written */
                    *bytes_written_ptr = bytes_w;
                }

                /* Decrement the session's write count */
                DVD_Dev_Registry[reg_index].session[session_index].active_cmds_cnt--;

                /* If there is a close session command pending */
                if ((DVD_Dev_Registry[reg_index].session[session_index].state_flag == DV_SES_LOCKED) &&
                    (DVD_Dev_Registry[reg_index].session[session_index].active_cmds_cnt == 0))
                {
                    /* Set the event flag bit signifying no active session commands */
                    (VOID)NU_Set_Events(
                            &DVD_Dev_Registry[reg_index].active_cmds_event,
                            (1 << session_index), (OPTION)NU_OR);
                }
            }
        }

        /* Return to user mode */
        NU_USER_MODE();
    }

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       DVC_Dev_Ioctl
*
*   DESCRIPTION
*
*       This function interfaces to the actual device's IOCTL function.
*
*   CALLED BY
*
*       Application
*
*   CALLS
*
*       Low Level Driver Ioctl
*
*   INPUTS
*
*       dev_handle                          Device handle
*       ioctl_num                           IOCTL command code
*       ioctl_data                          The IOCTL data
*       ioctl_data_len                      Length of the IOCTL data
*
*   OUTPUTS
*
*       status
*           NU_SUCCESS
*           DV_INVALID_INPUT_PARAMS
*           DV_DEV_NOT_REGISTERED
*           DV_SESSION_NOT_OPEN
*           Driver's IOCTL function returned error status
*
*************************************************************************/
STATUS DVC_Dev_Ioctl (DV_DEV_HANDLE dev_handle, INT ioctl_num,
                      VOID* ioctl_data, INT ioctl_data_len)
{
    DV_DEV_ID               dev_id;
    VOID                    *session_handle;
    INT32                   reg_index, session_index;
    STATUS                  status = NU_SUCCESS;
#if (DV_ERR_CHECK_ENABLE == NU_TRUE)
    INT32                   reuse_cnt;
#endif

    NU_SUPERV_USER_VARIABLES

    /* Get the device ID */
    dev_id = DV_GET_DEV_ID(dev_handle);

    /* Get the session that we are using */
    session_index = DV_GET_SES_INDEX(dev_handle);

    /* Get device registry index */
    reg_index = DV_GET_REG_INDEX(dev_id);

#if (DV_ERR_CHECK_ENABLE == NU_TRUE)
    /* Check the input parameters */
    if (((reg_index < (DV_DEV_ID)0) || (reg_index >= DVD_Max_Dev_Id_Cnt)) ||
        ((session_index < 0) || (session_index >= DV_MAX_DEV_SESSION_CNT)) ||
        (ioctl_data_len < 0))
    {
        /* Invalid input parameters */
        status = DV_INVALID_INPUT_PARAMS;
    }
    else
#endif
    {
        /* Switch to supervisor mode */
        NU_SUPERVISOR_MODE_ISR();

#if (DV_ERR_CHECK_ENABLE == NU_TRUE)

        /* Get device id reuse count */
        reuse_cnt = DV_GET_REUSE_CNT(dev_id);

        /* If the device was not registered */
        if ((DVD_Dev_Registry[reg_index].entry_active_flag != NU_TRUE) ||
            (reuse_cnt != DVD_Dev_Registry[reg_index].reuse_cnt))
        {
            /* Can't send a command to a device that is not registered */
            status = DV_DEV_NOT_REGISTERED;
        }
        else
        {
            /* If the session is not open */
            if (DVD_Dev_Registry[reg_index].session[session_index].state_flag != DV_SES_OPEN)
            {
                /* Can't send a command to a device that is not open */
                status = DV_SESSION_NOT_OPEN;
            }
        }
#endif

        /* If everything is OK */
        if (status == NU_SUCCESS)
        {
            /* Get the session handle to pass to the driver */
            session_handle = DVD_Dev_Registry[reg_index].session[session_index].handle;

            /* Increment the session's ioctl count */
            DVD_Dev_Registry[reg_index].session[session_index].active_cmds_cnt++;

            /* Call the driver's IOCTL function code */
            status = (*DVD_Dev_Registry[reg_index].drv_ioctl_ptr)(session_handle,
                                                    ioctl_num, ioctl_data, ioctl_data_len);

            /* Decrement the session's ioctl count */
            DVD_Dev_Registry[reg_index].session[session_index].active_cmds_cnt--;

            /* If there is a close session command pending */
            if ((DVD_Dev_Registry[reg_index].session[session_index].state_flag == DV_SES_LOCKED) &&
                (DVD_Dev_Registry[reg_index].session[session_index].active_cmds_cnt == 0))
            {
                /* Set the event flag bit signifying no active session commands */
                (VOID)NU_Set_Events(
                        &DVD_Dev_Registry[reg_index].active_cmds_event,
                        (1 << session_index), (OPTION)NU_OR);
            }
        }

        /* Return to user mode */
        NU_USER_MODE_ISR();
    }

    return (status);
}

