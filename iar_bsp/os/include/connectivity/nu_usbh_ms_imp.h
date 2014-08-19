/**************************************************************************
*
*               Copyright 2003 Mentor Graphics Corporation
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
*       nu_usbh_ms_imp.h
*
*
* COMPONENT
*
*       Nucleus USB Host Software.
*
* DESCRIPTION
*
*       This file contains the Control Block and other internal data
*       structures and definitions for Mass Storage Class Driver.
*
* DATA STRUCTURES
*
*       NU_USBH_MS                         Mass Storage Class Driver
*                                          Control Block.
*       USBH_MS_CB                         Command Block which contains a
*                                          command to be transmitted to
*                                          device.
*       MS_BOT_CBW                         Command Block Wrapper.
*       MS_BOT_CSW                         Command Status Wrapper.
*       MS_LUN                             This structure maintains
*                                          information about each LUN on
*                                          the device. This is allocated
*                                          per LUN and passed as an handle
*                                          to USER's.
*       NU_USBH_MS_DRIVE                   This structure maintains
*                                          information about each Mass
*                                          Storage Interface served by
*                                          this class driver.
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       nu_usbh_ms_dat.h                    Dispatch Table Definitions.
*       nu_usbh_ext.h                       USB Host definitions.
*
**************************************************************************/

/* ===================================================================== */

#ifndef     _NU_USBH_MS_IMP_H_
#define     _NU_USBH_MS_IMP_H_

/* ===================================================================== */

#ifndef     _NUCLEUS_USB_H_
#error Do not directly include internal headers \
use #include "common/stack/inc/nu_usb.h"
#endif

/* ====================  USB Include Files  ============================ */



/* BInterfaceProtocol's. */

#define     MS_PR_CBI               0x00    /* Control/Bulk/Interrupt.   */
#define     MS_PR_CB                0x01    /* Control/Bulk.             */
#define     MS_PR_BULK              0x50    /* bulk only.                */
#define     MS_PR_ANY               0xFF    /* Don't care of protocol.   */

/* bInterfaceSubClasses. */

#define     MS_SC_RBC               0x01    /* flash devices.            */
#define     MS_SC_8020              0x02    /* CD's DVD's.               */
#define     MS_SC_QIC               0x03    /* Tapes.                    */
#define     MS_SC_UFI               0x04    /* Floppy.                   */
#define     MS_SC_8070              0x05    /* Floppies(mostly).         */
#define     MS_SC_SCSI              0x06    /* SCSI devices.             */
#define     MS_SC_RAMD              0x13    /* Ram disk device.          */

/* Bulk Only Transport CSW return flags. */

#define     MS_CSW_GOOD             0x00
#define     MS_CSW_FAIL             0x01
#define     MS_CSW_PHASE_ERROR      0x02

/* CSW Validity. */
#define     MS_CSW_OK               1
#define     MS_CSW_BAD              0

/* Swapped cbw signature 0x55534243. */

/* spells out 'USBC'. */
#define     MS_CBW_SIGNATURE        0x43425355

/* Swapped csw signature 0x55534253. */

/* spells out 'USBS'. */
#define     MS_CSW_SIGNATURE        0x53425355

/* Direction. */
#define     MS_CBW_FLAG_IN          0x80
#define     MS_CBW_FLAG_OUT         0x00

#define     MS_MAX_CBW_LEN          0x1f
#define     MS_MAX_CB_LEN           0x0D
#define     MS_MAX_CSW_LEN          0x0D

/* class specific requests. */
#define     MS_BULK_RESET_REQUEST   0xff
#define     MS_BULK_GET_MAX_LUN     0xfe
#define     MS_CBI_ADSC             0x00

/* return codes. */
#define     NU_USBH_MS_TRANSPORT_FAILED   NU_USB_STATUS_BASE - 83
#define     NU_USBH_MS_TRANSPORT_ABORTED  NU_USB_STATUS_BASE - 84
#define     NU_USBH_MS_TRANSPORT_ERROR    NU_USB_STATUS_BASE - 85

