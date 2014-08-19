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
*
* FILE NAME
*
*       sll.c
*
* COMPONENT
*
*       SLL
*
* DESCRIPTION
*
*       Linked list routines used by net components.
*
* DATA STRUCTURES
*
*       SLL_NODE
*       SLL_ROOT
*
* FUNCTIONS
*
*       SLL_Insert
*       SLL_Insert_Sorted
*       SLL_Enqueue
*       SLL_Dequeue
*       SLL_Remove
*       SLL_Push
*       SLL_Pop
*       SLL_Length
*
* DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

/*
 * These routines all reference a linked-list header, used to point to
 * a particular list.  Each entry in the list is prefixed by one pointer
 * values, namely a next:
 *
 *               struct *next;
 *               <Application-specific structure fields>
 *                           .
 *                           .
 *                           .
 *               <end of structure>
 *
 * Internally, the linked list routines operate on only the first entry in
 * the structure, the "next" or next link.
 *
 * A linked list header that identifies the beginning of a particular list
 * is a single pointer value.  This pointer value contains a pointer to
 * the first entry ("first") on the list.  The "last" value of the last
 * entry on the list points to the last entry on the list.
 */

/* This structure is used to overlay the parameters passed into the
   functions in this module. */
typedef struct SLL_Node_Struct
{
    struct SLL_Node_Struct      *next;
} SLL_NODE;

/* This structure is used to overlay the header parameter passed into the
   functions in this module. */
typedef struct SLL_Root_Struct
{
    SLL_NODE                    *first;
    SLL_NODE                    *last;
} SLL_ROOT;


/*************************************************************************
*
* FUNCTION
*
*       SLL_Insert
*
* DESCRIPTION
*
*       Insert an item into a linked list.
*
* INPUTS
*
*       *h                      A pointer to the list on which to
*                               insert the new item.
*       *i                      A pointer to the new item to insert.
*       *l                      A pointer to the item on the list after
*                               which to insert the new item.
*
* OUTPUTS
*
*       VOID*                   Pointer to item that was inserted.
*
*************************************************************************/
VOID *SLL_Insert(VOID *h, VOID *i, VOID *l)
{
    SLL_ROOT    *hdr = h;
    SLL_NODE    *item = i;
    SLL_NODE    *lpos = l;

    SLL_NODE    *prev = NU_NULL;
    SLL_NODE    *curr;

    INT         old_level;

    /*  Temporarily lockout interrupts to protect global variables. */
    old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* If the list is empty. */
    if(hdr->first == NU_NULL)
    {
        /* Set the next pointer of the new item to NULL. */
        item->next = NU_NULL;

        /* Update the first and last node pointer of the list. */
        hdr->first = item;
        hdr->last = item;
    }
    else
    {
        /* Find the position where the item is to be inserted. */
        for(curr = hdr->first; curr != NU_NULL && curr != lpos;
            curr = curr->next)
        {
            /* Keep track of the previous node. */
            prev = curr;
        }

        /* If the item is to be inserted at the start of the list. */
        if(prev == NU_NULL)
        {
            /* Update the first node pointer. */
            hdr->first = item;
        }
        /* Otherwise item is being inserted in the middle or end. */
        else
        {
            /* Update the next pointer of the previous item. */
            prev->next = item;
        }

        /* Update the next pointer of the new item. */
        item->next = lpos;

        /* If item inserted to the end of list. */
        if(curr == NU_NULL)
        {
            /* Update the last node pointer. */
            hdr->last = item;
        }
    }

    /* Restore the previous interrupt lockout level.  */
    NU_Local_Control_Interrupts(old_level);

    /* Return pointer to the item that was inserted. */
    return (item);
}

