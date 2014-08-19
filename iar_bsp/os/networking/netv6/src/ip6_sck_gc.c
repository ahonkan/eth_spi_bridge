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
*       ip6_sck_gc.c                                 
*                                                                                  
*   DESCRIPTION                                                              
*                
*       This files contains the function for retrieving the value of the 
*       socket option for enabling or disabling the socket option for the 
*       IP layer to compute the checksum for outgoing RAW packets and 
*       verify the checksum for incoming RAW packets.
*                                                                          
*   DATA STRUCTURES                                                          
*                      
*       None                
*                                                                          
*   FUNCTIONS                                                                
*           
*       IP6_Get_IPV6_CHECKSUM
*                                                                          
*   DEPENDENCIES                                                             
*              
*       nu_net.h
*                                                                          
*************************************************************************/

#include "networking/nu_net.h"
#include "networking/ipraw.h"

/***********************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       IP6_Get_IPV6_CHECKSUM                                                       
*                                                                       
*   DESCRIPTION                                                           
*                                                                         
*       This functions retrieves the value of the socket option for 
*       enabling or disabling the socket option for the IP layer to
*       compute the checksum for outgoing RAW packets and verify the
*       checksum for incoming RAW packets.
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
*       NU_SUCCESS              Successful operation.
*                                                                       
*************************************************************************/
STATUS IP6_Get_IPV6_CHECKSUM(INT socketd, INT *optval, INT *optlen)
{
    struct  sock_struct *sck_ptr = SCK_Sockets[socketd];

    *optlen = sizeof(INT);

    /* If the socket option is set, return the integer offset into the
     * user data of where the checksum is located.
     */
    if (sck_ptr->s_options & SO_IPV6_CHECKSUM)
        *optval = IPR_Ports[sck_ptr->s_port_index]->ip_chk_off;
    else
        *optval = -1;

    return (NU_SUCCESS);

} /* IP6_Get_IPV6_CHECKSUM */
