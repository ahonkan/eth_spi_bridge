/*************************************************************************
*
*               Copyright 2005 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       nu_usbf_comm_imp.h
*
* COMPONENT
*
*       Nucleus USB Function Software : Communication Class Driver
*
* DESCRIPTION
*
*       This file contains the Control Block and other internal data
*       structures and definitions for Communication Class Driver.
*
* DATA STRUCTURES
*
*       NU_USBF_COMM_DISPATCH               Communication Class Driver
*                                           Dispatch Table.
*       NU_USBF_COMM                        Communication Class Driver
*                                           Control Block.
*       USBF_COMM_USER_NOTIFICATION         Notification structure for user
*       USBF_COMM_USER_CMD                  Command structure for user.
*       USBF_COMM_NOTIF_PKT                 Communication Class
*                                           notification packet.
*       USBF_COMM_DEVICE                    Communication Device structure
*                                           used throughout the driver.
*       COMMF_RX_BUF_GROUP                  Receive buffer group structure.
*       COMMF_BUF_QUEUE                     Buffer queue structure.
*       COMMF_DATA_CONF                     Data configuration structure.
*
* FUNCTIONS
*
*       None
*
* DEPENDENCIES
*
*       tc_defs.h                           Task control definitions.
*       nu_usbf_comm_dat.h                  Dispatch Table Definitions.
*       nu_usbf_drvr_ext.h                  Function class driver Interface
*                                           definition.
*
*************************************************************************/

/* ==================================================================== */

#ifndef _NU_USBF_COMM_IMP_H_
#define _NU_USBF_COMM_IMP_H_

#ifdef          __cplusplus
extern  "C"                                 /* C declarations in C++    */
{
#endif

/* ==================================================================== */
#ifndef _NUCLEUS_USB_H_
#error Do not directly include internal headers, use #include "nu_usb.h"
#endif
/* ==================================================================== */
#include "connectivity/nu_usbf_drvr_ext.h"

#if ((PLUS_VERSION_COMP == PLUS_1_14) || \
     (PLUS_VERSION_COMP == PLUS_1_13) || \
     (PLUS_VERSION_COMP == PLUS_1_11))
#include "plus/tc_defs.h"
#else
#include "kernel/nu_kernel.h"
#endif

/* =====================  Constants ================================  */

/* These definitions control which version of the user API/callbacks
   should be compatible with. This should allow new versions of API
   to be shipped but remain compatible with applications for
   previous versions. */

#define COMMF_1_5         1       /* COMMF 1.5 */
#define COMMF_2_0         2       /* COMMF 2.0 */

/* The version for which compatibility is desired. */
#ifndef COMMF_VERSION_COMP
    #define COMMF_VERSION_COMP    COMMF_2_0
#endif

#define COMMF_CLASS                     0x02

#define COMMF_SETUP_DIR                 (1<<7)

#define COMMF_NOTIF_REQ_TYPE            0xA1

/* This value should be updated for specific communication device. */
#define COMMF_MAX_CMD_LENGTH            256
#define COMMF_MAX_NOTIF_DATA_SIZE       8

#define COMMF_NOTIF_HDR_SIZE            8

#define COMMF_PENDING_QUEUE_COUNT       (4)
#define COMMF_BUF_GRP_QUEUE_COUNT       (2)
#define COMMF_SUBMISSION_QUEUE_SIZE     (10)

#define COMMF_SUB_HISR_PRI              (1)
/* If optimizations are enabled then use different values for macros. */
#if (NU_USB_OPTIMIZE_FOR_SIZE)
#define COMMF_SUB_HISR_STACK_SIZE       (4 * NU_MIN_STACK_SIZE)
#define COMMF_MAX_DEVICES               1
#else /* If no optimization is required. */
#define COMMF_SUB_HISR_STACK_SIZE       (4*1024)
#define COMMF_MAX_DEVICES               4
#endif /*#if (NU_USB_OPTIMIZE_FOR_SIZE) */

#define COMMF_RX_IN_PROGRESS            (1<<0)
#define COMMF_TX_IN_PROGRESS            (1<<1)
#define COMMF_RX_BUFF_GRP_MX            (1<<2)

