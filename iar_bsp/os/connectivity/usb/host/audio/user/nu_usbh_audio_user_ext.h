/**************************************************************************
*
*               Copyright 2012  Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
***************************************************************************
***************************************************************************
*
* FILE NAME                                    
*     nu_usbh_audio_user_ext.h                                         
*
* COMPONENT
*     Nucleus USB Host AUDIO user driver.
*
* DESCRIPTION
*     This file contains definitions for external Interfaces exposed by
*     Audio user driver.
*
* DATA STRUCTURES
*     None.
*
* FUNCTIONS
*     None.
*
* DEPENDENCIES
*     nu_usbh_audio_imp.h      Internal Definitions.
*
**************************************************************************/

/* ===================================================================== */
#ifndef _NU_USBH_AUDIO_USER_EXT_H_
#define _NU_USBH_AUDIO_USER_EXT_H_

#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* ===================================================================== */

/* =========================== Include Files =========================== */
/* Making internals visible. */
#include "nu_usbh_audio_user_imp.h"

/* ========================= Functions Prototypes ====================== */

/* Add the prototypes for extra services provided by Audio user driver 
 * here.
 */
STATUS NU_USBH_AUD_USER_Create (
       NU_USBH_AUD_USER          *pcb_user_drvr,
       CHAR                      *p_name,
       NU_AUDH_USER_CONN          Conn_Handler,
       NU_AUDH_USER_DISCON        Discon_Handler,
       NU_MEMORY_POOL            *p_memory_pool);

STATUS _NU_USBH_AUD_USER_Delete (
       VOID                      *pcb_user_drvr);

STATUS _NU_USBH_AUD_USER_Connect( 
       NU_USB_USER               *pcb_user,
       NU_USB_DRVR               *pcb_drvr,
       VOID                      *audio_dev);

STATUS _NU_USBH_AUD_USER_Disconnect(
       NU_USB_USER               *pcb_user_drvr,
       NU_USB_DRVR               *pcb_audio_drvr,
       VOID                      *pcb_device);

STATUS NU_USBH_AUD_USER_Set_FU_Req(
       NU_USBH_AUD_USER_DEV      *aud_usr_dev,
       UINT8                      function,
       UINT8                      ch_type,
       UINT16                     set_value,
       UINT8                      attribute,
       UINT16                     feature_type);

STATUS NU_USBH_AUD_USER_Get_FU_Req(
       NU_USBH_AUD_USER_DEV      *aud_usr_dev,
       UINT8                      function,
       UINT8                      ch_type,
       UINT16                    *get_value,
       UINT8                      attribute,
       UINT16                     feature_type);
       
STATUS NU_USBH_AUD_USER_Get_Vol_Stats(
       NU_USBH_AUD_USER_DEV      *aud_usr_dev,
       UINT8                      function,
       UINT8                      ch_type,
       NU_USBH_AUD_USR_FEATURE_INFO
                                 *feature_info);
       
STATUS NU_USBH_AUD_USER_Mute_Ctrl(
       NU_USBH_AUD_USER_DEV      *aud_usr_dev,
       UINT8                      function,
       UINT8                      ch_type,
       BOOLEAN                    mute);

STATUS NU_USBH_AUD_USER_Adjust_Vol(
       NU_USBH_AUD_USER_DEV      *aud_usr_dev,
       UINT8                      function,
       UINT8                      ch_type,
       UINT16                     vol_value);

STATUS NU_USBH_AUD_USER_Get_Aud_Funcs(
       NU_USBH_AUD_USER_DEV      *aud_usr_dev,
       NU_USBH_AUD_USR_FUNCTIONS *functions);
           
STATUS NU_USBH_AUD_Get_Supported_Ctrls( 
       NU_USBH_AUD_USER_DEV      *aud_usr_dev,
       UINT8                      function,
       UINT8                      ch_type,
       UINT16                    *bitmap);

STATUS NU_USBH_AUD_USER_Init(NU_MEMORY_POOL* pSystem_Memory, NU_MEMORY_POOL* pUncached_System_Memory);

STATUS NU_USBH_AUD_USER_DeInit(VOID *cb);
	
STATUS NU_USBH_AUD_USER_GetHandle ( VOID** handle);

STATUS   NU_USBH_AUD_USER_Register_Cb (NU_USBH_AUD_USER *cb,
                                       NU_AUDH_USER_CONN Conn_Handler,
                                       NU_AUDH_USER_DISCON Discon_Handler);
STATUS nu_os_conn_usb_host_audio_user_init (const CHAR * key, int startstop);

STATUS  NU_USBH_AUDIO_DM_Open (VOID *dev_handle);


STATUS  NU_USBH_AUDIO_DM_Close (VOID* dev_handle);

