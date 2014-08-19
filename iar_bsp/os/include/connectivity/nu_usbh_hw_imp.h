/***********************************************************************
*
*           Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
************************************************************************/

/************************************************************************ 
 * 
 * FILE NAME
 *     nu_usbh_hw_imp.h
 * 
 * COMPONENT 
 *     Nucleus USB Software 
 * 
 * DESCRIPTION 
 *     This file describes control block and dispatch table of 
 *     NU_USBH_HW layer. 
 * 
 * DATA STRUCTURES 
 *     NU_USBH_HW            user control block description.
 * 
 * FUNCTIONS 
 *     None 
 * 
 * DEPENDENCIES 
 *     nu_usbh_stack_ext.h   Host stack definitions.
 * 
 *************************************************************************/

/* ===================================================================  */
#ifndef _NU_USBH_HW_IMP_H_
#define _NU_USBH_HW_IMP_H_
/* ===================================================================  */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================================================================  */
#include "connectivity/nu_usbh_stack_ext.h"

/* GUID definition for USB Host Hardware. */
#define USBHHW_LABEL          {0x07,0x6c,0x02,0x71,0x6f,0xbe,0x49,0xfc,0xae,0xf5,0x05,0x1f,0x51,0xb4,0x87,0xb9}
#define USBH_PWR_LABEL        {0x9a,0xfd,0x66,0xc9,0x7b,0x00,0x4a,0x57,0xb8,0x7b,0x1c,0x28,0x5d,0x6c,0xea,0xe5}

/* IOCTL commands definitions for use with Nucleus USB Host
 * Stack.
 */
#define NU_USBH_IOCTL_BASE                  (TOTAL_USB_IOCTLS + 1)
#define NU_USBH_IOCTL_UPDATE_DEVICE         (NU_USBH_IOCTL_BASE + 0)
#define NU_USBH_IOCTL_INIT_DEVICE           (NU_USBH_IOCTL_BASE + 1)
#define NU_USBH_IOCTL_DEINIT_DEVICE         (NU_USBH_IOCTL_BASE + 2)
#define NU_USBH_IOCTL_UNSTALL_PIPE          (NU_USBH_IOCTL_BASE + 3)
#define NU_USBH_IOCTL_DISABLE_PIPE          (NU_USBH_IOCTL_BASE + 4)
#define NU_USBH_IOCTL_RESET_BANDWIDTH       (NU_USBH_IOCTL_BASE + 5)
#define NU_USBH_PWR_HIB_RESTORE             (NU_USBH_IOCTL_BASE + 6)
#define TOTAL_USBH_IOCTLS                   7

/* IOCTL commands definitions for use with USB host port power controller 
 * driver.
 */
#define NU_USBH_PWR_BASE                (DV_IOCTL0 + 1)
#define NU_USBH_PWR_IOCTL_ENABLE_VBUS   (NU_USBH_PWR_BASE + 1)
#define NU_USBH_PWR_IOCTL_DISABLE_VBUS  (NU_USBH_PWR_BASE + 2)
#define TOTAL_USBH_PWR_IOCTLS           2

/* =================== Data Structures ===============================  */
struct nu_usbh_hw
{
    NU_USB_HW      controller;
    NU_MEMORY_POOL *pool;
    UINT8          number_companion_controllers;
    BOOLEAN        pending;   /* whether interrupt is pending or not */
};

typedef struct _usb_device_ss_parameters
{
    NU_USB_DEVICE   *device;
    UINT8            packet_size;
    UINT32           sel;
    BOOLEAN          is_hub;
    UINT8            tt_time;
    UINT8            num_ports;
}USB_DEV_SS_PARAMS;

VOID USBH_LISR (INT vector_no);

STATUS USBH_HW_Register_LISR (NU_USBH_HW * controller,
                              INT vector_no);

STATUS USBH_HW_Deregister_LISR (NU_USBH_HW * controller,
                                INT vector_no);

/* ===================================================================  */
#endif /* _NU_USBH_HW_IMP_H_ */
/* ====================== end of file ================================  */

