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
*        nu_usb_sys_ext.c
*
* COMPONENT
*
*       Nucleus USB Software
*
* DESCRIPTION
*
*       This file provides the implementation of external interfaces of
*       system server and contains other internal methods.
*
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       NU_USB_SYS_Register_Device           Registers a driver with itself and the DM.
*       NU_USB_SYS_DeRegister_Device         Deregisters a driver with itself and the DM.
*       NU_USB_SYS_Event_Report              Processes a USB related event reported by any driver.
*       NU_USB_SYS_Handle_Function_Dev       Handles the data initialization for a USB function driver case.
*       NU_USB_SYS_Handle_Host_Dev           Handles the data initialization for a USB function driver case.
*       NU_USB_SYS_Open_Device               Common function for opening any session on a USB device
*       NU_USB_SYS_Close_Device              Common function for closing any session on a USB device
*       NU_USB_SYS_IOCTL                     Common function for performing any ioctrl operation on a USB device
*       NU_USB_SYS_Get_Oper_Mode             Checks for the operating mode of the device for a specific session.
*       NU_USB_SYS_Get_Instance              Get Instance of device for a particular session.
*       NU_USB_SYS_Read                      Common function for receiving data from a USB device
*       NU_USB_SYS_Write                     Common function for sending data to a USB device

*
* DEPENDENCIES
*
*       nu_usb.h
*
************************************************************************/

/* ======================  Include Files  ============================= */
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "connectivity/nu_usb.h"


/********************** Global Data    ********************************/
NU_USB_SYS_SERVER USB_System_Server = {0x00};
DV_DEV_LABEL pwr_lbl =  {POWER_CLASS_LABEL};

#ifdef CFG_NU_OS_NET_ENABLE
#if defined (CFG_NU_OS_DRVR_USB_FUNC_NET_IF_ENABLE) || defined (CFG_NU_OS_DRVR_USB_HOST_NET_IF_ENABLE)
DV_DEV_LABEL net_lbl =  {NETWORKING_LABEL};
DV_DEV_LABEL eth_lbl =  {ETHERNET_LABEL};
#endif
#endif

#ifdef CFG_NU_OS_STOR_FILE_VFS_ENABLE
#ifdef CFG_NU_OS_DRVR_USB_HOST_FILE_IF_ENABLE
DV_DEV_LABEL stor_lbl = {STORAGE_LABEL};
#endif
#endif

#ifdef CFG_NU_OS_CONN_USB_FUNC_COMM_ETH_ENABLE
DV_DEV_LABEL usbf_eth_lbl  = {USBF_ETHERNET_LABEL};
DV_DEV_LABEL usbf_ndis_lbl = {USBF_RNDIS_LABEL};
#endif
#ifdef CFG_NU_OS_CONN_USB_FUNC_MS_USER_ENABLE
DV_DEV_LABEL usbf_ms_lbl   = {USBF_STORE_LABEL};
#endif
#ifdef CFG_NU_OS_CONN_USB_FUNC_DFU_ENABLE
DV_DEV_LABEL usbf_dfu_lbl1 = {USBF_DFU_RTM_LABEL};
DV_DEV_LABEL usbf_dfu_lbl2 = {USBF_DFU_STDA_LABEL};
#endif
#ifdef CFG_NU_OS_CONN_USB_FUNC_COMM_MDM_ENABLE
DV_DEV_LABEL usbf_mdm_lbl  = {USBF_MODEM_LABEL};
#endif
#ifdef CFG_NU_OS_CONN_USB_FUNC_HID_KBD_ENABLE
DV_DEV_LABEL usbf_kbd_lbl  = {USBF_KEYBOARD_LABEL};
#endif

#ifdef CFG_NU_OS_CONN_USB_FUNC_HID_MSE_ENABLE
DV_DEV_LABEL usbf_mse_lbl  = {USBF_MOUSE_LABEL};
#endif
#ifdef CFG_NU_OS_CONN_USB_HOST_COMM_ETH_ENABLE
DV_DEV_LABEL usbh_eth_lbl  = {USBH_ETHERNET_LABEL};
#endif
#ifdef CFG_NU_OS_CONN_USB_HOST_MS_USER_ENABLE
DV_DEV_LABEL usbh_ms_lbl   = {USBH_STORE_LABEL};
#endif
#ifdef CFG_NU_OS_CONN_USB_HOST_COMM_MDM_ENABLE
DV_DEV_LABEL usbh_mdm_lbl  = {USBH_MODEM_LABEL};
#endif
#ifdef CFG_NU_OS_CONN_USB_HOST_HID_KEYBOARD_ENABLE
DV_DEV_LABEL usbh_kbd_lbl  = {USBH_KBD_LABEL};
#endif
#ifdef CFG_NU_OS_CONN_USB_HOST_HID_MOUSE_ENABLE
DV_DEV_LABEL usbh_mse_lbl  = {USBH_MSE_LABEL};
#endif
#ifdef CFG_NU_OS_CONN_USB_HOST_AUDIO_ENABLE
DV_DEV_LABEL usbh_audio_lbl  = {USBH_AUDIO_LABEL};
#endif

