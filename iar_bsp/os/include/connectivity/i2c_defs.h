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
*       i2c_defs.h
*
* COMPONENT
*
*       I2C - I2C Core
*
* DESCRIPTION
*
*       This file contains constants definitions and data structures
*       declarations for Nucleus I2C.
*
* DATA STRUCTURES
*
*       I2C_DRIVER_SERVICES                 Driver callback routines.
*
*       I2C_CB                              Nucleus I2C control block
*                                           structure.
*
*
* DEPENDENCIES
*
*       i2c.h                               Main definition file for
*                                           Nucleus I2C.
*
*       i2cbm_defs.h                        Definition file for Nucleus I2C
*                                           buffer management.
*
*       i2cm_defs.h                         Definition file for Nucleus I2C
*                                           Master.
*
*       i2c_driver.h                        Definition file for Nucleus I2C
*                                           Driver.
*
*************************************************************************/

/* Check to see if the file has been included already. */
#ifndef     I2C_DEFS_H
#define     I2C_DEFS_H

#include    "connectivity/i2c.h"
#include    "connectivity/i2cbm_defs.h"
#include    "connectivity/i2cm_defs.h"

/* Maximum option size in metadata file */
#define     I2C_MAX_OPTION_SIZE     20

/* Defines for the handlers. */

#define     I2C_MASTER_DATA_RECEIVED    0x01/* Data received.           */
#define     I2C_SLAVE_DATA_RECEIVED     0x02/* Data received.           */
#define     I2C_MASTER_DATA_TX_COMPLETE 0x04/* Data transfer complete.  */
#define     I2C_SLAVE_DATA_TX_COMPLETE  0x08/* Data transfer complete.  */
#define     I2C_DATA_ACK_RECEIVED       0x10/* Acknowledgment of data
                                               has received.            */
#define     I2C_ADDRESS_ACK_RECEIVED    0x20/* Acknowledgment of address
                                               has received.            */
#define     I2C_SLAVE_ADDRESS_RECEIVED  0x40/* Slave has been addressed.*/

/* Minimum valid 7-bit I2C slave address. It starts after the reserved
  group 1 i.e. 0000 XXXb. */
#define     I2C_MIN_7BIT_VALID_ADDRESS  8

/* Maximum valid 7-bit I2C slave address. It ends before the start of
  the reserved group 2 i.e. 1111 XXXb */
#define     I2C_MAX_7BIT_VALID_ADDRESS  119

/* Minimum valid 10-bit I2C slave address. */
#define     I2C_MIN_10BIT_VALID_ADDRESS 0

/* Maximum valid 10-bit I2C slave address. No address is reserved in
   the upper part of 10-bit range. */
#define     I2C_MAX_10BIT_VALID_ADDRESS 1023

/* Code for marking an address as 10-bit address. */
#define     I2C_10BIT_ADDRESS_CODE      0xF0

/* Slave address for the default callbacks. */
#define     I2C_DEFAULT_CBSLAVE_ADDRESS 0xFFFF

/* Masks for getting address bits. */
#define     I2C_10BIT_HIGH_ADDRESS_MASK 0x0300

/* Nucleus I2C node states. */

#define     I2C_TRANSMITTING_ADDRESS    1   /* Address transmission in
                                               process.                 */
#define     I2C_WAITING_ADDRESS_ACK     2   /* Waiting for acknowledgement
                                               from slave.              */
#define     I2C_SLAVE_ACKED             3   /* Slave has acknowledged its
                                               address. Data transfer in
                                               process.                 */
#define     I2C_STOP_SENT               4   /* Stop signal has been sent
                                               on the I2C bus but bus has
                                               not been released yet.   */
#define     I2C_RESTART_SENT            5   /* Restart signal sent on the
                                               bus.                     */
#define     I2C_NODE_IDLE               6   /* Node has completed a
                                               transfer in all respects.*/

/* Possible ways of writing I2C applications using Nucleus I2C APIs. */

#define     I2C_AUTOMATIC_TX_API        1
#define     I2C_AUTOMATIC_RX_API        2
#define     I2C_FINE_CONTROL_TX_API     4
#define     I2C_FINE_CONTROL_RX_API     8
#define     I2C_AUTOMATIC_API           \
            (I2C_AUTOMATIC_TX_API | I2C_AUTOMATIC_RX_API)
