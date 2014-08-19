/*************************************************************************
*
*            Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILE NAME
*
*       reg_impl_mem.c
*
*   COMPONENT
*
*       Registry
*
*   DESCRIPTION
*
*       This file contains the implementation functions for getting and setting 
*       values in a memory based registry. 
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       REG_Memory_Has_Key
*       REG_Memory_Key_Is_Writable
*       REG_Memory_Num_Child_Keys
*       REG_Memory_Get_Boolean
*       REG_Memory_Set_Boolean
*       REG_Memory_Get_UINT8
*       REG_Memory_Set_UINT8
*       REG_Memory_Get_UINT16
*       REG_Memory_Set_UINT16
*       REG_Memory_Get_UINT32
*       REG_Memory_Set_UINT32
*       REG_Memory_Get_String
*       REG_Memory_Set_String
*       REG_Memory_Get_Bytes
*       REG_Memory_Set_Bytes
*       REG_Memory_Set_Writable
*
*   DEPENDENCIES
*
*       stdlib.h
*       string.h
*       reg_impl.h
*       reg_impl_mem_node.h
*
*************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/reg_impl.h"
#include "services/reg_impl_mem_node.h"

/*************************************************************************
*
*   FUNCTION
*
*       REG_Memory_Get_Node
*
*   DESCRIPTION
*
*       This function returns the node in the registry tree corresponding
*       to the given key.  If no such key exist in the registry, then
*       NULL is returned.
*
*   INPUT
*
*       key - the key whose node will be looked up.
*
*   OUTPUT
*
*       !NULL - the address of the tree node associated with the given 
*               key. 
*       NULL  - no node with the given key was found.
*
*************************************************************************/
static REG_Memory_Node* REG_Memory_Get_Node(const CHAR *key)
{
    #define END_OF_KEY(c) ((c) == '/' || (c) == '\0')

    REG_Memory_Node *   child = root;
    const CHAR *        key_begin = key;
    INT                 match;

    /* Loop until at the end of the key string */
    while (*key != '\0')
    {
        if (*key++ != '/')
        {
            return 0;
        }

        /* Check for trailing '/'. */
        if (*key == '\0')
            break;

        /* Find the matching child. */
        match = 0;
        while (1)
        {
            UINT child_key_len = strlen(child->key);
            if (strncmp(child->key, key, (size_t)child_key_len) == 0
                && END_OF_KEY(*(key + child_key_len)))
            {
                key += child_key_len;
                match = 1;
                break;
            }

            /* Check for last node in table */
            if (((child->value.u32 == 0xDEADBEEF) && (child->children != (REG_Memory_Node *)NU_NULL)) ||
                (child->children == (REG_Memory_Node *)0xDEADBEEF))
            {
                break;
            }

            /* Move to next node in the current table */
            child++;
        }

        /* Check to see if there was a match */
        if (match)
        {
            /* Make sure not at the end of the key */
            if ((*key != '\0') && ((*key == '/') && (*(key+1) != '\0')))
            {
                child = child->children;
            }
        }
        else
        {
            return 0;
        }
    }
 
    return (key == key_begin) ? 0 : child;

    #undef END_OF_KEY
}


/* The following functions are the low-level implementations of what is
 * defined in 'reg_api.c' and 'reg_api.h'.  The interface documentation
 * is identical.  As such, the documentation is only provided in the 
 * aformentioned files 
 */


static BOOLEAN REG_Memory_Has_Key(const CHAR *key)
{
    return REG_Memory_Get_Node(key) != 0;
}

static BOOLEAN REG_Memory_Key_Is_Writable(const CHAR *key)
{
    /* Suppress warnings */
    NU_UNUSED_PARAM(key);

    return NU_FALSE;
}

static STATUS REG_Memory_Num_Child_Keys(const CHAR *key, UINT *num)
{
    REG_Memory_Node *node = REG_Memory_Get_Node(key);
    STATUS status = REG_BAD_PATH;

    if (node != 0)
    {
        REG_Memory_Node *begin = node->children;
        UINT count = 0;

        /* Make sure there is at least one child */
        if (begin != 0)
        {
            while (1)
            {
                count += 1;

                /* Check for last node in table */
                if (((begin->value.u32 == 0xDEADBEEF) && (begin->children != (REG_Memory_Node *)NU_NULL)) ||
                        (begin->children == (REG_Memory_Node *)0xDEADBEEF))
                {
                    break;
                }

                begin++;
            }
        }

        *num = count;
        status = NU_SUCCESS;
    }

    return status;
}

static STATUS REG_Memory_Set_Bytes(const CHAR *key, 
                                       const UNSIGNED_CHAR *value,
                                       UINT length)
{
    /* Suppress warnings */
    NU_UNUSED_PARAM(key);
    NU_UNUSED_PARAM(value);
    NU_UNUSED_PARAM(length);

    return REG_NOT_WRITABLE;
}

