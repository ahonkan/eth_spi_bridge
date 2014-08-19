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
*     nu_usbh_net_ext.c
*
* COMPONENT
*     Nucleus USB software : USB ethernet and Nucleus NET interface.
*
* DESCRIPTION
*     This file contains the interface layer of ethernet driver with
*     application.
*
* DATA STRUCTURES
*     None
*
* FUNCTIONS
*
*     NU_USBH_NET_Init_Intf       Initialization function for NET device
*                                 interface.
*     NU_USBH_NET_Attach_Device   Called by application to report
*                                 USB connection.
*     NU_USBH_NET_Detach_Device   Called by application to report
*                                 USB disconnection.
*
* DEPENDENCIES
*     nu_usb.h              All USB definitions.
*
**************************************************************************/

/* =====================  USB Include Files ===========================  */
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "connectivity/nu_usb.h"

#if (INCLUDE_SNMP == NU_TRUE)
#include SNMP_GLUE
#endif

NU_USBH_COM_ETH_HANDL func_table = {
    NU_USBH_NET_New_Connection,
    NU_USBH_NET_Disconnection,
    NU_USBH_NET_Receive_Handler,
    NU_USBH_NET_Event_Handler
};
NU_USBH_NET_DRVR USBH_NET_Drvr;


/**************************************************************************
* FUNCTION
*     NU_USBH_NET_Init_Intf
*
* DESCRIPTION
*     Ethernet user driver interface initialization routine
*
* INPUTS
*     device                Pointer to ethernet controller control block
*
* OUTPUTS
*     NU_SUCCESS            Indicates successful completion.
*
**************************************************************************/

STATUS NU_USBH_NET_Init_Intf (DV_DEVICE_ENTRY* device)
{
    NU_USBH_NET_DEV         *net_device;
    NU_USBH_COM_ETH_DEVICE  *ptr_eth_dev;
    STATUS                  status;

    status      = NU_USB_INVLD_ARG;
    net_device  = NU_USBH_NET_Find_Device_From_NET(device);
    if ( net_device != NU_NULL )
    {
        ptr_eth_dev                 = net_device->usb_dev;
        net_device->device_entry    = device;

        status = NU_USBH_NET_Attach_Device (net_device->device_entry,
                                            ptr_eth_dev);
        if ( status == NU_SUCCESS )
        {
            device->dev_open       = NU_USBH_COM_ETH_Open_Dev;
            device->dev_start      = NU_USBH_COM_ETH_Xmit_Packet;
            device->dev_output     = NET_Ether_Send;
            device->dev_input      = NET_Ether_Input;
            device->dev_ioctl      = NU_USBH_COM_ETH_Ioctl;
            device->dev_type       = DVT_ETHER;
            device->dev_addrlen    = 6;
            device->dev_hdrlen     = 14;   /* Size of an ethernet header. */
            device->dev_mtu        = 1500;

            device->dev_flags |= (DV_BROADCAST | DV_SIMPLEX | DV_MULTICAST);

            /* Initialize the basic interface information. */
            SNMP_ifDescr(device->dev_index, "USB Ethernet: Software revision 1.0");
            SNMP_ifType(device->dev_index, 6);
            SNMP_ifMtu(device->dev_index, device->dev_mtu);
            SNMP_ifSpeed(device->dev_index, 10000000); /* 10 Mbps */

            /* Initialize the physical address in the MIB. */
            SNMP_ifPhysAddress(device->dev_index, device->dev_mac_addr);

            /*  Initialize the device.  */
            (device->dev_open)(device->dev_mac_addr,
                               device );
        }
    }

    return status;
}



/**************************************************************************
* FUNCTION
*     NU_USBH_NET_Entry
*
* DESCRIPTION
*     Nucleus USB NET driver initialization routine, creates the necessary
*     links to get report for new device connection.
*
* INPUTS
*     None
*
* OUTPUTS
*     NU_SUCCESS            Indicates successful completion.
*
**************************************************************************/

