/**************************************************************************
*
*               Copyright 2005  Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
* FILE NAME
*
*       nu_usbh_com_user_ext.c
*
*
* COMPONENT
*
*       Nucleus USB software : Communication user driver.
*
* DESCRIPTION
*
*       This file contains the external interfaces exposed by user Base
*       driver.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       _NU_USBH_COM_USER_Create     Initializes the control block
*                                    of Communication user driver.
*       _NU_USBH_COM_USER_Delete     Deletes an instance of
*                                    Communication user driver.
*       NU_USBH_COM_USER_Wait        Wait for a new device to be
*                                    connected.
*       NU_USBH_COM_USER_Send_Data   Sends data to a Communication device.
*       NU_USBH_COM_USER_Get_Data    Get data from a Communication device.
*       NU_USBH_COM_USER_Register    Registers Communication user driver
*                                    with the Communication class driver.
*       NU_USBH_COM_USER_Start_Polling  Starts polling the Communication
*                                       device for incoming data.
*       NU_USBH_COM_USER_Stop_Polling   Stops polling the Communication
*                                       device for incoming data.
*       NU_USBH_COM_USER_Create_Polling Creates polling process for the
*                                       Communication device for incoming
*                                       data.
*       NU_USBH_COM_USER_Delete_Polling Deletes polling process for the
*                                       Communication device for incoming
*                                       data.
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions.
*
**************************************************************************/

/* =====================  USB Include Files ===========================  */
#include "connectivity/nu_usb.h"

/* ==========================  Functions =============================== */

/**************************************************************************
* FUNCTION
*     _NU_USBH_COM_USER_Create
*
* DESCRIPTION
*     Communication user driver initialization routine
*
* INPUTS
*     pcb_user_drvr    Pointer to driver control block.
*     p_name           Name of this USB object.
*     p_memory_pool    Memory pool to be used by user driver.
*     subclass         Subclass code for registration criteria.
*     p_handler        Pointer to application's structure for handlers.
*     usbh_com_user_dispatch     Pointer to user driver dispatch table.
*
* OUTPUTS
*     NU_SUCCESS            Indicates successful completion.
*     NU_INVALID_SEMAPHORE  Indicates control block is invalid.
*
**************************************************************************/

STATUS _NU_USBH_COM_USER_Create(
       NU_USBH_COM_USER*          pcb_user_drvr,
       CHAR*                      p_name,
       NU_MEMORY_POOL*            p_memory_pool,
       UINT8                      subclass,
       NU_USBH_COM_USER_HDL*      p_handler,
       NU_USBH_COM_USER_DISPATCH* usbh_com_user_dispatch)

{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    pcb_user_drvr->p_hndl_table = p_handler;

    /* Create base component. */
    status = _NU_USBH_USER_Create ((NU_USBH_USER *) pcb_user_drvr,
                                   p_name,
                                   p_memory_pool,
                                   subclass,
                                   0x00,
                                   usbh_com_user_dispatch);

    /* Revert to user mode. */
    NU_USER_MODE();

    return(status);
}

/**************************************************************************
* FUNCTION
*     _NU_USBH_COM_USER_Delete
*
* DESCRIPTION
*     This function deletes an instance of Communication user driver.
*
* INPUTS
*     pcb_user_drvr       Pointer to the USB Object control block.
*
* OUTPUTS
*     NU_SUCCESS          Indicates successful completion.
*     NU_INVALID_POINTER  Indicates control block is invalid
*
**************************************************************************/
STATUS _NU_USBH_COM_USER_Delete (VOID* pcb_user_drvr)
{
    STATUS status;
    /* Deleting base component. */
    status = _NU_USBH_USER_Delete (pcb_user_drvr);
    return status;
}

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_USER_Wait
*
* DESCRIPTION
*     This function provides services for user level threads to wait for
*     a particular device to be connected. Thread goes into a state
*     specified by suspension option if the device is not yet connected.
*     This service returns a device handle as output when the call is
*     successful, which can be used by applications to use other services
*     of this USER driver. When a thread first calls this service, it
*     checks if some device belonging to the user is already connected and
*     gives out its handle, else waits for device to be connected. If this
*     service is called again from the same thread, It checks for the
*     next available device and waits if it is not yet connected. This
*     can help applications in traversing the devices to find a suitable
*     device.
*
* INPUTS
*     pcb_user_drvr     Pointer to user control block.
*     suspend           Suspension option.
*     handle_out        Pointer to memory location to hold pointer to the
*                       device handle.
*
* OUTPUTS
*     NU_SUCCESS            Indicates successful completion.
*     NU_TIMEOUT            Indicates  timeout on suspension.
*     NU_NOT_PRESENT        Indicates event flags are not present.
*     NU_USB_INTERNAL_ERROR Indicates an internal error in USB subsystem.
*
**************************************************************************/

