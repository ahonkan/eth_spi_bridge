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
*       nd6opts.h                                    
*                                                                                  
*   DESCRIPTION                                                              
*                                                                          
*       This file includes macros, data structures and function 
*       declarations for processing Neighbor Discovery options.
*                                                                          
*   DATA STRUCTURES                                                          
*                 
*       nd_opts   
*       nd_opt_hdr
*       nd_opt_prefix_info
*       nd_prefix
*
*   DEPENDENCIES                                                             
*                                                                          
*       None
*                                                                          
*************************************************************************/

#ifndef ND6OPTS_H
#define ND6OPTS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* Neighbor Discovery Options */
union nd_opts
{
    struct nd_opt_hdr                *nd_opt_array[10]; /* max = target address list */
    UINT8                            padN[2];
    struct
    {
        struct nd_opt_hdr            *zero;
        struct nd_opt_hdr            *src_lladdr;
        struct nd_opt_hdr            *tgt_lladdr;
        struct nd_opt_prefix_info    *pi_beg; /* multiple opts, start */
        struct nd_opt_prefix_info    *pi_end;/* multiple opts, end */
        struct nd_opt_rd_hdr         *rh;
        struct nd_opt_mtu            *mtu;
        struct nd_opt_hdr            *six;
        struct nd_opt_advinterval    *adv;
        struct nd_opt_homeagent_info *hai;
        struct nd_opt_hdr            *src_addrlist;
        struct nd_opt_hdr            *tgt_addrlist;
        struct nd_opt_hdr            *search;   /* multiple opts */
        struct nd_opt_hdr            *last; /* multiple opts */
        int                          done;
    } nd_opt_each;
};

#define nd_opts_src_lladdr      nd_opt_each.src_lladdr
#define nd_opts_tgt_lladdr      nd_opt_each.tgt_lladdr
#define nd_opts_pi              nd_opt_each.pi_beg
#define nd_opts_pi_end          nd_opt_each.pi_end
#define nd_opts_rh              nd_opt_each.rh
#define nd_opts_mtu             nd_opt_each.mtu
#define nd_opts_adv             nd_opt_each.adv
#define nd_opts_hai             nd_opt_each.hai
#define nd_opts_src_addrlist    nd_opt_each.src_addrlist
#define nd_opts_tgt_addrlist    nd_opt_each.tgt_addrlist
#define nd_opts_search          nd_opt_each.search
#define nd_opts_last            nd_opt_each.last
#define nd_opts_done            nd_opt_each.done

/* Neighbor discovery option header */
struct nd_opt_hdr 
{
    UINT8   nd_opt_type;
    UINT8   nd_opt_len;
};

/* Prefix Information */
struct nd_opt_prefix_info
{
    UINT8               nd_opt_pi_type;
    UINT8               nd_opt_pi_len;
    UINT8               nd_opt_pi_prefix_len;
    UINT8               nd_opt_pi_flags_reserved;
    UINT32              nd_opt_pi_valid_time;
    UINT32              nd_opt_pi_preferred_time;
    UINT32              nd_opt_pi_reserved2;
    struct id_struct    nd_opt_pi_prefix;
};

struct nd_prefix 
{
    struct DV_DEVICE_ENTRY  *ndpr_ifp;
    struct addr_struct      ndpr_prefix;    /* prefix */
    struct id_struct        ndpr_mask; /* netmask derived from the prefix */
    struct id_struct        ndpr_addr; /* address that is derived from the prefix */
    UINT32                  ndpr_vltime;    /* advertised valid lifetime */
    UINT32                  ndpr_pltime;    /* advertised preferred lifetime */
    UNSIGNED                ndpr_expire;    /* expiration time of the prefix */
    UNSIGNED                ndpr_preferred; /* preferred time of the prefix */
    UINT32                  ndpr_stateflags; /* actual state flags */
    UINT8                   ndpr_plen;
    int                     ndpr_refcnt;    /* reference counter from addresses */
};

VOID nd6_option_init(void *opt, int icmp6len, union nd_opts *ndopts);
INT nd6_options(union nd_opts *ndopts);
struct nd_opt_hdr *nd6_option(union nd_opts *ndopts);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif
