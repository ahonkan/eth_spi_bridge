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
************************************************************************

***************************************************************************
*
* FILE NAME 
*
*       nu_usbf_drvr_ext.h 
*
*
* COMPONENT
*
*       Stack component / Nucleus USB Device Software.
*
* DESCRIPTION
*
*       This file contains the exported function prototypes for the
*       device (base) class driver.
*
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
*       nu_usbf_stack_ext.h          USB Function stack declarations
*       nu_usb_drvr_imp.h            USB Class driver control block
*
**************************************************************************/

#ifndef _NU_USBF_DRVR_EXT_H
#define _NU_USBF_DRVR_EXT_H

/* ==============  USB Include Files ==================================  */
#include    "connectivity/nu_usbf_stack_ext.h"
#include    "connectivity/nu_usbf_drvr_imp.h"

/* ====================  Function Prototypes =========================== */

STATUS _NU_USBF_DRVR_Create (NU_USBF_DRVR *cb,
                             CHAR         *name,
                             UINT32        match_flag,
                             UINT16        idVendor,
                             UINT16        idProduct,
                             UINT16        bcdDeviceLow,
                             UINT16        bcdDeviceHigh,
                             UINT8         bInterfaceClass,
                             UINT8         bInterfaceSubClass,
                             UINT8         bInterfaceProtocol,
                             const VOID   *dispatch);

STATUS NU_USBF_DRVR_Set_Intf (NU_USB_DRVR      *cb,
                              NU_USB_STACK     *stack,
                              NU_USB_DEVICE    *device,
                              NU_USB_INTF      *intf,
                              NU_USB_ALT_SETTG *alt_settg);

STATUS NU_USBF_DRVR_New_Setup (NU_USB_DRVR      *cb,
                               NU_USB_STACK     *stack,
                               NU_USB_DEVICE    *device,
                               NU_USB_SETUP_PKT *setup);

STATUS NU_USBF_DRVR_New_Transfer (NU_USB_DRVR   *cb,
                                  NU_USB_STACK  *stack,
                                  NU_USB_DEVICE *device,
                                  NU_USB_PIPE   *pipe);

STATUS NU_USBF_DRVR_Notify (NU_USB_DRVR   *cb,
                            NU_USB_STACK  *stack,
                            NU_USB_DEVICE *device,
                            UINT32         event);

STATUS _NU_USBF_DRVR_Delete (VOID *cb);

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

STATUS NU_USBF_DRVR_Get_Status(NU_USB_DRVR *cb,
                                UINT16       *status_out);

#endif
/* ===================================================================== */

#endif      /* _NU_USBF_DRVR_EXT_H       */

/* ======================  End Of File  ================================ */

