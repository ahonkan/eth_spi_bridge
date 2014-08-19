/*************************************************************************
*
*              Copyright 2011 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
* FILE NAME
*
*       nu_usbh_xhci_imp.h
*
* COMPONENT
*
*       USB XHCI Controller Driver : Nucleus USB Host Software.
*
* DESCRIPTION
*
*       This file contains control block and other structures for
*       NU_USBH_XHCI component(xHCI host controller).
*
* DATA STRUCTURES
*
*       NU_USBH_XHCI
*       NU_USBH_XHCI_EP_INFO
*       NU_USBH_XHCI_DEVICE
*       NU_USBH_XHCI_RAW_CTX
*       NU_USBH_XHCI_SLOT_CTX
*       NU_USBH_XHCI_EP_CTX
*       NU_USBH_XHCI_CTRL_CTX
*       NU_USBH_XHCI_OUT_CTX
*       NU_USBH_XHCI_IN_CTX
*       NU_USBH_XHCI_DEV_CTX_ARRAY
*       NU_USBH_XHCI_TD
*       NU_USBH_XHCI_TRB
*       NU_USBH_XHCI_SEGMENT
*       NU_USBH_XHCI_RING
*       NU_USBH_XHCI_CMD_RING
*       NU_USBH_XHCI_ERST
*       NU_USBH_XHCI_EVENT_RING;
*       NU_USBH_XHCI_SCRATCHPAD;
*       NU_USBH_XHCI_SP_ARRAY
*       NU_USBH_XHCI_STREAM_INFO
*       NU_USBH_XHCI_STREAM_CTX
*       NU_USBH_XHCI_INFO
*       NU_USBH_XHCI_IRP_INFO
*       XHCI_USBH_RH_STATUS
*       NU_USBH_XHCI_IRP
*       XHCI_SNAPSHOT_STATUS
*       XHCI_USB3_HUB
*       NU_USBH_XHCI_DISPATCH
*
* FUNCTIONS
*
*       None.
*
* DEPENDENCIES
*
*       nu_usb.h                            NU_USBH_HW definitions.
*       nu_usbh_xhci_cfg                    xHCI configuration file.
*       nu_usbh_xhci_regs.h                 xHCI Regs File.
*
**************************************************************************/
#ifndef NU_USBH_XHCI_IMP_H
#define NU_USBH_XHCI_IMP_H


/* ==============  USB Include Files =================================== */
#include "connectivity/nu_usb.h"
#include "nu_usbh_xhci_cfg.h"
#include "nu_usbh_xhci_regs.h"

/* ====================  Data definitions  ========================== */
typedef struct nu_usbh_xhci                 NU_USBH_XHCI;
typedef struct nu_usbh_xhci_ep_info         NU_USBH_XHCI_EP_INFO;
typedef struct nu_usbh_xhci_device          NU_USBH_XHCI_DEVICE;
typedef struct nu_usbh_xhci_raw_ctx         NU_USBH_XHCI_RAW_CTX;
typedef struct nu_usbh_xhci_slot_ctx        NU_USBH_XHCI_SLOT_CTX;
typedef struct nu_usbh_xhci_ep_ctx          NU_USBH_XHCI_EP_CTX;
typedef struct nu_usbh_xhci_ctrl_ctx        NU_USBH_XHCI_CTRL_CTX;
typedef struct nu_usbh_xhci_out_ctx         NU_USBH_XHCI_OUT_CTX;
typedef struct nu_usbh_xhci_in_ctx          NU_USBH_XHCI_IN_CTX;
typedef struct nu_usbh_xhci_dev_ctx_array   NU_USBH_XHCI_DEV_CTX_ARRAY;
typedef struct nu_usbh_xhci_td              NU_USBH_XHCI_TD;
typedef struct nu_usbh_xhci_trb             NU_USBH_XHCI_TRB;
typedef struct nu_usbh_xhci_segment         NU_USBH_XHCI_SEGMENT;
typedef struct nu_usbh_xhci_ring            NU_USBH_XHCI_RING;
typedef struct nu_usbh_xhci_cmd_ring        NU_USBH_XHCI_CMD_RING;
typedef struct nu_usbh_xhci_erst            NU_USBH_XHCI_ERST;
typedef struct nu_usbh_xhci_event_ring      NU_USBH_XHCI_EVENT_RING;
typedef struct nu_usbh_xhci_scratchpad      NU_USBH_XHCI_SCRATCHPAD;
typedef struct nu_usbh_xhci_sp_array        NU_USBH_XHCI_SP_ARRAY;
typedef struct nu_usbh_xhci_stream_info     NU_USBH_XHCI_STREAM_INFO;
typedef struct nu_usbh_xhci_stream_ctx      NU_USBH_XHCI_STREAM_CTX;
typedef struct nu_usbh_xhci_info            NU_USBH_XHCI_INFO;
typedef struct nu_usbh_xhci_irp_info        NU_USBH_XHCI_IRP_INFO;
typedef struct xhci_usbh_rh_status          XHCI_USBH_RH_STATUS;
typedef struct nu_usbh_xhci_irp             NU_USBH_XHCI_IRP;
typedef struct xhci_snapshot_status         XHCI_SNAPSHOT_STATUS;
typedef struct xhci_usb3_hub                XHCI_USB3_HUB;
typedef        NU_USBH_HW_DISPATCH          NU_USBH_XHCI_DISPATCH;

/* Number of registers per port */
#define  XHCI_NUM_PORT_REGS                 4

/* xHCI endpoint states encoding, section 6.2.3. */
#define XHCI_EP_STATE_DISABLED              0
#define XHCI_EP_STATE_RUNNING               1
#define XHCI_EP_STATE_HALTED                2
#define XHCI_EP_STATE_STOPPED               3
#define XHCI_EP_STATE_ERROR                 4

/* xHCI Slot states encoding, section 6.2.2. */
#define XHCI_SLOT_STATE_DISABLED            0
#define XHCI_SLOT_STATE_DEFALUT             1
#define XHCI_SLOT_STATE_ADDRESSED           2
#define XHCI_SLOT_STATE_CONFIG              3

/* Slot speeds encoding, section 6.2.2. */
#define XHCI_SLOT_SPEED_LOW                 0x02
#define XHCI_SLOT_SPEED_FULL                0x01
#define XHCI_SLOT_SPEED_HIGH                0x03
#define XHCI_SLOT_SPEED_SUPER               0x04

/* xHCI endpoint types encoding, section 6.2.3. */
#define XHCI_ISOC_OUT_EP                    1
#define XHCI_BULK_OUT_EP                    2
#define XHCI_INT_OUT_EP                     3
#define XHCI_CTRL_EP                        4
#define XHCI_ISOC_IN_EP                     5
#define XHCI_BULK_IN_EP                     6
#define XHCI_INT_IN_EP                      7
#define XHCI_EP_DIR_MASK                    0xF0
#define XHCI_EP_INDEX_MASK                  0x0F



/* TRB buffer pointers can't cross 64KB boundaries */
#define XHCI_TRB_MAX_BUFF_SHIFT             16
#define XHCI_SP_BUFF_SIZE                   (1 << 12)
#define XHCI_IN_CTX                         NU_TRUE

/* Size of TRB - 16 bytes. */
#define XHCI_TRB_SIZE                       16

#define XHCI_SEGMENT_SIZE                   (XHCI_TRBS_PER_SEGMENT * XHCI_TRB_SIZE)
/*
---------------------------------------------------------------------------
* TRB type IDs
* bulk, interrupt, isoc , and control data stage
---------------------------------------------------------------------------
*/
#define XHCI_TRB_NORMAL                     1

/* Setup stage for control transfers */
#define XHCI_TRB_SETUP                      2

/* Data stage for control transfers */
#define XHCI_TRB_DATA                       3

/* Status stage for controltransfers */
#define XHCI_TRB_STATUS                     4

