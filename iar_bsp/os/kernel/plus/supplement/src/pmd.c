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
*       pmd.c
*
*   COMPONENT
*
*       PM - Partition Memory Management
*
*   DESCRIPTION
*
*       This file contains global data structures for use within the
*       Partition Memory Management component.
*
*   DATA STRUCTURES
*
*       PMD_Created_Pools_List              Pointer to the linked-list
*                                           of created partition pools
*       PMD_Total_Pools                     Total number of created
*                                           partition pools
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

/* PMD_Created_Pools_List is the head pointer of the linked list of
   created partition pools.  If the list is NU_NULL, there are no partition
   pools created.  */

CS_NODE        *PMD_Created_Pools_List;


/* PMD_Total_Pools contains the number of currently created
   partition pools.  */

UNSIGNED        PMD_Total_Pools;
