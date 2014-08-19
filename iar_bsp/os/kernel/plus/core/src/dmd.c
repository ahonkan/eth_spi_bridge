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
*       dmd.c
*
*   COMPONENT
*
*       DM - Dynamic Memory Management
*
*   DESCRIPTION
*
*       This file contains global data structures for use within the
*       Dynamic Memory Management component.
*
*   DATA STRUCTURES
*
*       DMD_Created_Pools                   Pointer to the linked-list
*                                           of created dynamic pools
*       DMD_Total_Pools                     Total number of created
*                                           dynamic pools
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
************************************************************************/
#include        "nucleus.h"
#include        "kernel/nu_kernel.h"

/* DMD_Created_Pools_List is the head pointer of the linked list of
   created dynamic memory pools.  If the list is NU_NULL, there are no
   dynamic memory pools created.  */

CS_NODE         *DMD_Created_Pools_List;


/* DMD_Total_Pools contains the number of currently created
   dynamic memory pools.  */

UNSIGNED        DMD_Total_Pools;

