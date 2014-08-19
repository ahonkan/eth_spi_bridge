/*************************************************************************
*
*            Copyright 2002-2007 Mentor Graphics Corporation
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
*       can.h
*
* COMPONENT
*
*       Nucleus CAN
*
* DESCRIPTION
*
*       This file contains data structures and APIs provided with
*       Nucleus CAN.
*
* DATA STRUCTURES
*
*       CAN_PACKET                          Nucleus CAN packet structure.
*
*       CAN_QM                              Nucleus CAN queue management
*                                           structure.
*
*       CAN_QUEUE                           Nucleus CAN queue structure.
*
*       CAN_APP_CALLBACKS                   Nucleus CAN application
*                                           callbacks.
*
*       CAN_INIT                            Nucleus CAN initialization
*                                           control block.
*
*       CAN_CB                              Nucleus CAN control block.
*
* DEPENDENCIES
*
*       can_osal.h                          OS abstraction for Nucleus CAN.
*
*************************************************************************/
/* Check to see if the file has been included already. */
#ifndef     CAN_H

#ifdef      __cplusplus
extern  "C" {                               /* C declarations in C++. */
#endif

#define     CAN_H

#include    "can_osal.h"

/* Define Nucleus CAN release string */
#define     CAN_RELEASE_STRING                 "Nucleus CAN 2.0"

/* Define Nucleus CAN product identification string. */
#define     CAN_ID                             "CAN"

/* Define GUID for Nucleus CAN. */
#define     CAN_LABEL    {0xab, 0x75, 0xd0, 0x99, 0xb6, 0x80, 0x4e, 0x14, \
                          0x8b, 0x20, 0xf6, 0xe7, 0x8d, 0xb3, 0xcb, 0xbd}

/* Define Nucleus CAN version numbering. */

#define     CAN_1_1                 "1.1"
#define     CAN_1_2                 "1.2"
#define     CAN_1_3                 "1.3"
#define     CAN_2_0                 "2.0"
#define     NU_CAN_VERSION          CAN_2_0

/* Maximum length of CAN data packet (in bytes) as specified by ISO11898. */
#define     CAN_MAX_DATA            0x8

/* Maximum baud rate (in Kbps) supported by Nucleus CAN. */
#define     CAN_MAX_BAUDRATE        1000    /* 1000Kbps / 1Mbps. */

/* CAN identifier maximum values. */

#define     CAN_MAX_EXTENDED_ID     0x1FFFFFFFUL
#define     CAN_MAX_STANDARD_ID     0x000007FFUL

/* Defines for CAN message types. */

#define     CAN_DATA_MSG            0x0     /* CAN data message.        */
#define     CAN_RTR_MSG             0x1     /* CAN RTR message.         */

/* Defines for CAN message ID types. */

#define     CAN_STANDARD_ID         0x0     /* 11-bit identifier.       */
#define     CAN_EXTENDED_ID         0x1     /* 29-bit identifier.       */

/* Maximum number of ports that can be integrated with Nucleus CAN. */
#define     CAN_MAX_SUPPORTED_PORTS 4

/* Define to mark a Nucleus CAN device as initialized. */
#define     CAN_DEV_IS_INITIALIZED  0x1

/* IOCTL commands definitions for use with Nucleus CAN. */
#define NU_CAN_IOCTL_BASE           (DV_IOCTL0 + 1)
#define NU_CAN_INITIALIZE           0
#define NU_CAN_DATA_REQUEST         1
#define NU_CAN_SLEEP_NODE           2
#define NU_CAN_SET_ACP_MASK         3
#define NU_CAN_DATA_WRITE           4
#define NU_CAN_WAKEUP_NODE          5
#define NU_CAN_SET_BAUD_RATE        6
#define NU_CAN_REMOTE_REQUEST       7
#define NU_CAN_ASSIGN_MSG_BUFFER    8
#define NU_CAN_RELEASE_MSG_BUFFER   9
#define NU_CAN_SEND_RTR_RESPONSE    10
#define TOTAL_NU_CAN_IOCTLS         11

typedef     STATUS (*CAN_Dev_Init)(UINT8);

/* Data type representing data field in CAN packet. */
typedef     UINT8                   CAN_DATA[CAN_MAX_DATA];

/* Nucleus CAN device handle definition. */
typedef INT CAN_HANDLE;

