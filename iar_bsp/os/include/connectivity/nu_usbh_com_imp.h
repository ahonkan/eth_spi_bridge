/**************************************************************************
*
*               Copyright 2005  Mentor Graphics Corporation
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
*       nu_usbh_com_imp.h

*
* COMPONENT
*
*       Nucleus USB host software. Communication class driver.
*
* DESCRIPTION
*
*       This file contains the control block and other internal data
*       structures and definitions for Communication class host driver.
*
* DATA STRUCTURES
*
*       NU_USBH_COM                         Communication class host driver
*                                           control block.
*       NU_USBH_COM_DATA                    Communication class host driver
*                                           Data interface control block.
*       NU_USBH_COM_DEVICE                  This structure maintains
*                                           information about each
*                                           Communication device served by
*                                           this class driver.
*       NU_USBH_COM_XBLOCK                  Communication class driver's
*                                           data transfer structure.
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       nu_usbh_com_cfg.h                   Library built options.
*       nu_usbh_com_dat.h                   Dispatch table Definitions.
*
**************************************************************************/

/* ===================================================================== */
#ifndef _NU_USBH_COM_IMP_H_
#define _NU_USBH_COM_IMP_H_
/* ===================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================================================================== */

#ifdef __cplusplus
extern "C" {                                 /* C declarations in C++. */
#endif

/* Include files */
#include "connectivity/nu_usbh_com_cfg.h"

/* #Defines */
/* bInterfaceClass */
#define UH_COM_CLASS_CODE      0x02          /* Communication class code */
#define UH_COM_DATA_CLASS_CODE 0x0A          /* Data class code          */

/* bInterfaceSubClass Codes */
#define UH_DTL_CTRL_MDL 0x01
#define UH_ABS_CTRL_MDL 0x02
#define UH_TEL_CTRL_MDL 0x03
#define UH_MUL_CTRL_MDL 0x04
#define UH_CAP_CTRL_MDL 0x05
#define UH_ETH_CTRL_MDL 0x06
#define UH_ATM_CTRL_MDL 0x07

/* Functional descriptors */
#define UH_HEADER_FD      0x00
#define UH_CALL_MGMT_FD   0x01
#define UH_ACM_MGMT_FD    0x02
#define UH_DCM_MGMT_FD    0x03
#define UH_TEL_RING_FD    0x04
#define UH_TEL_CLST_FD    0x05
#define UH_UNION_FD       0x06
#define UH_CNT_SEL_FD     0x07
#define UH_TEL_OMOD_FD    0x08
#define UH_USBH_TER_FD    0x09
#define UH_NET_CTER_FD    0x0A
#define UH_PRT_UNIT_FD    0x0B
#define UH_EXT_UNIT_FD    0x0C
#define UH_MCH_MGMT_FD    0x0D
#define UH_CCM_MGMT_FD    0x0E
#define UH_ETH_NET_FD     0x0F
#define UH_ATM_NET_FD     0x10

/* Class specific request codes.*/
#define UH_SEND_ENCAPSULATED_COMMAND     0x00
#define UH_GET_ENCAPSULATED_RESPONSE     0x01
#define UH_SET_COMM_FEATURE              0x02
#define UH_GET_COMM_FEATURE              0x03
#define UH_CLEAR_COMM_FEATURE            0x04

/* Internal Definitions.*/
#define NU_USBH_COM_DATA_IN              0x01
#define NU_USBH_COM_DATA_OUT             0x02
#define NU_USBH_COM_TRANSFER_ERROR       0x03
#define NU_USBH_COM_NOT_SUPPORTED        0x04

/* Error Status.*/
#define NU_USBH_COM_XFER_FAILED         -3983
#define NU_USBH_COM_XFER_ABORTED        -3984
#define NU_USBH_COM_XFER_ERROR          -3985

/* Event Flags */
#define UHC_CTRL_SENT  0x01
#define UHC_INTR_SENT  0x02
#define UHC_IN_SENT    0x04
#define UHC_OUT_SENT   0x08

/* Internal Data Structures */

typedef struct usbh_com_tb
{
    VOID   *p_data_buf;                      /* Data in/out buffer. */
    UINT32 data_length;                      /* Data buffer length. */
    UINT32 transfer_length;                  /* Transferred length.*/
    UINT32 direction;                        /* Transfer direction. */
} NU_USBH_COM_XBLOCK;

/* Each device connected to Communication class driver is remembered by
 * this structure.
 */
typedef struct com_device
{
    /* USB Resources required by the driver. */
    CS_NODE            node;
    NU_USB_DRVR        *pcb_com_drvr;
    NU_USB_DRVR        *pcb_data_drvr;
    NU_USB_USER        *pcb_user_drvr;
    NU_USB_STACK       *pcb_stack;
    NU_USB_DEVICE      *pcb_device;
    NU_USB_INTF        *pcb_com_intf;
    NU_USB_INTF        *pcb_data_intf;
    NU_USB_ALT_SETTG   *pcb_com_alt_settg;
    NU_USB_ALT_SETTG   *pcb_data_alt_settg;
    NU_USBH_CTRL_IRP   *ctrl_irp;

    /* Resources required by the driver. */
    NU_TASK            *poll_task;
    UINT8              *poll_stack;
    NU_EVENT_GROUP     trans_events;
    NU_SEMAPHORE       sm_ctrl_trans;
    NU_SEMAPHORE       sm_in_trans;
    NU_SEMAPHORE       sm_out_trans;
    VOID*              model_spc_inform;
    NU_USBH_COM_XBLOCK cb_xfer_block;

    /* Pipes required by the driver. */
    NU_USB_PIPE        *pcb_control_pipe;
    NU_USB_PIPE        *pcb_intr_in_pipe;
    NU_USB_PIPE        *pcb_bulk_in_pipe;
    NU_USB_PIPE        *pcb_bulk_out_pipe;
    NU_USB_PIPE        *pcb_iso_in_pipe;
    NU_USB_PIPE        *pcb_iso_out_pipe;
    UINT8              master_intf;
    UINT8              slave_intf;
    UINT16             pad2;
} NU_USBH_COM_DEVICE;

/* Communication class driver Data interface control block. */
typedef struct nu_usbh_com_data
{
    /* The base class control block. */
    NU_USB_DRVR        cb_drvr;
    VOID               *pcb_com_drvr;
}
NU_USBH_COM_DATA;

/* Communication class driver control block. */
typedef struct nu_usbh_com
{
    /* The base class control block. */
    NU_USB_DRVR        cb_drvr;
    NU_USBH_COM_DATA   cb_data_drvr;
    NU_USBH_COM_DEVICE *pcb_first_device;
    NU_MEMORY_POOL     *p_memory_pool;
    NU_SEMAPHORE       sm_comdrvr_lock;
}
NU_USBH_COM;

/* Class driver API implementation prototypes. */
/* Internal Function Declarations. */

NU_USB_USER *NU_USBH_COM_Find_User (
       NU_USBH_COM*         pcb_com_drvr,
       UINT8                subclass);

STATUS NU_USBH_COM_Transfer_Out (
       NU_USBH_COM_DEVICE*   pcb_curr_com_dev,
       NU_USBH_COM_XBLOCK*   pcb_xblock);

STATUS NU_USBH_COM_Transfer_In (
       NU_USBH_COM_DEVICE*   pcb_curr_com_dev,
       NU_USBH_COM_XBLOCK*   pcb_xblock);

VOID   NU_USBH_COM_Ctrl_IRP_Complete (
       NU_USB_PIPE*          pcb_pipe,
       NU_USB_IRP*           pcb_irp);

VOID   NU_USBH_COM_Intr_IRP_Complete (
       NU_USB_PIPE*          pcb_pipe,
       NU_USB_IRP*           pcb_irp);

VOID   NU_USBH_COM_TX_IRP_Complete (
       NU_USB_PIPE*          pcb_pipe,
       NU_USB_IRP*           pcb_irp);

VOID   NU_USBH_COM_RX_IRP_Complete (
       NU_USB_PIPE*          pcb_pipe,
       NU_USB_IRP*           pcb_irp);

NU_USBH_COM_DEVICE* NU_USBH_COM_Check_Intf_Pair(
       NU_USBH_COM*         pcb_drvr,
       NU_USB_DEVICE*       pcb_device,
       NU_USB_INTF*         pcb_intf);

VOID   NU_USBH_COM_Intr_Poll(
       UINT32               pcb_drvr,
       VOID*                pcb_com_device);

STATUS NU_USBH_COM_Get_String(
       NU_USBH_COM_DEVICE*  pcb_curr_device,
       UINT8*               buffer_ptr,
       UINT8                size,
       UINT8                string_num);

UINT8* NU_USBH_COM_Parse_Strings(
       UINT8*               buffer_ptr,
       UINT32               size,
       UINT8                index);

STATUS NU_USBH_COM_Init_Device(
       NU_USBH_COM*         pcb_com_drvr,
       NU_USBH_COM_DEVICE*  pcb_curr_device,
       NU_USB_INTF*         pcb_intf,
       UINT8                sub_class);

STATUS NU_USBH_COM_Check_Union(
       NU_USB_DEVICE*       pcb_device,
       NU_USB_INTF*         pcb_intf,
       NU_USBH_COM_DEVICE*  pcb_curr_device);

STATUS NU_USBH_COM_Init_Data_intf (
       NU_USB_DEVICE* pcb_device,
       NU_USB_INTF*   pcb_intf,
       NU_USBH_COM_DEVICE* pcb_curr_device);

#include "connectivity/nu_usbh_com_dat.h"

#ifdef __cplusplus
    }                                        /* C declarations in C++. */
#endif

#endif /* _NU_USBH_COM_IMP_H_ */
