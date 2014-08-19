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
*       enc_common.c
*
*   COMPONENT
*
*       EN - Device Manager Notification
*
*   DESCRIPTION
*
*       This file contains the core functions of Event Notification component.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       NU_Notification_Queue_Create
*       NU_Notification_Listen_Start
*       NU_Notification_Send
*       NU_Notification_Get
*       NU_Notification_Listen_Stop
*       NU_Notification_Queue_Delete
*       NU_Notification_Register
*       NU_Notification_Unregister
*       NU_Notification_Find_Sender
*
*   DEPENDENCIES
*
*       <string.h>                          String functions
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       pipe.h                           Pipe Control functions
*       event_notification.h                           Event Notification functions
*       dv.h                           Device Manager functions
*
*************************************************************************/
#include        <string.h>
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"
#include        "os/kernel/plus/supplement/inc/pipe.h"
#include        "os/kernel/plus/supplement/inc/event_notification.h"
#include        "os/kernel/devmgr/inc/device_manager.h"
#include        "os/kernel/plus/core/inc/thread_control.h"

/* System memory pools */
extern          NU_MEMORY_POOL  System_Memory;

/* Device Manager Registry */
extern DV_DEV_REGISTRY  *DVD_Dev_Registry;

/* Device Manager max device id count */
extern INT              DVD_Max_Dev_Id_Cnt;

/* Notification Registry active dev count */
extern INT  END_Reg_Active_Cnt;

/* Notification Registry */
extern EN_REGISTRY *END_Registry;

/*  This is a dummy function solely used to in NU_Notification_Register
    to register a sender. This functions is a wrapper of DVC_Dev_Register,
    which requires pointers to open, close and ioctl functions of a device.
    Since no such functions exist for the sender, a dummy function is used */

static STATUS ENC_Dummy_Function()
{
    return NU_SUCCESS;
}

static DV_DRV_FUNCTIONS ENC_Dummy_Functions = {
    (DV_DRV_OPEN_FUNCTION)ENC_Dummy_Function,
    (DV_DRV_CLOSE_FUNCTION)ENC_Dummy_Function,
    (DV_DRV_READ_FUNCTION)ENC_Dummy_Function,
    (DV_DRV_WRITE_FUNCTION)ENC_Dummy_Function,
    (DV_DRV_IOCTL_FUNCTION)ENC_Dummy_Function
};

/*************************************************************************
*
* FUNCTION
*
*       NU_Notification_Queue_Create
*
* DESCRIPTION
*
*       This function creates a notification queue.
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       VOID*           handle              - Notification queue handle on success
*                                           - NU_NULL on error
*
*************************************************************************/
VOID *NU_Notification_Queue_Create(VOID)
{
    STATUS status = NU_SUCCESS;
    VOID *handle = NU_NULL;
    VOID *pipe_mem_ptr, *pipe_cb_ptr = NU_NULL;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Allocate memory for communication pipe */
    status = NU_Allocate_Memory(&System_Memory, &pipe_mem_ptr, EN_MAX_PIPE_LEN,
                                NU_NO_SUSPEND);

    /* Check to see if previous operation successful */
    if (status == NU_SUCCESS)
    {
        /* Allocate memory for pipe control block */
        status = NU_Allocate_Memory(&System_Memory, &pipe_cb_ptr, sizeof(NU_PIPE), NU_NO_SUSPEND);
    }

    /* If we successfully allocated memory for the semaphore control block */
    if (status == NU_SUCCESS)
    {
        /* Clear the memory we just allocated */
        (VOID)memset((VOID*)pipe_mem_ptr, 0, EN_MAX_PIPE_LEN);
        (VOID)memset((VOID*)pipe_cb_ptr, 0, sizeof(NU_PIPE));
    }

    /* Check to see if previous operation successful */
    if (status == NU_SUCCESS)
    {
        /* Create communication pipe */
        status = NU_Create_Pipe(pipe_cb_ptr, "DNTPIx", pipe_mem_ptr, EN_MAX_PIPE_LEN,
                                NU_VARIABLE_SIZE, EN_MAX_PIPE_MSG_LEN, NU_FIFO);
    }

    /* If everything is OK */
    if (status == NU_SUCCESS)
    {
        /* Return pipe control block as the handle */
        handle = (VOID*)pipe_cb_ptr;
    }

    /* Return to user mode */
    NU_USER_MODE();

    return (handle);
}


