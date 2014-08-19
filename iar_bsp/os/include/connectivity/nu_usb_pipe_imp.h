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
 *      nu_usb_pipe_imp.h 
 * 
 * COMPONENT 
 *      Nucleus USB Software 
 * 
 * DESCRIPTION 
 *        This file contains the data structures for the NU_USB_PIPE component.
 *
 * 
 * DATA STRUCTURES 
 *
 *      nu_usb_pipe    pipe control block.
 * 
 * FUNCTIONS 
 *      None 
 * 
 * DEPENDENCIES 
 *      None
 * 
 *************************************************************************/

/* ==================================================================== */
#ifndef _NU_USB_PIPE_IMP_H_
#define _NU_USB_PIPE_IMP_H_
/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ====================  Data Structures ============================== */

typedef struct nu_usb_endp NU_USB_ENDP;

struct nu_usb_pipe
{
    NU_USB_ENDP *endpoint;
    NU_USB_DEVICE *device;
};

/* ==================================================================== */

#endif /* _NU_USB_PIPE_IMP_H_ */

/* ======================  End Of File  =============================== */