STATUS NU_USBH_NET_Entry(CHAR* path)
{
    STATUS status;

    /* Save registry path. */
    strcpy(USBH_NET_Drvr.reg_path, path);

    /* Get Nucleus USB host Ethernet handle. */
    status = NU_USBH_COM_ETH_GetHandle((VOID **)&USBH_NET_Drvr.ethernet_drvr);

    /* Register callbacks with the Ethernet user. */
    NU_USBH_COM_ETH_Reg_Hndlr (USBH_NET_Drvr.ethernet_drvr,
                              &func_table);

    return status;
}

/**************************************************************************
* FUNCTION
*     NU_USBH_NET_New_Connection
*
* DESCRIPTION
*     This routine is called by the ethernet driver when a new USB device
*     of ethernet type gets connected on the USB.
*
* INPUTS
*     pcb_user_drvr         Pointer to control block of ethernet user driver.
*     session               A unique identifier for the device.
*     information           A device specific information structure.
*
* OUTPUTS
*     NU_SUCCESS            Indicates successful completion.
*
**************************************************************************/

VOID NU_USBH_NET_New_Connection (NU_USB_USER* pcb_user_drvr,
                                 VOID*        session,
                                 VOID*        information)
{
    STATUS status;
    NU_USBH_NET_DEV* net_device;
    NU_USBH_COM_ETH_DEVICE* ether_device = (NU_USBH_COM_ETH_DEVICE*)session;

    status = USB_Allocate_Object(sizeof(NU_USBH_NET_DEV),
                                 (VOID **)&net_device);
    if(status == NU_SUCCESS)
    {
        memset(net_device,0x00,sizeof(NU_USBH_NET_DEV));
        ether_device->net_dev  =(VOID*) net_device;
        net_device->usb_dev =  ether_device;
        NU_Place_On_List((CS_NODE**)&USBH_NET_Drvr.head_device,
                         (CS_NODE*)net_device);

        net_device->pm_state = USB_POWER_STATE_ON;

        status = NU_USB_SYS_Register_Device(session,
                                            NU_USBCOMPH_NET);

    }

}

/**************************************************************************
*
* FUNCTION
*     NU_USBH_NET_Disconnection
*
* DESCRIPTION
*     This routine is called by the ethernet driver when a USB device
*     of ethernet type and already reported gets disconnected on the USB.
*
* INPUTS
*     pcb_user_drvr         Pointer to control block of ethernet user driver.
*     session               A unique identifier for the device.
*
* OUTPUTS
*     NU_SUCCESS            Indicates successful completion.
*

**************************************************************************/

VOID NU_USBH_NET_Disconnection (NU_USB_USER  *pcb_user_drvr,
                                VOID         *session)
{

    NU_USBH_COM_ETH_DEVICE* ether_device = (NU_USBH_COM_ETH_DEVICE*)session;

    NU_Remove_From_List((CS_NODE**)&USBH_NET_Drvr.head_device,
                        (CS_NODE*)ether_device->net_dev);

    NU_USB_SYS_DeRegister_Device (session,
                                  NU_USBCOMPH_NET);

    USB_Deallocate_Memory(ether_device->net_dev);

}

/**************************************************************************
* FUNCTION
*     NU_USBH_NET_DM_Open
*
* DESCRIPTION
*     Called by the DM for opening a particular USB ethernet device.
*
* INPUTS
*     session               A unique identifier for the device.
*
*
* OUTPUTS
*     NU_SUCCESS            Indicates successful completion.
*
**************************************************************************/

