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
*       ip_sck_mo.c
*
* DESCRIPTION
*
*       This file contains routines for IP multicast option settings.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       IP_Get_Multi_Opt
*       IP_Set_Multi_Opt
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/***********************************************************************
*
*   FUNCTION
*
*       IP_Get_Multi_Opt
*
*   DESCRIPTION
*
*       Gets a multicast operation if IP multicasting is defined. At this
*       time the only option is the IP_MULTICAST_TTL.
*
*   INPUTS
*
*       socketd                 Socket descriptor.
*       optname                 The name of the option.
*       *optval                 A pointer to the option value.
*       *optlen                 A pointer to the option length.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_INVAL                One of the pointers is invalid.
*
*************************************************************************/
STATUS IP_Get_Multi_Opt(INT socketd, INT optname, VOID *optval,
                        INT *optlen)
{
    STATUS  status;

    switch (optname)
    {
    case IP_MULTICAST_IF   :

        IP_Getsockopt_IP_MULTICAST_IF(socketd, (UINT8*)optval, optlen);
        status = NU_SUCCESS;

        break;

    case IP_MULTICAST_TTL :

        IP_Getsockopt_IP_MULTICAST_TTL(socketd, (UINT8*)optval, optlen);
        status = NU_SUCCESS;

        break;

    default:

        status = NU_INVAL;
        break;
    }

    return (status);

} /* IP_Get_Multi_Opt */

/************************************************************************
*
*   FUNCTION
*
*       IP_Set_Multi_Opt
*
*   DESCRIPTION
*
*       Sets the Multicast option buffer to the socket.
*
*   INPUTS
*
*       socketd                 Socket descriptor.
*       optname                 Name of the multicast option
*       *optval                 A pointer to the option value
*       optlen                  The option length
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation
*       NU_MEM_ALLOC            The memory allocation failed
*       NU_ADDRINUSE            The address is in use
*       NU_INVAL                General invalid return
*       NU_INVALID_OPTION       Invalid multicast option
*
*************************************************************************/
STATUS IP_Set_Multi_Opt(INT socketd, INT optname, VOID *optval,
                        INT optlen)
{
    STATUS  status;

    switch (optname)
    {
    case IP_MULTICAST_IF   :

        /* Check the parameters. */
        if (optlen != IP_ADDR_LEN)
            status = NU_INVAL;
        else
            status = IP_Setsockopt_IP_MULTICAST_IF(socketd,
                                                    (UINT8*)optval);

        break;

    /* Add a multicast group. */
    case IP_ADD_MEMBERSHIP :

        /* Check the parameters. */
        if ( (optval == NU_NULL) || (optlen != (INT)sizeof(IP_MREQ)) )
            status = NU_INVAL;
        else
            status = IP_Process_Multicast_Listen(socketd,
                              (UINT8 *)&(((IP_MREQ *)optval)->sck_inaddr),
                              (UINT8 *)&(((IP_MREQ *)optval)->sck_multiaddr),
                                    MULTICAST_FILTER_EXCLUDE, NU_NULL, 0);

        break;

    /* Delete a multicast group. */
    case IP_DROP_MEMBERSHIP :

        /* Check the parameters. */
        if ( (optval == NU_NULL) || (optlen != (INT)sizeof(IP_MREQ)) )
            status = NU_INVAL;
        else
          status = IP_Process_Multicast_Listen(socketd,
                                (UINT8 *)&(((IP_MREQ *)optval)->sck_inaddr),
                                (UINT8 *)&(((IP_MREQ *)optval)->sck_multiaddr),
                                    MULTICAST_FILTER_INCLUDE, NU_NULL, 0);

        break;

    /* Set the TTL for this outgoing multicasts. */
    case IP_MULTICAST_TTL :

        /* Check the parameters. */
        if ( (optval == NU_NULL) || (optlen != 1) )
            status = NU_INVAL;
        else
            status = IP_Setsockopt_IP_MULTICAST_TTL(socketd,
                                                    *(UINT8*)optval);

        break;

    default :

        status = NU_INVALID_OPTION;
        break;
    }

    return (status);

} /* IP_Set_Multi_Opt */
