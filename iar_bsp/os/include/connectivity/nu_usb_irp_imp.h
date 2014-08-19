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
*        nu_usb_irp_imp.h
*
* COMPONENT
*
*        Nucleus USB Software
*
* DESCRIPTION
*       This file contains the Control Block and other internal data 
*       structures and definitions for IRP Component of
*       Nucleus USB Software.
*
*
* DATA STRUCTURES
*       _nu_usb_irp         IRP control block
*
* FUNCTIONS
*       None 
*
* DEPENDENCIES 
*       None
*
************************************************************************/

/* ==================================================================== */
#ifndef _NU_USB_IRP_IMP_H
#define _NU_USB_IRP_IMP_H

/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* ==================================================================== */

typedef VOID (*NU_USB_IRP_CALLBACK) (NU_USB_PIPE * pipe,
                                     NU_USB_IRP * irp);

/* =====================  #defines ===================================  */

/* Transfer parameters for 'flags' member in 'IRP' structure */

#define USB_SHORT_NOT_OK  0x00000001    /* Short packets are errors */
#define USB_ISO_ASAP      0x00000002    /* Start ISO transfer at the earliest */
#define USB_ZERO_PACKET   0x00000004    /* End OUT transfer with short packet */
#define USB_NO_INTERRUPT  0x00000008    /* no interrupt needed,except for error */

/* ====================  Data Types ==================================  */

struct _nu_usb_irp
{
    CS_NODE node;
    UINT8 version;                          /* IRP Class version */
    UINT32 flags;                           /* transfer options             */
    UINT32 length;                          /* transfer length              */
    VOID *buffer;                           /* buffer to transfer data      */
    NU_USB_IRP_CALLBACK callback;           /* function to be invoked after 
                                               transfer completion.         */
    VOID *callback_data;                    /* Can be used to 
                                               store context of IRP         */
    UINT32 actual_length;                   /* Actual length of data transfer
                                               completed.                   */
    STATUS status;                          /* Status of data transfer      */

    NU_USB_PIPE *pipe;                      /* Pipe for the transfer        */
    
    UINT16 interval;                        /* for interrupt transfers      */
	BOOLEAN buffer_type;                    /* NU_TRUE for cacheable buffer
	                                         * and NU_FALSE for non-caheable 
											 * buffer.
											 */
    UINT8      dummy_align;
};

/* ==================================================================== */

#endif /* _NU_USB_IRP_IMP_H    */

/* =======================  End Of File  ============================== */

