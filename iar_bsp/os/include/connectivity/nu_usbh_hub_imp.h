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
 *      nu_usbh_hub_imp.h
 *
 * COMPONENT
 *      Nucleus USB Host Stack
 *
 * DESCRIPTION
 *      This file contains definitions of control block and other
 *      structures for Nucleus USB Host Hub class driver.
 *
 * DATA STRUCTURES
 *      usbh_irp_hub        structure to maintain IRP info for a hub.
 *      hub_status          Hub port status data, returned by interrupt pipe.
 *      usb_hub_desc        Hub class specific descriptor
 *      nu_usbh_device_hub  Each connected hub is described by this structure.
 *      nu_usbh_hub         Hub class driver control block
 *
 * FUNCTIONS
 *      None
 *
 * DEPENDENCIES
 *      nu_usbh_hub_dat.h       HUB driver dispatch table.
 *      nu_usbh_ctrl_irp_ext.h  Control IRP definitions.
 *
 *************************************************************************/

/* ===================================================================  */
#ifndef _NU_USBH_HUB_IMP_H_
#define _NU_USBH_HUB_IMP_H_
/* ===================================================================  */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ===================================================================  */

/* ==================== USB Include Files ============================= */
#include "connectivity/nu_usbh_ctrl_irp_ext.h"

/* =================== Data Structures ===============================  */
typedef struct nu_usbh_device_hub NU_USBH_DEVICE_HUB;

#define USBH_MAX_HUB_CHAIN            7

typedef struct usbh_irp_hub
{
    NU_USBH_DEVICE_HUB *hub;       /* Pointer to the hub device   */
    UINT32             index;      /* Current IRP index   */
    NU_USB_IRP         irp;        /* irp being submitted */
    UINT8              *port_status;
}
USBH_IRP_HUB;

typedef struct hub_status
{
    UINT16 status;
    UINT16 change;
}
USBH_HUB_STATUS;

/* =====================  #defines ===================================  */
#define USBH_HUB_MAX_STATUS_IRPS                     2
#define USBH_HUB_MAX_ENUM_RETRIES                    3
#define USBH_HUB_MAX_CONSECUTIVE_ERRORS              5
#define USBH_HUB_MAX_CHILDREN                        15

/* If optimizations are enabled then use different values for macros. */
#if (NU_USB_OPTIMIZE_FOR_SIZE)
#define USBH_HUB_QUEUE_MSGS                          8
#else /* If no optimization is required. */
#define USBH_HUB_QUEUE_MSGS                          16
#endif /* #if (NU_USB_OPTIMIZE_FOR_SIZE) */

#define USBH_HUB_PORT_REQ                            0x00
#define USBH_HUB_REQ                                 0x01
#define USBH_HUB_NORMAL                              0x0
#define USBH_HUB_SHUTDOWN                            0x1
#define USBH_HUB_DIRTY                               0x2
#define HUB_SLEEP(x)                                 usb_wait_ms(x)
#define USBH_HUB_DEBOUNCE_TIME                       200
#define USBH_HUB_DEBOUNCE_STEP                       50
#define USBH_HUB_RESET_MIN_TIME                      10
#define USBH_HUB_RESET_MAX_TIME                      100
#define USBH_HUB_RESET_MAX_TRIES                     5
#define USBH_HUB_MAX_DEBOUNCE_ERRORS                 5
#define USBH_HUB_DESC_ID                             0x29
#define USBH_HUB_FEATURE_C_LOCAL_POWER               0x0
#define USBH_HUB_FEATURE_C_OVER_CURRENT              0x1

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

/* Downstream port U1 inactvity timer value 64 us. */
#define HUB_PORT_U1_TIMEOUT_VALUE                    0x7F

/* Downstream port U2 inactvity timer value. 1024 us*/
#define HUB_PORT_U2_TIMEOUT_VALUE                    0x04

/* Maximum hub depth for set_hub_depth request. */
#define USBH_HUB_MAX_DEPTH                           0x04
#endif