/* Structure describing a CAN packet. */
typedef struct CAN_PACKET_STRUCT
{
    UINT32          can_msg_id;             /* CAN Message ID.          */
    UINT8           can_msg_id_type;        /* Basic/Extended message.  */
    UINT8           can_msg_type;           /* Data/RTR.                */
    UINT8           can_msg_length;         /* Number of data bytes.    */
    CAN_HANDLE      can_dev;                /* CAN device (controller/
                                               driver) number to use.   */
    CAN_DATA        can_msg_data;           /* Data of CAN message.     */

#if         NU_CAN_ENABLE_TIME_STAMP

    UNSIGNED        can_msg_timestamp;      /* CAN message time stamp.  */

#endif      /* NU_CAN_ENABLE_TIME_STAMP */

#if         NU_CAN_MULTIPLE_PORTS_SUPPORT

    UINT8           can_port_id;            /* CAN driver port ID.      */
    UINT8           can_struct_padding[3];  /* Three bytes padding to
                                               align the structure.     */

#endif      /* NU_CAN_MULTIPLE_PORTS_SUPPORT */

} CAN_PACKET;

/* Structure for general queue manipulation. */
typedef struct CAN_QM_STRUCT
{
    CAN_PACKET     *can_qwrite;             /* Queue write pointer.     */
    CAN_PACKET     *can_qread;              /* Queue read pointer.      */
    CAN_PACKET     *can_qend;               /* Queue end pointer.       */
    STATUS          can_qerror;             /* Queue error pointer.     */
    UINT16          can_qcount;             /* Queue elements counter.  */
    UINT16          can_qsize;              /* Queue size in elements.  */

} CAN_QM;

/* Structure for Nucleus CAN queue management. */
typedef struct CAN_QUEUE_STRUCT
{
    CAN_PACKET     *can_buff_out;           /* Output CAN buffer. Points
                                               to the start of output
                                               queue.                   */
    CAN_PACKET     *can_buff_in;            /* Input CAN buffer. Points
                                               to the start of input
                                               queue.                   */
    CAN_QM          can_tx_queue;           /* CAN transmit data queue. */
    CAN_QM          can_rx_queue;           /* CAN receive data queue.  */

} CAN_QUEUE;

/* Structure for callback routines to be implemented by the user. */
typedef struct CAN_APP_CALLBACKS_STRUCT
{
    /* Callback function which will indicate the arrival of a data message
       in input queue. */
    VOID            (*can_data_indication)  (UINT8 can_port_id,
                                             CAN_HANDLE can_dev_id);

    /* Callback function which will indicate the arrival of an RTR
       message. The response to the RTR will need to be implemented
       in this function if the user doesn't use automatic RTR
       response option. */
    VOID            (*can_rtr_indication)   (CAN_PACKET *can_message);

    /* Callback function which will indicate the confirmation status of
       the last data transmission request. */
    VOID            (*can_data_confirm)     (CAN_PACKET *can_message,
                                             STATUS status);

    /* Callback function which will indicate the confirmation status of
       the last RTR transmission request. */
    VOID            (*can_rtr_confirm)      (CAN_PACKET *can_message,
                                             STATUS status);

    /* Callback function which will indicate the error code of a serious
       CAN error. */
    VOID            (*can_error)            (STATUS error_code,
                                             UINT8 can_port_id,
                                             CAN_HANDLE can_dev_id);
} CAN_APP_CALLBACKS;

/* Nucleus CAN initialization structure. */
typedef struct CAN_INIT_STRUCT
{
    CAN_APP_CALLBACKS   can_callbacks;        /* User callbacks.           */
    NU_MEMORY_POOL     *can_memory_pool;      /* Memory pool pointer.      */
    UINT16              can_baud;             /* CAN baud rate.            */
    UINT8               can_port_id;          /* CAN device port ID.       */
    DV_DEV_LABEL        can_controller_label; /* Label for CAN controller. */

} CAN_INIT;

