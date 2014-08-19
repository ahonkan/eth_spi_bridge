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
*       cand.c
*
* COMPONENT
*
*       Nucleus CAN
*
* DESCRIPTION
*
*       This file contains the global declarations for Nucleus CAN.
*
* DATA STRUCTURES
*
*       CAN_Devs
*       CAN_Dev_Control_Blocks
*       CAN_Devs_In_Port
*       CAN_HW_Init_Semaphore
*
* FUNCTIONS
*
*       None
*
* DEPENDENCIES
*
*       can_extr.h                          Function prototypes and
*                                           macros for Nucleus CAN.
*
*************************************************************************/
#define     NU_CAN_SOURCE_FILE

#include    "connectivity/can_extr.h"


/* Array of pointers to Nucleus CAN control blocks for each registered
   Nucleus CAN driver. */
CAN_CB         *CAN_Devs[NU_CAN_MAX_DEV_COUNT];

/* Control block for drivers for Nucleus CAN. */
CAN_CB          CAN_Dev_Control_Blocks[NU_CAN_MAX_DEV_COUNT];

#if         ((NU_CAN_OPERATING_MODE != NU_CAN_LOOPBACK))
/* Semaphore used for CAN hardware initialization. */
NU_SEMAPHORE    CAN_HW_Init_Semaphore;
#endif

#if         NU_CAN_MULTIPLE_PORTS_SUPPORT

/* Array containing the number of CAN controller in each
   CAN driver port. */
UINT8       CAN_Devs_In_Port[CAN_MAX_SUPPORTED_PORTS];

#endif      /* NU_CAN_MULTIPLE_PORTS_SUPPORT */

