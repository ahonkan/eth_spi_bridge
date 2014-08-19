/*************************************************************************/
/*                                                                       */
/*               Copyright 2013 Mentor Graphics Corporation              */
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
*       rtc_dv_interface.h
*
* COMPONENT
*
*       RTC Driver
*
* DESCRIPTION
*
*       Configuration and interface defines for RTC device driver
*
* DATA STRUCTURES
*
*       RTC_INSTANCE_HANDLE                - RTC instance handle
*       RTC_SESSION_HANDLE                 - RTC session handle
*
*************************************************************************/
#ifndef RTC_DV_INTERFACE_H
#define RTC_DV_INTERFACE_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#define RTC_LABEL {0x8F,0xE3,0xB9,0xA8,0x61,0xC5,0x49,0xD3,0xBA,0xFE,0xBF,0x4C,0x7E,0xA8,0xC8,0x98}

/* Error Codes */
#define NU_RTC_SESSION_UNAVAILABLE  -1
#define RTC_DEV_IN_USE              -2

/* CONFIGURABLE */
#define RTC_MODE_IOCTL_BASE         (DV_IOCTL0+1)
#define RTC_ENABLE_DEVICE           3
#define RTC_DISABLE_DEVICE          4

typedef struct  _rtc_instance_handle_struct
{
    BOOLEAN           device_in_use;
    DV_DEV_ID         dev_id;
    UINT32            rtc_io_addr;
    VOID              (*setup_func)(VOID);
    VOID              *rtc_reserved;
} RTC_INSTANCE_HANDLE;

typedef struct  _rtc_session_handle_struct
{
    UINT32               open_mode;
    RTC_INSTANCE_HANDLE *inst_ptr;
} RTC_SESSION_HANDLE;

/* Open Modes */
#define RTC_OPEN_MODE               (0x1)

/* Public function prototypes */
STATUS  RTC_Dv_Register(const CHAR * key, RTC_INSTANCE_HANDLE *inst_ptr);
STATUS  RTC_Dv_Unregister(const CHAR * key, INT startstop, DV_DEV_ID dev_id);
STATUS  RTC_Dv_Open(VOID *inst_ptr, DV_DEV_LABEL label_list[], INT label_cnt, VOID **session_handle);
STATUS  RTC_Dv_Close(VOID *sess_handle);
STATUS  RTC_Dv_Ioctl(VOID *sess_ptr, INT ioctl_cmd, VOID *ioctl_data, INT length);


#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* RTC_DV_INTERFACE_H */