/*************************************************************************
* FUNCTION
*       NU_USB_SYS_Register_Device
*
* DESCRIPTION
*
*       This routine is called by a USB class/user driver when a new device is connected
*
* INPUTS
*
*       dev_handle          device/driver handle.
*       comp_id             USB defined component ID.
*
* OUTPUTS
*
*      NU_SUCCESS           Successful completion.
*
*************************************************************************/
STATUS NU_USB_SYS_Register_Device (VOID* dev_handle,
                                   UINT8 comp_id )
{
    STATUS status;
    NU_USB_SYS_DEVICE *ptr_curr_dev;
    UINT8              dev_type;
    UINT8              i;
    DV_DRV_FUNCTIONS   sys_func_table = {NU_NULL};

    NU_USB_PTRCHK(dev_handle);
    status = USB_Allocate_Object(sizeof(NU_USB_SYS_DEVICE),
                                 (VOID**) &ptr_curr_dev);

    if (status == NU_SUCCESS )
    {
        memset(ptr_curr_dev, 0x00, sizeof(NU_USB_SYS_DEVICE));
        dev_type = (0x10 & comp_id);

        switch(dev_type)
        {
            case NU_USBCOMP_FUNCTION:
                status = NU_USB_SYS_Handle_Function_Dev(dev_handle,
                                                        comp_id,
                                                        ptr_curr_dev);


                break;

            case NU_USBCOMP_HOST:
                status = NU_USB_SYS_Handle_Host_Dev(dev_handle,
                                                    comp_id,
                                                    ptr_curr_dev);
                ptr_curr_dev->usr_ptr = dev_handle;
                break;

            default:
                status = NU_USB_INVLD_ARG;
                break;
        }
        if(status == NU_SUCCESS)
        {
            ptr_curr_dev->usr_ptr = dev_handle;
            
            sys_func_table.drv_open_ptr  = NU_USB_SYS_Open_Device;
            sys_func_table.drv_close_ptr = NU_USB_SYS_Close_Device;
            sys_func_table.drv_ioctl_ptr = NU_USB_SYS_IOCTL;
            sys_func_table.drv_read_ptr  = NU_USB_SYS_Read;
            sys_func_table.drv_write_ptr = NU_USB_SYS_Write;

            NU_Place_On_List (
                        (CS_NODE **)& USB_System_Server.dev_head,
                        (CS_NODE *) ptr_curr_dev);

            for(i = 0x00; i<USB_SYS_MAX_SESSIONS; i++ )
            {
                ptr_curr_dev->seesions[i].sess_state = USB_SYS_SESS_STATE_AVLB;
            }

            /* populate the link list and do thew device registry*/
            status = DVC_Dev_Register((VOID*)ptr_curr_dev,
                                       ptr_curr_dev->dev_labels,
                                       ptr_curr_dev->dev_label_cnt,
                                       &sys_func_table,
                                       &ptr_curr_dev->drvr_id);
            if (status != NU_SUCCESS)
            {
                NU_Remove_From_List(
                 (CS_NODE **) &(USB_System_Server.dev_head),
                 (CS_NODE *)&ptr_curr_dev);

                USB_Deallocate_Memory(ptr_curr_dev);
            }
        }
        else
        {
            USB_Deallocate_Memory(ptr_curr_dev);
        }
    }
    else
    {
        status = NU_INVALID_MEMORY;
    }

    return (status);
}

/*************************************************************************
* FUNCTION
*       NU_USB_SYS_DeRegister_Device
*
* DESCRIPTION
*
*       This routine is called by a USB class/user driver when a device is
*       disconnected or a fucntion class driver gets deleted.
*
* INPUTS
*
*       dev_handle          device/driver handle.
*       comp_id             USB defined component ID.
*
* OUTPUTS
*
*      NU_SUCCESS           Successful completion.
*
*************************************************************************/
STATUS NU_USB_SYS_DeRegister_Device (VOID* dev_handle,
                                     UINT8 comp_id)
{
    STATUS status = NU_USB_INVLD_ARG;
    DV_DEV_ID  dev_id;
    NU_USB_SYS_DEVICE *ptr_ret_dev = NU_NULL;

    NU_USB_PTRCHK(dev_handle);
    
    status =  NU_USB_SYS_Get_Dev_ID(dev_handle,&dev_id);
    
    if(status == NU_SUCCESS)
    {
        status = DVC_Dev_Unregister (dev_id,
                                     (VOID*)&ptr_ret_dev);
        
        if (( status == NU_SUCCESS )&&(ptr_ret_dev))
        {
            NU_Remove_From_List(
                    (CS_NODE **) &(USB_System_Server.dev_head),
                    (CS_NODE *)ptr_ret_dev);
        
            status = USB_Deallocate_Memory(ptr_ret_dev);
        }
        else
        {
            status = NU_USB_INVLD_ARG;
        }
    }
    return (status);
}


