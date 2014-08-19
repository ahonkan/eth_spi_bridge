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
 *      nu_usb_pipe_ext.h
 * 
 * COMPONENT 
 *      Nucleus USB Software 
 * 
 * DESCRIPTION 
 *        This file contains the exported function names for the pipe 
 *        component.
 *
 * 
 * DATA STRUCTURES 
 *      None
 * 
 * FUNCTIONS 
 *      None 
 * 
 * DEPENDENCIES 
 *      nu_usb_pipe_imp.h       Pipe's internal definitions.
 * 
 *************************************************************************/

/* ==================================================================== */
#ifndef _NU_USB_PIPE_EXT_H_
#define _NU_USB_PIPE_EXT_H_
/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

#define NU_USB_PIPE_Submit_IRP(pipe, irp) \
        NU_USB_Submit_IRP(pipe->device->stack, irp, pipe)

#define NU_USB_PIPE_Cancel_IRP(stack, pipe) \
        NU_USB_Cancel_IRP(stack,pipe)
/* ===================== Include Files ================================ */
#include "connectivity/nu_usb_pipe_imp.h"

/* ====================  Function Prototypes ========================== */

/* Status services */
STATUS NU_USB_PIPE_Get_Is_Active (NU_USB_PIPE * cb,
                                  BOOLEAN * is_active_out);
STATUS NU_USB_PIPE_Set_Is_Active (NU_USB_PIPE * cb,
                                  BOOLEAN is_active);
STATUS NU_USB_PIPE_Get_Is_Stalled (NU_USB_PIPE * cb,
                                   BOOLEAN * is_stalled_out);
STATUS NU_USB_PIPE_Stall (NU_USB_PIPE * cb);
STATUS NU_USB_PIPE_Unstall (NU_USB_PIPE * cb);

/* IRP services */
STATUS NU_USB_PIPE_Flush (NU_USB_PIPE * cb);

/* Services to access associated objects */
STATUS NU_USB_PIPE_Get_Endp (NU_USB_PIPE * cb,
                             NU_USB_ENDP ** endp_out);
STATUS NU_USB_PIPE_Set_Endp (NU_USB_PIPE * cb,
                             NU_USB_ENDP * endp);
STATUS NU_USB_PIPE_Get_Device (NU_USB_PIPE * cb,
                               NU_USB_DEVICE ** device_out);
STATUS NU_USB_PIPE_Set_Device (NU_USB_PIPE * cb,
                               NU_USB_DEVICE * device);

/* ==================================================================== */

#endif /* _NU_USB_PIPE_EXT_H_ */

/* ======================  End Of File  =============================== */

