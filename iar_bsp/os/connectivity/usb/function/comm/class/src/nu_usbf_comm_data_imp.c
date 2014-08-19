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
*       nu_usbf_comm_data_imp.c
*
* COMPONENT
*
*       Nucleus USB Function Software : Data Interface Class Driver
*
* DESCRIPTION
*
*       This file contains the internal implementation routines
*       of Data Interface Class Driver.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       COMMF_DATA_Submit_Rx_Data_IRP       Receive data IRP Submission.
*       COMMF_DATA_Submit_Tx_Data_IRP       Transmit data IRP submission.
*       COMMF_DATA_Tx_IRP_Complete          Transmit data completion
*                                           Callback.
*       COMMF_DATA_Rx_IRP_Complete          Receive data IRP Completion.
*       COMMF_Enqueue_Tx_Transfer           Adding a buffer in queue.
*       COMMF_Dequeue_Tx_Transfer           Removal of buffer from queue.
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions
*
*************************************************************************/

/* ==============  USB Include Files =================================  */
#include "connectivity/nu_usb.h"

/* ==========================  Functions ============================== */

/*************************************************************************
*
* FUNCTION
*
*        COMMF_DATA_Submit_Rx_Data_IRP
*
* DESCRIPTION
*
*        This function is used to post an event for submission of
*        receive IRP to the host.
*
*
* INPUTS
*
*       device                              Pointer to Communication
*                                           device.
*
* OUTPUTS
*
*       NU_SUCCESS                          Successful completion of
*                                           service.
*
*************************************************************************/
STATUS  COMMF_DATA_Submit_Rx_Data_IRP (USBF_COMM_DEVICE* device)
{
    STATUS status;
    BOOLEAN bAvailable;

    status = COMMF_Get_Event(device,
                             COMMF_RX_IN_PROGRESS,
                             &bAvailable,
                             NU_NO_SUSPEND,
                             NU_AND);

    if(status == NU_SUCCESS)
    {
        if(bAvailable == NU_FALSE)
        {
            status = COMMF_DATA_Submit_Rx_IRP_Impl(device);
        }
    }

    return status;
}

