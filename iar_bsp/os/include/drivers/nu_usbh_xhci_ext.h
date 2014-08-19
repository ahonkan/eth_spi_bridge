/*************************************************************************
*
*              Copyright 2011 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
* FILE NAME
*
*       nu_usbh_xhci_ext.h
*
* COMPONENT
*
*       USB XHCI Controller Driver : Nucleus USB Host Software.
*
* DESCRIPTION
*
*       This file contains definitions for external Interfaces exposed by
*       xHCI Host Controller Driver.
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
*       nu_usbh_xhci_imp.h                  Internal Definitions.
*
**************************************************************************/

/* ===================================================================== */
#ifndef _NU_USBH_XHCI_EXT_H_
#define _NU_USBH_XHCI_EXT_H_
/* ===================================================================== */


/* ===================================================================== */
#include "nu_usbh_xhci_imp.h"

/* ===================================================================== */

STATUS NU_USBH_XHCI_Create                 (NU_USBH_XHCI   *xhci,
                                             CHAR           *name,
                                             NU_MEMORY_POOL *uncacheable_pool,
                                             NU_MEMORY_POOL *cacheable_pool,
                                             VOID           *base_address,
                                             INT             vector);

STATUS _NU_USBH_XHCI_Delete                 (VOID           *xhci);

STATUS NU_USBH_XHCI_Open_SS_Pipe           (NU_USB_HW     *cb,
                                            UINT8         function_addr,
                                            UINT8         bEndpointAddress,
                                            UINT8         bmEndpointAttributes,
                                            UINT8         speed,
                                            UINT16        wMaxPacketSize,
                                            UINT32        interval,
                                            UINT32        load,
                                            UINT8         bMaxBurst,
                                            UINT8         SSEndpCompAttrib,
                                            UINT16        bytes_per_interval);

STATUS NU_USBH_XHCI_Obtain_Bandwidth       (NU_USBH_HW    *cb,
                                            UINT8         function_address);

STATUS NU_USBH_XHCI_Update_device          (NU_USBH_HW *cb,
                                            NU_USB_DEVICE *usb_device,
                                            UINT8 packet_size,
                                            UINT32 sel,
                                            BOOLEAN is_hub,
                                            UINT8   tt_time,
                                            UINT8   num_ports );

STATUS NU_USBH_XHCI_Flush_Pipe             (NU_USB_HW *cb,
                                            UINT8 function_address,
                                            UINT8 bEndpointAddress);

STATUS NU_USBH_XHCI_Initialize_Device      (NU_USBH_HW *cb);

STATUS NU_USBH_XHCI_Uninitialize_Device    (NU_USBH_HW *cb,
                                            UINT8      func_addr);

STATUS NU_USBH_XHCI_Submit_IRP             (NU_USB_HW  *cb,
                                            NU_USB_IRP *irp,
                                            UINT8      function_address,
                                            UINT8      bEndpointAddress);

STATUS NU_USBH_XHCI_Submit_Stream          (NU_USB_HW  *cb,
                                            NU_USB_STREAM *stream,
                                            UINT8      function_address,
                                            UINT8      bEndpointAddress);

STATUS NU_USBH_XHCI_Open_Pipe              (NU_USB_HW    *cb,
                                            UINT8        function_addr,
                                            UINT8        bEndpointAddress,
                                            UINT8        bmAttributes,
                                            UINT8        speed,
                                            UINT16       ep_max_size,
                                            UINT32       interval,
                                            UINT32       load);

STATUS NU_USBH_XHCI_Close_Pipe             (NU_USB_HW *cb,
                                            UINT8     function_address,
                                            UINT8     bEndpointAddress);

STATUS NU_USBH_XHCI_Initialize             (NU_USB_HW    *cb,
                                            NU_USB_STACK *stack);

STATUS NU_USBH_XHCI_Uninitialize           (NU_USB_HW * cb);

STATUS NU_USBH_XHCI_ISR                    (NU_USB_HW *cb);

STATUS NU_USBH_XHCI_Enable_Interrupts      (NU_USB_HW *cb);

STATUS NU_USBH_XHCI_Disable_Interrupts     (NU_USB_HW *cb);

STATUS NU_USBH_XHCI_Modify_Pipe            (NU_USB_HW *cb,
                                            UINT8 function_address,
                                            UINT8 bEndpointAddress,
                                            UINT8 bmAttributes,
                                            UINT16 ep_max_size,
                                            UINT32 interval,
                                            UINT32 load);

STATUS NU_USBH_XHCI_Unstall_Pipe           (NU_USBH_HW *cb,
                                            UINT8 bEndpointAddress,
                                            UINT8 function_address );

STATUS NU_USBH_XHCI_Disable_Pipe           (NU_USBH_HW *cb,
                                            UINT8 bEndpointAddress,
                                            UINT8 function_address );

STATUS NU_USBH_XHCI_Reset_Bandwidth         (NU_USBH_HW *cb,
                                             UINT8 function_address);
#endif  /* _NU_USBH_XHCI_EXT_H_ */

/* ======================  End Of File.  =============================== */
