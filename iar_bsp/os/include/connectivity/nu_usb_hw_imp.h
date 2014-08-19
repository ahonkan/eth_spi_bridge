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
************************************************************************

*************************************************************************
*
*    FILE NAME 
*
*        nu_usb_hw_imp.h
*
*    COMPONENT
*
*        Nucleus USB Software
*
*    DESCRIPTION
*
*        This file contains the function names and data structures
*        for the common controller driver component.
* 
*    DATA STRUCTURES 
*
*        nu_usb_hw           HW control block
*        usb_hw_dispatch     HW dispatch table
*
*    FUNCTIONS 
*
*        None 
*
*    DEPENDENCIES 
*
*        nu_usb_hw_spc.h     Hardware target specific definitions
*
************************************************************************/

/* ==================================================================== */
#ifndef _NU_USB_HW_IMP_H
#define _NU_USB_HW_IMP_H

/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* ====================  Data Types ==================================  */

struct nu_usb_hw
{
    NU_USB cb;
    UINT8 *base_address;            /* Base address in the memory map   */
    UINT8 speed;                    /* USB 2.0/1.1 /both                */
    UINT16 num_irq;                 /* Number of IRQ supported by FC    */
    INT irq[NU_USB_MAX_IRQ];        /* All the IRQ values, listed       */
    NU_USB_STACK *stack_cb;         /* USB Stack instance               */
    DV_DEV_HANDLE dv_handle;        /* Handle to USB hardware controller
                                     * acquired through open call       */
    INT ioctl_base_addr;            /* Base address of USB controller 
                                     * driver IOCTLs.                   */
};

/* These structures ar used as a paramet for certain IOCTL calls. */
typedef struct _usb_ep_info
{
    UINT32  interval;
    UINT32  load;
    UINT16  max_packet_size;
    UINT16  endp_sts;
    UINT8   function_addr;
    UINT8   endp_addr;
    UINT8   endp_attrib;
    UINT8   speed;
    UINT8   config_num;
    UINT8   intf_num;
    UINT8   alt_sttg;
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
    UINT8   max_burst;
    UINT8   ep_comp_attrib;
    UINT8   dummy_1;
    UINT8   dummy_2;
    UINT8   dummy_3;
    UINT16  bytes_per_interval;
    UINT16  dummy_4;
#else
    UINT8   dummy_5;
#endif
}USB_EP_INFO;

typedef struct _usb_irp_info
{
    NU_USB_IRP  *irp;
    UINT8       endp_addr;
    UINT8       func_addr;
}USB_IRP_INFO;

typedef struct _usb_port_info
{
    UINT8   port_id;
    UINT8   hw_role_out;
    UINT16  delay;
}USB_PORT_INFO;

typedef struct _usb_device_current_info
{
    NU_USB_DEVICE   *device;
    UINT8           cfg;
    BOOLEAN         is_current_available;
}USB_DEV_CURRENT_INFO;

/* IOCTL commands definitions for use with Nucleus USB Base Stack. */
#define NU_USB_IOCTL_BASE               (DV_IOCTL0 + 1)
#define NU_USB_IOCTL_INITIALIZE         0
#define NU_USB_IOCTL_IO_REQUEST         1
#define NU_USB_IOCTL_OPEN_PIPE          2
#define NU_USB_IOCTL_CLOSE_PIPE         3
#define NU_USB_IOCTL_MODIFY_PIPE        4
#define NU_USB_IOCTL_FLUSH_PIPE         5
#define NU_USB_IOCTL_EXECUTE_ISR        6
#define NU_USB_IOCTL_ENABLE_INT         7
#define NU_USB_IOCTL_DISABLE_INT        8
#define NU_USB_IOCTL_GET_ROLE           9
#define NU_USB_IOCTL_START_SESSION      10
#define NU_USB_IOCTL_END_SESSION        11
#define NU_USB_IOCTL_NOTIF_ROLE_SWITCH  12
#define NU_USB_IOCTL_NORMALIZE_BUFFER   13
#define NU_USB_IOCTL_TX_DONE            14
#define NU_USB_IOCTL_GET_SPEED          15
#define NU_USB_IOCTL_GET_HW_CB          16
#define NU_USB_IOCTL_IS_CURR_AVAILABLE  17
#define NU_USB_IOCTL_RELEASE_POWER      18
#define NU_USB_IOCTL_REQ_POWER_DOWN     19
#define NU_USB_IOCTL_OPEN_SS_PIPE       20
#define NU_USB_IOCTL_MODIFY_SS_PIPE     21
#define NU_USB_IOCTL_UPDATE_PWR_MODE    22
#define NU_USB_IOCTL_UPDATE_BELT_VAL    23
#define NU_USB_IOCTL_UNINITIALIZE       24
#define NU_USB_IOCTL_TEST_MODE          25
#define TOTAL_USB_IOCTLS                26

/* ==============  USB Include Files =================================  */

#include "connectivity/nu_usb_hw_spc.h"

/* ==================================================================== */

#endif /* _NU_USB_HW_IMP_H    */

/* =======================  End Of File  ============================== */

