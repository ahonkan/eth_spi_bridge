/*************************************************************************
*
*                  Copyright 2006 Mentor Graphics Corporation
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
*       i2c.h
*
* COMPONENT
*
*       I2C - Nucleus I2C Core
*
* DESCRIPTION
*
*       This file contains data structures and APIs provided with
*       Nucleus I2C.
*
* DATA STRUCTURES
*
*       I2C_HANDLE                          Nucleus I2C handle structure.
*
*       I2C_NODE                            Nucleus I2C node structure.
*
*       I2C_APP_CALLBACKS                   Data structure containing
*                                           Nucleus I2C application
*                                           callback pointers.
*
*       I2C_INIT                            Data structure to contain
*                                           Nucleus I2C initialization
*                                           information.
*
* DEPENDENCIES
*
*       i2c_osal.h                          OS abstraction for Nucleus
*                                           I2C.
*
*************************************************************************/
/* Check to see if the file has been included already. */
#ifndef     I2C_H_FILE

#ifdef      __cplusplus
extern      "C" {             /* C declarations in C++. */
#endif

#define     I2C_H_FILE

#include    "connectivity/i2c_osal.h"

/* Define Nucleus I2C release string */
#define     I2C_RELEASE_STRING          "Nucleus I2C 1.1"

/* Define Nucleus I2C product identification string. */
#define     I2C_ID                      "I2C"

/* Define Nucleus I2C version numbering. */

#define     I2C_1_0                     "1.0"
#define     I2C_1_1                     "1.1"
#define     NU_I2C_VERSION              I2C_1_1

/* Possible address length for the slave. */

#define     I2C_7BIT_ADDRESS            0
#define     I2C_10BIT_ADDRESS           1

/* Possible I2C abort operations. */

#define     I2C_STOP                    0
#define     I2C_RESTART                 1

/* I2C read/write operations. */

#define     I2C_WRITE                   0
#define     I2C_READ                    1
#define     I2C_RW_MASK                 0x1

/* Operating modes for Nucleus I2C. */

#define     I2C_INTERRUPT_MODE          0
#define     I2C_POLLING_MODE            1

/* Possible I2C acknowledgment callback indications. */

#define     I2C_DATA_ACK                0
#define     I2C_ADDRESS_ACK             1

/* General call address. */
#define     I2C_GENERAL_CALL_ADDRESS    0x00

/* Define the data type for the driver mode. */
typedef     UINT8                       I2C_DRIVER_MODE;


/* Data structure containing information about the node. */
typedef struct I2C_NODE_STRUCT
{
    UINT16  i2c_slave_address;              /* Address of the node
                                               as a slave.              */
    UINT8   i2c_address_type;               /* Is slave address
                                               7bit/10bit?              */
    UINT8   i2c_node_type;                  /* Is node master/
                                               slave/both?              */

} I2C_NODE;

#define I2C_HANDLE  UNSIGNED

/* Structure for callback routines to be implemented by the user. */
typedef struct I2C_APP_CALLBACKS_STRUCT
{
    /* Callback for telling the users of the received data. */
    VOID    (*i2c_data_indication)          (I2C_HANDLE    i2c_handle,
                                             UINT8         i2c_node_type,
                                             UNSIGNED_INT  byte_count);

    /* Callback for telling the user about the completion of write
       operation by master/slave. */
    VOID    (*i2c_transmission_complete)    (I2C_HANDLE    i2c_handle,
                                             UINT8         i2c_node_type);

    /* Callback to tell the user about address indication/match. */
    VOID    (*i2c_address_indication)       (I2C_HANDLE    i2c_handle,
                                             UINT8         rw);

    /* Callback for telling the user about acknowledgment. */
    VOID    (*i2c_ack_indication)           (I2C_HANDLE    i2c_handle,
                                             UINT8         ack_type);

    /* Callback function which will indicate the error code of a serious
       I2C error. */
    VOID    (*i2c_error)                    (I2C_HANDLE    i2c_handle,
                                            STATUS    error_code);
                                            

    /* Address of the I2C slave device for which these callbacks are
       registered. */
    UINT16  i2c_slave_address;

    /* Padding to align the structure. */
    UINT8   i2c_padding[2];
} I2C_APP_CALLBACKS;

/* Data structure for initializing Nucleus I2C and Nucleus I2C Driver. */
typedef struct I2C_INIT_STRUCT
{
    I2C_APP_CALLBACKS   i2c_app_cb;         /* Application callbacks.   */
    I2C_NODE            i2c_node_address;   /* Local slave address.     */
    VOID               *i2c_memory_pool;    /* Memory pool pointer      */
    UNSIGNED_INT        i2c_tx_buffer_size; /* Output buffer size in
                                               bytes.                   */
    UNSIGNED_INT        i2c_rx_buffer_size; /* Input buffer size in
                                               bytes.                   */
    UNSIGNED_INT        i2c_baudrate;       /* I2C baud rate.           */
    UINT8               i2c_dv_id;         /* The I2C controller
                                               identifier.              */
    UINT8               i2c_driver_mode;    /* Flag for Nucleus I2C
                                               polling/interrupt mode.  */
    UINT8               i2c_active_mode;    /* Keeps the record of whether I2C
                                               is operating in master or slave 
                                               mode. */ 
    UINT8               i2c_padding[1];     /* Padding to align the
                                               structure.               */
} I2C_INIT;


/* Definitions for various possible parameters to driver functions. */

#define     I2C_CHECK_ADDRESS_ACK       1
#define     I2C_CHECK_DATA_ACK          2
#define     I2C_CHECK_NACK              3
#define     I2C_CHECK_BUS_FREE          4
#define     I2C_CHECK_DATA              5
#define     I2C_CHECK_RESTART           6
#define     I2C_CHECK_STOP              7
#define     I2C_SEND_NACK               10
#define     I2C_SEND_ACK                11
#define     I2C_SEND_START_ADDRESS      12
#define     I2C_SEND_RESTART_ADDRESS    13
#define     I2C_SEND_ADDRESS2           14
#define     I2C_SEND_DATA               15
#define     I2C_SEND_STOP               16
#define     I2C_SET_BAUDRATE            17
#define     I2C_SET_SLAVE_ADDRESS       18
#define     I2C_SET_NODE_MODE_RX        19
#define     I2C_SET_NODE_MODE_TX        20
#define     I2C_FREE_BUS                21
#define     I2C_ENABLE_INTERRUPT        22
#define     I2C_DISABLE_INTERRUPT       23
#define     I2C_SET_MASTER_MODE         24
#define     I2C_SET_SLAVE_MODE          25
#define     I2C_SET_CONTROL_BLOCK       26
#define     I2C_GET_MW_CONFIG_PATH      27
#define     I2C_ENABLE_DEVICE           28
#define     I2C_DISABLE_DEVICE          29
#define     I2C_SEND_TRANSFER_COUNT     30
#define     I2C_CMD_PWR_HIB_RESTORE     31

/* Ioctl command delimiter */
#define     I2C_CLASS_CMD_DELIMITER     33

/* Error constant for Nucleus I2C.
   The range of errors is specified like this.
   1-10         OS specific errors
   11-30        I2C protocol specific error constants
   31-50        Nucleus I2C software specific errors
   51-60        Nucleus I2C buffer management errors
   61-70        Nucleus I2c device managemnet errors
   71-99        Reserved
   100-150      Hardware specific errors
   Remaining    Open for application.   */

/* OS specific error codes. */

#define     I2C_OS_ERROR                1   /* Any OS error occurred.   */
#define     I2C_NULL_GIVEN_FOR_MEM_POOL 2   /* Memory pool pointer was
                                               not set properly.        */
#define     I2C_NO_ACTIVE_NOTIFICATION  3   /* No active notification for
                                               I2C notification handler.*/
#define     I2C_NULL_GIVEN_FOR_INIT     4   /* Initialization pointer is
                                               null.                    */

/* I2C protocol specific error constants. */

#define     I2C_INVALID_SLAVE_ADDRESS   11  /* Slave address is not
                                               valid.                   */
#define     I2C_SLAVE_NOT_ACKED         12  /* Slave didn't acknowledge.*/
#define     I2C_BUS_BUSY                13  /* I2C bus is busy.         */
#define     I2C_TRANSFER_INIT_FAILED    14  /* Transfer on I2C bus could
                                               not be initialized.      */
#define     I2C_ACK_TX_FAILED           15  /* Acknowledgment transmission
                                               failure.                 */
#define     I2C_INVALID_ADDRESS_TYPE    16  /* Type of the slave address
                                               is neither 7-bit nor
                                               10-bit.                  */
#define     I2C_7BIT_ADDRESS_TYPE       17  /* 7-bit address type.      */
#define     I2C_10BIT_ADDRESS_TYPE      18  /* 10-bit address type.     */
#define     I2C_TRANSFER_STOP_FAILED    19  /* Transfer on I2C bus could
                                               not be stopped.          */
#define     I2C_NO_STOP_SIGNAL          20  /* STOP signal not received.*/
#define     I2C_NO_RESTART_SIGNAL       21  /* RESTART signal not
                                               received. */
#define     I2C_ADDRESS_TX_FAILED       22  /* Master could not address
                                               the slave node perhaps due
                                               to the reason that it lost
                                               arbitration.             */
#define     I2C_TRANSFER_IN_PROGRESS    23  /* An I2C transfer is already
                                               in progress.             */
#define     I2C_INVALID_NODE_TYPE       24  /* Node is not master/slave.*/
#define     I2C_NODE_NOT_SLAVE          25  /* The node is not slave.   */
#define     I2C_NODE_NOT_MASTER         26  /* The node is not master.  */
#define     I2C_INVALID_OPERATION       27  /* Invalid I2C operation type
                                               should be I2C_READ or
                                               I2C_WRITE.               */

/* Nucleus I2C software specific errors. */

#define     I2C_INVALID_PORT_ID         31  /* Invalid I2C driver port. */
#define     I2C_INVALID_DEVICE_ID       32  /* Invalid controller ID.   */
#define     I2C_INVALID_HANDLE          33  /* Nucleus I2C device handle
                                               is invalid. */
#define     I2C_INVALID_HANDLE_POINTER  34  /* Specified pointer is not
                                               valid.                   */
#define     I2C_SHUTDOWN_ERROR          35  /* I2C controller could not
                                               be closed.               */
#define     I2C_DEV_ALREADY_INIT        36  /* Nucleus I2C device is
                                               already initialized.     */
#define     I2C_DEV_SLEEPING            38  /* Nucleus I2C device is
                                               currently in sleep mode. */
#define     I2C_DRIVER_REGISTER_FAILED  39  /* Hardware driver registration
                                               with Nucleus I2C failed. */
#define     I2C_DRIVER_INIT_FAILED      40  /* The device initialization
                                               failed. */
#define     I2C_INVALID_DRIVER_MODE     41  /* Driver mode is not set to
                                               either interrupt driven
                                               or polling mode.         */
#define     I2C_SLAVE_TIME_OUT          42  /* Slave timed out.         */
#define     I2C_INVALID_VECTOR          43  /* Invalid vector ID.       */
#define     I2C_INVALID_PARAM_POINTER   44  /* Null given instead of a
                                               variable pointer.        */
#define     I2C_INVALID_SLAVE_COUNT     45  /* Slave count for
                                               multi transfer is wrong. */
#define     I2C_DATA_RECEPTION_FAILED   46  /* All data could not be
                                               received properly.       */
#define     I2C_DATA_TRANSMISSION_FAILED 47 /* All data could not be
                                               transmitted properly.    */
#define     I2C_INVALID_IOCTL_OPERATION 48  /* Unsupported operation
                                               requested from ioctl
                                               (driver I/O control)
                                               routine.                 */
#define     I2C_DEVICE_IN_USE           49  /* Device is being used by 
                                               other user.              */
/* Error constants for Nucleus I2C buffer management. */

#define     I2C_BUFFER_EMPTY            51  /* Data buffer is empty.    */
#define     I2C_BUFFER_FULL             52  /* Data buffer is full.     */
#define     I2C_BUFFER_NOT_EMPTY        53  /* Buffer still has data.   */
#define     I2C_BUFFER_HAS_LESS_DATA    54  /* More data was requested
                                               from buffer than
                                               available.               */
#define     I2C_INVALID_TX_BUFFER_SIZE  55  /* Transmission buffer size
                                               is zero.                 */
#define     I2C_INVALID_RX_BUFFER_SIZE  56  /* Receive buffer size is
                                               zero.                    */
#define     I2C_INVALID_TX_DATA_LENGTH  57  /* Invalid data length to put
                                               in the transmission
                                               buffer.                  */
#define     I2C_INVALID_RX_DATA_LENGTH  58  /* Invalid data length to put
                                               in the receive buffer.   */
#define     I2C_TX_BUFFER_NOT_ENOUGH    59  /* Insufficient transmission
                                               buffer memory space.     */
#define     I2C_RX_BUFFER_NOT_ENOUGH    60  /* Insufficient receive buffer
                                               memory space.            */

/* Nucleus I2C controller specific error constants. Refer to the
   respective product manual for detailed technical description of the
   errors. */

#define     I2C_SESSION_UNAVAILABLE     71
#define     I2C_DEV_NOT_FOUND           72
#define     I2C_DEV_REG_PATH_TOO_LONG   73   

#define     I2C_GENERAL_HARDWARE_ERROR  100 /* An undocumented error
                                               occurred in I2C driver.  */
#define     I2C_INVALID_BAUDRATE        101 /* The specified baud rate
                                               is not supported.        */
#define     I2C_TRANSMISSION_ABORTED    102 /* Notification transmission
                                               aborted.                 */
#define     I2C_POLLING_ABORTED         103 /* Polling aborted          */
#define     I2C_INVALID_PARAM           104 /* Invalid parameter    */

/***********************************************************************
 ***************    API declarations and mapping   *********************
 ***********************************************************************/

#ifndef     NU_I2C_SOURCE_FILE

/* Service call mapping for full control Nucleus I2C services.
These are more useful for interrupt driven mode. */

#define     NU_I2C_Master_Write_Byte        I2CMC_Write_Byte
#define     NU_I2C_Master_Read_Byte         I2CMC_Read_Byte
#define     NU_I2C_Master_Restart           I2CMC_Restart
#define     NU_I2C_Check_Ack                I2CC_Check_Ack
#define     NU_I2C_Ioctl_Driver             I2CC_Ioctl_Driver

/* Service call mapping for common Nucleus I2C routines. */

#define     NU_I2C_Start                    I2C_Start
#define     NU_I2C_Close                    I2C_Close
#define     NU_I2C_Receive_Data             I2CC_Receive_Data
#define     NU_I2C_Get_Node_State           I2CC_Get_Node_State
#define     NU_I2C_Slave_Get_Address        I2CSS_Slave_Get_Address
#define     NU_I2C_Slave_Set_Address        I2CSS_Slave_Set_Address

/* Service call mapping for Nucleus I2C master. */

#define     NU_I2C_Master_Start_Transfer    I2CMC_Start_Transfer
#define     NU_I2C_Master_Write             I2CMC_Write
#define     NU_I2C_Master_Read              I2CMC_Read
#define     NU_I2C_Master_Stop_Transfer     I2CMC_Stop_Transfer
#define     NU_I2C_Master_Multi_Transfer    I2CMC_Multi_Transfer
#define     NU_I2C_Master_Get_Baudrate      I2CMS_Get_Baudrate
#define     NU_I2C_Master_Set_Baudrate      I2CMS_Set_Baudrate
#define     NU_I2C_Master_Get_Callbacks     I2CMS_Get_Callbacks
#define     NU_I2C_Master_Set_Callbacks     I2CMS_Set_Callbacks
#define     NU_I2C_Master_Remove_Callbacks  I2CMS_Remove_Callbacks
#define     NU_I2C_Master_Get_Mode          I2CMS_Get_Mode

/* Service call mapping for Nucleus I2C slave. */
#define     NU_I2C_Slave_Response_To_Read   I2CSC_Response_to_Read

/* Service call mapping for general call related services. */

#define     NU_I2C_Master_Set_Slave_Address I2CMC_Set_Slave_Address
#define     NU_I2C_Master_Config_HW_Master  I2CMC_Config_HW_Master

/* Nucleus I2C common (master/slave) API. */

STATUS  NU_I2C_Start                    (I2C_HANDLE   *i2c_handle,
                                         I2C_INIT     *i2c_init);
STATUS  NU_I2C_Close                    (I2C_HANDLE    i2c_handle);
STATUS  NU_I2C_Receive_Data             (I2C_HANDLE    i2c_handle,
                                         UINT8        *data,
                                         UNSIGNED_INT  length);
STATUS  NU_I2C_Check_Ack                (I2C_HANDLE    i2c_handle);
STATUS  NU_I2C_Get_Node_State           (I2C_HANDLE    i2c_handle,
                                         UINT8        *node_state);

/* Nucleus I2C master API. */

STATUS  NU_I2C_Master_Read              (I2C_HANDLE    i2c_handle,
                                         I2C_NODE      slave_address,
                                         UINT8        *data,
                                         UNSIGNED_INT  length);

STATUS  NU_I2C_Master_Write             (I2C_HANDLE    i2c_handle,
                                         I2C_NODE      slave_address,
                                         UINT8        *data,
                                         UNSIGNED_INT  length);
STATUS  NU_I2C_Master_Multi_Transfer    (I2C_HANDLE    i2c_handle,
                                         I2C_NODE     *slaves,
                                         UINT8        *tx_data,
                                         UINT8        *rx_data,
                                         UNSIGNED_INT *lengths,
                                         UINT8        *rw,
                                         UINT8         slave_count);
STATUS  NU_I2C_Master_Get_Baudrate      (I2C_HANDLE    i2c_handle,
                                         UINT16       *baudrate);
STATUS  NU_I2C_Master_Set_Baudrate      (I2C_HANDLE    i2c_handle,
                                         UINT16        baudrate);
STATUS  NU_I2C_Master_Get_Callbacks     (I2C_HANDLE i2c_handle,
                                         UINT16 i2c_slave_address,
                                         I2C_APP_CALLBACKS **callbacks);
STATUS  NU_I2C_Master_Set_Callbacks     (I2C_HANDLE i2c_handle,
                                         UINT16 i2c_slave_address,
                                         I2C_APP_CALLBACKS *callbacks);
STATUS  NU_I2C_Master_Remove_Callbacks  (I2C_HANDLE i2c_handle,
                                         UINT16 i2c_slave_address);
STATUS  NU_I2C_Master_Get_Mode          (I2C_HANDLE i2c_handle,
                                         I2C_DRIVER_MODE *i2c_driver_mode);

/* Fine control API for Nucleus I2C (master/slave). These are more
   useful for polling mode. */

STATUS  NU_I2C_Master_Start_Transfer    (I2C_HANDLE    i2c_handle,
                                         I2C_NODE      slave,
                                         UINT8         rw);
STATUS  NU_I2C_Master_Write_Byte        (I2C_HANDLE    i2c_handle,
                                         UINT8         data);
STATUS  NU_I2C_Master_Read_Byte         (I2C_HANDLE    i2c_handle,
                                         UINT8        *data);
STATUS  NU_I2C_Master_Restart           (I2C_HANDLE    i2c_handle,
                                         I2C_NODE      slave,
                                         UINT8         rw);
STATUS  NU_I2C_Ioctl_Driver             (I2C_HANDLE    i2c_handle,
                                         UINT8         operation_code,
                                         VOID         *operation_data);

/* Nucleus I2C slave API. */

STATUS  NU_I2C_Slave_Response_To_Read   (I2C_HANDLE    i2c_handle,
                                         UINT8        *data,
                                         UNSIGNED_INT  length);
STATUS  NU_I2C_Slave_Get_Address        (I2C_HANDLE    i2c_handle,
                                         UINT16       *address,
                                         UINT8        *address_type);
STATUS  NU_I2C_Slave_Set_Address        (I2C_HANDLE    i2c_handle,
                                         UINT16        slave_address,
                                         UINT8         address_type);

/* Nucleus I2C general call based APIs. */

STATUS  NU_I2C_Master_Set_Slave_Address (I2C_HANDLE    i2c_handle,
                                         BOOLEAN       reset);
STATUS  NU_I2C_Master_Config_HW_Master  (I2C_HANDLE    i2c_handle,
                                         UINT8       hw_master_address,
                                         UINT8       hw_master_slave);

/* Mapping to provide some fine control API for Nucleus I2C. */

#define     NU_I2C_Send_Ack(i2c_handle)                     \
            NU_I2C_Ioctl_Driver(i2c_handle, I2C_SEND_ACK, NU_NULL)

#define     NU_I2C_Send_Nack(i2c_handle)                    \
            NU_I2C_Ioctl_Driver(i2c_handle, I2C_SEND_NACK,NU_NULL)

#define     NU_I2C_Master_Set_Node_Rx(i2c_handle)           \
            NU_I2C_Ioctl_Driver(i2c_handle, I2C_SET_NODE_MODE_RX, NU_NULL)

#define     NU_I2C_Master_Free_Bus(i2c_handle)          \
            NU_I2C_Ioctl_Driver(i2c_handle, I2C_FREE_BUS, NU_NULL)

#endif      /* !NU_I2C_SOURCE_FILE */

#ifdef      __cplusplus
}
#endif

#endif      /* !I2C_H_FILE */