/* Isoc transfers */
#define XHCI_TRB_ISOC                       5

/* TRB for linking ring segments */
#define XHCI_TRB_LINK                       6

#define XHCI_TRB_EVENT_DATA                 7

/* Transfer Ring No-op (not for the command ring) */
#define XHCI_TRB_TR_NOOP                    8

/* Command TRBs */
/* Enable Slot Command */
#define XHCI_TRB_ENABLE_SLOT                9

/* Disable Slot Command */
#define XHCI_TRB_DISABLE_SLOT               10

/* Address Device Command */
#define XHCI_TRB_ADDR_DEV                   11

/* Configure Endpoint Command */
#define XHCI_TRB_CONFIG_EP                  12

/* Evaluate Context Command */
#define XHCI_TRB_EVAL_CONTEXT               13

/* Reset Endpoint Command */
#define XHCI_TRB_RESET_EP                   14

/* Stop Transfer Ring Command */
#define XHCI_TRB_STOP_EP                    15

/* Set Transfer Ring Dequeue Pointer Command. */
#define XHCI_TRB_SET_DEQ                    16

/* Reset Device Command */
#define XHCI_TRB_RESET_DEV                  17

/* No-op Command - not for transfer rings. */
#define XHCI_TRB_CMD_NOOP                   23

#define XHCI_TRB_RSVD                       30

/* TRB IDs 24-31 reserved */
/* Event TRBS */
/* Transfer Event */
#define XHCI_TRB_TRANSFER                   32

/* Command Completion Event */
#define XHCI_TRB_COMPLETION                 33

/* Port Status Change Event */
#define XHCI_TRB_PORT_STATUS                34

/* Bandwidth Request Event (opt) */
#define XHCI_TRB_BANDWIDTH_EVENT            35

/* Doorbell Event (opt) */
#define XHCI_TRB_DOORBELL                   36

/* Host Controller Event */
#define XHCI_TRB_HC_EVENT                   37

/* Device Notification Event - device sent function wake notification */
#define XHCI_TRB_DEV_NOTE                   38

/* MFINDEX Wrap Event - microframe counter wrapped */
#define XHCI_TRB_MFINDEX_WRAP               39

/* TRB IDs 40-47 reserved, 48-63 is vendor-defined */

/*
---------------------------------------------------------------------------
                  TRB completion codes - section 6.4.5
---------------------------------------------------------------------------
*/

#define XHCI_TRB_COMPL_SUCCESS               1

/* Invalidity in some TRB field. */
#define XHCI_TRB_COMPL_INVALID_TRB_ERR       5

/* Stall Error */
#define XHCI_TRB_COMPL_STALL                 6

/* Resource Error*/
#define XHCI_TRB_COMPL_NO_MEM                7

/* Bandwidth Error */
#define XHCI_TRB_COMPL_BW_ERR                8

/* No Slots Available Error */
#define XHCI_TRB_COMPL_NOSTOTS_ERR           9

/* Doorbell is being rung for the device slot that is not available. */
#define XHCI_TRB_COMPL_EBADSLOT              11

/* Short Packet */
#define XHCI_TRB_COMPL_SHORT_TX              13

/* Parameter Error  */
#define XHCI_TRB_COMPL_PARAM_INVAID          17

/* Context State Error */
#define XHCI_TRB_COMPL_CTX_STATE             19

/*
---------------------------------------------------------------------------
                  Link States - section 5.4.8, Table 35
---------------------------------------------------------------------------
*/

#define XHCI_LINK_STATE_U0                  0   /* Representing link state
                                                   U0.                       */
#define XHCI_LINK_STATE_U1                  1   /* Representing link state
                                                 U1.                         */
#define XHCI_LINK_STATE_U2                  2   /* Representing link state
                                                 U2.                         */
#define XHCI_LINK_STATE_U3                  3   /* Representing link state
                                                 U3.                         */
#define XHCI_LINK_STATE_SSDISABLE           4   /* Representing link state
                                                 SS.Disable.                 */
#define XHCI_LINK_STATE_RXDETECT            5   /* Representing link state
                                                 RX.Detect.                  */
#define XHCI_LINK_STATE_SSINACTIVE          6   /* Representing link state
                                                 SS.Inactive.                */
#define XHCI_LINK_STATE_POLLING             7   /* Representing link state
                                                 Polling.                    */
#define XHCI_LINK_STATE_RECOVERY            8   /* Representing link state
                                                 Recovery.                   */
#define XHCI_LINK_STATE_HOT_RESET           9   /* Representing link is in
                                                 Hot Reset state.            */
#define XHCI_LINK_STATE_COMPLIANCE          10  /* Representing link is in
                                                 Compliance Mode state.      */
#define XHCI_LINK_STATE_LOOPBACK            11  /* Representing link state
                                                 loopback.                   */
#define XHCI_LINK_STATE_INVLD               12  /* Representing Invalid link
                                                    state.                   */
#define XHCI_LINK_STATE_RESUME              15  /* Representing xHCI reume
                                                   state.                    */

#define XHCI_DEVICE_OUT_CTX                 0

#define XHCI_DEVICE_IN_CTX                  1


#define XHCI_MAX_PEND_IRPS                  8

#define XHCI_CMD_RING_ALIGNMENT             64

#define XHCI_DCBAA_ALINMENT                 64

#define XHCI_TRANS_RING_ALIGNMENT           16

#define XHCI_ERST_ALIGNMENT                 16

/* xHCI error  codes.*/
#define XHCI_STATUS_BASE                    -4000

#define XHCI_ERR_RING_FULL                  (XHCI_STATUS_BASE - 1)

#define XHCI_ERR_INVLD_CTX                  (XHCI_STATUS_BASE - 2)

#define XHCI_ERR_FATAL                      (XHCI_STATUS_BASE - 3)

#define XHCI_ERR_CMD_FAILED                 (XHCI_STATUS_BASE - 4)

#define XHCI_ERR_PORT_DISABLED              (XHCI_STATUS_BASE - 5)

#define XHCI_SLOT_UNAVAILABLE               (XHCI_STATUS_BASE - 6)

#define XHCI_LIM_STREAM_ERR                 (XHCI_STATUS_BASE - 7)

#define XHCI_PEND_LIST_OVFLOW               (XHCI_STATUS_BASE - 8)

#define XHCI_INVLD_DCBAA                    (XHCI_STATUS_BASE - 9)

#define XHCI_INVLD_RING_ADDR                (XHCI_STATUS_BASE - 10)
/*
---------------------------------------------------------------------------
* struct nu_usbh_xhci_ep_info
* Nucleus USB xHC Endpoint Info Control Block
* Data structure used by the xHCI for maintaining transfers over the ednpoints.
---------------------------------------------------------------------------
*/

struct nu_usbh_xhci_ep_info
{
    NU_USBH_XHCI_RING         *ring;

    /* Related to endpoints that are configured to use stream IDs only. */
    NU_USBH_XHCI_STREAM_INFO  *stream_info;

    NU_USBH_XHCI_RING         *new_ring;

    UINT32                    ep_state;

    /* If streams are supported. */
    BOOLEAN                   has_streams;

    /* Padding .*/
    UINT8                     pad[3];
};

/*
---------------------------------------------------------------------------
* struct nu_usbh_xhci_stream_info - Nucleus USB xHC Stream Info Control Block
* Structure for keeping streams operation info
---------------------------------------------------------------------------
*/

struct nu_usbh_xhci_stream_info
{
    /* Rings used by the streams .*/
    NU_USBH_XHCI_RING          **stream_ring;

    NU_USBH_XHCI_STREAM_CTX    *stream_ctx_array;

    /* Total number of streams */
    UINT32                     num_streams;

    UINT32                     num_stream_ctxs;
};

/*
---------------------------------------------------------------------------
* struct nu_usbh_xhci_stream_ctx
* Stream Context Array data structure. Section 6.2.4 of xHCI specs
---------------------------------------------------------------------------
*/

