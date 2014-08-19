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
*        dll_i.c
*
* COMPONENT
*
*        Doubly sorted linked list component.
*
* DESCRIPTION
*
*         This file contains functions responsible for the maintenance
*         sorted doubly linked list.
*
* DATA STRUCTURES
*
*        None.
*
* FUNCTIONS
*
*        DLLI_Add_Node
*        DLLI_Remove_Node
*        DLLI_Search_Node
*        DLLI_Search_Next_Node
*
* DEPENDENCIES
*
*        nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

/*************************************************************************
*
* FUNCTION
*
*       DLLI_Add_Node
*
* DESCRIPTION
*
*       This function adds a node to the list ordered ascending with
*       respect to index.
*
* INPUTS
*
*       *hdr                    List header in which node is to be added.
*       *item                   The new node to be added.
*       index_type              type of index. Possible values to it are
*                               as follows:
*                                   1) DLLI_INDEX_32     for 32 bit index.
*                                   2) DLLI_INDEX_16     for 16 bit index.
*                                   3) DLLI_INDEX_8      for 8  bit index.
*
* OUTPUTS
*
*        NU_SUCCESS             The request was successful.
*        NU_INVALID_PARM        Parameter(s) was/were invalid or node
*                               with same index already exists.
*
*************************************************************************/
STATUS DLLI_Add_Node(VOID *hdr, VOID *item, UINT8 index_type)
{
    /* Node handle to parsing for searching purpose. */
    DLLI_NODE           *dummy;

    /* Variable to store index of node passed in. */
    UINT32              in_index;

    /* Variable to store index of current node while searching for the
       proper place. */
    UINT32              curr_index = 0;

    /* Status for returning error or success state. */
    STATUS              status = NU_SUCCESS;

    /* If parameter(s) is/are invalid then error otherwise proceed. */
    if ( (item == NU_NULL) || (hdr == NU_NULL) ||
         ((index_type != DLLI_INDEX_32) && (index_type != DLLI_INDEX_16) &&
          (index_type != DLLI_INDEX_8)) )
    {
        /* Setting status to error code. */
        status = NU_INVALID_PARM;
    }

    /* If we have valid parameters then proceed. */
    else
    {
        /* If index type is 32 bits. */
        if (index_type == DLLI_INDEX_32)
        {
            /* Getting index value in local variable. */
            in_index = ((DLLI_NODE *)item)->index.index32;
        }

        /* If index type is 16 bits. */
        else if (index_type == DLLI_INDEX_16)
        {
            /* Getting index value in local variable. */
            in_index = (UINT32)(((DLLI_NODE *)item)->index.index16);
        }

        /* It is obvious here that index_type would be DLLI_INDEX_8
           because parameters are valid and valid values for index_type
           are DLLI_INDEX_32, DLLI_INDEX_16 and DLLI_INDEX_8,
           so we need not to check. */
        else
        {
            /* Getting index value in local variable. */
            in_index = (UINT32)(((DLLI_NODE *)item)->index.index8);
        }

        /* Searching for the proper place because we have to maintain
           the sorted list. */

        dummy = ((DLLI_NODE*)hdr)->flink;

        while (dummy != NU_NULL)
        {
            /* Getting the index in local variable. */

            if (index_type == DLLI_INDEX_32)
                curr_index = dummy->index.index32;

            else if (index_type == DLLI_INDEX_16)
                curr_index = (UINT32)(dummy->index.index16);

            else
                curr_index = (UINT32)(dummy->index.index8);

            /* If we have reached the proper place then break through
               the loop. */
            if (curr_index >= in_index)
                break;

            /* Move forward in the list. */
            dummy = dummy->flink;
        }

        /* If we have the node with same index already present then return
           error code other wise add node. */
        if ( (dummy != NU_NULL) && (curr_index == in_index) )
        {
            status = NU_INVALID_PARM;
        }

        /* If all node have lesser index than new node then new
               node is to be inserted to at end of list. */
        else if (dummy == NU_NULL)
        {
            DLL_Enqueue(hdr, item);
        }

        /* Inserting at proper place. */
        else
        {
            DLL_Insert(hdr, item, dummy);
        }
    }

    /* Return status. */
    return (status);

} /* DLLI_Add_Node */

