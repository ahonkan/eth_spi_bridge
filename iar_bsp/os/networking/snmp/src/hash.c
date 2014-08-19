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
*       hash.c                                                   
*
*   DESCRIPTION
*
*       This file contains all functions to create and maintain a
*       hash table.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       DefaultHashFunc
*       NewHash
*       DelHash
*       HashAdd
*       HashRemove
*       HashSearch
*       HashSize
*       CleanHash
*       NewHashTable
*       DelHashTable
*       NewBucketEntry
*       GetHashKey
*       ReturnBucketEntry
*       GetBucketEntry
*       DelBucketEntry
*       DelBucketEntries
*       CleanBucketEntries
*       GetEntryData
*       SetEntryData
*       KeyEntryCmp
*       KeyCmp
*
*   DEPENDENCIES
*
*       target.h
*       externs.h
*       snmp.h
*       agent.h
*       hash.h
*
*************************************************************************/

#include "networking/target.h"
#include "networking/externs.h"
#include "networking/snmp.h"
#include "networking/agent.h"
#include "networking/hash.h"

#if (INCLUDE_MIB_RMON1 == NU_TRUE)

#define DEFAULT_HTSIZE  53

HashTable_t     *curr_ht;

HashKey_t       DefaultHashFunc(UINT8 *, UINT32, UINT32);
HashKey_t       GetHashKey(HashTable_t *, UINT8 *, UINT32);
HashTable_t     *NewHashTable(UINT32, HashFunction_t);
BucketEntry_t   *NewBucketEntry(UINT8 *, UINT32);
BucketEntry_t   *ReturnBucketEntry(HashTable_t *, HashKey_t, UINT8 *,
                                   UINT32);
BucketEntry_t   *GetBucketEntry(HashTable_t *, HashKey_t, UINT8 *,
                                UINT32);
VOID            DelHashTable(HashTable_t *);
VOID            DelBucketEntry(HashTable_t *, HashKey_t, BucketEntry_t *);
VOID            DelBucketEntries(HashTable_t *, HashKey_t);
VOID            CleanBucketEntries(HashTable_t *, HashKey_t,
                                   HashCleanFunction_t);
VOID            *GetEntryData(BucketEntry_t *);
VOID            *SetEntryData(BucketEntry_t *, VOID *);
UINT32          HashSize(HashTable_t *);
UINT32          KeyEntryCmp(BucketEntry_t *, UINT8 *, UINT32);
INT32           KeyCmp(UINT8 *, UINT32, UINT8 *, UINT32);

extern  NU_MEMORY_POOL  System_Memory;

/************************************************************************
*
*   FUNCTION
*
*       DefaultHashFunc
*
*   DESCRIPTION
*
*       This is the default hash table function.  The user may implement
*       their own specific hash table function.
*
*   INPUTS
*
*       *key                    A pointer to the hash key.
*       n                       The size of the hash key.
*       htsize                  The size of the hash table.
*
*   OUTPUTS
*
*       hk                      The hash value.
*       -1                      Either the key is NU_NULL or the size of
*                               the key is 0.
*
*************************************************************************/
HashKey_t DefaultHashFunc(UINT8 *key, UINT32 n, UINT32 htsize)
{
    UINT16      *keys;
    HashKey_t   hk;

    if ( (key != NU_NULL) && (n != 0) )
    {
        keys = (UINT16 *)key;

        if (n > 6)
        {
            hk = (UINT32)(keys[0] + keys[1] + keys[2] + keys[3] +
                          keys[4] + keys[5]);
        }
        else
        {
            hk = (UINT32)(keys[0] + keys[1] + keys[2]);
        }

        hk = hk % htsize;
    }

    else
        hk = (HashKey_t) - 1;

    return (hk);

} /* DefaultHashFunc */

/************************************************************************
*
*   FUNCTION
*
*       NewHash
*
*   DESCRIPTION
*
*       This function returns a pointer to a new hash table with the
*       specified size of size using the hash function hf.
*
*   INPUTS
*
*       size                    The size of the new hash table.
*       hf                      The hash function for the new hash table.
*
*   OUTPUTS
*
*       Pointer to the new hash table.
*
*************************************************************************/
HashTable_t *NewHash(UINT32 size, HashFunction_t hf)
{
    return (NewHashTable(size, hf));

} /* NewHash */