/*************************************************************************
*
* FUNCTION
*
*       NU_Notification_Listen_Start
*
* DESCRIPTION
*
*       This function attaches a notification queue to a device and
*       establishes the message types to be monitored.
*
* INPUTS
*
*       VOID*           handle              - Notification queue handle
*       DV_DEV_ID       dev_id              - Device ID to monitor
*       UINT32          type                - Message type to monitor
*       UINT32          type_mask           - Message type mask
*
* OUTPUTS
*
*       STATUS          status              - NU_SUCCESS
*                                           - NU_EN_INVALID_INPUT_PARAMS
*                                           - NU_EN_ALREADY_LISTENING
*                                           - EN_NO_AVAIL_LISTEN_ENTRY
*
*************************************************************************/
STATUS NU_Notification_Listen_Start(VOID *handle, DV_DEV_ID dev_id,
                                    UINT32 type, UINT32 type_mask)
{
    STATUS status = NU_SUCCESS;
    INT found = NU_FALSE;
    INT active_found_cnt = 0;
    INT i;
    INT32 reuse_cnt, reg_index;

    NU_SUPERV_USER_VARIABLES

    /* Get device id reuse count */
    reuse_cnt = DV_GET_REUSE_CNT(dev_id);

    /* Get device registry index */
    reg_index = DV_GET_REG_INDEX(dev_id);

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Check the input parameters */
    if (((handle == NU_NULL) || (((PI_PCB*)handle)->pi_id != PI_PIPE_ID)) ||
        ((reg_index < 0) || (reg_index >= DVD_Max_Dev_Id_Cnt)) ||
        (reuse_cnt != DVD_Dev_Registry[reg_index].reuse_cnt) ||
        ((type_mask == (UINT32)0) && (type != (UINT32)0)) ||
        ((type_mask & type) != type))
    {
        /* Return invalid input parameters error */
        status = NU_EN_INVALID_INPUT_PARAMS;
    }
    else
    {
        /* Protect against access to the registry.  */
        TCCT_Schedule_Lock();

        /* If this entry is active */
        if (END_Registry[reg_index].en_entry_active_flag == NU_TRUE)
        {
            /* Start at the first listen entry */
            i=0;

            /* Search the listener array for this handle */
            while ((status == NU_SUCCESS) && (active_found_cnt < END_Registry[reg_index].en_listen_cnt))
            {
                /* If this entry is active */
                if (END_Registry[reg_index].en_listen[i].en_handle != NU_NULL)
                {
                    /* Increment the number of active listeners found */
                    active_found_cnt++;

                    /* If we found this handle already being used */
                    if (END_Registry[reg_index].en_listen[i].en_handle == handle)
                    {
                        /* Return already listening to device error */
                        status = NU_EN_ALREADY_LISTENING;
                    }
                }

                /* Go to the next listen entry */
                i++;
            }

            /* If we did not find this handle already being used */
            if (status == NU_SUCCESS)
            {
                /* If we do not have an available listener entry */
                if (END_Registry[reg_index].en_listen_cnt >= EN_MAX_LISTEN_CNT)
                {
                    /* Return no available listener entry error */
                    status = NU_EN_NO_AVAIL_LISTEN_ENTRY;
                }
            }
        }

        /* If everything is OK */
        if (status == NU_SUCCESS)
        {
            /* If this entry is not active */
            if (END_Registry[reg_index].en_entry_active_flag == NU_FALSE)
            {
                /* Mark that this entry is active */
                END_Registry[reg_index].en_entry_active_flag = NU_TRUE;

                /* Increment the number of devices with listeners count */
                END_Reg_Active_Cnt++;
            }

            /* Search the listener array */
            for (i=0; ((i<EN_MAX_LISTEN_CNT) && (found == NU_FALSE)); i++)
            {
                /* If this entry is available */
                if (END_Registry[reg_index].en_listen[i].en_handle == NU_NULL)
                {
                    /* Increment the number of listeners for this device */
                    END_Registry[reg_index].en_listen_cnt++;

                    /* Remember the handle */
                    END_Registry[reg_index].en_listen[i].en_handle = handle;

                    /* Remember the message type this listener is monitoring */
                    END_Registry[reg_index].en_listen[i].en_type = type;

                    /* Remember the message type mask this listener is monitoring */
                    END_Registry[reg_index].en_listen[i].en_type_mask = type_mask;

                    /* Exit the loop */
                    found = NU_TRUE;
                }
            }
        }

        /* Release protection against access to the registry.  */
        TCCT_Schedule_Unlock();
    }

    /* Return to user mode */
    NU_USER_MODE();

    return (status);
}


