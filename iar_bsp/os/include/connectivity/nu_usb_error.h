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
*    FILE NAME 
*
*        nu_usb_error.h
*
*    COMPONENT
*
*        Nucleus USB Software
*
*    DESCRIPTION
*
*        This file contains the values for STATUS thats returned by
*        various APIs of Nucleus USB.
*
*    DATA STRUCTURES
*
*        None
*
*    FUNCTIONS
*
*        None 
*
*    DEPENDENCIES
*
*        None
*
************************************************************************/

/* ==================================================================== */
#ifndef _NU_USB_ERROR_H_
#define _NU_USB_ERROR_H_

/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ==================================================================== */

#define NU_USB_STATUS_BASE              -3800
#define NU_USB_INVLD_HCI                NU_USB_STATUS_BASE-1
#define NU_USB_INVLD_DESC               NU_USB_STATUS_BASE-2
#define NU_USB_NO_ADDR                  NU_USB_STATUS_BASE-3
#define NU_USB_INVLD_ARG                NU_USB_STATUS_BASE-4
#define NU_USB_NO_POWER                 NU_USB_STATUS_BASE-5
#define NU_USB_INVLD_DEV_ID             NU_USB_STATUS_BASE-6
#define NU_USB_NO_HUB_INTF              NU_USB_STATUS_BASE-7
#define NU_USB_DEVICE_NOT_RESPONDING    NU_USB_STATUS_BASE-8
#define NU_USB_INTERNAL_ERROR           NU_USB_STATUS_BASE-9
#define NU_USB_DRVR_ACTV                NU_USB_STATUS_BASE-10
#define NU_USB_NO_BANDWIDTH             NU_USB_STATUS_BASE-11
#define NU_USB_DEV_NOT_CONFIGURED       NU_USB_STATUS_BASE-12
#define NU_USB_SCHEDULE_ERROR           NU_USB_STATUS_BASE-13
#define NU_USB_NOT_SUPPORTED            NU_USB_STATUS_BASE-14
#define NU_USB_CRC_ERR                  NU_USB_STATUS_BASE-15
#define NU_USB_BITSTUFF_ERR             NU_USB_STATUS_BASE-16
#define NU_USB_TOGGLE_ERR               NU_USB_STATUS_BASE-17
#define NU_USB_STALL_ERR                NU_USB_STATUS_BASE-18
#define NU_USB_NO_RESPONSE              NU_USB_STATUS_BASE-19
#define NU_USB_INVLD_PID                NU_USB_STATUS_BASE-20
#define NU_USB_UNEXPECTED_PID           NU_USB_STATUS_BASE-21
#define NU_USB_DATA_OVERRUN             NU_USB_STATUS_BASE-22
#define NU_USB_DATA_UNDERRUN            NU_USB_STATUS_BASE-23
#define NU_USB_BFR_OVERRUN              NU_USB_STATUS_BASE-24
#define NU_USB_BFR_UNDERRUN             NU_USB_STATUS_BASE-25
#define NU_USB_NOT_ACCESSED             NU_USB_STATUS_BASE-26
#define NU_USB_EP_HALTED                NU_USB_STATUS_BASE-27
#define NU_USB_IRP_CANCELLED            NU_USB_STATUS_BASE-28
#define NU_USB_UNKNOWN_ERR              NU_USB_STATUS_BASE-29
#define NU_USB_REJECTED                 NU_USB_STATUS_BASE-30
#define NU_USB_MAX_EXCEEDED             NU_USB_STATUS_BASE-31
#define NU_USB_TRANSFER_FAILED          NU_USB_STATUS_BASE-32
#define NU_USB_HNP_NOT_ENABLED          NU_USB_STATUS_BASE-33
#define NU_USB_HNP_ENABLED_AT_ALT_PORT  NU_USB_STATUS_BASE-34
#define NU_USB_BUSY                     NU_USB_STATUS_BASE-35
#define NU_USB_DEVICE_DISCONNECTED      NU_USB_STATUS_BASE-36
#define NU_USB_NMI_ERROR                NU_USB_STATUS_BASE-37
#define NU_USB_IO_TIMEOUT               NU_USB_STATUS_BASE-38
#define NU_USB_A_SRP_FAILED             NU_USB_STATUS_BASE-39
#define NU_USB_B_SRP_FAILED             NU_USB_STATUS_BASE-40
#define NU_USB_A_HNP_FAILED             NU_USB_STATUS_BASE-41
#define NU_USB_B_HNP_FAILED             NU_USB_STATUS_BASE-42
#define NU_USB_HNP_ENABLE_REJECTED      NU_USB_STATUS_BASE-43
#define NU_USB_UNSUPPORTED_DEVICE       NU_USB_STATUS_BASE-44
#define NU_USB_SESSION_END              NU_USB_STATUS_BASE-45
#define NU_USB_MAX_HUB_EXCEEDED         NU_USB_STATUS_BASE-46
#define NU_USB_REQUEST_ERROR            NU_USB_STATUS_BASE-47
#define NU_USB_INVLD_DELETE             NU_USB_STATUS_BASE-48
#define NU_USB_DEVICE_INVLD_STATE       NU_USB_STATUS_BASE-49
#define NU_USB_INVLD_REQUEST            NU_USB_STATUS_BASE-50
#define NU_USB_INVLD_STATE              NU_USB_STATUS_BASE-51
#define NU_USB_INVLD_HUB                NU_USB_STATUS_BASE-52
#define NU_USB_INVLD_SPEED              NU_USB_STATUS_BASE-53
#define NU_USB_INVLD_DEVICE             NU_USB_STATUS_BASE-54
#define NU_USB_NOT_FOUND                NU_USB_STATUS_BASE-55
#define NU_USB_DEVICE_LTM_DISABLED      NU_USB_STATUS_BASE-56
#define NU_USB_INVLD_MEM_POOL           NU_USB_STATUS_BASE-57
#define NU_USB_NAME_ERROR               NU_USB_STATUS_BASE-58
#define NU_USB_NO_STRM_AVAILABLE        NU_USB_STATUS_BASE-59
#define NU_USB_STRM_BUSY                NU_USB_STATUS_BASE-60
#define NU_USB_DEVICE_MEDIA_NOT_PRESENT NU_USB_STATUS_BASE-61

/* ==================================================================== */

#endif /* _NU_USB_ERROR_H_ */

/* ======================  End Of File  =============================== */