STATUS NU_USBH_NET_DM_Open (VOID         *session)
{
    STATUS status = NU_SUCCESS;
    NU_USBH_COM_ETH_DEVICE *ether_device;
    NU_USBH_NET_DEV *net_device;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    ether_device = (NU_USBH_COM_ETH_DEVICE*)session;
    net_device = (NU_USBH_NET_DEV*)ether_device->net_dev;

    net_device->eth_dev.dv_name = net_device->name;
    ETHERNET_Create_Name (net_device->eth_dev.dv_name);

    net_device->eth_dev.dv_flags |= (DV_BROADCAST | DV_SIMPLEX | DV_MULTICAST);
    net_device->eth_dev.dv_init = NU_USBH_NET_Init_Intf;

    /* Revert to user mode. */
    NU_USER_MODE();

    return(status);
}

/**************************************************************************
* FUNCTION
*     NU_USBH_NET_DM_Close
*
* DESCRIPTION
*     Called through DM to close a session
*
* INPUTS
*     session               A unique identifier for the device.
*
*
* OUTPUTS
*     NU_SUCCESS            Indicates successful completion.
*
**************************************************************************/

STATUS NU_USBH_NET_DM_Close (VOID* session)
{
    STATUS status = NU_SUCCESS;
    NU_USBH_COM_ETH_DEVICE* eth_device = (NU_USBH_COM_ETH_DEVICE*)session;
    NU_USBH_NET_DEV* net_device = (NU_USBH_NET_DEV*)eth_device->net_dev;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    status = NU_USBH_NET_Detach_Device(net_device->device_entry, session);

    /* Revert to user mode. */
    NU_USER_MODE();

    return(status);
}



/**************************************************************************
* FUNCTION
*     NU_USBH_NET_Attach_Device
*
* DESCRIPTION
*     .
*
* INPUTS
*     net_device     Pointer to NET device main control block.
*     session        Pointer to USB ethernet device to be used as an
*                    identifier in future reference.
*
* OUTPUTS
*     NU_SUCCESS     Indicates successful completion.
*
**************************************************************************/

STATUS NU_USBH_NET_Attach_Device (DV_DEVICE_ENTRY*   net_device,
                                  VOID*              session)
{
    STATUS status ;
    UINT8 i;
    NU_USBH_COM_ECM_INFORM* inform_str;
    NU_USBH_COM_ETH_DEVICE* eth_adpt;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    eth_adpt = (NU_USBH_COM_ETH_DEVICE*) session;

    net_device->user_defined_1 = (UINT32)eth_adpt;

    inform_str = &(eth_adpt->inform_str);

    net_device->dev_mtu        = (inform_str->segment_size)-14;
    /* Set the device MAC Address */
    for(i=0; i<6; i++)
        net_device->dev_mac_addr[i] = (inform_str->MAC_addr)[i];

    status = USB_Allocate_Memory(USB_MEM_TYPE_UNCACHED,
            eth_adpt->inform_str.segment_size,
            &(eth_adpt->user_device.rx_xblock.p_data_buf));

    if (status == NU_SUCCESS)
    {
        eth_adpt->user_device.rx_xblock.data_length =
                             eth_adpt->inform_str.segment_size;

        status = NU_USBH_COM_USER_Create_Polling(
                 (NU_USBH_COM_USER*)eth_adpt->user_device.user_drvr,
                 &eth_adpt->user_device);

        if (status == NU_SUCCESS)
        {

            NU_USBH_COM_USER_Start_Polling(&eth_adpt->user_device);
        }
    }
    /* Revert to user mode. */
    NU_USER_MODE();

    return status;
}

/**************************************************************************
* FUNCTION
*     NU_USBH_NET_Detach_Device
*
* DESCRIPTION
*     This routine is called by the application to notify the disconnection
*     of USB device to NET driver. It also provides a device entry node
*     previously attached with USB ethernet device.
*
* INPUTS
*     net_device     Pointer to NET device main control block.
*     session        Pointer to USB ethernet device.
*
* OUTPUTS
*     NU_SUCCESS     Indicates successful completion.
*
**************************************************************************/

