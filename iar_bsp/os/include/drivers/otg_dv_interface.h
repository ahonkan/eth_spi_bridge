/*************************************************************************
*
*               Copyright 2012 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
*   FILE NAME
*
*       otg_dv_interface.h
*
*   COMPONENT
*
*       Generic USB OTG Controller Driver
*
*   DESCRIPTION
*
*       This file contains constant definitions and function declarations
*       for generic USB OTG controller drivers.
*
*************************************************************************/
#ifndef OTG_DV_INTERFACE_H
#define OTG_DV_INTERFACE_H

#include "drivers/usbh_hwctrl_dv_interface.h"
#include "drivers/usbf_dv_interface.h"

/*********************************/
/* FUNCTION PROTOTYPES           */
/*********************************/
STATUS OTG_Dv_Register (const CHAR * key);
STATUS OTG_Dv_Unregister (const CHAR *key, INT startstop, DV_DEV_ID dev_id);

/* This structure serves as a session handle for USB OTG controller.
 */
typedef struct _usb_otg_hwctrl_session_handle
{
    USBF_SESSION_HANDLE        *usbf_handle;
    USBH_HWCTRL_SESSION_HANDLE *usbh_handle;
    
}OTG_HWCTRL_SESSION_HANDLE;

/* IOCTL commands definitions for use with Nucleus USB Base Stack. */
#define NU_USB_OTG_IOCTL_BASE                          (DV_IOCTL0 + 1)
#define NU_USB_OTG_IOCTL_GET_ROLE                      0
#define NU_USB_OTG_IOCTL_START_HOST_SESSION            1
#define NU_USB_OTG_IOCTL_START_FUNC_SESSION            2
#define NU_USB_OTG_IOCTL_END_HOST_SESSION              3
#define NU_USB_OTG_IOCTL_END_FUNC_SESSION              4
#define NU_USB_OTG_IOCTL_NOTIFY_ROLE_SWITCH            5

typedef struct _nu_usbotg_notify_callbacks
{
    VOID (*usbh_role_switch_cb)(             /* Host notify role switch callback */
          NU_USB_HW *hw, 
          UINT8 port_id, 
          UINT8 role);
          
    VOID (*usbf_role_switch_cb)(            /* Function notify role switch callback */
          NU_USB_HW *hw, 
          UINT8 port_id, 
          UINT8 role);

}NU_USBOTG_NOTIFY_CALLBACKS;

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif      /* OTG_DV_INTERFACE_H */