STATUS NU_USBH_COM_USER_Wait (
       NU_USBH_COM_USER* pcb_user_drvr,
       UNSIGNED          suspend,
       VOID**            handle_out)
{
    STATUS status = NU_SUCCESS;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Just calling the base behavior. */
    status = _NU_USBH_USER_Wait((NU_USBH_USER*)pcb_user_drvr,
                                 suspend,
                                 handle_out);

     /* Revert to user mode. */
    NU_USER_MODE();

    return status;
}

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_USER_Send_Data
*
* DESCRIPTION
*     This function is used to send packet data on the Communication device.
*
* INPUTS
*     pcb_curr_device     Pointer to Selected Communication device
*     p_frame_data        Pointer to start of frame data.
*     frame_size          Frame Size.
*
* OUTPUTS
*     NU_SUCCESS          Indicates successful completion.
*     NU_USB_INVLD_ARG    Some pointer became stale before call
*                         completion.
*
**************************************************************************/

STATUS NU_USBH_COM_USER_Send_Data (
       NU_USBH_COM_USR_DEVICE* pcb_curr_device,
       UINT8*                  p_frame_data,
       UINT32                  frame_size)
{

   STATUS status ;
   NU_USBH_COM_XBLOCK xblock;

   /* Switch to supervisor mode. */
   NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

   if(pcb_curr_device == NU_NULL)
   {
       NU_USER_MODE();
       return  (NU_USB_INVLD_ARG);
   }

   xblock.p_data_buf  = (VOID* )p_frame_data;
   xblock.data_length = frame_size;
   xblock.direction   = NU_USBH_COM_DATA_OUT;

   status = NU_USBH_COM_Transfer( pcb_curr_device->usb_device,
                                  &xblock);

   /* Revert to user mode. */
   NU_USER_MODE();

   return (status);
}

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_USER_Get_Data
*
* DESCRIPTION
*     This function is used to get control data from Communication device.
*
* INPUTS
*     pcb_curr_device     Pointer to Selected Communication device
*     p_frame_data        Pointer to start of buffer to hold frame data.
*     frame_size          Frame Size.
*
* OUTPUTS
*     NU_SUCCESS           Indicates successful completion.
*     NU_USB_INVLD_ARG     Some pointer became stale before call
*                          completion.
*
*
**************************************************************************/

STATUS NU_USBH_COM_USER_Get_Data (
       NU_USBH_COM_USR_DEVICE* pcb_curr_device,
       UINT8*                  p_frame_data,
       UINT32                  frame_size,
       UINT32*                 actual_size)
{

   STATUS status ;
   NU_USBH_COM_XBLOCK xblock;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

   xblock.p_data_buf  = (VOID* )p_frame_data;
   xblock.data_length = frame_size;
   xblock.direction   = NU_USBH_COM_DATA_IN;

   status = NU_USBH_COM_Transfer(pcb_curr_device->usb_device,
                                 &xblock);
   *actual_size = xblock.transfer_length;

    /* Revert to user mode. */
    NU_USER_MODE();

   return (status);
}

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_USER_Register
*
* DESCRIPTION
*     This function is used to register the user driver with the class
*     driver.
*
* INPUTS
*     pcb_user_drvr           Pointer to Communication user driver.
*     pcb_com_drvr            Pointer to Communication class driver.
*
* OUTPUTS
*     NU_SUCCESS         Indicates successful completion.
*     NU_USB_INVLD_ARG   Some pointer became stale before call
*                        completion.
*
**************************************************************************/

STATUS NU_USBH_COM_USER_Register (
       NU_USBH_COM_USER* pcb_user_drvr,
       NU_USBH_COM*      pcb_com_drvr)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    pcb_user_drvr->pcb_class_drvr = pcb_com_drvr;
    status = NU_USB_DRVR_Register_User ((NU_USB_DRVR *)pcb_com_drvr,
                                        (NU_USB_USER *)pcb_user_drvr);

    /* Revert to user mode. */
    NU_USER_MODE();

    return status;

}

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_USER_Start_Polling
*
* DESCRIPTION
*     This function is called by the application whenever it wants to
*     receive data from modem or to start automatic receiving.
*
* INPUTS
*     pcb_curr_device       Pointer to Modem control block.
*
* OUTPUTS
*
**************************************************************************/