/************************************************************************
*
*   FUNCTION
*
*       DelHash
*
*   DESCRIPTION
*
*       This function deletes the specified hash table.
*
*   INPUTS
*
*       *ht                     A pointer to the hash table to be deleted.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID DelHash(HashTable_t *ht)
{
    DelHashTable(ht);

} /* DelHash */

/************************************************************************
*
*   FUNCTION
*
*       HashAdd
*
*   DESCRIPTION
*
*       This function adds an entry to the specified hash table.
*
*   INPUTS
*
*       *ht                     A pointer to the hash table to which to
*                               add an entry.
*       *key                    A pointer to the hash key.
*       n                       The size of the hash key.
*       *data                   The data to add.
*
*   OUTPUTS
*
*       NU_NULL                 The data was not successfully added.
*       data                    The data was successfully added.
*
*************************************************************************/
VOID *HashAdd(HashTable_t *ht, UINT8 *key, UINT32 n, VOID *data)
{
    HashKey_t       hk;
    BucketEntry_t   *be;

    curr_ht = ht;

    hk = GetHashKey(ht, key, n);

    if ((be = ReturnBucketEntry(ht, hk, key, n)) == NU_NULL)
        data = NU_NULL;

    else if (GetEntryData(be) != NU_NULL)
        data = NU_NULL;

    else
        SetEntryData(be, data);

    return (data);

} /* HashAdd */

/************************************************************************
*
*   FUNCTION
*
*       HashRemove
*
*   DESCRIPTION
*
*       This function removes an entry from the hash table.
*
*   INPUTS
*
*       *ht                     A pointer to the hash table from which
*                               to remove an entry.
*       *key                    A pointer to the hash key.
*       n                       The size of the has key.
*
*   OUTPUTS
*
*       NU_NULL                 The data was not removed.
*       data                    The data was removed.
*
*************************************************************************/
VOID *HashRemove(HashTable_t *ht, UINT8 *key, UINT32 n)
{
    VOID            *data = NU_NULL;
    HashKey_t       hk;
    BucketEntry_t   *be;

    hk = GetHashKey(ht, key, n);

    if ((be = GetBucketEntry(ht, hk, key, n)) != NU_NULL)
    {
        data = GetEntryData(be);
        DelBucketEntry(ht, hk, be);
    }

    return (data);

} /* HashRemove */

/************************************************************************
*
*   FUNCTION
*
*       HashSearch
*
*   DESCRIPTION
*
*       This function searches the specified hash table for the
*       specified key.
*
*   INPUTS
*
*       *ht                     A pointer to the hash table to search.
*       *key                    A pointer to the key for which to search.
*       n                       The size of the key.
*
*   OUTPUTS
*
*       NU_NULL                 There is no entry for that key.
*       data                    The data associated with the entry for
*                               the key.
*
*************************************************************************/
VOID *HashSearch(HashTable_t *ht, UINT8 *key, UINT32 n)
{
    HashKey_t       hk;
    BucketEntry_t   *be;
    VOID            *data = NU_NULL;

    hk = GetHashKey(ht, key, n);

    if ((be = GetBucketEntry(ht, hk, key, n)) != NU_NULL)
        data = GetEntryData(be);

    return (data);

} /* HashSearch */

/************************************************************************
*
*   FUNCTION
*
*       HashSize
*
*   DESCRIPTION
*
*       This function returns the size of the hash table.
*
*   INPUTS
*
*       *ht                     A pointer to the hash table of which to
*                               return the size.
*
*   OUTPUTS
*
*       The size of the hash table.
*
*************************************************************************/
UINT32 HashSize(HashTable_t *ht)
{
    return (ht->Total);

} /* HashSize */

/************************************************************************
*
*   FUNCTION
*
*       CleanHash
*
*   DESCRIPTION
*
*       This function cleans the specified hash table using the specified
*       hash clean function.
*
*   INPUTS
*
*       *htp                    A pointer to the hash table to clean.
*       f                       The hash function with which to clean
*                               the hash table.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID CleanHash(HashTable_t *htp, HashCleanFunction_t f)
{
    HashKey_t hk;

    if (htp != NU_NULL)
    {
        for (hk = 0; hk < htp->Size && htp->Occupied > 0; hk++)
            CleanBucketEntries(htp, hk, f);
    }

} /* CleanHash */

