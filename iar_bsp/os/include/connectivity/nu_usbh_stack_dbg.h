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
 *      nu_usbh_stack_dbg.h 
 * 
 * COMPONENT 
 *      Nucleus USB Host Software 
 * 
 * DESCRIPTION 
 *      This file defines debug interface exported by Nucleus USB host
 *      stack.
 * 
 * DATA STRUCTURES 
 *      None
 * 
 * FUNCTIONS 
 *      None 
 * 
 * DEPENDENCIES 
 *      None
 * 
 *************************************************************************/

/* ==================================================================== */
#ifndef _NU_USBH_STACK_DBG_H_
#define _NU_USBH_STACK_DBG_H_
/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* ====================  Function Prototypes ========================== */

/* Facts interface for stacks. */
UNSIGNED NU_USBH_Get_Root_Hubs_Count (NU_USBH_STACK * stack);

UNSIGNED NU_USBH_Get_Root_Hubs (NU_USBH_STACK * stack,
                                NU_USB_DEVICE ** root_hub_list,
                                UNSIGNED max_root_hubs);

UNSIGNED NU_USBH_Get_Device_Count (NU_USBH_STACK * stack,
                                   NU_USB_DEVICE * root_hub);

UNSIGNED NU_USBH_Get_Devices (NU_USBH_STACK * stack,
                              NU_USB_DEVICE * root_hub,
                              NU_USB_DEVICE ** device_list,
                              UNSIGNED max_devices);

/* ==================================================================== */
#endif /* _NU_USBH_STACK_DBG_H_ */

/* ======================  End Of File  =============================== */