struct nu_usbh_xhci_stream_ctx
{
    /* Ring address associated with the stream, 64 bit */
    UINT32 deq_ptr_lo;
    UINT32 deq_ptr_hi;

    /* Reserved fields. */
    UINT32  reserved[2];
};


/* Get stream context type */
#define XHCI_STREAM_CTX_TYPE(p)             (((p) << 1) & 0x7)


/* Stream array is of primary type with dequeue pointer to a transfer ring */
#define SCT_PRIMARY_TR                          1
/*
---------------------------------------------------------------------------
* struct nu_usbh_xhci_device
* Nucleus USB xHC Device Control Block
* Data structure used to maintain all the device related info like IN and
* OUT contexts, endpoint info and device address etc.
---------------------------------------------------------------------------
*/

struct nu_usbh_xhci_device
{
    /* Input context is used to pass commands to host controller.  */
    NU_USBH_XHCI_RAW_CTX       *dev_in_ctx;

    /* Host controller updates output context after commnads completion.  */
    NU_USBH_XHCI_RAW_CTX       *dev_out_ctx;

    /* Look up table for endpoint related data structures like xHCI
     * Ring and Stream Contexts.
     */
    NU_USBH_XHCI_EP_INFO       ep_info[31];

    /* Look up table for endpoint type. */
    UINT8                      ep_type[31];

    /* Device address assigned by the xHC after address assignment command. */
    UINT8                      device_addr;

    /* Device slot ID. */
    UINT8                      slot_id;

    /* Padding. */
    UINT8                      pad[2];
};


/*
---------------------------------------------------------------------------
struct nu_usbh_xhci_td - Transfer Descriptor Structure.
---------------------------------------------------------------------------
*/

struct nu_usbh_xhci_td
{
    CS_NODE node;

    /* Pointer to first TRB of TD. */
    NU_USBH_XHCI_TRB    *first_trb;

    /* Pointer to last TRB of TD. */
    NU_USBH_XHCI_TRB    *last_trb;

    /* Size of TD. */
    UINT32              num_trbs;

    /* Normalized buffer .*/
    UINT8               *normal_buffer;

    /* Raw buffer.*/
    UINT8               *raw_buffer;
};

/*
---------------------------------------------------------------------------
struct nu_usbh_xhci_irp - xHCI Specfic IRP Structure.
---------------------------------------------------------------------------
*/

struct nu_usbh_xhci_irp
{
    CS_NODE                 node;

    /* IRP pointer to IRP submitted by the stack.*/
    NU_USB_IRP              *irp;

    /* TD for current data transfer. */
    struct nu_usbh_xhci_td  td;

    /* Pointer to  buffer containing data. */
    UINT8                   *buffer;

    UINT32                  tx_length;

    UINT32                  rem_length;

    /* Total TRBS required to transfer data. */
    UINT32                  trbs_req;

    /* TRBS left to be transferred. */
    UINT32                  rem_trbs;

    /* Total length transferred. */
    UINT32                  total_length;

    /* Endpoint maximum packet size. */
    UINT16                  max_p;

    /* Direction of data transfer IN/OUT. */
    BOOLEAN                 direction;

    BOOLEAN                 empty_packet;

};

/*
---------------------------------------------------------------------------
* struct nu_usbh_xhci_ring - Nucleus USB xHCD Ring Data Structure.
* This structure is manitained by the Host Controller Driver for keeping
  information relating to TRB Ring.
---------------------------------------------------------------------------
*/

struct nu_usbh_xhci_ring
{
    NU_USBH_XHCI_TRB        *enqueue_ptr;

    NU_USBH_XHCI_TRB        *dequeue_ptr;

    /* Pointer to first segment of TRB ring.*/
    NU_USBH_XHCI_SEGMENT    *first_segment;

    /* Pointer to last segment of TRB ring.*/
    NU_USBH_XHCI_SEGMENT    *last_segment;

    /* Pointer to the segment where the TRB enqueue pointer is present.This
     * is required when we need to grow or shrink the ring.
     */
    NU_USBH_XHCI_SEGMENT    *enq_segment;

    /* Pointer to the segment where the TRB dequeue pointer is present.This
     * is required when we need to grow or shrink the ring.
     */
    NU_USBH_XHCI_SEGMENT    *deq_segment;

    /* xHCI specfic IRP structure. */
    NU_USBH_XHCI_IRP         xhci_irp;

    /* Write the cycle state into the TRB cycle field to give ownership of
     * the TRB to the host controller (if we are the producer), or to check
     * if we own the TRB (if we are the consumer).  See section 4.9.1.
     */
    UINT32                  cycle_state;

    /* The associated stream ID with the ring. */
    UINT32                  stream_id;

    UINT32                  total_trbs;
  /* Size of ring .*/
    UINT32                  available_trbs;

    /* Number of segments in the ring .*/
    UINT32                  num_segments;

};

/*
---------------------------------------------------------------------------
* struct nu_usbh_xhci_cmd_ring - Nucleus USB xHCD CMD Ring Data Structure.
* This structure is manitained by the Host Controller Driver for keeping
  information relating to command Ring.
---------------------------------------------------------------------------
*/

struct nu_usbh_xhci_cmd_ring
{
    /* Pointer to command ring .*/
    NU_USBH_XHCI_RING *cmd_ring;

    /* Command completion status code. */
    UINT32            cmd_status;

    /* Semaphore to flag command completion. */
    NU_SEMAPHORE      cmd_comp_lock;

};

/*
---------------------------------------------------------------------------
* struct nu_usbh_xhci_segment - Ring Segment Structure.
* This structure is manitained by the controller driver for queueing new
  segments in the ring.
---------------------------------------------------------------------------
*/

struct nu_usbh_xhci_segment
{
    /* Pointer to first TRB of segment. */
    NU_USBH_XHCI_TRB     *trbs;

    /* Pointer to next segment of ring. */
    NU_USBH_XHCI_SEGMENT *next;

};


/*
---------------------------------------------------------------------------
* struct nu_usbh_xhci_slot_ctx
* xHC Device Slot Context Data Structure
---------------------------------------------------------------------------
*/

struct nu_usbh_xhci_slot_ctx
{
    /* Contains route string,device speed, hub info and last valid ep info.*/
    UINT32  device_info1;

    /* device latency and root hub info. */
    UINT32  device_info2;

    /* split transfer info.
     */
    UINT32  tt_intr_info;

    /* Slot state and device address. */
    UINT32  device_state;

    /*  Reserved fields */
    UINT32  reserved[4];

};

/*
---------------------------------------------------------------------------
                        device_info1 - bitmasks
---------------------------------------------------------------------------
*/

#define SLOT_CTX_ROUTE_STR_MASK             (0xFFFFF)
#define SLOT_CTX_DEV_SPEED(p)               (((p) >> 20)& 0x0F)
#define SLOT_CTX_DEV_MTT                    (0x1 << 25)
#define SLOT_CTX_DEV_HUB                    (0x1 << 26)
#define SLOT_CTX_LAST_CTX_MASK              ((UINT32)0x1F << 27)
#define SLOT_CTX_LAST_CTX(p)                ((p) << 27)
#define SLOT_CTX_LAST_CTX_TO_EP_NUM(p)      (((p) >> 27) - 1)
#define SLOT_CTX_SLOT_FLAG                  (1 << 0)
#define SLOT_CTX_EP0_FLAG                   (1 << 1)

/*
---------------------------------------------------------------------------
                           device_info2 - bitmasks
---------------------------------------------------------------------------
*/

#define SLOT_CTX_MAX_EXIT_LAT               (0xFFFF)
#define SLOT_CTX_ROOT_HUB_PORT(p)           (((p) & 0xFF) << 16)
#define SLOT_CTX_XHCI_MAX_PORTS(p)          (((p) & 0xFF) << 24)

