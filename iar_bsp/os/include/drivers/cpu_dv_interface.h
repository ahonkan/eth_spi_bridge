/*************************************************************************
*
*            Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILE NAME
*
*        cpu_dv_interface.h
*
*   COMPONENT
*
*        CPU driver
*
*   DESCRIPTION
*
*        Contains data structures and prototypes of the CPU driver
*
*   DATA STRUCTURES
*
*        None
*
*   DEPENDENCIES
*
*        cpu_idle.h
*        cpu_dvfs.h
*
*************************************************************************/
#ifndef CPU_DV_INTERFACE_H
#define CPU_DV_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Values for OP indexing */
#define CPU_MAX_OP_COUNT            10
#define CPU_OP_ARRAY_SIZE           (CPU_MAX_OP_COUNT + 1)
#define CPU_STARTUP_OP_INDEX        CPU_MAX_OP_COUNT

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

STATUS CPU_Dv_Register(const CHAR * key, VOID *cpu_instance);
STATUS CPU_Dv_Unregister(const CHAR *key, INT startstop, DV_DEV_ID dev_id);
STATUS CPU_Dv_Open(VOID *instance_handle, DV_DEV_LABEL label_list[], INT label_cnt, VOID **session_handle);
STATUS CPU_Dv_Close(VOID *session_handle);
STATUS CPU_Dv_Ioctl(VOID *session_handle_ptr, INT ioctl_num, VOID *ioctl_data_ptr, INT ioctl_data_len);

#define CPU_IDLE_IOCTL_BASE         (DV_IOCTL0 + 1)
#define CPU_DVFS_IOCTL_BASE         (CPU_IDLE_IOCTL_BASE + CPU_IDLE_IOCTL_TOTAL)
#define CPU_DRAM_IOCTL_BASE         (CPU_DVFS_IOCTL_BASE + CPU_DVFS_IOCTL_TOTAL)
#define CPU_SELFREFRESH_IOCTL_BASE  (CPU_DRAM_IOCTL_BASE + CPU_DRAM_IOCTL_TOTAL)

/* Macro to make accessing the startup OP easier to read */
#define CPU_GET_OP_INDEX(index)     ((index == PM_STARTUP_OP) ? CPU_STARTUP_OP_INDEX : index)

/* Open Mode */
#define CPU_DRAM_CTRL_OPEN_MODE      0x1

typedef struct _cpu_session_handle_struct
{
    UINT32                 open_modes;
    UINT32                 dma_id;
} CPU_SESSION_HANDLE;

#endif  /* if CFG_NU_OS_SVCS_PWR_ENABLE IS define */

#ifdef __cplusplus
}
#endif

#endif /* CPU_DV_INTERFACE_H */
