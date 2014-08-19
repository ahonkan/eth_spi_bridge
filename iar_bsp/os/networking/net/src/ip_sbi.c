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
*       ip_sbi.c
*
*   DESCRIPTION
*
*       This file contains the routine to set the broadcast interface
*       for a socket.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       IP_Setsockopt_IP_BROADCAST_IF
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
*       IP_Setsockopt_IP_BROADCAST_IF
*
*   DESCRIPTION
*
*       This function sets the broadcast interface for a socket.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *dev_address            A pointer to the IP address of the
*                               interface to set as the broadcast
*                               interface for the socket.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVAL                The IP address provided does not
*                               reference an interface on the node.
*
*************************************************************************/
STATUS IP_Setsockopt_IP_BROADCAST_IF(INT socketd, UINT8 *dev_address)
{
    STATUS          status = NU_SUCCESS;
    UINT8           *ip_addr_ptr;
    struct          sock_struct  *sck_ptr = SCK_Sockets[socketd];
    DV_DEVICE_ENTRY *dv_ptr;

    /* If the IP address pointer is NU_NULL remove the interface */
    if (dev_address == NU_NULL)
        sck_ptr->s_bcast_if = NU_NULL;
    else
    {
        /* Point to the address */
        ip_addr_ptr = dev_address;

        /* If the IP address is all zeros, remove the interface */
        if (IP_ADDR(ip_addr_ptr) == IP_ADDR_ANY)
            sck_ptr->s_bcast_if = NU_NULL;
        else
        {
            /* Get a pointer to the interface */
            dv_ptr = DEV_Get_Dev_By_Addr(ip_addr_ptr);

            /* Set the broadcast interface for the socket */
            if (dv_ptr)
                sck_ptr->s_bcast_if = dv_ptr;
            else
                status = NU_INVAL;
        }
    }

    return (status);

} /* IP_Setsockopt_IP_BROADCAST_IF */
