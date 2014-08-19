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
*       ip6_sck_grrh.c                               
*                                                                                  
*   DESCRIPTION                                                              
*                
*       This file contains the routine for retrieving the value of the 
*       socket option for enabling or disabling the socket option to 
*       return the routing header of the most recently received IPv6 
*       packet on a socket.
*                                                                          
*   DATA STRUCTURES                                                          
*                      
*       None                
*                                                                          
*   FUNCTIONS                                                                
*           
*       IP6_Get_IPV6_RECVRTHDR
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
*       IP6_Get_IPV6_RECVRTHDR                                                       
*                                                                       
*   DESCRIPTION                                                           
*                                                                         
*       This functions retrieves the value of the socket option for 
*       enabling or disabling the socket option to return the routing 
*       header of the most recently received IPv6 packet on a socket.
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
STATUS IP6_Get_IPV6_RECVRTHDR(INT socketd, INT *optval, INT *optlen)
{
    *optlen = sizeof(INT);

    if (SCK_Sockets[socketd]->s_options & SO_IPV6_RTHDR_OP)
        *optval = 1;
    else
        *optval = 0;

    return (NU_SUCCESS);

} /* IP6_Get_IPV6_RECVRTHDR */