/*************************************************************************
* FUNCTION
*
*       NU_USB_SYS_Handle_Function_Dev
*
* DESCRIPTION
*
*       This routine populates the function table of the driver on the basis
*       of USB host component ID.
*
* INPUTS
*
*       dev_handle          device/driver handle.
*       comp_id             USB defined component ID.
*       ptr_curr_dev        Pointer to the system device control bloack.
*
* OUTPUTS
*
*      NU_SUCCESS           Successful completion.
*
*************************************************************************/
STATUS NU_USB_SYS_Handle_Function_Dev(VOID*              dev_handle,
                                      UINT8              comp_id,
                                      NU_USB_SYS_DEVICE* ptr_curr_dev)
{
#ifdef CFG_NU_OS_CONN_USB_FUNC_ENABLE

    STATUS status;
    UINT8         label_count = 0x00;
    DV_DEV_LABEL  base_labels[] = {{USBFUNCTION_LABEL},
                                   {USB_RS1_LABEL},
                                   {USB_RS2_LABEL},
                                   {USB_RS3_LABEL}};

    DV_DEV_LABEL  drv_labels[] =  {{USBF_STORE_LABEL},     // 0
                                   {USBF_ETHERNET_LABEL},  // 1
                                   {USBF_RNDIS_LABEL},     // 2
                                   {USBF_MODEM_LABEL},     // 3
                                   {USBF_NET_LABEL},       // 4
                                   {USBF_KEYBOARD_LABEL},  // 5
                                   {USBF_MOUSE_LABEL},     // 6
                                   {USBF_DFU_RTM_LABEL},   // 7
                                   {USBF_DFU_STDA_LABEL}}; // 8

    switch(comp_id)
    {
#ifdef CFG_NU_OS_CONN_USB_FUNC_MS_USER_ENABLE
        case NU_USBCOMPF_STORAGE:

            ptr_curr_dev->dev_type = NU_USBCOMPF_STORAGE;

            ptr_curr_dev->drvr_func_table.drv_open_ptr  = (DV_DRV_OPEN_FUNCTION)&NU_USBF_MS_DM_Open;
            ptr_curr_dev->drvr_func_table.drv_close_ptr = &NU_USBF_MS_DM_Close;
            ptr_curr_dev->drvr_func_table.drv_ioctl_ptr = &NU_USBF_MS_DM_IOCTL;
            ptr_curr_dev->drvr_func_table.drv_read_ptr  = &NU_USBF_MS_DM_Read;
            ptr_curr_dev->drvr_func_table.drv_write_ptr = &NU_USBF_MS_DM_Write;

            DVS_Label_Replace (base_labels,
                               0x2,
                               base_labels[1],
                               drv_labels[0]);

            label_count = 0x2;
            break;
#endif /* CFG_NU_OS_CONN_USB_FUNC_MS_USER_ENABLE */

#ifdef CFG_NU_OS_CONN_USB_FUNC_COMM_ETH_ENABLE
        case NU_USBCOMPF_RNDIS:
            ptr_curr_dev->dev_type = NU_USBCOMPF_RNDIS;

            ptr_curr_dev->drvr_func_table.drv_open_ptr  = (DV_DRV_OPEN_FUNCTION)&NU_USBF_RNDIS_DM_Open;
            ptr_curr_dev->drvr_func_table.drv_close_ptr = &NU_USBF_RNDIS_DM_Close;
            ptr_curr_dev->drvr_func_table.drv_ioctl_ptr = &NU_USBF_RNDIS_DM_IOCTL;
            ptr_curr_dev->drvr_func_table.drv_read_ptr  = &NU_USBF_RNDIS_DM_Read;
            ptr_curr_dev->drvr_func_table.drv_write_ptr = &NU_USBF_RNDIS_DM_Write;

            DVS_Label_Replace (base_labels,
                               0x2,
                               base_labels[1],
                               drv_labels[2]);

            label_count = 0x2;
            break;

        case NU_USBCOMPF_CDC:
            ptr_curr_dev->dev_type = NU_USBCOMPF_CDC;

            ptr_curr_dev->drvr_func_table.drv_open_ptr  = (DV_DRV_OPEN_FUNCTION)&NU_USBF_ETH_DM_Open;
            ptr_curr_dev->drvr_func_table.drv_close_ptr = &NU_USBF_ETH_DM_Close;
            ptr_curr_dev->drvr_func_table.drv_ioctl_ptr = &NU_USBF_ETH_DM_IOCTL;
            ptr_curr_dev->drvr_func_table.drv_read_ptr  = &NU_USBF_ETH_DM_Read;
            ptr_curr_dev->drvr_func_table.drv_write_ptr = &NU_USBF_ETH_DM_Write;
            
            DVS_Label_Replace (base_labels,
                               0x2,
                               base_labels[1],
                               drv_labels[1]);

            label_count = 0x2;
            break;
#endif /* CFG_NU_OS_CONN_USB_FUNC_COMM_ETH_ENABLE */

#ifdef CFG_NU_OS_CONN_USB_FUNC_COMM_MDM_ENABLE
        case NU_USBCOMPF_MODEM:
            ptr_curr_dev->dev_type = NU_USBCOMPF_MODEM;
            ptr_curr_dev->drvr_func_table.drv_open_ptr  = (DV_DRV_OPEN_FUNCTION)&NU_USBF_ACM_DM_Open;
            ptr_curr_dev->drvr_func_table.drv_close_ptr = &NU_USBF_ACM_DM_Close;
            ptr_curr_dev->drvr_func_table.drv_ioctl_ptr = &NU_USBF_ACM_DM_IOCTL;
            ptr_curr_dev->drvr_func_table.drv_read_ptr  = &NU_USBF_ACM_DM_Read;
            ptr_curr_dev->drvr_func_table.drv_write_ptr = &NU_USBF_ACM_DM_Write;

            DVS_Label_Replace (base_labels,
                               0x2,
                               base_labels[1],
                               drv_labels[3]);

            label_count = 0x2;
            break;
#endif /* CFG_NU_OS_CONN_USB_FUNC_COMM_MDM_ENABLE */

#ifdef CFG_NU_OS_NET_ENABLE
#ifdef CFG_NU_OS_DRVR_USB_FUNC_NET_IF_ENABLE
        case NU_USBCOMPF_NET:
            ptr_curr_dev->dev_type = NU_USBCOMPF_NET;

            ptr_curr_dev->drvr_func_table.drv_open_ptr  = (DV_DRV_OPEN_FUNCTION)&NU_USBF_NET_DM_Open;
            ptr_curr_dev->drvr_func_table.drv_close_ptr = &NU_USBF_NET_DM_Close;
            ptr_curr_dev->drvr_func_table.drv_ioctl_ptr = &NU_USBF_NET_DM_IOCTL;
            ptr_curr_dev->drvr_func_table.drv_read_ptr  = &NU_USBF_NET_DM_Read;
            ptr_curr_dev->drvr_func_table.drv_write_ptr = &NU_USBF_NET_DM_Write;

            DVS_Label_Replace (base_labels,
                               0x2,
                               base_labels[1],
                               drv_labels[4]);

            DVS_Label_Replace (base_labels,
                               0x3,
                               base_labels[2],
                               net_lbl);

            DVS_Label_Replace (base_labels,
                               0x4,
                               base_labels[3],
                               eth_lbl);
            label_count = 0x4;
            break;
#endif /* CFG_NU_OS_DRVR_USB_FUNC_NET_IF_ENABLE */
#endif /* CFG_NU_OS_NET_ENABLE */

#ifdef CFG_NU_OS_CONN_USB_FUNC_HID_KBD_ENABLE
        case NU_USBCOMPF_KBD:
            ptr_curr_dev->dev_type = NU_USBCOMPF_KBD;
            ptr_curr_dev->drvr_func_table.drv_open_ptr  = (DV_DRV_OPEN_FUNCTION)&NU_USBF_KB_DM_Open;
            ptr_curr_dev->drvr_func_table.drv_close_ptr = &NU_USBF_KB_DM_Close;
            ptr_curr_dev->drvr_func_table.drv_ioctl_ptr = &NU_USBF_KB_DM_IOCTL;
            ptr_curr_dev->drvr_func_table.drv_read_ptr  = &NU_USBF_KB_DM_Read;
            ptr_curr_dev->drvr_func_table.drv_write_ptr = &NU_USBF_KB_DM_Write;

            DVS_Label_Replace (base_labels,
                               0x2,
                               base_labels[1],
                               drv_labels[5]);

            label_count = 0x2;
            break;
#endif /* CFG_NU_OS_CONN_USB_FUNC_HID_KBD_ENABLE */

#ifdef CFG_NU_OS_CONN_USB_FUNC_HID_MSE_ENABLE
        case NU_USBCOMPF_MSE:
            ptr_curr_dev->dev_type = NU_USBCOMPF_MSE;
            ptr_curr_dev->drvr_func_table.drv_open_ptr  = (DV_DRV_OPEN_FUNCTION)&NU_USBF_MSE_DM_Open;
            ptr_curr_dev->drvr_func_table.drv_close_ptr = &NU_USBF_MSE_DM_Close;
            ptr_curr_dev->drvr_func_table.drv_ioctl_ptr = &NU_USBF_MSE_DM_IOCTL;
            ptr_curr_dev->drvr_func_table.drv_read_ptr  = &NU_USBF_MSE_DM_Read;
            ptr_curr_dev->drvr_func_table.drv_write_ptr = &NU_USBF_MSE_DM_Write;

            DVS_Label_Replace (base_labels,
                               0x2,
                               base_labels[1],
                               drv_labels[6]);

            label_count = 0x2;
            break;
#endif /* CFG_NU_OS_CONN_USB_FUNC_HID_MSE_ENABLE */

#ifdef CFG_NU_OS_CONN_USB_FUNC_DFU_ENABLE
        case NU_USBCOMPF_DFU:
            ptr_curr_dev->dev_type = NU_USBCOMPF_DFU;
            ptr_curr_dev->drvr_func_table.drv_open_ptr  = (DV_DRV_OPEN_FUNCTION)&NU_USBF_DFU_DM_Open;
            ptr_curr_dev->drvr_func_table.drv_close_ptr = &NU_USBF_DFU_DM_Close;
            ptr_curr_dev->drvr_func_table.drv_ioctl_ptr = &NU_USBF_DFU_DM_IOCTL;
            ptr_curr_dev->drvr_func_table.drv_read_ptr  = &NU_USBF_DFU_DM_Read;
            ptr_curr_dev->drvr_func_table.drv_write_ptr = &NU_USBF_DFU_DM_Write;

            DVS_Label_Replace (base_labels,
                               0x2,
                               base_labels[1],
                               drv_labels[7]
                               );

            DVS_Label_Replace (base_labels,
                               0x3,
                               base_labels[2],
                               drv_labels[8]
                               );
                        
            label_count = 0x3;
            break;
#endif /* CFG_NU_OS_CONN_USB_FUNC_DFU_ENABLE */			

        default:
            status = NU_USB_INVLD_ARG;
            break;
    }

    status = DVS_Label_Append(ptr_curr_dev->dev_labels,
                              label_count,
                              ptr_curr_dev->dev_labels,
                              ptr_curr_dev->dev_label_cnt,
                              base_labels,
                              label_count);

    ptr_curr_dev->dev_label_cnt = label_count;

    return (status);
    
#else

    return ( NU_USB_INVLD_ARG );
    
#endif
}

