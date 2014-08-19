/************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
************************************************************************/

/*************************************************************************
*
* FILE NAME
*
*       sll.h
*
* COMPONENT
*
*       SLL
*
* DESCRIPTION
*
*       Function prototypes for linked list routines used
*       by net components.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       None
*
************************************************************************/
#ifndef SLL_H
#define SLL_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* Define a type for the SLL comparison function. This function
   takes pointers to the two SLL nodes being compared and returns
   0, -1 or 1 if a == b, a < b or a > b respectively. */
typedef INT (*SLL_CMP_FUNC)(VOID *a, VOID *b);

/* Define a type for the SLL search function. This function
   takes a pointer to an SLL node and the data being searched
   and returns NU_TRUE on success and NU_FALSE on failure. */
typedef INT (*SLL_SEARCH_FUNC)(VOID *node, VOID *search_data);


/**** Function prototypes. ****/

VOID *SLL_Insert(VOID * h, VOID *i, VOID *l);
STATUS SLL_Insert_Sorted(VOID *h, VOID *i,
                         SLL_CMP_FUNC cmpfunc);
VOID *SLL_Enqueue(VOID *h, VOID *i);
VOID * SLL_Dequeue(VOID *h);
VOID *SLL_Remove(VOID *h , VOID *i);
VOID *SLL_Push(VOID *h, VOID *i);
VOID *SLL_Pop(VOID *h);
UINT32 SLL_Length(VOID *h);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* SLL_H */
