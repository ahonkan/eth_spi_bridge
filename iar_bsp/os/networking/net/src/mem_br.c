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
*       mem_br.c
*
* DESCRIPTION
*
*       This file contains the implementation of MEM_Buffer_Remove.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       MEM_Buffer_Remove
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
*       MEM_Buffer_Remove
*
*   DESCRIPTION
*
*       Removes a node from a buffer list.
*
*   INPUTS
*
*       *hdr                    Pointer to the net buffer header
*                               information
*       *item                   Pointer to the item to remove from the
*                               net buffer list
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID MEM_Buffer_Remove(NET_BUFFER_HEADER *hdr, NET_BUFFER *item)
{
    NET_BUFFER  *ent, *pre_ent = NU_NULL;
    INT         old_level;

    if ( (!hdr) || (!item) )
        return;

    /*  Temporarily lockout interrupts to protect the global buffer variables. */
    old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /*  Search the linked list until item is found or the end of the list
     *  is reached.
     */
    for (ent = hdr->head;((ent) && (ent != item)); pre_ent = ent, ent = ent->next)
        ;

    /* Make sure the item we are looking for was found. */
    if (ent)
    {

        /* If we're deleting the list head, this is just a dequeue operation */
        if (hdr->head == item)
            MEM_Buffer_Dequeue(hdr);
        else
            /*  If we are deleting the list tail, we need to reset the tail pointer
             *  and make the new tail point forward to 0.
             */
            if (hdr->tail == item)
            {
                hdr->tail = pre_ent;
                hdr->tail->next = NU_NULL;
            }
            else  /* We're removing this entry from the middle of the list */
            {
                if (pre_ent)
                    pre_ent->next = item->next;
            }

        /* Now clear the next pointer since this buffer is no
           longer on the list. */
        item->next = NU_NULL;
    }

    /*  Restore the previous interrupt lockout level.  */
    NU_Local_Control_Interrupts(old_level);

} /* MEM_Buffer_Remove */
