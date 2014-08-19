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
************************************************************************/

/************************************************************************ 
 * 
 * FILE NAME
 *
 *      nu_usbh_user_ext.h 
 * 
 * COMPONENT 
 *      Nucleus USB Host Software 
 * 
 * DESCRIPTION 
 *      This file contains definitions of external interfaces exported by
 *      Nucleus USB Host User Layer.
 * 
 * DATA STRUCTURES 
 *      None
 * 
 * FUNCTIONS 
 *      None
 *
 * DEPENDENCIES 
 *      nu_usbh_user_imp.h       Host User Layer Internal Definitions.
 * 
 *************************************************************************/
/* ==================================================================== */
#ifndef _NU_USBH_USER_EXT_H_
#define _NU_USBH_USER_EXT_H_
/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================== Include Files ================================ */
#include "connectivity/nu_usbh_user_imp.h"

/* USBH_User control block. */
typedef struct usbh_user_dispatch
{
    NU_USB_USER_DISPATCH usb_user_dispatch;
    STATUS (*Wait) (NU_USBH_USER * user,
                    UNSIGNED suspend,
                    VOID **handle_out);
    STATUS (*Open_Device) (NU_USBH_USER * user,
                           VOID *handle);
    STATUS (*Close_Device) (NU_USBH_USER * user,
                            VOID *handle);
    STATUS (*Remove_Device) (NU_USBH_USER * user,
                             VOID *handle,
                             UNSIGNED suspend);
}
NU_USBH_USER_DISPATCH;

/* ====================  Function Prototypes ========================== */

/* NU_USBH_USER user API. */
STATUS _NU_USBH_USER_Create (NU_USBH_USER * cb,
                             CHAR * name,
                             NU_MEMORY_POOL * pool,
                             UINT8 bInterfaceSubclass,
                             UINT8 bInterfaceProtocol,
                             const VOID *dispatch);

STATUS _NU_USBH_USER_Delete (VOID *cb);

/* Client interface for opening/closing a class driver for use, w/reference counting. */
STATUS NU_USBH_USER_Wait (NU_USBH_USER * cb,
                          UNSIGNED suspend,
                          VOID **handle_out);
STATUS NU_USBH_USER_Open_Device (NU_USBH_USER * user,
                                 VOID *handle);
STATUS NU_USBH_USER_Close_Device (NU_USBH_USER * user,
                                  VOID *handle);
STATUS NU_USBH_USER_Remove_Device (NU_USBH_USER * user,
                                   VOID *handle,
                                   UNSIGNED suspend);

/* NU_USBH_USER callback API. */
STATUS NU_USBH_USER_Connect (NU_USBH_USER * cb,
                             NU_USBH_DRVR * class_driver,
                             VOID *handle);
STATUS NU_USBH_USER_Disconnect (NU_USBH_USER * cb,
                                NU_USBH_DRVR * class_driver,
                                VOID *handle);

STATUS _NU_USBH_USER_Lock (NU_USBH_USER * cb);
STATUS _NU_USBH_USER_Unlock (NU_USBH_USER * cb);

STATUS _NU_USBH_USER_Wait (NU_USBH_USER * cb,
                           UNSIGNED suspend,
                           VOID **handle_out);
STATUS _NU_USBH_USER_Open_Device (NU_USBH_USER * user,
                                  VOID *handle);
STATUS _NU_USBH_USER_Close_Device (NU_USBH_USER * user,
                                   VOID *handle);
STATUS _NU_USBH_USER_Remove_Device (NU_USBH_USER * user,
                                    VOID *handle,
                                    UNSIGNED suspend);

/* Extended behavior to the base class methods */
STATUS _NU_USBH_USER_Connect (NU_USB_USER * cb,
                              NU_USB_DRVR * drvr,
                              VOID *handle);
STATUS _NU_USBH_USER_Disconnect (NU_USB_USER * cb,
                                 NU_USB_DRVR * drvr,
                                 VOID *handle);

STATUS NU_USBH_USER_Get_Drvr (NU_USBH_USER * user,
                              VOID *handle,
                              NU_USB_DRVR **drvr_out);

/* ==================================================================== */

#endif /* _NU_USBH_USER_EXT_H_ */

/* ======================  End Of File  =============================== */