/*
---------------------------------------------------------------------------
                           tt_intr_info - bitmasks
---------------------------------------------------------------------------
*/

#define SLOT_CTX_TT_SLOT                    (0xFF)

#define SLOT_CTX_TT_PORT(p)                 (((p)& 0xFF) << 8)
#define SLOT_CTX_INTR_TGT(p)                (((p) & 0x3FF) << 22)
#define SLOT_CTX_TT(p)                      (((p) & 0x3) << 16)

/*
---------------------------------------------------------------------------
                           device_state - bitmasks
---------------------------------------------------------------------------
*/

#define SLOT_CTX_DEV_ADDR_MASK              (0xFF)
#define SLOT_CTX_SLOT_STATE                 (0x1F << 27)
#define SLOT_CTX_GET_SLOT_STATE(p)          (((p) >> 27) & 0x1F)

/*
---------------------------------------------------------------------------
* struct nu_usbh_xhci_ep_ctx
* xHC Device Endpoint Context Data Structure
---------------------------------------------------------------------------
*/
struct nu_usbh_xhci_ep_ctx
{
    /* Contains ep state and streams related info */
    UINT32  ep_info1;

    /* Contains endpoint paramters maxp, burst etc     */
    UINT32  ep_info2;

    UINT32  deq_ptr_lo;
    UINT32  deq_ptr_hi;

    UINT32  ep_tx_info;

    /* Reserved fields for xHCI internal use */
    UINT32  reserved[3];
};

/*
---------------------------------------------------------------------------
                           ep_info1 - bitmasks
---------------------------------------------------------------------------
*/

#define EP_CTX_STATE_MASK                   (0x7)
#define EP_CTX_SET_STATE(p)                 ((p)& EP_CTX_STATE_MASK )
#define EP_CTX_MULT_MASK                    (0x3 << 8)
#define EP_CTX_MAX_PSTREAM_MASK             (0x1F << 10)
#define EP_CTX_SET_MAX_PSTREAM(p)           (((p) & 0x1F) << 10)
#define EP_CTX_HAS_LSA_MASK                 (1 << 15)

#define EP_CTX_INTERVAL_MASK                (0xFF << 16)

#define EP_CTX_SET_INTERVAL(p)              (((p) & 0xFF) << 16)
#define EP_CTX_SET_MULT(p)                  (((p) & 0x03) << 8)
/*
---------------------------------------------------------------------------
                           ep_info2 - bitmasks
---------------------------------------------------------------------------
*/

#define EP_CTX_FORCE_EVENT                  (0x01)
#define EP_CTX_ERROR_COUNT(p)               (((p) & 0x3) << 1)
#define EP_CTX_EP_TYPE(p)                   (((p) >> 3) & 0x3)
#define EP_CTX_EP_TYPE_SET(p)               (((p) & 0x7 ) << 3)
#define EP_CTX_SET_MAX_BURST(p)             (((p)& 0xFF) << 8)
#define EP_CTX_SET_MAX_PACKET(p)            (((p)& 0xFFFF) << 16)
#define EP_CTX_MAX_PACKET_MASK              ((UINT32)0xFFFF << 16)
#define EP_CTX_MAX_PACKET_SIZE(p)           (((p) >> 16) & 0xFFFF)

/*
---------------------------------------------------------------------------
* struct nu_usbh_xhci_input_ctrl_ctx 
* Input control context, Section 6.2.5.
---------------------------------------------------------------------------
*/

struct nu_usbh_xhci_ctrl_ctx
{
    /* Set the bit of the endpoint context which is to be disabled. */
    UINT32  drop_flags;

    /* Set the bit of the endpoint context which is to be disabled. */
    UINT32  add_flags;
    UINT32  rsvd[6];
};

#define XHCI_CTRL_CTX_FLAG(p)               ( 1 << ((p) + 1) )

/*
---------------------------------------------------------------------------
* struct nu_usbh_xhci_out_ctx
* Output Context Data Structure.
* xHCI updates out context" after command completion.
---------------------------------------------------------------------------
*/

struct nu_usbh_xhci_out_ctx
{
    NU_USBH_XHCI_SLOT_CTX *slot_ctx;
    NU_USBH_XHCI_EP_CTX   *ep_ctx;
};

/*
---------------------------------------------------------------------------
* struct nu_usbh_xhci_in_ctx
* Input Context Data Structure.
* 
---------------------------------------------------------------------------
*/

struct nu_usbh_xhci_in_ctx
{
    /* Input context comprises of control context, slot context
     * and ep context.
     */
    NU_USBH_XHCI_CTRL_CTX       *ctrl_ctx;
    NU_USBH_XHCI_SLOT_CTX       *slot_ctx;
    NU_USBH_XHCI_EP_CTX         *ep_ctx;

};

/*
---------------------------------------------------------------------------
* struct nu_usbh_xhci_raw_ctx - Raw Context Data Structure.
* This structure serves as buffer for device IN and OUT contexts.
---------------------------------------------------------------------------
*/

struct nu_usbh_xhci_raw_ctx
{
    UINT8                 *ctx_buffer;
    NU_USBH_XHCI_CTRL_CTX *ctrl_ctx;
    NU_USBH_XHCI_SLOT_CTX *slot_ctx;
    NU_USBH_XHCI_EP_CTX   *ep_ctx[31];
    UINT32                ctx_size;
    UINT32                ctx_type;
};

/*
---------------------------------------------------------------------------
struct nu_usbh_xhci_dev_ctx_array - Device Context Array.
---------------------------------------------------------------------------
*/

struct nu_usbh_xhci_dev_ctx_array
{
    /* The maximum size of Device Context Array is 256,64-bit enteries or
     * 2 Kbytes.
     */
    UINT32  dev_ctx_ptrs_lo;
    UINT32  dev_ctx_ptrs_hi;
};

/*
---------------------------------------------------------------------------
* struct nu_usbh_xhci_trb - Transfer Request Block Structure.
* Section 6.3
---------------------------------------------------------------------------
*/

struct nu_usbh_xhci_trb
{
    /* The format/contents of parameter component field would be defined by
     * the status field.
     */
    UINT32 parameter[2];

    /* The status field contains data buffer length, interrupt target and
     * some rsvd bits.
     */
    UINT32 status;

    /* The control field contins TRB type mainly and other flags/feilds
     * depending upon TRB type field,
     */
    UINT32 control;
};

/*
---------------------------------------------------------------------------
             Normal TRB status - Bit masks
---------------------------------------------------------------------------
*/

#define SET_NORM_TRB_LEN(p)                 ((p) & 0x1FFFF)
#define GET_NORM_TRB_LEN(p)                 ((p) & 0x1FFFF)
#define SET_NORM_TRB_TD_SIZE(p)             (((p) & 0x1F) << 17)
#define SET_NORM_TRB_INTR_TARGET(p)         (((p) & 0x3FF) << 22)

/*
---------------------------------------------------------------------------
             Normal TRB control - Bit masks
---------------------------------------------------------------------------
*/

#define SET_NORM_TRB_CYCLE                  (1 << 0)
#define TRB_CYCLE_BIT_MASK                  (1 << 0)
#define SET_NORM_TRB_ENT                    (1 << 1)
#define SET_NORM_TRB_ISP                    (1 << 2)
#define SET_NORM_TRB_NO_SNOOP               (1 << 3)
#define SET_NORM_TRB_CHAIN                  (1 << 4)
#define SET_NORM_TRB_IOC                    (1 << 5)
#define SET_NORM_TRB_IDT                    (1 << 6)
#define SET_TRB_DIR_IN                      (1 << 16)


/*
---------------------------------------------------------------------------
             Transfer Event TRB status - Bit masks
---------------------------------------------------------------------------
*/

#define  GET_EVENT_TRB_LENGTH(p)            ((p) & 0xFFFFFF)

