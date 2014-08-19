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
*        nu_usb_stack_cfg.h
*
* COMPONENT
*
*        Nucleus USB Software
*
* DESCRIPTION
*       This file contains the configurable parameters for common stack
*       component.
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
*       None
*
************************************************************************/
/* ==================================================================== */

#ifndef _NU_USB_STACK_CFG_H
#define _NU_USB_STACK_CFG_H

/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* ===================================================================== */

/* Definitions for USB specifications versions.
 * USB_VERSION_1_1    USB specifications version 1.1
 * USB_VERSION_2_0    USB specifications version 2.0
 * USB_VERSION_3_0    USB specifications version 3.0
 */
#define USB_VERSION_1_1        0x0110  /* USB specs version 1.1. */
#define USB_VERSION_2_0        0x0200  /* USB specs version 2.0. */
#define USB_VERSION_2_1        0x0210  /* USB specs version 2.1. */
#define USB_VERSION_3_0        0x0300  /* USB specs version 3.0. */

/* ==================================================================== */
/* If host stack is enabled then copy value of USB host stack related 
 * defines from generated configuration file, otherwise set all these
 * defines equal to zero.
 */
#ifdef CFG_NU_OS_CONN_USB_HOST_STACK_ENABLE
#define NU_USBH_MAX_HW                      CFG_NU_OS_CONN_USB_HOST_STACK_NCONTROLLER
#define NU_USBH_MAX_IRQ                     CFG_NU_OS_CONN_USB_HOST_STACK_MAX_IRQS
#define NU_USBH_MAX_USERS                   CFG_NU_OS_CONN_USB_HOST_STACK_MAX_USERS
#define NU_USBH_MAX_INTERFACES              CFG_NU_OS_CONN_USB_HOST_STACK_MAX_INTERFACES
#define NU_USBH_MAX_IADS                    CFG_NU_OS_CONN_USB_HOST_STACK_MAX_IADS
#define NU_USBH_MAX_CONFIGURATIONS          CFG_NU_OS_CONN_USB_HOST_STACK_MAX_CONFIGURATIONS
#define NU_USBH_MAX_ENDPOINTS               CFG_NU_OS_CONN_USB_HOST_STACK_MAX_ENDPOINTS
#define NU_USBH_MAX_ALT_SETTINGS            CFG_NU_OS_CONN_USB_HOST_STACK_MAX_ALT_SETTINGS
#define NU_USBH_MAX_STRINGS                 CFG_NU_OS_CONN_USB_HOST_STACK_MAX_STRINGS
#define NU_USBH_MAX_UNICODE_STRING_LEN      CFG_NU_OS_CONN_USB_HOST_STACK_MAX_UNICODE_STRING_LEN
#define NU_USBH_MAX_CLASS_DRIVERS           CFG_NU_OS_CONN_USB_HOST_STACK_MAX_CLASS_DRIVERS
#define NU_USBH_USER_MAX_TASKS              CFG_NU_OS_CONN_USB_HOST_STACK_USER_MAX_TASK

#else /* if host stack is not enabled */

#define NU_USBH_MAX_HW                      0
#define NU_USBH_MAX_IRQ                     0
#define NU_USBH_MAX_USERS                   0
#define NU_USBH_MAX_INTERFACES              0
#define NU_USBH_MAX_IADS                    0
#define NU_USBH_MAX_CONFIGURATIONS          0
#define NU_USBH_MAX_ENDPOINTS               0
#define NU_USBH_MAX_ALT_SETTINGS            0
#define NU_USBH_MAX_STRINGS                 0
#define NU_USBH_MAX_UNICODE_STRING_LEN      0
#define NU_USBH_MAX_CLASS_DRIVERS           0
#define NU_USBH_USER_MAX_TASKS              0
#endif /* #ifdef CFG_NU_OS_CONN_USB_HOST_STACK_ENABLE */

/* If function stack is enabled then copy value of USB function stack related 
 * defines from generated configuration file, otherwise set all these
 * defines equal to zero.
 */
#ifdef CFG_NU_OS_CONN_USB_FUNC_STACK_ENABLE
#define NU_USBF_MAX_HW                      CFG_NU_OS_CONN_USB_FUNC_STACK_NCONTROLLER
#define NU_USBF_MAX_IRQ                     CFG_NU_OS_CONN_USB_FUNC_STACK_MAX_IRQS
#define NU_USBF_MAX_USERS                   CFG_NU_OS_CONN_USB_FUNC_STACK_MAX_USERS
#define NU_USBF_MAX_INTERFACES              CFG_NU_OS_CONN_USB_FUNC_STACK_MAX_INTERFACES
#define NU_USBF_MAX_IADS                    CFG_NU_OS_CONN_USB_FUNC_STACK_MAX_IADS
#define NU_USBF_MAX_CONFIGURATIONS          CFG_NU_OS_CONN_USB_FUNC_STACK_MAX_CONFIGURATIONS
#define NU_USBF_MAX_ENDPOINTS               CFG_NU_OS_CONN_USB_FUNC_STACK_MAX_ENDPOINTS
#define NU_USBF_MAX_ALT_SETTINGS            CFG_NU_OS_CONN_USB_FUNC_STACK_MAX_ALT_SETTINGS
#define NU_USBF_MAX_STRINGS                 CFG_NU_OS_CONN_USB_FUNC_STACK_MAX_STRINGS
#define NU_USBF_MAX_UNICODE_STRING_LEN      CFG_NU_OS_CONN_USB_FUNC_STACK_MAX_UNICODE_STRING_LEN
#define NU_USBF_MAX_CLASS_DRIVERS           CFG_NU_OS_CONN_USB_FUNC_STACK_MAX_CLASS_DRIVERS

