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
*       ip6opt.c                                     
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This file contains functions used to build and traverse IPv6
*       extension headers.
*                                                                         
*   DATA STRUCTURES                                                       
*                                                                         
*       None
*                                                                         
*   FUNCTIONS                                                             
*                                                                         
*       inet6_opt_init
*       inet6_opt_append
*       inet6_opt_finish
*       inet6_opt_set_val
*       inet6_opt_next
*       inet6_opt_find
*       inet6_opt_get_val
*       ip6optlen
*                                                                         
*   DEPENDENCIES                                                          
*                                                                         
*       nu_net.h                                                        
*                                                                         
*************************************************************************/

#include "networking/nu_net.h"

STATIC INT ip6optlen(const UINT8 *opt, const UINT8 *lim);

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       inet6_opt_init
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function returns the number of bytes needed for the empty
*       extension header i.e. without any options.  If extbuf is not 
*       NULL it also initializes the extension header to have the correct 
*       length field.  In that case if the extlen value is not a positive 
*       (i.e., non-zero) multiple of 8 the function fails and returns -1.
*                                                                         
*   INPUTS                                                                
*                                                                         
*       *extbuf                 A pointer to the buffer containing the
*                               extension header being built.
*       extlen                  The length of the buffer extbuff.
*                                                                         
*   OUTPUTS                                                               
*                                                                         
*       Upon success, the function returns the number of bytes needed for
*       the empty extension header.
*
*       Otherwise, the function returns -1.
*
*************************************************************************/
INT inet6_opt_init(VOID *extbuf, UINT32 extlen)
{
    struct ip6_ext *ext = (struct ip6_ext *)extbuf;

    if (extlen % 8)
        return (-1);

    if (ext) 
    {
        if (extlen == 0)
            return (-1);

        ext->ip6e_len = (UINT8)((extlen >> 3) - 1);
    }

    return (2);     /* sizeof the next and the length fields */

} /* inet6_opt_init */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       inet6_opt_append
*                                                                         
*   DESCRIPTION                                                           
*
*       This function returns the updated total length taking into 
*       account adding an option with length 'len' and alignment 'align'.  
*       If extbuf is not NULL then, in addition to returning the length, 
*       the function inserts any needed pad option, initializes the option 
*       (setting the type and length fields) and returns a pointer to the 
*       location for the option content in databufp.  If the option does 
*       not fit in the extension header buffer the function returns -1.
*
*       Once inet6_opt_append() has been called the application can use 
*       the databuf directly, or use inet6_opt_set_val() to specify the 
*       content of the option.
*                                                                         
*   INPUTS                                                                
*                               
*       *extbuf                 A pointer to the buffer in which the
*                               extension header is being built.
*       extlen                  The length of the buffer extbuf.                                          
*       offset                  The length returned by inet6_opt_init() or a
*                               previous inet6_opt_append().  
*       type                    The 8-bit option type.  Must have a value 
*                               from 2 to 255, inclusive.  (0 and 1 are 
*                               reserved for the Pad1 and PadN options, 
*                               respectively.)
*       len                     The length of the option data, excluding
*                               the option type and option length fields.
*                               Must have a value between 0 and 255,
*                               inclusive, and is the length of the 
*                               option data that follows.
*       align                   Must have a value of 1, 2, 4, or 8.  The 
*                               align value cannot exceed the value of len.
*                                                                         
*   OUTPUTS                                                               
*                                                                         
*       Upon success, this function returns the updated total length of 
*       the buffer.
*
*       Otherwise, this function returns -1.
*
*************************************************************************/
INT inet6_opt_append(VOID *extbuf, UINT32 extlen, INT offset, UINT8 type,
                     UINT32 len, UINT8 align, VOID **databufp)
{
    INT             currentlen = offset, padlen = 0;
    UINT8 HUGE      *optp;

    /*
     * The option type must have a value from 2 to 255, inclusive.
     * (0 and 1 are reserved for the Pad1 and PadN options, respectively.)
     */
    if (type < 2)
        return (-1);

    /*
     * The option data length must have a value between 0 and 255,
     * inclusive, and is the length of the option data that follows.
     */
    if (len > 255)
        return (-1);

    /*
     * The align parameter must have a value of 1, 2, 4, or 8.
     * The align value can not exceed the value of len.
     */
    if ( (align != 1) && (align != 2) && (align != 4) && (align != 8) )
        return (-1);

    if (align > (UINT8)len)
        return (-1);

    /* Calculate the padding length. */
    currentlen += 2 + (INT)len;  /* 2 means "type + len" */

    if (currentlen % align)
        padlen = align - (currentlen % align);

    /* The option must fit in the extension header buffer. */
    currentlen += padlen;

    if ( (extlen) && (currentlen > (INT)extlen) )
        return (-1);

    if (extbuf) 
    {
        optp = (UINT8*)extbuf + offset;

        if (padlen == 1) 
        {
            /* insert a Pad1 option */
            *optp = IP6OPT_PAD1;
            optp++;
        }

        else if (padlen > 0) 
        {
            /* insert a PadN option for alignment */
            *optp++ = IP6OPT_PADN;
            *optp++ = (UINT8)(padlen - 2);
            memset(optp, 0, (unsigned int)(padlen - 2));
            optp += (padlen - 2);
        }

        *optp++ = type;
        *optp++ = (UINT8)len;

        *databufp = optp;
    }

    return (currentlen);

} /* inet6_opt_append */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       inet6_opt_finish
*                                                                         
*   DESCRIPTION                                                           
*
*       This function returns the updated total length taking into account 
*       the final padding of the extension header to make it a multiple of 
*       8 bytes.  If extbuf is not NULL the function also initializes the 
*       option by inserting a Pad1 or PadN option of the proper length.
*
*       If the necessary pad does not fit in the extension header buffer 
*       the function returns -1.
*                                                                         
*   INPUTS                                                                
*                               
*       *extbuf                 A pointer to the buffer in which the
*                               extension header is being built.
*       extlen                  The length of the buffer extbuf.                                          
*       offset                  The length returned by inet6_opt_init() 
*                               or inet6_opt_append().  
*                                                                         
*   OUTPUTS                                                               
*                                                                         
*       Upon success, this function returns the updated total length of 
*       the buffer.
*
*       Otherwise, this function returns -1.
*
*************************************************************************/
INT inet6_opt_finish(VOID *extbuf, UINT32 extlen, INT offset)
{
    INT     updatelen = offset > 0 ? (1 + ((offset - 1) | 7)) : 0;
    UINT8   *padp;
    INT     padlen;

    if (extbuf) 
    {       
        padlen = updatelen - offset;

        if (updatelen > (INT)extlen)
            return (-1);

        padp = (UINT8*)extbuf + offset;

        if (padlen == 1)
            *padp = IP6OPT_PAD1;

        else if (padlen > 0) 
        {
            *padp++ = IP6OPT_PADN;
            *padp++ = (UINT8)(padlen - 2);
            memset(padp, 0, (unsigned int)(padlen - 2));
        }
    }

    return (updatelen);

} /* inet6_opt_finish */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       inet6_opt_set_val
*                                                                         
*   DESCRIPTION                                                           
*
*       This function inserts data items of various sizes in the data 
*       portion of the option.  
*
*       The caller should ensure that each field is aligned on its natural
*       boundaries as described in Appendix B of [RFC-2460], but the 
*       function must not rely on the caller's behavior.  Even when the 
*       alignment requirement is not satisfied, inet6_opt_set_val should 
*       just copy the data as required.
*                                                                         
*   INPUTS                                                                
*                               
*       databuf                 A pointer returned by inet6_opt_append().  
*       offset                  Specifies where in the data portion of the 
*                               option the value should be inserted; the 
*                               first byte after the option type and length 
*                               is accessed by specifying an offset of zero.
*       *val                    A pointer to the data to be inserted.  
*       vallen                  The number of bytes to insert.
*                                                                         
*   OUTPUTS                                                               
*
*       The function returns the offset for the next field
*       (i.e., offset + vallen) which can be used when composing option 
*       content with multiple fields.
*
*************************************************************************/
INT inet6_opt_set_val(VOID *databuf, UINT32 offset, const VOID *val, INT vallen)
{
    memcpy((UINT8*)databuf + (UINT16)offset, val, (unsigned int)vallen);

    return((INT)offset + vallen);

} /* inet6_opt_set_val */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       inet6_opt_next
*                                                                         
*   DESCRIPTION                                                           
*
*       This function parses received option extension headers returning 
*       the next option.  The next option is returned by updating typep, 
*       lenp, and databufp.  This function returns the updated "previous" 
*       length computed by advancing past the option that was returned.  
*       This returned "previous" length can then be passed to subsequent 
*       calls to inet6_opt_next().  This function does not return any PAD1 
*       or PADN options.
*                                                                         
*   INPUTS                                                                
*                         
*       *extbuf                 A pointer to the buffer containing the
*                               extension header being built.
*       extlen                  The length of the extension header buffer.
*       offset                  Either zero (for the first option) or the 
*                               length returned by a previous call to 
*                               inet6_opt_next() or inet6_opt_find().
*                               It specifies the position where to continue 
*                               scanning the extension buffer.  
*       *typep                  The option type
*       *lenp                   The length of the option data (excluding 
*                               the option type and option length fields)
*       **databufp              Points to the data field of the option.        
*                                                                         
*   OUTPUTS                                                               
*
*       The function returns the updated previous length.
*
*       When there are no more options or if the option extension header
*       is malformed, the function returns -1.
*
*************************************************************************/
INT inet6_opt_next(VOID *extbuf, UINT32 extlen, INT offset, UINT8 *typep,
                   UINT32 *lenp, VOID **databufp)
{
    UINT8   *optp, *lim;
    INT     optlen;

    /* Validate extlen. XXX: is the variable really necessary?? */
    if ( (extlen == 0) || (extlen % 8) )
        return (-1);

    lim = (UINT8*)extbuf + (INT)extlen;

    /*
     * If this is the first time this function called for this options
     * header, simply return the 1st option.
     * Otherwise, search the option list for the next option.
     */
    if (offset == 0)
        optp = (UINT8*)((struct ip6_hbh *)extbuf + 1);

    else 
        optp = (UINT8*)extbuf + offset;

    /* Find the next option skipping any padding options. */
    while (optp < lim) 
    {
        switch (*optp) 
        {
            case IP6OPT_PAD1:

                optp++;
                break;

            case IP6OPT_PADN:

                if ((optlen = ip6optlen(optp, lim)) == 0)
                {
                    *databufp = NULL; /* for safety */
                    return (-1);
                }

                optp += optlen;
                break;

            default:    /* found */

                if ((optlen = ip6optlen(optp, lim)) == 0)
                {
                    *databufp = NULL; /* for safety */
                    return (-1);
                }

                *typep = *optp;
                *lenp = (UINT32)(optlen - 2);
                *databufp = optp + 2;

                return (INT)(optp + optlen - (UINT8*)extbuf);
        }
    }

    return (-1);

} /* inet6_opt_next */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       inet6_opt_find
*                                                                         
*   DESCRIPTION                                                           
*
*       This function is similar to inet6_opt_next(), except this function 
*       lets the caller specify the option type to be searched for instead 
*       of always returning the next option in the extension header.
*
*       If an option of the specified type is located, the function returns
*       the updated "previous" total length computed by advancing past the
*       option that was returned and past any options that didn't match the
*       type.  This returned "previous" length can then be passed to 
*       subsequent calls to inet6_opt_find() for finding the next occurrence
*       of the same option type.
*                                                                         
*   INPUTS                                                                
*                         
*       *extbuf                 A pointer to the buffer containing the
*                               extension header being built.
*       extlen                  The length of the extension header buffer.
*       offset                  Either zero (for the first option) or the 
*                               length returned by a previous call to 
*                               inet6_opt_next() or inet6_opt_find().
*                               It specifies the position where to continue 
*                               scanning the extension buffer.  
*       type                    The option type
*       *lenp                   The length of the option data (excluding 
*                               the option type and option length fields)
*       **databufp              Points to the data field of the option.        
*                                                                         
*   OUTPUTS                                                               
*
*       If an option of the specified type is located, the function returns
*       the updated "previous" total length.
*
*       If an option of the specified type is not located, the return 
*       value is -1.  If the option extension header is malformed, the 
*       return value is -1.
*
*************************************************************************/
INT inet6_opt_find(VOID *extbuf, UINT32 extlen, INT offset, UINT8 type,
                   UINT32 *lenp, VOID **databufp)
{
    UINT8   *optp, *lim;
    INT     optlen;

    /* Validate extlen. XXX: is the variable really necessary?? */
    if ( (extlen == 0) || (extlen % 8) )
        return (-1);

    lim = (UINT8*)((UINT8*)extbuf + (INT)extlen);

    /*
     * If this is the first time this function called for this options
     * header, simply return the 1st option.
     * Otherwise, search the option list for the next option.
     */
    if (offset == 0)
        optp = (UINT8*)((struct ip6_hbh *)extbuf + 1);

    else 
        optp = (UINT8*)extbuf + offset;

    /* Find the specified option */
    while (optp < lim) 
    {
        if ((optlen = ip6optlen(optp, lim)) == 0)
        {
            *databufp = NULL; /* for safety */
            return(-1);
        }

        if (*optp == type) 
        {
            *lenp = (UINT32)(optlen - 2);
            *databufp = optp + 2;

            return (INT)(optp + optlen - (UINT8*)extbuf);
        }

        optp += optlen;
    }

    return (-1);

} /* inet6_opt_find */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       inet6_opt_get_val
*                                                                         
*   DESCRIPTION                                                           
*
*       This function extracts data items of various sizes in the data 
*       portion of the option.  It is expected that each field is aligned 
*       on its natural boundaries as described in Appendix B of [RFC-2460], 
*       but the function must not rely on the alignment.
*
*   INPUTS                                                                
*                         
*       databuf                 A pointer returned by inet6_opt_next() or
*                               inet6_opt_find().  
*       val                     A pointer to the destination for the 
*                               extracted data.  
*       offset                  Specifies from where in the data portion 
*                               of the option the value should be extracted; 
*                               the first byte after the option type and 
*                               length is accessed by specifying an offset 
*                               of zero.
*                                                                         
*   OUTPUTS                                                               
*
*       The function returns the offset for the next field (i.e., offset +
*       vallen) which can be used when extracting option content with
*       multiple fields.
*
*************************************************************************/
INT inet6_opt_get_val(const VOID *databuf, UINT32 offset, VOID *val, INT vallen)
{
    /* we can't assume alignment here */
    memcpy(val, (UINT8 *)databuf + (INT)offset, (unsigned int)vallen);

    return ((INT)offset + vallen);

} /* inet6_opt_get_val */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       ip6optlen
*                                                                         
*   DESCRIPTION                                                           
*
*       Calculate the length of a given IPv6 option. Also checks if the 
*       option is safely stored in user's buffer according to the 
*       calculated length and the limitation of the buffer.
*
*   INPUTS                                                                
*                         
*       *opt                    A pointer to the beginning of the buffer.
*       *lim                    A pointer to the end of the buffer.
*                                                                         
*   OUTPUTS                                                               
*
*       The length of the IPv6 option.
*
*************************************************************************/
STATIC INT ip6optlen(const UINT8 *opt, const UINT8 *lim)    
{
    INT optlen;

    if (*opt == IP6OPT_PAD1)
        optlen = 1;

    else 
    {
        /* is there enough space to store type and len? */
        if ((opt + 2) > lim)
            return (0);

        optlen = *(opt + 1) + 2;
    }

    if ((opt + optlen) <= lim)
        return (optlen);

    return (0);

} /* ip6optlen */