STATUS NU_USBH_NET_Detach_Device(DV_DEVICE_ENTRY*   net_device,
                                 VOID*              session)
{
    STATUS status;
    NU_USBH_COM_ETH_DEVICE* eth_adpt;

    /* Switch to supervisor mode. */
    NU_SUPERV_USER_VARIABLES 
    NU_SUPERVISOR_MODE();

    eth_adpt = (NU_USBH_COM_ETH_DEVICE*) session;

    USB_Deallocate_Memory(eth_adpt->user_device.rx_xblock.p_data_buf);

    eth_adpt->user_device.rx_xblock.data_length = 0x00;

    status = NU_USBH_COM_USER_Stop_Polling(&eth_adpt->user_device);

    status = NU_USBH_COM_USER_Delete_Polling(&eth_adpt->user_device);

    //net_device->user_defined_1 = NU_NULL;

    //eth_adpt->net_dev = NU_NULL;
    /* Revert to user mode. */
    NU_USER_MODE();

    return status;
}


/*************************************************************************
*
*   FUNCTION
*
*       NU_USBH_NET_DM_Read
*
*   DESCRIPTION
*
*       This function reads the frame data from the USB device. Since NET is
*       still using the Event mechansim for buffer reportign so this routine
*       doesn't do anyhting involving the device.
*
*   INPUTS
*
*       VOID         *session_handle        - Session handle pointer
*       VOID         *buffer                - Read buffer
*       INT          numbyte                - Number of bytes to read
*       OFFSET_T     byte_offset            - Byte offset
*
*   OUTPUTS
*
*       STATUS       status                 - NU_SUCCESS
*
*************************************************************************/
STATUS NU_USBH_NET_DM_Read (VOID*     session_handle,
                            VOID*     buffer,
                            UINT32    numbyte,
                            OFFSET_T  byte_offset,
                            UINT32*   bytes_read)
{
    STATUS status = NU_SUCCESS;

    *bytes_read = numbyte;

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_USBH_NET_DM_Write
*
*   DESCRIPTION
*
*       This function sends frame data to attached USB device
*
*   INPUTS
*
*       VOID         *session_handle        - Session handle pointer
*       VOID         *buffer                - Write buffer
*       INT          numbyte                - Number of bytes to write
*       OFFSET_T     byte_offset            - Byte offset
*
*   OUTPUTS
*
*       INT          status                 - NU_SUCCESS, or number of bytes
*                                             written if different than numbyte
*
*************************************************************************/
STATUS NU_USBH_NET_DM_Write (
                          VOID*           session_handle,
                          const VOID*     buffer,
                          UINT32          numbyte,
                          OFFSET_T        byte_offset,
                          UINT32*         bytes_written)
{
    STATUS                status = NU_SUCCESS;
    NET_BUFFER            *buf_ptr = (NET_BUFFER *)buffer;

    NU_USBH_COM_ETH_DEVICE* eth_adpt = (NU_USBH_COM_ETH_DEVICE*)session_handle;

    NU_USBH_NET_DEV* net_device = (NU_USBH_NET_DEV*)eth_adpt->net_dev;

    /* Transmit the packet */
    status = NU_USBH_COM_ETH_Xmit_Packet (net_device->device_entry,
                                           buf_ptr);

    *bytes_written = numbyte;

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_USBH_NET_DM_IOCTL
*
*   DESCRIPTION
*
*       This function controls IO operations of the USB Net driver.
*
*   INPUTS
*
*       VOID          *session_handle       - Session handle of the driver
*       INT           cmd                   - Ioctl command
*       VOID          *data                 - Ioctl data pointer
*       INT           length                - Ioctl length
*
*   OUTPUTS
*
*       STATUS        status                - NU_SUCCESS
*
*************************************************************************/


STATUS NU_USBH_NET_DM_IOCTL(VOID*    in_dev_handle,
                            INT      ioctl_num,
                            VOID*    ioctl_data,
                            INT      ioctl_data_len)
{

    STATUS                status = NU_UNAVAILABLE;
    DV_DEV_HANDLE         *dev_handle;
    NU_DEVICE             *eth_mw;
    DV_DEVICE_ENTRY       *dv_device;
    INT                     *link_status;

    NU_USBH_COM_ETH_DEVICE*  ptr_eth_dev = (NU_USBH_COM_ETH_DEVICE*)in_dev_handle;
    STATUS                  pm_status = PM_INVALID_PARAMETER;
    PM_STATE_ID            *pm_data;
    CHAR                    in_reg_path[REG_MAX_KEY_LENGTH];
    ETHERNET_CONFIG_PATH    *config_path;
    DV_DEV_ID               dev_id;
    NET_IF_LINK_STATE_MSG   if_status_change_msg;


    NU_USBH_NET_DEV* net_device = (NU_USBH_NET_DEV*)ptr_eth_dev->net_dev;

    /* Process command */
    switch (ioctl_num)
    {

        /*******************/
        /* Ethernet Ioctls */
        /*******************/
        case (USB_ETHERNET_IOCTL_BASE + ETHERNET_CMD_SET_DEV_HANDLE):

            /* Get the dev handle from the data passed in */
            dev_handle = (DV_DEV_HANDLE *) ioctl_data;

            /* Save the handle to the device structure */
            net_device->device_handle = *dev_handle;
            net_device->eth_dev.dev_handle = *dev_handle;
            status = NU_SUCCESS;
            break;

        case (USB_ETHERNET_IOCTL_BASE + ETHERNET_CMD_GET_DEV_STRUCT):
            /* Get the NU_DEVICE structure from the data passed in */
            eth_mw = (NU_DEVICE *) ioctl_data;

            /* Return the populated structure */
            *eth_mw = net_device->eth_dev;
            status = NU_SUCCESS;
            break;
        case (USB_ETHERNET_IOCTL_BASE + ETHERNET_CMD_CTRL_INIT):
             dv_device = (DV_DEVICE_ENTRY *)ioctl_data;

             net_device->device_entry = dv_device;
             status = NU_SUCCESS;
             break;
        case (USB_ETHERNET_IOCTL_BASE + ETHERNET_CMD_GET_CONFIG_PATH):
            /* Get the Config Path structure from the data passed in */
            config_path = (ETHERNET_CONFIG_PATH *) ioctl_data;

            /* Return the middleware config path */
            strcpy(in_reg_path, USBH_NET_Drvr.reg_path);
            if(strlen(in_reg_path) <= ((REG_MAX_KEY_LENGTH -1) - strlen("/mw_settings")))
            {
                strcat(in_reg_path, "/mw_settings");
            }
            if(config_path->max_path_len <= REG_MAX_KEY_LENGTH)
            {
                strncpy(config_path->config_path, in_reg_path, config_path->max_path_len);
                status = NU_SUCCESS;
            }
            break;

        case (USB_ETHERNET_IOCTL_BASE + ETHERNET_CMD_SEND_LINK_STATUS):
            if(ptr_eth_dev->sys_var1 == 0x01)
            {
                strcpy(if_status_change_msg.msg, "LINK UP");
            }
            else
            {
                strcpy(if_status_change_msg.msg, "LINK DOWN");
            }

            status = NU_USB_SYS_Get_Dev_ID(ptr_eth_dev, &dev_id);

            if(status == NU_SUCCESS)
            {
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
            link_status = (INT *)ioctl_data;
            *link_status = ptr_eth_dev->sys_var1;
            status = NU_SUCCESS;
            break;

        case (USB_ETHERNET_IOCTL_BASE + ETHERNET_CMD_GET_XDATA):
        case (USB_ETHERNET_IOCTL_BASE + ETHERNET_CMD_TARGET_INIT):
        case (USB_ETHERNET_IOCTL_BASE + ETHERNET_CMD_PHY_INIT):
        case (USB_ETHERNET_IOCTL_BASE + ETHERNET_CMD_GET_ISR_INFO):
        case (USB_ETHERNET_IOCTL_BASE + ETHERNET_CMD_CTRL_ENABLE):
            status = NU_UNAVAILABLE;
            break;


        /*******************/
        /* Net Ioctls      */
        /*******************/
        case (USB_NET_IOCTL_BASE + ETHERNET_CMD_DEV_ADDMULTI):
        case (USB_NET_IOCTL_BASE + ETHERNET_CMD_DEV_DELMULTI):
        case (USB_NET_IOCTL_BASE + ETHERNET_CMD_REMDEV):
            status = NU_UNAVAILABLE;
            break;
        case (USB_POWER_IOCTL_BASE + POWER_IOCTL_GET_STATE):
            if ((ioctl_data_len == sizeof (PM_STATE_ID)) && (ioctl_data != NU_NULL))
            {
                pm_data = (PM_STATE_ID *)ioctl_data;
                *pm_data = net_device->pm_state;
            }
        
            status = NU_SUCCESS; 
            break;

        case (USB_POWER_IOCTL_BASE + POWER_IOCTL_SET_STATE):
            if ((ioctl_data_len == sizeof(PM_STATE_ID)) && (ioctl_data != NU_NULL))
            {
                pm_data = (PM_STATE_ID*)ioctl_data;

                if(status == NU_SUCCESS)
                {
                     if(((*pm_data == POWER_OFF_STATE)||(*pm_data == USB_POWER_STATE_OFF))
                        &&(net_device->pm_state == USB_POWER_STATE_OFF))
                     {
                         status =  NU_USBH_COM_USER_Suspend_Device ((VOID*)&ptr_eth_dev->user_device);
                         if(status == NU_SUCCESS)
                         {
                            net_device->pm_state = USB_POWER_STATE_OFF;
                            pm_status = NU_SUCCESS;
                         }
                         else
                            pm_status = PM_TRANSITION_FAILED;

                     }
                     else if(((*pm_data == POWER_ON_STATE)||(*pm_data == USB_POWER_STATE_ON))
                        &&(net_device->pm_state == USB_POWER_STATE_OFF))
                     {
                         status =  NU_USBH_COM_USER_Resume_Device ((VOID*)&ptr_eth_dev->user_device);
                         if(status == NU_SUCCESS)
                         {
                            net_device->pm_state = USB_POWER_STATE_ON;
                            pm_status = NU_SUCCESS;
                         }
                         else
                            pm_status = PM_TRANSITION_FAILED;

                     }
                     else
                     {
                         pm_status = PM_INVALID_POINTER;
                     }
                 }
                else
                {
                    pm_status = PM_INVALID_POINTER;
                }
            }
            status = (STATUS)pm_status;
            break;

        case (USB_POWER_IOCTL_BASE + POWER_IOCTL_GET_STATE_COUNT):
            if ((ioctl_data_len == sizeof(PM_STATE_ID)) && (ioctl_data != NU_NULL))
            {
                pm_data = (PM_STATE_ID *)ioctl_data;
                *pm_data = USB_TOTAL_POWER_STATES;
            }

            status = DV_IOCTL_INVALID_CMD;
            break;


        default:
            status = NU_UNAVAILABLE;
            break;
    }
    return status;
}

NU_USBH_NET_DEV*  NU_USBH_NET_Find_Device_From_NET(DV_DEVICE_ENTRY* device)
{

    NU_USBH_NET_DEV* net_device = USBH_NET_Drvr.head_device;
    NU_USBH_NET_DEV* ret_device = NU_NULL;
    for(;;)
    {
        if(strcmp(device->dev_net_if_name, net_device->name) == 0x00)
        {
            ret_device = net_device;
            break;
        }
        else if (net_device->link_dev.cs_next == (CS_NODE*)net_device)
        {
            break;
        }
        else
        {
            net_device = (NU_USBH_NET_DEV*)net_device->link_dev.cs_next;
        }
    }

    return(ret_device);
}
