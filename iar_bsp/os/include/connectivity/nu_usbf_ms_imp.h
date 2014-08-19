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
*       nu_usbf_ms_imp.h
*
* COMPONENT
*
*       Nucleus USB Function Mass Storage class driver.
*
* DESCRIPTION
*
*       This file contains the internal declarations for the Mass Storage
*       class driver.
*
* DATA STRUCTURES
*
*       NU_USBF_MS                          Mass Storage class driver
*                                           control block.
*       USBF_MS_DEVICE                      Internal representation of Mass
*                                           Storage device.
*       MSF_BOT_CBW                         Command Block Wrapper
*                                           structure.
*       MSF_BOT_CSW                         Command Status Wrapper
*                                           structure.
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       nu_usbf_drvr_ext.h                  External interface for driver.
*       nu_usbf_user_ext.h                  External interface for user
*                                           driver.
*       tc_defs.h                           Plus related definitions.
*
**************************************************************************/
#ifndef _NU_USBF_MS_IMP_H

#ifdef  __cplusplus
extern  "C" {                                /* C declarations in C++. */
#endif

#define _NU_USBF_MS_IMP_H

/* =========================  USB Include Files ======================== */

#include "connectivity/nu_usbf_drvr_ext.h"
#include "connectivity/nu_usbf_user_ext.h"

#if ((PLUS_VERSION_COMP == PLUS_1_14) || \
     (PLUS_VERSION_COMP == PLUS_1_13) || \
     (PLUS_VERSION_COMP == PLUS_1_11))

#include "plus/tc_defs.h"
#else
#include "kernel/nu_kernel.h"
#endif

#define UMSF_VER_2_1
/* =========================  Global data =============================  */

/* Mass Storage Class Code. */
#define USB_SPEC_MSF_CLASS                   0x08
#define USB_SPEC_MSF_SUBCLASS_SCSI           0x06
#define USB_SPEC_MSF_PROTOCOL_BOT            0x50

/* Bulk Only Transport(BOT)Specific definitions. */
#define USB_SPEC_MSF_CBW_SIGNATURE           (UINT32)0x43425355
#define USB_SPEC_MSF_CSW_SIGNATURE           (UINT32)0x53425355
#define USB_SPEC_MSF_CLASS_RESET             0xFF
#define USB_SPEC_MSF_CLASS_GET_MAX_LUN       0xFE
#define USB_SPEC_MSF_CBW_LENGTH              31
#define USB_SPEC_MSF_CBW_LUN_MASK            0x0F
#define USB_SPEC_MSF_CBW_RESERVED_BITS_MASK  0xF0
#define USB_SPEC_MSF_CSW_LENGTH              13
#define USB_SPEC_MSF_CBW_CMD_LEN             15
#define USB_SPEC_MSF_CSW_PHASE_ERROR         0x2
#define USB_SPEC_MSF_CSW_CMD_FAILED          0x1
#define USB_SPEC_MSF_CBW_CMD_LEN_MAX         16
#define USBF_SPEC_MSF_CBW_LUN_MASK           0x1F
#define USBF_SPEC_MSF_CBWCB_LUN_BYTE         0x1

/* CBW Validity. */
#define     MS_CBW_OK                        1
#define     MS_CBW_BAD                       0


/* Maximum Interfaces supported by mass storage class driver. */
#define NU_USBF_MS_MAX_INTERFACES            0x3
#define NU_USBF_MS_COMMAND                   0x1
#define NU_USBF_MS_DATA_STATUS               0x2
#define NU_USBF_MS_CLEAR_FEATURE             0x10

#define NU_USBF_MS_SEND_STALL                0x20
#define NU_USBF_MS_SEND_CSW                  0x30
#define NU_USBF_MS_IO_PENDING                0x29

#define NU_USBF_MS_DELETE_TASK               0x4
#define NU_USBF_MS_MEM_TASK                  0x3
#define NU_USBF_MS_MEM_STACK                 0x2
#define NU_USBF_MS_DELETE_EVENT              0x1
#define NU_USBF_MS_SHIFT_FIVE_BITS           5
#define NU_USBF_MS_TX_RX_DONE           0x8

