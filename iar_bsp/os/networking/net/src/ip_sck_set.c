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
*       ip_sck_set.c
*
*   DESCRIPTION
*
*       This file contains routines for setting IP options settings.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       IP_Set_Opt
*       IP_Set_IP_Opt
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
*       IP_Set_Opt
*
*   DESCRIPTION
*
*       Sets the option to NU_INVALID_OPTION unless IP multicasting is
*       defined.
*
*   INPUTS
*
*       socketd                 Socket descriptor.
*       optname                 The option name
*       *optval                 A pointer to the option value
*       optlen                  The option length
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation
*       NU_INVALID_OPTION       Invalid option was passed in
*       NU_MEM_ALLOC            No memory to allocate from
*       NU_ADDRINUSE            Address is in use
*       NU_INVAL                Generic invalid return
*
*************************************************************************/
STATUS IP_Set_Opt(INT socketd, INT optname, VOID *optval, INT optlen)
{
    STATUS      status;

    switch (optname)
    {

    case IP_BROADCAST_IF        :
    case IP_RECVIFADDR          :

        status = IP_Set_IP_Opt(socketd, optname, optval, optlen);
        break;

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)

    case IP_MULTICAST_IF        :
    case IP_ADD_MEMBERSHIP      :
    case IP_DROP_MEMBERSHIP     :
    case IP_MULTICAST_TTL       :

        status = IP_Set_Multi_Opt(socketd, optname, optval, optlen);
        break;

#endif

#if (INCLUDE_IP_RAW == NU_TRUE)

    case IP_HDRINCL:

        status = IP_Set_Raw_Opt(socketd, optval, optlen);
        break;

#endif

    case IP_TTL:

        if ( (optval == NU_NULL) || (optlen < (INT)(sizeof(UINT16))) )
            status = NU_INVAL;
        else
        {
            IP_Setsockopt_IP_TTL(socketd, *(UINT16*)optval);
            status = NU_SUCCESS;
        }
        break;

    case IP_TOS:

        if (optlen != (INT)(sizeof(UINT8)))
            status = NU_INVAL;
        else
            status = IP_Setsockopt_IP_TOS(socketd, *(UINT8 *)optval);

        break;

    case IP_PKTINFO:

        if ( (optval == NU_NULL) || (optlen != (INT)(sizeof(INT))) )
            status = NU_INVAL;
        else
            status = IP_Setsockopt_IP_PKTINFO(socketd, *(INT *)optval);

        break;

    default :
        status = NU_INVALID_OPTION;
    }

    return (status);

} /* IP_Set_Opt */

/***********************************************************************
*
*   FUNCTION
*
*       IP_Set_IP_Opt
*
*   DESCRIPTION
*
*       Sets an IP level socket option.
*
*   INPUTS
*
*       socketd                 Socket descriptor for the option
*       optname                 The option name itself
*       *optval                 Pointer to the option value
*       optlen                  Size of the area pointed to by optval
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operations
*       NU_INVAL                Invalid optlen, invalid address, or
*                               invalid optname
*
*************************************************************************/
STATUS IP_Set_IP_Opt(INT socketd, INT optname, VOID *optval, INT optlen)
{
    STATUS  status;

    switch (optname)
    {
        case IP_BROADCAST_IF        :

            /* Check the parameters. */
            if (optlen != IP_ADDR_LEN)
                status = NU_INVAL;
            else
                status = IP_Setsockopt_IP_BROADCAST_IF(socketd,
                                                       (UINT8*)optval);

            break;

        case IP_RECVIFADDR          :

            /* Check the parameters */
            if ( (optlen  < (INT)sizeof(UINT16)) || (optval == NU_NULL) )
                status = NU_INVAL;
            else
            {
                IP_Setsockopt_IP_RECVIFADDR(socketd, *(UINT16*)optval);
                status = NU_SUCCESS;
            }

            break;

        default:

            status = NU_INVAL;
            break;

    } /* end switch */

    return (status);

} /* IP_Set_IP_Opt */
