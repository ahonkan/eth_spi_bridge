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
*       nu_usbf_scsi_media_imp.h
*
* COMPONENT
*
*       Nucleus USB Function Mass Storage class driver.
*
* DESCRIPTION
*
*       This file contains the internal declarations for the Mass Storage
*       SCSI Media driver.
*
* DATA STRUCTURES
*
*       NU_USBF_SCSI_MEDIA                  SCSI Media driver control
*                                           block.
*       NU_USBF_SCSI_MEDIA_DISPATCH         SCSI Media driver dispatch
*                                           table.
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       nu_usbf_user_scsi_ext.h             Definitions for external
*                                           interfaces of SCSI.
*
**************************************************************************/
#ifndef _NU_USBF_SCSI_MEDIA_IMP_H

#ifdef  __cplusplus
extern  "C" {                                /* C declarations in C++. */
#endif

#define _NU_USBF_SCSI_MEDIA_IMP_H

/* =========================  USB Include Files ======================== */
#include  "connectivity/nu_usbf_user_scsi_ext.h"

/* Removing some Compiler Warnings. */
void *memset (void *s, int c, register size_t n);

/* =======================  #defines  ================================== */

#define USB_SPEC_MSF_CMD_23                 0x23
#define USB_SPEC_MSF_REQSENSE_DATA_LEN      18
#define USB_SPEC_MSF_INQUIRY_DATA_LEN       36
#define USB_SPEC_MSF_MODE_DATA_LEN          200
#define USB_SPEC_MSF_CAPACITY_DATA_LEN      8
#define USB_SPEC_MSF_SENSE_DATA_LEN         18
#define USB_SPEC_MSF_SENSE_EROR_CODE        0x70

#define USB_SPEC_MSF_READ_FMT_CAPACITY      0x23
#define USB_SPEC_MSF_CPACITY_LIST_LEN       255

#define USB_SPEC_MSF_FMT_CPTY_DATA_LEN      12
#define USB_SPEC_MSF_MOD_DATA_LEN           4
#define USB_SPEC_MSF_MOD_PAGE_CTRL          3
#define USB_SPEC_MSF_MAX_FORMAT_CPACITY     0x03
#define USB_SPEC_MSF_DATA_LEN_OFFSET        4

/* Sense Key codes. */
#define SCSI_SENSE_KEY_NO_SENSE             0x00
#define SCSI_SENSE_KEY_RECOVERED_ERROR      0x01
#define SCSI_SENSE_KEY_NOT_READY            0x02
#define SCSI_SENSE_KEY_MEDIUM_ERROR         0x03
#define SCSI_SENSE_KEY_HARDWARE_ERROR       0x04
#define SCSI_SENSE_KEY_ILLEGAL_REQUEST      0x05
#define SCSI_SENSE_KEY_UNIT_ATTENTION       0x06
#define SCSI_SENSE_KEY_DATA_PROTECT         0x07
#define SCSI_SENSE_KEY_BLANK_CHECK          0x08
#define SCSI_SENSE_KEY_VENDOR_SPECIFIC      0x09
#define SCSI_SENSE_KEY_COPY_ABORTED         0x0A
#define SCSI_SENSE_KEY_ABBORTED_COMMAND     0x0B
#define SCSI_SENSE_KEY_VOLUME_OVERFLOW      0x0D
#define SCSI_SENSE_KEY_MISCOMPARE           0x0E

/* Different Removable Media States. */
#define MEDIA_ABSENT                        0x00
#define MEDIA_INSERTED                      0x01
#define MEDIA_PRESENT                       0x02

#define USBF_SCSI_BUFFER_SIZE               ((USBF_SCSI_BLOCKS_PER_BUFFER) * \
                                             (USBF_SCSI_BLOCK_SIZE))
#define USBF_SCSI_BLOCK_SIZE                512
#define USBF_SCSI_BLOCKS_PER_BUFFER         CFG_NU_OS_CONN_USB_FUNC_MS_USER_SCSI_BLOCKS_PER_BUFFER
#define USBF_SCSI_SHIFT_ONE_BYTE            8
#define USBF_SCSI_SHIFT_TWO_BYTES           16
#define USBF_SCSI_SHIFT_THREE_BYTES         24
#define USBF_SCSI_LUN_MASK                  0x1F
#define USBF_SCSI_MASK                      (UINT32)0xFF

/* IOCTLS */

