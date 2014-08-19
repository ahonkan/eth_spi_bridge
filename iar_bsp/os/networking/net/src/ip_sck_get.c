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
*       ip_sck_get.c
*
*   DESCRIPTION
*
*       This file contains routines for retrieving IP options settings.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       IP_Get_Opt
*       IP_Get_IP_Opt
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/***********************************************************************
*
*   FUNCTION
*
*       IP_Get_Opt
*
*   DESCRIPTION
*
*       Gets a multicast option if IP multicasting is defined,
*       otherwise returns NU_INVALID_OPTION.
*
*   INPUTS
*
*       socketd                 Socket descriptor.
*       optname                 The option name
*       *optval                 A pointer to the option value
*       *optlen                 A pointer to the option length
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation
*       NU_INVALID_OPTION       Received an invalid option
*       NU_INVAL                Simple invalid response
*
*************************************************************************/
STATUS IP_Get_Opt(INT socketd, INT optname, VOID *optval, INT *optlen)
{
    STATUS      status;

    switch (optname)
    {

    case IP_BROADCAST_IF        :
    case IP_RECVIFADDR          :

        status = IP_Get_IP_Opt(socketd, optname, optval, optlen);
        break;


#if (INCLUDE_IP_MULTICASTING == NU_TRUE)

    case IP_MULTICAST_IF  :
    case IP_MULTICAST_TTL :

        status = IP_Get_Multi_Opt(socketd, optname, optval, optlen);
        break;

#endif

#if (INCLUDE_IP_RAW == NU_TRUE)

    case IP_HDRINCL :

        IP_Getsockopt_IP_HDRINCL(socketd, (INT16*)optval, optlen);
        status = NU_SUCCESS;

        break;

#endif

    case IP_TTL:

        IP_Getsockopt_IP_TTL(socketd, (UINT16*)optval, optlen);
        status = NU_SUCCESS;
        break;

    case IP_TOS:

        IP_Getsockopt_IP_TOS(socketd, (UINT8*)optval, optlen);
        status = NU_SUCCESS;
        break;

    case IP_PKTINFO:

        status = IP_Getsockopt_IP_PKTINFO(socketd, optval, optlen);
        break;

    default :
        status = NU_INVALID_OPTION;
    }

    return (status);

} /* IP_Get_Opt */

/***********************************************************************
*
*   FUNCTION
*
*       IP_Get_IP_Opt
*
*   DESCRIPTION
*
*       Gets an IP level socket option.
*
*   INPUTS
*
*       socketd                 Socket descriptor for the option
*       optname                 The option name itself
*       *optval                 Pointer to the option value
*       *optlen                 Pointer to the size of the area pointed
*                               to by optval
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation
*       NU_INVAL                One of the pointers is invalid
*
*************************************************************************/
STATUS IP_Get_IP_Opt(INT socketd, INT optname, VOID *optval, INT *optlen)
{
    STATUS  status;

    switch (optname)
    {
        case IP_BROADCAST_IF   :

            IP_Getsockopt_IP_BROADCAST_IF(socketd, (UINT8*)optval,
                                          optlen);
            status = NU_SUCCESS;

            break;

        case IP_RECVIFADDR      :

            IP_Getsockopt_IP_RECVIFADDR(socketd, (INT16*)optval, optlen);
            status = NU_SUCCESS;

            break;

        default:

            status = NU_INVAL;
            break;

    } /* end switch */

    return (status);

} /* IP_Get_IP_Opt */