/*************************************************************************
*
* FUNCTION
*
*       NU_Notification_Send
*
* DESCRIPTION
*
*       This function sends a notification message.
*
* INPUTS
*
*       DV_DEV_ID       dev_id              - Device ID of the sender
*       UINT32          type                - Message type
*       VOID*           msg                 - Message to be sent
*       UINT8           msg_len             - Length of message
*
* OUTPUTS
*
*       STATUS          status              - NU_SUCCESS
*                                           - NU_EN_INVALID_INPUT_PARAMS
*                                           - NU_EN_NO_ACTIVE_LISTENERS
*
*************************************************************************/
STATUS NU_Notification_Send (DV_DEV_ID dev_id, UINT32 type, VOID *msg, UINT8 msg_len)
{
    STATUS status = NU_SUCCESS;
    VOID* handle;
    UINT32 listen_type, listen_type_mask;
    EN_NOTIFY_MSG notify_msg;
    INT active_found_cnt = 0;
    INT i;
    INT32 reuse_cnt, reg_index;

    NU_SUPERV_USER_VARIABLES

    /* Get device id reuse count */
    reuse_cnt = DV_GET_REUSE_CNT(dev_id);

    /* Get device registry index */
    reg_index = DV_GET_REG_INDEX(dev_id);

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Check the input parameters */
    if (((reg_index < 0) || (reg_index >= DVD_Max_Dev_Id_Cnt)) ||
        (reuse_cnt != DVD_Dev_Registry[reg_index].reuse_cnt) ||
        ((msg == NU_NULL) && (msg_len != (UINT8)0)) ||
        ((msg != NU_NULL) && (msg_len == (UINT8)0)) ||
        (msg_len > EN_MAX_MSG_LEN))
    {
        /* Return invalid input parameters error */
        status = NU_EN_INVALID_INPUT_PARAMS;
    }
    else
    {
        /* Protect against access to the registry.  */
        TCCT_Schedule_Lock();

        /* If there are no active listeners */
        if (END_Registry[reg_index].en_entry_active_flag == NU_FALSE)
        {
            /* Return no active listeners error */
            status = NU_EN_NO_ACTIVE_LISTENERS;
        }

        /* If everything is OK */
        if (status == NU_SUCCESS)
        {
            /* Start at the first listen entry */
            i=0;

            /* Search the listener array */
            while (active_found_cnt < END_Registry[reg_index].en_listen_cnt)
            {
                /* If this entry is active */
                if (END_Registry[reg_index].en_listen[i].en_handle != NU_NULL)
                {
                    /* Increment the number of active listeners found */
                    active_found_cnt++;

                    /* Get the message type and type mask this listener is monitoring */
                    listen_type = END_Registry[reg_index].en_listen[i].en_type;
                    listen_type_mask = END_Registry[reg_index].en_listen[i].en_type_mask;

                    /* If this listener is looking for this message type */
                    if ((listen_type == (type & listen_type_mask)) || (listen_type_mask == (UINT32)0))
                    {
                        /* Get the handle */
                        handle = END_Registry[reg_index].en_listen[i].en_handle;

                        /* Build a notification message */
                        notify_msg.en_hdr.en_dev_id = dev_id;
                        notify_msg.en_hdr.en_type = type;
                        notify_msg.en_hdr.en_msg_len = msg_len;

                        /* If there is a message to send */
                        if (msg_len > (UINT8)0)
                        {
                            /* Copy the message */
                            memcpy(notify_msg.en_msg, msg, (size_t)msg_len);
                        }

                        /* Release protection against access to the registry.  */
                        TCCT_Schedule_Unlock();

                        /* Send notification message */
                        (VOID)NU_Send_To_Pipe((NU_PIPE*)handle, &notify_msg,
                                                  (sizeof(EN_NOTIFY_HDR) + msg_len), NU_NO_SUSPEND);

                        /* Protect against access to the registry.  */
                        TCCT_Schedule_Lock();
                    }
                }

                /* Go to the next listen entry */
                i++;
            }
        }

        /* Release protection against access to the registry.  */
        TCCT_Schedule_Unlock();
    }

    /* Return to user mode */
    NU_USER_MODE();

    return (status);
}


