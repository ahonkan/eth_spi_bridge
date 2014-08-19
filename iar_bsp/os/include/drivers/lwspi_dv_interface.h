/**************************************************************************
*            Copyright 2012 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
* FILE NAME
*
*       lwspi_dv_interface.h
*
* COMPONENT
*
*       Lightweight SPI, generic driver
*
* DESCRIPTION
*
*       This file contains the function prototypes and data structures
*       for lightweight SPI generic driver driver.
*
* DATA STRUCTURES
*
*       LWSPI_INSTANCE_HANDLE
*       LWSPI_SESSION_HANDLE
*
**************************************************************************/
#ifndef     LWSPI_DV_INTERFACE_H
#define     LWSPI_DV_INTERFACE_H

#include "connectivity/nu_connectivity.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* _cplusplus */

typedef struct  _lwspi_dma_intf
{
    UINT8           dma_enable;

    UINT8           tx_dma_dev_id;    /* DMA device ID as mentioned in the platform file. */
    UINT8           rx_dma_dev_id;    /* DMA device ID as mentioned in the platform file. */
    UINT8           tx_dma_peri_id;  
    UINT8           rx_dma_peri_id;  

} LWSPI_DMA_INTF;

/* LWSPI instance structure. */
typedef struct  _lwspi_instance_handle
{
    DV_DEV_ID               dev_id;
#if (CFG_NU_OS_CONN_LWSPI_INT_MODE_IO_ENABLE == NU_TRUE)
    NU_SPI_IRP              *curr_read_irp;
    NU_SPI_IRP              *curr_write_irp;
#endif
    VOID                    *spi_tgt_ptr;
    CHAR                    reg_path[REG_MAX_KEY_LENGTH];
    UINT32                  io_addr;
    UINT32                  clock;
    INT                     intr_vector;
    INT                     intr_priority;
    ESAL_GE_INT_TRIG_TYPE   intr_type;
    BOOLEAN                 device_in_use;
#if (CFG_NU_OS_CONN_LWSPI_INT_MODE_IO_ENABLE == NU_TRUE)
    UINT8                   io_operation;
    UINT8                   pad[2];
#else
    UINT8                   pad[3];
#endif
    CHAR                    ref_clock[NU_DRVR_REF_CLOCK_LEN];
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    PMI_DEV_HANDLE          pmi_dev;
#endif

    LWSPI_DMA_INTF          dma_intf;

}LWSPI_INSTANCE_HANDLE;

/* LWSPI session structure. */
typedef struct  _spi_drv_session_handle
{
    LWSPI_INSTANCE_HANDLE   *inst_ptr;
    UINT32                  open_mode;
}LWSPI_SESSION_HANDLE;

/* Power States */
#define SPI_OFF                     0
#define SPI_ON                      1

/* Lightweight SPI power states. */
#define LWSPI_POWER_STATE_COUNT     2

/* Open Modes */
#define LWSPI_OPEN_MODE             0x1

/* Definitions for I/O operations. */
#define LWSPI_IO_OPERATION_READ         1
#define LWSPI_IO_OPERATION_WRITE        2
#define LWSPI_IO_OPERATION_WRITE_READ   3

/* Lightweight SPI Driver Error codes */
#define SPI_DEV_IN_USE              -1
#define SPI_DEV_DEFAULT_ERROR       -2
#define SPI_DEV_INVLD_REG_PATH      -3

/* Base IOCTL ID for SPI and power modes. */
#define LWSPI_SPI_MODE_IOCTL_BASE   (DV_IOCTL0+1)
#define LWSPI_POWER_MODE_IOCTL_BASE (LWSPI_SPI_MODE_IOCTL_BASE + LWSPI_NUM_IOCTLS)

/*********************************/
/* FUNCTION PROTOTYPES           */
/*********************************/
STATUS  LWSPI_Dv_Register (const CHAR*, LWSPI_INSTANCE_HANDLE*);
STATUS  LWSPI_Dv_Unregister (const CHAR*, INT, DV_DEV_ID);
STATUS  LWSPI_Dv_Open (VOID*, DV_DEV_LABEL[], INT, VOID**);
STATUS  LWSPI_Dv_Close(VOID*);
STATUS  LWSPI_Dv_Ioctl (VOID*, INT, VOID*, INT);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif      /* !LWSPI_DV_INTERFACE_H */
