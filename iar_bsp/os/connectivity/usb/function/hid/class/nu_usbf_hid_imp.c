/**************************************************************************
*
*           Copyright 2005 Mentor Graphics Corporation
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
*       nu_usbf_hid_imp.c
*
* COMPONENT
*
*       HID Component / Nucleus USB Device Software
*
* DESCRIPTION
*
*       This file contains the internal functions of the HID
*       driver.
*
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       USBF_HID_Hisr                       Class driver HISR entry.
*       USBF_HID_Connect_User               Initialize event handling.
*       USBF_HID_Handle_Disconnected        Disconnect event handling.
*       USBF_HID_Handle_Class_Req_Rcvd      Class request handling.
*       USBF_HID_Ctrl_Data_IRP_Cmplt        Control transfer IRP callback.
*       USBF_HID_Handle_Ctrl_Xfer_Cmplt     Control transfer completion
*                                           handling.
*       USBF_HID_Handle_Report_Sent         Report transfer completion
*                                           handling.
*       USBF_HID_Handle_USB_Event_Rcvd      USB Report Handling.
*       USBF_HID_Interrupt_Data_IRP_Cmplt   Interrupt transfer IRP
*                                           callback.
*
* DEPENDENCIES
*
*       nu_usb.h                            Plus related definitions.
*
**************************************************************************/
#include    "connectivity/nu_usb.h"

/*************************************************************************
* FUNCTION
*
*       USBF_HID_Connect_User
*
* DESCRIPTION
*
*       This routine finds out the user for the class driver out of the
*       possible users and connects the user to the driver.
*
* INPUTS
*       dev             Pointer to Driver control block.
*
* OUTPUTS
*
*       NONE
*
*************************************************************************/
STATUS USBF_HID_Connect_User(USBF_HID_DEVICE *dev)
{
    STATUS status;
    INT i;
    UNSIGNED num_users;
    NU_USB_USER *user;
    NU_USB_USER *prev_user;
    UINT8 sub_class, protocol, intf_protocol, intf_sub_class;
    NU_USB_INTF *intf;
    NU_USB_USER *user_list[NU_USB_MAX_USERS];
    NU_USBF_HID *hid = (NU_USBF_HID *) dev->drvr;
    status = NU_SUCCESS;


    /* Give a disconnect callback to the user */
    prev_user = dev->hid_user;
    intf = dev->hid_intf;

    if(prev_user != NU_NULL)
        NU_USB_USER_Disconnect (prev_user,(NU_USB_DRVR*)hid,NU_NULL);


    /* Get number of registered users */
    num_users = NU_USB_DRVR_Get_Users((NU_USB_DRVR*)hid,&user_list[0],
                                       NU_USB_MAX_USERS);

    /* Get the interface descriptor and find out who is the user */
    /* go through all users to find who should be owning this */
    for(i = 0; i < num_users; i++)
    {
        if(i >= NU_USB_MAX_USERS)
        {
            status = NU_NOT_PRESENT;
            break;

        }
        user = user_list[i];

        status = NU_USB_USER_Get_Subclass(user, &sub_class);

        if (status != NU_SUCCESS)
            continue;

        status = NU_USB_USER_Get_Protocol (user, &protocol);
        if (status != NU_SUCCESS)
            continue;

        NU_USB_INTF_Get_SubClass (intf, &intf_sub_class);
        NU_USB_INTF_Get_Protocol( intf, &intf_protocol);

        if(    (sub_class == intf_sub_class)     &&
            (protocol     == intf_protocol)    &&
            (sub_class > 0))
         {
                /* Save user in the control block of device*/
                dev->hid_user = user;

                status = NU_USB_USER_Connect (user,
                                              (NU_USB_DRVR*)hid,
                                              (USBF_HID_DEVICE*)dev);

                if(status == NU_SUCCESS)
                    break;

        }
        dev->hid_user = NU_NULL;
    }

    /* In case no user is found report error status */
    if (i == num_users)
        status = NU_NOT_PRESENT;

    return status;

}/* USBF_HID_Handle_Initialized */

