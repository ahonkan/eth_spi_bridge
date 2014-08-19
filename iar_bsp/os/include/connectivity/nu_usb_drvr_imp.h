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
*        nu_usb_drvr_imp.h
*
*    COMPONENT
*
*        Nucleus USB Software
*
*    DESCRIPTION
*
*        This file contains the exported function names and data structures
*        for the common (base) class driver component.
*
*    DATA STRUCTURES
*
*        NU_USB_DRVR     Class driver control block description
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
#ifndef _NU_USB_DRVR_IMP_H
#define _NU_USB_DRVR_IMP_H
/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* =====================  #defines ===================================  */
#define USB_MATCH_VNDR_ID        1
#define USB_MATCH_PRDCT_ID       2
#define USB_MATCH_REL_NUM        4
#define USB_MATCH_CLASS          8
#define USB_MATCH_SUB_CLASS   0x10
#define USB_MATCH_PROTOCOL    0x20

/* ====================  Data Structures =============================  */
struct nu_usb_drvr
{
    NU_USB usb;
    UINT32 match_flag;         /* Matching Criteria            */
    UINT16 idVendor;           /* Vendor ID  */
    UINT16 idProduct;          /* Product ID */
    UINT16 bcdDeviceLow;       /* Low 16bit of Device Release number   */
    UINT16 bcdDeviceHigh;      /* High 16bit of Device Release number  */
    UINT8 bInterfaceClass;     /* Class code of the interface   */
    UINT8 bInterfaceSubClass;  /* SubClass code of the interface */
    UINT8 bInterfaceProtocol;  /* Protocol code of the interface */
    UINT8 score;
    NU_USB_USER *users[NU_USB_MAX_USERS];
    UINT8 num_users;
};

/* ==================================================================== */

#endif /* _NU_USB_DRVR_IMP_H  */

/* ======================  End Of File  =============================== */