/* The following commands have a port as the recipient */
#define USBH_HUB_FEATURE_PORT_CONNECTION             0
#define USBH_HUB_FEATURE_PORT_ENABLE                 1
#define USBH_HUB_FEATURE_PORT_SUSPEND                2
#define USBH_HUB_FEATURE_PORT_OVER_CURRENT           3
#define USBH_HUB_FEATURE_PORT_RESET                  4
#define USBH_HUB_FEATURE_PORT_POWER                  8
#define USBH_HUB_FEATURE_PORT_LOW_SPEED              9

#define USBH_HUB_FEATURE_C_PORT_CONNECTION           16
#define USBH_HUB_FEATURE_C_PORT_ENABLE               17
#define USBH_HUB_FEATURE_C_PORT_SUSPEND              18
#define USBH_HUB_FEATURE_C_PORT_OVER_CURRENT         19
#define USBH_HUB_FEATURE_C_PORT_RESET                20
#define USBH_HUB_FEATURE_PORT_TEST                   21
#define USBH_HUB_FEATURE_PORT_INDICATOR              22
#define USBH_HUB_FEATURE_C_PORT_SRP                  30

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
#define USBH_SS_HUB_FEATURE_PORT_POWER               (1 << 9)
#define USBH_HUB_FEATURE_PORT_LINK_STATE             5
#define USBH_HUB_FEATURE_PORT_U1_TIMEOUT             23
#define USBH_HUB_FEATURE_PORT_U2_TIMEOUT             24
#define USBH_HUB_FEATURE_PORT_RW_MASK                27
#define USBH_HUB_FEATURE_PORT_RESET_BH               28
#define USBH_HUB_FEATURE_PORT_FLS_ACCEPT             30
#define USBH_HUB_FEATURE_C_PORT_LINK_STATE           25
#define USBH_HUB_FEATURE_C_PORT_CONF_ERR             26
#define USBH_HUB_FEATURE_C_PORT_RESET_BH             29
#endif

/* Hub (as the recipient) commands */
#define USBH_HUB_CMD_GET_STATUS                      0x0
#define USBH_HUB_CMD_CLEAR_FEATURE                   0x1
#define USBH_HUB_CMD_SET_FEATURE                     0x3
#define USBH_HUB_CMD_GET_DESCRIPTOR                  0x6
#define USBH_HUB_CMD_SET_DESCRIPTOR                  0x7

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
#define USBH_HUB_CMD_SET_DEPTH                       12
#define USBH_HUB_CMD_GET_PORT_ERR_CNT                13
#endif

/* Various BIT positions        */

/* wHubStatus   */
#define USBH_HUB_BIT_POWER_LOCAL                     (0x01 << 0)
#define USBH_HUB_BIT_OVER_CURRENT                    (0x01 << 1)

/* wHubChange   */
#define USBH_HUB_BIT_C_POWER_LOCAL                   (0x01 << 0)
#define USBH_HUB_BIT_C_OVER_CURRENT                  (0x01 << 1)

/* wPortStatus  */
#define USBH_HUB_BIT_PORT_CONNECTION                 (0x01 << 0)
#define USBH_HUB_BIT_PORT_ENABLE                     (0x01 << 1)
#define USBH_HUB_BIT_PORT_SUSPEND                    (0x01 << 2)
#define USBH_HUB_BIT_PORT_OVER_CURRENT               (0x01 << 3)
#define USBH_HUB_BIT_PORT_RESET                      (0x01 << 4)
#define USBH_HUB_BIT_PORT_POWER                      (0x01 << 8)
#define USBH_HUB_BIT_PORT_LOW_SPEED                  (0x01 << 9)
#define USBH_HUB_BIT_PORT_HIGH_SPEED                 (0x01 << 10)
#define USBH_HUB_BIT_PORT_SRP                        (0x1 << 15)

/* wPortChange  */
#define USBH_HUB_BIT_C_PORT_CONNECTION               (0x01 << 0)
#define USBH_HUB_BIT_C_PORT_ENABLE                   (0x01 << 1)
#define USBH_HUB_BIT_C_PORT_SUSPEND                  (0x01 << 2)
#define USBH_HUB_BIT_C_PORT_OVER_CURRENT             (0x01 << 3)
#define USBH_HUB_BIT_C_PORT_RESET                    (0x01 << 4)
#define USBH_HUB_BIT_C_PORT_SRP                      (0x1 << 15)

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
#define USBH_HUB_BIT_C_PORT_LS                       (0x01 << 5)
#define USBH_HUB_PORT_SS_MASK                         0x1C00
#define USBH_HUB_LINK_STATE_MASK                      0x01E0
#define USBH_SS_HUB_DESC_ID                           0x2A
#define USBH_SS_HUB_DESC_LENGTH                       12
#endif

