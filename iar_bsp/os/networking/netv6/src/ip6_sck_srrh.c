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
*       ip6_sck_srrh.c                               
*                                                                                  
*   DESCRIPTION                                                              
*                
*       This file contains the routine for enabling or disabling the
*       socket option to return the routing header of the most recently
*       received IPv6 packet on a socket.
*                                                                          
*   DATA STRUCTURES                                                          
*                      
*       None                
*                                                                          
*   FUNCTIONS                                                                
*           
*       IP6_Set_IPV6_RECVRTHDR
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
*       IP6_Set_IPV6_RECVRTHDR                                                       
*                                                                       
*   DESCRIPTION                                                           
*                                                                         
*       This function sets the socket option to return the routing 
*       header of the most recently received packet.
*                                                                         
*   INPUTS                                                                
*                                                                         
*       socketd                 Specifies a socket descriptor
*       optval                  A value of zero disables the option on
*                               the socket.  A non-zero value enables the
*                               option on the socket.
*
*   OUTPUTS                                                               
*                                                                         
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_SOCKET       The socket is a TCP socket.
*                                                                       
*************************************************************************/
STATUS IP6_Set_IPV6_RECVRTHDR(INT socketd, INT optval)
{
    struct  sock_struct *sck_ptr = SCK_Sockets[socketd];
    STATUS  status;

    if (sck_ptr->s_protocol != NU_PROTO_TCP)
    {
        /* Enable the option on the socket */
        if (optval)
            sck_ptr->s_options |= SO_IPV6_RTHDR_OP;

        /* Disable the option on the socket */
        else  
            sck_ptr->s_options &= ~SO_IPV6_RTHDR_OP;

        status = NU_SUCCESS;
    }

    else
        status = NU_INVALID_SOCKET;

    return (status);

} /* IP6_Set_IPV6_RECVRTHDR */