/*************************************************************************
* FUNCTION
*       NU_USB_SYS_Handle_Host_Dev
*
* DESCRIPTION
*
*       This routine populates the function table of the driver on the basis
*       of USB host component ID.
*
* INPUTS
*
*       dev_handle          device/driver handle.
*       comp_id             USB defined component ID.
*       ptr_curr_dev        Pointer to the system device control bloack.
*
* OUTPUTS
*
*      NU_SUCCESS           Successful completion.
*
*************************************************************************/
STATUS NU_USB_SYS_Handle_Host_Dev(VOID*              dev_handle,
                                  UINT8              comp_id,
                                  NU_USB_SYS_DEVICE* ptr_curr_dev)
{
#ifdef CFG_NU_OS_CONN_USB_HOST_ENABLE

    STATUS status;
    UINT8         label_count = 0x00;
    DV_DEV_LABEL  base_labels[] = {{USBDEVICE_LABEL},
                                   {USB_RS1_LABEL},
                                   {USB_RS2_LABEL},
                                   {USB_RS3_LABEL}};

    DV_DEV_LABEL  drv_labels[] =  {{USBH_STORE_LABEL},
                                   {USBH_ETHERNET_LABEL},
                                   {USBH_MODEM_LABEL},
                                   {USBH_FILE_LABEL},
                                   {USBH_NET_LABEL},
                                   {USBH_KBD_LABEL},
                                   {USBH_MSE_LABEL},
                                {USBH_AUDIO_LABEL}};

    switch(comp_id)
    {
#ifdef CFG_NU_OS_CONN_USB_HOST_MS_USER_ENABLE
        case NU_USBCOMPH_STORAGE:
            ptr_curr_dev->dev_type = NU_USBCOMPH_STORAGE;
            ptr_curr_dev->drvr_func_table.drv_open_ptr  = (DV_DRV_OPEN_FUNCTION)&NU_USBH_MS_DM_Open;
            ptr_curr_dev->drvr_func_table.drv_close_ptr = &NU_USBH_MS_DM_Close;
            ptr_curr_dev->drvr_func_table.drv_ioctl_ptr = &NU_USBH_MS_DM_IOCTL;
            ptr_curr_dev->drvr_func_table.drv_read_ptr  = &NU_USBH_MS_DM_Read;
            ptr_curr_dev->drvr_func_table.drv_write_ptr = &NU_USBH_MS_DM_Write;

            DVS_Label_Replace (base_labels,
                               0x2,
                               base_labels[1],
                               drv_labels[0]);

            label_count = 0x2;
            break;
#endif /* CFG_NU_OS_CONN_USB_HOST_MS_USER_ENABLE */

#ifdef CFG_NU_OS_CONN_USB_HOST_COMM_ETH_ENABLE
        case NU_USBCOMPH_CDC:
            ptr_curr_dev->dev_type = NU_USBCOMPH_CDC;

            ptr_curr_dev->drvr_func_table.drv_open_ptr  = (DV_DRV_OPEN_FUNCTION)&NU_USBH_ETH_DM_Open;
            ptr_curr_dev->drvr_func_table.drv_close_ptr = &NU_USBH_ETH_DM_Close;
            ptr_curr_dev->drvr_func_table.drv_ioctl_ptr = &NU_USBH_ETH_DM_IOCTL;
            ptr_curr_dev->drvr_func_table.drv_read_ptr  = &NU_USBH_ETH_DM_Read;
            ptr_curr_dev->drvr_func_table.drv_write_ptr = &NU_USBH_ETH_DM_Write;

            DVS_Label_Replace (base_labels,
                               0x2,
                               base_labels[1],
                               drv_labels[1]);

            label_count = 0x2;
            break;
#endif /* CFG_NU_OS_CONN_USB_HOST_COMM_ETH_ENABLE */

#ifdef CFG_NU_OS_STOR_FILE_VFS_ENABLE
#ifdef CFG_NU_OS_DRVR_USB_HOST_FILE_IF_ENABLE
        case NU_USBCOMPH_FILE:
            ptr_curr_dev->dev_type = NU_USBCOMPH_FILE;
            ptr_curr_dev->drvr_func_table.drv_open_ptr  = (DV_DRV_OPEN_FUNCTION)&NU_USBH_FILE_DM_Open;
            ptr_curr_dev->drvr_func_table.drv_close_ptr = &NU_USBH_FILE_DM_Close;
            ptr_curr_dev->drvr_func_table.drv_ioctl_ptr = &NU_USBH_FILE_DM_IOCTL;
            ptr_curr_dev->drvr_func_table.drv_read_ptr  = &NU_USBH_FILE_DM_Read;
            ptr_curr_dev->drvr_func_table.drv_write_ptr = &NU_USBH_FILE_DM_Write;

            DVS_Label_Replace (base_labels,
                               0x2,
                               base_labels[1],
                               drv_labels[3]);

            DVS_Label_Replace (base_labels,
                               0x3,
                               base_labels[2],
                               stor_lbl);

            label_count = 0x3;
            break;
#endif /* CFG_NU_OS_DRVR_USB_HOST_FILE_IF_ENABLE */
#endif /* CFG_NU_OS_STOR_FILE_VFS_ENABLE */

#ifdef CFG_NU_OS_CONN_USB_HOST_COMM_MDM_ENABLE
        case NU_USBCOMPH_MODEM:
            ptr_curr_dev->dev_type = NU_USBCOMPH_MODEM;

            ptr_curr_dev->drvr_func_table.drv_open_ptr  = (DV_DRV_OPEN_FUNCTION)&NU_USBH_MDM_DM_Open;
            ptr_curr_dev->drvr_func_table.drv_close_ptr = &NU_USBH_MDM_DM_Close;
            ptr_curr_dev->drvr_func_table.drv_ioctl_ptr = &NU_USBH_MDM_DM_IOCTL;
            ptr_curr_dev->drvr_func_table.drv_read_ptr  = &NU_USBH_MDM_DM_Read;
            ptr_curr_dev->drvr_func_table.drv_write_ptr = &NU_USBH_MDM_DM_Write;

            DVS_Label_Replace (base_labels,
                               0x2,
                               base_labels[1],
                               drv_labels[2]);
            label_count = 0x2;
            break;
#endif /* CFG_NU_OS_CONN_USB_HOST_COMM_MDM_ENABLE */

#ifdef CFG_NU_OS_NET_ENABLE
#ifdef CFG_NU_OS_DRVR_USB_HOST_NET_IF_ENABLE
        case NU_USBCOMPH_NET:
            ptr_curr_dev->dev_type = NU_USBCOMPH_NET;

            ptr_curr_dev->drvr_func_table.drv_open_ptr  = (DV_DRV_OPEN_FUNCTION)&NU_USBH_NET_DM_Open;
            ptr_curr_dev->drvr_func_table.drv_close_ptr = &NU_USBH_NET_DM_Close;
            ptr_curr_dev->drvr_func_table.drv_ioctl_ptr = &NU_USBH_NET_DM_IOCTL;
            ptr_curr_dev->drvr_func_table.drv_read_ptr  = &NU_USBH_NET_DM_Read;
            ptr_curr_dev->drvr_func_table.drv_write_ptr = &NU_USBH_NET_DM_Write;

            DVS_Label_Replace (base_labels,
                               0x2,
                               base_labels[1],
                               drv_labels[4]);

            DVS_Label_Replace (base_labels,
                               0x3,
                               base_labels[2],
                               net_lbl);

            DVS_Label_Replace (base_labels,
                               0x4,
                               base_labels[3],
                               eth_lbl);
            label_count = 0x4;
            break;
#endif /* CFG_NU_OS_DRVR_USB_HOST_NET_IF_ENABLE */
#endif /* CFG_NU_OS_NET_ENABLE */

#ifdef CFG_NU_OS_CONN_USB_HOST_HID_KEYBOARD_ENABLE
        case NU_USBCOMPH_KBD:
            ptr_curr_dev->dev_type = NU_USBCOMPH_KBD;
            ptr_curr_dev->drvr_func_table.drv_open_ptr  = (DV_DRV_OPEN_FUNCTION)&NU_USBH_KBD_DM_Open;
            ptr_curr_dev->drvr_func_table.drv_close_ptr = &NU_USBH_KBD_DM_Close;
            ptr_curr_dev->drvr_func_table.drv_ioctl_ptr = &NU_USBH_KBD_DM_IOCTL;
            ptr_curr_dev->drvr_func_table.drv_read_ptr  = &NU_USBH_KBD_DM_Read;
            ptr_curr_dev->drvr_func_table.drv_write_ptr = &NU_USBH_KBD_DM_Write;

            DVS_Label_Replace (base_labels,
                               0x2,
                               base_labels[1],
                               drv_labels[5]);

            label_count = 0x2;
            break;
#endif /* CFG_NU_OS_CONN_USB_HOST_HID_KEYBOARD_ENABLE */

#ifdef CFG_NU_OS_CONN_USB_HOST_HID_MOUSE_ENABLE
        case NU_USBCOMPH_MSE:
            ptr_curr_dev->dev_type = NU_USBCOMPH_MSE;
            ptr_curr_dev->drvr_func_table.drv_open_ptr  = (DV_DRV_OPEN_FUNCTION)&NU_USBH_MSE_DM_Open;
            ptr_curr_dev->drvr_func_table.drv_close_ptr = &NU_USBH_MSE_DM_Close;
            ptr_curr_dev->drvr_func_table.drv_ioctl_ptr = &NU_USBH_MSE_DM_IOCTL;
            ptr_curr_dev->drvr_func_table.drv_read_ptr  = &NU_USBH_MSE_DM_Read;
            ptr_curr_dev->drvr_func_table.drv_write_ptr = &NU_USBH_MSE_DM_Write;

            DVS_Label_Replace (base_labels,
                               0x2,
                               base_labels[1],
                               drv_labels[6]);

            label_count = 0x2;
            break;
#endif /* CFG_NU_OS_CONN_USB_HOST_HID_KEYBOARD_ENABLE */

#ifdef CFG_NU_OS_CONN_USB_HOST_AUDIO_ENABLE
        case NU_USBCOMPH_AUDIO:
            ptr_curr_dev->dev_type = NU_USBCOMPH_AUDIO;
            ptr_curr_dev->drvr_func_table.drv_open_ptr	= (DV_DRV_OPEN_FUNCTION)&NU_USBH_AUDIO_DM_Open;
            ptr_curr_dev->drvr_func_table.drv_close_ptr = &NU_USBH_AUDIO_DM_Close;
            ptr_curr_dev->drvr_func_table.drv_ioctl_ptr = &NU_USBH_AUDIO_DM_IOCTL;
            ptr_curr_dev->drvr_func_table.drv_read_ptr	= &NU_USBH_AUDIO_DM_Read;
            ptr_curr_dev->drvr_func_table.drv_write_ptr = &NU_USBH_AUDIO_DM_Write;
            
            DVS_Label_Replace (base_labels,
                               0x2,
                               base_labels[1],
                               drv_labels[7]);
            label_count = 0x2;
            break;
#endif /* CFG_NU_OS_CONN_USB_HOST_AUDIO_ENABLE */

        default:
            status = NU_USB_INVLD_ARG;
            break;
    }

    status = DVS_Label_Append(ptr_curr_dev->dev_labels,
                              label_count,
                              ptr_curr_dev->dev_labels,
                              ptr_curr_dev->dev_label_cnt,
                              base_labels,
                              label_count);

    ptr_curr_dev->dev_label_cnt =  label_count;

    return (status);

#else

    return ( NU_USB_INVLD_ARG );
    
#endif
}

