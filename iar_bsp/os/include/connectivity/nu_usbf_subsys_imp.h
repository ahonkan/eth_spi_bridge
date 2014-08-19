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

****************************************************************************
*
* FILE NAME 
*
*       nu_usbf_subsys_imp.h 
*
*
* COMPONENT
*
*       Stack component / Nucleus USB Device Software.
*
* DESCRIPTION
*
*       This file contains the internal declarations for the device
*       subsystem.
*
* DATA STRUCTURES
*
*       NU_USBF_SUBSYS                      Subsystem control block.
*       NU_USBF_SUBSYS_DISPATCH             Subsystem dispatch table.
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       nu_usbf_subsys_dat.h                USB Function subsystem
*
**************************************************************************/

#ifndef _NU_USBF_SUBSYS_IMP_H
#define _NU_USBF_SUBSYS_IMP_H

/* =====================  Global data =================================  */

typedef NU_USB_SUBSYS NU_USBF_SUBSYS;

typedef NU_USB_SUB_SYSTEM_DISPATCH NU_USBF_SUBSYS_DISPATCH;

/* ===================================================================== */

#include "connectivity/nu_usbf_subsys_dat.h"

/* ===================================================================== */

#endif                                      /* _NU_USBF_SUBSYS_IMP_H     */

/* ======================  End Of File  ================================ */