/************************************************************************
*
*   FUNCTION
*
*       NewHashTable
*
*   DESCRIPTION
*
*       This function creates a new hash table with the size indicated.
*
*   INPUTS
*
*       size                    The indicated size of the new table.
*       hf                      The hash function to be used for the
*                               new table.
*
*   OUTPUTS
*
*       NU_NULL                 The new table was not created due to
*                               lack of resources.
*       htp                     The new table was created.
*
*************************************************************************/
HashTable_t *NewHashTable(UINT32 size, HashFunction_t hf)
{
    HashTable_t *htp = NU_NULL;

    if (size <= MAX_HASH_SIZE)
    {
        if (NU_Allocate_Memory(&System_Memory, (VOID**)&htp,
                               sizeof(HashTable_t),
                               NU_NO_SUSPEND) == NU_SUCCESS)
        {
            htp = TLS_Normalize_Ptr(htp);

            UTL_Zero(htp, sizeof(HashTable_t));

            if (size == 0)
                htp->Size = DEFAULT_HTSIZE;
            else
                htp->Size = size;

            if (hf == NU_NULL)
                htp->HashFunc = DefaultHashFunc;
            else
                htp->HashFunc = hf;

            htp->Occupied = 0;
            htp->Total = 0;
            htp->Table = NU_NULL;

            if (NU_Allocate_Memory(&System_Memory, (VOID**)&(htp->Table),
                                   (htp->Size * sizeof(BucketEntry_t *)),
                                   NU_NO_SUSPEND) == NU_SUCCESS)
            {
                htp->Table = TLS_Normalize_Ptr(htp->Table);

                UTL_Zero(htp->Table, htp->Size * sizeof(BucketEntry_t *));
            }
            else
            {
                NU_Deallocate_Memory((VOID*)htp);
                htp = NU_NULL;
            }
        }
    }

    return (htp);

} /* NewHashTable */

/************************************************************************
*
*   FUNCTION
*
*       DelHashTable
*
*   DESCRIPTION
*
*       This function deletes the specified hash table.
*
*   INPUTS
*
*       *htp                    A pointer to the hash table to delete.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID DelHashTable(HashTable_t *htp)
{
    HashKey_t   hk;

    if (htp != NU_NULL)
    {
        for (hk = 0; hk < htp->Size && htp->Occupied > 0; hk++)
            DelBucketEntries(htp, hk);

        htp->Size = 0;
        htp->Occupied = 0;
        htp->HashFunc = NU_NULL;

        NU_Deallocate_Memory((VOID*)htp->Table);
        htp->Table = NU_NULL;

        NU_Deallocate_Memory(htp);
    }

} /* DelHashTable */

/************************************************************************
*
*   FUNCTION
*
*       NewBucketEntry
*
*   DESCRIPTION
*
*       This function creates a new empty entry for a hash table.
*
*   INPUTS
*
*       *key                    The key associated with the new entry.
*       n                       The key size associated with the new
*                               entry.
*
*   OUTPUTS
*
*       NU_NULL                 The entry could not be created due to
*                               lack of resources.
*       bep                     The entry was successfully created.
*
*************************************************************************/
BucketEntry_t *NewBucketEntry(UINT8 *key, UINT32 n)
{
    BucketEntry_t   *bep = NU_NULL;

    if (NU_Allocate_Memory(&System_Memory, (VOID**)&bep,
                           sizeof(BucketEntry_t),
                           NU_NO_SUSPEND) == NU_SUCCESS)
    {
        bep = TLS_Normalize_Ptr(bep);

        UTL_Zero(bep, sizeof(BucketEntry_t));

        bep->Key = key;
        bep->KeySize = n;
        bep->Contents = NU_NULL;
        bep->Prev = NU_NULL;
        bep->Next = NU_NULL;
    }

    return (bep);

} /* NewBucketEntry */

/************************************************************************
*
*   FUNCTION
*
*       GetHashKey
*
*   DESCRIPTION
*
*       This function returns the hash key and size of the hash key for
*       a hash table.
*
*   INPUTS
*
*       *ht                     A pointer to the hash table.
*       *key                    A pointer into which to place the key.
*       n                       The size of the key.
*
*   OUTPUTS
*
*       HashKey_t               The hash key.
*       ht->HashFunc            The hash key.
*
*************************************************************************/
HashKey_t GetHashKey(HashTable_t *ht, UINT8 *key, UINT32 n)
{
    HashKey_t   data;

    if (ht != NU_NULL)
        data = ht->HashFunc(key, n, ht->Size);
    else
        data = (HashKey_t)-1;

    return (data);

} /* GetHashKey */

