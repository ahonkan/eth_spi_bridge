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

/*****************************************************************************
*
*   FILE NAME
*
*       dll.c
*
*   DESCRIPTION
*
*       Linked list routines used by tcp/ip
*
*   DATA STRUCTURES
*
*       DLL_NODE
*
*   FUNCTIONS
*
*       DLL_Insert
*       DLL_Enqueue
*       DLL_Dequeue
*       DLL_Remove
*       DLL_Remove_Node
*
*   DEPENDENCIES
*
*       externs.h
*
*****************************************************************************/

#include "networking/externs.h"

/*
 * These routines all reference a linked-list header, used to point to
 * a particular list.  Each entry in the list is prefixed by two pointer
 * values, namely a forward pointer and a backward pointer:
 *
 *               struct *flink,*blink;
 *               <Application-specific structure fields>
 *                           .
 *                           .
 *                           .
 *               <end of structure>
 *
 * Internally, the linked list routines operate on only the first two
 * entries in the structure, the "flink" or forward link, and the "blink"
 * the backward link.
 *
 * A linked list header that identifies the beginning of a particular list
 * is a single pointer value.  This pointer value contains a pointer to
 * the first entry ("head") on the list.  The "blink" value of the first entry
 * on the list points to the last entry on the list.
 */

/* This structure is used to overlay the parameters passed into the functions
   in this module. */
typedef struct DLL_Node_Struct
{
    struct DLL_Node_Struct      *flink;
    struct DLL_Node_Struct      *blink;
} DLL_NODE;

/*************************************************************************
*
*   FUNCTION
*
*       DLL_Insert
*
*   DESCRIPTION
*
*       Insert an item into a linked list.
*
*   INPUTS
*
*       *h                      A pointer to the list on which to
*                               insert the new item.
*       *i                      A pointer to the new item to insert.
*       *l                      A pointer to the item on the list after
*                               which to insert the new item.
*
*   OUTPUTS
*
*      None.
*
*************************************************************************/
VOID *DLL_Insert(VOID *h, VOID *i, VOID *l)
{
    DLL_NODE   *hdr = h;
    DLL_NODE   *item = i;
    DLL_NODE   *lpos = l;
    INT        old_level;

    /*  Temporarily lockout interrupts to protect the global buffer variables. */
    old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* If this is the first entry to be placed in the link list, we can just set the
        hdr's flink and blink to the item and set the item's flink and blink to
        NU_NULL. */
    if (hdr->flink == NU_NULL)
    {
        item->flink = NU_NULL;
        item->blink = NU_NULL;

        hdr->flink = item;
        hdr->blink = item;
    }

    /* If lpos is NULL, add the item to the end of the list */
    else if (lpos == NU_NULL)
        DLL_Enqueue(hdr, item);

    else
    {
        /* Make item's flink point to lpos */
        item->flink = lpos;

        /* Set item's blink to point to the node that currently precedes lpos */
        item->blink = lpos->blink;

        /*  If there is already a node in front of lpos, we want its flink to
         *  point to item.
         */
        if (lpos->blink)
            lpos->blink->flink = item;

        /* Set lpos's blink to point at item */
        lpos->blink = item;

        /* If lpos was the first node in the linked list.  We need to make
         * hdr's flink point to item, which is the new first node.
         */
        if (lpos == hdr->flink)
            hdr->flink = item;
    }
    /*  Restore the previous interrupt lockout level.  */
    NU_Local_Control_Interrupts(old_level);

    return (item);

} /* DLL_Insert */

/*************************************************************************
*
*   FUNCTION
*
*       DLL_Enqueue
*
*   DESCRIPTION
*
*       Insert an item at the end of a linked list.
*
*   INPUTS
*
*       *h                      A pointer to the list onto which to
*                               enqueue the new item.
*       *i                      A pointer to the new item to enqueue.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID *DLL_Enqueue(VOID *h, VOID *i)
{
    DLL_NODE *hdr = h;
    DLL_NODE *item = i;
    INT old_level;

    /*  Temporarily lockout interrupts to protect the global buffer variables. */
    old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* Set item's flink to point at NULL */
    item->flink = NU_NULL;

    /*  If there is currently a node in the linked list, we want add
     *  item after that node
    */
    if (hdr->flink)
    {
        /* Make the last node's flink point to item */
        hdr->blink->flink = item;

        /* Make item's blink point to the old last node */
        item->blink = hdr->blink;

        /* Make hdr's blink point to the new last node, item */
        hdr->blink = item;
    }
    /*  if the linked list was empty, we want both the hdr's flink and
     *  the hdr's blink to point to item.  Both of item's links will
     *  point to NULL, as there are no other nodes in the list
    */
    else
    {
        hdr->flink = hdr->blink = item;
        item->blink = NU_NULL;
    }

    /*  Restore the previous interrupt lockout level.  */
    NU_Local_Control_Interrupts(old_level);

    return (item);

} /* DLL_Enqueue */

