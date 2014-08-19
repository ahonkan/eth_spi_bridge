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
*    FILE NAME
*
*        nu_usb_ext.h
* 
*    COMPONENT 
*
*        Nucleus USB Software 
* 
*    DESCRIPTION 
*
*        This file contains the exported function  for the usb component.
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
*        nu_usb_imp.h            USB Component's Internal definitions
* 
*************************************************************************/

/* ==================================================================== */
#ifndef _NU_USB_EXT_H_
#define _NU_USB_EXT_H_

/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================== Include Files ================================ */
#include "connectivity/nu_usb_imp.h"

/* ====================  Function Prototypes ========================== */

STATUS NU_USB_Delete (VOID *cb);

STATUS NU_USB_Get_Name (NU_USB * cb,
                        CHAR * name_out);

STATUS NU_USB_Get_Object_Id (NU_USB * cb,
                             UINT32 *id_out);

/* USB extendable  functions */
STATUS _NU_USB_Create (NU_USB * cb,
                       NU_USB_SUBSYS * subsys,
                       CHAR * name,
                       const VOID *dispatch);

STATUS _NU_USB_Delete (VOID *cb);

STATUS _NU_USB_Get_Name (NU_USB * cb,
                         CHAR * name_out);

STATUS _NU_USB_Get_Object_Id (NU_USB * cb,
                              UINT32 *id_out);
/* ==================================================================== */

#endif /* _NU_USB_EXT_H_ */

/* ======================  End Of File  =============================== */