STATUS NU_USBH_COM_USER_Start_Polling (NU_USBH_COM_USR_DEVICE* device)
{

    return( NU_Resume_Task(device->data_poll_task));
}

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_USER_Stop_Polling
*
* DESCRIPTION
*     This function is called by the application whenever it no longer
*     wants to receive any data from modem or to stop automatic receiving.
*
* INPUTS
*     pcb_curr_device       Pointer to Modem control block.
*
* OUTPUTS
*
**************************************************************************/
STATUS NU_USBH_COM_USER_Stop_Polling (NU_USBH_COM_USR_DEVICE* device)
{
    STATUS status ;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    status = NU_USB_PIPE_Flush(device->usb_device->pcb_bulk_in_pipe);

    status |= NU_Obtain_Semaphore(&(device->poll_task_sync),
                                 (NU_PLUS_Ticks_Per_Second * 6));

    NU_Suspend_Task(device->data_poll_task);

    status |= NU_Release_Semaphore(&(device->poll_task_sync));
    /* Revert to user mode. */
    NU_USER_MODE();

    return status;
}

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_USER_Create_Polling
*
* DESCRIPTION
*     This function is called by the application to create the required
*     mechanism to automatically poll the device for incoming data.
*
* INPUTS
*     pcb_curr_device       Pointer to Modem control block.
*
* OUTPUTS
*
**************************************************************************/
STATUS NU_USBH_COM_USER_Create_Polling(
       NU_USBH_COM_USER*       pcb_user_drvr,
       NU_USBH_COM_USR_DEVICE* pcb_user_device)
{

    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    status = USB_Allocate_Object(sizeof (NU_TASK),
                                 (VOID**)&pcb_user_device->data_poll_task);
    if(pcb_user_device->data_poll_task != NU_NULL)
    {
        memset (pcb_user_device->data_poll_task,
                0,
                sizeof (NU_TASK));
    }

    if(status == NU_SUCCESS)
    {
        status = USB_Allocate_Memory(USB_MEM_TYPE_CACHED,
                                     DATA_POLL_STACK_SIZE,
                                     (VOID **)&(pcb_user_device->data_poll_stack));
    }

    if ( status == NU_SUCCESS )
    {
        pcb_user_device->com_user = pcb_user_drvr;
        status = NU_Create_Task(pcb_user_device->data_poll_task,
                                "DATAPOLL",
                                NU_USBH_COM_User_Poll_Data,
                                (UINT32)pcb_user_drvr,
                                pcb_user_device,
                                pcb_user_device->data_poll_stack,
                                DATA_POLL_STACK_SIZE,
                                DATA_POLL_TASK_PRIORITY,
                                0,
                                NU_PREEMPT,
                                NU_NO_START);
    }

    if ( status == NU_SUCCESS )
    {
        status = NU_Create_Semaphore(&(pcb_user_device->poll_task_sync),
                                     "COMPOLLTSK",1, NU_FIFO);
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return status;
}

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_MDM_Delete_Polling
*
* DESCRIPTION
*     This function is called by the application whenever it no longer
*     wants to receive any data from modem or to stop automatic receiving.
*
* INPUTS
*     pcb_curr_device       Pointer to Modem control block.
*
* OUTPUTS
*
**************************************************************************/
STATUS NU_USBH_COM_USER_Delete_Polling(NU_USBH_COM_USR_DEVICE* device)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    status = NU_Terminate_Task(device->data_poll_task);
    status = NU_Delete_Task(device->data_poll_task);

    status = NU_Delete_Semaphore(&(device->poll_task_sync));

    status = USB_Deallocate_Memory (device->data_poll_task);
    status = USB_Deallocate_Memory (device->data_poll_stack);

    /* Revert to user mode. */
    NU_USER_MODE();

    return status;
}

/**************************************************************************
* FUNCTION
*
*       NU_USBH_COM_USER_Suspend_Device.
*
* DESCRIPTION
*
*       Suspends a connected storage device.
*
* INPUTS
*
*       session                             Pointer to user device
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*
**************************************************************************/

STATUS NU_USBH_COM_USER_Suspend_Device (VOID *session)
{
    STATUS status = NU_USB_INVLD_ARG;
    NU_USBH_COM_USR_DEVICE *currDrive = NU_NULL;
    NU_USB_PTRCHK(session);

    currDrive = (NU_USBH_COM_USR_DEVICE*)session;

    status = NU_USBH_STACK_Suspend_Device( (NU_USBH_STACK*)currDrive->usb_device->pcb_stack,
                                         currDrive->usb_device->pcb_device);

    return (status);
}

/**************************************************************************
* FUNCTION
*
*       NU_USBH_COM_USER_Resume_Device.
*
* DESCRIPTION
*
*       Resume a suspended storage device.
*
* INPUTS
*
*       session                             Pointer to user device
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*
**************************************************************************/


STATUS NU_USBH_COM_USER_Resume_Device (VOID *session)
{
    STATUS status = NU_USB_INVLD_ARG;
    NU_USBH_COM_USR_DEVICE *currDrive = NU_NULL;
    NU_USB_PTRCHK(session);

    currDrive = (NU_USBH_COM_USR_DEVICE*)session;

    status = NU_USBH_STACK_Resume_Device( (NU_USBH_STACK*)currDrive->usb_device->pcb_stack,
                                         currDrive->usb_device->pcb_device);

    return (status);
}


