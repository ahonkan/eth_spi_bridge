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
*       nu_usbh_com_user_imp.h
*
*
* COMPONENT
*
*       Nucleus USB host software : Communication user driver.
*
* DESCRIPTION
*
*       This file contains the control block and other internal data
*       structures and definitions for Communication user driver.
*
* DATA STRUCTURES
*
*       NU_USBH_COM_USER_HDL            Application call back handlers
*                                       structure.
*       NU_USBH_COM_USER_DISPATCH       Communication user dispatch table.
*       NU_USBH_COM_USER                Communication user control block.
*       NU_USBH_COM_USR_DEVICE          Communication user device.
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       nu_usbh_user_ext.h                  Base component services.
*       nu_usbh_com_ext.h                   Class driver services.
*
**************************************************************************/

/* ===================================================================== */
#ifndef _NU_USBH_COM_USER_IMP_H_
#define _NU_USBH_COM_USER_IMP_H_

#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================================================================== */

/* =========================== Include Files =========================== */
/* Specializing nu_usbh_user driver */
#include "connectivity/nu_usbh_user_ext.h"
#include "connectivity/nu_usbh_com_ext.h"

/* =========================== Control Blocks ========================== */

/* Communication user driver control block */

typedef struct _nu_usbh_com_user_hdl
{
    VOID (*Connect_Handler )  (NU_USB_USER*        pcb_user_drvr,
                               VOID*               device,
                               VOID*               information);

    VOID (*Disconnect_Handler)(NU_USB_USER*        pcb_user_drvr,
                               VOID*               device);

    VOID (*Data_Handler )     (VOID*               device,
                               NU_USBH_COM_XBLOCK* xblock);

    VOID (*Event_Handler)     (VOID*               device,
                               NU_USBH_COM_XBLOCK* xblock);
}
NU_USBH_COM_USER_HDL;

typedef struct _nu_usbh_com_user
{
    NU_USBH_USER          parent;
    NU_USBH_COM*          pcb_class_drvr;
    VOID*                 pcb_first_device;
    NU_USBH_COM_USER_HDL* p_hndl_table;
}
NU_USBH_COM_USER;

typedef struct _nu_usb_com_usr_dev
{
    CS_NODE                node;
    NU_USB_USER*           user_drvr;
    NU_USB_DRVR*           class_drvr;
    NU_USBH_COM_USER*      com_user;
    NU_USBH_COM_DEVICE*    usb_device;

    NU_USBH_COM_XBLOCK     rx_xblock;
    NU_TASK*               data_poll_task;
    NU_SEMAPHORE           poll_task_sync;
    UINT8*                 data_poll_stack;
} NU_USBH_COM_USR_DEVICE;

VOID NU_USBH_COM_User_Poll_Data( UINT32,
                                 VOID* );

typedef VOID (*COM_USER_FPTR) (struct _nu_usbh_com_user *user,
                               VOID*  pcb_curr_device);

/* Communication user driver dispatch table */
typedef struct _nu_usbh_com_user_dispatch
{
    NU_USBH_USER_DISPATCH       dispatch;
    /* Other new members of control block (if any) go here. */
    STATUS (*Connect_Handler )      (NU_USB_USER*  pcb_user_drvr,
                                     NU_USB_DRVR*  pcb_com_drvr,
                                     VOID*         com_device,
                                     VOID*         dev_inform);

    STATUS (*Intr_Response_Handler) (VOID*                pcb_curr_device,
                                     NU_USBH_COM_XBLOCK*  pcb_xblock);

}
NU_USBH_COM_USER_DISPATCH;

#endif /* _NU_USBH_COM_USER_IMP_H_ */
