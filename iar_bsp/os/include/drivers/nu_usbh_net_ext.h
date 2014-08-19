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
*
* FILE NAME
*     nu_usbh_net_ext.h
*
* COMPONENT
*     Nucleus USB host software : Communication Application
*
* DESCRIPTION
*     This file contains the external interface for nucleus NET.
*
* DATA STRUCTURES
*     None.
*
* FUNCTIONS
*     None
*
* DEPENDENCIES
*     nu_usbh_com_ext.h     Nucleus USB Services.
*     nu_net.h              Nucleus NET services.

*
**************************************************************************/

/* ===================================================================== */
#ifndef _NU_USBH_NET_EXT_H_
#define _NU_USBH_NET_EXT_H_

#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================================================================== */

/* =========================== Include Files =========================== */
#include "networking/nu_net.h"
#include "drivers/ethernet.h"
#include "connectivity/nu_usb.h"
#include "drivers/ethernet.h"

#define DEV_CONNECT  1



typedef struct _nu_usbh_net_device
{
    CS_NODE            link_dev;
    DV_DEV_HANDLE      device_handle;
    DV_DEVICE_ENTRY    *device_entry;
    NU_USBH_COM_ETH_DEVICE* usb_dev;
    NU_DEVICE          eth_dev;
    PM_STATE_ID        pm_state; 
    CHAR               name[DEV_NAME_LENGTH]; 
}NU_USBH_NET_DEV;

typedef struct _nu_usbh_net_driver
{
    NU_USBH_NET_DEV    *head_device;
    NU_USBH_COM_ETH    *ethernet_drvr; 
    CHAR               reg_path[REG_MAX_KEY_LENGTH];
    
}NU_USBH_NET_DRVR;

extern NU_EVENT_GROUP   USB_Net_Init;

STATUS NU_USBH_NET_Init_Intf (
       DV_DEVICE_ENTRY*    device);

STATUS NU_USBH_COM_ETH_Open_Dev (
       UINT8*              ether_addr,
       DV_DEVICE_ENTRY*    device);

STATUS NU_USBH_COM_ETH_Xmit_Packet(
       DV_DEVICE_ENTRY*    device,
       NET_BUFFER*         buf_ptr);

STATUS NU_USBH_COM_ETH_Ioctl(
       DV_DEVICE_ENTRY*    dev,
       INT                 option,
       DV_REQ*             d_req);

VOID UHC_Get_Recv_Buffer(
       DV_DEVICE_ENTRY*    device,
       NU_USBH_COM_XBLOCK* cb_com_xblock);

VOID NU_USBH_NET_Receive_Handler (
     VOID*               device_in,
     NU_USBH_COM_XBLOCK* xblock);


VOID NU_USBH_NET_Event_Handler(
     VOID*                 device,
     NU_USBH_COM_XBLOCK*   xblock);

STATUS NU_USBH_NET_Attach_Device (
     DV_DEVICE_ENTRY*      net_device,
     VOID*                 session);

STATUS NU_USBH_NET_Detach_Device (
     DV_DEVICE_ENTRY*      net_device,
     VOID*                 session);

STATUS NU_USBH_NET_Entry(CHAR* path);

STATUS    NU_USBH_NET_DM_Open (VOID* dev_handle);

STATUS    NU_USBH_NET_DM_Close(VOID* dev_handle);

STATUS    NU_USBH_NET_DM_Read( VOID*    dev_handle,
                            VOID*    buffer,
                            UINT32      numbyte,
                            OFFSET_T byte_offset,
                            UINT32*     bytes_read_ptr);
                            
STATUS   NU_USBH_NET_DM_Write(VOID*       dev_handle,
                            const VOID* buffer,
                            UINT32         numbyte,
                            OFFSET_T    byte_offset,
                            UINT32*        bytes_written_ptr);
                            
STATUS   NU_USBH_NET_DM_IOCTL   (VOID*     dev_handle,
                                  INT       ioctl_num,
                                  VOID*     ioctl_data,
                                  INT       ioctl_data_len);                                                        

VOID NU_USBH_NET_New_Connection (NU_USB_USER* pcb_user_drvr,
                                 VOID*        session,
                                 VOID*        information);

VOID NU_USBH_NET_Disconnection (NU_USB_USER  *pcb_user_drvr,
                                VOID         *session);


NU_USBH_NET_DEV* NU_USBH_NET_Find_Device_From_NET(DV_DEVICE_ENTRY* device);

#define NU_USBH_COM_ETH_Init NU_USBH_NET_Init_Intf

#endif /* _NU_USBH_NET_EXT_H_ */
