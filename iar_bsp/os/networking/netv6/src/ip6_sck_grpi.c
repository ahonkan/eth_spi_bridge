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
*       ip6_sck_grpi.c                               
*                                                                                  
*   DESCRIPTION                                                              
*                
*       This file contains the routine for retrieving the value of the
*       socket option to return the received interface and/or destination 
*       address of the most recently received packet.
*                                                                          
*   DATA STRUCTURES                                                          
*                      
*       None                
*                                                                          
*   FUNCTIONS                                                                
*           
*       IP6_Get_IPV6_RECVPKTINFO
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
*       IP6_Get_IPV6_RECVPKTINFO                                                       
*                                                                       
*   DESCRIPTION                                                           
*                                                                         
*       This function retrieves the value of the socket option to 
*       return the received interface and/or destination address of 
*       the most recently received packet.
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
*                                                                       
*************************************************************************/
STATUS IP6_Get_IPV6_RECVPKTINFO(INT socketd, INT *optval, INT *optlen)
{
    *optlen = sizeof(INT);

    if (SCK_Sockets[socketd]->s_options & SO_IPV6_PKTINFO_OP)
        *optval = 1;
    else
        *optval = 0;

    return (NU_SUCCESS);

} /* IP6_Get_IPV6_RECVPKTINFO */
