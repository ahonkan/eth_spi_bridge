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
*        nu_usb_iso_irp_imp.h
*
* COMPONENT
*
*        Nucleus USB Software
*
* DESCRIPTION 
*       This file contains the Control Block and other internal data 
*       structures and definitions for ISO IRP Component of
*       Nucleus USB Software.
*
*
* DATA STRUCTURES
*       _nu_usb_iso_irp         ISO IRP control block
*
* FUNCTIONS
*       None 
*
* DEPENDENCIES 
*       None
*
************************************************************************/

/* ==================================================================== */
#ifndef _NU_USB_ISO_IRP_IMP_H
#define _NU_USB_ISO_IRP_IMP_H

/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* ==================================================================== */

/* ====================  Data Types ==================================  */

struct _nu_usb_iso_irp
{
    NU_USB_IRP      parent_irp;
    UINT16          num_transactions;
    UINT16          *lengths;
    UINT16          *actual_lengths;
    UINT8           **buffer_array;
    UINT16          actual_num_transactions;
};

/* ==================================================================== */

#endif /* _NU_USB_ISO_IRP_IMP_H    */

/* =======================  End Of File  ============================== */

