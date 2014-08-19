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
*     nu_usbh_audio_cfg.h
*
* COMPONENT
*     Nucleus USB Host AUDIO class driver.
*
* DESCRIPTION
*     This file contains configurable macros of Nucleus USB Host AUDIO
*     Class Driver.
*
* DATA STRUCTURES
*     None.
*
* FUNCTIONS
*     None.
*
* DEPENDENCIES
*     None.
*
**************************************************************************/
/* Maximum possible entity(terminal / unit) ids, An audio device can have
 * Its value must be equal to the maximum id from all entities present in
 * an Audio Device. Its value can range from 0-255.
 */
#define NU_AUDH_MAX_POSSIBLE_ENTITIES      CFG_NU_OS_CONN_USB_HOST_AUDIO_CLASS_MAX_POSSIBLE_ENTITIES

/* Maximum possible streaming interfaces present in an audio control
 * interface. Its value can range from 1 to 255. Its value cannot be
 * greater than NU_USB_MAX_INTERFACES macro, defined in nu_usb_stack_cfg.h
 * file.
 */
#define NU_AUDH_MAX_STREAMING_INFS         CFG_NU_OS_CONN_USB_HOST_AUDIO_CLASS_MAX_STREAMING_INFS

/* Priority of playing task. This task is created whenever user opens a
 * playing session and is deleted when user closes session. User should
 * define this priority according to his/her application.
 */
#define NU_AUDH_PLAY_TASK_PRIORITY          CFG_NU_OS_CONN_USB_HOST_AUDIO_CLASS_PLAY_TASK_PRIO
#define NU_AUDH_PLAY_TIME_SLICE             CFG_NU_OS_CONN_USB_HOST_AUDIO_CLASS_PLAY_TASK_TIME_SLICE

/* Priority of recording task. This task is created whenever user opens
 * recording session and is deleted when user closes session. User should
 * define this priority according to her/his application.
 */
#define NU_AUDH_RECORD_TASK_PRIORITY        CFG_NU_OS_CONN_USB_HOST_AUDIO_CLASS_RECORD_TASK_PRIO
#define NU_AUDH_RECORD_TIME_SLICE           CFG_NU_OS_CONN_USB_HOST_AUDIO_CLASS_RECORD_TASK_TIME_SLICE
