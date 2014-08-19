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

 *      nu_usbh_ext.h 
 * 
 * COMPONENT 
 *      Nucleus USB Host Software 
 * 
 * DESCRIPTION 
 *      This file contains the external Interfaces exported by Nucleus USB
 *      Host Singleton.
 *
 * 
 * DATA STRUCTURES 
 *      None 
 * 
 * FUNCTIONS 
 *      None 
 * 
 * DEPENDENCIES 
 *      nu_usbh_imp.h       USB Host Singleton Internal Definitions.
 * 
 *************************************************************************/

/* ==================================================================== */
#ifndef _NU_USBH_EXT_H_
#define _NU_USBH_EXT_H_
/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* ===================== Include Files ================================ */
#include "connectivity/nu_usbh_imp.h"

/* ====================  Function Prototypes ========================== */

STATUS NU_USBH_Create (NU_USBH * singleton_cb);
STATUS NU_USBH_Delete (void);

/* ==================================================================== */

#endif /* _NU_USBH_EXT_H_ */

/* ======================  End Of File  =============================== */

