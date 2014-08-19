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
*       sck_fs.c
*
* DESCRIPTION
*
*       This file contains the implementation of NU_Find_Socket.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_Find_Socket
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
*       NU_Find_Socket
*
*   DESCRIPTION
*
*       This function returns the socket descriptor associated with
*       the local and foreign address and the local and foreign port
*       number for the protocol specified.
*
*   INPUTS
*
*       protocol                NU_PROTO_TCP or NU_PROTO_UDP
*       *local_addr             A pointer to the data structure that holds
*                               the local address and port number.
*       *foreign_addr           A pointer to the data structure that holds
*                               the foreign address and port number.
*
*   OUTPUTS
*
*       This function returns the socket descriptor associated with
*       the connection if successful.  Otherwise, it returns one of
*       the following errors codes:
*
*       NU_INVALID_PARM         One of the pointers is NULL.
*       NU_INVALID_PROTOCOL     The specified protocol is not valid.
*       NU_INVALID_SOCKET       There is no socket associated with the
*                               parameters provided.
*
*************************************************************************/
INT NU_Find_Socket(INT protocol, const struct addr_struct *local_addr,
                   const struct addr_struct *foreign_addr)
{
    INT                 socket;

#if ((INCLUDE_TCP == NU_TRUE) || (INCLUDE_UDP == NU_TRUE))
    INT                 status;
    INT                 i;

#if (INCLUDE_IPV4 == NU_TRUE)
    UINT32              local_address;
    UINT32              foreign_address;
#endif
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
    UINT8               local_addressv6[IP6_ADDR_LEN];
    UINT8               foreign_addressv6[IP6_ADDR_LEN];
#endif

#if (INCLUDE_TCP == NU_TRUE)
    struct _TCP_Port    *tcp_port;
#endif

#if (INCLUDE_UDP == NU_TRUE)
    struct uport        *udp_port;
#endif
    NU_SUPERV_USER_VARIABLES

#if ((INCLUDE_TCP == NU_TRUE) || (INCLUDE_UDP == NU_TRUE))
    socket = 0;
#endif

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    if ( (!local_addr) || (!foreign_addr) )
    	return (NU_INVALID_PARM);

#endif

#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
#if ( (INCLUDE_TCP == NU_TRUE) || (INCLUDE_UDP == NU_TRUE) )
	local_address = 0;
	foreign_address = 0;
#endif
	if (local_addr->family == SK_FAM_IP6)
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
		NU_BLOCK_COPY(local_addressv6, local_addr->id.is_ip_addrs,
					  IP6_ADDR_LEN);

#if (INCLUDE_IPV4 == NU_TRUE)
	else
#endif
#endif

#if ((INCLUDE_IPV4 == NU_TRUE) && ((INCLUDE_TCP == NU_TRUE) || (INCLUDE_UDP == NU_TRUE)))

		local_address = IP_ADDR(local_addr->id.is_ip_addrs);
#endif

#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
	if (foreign_addr->family == SK_FAM_IP6)
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
		NU_BLOCK_COPY(foreign_addressv6, foreign_addr->id.is_ip_addrs,
					  IP6_ADDR_LEN);

#if (INCLUDE_IPV4 == NU_TRUE)
	else
#endif
#endif

#if ((INCLUDE_IPV4 == NU_TRUE) && ((INCLUDE_TCP == NU_TRUE) || (INCLUDE_UDP == NU_TRUE)))

		foreign_address = IP_ADDR(foreign_addr->id.is_ip_addrs);
#endif

	NU_SUPERVISOR_MODE();

	switch (protocol)
	{
#if (INCLUDE_TCP == NU_TRUE)

	case NU_PROTO_TCP:

		status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

		if (status != NU_SUCCESS)
		{
			NU_USER_MODE();
			return(status);
		}

		/* Loop through the port list until we find the socket */
		for (i = 0; i < TCP_MAX_PORTS; i++)
		{
			tcp_port = TCP_Ports[i];

			if ( (tcp_port) && (tcp_port->in.port == local_addr->port) &&
				 (tcp_port->out.port == foreign_addr->port) && (

#if (INCLUDE_IPV6 == NU_TRUE)

				 ((local_addr->family == SK_FAM_IP6) &&
				  (memcmp(local_addressv6, tcp_port->tcp_laddrv6, IP6_ADDR_LEN) == 0))

#if (INCLUDE_IPV4 == NU_TRUE)
				  ||
#else
				  ) &&
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

				  ((local_addr->family == SK_FAM_IP) &&
				   (local_address == tcp_port->tcp_laddrv4))) &&
#endif

#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
				(
#endif

#if (INCLUDE_IPV6 == NU_TRUE)

				 ((foreign_addr->family == SK_FAM_IP6) &&
				  (memcmp(foreign_addressv6, tcp_port->tcp_faddrv6, IP6_ADDR_LEN) == 0))

#if (INCLUDE_IPV4 == NU_TRUE)
				  ||
#else
				  )
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

				  ((foreign_addr->family == SK_FAM_IP) &&
				   (foreign_address == tcp_port->tcp_faddrv4)))

#if (INCLUDE_IPV6 == NU_TRUE)
		)
#endif
#endif
			{
				socket = tcp_port->p_socketd;

				if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
					NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
								   __FILE__, __LINE__);

				break;
			}
		}

		/* If we did not find a match, return an error */
		if (i >= TCP_MAX_PORTS)
		{
			if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
				NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
							   __FILE__, __LINE__);

			socket = NU_INVALID_SOCKET;
		}

		break;
#endif

#if (INCLUDE_UDP == NU_TRUE)

	case NU_PROTO_UDP:

		status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

		if (status != NU_SUCCESS)
		{
			NU_USER_MODE();
			return(status);
		}

		/* Loop through the port list until we find a match */
		for (i = 0; i < UDP_MAX_PORTS; i++)
		{
			udp_port = UDP_Ports[i];

			if ( (udp_port) && (udp_port->up_lport == local_addr->port) &&
				 (udp_port->up_fport == foreign_addr->port) && (

#if (INCLUDE_IPV6 == NU_TRUE)

				 ((local_addr->family == SK_FAM_IP6) &&
				  (foreign_addr->family == SK_FAM_IP6) &&
				  (memcmp(local_addressv6, udp_port->up_laddrv6, IP6_ADDR_LEN) == 0) &&
				  (memcmp(foreign_addressv6, udp_port->up_faddrv6, IP6_ADDR_LEN) == 0))

#if (INCLUDE_IPV4 == NU_TRUE)
				  ||
#else
				  ) )
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

				  ((local_addr->family == NU_FAMILY_IP) &&
				   (local_address == udp_port->up_laddr) &&
				   (foreign_address == udp_port->up_faddr))) )
#endif
			{
				socket = udp_port->up_socketd;

				if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
					NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
								   __FILE__, __LINE__);

				break;
			}
		}

		/* If we did not find a match, return an error */
		if (i >= UDP_MAX_PORTS)
		{
			if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
				NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
							   __FILE__, __LINE__);

			socket = NU_INVALID_SOCKET;
		}

		break;
#endif

	default:

		socket = NU_INVALID_PROTOCOL;
		break;
	}

    NU_USER_MODE();

    return (socket);

} /* NU_Find_Socket */