/*************************************************************************
* FUNCTION
*
*       USBF_HID_Handle_Disconnected
*
* DESCRIPTION
*
*       This routine processes the Disconnection handling.
*
* INPUTS
*       dev             Pointer to device control block.
*
* OUTPUTS
*
*       NONE
*
*************************************************************************/
VOID USBF_HID_Handle_Disconnected(USBF_HID_DEVICE *dev)
{
    /* If connected to user, send disconnect. */
    if(dev->hid_user != NU_NULL)
    {
        NU_USB_USER_Disconnect (dev->hid_user,
                                (NU_USB_DRVR*)dev->drvr,
                                NU_NULL);
    }

    /* Flush all pipes. */
    if(dev->control_pipe)
    {
        NU_USB_PIPE_Flush(dev->control_pipe);
    }

    if(dev->intr_in_pipe)
    {
        NU_USB_PIPE_Flush(dev->intr_in_pipe);
    }
    if(dev->intr_out_pipe)
    {
        NU_USB_PIPE_Flush(dev->intr_out_pipe);
    }

    /* Release the interface */
    NU_USB_INTF_Release (dev->hid_intf, (NU_USB_DRVR *)dev->drvr);

    /* Reset the control block components */
    dev->hid_user = NU_NULL;
    dev->hid_intf = NU_NULL;
    dev->control_pipe = NU_NULL;
    dev->intr_in_pipe = NU_NULL;
    dev->intr_out_pipe = NU_NULL;

    /* De-allocate memory for this device. */
    USB_Deallocate_Memory (dev->cmd_buffer);
    USB_Deallocate_Memory (dev);

}/* USBF_HID_Handle_Disconnected */


