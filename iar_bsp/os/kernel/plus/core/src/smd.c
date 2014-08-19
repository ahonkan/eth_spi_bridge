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
*       smd.c
*
*   COMPONENT
*
*       SM - Semaphore Management
*
*   DESCRIPTION
*
*       This file contains global data structures for use within the
*       semaphore management component.
*
*   DATA STRUCTURES
*
*       SMD_Created_Semaphores_List         Pointer to the linked-list
*                                           of created semaphores
*       SMD_Total_Semaphores                Total number of created
*                                           semaphores
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

/* SMD_Created_Semaphores_List is the head pointer of the linked list of
   created semaphores.  If the list is NU_NULL, there are no semaphores
   created.  */

CS_NODE        *SMD_Created_Semaphores_List;


/* SMD_Total_Semaphores contains the number of currently created
   semaphores.  */

UNSIGNED        SMD_Total_Semaphores;
