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
* FILE NAME
*
*       spi.h
*
* COMPONENT
*
*       Nucleus SPI
*
* DESCRIPTION
*
*       This file contains data structures and APIs provided with
*       Nucleus SPI.
*
* DATA STRUCTURES
*
*       SPI_HANDLE                          Nucleus SPI device handle
*                                           structure.
*
*       SPI_APP_CALLBACKS                   Data structure containing
*                                           Nucleus SPI application
*                                           callback pointers.
*
*       SPI_INIT                            Data structure to contain
*                                           Nucleus SPI initialization
*                                           information.
*
*       SPI_TRANSFER_CONFIG                 Structure for SPI transfer
*                                           attributes configuration.
*
* DEPENDENCIES
*
*       spi_osal.h                          OS abstraction for Nucleus
*                                           SPI.
*
*************************************************************************/
/* Check to see if the file has been included already. */
#ifndef     SPI_H_FILE

#ifdef      __cplusplus
extern  "C" {                               /* C declarations in C++. */
#endif

#define     SPI_H_FILE

#include    "connectivity/spi_osal.h"

/* Maximum number of SPI sessions/instances */
#define SPI_MAX_INSTANCES          1
#define SPI_MAX_SESSIONS          (1 * SPI_MAX_INSTANCES)

/* SPI GUID */
#define    SPI_LABEL  {0x45,0x8A,0x3F,0xEC,0x7D,0xED,0x43,0x62,0xA0,0xD5,0x4E,0xF8,0x78,0xB2,0x2F,0x30}

/* Define Nucleus SPI release string. */
#define     SPI_RELEASE_STRING          "Nucleus SPI 1.1"

/* Define Nucleus SPI product identification string. */
#define     SPI_ID                      "SPI"

/* Define Nucleus SPI version numbering. */

#define     SPI_1_0                     "1.0"
#define     SPI_1_1                     "1.1"
#define     NU_SPI_VERSION              SPI_1_1

/* Transfer types. */

#define     SPI_TX                    0x01    /* Transmission.          */
#define     SPI_RX                    0x02    /* Reception.             */
#define     SPI_DUPLEX    (SPI_TX | SPI_RX)   /* Duplex transfer.       */

/* SPI clock phase options for SPI mode. */

#define     SPI_CPHA_0                  0
#define     SPI_CPHA_1                  1

/* SPI clock polarity options for SPI mode. */

#define     SPI_CPOL_0                  0
#define     SPI_CPOL_1                  1

/* Bit order options. */

#define     SPI_MSB_FIRST               0
#define     SPI_LSB_FIRST               1

/* SPI slave-select polarity option. */

#define     SPI_SSPOL_LOW               0
#define     SPI_SSPOL_HIGH              1

/* SPI driver mode type definition. */
typedef     UINT8                       SPI_DRIVER_MODE;

/* Nucleus SPI device handle definition. */
typedef INT SPI_HANDLE;

/* Nucleus SPI device driver modes. */

#define     SPI_INTERRUPT_MODE          0x1
#define     SPI_POLLING_MODE            0x2

/* Flag value that specifies whether user buffers should be used or not
   while using interrupt driven mode. This flag is ignored in polling
   mode. */
#define     SPI_USER_BUFFERING          0x4 /* Third bit of driver
                                               mode is used as a flag
                                               for user buffer option.  */

/* Structure for callback routines to be implemented by the user. */
typedef struct SPI_APP_CALLBACKS_STRUCT
{
    /* Callback function which will indicate the completion of a
       transfer. */
    VOID            (*spi_transfer_complete)(DV_DEV_ID spi_dev_id,
                                             UINT16 spi_address,
                                             VOID *tx_data,
                                             VOID *rx_buffer,
                                             UNSIGNED_INT length,
                                             UINT8 element_size);

    /* Callback function which will indicate an error. */
    VOID            (*spi_error)            (STATUS error_code,
                                             DV_DEV_ID spi_dev_id);

    /* Address of the slave with which these callback functions
       are associated. */
    UINT16      spi_address;

    /* Padding to align the structure. */
    UINT8       spi_padding[2];

} SPI_APP_CALLBACKS;

/* Nucleus SPI initialization structure. */
typedef struct SPI_INIT_STRUCT
{
    /* Callbacks */
    SPI_APP_CALLBACKS   spi_callbacks;      /* User callbacks.          */
    
    /* Transfer Attributes */
    UINT32              spi_baud_rate;      /* Initial baud rate. This
                                               parameter is ignored for
                                               SPI slave devices.       */
    UINT8               spi_address;        /* Chip select address */
    UINT8               spi_clock_phase;    /* Initial clock phase.     */
    UINT8               spi_clock_polarity; /* Initial clock polarity.  */
    UINT8               spi_bit_order;      /* Initial bit order.       */
    UINT8               spi_transfer_size;  /* Initial transfer size.   */
    /* UINT8               spi_ss_polarity; */    /* Slave select polarity.   */

    /* Label of the SPI controller on SoC this slave device is connected to */
    DV_DEV_LABEL        spi_controller_label; /* SPI controller label    */

} SPI_INIT;


/* Nucleus SPI transfer attributes configuration structure. */
typedef struct SPI_TRANSFER_CONFIG_STRUCT
{
    UINT32      spi_baud_rate;              /* Baud rate.               */
    UINT16      spi_address;                /* Address of the slave
                                               with which this transfer
                                               attribute configuration is
                                               associated.              */
    UINT8       spi_clock_phase;            /* Clock phase.             */
    UINT8       spi_clock_polarity;         /* Clock polarity.          */
    UINT8       spi_bit_order;              /* Bit order.               */
    UINT8       spi_transfer_size;          /* Transfer size.           */
    UINT8       spi_ss_polarity;            /* Slave select polarity.   */
    BOOLEAN     handle_chipselect;          /* Boolean flag to indicate 
                                               whether the driver should 
                                               handle chipselect or not. */

} SPI_TRANSFER_CONFIG;

/* Error constants for Nucleus SPI.
   The range of errors is specified like this.
   1-10         OS specific errors
   11-40        Nucleus SPI software specific errors
   41-50        Nucleus SPI queue management errors
   51-100       Reserved
   101-150      Hardware specific errors
   Remaining    Open for application.   */

/* OS specific errors. */

#define     SPI_OS_ERROR                1   /* Any OS error occurred.   */
#define     SPI_NULL_GIVEN_FOR_MEM_POOL 2   /* Memory pool pointer was
                                               not set properly.        */
#define     SPI_NULL_GIVEN_FOR_INIT     3   /* Initialization pointer is
                                               null.                    */
#define     SPI_REGISTRY_ERROR          4   /* SPI Registry Error */
#define     SPI_DEVICE_ERROR            5   /* SPI Device error */

/* Nucleus SPI software specific errors. */

#define     SPI_INVALID_PORT_ID         11  /* Invalid SPI driver port. */
#define     SPI_INVALID_DEVICE_ID       12  /* Invalid device ID.       */
#define     SPI_INVALID_HANDLE          13  /* Nucleus SPI device handle
                                               is not valid. */
#define     SPI_INVALID_HANDLE_POINTER  14  /* Specified pointer is not
                                               valid.                   */
#define     SPI_SHUTDOWN_ERROR          15  /* SPI controller could not
                                               be closed.               */
#define     SPI_DEV_ALREADY_INIT        16  /* Nucleus SPI device is
                                               already initialized.     */
#define     SPI_DRIVER_REGISTER_FAILED  17  /* Hardware driver registration
                                               with Nucleus SPI failed. */
#define     SPI_DRIVER_INIT_FAILED      18  /* The device initialization
                                               failed.                  */
#define     SPI_INVALID_DRIVER_MODE     19  /* Driver mode is not set to
                                               either interrupt driven
                                               or polling mode.         */
#define     SPI_DRIVER_MODE_UNAVAILABLE 20  /* Specified driver mode is
                                               not available in current
                                               configuration.           */
#define     SPI_INVALID_VECTOR          21  /* Invalid vector ID.       */
#define     SPI_INVALID_PARAM_POINTER   22  /* Null given instead of a
                                               variable pointer.        */
#define     SPI_INVALID_TRANSFER_LENGTH 23  /* Transfer length is zero. */
#define     SPI_INVALID_ADDRESS         24  /* Invalid slave address.   */
#define     SPI_INVALID_MODE            25  /* Specified SPI mode is not
                                               valid.                   */
#define     SPI_INVALID_BIT_ORDER       26  /* Specified bit order is
                                               not valid.               */
#define     SPI_ELEMENT_SIZE_NOT_ENOUGH 27  /* The buffer elements cannot
                                               hold data units of current
                                               transfer size.           */
#define     SPI_INVALID_IOCTL_OPERATION 28  /* Unsupported operation
                                               requested for I/O
                                               control.                 */
#define     SPI_DEVICE_IN_USE           29  /* Nucleus SPI device is
                                               being used by some other
                                               user.                    */
#define     SPI_INVALID_SS_POLARITY     30  /* Specified slave select
                                               polarity is not valid.   */

/* Error constants for Nucleus SPI queue management. */

#define     SPI_QUEUE_FULL              41  /* Messages Queue is full.  */
#define     SPI_BUFFER_NOT_ENOUGH       42  /* Transfer request is too
                                               big to fit in the available
                                               space in buffer.         */
#define     SPI_INVALID_QUEUE_SIZE      43  /* Queue size is zero.      */
#define     SPI_INVALID_BUFFER_SIZE     44  /* Buffer size is zero.     */
#define     SPI_INVALID_CHIPSELECT      45  /* Chipselect flag is not 
                                               valid.                   */

/* Nucleus SPI controller specific error constants.
   Refer to the respective hardware manual for
   detailed technical description of the errors. */

/* An undocumented error occurred in SPI driver. */
#define     SPI_GENERAL_HARDWARE_ERROR      101

/* Specified SPI device cannot function as an SPI master. */
#define     SPI_MASTER_MODE_NOT_SUPPORTED   102

/* Specified SPI device cannot function as an SPI slave. */
#define     SPI_SLAVE_MODE_NOT_SUPPORTED    103

/* Rx overrun in SPI hardware. */
#define     SPI_RX_OVERRUN                  104

/* Tx underflow occurred in the SPI slave. */
#define     SPI_TX_UNDERFLOW                105

/* Specified bit order is not supported. */
#define     SPI_UNSUPPORTED_BIT_ORDER       106

/* Specified baud rate is not supported. */
#define     SPI_UNSUPPORTED_BAUD_RATE       107

/* Specified SPI mode is not supported. */
#define     SPI_UNSUPPORTED_MODE            108

/* Specified transfer size is not supported. */
#define     SPI_UNSUPPORTED_TRANSFER_SIZE   109

/* Specified slave-select polarity is not supported. */
#define     SPI_UNSUPPORTED_SS_POLARITY     110

/***********************************************************************
 ***************    API declarations and mapping   *********************
 ***********************************************************************/

#ifndef     NU_SPI_SOURCE_FILE

/* Define a dummy slave address to be used with internal services for SPI
   slave devices. */
#define     SPI_DUMMY_SLAVE_ADDR            0

/* Service call mapping for common Nucleus SPI routines. */

#define     NU_SPI_Start                    SPI_Start
#define     NU_SPI_Close                    SPI_Close

/* Service call mapping for Nucleus SPI Master services. */

#define     NU_SPI_Master_Transmit_8Bit(a,b,c,d)     SPICC_Transfer(a,b,c,NU_NULL,d,sizeof(UINT8),SPI_TX)
#define     NU_SPI_Master_Transmit_16Bit(a,b,c,d)    SPICC_Transfer(a,b,c,NU_NULL,d,sizeof(UINT16),SPI_TX)
#define     NU_SPI_Master_Transmit_32Bit(a,b,c,d)    SPICC_Transfer(a,b,c,NU_NULL,d,sizeof(UINT32),SPI_TX)
#define     NU_SPI_Master_Receive_8Bit(a,b,c,d)      SPICC_Transfer(a,b,NU_NULL,c,d,sizeof(UINT8),SPI_RX)
#define     NU_SPI_Master_Receive_16Bit(a,b,c,d)     SPICC_Transfer(a,b,NU_NULL,c,d,sizeof(UINT16),SPI_RX)
#define     NU_SPI_Master_Receive_32Bit(a,b,c,d)     SPICC_Transfer(a,b,NU_NULL,c,d,sizeof(UINT32),SPI_RX)
#define     NU_SPI_Master_Duplex_8Bit(a,b,c,d,e)     SPICC_Transfer(a,b,c,d,e,sizeof(UINT8),SPI_DUPLEX)
#define     NU_SPI_Master_Duplex_16Bit(a,b,c,d,e)    SPICC_Transfer(a,b,c,d,e,sizeof(UINT16),SPI_DUPLEX)
#define     NU_SPI_Master_Duplex_32Bit(a,b,c,d,e)    SPICC_Transfer(a,b,c,d,e,sizeof(UINT32),SPI_DUPLEX)
#define     NU_SPI_Master_Set_Baud_Rate     SPICC_Set_Baud_Rate
#define     NU_SPI_Master_Set_SPI_Mode      SPICC_Set_SPI_Mode
#define     NU_SPI_Master_Set_Bit_Order     SPICC_Set_Bit_Order
#define     NU_SPI_Master_Set_Transfer_Size SPICC_Set_Transfer_Size
#define     NU_SPI_Master_Set_Configuration SPICC_Set_Configuration
#define     NU_SPI_Master_Get_Configuration SPICC_Get_Configuration
#define     NU_SPI_Master_Set_Callbacks     SPICC_Set_Callbacks
#define     NU_SPI_Master_Get_Callbacks     SPICC_Get_Callbacks
#define     NU_SPI_Master_Get_Driver_Mode   SPICC_Get_Driver_Mode
#define     NU_SPI_Master_Set_SS_Polarity   SPICC_Set_SS_Polarity
#define     NU_SPI_Master_Handle_Chipselect SPICC_Handle_Chipselect

/* Service call mapping for I/O control service. */
#define     NU_SPI_Ioctl_Driver             SPICC_Ioctl_Driver

/* Nucleus SPI common (master/slave) API. */

/* SPI Middleware init function */
STATUS  nu_os_conn_spi_init(const CHAR * key, INT startorstop);

STATUS  NU_SPI_Start                    (SPI_HANDLE *spi_dev,
                                             SPI_INIT *spi_init);
STATUS  NU_SPI_Close                    (SPI_HANDLE spi_dev);

/* Service for I/O control. */
STATUS  NU_SPI_Ioctl_Driver             (SPI_HANDLE spi_dev,
                                             UINT16 address,
                                             INT control_code,
                                             VOID *control_info);

/* *********************  Nucleus SPI Master API.  ******************** */

/* Transfer attributes control services. */

STATUS  NU_SPI_Master_Set_Baud_Rate     (SPI_HANDLE spi_dev,
                                             UINT16 address,
                                             UINT32 baud_rate);
STATUS  NU_SPI_Master_Set_SPI_Mode      (SPI_HANDLE spi_dev,
                                             UINT16 address,
                                             UINT8 clock_polarity,
                                             UINT8 clock_phase);
STATUS  NU_SPI_Master_Set_Bit_Order     (SPI_HANDLE spi_dev,
                                             UINT16 address,
                                             UINT8 bit_order);
STATUS  NU_SPI_Master_Set_Transfer_Size (SPI_HANDLE spi_dev,
                                             UINT16 address,
                                             UINT8 transfer_size);
STATUS  NU_SPI_Master_Set_Configuration (SPI_HANDLE spi_dev,
                                             UINT16 address,
                                             SPI_TRANSFER_CONFIG *config);
STATUS  NU_SPI_Master_Get_Configuration (SPI_HANDLE spi_dev,
                                             UINT16 address,
                                             SPI_TRANSFER_CONFIG *config);
STATUS  NU_SPI_Master_Set_Callbacks     (SPI_HANDLE spi_dev,
                                             UINT16 address,
                                             SPI_APP_CALLBACKS *callbacks);
STATUS  NU_SPI_Master_Get_Callbacks     (SPI_HANDLE spi_dev,
                                             UINT16 address,
                                             SPI_APP_CALLBACKS *callbacks);
STATUS  NU_SPI_Master_Get_Driver_Mode   (SPI_HANDLE spi_dev,
                                             SPI_DRIVER_MODE *spi_driver_mode);
STATUS  NU_SPI_Master_Set_SS_Polarity   (SPI_HANDLE spi_dev,
                                             UINT16 address,
                                             UINT8 ss_polarity);
STATUS  NU_SPI_Master_Handle_Chipselect (SPI_HANDLE spi_dev, 
                                             UINT16 address,
                                             BOOLEAN handle_chipselect);

/* *********************  Nucleus SPI Slave API.  ********************* */

/* Transmission services. */

#define     NU_SPI_Slave_8Bit_Transmit_Response(spi_dev, tx_data, length)  \
            NU_SPI_Master_Transmit_8Bit(spi_dev, SPI_DUMMY_SLAVE_ADDR,     \
                                        tx_data, length)

#define     NU_SPI_Slave_16Bit_Transmit_Response(spi_dev, tx_data, length) \
            NU_SPI_Master_Transmit_16Bit(spi_dev, SPI_DUMMY_SLAVE_ADDR,    \
                                         tx_data, length)

#define     NU_SPI_Slave_32Bit_Transmit_Response(spi_dev, tx_data, length) \
            NU_SPI_Master_Transmit_32Bit(spi_dev, SPI_DUMMY_SLAVE_ADDR,    \
                                         tx_data, length)

/* Reception services. */

#define     NU_SPI_Slave_8Bit_Receive_Response(spi_dev, rx_buffer, length) \
            NU_SPI_Master_Receive_8Bit(spi_dev, SPI_DUMMY_SLAVE_ADDR,      \
                                       rx_buffer, length)

#define     NU_SPI_Slave_16Bit_Receive_Response(spi_dev, rx_buffer, length)\
            NU_SPI_Master_Receive_16Bit(spi_dev, SPI_DUMMY_SLAVE_ADDR,     \
                                        rx_buffer, length)

#define     NU_SPI_Slave_32Bit_Receive_Response(spi_dev, rx_buffer, length)\
            NU_SPI_Master_Receive_32Bit(spi_dev, SPI_DUMMY_SLAVE_ADDR,     \
                                        rx_buffer, length)

/* Duplex transfer services. */

#define     NU_SPI_Slave_8Bit_Duplex_Response(spi_dev, tx_data, rx_buffer, \
                                              length)                      \
            NU_SPI_Master_Duplex_8Bit(spi_dev, SPI_DUMMY_SLAVE_ADDR,       \
                                      tx_data, rx_buffer, length)