/* Definitions for 'state' in USBF_COMM_DEVICE. */
#define COMMF_NOTIF_SENT                1
#define COMMF_GET_CMD_SENT              2
#define COMMF_DATA_SENT                 3
#define COMMF_SET_CMD_RCVD              4
#define COMMF_DATA_RCVD                 5
#define COMMF_RX_BUFFS_CALLBACK         6
#define COMMF_NOTIFY                    7
#define COMMF_MNG_NOTIF                 8
#define COMMF_DATA_MNG_NOTIF            9
#define COMMF_DISCONNECT_NOTIF          10
#define COMMF_DATA_DISCONNECT_NOTIF     11

/* Values for 'data_state' in USBF_COMM_DEVICE. */
#define COMMF_SEND_ZERO_LENGTH          1
#define COMMF_ZERO_LENGTH_SENT          2

/* Notification constants for Users. */
#define     COMMF_NO_NOTIF               0xFF
#define     COMMF_NOTIF_CMPLT_AWAITED   -3899

/* Notification for application in addition of USB bus events. */
#define COMMF_EVENT_USER_INIT           0x11
#define COMMF_EVENT_USER_DEINIT         0x12

/* Application dispatch function declarations. */
typedef VOID (*COMMF_APP_NOTIFY) (NU_USBF_USER *user, UINT32 event,
                                                            VOID *handle);

#if (COMMF_VERSION_COMP >= COMMF_2_0)
typedef VOID (*COMMF_APP_RX_CALLBACK) (UINT8 * data_buffer,
                                    UINT32 data_length,
                                    UINT8 *pkt_info_data,
                                    UINT32 pkt_info_length,
                                    VOID *handle);

typedef VOID (*COMMF_APP_TX_DONE) (UINT8 * cmpltd_data_buffer,
                                    UINT32 length, VOID *handle);

typedef VOID (*COMMF_APP_IOCTL) (NU_USBF_USER *user, UINT32 code,
                                            VOID *data, UINT32 length);

typedef VOID (*COMMF_BUFF_FINISHED) (NU_USBF_USER *user, VOID *buff_grp, VOID *handle);

#else

typedef VOID (*COMMF_APP_RX_CALLBACK) (UINT8 * data_buffer,
                                    UINT32 data_length,
                                    UINT8 *pkt_info_data,
                                    UINT32 pkt_info_length);


typedef VOID (*COMMF_APP_TX_DONE) (UINT8 * cmpltd_data_buffer,
                                    UINT32 length);

typedef VOID (*COMMF_APP_IOCTL) (NU_USBF_USER *user, UINT32 code,
                                            VOID *data, UINT32 length);

typedef VOID (*COMMF_BUFF_FINISHED) (NU_USBF_USER *user, VOID *buff_grp);

#endif

/* =============Dispatch Table ============= */
typedef struct _nu_usbf_comm_dispatch
{
    NU_USBF_DRVR_DISPATCH   dispatch;
    /* Extension to NU_USBF_DRVR services
     * should be declared here. */
}
NU_USBF_COMM_DISPATCH;

/* ============= Control Block ============= */

/* Notification sent by user. */
typedef struct _usbf_comm_user_notification
{
    UINT8   *data;                          /* Notification data buffer.*/
    UINT16  notif_value;                    /* Notification value.      */
    UINT8   notification;                   /* Notification code.       */
    UINT8   length;                         /* Notification data length.*/
}
USBF_COMM_USER_NOTIFICATION;

/* Command sent to user. */
typedef struct _usbf_comm_user_cmd
{
    UINT8   *cmd_data;                      /* Command data pointer.    */
    UINT16  cmd_index;                      /* Command index.           */
    UINT16  cmd_value;                      /* Command value.           */
    UINT16  data_len;                       /* Length of command data.  */
    UINT8   command;                        /* Command code.            */

    UINT8   pad[1];                         /* To properly align this
                                             * structure on 32-bit boundary
                                             */
}
USBF_COMM_USER_CMD;

/* Notification data packet sent to host, first 8 bytes have standard
 * header with data following.
 */
typedef struct _usbf_comm_notif_pkt
{
    /* These fields are defined in USB Class Definition for communication
     * devices.
     */
    UINT8  bmRequestType;
    UINT8  bNotification;
    UINT16 wValue;
    UINT16 wIndex;
    UINT16 wLength;

    /* Notification data array, length depends on the specific
     * notification.
     */
    UINT8  notif_data[COMMF_MAX_NOTIF_DATA_SIZE];
}
USBF_COMM_NOTIF_PKT;

