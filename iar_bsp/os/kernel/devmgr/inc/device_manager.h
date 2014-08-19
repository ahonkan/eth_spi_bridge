/*************************************************************************
*
*              Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   DESCRIPTION
*
*       This file contains data structure definitions and constants for
*       the Device Manager component.
*
*************************************************************************/

/* Check to see if the file has been included already.  */

#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#ifdef          __cplusplus

extern  "C" {                               /* C declarations in C++ */

#endif /* _cplusplus */

/* Maximum number of open sessions a device can have */
#define DV_MAX_DEV_SESSION_CNT      CFG_NU_OS_KERN_DEVMGR_MAX_DEV_SESSION_CNT

/* Enable / Disable error checking in device manager. */
#define DV_ERR_CHECK_ENABLE         CFG_NU_OS_KERN_DEVMGR_ERR_CHECK_ENABLE

/* Define session state flag values */
#define DV_SES_CLOSED               0
#define DV_SES_LOCKED               1
#define DV_SES_OPEN                 2

/* Define Register Event Change bit masks */
#define DV_DEV_REGISTERED_BIT       ((UNSIGNED)1)
#define DV_DEV_UNREGISTERED_BIT     ((UNSIGNED)2)
#define DV_DEV_SCAN_REGISTRY_BIT    ((UNSIGNED)4)
#define DV_REG_CHANGE_BIT_MASK      (DV_DEV_REGISTERED_BIT | DV_DEV_UNREGISTERED_BIT)

/* Number of bits to allocate for storing the device's registry index */
#define DV_REG_INDEX_BIT_CNT        10

/* Number of bits to allocate for storing the device's session index */
#define DV_SES_INDEX_BIT_CNT        10

/* Number of bits to allocate for storing the device id reuse count */
#define DV_REUSE_BIT_CNT            (31 - DV_REG_INDEX_BIT_CNT - DV_SES_INDEX_BIT_CNT)

/* Number of bits to allocate for storing the device id */
#define DV_DEV_ID_BIT_CNT           (DV_REUSE_BIT_CNT + DV_REG_INDEX_BIT_CNT)

/* Define a macro to create a device id */
#define DV_CREATE_DEV_ID(reuse_cnt, reg_index) \
                ((DV_DEV_ID)(((UINT32)reuse_cnt << DV_REG_INDEX_BIT_CNT) | (UINT32)reg_index))

/* Define a macro to create a device handle */
#define DV_CREATE_DEV_HANDLE(dev_id, session_index) \
                ((DV_DEV_HANDLE)(((UINT32)dev_id << DV_SES_INDEX_BIT_CNT) | (UINT32)session_index))

/* Define a macro to get a device ID */
#define DV_GET_DEV_ID(dev_handle) ((DV_DEV_ID)(((UINT32)dev_handle & \
                (((1<<DV_DEV_ID_BIT_CNT)-1)<<DV_SES_INDEX_BIT_CNT))>>DV_SES_INDEX_BIT_CNT))

/* Define a macro to get the index into the device's session array */
#define DV_GET_SES_INDEX(dev_handle) ((INT32)((UINT32)dev_handle & \
                ((1<<DV_SES_INDEX_BIT_CNT)-1)))

/* Define a macro to get the device id reuse count */
#define DV_GET_REUSE_CNT(dev_id) ((INT32)(((UINT32)dev_id & \
                (((1<<DV_REUSE_BIT_CNT)-1)<<DV_REG_INDEX_BIT_CNT))>>DV_REG_INDEX_BIT_CNT))

/* Define a macro to get the index into the device registry array */
#define DV_GET_REG_INDEX(dev_id) ((INT32)((UINT32)dev_id & \
                ((1<<DV_REG_INDEX_BIT_CNT)-1)))

/* Validate max session entries per device count will fit within the number of bits allocated */
#if (DV_MAX_DEV_SESSION_CNT > ((1<<DV_SES_INDEX_BIT_CNT)-1))
#error CFG_NU_OS_KERN_DEVMGR_MAX_DEV_SESSION_CNT too large
#endif

/* Validate we have at least one bit for the reuse count */
#if (DV_REG_INDEX_BIT_CNT+DV_SES_INDEX_BIT_CNT > 30)
#error DV_REG_INDEX_BIT_CNT + DV_SES_INDEX_BIT_CNT > 30
#endif

/* DM Initialized flag value = "DVMG" */
#define DV_INITIALIZED              0x44564D47

/* Define device discovery task related parameters. */
#define DV_DEVICE_DISCOVERY_TASK_STACK_SIZE     CFG_NU_OS_KERN_DEVMGR_DISCOVERY_TASK_STACK_SIZE
#define DV_DEVICE_DISCOVERY_TASK_PRIORITY       (3)
#define DV_DEVICE_DISCOVERY_TASK_TIMESLICE      (0)

/* Device Registry definition */
typedef struct _dv_dev_registry_struct
{
    INT             entry_active_flag;
    DV_DEV_LABEL    label_list[DV_MAX_DEV_LABEL_CNT];
    INT             label_cnt;
    VOID*           instance_handle;
    INT             active_open_cnt;
    INT32           next_ses_index;
    INT32           reuse_cnt;
    struct _session_struct
    {
        INT     state_flag;
        VOID*   handle;
        INT     active_cmds_cnt;
    } session[DV_MAX_DEV_SESSION_CNT];

    NU_EVENT_GROUP         active_cmds_event;
    DV_DRV_OPEN_FUNCTION   drv_open_ptr;
    DV_DRV_CLOSE_FUNCTION  drv_close_ptr;
    DV_DRV_READ_FUNCTION   drv_read_ptr;
    DV_DRV_WRITE_FUNCTION  drv_write_ptr;
    DV_DRV_IOCTL_FUNCTION  drv_ioctl_ptr;

} DV_DEV_REGISTRY;

/* Internal function header. */
STATUS DVC_Reg_Change_Search (DV_APP_REGISTRY_CHANGE *app_struct_ptr);
STATUS DVS_Dev_Handles_Get (DV_DEV_ID dev_id, DV_DEV_HANDLE dev_handle_list[],
                            INT * dev_handle_cnt_ptr);

#ifdef          __cplusplus

/* End of C declarations */
}

#endif  /* __cplusplus */

#endif