/*************************************************************************
* FUNCTION
*
*       USBF_HID_Handle_Class_Req_Rcvd
*
* DESCRIPTION
*
*       This routine processes the class specific request.
*
* INPUTS
*       dev             Pointer to device control block.
*
* OUTPUTS
*
*       NONE
*
*************************************************************************/
STATUS USBF_HID_Handle_Class_Req_Rcvd(USBF_HID_DEVICE *dev)
{
    NU_USB_IRP *irp;
    STATUS status =NU_USB_INVLD_ARG;
    /* Device Command block pointer */
    NU_USBF_HID_CMD *cmd = &dev->hid_ctrl_cmd;
    /* Setup packet pointer*/
    NU_USB_SETUP_PKT *setup = &dev->setup_pkt;
    /* HID Class driver pointer*/
    NU_USBF_HID *hid = (NU_USBF_HID *)dev->drvr;
    UINT32 rem_length;
    UINT32 requested_length;
    UINT8 *buffer;
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
    UINT16 mps0;
#else
    UINT8 mps0;
#endif
    BOOLEAN use_empty_pkt = NU_FALSE;


    /* If not connected to a user, do not proceed. */
    if(dev->hid_user == NU_NULL)
    {
        status = NU_USB_INVLD_ARG;
    }
    else
    {
        /* Update the command received */
        cmd->command = setup->bRequest;
        /* Retrieve the control IRP pointer from the device */
        irp = &dev->ctrl_irp;

        /* if data direction is from host to device. */
        if((setup->bmRequestType & HIDF_SETUP_DIR_BIT) == 0)
        {
            /* If no data transfer is associated with setup packet. */
            if(setup->wLength == 0)
            {
                setup = &dev->setup_pkt;

                dev->hid_ctrl_cmd.data_len  = 0;
                dev->hid_ctrl_cmd.cmd_data  = NU_NULL;
                dev->hid_ctrl_cmd.cmd_Value = setup->wValue;
                dev->hid_ctrl_cmd.command   = setup->bRequest;

                /* Call the user New_Command callback with received
                 * command.
                 */
                status = NU_USBF_USER_New_Command(dev->hid_user,
                                         (NU_USBF_DRVR *)hid,
                                         NU_NULL,
                                         (UINT8 *)(&dev->hid_ctrl_cmd),
                                         sizeof(NU_USBF_HID_CMD),
                                         &buffer,
                                         &rem_length);

            }
            /* If Data OUT phase is pending. */
            else
            {
                status = NU_USB_IRP_Create(irp,
                                HIDF_MAX_OUT_REPORT_LENGTH,
                                dev->cmd_buffer,
                                NU_TRUE,
                                NU_FALSE,
                                USBF_HID_Ctrl_Data_IRP_Cmplt,
                                (VOID *)dev,
                                0);

                /* Submit the request to receive the data associated with
                 * class request.
                 */
                status = NU_USB_PIPE_Submit_IRP
                                    (dev->control_pipe, irp);
            }

        }
        else if((setup->bmRequestType & HIDF_SETUP_DIR_BIT) != 0)
        /* Direction of data is from device to host */
        {
            requested_length = setup->wLength;
            dev->hid_ctrl_cmd.data_len = 0;
            dev->hid_ctrl_cmd.cmd_data = NU_NULL;
            dev->hid_ctrl_cmd.command = setup->bRequest;

            /* Pass the command to user driver. */
            status = NU_USBF_USER_New_Command (dev->hid_user,
                                              (NU_USBF_DRVR *)dev,
                                               NU_NULL,
                                              (UINT8 *)&dev->hid_ctrl_cmd,
                                               sizeof(NU_USBF_HID_CMD),
                                               &buffer,
                                               &rem_length);

            if(status == NU_SUCCESS)
            {

                /* If the user driver wants to send the data for
                 * the last command, fill this data and submit
                 * the IRP.
                 */
                if(buffer != NU_NULL && rem_length > 0)
                {

                    /* If data to be sent is multiple of maximum
                     * packet size, zero length should be sent
                     * as end of transfer.
                     */
                    if(requested_length <= rem_length)
                    {
                        rem_length = requested_length;
                    }
                    else
                    {

                        NU_USB_DEVICE_Get_bMaxPacketSize0(dev->hid_dev,
                                                          &mps0);

                        if((rem_length % mps0) == 0 || (rem_length < mps0))
                        {
                            use_empty_pkt = NU_TRUE;
                        }
                    }

                    /* Save the command buffer data and length of the
                       data
                     */
                    dev->hid_ctrl_cmd.cmd_data = buffer;
                    dev->hid_ctrl_cmd.data_len = (UINT16)rem_length;

                    /* Retrieve the control IRP pointer from device
                       control block.
                     */
                    irp = &dev->ctrl_irp;


                    /* Create and submit the control IRP */
                    status = NU_USB_IRP_Create(irp,
                                rem_length,
                                buffer,
                                NU_TRUE,
                                use_empty_pkt,
                                USBF_HID_Ctrl_Data_IRP_Cmplt,
                                (VOID *)dev,
                                0);

                    if(status == NU_SUCCESS)
                    {

                        /* Submit request to transfer data by user. */
                        status = NU_USB_PIPE_Submit_IRP(dev->control_pipe,
                                                            irp);

                    }
                }
                /* If Host wants data, but user driver is not
                 * sending it, means request is not supported.
                 */
                else if(requested_length > 0)
                {
                    status = NU_USB_NOT_SUPPORTED;
                }
            }
        }

    }

    return status;
}/* USBF_HID_Handle_Class_Req_Rcvd */

/*************************************************************************
* FUNCTION
*
*       USBF_HID_Ctrl_Data_IRP_Cmplt
*
* DESCRIPTION
*
*       This is callback function for the control data IRP completion.
*
* INPUTS
*       pipe        Pointer to the pipe for the control transfer.
*       irp         Pointer to the control IRP
*
* OUTPUTS
*
*       NONE
*
*************************************************************************/
VOID USBF_HID_Ctrl_Data_IRP_Cmplt(NU_USB_PIPE * pipe, NU_USB_IRP  *irp)
{
    USBF_HID_DEVICE *dev;

    /*  Get context information from the IRP  */
    NU_USB_IRP_Get_Context(irp,(void **)&dev);

    USBF_HID_Handle_Ctrl_Xfer_Cmplt(dev);


}

