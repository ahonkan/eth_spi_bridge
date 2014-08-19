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
*       evd.c
*
*   COMPONENT
*
*       EV - Event Group Management
*
*   DESCRIPTION
*
*       This file contains global data structures for use within the
*       Event Group Management component.
*
*   DATA STRUCTURES
*
*       EVD_Created_Event_Groups_List       Pointer to the linked-list
*                                           of created event groups
*       EVD_Total_Event_Groups              Total number of created
*                                           event groups
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

/*! EVD_Created_Event_Groups_List is the head pointer of the linked list of
    created event groups.  If the list is NU_NULL, there are no event groups
    created. */
CS_NODE        *EVD_Created_Event_Groups_List;


/*! EVD_Total_Event_Groups contains the number of currently created
    event groups. */

UNSIGNED        EVD_Total_Event_Groups;

 /*! 
 *  \ingroup  EV
 */

