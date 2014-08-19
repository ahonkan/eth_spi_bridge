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
*       mbd.c
*
*   COMPONENT
*
*       MB - Mailbox Management
*
*   DESCRIPTION
*
*       This file contains global data structures for use within the
*       mailbox management component.
*
*   DATA STRUCTURES
*
*       MBD_Created_Mailboxes_List          Pointer to the linked-list
*                                           of created mailboxes
*       MBD_Total_Mailboxes                 Total number of created
*                                           mailboxes
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

/* MBD_Created_Mailboxes_List is the head pointer of the linked list of
   created mailboxes.  If the list is NU_NULL, there are no mailboxes
   created.  */

CS_NODE        *MBD_Created_Mailboxes_List;


/* MBD_Total_Mailboxes contains the number of currently created
   mailboxes.  */

UNSIGNED        MBD_Total_Mailboxes;