#define  GET_EVENT_TRB_COMP_CODE(p)         (((p) >> 24) & 0xFF)

/*
---------------------------------------------------------------------------
             Transfer Event TRB control - Bit masks
---------------------------------------------------------------------------
*/
#define GET_EVENT_TRB_CYCLE                 (1 << 0)
#define GET_EVENT_TRB_ED                    (1 << 2)
#define GET_EVENT_TRB_EP_ID(p)              (((p) >> 16) & 0x1F)
#define GET_EVENT_TRB_SLOT_ID(p)            (((p) >> 24) & 0xFF)
#define GET_PORT_ID(p)                      (((p) >> 24) & 0xFF)

/*
---------------------------------------------------------------------------
             Command Completion Event TRB status - Bit masks
---------------------------------------------------------------------------
*/

#define  GET_CMD_TRB_COMP_CODE(p)           (((p) >> 24) & 0xFF)

/*
---------------------------------------------------------------------------
             Command Completion Event TRB control - Bit masks
---------------------------------------------------------------------------
*/

#define GET_EVENT_TRB_VF_ID(p)              (((p) >> 16) & 0x1F)
#define GET_CMD_TRB_SLOT_ID(p)              (((p) >> 24) & 0xFF)
#define SET_TRB_TYPE(p)                     (((p) & 0x3F) << 10)
#define SET_TRB_SLOT_ID(p)                  (((p) & 0xFF) << 24)
#define GET_TRB_TYPE(p)                     (((p) >> 10) & 0x3F)
#define TRB_LINK_TOGGLE                     (0x1 << 1)
#define SET_TRB_EP_ID(p)                    ((((p) + 1) & 0x1F) << 16)
#define SET_STRM_ID_FOR_TRB(p)              (((p) & 0xFFFF) << 16)
#define SET_TRB_SP_FLAG                     ( 1 << 23 )
/*
---------------------------------------------------------------------------
* struct nu_usbh_xhci_erst
* Event Ring Segment Table Structure, Section 6.5
---------------------------------------------------------------------------
*/

struct nu_usbh_xhci_erst
{
    /* Event ring segment address - 64 bit */
    UINT32  seg_addr_lo;
    UINT32  seg_addr_hi;

    /* Event ring segment size. */
    UINT32  seg_size;

    /* reserved */
    UINT32  rsvd;
};

/*
---------------------------------------------------------------------------
* struct nu_usbh_xhci_event_ring - Wrapper Strcture for handling multiple
  ERST event ring segment tables.
---------------------------------------------------------------------------
*/

struct nu_usbh_xhci_event_ring
{
    NU_USBH_XHCI_ERST *entries;
    NU_USBH_XHCI_RING *event_ring;

    /* Number of event ring segments.*/
    UINT32            num_entries;

    /* Num entries the ERST can contain */
    UINT32            erst_size;
};

/*
---------------------------------------------------------------------------
* struct nu_usbh_xhci_scratchpad - Wrapper data strcture for maintaining
  scratchpad array and buffers.
---------------------------------------------------------------------------
*/

struct nu_usbh_xhci_scratchpad
{
    NU_USBH_XHCI_SP_ARRAY *sp_array;
    VOID                  *sp_buffer;
};

/*
---------------------------------------------------------------------------
* struct nu_usbh_xhci_sp_array - Scratchpad Array Entry
* Section 6.6.
---------------------------------------------------------------------------
*/

struct nu_usbh_xhci_sp_array
{
    UINT32 sp_buffer_lo;
    UINT32 sp_buffer_hi;
};

#define USBH_XHCI_RH_LSB(wValue)            (UINT8)(wValue & 0xFF)
#define USBH_XHCI_RH_MSB(wValue)            (UINT8)((wValue >> 8) & 0xFF)
#define USBH_XHCI_RH_RECIPIENT_PORT         3
#define USBH_XHCI_RH_GET_RECIPIENT(p)       (p & 0x0F)
#define USBH_XHCI_RH_LSB(wValue)            (UINT8)(wValue & 0xFF)
#define USBH_XHCI_RH_MSB(wValue)            (UINT8)((wValue >> 8) & 0xFF)
#define USBH_XHCI_RH_GET_RECIPIENT(p)       (p & 0x0F)

/*
---------------------------------------------------------------------------
* struct xhci_snapshot_status -
*
---------------------------------------------------------------------------
*/

struct xhci_snapshot_status
{
    UINT32  hub_status;
    UINT32  port_status[XHCI_MAX_PORTS];
};

/*
---------------------------------------------------------------------------
* struct xhci_usbh_rh_status -
*
---------------------------------------------------------------------------
*/

struct xhci_usbh_rh_status
{
    UINT32                       status_map;
    struct xhci_snapshot_status  previous;
    UINT32                       port_array[15];
    NU_USB_IRP                   *irp;
};

/*
---------------------------------------------------------------------------
* struct xhci_usb3_hub - xHCI USB3 hub control block.
* This structure mimics USB3 hub.
*
---------------------------------------------------------------------------
*/
struct xhci_usb3_hub
{
    XHCI_USBH_RH_STATUS         hs_rh_status;
    XHCI_USBH_RH_STATUS         ss_rh_status;
    UINT8                       ss_rh_hub_desc[20];
    UINT8                       hs_rh_hub_desc[20];
    UINT8                       port_array[XHCI_MAX_PORTS];
};

/*
---------------------------------------------------------------------------
* struct nu_usbh_xhci_info - Nucleus USB xHCD Info Control Block
* This data structres holds some useful xHC parameters.
---------------------------------------------------------------------------
*/

struct nu_usbh_xhci_info
{
  UINT32  page_size;
  UINT16  xhci_specs_ver;
  UINT16  max_interrupters;
  UINT8   max_slots;
  UINT8   num_ports;
  UINT8   pad[2];
};

/*
---------------------------------------------------------------------------
* struct nu_usbh_xhci - Nucleus USB xHC Driver Control Block
* This data structres mimics the eXtensible Host Controller(xHC) and
  serves as access point to various data structres and Host Controller
  register.
---------------------------------------------------------------------------
*/
struct nu_usbh_xhci
{
    /* Nucleus USB host HW control block instance .*/
    NU_USBH_HW                  cb;

    /* Copy of the Device Context Base Address Array data structure. */
    NU_USBH_XHCI_DEVICE         *device[XHCI_MAX_SLOTS];

    /* Pointer to device context base address array that is to be saved
     * in the DCBBAP register.
     */
    NU_USBH_XHCI_DEV_CTX_ARRAY  *dcbaa;

    /* Pointers to xHCI Command and Event Ring .*/
    NU_USBH_XHCI_CMD_RING       cmd_ring_cb;
    NU_USBH_XHCI_EVENT_RING     event_ring_cb;

    /* Access point to xHCI register base. */
    NU_USBH_XHCI_REG_BASE       reg_base;

    /* Scratchpad buffer for internal use of xHC */
    NU_USBH_XHCI_SCRATCHPAD     *scratchpad_cb;

    /* Memory pools used by the driver. */
    NU_MEMORY_POOL              *cacheable_pool;
    NU_MEMORY_POOL              *uncachable_pool;

    NU_SEMAPHORE                protect_sem;

    /* General info about the hardware controller. */
    NU_USBH_XHCI_INFO           info_hub;

    /* xHCI root hub control block. */
    XHCI_USB3_HUB               usb3_hub;

    /* LUT for retreveing slot id against fucntion address. */
    UINT32                      device_table[XHCI_MAX_SLOTS];

    /* State of controller halted,reset running.*/
    UINT32                      state;

    /* Variable to temporarily hold slot_id. */
    UINT8                       slot_id;

    /* xHCI interrupt status register replica. */
    UINT32                      int_sts_reg;

    /* Number of times interrupt is disabled. */
    UINT8                       int_disable_count;

    /* Current available in host controller. */
    UINT32                      available_current;