/*************************************************************************
*
* FUNCTION
*
*       SLL_Insert_Sorted
*
* DESCRIPTION
*
*       Insert an item into a linked list. The key used for sorting
*       must be unique for every item in the list. Duplicate items
*       are not allowed.
*
* INPUTS
*
*       *h                      A pointer to the list on which to
*                               insert the new item.
*       *i                      A pointer to the new item to insert.
*       *cmpfunc                Pointer to a comparison function that
*                               returns -1 if a < b, 1 if a > b or
*                               0 if a == b where a and b are the
*                               nodes of the linked list.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_INVALID_PARM         Item with the specified key already
*                               exists in the list.
*
*************************************************************************/
STATUS SLL_Insert_Sorted(VOID *h, VOID *i,
                         SLL_CMP_FUNC cmpfunc)
{
    INT         cmp_result;
    STATUS      status = NU_SUCCESS;
    SLL_ROOT    *hdr = h;
    SLL_NODE    *item = i;
    SLL_NODE    *prev = NU_NULL;
    SLL_NODE    *curr;
    INT         old_level;

    /*  Temporarily lockout interrupts to protect global variables. */
    old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* If the list is empty. */
    if(hdr->first == NU_NULL)
    {
        /* Set next pointer of item to NULL. */
        item->next = NU_NULL;

        /* Update the first and last pointers of list. */
        hdr->first = item;
        hdr->last = item;
    }
    else
    {
        /* Initially set result comparison to a negative value. */
        cmp_result = -1;

        /* Traverse the whole list. */
        for(curr = hdr->first; curr != NU_NULL; curr = curr->next)
        {
            /* Compare item being inserted with current item. */
            cmp_result = cmpfunc(item, curr);

            /* Check if item being inserted is smaller than the
               current item in the list. */
            if(cmp_result <= 0)
            {
                /* Sort position found. Break out of loop. */
                break;
            }

            /* Keep track of previous item. */
            prev = curr;
        }

        /* Check if a duplicate key was found. */
        if(cmp_result == 0)
        {
            /* Report error if duplicate key. */
            status = NU_INVALID_PARM;
        }
        else
        {
            /* Item is to be inserted at first position. */
            if(prev == NU_NULL)
            {
                /* Update the first node pointer. */
                hdr->first = item;
            }
            else
            {
                /* Otherwise update the next pointer of previous node. */
                prev->next = item;
            }

            /* Update the next pointer of item being inserted. */
            item->next = curr;

            /* If item is inserted at end of list. */
            if(curr == NU_NULL)
            {
                /* Also update the last node pointer. */
                hdr->last = item;
            }
        }
    }

    /* Restore the previous interrupt lockout level. */
    NU_Local_Control_Interrupts(old_level);

    /* Return pointer to item that was inserted. */
    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       SLL_Enqueue
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
VOID *SLL_Enqueue(VOID *h, VOID *i)
{
    SLL_ROOT    *hdr = h;
    SLL_NODE    *item = i;

    INT         old_level;

    item->next = NU_NULL;

    /*  Temporarily lockout interrupts to protect global variables. */
    old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* If the list is empty. */
    if(hdr->first == NU_NULL)
    {
        /* Add to start of list and also update last node pointer. */
        hdr->first = item;
    }
    else
    {
        /* Otherwise append to end of list. */
        hdr->last->next = item;
    }

    /* Update the last item pointer. */
    hdr->last = item;

    /*  Restore the previous interrupt lockout level.  */
    NU_Local_Control_Interrupts(old_level);

    /* Return pointer to the item that was added. */
    return (item);
}

/*************************************************************************
*
*   FUNCTION
*
*       SLL_Dequeue
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
VOID *SLL_Dequeue(VOID *h)
{
    SLL_ROOT        *hdr =h;
    SLL_NODE        *item;
    INT             old_level;

    /*  Temporarily lockout interrupts to protect global variables. */
    old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* Set to the first item in the list. */
    item = hdr->first;

    /* If list is not empty. */
    if (item != NU_NULL)
    {
        /* Update the first node pointer to point to next item. */
        hdr->first = item->next;

        /* If list is empty, also update last node pointer. */
        if (hdr->first == NU_NULL)
        {
            hdr->last = NU_NULL;
        }
    }

    /*  Restore the previous interrupt lockout level.  */
    NU_Local_Control_Interrupts(old_level);

    /* Set next pointer of the removed node to NULL. */
    if (item != NU_NULL)
        item->next = NU_NULL;

    /* Return the item that was removed. */
    return (item);
}

