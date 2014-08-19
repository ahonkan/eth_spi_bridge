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
*       nu_usbf_user_scsi_ext.h
*
* COMPONENT
*
*       Nucleus USB Function Mass Storage class driver.
*
* DESCRIPTION
*
*       This file contains the exported function prototypes for the
*       Mass storage SCSI Media container driver.
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
*       nu_usbf_user_scsi_imp.h             User SCSI definitions.
*
**************************************************************************/

#ifndef _NU_USBF_USER_SCSI_EXT_H

#ifdef  __cplusplus
extern  "C" {                                /* C declarations in C++. */
#endif

#define _NU_USBF_USER_SCSI_EXT_H

/* ===============  USB Include Files  ================================= */

#include    "connectivity/nu_usbf_user_scsi_imp.h"

/* =====================  Function Prototypes  ========================= */

STATUS NU_USBF_USER_SCSI_Create (
       NU_USBF_USER_SCSI         *cb,
       CHAR                      *name,
       UINT8                      max_lun);

STATUS _NU_USBF_USER_SCSI_Create (
       NU_USBF_USER_SCSI         *cb,
       CHAR                      *name,
       UINT8                      max_lun,
       NU_USBF_USER_SCSI_CMD     *cmd_list,
       UINT8                      num_cmds,
       const VOID                *dispatch);

STATUS _NU_USBF_USER_SCSI_Delete (
       VOID                      *cb);

STATUS NU_USBF_USER_SCSI_Reg_Media (
       NU_USBF_USER_SCSI         *cb,
       NU_USBF_SCSI_MEDIA        *media);

STATUS NU_USBF_USER_SCSI_Dereg_Media (
       NU_USBF_USER_SCSI         *cb,
       NU_USBF_SCSI_MEDIA        *media);

STATUS _NU_USBF_USER_SCSI_Connect (
       NU_USB_USER                 *cb,
       NU_USB_DRVR                 *drvr,
       VOID                        *handle);

STATUS _NU_USBF_USER_SCSI_Disconnect (
       NU_USB_USER                 *cb,
       NU_USB_DRVR                 *drvr,
       VOID                        *handle);

STATUS _NU_USBF_USER_SCSI_Reset (
       NU_USB_USER                 *cb,
       const NU_USB_DRVR           *drvr,
       const VOID                  *handle);

STATUS _NU_USBF_USER_SCSI_Command (
       NU_USB_USER                 *cb,
       NU_USB_DRVR                 *drvr,
       VOID                        *handle,
       UINT8                       *cmd,
       UINT16                       cmd_len,
       UINT8                      **buf_out,
       UINT32                      *data_length_out);

STATUS _NU_USBF_USER_SCSI_Transfer (
       NU_USB_USER                 *cb,
       NU_USB_DRVR                 *drvr,
       VOID                        *handle,
       UINT8                      **buf_out,
       UINT32                      *data_length_out);

STATUS _NU_USBF_USER_SCSI_Tx_Done (
       NU_USB_USER                 *cb,
       NU_USB_DRVR                 *drvr,
       VOID                        *handle,
       UINT8                       *buf,
       UINT32                       data_length,
       UINT8                      **buf_out,
       UINT32                      *data_length_out);

STATUS _NU_USBF_USER_SCSI_Notify (
       NU_USB_USER                 *cb,
       NU_USB_DRVR                 *drvr,
       VOID                        *handle,
       UINT32                       event);

STATUS _NU_USBF_USER_SCSI_Reg_Media (
       NU_USBF_USER_SCSI           *cb,
       NU_USBF_SCSI_MEDIA          *media);

STATUS _NU_USBF_USER_SCSI_Dereg_Media (
       NU_USBF_USER_SCSI           *cb,
       const NU_USBF_SCSI_MEDIA    *media);

STATUS NU_USBF_USER_SCSI_Delete (
       VOID                        *pcb);

STATUS nu_os_conn_usb_func_ms_user_init(CHAR *path, INT startstop);

STATUS NU_USBF_USER_SCSI_GetHandle (
       VOID                       **handle);

STATUS NU_USBF_USER_SCSI_Get_Class_Handl(
       const NU_USBF_USER_SCSI     *pcb_user_scsi,
       const NU_USBF_SCSI_MEDIA    *pcb_scsi_media,
       VOID                       **pcb_ms_device);

STATUS _NU_USBF_USER_SCSI_Get_Max_Lun (
       const NU_USB_USER           *pcb_user,
       const NU_USB_DRVR           *pcb_drvr,
       const VOID                  *p_handle,
       UINT8                       *lun_out);

/* ===================================================================== */

#ifdef  __cplusplus
}                                            /* End of C declarations. */
#endif

#endif                                       /* _NU_USBF_USER_SCSI_EXT_H */

/* =======================  End Of File  =============================== */
