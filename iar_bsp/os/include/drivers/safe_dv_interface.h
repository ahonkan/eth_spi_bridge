/*************************************************************************/
/*                                                                       */
/*               Copyright 2010 Mentor Graphics Corporation              */
/*                         All Rights Reserved.                          */
/*                                                                       */
/* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  */
/* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   */
/* SUBJECT TO LICENSE TERMS.                                             */
/*                                                                       */
/*************************************************************************/

/*************************************************************************
* FILE NAME
*
*       safe_dv_interface.h
*
* COMPONENT
*
*       SAFE Disk Driver
*
* DESCRIPTION
*
*       Configuration and interface defines for SAFE device driver
*
* DATA STRUCTURES
*
*       SAFE_DEV                            - SAFE device
*       SAFE_TGT                            - SAFE target info
*       SAFE_INSTANCE_HANDLE                - SAFE instance handle
*       SAFE_SESSION_HANDLE                 - SAFE session handle
*
* FUNCTIONS
*
*       None.
*
*************************************************************************/
#ifndef SAFE_DV_INTERFACE_H
#define SAFE_DV_INTERFACE_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#include "storage/wr_api.h"

#define SAFE_LABEL {0x8f,0xe3,0xb9,0xa8,0x61,0xc5,0x49,0xd3,0xba,0xfe,0xbf,0x4c,0x7e,0xa8,0xc8,0x98}

/* Error Codes */
#define NU_SAFE_SESSION_UNAVAILABLE    -1

typedef struct  _safe_instance_handle_struct
{
    CHAR              dev_name[FILE_MAX_DEVICE_NAME];
    CHAR              config_path[REG_MAX_KEY_LENGTH];
    FS_DRVMOUNT       mount_func;
    FS_PHYGETID       phy_func;
    long              (*getmem_func)(FS_PHYGETID);
    BOOLEAN           device_in_use;
    DV_DEV_ID         dev_id;
    UINT16            safe_opencount;
    UINT32            addr_reg;
    UINT32            cmd_reg;
    UINT32            data_reg;
    UINT32            CSH_reg;
    UINT32            CSL_reg;
    UINT32            stat_reg;
    UINT32            CE_bit;
    UINT32            RB_bit;
    VOID              *safe_reserved;
    WR_SAFE_DEV_CB    *safe_dev_cb;
    
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    PMI_DEV_HANDLE      pmi_dev;
#endif

} SAFE_INSTANCE_HANDLE;

typedef struct  _safe_session_handle_struct
{
    UINT32               open_modes;
    UINT32               base_addr;
    UINT32               device_misc;
    SAFE_INSTANCE_HANDLE *inst_info;
    NU_HISR              rx_hisr;
    UINT16               rx_wd_timeout;

} SAFE_SESSION_HANDLE;


/* SAFE Power Base */
#define SAFE_POWER_BASE               (STORAGE_CMD_BASE + TOTAL_STORAGE_IOCTLS)

/* Open Mode */
#define SAFE_OPEN_MODE                0x1

/* Power States */
#define SAFE_OFF                      0
#define SAFE_ON                       1

/* SAFE total power states */
#define SAFE_TOTAL_STATE_COUNT        2


/* Public function prototypes */
STATUS     SAFE_Dv_Register (const CHAR *key,
                           INT startstop,
                           DV_DEV_ID *dev_id,
                           SAFE_INSTANCE_HANDLE *tgt_info);
STATUS     Safe_Dv_Unregister (DV_DEV_ID dev_id);
STATUS     SAFE_Dv_Open (VOID *instance_handle,
                                  DV_DEV_LABEL labels_list[],
                                  INT labels_cnt,
                                  VOID* *session_handle);
STATUS     SAFE_Dv_Close(VOID *handle_ptr);
STATUS     SAFE_Dv_Ioctl(VOID *session_handle,
                                  INT cmd,
                                  VOID *data,
                                  INT length);

#ifdef          __cplusplus
}

#endif /* _cplusplus */


#endif /* SAFE_DV_INTERFACE_H */