    /* Port power control.*/
    BOOLEAN                     port_power_control;

    /* Session handle associated with host controller driver.*/
    VOID                        *ses_handle;
};


#define XHCI_CPU_2_BUS(cb, a, b)    b=a
#define XHCI_BUS_2_CPU(cb, a, b)    b=a

#define USB_HWX_READ32(cb, p, d)     d = HOST_2_LE32(ESAL_GE_MEM_READ32(p))
#define USB_HWX_WRITE32(cb, p, d)    ESAL_GE_MEM_WRITE32(p, LE32_2_HOST(d))

#define XHCI_HW_READ32(cb,addr,data)        USB_HWX_READ32(cb, addr, data)

#define XHCI_HW_WRITE32(cb,addr,data)       USB_HWX_WRITE32(cb, addr, data)

#define XHCI_READ_ADDRESS(cb, p, d) \
                                    { \
                                        UINT32 t1; \
                                        XHCI_HW_READ32(cb, (p), t1); \
                                        if (t1) \
                                        { \
                                            XHCI_BUS_2_CPU(cb, t1, (d)); \
                                        } \
                                        else \
                                        { \
                                            (d) = NU_NULL; \
                                        } \
                                    }

#define XHCI_WRITE_ADDRESS(cb, p, d) \
                                       { \
                                            UINT32 t1; \
                                            if (d) \
                                            { \
                                                XHCI_CPU_2_BUS(cb, d, t1);\
                                            }\
                                            else \
                                            { \
                                                t1 = NU_NULL;\
                                            }\
                                           XHCI_HW_WRITE32(cb, (p), t1); \
                                        }

#define XHCI_FILL_TRB(ptr,para1,para2,sts,ctrl) \
                     ((ptr)->parameter[0]) = (HOST_2_LE32((para1))); \
                     ((ptr)->parameter[1]) = (HOST_2_LE32((para2))); \
                     ((ptr)->status)       = (HOST_2_LE32((sts))); \
                     ((ptr)->control)      = (HOST_2_LE32((ctrl)))

#define XHCI_CONVERT_ENDIANESS_SLOT_CTX(slot_ctx) \
                    ((slot_ctx)->device_info1) = (HOST_2_LE32(slot_ctx->device_info1)); \
                    ((slot_ctx)->device_info2) = (HOST_2_LE32(slot_ctx->device_info2)); \
                    ((slot_ctx)->tt_intr_info) = (HOST_2_LE32(slot_ctx->tt_intr_info)); \
                    ((slot_ctx)->device_state) = (HOST_2_LE32(slot_ctx->device_state))

#define XHCI_CONVERT_ENDIANESS_CTRL_CTX(ctrl_ctx) \
                    ((ctrl_ctx)->add_flags)  = (HOST_2_LE32((ctrl_ctx)->add_flags)); \
                    ((ctrl_ctx)->drop_flags) = (HOST_2_LE32((ctrl_ctx)->drop_flags))

#define XHCI_CONVERT_ENDIANESS_EP_CTX(ep_ctx) \
                    ((ep_ctx)->ep_info1)   = (HOST_2_LE32((ep_ctx)->ep_info1)); \
                    ((ep_ctx)->ep_info2)   = (HOST_2_LE32((ep_ctx)->ep_info2)); \
                    ((ep_ctx)->ep_tx_info) = (HOST_2_LE32((ep_ctx)->ep_tx_info))

#define USBH_XHCI_PROTECT                   XHCI_Disable_Interrupts(xhci);
#define USBH_XHCI_UNPROTECT                 XHCI_Enable_Interrupts(xhci);

#define XHCI_SP_ARRAY_ALIGNMENT             64
#define XHCI_MAX_TRB_SIZE                   (1 << 12)
#define XHCI_EP_MASK                        0x3
#define XHCI_TRB_MAX_TD_SIZE                32768
#define XHCI_TRB_TD_SIZE_FIELD              31
#define XHCI_PORT_REV_SS                    0x03
#define XHCI_EXC_CAP_OFFSET                 0x08
#define XHCI_SPECS_0_95                     0x0095

#define XHCI_PORT_RO                        ((1 << 0)    \
                                            |(1 << 3)    \
                                            |(0xf << 10) \
                                            |(1 << 30))

#define XHCI_PORT_RWS                       ((0xf << 5)  \
                                            |(1 << 9)    \
                                            |(0x3 << 14) \
                                            |(0x7 << 25))

#define XHCI_EP_ADDR_TO_EP_INDEX(p) \
                                            ((p) & XHCI_EP_DIR_MASK) ? \
                                            ((p) & XHCI_EP_INDEX_MASK)*2: \
                                            ((p) & XHCI_EP_INDEX_MASK)*2 - 1

/* xHCI state table. */
enum XHCI_STATE {
    XHCI_STATE_RESET ,
    XHCI_STATE_RUNNING,
    XHCI_STATE_STOPPED,
};

/*
--------------------------------------------------------------------------
 Functions Prototype
--------------------------------------------------------------------------
*/

STATUS   XHCI_Allocate_TRB_Ring                     (NU_USBH_XHCI                  *cb,
                                                     NU_USBH_XHCI_RING             **ring,
                                                     UINT8                         num_segments,
                                                     UINT32                        alignment,
                                                     BOOLEAN                       link_trb);

STATUS   XHCI_Allocate_TRB_Segment                  (NU_USBH_XHCI                  *cb,
                                                     NU_USBH_XHCI_SEGMENT          **segment,
                                                     UINT32                        alignment);

STATUS   XHCI_Deallocate_TRB_Segment                (NU_USBH_XHCI_SEGMENT          *segment);

STATUS   XHCI_Link_TRB_Segments                     (NU_USBH_XHCI                  *cb,
                                                     NU_USBH_XHCI_SEGMENT          *curr_segment,
                                                     NU_USBH_XHCI_SEGMENT          *new_segment,
                                                     BOOLEAN                       link_trb);

STATUS   XHCI_Deallocate_TRB_Ring                   (NU_USBH_XHCI_RING             *ring);

STATUS   XHCI_Room_On_Ring                          (NU_USBH_XHCI                  *cb,
                                                     NU_USBH_XHCI_RING             *ring,
                                                     UINT32                        req_trbs,
                                                     UINT32                        alignment);

STATUS   XHCI_Reinitialize_Ring                     (NU_USBH_XHCI                  *xhci,
                                                     NU_USBH_XHCI_RING             *ring,
                                                     UINT8                         ep_index,
                                                     UINT8                         slot_id);

STATUS   XHCI_Initialize_HW_Controller              (NU_USB_HW                     *cb);

STATUS   XHCI_Uninitialize_HW_Controller            (NU_USBH_XHCI                  *xhci);

STATUS   XHCI_Queue_TRB_Segments                    (NU_USBH_XHCI                  *cb,
                                                     NU_USBH_XHCI_RING             *ring,
                                                     UINT8                         num_segments,
                                                     BOOLEAN                       link_trb,
                                                     UINT32                        alignment);

STATUS   XHCI_Allocate_Scratchpad_Memory            (NU_USBH_XHCI                  *cb);

STATUS   xhci_deallocate_scratchpad_memory          (NU_USBH_XHCI                  *cb);

STATUS   XHCI_Allocate_Device_Context               (NU_USBH_XHCI                  *cb,
                                                     NU_USBH_XHCI_RAW_CTX          **ctx,
                                                     BOOLEAN                       ctx_type);

STATUS   XHCI_Inc_Ring_Enqueue_Ptr                  (NU_USBH_XHCI_RING              *ring,
                                                     BOOLEAN                        trb_producer);

STATUS   XHCI_Inc_Ring_Dequeue_Ptr                  (NU_USBH_XHCI_RING              *ring,
                                                     BOOLEAN                        trb_consumer);

__inline STATUS   XHCI_Get_Input_Control_Context    (NU_USBH_XHCI_RAW_CTX           *ctx,
                                                     NU_USBH_XHCI_CTRL_CTX          **out_ctx);

