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
*
*   FILE NAME
*
*       sck_so.c
*
*   DESCRIPTION
*
*       This file contains the implementation of NU_Socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Socket
*
*   DEPENDENCIES
*
*       nu_net.h
*       ipraw.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/ipraw.h"

/*************************************************************************
*
*   FUNCTION
*
*       NU_Socket
*
*   DESCRIPTION
*
*       This function is responsible for establishing a new socket
*       descriptor and defining the type of communication protocol to
*       be established.  Must be called by both client and server whether
*       connection-oriented or connectionless transfer is established.
*
*   INPUTS
*
*       family                  The family type of the socket.  Either
*                               NU_FAMILY_IP to send/receive only IPv4
*                               packets using the socket or
*                               NU_FAMILY_IP6 to send/receive IPv4/IPv6
*                               packets using the socket.
*       type                    Valid entries for the type parameter
*                               include:
*
*                               NU_TYPE_STREAM  TCP Protocol
*                               NU_TYPE_DGRAM   UDP Protocol
*                               NU_TYPE_RAW     IP Protocol
*       protocol                The protocol parameter should be set to
*                               NU_NONE.
*
*   OUTPUTS
*
*       > = 0                   Socket descriptor
*       NU_INVALID_PROTOCOL     The specified protocol is invalid.
*       NU_NO_SOCK_MEMORY       The memory for the sockets is depleted.
*
*************************************************************************/
INT NU_Socket(INT16 family, INT16 type, INT16 protocol)
{
    INT                 return_socket;
    struct sock_struct  *sockptr;

    NU_SUPERV_USER_VARIABLES

    /* possible protocols based on [FAMILY, TYPE] */
    INT NU_proto_list[][5] =
    {
        {NU_PROTO_INVALID,  NU_PROTO_INVALID,  NU_PROTO_INVALID,
             NU_PROTO_INVALID,  NU_PROTO_INVALID},
        {NU_PROTO_INVALID,  NU_PROTO_INVALID,  NU_PROTO_INVALID,
             NU_PROTO_INVALID,  NU_PROTO_INVALID},
        {
#if (INCLUDE_TCP == NU_TRUE)
        NU_PROTO_TCP,
#else
        NU_PROTO_INVALID,
#endif

#if (INCLUDE_UDP == NU_TRUE)
        NU_PROTO_UDP,
#else
        NU_PROTO_INVALID,
#endif

#if (INCLUDE_IP_RAW == NU_TRUE)
        NU_PROTO_ICMP,
#else
        NU_PROTO_INVALID,
#endif
        NU_PROTO_INVALID, NU_PROTO_INVALID},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0},
        {
#if (INCLUDE_TCP == NU_TRUE)
        NU_PROTO_TCP,
#else
        NU_PROTO_INVALID,
#endif

#if (INCLUDE_UDP == NU_TRUE)
        NU_PROTO_UDP,
#else
        NU_PROTO_INVALID,
#endif

#if (INCLUDE_IP_RAW == NU_TRUE)
        NU_PROTO_ICMP,
#else
        NU_PROTO_INVALID,
#endif
        NU_PROTO_INVALID, NU_PROTO_INVALID}
    };

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

#if (INCLUDE_IPV6 == NU_TRUE)
    /* If the protocol type is not IPv4 or IPv6, return an error */
    if ( (
#if (INCLUDE_IPV4 == NU_TRUE)
         ((family < 0) || (family > 2)) &&
#endif
          (family != SK_FAM_IP6)) || (type < 0) || (type > 4) )
#else
    /* Make sure that family and type are in range. */
    if ( (family < 0) || (family > 2) || (type < 0) || (type > 4) )
#endif
        return (NU_INVALID_PROTOCOL);

    /* verify that we support the programmer's choice */
    if (!NU_proto_list[family][type])
        return (NU_INVALID_PROTOCOL);

#endif

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

#if (INCLUDE_IP_RAW == NU_FALSE)

    /* Clean up warnings.  This parameter is required for socket compatibility
     * but we are currently not making any use of it.
     */
    UNUSED_PARAMETER(protocol);

#endif

    /* Don't let any other users in until we are done.  */
    return_socket = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (return_socket == NU_SUCCESS)
    {
        /* Create a new socket. */
#if (INCLUDE_IP_RAW == NU_TRUE)
        if (type == NU_TYPE_RAW)
        {
            /* Could be many protocols with a Raw socket, so must use
             * the protocol entered.
             */
            return_socket = SCK_Create_Socket(protocol, family);
        }
        else
#endif
        {
            return_socket = SCK_Create_Socket(NU_proto_list[family][type],
                                              family);
        }

        /* If a socket was created, create a port structure for the
         * socket.
         */
        if (return_socket >= 0)
        {
            /* Get a pointer to the socket structure */
            sockptr = SCK_Sockets[return_socket];

            /* Allocate the port for this socket */
            switch (type)
            {
#if (INCLUDE_TCP == NU_TRUE)

                case NU_TYPE_STREAM:

                    sockptr->s_port_index = TCP_Make_Port(family, 0);

                    if (sockptr->s_port_index >= 0)
                    {
                        TCP_Ports[sockptr->s_port_index]->p_socketd =
                            return_socket;

                        /* Store the local port number in the socket
                         * structure.
                         */
                        sockptr->s_local_addr.port_num =
                            TCP_Ports[sockptr->s_port_index]->in.port;
                    }

                    break;
#endif

#if (INCLUDE_UDP == NU_TRUE)

                case NU_TYPE_DGRAM:

                    sockptr->s_port_index =
                        (INT)UDP_Make_Port(0, return_socket);

                    if (sockptr->s_port_index >= 0)
                    {
                        /* Store the local port number in the socket
                         * structure.
                         */
                        sockptr->s_local_addr.port_num =
                            UDP_Ports[sockptr->s_port_index]->up_lport;
                    }

                    break;
#endif

#if (INCLUDE_IP_RAW == NU_TRUE)

                case NU_TYPE_RAW:

                    sockptr->s_port_index =
                        (INT)IPRaw_Make_Port(return_socket);

                    break;
#endif

                default:

                    break;
            }

            /* If a port structure could not be created, deallocate
             * the memory for the socket structure and return an error.
             */
            if (sockptr->s_port_index < 0)
            {
#if (INCLUDE_STATIC_BUILD == NU_FALSE)
                if (NU_Deallocate_Memory((VOID*)SCK_Sockets[return_socket])
                    != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to deallocate memory for socket",
                                NERR_SEVERE, __FILE__, __LINE__);
                }
#endif
                SCK_Sockets[return_socket] = 0;

                return_socket = NU_NO_SOCK_MEMORY;
            }
        }

        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                            __FILE__, __LINE__);
        }
    }

    /* Trace log */
    T_SOCK_LIST(family, type, protocol, return_socket);

    /* Switch back to user mode. */
    NU_USER_MODE();

    /* return a SCK_Sockets index or an error status to caller */
    return (return_socket);

} /* NU_Socket */