/*************************************************************************
*
* FUNCTION
*
*       NU_Notification_Get
*
* DESCRIPTION
*
*       This function retrieves a notification message.
*
* INPUTS
*
*       VOID*       handle         - Notification queue handle
*       DV_DEV_ID   *dev_id_ptr    - Pointer to where device ID of sender will be
*                                    returned
*       UINT32      *type_ptr      - Pointer to where message type will be returned
*       VOID*       *msg_ptr       - Pointer to where the message will be returned
*       UINT8       *msg_len_ptr   - Pointer to where message length will be returned
*       UNSIGNED    suspend        - Determines whether to suspend or not until a
*                                    notification message is received. Or it can be
*                                    a timeout value to wait in system ticks.
*
* OUTPUTS
*
*       STATUS      status         - NU_SUCCESS
*                                  - NU_EN_INVALID_INPUT_PARAMS
*                                  - NU_Receive_From_Pipe returned error status
*
*************************************************************************/
STATUS NU_Notification_Get (VOID *handle, DV_DEV_ID *dev_id_ptr, UINT32 *type_ptr,
                            VOID* *msg_ptr, UINT8 *msg_len_ptr, UNSIGNED suspend)
{
    STATUS status = NU_SUCCESS;
    EN_NOTIFY_MSG notify_msg;
    UNSIGNED notify_msg_len;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Check the input parameters */
    if (((handle == NU_NULL) || (((PI_PCB*)handle)->pi_id != PI_PIPE_ID)) ||
        (dev_id_ptr == NU_NULL) || (type_ptr == NU_NULL) ||
        (msg_ptr == NU_NULL) || (msg_len_ptr == NU_NULL))
    {
        /* Return invalid input parameters error */
        status = NU_EN_INVALID_INPUT_PARAMS;
    }

    /* If everything is OK */
    if (status == NU_SUCCESS)
    {

        /* Read notification message from pipe */
        status =  NU_Receive_From_Pipe((NU_PIPE*)handle, &notify_msg, sizeof(EN_NOTIFY_MSG),
                                      &notify_msg_len, suspend);

        /* If we successfully read a message */
        if (status == NU_SUCCESS)
        {
            /* Return the device id */
            *dev_id_ptr = notify_msg.en_hdr.en_dev_id;

            /* Return the message type */
            *type_ptr = notify_msg.en_hdr.en_type;

            /* Return the message length */
            *msg_len_ptr = notify_msg.en_hdr.en_msg_len;

            /* If there is a message */
            if (notify_msg.en_hdr.en_msg_len > (UINT8)0)
            {
                /* Return the message */
                memcpy(msg_ptr, notify_msg.en_msg, (size_t)notify_msg.en_hdr.en_msg_len);
            }
        }
    }

    /* Return to user mode */
    NU_USER_MODE();

    return (status);
}


