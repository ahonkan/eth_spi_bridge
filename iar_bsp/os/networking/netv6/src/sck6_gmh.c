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
*       sck6_gmh.c                                   
*
*   DESCRIPTION
*
*       This file contains the implementation of 
*       NU_Getsockopt_IPV6_MULTICAST_HOPS.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Getsockopt_IPV6_MULTICAST_HOPS
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
*       NU_Getsockopt_IPV6_MULTICAST_HOPS                                                     
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function gets the hop limit to use for outgoing multicast 
*       packets.
*                                                                         
*   INPUTS                                                                
*                
*       socketd                 The socket index.                                                         
*       hop_limit               The hop limit to use.
*
*   OUTPUTS                                                               
*                                                                         
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_SOCKET       The specified socket descriptor is invalid.
*       NU_INVAL                hop_limit is invalid.
*                                                                         
*************************************************************************/
STATUS NU_Getsockopt_IPV6_MULTICAST_HOPS(INT socketd, INT *hop_limit)
{
    STATUS  status;

    if (hop_limit == NU_NULL)
        status = NU_INVAL;

    else
    {
        /* Obtain the semaphore and validate the socket */
        status = SCK_Protect_Socket_Block(socketd);

        if (status == NU_SUCCESS)
        {
            status = IP6_Get_IPV6_MULTICAST_HOPS(socketd, hop_limit);

            /* Release the semaphore */
            SCK_Release_Socket();
        }
    }

    return (status);

} /* NU_Getsockopt_IPV6_MULTICAST_HOPS */
