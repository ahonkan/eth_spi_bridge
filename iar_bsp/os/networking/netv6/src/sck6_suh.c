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
*       sck6_suh.c                                   
*
*   DESCRIPTION
*
*       This file contains the implementation of 
*       NU_Setsockopt_IPV6_UNICAST_HOPS.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Setsockopt_IPV6_UNICAST_HOPS
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/*************************************************************************
*                                                                         
*   FUNCTION                                                              
*                                                                         
*       NU_Setsockopt_IPV6_UNICAST_HOPS                                                     
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function sets the Hop Limit for an IPv6-enabled socket.
*                                                                         
*   INPUTS                                                                
*                                                                         
*       socketd                 Specifies a socket descriptor
*       hop_limit               The value of the new Hop Limit or -1 to
*                               set the Hop Limit back to the default.
*
*   OUTPUTS                                                               
*                                                                         
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_SOCKET       The specified socket descriptor is invalid.
*                                                                         
*************************************************************************/
STATUS NU_Setsockopt_IPV6_UNICAST_HOPS(INT socketd, INT16 hop_limit)
{
    STATUS  status;

    /* Obtain the semaphore and validate the socket */
    status = SCK_Protect_Socket_Block(socketd);

    if (status == NU_SUCCESS)
    {
        status = IP6_Set_IPV6_UNICAST_HOPS(socketd, hop_limit);

        /* Release the semaphore */
        SCK_Release_Socket();
    }

    return (status);

} /* NU_Setsockopt_IPV6_UNICAST_HOPS */
