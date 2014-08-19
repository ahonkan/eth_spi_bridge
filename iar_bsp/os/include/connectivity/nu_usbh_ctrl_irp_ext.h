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

*************************************************************************
*
* FILE NAME 
*
*        nu_usbh_ctrl_irp_ext.h
*
* COMPONENT
*        Nucleus USB Host Software
*
* DESCRIPTION
*
*        This file contains the exported function names and data structures
*        for the NU_USB_CTRL_IRP class.
*
*
* DATA STRUCTURES
*       None 
*
* FUNCTIONS
*       None 
*
* DEPENDENCIES 
*       nu_usbh_ctrl_irp_imp.h          Control IRP Internal Definitions.
*
************************************************************************/

/* ==================================================================== */
#ifndef _NU_USBH_CTRL_IRP_EXT_H
#define _NU_USBH_CTRL_IRP_EXT_H
/* ==================================================================== */

/* ==============  USB Include Files =================================  */
#include    "connectivity/nu_usbh_ctrl_irp_imp.h"

/* ====================  Function Prototypes ========================== */

STATUS NU_USBH_CTRL_IRP_Create (NU_USBH_CTRL_IRP * cb,
                                UINT8 *data,
                                NU_USB_IRP_CALLBACK callback,
                                VOID *context,
                                UINT8 bmRequest_Type,
                                UINT8 bRequest,
                                UINT16 wValue,
                                UINT16 wIndex,
                                UINT16 wLength);

STATUS NU_USBH_CTRL_IRP_Get_bmRequestType (NU_USBH_CTRL_IRP * cb,
                                           UINT8 *bmRequestType_out);
STATUS NU_USBH_CTRL_IRP_Set_bmRequestType (NU_USBH_CTRL_IRP * cb,
                                           UINT8 bmRequestType);

STATUS NU_USBH_CTRL_IRP_Get_bRequest (NU_USBH_CTRL_IRP * cb,
                                      UINT8 *bRequest_out);
STATUS NU_USBH_CTRL_IRP_Set_bRequest (NU_USBH_CTRL_IRP * cb,
                                      UINT8 bRequest);

STATUS NU_USBH_CTRL_IRP_Get_wIndex (NU_USBH_CTRL_IRP * cb,
                                    UINT16 *wIndex_out);
STATUS NU_USBH_CTRL_IRP_Set_wIndex (NU_USBH_CTRL_IRP * cb,
                                    UINT16 wIndex);

STATUS NU_USBH_CTRL_IRP_Get_wValue (NU_USBH_CTRL_IRP * cb,
                                    UINT16 *wValue_out);
STATUS NU_USBH_CTRL_IRP_Set_wValue (NU_USBH_CTRL_IRP * cb,
                                    UINT16 wValue);

STATUS NU_USBH_CTRL_IRP_Get_wLength (NU_USBH_CTRL_IRP * cb,
                                     UINT16 *wLength_out);
STATUS NU_USBH_CTRL_IRP_Set_wLength (NU_USBH_CTRL_IRP * cb,
                                     UINT16 wLength);

STATUS NU_USBH_CTRL_IRP_Get_Direction (NU_USBH_CTRL_IRP * cb,
                                       UINT8 *direction_out);

STATUS NU_USBH_CTRL_IRP_Get_Setup_Pkt (NU_USBH_CTRL_IRP * cb,
                                       NU_USB_SETUP_PKT ** pkt_out);

/* ==================================================================== */
#endif /* _NU_USBH_CTRL_IRP_EXT_H    */

/* =======================  End Of File  ============================== */

