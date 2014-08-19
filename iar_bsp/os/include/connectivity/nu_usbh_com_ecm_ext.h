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

**************************************************************************
*
* FILE NAME
*
*       nu_usbh_com_ecm_ext.h
*
*
* COMPONENT
*
*       Nucleus USB host software. Communication class driver.
*
* DESCRIPTION
*
*       This file contains prototypes for internal and external interfaces
*       exposed by communication class host driver's ECM component.
*
* DATA STRUCTURES
*
*       NU_USBH_COM_ECM_INFORM              Holds information for an ECM
*                                           device.
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
#ifndef _NU_USBH_COM_ECM_EXT_H_
#define _NU_USBH_COM_ECM_EXT_H_
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

#define  UH_SET_ETH_MULTICAST_FILTERS    0x40
#define  UH_SET_ETH_POWER_PATTERN_FILTER 0x41
#define  UH_GET_ETH_POWER_PATTERN_FILTER 0x42
#define  UH_SET_ETH_PACKET_FILTER        0x43
#define  UH_GET_ETH_STATISTIC            0x44

/* Data Structures. */
typedef struct usbh_com_ecm_inform
{
    UINT8  MAC_addr[6];
    UINT16 segment_size;
    UINT32 stats;
    UINT16 MC_filters;
    UINT8  power_filters;
    UINT8  com_tb_padding[1];            /* 1 Byte padding */
} NU_USBH_COM_ECM_INFORM;

/* API Function prototypes. */
STATUS NU_USBH_COM_Set_ETH_Mul_Filter(
       NU_USBH_COM_DEVICE*   pcb_curr_com_dev,
       VOID*                 p_data_buf,
       UINT32                data_length);

STATUS NU_USBH_COM_Set_ETH_Power_Filter(
       NU_USBH_COM_DEVICE*   pcb_curr_com_dev,
       VOID*                 p_data_buf,
       UINT32                data_length,
       UINT16                filter_num);

STATUS NU_USBH_COM_Get_ETH_Power_Filter(
       NU_USBH_COM_DEVICE*   pcb_curr_com_dev,
       VOID*                 p_data_buf,
       UINT32                data_length,
       UINT16                filter_num);

STATUS NU_USBH_COM_Set_ETH_Packet_Filter(
       NU_USBH_COM_DEVICE*   pcb_curr_com_dev,
       UINT16                filter_bmp);

STATUS NU_USBH_COM_Get_ETH_Static(
       NU_USBH_COM_DEVICE*    pcb_curr_com_dev,
       UINT16                 feature_selector,
       UINT32*                feature);

/* Internal Function prototypes. */
VOID   NU_USBH_COM_Check_ECM_Func_Desc(
       NU_USBH_COM_DEVICE*   pcb_curr_device,
       UINT8*                class_desc,
       UINT32                temp_length);

#endif /* INC_ECM_MDL */

#ifdef __cplusplus
    }                                        /* C declarations in C++. */
#endif

#endif /* _NU_USBH_COM_ECM_EXT_H_ */