/*************************************************************************
* FUNCTION
*
*       USBF_HID_Handle_Ctrl_Xfer_Cmplt
*
* DESCRIPTION
*
*       This routine processes the control transfer completion.
*
* INPUTS
*       dev             Pointer to device control block.
*
* OUTPUTS
*
*       NONE
*
*************************************************************************/
VOID USBF_HID_Handle_Ctrl_Xfer_Cmplt(USBF_HID_DEVICE *dev)
{
    NU_USB_IRP *irp = &dev->ctrl_irp;
    UINT32 rem_len;
    UINT8 *buffer;
    UINT8 *cmpltd_data;
    UINT32 cmpltd_len;
    UINT32 tx_handle;
    STATUS status;


    /* Get the transfer attributes. */
    NU_USB_IRP_Get_Actual_Length(irp, &cmpltd_len);
    NU_USB_IRP_Get_Data(irp, &cmpltd_data);

    /* If OUT transfer, pass the received command to user. */
    if((dev->setup_pkt.bmRequestType & HIDF_SETUP_DIR_BIT) == 0)
    {
        dev->hid_ctrl_cmd.data_len = cmpltd_len;
        dev->hid_ctrl_cmd.cmd_data = cmpltd_data;

        status = NU_USBF_USER_New_Command (dev->hid_user,
                      (NU_USBF_DRVR *)dev->drvr,
                      NU_NULL,
                      (UINT8 *)(&dev->hid_ctrl_cmd),
                      sizeof(NU_USBF_HID_CMD),
                      &buffer,
                      &rem_len);

    }
    /* If IN transfer, send the completion callback to user. */
    else
    {
        tx_handle = HIDF_CTRL_IN_XFER_CMPLT;
        status = NU_USBF_USER_Tx_Done((NU_USBF_USER *)dev->hid_user,
                                        (NU_USBF_DRVR *)dev->drvr,
                                        &tx_handle,
                                        cmpltd_data,
                                        cmpltd_len,
                                        &buffer,
                                        &rem_len);

    }

    if(status != NU_SUCCESS)
    {
        NU_USB_PIPE_Stall(dev->control_pipe);
    }

}/* USBF_HID_Handle_Ctrl_Xfer_Cmplt */

/*************************************************************************
* FUNCTION
*
*       USBF_HID_Handle_USB_Event_Rcvd
*
* DESCRIPTION
*
*       This routine processes the USB bus event notification.
*
* INPUTS
*       dev            Pointer to device control block.
*
* OUTPUTS
*
*       NONE
*
*************************************************************************/
VOID USBF_HID_Handle_USB_Event_Rcvd(USBF_HID_DEVICE *dev)
{

    if(dev != NU_NULL)
    {
        if ((dev->usb_event == USBF_EVENT_RESET)    ||
            (dev->usb_event == USBF_EVENT_DISCONNECT) )
        {
            /* If connected to user, give notification to it. */
            if (dev->hid_user != NU_NULL)
            {
                NU_USBF_USER_Notify (dev->hid_user, (NU_USB_DRVR *)dev->drvr,
                                           NU_NULL, dev->usb_event);
                NU_USB_USER_Disconnect (dev->hid_user, (NU_USB_DRVR *)dev->drvr, dev);
            }

            /* Flush all pipes. */
            if(dev->control_pipe)
            {
                NU_USB_PIPE_Flush(dev->control_pipe);
            }

            if(dev->intr_in_pipe)
            {
                NU_USB_PIPE_Flush(dev->intr_in_pipe);
            }
            if(dev->intr_out_pipe)
            {
                NU_USB_PIPE_Flush(dev->intr_out_pipe);
            }
            /* Release the interface */

            NU_USB_INTF_Release (dev->hid_intf, (NU_USB_DRVR *)dev->drvr);

            /* Reset the control block components */
            dev->hid_user = NU_NULL;
            dev->hid_intf = NU_NULL;
            dev->control_pipe = NU_NULL;
            dev->intr_in_pipe = NU_NULL;
            dev->intr_out_pipe = NU_NULL;

            /* De-allocate memory for this device. */
            USB_Deallocate_Memory (dev->cmd_buffer);
            USB_Deallocate_Memory (dev);
        }

        else if(dev->usb_event == USBF_EVENT_CONNECT)
        {
            /* If connected to user, give notification to it. */
            if (dev->hid_user != NU_NULL)
            {
                NU_USBF_USER_Notify (dev->hid_user, (NU_USB_DRVR *)dev->drvr,
                                           NU_NULL, dev->usb_event);
            }
        }
    }
}/* USBF_HID_Handle_USB_Report_Rcvd */