#define USBF_SCSI_MEDIA_SET_CALLBACKS       (USB_STORE_IOCTL_BASE + 0)
#define USBF_SCSI_MEDIA_GET_STATUS          (USB_STORE_IOCTL_BASE + 1)
#define USBF_SCSI_MEDIA_SET_LUN_SIZE        (USB_STORE_IOCTL_BASE + 2)
#define USBF_SCSI_MEDIA_SET_BLOCK_SIZE      (USB_STORE_IOCTL_BASE + 3)
#define USBF_SCSI_SET_INQUIRY_DATA          (USB_STORE_IOCTL_BASE + 4)
#define USBF_SCSI_SET_CAPACITY_DATA         (USB_STORE_IOCTL_BASE + 5)
#define USBF_SCSI_INSERT_MEDIA              (USB_STORE_IOCTL_BASE + 6)
#define USBF_SCSI_REMOVE_MEDIA              (USB_STORE_IOCTL_BASE + 7)

/* =======================  Data Types  ================================ */

/*Read write and connection call backs*/
typedef struct _nu_usbf_ms_callbacks
{
    STATUS (*Read) (
            NU_USBF_SCSI_MEDIA*,
            UINT32,
            UINT8*,
            UINT32);

    STATUS (*Write) (
            NU_USBF_SCSI_MEDIA*,
            UINT32,
            UINT8*,
            UINT32);

    STATUS (*conn_disconn) (
       VOID* handle,
       UINT8 event);
}NU_USBF_MS_CALLBACKS;

/* SCSI Media Control Block. */
struct _nu_usbf_scsi_media
{
    NU_USB                  usb;
    NU_SEMAPHORE            rw_lock;
    NU_USBF_USER_SCSI       *scsi_user;
    NU_USBF_MS_CALLBACKS    *rw_callbacks;
    UINT8                   *inquiry_data;
    UINT8                   *read_capacity_data;
    UINT8                   *temp_data;
    UINT8                   *mode_data;
    UINT8                   *data_buffer;
    UINT32                  block_num;
    UINT32                  remaining_length;
    UINT32                  last_length;
    UINT8                   last_command;
    UINT8                   sense_key;
    UINT8                   asc;
    UINT8                   asc_qual;
    UINT8                   media_state;
    BOOLEAN                 is_formatted;
    BOOLEAN                 media_present;
    DATA_ELEMENT            cs_padding[1];
};