#define usb_hub_set_port_feature(a, b, x, y, z)   \
    usb_hub_feature_req ((a), (b), (x), (y), (z),     \
                USBH_HUB_PORT_REQ, USBH_HUB_CMD_SET_FEATURE)

#define usb_hub_set_hub_feature(a, b, x)       \
    usb_hub_feature_req ((a), (b), (x), (0),(0),     \
                USBH_HUB_REQ, USBH_HUB_CMD_SET_FEATURE)

#define usb_hub_clear_port_feature(a, b, x, y, z) \
    usb_hub_feature_req ((a), (b), (x), (y), (z),         \
                USBH_HUB_PORT_REQ, USBH_HUB_CMD_CLEAR_FEATURE)

#define usb_hub_clear_hub_feature(a, b, x) \
    usb_hub_feature_req ((a), (b), (x), (0), (0),     \
                USBH_HUB_REQ, USBH_HUB_CMD_CLEAR_FEATURE)

#define usb_hub_get_port_status(a, x, y, z)    \
    usb_hub_status_req ((a), (x), (y), (z), USBH_HUB_PORT_REQ)

#define usb_hub_get_hub_status(a, x, y)    \
    usb_hub_status_req ((a), (x), (y), (0), USBH_HUB_REQ)

/* =================== Data Structures ===============================  */

typedef struct usb_hub_desc
{
    UINT8 bDescLength;                      /* Length of the descriptor */
    UINT8 bDescriptorType;                  /* 29h : HubDescriptor  */
    UINT8 bNbrPorts;                        /* Number of Ports  */
    UINT8 wHubCharacteristics0;             /* Hub description  */
    UINT8 wHubCharacteristics;              /* Hub description  */
    UINT8 bPwrOn2PwrGood;                   /* Time for this    */
    UINT8 bHubContrCurrent;                 /* Max Current requirements */

    /* The following is the array containing the last two members of the hub
     * descriptor. This array is created since the length of the these items
     * is not known at compile time. */
    /*
     * In addition to DeviceRemovable and PortPwrCtrlMask, there are two
     * more added, for roothubs, for otg support.These are SRPsupport and
     * HNPsupport.
     */
    UINT8 misc[4][(USBH_HUB_MAX_CHILDREN + 8) / 8];
}
USBH_HUB_DESC;

typedef USBH_HUB_DESC USBH_HUB20_DESC;

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
typedef struct usbh_hub30_desc
{
    /* Size of descriptor. */
    UINT8 bDescLength;

    /* Descriptor Type, DEVICE CAPABILITY type. */
    UINT8 bDescriptorType;

    /* Number of down stream ports in hub. */
    UINT8 bNrPorts;

    /* Please refer to Table 10-3 of USB 3.0 specification. */
    UINT16 wHubCharacteristics;

    /* Time in 2 – ms intervals from the time the power-on sequence begins
     * on a port until power is good on that port.
     */
    UINT8 bPwrOn2PwrGood;

    /* Maximum current requirement of the hub controller electronics when
     * hub is operating both in SS and USB 2.0 speeds.
     */
    UINT8 bHubContrCurrent;

    /* Hub packet header decode latency. */
    UINT8 bHubHdrDecLat;

    /* Average delay in nanoseconds a hub introduces on downstream 
     * following header packets.
     */
    UINT16 wHubDelay;

    /* Please refer to Table 10-3 of USB 3.0 specifications. */
    UINT16 DeviceRemovable;

}USBH_HUB30_DESC;
#endif

/* This structure contains the fields which are common in USB 2.0 and
 * USB 3.0 hubs.
 */
