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
*       sck6_spi.c                                   
*
*   DESCRIPTION
*
*       This file contains the implementation of 
*       NU_Setsockopt_IPV6_PKTINFO.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Setsockopt_IPV6_PKTINFO
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
*       NU_Setsockopt_IPV6_PKTINFO                                                     
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function sets the source address and outgoing interface
*       index Sticky Options on the socket.
*                                                                         
*   INPUTS                                                                
*                                                                         
*       socketd                 Specifies a socket descriptor
*       *pkt_info               A pointer to the in6_pktinfo structure
*                               holding the new data.
*       length                  The length of the pkt_info structure.
*
*   OUTPUTS                                                               
*                                                                         
*       NU_SUCCESS              Successful operation.
*       NU_MEM_ALLOC            Insufficient memory.
*       NU_INVALID_SOCKET       The specified socket descriptor is invalid.
*       NU_INVAL                The length of the options are greater
*                               than the stack can handle, the address
*                               specified does not exist on the node,
*                               the socket type is TCP and the source
*                               address cannot be changed, the 
*                               interface index is invalid, the
*                               interface is not IPv6-enabled or
*                               pkt_info is NULL.
*                                                                         
*************************************************************************/
STATUS NU_Setsockopt_IPV6_PKTINFO(INT socketd, in6_pktinfo *pkt_info)
{
    STATUS  status;

    if (pkt_info == NU_NULL)
        status = NU_INVAL;

    else
    {
        /* Obtain the semaphore and validate the socket */
        status = SCK_Protect_Socket_Block(socketd);

        if (status == NU_SUCCESS)
        {
            status = IP6_Set_IPV6_PKTINFO(socketd, pkt_info, sizeof(in6_pktinfo));

            /* Release the semaphore */
            SCK_Release_Socket();
        }
    }

    return (status);

} /* NU_Setsockopt_IPV6_PKTINFO */
