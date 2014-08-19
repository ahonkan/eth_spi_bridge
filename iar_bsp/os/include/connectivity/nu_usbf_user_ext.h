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
*       nu_usbf_user_ext.h 
*
*
* COMPONENT
*
*       Nucleus USB Device Software.
*
* DESCRIPTION
*
*       This file contains the exported function prototypes for the device
*       (base) user component.
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
*       nu_usbf_user_imp.h
*
**************************************************************************/

#ifndef _NU_USBF_USER_EXT_H
#define _NU_USBF_USER_EXT_H

/* ==============  USB Include Files =================================== */

#include    "connectivity/nu_usbf_user_imp.h"

/* ====================  Function Prototypes =========================== */

STATUS _NU_USBF_USER_Create (NU_USBF_USER * cb,
                             CHAR * name,
                             UINT8 bInterfaceSubclass,
                             UINT8 bInterfaceProtocol,
                             const VOID *dispatch);

STATUS _NU_USBF_USER_Delete (VOID *cb);

STATUS NU_USBF_USER_New_Command (NU_USBF_USER * cb,
                                 NU_USBF_DRVR * drvr,
                                 VOID *handle,
                                 UINT8 *command,
                                 UINT16 cmd_len,
                                 UINT8 **data,
                                 UINT32 *data_len);

STATUS NU_USBF_USER_New_Transfer (NU_USBF_USER * cb,
                                  NU_USBF_DRVR * drvr,
                                  VOID *handle,
                                  UINT8 **data,
                                  UINT32 *data_len);

STATUS NU_USBF_USER_Tx_Done (NU_USBF_USER * cb,
                             NU_USBF_DRVR * drvr,
                             VOID *handle,
                             UINT8 *completed_data,
                             UINT32 completed_data_len,
                             UINT8 **data,
                             UINT32 *data_len);

STATUS NU_USBF_USER_Notify (NU_USBF_USER * cb,
                            NU_USBF_DRVR * drvr,
                            VOID *handle,
                            UINT32 event);

/* ===================================================================== */

#endif                                      /* _NU_USBF_DRVR_EXT_H_      */

/* ======================  End Of File  ================================ */
