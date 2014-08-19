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
*       spic_extr.h
*
* COMPONENT
*
*       SPIC - Nucleus SPI Core Services
*
* DESCRIPTION
*
*       This file contains functions prototypes for the routines of
*       Nucleus SPI Core Services component.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       spic_defs.h                         Nucleus SPI Core Services
*                                           component definitions file.
*
*
*************************************************************************/
/* Check to see if the file has been included already. */
#ifndef     SPIC_EXTR_H
#define     SPIC_EXTR_H

#include    "connectivity/spic_defs.h"

/* Functions for SPI transfer attributes control. */

STATUS  SPICC_Set_Baud_Rate         (SPI_HANDLE spi_dev,
                                         UINT16 address,
                                         UINT32 baud_rate);
STATUS  SPICC_Set_SPI_Mode          (SPI_HANDLE spi_dev,
                                         UINT16 address,
                                         UINT8 clock_polarity,
                                         UINT8 clock_phase);
STATUS  SPICC_Set_Bit_Order         (SPI_HANDLE spi_dev,
                                         UINT16 address,
                                         UINT8 bit_order);
STATUS  SPICC_Set_Transfer_Size     (SPI_HANDLE spi_dev,
                                         UINT16 address,
                                         UINT8 transfer_size);
STATUS  SPICC_Set_Configuration     (SPI_HANDLE spi_dev,
                                         UINT16 address,
                                         SPI_TRANSFER_CONFIG *config);
STATUS  SPICC_Get_Configuration     (SPI_HANDLE spi_dev,
                                         UINT16 address,
                                         SPI_TRANSFER_CONFIG *config);
STATUS  SPICC_Handle_Chipselect     (SPI_HANDLE spi_dev, 
                                         UINT16 address,
                                         BOOLEAN handle_chipselect);

STATUS  SPICC_Transfer              (SPI_HANDLE    spi_dev,
                                          UINT16        address,
                                          VOID        *tx_data,
                                          VOID        *rx_buffer,
                                          UNSIGNED_INT  length,
                                          UINT8         element_size,
                                          UINT8         transfer_type);

/* Functions to process transfer requests. */

STATUS  SPICC_Process_Request       (SPI_HANDLE spi_dev,
                                         SPI_REQUEST *request);
STATUS  SPICC_Transfer_Interrupt    (SPI_CB *spi_cb,
                                         SPI_REQUEST *request);
STATUS  SPICC_Transfer_Polled       (SPI_CB *spi_cb,
                                         SPI_REQUEST *request);

/* Function to provide interface to driver I/O control service. */
STATUS  SPICC_Ioctl_Driver          (SPI_HANDLE spi_dev,
                                         UINT16 address,
                                         INT control_code,
                                         VOID *control_info);

/* Support functions. */

SPI_TRANSFER_CONFIG *
            SPICS_Get_Config_Struct     (SPI_CB *spi_cb,
                                         UINT16 address);
SPI_APP_CALLBACKS *
            SPICS_Get_Callbacks_Struct  (SPI_CB *spi_cb,
                                         UINT16 address);
STATUS      SPICS_Check_Address         (SPI_CB *spi_cb,
                                         UINT16 address);
STATUS      SPICS_Get_CBs               (SPI_HANDLE spi_dev,
                                         SPI_CB **spi_cb_ptr);
INT         SPICS_Get_CB_Index          (DV_DEV_ID dev_id);
VOID        SPICS_ISR_Action            (SPI_CB *spi_cb);
VOID        SPICS_Write_Action          (SPI_CB *spi_cb,
                                         UINT16 address);

#if         (NU_SPI_ERROR_CHECKING)

STATUS  SPICS_Check_Init_Params(SPI_INIT *spi_init, SPI_CB *spi_cb);

#else

#define     SPICS_Check_Init_Params(x,y)   NU_SUCCESS

#endif      /* (NU_SPI_ERROR_CHECKING) */

#endif      /* !SPIC_EXTR_H */
