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
*       nu_usbf_comm_ext.c
*
* COMPONENT
*
*       Nucleus USB Function Software : Communication Class Driver
*
* DESCRIPTION
*
*       This file contains the external Interfaces exposed by Communication
*       Class Driver.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*       NU_USBF_COMM_Init                   Initialize Communication Class
*                                           driver in NMI
*       NU_USBF_COMM_Init_GetHandle         Get handle to the Communication
*                                           class driver
*       NU_USBF_COMM_Create                 Creates an instance of
*                                           Communication Class Driver.
*       _NU_USBF_COMM_Delete                Deletes an instance of
*                                           Communication Class Driver.
*       NU_USBF_COMM_Send_Notification      Communication notification
*                                           processing.
*       _NU_USBF_COMM_Disconnect            Disconnect event handler for
*                                           the device.
*       _NU_USBF_COMM_Initialize_Intf       Connection event handler for
*                                           the interface.
*       _NU_USBF_COMM_New_Transfer          New transfer handling.
*       _NU_USBF_COMM_New_Setup             Class specific request handling
*       _NU_USBF_COMM_Set_Intf              New alternate setting
*                                           processing.
*       _NU_USBF_COMM_Notify                USB Events processing.
*       NU_USBF_COMM_Cancel_Io              Cancels ongoing IO on
*                                           communication interface.
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions
*       reg_api.h
*       runlevel_init.h
*
************************************************************************/

/* ==============  USB Include Files =================================  */
#include "connectivity/nu_usb.h"
#include "services/reg_api.h"
#include "services/runlevel_init.h"

char USBF_COMM_Configuration_String[NU_USB_MAX_STRING_LEN];
char USBF_COMM_Interface_String[NU_USB_MAX_STRING_LEN];

/* ==========================  Functions ============================== */

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_COMM_Init
*
* DESCRIPTION
*
*       Communication Class Driver Initialization
*
* INPUTS
*
*       path                                Registry path of component.
*       startstop                           Flag to find if component is
*                                           being enabled or disabled.
*
* OUTPUTS
*
*       NU_SUCCESS               Successful Initialization.
*       NU_USB_INVLD_ARG         Some input argument(s) are
*                                invalid.
*
*************************************************************************/
STATUS nu_os_conn_usb_func_comm_class_init(CHAR *path, INT startstop)
{
    VOID   *stack_handle = NU_NULL;
    UINT8   rollback = 0;
    STATUS  status = NU_SUCCESS, internal_sts = NU_SUCCESS, reg_status;
    CHAR    usb_func_comm_path[80];

    usb_func_comm_path[0] = '\0';
    strcat(usb_func_comm_path, path);

    /* Save registry settings of USB Function Communication Class. */
    strcat(usb_func_comm_path, "/configstring");
    reg_status = REG_Get_String(usb_func_comm_path, USBF_COMM_Configuration_String, NU_USB_MAX_STRING_LEN);
    if(reg_status == NU_SUCCESS)
    {
        usb_func_comm_path[0] = '\0';
        strcat(usb_func_comm_path, path);
        strcat(usb_func_comm_path, "/interfacestring");
        reg_status = REG_Get_String(usb_func_comm_path, USBF_COMM_Interface_String, NU_USB_MAX_STRING_LEN);
    }

    /* First initialize communication data driver. */
    internal_sts = nu_os_conn_usb_func_comm_data_init(path, startstop);

    if (startstop == RUNLEVEL_START)
    {
        /* Allocate Memory for Communication Class Driver. */
         status = USB_Allocate_Object(sizeof(NU_USBF_COMM),
                                      (VOID**)&NU_USBF_COMM_Cb_Pt);
        if(status != NU_SUCCESS)
        {
            rollback = 1;
        }

        if (!rollback)
        {
            /*Zero out the allocated memory*/
            memset(NU_USBF_COMM_Cb_Pt, 0, sizeof(NU_USBF_COMM));

            /* In following API call, passing memory pool ptr parameter
             * NU_NULL because in ReadyStart memory in USB system is
             * allocated through USB specific memory APIs, not directly
             * with any given memory pool pointer. This parameter remains
             * only for backwards code compatibility. */
            status = NU_USBF_COMM_Create(NU_USBF_COMM_Cb_Pt,
                                         "USBF-COMM",
                                         NU_NULL);
            if(status != NU_SUCCESS)
            {
                rollback = 2;
            }
        }

        /* Get the function stack handle */
        if (!rollback)
        {
            status = NU_USBF_Init_GetHandle (&stack_handle);
            if(status != NU_SUCCESS)
            {
                rollback = 3;
            }
        }

        /* Register Function COMM Driver to the stack */
        if (!rollback)
        {
            status = NU_USBF_STACK_Register_Drvr ((NU_USBF_STACK *) stack_handle,
                                                 (NU_USB_DRVR  *) NU_USBF_COMM_Cb_Pt);
            if(status != NU_SUCCESS)
            {
                rollback = 3;
            }
        }

        /* Clean up in case error occurs. */
        switch (rollback)
        {
            case 3:
                internal_sts = _NU_USBF_COMM_Delete ((VOID *) NU_USBF_COMM_Cb_Pt);

            case 2:
                if (NU_USBF_COMM_Cb_Pt)
                {
                    internal_sts |= USB_Deallocate_Memory((VOID *)NU_USBF_COMM_Cb_Pt);
                    NU_USBF_COMM_Cb_Pt = NU_NULL;
                }

            case 1:
            case 0:
            /* internal_sts is not used after this. So to remove
             * KW and PC-Lint warning set it as unused variable.
             */
            NU_UNUSED_PARAM(internal_sts);
        }
    }
    else if (startstop == RUNLEVEL_STOP)
    {
        NU_USBF_Init_GetHandle (&stack_handle);

        if(stack_handle)
        {
            NU_USB_STACK_Deregister_Drvr(stack_handle, (NU_USB_DRVR*)NU_USBF_COMM_Cb_Pt);
            _NU_USBF_COMM_Delete(NU_USBF_COMM_Cb_Pt);
            USB_Deallocate_Memory((VOID *)NU_USBF_COMM_Cb_Pt);
        }

        status = NU_SUCCESS;
    }

    return status;
}