static STATUS REG_Memory_Get_Bytes(const CHAR *key, UNSIGNED_CHAR *value,
                                        UINT length)
{
    STATUS status = REG_BAD_PATH;
    REG_Memory_Node *node = REG_Memory_Get_Node(key);

    if (node != 0)
    {
        memcpy(value, node->value.bytes, (size_t)length);
        status = NU_SUCCESS;
    }

    return status;
}

static STATUS REG_Memory_Get_Value(const CHAR *key, UNSIGNED_CHAR *value,
                                        UINT length)
{
    STATUS status = REG_BAD_PATH;
    REG_Memory_Node *node = REG_Memory_Get_Node(key);

    if (node != 0)
    {
        memcpy(value, &node->value, (size_t)length);
        status = NU_SUCCESS;
    }

    return status;
}

static STATUS REG_Memory_Set_Boolean(const CHAR *key, BOOLEAN value)
{
    /* Suppress warnings */
    NU_UNUSED_PARAM(key);
    NU_UNUSED_PARAM(value);

    return REG_NOT_WRITABLE;
}

static STATUS REG_Memory_Get_Boolean(const CHAR *key, BOOLEAN *value)
{
    STATUS status;
    UINT32 temp;

    status = REG_Memory_Get_Value(key, (UNSIGNED_CHAR*)&temp, sizeof(UINT32));

    if (status == NU_SUCCESS)
    {
        *value = (BOOLEAN)temp;
    }

    return status;
}

static STATUS REG_Memory_Set_UINT8(const CHAR *key, UINT8 value)
{
    /* Suppress warnings */
    NU_UNUSED_PARAM(key);
    NU_UNUSED_PARAM(value);

    return REG_NOT_WRITABLE;
}

static STATUS REG_Memory_Get_UINT8(const CHAR *key, UINT8 *value)
{
    STATUS status;
    UINT32 temp;

    status = REG_Memory_Get_Value(key, (UNSIGNED_CHAR*)&temp, sizeof(UINT32));

    if (status == NU_SUCCESS)
    {        
        *value = (UINT8)temp;
    }

    return status;
}

static STATUS REG_Memory_Set_UINT16(const CHAR *key, UINT16 value)
{
    /* Suppress warnings */
    NU_UNUSED_PARAM(key);
    NU_UNUSED_PARAM(value);

    return REG_NOT_WRITABLE;
}

static STATUS REG_Memory_Get_UINT16(const CHAR *key, UINT16 *value)
{
    STATUS status;
    UINT32 temp;

    status = REG_Memory_Get_Value(key, (UNSIGNED_CHAR*)&temp, sizeof(UINT32));

    if (status == NU_SUCCESS)
    {        
        *value = (UINT16)temp;
    }

    return status;
}

static STATUS REG_Memory_Set_UINT32(const CHAR *key, UINT32 value)
{
    /* Suppress warnings */
    NU_UNUSED_PARAM(key);
    NU_UNUSED_PARAM(value);

    return REG_NOT_WRITABLE;
}

static STATUS REG_Memory_Get_UINT32(const CHAR *key, UINT32 *value)
{
    return REG_Memory_Get_Value(key, (UNSIGNED_CHAR*)value, sizeof(UINT32));
}

static STATUS REG_Memory_Set_String(const CHAR *key, const CHAR *value,
                                        UINT length)
{
    /* Suppress warnings */
    NU_UNUSED_PARAM(key);
    NU_UNUSED_PARAM(value);
    NU_UNUSED_PARAM(length);

    return REG_NOT_WRITABLE;
}

static STATUS REG_Memory_Get_String(const CHAR *key, CHAR *value,
                                        UINT length)
{
    STATUS status = REG_BAD_PATH;
    REG_Memory_Node *node = REG_Memory_Get_Node(key);

    if (node != 0)
    {
        memcpy(value, node->value.str, (size_t)length);
        status = NU_SUCCESS;
    }

    return status;
}

static STATUS REG_Memory_Set_Writable(const CHAR *key, BOOLEAN is_writable)
{
    /* Suppress warnings */
    NU_UNUSED_PARAM(key);
    NU_UNUSED_PARAM(is_writable);

    return REG_NOT_WRITABLE;
}

static REG_IMPL memory_impl = {
     &REG_Memory_Has_Key,
     &REG_Memory_Key_Is_Writable,
     &REG_Memory_Num_Child_Keys,
     &REG_Memory_Set_Boolean,
     &REG_Memory_Get_Boolean,
     &REG_Memory_Set_UINT8,
     &REG_Memory_Get_UINT8,
     &REG_Memory_Set_UINT16,
     &REG_Memory_Get_UINT16,
     &REG_Memory_Set_UINT32,
     &REG_Memory_Get_UINT32,
     &REG_Memory_Set_String,
     &REG_Memory_Get_String,
     &REG_Memory_Set_Bytes,
     &REG_Memory_Get_Bytes,
     &REG_Memory_Set_Writable
};
REG_IMPL *impl = &memory_impl;

