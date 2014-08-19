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

***************************************************************************
*
* FILE NAME 
*
*       nu_usbf_stack_imp.h
*
*
* COMPONENT
*
*       Stack component / Nucleus USB Device Software.
*
* DESCRIPTION
*
*       This file contains the internal declarations for the Stack
*       component of the Nucleus USB Device Software.
*
* DATA STRUCTURES
*
*       NU_USBF_STACK                       Stack control block description
*       NU_USBF_DEVICE                      Device structure.
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       nu_usb_stack_ext.h                  USB Stack declarations
*       nu_usbf_stack_ext.h                 USB Function Stack declarations
*       nu_usbf_stack_dat.h                 USB Function Stack data
*
**************************************************************************/

#ifndef _NU_USBF_STACK_IMP_H
#define _NU_USBF_STACK_IMP_H

/* ==============  USB Include Files ==================================  */

#include "connectivity/nu_usb_stack_ext.h"
#include "connectivity/nu_usbf_ext.h"

/* =====================  #defines ====================================  */

/* Buffer size for USB function device general purpose transfer buffer. */
#define USBF_DEVICE_TXFR_BUFF_SIZE      20

#define USBF_EVENT_STACK_SHUTDOWN       0x0001
#define USBF_EVENT_RESET                0x0002
#define USBF_EVENT_SUSPEND              0x0003
#define USBF_EVENT_RESUME               0x0004
#define USBF_EVENT_CONNECT              0x0005
#define USBF_EVENT_DISCONNECT           0x0006
#define USBF_EVENT_CLEAR_HALTENDPOINT   0x0007 
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
#define USBF_EVENT_FUNCTION_SUSPEND     0x0008
#endif

/* These bits are set in addition to the Function Suspend Event value. */
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
#define USBF_EVENT_FUNCTION_SUSPEND_RW  0x80000000
#define USBF_EVENT_FUNCTION_SUSPEND_FS  0x40000000
#endif

#define USBF_SET_LENGTH(irp, x)     ((irp).length = (x))

#define USBF_SET_BUFFER(irp, x)     ((irp).buffer = (UINT32 *)(x))

#define USBF_GET_RECIPIENT(setup)   (((setup)->bmRequestType) & 0x0F)

#define USBF_NEXT_DRIVER(current)    \
        ((USB_DRVR_LIST *) ((current)->list_node.cs_next))

/* ====================  Data Types ===================================  */

typedef struct nu_usb_drvr NU_USBF_DRVR;

/* Using this structure, we track the Function controller and its other
 * parameters maintained by the Stack.
 */
typedef struct nu_usbf_device
{
    NU_USB_DEVICE *usb_device;            /* Shared device struct.       */
    UINT8         state;                  /* Current state.              */
    UINT8         other_speed;            /* high speed capable device is
                                             operating at full speed     */
    UINT8         pad[2];
    NU_USB_CFG    *active_config;         /* Current configuration.      */
    NU_USB_IRP    ctrl_irp;               /* Control transfers.          */
    UINT8         *buffer;                /* For holding transfer data.  */
}
NU_USBF_DEVICE;

typedef struct nu_usbf_hisr
{
    NU_HISR       hisr;
    NU_USBF_STACK *stack;
    INT           vector;
}
NU_USBF_HISR;

/* Stack control block.  */
struct nu_usbf_stack
{
    NU_USB_STACK stack;

    /* All registered Function controllers and their data.   */
    NU_USBF_DEVICE device_list[NU_USBF_MAX_HW];

    /* Number of registered controllers, as-on-now. */
    UINT32 num_devices;

    /* USBF HISR control block. */
    NU_USBF_HISR usbf_hisr;
};

/* Standard control request handling. */
typedef STATUS (*USBF_CTRL_REQ_WRKR) (NU_USBF_DEVICE   *device,
                                      NU_USB_SETUP_PKT *setup);

/* ====================  Function Prototypes =========================== */

/* Facts interface for stacks. */
STATUS USBF_Stack_Information (NU_USB_STACK * cb,
                               CHAR * name);
/* Internal functions. */
STATUS usbf_enable_device (NU_USBF_DEVICE * device);
STATUS usbf_enable_driver (NU_USBF_STACK * stack,
                           NU_USB_DRVR * driver);
STATUS usbf_disable_driver (NU_USBF_STACK * stack,
                            NU_USB_DRVR * driver);
STATUS usbf_disable_device (NU_USBF_DEVICE * device);

VOID usbf_ep0_transfer_complete (NU_USB_PIPE * pipe,
                                 NU_USB_IRP * irp);
STATUS usbf_feature (NU_USBF_DEVICE * device,
                     NU_USB_SETUP_PKT * setup);
STATUS usbf_get_configuration (NU_USBF_DEVICE * device,
                               NU_USB_SETUP_PKT * setup);
STATUS usbf_get_descriptor (NU_USBF_DEVICE * device,
                            NU_USB_SETUP_PKT * setup);
NU_USBF_DEVICE *usbf_get_device (NU_USB_STACK * cb,
                                 NU_USBF_HW * fc,
                                 INT * index);
STATUS usbf_get_interface (NU_USBF_DEVICE * device,
                           NU_USB_SETUP_PKT * setup);
STATUS usbf_get_status (NU_USBF_DEVICE * device,
                        NU_USB_SETUP_PKT * setup);
STATUS usbf_process_class_specific (NU_USBF_DEVICE * device,
                                    NU_USB_SETUP_PKT * setup);
STATUS usbf_request_error (NU_USBF_DEVICE * device,
                           NU_USB_SETUP_PKT * setup);
STATUS usbf_set_address (NU_USBF_DEVICE * device,
                         NU_USB_SETUP_PKT * setup);
STATUS usbf_set_configuration (NU_USBF_DEVICE * device,
                               NU_USB_SETUP_PKT * setup);
STATUS usbf_set_descriptor (NU_USBF_DEVICE * device,
                            NU_USB_SETUP_PKT * setup);
STATUS usbf_set_interface (NU_USBF_DEVICE * device,
                           NU_USB_SETUP_PKT * setup);
STATUS usbf_sync_frame (NU_USBF_DEVICE * device,
                        NU_USB_SETUP_PKT * setup);

STATUS usbf_validate_endpoint (NU_USBF_DEVICE * device,
                               UINT8 bEndpointAddress,
                               NU_USB_DRVR ** driver);

STATUS usbf_validate_interface (NU_USBF_DEVICE * device,
                                UINT8 intf_num,
                                NU_USB_DRVR ** driver);

STATUS usbf_validate_setup (NU_USBF_DEVICE * device,
                            NU_USB_SETUP_PKT * setup);

STATUS usbf_setup_descriptor(NU_USBF_DEVICE *device, 
                             UINT32 speed);
                             
STATUS usbf_close_pipes(NU_USBF_DEVICE   *device,
                        NU_USB_CFG   *cfg);

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

STATUS usbf_set_isochornous_delay(NU_USBF_DEVICE   *device,
                                  NU_USB_SETUP_PKT *setup);

STATUS usbf_set_system_exit_latency(NU_USBF_DEVICE   *device,
                                    NU_USB_SETUP_PKT *setup);
#endif

/* ===================================================================== */

#include "connectivity/nu_usbf_stack_dat.h"

/* ===================================================================== */

#endif /* _NU_USBF_STACK_IMP_H      */

/* ====================  Function Prototypes =========================== */

