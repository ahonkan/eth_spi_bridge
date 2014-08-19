/**************************************************************************
*
*               Copyright 2005  Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
* FILE NAME
*
*       nu_usbh_com_acm_ext.h
*
*
* COMPONENT
*
*       Nucleus USB host software. Communication class driver.
*
* DESCRIPTION
*
*       This file contains prototypes for external interfaces exposed by
*       communication class host driver's ACM component and some functions
*       required for internal implementation.
*
* DATA STRUCTURES
*
*       NU_USBH_COM_ACM_INFORM              ACM Specific information
*                                           container.
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       nu_usbh_com_imp.h                   Internal definitions.
*
**************************************************************************/

/* ===================================================================== */
#ifndef _NU_USBH_COM_ACM_EXT_H_
#define _NU_USBH_COM_ACM_EXT_H_
/* ===================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================================================================== */

#ifdef __cplusplus
extern "C" {                                 /* C declarations in C++.   */
#endif

/* Include files */
#include "connectivity/nu_usbh_com_imp.h"

#ifdef INC_ACM_MDL

#define  UH_SET_LINE_CODING               0x20
#define  UH_GET_LINE_CODING               0x21
#define  UH_SET_CONTROL_LINE_STATE        0x22
#define  UH_SEND_BREAK                    0x23

/* ACM specific information. */
typedef struct usbh_com_acm_inform
{
    UINT8  call_cap;
    UINT8  acm_cap;
    UINT8  com_tb_padding[2];            /* 2 Bytes padding */
} NU_USBH_COM_ACM_INFORM;

/* API Function prototypes. */
STATUS NU_USBH_COM_Set_Line_Coding (
       NU_USBH_COM_DEVICE*   pcb_curr_com_dev,
       VOID*                 p_data_buf);

STATUS NU_USBH_COM_Get_Line_Coding (
       NU_USBH_COM_DEVICE*   pcb_curr_com_dev,
       VOID*                 p_data_buf);

STATUS NU_USBH_COM_Set_Ctrl_LS (
       NU_USBH_COM_DEVICE*   pcb_curr_com_dev,
       UINT16                LS_bmp);

STATUS NU_USBH_COM_Send_Break (
       NU_USBH_COM_DEVICE*   pcb_curr_com_dev,
       UINT16                break_time);

/* Internal Function prototypes. */
VOID   NU_USBH_COM_Check_ACM_Func_Desc(
       NU_USBH_COM_DEVICE*   pcb_curr_device,
       UINT8*                class_desc,
       UINT32                temp_length);

#endif /* INC_ACM_MDL */

#ifdef __cplusplus
    }                                        /* C declarations in C++. */
#endif

#endif/* _NU_USBH_COM_ACM_EXT_H_ */