/*************************************************************************
* FUNCTION
*       NU_USB_SYS_Open_Device
*
* DESCRIPTION
*
*       This routine is called by the DM when it gets an open call from
*       the middleware.
*
* INPUTS
*
*       instance_handle
*       label_list
*       label_cnt
*       session_handle_ptr
*
* OUTPUTS
*
*      NU_SUCCESS           Successful completion.
*
*************************************************************************/
STATUS NU_USB_SYS_Open_Device(VOID*        instance_handle,
                              DV_DEV_LABEL label_list[],
                              INT          label_cnt,
                              VOID**       session_handle_ptr)
{
    STATUS status = NU_SUCCESS;
    UINT8   i;
    VOID* dummy_ptr;
    NU_USB_SYS_DEVICE* ptr_curr_dev = (NU_USB_SYS_DEVICE*)instance_handle;
    NU_USB_SYS_SESSION* session_ptr = NU_NULL;
    UINT32 operating_mode;

    NU_USB_PTRCHK(instance_handle);
    NU_USB_PTRCHK(session_handle_ptr);

    for(i = 0x00; i<label_cnt; i++ )
    {
        status |= DVS_Label_List_Contains(label_list,
                                 label_cnt,
                                 label_list[i]);
    }

    if(NU_SUCCESS == status)
    {
        operating_mode = NU_USB_SYS_Get_Oper_Mode(label_list,label_cnt);

        for(i = 0x00; i<USB_SYS_MAX_SESSIONS; i++ )
        {
            if(ptr_curr_dev->seesions[i].sess_state == USB_SYS_SESS_STATE_AVLB)
            {
                session_ptr = &ptr_curr_dev->seesions[i];
                break;
            }
        }

        if(session_ptr)
        {
            session_ptr->modes = operating_mode;
            session_ptr->sess_state = USB_SYS_SESS_STATE_OPEN;
            session_ptr->dev_inst = (VOID*)ptr_curr_dev;
            session_ptr->usr_ptr = ptr_curr_dev->usr_ptr;

            if(!(operating_mode & USB_SYS_SESS_MODE_POWER))
            {
                status = ptr_curr_dev->drvr_func_table.drv_open_ptr(session_ptr->usr_ptr,
                                                                    label_list,
                                                                    label_cnt,
                                                                   &dummy_ptr);
            }
            *session_handle_ptr = session_ptr;
         }
         else
         {
             status = DV_SESSION_NOT_OPEN;
         }
    }
    else
    {
        status = DV_LABEL_NOT_FOUND;
    }

    return (status);

}


