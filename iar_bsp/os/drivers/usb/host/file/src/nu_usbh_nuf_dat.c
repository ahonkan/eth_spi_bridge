/**************************************************************************
*
*              Copyright 2003 Mentor Graphics Corporation
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
*       nu_usbh_nuf_dat.c
*
* COMPONENT
*
*       Nucleus USB Host FILE Driver.
*
* DESCRIPTION
*
*       This file contains global declarations from Nucleus USB Host FILE
*       Driver.
*
* DATA STRUCTURES
*
*
* FUNCTIONS
*
*        None.
*
* DEPENDENCIES
*
*        nu_usbh_nuf_imp.h                  
*
**************************************************************************/

/* ====================  Standard Include Files ======================== */
#include "drivers/nu_usbh_nuf_imp.h"

/* ===================================================================== */

NUF_USBH_DRVR   *pUSBH_File_Drvr   = NU_NULL;

NU_USBH_MS_APP_CALLBACKS app_callbacks =
{
    NUF_USBH_Connect,
    NUF_USBH_Disconnect
};

/************************* End Of File ***********************************/
