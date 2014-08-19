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
*        nu_usbh_drvr_ext.h
*
* COMPONENT
*
*        Nucleus USB Host Software
*
* DESCRIPTION
*
*        This file contains the exported function names of NU_USBH_DRVR.
*
*
* DATA STRUCTURES
*        None
*
* FUNCTIONS
*       None 
*
* DEPENDENCIES 
*
*        nu_usbh_ext.h            USB Host Singleton definitions
*
************************************************************************/

/* ==================================================================== */

#ifndef _NU_USBH_DRVR_EXT_H_
#define _NU_USBH_DRVR_EXT_H_

/* ==================================================================== */

/* ==============  USB Include Files =================================  */
#include "connectivity/nu_usbh_ext.h"
/* ==================================================================== */

STATUS _NU_USBH_DRVR_Create (NU_USBH_DRVR * cb,
                             CHAR * name,
                             UINT32 match_flag,
                             UINT16 idVendor,
                             UINT16 idProduct,
                             UINT16 bcdDeviceLow,
                             UINT16 bcdDeviceHigh,
                             UINT8 bInterfaceClass,
                             UINT8 bInterfaceSubClass,
                             UINT8 bInterfaceProtocol,
                             const VOID *dispatch);

STATUS _NU_USBH_DRVR_Delete (VOID *cb);
#endif /* _NU_USBH_DRVR_EXT_H_ */

/* ========================== End of File ============================= */

