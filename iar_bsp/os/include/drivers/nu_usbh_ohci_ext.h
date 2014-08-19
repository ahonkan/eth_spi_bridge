/**************************************************************************
*
*           Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
* FILE NAME
*   nu_usbh_ohci_ext.h
*
* COMPONENT
*   Nucleus USB Host Software.
*
* DESCRIPTION
*   This file contains definitions for external Interfaces exposed by
*   OHCI Host Controller Driver.
*
* DATA STRUCTURES
*   None.
*
* FUNCTIONS
*   None.
*
* DEPENDENCIES
*   nu_usbh_ohci_imp.h         Internal Definitions.
*
**************************************************************************/

/* ===================================================================== */
#ifndef _NU_USBH_OHCI_EXT_H_
#define _NU_USBH_OHCI_EXT_H_
/* ===================================================================== */

/* ===================================================================== */
#include "drivers/nu_usbh_ohci_imp.h"

/* ===================================================================== */

/* NU_USBH_OHCI user API. */
STATUS NU_USBH_OHCI_Create (NU_USBH_OHCI * cb,
                            CHAR * name,
                            NU_MEMORY_POOL * pool,
                            VOID *base_address,
                            INT vector);

STATUS NU_USBH_OHCI_Create2 (NU_USBH_OHCI   *cb,
                            CHAR           *name,
                            NU_MEMORY_POOL *uncacheable_pool,
                            NU_MEMORY_POOL *cacheable_pool,
                            VOID           *base_address,
                            INT             vector);

STATUS _NU_USBH_OHCI_Delete (VOID *cb);

/* NU_USBH_OHCI extender API. */
STATUS _NU_USBH_OHCI_Create (NU_USBH_OHCI * cb,
                             CHAR * name,
                             NU_MEMORY_POOL * pool,
                             VOID *base_address,
                             INT vector,
                             const VOID *dispatch);

/* USBH_User specific API. */
STATUS NU_USBH_OHCI_Initialize (NU_USB_HW * cb,
                                 NU_USB_STACK * stack);

STATUS NU_USBH_OHCI_Uninitialize (NU_USB_HW * cb);

STATUS NU_USBH_OHCI_Enable_Int (NU_USB_HW * cb);

STATUS NU_USBH_OHCI_Disable_Int (NU_USB_HW * cb);

STATUS NU_USBH_OHCI_ISR (NU_USB_HW * cb);

STATUS NU_USBH_OHCI_Submit_IRP (NU_USB_HW * cb,
                                 NU_USB_IRP * irp,
                                 UINT8 function_address,
                                 UINT8 bEndpointAddress);

STATUS NU_USBH_OHCI_Flush_Pipe (NU_USB_HW * cb,
                                 UINT8 function_address,
                                 UINT8 bEndpointAddress);

STATUS NU_USBH_OHCI_Open_Pipe (NU_USB_HW * cb,
                                UINT8 function_address,
                                UINT8 bEndpointAddress,
                                UINT8 bmAttributes,
                                UINT8 speed,
                                UINT16 ep_max_size,
                                UINT32 interval,
                                UINT32 load);

STATUS NU_USBH_OHCI_Close_Pipe (NU_USB_HW * cb,
                                 UINT8 function_address,
                                 UINT8 bEndpointAddress);

STATUS NU_USBH_OHCI_Modify_Pipe (NU_USB_HW * cb,
                                  UINT8 function_address,
                                  UINT8 bEndpointAddress,
                                  UINT8 bmAttributes,
                                  UINT16 ep_max_size,
                                  UINT32 interval,
                                  UINT32 load);

/* ===================================================================== */
#endif /* _NU_USBH_OHCI_EXT_H_ */

/* ======================  End Of File.  =============================== */
