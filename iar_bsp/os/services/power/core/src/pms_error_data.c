/*************************************************************************
*
*            Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
* FILE NAME
*
*      pms_error_data.c
*
* COMPONENT
*
*      PMS - Power Error/Exception Handling
*
* DESCRIPTION
*
*      This file contains global data structures for use within this
*      component.
*
* DATA STRUCTURES
*
*      PMS_Error_Protect
*      PMS_Error_Handler
*      PMS_Error_Queue
*
* FUNCTIONS
*
*      None
*
* DEPENDENCIES
*
*       power_core.h
*
*************************************************************************/

#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "services/power_core.h"

/* Protect structure for use when updating error data */
NU_PROTECT PMS_Error_Protect;

/* Error handler */
VOID (*PMS_Error_Handler)(STATUS, VOID *, VOID *, UINT32);

/* Queue to be used by the error system
   to pass messages */
NU_QUEUE PMS_Error_Queue;

