/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 1998 - 2007
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILENAME                                               
*
*       hash.h                                                   
*
*   DESCRIPTION
*
*       This file contains macros, data structures and function
*       definitions used in the file hash.c.
*
*   DATA STRUCTURES
*
*       BucketEntry_s
*       HashTable_s
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/

#ifndef HASH_H
#define HASH_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

#define MAX_HASH_SIZE           16384

typedef UINT32 HashKey_t;
typedef HashKey_t(*HashFunction_t) (UINT8 *, UINT32, UINT32);

typedef struct BucketEntry_s
{
    UINT8                   *Key;
    UINT32                  KeySize;
    VOID                    *Contents;
    struct BucketEntry_s    *Prev;
    struct BucketEntry_s    *Next;
} BucketEntry_t;

typedef struct HashTable_s
{
    UINT32                  Size;
    UINT32                  Occupied;
    UINT32                  Total;
    HashFunction_t          HashFunc;
    BucketEntry_t           **Table;
} HashTable_t;

typedef VOID (*HashCleanFunction_t) (HashTable_t *, VOID *);

HashTable_t    *NewHash(UINT32, HashFunction_t);
VOID           CleanHash(HashTable_t *, HashCleanFunction_t);
VOID           DelHash(HashTable_t *);
VOID           *HashAdd(HashTable_t *, UINT8 *, UINT32, VOID *);
VOID           *HashRemove(HashTable_t *, UINT8 *, UINT32);
VOID           *HashSearch(HashTable_t *, UINT8 *, UINT32);

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif
