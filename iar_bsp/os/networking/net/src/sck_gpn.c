/*************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
* FILE NAME
*
*       sck_gpn.c
*
* DESCRIPTION
*
*       This file contains the implementation of NU_Get_Peer_Name.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_Get_Peer_Name
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/*************************************************************************
*
*   FUNCTION
*
*       NU_Get_Peer_Name
*
*   DESCRIPTION
*
*       This function returns the client endpoint for the specified
*       socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *peer                   A pointer to a structure containing the
*                               remote client's address
*       *addr_length            Return pointer for the length of the peer
*                               structure returned.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful retrieval of a socket option.
*       NU_INVALID_PARM         The address length is invalid.
*       NU_INVALID_SOCKET       The socket descriptor was invalid.
*       NU_NOT_CONNECTED        The specified socket descriptor did not
*                               have a client connected to it.
*
*************************************************************************/
STATUS NU_Get_Peer_Name(INT socketd, struct sockaddr_struct *peer,
                        INT16 *addr_length)
{
    STATUS return_status;

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    if ( (peer == NU_NULL) || (addr_length == NU_NULL) ||
         (*addr_length < (INT16)sizeof (struct sockaddr_struct) ))
        return (NU_INVALID_PARM);

#endif

    /* Obtain the semaphore and validate the socket */
    return_status = SCK_Protect_Socket_Block(socketd);

    if (return_status == NU_SUCCESS)
    {
        /* verify that the socket has a connection to a client */
        if ( (SCK_Sockets[socketd]->s_foreign_addr.port_num == 0)
             || (!SCK_Socket_Connected(socketd)))
             /* return error status to caller */
             return_status = NU_NOT_CONNECTED;
        else
        {
            /* store the client endpoints for return to the caller */
            memcpy (&peer->ip_num,
                    &SCK_Sockets[socketd]->s_foreign_addr.ip_num,
                    MAX_ADDRESS_SIZE);

            memcpy (&peer->port_num,
                    &SCK_Sockets[socketd]->s_foreign_addr.port_num, 2);

            peer->family = SCK_Sockets[socketd]->s_family;

            /* store the length of the endpoint structure for return to the
              caller */
            *addr_length = sizeof(struct sockaddr_struct);

            return_status = NU_SUCCESS;
        }

        /* Release the semaphore */
        SCK_Release_Socket();
    }

    /* return to caller */
    return (return_status);

} /* NU_Get_Peer_Name */
