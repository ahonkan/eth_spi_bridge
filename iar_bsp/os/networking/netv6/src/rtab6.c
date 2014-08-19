/*************************************************************************
*
*              Copyright 2002 Mentor Graphics Corporation              
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
*       rtab6.c                                      
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       This module contains the functions for accessing the IPv6 routing     
*       table.                                                           
*                                                                       
*   DATA STRUCTURES                                                       
*                                                                       
*                                                                       
*   FUNCTIONS                                                             
*           
*       RTAB6_Init
*                                                                       
*   DEPENDENCIES                                                          
*                                                                       
*       nu_net.h
*       defrtr6.h
*       prefix6.h
*                                                                       
*************************************************************************/

#include "networking/nu_net.h"
#include "networking/defrtr6.h"
#include "networking/prefix6.h"

ROUTE_NODE  *RTAB6_Root_Node;
ROUTE_NODE  *RTAB6_Default_Route;

RTAB_ROUTE_PARMS    RTAB6_Parms;
TQ_EVENT            DEFRTR6_Expire_Entry_Event;
TQ_EVENT            PREFIX6_Expire_Entry_Event;

/* Default Router List */
extern IP6_DEFAULT_ROUTER_LIST  *Default_Router_List;

/*************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       RTAB6_Init                                                       
*                                                                       
*   DESCRIPTION                                                           
*                  
*       This function initializes the Data Structure and Global Variables
*       associated with the IPv6 Routing Table.                                                     
*                                                                       
*   INPUTS                                                                
*                                                                       
*       None
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       None
*                                                                       
*************************************************************************/
VOID RTAB6_Init(VOID)
{
    STATUS  status;

    RTAB6_Root_Node = NU_NULL; 
    RTAB6_Default_Route = NU_NULL;


    /* Allocate memory for the Default Router list */
    status = NU_Allocate_Memory(MEM_Cached, (VOID**)&Default_Router_List,
                           sizeof(IP6_DEFAULT_ROUTER_LIST), 
                           NU_NO_SUSPEND);

    if (status == NU_SUCCESS)
    {
        Default_Router_List->dv_head = NU_NULL;
        Default_Router_List->dv_tail = NU_NULL;
    }
    else
        NLOG_Error_Log("Cannot create Default Router List due to lack of memory", 
                       NERR_SEVERE, __FILE__, __LINE__);

    /* Register the Default Router list events */
    if (EQ_Register_Event(DEFRTR6_Expire_Default_Router_Entry, 
                          &DEFRTR6_Expire_Entry_Event) != NU_SUCCESS)
        NLOG_Error_Log("Failed to register Default Router event", NERR_SEVERE, 
                       __FILE__, __LINE__);

    /* Set up the RTAB6_Parms data structure which holds the parameters 
     * specific to an IPv6 route that differ from an IPv4 route.
     */
    RTAB6_Parms.rt_family = NU_FAMILY_IP6;
    RTAB6_Parms.rt_root_node = &RTAB6_Root_Node;
    RTAB6_Parms.rt_bit_ip_len = 128;
    RTAB6_Parms.rt_byte_ip_len = 16;
    RTAB6_Parms.rt_insert_route_entry = RTAB6_Insert_Route_Entry;
    RTAB6_Parms.rt_determine_matching_prefix = RTAB6_Determine_Matching_Prefix;
    RTAB6_Parms.rt_setup_new_node = RTAB6_Setup_New_Node;

    /* Register the Prefix List events */
    if (EQ_Register_Event(PREFIX6_Expire_Entry, 
                          &PREFIX6_Expire_Entry_Event) != NU_SUCCESS)
        NLOG_Error_Log("Failed to register Prefix event", NERR_SEVERE, 
                       __FILE__, __LINE__);

} /* RTAB6_Init */