/*************************************************************************
*
* FUNCTION
*
*       DLLI_Remove_Node
*
* DESCRIPTION
*
*       This function removes a node from list ordered ascending with
*       respect to index.
*
* INPUTS
*
*       *hdr                    List header in which node is to be removed.
*       *index                  The index of node to be removed.
*       index_type              Type of index. Possible values to it are
*                               as follows:
*                                   1) DLLI_INDEX_32     for 32 bit index.
*                                   2) DLLI_INDEX_16     for 16 bit index.
*                                   3) DLLI_INDEX_8      for 8  bit index.
*
* OUTPUTS
*
*       Pointer to the removed node if it was present otherwise NU_NULL.
*
*************************************************************************/
VOID *DLLI_Remove_Node(VOID *hdr, const VOID *index, UINT8 index_type)
{
    /* Node handle for deleting purpose. */
    DLLI_NODE            *dummy;

    /* Searching for the node in list. */
    dummy = DLLI_Search_Node(hdr, index, index_type);

    /* If node to be deleted is found then remove and deallocate that
       node. */
    if (dummy != NU_NULL)
    {
        /* Removing the node from list. */
        DLL_Remove(hdr, dummy);
    }

    /* Return pointer to the removed node if present otherwise returning
       NU_NULL. */
    return (dummy);

} /* DLLI_Remove_Node */

/*************************************************************************
*
* FUNCTION
*
*       DLLI_Search_Node
*
* DESCRIPTION
*
*       This function searches a node from list ordered ascending with
*       respect to index.
*
* INPUTS
*
*       *hdr                    List header in which node is to be searched.
*       *index                  The index of node to be searched.
*       index_type              Type of index. Possible values to it are
*                               as follows:
*                                   1) DLLI_INDEX_32     for 32 bit index.
*                                   2) DLLI_INDEX_16     for 16 bit index.
*                                   3) DLLI_INDEX_8      for 8  bit index.
*
* OUTPUTS
*
*       VOID *                  Indicates that node with index passed in
*                               was found return pointer is pointing to
*                               found node.
*       NU_NULL                 Indicates that node with index passed in
*                               was not found.
*
*************************************************************************/
VOID *DLLI_Search_Node(const VOID *hdr, const VOID *index, UINT8 index_type)
{
    /* handle to the node for searching and returning. */
    DLLI_NODE            *dummy = NU_NULL;

    /* Variable to store index of node passed in. */
    UINT32              in_index;

    /* Variable to store index of current node while searching for the
       proper place. */
    UINT32              curr_index = 0;

    /* If parameter are invalid just return NU_NULL. */
    if ( (hdr != NU_NULL) && (index != NU_NULL) &&
         ((index_type == DLLI_INDEX_32) || (index_type == DLLI_INDEX_16) ||
          (index_type == DLLI_INDEX_8)) )
    {
        /* If index type is 32 bits. */
        if (index_type == DLLI_INDEX_32)
        {
            /* Getting index value in local variable. */
            in_index = (*((UINT32 *)index));
        }

        /* If index type is 16 bits. */
        else if (index_type == DLLI_INDEX_16)
        {
            /* Getting index value in local variable. */
            in_index = (UINT32)(*((UINT16 *)index));
        }

        /* It is obvious here that index_type would be DLLI_INDEX_16
           because parameters are valid, so need not to check. */
        else
        {
            /* Getting index value in local variable. */
            in_index = (UINT32)(*((UINT8 *)index));
        }

        /* Searching for the proper node because we have maintained
           the sorted list. */

        /* Initializing dummy pointer to start of the list to start
           finding the proper position. */
        dummy = ((DLLI_NODE*)hdr)->flink;

        /* Keep on searching for the proper node till the list for the
           proper place till we have not reach the proper place or list
           have more nodes. */
        while (dummy != NU_NULL)
        {
            /* Getting the index in local variable. */

            if (index_type == DLLI_INDEX_32)
                curr_index = dummy->index.index32;

            else if (index_type == DLLI_INDEX_16)
                curr_index = (UINT32)(dummy->index.index16);

            else
                curr_index = (UINT32)(dummy->index.index8);

            /* If we have reached the proper place then break through
               the loop. */
            if (curr_index >= in_index)
                break;

            /* Move forward in the list. */
            dummy = dummy->flink;
        }

        /* If we did not found the node having the same index as passed in
           then return NU_NULL. */
        if (curr_index != in_index)
        {
            /* Returning NU_NULL. */
            dummy = NU_NULL;
        }
    }

    /* Return Searched node if found otherwise NU_NULL. */
    return (dummy);

} /* DLLI_Search_Node */

