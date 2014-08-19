/*************************************************************************
*
*            Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
* FILE NAME
*
*       sd_disk.h
*
* COMPONENT
*
*       Nucleus SDIO FILE Function Driver - Application layer
*
* DESCRIPTION
*
*       This file contains routines of Nucleus SDIO FILE Function
*       Driver for Nucleus FILE interface.
*
* DATA STRUCTURES
*
*       SD_PHYS_DISK_CTRL
*       SD_DEV
*       SD_INSTANCE_HANDLE
*       SD_TGT
*
* FUNCTIONS
*
*       None
*
* DEPENDENCIES
*
*       sd_cfg.h
*       pcdisk.h
*       power_core.h
*
*************************************************************************/
#ifndef _SD_DISK_H
#define _SD_DISK_H

#include "kernel/nu_kernel.h"
#include "drivers/nu_drivers.h"
#include "services/nu_services.h"
#include "drivers/sd_cfg.h"         /* Driver configurations           */

#ifdef  CFG_NU_OS_STOR_ENABLE
#include "storage/pcdisk.h"         /* File definitions                */
#else
#include "drivers/sd_stor_defs.h"   /* File definitions required by SD */
#endif

#ifdef          __cplusplus
    extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

/* Define SD Physical Disk Control structure. */
#define PHYSICAL_DEV_NAME_LEN   3
typedef struct _sd_phys_disk_ctrl_
{
    /* System area start */
    CHAR                    name[FILE_MAX_DEVICE_NAME + 1];
    UINT16                  fs_dh;                  /* File system drive handle.                */
    UINT8                   open_modes;             /* Current open modes of the device         */
    UINT8                   open_count;             /* Physical open count.                     */
    UINT8                   media_type;             /* Media Type.                              */
    UINT8                   media_wp;               /* Media Write Protect.                     */

    UINT8                   secure_mode;            /* Current security mode, 1=In secure mode. ????? */
    UINT8                   bus_widths;             /* Current Bus Widths. 0 = 1bit 2 = 4bit.   ?????*/
    UINT16                  bytspblk;               /* Bytes per Block.                         ?????*/
    UINT16                  multiblk;               /* Multi Sector Transfers.                  ?????*/
    UINT32                  protected_area_size;    /* size of protected area in bytes.         ?????*/
    UINT32                  rca;                    /* RCA                                      ?????*/
    UINT32                  ocr;                    /* OCR                                      ?????*/

    /* From CARD CSD information. */
    UINT8                   read_bl_len;            /* Maximum read data block length.  */
    UINT8                   c_size_mult;            /* Device size multiplier.          */
    UINT8                   tran_speed;             /* Maximum data transfer rate.      */
    UINT32                  card_cap;               /* Device size in 512 bytes blocks. */
    UINT8                   structure_ver;          /* CSD structure version.           */

    UINT8                   user_address;           /* Current data transfer buffer in use.     */
    struct _SDDEVICE        *p_SDDevice;            /* Pointer to SDDEVICE struct representing
                                                       inserted card. It has a reference to host controller
                                                       to which this device is connected.*/
    struct _SDFUNCTION      *p_SDFunction;          /* Pointer to SDFUNCTION struct representing
                                                       inserted card. It has a reference to function driver
                                                       to which this device is connected.*/
    struct _sd_phys_disk_ctrl_ *previous;
    struct _sd_phys_disk_ctrl_ *next;

}SD_PHYS_DISK_CTRL;

/*** Define SD DM defines and typedefs ***/

#define SD_LABEL    {0x0d,0x85,0x44,0x44,0x5a,0xc5,0x46,0xa1,0x8f,0x7c,0x3c,0xba,0x20,0x69,0x2c,0xb2}

#define SD_MEDIA_INFO_IOCTL         0x01
#define TOTAL_SD_MEDIA_IOCTLS       1

/* SD Error codes */
#define SD_NO_INSTANCE_AVAILABLE    -1
#define SD_TGT_SETUP_FAILED         -2
#define SD_SESSION_UNAVAILABLE      -3

typedef struct  _sd_dev_struct
{
    BOOLEAN           device_in_use;
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE    
    PM_MIN_REQ_HANDLE min_op_request_handle;    /* Min OP at which SD can work correctly */
    PM_STATE_ID       current_state;            /* Current state of SD device */
#endif    
    NU_EVENT_GROUP    *write_event_ptr;
    NU_EVENT_GROUP    *read_event_ptr;

} SD_DEV;

typedef struct  _sd_instance_handle_struct
{
    CHAR              config_path[REG_MAX_KEY_LENGTH];
    SD_DEV            dev_info;
    DV_DEV_ID         dev_id;
    SD_PHYS_DISK_CTRL pdcb;

} SD_INSTANCE_HANDLE;

typedef struct _sd_tgt_struct
{
    CHAR                config_path[REG_MAX_KEY_LENGTH];

} SD_TGT;

STATUS      SD_Register (SD_PHYS_DISK_CTRL *pdcb);
STATUS      SD_Unregister (DV_DEV_ID dev_id);

#ifdef          __cplusplus
    }
#endif /* _cplusplus */

#endif   /* ! _SD_DISK_H */
