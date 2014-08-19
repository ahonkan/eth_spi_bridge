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
*       nu_usbf_ext.h
*
*
* COMPONENT
*
*       Stack Component / Nucleus USB Device Software.
*
* DESCRIPTION
*
*       This file contains the exported function prototypes for the
*       singleton object.
*
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
*       nu_usbf_imp.h
*
**************************************************************************/

#ifndef _NU_USBF_EXT_H_
#define _NU_USBF_EXT_H_

/* ==============  USB Include Files ==================================  */

#include "connectivity/nu_usbf_imp.h"

/* ====================  Function Prototypes =========================== */

STATUS NU_USBF_Create (NU_USBF * singleton_cb);
STATUS NU_USBF_Delete (VOID);

/* ===================================================================== */

#endif      /* _NU_USBF_EXT_H_           */

/* ======================  End Of File  ================================ */