/*************************************************************************
* FUNCTION
*
*       USBF_HID_Interrupt_Data_IRP_Cmplt
*
* DESCRIPTION
*
*       Callback function which is invoked when the submitted IRP is
*       completed on the interrupt IN pipe.
*
* INPUTS
*       pipe            Pointer to pipe on which IRP is submitted.
*       irp              Pointer to the irp.
*
* OUTPUTS
*
*       NONE
*
*************************************************************************/
VOID USBF_HID_Interrupt_Data_IRP_Cmplt(NU_USB_PIPE * pipe, NU_USB_IRP  *irp)
{

    USBF_HID_DEVICE *dev = NU_NULL;


    NU_USB_IRP_Get_Context (irp, (VOID **)&dev);

    USBF_HID_Handle_Interrupt_Transfer(dev);

}

/*************************************************************************
* FUNCTION
*
*       USBF_HID_Handle_Interrupt_Transfer
*
* DESCRIPTION
*
*       This routine processes the completion of interrupt transfers..
*
* INPUTS
*       dev             Pointer to device control block.
*
* OUTPUTS
*
*       NONE
*
*************************************************************************/
STATUS USBF_HID_Handle_Interrupt_Transfer(USBF_HID_DEVICE *dev)
{
    NU_USB_IRP *irp = &dev->intr_irp;
    UINT8 rollback = 0;
    UINT8 *buffer  = (UINT8 *)0;
    UINT8 *cmpltd_data;
    UINT32 cmpltd_len;
    STATUS status;
    UINT32 tx_handle;
    UINT32 rem_len;

    /* Get the transfer attributes. */
    NU_USB_IRP_Get_Actual_Length(irp, &cmpltd_len);

    NU_USB_IRP_Get_Data(irp, &cmpltd_data);


     /* If zero length is to be sent as end of transfer, submit the request
      * to send it.
      */

    if(dev->report_tx_state == HIDF_SEND_ZERO_LEN)
    {

        status = NU_USB_IRP_Create(irp,
                            0,
                            buffer,
                            NU_TRUE,
                            NU_FALSE,
                            USBF_HID_Interrupt_Data_IRP_Cmplt,
                            (VOID *)dev,
                            0);

        status = NU_USB_PIPE_Submit_IRP(dev->intr_in_pipe, irp);


        dev->report_tx_state = HIDF_ZERO_LEN_SENT;
        dev->report_data = cmpltd_data;
        dev->report_len = cmpltd_len;

        rollback = 1;
    }

    /* If callback is received for zero length, restore the save transfer
     * attributes.
     */
    else if(dev->report_tx_state == HIDF_ZERO_LEN_SENT)
    {
        cmpltd_data = dev->report_data;
        cmpltd_len = dev->report_len;
    }

    /* Otherwise, send the completion notification to user. */
    if(rollback == 0)
    {

        tx_handle = HIDF_REPORT_SENT;

        status = NU_USBF_USER_Tx_Done(dev->hid_user,
                                      dev->drvr,
                                      &tx_handle,
                                      (UINT8*)(dev->intr_irp.buffer),
                                      dev->intr_irp.actual_length,
                                      &buffer,
                                      &rem_len);



    }

    /* If error occurred in the processing, stall interrupt IN pipe. */
    if(status != NU_SUCCESS)
    {
        NU_USB_PIPE_Stall(dev->intr_in_pipe);
    }
    return status;
}

 /* ======================  End Of File  =============================== */
