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
*       nu_usbh_com_eth_imp.h
*
*
* COMPONENT
*       Nucleus USB host software : Communication user driver
*
* DESCRIPTION
*       This file contains the control block and other internal data
*       structures and definitions for Communication user driver.
*
* DATA STRUCTURES
*
*       NU_USBH_COM_ETH_DEVICE            Ethernet device control block.
*
* FUNCTIONS
*       None
*
* DEPENDENCIES
*       nu_usbh_com_user_ext.h"
*       nu_usbh_com_eth_dat.h"
*
**************************************************************************/

/* ===================================================================== */
#ifndef _NU_USBH_COM_ETH_IMP_H_
#define _NU_USBH_COM_ETH_IMP_H_

#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================================================================== */

/* =========================== Include Files =========================== */
/* Specializing nu_usbh_user driver */
#include "connectivity/nu_usbh_com_user_ext.h"

/* =========================== Control Blocks ========================== */

VOID NU_USBH_COM_ETH_User_Data_Poll(
     UINT32   pcb_drvr,
     VOID*    pcb_com_device);

typedef NU_USBH_COM_USER NU_USBH_COM_ETH;
typedef NU_USBH_COM_USER_HDL NU_USBH_COM_ETH_HANDL;

typedef struct _nu_usb_com_eth_dev
{
    NU_USBH_COM_USR_DEVICE  user_device;
    NU_USBH_COM_ECM_INFORM  inform_str;
    VOID*                   net_dev;
    VOID*                   sys_ptr;
    UINT32                  sys_var1;
    UINT32                  sys_var2; 
} NU_USBH_COM_ETH_DEVICE;

/* =========================== Include Files =========================== */
/* Making internals visible */
#include "connectivity/nu_usbh_com_eth_dat.h"
/* ===================================================================== */

#endif /* _NU_USBH_COM_ETH_IMP_H_ */