/*************************************************************************
*
* FUNCTION
*
*       NU_Notification_Listen_Stop
*
* DESCRIPTION
*
*       This function detaches a notification queue from a device.
*
* INPUTS
*
*       VOID*           handle              - Notification queue handle
*       DV_DEV_ID       dev_id              - Device ID that was being monitored
*
* OUTPUTS
*
*       STATUS          status              - NU_SUCCESS
*                                           - NU_EN_INVALID_INPUT_PARAMS
*                                           - EN_LISTENER_NOT_FOUND
*
*************************************************************************/
STATUS NU_Notification_Listen_Stop (VOID *handle, DV_DEV_ID dev_id)
{
    STATUS status = NU_SUCCESS;
    INT found = NU_FALSE;
    INT active_found_cnt = 0;
    INT i;
    INT32 reuse_cnt, reg_index;

    NU_SUPERV_USER_VARIABLES

    /* Get device id reuse count */
    reuse_cnt = DV_GET_REUSE_CNT(dev_id);

    /* Get device registry index */
    reg_index = DV_GET_REG_INDEX(dev_id);

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Check the input parameters */
    if (((handle == NU_NULL) || (((PI_PCB*)handle)->pi_id != PI_PIPE_ID)) ||
        ((reg_index < 0) || (reg_index >= DVD_Max_Dev_Id_Cnt)) ||
        (reuse_cnt != DVD_Dev_Registry[reg_index].reuse_cnt))
    {
        /* Return invalid input parameters error */
        status = NU_EN_INVALID_INPUT_PARAMS;
    }

    /* If everything is OK */
    if (status == NU_SUCCESS)
    {
        /* Protect against access to the registry.  */
        TCCT_Schedule_Lock();

        /* If this entry is active */
        if (END_Registry[reg_index].en_entry_active_flag == NU_TRUE)
        {
            /* Start at the first listen entry */
            i=0;

            /* Search the listener array for this handle */
            while ((found == NU_FALSE) && (active_found_cnt < END_Registry[reg_index].en_listen_cnt))
            {
                /* If this entry is active */
                if (END_Registry[reg_index].en_listen[i].en_handle != NU_NULL)
                {
                    /* Increment the number of active listeners found */
                    active_found_cnt++;

                    /* If this listener was using this handle */
                    if (END_Registry[reg_index].en_listen[i].en_handle == handle)
                    {
                        /* Clear the handle */
                        END_Registry[reg_index].en_listen[i].en_handle = NU_NULL;

                        /* Clear the listener type */
                        END_Registry[reg_index].en_listen[i].en_type = (UINT32)0;

                        /* Clear the listener type mask */
                        END_Registry[reg_index].en_listen[i].en_type_mask = (UINT32)0;

                        /* Decrement the active number of listeners for this device */
                        END_Registry[reg_index].en_listen_cnt--;

                        /* Exit the loop */
                        found = NU_TRUE;
                    }
                }

                /* Go to the next listen entry */
                i++;
            }

            /* If we found a listener using this handle */
            if (found == NU_TRUE)
            {
                /* If there are no active listeners */
                if (END_Registry[reg_index].en_listen_cnt <= 0)
                {
                    /* Clear the entry active flag */
                    END_Registry[reg_index].en_entry_active_flag = NU_FALSE;

                    /* Decrement the number of devices with active listeners count */
                    END_Reg_Active_Cnt--;
                }
            }
            else
            {
                /* Return listener not found error */
                status = NU_EN_LISTENER_NOT_FOUND;
            }
        }
        else
        {
            /* Return listener not found error */
            status = NU_EN_LISTENER_NOT_FOUND;
        }

        /* Release protection against access to the registry.  */
        TCCT_Schedule_Unlock();
    }

    /* Return to user mode */
    NU_USER_MODE();

    return (status);
}


