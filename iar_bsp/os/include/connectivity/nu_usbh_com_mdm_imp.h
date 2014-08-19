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
*
*     nu_usbh_com_mdm_imp.h
*
* COMPONENT
*     Nucleus USB host software : Communication user driver
*
* DESCRIPTION
*     This file contains the control block and other internal data
*     structures and definitions for Communication user driver.
*
* DATA STRUCTURES
*     NU_USBH_COM_MDM_DISPATCH    Modem user dispatch table.
*     NU_USBH_COM_MDM_DISPATCH    Modem user dispatch table.
*     NU_USBH_COM_MDM             Modem user control block.
*
* FUNCTIONS
*     None
*
* DEPENDENCIES
*     nu_usbh_com_user_ext.h"
*     nu_usbh_com_mdm_dat.h"
*
**************************************************************************/

/* ===================================================================== */
#ifndef _NU_USBH_COM_MDM_IMP_H_
#define _NU_USBH_COM_MDM_IMP_H_

#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================================================================== */

/* =========================== Include Files =========================== */
/* Specializing nu_usbh_user driver */
#include "connectivity/nu_usbh_com_user_ext.h"

/* =========================== Control Blocks ========================== */

typedef NU_USBH_COM_USER       NU_USBH_COM_MDM;
typedef NU_USBH_COM_USER_HDL   NU_USBH_COM_MDM_HANDL;

typedef struct _nu_usb_com_mdm_dev
{
    NU_USBH_COM_USR_DEVICE  user_device;
    NU_USBH_COM_ACM_INFORM  inform_str;
    VOID*                   usr_var1;
    VOID*                   usr_var2;
    UINT32                  tx_buffer_write;
    UINT32                  rx_buffer_read;

} NU_USBH_COM_MDM_DEVICE;

/* =========================== Include Files =========================== */
/* Making internals visible */
#include "connectivity/nu_usbh_com_mdm_dat.h"
/* ===================================================================== */

#endif /* _NU_USBH_COM_MDM_IMP_H_ */
