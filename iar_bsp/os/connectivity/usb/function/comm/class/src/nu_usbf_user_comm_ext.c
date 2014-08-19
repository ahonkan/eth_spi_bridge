/*************************************************************************
*
*               Copyright 2005 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       nu_usbf_user_comm_ext.c
*
* COMPONENT
*
*       Nucleus USB Software: Function Communication driver
*
* DESCRIPTION
*
*       This file provides the implementation of external interfaces of
*       Base Class USER_COMM (USER_COMM Layer).
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       _NU_USBF_USER_COMM_Create           Protected constructor API to be
*                                           used by extenders.
*       NU_USBF_USER_COMM_DATA_Connect      Base data_connect callback
*                                           behavior.
*       NU_USBF_USER_COMM_DATA_Discon       Base data_disconnect callback
*                                           behavior.
*       _NU_USBF_USER_COMM_Delete           Deletes a USER_COMM Driver.
*       _NU_USBF_USER_COMM_DATA_Connect     Base data_connect callback
*                                           behavior.
*       _NU_USBF_USER_COMM_DATA_Discon      Base data_disconnect callback
*                                           behavior.
*       _NU_USBF_USER_COMM_Connect          Base data_connect callback
*                                           behavior.
*       _NU_USBF_USER_COMM_Disconnect       Base data_disconnect callback
*                                           behavior.
*       NU_USBF_USER_COMM_Wait              Initialization wait routine.
*       _NU_USBF_USER_COMM_Wait             Initialization wait routine.
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions
*
*************************************************************************/

/************************************************************************/
#include "connectivity/nu_usb.h"
/************************************************************************/

