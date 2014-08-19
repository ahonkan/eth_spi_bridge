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
*       mem_bi.c
*
* DESCRIPTION
*
*       This file contains the implementation of MEM_Buffer_Insert.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       MEM_Update_Buffer_Lists
*       MEM_Buffer_Insert
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
*       MEM_Update_Buffer_Lists
*
*   DESCRIPTION
*
*       This function removes the first node from the source list and
*       places it at the tail of the destination list. Then returns a
*       pointer to the moved node.
*
*   INPUTS
*
*       *source                 Pointer to the list from which the node
*                               will be removed.
*       *dest                   Pointer to the list to which the node
*                               will be placed.
*
*   OUTPUTS
*
*       NET_BUFFER*             Pointer to the node that was moved.
*
*************************************************************************/
NET_BUFFER *MEM_Update_Buffer_Lists(NET_BUFFER_HEADER *source,
                                    NET_BUFFER_HEADER *dest)
{
    NET_BUFFER          *tmp_ptr;
    INT                 old_level;

    /*  Temporarily lockout interrupts to protect the global buffer variables. */
    old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* Get a pointer to the lists. */
    tmp_ptr = MEM_Buffer_Dequeue(source);

    /* Make sure there was a node to move. */
    if (tmp_ptr != NU_NULL)
        MEM_Buffer_Enqueue(dest, tmp_ptr);

    /*  Restore the previous interrupt lockout level.  */
    NU_Local_Control_Interrupts(old_level);

    return(tmp_ptr);

} /* MEM_Update_Buffer_Lists */

/*************************************************************************
*
*   FUNCTION
*
*       MEM_Buffer_Insert
*
*   DESCRIPTION
*
*       Insert an item into a linked list just before lpos and just after
*       fpos.
*
*   INPUTS
*
*       *hdr                    Pointer to the head of the net buffer
*                               list
*       *item                   Pointer to the item to insert in the
*                               net buffer list
*       *lpos                   Pointer to the last position in the net
*                               buffer list
*       *fpos                   Pointer to the first position in the net
*                               buffer list
*
*   OUTPUTS
*
*      NET_BUFFER*              Pointer to the net buffer item added to
*                               the net buffer list
*
*************************************************************************/
NET_BUFFER *MEM_Buffer_Insert(NET_BUFFER_HEADER *hdr, NET_BUFFER *item,
                              NET_BUFFER *lpos, NET_BUFFER *fpos)
{
    INT         old_level;

    /*  Temporarily lockout interrupts to protect the global buffer variables. */
    old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* Make item's next point to lpos */
    item->next = lpos;

    /* If fpos is NULL, then lpos is the first entry in the list.  Make
     * hdr's head point to item, which is the new first node.
     */
    if (fpos == NU_NULL)
        hdr->head = item;
    else
        /* Make fpos point to item. */
        fpos->next = item;

    /* If fpos is the tail of the list then we need to update the
       headers tail pointer to point at item. */
    if (fpos == hdr->tail)
        hdr->tail = item;

    /*  Restore the previous interrupt lockout level.  */
    NU_Local_Control_Interrupts(old_level);

    return(item);

} /* MEM_Buffer_Insert */
