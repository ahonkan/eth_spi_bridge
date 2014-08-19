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
*       nu_usbh_ms_ext.h
*
*
* COMPONENT
*
*       Nucleus USB Host Software
*
* DESCRIPTION
*
*       This file contains definitions for external Interfaces exposed
*       by Mass Storage Class Driver.
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
*       nu_usbh_ms_imp.h                    Internal Definitions.
*
**************************************************************************/

/* ===================================================================== */

#ifndef     _NU_USBH_MS_EXT_H_
#define     _NU_USBH_MS_EXT_H_

/* ===================================================================== */

#ifndef     _NUCLEUS_USB_H_
#error Do not directly include internal headers,\
use #include "usb/common/stack/inc/nu_usb.h"
#endif

/* ===================================================================== */

#include "connectivity/nu_usbh_ms_imp.h"

/* ====================  Function Prototypes  ========================== */

STATUS      nu_os_conn_usb_host_ms_class_init(CHAR *path, INT startstop);

STATUS      NU_USBH_MS_Set_Usrptr(VOID *handle, VOID *user_pointer);

VOID       *NU_USBH_MS_Get_Usrptr(VOID *handle);


STATUS      NU_USBH_MS_Create(NU_USBH_MS *cb, CHAR * name,
                                    NU_MEMORY_POOL *pool);

STATUS      NU_USBH_MS_Transport(NU_USBH_MS *cb, NU_USBH_USER *user,
                                    VOID *session, VOID *command,
                                    UINT8 cmd_length, VOID *data_buffer,
                                    UINT32 data_length, UINT8 direction);
                                    
STATUS      NU_USBH_MS_Set_Data_Buff_Cachable(NU_USBH_MS_DRIVE   *drive,
                                            BOOLEAN             data_buff_type);

STATUS      _NU_USBH_MS_Delete(VOID *cb);

STATUS      _NU_USBH_MS_Initialize_Intf(NU_USB_DRVR *cb,
                                    NU_USB_STACK *stack,
                                    NU_USB_DEVICE *device,
                                    NU_USB_INTF *intf);

STATUS      _NU_USBH_MS_Disconnect(NU_USB_DRVR *cb, NU_USB_STACK * stack,
                                    NU_USB_DEVICE * device);


STATUS NU_USBH_MS_Suspend_Device (VOID *session);

STATUS NU_USBH_MS_Resume_Device (VOID *session);

STATUS NU_USBH_MS_Init_GetHandle(VOID  **handle);
/* ===================================================================== */

#endif      /* _NU_USBH_MS_EXT_H_ */

/* ====================  End Of File  ================================== */