STATUS  NU_USBH_AUDIO_DM_Read (VOID *session_handle,
                                        VOID *buffer,
                                        UINT32 numbyte,
                                        OFFSET_T byte_offset,
                                        UINT32 *bytes_read_ptr);

STATUS  NU_USBH_AUDIO_DM_Write (VOID *session_handle,
                                        const VOID *buffer,
                                        UINT32 numbyte,
                                        OFFSET_T byte_offset,
                                        UINT32 *bytes_written_ptr);


STATUS  NU_USBH_AUDIO_DM_IOCTL (VOID *session_handle,
                                        INT cmd,
                                        VOID *data,
                                        INT length);

/* ====================Device manager related things==================== */
#define USBH_AUDIO_OPEN_PLAY_SESSION               (USB_AUDIO_IOCTL_BASE + 0)
#define USBH_AUDIO_CLOSE_PLAY_SESSION              (USB_AUDIO_IOCTL_BASE + 1)
#define USBH_AUDIO_OPEN_RECORD_SESSION             (USB_AUDIO_IOCTL_BASE + 2)
#define USBH_AUDIO_CLOSE_RECORD_SESSION            (USB_AUDIO_IOCTL_BASE + 3)
#define USBH_AUDIO_REGISTER_RECORD_CALLBACK        (USB_AUDIO_IOCTL_BASE + 4)
#define USBH_AUDIO_REGISTER_PLAY_CALLBACK          (USB_AUDIO_IOCTL_BASE + 5)
#define USBH_AUDIO_PLAY_SOUND                      (USB_AUDIO_IOCTL_BASE + 6)
#define USBH_AUDIO_RECORD_SOUND                    (USB_AUDIO_IOCTL_BASE + 7)
#define USBH_AUDIO_GET_FUNC_COUNT                  (USB_AUDIO_IOCTL_BASE + 8)
#define USBH_AUDIO_GET_FREQ_COUNT                  (USB_AUDIO_IOCTL_BASE + 9)
#define USBH_AUDIO_GET_FUNC_SETTINGS               (USB_AUDIO_IOCTL_BASE + 10)
#define USBH_AUDIO_REGISTER_NOTIFICATION_CALLBACKS (USB_AUDIO_IOCTL_BASE + 11)
#define USBH_AUDIO_USER_GET_VOL_STATS              (USB_AUDIO_IOCTL_BASE + 12)
#define USBH_AUDIO_USER_MUTE_CTRL                  (USB_AUDIO_IOCTL_BASE + 13)
#define USBH_AUDIO_USER_ADJUST_VOL                 (USB_AUDIO_IOCTL_BASE + 14)
#define USBH_AUDIO_USER_GET_AUD_FUNCS              (USB_AUDIO_IOCTL_BASE + 15)
#define USBH_AUDIO_USER_GET_SUPPORTED_CTRLS        (USB_AUDIO_IOCTL_BASE + 16)
#define USBH_AUDIO_USER_GET_DEV_CB                 (USB_AUDIO_IOCTL_BASE + 17)

typedef struct _nu_usbh_audio_notify_callbacks
{
    NU_AUDH_USER_DISCON disconnect_callback;
    NU_AUDH_USER_CONN   connect_callback;

}NU_USBH_AUDIO_NOTIFY_CALLBACKS;

typedef struct _nu_audio_device_settings
{
    NU_USBH_AUD_DEV   *aud_dev;
    UINT32  sampling_rate;
    UINT16  lock_delay;
    UINT8   channels;
    UINT8   sample_size;
    UINT8   reserved_bits;
    BOOLEAN double_buffering;
    UINT8   function;
    UINT32  index;
    UINT32  func_count;
    UINT32  freq_count;
    BOOLEAN freq_type;
    UINT32  *freq_list;

    
} NU_AUDIO_DEVICE_SETTINGS;

typedef struct _nu_audio_op_param
{
    NU_USBH_AUD_DEV   *aud_dev;
    
    VOID * buffer;

    VOID * loopback_buffer;
    
    UINT32 samples;

    UINT32 max_samples;
    
    UINT32 sampling_rate;
    
    UINT32 event;
    
    UINT8 sample_size;
    
    UINT8 channels;

    UINT8 handle;
	
    DATA_ELEMENT padding[1];

	
} NU_AUDIO_OP_PARAM;

typedef struct _nu_audio_user_device_settings
{
    NU_USBH_AUD_USER_DEV           *aud_user_dev;
    UINT8                          function;
    UINT8                          ch_type;
    BOOLEAN                        mute;
    NU_USBH_AUD_USR_FEATURE_INFO   *vol_info;
    UINT16                         vol_val;
    NU_USBH_AUD_USR_FUNCTIONS      *functions;
    UINT16                         *bitmap;
    DATA_ELEMENT                   padding[1];
    
} NU_AUDIO_USER_DEVICE_SETTINGS;

#endif /* _NU_USBH_AUDIO_USER_EXT_H_ */