/*************************************************************************
*
*   FUNCTION
*
*       DLL_Dequeue
*
*   DESCRIPTION
*
*       Remove and return the first node in a linked list.
*
*   INPUTS
*
*       *h                      A pointer to the list from which to
*                               dequeue the item.
*
*   OUTPUTS
*
*       VOID*                   A pointer to the item dequeued.
*       NU_NULL                 The list is empty.
*
*************************************************************************/
VOID *DLL_Dequeue(VOID *h)
{
   DLL_NODE *hdr = h;
   DLL_NODE *ent;
   INT old_level;

   /*  Temporarily lockout interrupts to protect the global buffer variables. */
   old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

   /* Create a pointer to the first node in the linked list */
   ent = hdr->flink;

   /* If there is a node in the list we want to remove it. */
   if (ent)
   {
       /* Make the hdr point the second node in the list */
       hdr->flink = ent->flink;

       /*  If there was a second node, we want that node's blink to at 0. */
       if (hdr->flink)
           hdr->flink->blink = NU_NULL;

       /* Clear the next and previous pointers.*/
       ent->flink = ent->blink = NU_NULL;
   }

   /*  Restore the previous interrupt lockout level.  */
   NU_Local_Control_Interrupts(old_level);

   /* Return a pointer to the removed node */
   return (ent);

}  /* DLL_Dequeue */

/*************************************************************************
*
*   FUNCTION
*
*       DLL_Remove
*
*   DESCRIPTION
*
*       Remove a node and return a pointer to the next node in the list
*       following the node that was removed.
*
*   INPUTS
*
*       *h                      A pointer to the list from which to
*                               remove the item.
*       *i                      A pointer to the item to remove.
*
*   OUTPUTS
*
*       VOID*                   A pointer to the node in the list
*                               following the node that was removed.
*       NU_NULL                 The target node does not exist in the
*                               list or the last node was removed from
*                               the list.
*
*************************************************************************/
VOID *DLL_Remove(VOID *h, VOID *i)
{
    DLL_NODE     *hdr = h;
    DLL_NODE     *item = i;
    DLL_NODE     *ent, *retval;
    INT          old_level;

    /*  Temporarily lockout interrupts to protect the global buffer variables. */
    old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /*  Search the linked list until item is found or the end of the list
     *  is reached.
     */
    for (ent = hdr->flink;( (ent) && (ent != item) ); ent = ent->flink)
        ;

    /*  If item was not before reaching the end of the list, return a
     *  pointer to 0.
     */
    if (!ent)
    {
        /*  Restore the previous interrupt lockout level.  */
        NU_Local_Control_Interrupts(old_level);

        return (NU_NULL);
    }

    /* Keep a pointer to the node to be returned */
    retval = item->flink;

    /* If we are deleting the list head, this is just a dequeue operation */
    if (hdr->flink == item)
        DLL_Dequeue(hdr);

    /*  If we are deleting the list tail, we need to reset the tail pointer
     *  and make the new tail point forward to 0.
     */
    else if (hdr->blink == item)
    {
        hdr->blink = item->blink;
        hdr->blink->flink = NU_NULL;
    }

    /* We are removing this entry from the middle of the list */
    else
    {
        item->blink->flink = item->flink;
        item->flink->blink = item->blink;
    }

    /*  Restore the previous interrupt lockout level.  */
    NU_Local_Control_Interrupts(old_level);

    /* return a pointer to the removed node */
    return (retval);

} /* DLL_Remove */

/*************************************************************************
*
*   FUNCTION
*
*       DLL_Remove_Node
*
*   DESCRIPTION
*
*       Remove a node and return a pointer to the node that was removed.
*       The only difference between this routine and DLL_Remove is that
*       DLL_Remove returns a pointer to the next node in the list, and
*       this routine returns a pointer to the node that was removed from
*       the list.  The reason both routines are needed is because DLL_Remove
*       will return NU_NULL if there is no next node in the list or if the
*       node to be removed was not found in the list.  It is crucial for
*       some routines to know if the node was found in the list.  This
*       routine serves that purpose.
*
*   INPUTS
*
*       *h                      A pointer to the list from which to
*                               remove the item.
*       *i                      A pointer to the item to remove.
*
*   OUTPUTS
*
*       VOID*                   A pointer to the node in the list
*                               that was removed.
*       NU_NULL                 The target node does not exist in the
*                               list.
*
*************************************************************************/
VOID *DLL_Remove_Node(VOID *h, VOID *i)
{
    DLL_NODE     *hdr = h;
    DLL_NODE     *item = i;
    DLL_NODE     *ent, *retval;
    INT          old_level;

    /*  Temporarily lockout interrupts to protect the global buffer variables. */
    old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /*  Search the linked list until item is found or the end of the list
     *  is reached.
     */
    for (ent = hdr->flink;( (ent) && (ent != item) ); ent = ent->flink)
        ;

    /*  If item was not before reaching the end of the list, return a
     *  pointer to 0.
     */
    if (!ent)
    {
        /*  Restore the previous interrupt lockout level.  */
        NU_Local_Control_Interrupts(old_level);

        return (NU_NULL);
    }

    /* Keep a pointer to the node to be returned */
    retval = item;

    /* If we are deleting the list head, this is just a dequeue operation */
    if (hdr->flink == item)
        DLL_Dequeue(hdr);

    /*  If we are deleting the list tail, we need to reset the tail pointer
     *  and make the new tail point forward to 0.
     */
    else if (hdr->blink == item)
    {
        hdr->blink = item->blink;
        hdr->blink->flink = NU_NULL;
    }

    /* We are removing this entry from the middle of the list */
    else
    {
        item->blink->flink = item->flink;
        item->flink->blink = item->blink;
    }

    /*  Restore the previous interrupt lockout level.  */
    NU_Local_Control_Interrupts(old_level);

    /* return a pointer to the removed node */
    return (retval);

} /* DLL_Remove_Node */
