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
 *
 *      nu_usbh_stack_imp.h 
 * 
 * COMPONENT 
 *      Nucleus USB Host Stack 
 * 
 * DESCRIPTION 
 *      This file contains control block and other internal definitions for
 *      Nucleus USB Host stack.
 *
 * 
 * DATA STRUCTURES 
 *      nu_usbh_stack           Host stack control block.
 *      usbh_bus_resource       Host Bus resources
 *      usb_iso_descriptor      Isochronous descriptor.
 *      nu_usbh_hisr            Host HISR structure.
 * 
 * FUNCTIONS 
 *      None 
 * 
 * DEPENDENCIES 
 *      nu_usbh_stack_dat.h     Host stack dispatch table.
 *      nu_usbh_stack_dbg.h     Host stack debug interface definitions.
 *      nu_usbh_hub_imp.h       Hub class driver definitions.
 * 
 *************************************************************************/

/* ===================================================================  */
#ifndef _NU_USBH_STACK_IMP_H_
#define _NU_USBH_STACK_IMP_H_
/* ===================================================================  */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif

/* ======================== Global Data ==============================  */
typedef NU_USB_DRVR NU_USBH_DRVR;
typedef struct nu_usbh_hw NU_USBH_HW;
typedef struct nu_usbh_stack NU_USBH_STACK;
typedef struct nu_usbh_user NU_USBH_USER;

/* ====================== #defines ===================================  */
#define USB_MAX_DEVID  128
#define USB1_BANDWIDTH  900     /* in us,the total b/w for periodic Txs in usb1.1 */
#define USB2_BANDWIDTH  800     /* in us,the total b/w for periodic Txs in usb2.0 */

#define NU_USB_HOST_DELAY 1000L /*  HC set up delay in Nano secs */
#define NU_USB_HUB_LS_SETUP_DELAY 333   /* Low speed Hub set up delay-
                                           Nano secs */

#define USB_DEFAULT_DEV_ID   0

/* Pipe transfer types-n-directions */
#define PIPE_CONTROL          0
#define OUT_PIPE_ISOCHRONOUS  1
#define OUT_PIPE_BULK         2
#define OUT_PIPE_INTERRUPT    3
#define IN_PIPE_ISOCHRONOUS   4
#define IN_PIPE_BULK          5
#define IN_PIPE_INTERRUPT     6

/* Number of Languages */
#define ONE_LANGUAGE     1
#define MULTI_LANGUAGES  NU_FALSE        /* Default to one */

/* Product ID and Vendor ID of OTG test device defined by OTG specification. */
#define NU_USB_OTG_TEST_DEV_VID 0x1A0A
#define NU_USB_OTG_TEST_DEV_PID 0xBADD

/* Host Test Modes PID Definitions */
#define NU_USBH_TEST_SE0_NAK_DEV_PID    0x0101
#define NU_USBH_TEST_J_DEV_PID          0x0102
#define NU_USBH_TEST_K_DEV_PID          0x0103
#define NU_USBH_TEST_PACKET_DEV_PID     0x0104
#define NU_USBH_SUSPEND_RESUME_DEV_PID  0x0106
#define NU_USBH_GET_DESC_DEV_PID        0x0107
#define NU_USBH_SET_FEATURE_DEV_PID     0x0108

#define USBH_IS_ROOT_HUB(A)  (((A)->function_address == USB_ROOT_HUB) ? 1: 0)

#define USBH_FILL_CNTRLPKT(irp,requestType, request,value, index, length) \
      NU_USBH_CTRL_IRP_Set_bmRequestType(irp, requestType); \
      NU_USBH_CTRL_IRP_Set_bRequest(irp, request); \
      NU_USBH_CTRL_IRP_Set_wValue(irp, HOST_2_LE16(value)); \
      NU_USBH_CTRL_IRP_Set_wIndex(irp, HOST_2_LE16(index)); \
      NU_USBH_CTRL_IRP_Set_wLength(irp, HOST_2_LE16(length))

#define usb_calc_direction_n_type(end_point, bmAttributes) \
        ((((bmAttributes) & 0x3) != 0) ? \
          (((((end_point) & 0x80)>>7)*3) + ((bmAttributes) & 0x3)) : 0)

