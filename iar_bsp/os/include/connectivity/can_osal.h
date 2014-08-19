/****************************************************************************
*
*                  Copyright 2002 Mentor Graphics Corporation
*                             All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
****************************************************************************/

/**************************************************************************
*
* FILE NAME
*
*       can_osal.h
*
* COMPONENT
*
*       CAN OSAL - OS Abstraction Layer for Nucleus CAN
*
* DESCRIPTION
*
*       This file contains constant definitions and structure declarations
*       to provide OS abstraction for various services of Nucleus CAN.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       can_cfg.h                           Nucleus CAN configuration
*                                           file.
*
*       <os>.h                              Main definition and API file
*                                           for the underlying OS. <os>
*                                           stands for either nucleus or
*                                           osek.
*
*************************************************************************/
/* Check to see if the file has already been included. */
#ifndef     CAN_OSAL_H
#define     CAN_OSAL_H

#include    "can_cfg.h"

/* Include the required OS kernel API and constants file. */

#include    "nucleus.h"
#include    "kernel/nu_kernel.h"

/* Mapping to provide abstraction for memory allocation. */

#define     CAN_HANDLER_MEM_STACK           NU_MIN_STACK_SIZE
#define     CAN_HANDLER_PRIORITY            0

/* HISRs for handling Nucleus CAN message events. */

extern      NU_HISR   CAN_Tx_Handler;
extern      NU_HISR   CAN_Rx_Handler;

#endif      /* !CAN_OSAL_H */

