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
*       nu_usbf_comm_imp.c
*
* COMPONENT
*
*       Nucleus USB Function Software : Communication Class Driver
*
* DESCRIPTION
*
*       This file contains the internal implementation of Nucleus
*       USB Communication Interface class driver.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       COMMF_Notify_IRP_Complete           Notification Completion
*                                           Callback.
*       COMMF_Set_Cmd_IRP_Complete          Set Command transfer Callback.
*       COMMF_Get_Cmd_IRP_Complete          Get Command transfer Callback.
*       COMMF_Mng_Connect_User              User connect processing routine
*       COMMF_Handle_Mng_Init               Management interface
*                                           initialization.
*       COMMF_Handle_Cmd_Rcvd               Control command processing.
*       COMMF_Handle_Tx_Done                Transmission complete
*                                           processing.
*       COMMF_Init_Tx_Queue                 Initializes pending TX
*                                           transfers queue.
*       COMMF_Uninit_Tx_Queue               Un-initializes pending TX
*                                           transfers queue.
*       COMMF_Error_Line                    Error reporting utility
*                                           function.
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions
*
************************************************************************/

/* ==============  USB Include Files =================================  */
#include "connectivity/nu_usb.h"

/* ==========================  Functions ============================== */

/*************************************************************************
*
* FUNCTION
*
*       COMMF_Notify_IRP_Complete
*
* DESCRIPTION
*
*       Communication Class driver : Notification transfer completion
*                                     Callback
* INPUTS
*
*       irp                                 Completed Data transfer.
*       pipe                                Pipe of transfer.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID    COMMF_Notify_IRP_Complete (NU_USB_PIPE * pipe, NU_USB_IRP * irp)
{
    USBF_COMM_DEVICE *device = NU_NULL;
    STATUS status;
    STATUS irp_status;
	UINT8  *temp_buff = NU_NULL;

    status = NU_USB_IRP_Get_Status (irp, &irp_status);	

    if(status == NU_SUCCESS)
    {
        if (irp_status == NU_SUCCESS)
        {
            /* Get owner of this IRP. */
            status = NU_USB_IRP_Get_Context (irp,
                                (VOID **)&device);

            if(status == NU_SUCCESS)
            {
                status = COMMF_Handle_Tx_Done(
                            (NU_USBF_COMM*)device->mng_drvr,
                            device,
                            COMMF_NOTIF_SENT);
            }
        }
    }
    /* Deallocate uncached memory buffer. */
    NU_USB_IRP_Get_Data(irp, &temp_buff);
    if(temp_buff)
    {
    	USB_Deallocate_Memory(temp_buff);
    }

    return;
}

/*************************************************************************
*
* FUNCTION
*
*       COMMF_Set_Cmd_IRP_Complete
*
* DESCRIPTION
*
*       Communication Class driver : Set command transfer completion
*       Callback
*
* INPUTS
*
*       irp                                 Completed Data transfer
*       pipe                                Pipe of transfer
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID    COMMF_Set_Cmd_IRP_Complete (NU_USB_PIPE * pipe, NU_USB_IRP  *irp)
{
    USBF_COMM_DEVICE *device = NU_NULL;
    STATUS status;
    STATUS irp_status;
    NU_USBF_COMM *comm;
	UINT8 *temp_buff = NU_NULL;

    status = NU_USB_IRP_Get_Status (irp, &irp_status);
    if(status == NU_SUCCESS)
    {
        if (irp_status == NU_SUCCESS)
        {
            /* Get owner of this IRP. */
            status = NU_USB_IRP_Get_Context (irp, (VOID **) &device);

            if(status == NU_SUCCESS)
            {
                comm = (NU_USBF_COMM*)device->mng_drvr;

                status = COMMF_Handle_Cmd_Rcvd(
                            comm,
                            device,
                            COMMF_SET_CMD_RCVD);
            }
        }
    }

    /* Deallocate uncached memory buffer. */
    NU_USB_IRP_Get_Data(irp, &temp_buff);
    if (temp_buff)
    {
    	USB_Deallocate_Memory(temp_buff);
    }

    return;
}

