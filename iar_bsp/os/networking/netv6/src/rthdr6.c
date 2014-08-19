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
/* Portions of this program were written by: */
/*************************************************************************
*                                                                         
* Copyright (C) 1995, 1996, 1997, 1998, 1999, 2000 and 2001 WIDE Project.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
* 3. Neither the name of the project nor the names of its contributors
*    may be used to endorse or promote products derived from this 
*    software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
* PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS 
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
* THE POSSIBILITY OF SUCH DAMAGE.
*                                                                         
*************************************************************************/

/*************************************************************************
*                                                                       
*   FILE NAME                                                               
*                                                                               
*       rthdr6.c                                     
*                                                                               
*   DESCRIPTION                                                           
*                                                                       
*       This file contains routines used for building and traversing 
*       IPv6 Routing Headers.
*                                                                       
*   DATA STRUCTURES                                                       
*                                                                       
*       None
*                                                                       
*   FUNCTIONS                                                             
*           
*       inet6_rth_space
*       inet6_rth_init
*       inet6_rth_add
*       inet6_rth_reverse
*       inet6_rth_segments
*       inet6_rth_getaddr
*                                                                       
*   DEPENDENCIES                                                          
*                                                                       
*       nu_net.h
*                                                                       
*************************************************************************/

#include "networking/nu_net.h"

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       inet6_rth_space
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function returns the number of bytes required to hold a 
*       Routing header of the specified type containing the specified 
*       number of segments (addresses).
*
*       When the application uses ancillary data it must pass the 
*       returned length to CMSG_SPACE() to determine how much memory is 
*       needed for the ancillary data object (including the cmsghdr 
*       structure).
*                                                                         
*   INPUTS                                                                
*                                                                         
*       type                    The type of routing header.  Currently,
*                               only the Type 0 Routing header is
*                               supported - IPV6_RTHDR_TYPE_0.
*       segments                The number of segments that will be put
*                               in the routing header.  For an IPv6 Type 
*                               0 Routing header, the number of segments 
*                               must be between 0 and 127, inclusive.
*                                                                         
*   OUTPUTS                                                               
*                                                                         
*       The space required for the routing header.
*
*       If the return value is 0, then either the type of the Routing 
*       header is not supported by this implementation or the number of 
*       segments is invalid for this type of Routing header.
*
*************************************************************************/
UINT32 inet6_rth_space(INT type, INT segments)
{
    UINT32  rth_space;

    switch (type)
    {
        case IPV6_RTHDR_TYPE_0:

            /* The Type 0 Routing header supports up to 127 nodes */
            if ( (segments >= 0) && (segments <= 127) )
                rth_space = 
                    (segments << 4) + sizeof(struct ip6_rthdr0);

            /* The number of segments is invalid */
            else
                rth_space = 0;

            break;

        default:

            rth_space = 0;
            break;
    }

    return (rth_space);

} /* inet6_rth_space */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       inet6_rth_init
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function initializes the buffer pointed to by bp to contain 
*       a Routing header of the specified type and sets ip6r_len based 
*       on the segments parameter. 
*
*       When the application uses ancillary data the application must
*       initialize any cmsghdr fields.
*                                                                         
*   INPUTS                                                                
*                                                                         
*       *bp                     A pointer to the buffer to initialize 
*                               that will contain the routing header.
*       bp_len                  The total length of the final routing
*                               header as determined by inet6_rth_space().
*       type                    The type of routing header.  Currently,
*                               only the Type 0 Routing header is
*                               supported - IPV6_RTHDR_TYPE_0.
*       segments                The number of segments that will be put
*                               in the routing header.  For an IPv6 Type 
*                               0 Routing header, the number of segments 
*                               must be between 0 and 127, inclusive.
*                                                                         
*   OUTPUTS                                                               
*                                                                         
*       Upon success, the pointer to the buffer *bp is returned.
*       Upon an error, NU_NULL is returned.
*
*************************************************************************/
VOID *inet6_rth_init(VOID *bp, UINT32 bp_len, INT type, INT segments)
{
    struct ip6_rthdr0   *rthdr0;

    switch (type)
    {
        case IPV6_RTHDR_TYPE_0:

            rthdr0 = (struct ip6_rthdr0 *)bp;

            /* Compute the total length of the finished routing header */
            rthdr0->ip6r0_len = (UINT8)(segments << 1);

            /* Verify that the length of the buffer is big enough for all
             * segments.
             */
            if (rthdr0->ip6r0_len > (UINT8)bp_len)
                return (NU_NULL);

            /* Initialize the header fields for the routing header */
            rthdr0->ip6r0_nxt = 0;
            rthdr0->ip6r0_segleft = 0;
            rthdr0->ip6r0_type = IPV6_RTHDR_TYPE_0;
            rthdr0->ip6r0_reserved = 0;

            /* Return a pointer to the buffer */
            return (bp);

        default:

            return (NU_NULL);
    }

} /* inet6_rth_init */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       inet6_rth_add
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function adds the IPv6 address pointed to by addr to the 
*       end of the Routing header being constructed.
*                                                                         
*   INPUTS                                                                
*                                                                         
*       *bp                     A pointer to the buffer to initialize 
*                               that contains the routing header.
*       *addr                   A pointer to the IPv6 address to add
*                               to the routing header.
*                                                                         
*   OUTPUTS                                                               
*                                                                         
*       If successful, the return value of the function is 0.  
*       Upon an error the return value of the function is -1.
*
*************************************************************************/
INT inet6_rth_add(VOID *bp, const UINT8 *addr)
{
    struct ip6_rthdr    *rthdr = (struct ip6_rthdr*)bp;
    struct ip6_rthdr0   *rthdr0;

    switch(rthdr->ip6r_type)
    {
        case IPV6_RTHDR_TYPE_0:
        {
            rthdr0 = (struct ip6_rthdr0 *)bp;

            /* If the maximum number of segments have been added to the
             * routing header, return an error.
             */
            if (rthdr0->ip6r0_segleft == 127)
                return (-1);

            /* Copy the address into the end of the routing header */
            NU_BLOCK_COPY(&((CHAR*)rthdr0)[sizeof(struct ip6_rthdr0) + 
                          (rthdr0->ip6r0_segleft << 4)], addr, IP6_ADDR_LEN);

            /* Increment the number of segments in the routing header */
            rthdr0->ip6r0_segleft++;

            break;
        }

        default:

            return (-1);
    }

    return (0);

} /* inet6_rth_add */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       inet6_rth_reverse
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function takes a Routing header extension header (pointed 
*       to by the first argument) and writes a new Routing header that 
*       sends datagrams along the reverse of that route.  The function 
*       reverses the order of the addresses and sets the segleft member 
*       in the new Routing header to the number of segments.  Both 
*       arguments are allowed to point to the same buffer (that is, the 
*       reversal can occur in place).
*                                                                         
*   INPUTS                                                                
*                                                                         
*       *in                     A pointer to the source routing header.
*       *out                    A pointer to the target routing header.
*                                                                         
*   OUTPUTS                                                               
*                                                                         
*       The return value of the function is 0 on success, or -1 upon an
*       error.
*
*************************************************************************/
INT inet6_rth_reverse(const VOID *in, VOID *out)
{
    struct  ip6_rthdr *rth_in = (struct ip6_rthdr *)in;
    struct  ip6_rthdr0 *rth0_in, *rth0_out;
    INT     i, segments;
    UINT8   addr_tmp[IP6_ADDR_LEN], *addr1, *addr2;

    switch(rth_in->ip6r_type) 
    {
        case IPV6_RTHDR_TYPE_0:
            
            rth0_in = (struct ip6_rthdr0 *)in;
            rth0_out = (struct ip6_rthdr0 *)out;

            segments = rth0_in->ip6r0_len >> 1;

            /* we can't use memcpy here, since in and out may overlap */
            memmove((void *)rth0_out, (void *)rth0_in, 
                    sizeof(struct ip6_rthdr0) + (rth0_in->ip6r0_len << 3));

            rth0_out->ip6r0_segleft = (UINT8)segments;

            /* reverse the addresses */
            for (i = 0; i < segments / 2; i++) 
            {
                addr1 = 
                    (UINT8 *)(rth0_out) + sizeof(struct ip6_rthdr0) + 
                                          (i << 4);
                addr2 = 
                    (UINT8 *)(rth0_out) + sizeof(struct ip6_rthdr0) +
                                          ((segments - (i + 1)) << 4);

                NU_BLOCK_COPY(addr_tmp, addr1, IP6_ADDR_LEN);
                NU_BLOCK_COPY(addr1, addr2, IP6_ADDR_LEN);
                NU_BLOCK_COPY(addr2, addr_tmp, IP6_ADDR_LEN);
            }
            
            break;
    
        default:

            return (-1);    /* type not supported */
    }

    return (0);

} /* inet6_rth_reverse */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       inet6_rth_segments
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function returns the number of segments (addresses) 
*       contained in the Routing header described by bp. 
*                                                                         
*   INPUTS                                                                
*                                                                         
*       *bp                     A pointer to the routing header.
*                                                                         
*   OUTPUTS                                                               
*                                                                         
*       The return value of the function is 0 or greater on success, 
*       or -1 upon an error.
*
*************************************************************************/
INT inet6_rth_segments(const VOID *bp)
{
    struct ip6_rthdr    *rh = (struct ip6_rthdr *)bp;
    struct ip6_rthdr0   *rh0;
    INT                 addrs;

    switch(rh->ip6r_type) 
    {
        case IPV6_RTHDR_TYPE_0:

            rh0 = (struct ip6_rthdr0 *)bp;

            /*
             * Validation for a type-0 routing header.
             * Is this too strict?
             */
            if ( (rh0->ip6r0_len & 1) ||
                 (addrs = (rh0->ip6r0_len >> 1)) < (INT)(rh0->ip6r0_segleft) )
                return (-1);

            return (addrs);

        default:

            return (-1);    /* unknown type */
    }

} /* inet6_rth_segments */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       inet6_rth_getaddr
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function returns a pointer to the IPv6 address specified by
*       index (which must have a value between 0 and one less than the 
*       value returned by inet6_rth_segments()) in the Routing header 
*       described by bp.  An application should first call 
*       inet6_rth_segments() to obtain the number of segments in the 
*       Routing header.
*                                                                         
*   INPUTS                                                                
*                                                                         
*       *bp                     A pointer to the routing header.
*       index                   The index value of the IPv6 address
*                               to return.               
*                                                                         
*   OUTPUTS                                                               
*                                                                         
*       Upon an error, the return value is NU_NULL.
*
*************************************************************************/
UINT8 *inet6_rth_getaddr(const VOID *bp, INT index)
{
    struct ip6_rthdr    *rh = (struct ip6_rthdr *)bp;
    struct ip6_rthdr0   *rh0;
    INT                 rthlen, addrs;

    switch(rh->ip6r_type) 
    {
        case IPV6_RTHDR_TYPE_0:

            rh0 = (struct ip6_rthdr0 *)bp;
            rthlen = rh0->ip6r0_len << 3;
         
            /*
             * Validation for a type-0 routing header.
             * Is this too strict?
             */
            if ( (rthlen & 1) ||
                 (addrs = (rthlen >> 4)) < (INT)(rh0->ip6r0_segleft) )
                return (NU_NULL);

            if (index < 0 || addrs <= index)
                return (NU_NULL);

            return (((UINT8*)(rh0)) + sizeof(struct ip6_rthdr0) + 
                                      (index << 4));

        default:

            return (NU_NULL);   /* unknown type */
    }

} /* inet6_rth_getaddr */
