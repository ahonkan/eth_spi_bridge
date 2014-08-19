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
 *      nu_usb_user_imp.h 
 * 
 * COMPONENT 
 *      Nucleus USB Software 
 * 
 * DESCRIPTION 
 *
 *      This file describes control block and dispatch table of
 *      NU_USB_USER layer. It also contains declaration for internal
 *      functions.
 *
 * 
 * DATA STRUCTURES 
 *      NU_USB_USER         user control block description.
 * 
 * FUNCTIONS 
 *      None 
 * 
 * DEPENDENCIES 
 *      None
 * 
 *************************************************************************/

/* ===================================================================  */
#ifndef _NU_USB_USER_IMP_H_
#define _NU_USB_USER_IMP_H_
/* ===================================================================  */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================================================================  */

/* =================== Data Structures ===============================  */

/* The Control Block */
struct nu_usb_user
{
    NU_USB cb;
    UINT8 bInterfaceSubclass;
    UINT8 bInterfaceProtocol;
    UINT8 num_entries;
};

/* ===================================================================  */
#endif /* _NU_USB_USER_IMP_H_ */
/* ====================== end of file ================================  */