#else /* if function stack is not enabled */

#define NU_USBF_MAX_HW                      0
#define NU_USBF_MAX_IRQ                     0
#define NU_USBF_MAX_USERS                   0
#define NU_USBF_MAX_INTERFACES              0
#define NU_USBF_MAX_IADS                    0
#define NU_USBF_MAX_CONFIGURATIONS          0
#define NU_USBF_MAX_ENDPOINTS               0
#define NU_USBF_MAX_ALT_SETTINGS            0
#define NU_USBF_MAX_STRINGS                 0
#define NU_USBF_MAX_UNICODE_STRING_LEN      0
#define NU_USBF_MAX_CLASS_DRIVERS           0
#endif /* #ifdef CFG_NU_OS_CONN_USB_FUNC_STACK_ENABLE */

/* Compare value of analogous defines present in  host and function stack.
 * Use the value which is larger.
 */ 
#if NU_USBF_MAX_IRQ > NU_USBH_MAX_IRQ
#define NU_USB_MAX_IRQ                      NU_USBF_MAX_IRQ
#else
#define NU_USB_MAX_IRQ                      NU_USBH_MAX_IRQ
#endif

#if NU_USBF_MAX_USERS > NU_USBH_MAX_USERS
#define NU_USB_MAX_USERS                    NU_USBF_MAX_USERS
#else
#define NU_USB_MAX_USERS                    NU_USBH_MAX_USERS
#endif

#if NU_USBF_MAX_INTERFACES > NU_USBH_MAX_INTERFACES
#define NU_USB_MAX_INTERFACES               NU_USBF_MAX_INTERFACES
#else
#define NU_USB_MAX_INTERFACES               NU_USBH_MAX_INTERFACES
#endif

#if NU_USBF_MAX_IADS > NU_USBH_MAX_IADS
#define NU_USB_MAX_IADS                     NU_USBF_MAX_IADS
#else
#define NU_USB_MAX_IADS                     NU_USBH_MAX_IADS
#endif

#if NU_USBF_MAX_CONFIGURATIONS > NU_USBH_MAX_CONFIGURATIONS
#define NU_USB_MAX_CONFIGURATIONS           NU_USBF_MAX_CONFIGURATIONS
#else
#define NU_USB_MAX_CONFIGURATIONS           NU_USBH_MAX_CONFIGURATIONS
#endif

#if NU_USBF_MAX_ENDPOINTS > NU_USBH_MAX_ENDPOINTS
#define NU_USB_MAX_ENDPOINTS                NU_USBF_MAX_ENDPOINTS
#else
#define NU_USB_MAX_ENDPOINTS                NU_USBH_MAX_ENDPOINTS
#endif

#if NU_USBF_MAX_ALT_SETTINGS > NU_USBH_MAX_ALT_SETTINGS
#define NU_USB_MAX_ALT_SETTINGS             NU_USBF_MAX_ALT_SETTINGS
#else
#define NU_USB_MAX_ALT_SETTINGS             NU_USBH_MAX_ALT_SETTINGS
#endif

#if NU_USBF_MAX_STRINGS > NU_USBH_MAX_STRINGS
#define NU_USB_MAX_STRINGS                  NU_USBF_MAX_STRINGS
#else
#define NU_USB_MAX_STRINGS                  NU_USBH_MAX_STRINGS
#endif

#if NU_USBF_MAX_UNICODE_STRING_LEN > NU_USBH_MAX_UNICODE_STRING_LEN
#define NU_USB_MAX_STRING_LEN               NU_USBF_MAX_UNICODE_STRING_LEN
#else
#define NU_USB_MAX_STRING_LEN               NU_USBH_MAX_UNICODE_STRING_LEN
#endif

#if NU_USBF_MAX_CLASS_DRIVERS > NU_USBH_MAX_CLASS_DRIVERS
#define NU_USB_MAX_CLASS_DRIVERS            NU_USBF_MAX_CLASS_DRIVERS
#else
#define NU_USB_MAX_CLASS_DRIVERS            NU_USBH_MAX_CLASS_DRIVERS
#endif

/* ==================================================================== */

#endif /* _NU_USB_STACK_CFG_H  */

/* ======================  End Of File  =============================== */