/************************************************************************
*
*   FUNCTION
*
*       ReturnBucketEntry
*
*   DESCRIPTION
*
*       This function returns the bucket entry associated with the key
*       in the given hash table.
*
*   INPUTS
*
*       *ht                     A pointer to the hash table to search.
*       hk                      The hash table key.
*       *key                    A pointer to the key associated with
*                               the entry.
*       n                       The size of the key.
*
*   OUTPUTS
*
*       NU_NULL                 There is no bucket entry for that key.
*       *be                     A pointer to the bucket entry for that
*                               key.
*
*************************************************************************/
BucketEntry_t *ReturnBucketEntry(HashTable_t *ht, HashKey_t hk,
                                 UINT8 *key, UINT32 n)
{
    BucketEntry_t   *be = NU_NULL;

    if (ht != NU_NULL)
    {
        if (ht->Table[hk] == NU_NULL)
        {
            if ((be = NewBucketEntry(key, n)) != NU_NULL)
            {
                be->Prev = NU_NULL;
                be->Next = NU_NULL;
                ht->Table[hk] = be;
                ht->Occupied++;
                ht->Total++;
            }
        }

        else
        {
            for (be = ht->Table[hk]; be != NU_NULL; be = be->Next)
                if (!KeyEntryCmp(be, key, n))
                    break;

            if (be == NU_NULL)
            {
                if ((be = NewBucketEntry(key, n)) != NU_NULL)
                {
                    be->Prev = NU_NULL;
                    be->Next = ht->Table[hk];
                    ht->Table[hk]->Prev = be;
                    ht->Table[hk] = be;
                    ht->Total++;
                }
            }
        }
    }

    return (be);

} /* ReturnBucketEntry */

/************************************************************************
*
*   FUNCTION
*
*       GetBucketEntry
*
*   DESCRIPTION
*
*       This function gets the bucket entry in the specified hash
*       table for the given hash table key.
*
*   INPUTS
*
*       *ht                     A pointer to the hash table from which
*                               to retrieve the data.
*       hk                      The hash table key.
*       *key
*       n
*
*   OUTPUTS
*
*       NU_NULL                 No data was retrieved.
*       be                      The bucket entry data.
*
*************************************************************************/
BucketEntry_t *GetBucketEntry(HashTable_t *ht, HashKey_t hk,
                              UINT8 *key, UINT32 n)
{
    BucketEntry_t   *be = NU_NULL;

    if (ht != NU_NULL)
    {
        for (be = ht->Table[hk]; be != NU_NULL; be = be->Next)
            if (!KeyEntryCmp(be, key, n))
                break;
    }

    return (be);

} /* GetBucketEntry */

/************************************************************************
*
*   FUNCTION
*
*       DelBucketEntry
*
*   DESCRIPTION
*
*       This function deletes a bucket entry from a given hash table.
*
*   INPUTS
*
*       *ht                     The hash table from which to delete the
*                               entry.
*       hk                      The hash table key.
*       *be                     A pointer to the bucket entry.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID DelBucketEntry(HashTable_t *ht, HashKey_t hk, BucketEntry_t *be)
{
    if (be->Prev != NU_NULL)
        be->Prev->Next = be->Next;
    else
        ht->Table[hk] = be->Next;

    if (be->Next != NU_NULL)
        be->Next->Prev = be->Prev;

    NU_Deallocate_Memory((VOID*)be);

    ht->Total--;

    if (ht->Table[hk] == NU_NULL)
        ht->Occupied--;

} /* DelBucketEntry */

/************************************************************************
*
*   FUNCTION
*
*       DelBucketEntries
*
*   DESCRIPTION
*
*       This function deletes all bucket entries in a given hash table
*       beginning at the specified hash table key.
*
*   INPUTS
*
*       *ht                     A pointer to the hash table of which to
*                               delete the entries.
*       hk                      The hash table key at which to begin
*                               deleting.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID DelBucketEntries(HashTable_t *ht, HashKey_t hk)
{
    BucketEntry_t *be1, *be2;

    if (ht != NU_NULL)
    {
        for (be2 = ht->Table[hk]; be2 != NU_NULL; be2 = be1)
        {
            be1 = be2->Next;
            NU_Deallocate_Memory((VOID*)be2);
        }
    }

} /* DelBucketEntries */

