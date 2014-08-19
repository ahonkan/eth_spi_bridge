/**************************************************************************
*
*               Copyright 2005 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
* FILE NAME
*     nu_usbh_net_imp.c
*
* COMPONENT
*     Nucleus USB software : USB Ethernet to Nucleus NET interface.
*
* DESCRIPTION
*     This file contains the interface layer of ethernet driver with
*     Application
*
* DATA STRUCTURES
*     None
*
* FUNCTIONS
*
*     NU_USBH_COM_ETH_Open_Dev    Boot routine.
*     NU_USBH_COM_ETH_Xmit_Packet To send data.
*     NU_USBH_COM_ETH_Ioctl       Managing multicast groups.
*     Rcvd_Packet_Handler         Handler for incoming data packets.
*     Event_Handler               Handler for incoming response events.
*
* DEPENDENCIES
*     nu_usb.h              All USB definitions.
*
**************************************************************************/

/* =====================  USB Include Files ===========================  */
#include "connectivity/nu_usb.h"

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_ETH_Open_Dev
*
* DESCRIPTION
*     Application's ethernet controller boot routine
*
* INPUTS
*     ether_addr     Pointer to string containing ethernet address.
*     device         Pointer to controller control block.
*
* OUTPUTS
*     NU_SUCCESS            Indicates successful completion.
*
**************************************************************************/

STATUS NU_USBH_COM_ETH_Open_Dev (
       UINT8*           ether_addr,
       DV_DEVICE_ENTRY* device)
{
    STATUS status = NU_SUCCESS;

    return(status);
}

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_ETH_Xmit_Packet
*
* DESCRIPTION
*     Application's data send routine
*
* INPUTS
*     device        Pointer to ethernet controller control block
*     buf_ptr       Pointer to buffer containing control and packet data.
*
* OUTPUTS
*     NU_SUCCESS                   Indicates successful completion.
*     NU_USBH_COM_XFER_ERR         Indicates command didn't complete
*                                  successfully.
*     NU_USBH_COM_XFER_FAILED      Indicates command failed by Ethernet
*                                  driver.
*     NU_USBH_COM_NOT_SUPPORTED    Indicates that the feature is not
*                                  supported by the Ethernet driver.
*
*
**************************************************************************/

STATUS NU_USBH_COM_ETH_Xmit_Packet(
       DV_DEVICE_ENTRY*  device,
       NET_BUFFER*       buf_ptr)
{
    STATUS              status = NU_SUCCESS;
    VOID*               usb_buffer;

    NU_USBH_COM_ETH_DEVICE* pcb_eth_dev;

    NU_USBH_COM_XBLOCK  xblock;
    NU_USBH_COM_DEVICE* pcb_curr_device;
    NET_BUFFER*         push_buf_ptr;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();


    pcb_eth_dev =(NU_USBH_COM_ETH_DEVICE*)(device->user_defined_1);

    if(pcb_eth_dev == NU_NULL)
    {
        /* Revert to user mode. */
        NU_USER_MODE();
        return(NU_NOT_PRESENT);
    }


    pcb_curr_device =
    (NU_USBH_COM_DEVICE*)(pcb_eth_dev->user_device.usb_device);

    push_buf_ptr = buf_ptr;

    for(;;)
    {
        xblock.data_length = 0x00;
        xblock.direction   = NU_USBH_COM_DATA_OUT;
        while(buf_ptr)
        {
            xblock.data_length += buf_ptr->data_len;
            buf_ptr = buf_ptr->next_buffer;
        }

        status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                     xblock.data_length,
                                     &(xblock.p_data_buf));

        buf_ptr = push_buf_ptr;
        usb_buffer = xblock.p_data_buf;
        while(buf_ptr)
        {
            memcpy(xblock.p_data_buf,buf_ptr->data_ptr,buf_ptr->data_len);
            xblock.p_data_buf = (VOID*)
                            ((UINT32)xblock.p_data_buf +buf_ptr->data_len);
            buf_ptr = buf_ptr->next_buffer;
        }

        xblock.p_data_buf = usb_buffer ;

        status = NU_USBH_COM_Transfer_Out(pcb_curr_device,
                                          &xblock);

        USB_Deallocate_Memory(usb_buffer);

        if(device->dev_transq.head && status == NU_SUCCESS)
        {
            DEV_Recover_TX_Buffers(device);

            if(device->dev_transq.head)
            {
                buf_ptr = device->dev_transq.head;
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }
    }
    /* Revert to user mode. */
    NU_USER_MODE();

    return(status);
}

/**************************************************************************
* FUNCTION
*     NU_USBH_COM_ETH_Ioctl
*
* DESCRIPTION
*     Application's characteristics control routine
*
* INPUTS
*     device    Pointer to ethernet controller control block
*     option    Name of this USB object.
*     d_req     Memory pool to be used by user driver.
*
* OUTPUTS
*     NU_SUCCESS            Indicates successful completion.
*
**************************************************************************/
STATUS  NU_USBH_COM_ETH_Ioctl(
        DV_DEVICE_ENTRY* device,
        INT              option,
        DV_REQ*          d_req)
{
    STATUS status = NU_SUCCESS;

    return(status);
}

