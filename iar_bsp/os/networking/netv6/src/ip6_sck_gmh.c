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
*       ip6_sck_gmh.c                                
*                                                                                  
*   DESCRIPTION                                                              
*                
*       This file contains the routine for getting the multicast 
*       hop limit.
*                                                                          
*   DATA STRUCTURES                                                          
*                      
*       None                
*                                                                          
*   FUNCTIONS                                                                
*           
*       IP6_Get_IPV6_MULTICAST_HOPS
*                                                                          
*   DEPENDENCIES                                                             
*              
*       nu_net.h
*                                                                          
*************************************************************************/

#include "networking/nu_net.h"

/***********************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       IP6_Get_IPV6_MULTICAST_HOPS                                                       
*                                                                       
*   DESCRIPTION                                                           
*                                                                         
*       This function gets the multicast hop limit for a socket.
*                                                                         
*   INPUTS                                                                
*                                                                         
*       socketd                 Specifies a socket descriptor
*       hop_limit               The multicast hop limit of the interface.
*
*   OUTPUTS                                                               
*                                                                         
*       NU_SUCCESS              Successful operation.
*                                                                       
*************************************************************************/
STATUS IP6_Get_IPV6_MULTICAST_HOPS(INT socketd, INT *hop_limit)
{
    struct  sock_struct *sck_ptr = SCK_Sockets[socketd];

    /* Make sure one has been set. */
    if (sck_ptr->s_moptions_v6)
        *hop_limit = sck_ptr->s_moptions_v6->multio_hop_lmt;
    else
        *hop_limit = IP6_DEFAULT_MULTICAST_TTL;

    return (NU_SUCCESS);

} /* IP6_Get_IPV6_MULTICAST_HOPS */