#define     NU_USBH_MS_UNSUPPORTED        NU_USB_STATUS_BASE - 86
#define     NU_USBH_MS_DUPLICATE_INIT     NU_USB_STATUS_BASE - 87
#define     NU_USBH_MS_NOT_INITIALIZED    NU_USB_STATUS_BASE - 88

/* Queue message identifiers  */
#define     NU_USBH_MS_DISCONNECTED       0x01
#define     NU_USBH_MS_CONNECTED          0x00

/* User may change the time interval. Range (1 – 4,294,967,293).
 * The calling task is suspended until the  semaphore is obtained or until
 * the specified number of ticks has expired. Currently timeouts are set
 * with suspend option.
 */

/* USER CONFIGUREABLE PARAMETERS. */
/* These are the suspend option when UMS class driver requests IRP to USB
 * stack. User is advised to set these values to NU_SUSPEND. */
#define     NU_USBH_MS_DRIVE_LOCK_TIMEOUT  NU_SUSPEND
#define     NU_USBH_MS_TX_LOCK_TIMEOUT     NU_SUSPEND

#define CALCULATE_NUM_SUBCLASSES(a,b,c,d)    a+b+c+d

/*This is the max number of protocols which may exist.Since each protocol
  *number will work as the index to the array so this array must be large
  *enough to accommodate all of them The largest number is MS_SC_RAMD=0x13 */
  
#define MAX_NUM_SUBCLASSES                  CALCULATE_NUM_SUBCLASSES(INCLUDE_SCSI,\
                                                                      INCLUDE_UFI,\
                                                                      INCLUDE_SFF8020I,\
                                                                      INCLUDE_SFF8070I)

/* Maximum logical units to be supported by UMS class driver on a single
* USB storage device. */
#define     NU_USBH_MS_MAX_LUNS            CFG_NU_OS_CONN_USB_HOST_MS_CLASS_MAX_LUNS_PER_DEVICE
/*---------------------------Setting Macros-------------------------------*/
/* UMS class driver has an internal task to report the asynchronous events
* like connection and disconnection to upper layer. These are the task
* related parameters. */

#define     NU_USBH_MS_MAX_DEVICES             CFG_NU_OS_CONN_USB_HOST_MS_CLASS_MAX_DEVICES
#define     MSC_EVENT_REPORTER_TASK_STACK\
            CFG_NU_OS_CONN_USB_HOST_MS_CLASS_EVENT_REPORTER_TASK_STACKSIZE_FACTOR *\
            NU_MIN_STACK_SIZE 
#define     MSC_EVENT_REPORTER_TASK_PRIORITY\
            CFG_NU_OS_CONN_USB_HOST_MS_CLASS_EVENT_REPORTER_TASK_PRIORITY /*Task Priority 0-255*/
/*Task Preemption ,Preemptable or non Preemptable*/
#if CFG_NU_OS_CONN_USB_HOST_MS_CLASS_EVENT_REPORTER_TASK_PREEMPTION_ENABLE
#define     MSC_EVENT_REPORTER_TASK_PREEMPTION NU_PREEMPT
#else
#define     MSC_EVENT_REPORTER_TASK_PREEMPTION NU_NO_PREEMPT
#endif
#define     MSC_EVENT_REPORTER_TASK_TIMESLICE\
            CFG_NU_OS_CONN_USB_HOST_MS_CLASS_EVENT_REPORTER_TASK_TIMESLICE

/* For each connection and disconnection reporting to upper layer a PIPE
* mechanism is used. This parameter defines the total buffer size for the
* pipe. 1024 bytes means the UMS class driver can report 128 connections
* to upper layer simultaneously. */

#define     NU_USBH_MS_MSG_SIZE            2
#define     NU_USBH_MS_QUEUE_ELEMENTS      (2*NU_USBH_MS_MAX_DEVICES)
#define     NU_USBH_MS_QUEUE_SIZE          (NU_USBH_MS_QUEUE_ELEMENTS *  \
                                            NU_USBH_MS_MSG_SIZE *        \
                                            sizeof(UNSIGNED))
