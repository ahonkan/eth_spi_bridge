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

***************************************************************************
*
* FILE NAME 
*
*       nu_usbf_dat.c
*
*
* COMPONENT
*
*       Stack component / Nucleus USB Device Software.
*
* DESCRIPTION
*
*       This file contains the shared data declarations for the Nucleus USB
*       device stack, singleton object.
*
* DATA STRUCTURES
*
*       nu_usbf                             Container for Nucleus USB
*                                           Device.
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions.
*
**************************************************************************/

/* ==============  USB Include Files ==================================  */

#include    "connectivity/nu_usb.h"

/* =====================  Global data =================================  */
/* Singleton container for Nucleus USB Device Software. */
NU_USBF    *nu_usbf;

/* Function Stack Control Block pointer */
NU_USBF_STACK   *NU_USBF_Stack_CB_Pt = NU_NULL;

/* usbf_hisr stack */
UINT8   usbf_hisr_stack[USBF_HISR_STACK_SIZE];

/* ======================  End Of File  ================================ */
