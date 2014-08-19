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
*       nu_usbf_user_ms_imp.h
*
* COMPONENT
*
*       Nucleus USB Function Mass Storage class driver.
*
* DESCRIPTION
*
*       This file contains the internal declarations for the Mass Storage
*       user driver.
*
* DATA STRUCTURES
*
*       NU_USBF_USER_MS                     Mass storage user control
*                                           block.
*       NU_USBF_USER_MS_DISPATCH            Mass storage user dispatch
*                                           table.
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       nu_usbf_ms_ext.h                    Function mass storage external
*                                           interface.
*
**************************************************************************/

#ifndef _NU_USBF_USER_MS_IMP_H

#ifdef          __cplusplus
extern  "C" {                                /* C declarations in C++. */
#endif

#define _NU_USBF_USER_MS_IMP_H

/* =========================  USB Include Files ======================== */
#include "connectivity/nu_usbf_ms_ext.h"

/* Data Types. */
typedef struct _nu_usbf_user_ms
{
    NU_USBF_USER          parent;
    UINT8                 max_lun;
    UINT8                 subclass;

#if     PAD_2
    DATA_ELEMENT          cs_padding[PAD_2];
#endif

}   NU_USBF_USER_MS;

typedef struct _nu_usbf_user_ms_dispatch
{
    NU_USBF_USER_DISPATCH dispatch;
    STATUS (*Reset) (
           NU_USB_USER       *cb,
           const NU_USB_DRVR *drvr,
           const VOID        *handle);

    STATUS (*Get_Max_Lun) (
		   const NU_USB_USER *pcb_user_ms,
           const NU_USB_DRVR *p_drvr,
           const VOID        *p_handle,
           UINT8             *p_max_lun_out);
}   NU_USBF_USER_MS_DISPATCH;

#ifdef  __cplusplus
}                                            /* End of C declarations. */
#endif

#endif                                       /* _NU_USBF_USER_MS_IMP_H  */

/* ======================  End Of File  ================================ */
