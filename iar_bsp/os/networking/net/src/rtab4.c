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
*       rtab4.c
*
*   COMPONENT
*
*       IPv4 Routing
*
*   DESCRIPTION
*
*       This module contains the functions for initializing the IPv4
*       routing table.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       RTAB4_Init
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV4 == NU_TRUE)

/* The top or root of the route table tree. */
ROUTE_NODE  *RTAB4_Root_Node;

/* This is the default route to return when no other route can be found. */
ROUTE_NODE  *RTAB4_Default_Route;

RTAB_ROUTE_PARMS    RTAB4_Parms;

/*************************************************************************
*
*   FUNCTION
*
*       RTAB4_Init
*
*   DESCRIPTION
*
*       Initialize the default_route structure
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID RTAB4_Init(VOID)
{
    RTAB4_Default_Route  = NU_NULL;
    RTAB4_Root_Node      = NULL_ROUTE_NODE;

    RTAB4_Parms.rt_family = NU_FAMILY_IP;
    RTAB4_Parms.rt_root_node = &RTAB4_Root_Node;
    RTAB4_Parms.rt_bit_ip_len = 32;
    RTAB4_Parms.rt_byte_ip_len = 4;
    RTAB4_Parms.rt_insert_route_entry = RTAB4_Insert_Route_Entry;
    RTAB4_Parms.rt_determine_matching_prefix = RTAB4_Determine_Matching_Prefix;
    RTAB4_Parms.rt_setup_new_node = RTAB4_Setup_New_Node;

} /* RTAB4_Init */

#endif
