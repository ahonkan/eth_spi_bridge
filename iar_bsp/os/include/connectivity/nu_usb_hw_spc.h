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
*        nu_usb_hw_spc.h 
* 
*    COMPONENT
*
*        Nucleus USB Software 
* 
*    DESCRIPTION
*
*        This file contains target specific definitions of the HW component.
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
*        None
* 
*************************************************************************/
/* ==================================================================== */
#ifndef _NU_USB_HW_SPC_H
#define _NU_USB_HW_SPC_H

/* ===================================================================  */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================================================================  */

/* ======================== #defines ================================== */

/* Endian Specific Macros */

#if (ESAL_PR_ENDIANESS == ESAL_BIG_ENDIAN)
#define LE16_2_HOST(A)       endianswap16((UINT16)(A))
#define LE32_2_HOST(A)       endianswap32((UINT32)(A))
#define HOST_2_LE16(A)       endianswap16((UINT16)(A))
#define HOST_2_LE32(A)       endianswap32((UINT32)(A))
#else
#define LE16_2_HOST(A)       A
#define LE32_2_HOST(A)       A
#define HOST_2_LE16(A)       A
#define HOST_2_LE32(A)       A
#endif
/* ==================================================================== */

#endif /* _NU_USB_HW_SPC_H    */

/* =======================  End Of File  ============================== */

