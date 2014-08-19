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
************************************************************************/

/************************************************************************
 *
 * FILE NAME
 *
 *      nu_usbh_subsys_imp.h
 *
 * COMPONENT
 *      Nucleus USB Host Software
 *
 * DESCRIPTION
 *
 *        This file describes control block and dispatch table of
 *        NU_USBH_SUBSYS. It also contains declaration for internal
 *        functions.
 *
 * DATA STRUCTURES
 *      nu_usbh_subsys_dispatch     subsystem Dispatch table description.
 *      nu_usbh_subsys              user control block description.
 *
 * FUNCTIONS
 *      None
 *
 * DEPENDENCIES
 *      nu_usbh_subsys_dat.h       host subsystem dispatch table definitions
 *
 *************************************************************************/

/* ===================================================================  */
#ifndef _NU_USBH_SUBSYS_IMP_H_
#define _NU_USBH_SUBSYS_IMP_H_
/* ===================================================================  */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================================================================  */

/* =================== Data Structures ===============================  */

typedef struct _nu_usbh_subsys NU_USBH_SUBSYS;

typedef NU_USB_SUB_SYSTEM_DISPATCH NU_USBH_SUBSYS_DISPATCH;

struct _nu_usbh_subsys
{
    NU_USB_SUBSYS cb;

#if (!NU_USB_OPTIMIZE_FOR_SIZE)
    /* Extended */
    NU_SEMAPHORE lock;
#endif /* #if (!NU_USB_OPTIMIZE_FOR_SIZE) */

};

/* ================== Function Prototypes ============================  */
/* NU_USBH_Sub_System API. */
STATUS NU_USBH_SUBSYS_Create (NU_USBH_SUBSYS * cb);

/* Base class's extended  methods */
STATUS _NU_USBH_SUBSYS_Delete (NU_USB_SUBSYS * cb);

/* NU_USBH_Sub_System Reentrance Protection services */

STATUS _NU_USBH_SUBSYS_Lock (NU_USB_SUBSYS * cb);

STATUS _NU_USBH_SUBSYS_Unlock (NU_USB_SUBSYS * cb);

/* ==============  USB Include Files =================================  */

#include "connectivity/nu_usbh_subsys_dat.h"

/* ===================================================================  */
#endif /* _NU_USBH_SUBSYS_IMP_H_ */
/* ====================== end of file ================================  */

