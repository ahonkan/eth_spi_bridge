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
*       ip_gbi.c
*
*   DESCRIPTION
*
*       This file contains the routine to get the broadcast interface
*       for a socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       IP_Getsockopt_IP_BROADCAST_IF
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV4 == NU_TRUE)

/*************************************************************************
*
*   FUNCTION
*
*       IP_Getsockopt_IP_BROADCAST_IF
*
*   DESCRIPTION
*
*       This function gets the broadcast interface for a socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *dev_address            A pointer to the IP address of the
*                               interface.
*       *addr_len               A pointer to the length of the IP
*                               address.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID IP_Getsockopt_IP_BROADCAST_IF(INT socketd, UINT8 *dev_address,
                                   INT *addr_len)
{
    struct  sock_struct  *sck_ptr = SCK_Sockets[socketd];

    /* Set the length */
    *addr_len = IP_ADDR_LEN;

    /* Make sure one has been set. */
    if (sck_ptr->s_bcast_if)
    {
        ((UINT8*)dev_address)[0] = (UINT8)(0x000000FF &
            (sck_ptr->s_bcast_if->dev_addr.dev_addr_list.
             dv_head->dev_entry_ip_addr >> 24));

        ((UINT8*)dev_address)[1] = (UINT8)(0x000000FF &
            (sck_ptr->s_bcast_if->dev_addr.dev_addr_list.
             dv_head->dev_entry_ip_addr >> 16));

        ((UINT8*)dev_address)[2] = (UINT8)(0x000000FF &
            (sck_ptr->s_bcast_if->dev_addr.dev_addr_list.
             dv_head->dev_entry_ip_addr >> 8));

        ((UINT8*)dev_address)[3] = (UINT8)(0x000000FF &
            (sck_ptr->s_bcast_if->dev_addr.dev_addr_list.
             dv_head->dev_entry_ip_addr));
    }
    else
    {
        /* There is no interface set so set the return to zero. */
        ((UINT8 *)dev_address)[0] = 0;
        ((UINT8 *)dev_address)[1] = 0;
        ((UINT8 *)dev_address)[2] = 0;
        ((UINT8 *)dev_address)[3] = 0;
    }

} /* IP_Getsockopt_IP_BROADCAST_IF */

#endif