__inline STATUS   XHCI_Get_Slot_Context             (NU_USBH_XHCI_RAW_CTX           *ctx,
                                                     NU_USBH_XHCI_SLOT_CTX          **out_ctx);

__inline STATUS   XHCI_Get_Endpoint_Context         (NU_USBH_XHCI_RAW_CTX           *ctx,
                                                     NU_USBH_XHCI_EP_CTX            **out_ctx,
                                                     UINT8                          ep_index);

__inline STATUS   XHCI_Get_Capability_Regs_Handle   (NU_USBH_XHCI_REG_BASE          *reg_base,
                                                     NU_USBH_XHCI_CAP_REGS          **cap_reg);

__inline STATUS   XHCI_Get_Operational_Regs_Handle  (NU_USBH_XHCI_REG_BASE         *reg_base,
                                                     NU_USBH_XHCI_OP_REGS          **op_reg);

__inline STATUS   XHCI_Get_Doorbell_Regs_Handle     (NU_USBH_XHCI_REG_BASE         *reg_base,
                                                     NU_USBH_XHCI_DB_ARRAY         **db_reg);

__inline STATUS   XHCI_Get_Run_Time_Regs_Handle     (NU_USBH_XHCI_REG_BASE         *reg_base,
                                                     NU_USBH_XHCI_RUN_REGS         **rt_reg);

__inline STATUS   XHCI_Get_Stream_Ring_handle       (NU_USBH_XHCI_STREAM_INFO      *strm_info,
                                                     UINT16                        stream_id,
                                                     NU_USBH_XHCI_RING             **ring );

__inline STATUS    XHCI_Get_Endpoint_Handle         (NU_USBH_XHCI_DEVICE           *xhci_device,
                                                     UINT8                         ep_index,
                                                     NU_USBH_XHCI_EP_INFO          **ep_info);

__inline STATUS    XHCI_Get_Device_Handle           (NU_USBH_XHCI                  *xhci,
                                                     UINT8                         slot_id,
                                                     NU_USBH_XHCI_DEVICE           **xhci_device);

__inline STATUS   XHCI_Ring_Endpoint_Doorbell       (NU_USBH_XHCI                  *cb,
                                                     UINT8                         slot_id,
                                                     UINT16                        stream_id,
                                                     UINT8                         ep_index);

__inline STATUS XHCI_Init_IRP                       (NU_USBH_XHCI_IRP              *xhci_irp,
                                                     NU_USB_IRP                    *irp,
                                                     NU_USBH_XHCI_TD               **td,
                                                     UINT16                        ep_max_pack,
                                                     UINT32                        trbs_req,
                                                     BOOLEAN                       direction,
                                                     BOOLEAN                       empty_packet );

__inline STATUS  XHCI_Get_Endpoint_Ring_Handle      (NU_USBH_XHCI_EP_INFO          *ep_info,
                                                     NU_USBH_XHCI_RING             **ep_ring);

__inline STATUS XHCI_Get_Command_Ring_Handle        (NU_USBH_XHCI_CMD_RING         *cb,
                                                     NU_USBH_XHCI_RING             **ring);

STATUS   XHCI_Initiate_Bulk_Transfer                (NU_USBH_XHCI                  *cb,
                                                     NU_USB_IRP                    *irp,
                                                     NU_USBH_XHCI_DEVICE           *xhci_dev,
                                                     UINT8                         ep_index,
                                                     BOOLEAN                       direction);

STATUS   XHCI_Handle_Bulk_TD                        (NU_USBH_XHCI                  *xhci,
                                                     NU_USBH_XHCI_RING             *ring,
                                                     NU_USBH_XHCI_IRP              *xhci_irp,
                                                     UINT16                        stream_id,
                                                     UINT16                        max_packet);

STATUS   XHCI_Initiate_Control_Transfer             (NU_USBH_XHCI                  *cb,
                                                     NU_USB_IRP                    *irp,
                                                     NU_USBH_XHCI_DEVICE           *xhci_dev,
                                                     UINT8                         ep_index);

STATUS   XHCI_Make_TD                               (NU_USBH_XHCI                  *xhci_cb,
                                                     NU_USBH_XHCI_RING             *ring,
                                                     NU_USBH_XHCI_TD               **td,
                                                     UINT32                        trbs_req);

STATUS   XHCI_Handle_Rx_Event                       (NU_USBH_XHCI                  *cb ,
                                                     NU_USBH_XHCI_TRB              *event_trb);

STATUS   XHCI_Complete_Bulk_Transfer                (NU_USBH_XHCI                  *xhci,
                                                     NU_USBH_XHCI_EP_INFO          *ep_info,
                                                     NU_USBH_XHCI_TRB              *event_trb,
                                                     NU_USBH_XHCI_IRP              *curr_irp);

STATUS XHCI_Process_Bulk_IRP                        (NU_USBH_XHCI                  *xhci,
                                                     NU_USB_IRP                    *irp,
                                                     NU_USBH_XHCI_RING             *ring,
                                                     UINT16                        stream_id,
                                                     UINT16                        ep_max_pack,
                                                     UINT32                        trbs_req,
                                                     BOOLEAN                       direction,
                                                     BOOLEAN                       empty_packet);

STATUS XHCI_Process_Remaining_IRP                   (NU_USBH_XHCI                  *xhci,
                                                     NU_USBH_XHCI_RING             *ring,
                                                     NU_USBH_XHCI_IRP              *xhci_irp,
                                                     STATUS                        irp_status,
                                                     UINT32                        data_length,
                                                     UINT8                         ep_index,
                                                     UINT16                        strm_id,
                                                     UINT8                         slot_id);

STATUS   XHCI_Handle_Stream_Completion              (NU_USBH_XHCI                  *xhci,
                                                     NU_USBH_XHCI_EP_INFO          *ep_info,
                                                     NU_USBH_XHCI_TRB              *event_trb,
                                                     NU_USBH_XHCI_RING             *ring);

STATUS   XHCI_Complete_Control_Transfer             (NU_USBH_XHCI                  *cb,
                                                     NU_USBH_XHCI_EP_INFO          *ep_info,
                                                     NU_USBH_XHCI_TRB              *event_trb,
                                                     NU_USBH_XHCI_IRP              *xhci_irp);

STATUS   XHCI_Handle_IRP_Completion                 (NU_USB_IRP                    *irp,
                                                     STATUS                        irp_status,
                                                     UINT32                        data_length);

STATUS   XHCI_Allocate_Streams                      (NU_USBH_XHCI                  *xhci,
                                                     UINT16                        num_streams,
                                                     NU_USBH_XHCI_STREAM_INFO      **strm_info);

STATUS   XHCI_Deallocate_Streams                    (NU_USBH_XHCI                  *xhci,
                                                     NU_USBH_XHCI_STREAM_INFO      *strm_info );

STATUS   XHCI_Queue_Command                         (NU_USBH_XHCI                  *xhci_cb,
                                                     UINT32                        trb_info1,
                                                     UINT32                        trb_info2,
                                                     UINT32                        trb_info3,
                                                     UINT32                        trb_info4);

STATUS   XHCI_Queue_Slot_Control_Command            (NU_USBH_XHCI                  *xhci,
                                                     UINT32                        trb_type,
                                                     UINT8                         slot_id);

STATUS   XHCI_Queue_Configure_EP_Command            (NU_USBH_XHCI                  *xhci_cb,
                                                     NU_USBH_XHCI_RAW_CTX          *ctx_ptr,
                                                     UINT8                         slot_id);

STATUS   XHCI_Queue_NOOP_Command                    (NU_USBH_XHCI                  *xhci);

STATUS   XHCI_Queue_Reset_EP_Command                (NU_USBH_XHCI                  *xhci_cb,
                                                     UINT8                         ep_index,
                                                     UINT8                         slot_id);

