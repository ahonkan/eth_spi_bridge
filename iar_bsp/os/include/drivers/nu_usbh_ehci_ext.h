/************************************************************************
*
*               Copyright 2003 Mentor Graphics Corporation 
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
*************************************************************************

 *************************************************************************
* FILE NAME
*
*        nu_usbh_ehci_ext.h
*
* COMPONENT
*
*       Nucleus USB Host software
*
* DESCRIPTION
*
*       This file contains control block and other structures for
*       NU_USBH_EHCI component(EHCI host controller).
*
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       None
*
* DEPENDENCIES
*
*       nu_usbh_ehci_imp.h              All EHCI definitions
*
************************************************************************/

#ifndef _NU_USBH_EHCI_EXT_H_
#define _NU_USBH_EHCI_EXT_H_

#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* ==================================================================== */

#include "drivers/nu_usbh_ehci_imp.h"

/* ==================================================================== */

/* NU_USBH_EHCI user API. */
STATUS NU_USBH_EHCI_Create (NU_USBH_EHCI * cb,
                            CHAR * name,
                            NU_MEMORY_POOL * pool,
                            VOID *base_address,
                            INT vector);

STATUS NU_USBH_EHCI_Create2 (NU_USBH_EHCI * cb,
                            CHAR * name,
                            NU_MEMORY_POOL *uncacheable_pool,
                            NU_MEMORY_POOL *cacheable_pool,
                            VOID *base_address,
                            INT vector);
/* NU_USBH_EHCI extended API. */

STATUS _NU_USBH_EHCI_Delete (VOID *cb);

STATUS _NU_USBH_EHCI_Create (NU_USBH_EHCI * cb,
                             CHAR * name,
                             NU_MEMORY_POOL * pool,
                             VOID *base_address,
                             INT vector,
                             const VOID *dispatch);

STATUS NU_USBH_EHCI_Initialize (NU_USB_HW * cb,
                                 NU_USB_STACK * stack);
STATUS NU_USBH_EHCI_Uninitialize (NU_USB_HW * cb);

STATUS NU_USBH_EHCI_Enable_Int (NU_USB_HW * cb);

STATUS NU_USBH_EHCI_Disable_Int (NU_USB_HW * cb);

STATUS NU_USBH_EHCI_ISR (NU_USB_HW * cb);

STATUS NU_USBH_EHCI_Submit_IRP (NU_USB_HW * cb,
                                 NU_USB_IRP * irp,
                                 UINT8 function_address,
                                 UINT8 bEndpointAddress);

STATUS NU_USBH_EHCI_Flush_Pipe (NU_USB_HW * cb,
                                 UINT8 function_address,
                                 UINT8 bEndpointAddress);

STATUS NU_USBH_EHCI_Open_Pipe (NU_USB_HW * cb,
                                UINT8 function_address,
                                UINT8 bEndpointAddress,
                                UINT8 bmAttributes,
                                UINT8 speed,
                                UINT16 ep_max_size,
                                UINT32 interval,
                                UINT32 load);

STATUS NU_USBH_EHCI_Close_Pipe (NU_USB_HW * cb,
                                 UINT8 function_address,
                                 UINT8 bEndpointAddress);

STATUS NU_USBH_EHCI_Modify_Pipe (NU_USB_HW * cb,
                                  UINT8 function_address,
                                  UINT8 bEndpointAddress,
                                  UINT8 bmAttributes,
                                  UINT16 ep_max_size,
                                  UINT32 interval,
                                  UINT32 load);

/* ==================================================================== */

#endif /* _NU_USBH_EHCI_EXT_H_ */

/* ======================  End Of File  =============================== */