/*************************************************************************
*
* FUNCTION
*
*       COMMF_Get_Cmd_IRP_Complete
*
* DESCRIPTION
*
*       Communication Class driver : Get command transfer completion
*       Callback
*
*
* INPUTS
*
*       irp                                 Completed Data transfer
*       pipe                                Pipe of transfer
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID    COMMF_Get_Cmd_IRP_Complete (NU_USB_PIPE * pipe, NU_USB_IRP  *irp)
{
    USBF_COMM_DEVICE *device = NU_NULL;
    STATUS status;
    STATUS irp_status;
    NU_USBF_COMM *comm;
    UINT8 *temp_buff = NU_NULL;

    status = NU_USB_IRP_Get_Status (irp, &irp_status);

    if(status == NU_SUCCESS)
    {
        if (irp_status == NU_SUCCESS)
        {
            /* Get owner of this IRP. */
            status = NU_USB_IRP_Get_Context (irp, (VOID **) &device);

            if(status == NU_SUCCESS)
            {
                comm = (NU_USBF_COMM*)device->mng_drvr;

                status = COMMF_Handle_Tx_Done(comm,
                                              device,
                                              COMMF_GET_CMD_SENT);
            }
        }
    }
    /* Deallocate uncached memory buffer. */
    NU_USB_IRP_Get_Data(irp, &temp_buff);
    if (temp_buff)
    {
      	USB_Deallocate_Memory(temp_buff);
    }

    return;
}

