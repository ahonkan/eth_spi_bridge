/*************************************************************************
*
*               Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
*   FILE NAME
*
*       touchpanel_dv_interface.h
*
*   COMPONENT
*
*       TOUCHPANEL                          - Touchpanel Library
*
*   DESCRIPTION
*
*       This file contains constant definitions and function declarations
*       for the Touchpanel Library Driver module.
*
*************************************************************************/
#ifndef TOUCHPANEL_DV_INTERFACE_H
#define TOUCHPANEL_DV_INTERFACE_H


#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

/* Max number of touchpanel labels */
#define TOUCHPANEL_MAX_LABEL_CNT          5

/* Touchpanel error codes */
#define TOUCHPANEL_REGISTRY_ERROR        -1
#define TOUCHPANEL_ALREADY_OPEN          -2

/*********************************/
/* Data Structures               */
/*********************************/
typedef struct _touchpanel_instance_handle_struct
{
    UINT32          x_intercept;            /* X intercept for target to screen coordinate conversion */
    UINT32          y_intercept;            /* Y intercept for target to screen coordinate conversion */
    UINT32          x_slope;                /* X slope for target to screen coordinate conversion */
    UINT32          y_slope;                /* Y slope for target to screen coordinate conversion */
    UINT32          irq_vector_id;          /* Irq vector ID */
    UINT32          irq_bitmask;            /* Irq bitmask */
    UINT32          irq_data;               /* Irq data */
    UINT32          interface_base;         /* Interface chip select base address */
    UINT32          interface_baud_rate;    /* Baud rate of interface */  
    UINT32          interface_address;      /* Slave address /Chip Select for interface */
    UINT8           interface_mode;         /* Interrupt or Polling. Required if I2C is the interface */
    CHAR            reg_path[REG_MAX_KEY_LENGTH];   /* Registration path */
    DV_DEV_LABEL    interface_label;                /* Interface label */
    BOOLEAN         dev_in_use;             /* Flag to indicate device in use status */
    UINT16          wd_timeout;             /* Watchdog timeout */
    DV_DEV_ID       dev_id;                 /* Touchpanel device id */
    VOID            *touchpanel_reserved;
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    PMI_DEV_HANDLE  pmi_dev;
#endif

} TOUCHPANEL_INSTANCE_HANDLE;

typedef struct _touchpanel_session_handle_struct
{
    UINT32                      open_modes;
    TOUCHPANEL_INSTANCE_HANDLE *inst_info;

} TOUCHPANEL_SESSION_HANDLE;

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE))

/* Hibernate-Resume structure */
typedef struct  _touchpanel_hib_res_struct
{
    PM_STATE_ID           device_state;
    DV_DEV_ID             device_id;
    VOID                  *device_reserved_1;

} TOUCHPANEL_HIB_RES;
#endif 

/*********************************/
/* MACROS                        */
/*********************************/
#define TOUCHPANEL_OPEN_MODE            0x1

/*********************************/
/*   POWER STATE MACROS          */
/*********************************/
/* Power States */
#define TOUCHPANEL_OFF                  0
#define TOUCHPANEL_ON                   1

/* Touchpanel total power states */
#define TOUCHPANEL_TOTAL_STATE_COUNT    2

/***********************/
/*  Touchpanel IOCTL Base  */
/***********************/
#define IOCTL_TOUCHPANEL_BASE           (DV_IOCTL0+1)

/***********************/
/*  Touchpanel IOCTL commands  */
/***********************/
#define TOUCHPANEL_PWR_HIB_RESTORE      0

/* IOCTL count */
#define TOUCHPANEL_IOCTL_TOTAL          1

/***********************/
/* Touchpanel Power Base   */
/***********************/
#define TOUCHPANEL_POWER_BASE           (IOCTL_TOUCHPANEL_BASE + TOUCHPANEL_IOCTL_TOTAL)

/***********************/
/* Touchpanel UII Base     */
/***********************/
#define TOUCHPANEL_UII_BASE             (TOUCHPANEL_POWER_BASE + POWER_IOCTL_TOTAL)

/*********************************/
/* FUNCTION PROTOTYPES           */
/*********************************/
STATUS  Touchpanel_Dv_Register(const CHAR *key, INT startstop, DV_DEV_ID *dev_id,
                               TOUCHPANEL_INSTANCE_HANDLE *instance_handle);
STATUS  Touchpanel_Dv_Unregister(const CHAR *key, INT startstop, DV_DEV_ID dev_id);
STATUS  Touchpanel_Dv_Open(VOID *instance_handle, DV_DEV_LABEL label_list[],
                           INT label_cnt, VOID* *session_handle);
STATUS  Touchpanel_Dv_Close(VOID *session_handle);
STATUS  Touchpanel_Dv_Ioctl(VOID *session_handle, INT ioctl_cmd, VOID *data, INT length);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* !TOUCHPANEL_DV_INTERFACE_H */
