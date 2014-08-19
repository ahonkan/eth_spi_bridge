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
*       sck6_lg.c                                    
*
*   DESCRIPTION
*
*       This file contains the implementation of 
*       NU_Setsockopt_IPV6_LEAVE_GROUP.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Setsockopt_IPV6_LEAVE_GROUP
*
*   DEPENDENCIES
*
*       nu_net.h
*		externs6.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/externs6.h"

/*************************************************************************
*                                                                         
*   FUNCTION                                                              
*                                                                         
*       NU_Setsockopt_IPV6_LEAVE_GROUP                                                     
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function leaves a multicast group on a socket.
*                                                                         
*   INPUTS                                                                
*                                                                         
*       socketd                 Specifies a socket descriptor
*       *mreq                   A pointer to the multicast data structure.
*
*   OUTPUTS                                                               
*                                                                         
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_SOCKET       The specified socket descriptor is invalid.
*       NU_INVALID_PARM         mreq is invalid.
*       NU_MEM_ALLOC            Insufficient memory.
*       NU_INVAL                The multicast address is invalid, or the
*                               socket is not a member of the group.
*                                                                         
*************************************************************************/
STATUS NU_Setsockopt_IPV6_LEAVE_GROUP(INT socketd, const IP6_MREQ *mreq)
{
    STATUS  status;

    if ( (mreq == NU_NULL) || 
         (!(IPV6_IS_ADDR_MULTICAST(mreq->ip6_mreq_multiaddr))) )
        status = NU_INVAL;

    else
    {
        /* Obtain the semaphore and validate the socket */
        status = SCK_Protect_Socket_Block(socketd);

        if (status == NU_SUCCESS)
        {
            status = IP6_Process_Multicast_Listen(socketd, mreq->ip6_mreq_dev_index,
                                             mreq->ip6_mreq_multiaddr, 
                                             MULTICAST_FILTER_INCLUDE, NU_NULL, 0);

            /* Release the semaphore */
            SCK_Release_Socket();
        }
    }

    return (status);

} /* NU_Setsockopt_IPV6_LEAVE_GROUP */
