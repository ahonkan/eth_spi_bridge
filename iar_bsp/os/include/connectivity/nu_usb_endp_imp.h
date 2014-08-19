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
*        nu_usb_endp_imp.h
* 
*    COMPONENT 
*
*        Nucleus USB Software 
* 
*    DESCRIPTION
*
*        This file contains the Control Block and other internal data 
*        structures and definitions for Endpoint Component of
*        Nucleus USB Software.
*
*    DATA STRUCTURES
*
*        nu_usb_endp         Endpoint control block.
* 
*    FUNCTIONS
*
*        None 
* 
*    DEPENDENCIES 
*
*        None 
* 
*************************************************************************/

/* ==================================================================== */
#ifndef _NU_USB_ENDP_IMP_H_
#define _NU_USB_ENDP_IMP_H_

/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* ============================= Defines ============================ */
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

#define SSEPDESC_MAXSTREAM_MASK     0x1F
#define SSEPDESC_MULT_MASK          0x03

#endif
/* ====================  Data Structures ============================== */

struct nu_usb_endp
{
    NU_USB_ENDP_DESC *desc;
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
    NU_USB_SSEPCOMPANION_DESC   *epcompanion_desc;
#endif
    UINT8            *class_specific;
    UINT32           length;
    NU_USB_PIPE      pipe;        /* Pipe associated with this endpoint */
    NU_USB_DEVICE    *device;
    NU_USB_ALT_SETTG *alt_setting;
    UINT32           load;
};

/* ==================================================================== */

#endif /* _NU_USB_ENDP_IMP_H_ */

/* ======================  End Of File  =============================== */

