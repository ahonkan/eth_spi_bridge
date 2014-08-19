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
*     nu_usbh_audio_ext.h
*
* COMPONENT
*     Nucleus USB Host AUDIO class driver.
*
* DESCRIPTION
*     This file contains prototypes for external interfaces exposed by
*     Nucleus USB Host AUDIO Class Driver.
*
* DATA STRUCTURES
*     None.
*
* FUNCTIONS
*     None.
*
* DEPENDENCIES
*     nu_usbh_audio_imp.h         Internal definitions.
*
**************************************************************************/

/* ===================================================================== */
#ifndef _NU_USBH_AUD_EXT_H_
#define _NU_USBH_AUD_EXT_H_
/* ===================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================================================================== */

#ifdef __cplusplus
extern "C" {                                 /* C declarations in C++. */
#endif

/* Include files. */
#include "nu_usbh_audio_imp.h"

/* Global defines. */

#define AUDH_QUEUE_SIZE 15

/* Function prototypes. */
STATUS nu_os_conn_usb_host_audio_class_init(CHAR *path, INT compctrl);

STATUS NU_USBH_AUD_Create(
       NU_USBH_AUDIO          *cb_audio_drvr,
       CHAR                   *p_name,
       NU_MEMORY_POOL         *p_mem_pool);

STATUS _NU_USBH_AUD_Delete(
       VOID                   *cb);

STATUS _NU_USBH_AUD_Initialize_Intf(
       NU_USB_DRVR            *drvr_cb,
       NU_USB_STACK           *cb_stack,
       NU_USB_DEVICE          *cb_device,
       NU_USB_INTF            *intf);

STATUS _NU_USBH_AUD_Disconnect(
       NU_USB_DRVR            *pcb_drvr,
       NU_USB_STACK           *pcb_stack,
       NU_USB_DEVICE          *pcb_device);

STATUS NU_USBH_AUD_Feature_Unit_Req(
       NU_USBH_AUD_DEV        *audio_dev,
       NU_USBH_AUD_CTRL_REQ   *ac_req);

STATUS NU_USBH_AUD_Sample_Freq_Req(
       NU_USBH_AUD_DEV        *audio_dev,
       NU_USBH_AUD_EP_REQ     *ac_req,
       UINT8                   function);

STATUS NU_USBH_AUD_Pitch_Ctrl_Req(
       NU_USBH_AUD_DEV        *audio_dev,
       NU_USBH_AUD_EP_REQ     *ac_req,
       UINT8                   function);

STATUS NU_USBH_AUD_Is_Device_Capable(
       NU_USBH_AUD_DEV        *audio_dev,
       UINT8                   function,
       UINT32                  freq,
       UINT8                   no_ch,
       UINT8                   sub_frame_size);

STATUS NU_USBH_AUD_Open_Play_Session(
       NU_USBH_AUD_DEV        *pcb_aud_device,
       UINT8                   ch_count,
       UINT8                   sample_size,
       UINT32                  sample_rate);

STATUS NU_USBH_AUD_Close_Play_Session(
       NU_USBH_AUD_DEV        *pcb_aud_device);

STATUS NU_USBH_AUD_Open_Record_Session(
       NU_USBH_AUD_DEV        *pcb_aud_device,
       UINT8                   ch_count,
       UINT8                   sample_size,
       UINT32                  sample_rate,
       BOOLEAN                 double_buffering);

STATUS NU_USBH_AUD_Close_Record_Session(
       NU_USBH_AUD_DEV        *pcb_aud_device);

STATUS NU_USBH_AUD_Play_Sound(
       NU_USBH_AUD_DEV        *pcb_aud_device,
       UINT8                  *tx_data,
       UINT32                  tx_len,
       NU_USBH_AUD_Data_Callback
                               call_back);

STATUS NU_USBH_AUD_Record_Sound(
       NU_USBH_AUD_DEV        *pcb_aud_device,
       UINT8                  *rx_data,
       UINT32                  rx_len,
       NU_USBH_AUD_Data_Callback
                               call_back);

STATUS NU_AUDH_Find_Play_Function(
       NU_USBH_AUD_DEV        *pcb_aud_device,
       NU_USBH_AUD_FUNC_INFO  *function_info);

STATUS NU_AUDH_Find_Record_Function(
       NU_USBH_AUD_DEV        *pcb_aud_device,
       NU_USBH_AUD_FUNC_INFO  *function_info);

STATUS NU_USBH_AUD_Register_User(
       NU_USBH_AUDIO          *pcb_aud_drvr,
       NU_USBH_USER           *pcb_user_drvr);


STATUS NU_USBH_AUD_GetHandle(VOID  **handle);

STATUS NU_USBH_AUD_Get_Function_Count(NU_USBH_AUD_DEV *pcb_aud_device,
                                      UINT8            function,
                                      UINT32 *         pCount);

STATUS NU_USBH_AUD_Get_Freqs_Count(NU_USBH_AUD_DEV *pcb_aud_device,
                                   UINT8            function,
                                   UINT32           function_index,
                                   UINT32 *         pCount);

STATUS NU_USBH_AUD_Get_Func_Settings(NU_USBH_AUD_DEV *pcb_aud_device,
                                   UINT8            function,
                                   UINT32           function_index,
                                   BOOLEAN *        pFreqType,
                                   UINT32 *         pFreqList,
                                   UINT32 *         pFreqCount,
                                   UINT8 *          pChnlCount,
                                   UINT8 *          pSmplSize,
                                   UINT16 *         pLockDelay,
                                   UINT8 *          pResBits);

#ifdef __cplusplus
    }                                        /* C declarations in C++. */
#endif

#endif
