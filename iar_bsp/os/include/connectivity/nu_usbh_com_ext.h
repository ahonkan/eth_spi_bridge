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
*       nu_usbh_com_ext.h
*
*
* COMPONENT
*
*       Nucleus USB host software. Communication class driver.
*
* DESCRIPTION
*
*       This file contains prototypes for external interfaces exposed by
*       communication class host driver.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       nu_usbh_com_imp.h                   Internal definitions.
*       nu_usbh_com_acm_ext.h               Definitions for ACM.
*       nu_usbh_com_ecm_ext.h               Definitions for ECM.
*
**************************************************************************/

/* ===================================================================== */
#ifndef _NU_USBH_COM_EXT_H_
#define _NU_USBH_COM_EXT_H_
/* ===================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================================================================== */

#ifdef __cplusplus
extern "C" {                                 /* C declarations in C++. */
#endif

/* Include files */
#include "connectivity/nu_usbh_com_imp.h"

#ifdef INC_ECM_MDL
#include "connectivity/nu_usbh_com_acm_ext.h"
#endif

#ifdef INC_ECM_MDL
#include "connectivity/nu_usbh_com_ecm_ext.h"
#endif

/* Event Group for initialized Communication user drivers. */
extern NU_EVENT_GROUP   USBH_COMM_USER_INIT_Events;

/* Function prototypes. */

STATUS NU_USBH_COM_Create (
       NU_USBH_COM*         pcb_com_drvr,
       CHAR*                p_name,
       NU_MEMORY_POOL*      p_memory_pool,
       NU_USB_STACK*        usb_stack);

STATUS _NU_USBH_COM_Delete (
       VOID*                cb);

STATUS _NU_USBH_COM_Initialize_Intf (
       NU_USB_DRVR*         pcb_drvr,
       NU_USB_STACK*        pcb_stack,
       NU_USB_DEVICE*       pcb_device,
       NU_USB_INTF*         pcb_intf);

STATUS _NU_USBH_COM_Data_Initialize_Intf (
       NU_USB_DRVR*         pcb_drvr,
       NU_USB_STACK*        pcb_stack,
       NU_USB_DEVICE*       pcb_device,
       NU_USB_INTF*         pcb_intf);

STATUS NU_USBH_COM_Transfer (
       NU_USBH_COM_DEVICE*  pcb_curr_com_dev,
       NU_USBH_COM_XBLOCK*  pcb_xblock);

STATUS NU_USBH_COM_Send_Encap_Cmd (
       NU_USBH_COM_DEVICE*  pcb_curr_com_dev,
       VOID*                p_data_buf,
       UINT32               data_length);

STATUS NU_USBH_COM_Get_Encap_Resp (
       NU_USBH_COM_DEVICE*  pcb_curr_com_dev,
       VOID*                p_data_buf,
       UINT32               data_length);

STATUS NU_USBH_COM_Set_Comm_Feature (
       NU_USBH_COM_DEVICE*  pcb_curr_com_dev,
       VOID*                p_data_buf,
       UINT32               data_length,
       UINT16               feature_sel);

STATUS NU_USBH_COM_Get_Comm_Feature (
       NU_USBH_COM_DEVICE*  pcb_curr_com_dev,
       VOID*                p_data_buf,
       UINT32               data_length,
       UINT16               feature_sel);

STATUS NU_USBH_COM_Clear_Comm_Feature (
       NU_USBH_COM_DEVICE*  pcb_curr_com_dev,
       UINT16               feature_sel);

STATUS _NU_USBH_COM_Disconnect (
       NU_USB_DRVR*         pcb_drvr,
       NU_USB_STACK*        pcb_stack,
       NU_USB_DEVICE*       pcb_device);

STATUS _NU_USBH_COM_Data_Disconnect (
       NU_USB_DRVR*         pcb_drvr,
       NU_USB_STACK*        pcb_stack,
       NU_USB_DEVICE*       pcb_device);

STATUS nu_os_conn_usb_host_comm_class_init(CHAR *path, INT startstop);

STATUS NU_USBH_COMM_Init_GetHandle (
       VOID  **handle);

#ifdef __cplusplus
    }                                        /* C declarations in C++. */
#endif

#endif /* _NU_USBH_COM_EXT_H_ */
