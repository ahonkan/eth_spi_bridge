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
*       spiq_extr.h
*
* COMPONENT
*
*       SPIQ - Nucleus SPI Queue Management
*
* DESCRIPTION
*
*       This file contains function prototypes for Nucleus SPI Queue
*       Management component.
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
*       spiq_defs.h                         Nucleus SPI Queue Management
*                                           component definitions file.
*
*************************************************************************/
/* Check to see if the file has been included already. */
#ifndef     SPIQ_EXTR_H
#define     SPIQ_EXTR_H

#include    "connectivity/spic_defs.h"
#include    "connectivity/spiq_defs.h"

/* Core routines for queue management. */

STATUS      SPIQC_Put_Queue_Tx          (SPI_CB *spi_cb,
                                         SPI_REQUEST *request,
                                         BOOLEAN thread_context);
VOID        SPIQC_Get_Queue_Tx          (SPI_CB *spi_cb,
                                         UINT32 *data,
                                         BOOLEAN thread_context);
VOID        SPIQC_Put_Queue_Rx          (SPI_CB *spi_cb, UINT32 data);

/* Support functions for queue management. */

STATUS      SPIQS_Put_In_Buffer         (SPI_QUEUE *queue,
                                         SPI_REQUEST *request);
VOID        SPIQS_Update_Buffer_State   (SPI_CB *spi_cb);
VOID        SPIQS_Copy_To_Buffer        (UINT8 *buffer,
                                         SPI_REQUEST *request);

#endif      /* !SPIQ_EXTR_H */
