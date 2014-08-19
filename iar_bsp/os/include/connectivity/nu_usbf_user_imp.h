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
*       nu_usbf_user_imp.h 
*
*
* COMPONENT
*
*       Nucleus USB Device Software
*
* DESCRIPTION
*
*       This file contains the internal declarations for the device
*       (base) user component.
*
* DATA STRUCTURES
*
*       NU_USBF_USER                        User control block.
*       NU_USBF_USER_DISPATCH               User dispatch table.
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       nu_usbf_stack_ext.h                USB Function stack declarations
*
**************************************************************************/

/* ===================================================================== */

#ifndef _NU_USBF_USER_IMP_H
#define _NU_USBF_USER_IMP_H

/* ==============  USB Include Files =================================== */

#include "connectivity/nu_usbf_stack_ext.h"

/* ====================  Data Types ==================================== */

typedef NU_USB_USER NU_USBF_USER;

typedef struct nu_usbf_user_dispatch
{
    NU_USB_USER_DISPATCH dispatch;

    STATUS (*New_Command) (NU_USBF_USER * cb,
                           NU_USBF_DRVR * drvr,
                           VOID *handle,
                           UINT8 *command,
                           UINT16 cmd_len,
                           UINT8 **data,
                           UINT32 *data_len);

    STATUS (*New_Transfer) (NU_USBF_USER * cb,
                            NU_USBF_DRVR * drvr,
                            VOID *handle,
                            UINT8 **data,
                            UINT32 *data_len);

    STATUS (*Transfer_Complete) (NU_USBF_USER * cb,
                                 NU_USBF_DRVR * drvr,
                                 VOID *handle,
                                 UINT8 *completed_data,
                                 UINT32 completed_data_len,
                                 UINT8 **data,
                                 UINT32 *data_len);

    STATUS (*Notify) (NU_USBF_USER * cb,
                      NU_USBF_DRVR * drvr,
                      VOID *handle,
                      UINT32 event);
}
NU_USBF_USER_DISPATCH;

/* ====================  Function Prototypes =========================== */

/* ===================================================================== */

#endif                                      /* _NU_USBF_DRVR_IMP_H_      */

/* ======================  End Of File  ================================ */

