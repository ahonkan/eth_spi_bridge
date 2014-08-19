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
*       qud.c
*
*   COMPONENT
*
*       QU - Queue Management
*
*   DESCRIPTION
*
*       This file contains global data structures for use within the
*       queue management component.
*
*   DATA STRUCTURES
*
*       QUD_Created_Queue_List              Pointer to the linked-list
*                                           of created queues
*       QUD_Total_Queues                    Total number of created
*                                           queues
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

/* QUD_Created_Queues_List is the head pointer of the linked list of
   created queues.  If the list is NU_NULL, there are no queues
   created.  */

CS_NODE        *QUD_Created_Queues_List;


/* QUD_Total_Queues contains the number of currently created
   queues.  */

UNSIGNED        QUD_Total_Queues;