/*************************************************************************
*   FUNCTION
*
*       NU_USBF_COMM_Init_GetHandle
*
*   DESCRIPTION
*
*       This function is called to retrieve the function Communication
*  class driver's address.
*
*   INPUTS
*
*       handle          Double pointer used to retrieve the class
*                           driver's address.
*
*   OUTPUTS
*
*       NU_SUCCESS          Indicate there exists a Function Communication
*       class driver.
*       NU_NOT_PRESENT      Indicate that handle does not exist
*
*************************************************************************/
STATUS  NU_USBF_COMM_Init_GetHandle (VOID **handle)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    status = NU_SUCCESS;
    *handle = NU_USBF_COMM_Cb_Pt;
    if (NU_USBF_COMM_Cb_Pt == NU_NULL)
    {
        status = NU_NOT_PRESENT;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return status;
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_COMM_Create
*
* DESCRIPTION
*
*       Communication Class Driver initialization routine
*
* INPUTS
*
*       cb                                  Pointer to Driver control block
*       name                                Name of this USB object.
*       pool                                Pointer to driver's memory pool
*
* OUTPUTS
*
*       NU_SUCCESS                          Successful Initialization.
*       NU_USB_INVLD_ARG                    Some input argument(s) are
*                                           invalid.
*
*************************************************************************/
STATUS  NU_USBF_COMM_Create (NU_USBF_COMM *cb, CHAR *name,
                            NU_MEMORY_POOL *pool)
{
    STATUS status;
    INT i;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Check if input arguments are valid.*/
    NU_USB_MEMPOOLCHK_RETURN(pool);
    if (cb == NU_NULL)
    {
        status = NU_USB_INVLD_ARG;
    }
    else
    {
        /* Reset the control block */
        memset((VOID *)cb, 0, sizeof(NU_USBF_COMM));

        /* Disable all devices. */
        for (i = 0; i < COMMF_MAX_DEVICES; i++)
        {
            cb->devices[i].is_valid = 0;
        }

        cb->mem_pool = pool;
        {
            /* Call the parent's create function. */
            status = _NU_USBF_DRVR_Create (&cb->parent, name,
                               (USB_MATCH_CLASS), 0, 0, 0, 0,
                               (COMMF_CLASS), 0, 0,
                               &usbf_comm_dispatch);
        }
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*      _NU_USBF_COMM_Delete
*
* DESCRIPTION
*
*      This function deletes an instance of Communication Class driver.
*
* INPUTS
*
*      cb                                   Pointer to the USB Object
*                                           control block.
*
* OUTPUTS
*
*      NU_SUCCESS                           Indicates successful completion
*
*************************************************************************/
STATUS  _NU_USBF_COMM_Delete (VOID *cb)
{
    STATUS status;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Call the base driver's delete function. */
    status = _NU_USBF_DRVR_Delete(cb);

    /* Revert to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_COMM_Send_Notification
*
* DESCRIPTION
*
*       This function sends the notification of communication events to
*       USB Host for multiple function communication devices.
*
* INPUTS
*
*       cb                                  Pointer to driver control block
*       notif                               Pointer to notification
*                                           structure.
*       handle                              Handle for this notification.
*
* OUTPUTS
*
*       NU_SUCCESS                          Successful completion.
*       NU_USB_INVLD_ARG                    Some input argument(s) are
*                                           invalid.
*       NU_USB_NOT_SUPPORTED                Notification is not supported
*                                           by this device.
*
*************************************************************************/
STATUS  NU_USBF_COMM_Send_Notification(NU_USBF_DRVR *cb,
                                      USBF_COMM_USER_NOTIFICATION *notif,
                                      VOID *handle)
{
    USBF_COMM_DEVICE *device;
    USBF_COMM_NOTIF_PKT *notification;
    NU_USB_IRP *irp;
    UINT8   intf_num;
    STATUS status;
    UINT8 *temp_buff = NU_NULL;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    status = NU_SUCCESS;
    NU_ASSERT(cb);
    device = (USBF_COMM_DEVICE *)handle;
    notification = &device->curr_notification;

    /* Error condition, if communication device is not properly
     * initialized.
     */
    if (device->is_valid == NU_FALSE)
    {
        status = NU_USB_INVLD_ARG;
    }
    else
    {
        /* Return error, if notification pipe is not available. */
        if (device->notif_pipe == (NU_USB_PIPE *)NU_NULL)
        {
            status = NU_USB_NOT_SUPPORTED;
        }
        else
        {
            irp = &(device->intr_irp);

            /* Now, fill the 8 byte header for notification data. */

            /* bmRequestType is always 0xA1 in notification header. */
            notification->bmRequestType = COMMF_NOTIF_REQ_TYPE;
            notification->bNotification = notif->notification;
            notification->wValue = HOST_2_LE16(notif->notif_value);
            status = NU_USB_INTF_Get_Intf_Num(device->master_intf,
                                                               &intf_num);
            notification->wIndex = HOST_2_LE16((UINT16)intf_num);
            notification->wLength = HOST_2_LE16(notif->length);

            /* If notification has associated data, copy the data in the
             * packet.
             */
            if ((notif->length > 0) && (notif->length <= COMMF_MAX_NOTIF_DATA_SIZE))
            {
                memcpy (notification->notif_data, notif->data,
                                                           notif->length);
            }
            /* Create uncached buffer for I/O. */
            status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                         COMMF_NOTIF_HDR_SIZE+notif->length,
                                         (VOID **)&temp_buff);
            if (status == NU_SUCCESS)
            {
                memcpy(temp_buff, notification, COMMF_NOTIF_HDR_SIZE+notif->length);

                status = NU_USB_IRP_Create(irp,
                                    COMMF_NOTIF_HDR_SIZE+notif->length,
                                    temp_buff,
                                    NU_TRUE,
                                    NU_FALSE,
                                    COMMF_Notify_IRP_Complete,
                                    device,
                                    0);
            }

            /* Submit the IRP. */
            if(status == NU_SUCCESS)
            {
                status = NU_USB_PIPE_Submit_IRP (device->notif_pipe, irp);
            }
        }
    }
    /* Deallocate uncached memory on IRP failure. */
    if (status != NU_SUCCESS && temp_buff)
    {
        USB_Deallocate_Memory(temp_buff);
    }


    /* Revert to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       _NU_USBF_COMM_Initialize_Intf
*
* DESCRIPTION
*
*       Connect notification function invoked by stack when driver is
*       given an opportunity to own an Interface
*
* INPUTS
*
*       cb                                  Pointer to Class Driver Control
*                                           Block.
*       stk                                 Pointer to Stack Control Block
*                                           of the calling stack.
*       dev                                 Pointer to Device Control Block
*                                           of the device found.
*       intf                                Pointer to Interface control
*                                           Block to be served by this
*                                           class driver.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*                                           of the service.
*       NU_USB_INVLD_ARG                    Some input argument(s) are
*                                           invalid.
*       NU_USB_MAX_EXCEEDED                 Some value is exceeded the
*                                           allowed limit.
*
*************************************************************************/
STATUS  _NU_USBF_COMM_Initialize_Intf (NU_USB_DRVR * cb,
                                        NU_USB_STACK * stk,
                                        NU_USB_DEVICE * dev,
                                        NU_USB_INTF * intf)
{
    BOOLEAN isclaimed;
    NU_USB_DRVR *old;
    STATUS status;
    NU_USBF_COMM *comm;
    USBF_COMM_DEVICE *device = NU_NULL;
    INT i;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    comm = (NU_USBF_COMM *)cb;
    status = NU_USB_INVLD_ARG;

    /* If connection is received for first time, allocate a new device. */
    /* Find empty slot. */
    for (i = 0; i < COMMF_MAX_DEVICES; i++)
    {
        device = &comm->devices[i];
        if (device->is_valid == NU_FALSE)
        {
            status = NU_SUCCESS;
            break;
        }
    }

    if (i == COMMF_MAX_DEVICES)
    {
        status = NU_USB_MAX_EXCEEDED;
    }

    if (status == NU_SUCCESS)
    {
        memset(device, 0, sizeof(USBF_COMM_DEVICE));
        device->is_valid = 1;

        /* Initialize the device structure for the received values. */
        device->dev = dev;
        device->master_intf = intf;

        device->user = NU_NULL;
        device->mng_drvr = (NU_USBF_DRVR *) comm;
    }

    if(status == NU_SUCCESS)
    {
        status = COMMF_Init_Internals(device);
    }

    if(status == NU_SUCCESS)
    {
        status = COMMF_Set_Event(device,
                                 COMMF_RX_BUFF_GRP_MX);
    }

    if (status == NU_SUCCESS)
    {
        /* Invoke set_intf handler with default alt_settg. */
        status = _NU_USBF_COMM_Set_Intf ((NU_USB_DRVR *) comm,
                             stk,
                             device->dev,
                             device->master_intf,
                             &device->master_intf->alt_settg[0]);
    }

    if (status == NU_SUCCESS)
    {
        /* Release the interface if already claimed. */
        status = NU_USB_INTF_Get_Is_Claimed(intf, &isclaimed, &old);

        if(status == NU_SUCCESS)
        {
            if (isclaimed)
            {
                status = NU_USB_INTF_Release(intf, old);
            }
        }

        /* Claim the interface. */
        status = NU_USB_INTF_Claim (intf, cb);
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       _NU_USBF_COMM_Disconnect
*
* DESCRIPTION
*
*       Disconnect Callback function, invoked by stack when a
*       Device/Interface served by Communication is removed from the BUS.
*
* INPUTS
*
*       cb                                  Pointer to Class Driver Control
*                                           Block claimed this Interface.
*       stk                                 Pointer to Stack Control Block.
*       dev                                 Pointer to NU_USB_DEVICE
*                                           Control Block disconnected.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates successful completion
*                                           of the service.
*       NU_NOT_PRESENT                      Some required resource is not
*                                           found.
*
*************************************************************************/
STATUS  _NU_USBF_COMM_Disconnect(
        NU_USB_DRVR * cb,
        NU_USB_STACK * stack,
        NU_USB_DEVICE * dev)
{
    STATUS status;
    NU_USBF_COMM *comm;
    USBF_COMM_DEVICE *device = NU_NULL;

    INT i;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    status = NU_SUCCESS;

    comm = (NU_USBF_COMM *) cb;
    /* Find the device for which disconnect is received. */
    for (i = 0; i < COMMF_MAX_DEVICES; i++)
    {
        device = &comm->devices[i];
        if ((device->is_valid) && (device->dev == dev))
        {
            break;
        }
    }

    if (i == COMMF_MAX_DEVICES)
    {
        status = NU_NOT_PRESENT;
    }

    if(status == NU_SUCCESS)
    {
        /* Release the interface. */
        status = NU_USB_INTF_Release (device->master_intf,
                             (NU_USB_DRVR *) comm);

        if (device->user != NU_NULL)
        {
            /* Update the status. */
            status = NU_USB_USER_Disconnect (device->user,
                                   (NU_USB_DRVR*)comm, device);
            if(status == NU_SUCCESS)
            {
                if (device->notif_pipe)
                {
                    status = NU_USB_PIPE_Flush(device->notif_pipe);
                }

                if (device->ctrl_pipe)
                {
                    status = NU_USB_PIPE_Flush(device->ctrl_pipe);
                }
            }
        }
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       _NU_USBF_COMM_Set_Intf
*
* DESCRIPTION
*
*       Notifies driver of change in alternate setting.
*
* INPUTS
*
*       cb                                  Class driver for which this
*                                           callback is sent
*       stack                               Stack invoking this function
*       device                              Device on which this event has
*                                           happened
*       intf                                Interface which is affected
*       alt_settg                           New alternate setting for the
*                                           interface
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the class driver
*                                           could process the event.
*                                           successfully.
*       NU_USB_INVLD_ARG                    Indicates the one of the input
*                                           parameters has been incorrect
*                                           AND/OR the event could not be
*                                           processed without an error.
*       NU_NOT_PRESENT                      Some required resource is not
*                                           found.
*
*************************************************************************/
STATUS  _NU_USBF_COMM_Set_Intf (NU_USB_DRVR * cb,
                              NU_USB_STACK * stack,
                              NU_USB_DEVICE * dev,
                              NU_USB_INTF * intf,
                              NU_USB_ALT_SETTG * alt_settg)
{
    STATUS status;
    INT i;
    USBF_COMM_DEVICE *device = NU_NULL;
    NU_USBF_COMM *comm;
    NU_USB_PIPE *pipe;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    comm = (NU_USBF_COMM *) cb;

    /* Find device, for which callback is received. */
    for (i = 0; i < COMMF_MAX_DEVICES; i++)
    {
        device = &comm->devices[i];
        if ((device->is_valid) && (device->dev == dev) &&
                                 (device->master_intf == intf))
        {
            break;
        }
    }
    if (i == COMMF_MAX_DEVICES)
    {
        status = NU_NOT_PRESENT;
    }
    else
    {
        device->master_alt_settg = alt_settg;

        /* Find the default pipe */
        status = NU_USB_ALT_SETTG_Find_Pipe (alt_settg,
                                            USB_MATCH_EP_ADDRESS,
                                            0,
                                            0,
                                            0,
                                            &pipe);
        if (status == NU_SUCCESS)
        {
            device->ctrl_pipe = pipe;
            device->notif_pipe = NU_NULL;
            /* Find the Interrupt IN  pipe  */
            status = NU_USB_ALT_SETTG_Find_Pipe (alt_settg,
                                                 USB_MATCH_EP_TYPE |
                                                 USB_MATCH_EP_DIRECTION, 0,
                                                 USB_DIR_IN, USB_EP_INTR,
                                                 &pipe);

            /* If interrupt endpoint is present, then do the relevant
             * initialization.
             */
            if (status == NU_SUCCESS)
            {
                device->notif_pipe = pipe;
                status = COMMF_Mng_Connect_User(device);
            }
        }
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       _NU_USBF_COMM_New_Setup
*
* DESCRIPTION
*
*       Processes a new class specific SETUP packet from the Host.
*
*       The format of the setup packet is defined by the corresponding
*       class specification / custom protocol. The setup packet is
*       validated by this function and processed further as per the class
*       specification.
*
*       If there is any data phase associated with the setup request,
*       then the NU_USB_Submit_Irp function can be used to submit the
*       transfer to the stack. Status phase is automatically handled by
*       the Stack. If there is no data transfer associated with the
*       command, then no transfer is submitted.
*
*       For unknown and unsupported command, this function returns
*       appropriate error status. If this function returns any status
*       other than NU_SUCCESS, then the default endpoint will be stalled.
*
* INPUTS
*
*       cb                                  Class driver for which this
*                                           callback is meant for.
*       stack                               Stack invoking this function
*       device                              Device on which this event has
*                                           happened
*       setup                               the 8 byte setup packet
*                                           originating from the Host
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the class driver
*                                           could process the event
*                                           successfully.
*       NU_USB_INVLD_ARG                    Indicates the one of the input
*                                           parameters has been incorrect
*                                           AND/OR the event could not be
*                                           processed without an error.
*       NU_NOT_PRESENT                      Some required resource is not
*                                           found.
*       NU_USB_NOT_SUPPORTED                Current request is not
*                                           supported.
*
*************************************************************************/
STATUS  _NU_USBF_COMM_New_Setup(NU_USB_DRVR * cb,
                               NU_USB_STACK * stack,
                               NU_USB_DEVICE * dev,
                               NU_USB_SETUP_PKT * setup)
{
    USBF_COMM_DEVICE *device = NU_NULL;
    NU_USBF_COMM *comm;
    NU_USB_IRP *irp;
    STATUS status;
    USBF_COMM_USER_CMD *cmd;
    UINT32 rem_length;
    UINT32 requested_length;
    UINT8 *buffer;
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
    UINT16 mps0;
#else
    UINT8 mps0;
#endif
    UINT8 intf_num;
    BOOLEAN use_empty_pkt;
    UINT8 *temp_buff = NU_NULL;

    INT i;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    comm = (NU_USBF_COMM *) cb;
    use_empty_pkt = NU_FALSE;
    status = NU_SUCCESS;

    /* Check for the device who is recipient for this setup. */
    for (i = 0;
        (i < COMMF_MAX_DEVICES) &&
        (status == NU_SUCCESS);
        i++)
    {
        device = &comm->devices[i];
        if ((device->is_valid) && (device->dev == dev))
        {
            status = NU_USB_INTF_Get_Intf_Num(
                        device->master_intf,
                        &intf_num);

            if (intf_num == (UINT8)setup->wIndex)
            {
                break;
            }
        }
    }

    if(status == NU_SUCCESS)
    {
        if (i == COMMF_MAX_DEVICES)
        {
            status = NU_NOT_PRESENT;
        }
    }

    if(status == NU_SUCCESS)
    {
        cmd = &device->curr_command;
        /* Retrieve the client associated with this interface. */

        if (device->user == NU_NULL)
        {
            status = NU_USB_INVLD_ARG;
        }
        else
        {
            /* Initialize command structure for user. */
            cmd->command = setup->bRequest;
            cmd->cmd_value = setup->wValue;
            cmd->cmd_index = setup->wIndex;
            irp = &(device->ctrl_irp);

            /* if data direction is from host to device. */
            if (!(setup->bmRequestType & COMMF_SETUP_DIR))
            {
                /* If no data transfer is associated with setup packet. */
                if (setup->wLength == 0)
                {
                    cmd->data_len = 0;
                    cmd->cmd_data = (UINT8 *)NU_NULL;

                    status = COMMF_Handle_Cmd_Rcvd(
                        comm,
                        device,
                        COMMF_SET_CMD_RCVD);
                }
                /* If Data OUT phase is pending, submit request to receive
                 * data.
                 */
                else
                {
                    //cmd->cmd_data = device->cmd_buffer;
                    status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                                  setup->wLength,
                                                 (VOID **) &temp_buff);

                    if (status == NU_SUCCESS)
                    {
                        memcpy(temp_buff, device->cmd_buffer, setup->wLength);
                        status = NU_USB_IRP_Create(irp,
                                                setup->wLength,
                                                temp_buff,
                                                NU_TRUE,
                                                NU_FALSE,
                                                COMMF_Set_Cmd_IRP_Complete,
                                                device,
                                                0);
                    }
                    if(status == NU_SUCCESS)
                    {
                        status = NU_USB_PIPE_Submit_IRP
                                            (device->ctrl_pipe,
                                            irp);
                    }
                }
            }
            /* if data direction is from device to host. */
            else if (setup->bmRequestType & COMMF_SETUP_DIR)
            {
                requested_length = setup->wLength;
                cmd->data_len = 0;
                cmd->cmd_data = (UINT8 *)NU_NULL;
                /* Pass the command to user driver. */
                status = NU_USBF_USER_New_Command (device->user,
                                  device->mng_drvr,
                                  device,
                                  (UINT8 *)(&device->curr_command),
                                  sizeof(device->curr_command),
                                  &buffer,
                                  &rem_length);
                if (status == NU_SUCCESS)
                {
                    /* If the user driver want to send the data for
                     * the last command, Fill this data and submit
                     * the IRP.
                     */
                    if ((buffer != NU_NULL) && (rem_length > 0))
                    {
                        /* If data to be sent is multiple of maximum
                         * packet size, zero length should be sent
                         * as end of transfer.
                         */
                        if (requested_length <= rem_length)
                        {
                            rem_length = requested_length;
                        }
                        else
                        {
                            status = NU_USB_DEVICE_Get_bMaxPacketSize0(
                                        dev,
                                        &mps0);

                            if ((rem_length % mps0) == 0)
                            {
                                use_empty_pkt = NU_TRUE;
                            }
                        }

                        cmd->cmd_data = buffer;
                        cmd->data_len = (UINT16)rem_length;
                        /* Allocate uncached memory buffer. */
                        status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                                     cmd->data_len,
                                                     (VOID **) &temp_buff);
                        if (status == NU_SUCCESS)
                        {
                            memcpy(temp_buff, cmd->cmd_data, cmd->data_len);


                            status = NU_USB_IRP_Create(
                                                irp,
                                                cmd->data_len,
                                                temp_buff,
                                                NU_TRUE, use_empty_pkt,
                                                COMMF_Get_Cmd_IRP_Complete,
                                                device, 0);
                        }

                        if(status == NU_SUCCESS)
                        {
                            status = NU_USB_PIPE_Submit_IRP(
                                        device->ctrl_pipe,
                                        irp);
                        }
                    }
                    /* If Host wants data, but user driver is not
                     * sending it, means request is not supported.
                     */
                    else if (requested_length > 0)
                    {
                        status = NU_USB_NOT_SUPPORTED;
                    }
                }
            }
            else
            {
                status = NU_USB_INVLD_ARG;
            }
        }
    }
    /* Deallocate uncached memory on failure. */
    if (status != NU_SUCCESS && temp_buff)
    {
        USB_Deallocate_Memory(temp_buff);
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       _NU_USBF_COMM_New_Transfer
*
* DESCRIPTION
*
*       Processes the new transfer request from the Host.
*
*       If there is any data transfer pending with the class driver,
*       then the NU_USB_Submit_Irp function can be used to submit the
*       transfer to the stack. If there is no data transfer associated
*       with the command, then no transfer is submitted.
*
*       This function is never invoked on default control endpoint. Since
*       stack ignores the return value of this function, any required
*       error processing is carried out by the class driver itself.
*
* INPUTS
*
*       cb                                  Class driver for which this
*                                           callback is meant for.
*       stack                               Stack invoking this function
*       device                              Device on which this event has
*                                           happened.
*       pipe                                Pipe on which the data transfer
*                                           is initiated by the Host.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the class driver
*                                           could process the event
*                                           successfully.
*
*************************************************************************/
STATUS  _NU_USBF_COMM_New_Transfer(NU_USB_DRVR * cb,
                                  NU_USB_STACK * stack,
                                  NU_USB_DEVICE * device,
                                  NU_USB_PIPE * pipe)
{

    /* This function is not expected to be invoked because there is no OUT
     * endpoint in this interface.
     */
    return (NU_SUCCESS);

}

/*************************************************************************
*
* FUNCTION
*
*       _NU_USBF_COMM_Notify
*
* DESCRIPTION
*
*       Notifies Driver of USB Events.
*
* INPUTS
*
*       cb                                  Class driver for which this
*                                           callback is meant for.
*       stack                               Stack invoking this function.
*       dev                                 Device on which this event has
*                                           happened.
*       event                               USB event that has occurred.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that the class driver
*                                           could process the event
*                                           successfully.
*
*************************************************************************/
STATUS  _NU_USBF_COMM_Notify(NU_USB_DRVR * cb,
                            NU_USB_STACK * stack,
                            NU_USB_DEVICE * dev,
                            UINT32 event)
{
    NU_USBF_COMM *comm;
    USBF_COMM_DEVICE *device;
    STATUS status = NU_SUCCESS;
    UINT32 capability = 0;

    BOOLEAN skip_notification;

    INT i;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    comm = (NU_USBF_COMM *) cb;

    /* Check for the device who is recipient for this setup. */
    for (i = 0; i < COMMF_MAX_DEVICES; i++)
    {
        device = &comm->devices[i];
        if ((device->is_valid) &&
            (device->dev == dev))
        {
            skip_notification = NU_FALSE;

            if ((event == USBF_EVENT_RESET) ||
                (event == USBF_EVENT_DISCONNECT) ||
                (event == USBF_EVENT_CONNECT))
            {
                /* Flush all pipes. */
                if (device->notif_pipe)
                {
                    (VOID)NU_USB_PIPE_Flush(
                                device->notif_pipe);
                }

                if(status == NU_SUCCESS)
                {
                    if (device->ctrl_pipe)
                    {
                        (VOID)NU_USB_PIPE_Flush(
                                       device->ctrl_pipe);
                    }
                }

                if(status == NU_SUCCESS)
                {
                    if (device->in_pipe)
                    {
                        (VOID)NU_USB_PIPE_Flush(
                                        device->in_pipe);
                    }
                }

                if(status == NU_SUCCESS)
                {
                    (VOID)COMMF_Clear_Event(
                                device,
                                COMMF_TX_IN_PROGRESS);
                }
            }

            /*
             * See if underlying USB function controller handles
             * select_configuration's standard request implicitly.
             * If so, class driver's initialize_interface will not
             * be called upon arrival of such request; hence references
             * to pipes should not be re-initialized.
             */
            if(status == NU_SUCCESS)
            {
                status = NU_USBF_HW_Get_Capability (
                                ((NU_USBF_HW*)dev->hw),
                                &capability);
            }

            if (status == NU_SUCCESS)
            {
                /*
                 * If function controller does not handle SET_CFG
                 * implicitly.
                 */
                if ((capability & (0x01 << USB_SET_CONFIGURATION)) == 0)
                {
                    if ((event == USBF_EVENT_DISCONNECT) ||
                        (event == USBF_EVENT_RESET) ||
                        (event == USBF_EVENT_CONNECT))
                    {
                        status = NU_USBF_COMM_DATA_Dis_Reception(
                                    (NU_USBF_COMM_DATA *)device->data_drvr,
                                    device);

                        device->notif_pipe = NU_NULL;
                        device->ctrl_pipe = NU_NULL;
                        device->in_pipe = NU_NULL;
                        device->out_pipe = NU_NULL;
                    }
                }

                if ((capability & (0x01 << USB_SET_CONFIGURATION)) != 0)
                {
                    if (event == USBF_EVENT_DISCONNECT)
                    {
                        skip_notification = NU_TRUE;
                    }
                }

                if (skip_notification == NU_FALSE)
                {
                    /* Store the received event and activate HISR to pass
                     * this to user.
                     */
                    if (device->user != NU_NULL)
                    {
                        if ((event == USBF_EVENT_DISCONNECT) ||
                            (event == USBF_EVENT_RESET) ||
                            (event == USBF_EVENT_CONNECT))
                        {
                            /*
                             * See if underlying USB function controller
                             * handles select_configuration's standard
                             * request implicitly. If so, class driver's
                             * initialize_interface will not be called upon
                             *  arrival of such request; communication
                             * device should not be marked invalid.
                             */
                            status = NU_USBF_HW_Get_Capability (
                                            ((NU_USBF_HW*)device->dev->hw),
                                            &capability);

                            if (status == NU_SUCCESS)
                            {
                                /*
                                 * If function controller does not handle
                                 * SET_CFG implicitly.
                                 */
                                if ((capability &
                                    (0x01 << USB_SET_CONFIGURATION)) == 0)
                                {
                                    if(device->data_drvr)
                                    {
                                        status = COMMF_Uninit_Tx_Queue(
                                                device);

                                        if(status == NU_SUCCESS)
                                        {
                                            status = COMMF_Uninit_Internals(
                                                    device);
                                        }
                                    }

                                    /*
                                     * Mark the device as invalid.
                                     */
                                    if(status == NU_SUCCESS)
                                    {
                                        device->is_valid = NU_FALSE;
                                    }
                                }
                            }

                            if(status == NU_SUCCESS)
                            {
                                status = NU_USB_USER_Disconnect (
                                                    device->user,
                                                    (NU_USB_DRVR*)comm,
                                                    device);
                            }

                            if ((status == NU_SUCCESS) && (device->data_drvr))
                            {
                                status = NU_USBF_USER_COMM_DATA_Discon(
                                            device->user,
                                            device->data_drvr,
                                            device);
                            }
                        }

                        status = NU_USBF_USER_Notify (device->user,
                                            (NU_USB_DRVR*)comm,
                                            device, event);
                    }
                }
            }
        }
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_COMM_Cancel_Io
*
* DESCRIPTION
*
*       This function cancels all notification operations submitted to
*       communication interface of communication class driver.
*
* INPUTS
*
*       *cb                                 Pointer to the USB driver
*                                           control block.
*       *handle                             Identification handle for the
*                                           device.
*
* OUTPUTS
*
*       NU_SUCCESS                          Indicates that IO cancellation
*                                           is successful.
*       NU_USB_INVLD_ARG                    Indicates that in_pipe's
*                                           reference is not initialized
*                                           properly. Device is in
*                                           disconnected state.
*
*************************************************************************/
STATUS  NU_USBF_COMM_Cancel_Io(
            NU_USBF_COMM *cb,
            VOID * handle)
{
    STATUS status;

    USBF_COMM_DEVICE * device;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    if ((cb == NU_NULL) || (handle == NU_NULL) )
    {
        status = NU_USB_INVLD_ARG;
    }
    else
    {
        device = (USBF_COMM_DEVICE *)handle;

        if ((device->notif_pipe) != NU_NULL)
        {
            status = NU_USB_PIPE_Flush(device->notif_pipe);
        }

        else
        {
            status = NU_INVALID_POINTER;
        }
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return (status);
}

/* ======================  End Of File  =============================== */
