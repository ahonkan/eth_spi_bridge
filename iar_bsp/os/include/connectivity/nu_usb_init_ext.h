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
*        nu_usb_init_ext.h
*
* COMPONENT
*
*        Nucleus USB Initialization
*
* DESCRIPTION
*
*       This file contains the exported function names and data structures
*       for the USB Memory Pools Initialization.
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
*       nu_usb.h                            All USB definitions.
*
************************************************************************/

/* ==================================================================== */

#ifndef _NU_USB_INIT_EXT_H
#define _NU_USB_INIT_EXT_H

/* ==================================================================== */

#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================================================================  */
#include "connectivity/nu_usb.h"

/*********************************/
/* FUNCTION PROTOTYPES           */
/*********************************/
VOID    NU_Printf_USB_Msg(CHAR* string);
STATUS  nu_os_conn_usb_com_stack_init(INT startstop);

/* ==================================================================== */

#endif /* _NU_USB_INIT_EXT_H    */

/* =======================  End Of File  ============================== */

