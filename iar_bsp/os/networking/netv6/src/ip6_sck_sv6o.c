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
*       ip6_sck_sv6o.c                               
*                                                                                  
*   DESCRIPTION                                                              
*                
*       This file contains the routine for enabling or disabling the
*       socket option for a NU_FAMILY_IP6 socket to be used for IPv6
*       communications only.
*                                                                          
*   DATA STRUCTURES                                                          
*                      
*       None                
*                                                                          
*   FUNCTIONS                                                                
*           
*       IP6_Set_IPV6_V6ONLY
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
*       IP6_Set_IPV6_V6ONLY                                                       
*                                                                       
*   DESCRIPTION                                                           
*                                                                         
*       This function sets the socket option for a NU_FAMILY_IP6 socket
*       to be used for IPv6 communications only.
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
*       NU_INVALID_SOCKET       The socket is not an IPv4/IPv6 socket.
*                                                                       
*************************************************************************/
STATUS IP6_Set_IPV6_V6ONLY(INT socketd, INT optval)
{
    struct  sock_struct *sck_ptr = SCK_Sockets[socketd];
    STATUS  status;

    /* Check that the socket type is SK_FAM_IP6 */
    if (sck_ptr->s_family == SK_FAM_IP6)
    {
        /* Enable the option on the socket */
        if (optval)
            sck_ptr->s_options |= SO_IPV6_V6ONLY;

        /* Disable the option on the socket */
        else  
            sck_ptr->s_options &= ~SO_IPV6_V6ONLY;

        status = NU_SUCCESS;
    }

    else
        status = NU_INVALID_SOCKET;

    return (status);

} /* IP6_Set_IPV6_V6ONLY */