/**************************************************************************
* FUNCTION
*     NU_USBH_NET_Receive_Handler
*
* DESCRIPTION
*     Routine to handle incoming data packets. These packets are received
*     in a pre-allocated buffer of USB class driver and are then copied to
*     NET buffers in this routine.
*
* INPUTS
*     device_in      Pointer to ethernet controller control block.
*     xblock         Pointer to control block of Communication transfers.
*
* OUTPUTS
*
**************************************************************************/

VOID NU_USBH_NET_Receive_Handler (
     VOID*               device_in,
     NU_USBH_COM_XBLOCK* xblock)
{
    NET_BUFFER        *hdr_buffer  = NU_NULL;
    NET_BUFFER        *new_buffer  = NU_NULL;
    STATUS status;
    DV_DEV_ID       dev_id;
    NET_IF_LINK_STATE_MSG   if_status_change_msg;

    NU_USBH_COM_ETH_DEVICE* pcb_eth_dev =
                            (NU_USBH_COM_ETH_DEVICE*)device_in;

    NU_USBH_NET_DEV* net_device = (NU_USBH_NET_DEV*)pcb_eth_dev->net_dev;

    DV_DEVICE_ENTRY    *device =  (DV_DEVICE_ENTRY*)net_device->device_entry;

    NU_USBH_COM_XBLOCK pcb_com_xblock;
    UINT32             copy_count = 0x00;

    pcb_com_xblock = *(xblock);

    if(pcb_eth_dev->sys_var1 == 0x00)
    {
        status = NU_USB_SYS_Get_Dev_ID(pcb_eth_dev, &dev_id);
        if ( status == NU_SUCCESS )
        {
            strcpy(if_status_change_msg.msg, "LINK UP");
#if (CFG_NU_OS_NET_STACK_EQM_SUPPORT_ENABLED == NU_TRUE)
            if_status_change_msg.dev_id = dev_id;
            if_status_change_msg.event_type = LINK_CHANGE_STATE;
            NU_EQM_Post_Event(&NET_Eqm, (EQM_EVENT*)(&if_status_change_msg),
                              sizeof(if_status_change_msg), NU_NULL);
#else
            NU_Notification_Send (dev_id,
                                  LINK_CHANGE_STATE,
                                  if_status_change_msg.msg,
                                  strlen(if_status_change_msg.msg)+1);
#endif
        }
        pcb_eth_dev->sys_var1 = 0x01;

    }


    /* Submit buffers to NET only if data is availble. */
    if(pcb_com_xblock.transfer_length)
    {
        /* We can't receive more packets over USB unless buffer is availble.
         * The already recieved data can't be discarded because the adpter
         * device has successfully transmitted that to USB Host.
         */
        while(hdr_buffer == NU_NULL)
        {
            hdr_buffer = MEM_Buffer_Dequeue(&MEM_Buffer_Freelist);
            /* Waiting untill buffers are availbe to copy the data. */
            if(hdr_buffer == NU_NULL)
            {
                NU_Sleep(1);
            }
        }

        new_buffer = hdr_buffer;
        /* Get buffers from NET and copy the data into untill all the data
         * is transferred.
         */
        while(pcb_com_xblock.transfer_length > 0x00)
        {
            /* NET buffers are in chain-form so setting next to NULL. */
            new_buffer->next_buffer = NU_NULL;

            /* If this is the only or last buffer? */
            if(pcb_com_xblock.transfer_length <= NET_PARENT_BUFFER_SIZE)
            {
                copy_count = pcb_com_xblock.transfer_length;
                pcb_com_xblock.transfer_length = 0x00;
            }
            /* Need more buffer to copy the data. */
            else
            {
                copy_count = NET_PARENT_BUFFER_SIZE;
                pcb_com_xblock.transfer_length -= copy_count;

                /* Wait untill a buffer is availble from NET. */
                while(new_buffer->next_buffer == NU_NULL)
                {
                    new_buffer->next_buffer = MEM_Buffer_Dequeue(
                                               &MEM_Buffer_Freelist);
                    if(new_buffer->next_buffer == NU_NULL)
                    {
                        NU_Sleep(1);
                    }
                }
            }
            /* Copying the frame data . */
            memcpy(new_buffer,
                   pcb_com_xblock.p_data_buf,
                   copy_count);

            /* Setting up the buffer information. */
            new_buffer->data_ptr = (UINT8*)new_buffer;
            new_buffer->data_len = copy_count;
            new_buffer->mem_buf_device = device;
            pcb_com_xblock.p_data_buf = (VOID*)
                                        ((UINT32)(pcb_com_xblock.p_data_buf)
                                        +copy_count);
            /* Moving to the next buffer in the chain. */
            new_buffer = new_buffer->next_buffer;
        }
        /* Pointing NET to act on the incomming packet. Only to submit the
         * head of buffer cahin.
         */
        MEM_Buffer_Enqueue (&MEM_Buffer_List, hdr_buffer);
        NU_Set_Events(&Buffers_Available, (UNSIGNED)2, NU_OR);
    }
}

/**************************************************************************
* FUNCTION
*     Event_Handler
*
* DESCRIPTION
*     Application's routine to handle incoming responses.
*
* INPUTS
*     device         Pointer to ethernet controller control block
*     cb_com_xblock  Pointer to control block of Communication transfers.
*
* OUTPUTS
*
**************************************************************************/

VOID NU_USBH_NET_Event_Handler (VOID*               device,
                                NU_USBH_COM_XBLOCK* xblock)
{

}

