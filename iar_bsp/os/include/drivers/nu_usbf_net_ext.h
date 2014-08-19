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
*     nu_usbf_net_ext.h
*
* COMPONENT
*     Nucleus USB software : Nucleus NET Driver
*
* DESCRIPTION
*     This file contains the interface layer of function ethernet driver
*     with Nucleus NET.
*
* DATA STRUCTURES
*     None.
*
* FUNCTIONS
*     None
*
* DEPENDENCIES
*     nu_net.h              Nucleus NET services
*     nu_usbf_comm_ext.h    All USB Comm. definitions
*
**************************************************************************/

/* ===================================================================== */
#ifndef _NU_USBF_NET_EXT_H_
#define _NU_USBF_NET_EXT_H_

#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================================================================== */

/* =========================== Include Files =========================== */

#include "networking/nu_net.h"
#include "connectivity/nu_usbf_comm_ext.h"
#include "drivers/ethernet.h"

#define ETHF_NET_MAX_SEG_SIZE   CFG_NU_OS_DRVR_USB_FUNC_NET_IF_MAX_BUF_SIZE
#define ETHF_NUM_RX_GRP_BUFS    CFG_NU_OS_DRVR_USB_FUNC_NET_IF_NUM_RX_GRP_BUFS

#define USBF_DEV_LINK_UP        1
#define USBF_DEV_LINK_DOWN      0

/*----------------------Structures--------------------------*/
typedef struct nu_usb_net_device
{
    CS_NODE            link_dev;
    NU_USBF_USER       *user;
    VOID               *usb_device;
    DV_DEV_HANDLE      device_handle;
    DV_DEVICE_ENTRY    *device_entry;
    NU_DEVICE          eth_dev; 
    NU_SEMAPHORE       tx_sem;
    UINT8              *Buffer_List1[ETHF_NUM_RX_GRP_BUFS];
    UINT8              *Buffer_List2[ETHF_NUM_RX_GRP_BUFS];
    COMMF_RX_BUF_GROUP Rx_Group1;
    COMMF_RX_BUF_GROUP Rx_Group2;
    UINT8              *Tx_Buff;
    UINT32             device_type;
    UINT32             Ethf_Offset;
    UINT8              link_status; 
    CHAR               name[8];
}NU_USBF_NET_DEV;

/*----------------------Structures--------------------------*/
typedef struct nu_usb_net_drvr
{
    NU_USBF_NET_DEV    *head_device;
    NU_MEMORY_POOL     *mem_pool;
    NU_USBF_COMM_DATA  *com_data_drvr;
    NU_USBF_NDIS_USER  *ndis_pt;
    NU_USBF_ETH        *eth_pt;
    CHAR               reg_path[REG_MAX_KEY_LENGTH];
}NU_USBF_NET_DRVR;



/*-----------------Functions Declerations-------------------*/
STATUS NU_USBF_COMM_ETH_Init (
       DV_DEVICE_ENTRY*    device);

STATUS NU_USBF_NET_Open_Dev (
       UINT8*              ether_addr,
       DV_DEVICE_ENTRY*    device);

STATUS NU_USBF_NET_Xmit_Packet(
       DV_DEVICE_ENTRY*    device,
       NET_BUFFER*         buf_ptr);

STATUS NU_USBF_NET_Ioctl(
       DV_DEVICE_ENTRY*    dev,
       INT                 option,
       DV_REQ*             d_req);

STATUS NU_USBF_NET_Check_Connection(
       NU_DEVICE*          device,
       UINT32              suspend);

STATUS NU_USBF_NET_Init(VOID);

VOID NU_USBF_NET_Notify (
                  NU_USBF_USER *user,
                  UINT32       event,
                  VOID         *handle);

VOID NU_USBF_NET_Rx_Buff_Init(
                 NU_USBF_NET_DEV* net_device);

VOID NU_USBF_NET_Buffer_Disconn(
                 NU_USBF_NET_DEV* net_device);

VOID NU_USBF_NET_Mark_buffer_Free(
                 NU_USBF_NET_DEV* net_device,
                 UINT8*           data_buffer);

VOID NU_USBF_NET_Rcvd_Cb(
                 UINT8        *eth_frame_data,
                 UINT32       eth_frame_length,
                 UINT8        *pkt_info_data,
                 UINT32       pkt_info_length,
                 VOID         *handle);

VOID NU_USBF_NET_Tx_Done(
                 UINT8        *cmpltd_data_buffer,
                 UINT32       length,
                 VOID         *handle);

VOID NU_USBF_NET_Submit_Rx_Buffers(
                 COMMF_RX_BUF_GROUP *group,
                 NU_USBF_NET_DEV    *handle);

VOID NU_USBF_NET_ETH_Ioctl (
                 NU_USBF_USER   *user,
                 UINT32          code,
                 VOID           *data,
                 UINT32 length);

VOID NU_USBF_NET_Buff_Finished(
                 NU_USBF_USER  *user,
                 VOID          *buffer_grp,
                 VOID          *handle);

STATUS    NU_USBF_NET_DM_Open (
                 VOID*         dev_handle);

STATUS    NU_USBF_NET_DM_Close(
                 VOID*         dev_handle);

STATUS NU_USBF_NET_DM_Read (
                 VOID          *session_handle,
                 VOID          *buffer,
                 UINT32        numbyte,
                 OFFSET_T      byte_offset,
                 UINT32        *bytes_read);

STATUS NU_USBF_NET_DM_Write (
                 VOID          *session_handle,
                 const VOID    *buffer,
                 UINT32        numbyte,
                 OFFSET_T      byte_offset,
                 UINT32        *bytes_written);

STATUS NU_USBF_NET_DM_IOCTL(
                 VOID          *session_handle,
                 INT           cmd,
                 VOID          *data,
                 INT           length);

STATUS NU_USBF_NET_Initialize(
                 CHAR*         path);

NU_USBF_NET_DEV* NU_USBF_NET_Find_Device(
                 VOID*         handle);
                 
NU_USBF_NET_DEV* NU_USBF_NET_Find_Net_Device(
                 VOID*         handle);

NU_USBF_NET_DEV* NU_USBF_NET_Find_Net_Device_By_Name(
                 CHAR*          name);


#endif /* _NU_USBF_NET_EXT_H_ */
