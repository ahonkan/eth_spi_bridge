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
*       sck6_grhdo.c                                 
*
*   DESCRIPTION
*
*       This file contains the implementation of 
*       NU_Getsockopt_IPV6_RTHDRDSTOPTS.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Getsockopt_IPV6_RTHDRDSTOPTS
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
*       NU_Getsockopt_IPV6_RTHDRDSTOPTS                                                     
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function retrieves the value of the destination options 
*       Sticky Option for destination options that precede the Routing
*       Header that was previously set by the application.
*                                                                         
*   INPUTS                                                                
*                                                                         
*       socketd                 Specifies a socket descriptor
*       *dst_hdr                A pointer to the ip6_dest to fill in
*                               with the destination options.
*       *optlen                 The length of the data filled into the
*                               ip6_dest structure.
*
*   OUTPUTS                                                               
*                                                                         
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_SOCKET       The specified socket descriptor is invalid.
*       NU_INVAL                Either optval or optlen are invalid.
*                                                                         
*************************************************************************/
STATUS NU_Getsockopt_IPV6_RTHDRDSTOPTS(INT socketd, struct ip6_dest *dst_hdr,
                                       INT *optlen)
{
    STATUS  status;

    if (dst_hdr == NU_NULL)
        status = NU_INVAL;

    else
    {
        /* Obtain the semaphore and validate the socket */
        status = SCK_Protect_Socket_Block(socketd);

        if (status == NU_SUCCESS)
        {
            status = IP6_Get_IPV6_RTHDRDSTOPTS(socketd, dst_hdr, optlen);

            /* Release the semaphore */
            SCK_Release_Socket();
        }
    }

    return (status);

} /* NU_Getsockopt_IPV6_RTHDRDSTOPTS */
