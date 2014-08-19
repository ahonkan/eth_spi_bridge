/***********************************************************************
*
*           Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
************************************************************************

*************************************************************************
*
* FILE NAME
*
*        nu_usbh_user_ext.c
*
* COMPONENT
*
*       Nucleus USB Host Stack : User Layer
*
* DESCRIPTION
*       This file contains the implementation of external interfaces
*       exported by Nucleus USB Host User Layer.
*
*
* DATA STRUCTURES
*       None
*
* FUNCTIONS
*
*       _NU_USBH_USER_Create              Protected constructor
*       _NU_USBH_USER_Lock                Protection of data structures
*       _NU_USBH_USER_Unlock              Release protection.
*       NU_USBH_USER_Wait                 Wait for a device to be connected.
*       NU_USBH_USER_Open_Device          Open a device for use by applications.
*       NU_USBH_USER_Close_Device         Close the device when application
*                                         is done.
*       NU_USBH_USER_Remove_Device        Remove the device such that no threads
*                                         would be using it.
*       _NU_USBH_USER_Close_Device        -Decrements the reference
*                                         count for the device
*       _NU_USBH_USER_Connect             -Detects a new device connected
*       _NU_USBH_USER_Disconnect          -Detects a device disconnected.
*       _NU_USBH_USER_Open_Device         -Increments the reference
*                                         count for the device
*       _NU_USBH_USER_Remove_Device       -Provide service to remove a
*                                         device ensuring data
*                                         consistency.
*       _NU_USBH_USER_Wait                -Wait for a new device to be
*                                         connected which the calling
*                                         thread has not seen yet.
*       _NU_USBH_USER_Delete              - Deletes a Host User Driver.
*       NU_USBH_USER_Get_Drvr             Retrieves the class driver for
*                                         a session
* DEPENDENCIES
*
*       nu_usb.h                          All USB definitions
*
************************************************************************/
#ifndef USBH_USER_EXT_C
#define USBH_USER_EXT_C

/* ==============  Standard Include Files ============================  */
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "connectivity/nu_usb.h"

/*************************************************************************
* FUNCTION
*        _NU_USBH_USER_Create
*
* DESCRIPTION
*       This function initializes the data structures required by
*       NU_USBH_USER. This is used by extenders of user layer to initialize
*       base resources.
*
* INPUTS
*       cb                  pointer to user control block.
*       name                name for this USB object.
*       pool                pointer to memory pool used by USER.
*       bInterfaceSubclass  subclass this user is serving (else 0xFF)
*       bInterfaceProtocol  protocol this user is serving (else 0xFF)
*       dispatch            pointer to dispatch table filled by child.
*
* OUTPUTS
*       NU_SUCCESS              Indicates successful completion.
*       NU_INVALID_SEMAPHORE    Indicates control block is invalid.
*       NU_INVALID_GROUP        Indicates control block is invalid.
*
*************************************************************************/
STATUS _NU_USBH_USER_Create (NU_USBH_USER * cb,
                             CHAR * name,
                             NU_MEMORY_POOL * pool,
                             UINT8 bInterfaceSubclass,
                             UINT8 bInterfaceProtocol,
                             const VOID *dispatch)
{
    STATUS status;
    STATUS internal_sts = NU_SUCCESS;
    UINT8 rollback = 0;
    NU_USBH_SUBSYS *subsys;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(name);
    NU_USB_MEMPOOLCHK_RETURN(pool);
    NU_USB_PTRCHK_RETURN(dispatch);

    /* Check if usb host subsystem is not NU_NULL. */
    if(nu_usbh != NU_NULL)
    {
        /* get the subsystem this user is a member of */
        subsys = &nu_usbh->user_subsys;

        /* Create sessions list access lock. */
        status = NU_Create_Semaphore(&cb->list_lock, "usrLk", 1, NU_PRIORITY);
    }
    else
    {
        status = NU_USB_INTERNAL_ERROR;
    }

    if (status == NU_SUCCESS)
    {
        /*Initialize tasks bitmap */
        cb->waiting_tasks = 0;

        /* Create event flag group.  */
        status = NU_Create_Event_Group (&cb->device_ready_event, "DevEvt");
        if (status != NU_SUCCESS)
            rollback = 1;

        if (!rollback)
        {
            /* Initialize other elements of control block */
            cb->session_list_head = NU_NULL;
            cb->memory_pool = pool;

            /* call Base class behavior */
            status = _NU_USB_USER_Create ((NU_USB_USER *) cb,
                                      (NU_USB_SUBSYS *) subsys, name,
                                      bInterfaceSubclass, bInterfaceProtocol,
                                      dispatch);
            if (NU_SUCCESS != status)
                rollback = 2;
        }

        switch (rollback)
        {
                /* Rollback if failed */
            case 2:
                internal_sts = NU_Delete_Event_Group (&(cb->device_ready_event));
            case 1:
                internal_sts |= NU_Delete_Semaphore(&(cb->list_lock));
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ((status == NU_SUCCESS) ? internal_sts : status);
}

/*************************************************************************
* FUNCTION
*        _NU_USBH_USER_Lock
*
* DESCRIPTION
*       This is a protected interface which can be used by extenders to
*       secure their internal data structures. This call locks the access
*       to data structures.
*
* INPUTS
*       cb      pointer to User control block.
*
* OUTPUTS
*       NU_SUCCESS              Indicates successful completion
*       NU_INVALID_SEMAPHORE    Indicates control block is invalid.
*
*************************************************************************/
STATUS _NU_USBH_USER_Lock (NU_USBH_USER * cb)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);

    /* Protect against access to the list of session entries.  */
    status = NU_Obtain_Semaphore(&cb->list_lock, NU_SUSPEND);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*        _NU_USBH_USER_Unlock
*
* DESCRIPTION
*       This is a protected interface which can be used by extenders to
*       secure their internal data structures. This call unlocks the access
*       to data structures.
*
* INPUTS
*       cb      pointer to User control block.
*
* OUTPUTS
*       NU_SUCCESS              Indicates successful completion
*       NU_INVALID_SEMAPHORE    Indicates control block is invalid.
*
*************************************************************************/
STATUS _NU_USBH_USER_Unlock (NU_USBH_USER * cb)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    /* Protect against access to the list of session entries.  */
    status = NU_Release_Semaphore(&cb->list_lock);

   /* Switch back to user mode. */
   NU_USER_MODE();

   return (status);
}