typedef struct usbh_hub_desc_header
{
    UINT8 bDescLength;                      /* Length of the descriptor */
    UINT8 bDescriptorType;                  /* HubDescriptor  */
    UINT8 bNbrPorts;                        /* Number of Ports  */
    UINT8 wHubCharacteristics0;             /* Hub description  */
    UINT8 wHubCharacteristics;              /* Hub description  */
    UINT8 bPwrOn2PwrGood;                   /* Time for this    */
    UINT8 bHubContrCurrent;                 /* Max Current requirements */
}USBH_HUB_DESC_HEADER;

#define USBH_HUB_MAX_DESC_LEN  30

struct nu_usbh_device_hub
{
    CS_NODE node;                           /* list of hub sessions */

    /* Identifies a session */
    NU_USB_DRVR      *drvr;
    NU_USB_STACK     *stack;
    NU_USB_DEVICE    *device;
    NU_USB_INTF      *intf;
    NU_USB_ALT_SETTG *alt_settg;

    UINT8            refCount;              /* Current reference count  */
    UINT8            state;                 /* NORMAL, SHUTDOWN, DIRTY  */
    UINT16           availablePower;        /* Power available. Chap 7  */
    UINT32           error_count;           /* < 5 to be good enough    */
    NU_USBH_CTRL_IRP *ctrlIrp;              /* Control IRP pointer       */
    NU_USB_PIPE      *default_pipe;         /* pipe for endpoint 0 */
    NU_USB_PIPE      *interrupt_pipe;       /* status IRP's pipe */
    USBH_IRP_HUB     status_irp[USBH_HUB_MAX_STATUS_IRPS];

    /* index to the child[] is port number starting from 1.
       Port 0 is the upstream facing port */
    NU_USB_DEVICE    *child[USBH_HUB_MAX_CHILDREN + 1];

    /* Array for holding the original Hub descriptor. */
    UINT8   *raw_desc;

    /* Pointer, pointing to a valid hub 2.0 descriptor if hub is enumerated
     * in high speed.
     */
    USBH_HUB20_DESC *hub20_desc;

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
    /* Pointer, pointing to a valid hub 3.0 descriptor if hub is enumerated
     * in super speed.
     */
    USBH_HUB30_DESC *hub30_desc;
#endif

    /* This points to a companion hub device (the hub device which has the
     * same ContainerID).
     */
    struct nu_usbh_device_hub  *companion_hub_device;

    /* Semaphore to protect ctrlIrp */
    NU_SEMAPHORE     lock;

    /* Variable to hold value of isoch delay. */
    UINT16           isoch_delay;
};

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
#define USBH_HUB_INIT_DESCRIPTOR(hub,speed) \
    if (speed == USB_SPEED_SUPER) \
    { \
        hub->hub30_desc = (USBH_HUB30_DESC *)hub->raw_desc; \
        hub->hub20_desc = NU_NULL; \
    }\
    else \
    { \
        hub->hub20_desc = (USBH_HUB20_DESC *)hub->raw_desc; \
        hub->hub30_desc = NU_NULL; \
    }
#else
#define USBH_HUB_INIT_DESCRIPTOR(hub,speed) \
        hub->hub20_desc = (USBH_HUB20_DESC *)hub->raw_desc;
#endif

#define USBH_HUB_THINK_TIME(p)    (((p)& 0x60) >> 5)

typedef struct nu_usbh_hub
{
    NU_USB_DRVR        driver;              /* Inherits from the base class */
    NU_USBH_DEVICE_HUB *session_list_head;  /* list of hubs the drvr owns */
    NU_TASK            hubTask;
    NU_MEMORY_POOL     *hubMemPool;
    UNSIGNED           irplist[USBH_HUB_QUEUE_MSGS];
    NU_QUEUE           queue;

    /* The semaphore irpCompleteLock is used for synchronization between the
     * control irp submission thread and the irp complete callback thread.
     */
    NU_SEMAPHORE irpCompleteLock;

    NU_USBH_STACK      *stack;              /* the associated stack */
}
NU_USBH_DRVR_HUB;

typedef NU_USB_DRVR_DISPATCH NU_USBH_HUB_DISPATCH;

/* ================== Function Prototypes ============================  */

STATUS NU_USBH_HUB_Create (NU_USBH_DRVR_HUB * cb,
                           NU_USBH_STACK * stack,
                           NU_MEMORY_POOL * pool,
                           VOID *hub_task_stack_address,
                           UNSIGNED hub_task_stack_size,
                           OPTION hub_task_priority);

