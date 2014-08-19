/**************************************************************************
*
*               Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
* FILE NAME
*
*       nu_usbf_user_ms_ext.h
*
* COMPONENT
*
*       Nucleus USB Function Mass Storage class driver.
*
* DESCRIPTION
*
*       This file contains the exported function prototypes for the Mass
*       Storage user driver.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       nu_usbf_user_ms_imp.h               Definitions for function user
*                                           Mass Storage.
*
**************************************************************************/

#ifndef _NU_USBF_USER_MS_EXT_H

#ifdef  __cplusplus
extern  "C" {                                /* C declarations in C++. */
#endif

#define _NU_USBF_USER_MS_EXT_H

/* =========================  USB Include Files ======================== */
#include    "connectivity/nu_usbf_user_ms_imp.h"

/* Function Prototypes. */
STATUS _NU_USBF_USER_MS_Create (
       NU_USBF_USER_MS        *cb,
       CHAR                   *name,
       UINT8                   subclass,
       UINT8                   max_lun,
       const VOID             *dispatch);

STATUS NU_USBF_USER_MS_Reset (
       NU_USB_USER            *cb,
       NU_USB_DRVR            *drvr,
       VOID                   *handle);

STATUS NU_USBF_USER_MS_Get_Max_LUN (
       const NU_USB_USER      *cb,
       const NU_USB_DRVR      *drvr,
       const VOID             *handle,
       UINT8                  *max_lun_out);

STATUS _NU_USBF_USER_MS_Delete (
       NU_USB                 *cb);

#ifdef  __cplusplus
}                                            /* End of C declarations. */
#endif

#endif                                       /* _NU_USBF_USER_MS_EXT_H  */

/* ========================  End Of File  ============================== */