/*************************************************************************
* FUNCTION
*       _NU_USBF_USER_COMM_Create
*
* DESCRIPTION
*
*       Protected Call used by extenders of USER_COMM to create the base
*       USER_COMM control block.
*
* INPUTS
*       cb                                  USER_COMM Control Block.
*       name                                Name of Instance.
*       bInterfaceSubclass                  Subclass this USER_COMM serves.
*       bInterfaceProtocol                  Protocol this USER_COMM serves.
*       reqrd_data                          Data interface is present or
*                                           not
*       dispatch                            Dispatch table.
*
* OUTPUTS
*       NU_SUCCESS                          Successful completion.
*
*************************************************************************/
STATUS  _NU_USBF_USER_COMM_Create (NU_USBF_USER_COMM * cb,
                            CHAR * name,
                            UINT8 bInterfaceSubclass,
                            UINT8 bInterfaceProtocol,
                            BOOLEAN reqrd_data,
                            const VOID *dispatch)
{
    STATUS status;

    /*
     * Switching to supervisor mode.
     */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Call the base driver service. */
    status = _NU_USBF_USER_Create ((NU_USBF_USER *) cb, name,
                             bInterfaceSubclass, bInterfaceProtocol,
                             dispatch);
    if (status == NU_SUCCESS)
    {
        /* Create event group for initializations. */
        status = NU_Create_Event_Group(&cb->device_init_event,
                        "DevEvent");
        if (status == NU_SUCCESS)
        {
            /* Create semaphore for protecting COMM user. */
            status = NU_Create_Semaphore(&cb->comm_lock,
                                  "COMMLCK", 1, NU_FIFO);
            if (status == NU_SUCCESS)
            {
                /* Reset data. */
                cb->mng_drvr = NU_NULL;
                cb->data_drvr = NU_NULL;
                cb->handle = NU_NULL;
                cb->require_data_intf = reqrd_data;
            }
        }
    }

    /*
     * Switching back to user mode.
     */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_USER_COMM_DATA_Connect
*
* DESCRIPTION
*
*       This is connect callback function is called by Data interface
*       class driver in its Initialize interface.
*
* INPUTS
*
*       cb                                  USER_COMM's Control Block
*       class_driver                        Driver control block calling
*                                           the callback.
*       handle                              context information.
*
* OUTPUTS
*
*       NU_SUCCESS                          Successful completion.
*
*************************************************************************/
STATUS  NU_USBF_USER_COMM_DATA_Connect (NU_USB_USER * cb,
                            NU_USB_DRVR * class_driver,
                            VOID *handle)
{
    STATUS status;


    /*
     * Switching to supervisor mode.
     */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Call the function from dispatch table */
    status = ((NU_USBF_USER_COMM_DISPATCH *) (((NU_USB *) cb)->
            usb_dispatch))->DATA_Connect (cb, class_driver, handle);

    /*
     * Switching back to user mode.
     */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_USER_COMM_DATA_Discon
*
* DESCRIPTION
*
*       DATA_Disconnect callback called by class driver when a device
*       being served by this USER_COMM is data_disconnected across bus.
*
* INPUTS
*
*       cb                                  USER_COMM's Control Block
*       class_driver                        Driver control block calling
*                                           the callback.
*       handle                              context information.
*
* OUTPUTS
*       NU_SUCCESS                          Successful completion.
*
*************************************************************************/
STATUS  NU_USBF_USER_COMM_DATA_Discon (NU_USB_USER * cb,
                               NU_USB_DRVR * class_driver,
                               VOID *handle)
{
    STATUS status;

    /*
     * Switching to supervisor mode.
     */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Call the function from dispatch table */
    status = ((NU_USBF_USER_COMM_DISPATCH *) (((NU_USB *) cb)->
            usb_dispatch))->DATA_Disconnect (cb, class_driver, handle);

    /*
     * Switching back to user mode.
     */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       _NU_USBF_USER_COMM_Delete
*
* DESCRIPTION
*
*       This function deletes a specified USER_COMM driver.
*
* INPUTS
*
*       cb                                  pointer to USER_COMM driver
*                                           control block.
*
* OUTPUTS
*
*       NU_SUCCESS                          USER_COMM driver deleted
*                                           successfully
*
*************************************************************************/
STATUS  _NU_USBF_USER_COMM_Delete (VOID *cb)
{

    STATUS status;

    /*
     * Switching to supervisor mode.
     */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    status = _NU_USBF_USER_Delete (cb);
    if (status == NU_SUCCESS)
    {
        /* Delete event group used for initializations. */
        status = NU_Delete_Event_Group(&((NU_USBF_USER_COMM *)
                                    cb)->device_init_event);
        if (status == NU_SUCCESS)
        {
            /* Delete semaphore used for protecting COMM user. */
            status = NU_Delete_Semaphore(&((NU_USBF_USER_COMM *)cb)->
                                        comm_lock);
        }
    }

    /*
     * Switching back to user mode.
     */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       _NU_USBF_USER_COMM_DATA_Connect
*
* DESCRIPTION
*
*       This is connect callback function is called by Data interface
*       class driver in its Initialize interface.
*
* INPUTS
*
*       cb                                  USER_COMM's Control Block
*       class_driver                        Driver control block calling
*                                           the callback.
*       handle                              context information.
*
* OUTPUTS
*
*       NU_SUCCESS                          Successful completion.
*
*************************************************************************/
STATUS  _NU_USBF_USER_COMM_DATA_Connect (NU_USB_USER * cb,
                            NU_USB_DRVR * class_driver,
                            VOID *handle)
{

    STATUS status;
    NU_USBF_USER_COMM *user;

    /*
     * Switching to supervisor mode.
     */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    user = (NU_USBF_USER_COMM *)cb;
    NU_ASSERT(cb);
    NU_ASSERT(class_driver);
    user->data_drvr = (NU_USBF_DRVR *)class_driver;

    /* Set event for data interface initialization. */
    status = NU_Set_Events(&user->device_init_event,
                COMMF_DATA_INIT,
                NU_OR);

    /*
     * Switching back to user mode.
     */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
* FUNCTION
*
*       _NU_USBF_USER_COMM_DATA_Discon
*
* DESCRIPTION
*
*       DATA_Disconnect callback called by class driver when a device
*       being served by this USER_COMM is data_disconnected across bus.
*
* INPUTS
*
*       cb                                  USER_COMM's Control Block
*       class_driver                        Driver control block calling
*                                           the callback.
*       handle                              context information.
*
* OUTPUTS
*
*       NU_SUCCESS                          Successful completion.
*
*************************************************************************/
STATUS  _NU_USBF_USER_COMM_DATA_Discon (NU_USB_USER * cb,
                               NU_USB_DRVR * class_driver,
                               VOID *handle)
{
    STATUS status;

    /*
     * Switching to supervisor mode.
     */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_ASSERT(cb);
    NU_ASSERT(class_driver);
    ((NU_USBF_USER_COMM *)cb)->data_drvr = (NU_USBF_DRVR *)NU_NULL;

    /* Now clear the event set by DATA_Connect. */
    status = NU_Set_Events(&((NU_USBF_USER_COMM *)cb)->device_init_event,
                                                ~COMMF_DATA_INIT, NU_AND);

    /*
     * Switching back to user mode.
     */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       _NU_USBF_USER_COMM_Connect
*
* DESCRIPTION
*
*       DATA_Disconnect callback called by class driver when a device
*       being served by this USER_COMM is data_disconnected across bus.
*
* INPUTS
*
*       cb                                  USER_COMM's Control Block
*       class_driver                        Driver control block calling
*                                           the callback.
*       handle                              context information.
*
* OUTPUTS
*
*       NU_SUCCESS                          Successful completion.
*
*************************************************************************/
STATUS  _NU_USBF_USER_COMM_Connect (NU_USB_USER * cb,
                                   NU_USB_DRVR * class_driver,
                                   VOID *handle)
{
    STATUS status;
    NU_USBF_USER_COMM *user;

    /*
     * Switching to supervisor mode.
     */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    user = (NU_USBF_USER_COMM *)cb;
    NU_ASSERT(cb);
    NU_ASSERT(class_driver);
    user->mng_drvr = (NU_USBF_DRVR *)class_driver;
    user->handle = handle;

    /* Set the event associated with COMM interface initialization. */
    status = NU_Set_Events(&user->device_init_event, COMMF_MNG_INIT,
                NU_OR);

    /*
     * Switching back to user mode.
     */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       _NU_USBF_USER_COMM_Disconnect
*
* DESCRIPTION
*
*       DATA_Disconnect callback called by class driver when a device
*       being served by this USER_COMM is data_disconnected across bus.
*
* INPUTS
*
*       cb                                  USER_COMM's Control Block
*       class_driver                        Driver control block calling
*                                           the callback.
*       handle                              context information.
*
* OUTPUTS
*
*       NU_SUCCESS                          Successful completion.
*
*************************************************************************/
STATUS  _NU_USBF_USER_COMM_Disconnect (NU_USB_USER * cb,
                                      NU_USB_DRVR * class_driver,
                                      VOID *handle)
{
    STATUS status;

    /*
     * Switching to supervisor mode.
     */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    NU_ASSERT(cb);
    NU_ASSERT(class_driver);
    ((NU_USBF_USER_COMM *)cb)->mng_drvr = NU_NULL;

    /* Now clear the event set the Connect callback. */
    status = NU_Set_Events(&((NU_USBF_USER_COMM *)cb)->device_init_event,
                                                    ~COMMF_MNG_INIT,
                                                    NU_AND);

    /*
     * Switching back to user mode.
     */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_USER_COMM_Wait
*
* DESCRIPTION
*
*       This function provides services for user level threads to wait for
*       a particular device to be connected. Thread goes into a state
*       specified by suspension option if the device is not yet connected.
*       This service returns a device handle as output when the call is
*       successful, which can be used by applications to use other
*       services of USER layer. When a thread first calls this service, it
*       checks if some device belonging to the user is already connected
*       and gives out its handle, else waits for device to be connected.
*       If this service is called again from the same thread, It checks
*       for the next available device and waits if it is not yet
*       connected. This can helps applications in traversing the devices
*       to find a suitable device.
*
* INPUTS
*
*       cb                                  pointer to User control block.
*       suspend                             Suspension option.
*       handle_out                          pointer to memory location to
*                                           hold pointer to the device
*                                           handle.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*       NU_TIMEOUT                          Indicates timeout on suspension
*       NU_NOT_PRESENT                      Indicates event flags are not
*                                           present.
*       NU_USB_INTERNAL_ERROR               Indicates a internal error in
*                                           USB subsystem
*
*************************************************************************/
STATUS  NU_USBF_USER_COMM_Wait (NU_USBF_USER_COMM * cb,
                           UNSIGNED suspend,
                           VOID **handle_out)
{
    STATUS status;

    /*
     * Switching to supervisor mode.
     */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Call the function through dispatch table. */
    status = ((NU_USBF_USER_COMM_DISPATCH *) (((NU_USB *) cb)->
            usb_dispatch))->Wait (cb, suspend, handle_out);

    /*
     * Switching back to user mode.
     */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_USER_COMM_Wait
*
* DESCRIPTION
*
*       This function provides services for user level threads to wait for
*       a particular device to be connected. Thread goes into a state
*       specified by suspension option if the device is not yet connected.
*       This service returns a device handle as output when the call is
*       successful, which can be used by applications to use other
*       services of USER layer. When a thread first calls this service, it
*       checks if some device belonging to the user is already connected
*       and gives out its handle, else waits for device to be connected.
*       If this service is called again from the same thread, It checks
*       for the next available device and waits if it is not yet
*       connected. This can helps applications in traversing the devices
*       to find a suitable device.
*
* INPUTS
*
*       cb                                  pointer to User control block.
*       suspend                             Suspension option.
*       handle_out                          pointer to memory location to
*                                           hold pointer to the device
*                                           handle.
*
* OUTPUTS
*
*       NU_SUCCESS                          Successful completion.
*       NU_TIMEOUT                          Indicates timeout on suspension
*       NU_NOT_PRESENT                      Indicates event flags are not
*                                           present.
*       NU_USB_INTERNAL_ERROR               Indicates a internal error in
*                                           USB subsystem
*
*************************************************************************/
STATUS  _NU_USBF_USER_COMM_Wait (NU_USBF_USER_COMM * cb,
                           UNSIGNED suspend,
                           VOID **handle_out)
{
    STATUS status;
    UINT32 event;
    UINT32 requested ;

    /*
     * Switching to supervisor mode.
     */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    requested = 0;
    NU_ASSERT(cb);

    /* Obtain the COMM lock. */
    status = NU_Obtain_Semaphore(&cb->comm_lock, suspend);
    if (status == NU_SUCCESS)
    {
        /* Check whether data interface should be present along with COMM.
         */
        if (cb->require_data_intf)
        {
            requested = COMMF_MNG_DATA_INIT;
        }
        else
        {
            requested = COMMF_MNG_INIT;
        }

        /* Wait for the connect of required interfaces. */
        status = NU_Retrieve_Events(&cb->device_init_event, requested,
                                NU_AND_CONSUME, &event, suspend);
        if (status == NU_SUCCESS)
        {
            /* Update the received handle. */
            NU_ASSERT(cb->handle);
            *handle_out = cb->handle;
            status = NU_Release_Semaphore(&cb->comm_lock);
        }
        else
        {
            (VOID)NU_Release_Semaphore(&cb->comm_lock);
        }
    }

    /*
     * Switching back to user mode.
     */
    NU_USER_MODE();

    return (status);
}

/* ====================== End of File ================================= */