/*************************************************************************
*
* FUNCTION
*
*       NU_Notification_Queue_Delete
*
* DESCRIPTION
*
*       This function deletes a notification queue.
*
* INPUTS
*
*       VOID*           handle              - Notification queue handle
*
* OUTPUTS
*
*       STATUS          status              - NU_SUCCESS
*                                           - NU_EN_INVALID_INPUT_PARAMS
*                                           - NU_Delete_Pipe returned error status
*                                           - NU_Deallocate_Memory returned error status
*
*************************************************************************/
STATUS NU_Notification_Queue_Delete (VOID *handle)
{
    STATUS status = NU_SUCCESS;
    VOID *pipe_mem_ptr;
    INT active_entry_found_cnt;
    INT active_listen_found_cnt;
    INT active_entry_cnt;
    INT active_listen_cnt;
    INT found;
    INT i,j;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Check the input parameters */
    if ((handle == NU_NULL) || (((PI_PCB*)handle)->pi_id != PI_PIPE_ID))
    {
        /* Return invalid input parameters error */
        status = NU_EN_INVALID_INPUT_PARAMS;
    }

    /* If everything is OK */
    if (status == NU_SUCCESS)
    {
        /* Protect against access to the registry.  */
        TCCT_Schedule_Lock();

        /* Remember how many active registry entries we have */
        active_entry_cnt = END_Reg_Active_Cnt;

        /* Start at the first registry entry */
        i=0;

        /* Reset the active entry found count */
        active_entry_found_cnt = 0;

        /* Search the registry for an entry using this handle */
        while (active_entry_found_cnt < active_entry_cnt)
        {
            /* If this registry entry is active */
            if (END_Registry[i].en_entry_active_flag == NU_TRUE)
            {
                /* Increment the number of active registry entries found */
                active_entry_found_cnt++;

                /* Remember how many active listen entries we have */
                active_listen_cnt = END_Registry[i].en_listen_cnt;

                /* Start at the first listen entry */
                j=0;

                /* Reset the active listen found count */
                active_listen_found_cnt = 0;

                /* Reset the found flag */
                found = NU_FALSE;

                /* Search the listener array for this handle */
                while ((found == NU_FALSE) && (active_listen_found_cnt < active_listen_cnt))
                {
                    /* If this listen entry is active */
                    if (END_Registry[i].en_listen[j].en_handle != NU_NULL)
                    {
                        /* Increment the number of active listeners found */
                        active_listen_found_cnt++;

                        /* If this listener was using this handle */
                        if (END_Registry[i].en_listen[j].en_handle == handle)
                        {
                            /* Clear the handle */
                            END_Registry[i].en_listen[j].en_handle = NU_NULL;

                            /* Clear the listener type */
                            END_Registry[i].en_listen[j].en_type = (UINT32)0;

                            /* Clear the listener type mask */
                            END_Registry[i].en_listen[j].en_type_mask = (UINT32)0;

                            /* Decrement the active number of listeners for this device */
                            END_Registry[i].en_listen_cnt--;

                            /* Exit the loop */
                            found = NU_TRUE;
                        }
                    }

                    /* Go to the next listen entry */
                    j++;
                }

                /* If there are no active listeners left */
                if (END_Registry[i].en_listen_cnt <= 0)
                {
                    /* Clear the entry active flag */
                    END_Registry[i].en_entry_active_flag = NU_FALSE;

                    /* Decrement the number of devices with active listeners count */
                    END_Reg_Active_Cnt--;
                }
            }

            /* Go to the next registry entry */
            i++;
        }

        /* Get the start address of the pipe memory */
        pipe_mem_ptr = ((PI_PCB*)handle)->pi_start;

        /* Release protection against access to the registry.  */
        TCCT_Schedule_Unlock();

        /* Delete communication pipe */
        status = NU_Delete_Pipe((NU_PIPE*)handle);

        /* Check to see if previous operation successful */
        if (status == NU_SUCCESS)
        {
            /* Deallocate the pipe control block memory */
            status = NU_Deallocate_Memory(handle);
        }
        else
        {
            /* Check to see if previous operation successful */
            if (status == NU_SUCCESS)
            {
                /* Deallocate the pipe memory */
                status = NU_Deallocate_Memory(pipe_mem_ptr);
            }
        }
    }

    /* Return to user mode */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_Notification_Register
*
* DESCRIPTION
*
*       This function register a notification sender.
*
* INPUTS
*
*       DV_DEV_LABEL label                  - Sender label
*
* OUTPUTS
*
*       DV_DEV_ID* sender_id                - Newly created ID for the sender
*
*       STATUS          status              - NU_SUCCESS
*                                           - DVC_Dev_Register returned error status
*
*************************************************************************/
STATUS NU_Notification_Register (DV_DEV_LABEL label, DV_DEV_ID* sender_id)
{
    STATUS     status = NU_SUCCESS;
    DV_DEV_LABEL    labels[1];

    /* prepare a list of 1 label */
    labels[0] = label;

    /* call Device Manager to register the sender */
    status = DVC_Dev_Register (NULL /* instance handle, not used*/,
                               labels, 1 /* label count */,
                               &ENC_Dummy_Functions,
                               sender_id);
    return status;
}


/*************************************************************************
*
* FUNCTION
*
*       NU_Notification_Unregister
*
* DESCRIPTION
*
*       This function unregister a notification sender.
*
* INPUTS
*
*       DV_DEV_ID sender_id                  - Sender ID previously registered
*
* OUTPUTS
*
*       STATUS status                       - NU_SUCCESS
*                                           - DVC_Dev_Unregister returned error status
*
*************************************************************************/
STATUS NU_Notification_Unregister (DV_DEV_ID sender_id)
{
    STATUS  status = NU_SUCCESS;
    VOID*   handle;

     /* call Device Manager to unregister the sender */
    status = DVC_Dev_Unregister (sender_id, &handle);

    return status;
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_Notification_Find_Sender
*
*   DESCRIPTION
*
*       This function searches the sender list for all sender ID(s) that
*       satisfy the label list.
*
*   INPUTS
*
*       label_list[]                        Sender label list
*       label_cnt                           Number of sender labels. If
*                                           0, then return all sender IDs
*       sender_id_list[]                    sender ID list we return
*       *sender_id_cnt_ptr                  Pointer to variable that contains
*                                           the maximum IDs to return and
*                                           it is also the place where we
*                                           return the number of IDs found
*
*   OUTPUTS
*
*       STATUS status                       - NU_SUCCESS
*                                           - DVC_Dev_ID_Get returned error status
*
*************************************************************************/
STATUS NU_Notification_Find_Sender (DV_DEV_LABEL label_list[],
                                    UINT32 label_cnt,
                                    DV_DEV_ID sender_id_list[],
                                    UINT32* sender_id_cnt)
{
    return DVC_Dev_ID_Get (label_list, (INT)label_cnt, sender_id_list, (INT *)sender_id_cnt);
}
