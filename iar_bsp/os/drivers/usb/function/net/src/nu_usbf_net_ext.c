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
*
*     nu_usbf_net_ext.c
*
* COMPONENT
*     Nucleus USB software : Nucleus NET Driver
*
* DESCRIPTION
*     This file contains the interface layer of function ethernet driver with
*     Nucleus NET.
*
* DATA STRUCTURES
*     None
*
* FUNCTIONS
*
*     NU_USBF_COMM_ETH_Init             Initialization function.
*     NU_USBF_NET_Init                  Creates necessary framework.
*     NU_USBF_NET_Check_Connection      Check if device is connected on USB.
*     NU_USBF_NET_Open_Dev              Boot routine.
*     NU_USBF_NET_Xmit_Packet           Data sending service.
*     NU_USBF_NET_Ioctl                 Managing multicast groups.
*     NU_USBF_NET_Rx_Buff_Init          Handler for in coming data packets.
*     NU_USBF_NET_Submit_Rx_Buffers     Handler for incoming response events.
*     NU_USBF_NET_Notify                USB Event Notification callback.
*     NU_USBF_NET_Rcvd_Cb               Data reception callback.
*     NU_USBF_NET_Tx_Done               Data transmission callback.
*     NU_USBF_NET_ETH_Ioctl             IOCTL routine.
*     NU_USBF_NET_Buff_Finished         USB Receive buffer group finish callback.
*     NU_USBF_NET_Mark_buffer_Free      Marking used buffers free in list
*     NU_USBF_NET_Buffer_Disconn        Making buffers free at disconnection.
*
* DEPENDENCIES
*     nu_usb.h              All USB definitions
*
**************************************************************************/

/* =====================  USB Include Files ===========================  */
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "connectivity/nu_usb.h"

#if (INCLUDE_SNMP == NU_TRUE)
#include SNMP_GLUE
#endif

NU_EVENT_GROUP              Usb_Init;
NU_USBF_NET_DRVR            USBF_NET_Drvr;

COMMF_DATA_CONF  FNET_Data_Conf = {NU_NULL,
                                   NU_NULL,
                                   0x00,
                                   NU_NULL,
                                   NU_NULL,
                                   0x00,
                                   NU_NULL,
                                   NU_NULL,
                                   0x00,
                                   NU_TRUE};


/**************************************************************************
* FUNCTION
*     NU_USBF_COMM_ETH_Init
*
* DESCRIPTION
*     Ethernet user driver interface initialization routine
*
* INPUTS
*     device                Pointer to NET device.
*
* OUTPUTS
*     NU_SUCCESS            Indicates successful completion.
*
**************************************************************************/

STATUS NU_USBF_COMM_ETH_Init (DV_DEVICE_ENTRY* device)
{

    INT i;
    NU_USBF_NET_DEV*    net_device;

    /* MAC address of Nucleus NET host. */
    UINT8 mac_addr[] = {00, 01, 02, 03, 04, 05};

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    net_device = NU_USBF_NET_Find_Net_Device_By_Name(device->dev_net_if_name);
    if ( net_device == NU_NULL)
    {
        NU_USER_MODE();
        return (NU_NOT_PRESENT);
    }

    device->dev_open       = NU_USBF_NET_Open_Dev;
    device->dev_start      = NU_USBF_NET_Xmit_Packet;
    device->dev_output     = NET_Ether_Send;
    device->dev_input      = NET_Ether_Input;
    device->dev_ioctl      = NU_USBF_NET_Ioctl;
    device->dev_type       = DVT_ETHER;
    device->dev_addrlen    = 6;
    device->dev_hdrlen     = 14;   /* Size of an ethernet header. */
    device->dev_mtu        = NF_MAXIMUM_FRAME_SIZE;

    /* Set the device MAC Address */
    for(i=0; i<6; i++)
    {
        device->dev_mac_addr[i] = mac_addr[i];
    }

    device->dev_flags |= (DV_BROADCAST | DV_SIMPLEX | DV_MULTICAST);

    /* Initialize the basic interface information. */
    SNMP_ifDescr(device->dev_index, "USB Ethernet: Software revision 1.0");
    SNMP_ifType(device->dev_index, 6);
    SNMP_ifMtu(device->dev_index, device->dev_mtu);
    SNMP_ifSpeed(device->dev_index, 10000000); /* 10 Mbps */

    /* Initialize the Physical address in the MIB. */
    SNMP_ifPhysAddress(device->dev_index, device->dev_mac_addr);

    /* Save device in USBF net driver control block. */
    net_device->device_entry = device;

    /*  Initialize the device.  */
    (device->dev_open)(device->dev_mac_addr,
                       device );
    /* Revert to user mode. */
    NU_USER_MODE();

    return NU_SUCCESS;
}