/*************************************************************************
* FUNCTION
*       NU_USB_SYS_Close_Device
*
* DESCRIPTION
*
*       This routine is called by the DM when it gets a close call from
*       the middleware.
*
* INPUTS
*
*       session_handle_ptr
*
* OUTPUTS
*
*      NU_SUCCESS           Successful completion.
*
*************************************************************************/
STATUS NU_USB_SYS_Close_Device(VOID*  session_handle_ptr)
{
    STATUS status;
    NU_USB_SYS_DEVICE* ptr_curr_dev = NU_NULL;
    NU_USB_SYS_SESSION* sess_ptr = (NU_USB_SYS_SESSION*)session_handle_ptr;
    UINT8 i;

    NU_USB_PTRCHK(session_handle_ptr);

    ptr_curr_dev = (NU_USB_SYS_DEVICE*)sess_ptr->dev_inst;

    if(ptr_curr_dev)
    {
        status = ptr_curr_dev->drvr_func_table.drv_close_ptr(sess_ptr->usr_ptr);
        for(i = 0x00; i < USB_SYS_MAX_SESSIONS; i++ )
        {
            if(&(ptr_curr_dev->seesions[i]) == session_handle_ptr)
            {
                ptr_curr_dev->seesions[i].sess_state = USB_SYS_SESS_STATE_AVLB;
                status = NU_SUCCESS;
                break;
            }
        }
    }

    else
    {
        status = DV_INVALID_INPUT_PARAMS;
    }

    return (status);
}
/*************************************************************************
* FUNCTION
*       NU_USB_SYS_IOCTL
*
* DESCRIPTION
*
*       This routine is called by the DM when it gets an IOCTL call from
*       the middleware for any USB device.
*
* INPUTS
*
*       session_handle     A unique pointer to identify the session passed as context
*       cmd                IOCTL number
*       data               IOCTL data pointer of variable type
*       length             IOCTL data length in bytes
*
* OUTPUTS
*
*      NU_SUCCESS           Successful completion.
*
*************************************************************************/
STATUS NU_USB_SYS_IOCTL(VOID            *session_handle,
                        INT             cmd,
                        VOID            *data,
                        INT             length)
{
     STATUS status = NU_SUCCESS;
     DV_IOCTL0_STRUCT      *ioctl0 = (DV_IOCTL0_STRUCT*)data;
     NU_USB_SYS_SESSION*    session = (NU_USB_SYS_SESSION*)session_handle;
     NU_USB_SYS_DEVICE* sys_device = (NU_USB_SYS_DEVICE*)session->dev_inst;
     DV_DEV_LABEL* label = &ioctl0->label;
     UINT32 oper_mode;

     oper_mode = NU_USB_SYS_Get_Oper_Mode(label,0x1);

     if(cmd == DV_IOCTL0)
     {
        if((session->modes & oper_mode)&&(session->sess_state == USB_SYS_SESS_STATE_OPEN))
        {
            if (DV_COMPARE_LABELS(&pwr_lbl, label))
            {
                 ioctl0->base = USB_POWER_IOCTL_BASE;
            }
#ifdef CFG_NU_OS_NET_ENABLE
#if defined (CFG_NU_OS_DRVR_USB_FUNC_NET_IF_ENABLE) || defined (CFG_NU_OS_DRVR_USB_HOST_NET_IF_ENABLE)
            else if (DV_COMPARE_LABELS(&eth_lbl, label))
            {
                 ioctl0->base = USB_ETHERNET_IOCTL_BASE;
            }
            else if (DV_COMPARE_LABELS(&net_lbl, label))
            {
                 ioctl0->base = USB_NET_IOCTL_BASE;
            }
#endif
#endif
#ifdef CFG_NU_OS_STOR_FILE_VFS_ENABLE
#ifdef CFG_NU_OS_DRVR_USB_HOST_FILE_IF_ENABLE
            else if (DV_COMPARE_LABELS(&stor_lbl, label))
            {
             ioctl0->base = USB_FILE_IOCTL_BASE;
            }
#endif
#endif
#ifdef CFG_NU_OS_CONN_USB_FUNC_COMM_ETH_ENABLE
            else if ((DV_COMPARE_LABELS(&usbf_eth_lbl,  label))||
                  (DV_COMPARE_LABELS(&usbf_ndis_lbl, label)))
            {
             ioctl0->base = USB_CDC_IOCTL_BASE;
            }
#endif
#ifdef CFG_NU_OS_CONN_USB_HOST_COMM_ETH_ENABLE
            else if ((DV_COMPARE_LABELS(&usbh_eth_lbl,  label)))
            {
             ioctl0->base = USB_CDC_IOCTL_BASE;
            }
#endif
#ifdef CFG_NU_OS_CONN_USB_FUNC_MS_USER_ENABLE
            else if ((DV_COMPARE_LABELS(&usbf_ms_lbl, label)))
            {
             ioctl0->base = USB_STORE_IOCTL_BASE;
            }
#endif
#ifdef CFG_NU_OS_CONN_USB_HOST_MS_USER_ENABLE
            else if ((DV_COMPARE_LABELS(&usbh_ms_lbl, label)))
            {
             ioctl0->base = USB_STORE_IOCTL_BASE;
            }
#endif
#ifdef CFG_NU_OS_CONN_USB_FUNC_DFU_ENABLE
            else if ((DV_COMPARE_LABELS(&usbf_dfu_lbl1, label)) ||
                    (DV_COMPARE_LABELS(&usbf_dfu_lbl2, label)))
            {
             ioctl0->base = USB_DFU_IOCTL_BASE;
            }
#endif
#ifdef CFG_NU_OS_CONN_USB_FUNC_COMM_MDM_ENABLE
            else if ((DV_COMPARE_LABELS(&usbf_mdm_lbl, label)))
            {
             ioctl0->base = USB_MODEM_IOCTL_BASE;
            }
#endif
#ifdef CFG_NU_OS_CONN_USB_HOST_COMM_MDM_ENABLE
            else if ((DV_COMPARE_LABELS(&usbh_mdm_lbl, label)))
            {
             ioctl0->base = USB_MODEM_IOCTL_BASE;
            }
#endif
#ifdef CFG_NU_OS_CONN_USB_HOST_HID_KEYBOARD_ENABLE
            else if ((DV_COMPARE_LABELS(&usbh_kbd_lbl, label)))
            {
             ioctl0->base = USB_KBD_IOCTL_BASE;
            }
#endif
#ifdef CFG_NU_OS_CONN_USB_HOST_HID_MOUSE_ENABLE
            else if ((DV_COMPARE_LABELS(&usbh_mse_lbl, label)))
            {
             ioctl0->base = USB_MSE_IOCTL_BASE;
            }
#endif
#ifdef CFG_NU_OS_CONN_USB_HOST_AUDIO_ENABLE
            else if ((DV_COMPARE_LABELS(&usbh_audio_lbl, label)))
            {
             ioctl0->base = USB_AUDIO_IOCTL_BASE;
            }
#endif

#ifdef CFG_NU_OS_CONN_USB_FUNC_HID_KBD_ENABLE
            else if ((DV_COMPARE_LABELS(&usbf_kbd_lbl, label)))
            {
             ioctl0->base = USB_KBD_IOCTL_BASE;
            }
#endif
#ifdef CFG_NU_OS_CONN_USB_FUNC_HID_MSE_ENABLE
            else if ((DV_COMPARE_LABELS(&usbf_mse_lbl, label)))
            {
             ioctl0->base = USB_MSE_IOCTL_BASE;
            }
#endif

        }
    }
    else
    {
        sys_device->drvr_func_table.drv_ioctl_ptr(session->usr_ptr,cmd,data,length);
    }

     return(status);
}
/*************************************************************************
*
* FUNCTION
*
*       NU_USB_SYS_Read
*
* DESCRIPTION
*
*       This function is called by the application when it wants to read
*       data from the usb device. This routine simply calls the actual USB
*       driver's read routine by passing the exact context. 
*
* INPUTS
*
*       session_handle     Pointer to the USB driver's passed context.
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
STATUS NU_USB_SYS_Read (VOID *session_handle,
                            VOID *buffer,
                            UINT32 numbyte,
                            OFFSET_T byte_offset,
                            UINT32 *bytes_read_ptr)
{
     STATUS status;
     NU_USB_SYS_SESSION*    session = (NU_USB_SYS_SESSION*)session_handle;
     NU_USB_SYS_DEVICE* sys_device = (NU_USB_SYS_DEVICE*)session->dev_inst;

     status = sys_device->drvr_func_table.drv_read_ptr(session->usr_ptr,
                                                        buffer,
                                                        numbyte,
                                                        byte_offset,
                                                        bytes_read_ptr);

     return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       NU_USB_SYS_Write
*
* DESCRIPTION
*
*       This function is called by the application when it wants to write
*       data to the usb device. This routine simply calls the actual USB
*       driver's write routine by passing the exact context. 
*
* INPUTS
*
*       session_handle     Pointer to the USB driver's passed context.
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
STATUS  NU_USB_SYS_Write (VOID *session_handle,
                                        const VOID *buffer,
                                        UINT32 numbyte,
                                        OFFSET_T byte_offset,
                                        UINT32 *bytes_written_ptr)
{
     STATUS status;
     NU_USB_SYS_SESSION*    session = (NU_USB_SYS_SESSION*)session_handle;
     NU_USB_SYS_DEVICE* sys_device = (NU_USB_SYS_DEVICE*)session->dev_inst;

     status = sys_device->drvr_func_table.drv_write_ptr(session->usr_ptr,
                                                        buffer,
                                                        numbyte,
                                                        byte_offset,
                                                        bytes_written_ptr); 

    return (status);
}
/*************************************************************************
* FUNCTION
*       NU_USB_SYS_Get_Oper_Mode
*
* DESCRIPTION
*
*       This routine is internally called by the system to calculate the 
*       operating mode flags based on the passed labels..
*
* INPUTS
*
*       labels_list     List of 128bit device labels
*       labels_cnt      Label count
*
* OUTPUTS
*
*      NU_SUCCESS           Successful completion.
*
*************************************************************************/
UINT32 NU_USB_SYS_Get_Oper_Mode(DV_DEV_LABEL labels_list[],
                                INT           labels_cnt)
{
    UINT32 operating_mode = 0x00;
    UINT8 i;

    for (i = 0; i < labels_cnt; i++)
    {
       if (DV_COMPARE_LABELS(&pwr_lbl, &(labels_list[i])))
       {
           operating_mode |= USB_SYS_SESS_MODE_POWER;
       }
#ifdef CFG_NU_OS_NET_ENABLE
#if defined (CFG_NU_OS_DRVR_USB_FUNC_NET_IF_ENABLE) || defined (CFG_NU_OS_DRVR_USB_HOST_NET_IF_ENABLE)
       if (DV_COMPARE_LABELS(&eth_lbl, &(labels_list[i])))
       {
           operating_mode |= USB_SYS_SESS_MODE_ETHERNET;
       }
       if (DV_COMPARE_LABELS(&net_lbl, &(labels_list[i])))
       {
           operating_mode |= USB_SYS_SESS_MODE_NETWORKING;
       }
#endif
#endif
#ifdef CFG_NU_OS_STOR_FILE_VFS_ENABLE
#ifdef CFG_NU_OS_DRVR_USB_HOST_FILE_IF_ENABLE
       if (DV_COMPARE_LABELS(&stor_lbl, &(labels_list[i])))
       {
           operating_mode |= USB_SYS_SESS_MODE_FILE;
       }
#endif
#endif
#ifdef CFG_NU_OS_CONN_USB_FUNC_MS_USER_ENABLE       
       if ((DV_COMPARE_LABELS(&usbf_ms_lbl, &(labels_list[i]))))
       {
           operating_mode |= USB_SYS_SESS_MODE_STORAGE;
       }
#endif
#ifdef CFG_NU_OS_CONN_USB_HOST_MS_USER_ENABLE
       if ((DV_COMPARE_LABELS(&usbh_ms_lbl, &(labels_list[i]))))
       {
           operating_mode |= USB_SYS_SESS_MODE_STORAGE;
       }
#endif
#ifdef CFG_NU_OS_CONN_USB_FUNC_DFU_ENABLE
            if ((DV_COMPARE_LABELS(&usbf_dfu_lbl1, &(labels_list[i]))) ||
                (DV_COMPARE_LABELS(&usbf_dfu_lbl2, &(labels_list[i]))))
            {
                operating_mode |= USB_SYS_SESS_MODE_DFU;
            }
#endif
#ifdef CFG_NU_OS_CONN_USB_FUNC_COMM_ETH_ENABLE
       if ((DV_COMPARE_LABELS(&usbf_eth_lbl,  &(labels_list[i])))||
           (DV_COMPARE_LABELS(&usbf_ndis_lbl,  &(labels_list[i]))))
       {
           operating_mode |= USB_SYS_SESS_MODE_CDC;
       }
#endif
#ifdef CFG_NU_OS_CONN_USB_HOST_COMM_ETH_ENABLE
       if ((DV_COMPARE_LABELS(&usbh_eth_lbl,  &(labels_list[i]))))
       {
           operating_mode |= USB_SYS_SESS_MODE_CDC;
       }
#endif
#ifdef CFG_NU_OS_CONN_USB_FUNC_COMM_MDM_ENABLE
       if ((DV_COMPARE_LABELS(&usbf_mdm_lbl, &(labels_list[i]))))
       {
           operating_mode |= USB_SYS_SESS_MODE_MODEM;
       }
#endif
#ifdef CFG_NU_OS_CONN_USB_HOST_COMM_MDM_ENABLE
       if ((DV_COMPARE_LABELS(&usbh_mdm_lbl, &(labels_list[i]))))
       {
           operating_mode |= USB_SYS_SESS_MODE_MODEM;
       }
#endif
#ifdef CFG_NU_OS_CONN_USB_HOST_HID_KEYBOARD_ENABLE
       if ((DV_COMPARE_LABELS(&usbh_kbd_lbl, &(labels_list[i]))))
       {
           operating_mode |= USB_SYS_SESS_MODE_KBD;
       }
#endif

#ifdef CFG_NU_OS_CONN_USB_HOST_HID_MOUSE_ENABLE
       if ((DV_COMPARE_LABELS(&usbh_mse_lbl, &(labels_list[i]))))
       {
           operating_mode |= USB_SYS_SESS_MODE_MSE;
       }
#endif
#ifdef CFG_NU_OS_CONN_USB_HOST_AUDIO_ENABLE
       if ((DV_COMPARE_LABELS(&usbh_audio_lbl, &(labels_list[i]))))
       {
           operating_mode |= USB_SYS_SESS_MODE_AUDIO;
       }
#endif

#ifdef CFG_NU_OS_CONN_USB_FUNC_HID_KBD_ENABLE
       if ((DV_COMPARE_LABELS(&usbf_kbd_lbl, &(labels_list[i]))))
       {
           operating_mode |= USB_SYS_SESS_MODE_KBD;
       }
#endif
#ifdef CFG_NU_OS_CONN_USB_FUNC_HID_MSE_ENABLE
       if ((DV_COMPARE_LABELS(&usbf_mse_lbl, &(labels_list[i]))))
       {
           operating_mode |= USB_SYS_SESS_MODE_MSE;
       }
#endif

    }

    return(operating_mode);
}

