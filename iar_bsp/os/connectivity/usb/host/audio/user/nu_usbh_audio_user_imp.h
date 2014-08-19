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
*     nu_usbh_audio_user_imp.h                                         
*
* COMPONENT
*     Nucleus USB Host AUDIO user driver.
*
* DESCRIPTION
*     This file contains the control block and other internal data
*     structures and definitions for Audio user driver.
*
* DATA STRUCTURES
*     NU_USBH_AUD_USER_DISPATCH    user dispatch table.
*     NU_USBH_AUD_USER             user control block.
*
* FUNCTIONS
*     None.
*
* DEPENDENCIES
*     nu_usbh_user_ext.h
*     nu_usbh_audio_ext.h
*
**************************************************************************/

/* ===================================================================== */
#ifndef _NU_USBH_AUD_USER_IMP_H_
#define _NU_USBH_AUD_USER_IMP_H_

#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* ===================================================================== */
/* =========================== Include Files =========================== */
/* Specializing nu_usbh_user driver. */
#include "connectivity/nu_usbh_user_ext.h"
//#include "nu_usbh_audio_user_imp.h"

/* ===================================================================== */

/* =========================== Control Blocks ========================== */
#define NU_AUDH_USR_CURR_FEATURE     0x01
#define NU_AUDH_USR_MIN_FEATURE      0x02
#define NU_AUDH_USR_MAX_FEATURE      0x03
#define NU_AUDH_USR_RES_FEATURE      0x04

/* Validation macro. */
#define NU_USBH_AUD_USR_IS_STRM_TERM(a) ((a == 0x0101) || (a == 0x01ff))

/* Audio user volume info structure. */ 
typedef struct _feature_info
{
    UINT16     min_attribute;                /* Minimum attribute. */
    UINT16     max_attribute;                /* Maximum attribute. */
    UINT16     curr_attribute;               /* Current attribute. */
    UINT16     res_attribute;                /* Resolution of attribute. */
}
NU_USBH_AUD_USR_FEATURE_INFO;

/* Audio user supported functions block. */ 
typedef struct _nu_usbh_aud_usr_functions
{
    BOOLEAN speaker;
    BOOLEAN microphone;
}
NU_USBH_AUD_USR_FUNCTIONS;

/* Audio user device info structure. */ 
typedef struct _nu_usb_aud_usr_dev
{
    CS_NODE                    node;
    NU_USB_USER*               user_drvr;   
    NU_USB_DRVR               *class_drvr;
    NU_USBH_AUD_DEV           *audio_dev;    /* Pointer to Audio user 
                                              * device.               
                                              */                      
    NU_USBH_AUD_FUNC_INFO      mphone;       /* Microphone info structure.*/
    NU_USBH_AUD_FUNC_INFO      speaker;      /* Speaker info structure. */
    NU_USBH_AUD_USR_FUNCTIONS  supported_fnc;/* Supported functions.*/

} NU_USBH_AUD_USER_DEV;

/* For disconnection notification to demo application. */
typedef VOID (*NU_AUDH_USER_DISCON) (VOID*      device);    
typedef VOID (*NU_AUDH_USER_CONN) (VOID*      device);    

/* Audio user driver control block. */
typedef struct _nu_usbh_aud_user
{
    NU_USBH_USER          parent;
    NU_USBH_AUDIO        *pcb_class_drvr;
    VOID                 *pcb_first_device;
    NU_AUDH_USER_DISCON   Disconect_Handler;
    NU_AUDH_USER_CONN     Connect_Handler;
}
NU_USBH_AUD_USER;

/* Audio user driver dispatch table. */
typedef NU_USBH_USER_DISPATCH    NU_USBH_AUD_USER_DISPATCH;

/* Function prototype. */
VOID NU_AUDH_USER_Get_FU_Id(NU_USBH_AUD_USER_DEV  *aud_usr_dev,
                            UINT8                 *fu_id,
                            UINT8                  function);
/* =========================== Include Files =========================== */
/* Making internals visible. */
#include "nu_usbh_audio_user_dat.h"
/* ===================================================================== */

#endif /* _NU_USBH_AUD_USER_IMP_H_ */