/*************************************************************************
*
* FUNCTION
*
*        COMMF_DATA_Submit_Rx_IRP_Impl
*
* DESCRIPTION
*
*        This function is used to submit IRP to receive data from
*        host.
*
*
*
* INPUTS
*
*       device                              Pointer to Communication
*                                           device.
*
* OUTPUTS
*
*       NU_SUCCESS                          Successful completion of
*                                           service.
*
*************************************************************************/
STATUS  COMMF_DATA_Submit_Rx_IRP_Impl (USBF_COMM_DEVICE* device)
{
    NU_USB_IRP *irp = NU_NULL;
    STATUS status;
    UINT8 *rx_buffer = NU_NULL;
    UINT32 rx_length;
    static BOOLEAN non_reent = NU_FALSE;

    NU_Protect(&(device->rx_submit_prot));

    if(non_reent == NU_FALSE)
    {
        non_reent = NU_TRUE;
        status = NU_SUCCESS;
    }

    else
    {
        status = NU_NOT_PRESENT;
        for(;;);
    }

    if(status == NU_SUCCESS)
    {
        status = COMMF_Get_Rx_Buffer(
                    device,
                    (VOID**)&rx_buffer,
                    &rx_length);

        if(status == NU_SUCCESS)
        {
            irp = &device->out_irp;

            /* Create the IRP for current transfer. */
            status = NU_USB_IRP_Create(irp,
                                       rx_length,
                                       rx_buffer,
                                       NU_TRUE,
                                       NU_FALSE,
                                       COMMF_DATA_Rx_IRP_Complete,
                                       device,
                                       0);

            if(status != NU_SUCCESS)
            {
                (VOID)COMMF_Put_Back_Rx_Buffer(device);
            }
        }

        if(status == NU_SUCCESS)
        {
            status = COMMF_Set_Event(
                        device,
                        COMMF_RX_IN_PROGRESS);
        }

        if(status == NU_SUCCESS)
        {
            /* Submit the IRP to the Bulk OUT pipe. */
            status = NU_USB_PIPE_Submit_IRP(device->out_pipe, irp);

            /* If failed to submit IRP clear the flag. */
            if (status != NU_SUCCESS)
            {
                (VOID)COMMF_Put_Back_Rx_Buffer(device);
                (VOID)COMMF_Clear_Event(
                        device,
                        COMMF_RX_IN_PROGRESS);
            }
        }

        non_reent = NU_FALSE;
        NU_Unprotect();
    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*        COMMF_DATA_Submit_Tx_Data_IRP
*
* DESCRIPTION
*
*        This function is used to post an event to the submission queue
*        and activates the HISR which does processing of the event.
*
*
* INPUTS
*
*        device                             Pointer to Communication device
*
* OUTPUTS
*
*        NU_SUCCESS                         Successful completion of
*                                           service.
*
*************************************************************************/
STATUS  COMMF_DATA_Submit_Tx_Data_IRP (USBF_COMM_DEVICE* device)
{
    STATUS status;
    BOOLEAN bAvailable;

    status = COMMF_Get_Event(device,
                             COMMF_TX_IN_PROGRESS,
                             &bAvailable,
                             NU_NO_SUSPEND,
                             NU_AND);
	
    if(status == NU_SUCCESS)
    {
        if(bAvailable == NU_FALSE)
        {
            status = COMMF_DATA_Submit_Tx_IRP_Impl(device);
        }
    }

    return status;
}

/*************************************************************************
*
* FUNCTION
*
*        COMMF_DATA_Submit_Tx_IRP_Impl
*
* DESCRIPTION
*
*        This function is used to submit the IRP to transmit the data to
*        USB Host.
*
* INPUTS
*
*        device                             Pointer to Communication device
*
* OUTPUTS
*
*        NU_SUCCESS                         Successful completion of
*                                           service.
*
*************************************************************************/
STATUS  COMMF_DATA_Submit_Tx_IRP_Impl (USBF_COMM_DEVICE* device)
{
    NU_USB_IRP *irp = NU_NULL;
    STATUS status;
    UINT16 maxp = 0;
    NU_USB_ENDP *endp = NU_NULL;
    UINT8* buffer = NU_NULL;
    UINT32 length = 0;
    static BOOLEAN non_reent = NU_FALSE;

    NU_Protect(&(device->tx_submit_prot));

    if(non_reent == NU_FALSE)
    {
        non_reent = NU_TRUE;
        status = NU_SUCCESS;
    }

    else
    {
        status = NU_NOT_PRESENT;
        for(;;);
    }

    if(status == NU_SUCCESS)
    {
        status = COMMF_Dequeue_Tx_Transfer(device,
                                           (VOID**)&buffer,
                                           &length);

        if(status == NU_SUCCESS)
        {
            device->tx_data = buffer;
            device->tx_data_len = length;

            irp = &device->in_irp;

            /* Create the IRP for current transfer. */
            status = NU_USB_IRP_Create(irp,
                                       length,
                                       buffer,
                                       NU_TRUE,
                                       NU_FALSE,
                                       COMMF_DATA_Tx_IRP_Complete,
                                       device,
                                       0);
        }

        if(status == NU_SUCCESS)
        {
            status = COMMF_Set_Event(
                        device,
                        COMMF_TX_IN_PROGRESS);
        }

        if(status == NU_SUCCESS)
        {
            /* Submit the IRP to the Bulk IN pipe. */
            status = NU_USB_PIPE_Submit_IRP(device->in_pipe, irp);

            if(status != NU_SUCCESS)
            {
                (VOID)COMMF_Clear_Event(
                        device,
                        COMMF_TX_IN_PROGRESS);
                /*
                 * TX IRP could not be submitted successfully. Call tx_done
                 * callback of user driver with status cancelled.
                 */
                (VOID)NU_USB_IRP_Set_Status(irp, NU_USB_IRP_CANCELLED);
                (VOID)NU_USB_IRP_Set_Actual_Length(irp, 0);
                (VOID)COMMF_Handle_Tx_Done(
                            (NU_USBF_COMM*)device->mng_drvr,
                            device,
                            COMMF_DATA_SENT);
            }
        }

        if (status == NU_SUCCESS)
        {
            status = NU_USB_PIPE_Get_Endp(device->in_pipe, &endp);
        }

        if(status == NU_SUCCESS)
        {
            status = NU_USB_ENDP_Get_Max_Packet_Size(endp, &maxp);
        }

        if(status == NU_SUCCESS && (maxp > 0))
        {
            device->data_state = 0;

            /* If length of data is multiple of maximum packet size, then zero
             * length packet should be transfer to mark end of transfer.
             * 'data_state' will be checked in IRP Complete handler to send
             * zero length packet.
             */
            if ((device->delineate == NU_TRUE) &&
                (length > 0) &&
                (length%maxp == 0))
            {
                device->data_state = COMMF_SEND_ZERO_LENGTH;
            }
        }


        non_reent = NU_FALSE;

        NU_Unprotect();
    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*        COMMF_DATA_Tx_IRP_Complete
*
* DESCRIPTION
*
*        Data Interface Class driver : Transmitted data transfer
*        completion Callback
*
* INPUTS
*
*        irp                                Completed Data transfer
*        pipe                               Pipe of transfer
*
* OUTPUTS
*
*        None
*
*************************************************************************/
VOID    COMMF_DATA_Tx_IRP_Complete (NU_USB_PIPE * pipe, NU_USB_IRP  *irp)
{
    USBF_COMM_DEVICE *device = NU_NULL;
    UINT32 length;
    UINT8 *buffer = NU_NULL;
    STATUS status;
    STATUS irp_status;
    UINT8 rollback = 0;

    status = NU_USB_IRP_Get_Status (irp, &irp_status);

    if(status == NU_SUCCESS)
    {
        status = NU_USB_IRP_Get_Context (irp, (VOID **) &device);
    }

    /* If zero length packet is to sent as end of transfer marker
     * (applicable if transfer length is multiple of maximum packet size.
     */
    if(status == NU_SUCCESS)
    {
        if(irp_status == NU_SUCCESS && (device != NU_NULL))
        {
            if (device->data_state == COMMF_SEND_ZERO_LENGTH)
            {
                /* Update the transmit state. */
                device->data_state = COMMF_ZERO_LENGTH_SENT;

                /* Get the transmitted data and transmitted length. */
                status = NU_USB_IRP_Get_Actual_Length (irp, &length);

                if(status == NU_SUCCESS)
                {
                    status = NU_USB_IRP_Get_Data (irp, &buffer);
                }

                if(status == NU_SUCCESS)
                {
                    device->tx_data_len = (UINT32)length;
                    device->tx_data = buffer;
                    /* Set the length of zero to transmit. */
                    status = NU_USB_IRP_Set_Length(irp, 0);
                }

                /* Submit the IRP to same pipe, from which callback was called. */
                if(status == NU_SUCCESS)
                {
                    status = NU_USB_PIPE_Submit_IRP(pipe, irp);
                }

                if(status == NU_SUCCESS)
                {
                    rollback = 1;
                }
            }

            /* This block is executed if neither zero length packet was sent nor
             * it was needed. */
            else if (device->data_state != COMMF_ZERO_LENGTH_SENT)
            {
                status = NU_USB_IRP_Get_Actual_Length (irp, &length);

                if(status == NU_SUCCESS)
                {
                    status = NU_USB_IRP_Get_Data (irp, &buffer);
                }

                if(status == NU_SUCCESS)
                {
                    device->tx_data_len = (UINT32)length;
                    device->tx_data = buffer;
                }

            }
        }

        else if ((device != NU_NULL))
        {
            device->tx_data_len = 0;
            status = NU_USB_IRP_Get_Data (irp, &(device->tx_data));
        }

        if((rollback == 0) && (device != NU_NULL))
        {
            status = COMMF_Clear_Event(
                        device,
                        COMMF_TX_IN_PROGRESS);

            /* Reset the transmit state. */
            device->data_state = 0;

            (VOID)COMMF_Handle_Tx_Done(
                        (NU_USBF_COMM*)device->mng_drvr,
                        device,
                        COMMF_DATA_SENT);

            /* If a transfer is pending, submit request for it. */
            if(status == NU_SUCCESS)
            {
                status = COMMF_DATA_Submit_Tx_Data_IRP(device);
            }
        }
    }

    return;
}

/*************************************************************************
*
* FUNCTION
*
*        COMMF_DATA_Rx_IRP_Complete
*
* DESCRIPTION
*
*        Data Interface Class driver : Received data transfer completion
*                                      Callback
*
* INPUTS
*
*        irp                                Completed Data transfer
*        pipe                               Pipe of transfer
*
* OUTPUTS
*
*        None
*
*************************************************************************/

VOID    COMMF_DATA_Rx_IRP_Complete (NU_USB_PIPE * pipe, NU_USB_IRP  *irp)
{
    USBF_COMM_DEVICE *device = NU_NULL;
    STATUS status;
    STATUS irp_status = NU_SUCCESS;
    UINT8 *data_out = NU_NULL;
    UINT32 length = 0;
    COMMF_RX_BUF_GROUP * buffer_list;
    BOOLEAN bFinished;

    status = NU_USB_IRP_Get_Context (irp, (VOID **) &device);

    if(status == NU_SUCCESS)
    {
        status = NU_USB_IRP_Get_Status (
                    irp,
                    &irp_status);
    }

    /* Get the received data and length. */
    if(status == NU_SUCCESS)
    {
        status = NU_USB_IRP_Get_Actual_Length (irp, &length);
    }

    if(status == NU_SUCCESS)
    {
        status = NU_USB_IRP_Get_Data (irp, &data_out);
    }

    /*
     * Mark that there is no IRP submitted on BULK OUT endpoint.
     */
    if(status == NU_SUCCESS)
    {
        /* In case if IRP was cancelled then do nothing but return. */
        if (irp_status == NU_USB_IRP_CANCELLED)
        {
            return;
        }

        if (irp_status == NU_SUCCESS)
        {
            device->cmplt_rx_buffer = data_out;
            device->cmplt_rx_buffer_len = length;

            (VOID)NU_USBF_USER_New_Transfer (
                           device->user,
                           device->data_drvr,
                           device,
                           NU_NULL,
                           NU_NULL);

            status = COMMF_Is_Rx_Buff_Grp_Finished(device,
                                                   &bFinished,
                                                   &buffer_list);

            if(status == NU_SUCCESS)
            {
                if(bFinished == NU_TRUE)
                {
                    if(buffer_list != NU_NULL)
                    {
                        if(buffer_list->callback != NU_NULL)
                        {
                            #if (COMMF_VERSION_COMP >= COMMF_2_0)
                            buffer_list->callback(
                                    device->user,
                                    buffer_list, device);
                            #else
                            buffer_list->callback(
                                    device->user,
                                    buffer_list);
                            #endif
                        }
                    }
                }
            }
        }

        /*
         * Pipe should be stalled in case of over-run or under-run conditions.
         */
        else if ((irp_status == NU_USB_BFR_OVERRUN) ||
                 (irp_status == NU_USB_BFR_UNDERRUN))
        {
            (VOID)COMMF_Put_Back_Rx_Buffer(device);
            /*
             * Stall the pipe.
             */
            status = NU_USB_PIPE_Stall(device->out_pipe);
        }

        else
        {
            status = COMMF_Put_Back_Rx_Buffer(device);
        }


        status = COMMF_Clear_Event(device, COMMF_RX_IN_PROGRESS);

        if(status == NU_SUCCESS)
        {
            status = COMMF_DATA_Submit_Rx_Data_IRP(device);
        }
    }

    else
    {
        for(;;);
    }

    return;
}

/*************************************************************************
*
* FUNCTION
*
*        COMMF_Enqueue_Tx_Transfer
*
* DESCRIPTION
*
*        Enqueue TX transfer into pending queue.
*
* INPUTS
*
*       device                              Pointer to device's control
*                                           block.
*       buffer                              Pointer to the data buffer
*                                           to be transferred.
*       length                              Length of payload data in
*                                           bytes.
*
* OUTPUTS
*
*        NU_SUCCESS                         TX transfer has been
*                                           successfully queued into
*                                           pending TX transfers queue.
*
*************************************************************************/
STATUS    COMMF_Enqueue_Tx_Transfer (
                USBF_COMM_DEVICE * device,
                VOID * buffer,
                UINT32 length)
{
    STATUS status;
    COMMF_TRANSFER transfer;
    UINT32 element_size;

    element_size = sizeof(COMMF_TRANSFER)/sizeof(UNSIGNED);

    transfer.buffer = buffer;
    transfer.length = length;

    status = NU_Send_To_Queue(&(device->pending_tx_queue),
                              &transfer,
                              element_size,
                              NU_NO_SUSPEND);

    return status;
}

/*************************************************************************
*
* FUNCTION
*
*        COMMF_Dequeue_Tx_Transfer
*
* DESCRIPTION
*
*        Dequeue TX transfer from pending queue.
*
* INPUTS
*
*        device                             Pointer to device's control
*                                           block.
*        buffer                             Pointer to the data buffer
*                                           to be transferred.
*        length                             Length of payload data in
*                                           bytes.
*
* OUTPUTS
*
*        NU_SUCCESS                         TX transfer has been
*                                           successfully queued into
*                                           pending TX transfers queue.
*
*************************************************************************/
STATUS    COMMF_Dequeue_Tx_Transfer (
                USBF_COMM_DEVICE * device,
                VOID ** buffer,
                UINT32 * length)
{
    STATUS status;
    COMMF_TRANSFER transfer;
    UINT32 element_size;
    UINT32 actual_size;

    element_size = sizeof(COMMF_TRANSFER)/sizeof(UNSIGNED);

    status = NU_Receive_From_Queue(&(device->pending_tx_queue),
                              &transfer,
                              element_size,
                              &actual_size,
                              NU_NO_SUSPEND);

    if(status == NU_SUCCESS)
    {
        *buffer = transfer.buffer;
        *length = transfer.length;
    }

    return status;
}

/*************************************************************************
*
* FUNCTION
*
*        COMMF_Enqueue_Rx_Buff_Grp
*
* DESCRIPTION
*
*        Enqueue RX buffer group.
*
* INPUTS
*
*        device                             Pointer to device's control
*                                           block.
*        buff_grp                           Pointer to the RX buffer
*                                           group.
*
* OUTPUTS
*
*        NU_SUCCESS                         RX buffer group has
*                                           successfully been queued into
*                                           RX buffer group queue.
*
*************************************************************************/
STATUS    COMMF_Enqueue_Rx_Buff_Grp (
                USBF_COMM_DEVICE * device,
                COMMF_RX_BUF_GROUP * buff_grp)
{
    STATUS status;
    UINT32 element_size = 0;
    COMMF_RX_BUF_GROUP_IMPL grp;

    element_size = sizeof(COMMF_RX_BUF_GROUP_IMPL)/sizeof(UNSIGNED);

    grp.buffer_group = buff_grp;
    grp.buffer_index = 0;

    status = NU_Send_To_Queue(
                &(device->rx_buf_grp_queue),
                &grp,
                element_size,
                NU_NO_SUSPEND);

    return status;
}

/*************************************************************************
*
* FUNCTION
*
*        COMMF_Clear_Rx_Buff_Grp_Queue
*
* DESCRIPTION
*
*        Clear the contents of RX buffer group queue.
*
* INPUTS
*
*        device                             Pointer to device's control
*                                           block.
*
* OUTPUTS
*
*        NU_SUCCESS                         RX buffer group has
*                                           successfully been cleared.
*
*************************************************************************/
STATUS    COMMF_Clear_Rx_Buff_Grp_Queue (
                USBF_COMM_DEVICE * device)
{
    STATUS status;

    status = NU_Reset_Queue(&(device->rx_buf_grp_queue));

    return status;
}

/*************************************************************************
*
* FUNCTION
*
*       COMMF_Get_Rx_Buffer
*
* DESCRIPTION
*
*       Retrieve a RX buffer from Rx buffer group currently on head of
*       RX buffer group queue.
*
* INPUTS
*
*       device                              Pointer to device's control
*                                           block.
*       buffer                              Pointer to location where
*                                           address of RX buffer is to
*                                           be placed.
*       length                              Pointer to location where
*                                           length of RX buffer is to
*                                           be placed.
*
* OUTPUTS
*
*       NU_SUCCESS                          RX buffer has been
*                                           successfully retrieved
*                                           from RX buffer group.
*       NU_UNAVAILABLE                      Another thread of execution
*                                           is currently operating on
*                                           RX buffer group queue.
*
*************************************************************************/
STATUS    COMMF_Get_Rx_Buffer (
                USBF_COMM_DEVICE * device,
                VOID ** buffer,
                UINT32 * length)
{
    STATUS status;
    UINT32 element_size = 0;
    UINT32 acutal_size = 0;
    COMMF_RX_BUF_GROUP_IMPL grp;
    BOOLEAN bAvailable = NU_FALSE;

    status = COMMF_Get_Event(device,
                            COMMF_RX_BUFF_GRP_MX,
                            &bAvailable,
                            NU_NO_SUSPEND,
                            NU_AND_CONSUME);

    if(status == NU_SUCCESS)
    {
        if(bAvailable == NU_FALSE)
        {
            status = NU_UNAVAILABLE;
        }

        else
        {
            element_size =
                    sizeof(COMMF_RX_BUF_GROUP_IMPL)/sizeof(UNSIGNED);

            status = NU_Receive_From_Queue(
                        &(device->rx_buf_grp_queue),
                        &grp,
                        element_size,
                        &acutal_size,
                        NU_NO_SUSPEND);

            if(status == NU_SUCCESS)
            {
                if(grp.buffer_index < grp.buffer_group->num_of_buffers)
                {
                    (*buffer) =
                         grp.buffer_group->buffer_array[grp.buffer_index];
                    (*length) = grp.buffer_group->buffer_size;
                    grp.buffer_index++;
                }

                else
                {
                    (*buffer) = NU_NULL;
                    (*length) = 0;
                    status = NU_NOT_PRESENT;
                }

                (VOID)NU_Send_To_Front_Of_Queue(
                        &(device->rx_buf_grp_queue),
                        &grp,
                        element_size,
                        NU_NO_SUSPEND);
            }
        }

        (VOID)COMMF_Set_Event(device, COMMF_RX_BUFF_GRP_MX);
    }

    return status;
}

/*************************************************************************
*
* FUNCTION
*
*       COMMF_Put_Back_Rx_Buffer
*
* DESCRIPTION
*
*       Put the previously retrieved Rx buffer back into the RX buffer
*       group currently on the head of RX buffer group queue.
*
* INPUTS
*
*       device                              Pointer to device's control
*                                           block.
*
* OUTPUTS
*
*       NU_SUCCESS                          RX buffer has
*                                           successfully been put
*                                           back into RX buffer group.
*       NU_UNAVAILABLE                      Another thread of execution
*                                           is currently operating on
*                                           RX buffer group queue.
*
*************************************************************************/
STATUS    COMMF_Put_Back_Rx_Buffer (
                USBF_COMM_DEVICE * device)
{
    STATUS status;
    UINT32 element_size = 0;
    UINT32 acutal_size = 0;
    COMMF_RX_BUF_GROUP_IMPL grp;
    BOOLEAN bAvailable = NU_FALSE;

    status = COMMF_Get_Event(device,
                            COMMF_RX_BUFF_GRP_MX,
                            &bAvailable,
                            NU_NO_SUSPEND,
                            NU_AND_CONSUME);

    if(status == NU_SUCCESS)
    {
        if(bAvailable == NU_FALSE)
        {
            status = NU_UNAVAILABLE;
        }

        else
        {
            element_size =
                    sizeof(COMMF_RX_BUF_GROUP_IMPL)/sizeof(UNSIGNED);

            status = NU_Receive_From_Queue(
                        &(device->rx_buf_grp_queue),
                        &grp,
                        element_size,
                        &acutal_size,
                        NU_NO_SUSPEND);

            if(status == NU_SUCCESS)
            {
                grp.buffer_index--;

                status = NU_Send_To_Front_Of_Queue(
                            &(device->rx_buf_grp_queue),
                            &grp,
                            element_size,
                            NU_NO_SUSPEND);
            }

            (VOID)COMMF_Set_Event(device, COMMF_RX_BUFF_GRP_MX);
        }
    }

    return status;
}

/*************************************************************************
*
* FUNCTION
*
*       COMMF_Is_Rx_Buff_Grp_Finished
*
* DESCRIPTION
*
*       Check if RX buffer group on the head of the RX buffer group
*       queue has expired or not. If expired, pointer to callback
*       function is provided to the caller.
*
* INPUTS
*
*       device                              Pointer to device's control
*                                           block.
*       bFinished                           Boolean variable pointed
*                                           by this pointer will be
*                                           set to NU_TRUE if RX
*                                           buffer group has expired
*                                           otherwise it will be
*                                           set to NU_FALSE.
*       group                               Pointer to location where
*                                           pointer to RX buffer group
*                                           is to be placed.
*                                           Only valid when boolean
*                                           variable pointed by
*                                           bFinished is set to NU_TRUE.
*
* OUTPUTS
*
*       NU_SUCCESS                          Status of RX buffer group
*                                           has been successfully checked.
*       NU_UNAVAILABLE                      Another thread of execution
*                                           is currently operating on
*                                           RX buffer group queue.
*
*************************************************************************/
STATUS    COMMF_Is_Rx_Buff_Grp_Finished (
                USBF_COMM_DEVICE * device,
                BOOLEAN * bFinished,
                COMMF_RX_BUF_GROUP ** group)
{
    STATUS status;
    UINT32 element_size = 0;
    UINT32 acutal_size = 0;
    COMMF_RX_BUF_GROUP_IMPL grp;
    BOOLEAN bAvailable = NU_FALSE;

    status = COMMF_Get_Event(device,
                            COMMF_RX_BUFF_GRP_MX,
                            &bAvailable,
                            NU_NO_SUSPEND,
                            NU_AND_CONSUME);

    if(status == NU_SUCCESS)
    {
        if(bAvailable == NU_FALSE)
        {
            status = NU_UNAVAILABLE;
        }

        else
        {
            element_size =
                    sizeof(COMMF_RX_BUF_GROUP_IMPL)/sizeof(UNSIGNED);

            status = NU_Receive_From_Queue(
                        &(device->rx_buf_grp_queue),
                        &grp,
                        element_size,
                        &acutal_size,
                        NU_NO_SUSPEND);

            if(status == NU_SUCCESS)
            {
                if(grp.buffer_index == grp.buffer_group->num_of_buffers)
                {
                    (*group) = grp.buffer_group;
                    (*bFinished) = NU_TRUE;
                }

                else if(grp.buffer_index > grp.buffer_group->num_of_buffers)
                {
                    for(;;);
                }

                else
                {
                    (*group) = NU_NULL;
                    (*bFinished) = NU_FALSE;
                }

                if((*bFinished) == NU_FALSE)
                {
                    status = NU_Send_To_Front_Of_Queue(
                                &(device->rx_buf_grp_queue),
                                &grp,
                                element_size,
                                NU_NO_SUSPEND);
                }
            }

            (VOID)COMMF_Set_Event(device, COMMF_RX_BUFF_GRP_MX);
        }
    }

    return status;
}

/*************************************************************************
*
* FUNCTION
*
*       COMMF_Init_Rx_Buff_Grp_Queue
*
* DESCRIPTION
*
*       Initialize RX buffer group queue.
*
* INPUTS
*
*       device                              Pointer to device's control
*                                           block.
*
* OUTPUTS
*
*       NU_SUCCESS                          RX buffer group queue has
*                                           successfully been initialized.
*
*************************************************************************/
STATUS    COMMF_Init_Rx_Buff_Grp_Queue (
                USBF_COMM_DEVICE * device)
{
    STATUS status;

    UINT32 element_size;

    element_size = sizeof(COMMF_RX_BUF_GROUP_IMPL)/sizeof(UNSIGNED);

    memset(&(device->rx_buf_grp_queue), 0, sizeof(NU_QUEUE));

    status = NU_Create_Queue(
            &(device->rx_buf_grp_queue),
            "COMMFRBG",
            device->rx_buf_grp_queue_buffer,
            (COMMF_BUF_GRP_QUEUE_COUNT*element_size),
            NU_FIXED_SIZE,
            element_size,
            NU_FIFO);

    return status;
}

/*************************************************************************
*
* FUNCTION
*
*       COMMF_Uninit_Rx_Buff_Grp_Queue
*
* DESCRIPTION
*
*       Un-initialize RX buffer group queue.
*
* INPUTS
*
*       device                              Pointer to device's control
*                                           block.
*
* OUTPUTS
*
*       NU_SUCCESS                          RX buffer group queue has
*                                           successfully been
*                                           un-initialized.
*
*************************************************************************/
STATUS    COMMF_Uninit_Rx_Buff_Grp_Queue (
                USBF_COMM_DEVICE * device)
{
    STATUS status;

    status = NU_Delete_Queue(
            &(device->rx_buf_grp_queue));

    return status;
}

/* ======================  End Of File  =============================== */