#define     NU_SPI_Slave_16Bit_Duplex_Response(spi_dev, tx_data, rx_buffer,\
                                               length)                     \
            NU_SPI_Master_Duplex_16Bit(spi_dev, SPI_DUMMY_SLAVE_ADDR,      \
                                       tx_data, rx_buffer, length)

#define     NU_SPI_Slave_32Bit_Duplex_Response(spi_dev, tx_data, rx_buffer,\
                                               length)                     \
            NU_SPI_Master_Duplex_32Bit(spi_dev, SPI_DUMMY_SLAVE_ADDR,      \
                                       tx_data, rx_buffer, length)

#define     NU_SPI_Slave_Set_SPI_Mode(spi_dev, clock_polarity, clock_phase)\
            NU_SPI_Master_Set_SPI_Mode(spi_dev, SPI_DUMMY_SLAVE_ADDR,      \
                                       clock_polarity, clock_phase)

/* Transfer attributes control services. */

#define     NU_SPI_Slave_Set_Bit_Order(spi_dev, bit_order)                 \
            NU_SPI_Master_Set_Bit_Order(spi_dev, SPI_DUMMY_SLAVE_ADDR,     \
                                        bit_order)

#define     NU_SPI_Slave_Set_Transfer_Size(spi_dev, transfer_size)         \
            NU_SPI_Master_Set_Transfer_Size(spi_dev, SPI_DUMMY_SLAVE_ADDR, \
                                            transfer_size)

#define     NU_SPI_Slave_Set_Configuration(spi_dev, config_ptr)            \
            NU_SPI_Master_Set_Configuration(spi_dev, SPI_DUMMY_SLAVE_ADDR, \
                                            config_ptr);

#define     NU_SPI_Slave_Get_Configuration(spi_dev, config_ptr)            \
            NU_SPI_Master_Get_Configuration(spi_dev, SPI_DUMMY_SLAVE_ADDR, \
                                            config_ptr);

#endif      /* !NU_SPI_SOURCE_FILE */

#ifdef      __cplusplus
}
#endif

#endif      /* !SPI_H_FILE */
