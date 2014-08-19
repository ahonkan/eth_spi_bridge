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
*       ip_oc.c
*
* DESCRIPTION
*
*       This file contains the implementation of IP_Option_Copy.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       IP_Option_Copy
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/************************************************************************
*
*   FUNCTION
*
*       IP_Option_Copy
*
*   DESCRIPTION
*
*       Copies the options
*
*   INPUTS
*
*       *dip                    Destination IP packet header
*       *sip                    Source IP packet header
*
*   OUTPUTS
*
*       INT                     The length of the option data copied.
*
*************************************************************************/
INT IP_Option_Copy(IPLAYER *dip, IPLAYER *sip)
{
    UINT8       *src, *dest;
    INT         cnt, opt, optlen;

    /* Point to the first byte of option data in each of the IP packets. */
    src = (UINT8 *) (sip + 1);
    dest = (UINT8 *) (dip + 1);

    cnt = ((GET8(sip, IP_VERSIONANDHDRLEN_OFFSET) & 0x0f)  << 2) - IP_HEADER_LEN;

    for (; cnt > 0; cnt -= optlen, src += optlen)
    {
        opt = src[0];

        /* Stop when the EOL option is encountered. */
        if (opt == IP_OPT_EOL)
            break;

        /* Copy NOPs to preserve alignment constraints. */
        if (opt == IP_OPT_NOP)
        {
            *dest++ = IP_OPT_NOP;
            optlen = 1;
            continue;
        }
        else
            optlen = src[IP_OPT_OLEN];

        /* Truncate an option length that is too large. This should not occur. */
        if (optlen > cnt)
            optlen = cnt;

        /* If the copied bit is set then copy the option. */
        if (IP_OPT_COPIED(opt))
        {
            memcpy(dest, src, (unsigned int)optlen);
            dest += optlen;
        }
    }

    /* Pad the option list, if necessary, out to a 4-byte boundary. */
    for (optlen = (INT)(dest - (UINT8 *)(dip + 1)); optlen & 0x3; optlen++)
        *dest++ = IP_OPT_EOL;

    return (optlen);

} /* IP_Option_Copy */