/* This structure is used for registering receive buffers. */
typedef struct _commf_rx_buf_group
{
    CS_NODE  node;                          /* List node element.       */
    COMMF_BUFF_FINISHED  callback;          /* Buffer finish callback.  */

    /* Array containing buffer pointers.*/
    UINT8** buffer_array;

    /* Number of buffers in array.*/
    UINT32 num_of_buffers;
    UINT32 buffer_size;                    /* Size of each buffer.     */
}
COMMF_RX_BUF_GROUP;

typedef struct _commf_rx_buf_group_impl
{
    COMMF_RX_BUF_GROUP * buffer_group;
    UINT32 buffer_index;
}
COMMF_RX_BUF_GROUP_IMPL;

/*
 * This structure is used to store pending TX transfers.
 */
typedef struct _commf_transfer
{
    VOID * buffer;
    UINT32 length;
}COMMF_TRANSFER;

/*
 * This structure is used to pass completion context information
 * to user driver.
 */
typedef struct _commf_cmpl_ctx
{
    VOID    *handle; /* Device pointer */
    STATUS status;
    UINT32 transfer_type;
}COMMF_CMPL_CTX;

/* This structure is passed for initialization of internal buffer queues
 * and other transfer parameters.
 */
typedef struct _commf_data_conf
{
    /* Completed receive buffer pointers array. */
    UINT8** cmpltd_rx_buffer_list;

    /* Completed receive buffer lengths array. */
    UINT32 *cmpltd_rx_len_list;

    /* Number of buffers in completed receive queue. */
    UINT32 max_cmpltd_rx_buffers;

    /* Pending transmit buffer pointers array. */
    UINT8** pend_tx_buffer_list;

    /* Pending transmit buffer lengths array. */
    UINT32 *pend_tx_len_list;

    /* Number of buffers in pending transmit queue. */
    UINT32 max_pend_tx_buffers;

    /* Completed transmit buffer pointers array. */
    UINT8** cmpltd_tx_buffer_list;

    /* Completed transmit buffer lengths array. */
    UINT32 *cmpltd_tx_len_list;

    /* Number of buffers in completed transmit queue. */
    UINT32 max_cmpltd_tx_buffers;

    /* Frame/transfer delineation flag. */
    BOOLEAN delineate;

    /* To properly align this structure on 32-bit boundary */
    UINT8   pad[3];
}
COMMF_DATA_CONF;

/* Communication device structure, shared by both management and data
 * class interface drivers.
 */
typedef struct _usbf_comm_device
{
    NU_USB_DEVICE *dev;                     /* USB device               */
    NU_USB_IRP in_irp;                      /* Bulk Transfer.           */
    NU_USB_IRP out_irp;                     /* Bulk Transfer.           */
    NU_USB_IRP intr_irp;                    /* Interrupt Transfer.      */
    NU_USB_IRP ctrl_irp;                    /* Control Transfer.        */
    USBF_COMM_NOTIF_PKT  curr_notification; /* Current notification
                                               packet. */
    USBF_COMM_USER_CMD   curr_command;      /* Code for current command.*/

    NU_EVENT_GROUP internal_events;

    NU_QUEUE pending_tx_queue;

    NU_QUEUE rx_buf_grp_queue;

    NU_PROTECT rx_submit_prot;

    NU_PROTECT tx_submit_prot;

    /* Which alternate setting am i on */
    NU_USB_ALT_SETTG *master_alt_settg;

    /* Which alternate setting am i on */
    NU_USB_ALT_SETTG *slave_alt_settg;

    /* Who is the user for me */
    NU_USB_USER *user;

    /* Management element driver. */
    NU_USBF_DRVR *mng_drvr;

    /* Data element driver*/
    NU_USBF_DRVR *data_drvr;

    /* Master interface (Communication class interface. */
    NU_USB_INTF *master_intf;

    /* Slave interface, Data Class interface. */
    NU_USB_INTF *slave_intf;
    NU_USB_PIPE *in_pipe;                   /* Bulk IN endpoint.        */
    NU_USB_PIPE *out_pipe;                  /* Bulk OUT endpoint.       */
    NU_USB_PIPE *notif_pipe;                /* Interrupt IN endpoint.   */
    NU_USB_PIPE *ctrl_pipe;                 /* Default control endpoint.*/

    VOID * cmplt_rx_buffer;                 /* Pointer to last received
                                             * data buffer.
                                             */

    UINT32 cmplt_rx_buffer_len;             /* Length of last received
                                             * data buffer.
                                             */

    UINT8   *tx_data;                       /* Transmitted data buffer. */

    UINT32  tx_data_len;                    /* Transmitted data length. */

    /* Maximum command data length. */
    UINT8 cmd_buffer[COMMF_MAX_CMD_LENGTH];

    UINT8 pending_tx_queue_buffer[COMMF_PENDING_QUEUE_COUNT*
                                  sizeof(COMMF_TRANSFER)];

    UINT8 rx_buf_grp_queue_buffer[COMMF_BUF_GRP_QUEUE_COUNT*
                                  sizeof(COMMF_RX_BUF_GROUP_IMPL)];

    UINT8 submission_queue_buffer[COMMF_SUBMISSION_QUEUE_SIZE*
                                  sizeof(UINT32)];

    UINT8 submission_hisr_stack[COMMF_SUB_HISR_STACK_SIZE];

    /*
     * This flag is set to NU_TRUE when singleton functionality of
     * communication and data interface of the class driver
     * has been initialized.
     */
    BOOLEAN   is_valid;

    UINT8   data_state;                 /* Transmission state.       */

    BOOLEAN delineate;                  /*Transfer delineation flag. */

    UINT8 pad[1];
}
USBF_COMM_DEVICE;

