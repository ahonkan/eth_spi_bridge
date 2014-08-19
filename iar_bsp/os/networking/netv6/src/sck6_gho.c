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
*       sck6_gho.c                                   
*
*   DESCRIPTION
*
*       This file contains the implementation of 
*       NU_Getsockopt_IPV6_HOPOPTS.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Getsockopt_IPV6_HOPOPTS
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
*       NU_Getsockopt_IPV6_HOPOPTS                                                     
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function retrieves the value of the hop-by-hop options 
*       Sticky Option that was previously set by the application.
*                                                                         
*   INPUTS                                                                
*                                                                         
*       socketd                 Specifies a socket descriptor
*       *hbh_hdr                A pointer to the ip6_hbh to fill in
*                               with the hop-by-hop options.
*       *optlen                 The length of the data filled into the
*                               ip6_hbh structure.
*
*   OUTPUTS                                                               
*                                                                         
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_SOCKET       The specified socket descriptor is invalid.
*       NU_INVAL                Either optval or optlen are invalid.
*                                                                         
*************************************************************************/
STATUS NU_Getsockopt_IPV6_HOPOPTS(INT socketd, struct ip6_hbh *hbh_hdr,
                                  INT *optlen)
{
    STATUS  status;

    if (hbh_hdr == NU_NULL)
        status = NU_INVAL;

    else
    {
        /* Obtain the semaphore and validate the socket */
        status = SCK_Protect_Socket_Block(socketd);

        if (status == NU_SUCCESS)
        {
            status = IP6_Get_IPV6_HOPOPTS(socketd, hbh_hdr, optlen);

            /* Release the semaphore */
            SCK_Release_Socket();
        }
    }

    return (status);

} /* NU_Getsockopt_IPV6_HOPOPTS */