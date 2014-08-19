/***********************************************************************
*
*            Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
************************************************************************

************************************************************************
*
*   FILE NAME
*
*       pid.c
*
*   COMPONENT
*
*       PI - Pipe Management
*
*   DESCRIPTION
*
*       This file contains global data structures for use within the
*       pipe management component.
*
*   DATA STRUCTURES
*
*       PID_Created_Pipe_List               Pointer to the linked-list
*                                           of created pipes
*       PID_Total_Pipes                     Total number of created
*                                           pipes
*
*   FUNCTIONS
*
*       None
*
*   DEPENDENCIES
*
*       nucleus.h                           Nucleus System constants
*       nu_kernel.h                         Kernel constants
*
***********************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"

/* PID_Created_Pipes_List is the head pointer of the linked list of
   created pipes.  If the list is NU_NULL, there are no pipes
   created.  */

CS_NODE        *PID_Created_Pipes_List;


/* PID_Total_Pipes contains the number of currently created
   pipes.  */

UNSIGNED        PID_Total_Pipes;