/*************************************************************************
*
* FUNCTION
*
*       DLLI_Search_Next_Node
*
* DESCRIPTION
*
*       This function searches a node with least greater index from list
*       ordered ascending with respect to index. If node with greater
*       index is not present then it return pointer to first node in the
*       list.
*
* INPUTS
*
*       *hdr                    List header in which node is to be
*                               searched.
*       *index                  The index of node to be searched.
*       index_type              Type of index. Possible values to it are
*                               as follows:
*                                   1) DLLI_INDEX_32     for 32 bit index.
*                                   2) DLLI_INDEX_16     for 16 bit index.
*                                   3) DLLI_INDEX_8      for 8  bit index.
*
* OUTPUTS
*
*       VOID *                  Indicates that node with index passed in
*                               was found return pointer is pointing to
*                               found node.
*       NU_NULL                 Indicates that node with index passed in
*                               was not found.
*
*************************************************************************/
VOID *DLLI_Search_Next_Node(const VOID *hdr, const VOID *index, UINT8 index_type)
{
    /* handle to the node for searching and returning. */
    DLLI_NODE           *dummy;

    /* Variable to store index of node passed in. */
    UINT32              in_index;

    /* Variable to store index of current node while searching for the
       proper place. */
    UINT32              curr_index;

    /* If parameters are invalid then return NU_NULL. */
    if ( (hdr == NU_NULL) || (index == NU_NULL) ||
         ((index_type != DLLI_INDEX_32) && (index_type != DLLI_INDEX_16) &&
          (index_type != DLLI_INDEX_8)) )
    {
        /* Returning NU_NULL. */
        dummy = NU_NULL;
    }

    /* If we have valid parameters then proceeds. */
    else
    {
        /* If index type is 32 bits. */
        if (index_type == DLLI_INDEX_32)
        {
            /* Getting index value in local variable. */
            in_index = (*((UINT32 *)index));
        }

        /* If index type is 16 bits. */
        else if (index_type == DLLI_INDEX_16)
        {
            /* Getting index value in local variable. */
            in_index = (UINT32)(*((UINT16 *)index));
        }

        /* It is obvious here that index_type would be DLLI_INDEX_16
           because parameters are valid, so need not to check. */
        else
        {
            /* Getting index value in local variable. */
            in_index = (UINT32)(*((UINT8 *)index));
        }

        /* Initializing dummy pointer to start of the list.
           If index value is zero we need to return this value. */
        dummy = ((DLLI_NODE*)hdr)->flink;

        /* If index value is non-zero then search the node having
           greater/equal index value. If index value is zero return first
           node in the list. */
        if (in_index != 0)
        {
            while (dummy != NU_NULL)
            {
                /* Getting the index in local variable. */

                if (index_type == DLLI_INDEX_32)
                    curr_index = dummy->index.index32;

                else if (index_type == DLLI_INDEX_16)
                    curr_index = (UINT32)(dummy->index.index16);

                else
                    curr_index = (UINT32)(dummy->index.index8);

                /* If we have reached the proper place then break through
                   the loop. */
                if (curr_index >= in_index)
                    break;

                /* Move forward in the list. */
                dummy = dummy->flink;
            }

            /* If we did not find the next node, return first node in the
               list. */
            if (dummy == NU_NULL)
            {
                /* Returning first node of the list. */
                dummy = ((DLLI_NODE*)hdr)->flink;
            }
        }
    }

    /* Returning next node if found otherwise NU_NULL. */
    return (dummy);

} /* DLLI_Search_Next_Node */