/* Nucleus CAN control block structure. */
typedef struct CAN_CB_STRUCT
{
    CAN_APP_CALLBACKS   can_ucb;            /* User callbacks.            */
    VOID               *can_reserved;       /* Field reserved for future
                                               usage.                     */
    UINT8              *can_base_address;   /* CAN base address pointer.  */
    STATUS              can_state;          /* Current CAN node state.    */
    STATUS              can_status;         /* CAN communication status.  */
    INT                 can_vector;         /* CAN vector number.         */
    UINT16              can_baud;           /* CAN baud rate.             */
    DV_DEV_ID           can_dev_id;         /* CAN device number.         */
    UINT8               can_handler_type;   /* Multiplexed field for
                                               CAN message handler type.  */
    CAN_QUEUE           can_queue;          /* CAN I/O messages queue.    */
    CAN_PACKET          can_buffer;         /* CAN message confirmation
                                               buffer. Only the last
                                               confirmed message will be
                                               placed here.               */
    UINT8               can_dev_init;       /* CAN driver port ID.        */
    UINT8               can_port_id;        /* CAN driver port ID.        */
    UINT8               can_struct_pad[2];  /* Two bytes padding to
                                               align the structure.       */
    DV_DEV_HANDLE       can_dev_handle;     /* Handle for CAN controller. */
    INT                 can_ioctl_base;
    BOOLEAN             is_opened;

} CAN_CB;

/* IOCTL structure for NU_CAN_SET_ACP_MASK. */
typedef struct _dv_ioctl_acp_mask_struct
{
    UINT8       buffer_no;
    UINT32      mask_value;
    CAN_HANDLE  can_dev_id;

} CAN_DRV_IOCTL_ACP_MASK;

/* IOCTL structure for NU_CAN_SET_BAUD_RATE. */
typedef struct _dv_ioctl_set_baud_struct
{
    UINT16      baud_rate;
    CAN_HANDLE  can_dev_id;

} CAN_DRV_IOCTL_SET_BAUD;

/* Index for loopback device. Do not change it. */
#define     CAN_LOOPBACK_DEVICE             0

/* CAN message handler types.
   These must be used by a hardware driver port to indicate which type of
   handler to execute when message is received. */

#define     CAN_DATA_TRANSMITTED        0x01U
#define     CAN_RTR_TRANSMITTED         0x02U
#define     CAN_DATA_RECEIVED           0x04U
#define     CAN_RTR_RECEIVED            0x08U

/* Mask buffer IDs for setting acceptance masks on various buffers. If a
   controller supports acceptance mask buffer per message buffer then the
   numerical number of that buffer may be given as input. */

#define     CAN_STANDARD_RECEIVE_MB     255
#define     CAN_EXTENDED_RECEIVE_MB     254
#define     CAN_GLOBAL_RECEIVE_MB       253

/* Mask values for acceptance configurations.
   Binary 1 at a bit position means, "must match".
   Binary 0 at a bit position means, "ignore/pass". */

#define     CAN_STANDARD_RECEIVE_MB_MASK    0x001FFFFF
#define     CAN_EXTENDED_RECEIVE_MB_MASK    0x00000000

/* Mask for transmission handler. */
#define     CAN_TX_HANDLER_MASK         \
            (CAN_DATA_TRANSMITTED | CAN_RTR_TRANSMITTED)

/* Mask for reception handler. */
#define     CAN_RX_HANDLER_MASK         \
            (CAN_DATA_RECEIVED | CAN_RTR_RECEIVED)

/* Define containing the size of CAN_PACKET structure in bytes. */
#if         (NU_CAN_MULTIPLE_PORTS_SUPPORT && NU_CAN_ENABLE_TIME_STAMP)

#define     CAN_PACKET_SIZE                 24

#elif       ((NU_CAN_MULTIPLE_PORTS_SUPPORT || NU_CAN_ENABLE_TIME_STAMP))

#define     CAN_PACKET_SIZE                 20

#else

#define     CAN_PACKET_SIZE                 16

#endif      /* NU_CAN_MULTIPLE_PORTS_SUPPORT && NU_CAN_ENABLE_TIME_STAMP */

/* Macro to calculate the required memory in bytes for Nucleus CAN message
   handlers and queues. */
#define     CAN_STACK_MEM_SIZE ((CAN_PACKET_SIZE  *                       \
                                (CAN_INQUEUE_SIZE + CAN_OUTQUEUE_SIZE)) + \
                                (CAN_HANDLER_MEM_STACK * 2))

/* Error constant for Nucleus CAN.
   The range of errors is specified like this.
   1-10         OS specific errors
   11-30        ISO11898 specific ERROR constants
   31-50        Nucleus CAN software specific errors
   51-60        Nucleus CAN queue management errors
   61-70        Loopback errors
   71-99        Reserved
   101-150      Hardware specific errors
   Remaining    Open for application.   */

/* OS specific errors. */

