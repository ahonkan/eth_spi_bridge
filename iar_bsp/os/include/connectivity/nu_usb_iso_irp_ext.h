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
*        nu_usb_iso_irp_ext.h 
*
* COMPONENT
*
*        Nucleus USB Device Software
*
* DESCRIPTION
*
*        This file contains the exported function names and data structures
*        for the NU_USB_ISO_IRP class.
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
*       nu_usb_iso_irp_imp.h    IRP's Internal definitions
*
************************************************************************/

/* ==================================================================== */
#ifndef _NU_USB_ISO_IRP_EXT_H
#define _NU_USB_ISO_IRP_EXT_H

/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* ==================================================================== */

/* ==============  USB Include Files =================================  */
#include    "connectivity/nu_usb_iso_irp_imp.h"

/* ====================  Function Prototypes ========================== */

/* Create and Delete    */

STATUS  NU_USB_ISO_IRP_Create(NU_USB_ISO_IRP *cb,
                              UINT16 num_transactions,
                              UINT8 **buffer_array,
                              UINT16 *lengths,
                              UINT16 *actual_lengths,
                              NU_USB_IRP_CALLBACK callback,
                              VOID *context);

STATUS  NU_USB_ISO_IRP_Delete(NU_USB_ISO_IRP *cb);

STATUS  NU_USB_ISO_IRP_Get_Num_Transactions(NU_USB_ISO_IRP *cb,
                                            UINT16 *num_transactions_out);

STATUS  NU_USB_ISO_IRP_Set_Num_Transactions(NU_USB_ISO_IRP *cb,
                                            UINT16 num_transactions);

STATUS  NU_USB_ISO_IRP_Get_Buffer_Array(NU_USB_ISO_IRP *cb,
                                        UINT8 ***buffer_array_out);

STATUS  NU_USB_ISO_IRP_Set_Buffer_Array(NU_USB_ISO_IRP *cb,
                                        UINT8 **buffer_array);

STATUS  NU_USB_ISO_IRP_Get_Lengths(NU_USB_ISO_IRP *cb, UINT16 **lengths_out);

STATUS  NU_USB_ISO_IRP_Set_Lengths(NU_USB_ISO_IRP *cb, UINT16 *lengths);

STATUS  NU_USB_ISO_IRP_Get_Actual_Num_Transactions(NU_USB_ISO_IRP *cb,
                                                   UINT16 *actual_num_transactions_out);

STATUS  NU_USB_ISO_IRP_Set_Actual_Num_Transactions(NU_USB_ISO_IRP *cb,
                                                   UINT16 actual_num_transactions);

STATUS  NU_USB_ISO_IRP_Get_Actual_Lengths(NU_USB_ISO_IRP *cb,
                                          UINT16 **lengths_in);

STATUS  NU_USB_ISO_IRP_Set_Actual_Lengths(NU_USB_ISO_IRP *cb,
                                          UINT16 *actual_lengths);

/* ==================================================================== */

#endif /* _NU_USB_ISO_IRP_EXT_H    */

/* =======================  End Of File  ============================== */