/* HUB class APIs for the stack */
STATUS NU_USBH_HUB_Allocate_Power (NU_USBH_DRVR_HUB * cb,
                                   NU_USB_DEVICE * device,
                                   UINT16 power);

STATUS NU_USBH_HW_Release_Power (NU_USBH_DRVR_HUB * cb,
                                 NU_USB_DEVICE * device,
                                 UINT16 power);

STATUS NU_USBH_HUB_Disconnect (NU_USBH_DRVR_HUB * cb,
                               NU_USB_DEVICE * dev);

/* Class Driver API implementation prototypes */
STATUS USBH_HUB_Initialize_Intf (NU_USB_DRVR * cb,
                                NU_USB_STACK * stk,
                                NU_USB_DEVICE * dev,
                                NU_USB_INTF * intf);

STATUS _NU_USBH_HUB_Disconnect (NU_USB_DRVR * driver,
                                NU_USB_STACK * stk,
                                NU_USB_DEVICE * dev);

STATUS USBH_HUB_Disconnect_Device (NU_USBH_DRVR_HUB * cb,
                                   NU_USB_DEVICE * dev);

/* The delete function */
STATUS _NU_USBH_HUB_Delete (VOID *cb);

VOID USBH_Hub_Task (UNSIGNED argc,
                    VOID *argv);

STATUS usbh_hub_get_port_capabilities (NU_USBH_DRVR_HUB * hub_driver,
                                       NU_USB_DEVICE * hub_dev,
                                       UINT8 port_num,
                                       UINT8 *capability_out);

STATUS usbh_hub_suspend_port(NU_USBH_DRVR_HUB * hub_driver,
                             NU_USB_DEVICE * hub_dev,
                             UINT8 port_num);

STATUS usbh_hub_resume_port(NU_USBH_DRVR_HUB * hub_driver,
                            NU_USB_DEVICE * hub_dev,
                            UINT8 port_num);

NU_USBH_DEVICE_HUB *usb_find_hub (NU_USBH_DRVR_HUB * hcb,
                                  NU_USB_DEVICE * device);

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

STATUS usbh_hub_set_depth_req       (NU_USBH_STACK       *cb,
                                     NU_USBH_DEVICE_HUB  *hub,
                                     UINT8           hub_depth);

STATUS usbh_hub_get_port_err_cnt_req(NU_USBH_STACK       *cb,
                                     NU_USBH_DEVICE_HUB  *hub,
                                     UINT8               port,
                                     UINT16              *port_err_cnt);

STATUS usbh_hub_port_set_timeout    (NU_USBH_STACK       *cb,
                                     NU_USBH_DEVICE_HUB  *hub,
                                     UINT8               port_num,
                                     UINT8               feature_selector,
                                     UINT8               timeout_val);

STATUS usbh_hub_port_rw_mask        (NU_USBH_STACK       *cb,
                                     NU_USBH_DEVICE_HUB  *hub,
                                     UINT8               port_num,
                                     UINT8               remote_wake_mask);

STATUS usbh_hub_find_companion      (NU_USBH_DRVR_HUB   *cb,
                                     NU_USBH_DEVICE_HUB *hub,
                                     NU_USBH_DEVICE_HUB **companion_hub);

STATUS usbh_hub_port_set_link_state (NU_USBH_STACK       *cb,
                                     NU_USBH_DEVICE_HUB  *hub,
                                     UINT8               port_num,
                                     UINT8               link_state);

STATUS usbh_hub_port_suspend_link   (NU_USBH_STACK       *cb,
                                     NU_USBH_DEVICE_HUB  *hub,
                                     UINT8               port_num);

STATUS usbh_hub_port_resume_link    (NU_USBH_STACK       *cb,
                                     NU_USBH_DEVICE_HUB  *hub,
                                     UINT8               port_num);

#endif      /* ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE ) */

/* ==============  USB Include Files =================================  */
#include "connectivity/nu_usbh_hub_dat.h"

/* ===================================================================  */
#endif /* _NU_USBH_HUB_IMP_H_ */
/* ====================== end of file ================================  */