/**************************************************************************
* FUNCTION
*     NU_USBH_NET_Initialize
*
* DESCRIPTION
*     Entry function of the Net driver for initilizing internals.
*
* INPUTS
*     None.
*
* OUTPUTS
*     NU_SUCCESS            Indicates successful completion.
*
**************************************************************************/
STATUS NU_USBF_NET_Initialize(CHAR* path)
{
    STATUS status = NU_SUCCESS;
    static BOOLEAN init_flag = NU_FALSE;
    NU_USBF_NET_DEV* net_device;
    UINT8 idx;

    if(init_flag == NU_FALSE)
    {
        memset(&USBF_NET_Drvr,0x00, sizeof(USBF_NET_Drvr));

        init_flag               = NU_TRUE;

        /* Set mem_pool to NU_NULL because in ReadyStart memory in USB
         * system is allocated through USB specific memory APIs, not
         * directly with any given memory pool pointer.*/
        USBF_NET_Drvr.mem_pool  = NU_NULL;
        strcpy(USBF_NET_Drvr.reg_path, path);

        status = NU_USBF_COMM_DATA_GetHandle((VOID**)&USBF_NET_Drvr.com_data_drvr);
        if ( status == NU_SUCCESS )
        {
            status = NU_USBF_NDIS_USER_GetHandle((VOID**)&USBF_NET_Drvr.ndis_pt);
            if ( status == NU_SUCCESS )
            {
                status = NU_USBF_ETH_GetHandle((VOID**)&USBF_NET_Drvr.eth_pt);
                if ( status == NU_SUCCESS )
                {
                    status = NU_USBF_NDIS_USER_Register_Cb(USBF_NET_Drvr.ndis_pt,
                                                        NU_USBF_NET_Notify,
                                                        NU_USBF_NET_Rcvd_Cb,
                                                        NU_USBF_NET_Tx_Done,
                                                        NU_USBF_NET_ETH_Ioctl);
                    if ( status == NU_SUCCESS )
                    {
                        status = NU_USBF_ETH_Register_Cb(USBF_NET_Drvr.eth_pt,
                                                        NU_USBF_NET_Notify,
                                                        NU_USBF_NET_Rcvd_Cb,
                                                        NU_USBF_NET_Tx_Done,
                                                        NU_USBF_NET_ETH_Ioctl);
                    }
                }
            }
        }
    }
    
    if(status == NU_SUCCESS)
    {
        status = USB_Allocate_Object(sizeof(NU_USBF_NET_DEV),
                                           (VOID **)&net_device);
        if(status == NU_SUCCESS)
        {
            memset(net_device,0x00,sizeof(NU_USBF_NET_DEV));

            NU_Create_Semaphore(&net_device->tx_sem,
                                "usbf_net",
                                0,
                                NU_NO_SUSPEND);

            NU_Place_On_List((CS_NODE**)&USBF_NET_Drvr.head_device,
                             (CS_NODE*)net_device);

            net_device->usb_device  = NU_NULL;
            net_device->user        = NU_NULL;
            net_device->link_status = USBF_DEV_LINK_DOWN;
        
            status = USB_Allocate_Aligned_Memory(USB_MEM_TYPE_UNCACHED,
                                                (NF_MAX_DATA_XFER_SIZE),
                                                32,
                                                (VOID **)&(net_device->Tx_Buff));
                                                                      

        }
                            
        if ( status == NU_SUCCESS )
        {
            /* Allocate memory for buffer groups. */
            for(idx=0; idx<ETHF_NUM_RX_GRP_BUFS; idx++)
            {
                status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                             ETHF_NET_MAX_SEG_SIZE,
                                             (VOID **)&net_device->Buffer_List1[idx]);
                if ( status != NU_SUCCESS )
                {
                    break;
                }
            }

            if ( status == NU_SUCCESS )
            {
                /* Allocate memory for buffer groups. */
                for(idx=0; idx<ETHF_NUM_RX_GRP_BUFS; idx++)
                {
                    status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
                                                 ETHF_NET_MAX_SEG_SIZE,
                                                 (VOID **)&net_device->Buffer_List2[idx]);
                    if ( status != NU_SUCCESS )
                    {
                        break;
                    }
                }

                if ( status == NU_SUCCESS )
                {
                    NU_USBF_NET_Rx_Buff_Init(net_device);

                    /* Configure the data transfer internal queues etc of class driver
                     * for this user/application.
                     */

                     NU_USB_SYS_Register_Device(net_device,
                                                NU_USBCOMPF_NET);
                }
            }
        }
    }

    return (status);
}


/**************************************************************************
* FUNCTION
*     NU_USBF_NET_Init
*
* DESCRIPTION
*     USB Ethernet NET driver interface initialization routine.
*
* INPUTS
*     None.
*
* OUTPUTS
*     NU_SUCCESS            Indicates successful completion.
*
**************************************************************************/
STATUS NU_USBF_NET_Init(VOID)
{
    STATUS status ;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    status = NU_Create_Event_Group(&Usb_Init,
                                   "USBINIT");

    /* Revert to user mode. */
    NU_USER_MODE();

    return status;
}