#define     I2C_FINE_CONTROL_API        \
            (I2C_FINE_CONTROL_TX_API | I2C_FINE_CONTROL_RX_API)

/* API error checking macros for I2C */
#if ( NU_I2C_ERROR_CHECKING )

#define NU_I2C_PTRCHK(a) \
    if((a) == NU_NULL)     \
        return (I2C_INVALID_PARAM_POINTER);
        
#define NU_I2C_PTRCHK_RETURN(a) \
    if((a) == NU_NULL){         \
        NU_USER_MODE();         \
        return (I2C_INVALID_PARAM_POINTER);\
    }
    
#define NU_I2C_VALCHK_RETURN(a) \
    if(!a){         \
        NU_USER_MODE();         \
        return (I2C_INVALID_PARAM);\
    }
    
#else

#define NU_I2C_PTRCHK(a)
#define NU_I2C_PTRCHK_RETURN(a)
#define NU_I2C_VALCHK_RETURN(a)

#endif


/* Structure for implementing the callbacks list. */
typedef struct I2C_APP_CALLBACKS_NODE_STRUCT
{
    /* Pointer to the callbacks structure. */
    I2C_APP_CALLBACKS                       *callback;

    /* Pointer to next callbacks node structure. */
    struct I2C_APP_CALLBACKS_NODE_STRUCT    *next;

} I2C_APP_CALLBACKS_NODE;

/* Nucleus I2C control block structure. */
typedef struct I2C_CB_STRUCT
{
    /* DM integration */
    INT                 i2c_ioctl_base;
    INT                 device_opened;
    DV_DEV_HANDLE       i2c_dv_handle;        /* I2C device handle */            
    INT                 is_opened;
    
    I2C_IO_BUFFER       i2c_io_buffer;      /* I2C I/O messages buffer. */
    I2C_NODE            i2c_node_address;   /* I2C slave address.       */
    I2C_MTR_QUEUE       i2c_mtr_queue;      /* Contains pointer for data
                                               required multi transfer. */
    I2C_APP_CALLBACKS_NODE  
                        i2c_ucb;            /* List of user callbacks.  */
    VOID               *i2c_memory_pool;    /* Memory pool pointer.     */
    VOID               *i2c_reserved;       /* Reserved field.          */
    UNSIGNED_INT        i2c_reserved2;      /* Reserved field 2.        */
    UNSIGNED_INT        i2c_transfer_size;  /* Keeps length of current 
                                               transfer.                */
    INT                 i2c_handler_type;   /* I2C handler type.        */    
    UINT16              i2c_baudrate;       /* I2C baud rate.           */
    DV_DEV_ID           i2c_dv_id;          /* I2C device id */  
    UINT8               i2c_driver_mode;    /* Flag to check if Nucleus
                                               I2C driver is operating in
                                               polling mode or Interrupt
                                               driven mode.             */
    UINT8               i2c_node_state;     /* Flag to check if node is
                                               transmitter or receiver. */
    UINT8               i2c_10bit_address2; /* Used to save bit 0-7 for
                                               10-bit slave address.    */
    UINT8               i2c_api_mode;       /* Identifies which type of
                                               API (automatic or fine
                                               control) is executing.   */
    UINT8               i2c_dev_user_count; /* Keeps the record of the
                                               number of I2C device users
                                               currently using the
                                               device.                  */
    UINT8               i2c_active_mode;    /* Keeps the record of whether 
                                               I2C is operating in master 
                                               or slave mode. */ 

    /* Padding to align the structure. */
    UINT8               i2c_padding[2];

} I2C_CB;


#define I2CS_Get_CB(i2c_handle,p_i2c_cb)\
    {\
        (*p_i2c_cb) = (I2C_CB*)(i2c_handle);\
    }

#define I2CS_Get_Handle(i2c_cb,p_i2c_handle)\
    {\
        *(p_i2c_handle) = (I2C_HANDLE)i2c_cb;\
    }

#endif      /* !I2C_DEFS_H */
