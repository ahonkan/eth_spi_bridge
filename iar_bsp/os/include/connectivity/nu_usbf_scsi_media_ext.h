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
*       nu_usbf_scsi_media_ext.h
*
* COMPONENT
*
*       Nucleus USB Function Mass Storage class driver.
*
* DESCRIPTION
*
*       This file contains the exported function prototypes for the Mass
*       Storage SCSI Media driver.
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
*       nu_usbf_scsi_media_imp.h           Definitions for scsi media.
*
**************************************************************************/

#ifndef _NU_USBF_SCSI_MEDIA_EXT_H

#ifdef  __cplusplus
extern  "C" {                                /* C declarations in C++. */
#endif

#define _NU_USBF_SCSI_MEDIA_EXT_H

/* =========================  USB Include Files ======================== */
#include    "connectivity/nu_usbf_scsi_media_imp.h"

/* ========================  Function Prototypes ======================= */
STATUS _NU_USBF_SCSI_MEDIA_Create (
       NU_USBF_SCSI_MEDIA         *cb,
       CHAR                       *name,
       UINT8                      *inquiry_data,
       UINT8                      *read_capacity_data,
       BOOLEAN                     is_formatted,
       const VOID                 *dispatch);

STATUS _NU_USBF_SCSI_MEDIA_Insert (NU_USBF_SCSI_MEDIA *pcb_scsi_media);
STATUS _NU_USBF_SCSI_MEDIA_Remove (NU_USBF_SCSI_MEDIA *pcb_scsi_media);

STATUS NU_USBF_SCSI_MEDIA_Connect (
       const NU_USBF_SCSI_MEDIA   *cb,
       const NU_USBF_USER_SCSI    *scsi);

STATUS NU_USBF_SCSI_MEDIA_Disconnect (
       const NU_USBF_SCSI_MEDIA   *cb,
       const NU_USBF_USER_SCSI    *scsi);

STATUS NU_USBF_SCSI_MEDIA_New_Transfer (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS NU_USBF_SCSI_MEDIA_Tx_Done (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *completed_data,
       const UINT32                completed_data_length,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS NU_USBF_SCSI_MEDIA_Ready (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *cmd,
       const UINT16                cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS NU_USBF_SCSI_MEDIA_Sense (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *cmd,
       const UINT16                cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS NU_USBF_SCSI_MEDIA_Inquiry (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *cmd,
       const UINT16                cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS NU_USBF_SCSI_MEDIA_Mode_Sense_6 (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                 *cmd,
       const UINT16                 cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS NU_USBF_SCSI_MEDIA_Mode_Sense_10 (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                 *cmd,
       const UINT16                 cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS NU_USBF_SCSI_MEDIA_Mode_Sel6 (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *cmd,
       const UINT16                cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS NU_USBF_SCSI_MEDIA_Mode_Sel_10 (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *cmd,
       const UINT16                cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS NU_USBF_SCSI_MEDIA_Snd_Diag (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *cmd,
       const UINT16                cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS NU_USBF_SCSI_MEDIA_Format (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *cmd,
       const UINT16                cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS NU_USBF_SCSI_MEDIA_Reserve_Unit (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *cmd,
       const UINT16                cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS NU_USBF_SCSI_MEDIA_Release_Unit (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *cmd,
       const UINT16                cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS NU_USBF_SCSI_MEDIA_Capacity (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *cmd,
       const UINT16                cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS NU_USBF_SCSI_MEDIA_Verify (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *cmd,
       const UINT16                cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS NU_USBF_SCSI_MEDIA_Prevent_Allow(
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *cmd,
       const UINT16                cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS NU_USBF_SCSI_MEDIA_Start_Stop(
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *cmd,
       const UINT16                cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS NU_USBF_SCSI_MEDIA_Read_6 (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *cmd,
       const UINT16                cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS NU_USBF_SCSI_MEDIA_Read_10 (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *cmd,
       const UINT16                cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS NU_USBF_SCSI_MEDIA_Write_6 (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *cmd,
       const UINT16                cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS NU_USBF_SCSI_MEDIA_Write_10 (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *cmd,
       const UINT16                cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS NU_USBF_SCSI_MEDIA_Read_12 (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *cmd,
       const UINT16                cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS NU_USBF_SCSI_MEDIA_Write_12 (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *cmd,
       const UINT16                cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS NU_USBF_SCSI_MEDIA_Unknown_Cmd (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *cmd,
       const UINT16                cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS NU_USBF_SCSI_MEDIA_Reset_Device (
       NU_USBF_SCSI_MEDIA         *cb,
       NU_USBF_USER_SCSI          *scsi);

STATUS NU_USBF_SCSI_MEDIA_Read (
       NU_USBF_SCSI_MEDIA         *cb,
       UINT32                      block_num,
       UINT8                      *buffer,
       UINT32                      length);

STATUS NU_USBF_SCSI_MEDIA_Write (
       NU_USBF_SCSI_MEDIA         *cb,
       UINT32                      block_num,
       UINT8                      *buffer,
       UINT32                      length);

STATUS _NU_USBF_SCSI_MEDIA_Connect (
       const NU_USBF_SCSI_MEDIA   *cb,
       const NU_USBF_USER_SCSI    *scsi);

STATUS _NU_USBF_SCSI_MEDIA_Disconnect (
       const NU_USBF_SCSI_MEDIA   *cb,
       const NU_USBF_USER_SCSI    *scsi);

STATUS _NU_USBF_SCSI_MEDIA_Transfer (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS _NU_USBF_SCSI_MEDIA_Tx_Done (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *completed_data,
       const UINT32                completed_data_length,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS _NU_USBF_SCSI_MEDIA_Ready (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *cmd,
       const UINT16                cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS _NU_USBF_SCSI_MEDIA_Sense (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *cmd,
       const UINT16                cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS _NU_USBF_SCSI_MEDIA_Inquiry (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *cmd,
       const UINT16                cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS _NU_USBF_SCSI_MEDIA_Mode_Sense6 (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *cmd,
       const UINT16                cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS _NU_USBF_SCSI_MEDIA_Mode_Sense_10 (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *cmd,
       const UINT16                cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS _NU_USBF_SCSI_MEDIA_Mode_Sel6 (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *cmd,
       const UINT16                cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS _NU_USBF_SCSI_MEDIA_Mode_Sel_10 (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *cmd,
       const UINT16                cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS _NU_USBF_SCSI_MEDIA_Prevent_Allow(
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *cmd,
       const UINT16                cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS _NU_USBF_SCSI_MEDIA_Start_Stop(
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *cmd,
       const UINT16                cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS _NU_USBF_SCSI_MEDIA_Snd_Diag (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *cmd,
       const UINT16                cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS _NU_USBF_SCSI_MEDIA_Format (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *cmd,
       const UINT16                cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS _NU_USBF_SCSI_MEDIA_ReserveUnit (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *cmd,
       const UINT16                cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS _NU_USBF_SCSI_MEDIA_ReleaseUnit (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *cmd,
       const UINT16                cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS _NU_USBF_SCSI_MEDIA_Capacity (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *cmd,
       const UINT16                cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS _NU_USBF_SCSI_MEDIA_Verify (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *cmd,
       const UINT16                cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS _NU_USBF_SCSI_MEDIA_Command_23 (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *cmd,
       const UINT16                cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS _NU_USBF_SCSI_MEDIA_Read_Write6 (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *cmd,
       const UINT16                cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS _NU_USBF_SCSI_MEDIA_RW10 (
       NU_USBF_SCSI_MEDIA         *cb,
       const NU_USBF_USER_SCSI    *scsi,
       const UINT8                *cmd,
       const UINT16                cmd_len,
       UINT8                     **buf_out,
       UINT32                     *data_length_out);

STATUS _NU_USBF_SCSI_MEDIA_RW12 (
       NU_USBF_SCSI_MEDIA       *pcb_scsi_media,
       const NU_USBF_USER_SCSI  *pcb_user_scsi,
       const UINT8              *p_cmd,
       const UINT16              cmd_len,
       UINT8                   **pp_buf_out,
       UINT32                   *p_data_len_out);

STATUS _NU_USBF_SCSI_MEDIA_Reset_Device(
       NU_USBF_SCSI_MEDIA         *cb,
       NU_USBF_USER_SCSI          *scsi);

STATUS _NU_USBF_SCSI_MEDIA_Delete (
       VOID                       *cb);



STATUS    NU_USBF_MS_DM_Open (VOID* dev_handle);

STATUS    NU_USBF_MS_DM_Close(VOID* dev_handle);

STATUS    NU_USBF_MS_DM_Read(VOID*    dev_handle,
                             VOID*    buffer,
                             UINT32   numbyte,
                             OFFSET_T byte_offset,
                             UINT32*  bytes_read_ptr);
                            
STATUS   NU_USBF_MS_DM_Write(
                            VOID*       dev_handle,
                            const VOID* buffer,
                            UINT32      numbyte,
                            OFFSET_T    byte_offset,
                            UINT32*     bytes_written_ptr);
                            
STATUS   NU_USBF_MS_DM_IOCTL(
                            VOID*     dev_handle,
                            INT       ioctl_num,
                            VOID*     ioctl_data,
                            INT       ioctl_data_len);

#ifdef  __cplusplus
}                                            /* End of C declarations. */
#endif

#endif                                       /*_NU_USBF_SCSI_MEDIA_EXT_H */

/* ============================  End Of File  ========================== */