STATUS   XHCI_Queue_Set_TR_Dequeue_Command          (NU_USBH_XHCI                  *xhci_cb,
                                                     VOID                          *deq_ptr,
                                                     UINT8                         ep_index,
                                                     UINT32                        cycle_state,
                                                     UINT16                        stream_id,
                                                     UINT8                         slot_id);

STATUS   XHCI_Queue_Address_Device_Command          (NU_USBH_XHCI                  *cb,
                                                     NU_USB_IRP                    *irp,
                                                     NU_USBH_XHCI_DEVICE           *xhci_dev,
                                                     UINT8                         slot_id,
                                                     UINT8                         ep_index,
                                                     UINT8                         func_addr);

STATUS   XHCI_Parse_Config_Endpoint_In_Ctx          (NU_USBH_XHCI                  *xhci,
                                                     NU_USBH_XHCI_RAW_CTX          *in_ctx,
                                                     VOID                          *addr,
                                                     UINT8                         bEndpointAddress,
                                                     UINT8                         bmEndpointAttributes,
                                                     UINT8                         speed,
                                                     UINT8                         bMaxBurst,
                                                     UINT16                        wMaxPacketSize,
                                                     UINT32                        interval,
                                                     UINT8                         SSEndpCompAttrib,
                                                     UINT16                        bytes_per_interval);

STATUS   XHCI_Queue_Evaluate_Context_Command        (NU_USBH_XHCI                  *xhci_cb,
                                                     NU_USBH_XHCI_RAW_CTX          *in_ctx,
                                                     UINT8                         slot_id);

STATUS   XHCI_Queue_Reset_Endpoint_Command          (NU_USBH_XHCI                  *xhci_cb,
                                                     UINT8                         slot_id,
                                                     UINT8                         ep_index);

STATUS   NU_USBH_XHCI_Unlock                        (NU_USBH_XHCI                  *xhci);

STATUS   NU_USBH_XHCI_Lock                          (NU_USBH_XHCI                  *xhci);
                                                                                   
STATUS   XHCI_RH_Get_Status                         (NU_USBH_XHCI                  *xhci,
                                                     NU_USBH_CTRL_IRP              *ctrl_irp,
                                                     UINT8                         speed);

STATUS   XHCI_RH_Clear_Feature                      (NU_USBH_XHCI                  *xhci,
                                                     NU_USBH_CTRL_IRP              *ctrl_irp,
                                                     UINT8                         speed);

STATUS   XHCI_Setup_RH_Ports                        (NU_USBH_XHCI                  *xhci,
                                                     XHCI_USB3_HUB                 *usb3_hub);

STATUS   XHCI_RH_Get_State                          (NU_USBH_XHCI                  *xhci,
                                                     NU_USBH_CTRL_IRP              *ctrl_irp,
                                                     UINT8                         speed);

STATUS   XHCI_RH_Set_Feature                        (NU_USBH_XHCI                  *xhci,
                                                     NU_USBH_CTRL_IRP              *ctrl_irp,
                                                     UINT8                         speed);

STATUS   XHCI_RH_Invalid_CMD                        (NU_USBH_XHCI                  *xhci,
                                                     NU_USBH_CTRL_IRP              *ctrl_irp,
                                                     UINT8                         speed);

STATUS   XHCI_RH_Nothing_To_Do_CMD                  (NU_USBH_XHCI                  *xhci,
                                                     NU_USBH_CTRL_IRP              *ctrl_irp,
                                                     UINT8                         speed);

STATUS   XHCI_RH_Get_Descriptor                     (NU_USBH_XHCI                  *xhci,
                                                     NU_USBH_CTRL_IRP              *ctrl_irp,
                                                     UINT8                         speed);

VOID    XHCI_Disable_Interrupts                     (NU_USBH_XHCI                  *xhci);

VOID    XHCI_Enable_Interrupts                      (NU_USBH_XHCI                  *xhci);

STATUS  XHCI_Place_IRP_On_Pending_List              (NU_USBH_XHCI                  *xhci,
                                                     NU_USBH_XHCI_EP_INFO          *ep_cb,
                                                     NU_USB_IRP                    *irp,
                                                     UINT32                        req_trbs,
                                                     UINT16                        stream_id);

STATUS XHCI_Process_Pending_IRPS                    (NU_USBH_XHCI                  *xhci,
                                                     NU_USBH_XHCI_EP_INFO          *ep_cb,
                                                     UINT8                         ep_index,
                                                     UINT8                         slot_id,
                                                     UINT16                        ep_max_pack);

STATUS XHCI_Handle_Control_TD                       (NU_USBH_XHCI                  *xhci,
                                                     NU_USBH_XHCI_RING             *ring,
                                                     NU_USBH_XHCI_TD               *td,
                                                     NU_USBH_XHCI_IRP              *xhci_irp,
                                                     BOOLEAN                       data_in,
                                                     UINT16                        max_packet);

STATUS XHCI_Copy_Endpoint_Context                   (NU_USBH_XHCI                  *xhci,
                                                     NU_USBH_XHCI_RAW_CTX          *in_ctx,
                                                     NU_USBH_XHCI_RAW_CTX          *out_ctx,
                                                     UINT8                         ep_index);

STATUS XHCI_Copy_Slot_Context                       (NU_USBH_XHCI                  *xhci,
                                                     NU_USBH_XHCI_RAW_CTX          *in_ctx,
                                                     NU_USBH_XHCI_RAW_CTX          *out_ctx);

STATUS   XHCI_RH_Initialize                         (NU_USBH_XHCI                  *xhci);
                                                    
STATUS   NU_USBH_XHCI_Cleanup                       (NU_USBH_XHCI                  *xhci);
                                                    
STATUS   XHCI_Deallocate_Device_Context             (NU_USBH_XHCI_RAW_CTX          *ctx);
                                                    
STATUS   XHCI_Queue_Stop_Endpoint_Command           (NU_USBH_XHCI                  *xhci,
                                                     UINT8                         ep_index,
                                                     UINT32                        cycle_state,
                                                     UINT16                        stream_id,
                                                     UINT8                         slot_id);

STATUS   XHCI_RH_Handle_IRP                         (NU_USBH_XHCI                  *xhci,
                                                     NU_USB_IRP                    *irp,
                                                     UINT8                          bEndpointAddress);

VOID    XHCI_Event_Handler                          (NU_USBH_XHCI                  *xhci);

STATUS  XHCI_Ring_Command_Doorbell                  (NU_USBH_XHCI                  *xhci);

STATUS  XHCI_Parse_Address_Cmd_In_Ctx               (NU_USBH_XHCI                  *xhci,
                                                     NU_USBH_XHCI_DEVICE           *xhci_dev,
                                                     NU_USBH_XHCI_RING             *ring,
                                                     NU_USB_DEVICE                 *usb_device);

STATUS  xhci_transfer_done                          (NU_USBH_XHCI                  *xhci,
                                                     UINT8                         *actual_buffer,
                                                     UINT8                         *non_cache_buffer,
                                                     UINT8                         *raw_buffer,
                                                     UINT32                        length,
                                                     UINT32                        alignment,
                                                     BOOLEAN                       sync);

STATUS xhci_normalize_buffer                        (NU_USBH_XHCI                  *xhci,
                                                     UINT8                         *buffer,
                                                     UINT32                        length,
                                                     UINT32                        alignment,
                                                     BOOLEAN                       sync,
                                                     UINT8                         **buffer_out,
                                                     UINT8                         **raw_buffer);

STATUS XHCI_Start_Controller                        (NU_USBH_XHCI                  *xhci);

STATUS XHCI_Stop_Controller                         (NU_USBH_XHCI                  *xhci);

#include "nu_usbh_xhci_dat.h"

#endif   /* NU_USBH_XHCI_IMP_H. */

/* ======================  End Of File.  =============================== */
