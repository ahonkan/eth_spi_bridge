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
*       sck6_srrh.c                                  
*
*   DESCRIPTION
*
*       This file contains the implementation of 
*       NU_Setsockopt_IPV6_RECVRTHDR.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Setsockopt_IPV6_RECVRTHDR
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
*       NU_Setsockopt_IPV6_RECVRTHDR                                                     
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function sets the socket option to return all routing 
*       headers of the most recently received packet.
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
*       NU_MEM_ALLOC            Insufficient memory.
*       NU_INVALID_SOCKET       The specified socket descriptor is invalid.
*                                                                         
*************************************************************************/
STATUS NU_Setsockopt_IPV6_RECVRTHDR(INT socketd, INT optval)
{
    STATUS  status;

    /* Obtain the semaphore and validate the socket */
    status = SCK_Protect_Socket_Block(socketd);

    if (status == NU_SUCCESS)
    {
        status = IP6_Set_IPV6_RECVRTHDR(socketd, optval);

        /* Release the semaphore */
        SCK_Release_Socket();
    }

    return (status);

} /* NU_Setsockopt_IPV6_RECVRTHDR */