/*************************************************************************
*
* FUNCTION
*
*       COMMF_Mng_Connect_User
*
* DESCRIPTION
*
*       Communication Class driver : User connect sending routine.
*
* INPUTS
*
*       device                              Pointer to communication device
*
* OUTPUTS
*
*       NU_SUCCESS                          Successful completion.
*
*************************************************************************/
STATUS  COMMF_Mng_Connect_User(USBF_COMM_DEVICE *device)
{
    STATUS status = NU_SUCCESS;
    UNSIGNED i;
    UINT8 sub_class, intf_sub_class;
    UINT8 protocol = 0;
    UINT8 intf_protocol = 0;

    NU_USB_USER *user;
    NU_USB_USER *user_list[NU_USB_MAX_USERS];
    UNSIGNED num_users;
    NU_USBF_COMM *comm = (NU_USBF_COMM *)device->mng_drvr;

    /* Get the interface descriptor and find out who is the user. */
    /* Go through all users to find who should be owning this. */
    num_users = NU_USB_DRVR_Get_Users ((NU_USB_DRVR*)comm,
                                        &user_list[0],
                                   NU_USB_MAX_USERS);

    for (i = 0; i < num_users; i++)
    {
        if(i >= NU_USB_MAX_USERS)
		{
			break;
		}
		user = user_list[i];

        status = NU_USB_USER_Get_Subclass (user, &sub_class);
        if (status != NU_SUCCESS)
        {
            continue;
        }

        status = NU_USB_INTF_Get_SubClass (
                    device->master_intf,
                    &intf_sub_class);

        if(status == NU_SUCCESS)
        {
            status = NU_USB_USER_Get_Protocol(user, &protocol);
            if (status != NU_SUCCESS)
            {
                continue;
            }
        }

        if(status == NU_SUCCESS)
        {
            status = NU_USB_INTF_Get_Protocol(
                                device->master_intf,
                                &intf_protocol);
        }

        /* If both subclass/protocol codes of user and interface
         * matches then send connect user.
         */
        if(status == NU_SUCCESS)
        {
            if ((sub_class == intf_sub_class) &&
                (protocol == intf_protocol))
            {
                device->user = user;

                status = COMMF_Handle_Mng_Init(
                        comm,
                        device);

                break;
            }
        }
    }

    /* If user could not be found for this device, return error. */
    if(status == NU_SUCCESS)
    {
        if (i == num_users)
        {
            status = NU_NOT_PRESENT;
        }
    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       COMMF_Handle_Mng_Init
*
* DESCRIPTION
*
*       Communication Class driver : Management driver initialization in
*       HISR.
*
* INPUTS
*
*       comm                                Pointer to communication driver
*       device                              Pointer to communication device
*
* OUTPUTS
*
*       None                                Successful completion.
*
*************************************************************************/
STATUS COMMF_Handle_Mng_Init(NU_USBF_COMM *comm, USBF_COMM_DEVICE *device)
{
    STATUS status;

    /* Send connect to user. */
    status = NU_USB_USER_Connect (device->user,
                                  (NU_USB_DRVR*)comm,
                                  device);

    if (status != NU_SUCCESS)
    {
         device->user = NU_NULL;
    }

    return status;
}

/*************************************************************************
*
* FUNCTION
*
*       COMMF_Handle_Cmd_Rcvd
*
* DESCRIPTION
*
*       Communication Class driver : Control pipe command processing in
*       HISR.
*
* INPUTS
*
*       comm                                Pointer to communication driver
*       device                              Pointer to communication device
*
* OUTPUTS
*
*       NU_SUCCESS                          Successful completion.
*       NU_USB_INVLD_ARG                    Invalid state parameter
*                                           passed.
*
*************************************************************************/
STATUS  COMMF_Handle_Cmd_Rcvd(NU_USBF_COMM *comm,
        USBF_COMM_DEVICE *device,
        UINT8 state)
{
    USBF_COMM_USER_CMD *cmd;
    UINT32 actual_length;
    UINT8 *buffer;
    NU_USB_IRP *irp;
    STATUS status;

    cmd = &device->curr_command;
    irp = &device->ctrl_irp;
    /* If transfer is associated, get its results. */
    if (state == COMMF_SET_CMD_RCVD)
    {
        status = NU_USB_IRP_Get_Actual_Length (irp, &actual_length);

        if(status == NU_SUCCESS)
        {
            cmd->data_len = (UINT16)actual_length;
            status = NU_USB_IRP_Get_Data (irp, &buffer);
            cmd->cmd_data = buffer;
        }
    }

    else
    {
        status = NU_USB_INVLD_ARG;
    }

    /* Pass this command to user. */
    if(status == NU_SUCCESS)
    {
        status = NU_USBF_USER_New_Command (device->user,
                   device->mng_drvr,
                   device,
                   (UINT8 *)cmd,
                   sizeof(USBF_COMM_USER_CMD),
                   &buffer,
                   &actual_length);
    }

    return status;
}

/*************************************************************************
*
* FUNCTION
*
*       COMMF_Handle_Cmd_Rcvd
*
* DESCRIPTION
*
*       Communication Class driver : Transmission complete processing in
*       HISR.
*
* INPUTS
*
*       comm                                Pointer to communication driver
*       device                              Pointer to communication device
*
* OUTPUTS
*
*       None                                Successful completion.
*
*************************************************************************/
STATUS    COMMF_Handle_Tx_Done(
                NU_USBF_COMM *comm,
                USBF_COMM_DEVICE *device,
                UINT8 state)
{
    UINT32 actual_length = 0;
    UINT8 *buffer = NU_NULL;
    NU_USB_IRP *irp;
    NU_USBF_DRVR *drvr = NU_NULL;
    COMMF_CMPL_CTX ctx;

    STATUS status;

    /* Get the IRP for completed transfer. */
    if (state == COMMF_NOTIF_SENT)
    {
        irp = &device->intr_irp;
        status = NU_USB_IRP_Get_Actual_Length (irp, &actual_length);

        if(status == NU_SUCCESS)
        {
            status = NU_USB_IRP_Get_Data (irp, &buffer);
        }
    }

    else if (state == COMMF_GET_CMD_SENT)
    {
        irp = &device->ctrl_irp;

        status = NU_USB_IRP_Get_Actual_Length (irp, &actual_length);

        if(status == NU_SUCCESS)
        {
            status = NU_USB_IRP_Get_Data (irp, &buffer);
        }
    }

    else if (state == COMMF_DATA_SENT)
    {
        irp = &device->in_irp;
        actual_length = device->tx_data_len;
        buffer = device->tx_data;
    }

    else
    {
        irp = NU_NULL;
        COMMF_Error_Line(__FILE__,
                         __LINE__,
                         NU_USB_NOT_SUPPORTED);
    }

    status = NU_USB_IRP_Get_Status(irp, &(ctx.status));
    ctx.transfer_type = state;
	ctx.handle = (VOID *)device;

    if(status == NU_SUCCESS)
    {
        if (state == COMMF_DATA_SENT)
        {
            drvr = device->data_drvr;
        }

        else
        {
            drvr = device->mng_drvr;
        }
    }

    if (status == NU_SUCCESS)
    {
        /* Pass notification to user. */
        status = NU_USBF_USER_Tx_Done (
                              device->user,
                              drvr,
                              (VOID*)&ctx,
                              buffer,
                              actual_length,
                              NU_NULL,
                              NU_NULL);
    }

    return status;
}

/*************************************************************************
*
* FUNCTION
*
*       COMMF_Init_Tx_Queue
*
* DESCRIPTION
*
*       This function initializes pending TX transfers queue.
*
* INPUTS
*
*       comm                                Pointer to communication driver
*       device                              Pointer to communication device
*
* OUTPUTS
*
*       STATUS                              Status of internal operations.
*
*************************************************************************/
STATUS    COMMF_Init_Tx_Queue(USBF_COMM_DEVICE *device)
{
    STATUS status;
    UINT32 element_size;

    element_size = sizeof(COMMF_TRANSFER)/sizeof(UNSIGNED);

    memset(&(device->pending_tx_queue), 0, sizeof(NU_QUEUE));

    status = NU_Create_Queue(&(device->pending_tx_queue),
                             "COMMFPTX",
                             device->pending_tx_queue_buffer,
                             COMMF_PENDING_QUEUE_COUNT,
                             NU_FIXED_SIZE,
                             element_size,
                             NU_FIFO);

    return status;
}

/*************************************************************************
*
* FUNCTION
*
*       COMMF_Uninit_Tx_Queue
*
* DESCRIPTION
*
*       This function cancels all the pending pending TX transfers and
*       un-initializes the queue.
*
* INPUTS
*
*       comm                                Pointer to communication driver
*       device                              Pointer to communication device
*
* OUTPUTS
*
*       STATUS                              Status of internal operations.
*
*************************************************************************/
STATUS    COMMF_Uninit_Tx_Queue(USBF_COMM_DEVICE *device)
{
    STATUS status;
    COMMF_CMPL_CTX ctx;
    VOID * buffer = NU_NULL;
    UINT32 length = 0;

    do
    {
        status = COMMF_Dequeue_Tx_Transfer(device,
                                           &buffer,
                                           &length);

        if(status == NU_SUCCESS)
        {
            ctx.status = NU_USB_IRP_CANCELLED;
            ctx.transfer_type = COMMF_DATA_SENT;
			ctx.handle = (VOID*)device;

            /* Pass notification to user. */
            (VOID)NU_USBF_USER_Tx_Done (
                                      device->user,
                                      device->data_drvr,
                                      (VOID*)&ctx,
                                      buffer,
                                      0,
                                      NU_NULL,
                                      NU_NULL);
        }
    }while(status == NU_SUCCESS);

    status = NU_Delete_Queue(&(device->pending_tx_queue));

    return status;
}

/*************************************************************************
*
* PRIVATE FUNCTION
*
*       COMMF_Error_Line
*
* DESCRIPTION
*
*       This function is responsible for retaining filename and the line
*       number information about the system error occurred from within.
*       this driver.
*
* INPUTS
*
*       filename                            Name of file in which
*                                           error occurred.
*       lineno                              Line number in the source code
*                                           on which error occurred.
*       errorcode                           Code identifying type of error
*                                           occurred.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID   COMMF_Error_Line(
       CHAR * filename,
       UINT32 lineno,
       STATUS errorcode)
{
    CHAR * pfilename = filename;
    INT nLineno = lineno;
    STATUS errCode = errorcode;

    for ( ; ; )
    {
        filename = pfilename;
        lineno = nLineno;
        errorcode = errCode;
    }
}

/*************************************************************************
*
* PRIVATE FUNCTION
*
*       COMMF_Init_Internals
*
* DESCRIPTION
*
*       Initialize internal components.
*
* INPUTS
*
*       device                              Pointer to COMM device's
*                                           control block.
*
* OUTPUTS
*
*       NU_SUCCESS                          Internal components has
*                                           successfully been initialized.
*
*************************************************************************/
STATUS COMMF_Init_Internals(
            USBF_COMM_DEVICE * device)
{
    STATUS status;

    memset(&(device->internal_events), 0, sizeof(NU_EVENT_GROUP));

    status = NU_Create_Event_Group(
                    &(device->internal_events),
                    "COMMFEVT");

    if(status == NU_SUCCESS)
    {
        status = COMMF_Clear_Event(
                    device,
                    0xffffffff);
    }

    return status;
}

/*************************************************************************
*
* PRIVATE FUNCTION
*
*       COMMF_Uninit_Internals
*
* DESCRIPTION
*
*       Un-initialize internal components.
*
* INPUTS
*
*       device                              Pointer to COMM device's
*                                           control block.
*
* OUTPUTS
*
*       NU_SUCCESS                          Internal components have
*                                           successfully been
*                                           un-initialized.
*
*************************************************************************/
STATUS COMMF_Uninit_Internals(
            USBF_COMM_DEVICE * device)
{
    STATUS status;

    status = NU_Delete_Event_Group(
                    &(device->internal_events));
    

    return status;
}

/*************************************************************************
*
* PRIVATE FUNCTION
*
*       COMMF_Clear_Event
*
* DESCRIPTION
*
*       Clear internal event of the internal_events event group of the
*       device.
*
* INPUTS
*
*       device                              Pointer to COMM device's
*                                           control block.
*       events                              Bitmap of event codes to be
*                                           cleared.
*
* OUTPUTS
*
*       NU_SUCCESS                          Events have successfully been
*                                           cleared.
*
*************************************************************************/
STATUS COMMF_Clear_Event(
            USBF_COMM_DEVICE * device,
            UINT32 events)
{
    STATUS status;

    status = NU_Set_Events(&(device->internal_events),
                           (~events),
                           NU_AND);

    return status;
}

/*************************************************************************
*
* PRIVATE FUNCTION
*
*       COMMF_Set_Event
*
* DESCRIPTION
*
*       Set internal event of the internal_events event group of the
*       device.
*
* INPUTS
*
*       device                              Pointer to COMM device's
*                                           control block.
*       events                              Bitmap of event codes to be
*                                           cleared.
*
* OUTPUTS
*
*       NU_SUCCESS                          Events have successfully been
*                                           set.
*
*************************************************************************/
STATUS COMMF_Set_Event(
            USBF_COMM_DEVICE * device,
            UINT32 events)
{
    STATUS status;

    status = NU_Set_Events(&(device->internal_events),
                           events,
                           NU_OR);

    return status;
}

/*************************************************************************
*
* PRIVATE FUNCTION
*
*       COMMF_Get_Event
*
* DESCRIPTION
*
*       Set internal event of the internal_events event group of the
*       device.
*
* INPUTS
*
*       device                              Pointer to COMM device's
*                                           control block.
*       events                              Bitmap of event codes to be
*                                           cleared.
*       bAvailable                          Pointer to location where
*                                           current status of events
*                                           is stored.
*                                           Set to NU_TRUE, if all
*                                           all events are present
*                                           and set to NU_FALSE, if
*                                           any of the events is
*                                           not present.
*       suspend                             Suspension time-out for
*                                           blocking operations.
*       operation                           Operation to be performed
*                                           on events.
*
* OUTPUTS
*
*       NU_SUCCESS                          Status of events have
*                                           successfully been retrieved.
*
*************************************************************************/
STATUS COMMF_Get_Event(
            USBF_COMM_DEVICE * device,
            UINT32 events,
            BOOLEAN * bAvailable,
            OPTION suspend,
            OPTION operation)
{
    STATUS status;
    UNSIGNED ret_flags;

    status = NU_Retrieve_Events(&(device->internal_events),
                               events,
                               operation,
                               &ret_flags,
                               suspend);

    if(status == NU_SUCCESS)
    {
        if((ret_flags&events) == events)
        {
            (*bAvailable) = NU_TRUE;
        }
    }

    else if(status == NU_NOT_PRESENT)
    {
        (*bAvailable) = NU_FALSE;
        status = NU_SUCCESS;
    }

    return status;
}

/* ======================  End Of File  =============================== */

