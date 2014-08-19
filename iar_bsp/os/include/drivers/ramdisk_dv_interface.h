/*************************************************************************
*
*               Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
* FILE NAME
*
*       ramdisk_dv_interface.h
*                                                                       
* COMPONENT                                                             
*                                                                       
*       RAM Disk Driver
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Configuration and interface defines for RAMDISK device driver
*                                                                       
*************************************************************************/
#ifndef RAMDISK_DV_INTERFACE_H
#define RAMDISK_DV_INTERFACE_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/*** Define Ram Disk DM defines and typedefs ***/ 

#define RD_LABEL    {0x8e,0xe2,0xb8,0xa7,0x60,0xc4,0x48,0xd2,0xb9,0xfd,0xbe,0x4b,0x7d,0xa7,0xc7,0x97}

/* RD Error codes */
#define RD_NO_INSTANCE_AVAILABLE    -1
#define RD_TGT_SETUP_FAILED         -2
#define RD_SESSION_UNAVAILABLE      -3

/* Open Modes */
#define STORAGE_OPEN_MODE           0x1

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

/* Power Event Mask */
#define RD_POWER_EVENT_MASK         0x3

/* Power Base */
#define RD_POWER_BASE               (STORAGE_CMD_BASE + TOTAL_STORAGE_IOCTLS)

/* Power States */
#define RD_OFF                      0
#define RD_ON                       1

/* RD total power states */
#define RD_TOTAL_STATE_COUNT        2

#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

/* Minimum DVFS OP for RD to perform correctly */
#define RD_MIN_DVFS_OP              1

typedef struct  _rd_instance_handle_struct
{
    BOOLEAN           device_in_use;
    CHAR              config_path[REG_MAX_KEY_LENGTH];
    CHAR              dev_name[8];

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    PMI_DEV_HANDLE    pmi_dev;
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */
    VOID              *rd_reserved; 
    DV_DEV_ID         dev_id;
    NU_PARTITION_POOL NUF_RAMDISK_PARTITION;
    UINT8             *rd_pages[NUM_RAMDISK_PAGES];
    UINT16            rd_opencount;
    UINT16            rd_pool_init_completed;
    INT               rd_logicals_opened;
    UINT16            rd_sector_size; /* The size (in bytes) of a physical sector */

} RD_INSTANCE_HANDLE;

typedef struct  _rd_session_handle_struct
{
    UINT32             open_modes;
    RD_INSTANCE_HANDLE *inst_info;

} RD_SESSION_HANDLE;

STATUS  RD_Dv_Register (const CHAR *key, RD_INSTANCE_HANDLE *inst_handle);
STATUS  RD_Dv_Unregister (const CHAR *key, INT startstop, DV_DEV_ID dev_id);
STATUS  RD_Dv_Open (VOID *instance_handle, DV_DEV_LABEL labels_list[],
                     INT labels_cnt, VOID* *session_handle);
STATUS  RD_Dv_Close (VOID *handle_ptr);
STATUS  RD_Dv_Ioctl (VOID *session_handle, INT cmd, VOID *data, INT length);

#ifdef          __cplusplus
}

#endif /* _cplusplus */


#endif /* RAMDISK_DV_INTERFACE_H */
