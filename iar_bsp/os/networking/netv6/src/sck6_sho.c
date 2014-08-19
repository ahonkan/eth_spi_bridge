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
*       sck6_sho.c                                   
*
*   DESCRIPTION
*
*       This file contains the implementation of 
*       NU_Setsockopt_IPV6_HOPOPTS.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Setsockopt_IPV6_HOPOPTS
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
*       NU_Setsockopt_IPV6_HOPOPTS                                                     
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function sets the hop-by-hop options Sticky Option.
*                                                                         
*   INPUTS                                                                
*                                                                         
*       socketd                 Specifies a socket descriptor
*       *hbh_hdr                A pointer to the IPv6 hop-by-hop options.
*       length                  The length of the hbh_hdr structure.
*
*   OUTPUTS                                                               
*                                                                         
*       NU_SUCCESS              Successful operation.
*       NU_MEM_ALLOC            Insufficient memory.
*       NU_INVALID_SOCKET       The specified socket descriptor is invalid.
*       NU_INVAL                hbh_hdr is NULL or length is zero.
*                                                                         
*************************************************************************/
STATUS NU_Setsockopt_IPV6_HOPOPTS(INT socketd, const struct ip6_hbh *hbh_hdr, 
                                  INT length)
{
    STATUS  status;

    if ( (hbh_hdr == NU_NULL) && (length != 0) )
        status = NU_INVAL;

    else
    {
        /* Obtain the semaphore and validate the socket */
        status = SCK_Protect_Socket_Block(socketd);

        if (status == NU_SUCCESS)
        {
            status = IP6_Set_IPV6_HOPOPTS(socketd, hbh_hdr, length);

            /* Release the semaphore */
            SCK_Release_Socket();
        }
    }

    return (status);

} /* NU_Setsockopt_IPV6_HOPOPTS */
