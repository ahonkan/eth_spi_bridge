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
*        nu_usb_iad_ext.h 
*
* COMPONENT
*
*        Nucleus USB Device Software
*
* DESCRIPTION
*
*        This file contains the exported function names and data structures
*        for the NU_USB_IAD class.
*
*
* DATA STRUCTURES
*       None 
*
* FUNCTIONS
*
*       None 
*
* DEPENDENCIES 
*
*       nu_usb_iad_imp.h    IAD's Internal definitions
*
************************************************************************/

/* ==================================================================== */
#ifndef _NU_USB_IAD_EXT_H
#define _NU_USB_IAD_EXT_H

/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* ==================================================================== */

/* ==============  USB Include Files =================================  */
#include    "connectivity/nu_usb_iad_imp.h"

/* ====================  Function Prototypes ========================== */

STATUS NU_USB_IAD_Check_Interface(NU_USB_IAD *cb,
                                  UINT8 intf_num);

STATUS NU_USB_IAD_Get_First_Interface(NU_USB_IAD *cb,
                                      UINT8 *intf_num_out);

STATUS NU_USB_IAD_Get_Last_Interface(NU_USB_IAD *cb,
                                     UINT8 *intf_num_out);

STATUS NU_USB_IAD_Get_Desc(NU_USB_IAD *cb,
                           NU_USB_IAD_DESC **desc_out);

/* ==================================================================== */

#endif /* _NU_USB_IAD_EXT_H    */

/* =======================  End Of File  ============================== */

