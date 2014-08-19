/*************************************************************************
*
*               Copyright 2012 Mentor Graphics Corporation
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
*       display_dv_interface.h
*
*   COMPONENT
*
*       DISPLAY                              - Display Library
*
*   DESCRIPTION
*
*       This file contains constant definitions and function declarations
*       for the Display Library Driver module.
*
*************************************************************************/
#ifndef DISPLAY_DV_INTERFACE_H
#define DISPLAY_DV_INTERFACE_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

/* DISPLAY instance handle structure. */
typedef struct _display_instance_handle_struct
{
    VOID            *display_framebuffer_addr;
    VOID            *display_tgt_ptr;
    BOOLEAN         device_in_use;
    CHAR            mw_config_path[100];
    DV_DEV_ID       dev_id;
    VOID            *display_reserved;
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    PMI_DEV_HANDLE  pmi_dev;
#endif /* CFG_NU_OS_SVCS_PWR_ENABLE */

} DISPLAY_INSTANCE_HANDLE;

/* DISPLAY session handle structure. */
typedef struct _display_session_handle_struct
{
    UINT32                     open_modes;
    DISPLAY_INSTANCE_HANDLE    *inst_info;

} DISPLAY_SESSION_HANDLE;

#if (defined(CFG_NU_OS_SVCS_PWR_ENABLE) && (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE))

/* Hibernate-Resume structure */
typedef struct  _display_hib_res_struct
{
    PM_STATE_ID           device_state;
    DV_DEV_ID             device_id;
    VOID                  *device_reserved_1;

} DISPLAY_HIB_RES;
#endif 

/* DISPLAY error codes. */
#define     DISPLAY_NO_INSTANCE_AVAILABLE   -1
#define     DISPLAY_SESSION_NOT_FOUND       -2
#define     DISPLAY_REGISTRY_ERROR          -3
#define     DISPLAY_NOT_REGISTERED          -4
#define     DISPLAY_PM_ERROR                -5

/* Open modes */
#define DISPLAY_OPEN_MODE                   0x1

/*********************************/
/* FUNCTION PROTOTYPES           */
/*********************************/
STATUS  Display_Dv_Register(const CHAR *key, DISPLAY_INSTANCE_HANDLE *inst_handle);
STATUS  Display_Dv_Unregister(const CHAR * key, INT startstop, DV_DEV_ID dev_id);
STATUS  Display_Dv_Open (VOID *instance_handle, DV_DEV_LABEL labels_list[],
                        INT labels_cnt, VOID **session_handle);
STATUS  Display_Dv_Close(VOID *sess_handle);
STATUS  Display_Dv_Ioctl(VOID *session_ptr, INT ioctl_cmd, VOID *data, INT length);


#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* !DISPLAY_DV_INTERFACE_H */