/*************************************************************************
* FUNCTION
*        NU_USBH_USER_Wait
*
* DESCRIPTION
*       This function provides services for user level threads to wait for
*       a particular device to be connected. Thread goes into a state
*       specified by suspension option if the device is not yet connected.
*       This service returns a device handle as output when the call is
*       successful, which can be used by applications to use other services
*       of USER layer. When a thread first calls this service, it checks if
*       some device belonging to the user is already connected and gives
*       out its handle, else waits for device to be connected. If this
*       service is called again from the same thread, It checks for the
*       next available device and waits if it is not yet connected. This
*       can helps applications in traversing the devices to find a suitable
*       device.
*
* INPUTS
*       cb          pointer to User control block.
*       suspend     Suspension option.
*       handle_out  pointer to memory location to hold pointer to the
*                   device handle.
*
* OUTPUTS
*       NU_SUCCESS              Indicates successful completion.
*       NU_TIMEOUT              Indicates  timeout on suspension.
*       NU_NOT_PRESENT          Indicates event flags are not present.
*       NU_USB_INTERNAL_ERROR    Indicates a internal error in USB subsystem
*
*************************************************************************/
STATUS NU_USBH_USER_Wait (NU_USBH_USER * cb,
                          UNSIGNED suspend,
                          VOID **handle_out)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(handle_out);
    NU_USB_PTRCHK_RETURN(((NU_USB *) cb)->usb_dispatch);

    status = ((NU_USBH_USER_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
             Wait (cb, suspend, handle_out);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*        NU_USBH_USER_Open_Device
*
* DESCRIPTION
*       This function should be  called from user level threads/applications
*       when they want to use a device. This increases the reference count
*       for the device.
*
* INPUTS
*       cb      pointer to USER control block.
*       handle  handle/cookie for the concerned device.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*       NU_USB_INVLD_ARG    Indicates some argument became stale before
*                           call gets completed.
*       NU_NOT_PRESENT      Indicates Device has been safely Removed.
*
*************************************************************************/
STATUS NU_USBH_USER_Open_Device (NU_USBH_USER * cb,
                                 VOID *handle)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(handle);
    NU_USB_PTRCHK_RETURN(((NU_USB *) cb)->usb_dispatch);

    status = ((NU_USBH_USER_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
             Open_Device (cb, handle);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*        NU_USBH_USER_Close_Device
*
* DESCRIPTION
*
*       This function is called by the NU_USBH_USER's users.
* The applications can call this routine when the application has
* finished using the device. This decrement the reference count of
* the device. If safe removal is in progress and reference count reaches
* zero this task wakes up the task which is blocked on safe removal.
*
* INPUTS
*       cb      pointer to USER control block.
*       handle  handle/cookie for the concerned device.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*       NU_USB_INVLD_ARG    Indicates some pointer became stale before call
*                           gets completed.
*       NU_NOT_PRESENT      Indicates device has been safely removed.
*
*************************************************************************/
STATUS NU_USBH_USER_Close_Device (NU_USBH_USER * cb,
                                  VOID *handle)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(handle);
    NU_USB_PTRCHK_RETURN(((NU_USB *) cb)->usb_dispatch);

    status = ((NU_USBH_USER_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
            Close_Device (cb, handle);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*        NU_USBH_USER_Remove_Device
*
* DESCRIPTION
*       This function is used to remove a device safely from the subsystem
*       ensuring data consistency. Thread calling this function
*       suspends/timeout depending on the suspension option provided until
*       all the threads communicating with the device are completed.
*       Subsequent threads can not start using the device when safe removal
*       is in progress.
*
* INPUTS
*       cb      pointer to User control block.
*       handle  handle of the device to be removed safely.
*       suspend Suspension option.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*       NU_USB_INVLD_ARG    Some pointer became stale before call
*                           completion.
*
*************************************************************************/
STATUS NU_USBH_USER_Remove_Device (NU_USBH_USER * cb,
                                   VOID *handle,
                                   UNSIGNED suspend)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(handle);
    NU_USB_PTRCHK_RETURN(((NU_USB *) cb)->usb_dispatch);

    status = ((NU_USBH_USER_DISPATCH *) (((NU_USB *) cb)->usb_dispatch))->
            Remove_Device (cb, handle, suspend);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*        _NU_USBH_USER_Delete
*
* DESCRIPTION
*      This function deletes a USBH User. Note that this function does not
*      free the memory associated with the User control block.
*
* INPUTS
*
*       cb              pointer to the USB Object control block.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*       NU_INVALID_POINTER  Indicates control block is invalid
*
*************************************************************************/
STATUS _NU_USBH_USER_Delete (VOID *user)
{
    STATUS status;
    STATUS internal_sts = NU_SUCCESS;
    NU_USBH_USER *cb = (NU_USBH_USER *) user;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);

    /* Delete the device connect/disconnect event group */
    status = NU_Delete_Event_Group (&(cb->device_ready_event));

    /* Delete the list protection mutex */
    if (status == NU_SUCCESS)
    {
        status = NU_Delete_Semaphore(&(cb->list_lock));
    }

    /* Call Base behavior */
    internal_sts = _NU_USB_USER_Delete (cb);
    /* Switch back to user mode. */
    NU_USER_MODE();
    return ((status == NU_SUCCESS) ? internal_sts : status);
}

/*************************************************************************
* FUNCTION
*        _NU_USBH_USER_Connect
*
* DESCRIPTION
*       This function is called by class driver's connect routine when a
*       new device which can be served by this user gets connected. It
*       allocates all the structures required to provide
*       open/close/safely_remove services to applications for this device.
*       And unblocks the tasks waiting for the device to be connected.
*
* INPUTS
*       cb              pointer to user control block.
*       class_driver    pointer to calling class driver's control block.
*       handle          handle for the device connected.
*
* OUTPUTS
*       NU_SUCCESS          indicates successful completion.
*       NU_USB_MAX_EXCEEDED indicates user is already serving maximum
*                           devices it can support.
*       NU_INVALID_GROUP    Event group control block pointer is invalid
*
*************************************************************************/
STATUS _NU_USBH_USER_Connect (NU_USB_USER * cb,
                              NU_USB_DRVR * class_driver,
                              VOID *handle)
{
    /* new session(which can be served by "this" user) is 'opened'. */
    USBH_USER_SESSION *current_session = NU_NULL;
    STATUS internal_sts = NU_SUCCESS;
    STATUS status = NU_SUCCESS;
    /* A flag to find whether error has occured in user_lock or not. */
    BOOLEAN  user_lock_err = NU_TRUE;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(handle);
    NU_USB_PTRCHK_RETURN(class_driver);

    /* Can another session be supported ? */
    if (((NU_USBH_USER *) cb)->num_sessions == NU_USBH_USER_MAX_DEVICES)
        status = NU_USB_MAX_EXCEEDED;

    /* Allocate the current USBH_USER_SESSION structure. Each detected
     * device is recognized by this structure in the User Layer.
     */
    if (status == NU_SUCCESS)
    {
        status = USB_Allocate_Object(sizeof (USBH_USER_SESSION),
                                     (VOID **) &current_session);
    }

    /* If allocation fails(rare) */
    if (status == NU_SUCCESS && current_session != NU_NULL)
    {
        memset (current_session, 0, sizeof (USBH_USER_SESSION));

        /* Initialize other data members of USBH_USER_SESSION structure */
        status =
            NU_Create_Semaphore (&current_session->exclusive_lock, "xclsv", 1,
                                 NU_FIFO);
        if (status != NU_SUCCESS)
        {
            internal_sts = USB_Deallocate_Memory(current_session);
        }
    }

    if (status == NU_SUCCESS && current_session != NU_NULL)
    {
        current_session->reference_count = 1;
        current_session->handle = handle;
        current_session->drvr = class_driver;
        current_session->trying_to_remove = NU_FALSE;

        /* Protect against access to the list of session entries.  */
        status = _NU_USBH_USER_Lock ((NU_USBH_USER *)cb);

        if(status == NU_SUCCESS)
        {
            /* User is locked successfully. */
            user_lock_err = NU_FALSE;
            /* NU_USBH_USER maintains a list of all the sessions served by it.
             * Add the current session entry in list.
             */
            NU_Place_On_List ((CS_NODE **) &
                               (((NU_USBH_USER *) cb)->session_list_head),
                               (CS_NODE *) current_session);

            ((NU_USBH_USER *) cb)->num_sessions++;
            /* Device is connected at this point. Set the event flag 
             * so that the threads waiting for new device can be awakened.
             */
            status = NU_Set_Events(&(((NU_USBH_USER *)cb)->device_ready_event),
                                    ((NU_USBH_USER *)cb)->waiting_tasks,
                                    NU_OR);
            /* Clear the waiting tasks bitmap */
            ((NU_USBH_USER *) cb)->waiting_tasks = 0;

            /* Release protection against access to the list of session entries. */
            internal_sts = _NU_USBH_USER_Unlock ((NU_USBH_USER*)cb);
        }

        if(status != NU_SUCCESS)
        {
            /* Check if error is occured in user lock or not. */
            if(user_lock_err == NU_FALSE)
            {
                /* This means error occured in set events and now 
                 * current session should be removed from the list as well. 
                 */
                ((NU_USBH_USER *) cb)->num_sessions--;
                NU_Remove_From_List((CS_NODE **) &
                                     (((NU_USBH_USER *) cb)->session_list_head),
                                     (CS_NODE *) current_session);
            }
            /* Whether error is occured in user lock or set events, 
             * Reclaim the resources. 
             */
            internal_sts = 
            NU_Delete_Semaphore(&current_session->exclusive_lock);
            internal_sts |= USB_Deallocate_Memory(current_session);
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ((status == NU_SUCCESS) ?internal_sts : status);
}

/*************************************************************************
* FUNCTION
*        _NU_USBH_USER_Disconnect
*
* DESCRIPTION
*       This function is called by the class driver's disconnect function
*       when a device being served by this user gets disconnected. This
*       cleans up the device specific entries and clear the event flags
*       associated with the device.
*
* INPUTS
*       cb              pointer to user control block.
*       class_driver    pointer to calling class driver's control block.
*       handle          handle of the device disconnected.
*
* OUTPUTS
*       NU_SUCCESS      Indicates successful completion.
*       NU_NOT_PRESENT  If control block is deleted before call
*                       completion.
*
*************************************************************************/
STATUS _NU_USBH_USER_Disconnect (NU_USB_USER * cb,
                                 NU_USB_DRVR * class_driver,
                                 VOID *handle)
{
    STATUS internal_sts = NU_SUCCESS;
    STATUS status = NU_SUCCESS;
    USBH_USER_SESSION *current_session;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(handle);
    NU_USB_PTRCHK_RETURN(class_driver);

    current_session = USBH_USER_Find_Session ((NU_USBH_USER *) cb, handle);

    if (current_session != NU_NULL)
    {
        /* Protect against access to the list of session entries.  */
        status = _NU_USBH_USER_Lock ((NU_USBH_USER *)cb);

        if(status == NU_SUCCESS)
        {
            /* Remove the current device from the list. */
            NU_Remove_From_List ((CS_NODE **) &
                                  (((NU_USBH_USER *) cb)->session_list_head),
                                  (CS_NODE *) current_session);

            ((NU_USBH_USER *) cb)->num_sessions--;

            /* Release protection against access to the list of session entries. */
            status = _NU_USBH_USER_Unlock ((NU_USBH_USER*)cb);
        }
        internal_sts = NU_Delete_Semaphore (&(current_session->exclusive_lock));

        internal_sts |= USB_Deallocate_Memory (current_session);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ((status == NU_SUCCESS)? internal_sts : status);
}

/*************************************************************************
* FUNCTION
*        _NU_USBH_USER_Wait
*
* DESCRIPTION
*       This function provides services for user level threads to wait for
*       a particular device to be connected. Thread goes into a state
*       specified by suspension option if the device is not yet connected.
*       This service returns a device handle as output when the call is
*       successful, which can be used by applications to use other services
*       of USER layer. When a thread first calls this service, it checks if
*       some device belonging to the user is already connected and gives
*       out its handle, else waits for device to be connected. If this
*       service is called again from the same thread, It checks for the
*       next available device and waits if it is not yet connected. This
*       can helps applications in traversing the devices to find a suitable
*       device.
*
* INPUTS
*       cb          pointer to User control block.
*       suspend     Suspension option.
*       handle_out  pointer to memory location to hold pointer to the
*                   device handle.
*
* OUTPUTS
*       NU_SUCCESS              Indicates successful completion.
*       NU_TIMEOUT              Indicates  timeout on suspension.
*       NU_NOT_PRESENT          Indicates event flags are not present.
*       NU_USB_INTERNAL_ERROR   Indicates a internal error in USB subsystem
*
*************************************************************************/
STATUS _NU_USBH_USER_Wait (NU_USBH_USER * cb,
                           UNSIGNED suspend,
                           VOID **handle_out)
{
    /* Pointer to each session entry */
    CS_NODE *node_ptr;
    UINT32 retrieved, i;
    USBH_USER_SESSION *temp_entry;
    NU_TASK *current_task;
    STATUS status;
    STATUS internal_sts = NU_SUCCESS;
    UINT8 found;
    UINT32 bitmap;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(handle_out);

    current_task = NU_Current_Task_Pointer ();

    do{
        /* Protect against access to the list of session entries.  */
        status = _NU_USBH_USER_Lock (cb);

        if (status != NU_SUCCESS)
            break;

        /* Loop until list */
        node_ptr = (CS_NODE *) cb->session_list_head;

        while (node_ptr)
        {
            temp_entry = (USBH_USER_SESSION *) node_ptr;
            found = 0;

            for (i = 0; i < temp_entry->num_tasks; i++)
            {
                if (temp_entry->tasks_list[i] == current_task)
                {
                    found = 1;
                    break;
                }
            }

            /* if the task is not found add this task to the list */
            if (!found)
            {
                if(temp_entry->num_tasks >= NU_USBH_USER_MAX_TASKS)
                {
                    /* Release protection against access to the list of session entries. */
                    status = _NU_USBH_USER_Unlock (cb);

                    NU_UNUSED_PARAM(status);


                    /* Switch back to user mode. */
                    NU_USER_MODE();


                    return (NU_USB_MAX_EXCEEDED);
                }

                temp_entry->tasks_list[temp_entry->num_tasks] = current_task;
                temp_entry->num_tasks++;
                *handle_out = temp_entry->handle;

                /* Release protection against access to the list of session entries. */
                status = _NU_USBH_USER_Unlock (cb);


                /* Switch back to user mode. */
                NU_USER_MODE();


                return (status);
            }

            /* if the task is found go to next session entry */
            /* Position the node pointer to the next node.  */
            node_ptr = node_ptr->cs_next;

            /* Determine if the pointer is at the head of the list.  */
            if (node_ptr == (CS_NODE *) cb->session_list_head)

                /* The list search is complete.  */
                node_ptr = NU_NULL;
        }

        /* Return from here if suspend option is NU_NO_SUSPEND */
        if(suspend == NU_NO_SUSPEND)
        {
            *handle_out = NU_NULL;
            internal_sts = _NU_USBH_USER_Unlock(cb);

            NU_UNUSED_PARAM(internal_sts);
            status = NU_NOT_PRESENT;
            break;
        }

        /* Look for the first free index */
        for (i = 0, bitmap = 1; i < 32; i++, (bitmap = bitmap << 1))
            if ((bitmap & cb->waiting_tasks) == 0)
            {
                cb->waiting_tasks |= bitmap;
                break;
            }

        if(i == 32)
        {
            *handle_out = NU_NULL;
            internal_sts = _NU_USBH_USER_Unlock (cb);

            NU_UNUSED_PARAM(internal_sts);
            status = NU_USB_MAX_EXCEEDED;
            break;
        }

        /* Release protection against access to the list of session entries. */
        internal_sts = _NU_USBH_USER_Unlock (cb);

        NU_UNUSED_PARAM(internal_sts);
        /* Task is found in all the session entries */
        /* Suspend the task until at least one device (which can be served by this
         * user) is connected to the host
         */

        status = NU_Retrieve_Events (&cb->device_ready_event, bitmap,
                NU_OR_CONSUME, &retrieved, suspend);

        if (status != NU_SUCCESS)
        {
            *handle_out = NU_NULL;
            break;
        }

        /* A new device is connected now
         * add the task into the session entry's
         * task list and return session pointer
         */
    }
    while(1);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ((status == NU_SUCCESS) ? internal_sts : status);
}

/*************************************************************************
* FUNCTION
*        _NU_USBH_USER_Open_Device
*
* DESCRIPTION
*       This function should be  called from user level threads/applications
*       when they want to use a device. This increases the reference count
*       for the device.
*
* INPUTS
*       cb      pointer to USER control block.
*       handle  handle/cookie for the concerned device.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*       NU_USB_INVLD_ARG    Indicates some argument became stale before
*                           call gets completed.
*       NU_NOT_PRESENT      Indicates Device has been safely Removed.
*
*************************************************************************/
STATUS _NU_USBH_USER_Open_Device (NU_USBH_USER * cb,
                                  VOID *handle)
{
    STATUS status;
    STATUS internal_sts = NU_SUCCESS;
    USBH_USER_SESSION *session_entry;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(handle);

    /* Lock */
    status = _NU_USBH_USER_Lock (cb);
    if (status == NU_SUCCESS)
    {
        /* Find the session corresponding to this device handle */
        session_entry = USBH_USER_Find_Session ((NU_USBH_USER *) cb, handle);

        /* if valid session */
        if (session_entry != NU_NULL)
        {
            /* If safe removal is in progress fail the Open call */
            if (session_entry->trying_to_remove == NU_TRUE)
            {
                internal_sts = _NU_USBH_USER_Unlock (cb);
                status = NU_NOT_PRESENT;
            }
            else
            {
                /* If this is the first open grab the exclusive lock */
                if (session_entry->reference_count == 1)
                    internal_sts =
                    NU_Obtain_Semaphore (&session_entry->exclusive_lock, NU_SUSPEND);

                /* Increment the reference count for this device */
                session_entry->reference_count++;

                status = _NU_USBH_USER_Unlock (cb);
            }
        }
        else
        {
            /* No session corresponding to device handle is found. */
            internal_sts = _NU_USBH_USER_Unlock (cb);
            status = NU_USB_INVLD_ARG;
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ((status == NU_SUCCESS) ? internal_sts : status);
}

/*************************************************************************
* FUNCTION
*        _NU_USBH_USER_Close_Device
*
* DESCRIPTION
*
*       This function is called by the NU_USBH_USER's users.
* The applications can call this routine when the application has
* finished using the device. This decrement the reference count of
* the device. If safe removal is in progress and reference count reaches
* zero this task wakes up the task which is blocked on safe removal.
*
* INPUTS
*       cb      pointer to USER control block.
*       handle  handle/cookie for the concerned device.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*       NU_USB_INVLD_ARG    Indicates some pointer became stale before call
*                           gets completed.
*       NU_NOT_PRESENT      Indicates device has been safely removed.
*
*************************************************************************/
STATUS _NU_USBH_USER_Close_Device (NU_USBH_USER * cb,
                                   VOID *handle)
{
    STATUS status = NU_SUCCESS;
    STATUS internal_sts = NU_SUCCESS;
    USBH_USER_SESSION *session_entry;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(handle);

    /* Lock */
    status = _NU_USBH_USER_Lock (cb);
    if ( status == NU_SUCCESS )
    {
        session_entry = USBH_USER_Find_Session ((NU_USBH_USER *) cb, handle);

        /* if valid session */
        if (session_entry != NU_NULL)
        {
            /* Decrement the reference count for this device */
            session_entry->reference_count--;

            if (session_entry->reference_count < 1)
            {
                internal_sts = _NU_USBH_USER_Unlock (cb);
                status = NU_NOT_PRESENT;
            }
            else
            {
                /* If this is last close on the device release the exclusive lock
                 */
                if (session_entry->reference_count == 1)
                {
                    internal_sts =_NU_USBH_USER_Unlock (cb);
                    status = NU_Release_Semaphore (&session_entry->exclusive_lock);
                }
                else
                    status = _NU_USBH_USER_Unlock (cb);
            }
        }
        else
        {
            internal_sts =_NU_USBH_USER_Unlock (cb);
            status = NU_USB_INVLD_ARG;
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ((status == NU_SUCCESS) ? internal_sts : status);
}

/*************************************************************************
* FUNCTION
*        _NU_USBH_USER_Remove_Device
*
* DESCRIPTION
*       This function is used to remove a device safely from the subsystem
*       ensuring data consistency. Thread calling this function
*       suspends/timeout depending on the suspension option provided until
*       all the threads communicating with the device are completed.
*       Subsequent threads can not start using the device when safe removal
*       is in progress.
*
* INPUTS
*       cb      pointer to User control block.
*       handle  handle of the device to be removed safely.
*       suspend Suspension option.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*       NU_USB_INVLD_ARG    Some pointer became stale before call
*                           completion.
*
*************************************************************************/
STATUS _NU_USBH_USER_Remove_Device (NU_USBH_USER * cb,
                                    VOID *handle,
                                    UNSIGNED suspend)
{
    STATUS status;
    STATUS internal_sts = NU_SUCCESS;
    USBH_USER_SESSION *session_entry;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(cb);
    NU_USB_PTRCHK_RETURN(handle);

    /* Lock */
    status = _NU_USBH_USER_Lock (cb);
    if (status == NU_SUCCESS)
    {
        session_entry = USBH_USER_Find_Session ((NU_USBH_USER *) cb, handle);

        /* if valid session */
        if (session_entry != NU_NULL)
        {
            /* Set the state and grab exclusive lock */
            session_entry->trying_to_remove = NU_TRUE;
            internal_sts = _NU_USBH_USER_Unlock (cb);
            status = NU_Obtain_Semaphore (&session_entry->exclusive_lock, suspend);
        }
        else
        {
            /* No session corresponding found */
            internal_sts = _NU_USBH_USER_Unlock (cb);
            status = NU_USB_INVLD_ARG;
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ((status == NU_SUCCESS) ? internal_sts : status);
}

/*************************************************************************
* FUNCTION
*        NU_USBH_USER_Get_Drvr
*
* DESCRIPTION
*
*       This function returns the class driver CB associated with the 'handle'.
*       This function is meant for use by the extenders of the Host User driver.
*
* INPUTS
*       cb      pointer to USER control block.
*       handle  handle/cookie for the concerned device.
*       drvr_out    Location where the Class driver CB pointer will be stored.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*       NU_USB_INVLD_ARG    Indicates some pointer became stale before call
*                           gets completed.
*
*************************************************************************/
STATUS NU_USBH_USER_Get_Drvr (NU_USBH_USER * user,
                              VOID *handle,
                              NU_USB_DRVR **drvr_out)
{
    STATUS status = NU_USB_INVLD_ARG;
    STATUS internal_sts = NU_SUCCESS;
    USBH_USER_SESSION *session_entry;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_USB_PTRCHK_RETURN(user);
    NU_USB_PTRCHK_RETURN(handle);
    NU_USB_PTRCHK_RETURN(drvr_out);

    status = _NU_USBH_USER_Lock (user);

    if(status == NU_SUCCESS)
    {
        session_entry = USBH_USER_Find_Session ( user, handle);

        /* if valid session */
        if (session_entry != NU_NULL)
        {
            status = NU_SUCCESS;
            *drvr_out = session_entry->drvr;
        }
        internal_sts = _NU_USBH_USER_Unlock (user);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return ((status == NU_SUCCESS) ? internal_sts : status);
}

/************************************************************************/

#endif /* USBH_USER_EXT_C */
/* ======================  End Of File  =============================== */