/*------------------------------MACROS------------------------------------*/
#define     MSC_CONNECT_MESSAGE            0x10101010
#define     MSC_DISCONNECT_MESSAGE         0x11111111
#define     NU_USBH_MS_CB_SIZE             12
#define     NU_USBH_MS_CBW_CB_SIZE         16
#define     NU_USBH_MS_IRQ_DATA_SIZE         2
/* ====================  Data Types  =================================== */

/* ====== Bulk Only Data Structures ====== */

/* Command Status Wrapper. */
typedef struct ms_bot_csw
{
    UINT32 signature;
    UINT32 tag;
    UINT32 data_residue;
    UINT8 status;
    /* Padding bytes. */
    UINT8 pad[3];
}   MS_BOT_CSW;
/* Command Status Wrapper. */

/* Command Block Wrapper. */
typedef struct ms_bot_cbw
{
    UINT32 signature;
    UINT32 tag;
    UINT32 data_transfer_length;
    UINT8 flags;
    UINT8 lun;
    UINT8 cb_length;
    UINT8 cb[NU_USBH_MS_CBW_CB_SIZE];
    /* Padding bytes. */
    UINT8 pad;
}   MS_BOT_CBW;
/* Command Block Wrapper. */

/* LUN mapping structure. */
typedef struct ms_lun
{
    VOID            *user_pointer;
    struct ms_drive *drive;
    UINT8           lun;
    UINT32  block_size;
    UINT8           pad_idx;    /*used to store index generated
                                  *by user driver for new api*/
}   MS_LUN;
/* LUN mapping structure. */

/* ======================== Internal Data Structures =================== */

/* Each device connected to Mass Storage Class Driver is
 * remembered by this structure.
 */

typedef struct ms_drive
{
    CS_NODE node;

    /* Session identification. */
    NU_USB_DRVR  *drvr;
    NU_USB_DEVICE *device;
    NU_USB_STACK *stack;
    NU_USB_INTF  *intf;
    NU_USB_USER  *user;                      /* Associated User.*/
    NU_USB_ALT_SETTG *alt_settg;

    MS_LUN lun_info[NU_USBH_MS_MAX_LUNS];

    /* information about the device. */
    UINT8        protocol;                  /* bInterfaceProtocol*/
    UINT8        device_state;               /* state of the bus*/
    UINT16       dummy2;                    /* Ensure that max_lun is 4
                                             * byte aligned */
    UINT8        max_lun;                   /* Logical Unit Number.*/
    NU_SEMAPHORE drive_lock;                /* Protect access per drive. */
    NU_SEMAPHORE tx_request;                /* This semaphore is used to
                                             * synchronize interrupt bulk
                                             * and control IRP submission
                                             * thread.                   */
    /* Pipes required by the driver. */
    NU_USB_PIPE *bulk_in_pipe;
    NU_USB_PIPE *bulk_out_pipe;
    NU_USB_PIPE *control_pipe;
    NU_USB_PIPE *interrupt_pipe;

    NU_USB_IRP   bulk_in_irp;
    NU_USB_IRP   bulk_out_irp;
    NU_USB_IRP   interrupt_irp;
    NU_USBH_CTRL_IRP   *ctrl_irp;
    UINT32       ms_cbw_tag;                          /* our CBW tag. */
    /* Pointer to Bulk Only Transfer, CBW and CSW. */
    UINT8       *cbw_ptr;
    UINT8       *csw_ptr;
    UINT8       *irq_data;                            /* Meant only for CBI.*/
    UINT8        subclass;                            /*subclass protocol*/
    BOOLEAN      is_data_buff_cachable;
    UINT8 dw_pad[3];

}   NU_USBH_MS_DRIVE;

/* Command Block Structure used by Internal Transport Routines
 * to transfer a command across USB
 */

typedef struct usbh_ms_cb
{

    VOID *command;                          /* SubClass specific command.*/
    UINT8 cmd_length;                       /* command length.           */
    VOID *data_buf;                         /* Data in/out buffer.       */
    UINT32 buf_length;                      /* data buffer length.       */
    UINT8 direction;                        /* data transfer direction.  */
                                            /* USB_DIR_IN/USB_DIR_OUT.   */
}   USBH_MS_CB;