#define USB_SPEC_MSF_CSW_CMD_SUCCESS         0x00
/* Clear Feature event is added in stack 2.6 version, define it to make
 * function mass storage able to comply with previous versions of stack.
 */
#ifndef USBF_EVENT_CLEAR_HALTENDPOINT
#define USBF_EVENT_CLEAR_HALTENDPOINT        0x0007
#endif

/* Command Block Wrapper definition. Refer to the Mass Storage Bulk Only
 * Transport specifications for more details.
 */
typedef struct _msbot_cbw
{
    UINT32               dCBWSignature;
    UINT32               dCBWTag;
    UINT32               dCBWTransferLength;
    UINT8                bCBWFlags;
    UINT8                bCBWLUN;
    UINT8                bCBWCBLength;
    UINT8                cbwcb[USB_SPEC_MSF_CBW_CMD_LEN_MAX];

#if     PAD_3
    DATA_ELEMENT         cs_padding[PAD_3];
#endif

}   MSF_BOT_CBW;

/* Command Status Wrapper definition. Refer to the Mass Storage Bulk Only
 * Transport specifications for more details.
 */
typedef struct _msbot_csw
{
    UINT32               dCSWSignature;
    UINT32               dCSWTag;
    UINT32               dCSWResidue;
    UINT8                bCSWStatus;

#if     PAD_1
    DATA_ELEMENT         cs_padding[PAD_1];
#endif

}   MSF_BOT_CSW;

/* Mass storage device control block. */
typedef enum {
              USBF_MS_BOT_STAGE_NONE, 
              USBF_MS_BOT_STAGE_CMD, 
              USBF_MS_BOT_STAGE_DATA, 
              USBF_MS_BOT_STAGE_STATUS, 
              USBF_MS_BOT_STAGE_STALL
              }
               _USBF_MS_BOT_STAGE;

typedef struct _usbf_ms_device
{
    NU_USB_DEVICE       *dev;                /* Pointer to USB device.      */
    NU_USB_INTF         *intf;               /* Pointer to interface
                                              * control block for which
                                              * request has been made.      */
    NU_USB_DRVR         *usb_drvr;
    NU_USB_ALT_SETTG    *alt_settg;          /* Pointer to alternate
                                              * setting control block.      */
    NU_USB_USER         *user;               /* Pointer to the user of this
                                              * driver.                     */
    NU_USB_STACK        *stack;              /* Pointer to stack control
                                              * block.                      */
    NU_USBF_DRVR        *drvr;               /* Pointer to the class parent
                                              * control block.              */
    NU_USB_PIPE         *in_pipe;            /* Bulk IN endpoint.           */
    NU_USB_PIPE         *out_pipe;           /* Bulk OUT endpoint.          */
    NU_USB_PIPE         *ctrl_pipe;          /* Default control endpoint.   */
    UINT8               *data;               /* Data to be transferred.     */
    MSF_BOT_CBW         *cbw;                /* Last received CBW from
                                              * host.                       */
    MSF_BOT_CSW         *csw;                /* Last sent CSW to the Host.  */
    UINT8               *max_lun;            /* Maximum LUNs supported.     */
    NU_TASK             *ms_task;            /* MS task control block.      */
    VOID                *ms_stack;           /* MS task stack pointer.      */
    NU_USB_IRP          *data_irp_complete;  /* BULK IRP ( IN or OUT).      */
    NU_USB_PIPE         *data_pipe;          /* BULK PIPE (IN or OUT).      */
    NU_USB_IRP          bulk_out_irp;        /* Bulk transfer IRP.          */
    NU_USB_IRP          bulk_in_irp;         /* Bulk transfer IRP.          */
    NU_USB_IRP          ctrl_irp;            /* Control transfer IRP.       */
    NU_EVENT_GROUP      event_group;         /* Event group for task and
                                              * HISR synchronization.       */
    NU_EVENT_GROUP      rw_events;           /* Event group for read and
                                              * write functions.            */
    UINT32              data_len;            /* Length of data to be
                                              * transferred.                */
    UINT32              residue;             /* Data residue if any.        */
    UINT32              remaining_length;    /* Length of data to be
                                              * transferred.                */
    UINT8               roll_back;           /* For Task memory rollback.   */
    UINT8               cmd_failed;          /* Command failed.             */
    volatile UINT8      invalid_cbw;         /* Invalid CBW flag.           */
    UINT8               phase_error;         /* Flag to indicate phase error.*/
    UINT8               data_direction;
    BOOLEAN             in_use;              /* Is the device free or in
                                              * use.                        */
    _USBF_MS_BOT_STAGE   bot_stage;          /* Holds current BOT stage.    */
    UINT8                dummy;                                           
}   NU_USBF_MS_DEVICE;

