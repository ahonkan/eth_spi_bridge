/*************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILE NAME
*
*       rtab4_grn.c
*
*   COMPONENT
*
*       IPv4 Routing
*
*   DESCRIPTION
*
*       This module contains the functions for returning the Root Node
*       of the IPv4 routing tree.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       RTAB4_Get_Root_Node
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV4 == NU_TRUE)

/* The top or root of the route table tree. */
extern ROUTE_NODE   *RTAB4_Root_Node;

/*************************************************************************
*
*   FUNCTION
*
*       RTAB4_Get_Root_Node
*
*   DESCRIPTION
*
*       Returns the Tree top to the calling function.
*
*   INPUTS
*
*      None.
*
*   OUTPUTS
*
*      *ROUTE_NODE              A pointer to the Root Node.
*
*************************************************************************/
ROUTE_NODE *RTAB4_Get_Root_Node(VOID)
{
    return (RTAB4_Root_Node);

} /* RTAB4_Get_Root_Node */

#endif
