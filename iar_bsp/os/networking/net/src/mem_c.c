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
*       mem_c.c
*
* DESCRIPTION
*
*       This file contains the implementation of MEM_Cat.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       MEM_Cat
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/*************************************************************************
*
*   FUNCTION
*
*       MEM_Cat
*
*   DESCRIPTION
*
*       This function concatenates the source buffer chain and the
*       destination buffer chain.
*
*   INPUTS
*
*       *dest                   The destination chain.
*       *src                    The source chain. will be added to the
*                               end of the destination chain.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID MEM_Cat(NET_BUFFER *dest, NET_BUFFER *src)
{

    NET_BUFFER  *dest_work;

    if ( (!src) || (!dest) )
        return;

    /* Get work pointers */
    dest_work = dest;

    /* Find the last buffer in the chain. */
    while (dest_work->next_buffer)
        dest_work = dest_work->next_buffer;

    /* Add the src chain to the destination chain. */
    dest_work->next_buffer = src;

} /* MEM_Cat */
