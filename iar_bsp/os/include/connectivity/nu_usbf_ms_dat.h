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
*       nu_usbf_ms_dat.h
*
* COMPONENT
*
*       Nucleus USB Function Mass Storage class driver.
*
* DESCRIPTION
*
*       This file contains the shared data declarations for the Mass
*       storage class driver.
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
*       None.
*
**************************************************************************/

#ifndef _NU_USBF_MS_DAT_H

#ifdef  __cplusplus
extern  "C" {                                /* C declarations in C++. */
#endif

#define _NU_USBF_MS_DAT_H

/* ======================  Global data ================================  */

extern NU_USBF_MS *NU_USBF_MS_Cb_Pt;

extern const NU_USBF_DRVR_DISPATCH usbf_ms_dispatch;

/* ===================================================================== */

#ifdef  __cplusplus
}                                            /* End of C declarations. */
#endif

#endif                                       /* _NU_USBF_MS_DAT_H */

/* ======================  End Of File  ================================ */
