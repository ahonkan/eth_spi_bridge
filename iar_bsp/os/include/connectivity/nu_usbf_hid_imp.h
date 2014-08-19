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
*       nu_usbf_hid_imp.h
*
* COMPONENT
*
*       HID Component / Nucleus USB Device Software.
*
* DESCRIPTION
*
*       This file contains the internal declarations for the HID class
*       driver.
*
* DATA STRUCTURES
*       NU_USBF_HID_DISPATCH                Dispatch table for the HID
*                                           class device.
*       NU_USBF_HID                         HID class driver control block.
*       USBF_HID_DEVICE                     Internal representation of HID
*                                           class device.
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       nu_usbf_hid_dat.h                   Dispatch Table Definitions.
*       nu_usbf_drvr_ext.h                  Function driver external
*                                           definition.
*       tc_defs.h                           Thread component external
*                                           definitions.
*
**************************************************************************/

/* ==================================================================== */

#ifndef _NU_USBF_HID_IMP_H
#define _NU_USBF_HID_IMP_H_

/* ==============  USB Include Files =================================  */
#include "connectivity/nu_usbf_drvr_ext.h"

/* =====================  Global data ================================  */
#define HIDF_CLASS                          0x03
#define HIDF_SUBCLASS                       0x01
#define USB_DESC_TYPE_HID                   0x21
#define HIDF_MAX_OUT_REPORT_LENGTH          8
#define HIDF_SETUP_DIR_BIT                  (1<<7)
#define HIDF_EP_DIR_BIT                     (1<<7)

/* IN Transfer state values */
#define HIDF_CTRL_IN_XFER_CMPLT             2
#define HIDF_REPORT_SENT                    3

/* IN transfer state values */
#define HIDF_SEND_ZERO_LEN                  1
#define HIDF_ZERO_LEN_SENT                  2


/* Class Specific Requests */
#define HIDF_GET_DESCRIPTOR                 0x06
#define HIDF_GET_REPORT                     0x01
#define HIDF_SET_REPORT                     0x09
#define HIDF_SET_IDLE                       0x0A
#define HIDF_GET_PROTOCOL                   0x03
#define HIDF_SET_PROTOCOL                   0x0B

#define HIDF_MAX_DEVICES                    5

/* Event Flags */
#define HIDF_DEVICE_CONNECT                 4
#define HIDF_DATA_SENT                      5

/* =============Dispatch Table ============= */
typedef struct _nu_usbf_hid_dispatch
{
    NU_USBF_DRVR_DISPATCH   dispatch;
    /* Extension to NU_USBF_DRVR services
     * should be declared here */
}
NU_USBF_HID_DISPATCH;

/* =============Command Block============== */
typedef struct _nu_usbf_hid_cmd
{
    UINT8   *cmd_data;                      /* Command data pointer. */
    UINT32  command;                        /* Command code. */
    UINT32  data_len;                       /* Length of command data. */
    UINT16  cmd_Value;                      /* Value from setup packet.*/
    UINT8   pad[2];
}
NU_USBF_HID_CMD;


/* HID Device structure */
typedef struct _usbf_hid_device
{
    NU_USB_DEVICE   *hid_dev;               /* USB Device */
    NU_USBF_DRVR    *drvr;                  /* HID Driver */
    UINT8           *cmd_buffer;            /* Control command data buffer */

    /* IRP structure for control transfer */
    NU_USB_IRP  ctrl_irp;
    /* IRP structure for interrupt transfer */
    NU_USB_IRP  intr_irp;
    /* Structure for control pipe command*/
    NU_USBF_HID_CMD hid_ctrl_cmd;
    /* Buffer for received setup packet */
    NU_USB_SETUP_PKT setup_pkt;
    NU_USBF_USER *hid_user;                 /* Pointer to HID user */
    NU_USB_INTF *hid_intf;                  /* Pointer to HID interface */
    /* Pointer to HID alternate setting */
    NU_USB_ALT_SETTG *hid_alt_set;
    /* Pointer to control pipe of driver*/
    NU_USB_PIPE *control_pipe;
    /* Pointer to interrupt IN pipe */
    NU_USB_PIPE *intr_in_pipe;
    /* Pointer to interrupt OUT pipe */
    NU_USB_PIPE *intr_out_pipe;
    /* Pointer to report transfer buffer */
    UINT8   *report_data;

    /* Buffer for received bus event */
    UINT32  usb_event;
     /* Field for report transfer state */
    UINT32  report_tx_state;
    /* Field for report transfer length */
    UINT32  report_len;

}USBF_HID_DEVICE;


/* ========== Device Control Block ==========*/
typedef struct _nu_usbf_hid
{
    NU_USBF_DRVR parent;                    /* Parent's control block */

    /* HID device for this driver */
    USBF_HID_DEVICE *devices[HIDF_MAX_DEVICES];

    NU_MEMORY_POOL *mem_pool;               /* Pointer to driver's memory
                                               pool */
    /* Temporary buffer for hid device pointers */
    NU_USB_DEVICE *temp_hid_dev[HIDF_MAX_DEVICES];

    /* Temporary buffer for hid interface pointers */
    NU_USB_INTF *temp_hid_intf[HIDF_MAX_DEVICES];

    UINT8  hid_init_intf;

    /* This is to properly align this structure on 32-bit boundary */
    UINT8  pad[3];
}
NU_USBF_HID;


/* ======================== Functions ================================= */
VOID USBF_HID_Hisr(VOID);
VOID USBF_HID_Handle_Disconnected(USBF_HID_DEVICE *dev);
VOID USBF_HID_Handle_Ctrl_Xfer_Cmplt(USBF_HID_DEVICE *dev);
VOID USBF_HID_Handle_Event_Sent(USBF_HID_DEVICE *dev);
VOID USBF_HID_Handle_USB_Event_Rcvd(USBF_HID_DEVICE *dev);
VOID USBF_HID_Ctrl_Data_IRP_Cmplt(NU_USB_PIPE * pipe, NU_USB_IRP  *irp);
VOID USBF_HID_Interrupt_Data_IRP_Cmplt(NU_USB_PIPE * pipe,
                                       NU_USB_IRP  *irp);
STATUS USBF_HID_Handle_Report_Sent(USBF_HID_DEVICE *hid);
STATUS USBF_HID_Connect_User(USBF_HID_DEVICE *dev);
STATUS USBF_HID_Handle_Class_Req_Rcvd(USBF_HID_DEVICE *dev);
STATUS USBF_HID_Handle_Interrupt_Transfer(USBF_HID_DEVICE *dev);

/* ==================================================================== */

#include "connectivity/nu_usbf_hid_dat.h"

/* ==================================================================== */

#endif /* _NU_USBF_HID_IMP_H  */

/* ======================  End Of File  =============================== */
