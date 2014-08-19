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
*       rtab4_gdr.c
*
*   COMPONENT
*
*       IPv4 Routing
*
*   DESCRIPTION
*
*       This module contains the functions for returning the Default
*       Route.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       RTAB4_Get_Default_Route
*
*   DEPENDENCIES
*
*       nu_net.h
*       rtab4.h
*
*************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV4 == NU_TRUE)

/* This is the default route to return when no other route can be found. */
extern ROUTE_NODE   *RTAB4_Default_Route;

/*************************************************************************
*
*   FUNCTION
*
*       RTAB4_Get_Default_Route
*
*   DESCRIPTION
*
*       Get the default route for the system.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       *ROUTE_NODE             A pointer to the Default Route
*       NU_NULL                 No default route exists
*
*************************************************************************/
ROUTE_NODE *RTAB4_Get_Default_Route(VOID)
{
    return (RTAB4_Default_Route);

} /* RTAB4_Get_Default_Route */

#endif