/************************************************************************
*
*   FUNCTION
*
*       CleanBucketEntries
*
*   DESCRIPTION
*
*       This function cleans the hash table using the specified hash
*       function.
*
*   INPUTS
*
*       *ht                     A pointer to the hash table to clean.
*       hk                      The hash table key.
*       f                       The function to use to clean the hash
*                               table.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID CleanBucketEntries(HashTable_t *ht, HashKey_t hk,
                        HashCleanFunction_t f)
{
    BucketEntry_t *be;

    if (ht != NU_NULL)
    {
        for (be = ht->Table[hk]; be != NU_NULL; be = be->Next)
            f(ht, GetEntryData(be));
    }

} /* CleanBucketEntries */

/************************************************************************
*
*   FUNCTION
*
*       GetEntryData
*
*   DESCRIPTION
*
*       This function returns the contents of a bucket entry.
*
*   INPUTS
*
*       *be                     A pointer to the bucket entry of which
*                               to return its contents.
*
*   OUTPUTS
*
*       NU_NULL                 The bucket entry is NU_NULL.
*       be->Contents            The data in the bucket entry.
*
*************************************************************************/
VOID *GetEntryData(BucketEntry_t *be)
{
    VOID    *data = NU_NULL;

    if (be != NU_NULL)
        data = be->Contents;

    return (data);

} /* GetEntryData */

/************************************************************************
*
*   FUNCTION
*
*       SetEntryData
*
*   DESCRIPTION
*
*       This function fills the given bucket entry with the data.
*
*   INPUTS
*
*       *be                     A pointer to the bucket entry to fill.
*       *data                   A pointer to the data to put in the
*                               bucket entry.
*
*   OUTPUTS
*
*       NU_NULL                 The bucket entry was NU_NULL.
*       be->Contents            The bucket entry was filled.
*
*************************************************************************/
VOID *SetEntryData(BucketEntry_t *be, VOID *data)
{
    if (be == NU_NULL)
        return (NU_NULL);

    return (be->Contents = data);

} /* SetEntryData */

/************************************************************************
*
*   FUNCTION
*
*       KeyEntryCmp
*
*   DESCRIPTION
*
*       This function determines whether the given bucket entry is
*       associated with the specified key.
*
*   INPUTS
*
*       *be                     A pointer to the bucket entry.
*       *key                    A pointer to the key.
*       n                       The size of the key.
*
*   OUTPUTS
*
*       1                       The bucket entry is associated with the
*                               key.
*       keyCmp                  The bucket entry is not associated with
*                               the key.
*
*************************************************************************/
UINT32 KeyEntryCmp(BucketEntry_t *be, UINT8 *key, UINT32 n)
{
    UINT32  success = 1;

    if (be != NU_NULL)
        success = (UINT32)KeyCmp(be->Key, be->KeySize, key, n);

    return (success);

} /* KeyEntryCmp */

/************************************************************************
*
*   FUNCTION
*
*       KeyCmp
*
*   DESCRIPTION
*
*       This function determines if two hash keys are equal.
*
*   INPUTS
*
*       *key1                   A pointer to the first key.
*       n1                      The size of the first key.
*       *key2                   A pointer to the second key.
*       n2                      The size of the second key.
*
*   OUTPUTS
*
*       -1                      The two keys are not equal.
*       1                       The two keys are equal.
*
*************************************************************************/
INT32 KeyCmp(UINT8 *key1, UINT32 n1, UINT8 *key2, UINT32 n2)
{
    UINT32  i;
    INT32   success = 1;

    i = 0;

    while ( (i < n1) && (i < n2) && (key1[i] == key2[i]) )
        i++;

    if ( (i == n1) || (i == n2) )
    {
        if (n1 != n2)
        {
            if (n1 < n2)
                success = -1;
            else
                success = 1;
        }
    }

    else if (key1[i] < key2[i])
        success = -1;

    else
        success = 1;

    return (success);

} /* KeyCmp */

#endif


