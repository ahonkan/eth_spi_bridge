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
 *      nu_usb_user_ext.h 
 * 
 * COMPONENT 
 *      Nucleus USB Software 
 * 
 * DESCRIPTION 
 *
 *        This file contains the exported function names and data structures
 *        for the User Layer.
 * 
 * DATA STRUCTURES 
 *      NU_USB_USER_DISPATCH   NU_USB_USER Dispatch table description.
 * 
 * FUNCTIONS 
 *      None
 *
 * DEPENDENCIES 
 *      nu_usb_user_imp.h       User Layer Internal Definitions.
 * 
 *************************************************************************/
/* ==================================================================== */
#ifndef _NU_USB_USER_EXT_H_
#define _NU_USB_USER_EXT_H_
/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================== Include Files ================================ */
#include "connectivity/nu_usb_user_imp.h"

/* ====================  Function Prototypes ========================== */

/* NU_USB_USER Dispatch Table */
typedef struct usb_user_dispatch
{
    NU_USB_DISPATCH usb_dispatch;
    STATUS (*Connect) (NU_USB_USER * user,
                       NU_USB_DRVR * class_driver,
                       VOID *handle);
    STATUS (*Disconnect) (NU_USB_USER * user,
                          NU_USB_DRVR * class_driver,
                          VOID *handle);

}
NU_USB_USER_DISPATCH;

/* NU_USB_USER user API. */

STATUS NU_USB_USER_Get_Subclass (NU_USB_USER * cb,
                                 UINT8 *subclass_out);
STATUS NU_USB_USER_Get_Protocol (NU_USB_USER * cb,
                                 UINT8 *protocol_out);

/* NU_USB_USER extended API. */
STATUS _NU_USB_USER_Create (NU_USB_USER * cb,
                            NU_USB_SUBSYS * subsys,
                            CHAR * name,
                            UINT8 bInterfaceSubclass,
                            UINT8 bInterfaceProtocol,
                            const VOID *dispatch);

/* NU_USB_USER callback API. */
STATUS NU_USB_USER_Connect (NU_USB_USER * cb,
                            NU_USB_DRVR * class_driver,
                            VOID *handle);
STATUS NU_USB_USER_Disconnect (NU_USB_USER * cb,
                               NU_USB_DRVR * class_driver,
                               VOID *handle);

STATUS _NU_USB_USER_Delete (VOID *cb);

/* ==================================================================== */

#endif /* _NU_USB_USER_EXT_H_ */

/* ======================  End Of File  =============================== */