/**************************************************************************
* FUNCTION
*     NU_USBF_NET_Check_Connection
*
* DESCRIPTION
*     To check if the USB function device is connected over USB or not.
*
* INPUTS
*     device                Pointer to NET device.
*     suspend               Option to suspend or not while waiting for
*                           connection. Valid options are NU_SUSPEND and
*                           NU_NO_SUSPEND.
* OUTPUTS
*     NU_SUCCESS            Indicates successful completion.
*
**************************************************************************/
STATUS NU_USBF_NET_Check_Connection(NU_DEVICE*  device,
                                    UINT32      suspend)
{
    STATUS status ;
    UINT32 requested = 0x00;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    /* Wait until USB Device is connected and configured by Host. */
    status = NU_Retrieve_Events(&Usb_Init,
                                1,
                                NU_OR_CONSUME,
                                &requested,
                                suspend);

    if(status == NU_SUCCESS)
    {
       status = NU_SUCCESS;
    }
    else if(status == NU_NOT_PRESENT)
    {
        status = NU_NOT_CONNECTED;
    }

    /* Revert to user mode. */
    NU_USER_MODE();

    return status;
}

/**************************************************************************
* FUNCTION
*     NU_USBF_NET_Open_Dev
*
* DESCRIPTION
*     Application's ethernet controller boot routine
*
* INPUTS
*     ether_addr     Pointer to string containing ethernet address.
*     device         Pointer to NET device.
*
* OUTPUTS
*     NU_SUCCESS            Indicates successful completion.
*
**************************************************************************/

STATUS NU_USBF_NET_Open_Dev (
       UINT8*           ether_addr,
       DV_DEVICE_ENTRY* device)
{
    STATUS status = NU_SUCCESS;
    return(status);
}

/**************************************************************************
* FUNCTION
*     NU_USBF_NET_Xmit_Packet
*
* DESCRIPTION
*     Application's data send routine
*
* INPUTS
*     device        Pointer to NET device.
*     buf_ptr       Pointer to buffer containing packet data.
*
* OUTPUTS
*     NU_SUCCESS                   Indicates successful completion.
*
**************************************************************************/
STATUS NU_USBF_NET_Xmit_Packet(
       DV_DEVICE_ENTRY*  device,
       NET_BUFFER*       buf_ptr)
{

    STATUS              status = NU_SUCCESS;
    UINT8 *             frame_buffer;
    UINT32              frame_length = 0;
    NET_BUFFER*         next_buf;
    NU_USBF_NET_DEV*    net_device;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();
    
    net_device = NU_USBF_NET_Find_Net_Device(device);

    if ( (net_device    == NU_NULL)             ||
         (net_device->usb_device    == NU_NULL) ||
         (net_device->user          == NU_NULL) )
    {
        NU_USER_MODE();
        return (NU_NOT_PRESENT);
    }

    frame_buffer = (UINT8*)((UINT32)net_device->Tx_Buff + net_device->Ethf_Offset);
    next_buf = buf_ptr;

    while(next_buf)
    {
        /* Copy the data */
        memcpy(frame_buffer, next_buf->data_ptr, next_buf->data_len);
        frame_buffer = frame_buffer + next_buf->data_len;
        frame_length = frame_length + next_buf->data_len;
        next_buf = next_buf ->next_buffer;
    }

    if(net_device->user == (NU_USBF_USER*)USBF_NET_Drvr.ndis_pt)
    {
        status = NU_USBF_NDIS_Send_Eth_Frame(USBF_NET_Drvr.ndis_pt,
                                        net_device->usb_device,
                                        (UINT8*)net_device->Tx_Buff,
                                        frame_length,
                                        NU_NULL,
                                        NU_NULL);
    }
    else if(net_device->user == (NU_USBF_USER*)USBF_NET_Drvr.eth_pt)
    {
        status = NU_USBF_ETH_Send_Data(USBF_NET_Drvr.eth_pt,
                                       net_device->usb_device,
                                       (UINT8*)net_device->Tx_Buff,
                                       frame_length);
    }


    /* Revert to user mode. */
    NU_USER_MODE();

    return(status);

}

/**************************************************************************
* FUNCTION
*     NU_USBF_NET_Ioctl
*
* DESCRIPTION
*     Application's characteristics control routine
*
* INPUTS
*     device    Pointer to NET device.
*     option    Option for this request.
*     d_req     Pointer to device request.
*
* OUTPUTS
*     NU_SUCCESS            Indicates successful completion.
*
**************************************************************************/
STATUS  NU_USBF_NET_Ioctl(
        DV_DEVICE_ENTRY* device,
        INT              option,
        DV_REQ*          d_req)
{
    STATUS status = NU_SUCCESS;
    return(status);
}

/*************************************************************************
* FUNCTION
*       NU_USBF_NET_Rx_Buff_Init
*
* DESCRIPTION
*       This function initializes the receive buffer groups used by
*       ethernet for receiving data from Host.
*
* INPUTS
*       device.
*
* OUTPUTS
*       None.
*
*************************************************************************/

VOID NU_USBF_NET_Rx_Buff_Init(NU_USBF_NET_DEV* device)
{
    /* Initialize the receive buffer groups using class driver calls. */
    NU_USBF_COMM_DATA_Rbg_Create(&device->Rx_Group1, NU_USBF_NET_Buff_Finished,
                                device->Buffer_List1, ETHF_NUM_RX_GRP_BUFS,
                                ETHF_NET_MAX_SEG_SIZE);

    NU_USBF_COMM_DATA_Rbg_Create(&device->Rx_Group2, NU_USBF_NET_Buff_Finished,
                                device->Buffer_List2, ETHF_NUM_RX_GRP_BUFS,
                                ETHF_NET_MAX_SEG_SIZE);
}