typedef struct _nu_usbf_scsi_media_dispatch
{
    NU_USB_DISPATCH usb_dispatch;

    STATUS (*Connect) (
           const NU_USBF_SCSI_MEDIA  *pcb_scsi_media,
           const NU_USBF_USER_SCSI   *pcb_user_scsi);

    STATUS (*Disconnect) (
           const NU_USBF_SCSI_MEDIA  *pcb_scsi_media,
           const NU_USBF_USER_SCSI   *pcb_user_scsi);

    STATUS (*New_Transfer) (
           NU_USBF_SCSI_MEDIA        *pcb_scsi_media,
           const NU_USBF_USER_SCSI   *pcb_user_scsi,
           UINT8                    **pp_buf_out,
           UINT32                    *p_data_length_out);

    STATUS (*Tx_Done) (
           NU_USBF_SCSI_MEDIA        *pcb_scsi_media,
           const NU_USBF_USER_SCSI   *pcb_user_scsi,
           const UINT8               *p_completed_data,
           const UINT32               completed_data_length,
           UINT8                    **pp_buf_out,
           UINT32                    *p_data_length_out);

    STATUS (*Ready) (
           NU_USBF_SCSI_MEDIA        *pcb_scsi_media,
           const NU_USBF_USER_SCSI   *pcb_user_scsi,
           const UINT8               *p_cmd,
           const UINT16               cmd_len,
           UINT8                    **pp_buf_out,
           UINT32                    *p_data_length_out);

    STATUS (*Sense) (
           NU_USBF_SCSI_MEDIA        *pcb_scsi_media,
           const NU_USBF_USER_SCSI   *pcb_user_scsi,
           const UINT8               *p_cmd,
           const UINT16               cmd_len,
           UINT8                    **pp_buf_out,
           UINT32                    *p_data_length_out);

    STATUS (*Inquiry) (
           NU_USBF_SCSI_MEDIA        *pcb_scsi_media,
           const NU_USBF_USER_SCSI   *pcb_user_scsi,
           const UINT8               *p_cmd,
           const UINT16               cmd_len,
           UINT8                    **pp_buf_out,
           UINT32                    *p_data_length_out);

    STATUS (*Mode_Sense_6) (
           NU_USBF_SCSI_MEDIA        *pcb_scsi_media,
           const NU_USBF_USER_SCSI   *pcb_user_scsi,
           const UINT8               *p_cmd,
           const UINT16               cmd_len,
           UINT8                    **pp_buf_out,
           UINT32                    *p_data_length_out);

    STATUS (*Mode_Sel6) (
           NU_USBF_SCSI_MEDIA        *pcb_scsi_media,
           const NU_USBF_USER_SCSI   *pcb_user_scsi,
           const UINT8               *p_cmd,
           const UINT16               cmd_len,
           UINT8                    **pp_buf_out,
           UINT32                    *p_data_length_out);

    STATUS (*Snd_Diag) (
           NU_USBF_SCSI_MEDIA        *pcb_scsi_media,
           const NU_USBF_USER_SCSI   *pcb_user_scsi,
           const UINT8               *p_cmd,
           const UINT16               cmd_len,
           UINT8                    **pp_buf_out,
           UINT32                    *p_data_length_out);

    STATUS (*Format) (
           NU_USBF_SCSI_MEDIA        *pcb_scsi_media,
           const NU_USBF_USER_SCSI   *pcb_user_scsi,
           const UINT8               *p_cmd,
           const UINT16               cmd_len,
           UINT8                    **pp_buf_out,
           UINT32                    *p_data_length_out);

    STATUS (*Reserve_Unit) (
           NU_USBF_SCSI_MEDIA        *pcb_scsi_media,
           const NU_USBF_USER_SCSI   *pcb_user_scsi,
           const UINT8               *p_cmd,
           const UINT16               cmd_len,
           UINT8                    **pp_buf_out,
           UINT32                    *p_data_length_out);

    STATUS (*Release_Unit) (
           NU_USBF_SCSI_MEDIA        *pcb_scsi_media,
           const NU_USBF_USER_SCSI   *pcb_user_scsi,
           const UINT8               *p_cmd,
           const UINT16               cmd_len,
           UINT8                    **pp_buf_out,
           UINT32                    *p_data_length_out);

    STATUS (*Capacity) (
           NU_USBF_SCSI_MEDIA        *pcb_scsi_media,
           const NU_USBF_USER_SCSI   *pcb_user_scsi,
           const UINT8               *p_cmd,
           const UINT16               cmd_len,
           UINT8                    **pp_buf_out,
           UINT32                    *p_data_length_out);

    STATUS (*Verify) (
           NU_USBF_SCSI_MEDIA  *pcb_scsi_media,
           const NU_USBF_USER_SCSI   *pcb_user_scsi,
           const UINT8               *p_cmd,
           const UINT16               cmd_len,
           UINT8                    **pp_buf_out,
           UINT32                    *p_data_length_out);

    STATUS (*Unknown_SCSI_Command) (
           NU_USBF_SCSI_MEDIA        *pcb_scsi_media,
           const NU_USBF_USER_SCSI   *pcb_user_scsi,
           const UINT8               *p_cmd,
           const UINT16               cmd_len,
           UINT8                    **pp_buf_out,
           UINT32                    *p_data_length_out);

    STATUS (*Reset_Device) (
           NU_USBF_SCSI_MEDIA        *pcb_scsi_media,
           NU_USBF_USER_SCSI         *pcb_user_scsi);

    STATUS (*Read) (
           NU_USBF_SCSI_MEDIA        *pcb_scsi_media,
           UINT32                     block_num,
           UINT8                     *buffer,
           UINT32                     length);

    STATUS (*Write) (
           NU_USBF_SCSI_MEDIA        *pcb_scsi_media,
           UINT32                     block_num,
           UINT8                     *buffer,
           UINT32                     length);

    STATUS (*Mode_Sense_10) (
           NU_USBF_SCSI_MEDIA        *pcb_scsi_media,
           const NU_USBF_USER_SCSI   *pcb_user_scsi,
           const UINT8               *p_cmd,
           const UINT16               cmd_len,
           UINT8                    **pp_buf_out,
           UINT32                    *p_data_length_out);

    STATUS (*Mode_Sel_10) (
           NU_USBF_SCSI_MEDIA        *pcb_scsi_media,
           const NU_USBF_USER_SCSI   *pcb_user_scsi,
           const UINT8               *p_cmd,
           const UINT16               cmd_len,
           UINT8                    **pp_buf_out,
           UINT32                    *p_data_length_out);

    STATUS (*Prevent_Allow) (
           NU_USBF_SCSI_MEDIA        *pcb_scsi_media,
           const NU_USBF_USER_SCSI   *pcb_user_scsi,
           const UINT8               *p_cmd,
           const UINT16               cmd_len,
           UINT8                    **pp_buf_out,
           UINT32                    *p_data_length_out);

    STATUS (*Start_Stop) (
           NU_USBF_SCSI_MEDIA        *pcb_scsi_media,
           const NU_USBF_USER_SCSI   *pcb_user_scsi,
           const UINT8               *p_cmd,
           const UINT16               cmd_len,
           UINT8                    **pp_buf_out,
           UINT32                    *p_data_length_out);

}   NU_USBF_SCSI_MEDIA_DISPATCH;

/* ===================================================================== */

#include "connectivity/nu_usbf_scsi_media_dat.h"

/* ===================================================================== */

#ifdef  __cplusplus
}                                            /* End of C declarations. */
#endif

#endif                                       /*_NU_USBF_SCSI_MEDIA_IMP_H */

/* ======================  End Of File  ================================ */
