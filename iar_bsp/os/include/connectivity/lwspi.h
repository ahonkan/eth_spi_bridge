/*************************************************************************
*
*                  Copyright 2012 Mentor Graphics Corporation
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
*       lwspi.h
*
* COMPONENT
*
*       Nucleus lightweight SPI
*
* DESCRIPTION
*
*       This file contains data structures and APIs provided with
*       Nucleus lightweitht SPI.
*
* DATA STRUCTURES
*
*       NU_SPI_BUS                          SPI bus control block.
*       NU_SPI_DEVICE                       SPI slave control block.
*       NU_SPI_IRP                          I/O request packet.
*       NU_SPI_HANDLE                       SPI slave handle.
*
* DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*
*************************************************************************/
#ifndef _LWSPI_H
#ifdef  __cplusplus
extern  "C" {                               /* C declarations in C++. */
#endif

#define _LWSPI_H

#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "drivers/dma.h"

/* LWSPI GUID */
#define LWSPI_LABEL {0xe0,0x76,0xe3,0x6f,0xd9,0xad,0x43,0x85,0xbc,0x5b,0xe3,0x8b,0xe6,0x99,0xfc,0x20}

#define NU_SPI_BUS_NAME_LEN         8

/* Definitions for number of SPI buses and slaves on each bus and 
 * other configurations.
 */
#define LWSPI_NUM_BUSES             CFG_NU_OS_CONN_LWSPI_NUM_SPI_BUSES
#define LWSPI_NUM_DEVICES           CFG_NU_OS_CONN_LWSPI_NUM_SPI_SLAVES
#define LWSPI_ERR_CHECK_ENABLE      CFG_NU_OS_CONN_LWSPI_ERR_CHECK_ENABLE
#define LWSPI_INT_MODE_IO_ENABLE    CFG_NU_OS_CONN_LWSPI_INT_MODE_IO_ENABLE

#define LWSPI_NUM_MASTERS           1

        /* LWSPI specific error codes. */
#define NU_SPI_INVLD_ARG                        -4000
#define NU_SPI_NO_FREE_BUS_SLOT                 -4001
#define NU_SPI_NO_FREE_DEVICE_SLOT              -4002
#define NU_SPI_BUS_SLOT_NOT_FOUND               -4003
#define NU_SPI_INVLD_IO_PTR                     -4004
#define NU_SPI_BUSY                             -4005
#define NU_SPI_TIMEOUT                          -4006
#define NU_SPI_SEM_ERROR                        -4007
#define NU_SPI_INVALID_HANDLE                   -4008

/* ah terabit radios
 * Reconfigured SPI word to be more generic.  
 * Low level TGT routines will decode and set parameters
 * accordingly
 */        
/* SPI configuration word */        

/* Definitions for Data types. */
#define SPI_CFG_8Bit              8
#define SPI_CFG_16Bit             16


/* Definitions for slave select polarity. */
#define SPI_CFG_SS_POL_LO                   (1UL << 0)
#define SPI_CFG_SS_POL_HI                   (1UL << 1)
#define SPI_CFG_SS_POL_SW_CONTROL           (SPI_CFG_SS_POL_LO | SPI_CFG_SS_POL_HI)

/* Definitions for bit order. */
#define SPI_CFG_BO_LSB_FIRST                (1UL << 2)
#define SPI_CFG_BO_MSB_FIRST                (1UL << 3)
#define SPI_CFG_BO_ERROR                    (SPI_CFG_BO_LSB_FIRST | SPI_CFG_BO_MSB_FIRST)

/* Defines for SPI_CFG Clock Polarity */
#define SPI_CFG_MODE_POL_LO                 (1UL << 4)
#define SPI_CFG_MODE_POL_HI                 (1UL << 5)
#define SPI_CFG_MODE_POL_ERROR              (SPI_CFG_MODE_POL_LO | SPI_CFG_MODE_POL_HI)

/* Defines for SPI_CFG Clock Phase */
#define SPI_CFG_MODE_PHA_FIRST_EDGE         (1UL << 6)
#define SPI_CFG_MODE_PHA_SECOND_EDGE        (1UL << 7)
#define SPI_CFG_MODE_PHA_ERROR              (SPI_CFG_MODE_PHA_FIRST_EDGE | SPI_CFG_MODE_PHA_SECOND_EDGE)

/* Defines for SPI_CFG Master/Slave */
#define SPI_CFG_DEV_MASTER                  (1UL << 8)
#define SPI_CFG_DEV_SLAVE                   (1UL << 9)
#define SPI_CFG_DEV_ERROR                   (SPI_CFG_DEV_MASTER | SPI_CFG_DEV_SLAVE)

/* Defines for SPI_CFG Protocol- TI mode, SPI_CFG mode */
#define SPI_CFG_PROT_TI                     (1UL << 10)
#define SPI_CFG_PROT_MOT                    (1UL << 11)
#define SPI_CFG_PROT_ERROR                  (SPI_CFG_PROT_TI | SPI_CFG_PROT_MOT)

/* Definitions for I/O types. */
#define SPI_POLLED_IO                           0
#define SPI_INTERRUPT_IO                        NU_SUSPEND


/* Lightweight SPI IOCTLs. */
#define LWSPI_IOCTL_GET_BUS_INFO                0
#define LWSPI_IOCTL_PREP_DMA                    1
#define LWSPI_NUM_IOCTLS                        2

typedef struct _nu_spi_bus          NU_SPI_BUS;
typedef struct _nu_spi_device       NU_SPI_DEVICE;
typedef struct _nu_spi_irp          NU_SPI_IRP;
typedef struct _nu_spi_io_ptrs      NU_SPI_IO_PTRS;
typedef UINT32                      NU_SPI_HANDLE;
typedef VOID (*SPI_SS_CALLBACK)(BOOLEAN);

struct _nu_spi_irp
{
    NU_SPI_DEVICE       *device;
    VOID                *buffer;
    UINT32              length;
    UINT32              actual_length;
};

struct _nu_spi_device
{
    NU_SPI_IRP          tx_irp;
    NU_SPI_IRP          rx_irp;
    NU_SEMAPHORE        async_io_lock;
    UINT32              baud_rate;
    UINT32              transfer_size;
    NU_SPI_HANDLE       handle;
    NU_SPI_BUS          *bus;
    UINT32              spi_config;
    UINT8               index;
    UINT8               ss_index;

#ifdef  CFG_NU_OS_DRVR_DMA_ENABLE
    /* DMA Transfer vars */
    DMA_REQ             dma_tx_req;
    DMA_REQ             dma_rx_req;
#endif

};

struct _nu_spi_io_ptrs
{
    VOID (*configure)(VOID*, NU_SPI_DEVICE*);
    VOID (*check_power_on)(VOID*);
    BOOLEAN (*isr_read)(VOID*, NU_SPI_IRP*);
    BOOLEAN (*isr_write)(VOID*, NU_SPI_IRP*);
    BOOLEAN (*isr_write_read)(VOID*, NU_SPI_IRP*, NU_SPI_IRP*);
    STATUS (*read)(VOID*, BOOLEAN, NU_SPI_IRP*);
    STATUS (*write)(VOID*, BOOLEAN, NU_SPI_IRP*);
    STATUS (*write_read)(VOID*, BOOLEAN, NU_SPI_IRP*, NU_SPI_IRP*);
};
  
struct _nu_spi_bus
{
    NU_SEMAPHORE        bus_lock;
    NU_SPI_IO_PTRS      io_ptrs;
    CHAR                name[NU_SPI_BUS_NAME_LEN + 1];
    NU_SPI_DEVICE       *spi_devices[LWSPI_NUM_DEVICES];
#if (LWSPI_NUM_DEVICES > 1)
    NU_SPI_DEVICE       *current_device;
#endif
    VOID                *dev_context;
    DV_DEV_HANDLE       dev_handle;
    DV_DEV_ID           dev_id;
    INT                 ioctl_base;
    BOOLEAN             is_used;
    UINT8               index;

#ifdef  CFG_NU_OS_DRVR_DMA_ENABLE
    DMA_DEVICE_HANDLE       dma_tx_handle;
    DMA_CHAN_HANDLE         chan_tx_handle;
    DMA_DEVICE_HANDLE       dma_rx_handle;
    DMA_CHAN_HANDLE         chan_rx_handle;
#endif

    UINT8               pad[2];
};


STATUS NU_SPI_Register(CHAR*, UINT32, UINT32 , UINT32, NU_SPI_HANDLE*);
STATUS NU_SPI_Unregister(NU_SPI_HANDLE);
STATUS NU_SPI_Read(NU_SPI_HANDLE, UNSIGNED, VOID*, UINT32);
STATUS NU_SPI_Write(NU_SPI_HANDLE, UNSIGNED, VOID*, UINT32);
STATUS NU_SPI_Write_Read(NU_SPI_HANDLE, UNSIGNED, VOID*, VOID*, UINT32);
STATUS NU_SPI_Check_Complete(NU_SPI_HANDLE, UINT32);

#ifdef  CFG_NU_OS_DRVR_DMA_ENABLE

STATUS NU_SPI_DMA_Setup(NU_SPI_HANDLE   handle);

STATUS NU_SPI_DMA_Transfer(NU_SPI_HANDLE handle,
                           VOID* dma_tx_data_ptr,
                           VOID* dma_rx_data_ptr,
                           UINT16 data_len);

#endif

#if (CFG_NU_OS_CONN_LWSPI_EXTENDED_API_ENABLE == NU_TRUE)
STATUS NU_SPI_Set_Slave_Select_Index(NU_SPI_HANDLE, UINT8);
#endif /* (CFG_NU_OS_CONN_LWSPI_EXTENDED_API_ENABLE == NU_TRUE) */

#ifdef      __cplusplus
}
#endif
#endif  /* #ifndef _LWSPI_H */
