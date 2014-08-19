/**************************************************************************
*
*              Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
*
* FILE NAME
*
*       nu_usbh_nuf_ext.h
*
* COMPONENT
*
*       Nucleus USB Host FILE Driver.
*
* DESCRIPTION
*
*       This file contains the definitions for Nucleus USB Host File
*       Drivers External Interface.
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
*       nucleus.h
*       nu_kernel.h
*       pcdisk.h
*       nu_usb.h
*
**************************************************************************/

/* ===================================================================== */

#ifndef     _NU_USBH_NUF_EXT_H

#ifdef      __cplusplus
extern  "C" {                               /* C declarations in C++.    */
#endif

#define _NU_USBH_NUF_EXT_H

/* ====================  Include Files  =============+============= */
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "connectivity/nu_usb.h"
#include "storage/pcdisk.h"

/* ======================  Macro Function  ============================= */

STATUS NU_USBH_NUF_Create_Drv(VOID);

STATUS NU_USBH_NUF_Delete_Drv(VOID);

STATUS nu_os_conn_usbh_ms_file_init(const CHAR *path, INT startstop);

STATUS  NU_USBH_FILE_DM_Open (VOID* dev_handle);

STATUS  NU_USBH_FILE_DM_Close (VOID* dev_handle);

STATUS  NU_USBH_FILE_DM_Read (VOID *session_handle,
                                        VOID *buffer,
                                        UINT32 numbyte,
                                        OFFSET_T byte_offset,
                                        UINT32 *bytes_read_ptr);
STATUS  NU_USBH_FILE_DM_Write (VOID *session_handle,
                                        const VOID *buffer,
                                        UINT32 numbyte,
                                        OFFSET_T byte_offset,
                                        UINT32 *bytes_written_ptr);
STATUS  NU_USBH_FILE_DM_IOCTL (VOID *session_handle,
                                        INT cmd,
                                        VOID *data,
                                        INT length);
/* ===================================================================== */
#ifdef  __cplusplus
}                                           /* End of C declarations.    */
#endif

#endif      /* End of #ifdef FILE_VERSION_COMP    */

/* ======================  End Of File  ================================ */
