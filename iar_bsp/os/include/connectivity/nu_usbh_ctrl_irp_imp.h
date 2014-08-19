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
*        nu_usbh_ctrl_irp_imp.h
*
* COMPONENT
*
*        Nucleus USB Host Software
*
* DESCRIPTION
*
*        This file contains the exported function names and data structures
*        for the NU_USB_CTRL_IRP class.
*
*
* DATA STRUCTURES
*         _nu_usbh_ctrl_irp     Control IRP control block
*
* FUNCTIONS
*       None 
*
* DEPENDENCIES 
*       None
*
************************************************************************/

/* ==================================================================== */

#ifndef _NU_USBH_CTRL_IRP_IMP_H
#define _NU_USBH_CTRL_IRP_IMP_H

/* ==================================================================== */

/* ====================  Data Types ==================================  */

typedef struct _nu_usbh_ctrl_irp NU_USBH_CTRL_IRP;

struct _nu_usbh_ctrl_irp
{
    NU_USB_IRP usb_irp;
    NU_USB_SETUP_PKT setup_data;            /* for control transfers */
};

/* ==================================================================== */

#endif /* _NU_USBH_CTRL_IRP_IMP_H    */

/* =======================  End Of File  ============================== */