#define     CAN_GENERAL_ERROR           1   /* Any error in CAN.        */
#define     CAN_NULL_GIVEN_FOR_MEM_POOL 3   /* Memory pool pointer was
                                               not set properly.        */

/* ISO11898 specific ERROR constants. */

#define     CAN_ID_RANGE_ERROR          11  /* Specified message ID is
                                               out of range.            */
#define     CAN_INVALID_MSG_ID_TYPE     12  /* Message ID type is not
                                               valid.                   */
#define     CAN_NOT_RTR_MSG             13  /* Specified message is not
                                               an RTR message.          */
#define     CAN_NOT_DATA_MSG            14  /* Specified message is not a
                                               DATA message.            */
#define     CAN_INVALID_MSG_TYPE        15  /* Message type is not
                                               standard/extended.       */
#define     CAN_INVALID_DATA_LENGTH     16  /* Message length is greater
                                               than max allowed.        */
#define     CAN_RTR_ALREADY_ASSIGNED    17  /* The RTR response message
                                               is already set on node.  */

/* Nucleus CAN software specific errors. */

#define     CAN_UNSUPPORTED_PORT        31  /* Invalid CAN driver port. */
#define     CAN_UNSUPPORTED_CONTROLLER  32  /* Invalid controller ID.   */
#define     CAN_SERVICE_NOT_SUPPORTED   33  /* Requested service is not
                                               available.               */
#define     CAN_INVALID_POINTER         34  /* Specified pointer is
                                               null.                    */
#define     CAN_INVALID_MSG_POINTER     35  /* Specified message pointer
                                               is null.                 */
#define     CAN_SHUTDOWN_ERROR          36  /* CAN controller could not
                                               be closed.               */
#define     CAN_INVALID_PARAMETER       37  /* Value of a parameter given
                                               to the API is not valid. */
#define     CAN_DEV_ALREADY_INIT        38  /* Nucleus CAN device is
                                               already initialized.     */
#define     CAN_DEV_NOT_INIT            39  /* Nucleus CAN device is not
                                               initialized.             */
#define     CAN_DEV_SLEEPING            40  /* Nucleus CAN device is
                                               currently in sleep mode. */
#define     CAN_SESSION_NOT_AVAILABLE   41  /* There is no open session 
                                               available. */

/* Error constants for Nucleus CAN queue management. */

#define     CAN_QUEUE_EMPTY             51  /* Messages queue is empty. */
#define     CAN_QUEUE_FULL              52  /* Messages Queue is full.  */
#define     CAN_INPUTE_QUEUE_ERROR      53  /* Unspecified input queue
                                               error. */
#define     CAN_OUTPUT_QUEUE_ERROR      54  /* Unspecified output queue
                                               error.                   */
#define     CAN_SW_OVERRUN              55  /* Software overrun occurred
                                               in the queue.            */

/* Error constants for Nucleus CAN loopback device. */

#define     CAN_IN_LOOPBACK             61  /* Nucleus CAN is running in
                                               loopback.                */
#define     CAN_LOOPBACK_TX_ERROR       62  /* Error in loopback
                                               transmission.            */
#define     CAN_LOOPBACK_RX_ERROR       63  /* Error in loopback
                                               reception.               */
#define     CAN_LOOPBACK_NOT_INIT       64  /* Loopback device is not
                                               initialized.             */

/* Nucleus CAN controller specific error constants.
   Refer to the respective product manual for
   detailed technical description of the errors. */

#define     CAN_GENERAL_HARDWARE_ERROR  100 /* An undocumented error
                                               occurred in CAN driver.  */
#define     CAN_HW_OVERRUN              101 /* Hardware overrun in CAN .*/
#define     CAN_ERROR_ACTIVE_STATE      102 /* CAN node in error active
                                               state (i.e. normal state)*/
#define     CAN_ERROR_PASSIVE_STATE     103 /* CAN node in error passive
                                               state (temporary error). */
#define     CAN_BUS_OFF_STATE           104 /* CAN node in bus off state
                                               (permanent error).       */
#define     CAN_BIT_ERROR               105 /* CAN bit error.           */
#define     CAN_FRAME_ERROR             106 /* CAN framing error.       */
#define     CAN_ACK_ERROR               107 /* CAN acknowledgement error*/
#define     CAN_CRC_ERROR               108 /* CAN CRC error.           */
#define     CAN_ERROR_LIGHT             109 /* CAN error counter is in
                                               between 96 and 128.      */