/*************************************************************************
* FUNCTION
*       NU_USB_SYS_Get_Dev_ID
*
* DESCRIPTION
*
*       An internal routine to find the driver ID of the instance in which 
*       a particular session has been opened.
*
* INPUTS
*
*       dev_handle              An already opened session handle.
*       dev_id_out              OUTPUT: pointer to DV_DE_ID value.
*
* OUTPUTS
*
*      NU_SUCCESS        ID populated.
*      NU_NOT_PRESENT    Unable to find a related Instance.
*
*************************************************************************/
STATUS NU_USB_SYS_Get_Dev_ID(VOID *dev_handle, DV_DEV_ID *dev_id_out)
{
    STATUS              status = NU_NOT_PRESENT;
    NU_USB_SYS_DEVICE   *curr_device;

    status      = NU_NOT_PRESENT;
    *dev_id_out = 0;
    curr_device = USB_System_Server.dev_head;

    if(curr_device != NU_NULL)
    {
        for(;;)
        {
            if (curr_device->usr_ptr == dev_handle)
            {
                *dev_id_out = curr_device->drvr_id;
                status      = NU_SUCCESS;
                break;
            }

            curr_device = (NU_USB_SYS_DEVICE*)(curr_device->dev_link.cs_next);
            
            if ( curr_device == USB_System_Server.dev_head )
            {
                break;
            }
        }
    }

    return ( status );
}
