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
*       nd6opts.c                                    
*                                                                               
*   DESCRIPTION                                                           
*                                                                       
*       This file contains functions used to validate incoming Neighbor
*       Discovery messages. 
*                                                                       
*   DATA STRUCTURES                                                       
*                                                                       
*       None
*                                                                       
*   FUNCTIONS                                                             
*           
*       nd6_option_init
*       nd6_options
*       nd6_option
*                                                                       
*   DEPENDENCIES                                                          
*                                                                       
*       target.h
*       externs.h
*       nd6opts.h
*                                                                       
*************************************************************************/

#include "networking/target.h"
#include "networking/externs.h"
#include "networking/nd6opts.h"

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       nd6_option_init
*                                                                         
*   DESCRIPTION                                                           
*
*       This function initializes the options validation data structure.
*                                                                                 
*   INPUTS                                                                
*                                             
*       *opt                    A pointer to the options.
*       icmp6len                The total length of all options.
*       *ndopts                 A pointer to the options validation data 
*                               structure.
*                                                                         
*   OUTPUTS                                                               
*
*       None
*
*************************************************************************/
VOID nd6_option_init(void *opt, int icmp6len, union nd_opts *ndopts)
{
    /* Zero out the options data structure */
    UTL_Zero(ndopts, sizeof(*ndopts));

    /* If there are no options, indicate so */
    if (icmp6len == 0) 
    {
        ndopts->nd_opts_done = 1;
        ndopts->nd_opts_search = NU_NULL;
        ndopts->nd_opts_last = NU_NULL;
    }

    else
    {
        /* Set the beginning pointer to options */
        ndopts->nd_opts_search = (struct nd_opt_hdr *)opt;

        /* Set the ending pointer to options */
        ndopts->nd_opts_last = (struct nd_opt_hdr *)(((UINT8 *)opt) + icmp6len);
    }

} /* nd6_option_init */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       nd6_options
*                                                                         
*   DESCRIPTION                                                           
*
*       This function validates the length of the options.
*                                                                                 
*   INPUTS                                                                
*                                     
*       *ndopts                 A pointer to the options validation data 
*                               structure.        
*                                                                         
*   OUTPUTS                                                               
*
*       0                       The options are all valid.
*       -1                      At least one option is invalid.
*
*************************************************************************/
INT nd6_options(union nd_opts *ndopts)  
{
    struct nd_opt_hdr *nd_opt;

    /* If there are no options or we are at the end of the list */
    if ( (!ndopts) || (!ndopts->nd_opts_last) || (!ndopts->nd_opts_search) )
        return (0);

    for (;;)
    {
        nd_opt = nd6_option(ndopts);

        /* Message validation requires that all included options have a 
         * length that is greater than zero.
         */
        if (!nd_opt && !ndopts->nd_opts_last) 
        {
            NLOG_Error_Log("Option length is zero", NERR_INFORMATIONAL, 
                           __FILE__, __LINE__);

            UTL_Zero(ndopts, sizeof(*ndopts));
            return (-1);
        }

        if (!nd_opt)
            break;

        switch (nd_opt->nd_opt_type) 
        {
        case IP6_ICMP_OPTION_SRC_ADDR:
        case IP6_ICMP_OPTION_TAR_ADDR:
        case IP6_ICMP_OPTION_MTU:
        case IP6_ICMP_OPTION_RED_HDR:

            /* Fill in the array value at index option type with the
             * type and length of the option.
             */
            if (!ndopts->nd_opt_array[nd_opt->nd_opt_type]) 
                ndopts->nd_opt_array[nd_opt->nd_opt_type] = nd_opt;
            break;

        case IP6_ICMP_OPTION_PREFIX:

            /* Fill in the array value at index option type with the
             * type and length of the option.
             */
            if (ndopts->nd_opt_array[nd_opt->nd_opt_type] == 0)
                ndopts->nd_opt_array[nd_opt->nd_opt_type] = nd_opt;

            ndopts->nd_opts_pi_end = (struct nd_opt_prefix_info *)nd_opt;
            break;

        default:
             /* Unknown options must be silently ignored,
              * to accommodate future extension to the protocol.
              */
            break;
        }
    }

    return (0);

} /* nd6_options */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       nd6_option
*                                                                         
*   DESCRIPTION                                                           
*
*       The function validates one option.
*                                                                                 
*   INPUTS                                                                
*                                         
*       *ndopts                 A pointer to the options validation 
*                               structure.    
*                                                                         
*   OUTPUTS                                                               
*
*       0                       The option is valid.
*       -1                      The option is invalid.
*
*************************************************************************/
struct nd_opt_hdr *nd6_option(union nd_opts *ndopts)
{
    struct nd_opt_hdr *nd_opt = NU_NULL;
    int olen;

    /* If there are no options or we are at the end of the list, return */
    if ( (ndopts) && (ndopts->nd_opts_last) && (ndopts->nd_opts_search) &&
         !(ndopts->nd_opts_done) )
    {
        /* Set the options pointer to the beginning of the list of options */
        nd_opt = ndopts->nd_opts_search;

        /* If the length of the options is greater than the pointer to the last
         * option, return
         */
        if ((CHAR *)&nd_opt->nd_opt_len >= (CHAR *)ndopts->nd_opts_last)
        {
            NLOG_Error_Log("Length of options is greater than the pointer to the last option", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);

            UTL_Zero(ndopts, sizeof(*ndopts));
            nd_opt = NU_NULL;
        }

        else
        {
            /* Set olen to the length of the option multiplied by 8, because
             * the length is measured in terms of 8 octets.
             */
            olen = nd_opt->nd_opt_len << 3;

            /* Message validation requires that all included options have a length 
             * that is greater than zero.
             */
            if (olen == 0) 
            {
                NLOG_Error_Log("Option length is zero", NERR_INFORMATIONAL, 
                               __FILE__, __LINE__);

                UTL_Zero(ndopts, sizeof(*ndopts));
                nd_opt = NU_NULL;
            }

            else
            {
                ndopts->nd_opts_search = (struct nd_opt_hdr *)((CHAR *)nd_opt + olen);
    
                /* Option overruns the end of buffer, invalid */
                if (ndopts->nd_opts_search > ndopts->nd_opts_last) 
                {       
                    NLOG_Error_Log("Option overruns the end of buffer", 
                                   NERR_INFORMATIONAL, __FILE__, __LINE__);

                    UTL_Zero(ndopts, sizeof(*ndopts));
                    nd_opt = NU_NULL;
                }

                /* Reached the end of options chain */
                else if (ndopts->nd_opts_search == ndopts->nd_opts_last) 
                {       
                    ndopts->nd_opts_done = 1;
                    ndopts->nd_opts_search = NULL;
                }
            }
        }
    }

    return (nd_opt);

} /* nd6_option */