#define     CAN_ERROR_HEAVY             110 /* CAN error counter is in
                                               between 127 and 256.     */
#define     CAN_ERROR_FATAL             111 /* CAN error counter exceeded
                                               255. */
#define     CAN_NO_FREE_MB              112 /* Currently no message buffer
                                               is free to use.          */
#define     CAN_NO_ASSIGNED_MB          113 /* No message buffer is
                                               assigned to the specified
                                               RTR response.            */
#define     CAN_NO_MASK_BUFF            114 /* The specified mask buffer
                                               is not supported by the
                                               controller.              */
#define     CAN_INVALID_BAUDRATE        115 /* The specified baud rate
                                               is not supported.        */
#define     CAN_TRANSMISSION_ABORTED    116 /* Message transmission
                                               aborted.                 */

/***********************************************************************
 ***************    API declarations and mapping   *********************
 ***********************************************************************/

#ifndef     NU_CAN_SOURCE_FILE

/* Service call mapping for Nucleus CAN. */
#define     NU_CAN_Start                    CANC_Start
#define     NU_CAN_Request_Data_Transfer    ISO11898_Data_Request
#define     NU_CAN_Request_Remote_Transfer  ISO11898_Remote_Request
#define     NU_CAN_Assign_RTR_Response      ISO11898_Assign_Remote
#define     NU_CAN_Clear_RTR_Response       ISO11898_Clear_Remote
#define     NU_CAN_Receive_Data             CANC_Receive_Data
#define     NU_CAN_Get_Node_State           CANC_Node_State
#define     NU_CAN_Sleep                    CANC_Sleep_Device
#define     NU_CAN_Wakeup                   CANC_Wakeup_Device
#define     NU_CAN_Set_Baud_Rate            CANC_Set_Baud_Rate
#define     NU_CAN_Get_Baud_Rate            CANC_Get_Baud_Rate
#define     NU_CAN_Close_Driver             CANC_Close_Driver
#define     NU_CAN_Set_Acceptance_Mask      CANFC_Set_Mask

/* Nucleus CAN API prototypes. */

/* APIs for Initialization/closing of Nucleus CAN. */

STATUS  NU_CAN_Start                    (CAN_HANDLE *can_dev, CAN_INIT *can_init);
STATUS  NU_CAN_Close_Driver             (UINT8  can_port_id,
                                             CAN_HANDLE  can_dev_id);

/* APIs for packet transmission from node. */

STATUS  NU_CAN_Request_Data_Transfer    (CAN_PACKET *can_msg);
STATUS  NU_CAN_Request_Remote_Transfer  (CAN_PACKET *can_msg);

/* APIs for setting/clearing automatic RTR responses. */

STATUS  NU_CAN_Assign_RTR_Response      (CAN_PACKET *can_msg);
STATUS  NU_CAN_Clear_RTR_Response       (CAN_PACKET *can_msg);

/* API to receive data from the input queue. */
STATUS  NU_CAN_Receive_Data             (CAN_PACKET *can_msg);

/* APIs for node sleep and wake up mode. */

STATUS  NU_CAN_Sleep                    (UINT8  can_port_id,
                                         CAN_HANDLE  can_dev_id);
STATUS  NU_CAN_Wakeup                   (UINT8  can_port_id,
                                         CAN_HANDLE  can_dev_id);

/* APIs for node state,baud rate and mask handling. */

STATUS  NU_CAN_Get_Node_State           (UINT8  can_port_id,
                                         CAN_HANDLE  can_dev_id);
UINT16  NU_CAN_Get_Baud_Rate            (UINT8  can_port_id,
                                         CAN_HANDLE  can_dev_id);
STATUS  NU_CAN_Set_Baud_Rate            (UINT8  can_port_id,
                                         CAN_HANDLE  can_dev_id,
                                         UINT16 baud_rate);
STATUS  NU_CAN_Set_Acceptance_Mask      (UINT8  can_port_id,
                                         CAN_HANDLE  can_dev_id,
                                         UINT8  buffer_no,
                                         UINT32 mask_value);

/* API to copy a CAN message from source to destination. */
VOID    NU_CAN_Copy_Message             (const CAN_PACKET *src_msg,
                                         CAN_PACKET       *dest_msg);

#endif      /* !NU_CAN_SOURCE_FILE */

#ifdef      __cplusplus
}
#endif

#endif      /* !CAN_H */

