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
*       ip6_sck_gv6o.c                               
*                                                                                  
*   DESCRIPTION                                                              
*                
*       This file contains the routine for retrieving the value of the 
*       socket option to receive only IPv6 packets on NU_FAMILY_IP6
*       sockets.
*                                                                          
*   DATA STRUCTURES                                                          
*                      
*       None                
*                                                                          
*   FUNCTIONS                                                                
*           
*       IP6_Get_IPV6_V6ONLY
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
*       IP6_Get_IPV6_V6ONLY                                                       
*                                                                       
*   DESCRIPTION                                                           
*                                                                         
*       This functions retrieves the value of the socket option for 
*       enabling or disabling the socket option to receive only IPv6
*       packets on NU_FAMILY_IP6 sockets.
*                                                                         
*   INPUTS                                                                
*                                                                         
*       socketd                 Specifies a socket descriptor
*       *optval                 The value of the socket option.
*       *optlen                 The length of the value stored in the
*                               socket option.
*
*   OUTPUTS                                                               
*                                                                         
*       1                       The socket option is enabled on the 
*                               socket.
*       0                       The socket option is not enabled on
*                               the socket.
*                                                                       
*************************************************************************/
STATUS IP6_Get_IPV6_V6ONLY(INT socketd, INT *optval, INT *optlen)
{
    *optlen = sizeof(INT);

    if (SCK_Sockets[socketd]->s_options & SO_IPV6_V6ONLY)
        *optval = 1;
    else
        *optval = 0;

    return (NU_SUCCESS);

} /* IP6_Get_IPV6_V6ONLY */
