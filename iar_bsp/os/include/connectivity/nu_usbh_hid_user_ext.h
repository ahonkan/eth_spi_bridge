/**************************************************************************
*
*               Copyright 2004 Mentor Graphics Corporation
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
*       nu_usbh_hid_user_ext.h  
*         
* COMPONENT
*
*       Nucleus USB Host HID Base Class Driver.
*
* DESCRIPTION
*
*       This file contains definitions of external interfaces exported by
*       Nucleus USB Host HID User Layer.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       nu_usbh_hid_user_imp.h              HID User Layer Internal
*                                           Definitions.
*
**************************************************************************/

#ifndef _NU_USBH_HID_USER_EXT_H_
#define _NU_USBH_HID_USER_EXT_H_

#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

#include "connectivity/nu_usbh_hid_user_imp.h"


/* =================== Definitions ===============================  */

#define _NU_USBH_HID_USER_Connect       _NU_USBH_USER_Connect

#define _NU_USBH_HID_USER_Disconnect    _NU_USBH_USER_Disconnect

/* =================== Data Structures ===============================  */


/* dispatch table. */

typedef struct usbh_hid_user_dispatch
{
    NU_USBH_USER_DISPATCH dispatch;


    STATUS (*Get_Usages) (NU_USBH_HID_USER * cb,
            NU_USBH_HID * hid_drvr,
            NU_USBH_HID_USAGE *usage_out,
            UINT8 num_usages);

    STATUS (*Notify_Report) (NU_USBH_HID_USER * user,
            NU_USBH_HID * hid_drvr,
            VOID *handle,
            UINT8 *report,
            UINT16 report_length);
}
NU_USBH_HID_USER_DISPATCH;


/* ====================  Function Prototypes ========================== */

/* NU_USBH_HID_USER user API. */

STATUS _NU_USBH_HID_USER_Create (   NU_USBH_HID_USER    *cb,
                                    CHAR                *name,
                                    NU_MEMORY_POOL      *pool,
                                    UINT8               num_usages,
                                    const VOID          *dispatch);

/* NU_USBH_HID_USER callback API. */

STATUS NU_USBH_HID_USER_Get_Num_Usages (NU_USBH_HID_USER *cb,
                                        NU_USBH_HID      *hid_driver,
                                        UINT8            *num_usages_out);

STATUS NU_USBH_HID_USER_Get_Usages (NU_USBH_HID_USER    *cb,
                                    NU_USBH_HID         *hid_driver,
                                    NU_USBH_HID_USAGE   *usage_out,
                                    UINT8               num_usages);

STATUS NU_USBH_HID_USER_Notify_Report(  NU_USBH_HID_USER    *cb,
                                        NU_USBH_HID         *hid_driver,
                                        VOID                *handle,
                                        UINT8               *report_data,
                                        UINT16              report_len);

STATUS _NU_USBH_HID_USER_Delete (VOID *cb);


/* ==================================================================== */

#endif /* _NU_USBH_HID_USER_EXT_H_ */

/* ======================  End Of File  =============================== */