/* =================== Data Structures ===============================  */
typedef struct usbh_bus_resources
{
    NU_USBH_HW *controller;
    UINT8 bus_id;
    UINT32 total_bandwidth;               /* Total b/w in usecs in a frame */
    UINT32 avail_bandwidth;               /* B/W available for new EPs */
    NU_USB_DEVICE *root_hub;
	
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
	NU_USB_DEVICE *ss_root_hub;           /* SuperSpeed root hub. */
#endif

    NU_USB_DEVICE *dev_list;              /* Database of devices on this HC */
    UINT8 dev_ids[USB_MAX_DEVID / 8];     /* bit map for device ids */
    UINT8 last_dev_id;                    /* Last device id thats assigned */
}
USBH_BUS_RESOURCES;

/* used for iso transfers */
typedef struct usb_iso_descriptor
{
    UINT16 bytes_requested;     /* Bytes to be transferred per frame */
    UINT16 bytes_transferred;   /* Bytes actually transferred in each frame */
    UINT32 reserved;
}
NU_USB_ISO_PKT;

typedef struct nu_usbh_hisr
{
    NU_HISR hisr;
    NU_USBH_STACK *stack;
}
NU_USBH_HISR;

/* ==============  USB Include Files =================================  */
#include "connectivity/nu_usbh_hub_imp.h"

/* USBH_Stack control block. */
struct nu_usbh_stack
{
    NU_USB_STACK usb_stack;
    NU_MEMORY_POOL *pool;
    USBH_BUS_RESOURCES bus_resources[NU_USBH_MAX_HW];

    /* lock for controlling access to controller and class drivers */
    NU_SEMAPHORE lock;

    /* IRP pointer for control transfers */
    NU_USBH_CTRL_IRP *ctrl_irp;

    /* Ctrl IRP's callback completion binary semaphore */
    NU_SEMAPHORE callback_semaphore;

    /* semaphore to ensure that only one USBD ctrl IRP is outstanding */
    NU_SEMAPHORE ctrl_msgs_semaphore;

    /* Interrupt notification semaphore */
    NU_SEMAPHORE irq_semaphore;

    /* USBH HISR control block */
    NU_USBH_HISR usbh_hisr;

    /* USBH-Stk task control block */
    NU_TASK usbh_stack_task;

    /* Hub class driver's control block */
    NU_USBH_DRVR_HUB hub_driver;
};

/* ================== Function Prototypes ============================  */
/* USBH_Stack specific API */
STATUS _NU_USBH_STACK_Delete (VOID *stack);

STATUS _NU_USBH_STACK_Register_Drvr (NU_USB_STACK * cb,
                                     NU_USB_DRVR * class_driver);

STATUS _NU_USBH_STACK_Deregister_Drvr (NU_USB_STACK * cb,
                                       NU_USB_DRVR * class_driver);

STATUS _NU_USBH_STACK_Add_Hw (NU_USB_STACK * cb,
                              NU_USB_HW * controller);

STATUS _NU_USBH_STACK_Remove_Hw (NU_USB_STACK * cb,
                                 NU_USB_HW * controller);

STATUS _NU_USBH_STACK_Set_Config (NU_USB_STACK * cb,
                                  NU_USB_DEVICE * device,
                                  UINT8 cnfgno);

STATUS _NU_USBH_STACK_Get_Configuration (NU_USB_STACK * cb,
                                         NU_USB_DEVICE * device,
                                         UINT8 *cnfgno_out);

STATUS _NU_USBH_STACK_Set_Intf (NU_USB_STACK * cb,
                                NU_USB_DEVICE * device,
                                UINT8 interface_index,
                                UINT8 alt_setting_index);

STATUS _NU_USBH_STACK_Get_Interface (NU_USB_STACK * cb,
                                     NU_USB_DEVICE * device,
                                     UINT8 interface_index,
                                     UINT8 *alt_setting_index_out);

STATUS _NU_USBH_STACK_Get_Interface_Status (NU_USB_STACK * cb,
                                            NU_USB_DEVICE * device,
                                            UINT8 interface_index,
                                            UINT16 *status);

STATUS _NU_USBH_STACK_Stall_Endp (NU_USB_STACK * cb,
                                  NU_USB_PIPE * pipe);

STATUS _NU_USBH_STACK_Unstall_Endp (NU_USB_STACK * cb,
                                    NU_USB_PIPE * pipe);

STATUS _NU_USBH_STACK_Get_Endpoint_Status (NU_USB_STACK * cb,
                                           NU_USB_PIPE * pipe,
                                           UINT16 *status);

STATUS _NU_USBH_STACK_Is_Endp_Stalled (NU_USB_STACK * cb,
                                       NU_USB_PIPE * pipe,
                                       BOOLEAN * status_out);

