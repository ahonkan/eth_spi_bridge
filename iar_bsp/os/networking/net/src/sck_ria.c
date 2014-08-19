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
*       sck_ria.c
*
*   DESCRIPTION
*
*       This file contains the implementation of SCK_Recv_IF_Addr.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       SCK_Recv_IF_Addr
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV4 == NU_TRUE)

/***********************************************************************
*
*   FUNCTION
*
*       SCK_Recv_IF_Addr
*
*   DESCRIPTION
*
*       Returns the IP address of the interface that received the
*       last datagram read by the application. This is useful when
*       receiving multicast and broadcast datagrams since they do not
*       contain a specific destination IP address.
*
*   INPUTS
*
*       socketd                 Socket descriptor.
*       *if_addr                Pointer to the location to store the IP
*                               address.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful completion of the service.
*       NU_INVALID_PARM         The if_addr pointer is not valid.
*       NU_INVALID_SOCKET       The socket descriptor is not valid.
*       NU_NOT_CONNECTED        The if_addr pointer is not valid.
*
*************************************************************************/
STATUS SCK_Recv_IF_Addr(INT socketd, UINT8 *if_addr)
{
    SOCKET_STRUCT       *sck;
    STATUS              status;

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    if (if_addr == NU_NULL)
        return (NU_INVALID_PARM);

#endif

	/* Obtain the semaphore and validate the socket */
	status = SCK_Protect_Socket_Block(socketd);

	if (status == NU_SUCCESS)
	{
		sck = SCK_Sockets[socketd];

		/* Make sure one has been set. */
		if(((sck->s_recv_if != NU_NULL) &&
				(sck->s_recv_if->dev_addr.
				 dev_addr_list.dv_head != NU_NULL)))
		{
			((UINT8 *)if_addr)[0] = (UINT8)(0x000000FF &
					 (sck->s_recv_if->dev_addr.
					  dev_addr_list.dv_head->dev_entry_ip_addr >> 24));
			((UINT8 *)if_addr)[1] = (UINT8)(0x000000FF &
					 (sck->s_recv_if->dev_addr.
					  dev_addr_list.dv_head->dev_entry_ip_addr >> 16));
			((UINT8 *)if_addr)[2] = (UINT8)(0x000000FF &
					 (sck->s_recv_if->dev_addr.
					  dev_addr_list.dv_head->dev_entry_ip_addr >> 8));
			((UINT8 *)if_addr)[3] = (UINT8)(0x000000FF &
					 (sck->s_recv_if->dev_addr.
					  dev_addr_list.dv_head->dev_entry_ip_addr));
		}
		else
		{
			/* There is no interface set so set the return to zero. */
			((UINT8 *)if_addr)[0] = 0;
			((UINT8 *)if_addr)[1] = 0;
			((UINT8 *)if_addr)[2] = 0;
			((UINT8 *)if_addr)[3] = 0;
		}

		/* Release the semaphore */
		SCK_Release_Socket();
    }

    return (status);

} /* SCK_Recv_IF_Addr */

#endif
