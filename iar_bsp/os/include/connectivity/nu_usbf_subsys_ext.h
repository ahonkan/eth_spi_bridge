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
*       nu_usbf_subsys_ext.h 
*
*
* COMPONENT
*
*       Stack component / Nucleus USB Device Software.
*
* DESCRIPTION
*
*       This file contains the exported function prototypes
*       for the device subsystem.
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
*       nu_usbf_subsys_imp.h                USB Function subsystem
*
**************************************************************************/

#ifndef _NU_USBF_SUBSYS_EXT_H
#define _NU_USBF_SUBSYS_EXT_H

/* ==============  USB Include Files ==================================  */

#include    "connectivity/nu_usbf_subsys_imp.h"

/* ====================  Function Prototypes =========================== */

/* NU_USBF_Sub_System API. */
STATUS NU_USBF_SUBSYS_Create (NU_USBF_SUBSYS * cb);

/* Base class extended  methods. */
STATUS NU_USBF_SUBSYS_Delete (NU_USBF_SUBSYS * cb);

/* ===================================================================== */

#endif                                      /* _NU_USBF_SUBSYS_EXT_H     */

/* ======================  End Of File  ================================ */

