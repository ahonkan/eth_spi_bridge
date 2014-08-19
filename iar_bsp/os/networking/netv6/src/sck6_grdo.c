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
*       sck6_grdo.c                                  
*
*   DESCRIPTION
*
*       This file contains the implementation of 
*       NU_Getsockopt_IPV6_RECVDSTOPTS.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Getsockopt_IPV6_RECVDSTOPTS
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
*       NU_Getsockopt_IPV6_RECVDSTOPTS                                                     
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This functions retrieves the value of the socket option for 
*       enabling or disabling the socket option to return the destination
*       options of the most recently received IPv6 packet on a socket.
*                                                                         
*   INPUTS                                                                
*                                                                         
*       socketd                 Specifies a socket descriptor
*       *optval                 The value of the option.
*       *optlen                 The length of the value of the option.
*
*   OUTPUTS                                                               
*                                                                         
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_SOCKET       The specified socket descriptor is invalid.
*       NU_INVAL                Either optval or optlen are invalid.
*                                                                         
*************************************************************************/
STATUS NU_Getsockopt_IPV6_RECVDSTOPTS(INT socketd, INT *optval, INT *optlen)
{
    STATUS  status;

    if ( (optval == NU_NULL) || (*optlen < (INT)(sizeof(INT))) )
        status = NU_INVAL;

    else
    {
        /* Obtain the semaphore and validate the socket */
        status = SCK_Protect_Socket_Block(socketd);

        if (status == NU_SUCCESS)
        {
            status = IP6_Get_IPV6_RECVDSTOPTS(socketd, optval, optlen);

            /* Release the semaphore */
            SCK_Release_Socket();
        }
    }

    return (status);

} /* NU_Getsockopt_IPV6_RECVDSTOPTS */

