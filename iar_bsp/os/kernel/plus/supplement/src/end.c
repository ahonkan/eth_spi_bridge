/*************************************************************************
*
*              Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILENAME
*
*       end_common.c
*
*   COMPONENT
*
*       EN - Event Notification
*
*   DESCRIPTION
*
*       This file contains the global variables for the Event Notification component
*
*
*   DEPENDENCIES
*
*       <string.h>                          String functions
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*       event_notification.h                Event Notification functions
*
*************************************************************************/

#include <string.h>
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "os/kernel/plus/supplement/inc/event_notification.h"

/* Notification Registry active dev count */
INT  END_Reg_Active_Cnt;

/* Notification Registry */
EN_REGISTRY *END_Registry;