/* Communication class control block. */
typedef struct _nu_usbf_comm
{
    NU_USBF_DRVR parent;                /* Parent driver control block. */

    /* Communication device for this driver. */
    USBF_COMM_DEVICE devices[COMMF_MAX_DEVICES];

    NU_MEMORY_POOL *mem_pool;           /* Memory pool pointer.     */
}
NU_USBF_COMM;

/* ====================  Function Prototypes ========================== */

VOID COMMF_Notify_IRP_Complete (NU_USB_PIPE * pipe, NU_USB_IRP * irp);

VOID COMMF_Set_Cmd_IRP_Complete (NU_USB_PIPE * pipe, NU_USB_IRP  *irp);

VOID COMMF_Get_Cmd_IRP_Complete (NU_USB_PIPE * pipe, NU_USB_IRP  *irp);

STATUS COMMF_Mng_Connect_User(USBF_COMM_DEVICE *device);

STATUS COMM_Handle_USB_Event(NU_USBF_COMM *comm, USBF_COMM_DEVICE *device,
                                            INT index);

STATUS COMMF_Handle_Mng_Init(NU_USBF_COMM *comm, USBF_COMM_DEVICE *device);

STATUS COMMF_Handle_Cmd_Rcvd(NU_USBF_COMM *comm,
        USBF_COMM_DEVICE *device,
        UINT8 state);

STATUS COMMF_Handle_Tx_Done(NU_USBF_COMM *comm, USBF_COMM_DEVICE *device,
                                            UINT8 state);

STATUS    COMMF_Init_Tx_Queue(USBF_COMM_DEVICE *device);

STATUS    COMMF_Uninit_Tx_Queue(USBF_COMM_DEVICE *device);

VOID COMMF_Error_Line(
       CHAR * filename,
       UINT32 lineno,
       STATUS errorcode);

STATUS COMMF_Init_Internals(
            USBF_COMM_DEVICE * device);

STATUS COMMF_Uninit_Internals(
            USBF_COMM_DEVICE * device);

STATUS COMMF_Clear_Event(
            USBF_COMM_DEVICE * device,
            UINT32 events);

STATUS COMMF_Set_Event(
            USBF_COMM_DEVICE * device,
            UINT32 events);

STATUS COMMF_Get_Event(
            USBF_COMM_DEVICE * device,
            UINT32 events,
            BOOLEAN * bAvailable,
            OPTION suspend,
            OPTION operation);

VOID COMMF_IO_Sch_Timer_Func(UNSIGNED id);

/* ==================================================================== */

#include "connectivity/nu_usbf_comm_dat.h"

/* ==================================================================== */

#ifdef          __cplusplus
}                                           /* End of C declarations    */
#endif

#endif      /* _NU_USBF_COMM_IMP_H_ */

/* ======================  End Of File  =============================== */

