/*************************************************************************
*
*            Copyright 2010 Mentor Graphics Corporation
*                      All Rights Reserved.
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
*       keypad_dv_interface.h
*
*   COMPONENT
*
*       Keypad Driver
*
*   DESCRIPTION
*
*       This file contains data structures and function prototypes of Atmel
*       keypad for integration with Device Manager.
*
*************************************************************************/

#ifndef KEYPAD_DV_INTERFACE_H
#define KEYPAD_DV_INTERFACE_H

/* IOCTL bases */
#define KP_IOCTL_BASE               (DV_IOCTL0 + 1)

/* KP error codes */
#define KP_NOT_REGISTERED           -4

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE

/* IOCTL count */
#define KP_IOCTL_TOTAL    		    1

#define KP_POWER_BASE               (KP_IOCTL_BASE + KP_IOCTL_TOTAL)
#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE)
#define KP_UII_BASE                 (KP_POWER_BASE + POWER_IOCTL_TOTAL)
#endif /* (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_WATCHDOG == NU_TRUE) */
/* KP total power states */
#define KP_TOTAL_STATE_COUNT        2

/* Power States */
#define KP_OFF                      0
#define KP_ON                       1

#endif /* #ifdef CFG_NU_OS_SVCS_PWR_ENABLE */

/* Open Mode */
#define KP_OPEN_MODE                0x1

typedef struct _kp_instance_handle_struct
{
    VOID                *tgt_info;          /* Keypad target info */
    DV_DEV_ID           dev_id;             /* Keypad device id */
    INT                 register_flag;      /* Register flag */
    INT                 device_in_use;
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    PMI_DEV_HANDLE      pmi_dev;
#endif /* #ifdef CFG_NU_OS_SVCS_PWR_ENABLE */
    VOID               *keypad_reserved;
} KP_INSTANCE_HANDLE;

typedef struct _kp_session_handle_struct
{
    UINT32              open_modes;          /* Open modes */
    KP_INSTANCE_HANDLE  *inst_info;          /* Keypad instance handle */

} KP_SESSION_HANDLE;

/* Public function prototypes */
STATUS  	Keypad_Dv_Register(const CHAR * key, KP_INSTANCE_HANDLE *instance_handle);
STATUS 		Keypad_Dv_Unregister(const CHAR * key, INT startstop, DV_DEV_ID dev_id);

#endif /* KEYPAD_DV_INTERFACE_H */
