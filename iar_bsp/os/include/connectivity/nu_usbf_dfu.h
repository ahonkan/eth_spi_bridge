/**************************************************************************
*
*               Copyright Mentor Graphics Corporation 2013
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
*
* FILE NAME
*
*   nu_usbf_dfu.h
*
* COMPONENT
*
*   Nucleus USB Function Software : DFU Class Driver.
*
* DESCRIPTION
*
*   This file contains prototypes for the external interfaces, data
*   structures and macros exposed by DFU Class Driver.
*
* DATA STRUCTURES
*
*   None.
*
* FUNCTIONS
*
*   None. 
*
* DEPENDENCIES
*
*   None.
*
**************************************************************************/

/* =====================================================================*/
#ifndef _NU_USBF_DFU_H_
#define _NU_USBF_DFU_H_
#ifdef  __cplusplus
extern  "C" {                                /* C declarations in C++. */
#endif

/* Ideally a public header file must not include any private header files.
 * These files are included temporarly. These should be removed from here
 * in future.
 */
#include "os/connectivity/usb/function/dfu/nu_usbf_dfu_ext.h"

#ifdef  __cplusplus
}                                            /* End of C declarations. */
#endif

#endif /* _NU_USBF_DFU_H_ */

/*============================  End Of File  ===========================*/