/* =========================== Control Block =========================== */

/* Mass Storage Class Driver Control Block. */
typedef struct nu_usbh_ms
{
    NU_USB_DRVR cb;                         /* The base class Control
                                             * Block */
    NU_USBH_MS_DRIVE *session_list_head;    /* list of MS drives it owns */
    NU_MEMORY_POOL *ms_mem_pool;
    NU_SEMAPHORE    driver_lock;
    NU_SEMAPHORE    delete_lock;
    NU_TASK         disconn_task;           /* Connection/Disconnection
                                             * monitoring task           */
    NU_QUEUE        ms_queue;               /* MS queue to hold connect and
                                             * disconnect events         */
    VOID*           p_task_stack;           /* Task stack */
    VOID*           p_queue_mem;            /* Memory pool for the queue */
    UINT32          queue_av;               /* Current tally of event space
                                             * left in the queue         */
    INT32           connect_count;          /* Current tally of connect
                                             * events residing in queue  */

}   NU_USBH_MS;


/* ====================  Function Prototypes  ========================== */

/* Class Driver API implementation prototypes. */

NU_USB_USER *UHMS_Find_User (NU_USBH_MS * cb,
                                UINT8 subclass);

STATUS UHMS_Bulk_Reset_Recovery (NU_USBH_MS * cb,
                                NU_USB_STACK * stack,
                                NU_USBH_MS_DRIVE * currDrive);

STATUS UHMS_Bulk_Get_Max_LUN (NU_USBH_MS * cb,
                                NU_USB_STACK * stack,
                                NU_USBH_MS_DRIVE * currDrive);

STATUS UHMS_CBI_Send_ADSC (NU_USBH_MS * cb,
                                NU_USB_STACK * stack,
                                NU_USBH_MS_DRIVE * currDrive,
                                USBH_MS_CB * cmd_blk);

STATUS UHMS_CBI_Command_Block_Reset (NU_USBH_MS * cb,
                                NU_USB_STACK * stack,
                                NU_USBH_MS_DRIVE * currDrive);

UINT8 UHMS_Validate_CSW (NU_USBH_MS_DRIVE * currDrive,
                                MS_BOT_CSW csw,
                                USBH_MS_CB * cmd_blk);

STATUS UHMS_Bulk_Transport (NU_USBH_MS * cb,
                                NU_USB_STACK * stack,
                                NU_USBH_MS_DRIVE * currDrive,
                                UINT8 lun,
                                USBH_MS_CB * cmd_blk);

STATUS UHMS_CBI_Transport (NU_USBH_MS * cb,
                                NU_USB_STACK * stack,
                                NU_USBH_MS_DRIVE * currDrive,
                                USBH_MS_CB * cmd_blk);

STATUS UHMS_CB_Transport (NU_USBH_MS * cb,
                                NU_USB_STACK * stack,
                                NU_USBH_MS_DRIVE * currDrive,
                                USBH_MS_CB * cmd_blk);

VOID UHMS_IRP_Complete (NU_USB_PIPE * pipe,
                                NU_USB_IRP * irp);

VOID UHMS_Intr_Callback (NU_USB_PIPE * pipe,
                                NU_USB_IRP * irp);

NU_USBH_MS_DRIVE*  UHMS_Validate_Drive (NU_USBH_MS   * cb,
                                VOID         * session);

BOOLEAN UHMS_Validate_Device(NU_USBH_MS          * cb,
                                NU_USBH_MS_DRIVE    * drive);

VOID UHMS_Event_Reporter (UNSIGNED argc,
                                VOID * cb);

STATUS UHMS_Submit_IRP (NU_USBH_MS_DRIVE*   currDrive,
                                NU_USB_PIPE*        p_pipe,
                                NU_USB_IRP*         p_irp);

/* ===================================================================== */

#include "connectivity/nu_usbh_ms_dat.h"

/* ===================================================================== */

#endif      /* _NU_USBH_MS_IMP_H_ */

/* ====================  End Of File  ================================== */
