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
*       mem_bc.c
*
* DESCRIPTION
*
*       This file contains the implementation of MEM_Buffer_Cleanup.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       MEM_Buffer_Cleanup
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
*       MEM_Buffer_Cleanup
*
*   DESCRIPTION
*
*       This function takes a list header and moves all of the buffers off
*       of the list and onto the MEM_Buffer_Freelist.  Mainly used to
*       deallocate any unprocessed buffers when ever a TCP connection is
*       closed.
*
*   INPUTS
*
*       *hdr                    Pointer to the head of a linked list.
*
*   OUTPUTS
*
*      None.
*
*************************************************************************/
VOID MEM_Buffer_Cleanup(NET_BUFFER_HEADER *hdr)
{
    while (hdr->head)
        MEM_Buffer_Chain_Free(hdr, &MEM_Buffer_Freelist);

} /* MEM_Buffer_Cleanup */
