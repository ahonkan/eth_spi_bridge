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
*       rtab6_gdr.c                                  
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
*       RTAB6_Get_Default_Route
*                                                                       
*   DEPENDENCIES                                                          
*                                                                       
*       nu_net.h
*                                                                       
*************************************************************************/

#include "networking/nu_net.h"

/* This is the default route to return when no other route can be found. */
extern ROUTE_NODE   *RTAB6_Default_Route;

/*************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       RTAB6_Get_Default_Route                                             
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       Get the default route for the NU_Ripng system.                    
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
ROUTE_NODE *RTAB6_Get_Default_Route(VOID)
{
    return (RTAB6_Default_Route);

} /* RTAB6_Get_Default_Route */
