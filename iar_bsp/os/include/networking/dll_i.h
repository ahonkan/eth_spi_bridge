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
*        dll_i.h
*
* COMPONENT
*
*        Doubly sorted linked list component.
*
* DESCRIPTION
*
*        This file contains utility for sorted list and data structure
*        required by these function.
*
* DATA STRUCTURES
*
*        DLLI_NODE
*
* DEPENDENCIES
*
*        None
*
*************************************************************************/
#ifndef DLL_NODE_I
#define DLL_NODE_I

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* _cplusplus */

/* Following macros holds the index type. */
#define DLLI_INDEX_32    4
#define DLLI_INDEX_16    2
#define DLLI_INDEX_8     1

/*
 * These routines all reference a linked-list header, used to point to
 * a particular list. Each entry in the list is prefixed by two pointer
 * values and index , namely a forward pointer and a backward pointer:
 *
 *               struct *flink,*blink;
 *               int index
 *               <Application-specific structure fields>
 *                           .
 *                           .
 *                           .
 *               <end of structure>
 *
 * Internally, the linked list routines operate on only the first three
 * entries in the structure, the "flink" or forward link,the "blink" the
 * backward link, and index value.
 *
 * A linked list header that identifies the beginning of a particular list
 * is a single pointer value.  This pointer value contains a pointer to
 * the first entry ("head") on the list.  The "blink" value of the first
 * entry on the list points to the last entry on the list.
 */

/* This structure is used to overlay the parameters passed into the
   functions in this module. */
typedef struct _dlli_node
{
    /* Forward link. */
    VOID    *flink;

    /* Backward link. */
    VOID    *blink;

    /* Union to hold index. */
    union node_index
    {
        /* 8 bits index. */
        UINT8   index8;

        /* 16 bits index. */
        UINT16  index16;

        /* 32 bits index. */
        UINT32  index32;
    }index;

}DLLI_NODE;

STATUS DLLI_Add_Node(VOID *hdr, VOID *item, UINT8 index_type);
VOID *DLLI_Remove_Node(VOID *hdr, const VOID *index, UINT8 index_type);
VOID *DLLI_Search_Node(const VOID *hdr, const VOID *index, UINT8 index_type);
VOID *DLLI_Search_Next_Node(const VOID *hdr, const VOID *index,
                            UINT8 index_type);
#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* DLL_NODE_I*/