STATUS _NU_USBH_STACK_Get_Device_Status (NU_USB_STACK * cb,
                                         NU_USB_DEVICE * device,
                                         UINT16 *status);

STATUS _NU_USBH_STACK_Is_Device_Self_Powered (NU_USBH_STACK * cb,
                                              NU_USB_DEVICE * device,
                                              BOOLEAN * status_out);

STATUS _NU_USBH_STACK_Start_Session (NU_USB_STACK * cb,
                                     NU_USB_HW * hw,
                                     UINT8 port_id, 
                                     UINT16 delay);

STATUS _NU_USBH_STACK_End_Session (NU_USB_STACK * cb,
                                   NU_USB_HW * hw,
                                   UINT8 port_id);

STATUS USBH_Is_Device_Remote_Wakeup (NU_USBH_STACK * cb,
                                     NU_USB_DEVICE * device,
                                     BOOLEAN * status_out);

STATUS _NU_USBH_STACK_Submit_IRP (NU_USB_STACK * cb,
                                  NU_USB_IRP * irp,
                                  NU_USB_PIPE * pipe);

STATUS _NU_USBH_STACK_Cancel_IRP (NU_USB_STACK * cb,
                                  NU_USB_PIPE * pipe);

STATUS _NU_USBH_STACK_Flush_Pipe (NU_USB_STACK * cb,
                                  NU_USB_PIPE * pipe);

STATUS USBH_Get_Frame_Number (NU_USB_STACK * cb,
                              NU_USB_DEVICE * device,
                              UINT16 *frame_num);

STATUS USBH_Enumerate_Device (NU_USBH_STACK * stack,
                              NU_USB_DEVICE * parent,
                              UINT8 port_number,
                              UINT8 speed,
                              NU_USB_DEVICE ** new_device);

STATUS USBH_Deenumerate_Device (NU_USBH_STACK * stack,
                                NU_USB_DEVICE * dev);

STATUS USBH_Get_Parent (NU_USBH_STACK * stack,
                        NU_USB_DEVICE * dev,
                        NU_USB_DEVICE ** parent);

STATUS USBH_Get_Port_Number (NU_USBH_STACK * stack,
                             NU_USB_DEVICE * dev,
                             UINT8 *port_number);

STATUS USBH_Is_Device_Self_Powered (NU_USBH_STACK * stack,
                                    NU_USB_DEVICE * device,
                                    BOOLEAN * reply_out);

STATUS USBH_Get_Active_Config (NU_USB_STACK * stack,
                               NU_USB_DEVICE * device,
                               NU_USB_CFG_DESC * cnfg);

STATUS USBH_Get_Full_Config (NU_USB_STACK * stack,
                             NU_USB_DEVICE * device,
                             UINT8 cnfgno,
                             UINT8 **cnfg);

STATUS _NU_USBH_STACK_Lock (NU_USB_STACK * cb);

STATUS _NU_USBH_STACK_Unlock (NU_USB_STACK * cb);

BOOLEAN _NU_USBH_STACK_Is_Valid_Device (NU_USB_STACK * cb,
                                        NU_USB_DEVICE * device);

VOID usbh_hisr (VOID);

VOID usbh_scan_controllers (UNSIGNED dummy1, VOID *dummy2);

VOID usb_callback_hndlr (NU_USB_PIPE * pipe, NU_USB_IRP * irp);

UINT8 usb_get_id (UINT8 *bits, UINT8 size, UINT8 last_id);

VOID usb_release_id (UINT8 *bits, UINT8 size, UINT8 id);

UINT8 usb_is_id_set (UINT8 *bits, UINT8 size, UINT8 id);

UINT32 usb_control_transfer (NU_USBH_STACK * stack,
                             NU_USB_PIPE * pipe,
                             NU_USBH_CTRL_IRP * irp,
                             VOID *buffer,
                             UINT32 len);

STATUS usb_get_configuration (NU_USBH_STACK * cb,
                              NU_USB_DEVICE * dev);

STATUS usb_close_pipes (NU_USB_DEVICE * dev,
                        USBH_BUS_RESOURCES * bus, 
                        UINT8 close_default_pipe);

STATUS usb_set_interface_id (NU_USBH_STACK * cb,
                             NU_USB_DEVICE * dev,
                             USBH_BUS_RESOURCES * bus,
                             NU_USB_CFG * cnfg,
                             UINT8 interface_index,
                             UINT8 alt_setting_index,
                             BOOLEAN send_request);

VOID usb_init_interfaces(NU_USBH_STACK *stack,
                         NU_USB_DEVICE *device,
                         USB_DRVR_LIST *list);