/* Mass Storage control block. */
typedef struct _nu_usbf_ms
{
    NU_USBF_DRVR         parent;             /* Parent control block. */

    /* Mass storage devices array. Each interface is treated as a device. */
    NU_USBF_MS_DEVICE    devices[NU_USBF_MS_MAX_INTERFACES];

    /* Function controller capabilities. */
    UINT32               hw_capability;
    NU_MEMORY_POOL       *mem_pool;
    
    /* Device configuration allocated USB function. */
    USBF_FUNCTION       *ms_function;

    UINT8                slot_no;            /* Index of device array. */

#if     PAD_1
    DATA_ELEMENT         cs_padding[PAD_1];
#endif

}   NU_USBF_MS;

/* =======================  Function Prototypes =======================  */
VOID NU_MSF_Data_IRP_Complete (
     NU_USB_PIPE               *pipe,
     NU_USB_IRP                *irp);

VOID NU_MSF_Command_IRP_Complete (
     NU_USB_PIPE               *pipe,
     NU_USB_IRP                *irp);
VOID NU_MSF_Process_Command (
     UNSIGNED                   pcb_ms_drvr,
     VOID                      *pcb_ms_device);

STATUS NU_MSF_Process_CBW (
       NU_USBF_MS_DEVICE       *device);

STATUS NU_MSF_Stall_Pipes (
       const NU_USBF_MS_DEVICE *device);

STATUS NU_MSF_Submit_Command_IRP (
       NU_USBF_MS_DEVICE       *device);

STATUS NU_MSF_Submit_Status_IRP (
       NU_USBF_MS_DEVICE       *device,
       UINT8                    status);

STATUS NU_MSF_Initialize_Device (
       NU_USBF_MS_DEVICE       *device);

STATUS NU_MSF_Process_Connect_event (
       NU_USBF_MS_DEVICE       *pcb_ms_device);

STATUS NU_MSF_Process_Data_And_Status (
       NU_USBF_MS_DEVICE       *pcb_ms_device);

STATUS NU_MSF_Release_Memory(
       NU_USBF_MS_DEVICE       *pcb_ms_device);

STATUS NU_MSF_Init_Task(
       NU_USBF_MS_DEVICE       *pcb_ms_device);

STATUS NU_MSF_Clear_Feature_Callback(
       NU_USBF_MS_DEVICE       *pcb_ms_device);

STATUS NU_MSF_Unstall_Pipes (
       NU_USBF_MS_DEVICE       *pcb_ms_device);

VOID MSF_RW_Complete(
        NU_USB_PIPE * pipe,
        NU_USB_IRP  *pcb_irp);

UINT8 NU_MSF_Is_Valid_CBW (
        NU_USB_IRP  *pcb_irp);

STATUS NU_MSF_Obtain_Semaphore (
        NU_SEMAPHORE *semaphore, 
        UNSIGNED suspend);

STATUS NU_MSF_Release_Semaphore (
        NU_SEMAPHORE *semaphore);


/* ===================================================================== */

#include "connectivity/nu_usbf_ms_dat.h"

/* ===================================================================== */

#ifdef  __cplusplus
}                                            /* End of C declarations. */
#endif

#endif                                       /* _NU_USBF_MS_IMP_H_  */

/* ======================  End Of File  ================================ */
