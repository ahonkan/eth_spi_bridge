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
*       nu_usbh_ms_user_ext.h
*
* COMPONENT
*
*       Nucleus USB Host Mass Storage class User Driver.
*
* DESCRIPTION
*
*       This file contains the definitions for external interfaces
*       exported by USBH MS User driver component.
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
*       nu_usbh_ms_user_imp.h               Internal definitions.
*
**************************************************************************/

/* ===================================================================== */

#ifndef     _NU_USBH_MS_USER_EXT_H_
#define     _NU_USBH_MS_USER_EXT_H_

/* ===================================================================== */

#ifndef     _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* ====================  USB Include Files ============================= */

#include "connectivity/nu_usbh_ms_user_imp.h"

/* ====================  Function Prototypes  ========================== */

/* NU_USBH_MS_SCSI user API. */

STATUS      NU_USBH_MS_USER_Create(NU_USBH_MS_USER* cb, CHAR *name,
                                    NU_MEMORY_POOL * pool);

STATUS      NU_USBH_MS_USER_Delete(VOID * cb );

STATUS      NU_USBH_MS_USER_SET_App_Callbacks(VOID *dispatch);

STATUS      NU_USBH_MS_USER_Init(NU_MEMORY_POOL *USB_Cached_Memory_Pool,
                                   NU_MEMORY_POOL *USB_Uncached_Memory_Pool);

STATUS      NU_USBH_MS_USER_GetHandle ( NU_USBH_MS_USER** handle);

STATUS      NU_USBH_MS_USER_Get_SC_Dispatch(
                                   NU_USBH_MS_USER_DISPATCH **dispatch,
                                   VOID *handle);

STATUS nu_os_conn_usb_host_ms_user_init (const CHAR * key, int startstop);
STATUS  NU_USBH_MS_DM_Open (VOID* dev_handle);

STATUS  NU_USBH_MS_DM_Close (VOID* dev_handle);

STATUS  NU_USBH_MS_DM_Read (VOID *session_handle,
                                        VOID *buffer,
                                        UINT32 numbyte,
                                        OFFSET_T byte_offset,
                                        UINT32 *bytes_read_ptr);
STATUS  NU_USBH_MS_DM_Write (VOID *session_handle,
                                        const VOID *buffer,
                                        UINT32 numbyte,
                                        OFFSET_T byte_offset,
                                        UINT32 *bytes_written_ptr);
STATUS  NU_USBH_MS_DM_IOCTL (VOID *session_handle,
                                        INT cmd,
                                        VOID *data,
                                        INT length);
/* ===================================================================== */

#endif      /* _NU_USBH_MS_USER_EXT_H_ */

/* ====================  End Of File  ================================== */