/*************************************************************************
*
*   FUNCTION
*
*       SLL_Remove
*
*   DESCRIPTION
*
*       Remove a node and return a pointer to the succeeding node.
*
*   INPUTS
*
*       *h                      A pointer to the list from which to
*                               remove the item.
*       *i                      A pointer to the item to remove.
*
*   OUTPUTS
*
*       VOID*                   A pointer to the node after the removed
*                               node.
*       NU_NULL                 The target node does not exist in the
*                               list.
*
*************************************************************************/
VOID *SLL_Remove(VOID *h , VOID *i)
{
    SLL_ROOT    *hdr = h;
    SLL_NODE    *item = i;
    SLL_NODE    *curr;
    SLL_NODE    *prev = NU_NULL;

    INT         old_level;

    /*  Temporarily lockout interrupts to protect global variables. */
    old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* Traverse the whole list. */
    for(curr = hdr->first; curr != NU_NULL; curr = curr->next)
    {
        /* If item being removed is found. */
        if(curr == item)
        {
            /* Leave the loop. */
            break;
        }

        /* Also keep track of the previous item. */
        prev = curr;
    }

    /* If the item was found. */
    if(curr != NU_NULL)
    {
        /* If item not the first item in the list. */
        if(prev != NU_NULL)
        {
            prev->next = curr->next;
        }
        /* Else it is the first item. */
        else
        {
            /* Update the first node pointer. */
            hdr->first = curr->next;
        }

        /* If the last item was removed. */
        if(curr->next == NU_NULL)
        {
            /* Set the last item to the 2nd last item. */
            hdr->last = prev;
        }
    }

    /*  Restore the previous interrupt lockout level.  */
    NU_Local_Control_Interrupts(old_level);

    /* Return the item which was removed. */
    return (curr);
}

/*************************************************************************
*
*   FUNCTION
*
*       SLL_Push
*
*   DESCRIPTION
*
*       Insert an item at the start of a linked list.
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
VOID *SLL_Push(VOID *h, VOID *i)
{
    SLL_ROOT        *hdr =h;
    SLL_NODE        *item = i;

    INT         old_level;

    /*  Temporarily lockout interrupts to protect global variables. */
    old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* If the list is empty. */
    if(hdr->first == NU_NULL)
    {
        /* Update last node pointer of list. */
        hdr->last = item;
    }
    /* Otherwise list is not empty. */
    else
    {
        /* Set next pointer of new node to first item in list. */
        item->next = hdr->first;

    }

    /* Update the first pointer of the list. */
    hdr->first = item;

    /*  Restore the previous interrupt lockout level.  */
    NU_Local_Control_Interrupts(old_level);

    /* Return pointer to item that was pushed. */
    return (item);
}

/*************************************************************************
*
*   FUNCTION
*
*       SLL_Pop
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
VOID *SLL_Pop(VOID *h)
{
    SLL_ROOT        *hdr = h;
    SLL_NODE        *item;

    INT         old_level;

    /*  Temporarily lockout interrupts to protect global variables. */
    old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* Set item to first item in the list. */
    item = hdr->first;

    /* If the list is not empty. */
    if (item != NU_NULL)
    {
        /* Update the first node pointer of the list. */
        hdr->first = item->next;

        /* If list is empty after pop. */
        if (hdr->first == NU_NULL)
        {
            /* Also set last node pointer to NULL. */
            hdr->last = NU_NULL;
        }
    }

    /*  Restore the previous interrupt lockout level.  */
    NU_Local_Control_Interrupts(old_level);

    /* Update the next pointer of popped node. */
    if (item != NU_NULL)
        item->next = NU_NULL;

    /* Return pointer to item which was popped. */
    return (item);
}

/*************************************************************************
*
*   FUNCTION
*
*       SLL_Length
*
*   DESCRIPTION
*
*       Returns the total number of nodes in the linked list.
*
*   INPUTS
*
*       *h                      A pointer to the header of the list.
*
*   OUTPUTS
*
*       Number of nodes in list.
*
*************************************************************************/
UINT32 SLL_Length(VOID *h)
{
    SLL_ROOT        *hdr = h;
    SLL_NODE        *item;
    UINT32          count = 0;
    INT             old_level;

    /*  Temporarily lockout interrupts to protect global variables. */
    old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    /* Set item to first item in list. */
    item = hdr->first;

    /* Loop until end of list. */
    while(item != NU_NULL)
    {
        /* Move to next item in list. */
        item = item->next;

        /* Increment node count. */
        count++;
    }

    /*  Restore the previous interrupt lockout level.  */
    NU_Local_Control_Interrupts(old_level);

    /* Return the node count of list. */
    return (count);
}
