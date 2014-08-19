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
*       nu_usbf_user_scsi_imp.h
*
* COMPONENT
*
*       Nucleus USB Function Mass Storage class driver.
*
* DESCRIPTION
*
*       This file contains the internal declarations for the mass storage
*       SCSI Media container driver.
*
* DATA STRUCTURES
*
*       NU_USBF_USER_SCSI                   SCSI media control block.
*       NU_USBF_USER_SCSI_DISPATCH          SCSI media dispatch table.
*       USBF_USER_SCSI_DISK                 SCSI disk representation.
*       NU_USBF_USER_SCSI_CMD               SCSI command representation
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       nu_usbf_user_ms_ext.h               Function user mass storage
*                                           external interface.
*       nu_usbf_user_scsi_cfg.h             Configurations for SCSI.
*
**************************************************************************/

#ifndef _NU_USBF_USER_SCSI_IMP_H

#ifdef  __cplusplus
extern  "C" {                                /* C declarations in C++. */
#endif

#define _NU_USBF_USER_SCSI_IMP_H

/* ==============  USB Include Files  ================================== */

#include "connectivity/nu_usbf_user_ms_ext.h"
#include "connectivity/nu_usbf_user_scsi_cfg.h"

/* =====================  #defines  ==================================== */

#define USBF_SCSI_MAX_COMMANDS               25

/* Mass storage SCSI sub class. */
#define USB_SPEC_MSF_SCSI_SUBCLASS           0x06

/* Mass storage commands as defined by the specification. */
#define USB_SPEC_MSF_TEST_UNIT_READY         0x00
#define USB_SPEC_MSF_REQUEST_SENSE           0x03
#define USB_SPEC_MSF_INQUIRY                 0x12

#define USB_SPEC_MSF_MODE_SELECT_6           0x15
#define USB_SPEC_MSF_MODE_SELECT_10          0x55

#define USB_SPEC_MSF_MODE_SENSE_6            0x1A
#define USB_SPEC_MSF_MODE_SENSE_10           0x5A

#define USB_SPEC_MSF_SEND_DIAGNOSTIC         0x1D
#define USB_SPEC_MSF_FORMAT_UNIT             0x04

#define USB_SPEC_MSF_READ_6                  0x08
#define USB_SPEC_MSF_READ_12                 0xA8

#define USB_SPEC_MSF_PREVENT_ALLOW           0x1E
#define USB_SPEC_MSF_START_STOP              0x1B

#define USB_SPEC_MSF_WRITE_6                 0x0A
#define USB_SPEC_MSF_WRITE_12                0xAA
#define USB_SPEC_MSF_RESERVE_UNIT            0x16
#define USB_SPEC_MSF_RELEASE_UNIT            0x17
#define USB_SPEC_MSF_READ_CAPACITY           0x25
#define USB_SPEC_MSF_READ_10                 0x28
#define USB_SPEC_MSF_WRITE_10                0x2A
#define USB_SPEC_MSF_23                      0x23
#define USB_SPEC_MSF_VERIFY                  0x2F
#define USB_SPEC_MSF_CMD_LUN_BYTE            0x1
#define USB_SPEC_MSF_NO_DATA_STAGE           0xFF

#ifndef USBF_EVENT_DISCONNECT
#define USBF_EVENT_DISCONNECT                0x0004
#endif

#ifndef USBF_EVENT_CONNECT
#define USBF_EVENT_CONNECT                   0x0008
#endif

/* =====================  Data Types  ================================== */

typedef struct _nu_usbf_user_scsi   NU_USBF_USER_SCSI;

typedef struct _nu_usbf_scsi_media  NU_USBF_SCSI_MEDIA;

/* SCSI Command Processing function. */
typedef STATUS (*USBF_SCSI_CMD_WORKER) (
        NU_USBF_SCSI_MEDIA         *cb,
        const NU_USBF_USER_SCSI    *scsi,
        const UINT8                *cmd,
        const UINT16                cmd_len,
        UINT8                     **buf_out,
        UINT32                     *data_length_out);

/* SCSI command definition. */
typedef struct _usbf_scsi_cmd
{
    UINT16                          opcode;
    UINT16                          direction;
    USBF_SCSI_CMD_WORKER            handler;

}   NU_USBF_USER_SCSI_CMD;

/* Each SCSI disk representation. */
typedef struct _usbf_user_scsi_disk
{
    VOID                           *handle;
    NU_USB_DRVR                    *drvr;
    NU_USBF_SCSI_MEDIA             *media;

}   USBF_USER_SCSI_DISK;

/* User SCSI Control Block. */
struct _nu_usbf_user_scsi
{
    NU_USBF_USER_MS                 parent;
    USBF_USER_SCSI_DISK             disks[NU_USBF_USER_SCSI_NUM_DISKS];
    NU_USBF_USER_SCSI_CMD          *scsi_command_list;
    VOID                           *handle;
    UINT8                           num_cmds;
    UINT8                           num_disks;
    UINT8                           curr_comand_lun;
	UINT8                           max_lun;
};

typedef struct _nu_usbf_user_scsi_dispatch
{
    NU_USBF_USER_MS_DISPATCH file_drvr_dispatch;

    STATUS (*Register_Media) (
           NU_USBF_USER_SCSI        *cb,
           NU_USBF_SCSI_MEDIA       *media);

    STATUS (*Deregister_Media) (
           NU_USBF_USER_SCSI        *cb,
           const NU_USBF_SCSI_MEDIA *media);

}   NU_USBF_USER_SCSI_DISPATCH;

/* ===================================================================== */

#include "connectivity/nu_usbf_user_scsi_dat.h"

/* ===================================================================== */

#ifdef  __cplusplus
}                                            /* End of C declarations. */
#endif

#endif                                       /* _NU_USBF_USER_SCSI_IMP_H */

/* =======================  End Of File  =============================== */
