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
*       ip6_sck_gmi.c                                
*                                                                                  
*   DESCRIPTION                                                              
*                
*       This file contains the routine for getting the multicast 
*       interface.
*                                                                          
*   DATA STRUCTURES                                                          
*                      
*       None                
*                                                                          
*   FUNCTIONS                                                                
*           
*       IP6_Get_IPV6_MULTICAST_IF
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
*       IP6_Get_IPV6_MULTICAST_IF                                                       
*                                                                       
*   DESCRIPTION                                                           
*                                                                         
*       This function gets the multicast interface for a socket.
*                                                                         
*   INPUTS                                                                
*                                                                         
*       socketd                 Specifies a socket descriptor
*       if_index                The interface index of the interface.
*
*   OUTPUTS                                                               
*                                                                         
*       NU_SUCCESS              Successful operation.
*                                                                       
*************************************************************************/
STATUS IP6_Get_IPV6_MULTICAST_IF(INT socketd, INT32 *if_index)
{
    struct  sock_struct *sck_ptr = SCK_Sockets[socketd];

    /* Make sure one has been set. */
    if ( (sck_ptr->s_moptions_v6) && (sck_ptr->s_moptions_v6->multio_device) )
        *if_index = (INT32)(sck_ptr->s_moptions_v6->multio_device->dev_index);
    else
        *if_index = -1;

    return (NU_SUCCESS);

} /* IP6_Get_IPV6_MULTICAST_IF */