VOID usb_attempt_unclaimed_devs (NU_USBH_STACK * cb,
                                 USB_DRVR_LIST * drvr);

UINT8 usb_any_claimed_device (NU_USBH_STACK * cb,
                              NU_USB_DRVR * drvr);

UINT32 usb_calc_load (UINT8 direction_n_type,
                      UINT8 speed,
                      UINT16 max_packet_size);

STATUS usb_set_config (NU_USBH_STACK * cb,
                       NU_USB_DEVICE * dev,
                       USBH_BUS_RESOURCES * bus,
                       NU_USB_CFG * cnfg);

STATUS usb_unset_config (NU_USBH_STACK * cb,
                         NU_USB_DEVICE * dev,
                         USBH_BUS_RESOURCES * bus);

STATUS usb_modify_ep (NU_USBH_STACK * cb,
                      NU_USB_DEVICE * dev,
                      NU_USB_PIPE * pipe,
                      UINT8 feature);

NU_USB_ENDP_DESC *usb_verify_pipe (NU_USB_DEVICE * dev,
                                   NU_USB_PIPE * pipe);

STATUS usb_fetch_string (NU_USBH_STACK * cb,
                         NU_USB_DEVICE * dev,
                         UINT8 index,
                         CHAR * string);

USBH_BUS_RESOURCES *usbh_find_bus (NU_USBH_STACK * stack,
                                   NU_USB_DEVICE * device);

STATUS usb_fetch_string_desc (NU_USBH_STACK * cb,
                              NU_USB_DEVICE * dev,
                              UINT8 index);

NU_USB_DRVR *usb_find_next_best_vendor_drvr (NU_USBH_STACK * cb,
                                 NU_USB_DEVICE_DESC *device_descriptor,
                                 USB_DRVR_LIST * current,
                                 UINT8 only_this_driver);

USB_DRVR_LIST *usb_find_next_best_std_driver (NU_USBH_STACK * cb,
                                 NU_USB_INTF_DESC *interface_descriptor,
                                 USB_DRVR_LIST * current,
                                 UINT8 only_this_driver);

NU_USB_DRVR *usb_bind_vendor_driver (NU_USBH_STACK *stack,
                                     NU_USB_DEVICE *device,
                                     USB_DRVR_LIST *driver,
                                     UINT8 only_this_driver);

STATUS usb_bind_standard_driver(NU_USBH_STACK *stack,
                                NU_USB_DEVICE *device,
                                USBH_BUS_RESOURCES *bus);

#if (USB_TEST_MODE_SUPPORT == NU_TRUE)
STATUS usbh_test_mode_enable_using_pid(NU_USBH_STACK *cb,
                                       NU_USB_DEVICE *device, UINT16 PID);

STATUS usbh_test_mode_get_descriptor(NU_USBH_STACK *cb, NU_USB_DEVICE *device);

STATUS usbh_test_mode_enumerate(NU_USBH_STACK *cb, NU_USB_DEVICE *device,
                           UINT8 config_index);
#endif

extern STATUS NU_USB_HW_Request_Power_Down_Mode (NU_USB_HW *hw);

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

STATUS USBH_Get_BOS_Descriptor      (NU_USBH_STACK *cb,
                                     NU_USB_DEVICE *dev);

STATUS USBH_Set_Isochronous_Delay   (NU_USBH_STACK *cb,
                                     NU_USB_DEVICE *dev);

STATUS USBH_Set_System_Exit_Latency (NU_USBH_STACK *cb,
                                     NU_USB_DEVICE *dev);

STATUS USBH_Initialize_PM_interface(NU_USBH_STACK *stack,
                                    NU_USBH_HW    *hw_ctrl,
									NU_USB_DEVICE *device);
STATUS USBH_Modify_Interface_Feature(NU_USBH_STACK  *cb,
                                     NU_USB_DEVICE   *device,
                                     NU_USB_INTF     *intf,
                                     UINT8           feature,
                                     UINT8           options,
                                     UINT8           feature_selector);

STATUS USBH_Modify_Device_Feature   (NU_USBH_STACK  *cb,
                                     NU_USB_DEVICE   *device,
                                     UINT8           feature,
                                     UINT8           feature_selector);

#endif      /* ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE ) */
/* ==============  USB Include Files =================================  */

#include "connectivity/nu_usbh_stack_dat.h"
#include "connectivity/nu_usbh_stack_dbg.h"

/* ===================================================================  */
#endif /* _NU_USBH_STACK_IMP_H_ */
/* ====================== end of file ================================  */

