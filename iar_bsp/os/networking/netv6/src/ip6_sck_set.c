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
*       ip6_sck_set.c                                
*                                                                                  
*   DESCRIPTION                                                              
*                
*       This file contains routines for setting IPv6 options.
*                                                                          
*   DATA STRUCTURES                                                          
*                      
*       None                
*                                                                          
*   FUNCTIONS                                                                
*           
*       IP6_Set_Opt
*                                                                          
*   DEPENDENCIES                                                             
*              
*       externs.h
*                                                                          
*************************************************************************/

#include "networking/externs.h"
#include "networking/externs6.h"

/***********************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       IP6_Set_Opt                                                       
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       This function sets socket options associated with IPv6 sockets.
*                                                                       
*   INPUTS                                                                
*                                                                       
*       socketd                 The socket descriptor associated with
*                               the socket for which to set the option.
*       optname                 The option to set:
*
*                               IPV6_UNICAST_HOPS
*                               IPV6_RECVPKTINFO
*                               IPV6_RECVHOPLIMIT
*                               IPV6_PKTINFO
*                               IPV6_NEXTHOP
*                               IPV6_RECVTCLASS
*                               IPV6_TCLASS
*                               IPV6_RTHDR
*                               IPV6_RECVRTHDR
*                               IPV6_RECVHOPOPTS
*                               IPV6_HOPOPTS
*                               IPV6_RECVDSTOPTS
*                               IPV6_DSTOPTS
*
*       *optval                 The new value of the option to set.
*       optlen                  The length of the option to set.
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       NU_SUCCESS              The option was successfully set.                                         
*       NU_INVALID_OPTION       The option is invalid.
*       NU_INVAL                A parameter is invalid.
*                                                                       
*************************************************************************/
STATUS IP6_Set_Opt(INT socketd, INT optname, VOID *optval, INT optlen)
{
    STATUS  status;

    switch (optname)
    {
    case IPV6_UNICAST_HOPS:

        if ( (optval != NU_NULL) && (optlen >= (INT)(sizeof(INT))) )
            status = IP6_Set_IPV6_UNICAST_HOPS(socketd, *((INT16*)optval));
        else
            status = NU_INVAL;

        break;

    case IPV6_RECVPKTINFO:

        if ( (optval != NU_NULL) && (optlen >= (INT)(sizeof(INT))) )
            status = IP6_Set_IPV6_RECVPKTINFO(socketd, *((INT*)optval));
        else
            status = NU_INVAL;

        break;

    case IPV6_RECVHOPLIMIT:

        if ( (optval != NU_NULL) && (optlen >= (INT)(sizeof(INT))) )
            status = IP6_Set_IPV6_RECVHOPLIMIT(socketd, *((INT*)optval));
        else
            status = NU_INVAL;

        break;

    case IPV6_RECVTCLASS:

        if ( (optval != NU_NULL) && (optlen >= (INT)(sizeof(INT))) )
            status = IP6_Set_IPV6_RECVTCLASS(socketd, *((INT*)optval));
        else
            status = NU_INVAL;

        break;

    case IPV6_PKTINFO:

        if ( (optval != NU_NULL) && (optlen >= (INT)(sizeof(in6_pktinfo))) )
            status = IP6_Set_IPV6_PKTINFO(socketd, (in6_pktinfo*)optval, optlen);
        else
            status = NU_INVAL;

        break;

    case IPV6_NEXTHOP:

        if ( (optval != NU_NULL) || (optlen == 0) )
            status = IP6_Set_IPV6_NEXTHOP(socketd, (struct addr_struct*)optval, optlen);
        else
            status = NU_INVAL;

        break;

    case IPV6_TCLASS:

        if ( (optval != NU_NULL) && (optlen >= (INT)(sizeof(INT))) )
            status = IP6_Set_IPV6_TCLASS(socketd, *((INT*)optval));
        else
            status = NU_INVAL;

        break;

    case IPV6_RTHDR:

        if ( (optval != NU_NULL) || (optlen == 0) )
            status = IP6_Set_IPV6_RTHDR(socketd, (struct ip6_rthdr*)optval, optlen);
        else
            status = NU_INVAL;

        break;

    case IPV6_RECVRTHDR:

        if ( (optval != NU_NULL) && (optlen >= (INT)(sizeof(INT))) )
            status = IP6_Set_IPV6_RECVRTHDR(socketd, *((INT*)optval));
        else
            status = NU_INVAL;

        break;

    case IPV6_RECVHOPOPTS:

        if ( (optval != NU_NULL) && (optlen >= (INT)(sizeof(INT))) )
            status = IP6_Set_IPV6_RECVHOPOPTS(socketd, *((INT*)optval));
        else
            status = NU_INVAL;

        break;

    case IPV6_HOPOPTS:

        if ( (optval != NU_NULL) || (optlen == 0) )
            status = IP6_Set_IPV6_HOPOPTS(socketd, (struct ip6_hbh*)optval, 
                                          optlen);
        else
            status = NU_INVAL;

        break;

    case IPV6_RECVDSTOPTS:

        if ( (optval != NU_NULL) && (optlen >= (INT)(sizeof(INT))) )
            status = IP6_Set_IPV6_RECVDSTOPTS(socketd, *((INT*)optval));
        else
            status = NU_INVAL;

        break;

    case IPV6_DSTOPTS:

        if ( (optval != NU_NULL) || (optlen == 0) )
            status = IP6_Set_IPV6_DSTOPTS(socketd, (struct ip6_dest*)optval, 
                                          optlen);
        else
            status = NU_INVAL;

        break;

    case IPV6_RTHDRDSTOPTS:

        if ( (optval != NU_NULL) || (optlen == 0) )
            status = IP6_Set_IPV6_RTHDRDSTOPTS(socketd, (struct ip6_dest*)optval, 
                                               optlen);
        else
            status = NU_INVAL;

        break;

    case IPV6_V6ONLY:

        if ( (optval != NU_NULL) && (optlen >= (INT)(sizeof(INT))) )
            status = IP6_Set_IPV6_V6ONLY(socketd, *(INT*)optval);
        else
            status = NU_INVAL;

        break;

    case IPV6_CHECKSUM:

        if ( (optval != NU_NULL) && (optlen >= (INT)(sizeof(INT))) )
            status = IP6_Set_IPV6_CHECKSUM(socketd, *(INT*)optval);
        else
            status = NU_INVAL;

        break;

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)

    case IPV6_JOIN_GROUP:

        /* Join the Multicast group */
        if ( (optval != NU_NULL) && (optlen >= (INT)(sizeof(IP6_MREQ*))) )
            status = IP6_Process_Multicast_Listen(socketd, ((IP6_MREQ*)optval)->ip6_mreq_dev_index, 
                                            ((IP6_MREQ*)optval)->ip6_mreq_multiaddr, 
                                            MULTICAST_FILTER_EXCLUDE, NU_NULL, 0);
        else
            status = NU_INVAL;

        break;

    case IPV6_LEAVE_GROUP:

        /* Leave the multicast group */
        if ( (optval != NU_NULL) && (optlen >= (INT)(sizeof(IP6_MREQ*))) )
            status = IP6_Process_Multicast_Listen(socketd, ((IP6_MREQ*)optval)->ip6_mreq_dev_index, 
                                            ((IP6_MREQ*)optval)->ip6_mreq_multiaddr, 
                                            MULTICAST_FILTER_INCLUDE, NU_NULL, 0);
        else
            status = NU_INVAL;

        break;

    case IPV6_MULTICAST_IF:

        if ( (optval != NU_NULL) && (optlen >= (INT)(sizeof(INT32))) )
            status = IP6_Set_IPV6_MULTICAST_IF(socketd, *(INT32*)optval);
        else
            status = NU_INVAL;

        break;

    case IPV6_MULTICAST_HOPS:

        if ( (optval != NU_NULL) && (optlen >= (INT)(sizeof(INT))) )
            status = IP6_Set_IPV6_MULTICAST_HOPS(socketd, *(INT*)optval);
        else
            status = NU_INVAL;

        break;

    /* ATI_HOST2_RELEASE */
    case IPV6_MULTICAST_LOOP:

#endif

    default :

        status = NU_INVALID_OPTION;
    }

    return (status);

} /* IP6_Set_Opt */


