/*************************************************************************
*
*               Copyright 2006 Mentor Graphics Corporation
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
*       spic_defs.h
*
* COMPONENT
*
*       SPIC - Nucleus SPI Core Services
*
* DESCRIPTION
*
*       This file contains data structures and constants for
*       Nucleus SPI Core Services component.
*
* DATA STRUCTURES
*
*       SPI_DRV_SERVICES                    Structure for underlying
*                                           SPI hardware driver services.
*
*       SPI_CB                              Nucleus SPI device control
*                                           block structure.
*
*       SPI_DRV_IOCTL_DATA                  Ioctl data structure - 1
*
*       SPI_DRV_IOCTL_WR_DATA               Ioctl data structure - 2
*
* DEPENDENCIES
*
*       spi.h                               Nucleus SPI API, definitions
*                                           and constants.
*
*       spiq_defs.h                         Nucleus SPI Queue Management
*                                           component definitions file.
*
*************************************************************************/
/* Check to see if the file has been included already. */
#ifndef     SPIC_DEFS_H
#define     SPIC_DEFS_H

#include    "connectivity/spi.h"
#include    "connectivity/spiq_defs.h"

/* Dummy data to drive reception. */
#define     SPI_DUMMY_DATA              0xFFFFFFFFUL

/* Flags to specify different SPI transfer attributes for control. */

#define     SPI_SET_BAUD_RATE           1   /* Set baud rate.           */
#define     SPI_SET_MODE                2   /* Set SPI mode.            */
#define     SPI_SET_BIT_ORDER           3   /* Set bit order.           */
#define     SPI_SET_TRANSFER_SIZE       4   /* Set transfer size.       */
#define     SPI_SETUP_SLAVE_ADDRS       5   /* Setup available slave
                                               addresses in the transfer
                                               attributes list.         */
#define     SPI_APPLY_TRANSFER_ATTRIBS  6   /* Apply the specified
                                               transfer attributes.     */
#define     SPI_SET_SS_POLARITY         7   /* Set the slave-select
                                               polarity.                */
#define     SPI_DRV_READ                8
#define     SPI_DRV_WRITE               9
#define     SPI_SET_CB                  10
#define     SPI_GET_SLAVE_CNT           11
#define     SPI_GET_MASTER_SLAVE_MODE   12
#define     SPI_GET_INTR_POLL_MODE      13
#define     SPI_GET_MW_CONFIG_PATH      14
#define     SPI_ENABLE_DEVICE           15
#define     SPI_HANDLE_CHIPSELECT       16
#define     SPI_GET_SIM_TX_ATTRIBS      17

#if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE)

#define     SPI_PWR_HIB_RESTORE         18

/* Ioctl command delimiter */
#define     SPI_CLASS_CMD_DELIMITER     19

#else
/* Ioctl command delimiter */
#define     SPI_CLASS_CMD_DELIMITER     18

#endif /* #if (CFG_NU_OS_SVCS_PWR_CORE_ENABLE_HIBERNATE == NU_TRUE) */

/* Nucleus SPI device control block structure. */
typedef struct SPI_CB_STRUCT
{
    SPI_QUEUE          spi_queue;           /* Nucleus SPI queue.       */
    SPI_APP_CALLBACKS  *spi_ucb;            /* List of user callbacks.  */
    VOID               *spi_reserved;       /* Reserved field.          */
    SPI_TRANSFER_CONFIG
                       *spi_slaves_attribs; /* List of transfer
                                               attributes. For a slave
                                               device this list contains
                                               a single entry whereas
                                               for a master device this
                                               contains transfer
                                               attributes for all
                                               slaves.                  */
    UINT8              *spi_current_tx_data;/* Tx buffer pointer of
                                               currently active
                                               transfer.                */
    UINT8              *spi_current_rx_buffer;
                                            /* Rx buffer pointer for
                                               currently active
                                               transfer.                */
    UNSIGNED_INT        spi_current_length; /* Length of currently
                                               active transfer.         */
    UNSIGNED_INT        spi_current_count;  /* Number of remaining data
                                               units to be transferred
                                               during current transfer. */
    INT                 spi_vector;         /* SPI vector number.       */
    UINT16              spi_notify_pending; /* Count of notification
                                               requests pending for
                                               servicing by Nucleus SPI
                                               notification handler.    */
    UINT16              spi_slaves_count;   /* Number of slaves if the
                                               device is a master.      */
    DV_DEV_ID           spi_dev_id;         /* SPI device ID.           */
    UINT8               spi_dev_user_count; /* SPI device user count.   */
    BOOLEAN             spi_transfer_active;/* Flag to indicate that an
                                               SPI transfer is currently
                                               active.                  */
    BOOLEAN             spi_transfer_started;
                                            /* Flag to indicate that a
                                               new transfer is to be
                                               started.                 */
                                               
    INT                 spi_master_mode;    /* Flag to indicate whether
                                               this SPI device is running
                                               as a master.             */
    INT                 spi_driver_mode;    /* Specifies whether Nucleus
                                               SPI will handle this
                                               device in polling,
                                               interrupt driven or
                                               interrupt driven with user
                                               buffering mode .         */
    BOOLEAN             spi_sim_tx_attribs; /* Flag to indicate if SPI 
                                               controller would require
                                               Transfer attribute 
                                               simulation.              */

    /* Middleware config from Registry */
    UINT16              spi_queue_size;
    UINT32              spi_buffer_size;
                                               
    /* DM integration */
    DV_DEV_HANDLE spi_dev_handle;
    INT spi_ioctl_base;
    INT init_flag;
    NU_MEMORY_POOL  *spi_memory_pool;
    CHAR reg_config_path[REG_MAX_KEY_LENGTH];

    /* Padding to align the structure. */
    UINT8               spi_padding[1];

} SPI_CB;


/* SPI Ioctl data */
typedef struct _spi_drv_ioctl_data_struct
{
    
    UINT16 address;                         /* slave device address */
    SPI_TRANSFER_CONFIG * xfer_attrs;       /* slave transfer attributes */
    
} SPI_DRV_IOCTL_DATA;


/* SPI Wr Ioctl data */
typedef struct _spi_drv_ioctl_wr_data_struct
{
    
    UINT16  address;                        /* slave device address */
    UINT32  wr_data_val;                    /* data to be written to slave device */
    BOOLEAN thread_context;                 /* 1 indicates thread context, 0 indicates isr context */
    
} SPI_DRV_IOCTL_WR_DATA;

#endif      /* !SPIC_DEFS_H */
