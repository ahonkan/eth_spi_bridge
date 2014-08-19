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
*        nu_usb_intf_imp.h
*
 * COMPONENT 
 *      Nucleus USB Software 
 * 
 * DESCRIPTION 
 *       This file contains the Control Block and other internal data 
 *       structures and definitions for Interface Component of
 *       Nucleus USB Software.
 *
 * 
 * DATA STRUCTURES 
 *      nu_usb_intf        Interface control block. 
 * 
 * FUNCTIONS 
 *      None 
 * 
 * DEPENDENCIES 
 *      None 
 * 
 *************************************************************************/

/* ==================================================================== */
#ifndef _NU_USB_INTF_IMP_H_
#define _NU_USB_INTF_IMP_H_
/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* ====================  Data Structures ============================== */

struct nu_usb_intf
{
    UINT8 intf_num;
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
	BOOLEAN is_master;
    DATA_ELEMENT pad[2];
	NU_USB_INTF_PW_ATTRIB pw_attrib;
#else
    DATA_ELEMENT pad[3];
#endif
    NU_USB_ALT_SETTG *current;
    NU_USB_ALT_SETTG alt_settg[NU_USB_MAX_ALT_SETTINGS];
    VOID *driver;
    UINT32 load;
    NU_USB_IAD *iad;
    NU_USB_CFG *cfg;
    NU_USB_DEVICE *device;
};

/* ==================================================================== */

#endif /* _NU_USB_INTF_IMP_H_ */

/* ======================  End Of File  =============================== */