/*************************************************************************
* FUNCTION
*       NU_USBF_NET_Submit_Rx_Buffers
*
* DESCRIPTION
*       This function submits the receive buffer group to class driver.
*
* INPUTS
*       group              Pointer to buffer group
*       device             Pointer to the communication device
* OUTPUTS
*       None.
*
*************************************************************************/

VOID NU_USBF_NET_Submit_Rx_Buffers(COMMF_RX_BUF_GROUP *group,
                                   NU_USBF_NET_DEV*   device)
{
    /* Register the specified buffer group with the class driver. */
    NU_USBF_COMM_DATA_Reg_Rx_Buffer(USBF_NET_Drvr.com_data_drvr,
                    device->usb_device,
                    group);
}

/*************************************************************************
* FUNCTION
*       NU_USBF_NET_Notify
*
* DESCRIPTION
*       This function is notification callback of both Ethernet and RNDIS
*       user drivers. User driver notifies the USB bus event and Host
*       configuration events through this callback.
*
* INPUTS
*       user              Pointer to user driver sending this notification.
*       event             Event code
*       handle            Handle for current user assigned by class driver.
*
* OUTPUTS
*       None.
*
*************************************************************************/
void NU_USBF_NET_Notify (NU_USBF_USER *user,
                         UINT32        event,
                         VOID         *handle)
{

    STATUS          status;
    NU_USBF_NET_DEV *net_device;
    DV_DEV_ID       dev_id;
    NET_IF_LINK_STATE_MSG    if_status_change_msg;

    /* Only handling Host configuration event in this callback. */
    if(event == COMMF_EVENT_USER_INIT)
    {
        net_device = NU_USBF_NET_Find_Device(NU_NULL);
        if(net_device)
        {
            net_device->usb_device = handle;
            net_device->user       = user;

            status = NU_USBF_COMM_DATA_Config_Xfers(USBF_NET_Drvr.com_data_drvr,
                                                    net_device->usb_device,
                                                    &FNET_Data_Conf);
            if(status != NU_SUCCESS)
            {
                return;
            }

            /* Register both receive buffer groups in beginning. */
            NU_USBF_NET_Submit_Rx_Buffers(&net_device->Rx_Group1, net_device);
            NU_USBF_NET_Submit_Rx_Buffers(&net_device->Rx_Group2, net_device);

            /* Initialize user dependent demo variables. */
            if(user == (NU_USBF_USER *)USBF_NET_Drvr.ndis_pt)
            {
                net_device->Ethf_Offset = 44;
            }
            else if(user == (NU_USBF_USER *)USBF_NET_Drvr.eth_pt)
            {
                net_device->Ethf_Offset = 0;
            }

            status = NU_USB_SYS_Get_Dev_ID(net_device, &dev_id);
            if ( status == NU_SUCCESS )
            {
                net_device->link_status = USBF_DEV_LINK_UP;
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
        }
        else
        {

        }
    }
    else if(event == COMMF_EVENT_USER_DEINIT)
    {
        net_device = NU_USBF_NET_Find_Device(handle);
        if(net_device)
        {
            status = NU_USB_SYS_Get_Dev_ID(net_device, &dev_id);
            if ( status == NU_SUCCESS )
            {
                net_device->link_status = USBF_DEV_LINK_DOWN;
                strcpy(if_status_change_msg.msg, "LINK DOWN");
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

            net_device->usb_device = NU_NULL;
            net_device->user       = NU_NULL;
        }
    }
}

/*************************************************************************
* FUNCTION
*       NU_USBF_NET_Rcvd_Cb
*
* DESCRIPTION
*       This function is callback for the received data from Host. This
*       is common for Ethernet and RNDIS. Application will read the data
*       at offset specific for currently active user.
*
* INPUTS
*       data_buffer               Pointer to ethernet data.
*       eth_frame_length          Length of ethernet data.
*       pkt_info_data             Pointer to info buffer (only for RNDIS)
*       pkt_info_length           Length of information (only for RNDIS)
*       handle                    Pointer to the communication device
*
* OUTPUTS
*       None.
*
*************************************************************************/
VOID NU_USBF_NET_Rcvd_Cb(UINT8 * data_buffer,
                         UINT32 eth_frame_length,
                         UINT8 *pkt_info_data,
                         UINT32 pkt_info_length,
                         VOID  *handle
)
{
    NET_BUFFER      *rcvd_buffer;
    NET_BUFFER      *work_buffer;
    UINT32          bytes_left;
    UINT8           *src_ptr;
    UINT8           *dest_ptr;
    NU_USBF_NET_DEV *net_device;

    net_device = NU_USBF_NET_Find_Device(handle);
    if ( net_device != NU_NULL )
    {
        rcvd_buffer = MEM_Buffer_Chain_Dequeue(&MEM_Buffer_Freelist, (INT32)eth_frame_length);

        if (rcvd_buffer != NU_NULL)
        {
            bytes_left = eth_frame_length;

            rcvd_buffer->mem_buf_device = net_device->device_entry;
            rcvd_buffer->mem_flags      = 0;
            rcvd_buffer->mem_total_data_len = eth_frame_length;
            src_ptr = (data_buffer + net_device->Ethf_Offset);

            work_buffer = rcvd_buffer;

            while (bytes_left)
            {
                if (work_buffer == rcvd_buffer)
                {
                    /* Store length of buffer based upon remaining bytes. */
                    work_buffer->data_len =
                                    (bytes_left <= NET_PARENT_BUFFER_SIZE) ?
                                    bytes_left :  NET_PARENT_BUFFER_SIZE;

                    /* Set buffer data pointer to point to the parent. */
                    work_buffer->data_ptr = work_buffer->mem_parent_packet;
                }
                else
                {
                    /* Store length of buffer based upon remaining bytes. */
                    work_buffer->data_len =
                                    (bytes_left <= NET_MAX_BUFFER_SIZE) ?
                                    bytes_left :  NET_MAX_BUFFER_SIZE;

                    /* Set buffer data pointer to point to the packet. */
                    work_buffer->data_ptr = work_buffer->mem_packet;
                }

                dest_ptr = (UINT8 *)work_buffer->data_ptr;

                memcpy(dest_ptr, src_ptr, work_buffer->data_len);

                src_ptr += work_buffer->data_len;

                /* Update a count of the bytes that still must be moved. */
                bytes_left -= work_buffer->data_len;

                /* Move to the next buffer in the chain */
                work_buffer = (NET_BUFFER *)work_buffer->next_buffer;

            } /* while (bytes_left) */

            /* Put head of the chain onto incoming packet buffer
               list. */
            MEM_Buffer_Enqueue(&MEM_Buffer_List, rcvd_buffer);

            NU_Set_Events(&Buffers_Available, (UNSIGNED)2, NU_OR);
        }
    }
}

/*************************************************************************
* FUNCTION
*       NU_USBF_NET_Tx_Done
*
* DESCRIPTION
*       This function is transmission complete callback both for Ethernet
*       and RNDIS.
*
* INPUTS
*       cmpltd_data_buffer              Pointer to completed data buffer.
*       length                          Length of completed data
*       handle                          Pointer to the communication
*                                       device.
*
* OUTPUTS
*       None.
*
*************************************************************************/
VOID NU_USBF_NET_Tx_Done(UINT8  *cmpltd_data_buffer,
                         UINT32 length,
                         VOID   *handle)

{

    NU_USBF_NET_DEV*  net_device = NU_USBF_NET_Find_Device(handle);

    if(net_device == NU_NULL)
    {
        return;
    }

    else if(net_device->device_entry->dev_transq.head)
    {
        /* Remove the packet from the list. */
        DEV_Recover_TX_Buffers(net_device->device_entry);

        if(net_device->device_entry->dev_transq.head)
        {
            NU_USBF_NET_Xmit_Packet(net_device->device_entry,
                                    net_device->device_entry->dev_transq.head);
        }
        else
        {
            NU_Release_Semaphore(&net_device->tx_sem);
        }
    }
    else
    {
        NU_Release_Semaphore(&net_device->tx_sem);
    }
}

/*************************************************************************
* FUNCTION
*       NU_USBF_NET_ETH_Ioctl
*
* DESCRIPTION
*       This function reports received command to application from Host.
*       For RNDIS, this function is called when SET_MESSAGE request is
*       received from Host. OID is reported in code and SET_MESSAGE data
*       is reported by data and length arguments. For ethernet user, this
*       this function is called for SEND_ENCAP_CMD, SET_ETH_FILTER and
*       SET_PWR_MGMT_FILTER commands. Data associated with each command is
*       reported through data and length arguments.
*
* INPUTS
*       user              User sending this callback.
*       code              Code for the specific command.
*       data              Pointer to command associated data.
*       length            Length of command associated data.
*
* OUTPUTS
*       None.
*
*************************************************************************/

VOID NU_USBF_NET_ETH_Ioctl (NU_USBF_USER *user, UINT32 code, VOID *data,
                                                 UINT32 length)
{
}

/*************************************************************************
* FUNCTION
*       NU_USBF_NET_Buff_Finished
*
* DESCRIPTION
*       This function receive buffer group finish callback.
*
* INPUTS
*       user              Pointer to user driver send this callback.
*       buffer_grp        Pointer to finished buffer group.
*       handle            Pointer to the communication device.
*
* OUTPUTS
*       None.
*
*************************************************************************/

VOID NU_USBF_NET_Buff_Finished(NU_USBF_USER  *user,
                               VOID          *buffer_grp,
                               VOID          *handle)
{
    NU_USBF_NET_DEV*   net_device = NU_USBF_NET_Find_Device(handle);

    if(net_device)
    {
        NU_USBF_NET_Submit_Rx_Buffers((COMMF_RX_BUF_GROUP *)buffer_grp,
                                  net_device);
    }
}

/*************************************************************************
* FUNCTION
*       NU_USBF_NET_Mark_buffer_Free
*
* DESCRIPTION
*       This function is internal routine which marks a buffer as free when
*       the contained data is consumed by NET.
*
* INPUTS
*       net_device
*       data_buffer               Pointer to ethernet data.
*
* OUTPUTS
*       None.
*
*************************************************************************/
VOID NU_USBF_NET_Mark_buffer_Free(NU_USBF_NET_DEV *net_device,
                                  UINT8           *data_buffer)
{
    UINT32 i;
    for(i=0; i<ETHF_NUM_RX_GRP_BUFS; i++)
    {
        if(net_device->Rx_Group1.buffer_array[i] == data_buffer)
        {
            net_device->Rx_Group1.buffer_array[i] = NU_NULL;
        }
    }

    for(i=0; i<ETHF_NUM_RX_GRP_BUFS; i++)
    {
        if(net_device->Rx_Group2.buffer_array[i] == data_buffer)
        {
            net_device->Rx_Group2.buffer_array[i] = NU_NULL;
        }
    }
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_NET_DM_Open
*
* DESCRIPTION
*
*       This function is called by the application when it opens a device
*       for read/write/ioctl operations.
*
* INPUTS
*
*       dev_handle         Pointer to the net device passed as context.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*
**************************************************************************/
STATUS    NU_USBF_NET_DM_Open (VOID* dev_handle)
{
    STATUS          status;
    NU_USBF_NET_DEV *net_device;
    NU_DEVICE       *eth_dev;

    NU_USB_PTRCHK(dev_handle);

    net_device  = (NU_USBF_NET_DEV*) dev_handle;
    eth_dev     = &net_device->eth_dev;

    status = NU_USBF_ETH_DM_Open(USBF_NET_Drvr.eth_pt);
    NU_USB_ASSERT( status == NU_SUCCESS );

    status = NU_USBF_RNDIS_DM_Open(USBF_NET_Drvr.ndis_pt);
    NU_USB_ASSERT( status == NU_SUCCESS );

    eth_dev->dv_name                     = "";
    eth_dev->dv_hw.ether.dv_irq          = 0;
    eth_dev->dv_hw.ether.dv_io_addr      = 0;
    eth_dev->dv_init                     = &NU_USBF_COMM_ETH_Init;
    eth_dev->dv_hw.ether.dv_shared_addr  = 0;
    eth_dev->dv_flags                    = 0;

    net_device->eth_dev.dv_name = net_device->name;

    /* Create a name for USB function ethernet device.  */
    ETHERNET_Create_Name (net_device->eth_dev.dv_name);

    return (status) ;
}
/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_NET_DM_Close
*
* DESCRIPTION
*
*       This function is called by the application when it wants to close a device
*       which it has opend already for read/write/ioctl operations.
*
* INPUTS
*
*       dev_handle         Pointer to the net device passed as context.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*
**************************************************************************/

STATUS    NU_USBF_NET_DM_Close(VOID* dev_handle)
{
    STATUS status;
    status = NU_USBF_ETH_DM_Close(USBF_NET_Drvr.eth_pt);
    status = NU_USBF_RNDIS_DM_Close(USBF_NET_Drvr.ndis_pt);
    return (status) ;
}
/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_NET_DM_Read
*
* DESCRIPTION
*
*       This function is called by the application when it wants to read
*       data from the device. In net driver case this is a dummy function as
*       reading is done through buffer group registeration which is done
*       at the connection time.
*
* INPUTS
*
*       dev_handle         Pointer to the etherent driver passed as context.
*       buffer             Pointer to memory location where to put the read data.
*       numbyte            Number of bytes to be read.
*       byte_offset        In case read data is to be placed at certain offset in the buffer.
*       bytes_read_ptr     OUTPUT: Number of bytes actually read.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*
**************************************************************************/
STATUS NU_USBF_NET_DM_Read (VOID    *session_handle,
                            VOID    *buffer,
                            UINT32   numbyte,
                            OFFSET_T byte_offset,
                            UINT32  *bytes_read)
{
    STATUS status = NU_SUCCESS;

    *bytes_read = numbyte;
     numbyte = 0;

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_NET_DM_Write
*
* DESCRIPTION
*
*       This function is called by the application when it wants to write
*       data from the device.
*
* INPUTS
*
*       dev_handle         Pointer to the etherent driver passed as context.
*       buffer             Pointer to memory location where data to be written is avaiable.
*       numbyte            Number of bytes to be written.
*       byte_offset        In case data is to be read at certain offset in the buffer.
*       bytes_written_ptr  OUTPUT: Number of bytes actually written.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*
**************************************************************************/
STATUS NU_USBF_NET_DM_Write (
                             VOID       *dev_handle,
                             const VOID       *buffer,
                             UINT32      numbyte,
                             OFFSET_T    byte_offset,
                             UINT32     *bytes_written)
{
    STATUS                status = NU_SUCCESS;
    NET_BUFFER            *buf_ptr = (NET_BUFFER *)buffer;
    NU_USBF_NET_DEV* net_device = (NU_USBF_NET_DEV*)dev_handle;

    /* Transmit the packet */
    status = NU_USBF_NET_Xmit_Packet(net_device->device_entry,
                                     buf_ptr);

    status = NU_Obtain_Semaphore(&net_device->tx_sem,
                                  NU_SUSPEND);
    *bytes_written = numbyte;
    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_NET_DM_IOCTL
*
* DESCRIPTION
*
*       This function is called by the application when it wants to perform a control
*       operation on the device.
*
* INPUTS
*
*       session_handle     Pointer to the etherent driver passed as context.
*       cmd                IOCTL number.
*       data               IOCTL data pointer of variable type.
*       length             IOCTL data length in bytes.
*
* OUTPUTS
*
*       NU_SUCCESS         Indicates successful completion.
*
**************************************************************************/
STATUS NU_USBF_NET_DM_IOCTL(VOID     *session_handle,
                            INT       cmd,
                            VOID     *data,
                            INT       length)
{
    STATUS                  status = NU_SUCCESS;
    DV_DEV_HANDLE           *dev_handle;
    NU_DEVICE               *eth_mw;
    DV_DEVICE_ENTRY         *device_entry;
    DV_DEV_ID               dev_id;
    INT                     *link_status;
    CHAR                    reg_path[REG_MAX_KEY_LENGTH];
    NET_IF_LINK_STATE_MSG   if_status_change_msg;
    ETHERNET_CONFIG_PATH    *config_path;
    NU_USBF_NET_DEV *pdevice = (NU_USBF_NET_DEV *)session_handle;
    NU_USBF_NDIS_MAC_DATA  *mac_data;
    UINT8 idx;

    /* Process command */
    switch (cmd)
    {
        /*******************/
        /* Ethernet Ioctls */
        /*******************/
        case (USB_ETHERNET_IOCTL_BASE + ETHERNET_CMD_SET_DEV_HANDLE):

            /* Get the dev handle from the data passed in */
            dev_handle = (DV_DEV_HANDLE *) data;

            /* Save the handle to the device structure */
            pdevice->device_handle = *dev_handle;
            pdevice->eth_dev.dev_handle = *dev_handle;
            status = NU_SUCCESS;
            break;
        case (USB_ETHERNET_IOCTL_BASE + ETHERNET_CMD_GET_DEV_STRUCT):
            /* Get the NU_DEVICE structure from the data passed in */
            eth_mw = (NU_DEVICE *) data;

            /* Return the populated structure */
            *eth_mw = pdevice->eth_dev;
            status = NU_SUCCESS;
            break;
        case (USB_ETHERNET_IOCTL_BASE + ETHERNET_CMD_CTRL_INIT):
             device_entry = (DV_DEVICE_ENTRY *)data;

             pdevice->device_entry = device_entry;
             status = NU_SUCCESS;
             break;
        case (USB_ETHERNET_IOCTL_BASE + ETHERNET_CMD_GET_CONFIG_PATH):
            /* Get the Config Path structure from the data passed in */
            config_path = (ETHERNET_CONFIG_PATH *) data;

            /* Return the middleware config path */
            strcpy(reg_path, USBF_NET_Drvr.reg_path);
            if(strlen(reg_path) <= ((REG_MAX_KEY_LENGTH -1) - strlen("/mw_settings")))
            {
                strcat(reg_path, "/mw_settings");
            }
            if(config_path->max_path_len <= REG_MAX_KEY_LENGTH)
            {
                strncpy(config_path->config_path, reg_path, config_path->max_path_len);
                status = NU_SUCCESS;
                break;
            }
        case (USB_ETHERNET_IOCTL_BASE + ETHERNET_CMD_SEND_LINK_STATUS):
            status = NU_USB_SYS_Get_Dev_ID(pdevice, &dev_id);
            if ( status == NU_SUCCESS )
            {
                if (pdevice->link_status == USBF_DEV_LINK_UP)
                {
                    strcpy(if_status_change_msg.msg, "LINK UP");
                }
                else
                {
                    strcpy(if_status_change_msg.msg, "LINK DOWN");
                }

#if (CFG_NU_OS_NET_STACK_EQM_SUPPORT_ENABLED == NU_TRUE)
                if_status_change_msg.dev_id = dev_id;
                if_status_change_msg.event_type = LINK_CHANGE_STATE;
                NU_EQM_Post_Event(&NET_Eqm, (EQM_EVENT*)(&if_status_change_msg),
                                  sizeof(if_status_change_msg), NU_NULL);
#else
                /* Use EVENT manager to send this change */
                NU_Notification_Send (dev_id,
                                      LINK_CHANGE_STATE,
                                      if_status_change_msg.msg,
                                      strlen(if_status_change_msg.msg)+1);
#endif
            }
            break;
        case (USB_ETHERNET_IOCTL_BASE + ETHERNET_CMD_GET_LINK_STATUS):
            link_status = (INT *)data;
            *link_status = pdevice->link_status;
            status = NU_SUCCESS;
            break;
        case (USB_ETHERNET_IOCTL_BASE + ETHERNET_CMD_GET_XDATA):
        case (USB_ETHERNET_IOCTL_BASE + ETHERNET_CMD_TARGET_INIT):
        case (USB_ETHERNET_IOCTL_BASE + ETHERNET_CMD_PHY_INIT):
        case (USB_ETHERNET_IOCTL_BASE + ETHERNET_CMD_GET_ISR_INFO):
        case (USB_ETHERNET_IOCTL_BASE + ETHERNET_CMD_CTRL_ENABLE):
            status = NU_UNAVAILABLE;
            break;

        case (USB_ETHERNET_IOCTL_BASE + ETHERNET_CMD_SET_HW_ADDR):
            mac_data = (NU_USBF_NDIS_MAC_DATA *) data;

            status = NU_USBF_NDIS_Set_MAC_Address((NU_USBF_NDIS_USER*) pdevice ->user,
                                                   mac_data->handle,
                                                   mac_data->mac_addr,
                                                   NF_MAC_ADDR_LEN);

            memcpy(pdevice->device_entry->dev_mac_addr, mac_data->mac_addr, 6);
            break;

        case (USB_ETHERNET_IOCTL_BASE + ETHERNET_CMD_GET_HW_ADDR):

            mac_data = (NU_USBF_NDIS_MAC_DATA *) data;

            status =  NU_USBF_NDIS_Get_MAC_Address((NU_USBF_NDIS_USER*) pdevice ->user,
                                                   mac_data->handle,
                                                   mac_data->mac_addr);
            break;
        /*******************/
        /* Net Ioctls      */
        /*******************/
        case (USB_NET_IOCTL_BASE + ETHERNET_CMD_DEV_ADDMULTI):
        case (USB_NET_IOCTL_BASE + ETHERNET_CMD_DEV_DELMULTI):
            status = NU_UNAVAILABLE;
            break;
        case (USB_NET_IOCTL_BASE + ETHERNET_CMD_REMDEV):

            NU_Remove_From_List((CS_NODE**)&USBF_NET_Drvr.head_device,
                               (CS_NODE*)pdevice);
            
            NU_USB_SYS_DeRegister_Device (pdevice,
                                          NU_USBCOMPF_NET);

            for(idx=0; idx<ETHF_NUM_RX_GRP_BUFS; idx++)
            {
                USB_Deallocate_Memory(pdevice->Buffer_List1[idx]);
            }
    
            for(idx=0; idx<ETHF_NUM_RX_GRP_BUFS; idx++)
            {
                USB_Deallocate_Memory(pdevice->Buffer_List2[idx]);
            }

            USB_Deallocate_Memory(pdevice->Tx_Buff);

            NU_Delete_Semaphore(&(pdevice->tx_sem));

            USB_Deallocate_Memory(pdevice);

            break;

      default:
            status = NU_UNAVAILABLE;
            break;
    }

    return status;
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_NET_Find_Device
*
* DESCRIPTION
*
*       This function is internally called to return a net device on the
*       basis of USB device.
*
* INPUTS
*
*       handle             Pointer to USB user device.
*
* OUTPUTS
*
*       NU_USBF_NET_DEV    Pointer to device found.
*
**************************************************************************/

NU_USBF_NET_DEV* NU_USBF_NET_Find_Device(VOID* handle)
{
    NU_USBF_NET_DEV* curr_device;
    curr_device = USBF_NET_Drvr.head_device;

    if(curr_device != NU_NULL)
    {
        for(;;)
        {
            if(curr_device->usb_device == handle)
            {
                break;
            }
            else if (curr_device->link_dev.cs_next == (CS_NODE*)curr_device)
            {
                curr_device = NU_NULL;
                break;
            }
            else
            {
                curr_device = (NU_USBF_NET_DEV*)&curr_device->link_dev.cs_next;
            }
        }
    }
    return (curr_device);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_NET_Find_Net_Device
*
* DESCRIPTION
*
*       This function is internally called to return a net device on the
*       basis of device entry device.
*
* INPUTS
*
*       handle             Pointer to DV_DEVICE_Entry device.
*
* OUTPUTS
*
*       NU_USBF_NET_DEV    Pointer to device found.
*
**************************************************************************/
NU_USBF_NET_DEV* NU_USBF_NET_Find_Net_Device(VOID* handle)
{
    NU_USBF_NET_DEV* curr_device;
    curr_device = USBF_NET_Drvr.head_device;
    if(curr_device != NU_NULL)
    {
        for(;;)
        {
            if(curr_device->device_entry == handle)
            {
                break;
            }
            else if (curr_device->link_dev.cs_next == (CS_NODE*)curr_device)
            {
                curr_device = NU_NULL;
                break;
            }
            else
            {
                curr_device = (NU_USBF_NET_DEV*)&curr_device->link_dev.cs_next;
            }
        }
    }
    return (curr_device);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USBF_NET_Find_Net_Device_By_Name
*
* DESCRIPTION
*
*       This function is internally called to return a net device on the
*       basis of net device name.
*
* INPUTS
*
*       CHAR *             Pointer to name of the device.
*
* OUTPUTS
*
*       NU_USBF_NET_DEV    Pointer to device found.
*
**************************************************************************/
NU_USBF_NET_DEV* NU_USBF_NET_Find_Net_Device_By_Name(CHAR* name)
{
    NU_USBF_NET_DEV* curr_device;
    curr_device = USBF_NET_Drvr.head_device;
    if(curr_device != NU_NULL)
    {
        for(;;)
        {
            if(strcmp(curr_device->name, name) == 0)
            {
                break;
            }
            else if (curr_device->link_dev.cs_next == (CS_NODE*)curr_device)
            {
                curr_device = NU_NULL;
                break;
            }
            else
            {
                curr_device = (NU_USBF_NET_DEV*)&curr_device->link_dev.cs_next;
            }
        }
    }
    return (curr_device);
}
