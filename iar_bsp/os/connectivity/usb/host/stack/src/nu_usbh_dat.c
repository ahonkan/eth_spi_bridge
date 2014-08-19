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
*        nu_usbh_dat.c
*
* COMPONENT
*       Nucleus USB Host software.
*
* DESCRIPTION
*       This file defines the NU_USBH singleton instance. 
*
* DATA STRUCTURES
*       None
*
* FUNCTIONS
*       None
*
* DEPENDENCIES 
*       nu_usb.h              All USB definitions
*
************************************************************************/
#ifndef USBH_DAT_C
#define USBH_DAT_C

/* ==============  Standard Include Files ============================  */
#include "connectivity/nu_usb.h"

/* Singleton instance of Nucleus USB Host. */
NU_USBH *nu_usbh;

/* Host Stack Control Block */
NU_USBH_STACK *NU_USBH_Stack_CB_Pt = NU_NULL;

/************************************************************************/

#endif /* USBH_DAT_C */
/* ======================  End Of File  =============================== */
