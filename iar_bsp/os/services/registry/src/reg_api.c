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
*       reg_api.c
*
*   COMPONENT
*
*       Registry
*
*   DESCRIPTION
*
*       This file contains the public API functions for getting and setting 
*       values in the registry.  The functions defined in this file are really
*       a very thin wrapper around the "true implementation" which is contained
*       in a set of function pointers held in the 'impl' variable.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       REG_Has_Key
*       REG_Key_Is_Writable
*       REG_Num_Child_Keys_Value
*       REG_Get_Boolean_Value
*       REG_Set_Boolean_Value
*       REG_Get_UINT8_Value
*       REG_Set_UINT8_Value
*       REG_Get_UINT16_Value
*       REG_Set_UINT16_Value
*       REG_Get_UINT32_Value
*       REG_Set_UINT32_Value
*       REG_Get_String_Value
*       REG_Set_String_Value
*       REG_Get_Bytes_Value
*       REG_Set_Bytes_Value
*       REG_Set_Writable_Value
*
*   DEPENDENCIES
*
*       reg_impl.h
*       reg_api.h
*
*************************************************************************/
#include <string.h>
#include "services/reg_impl.h"
#include "services/reg_api.h"

/* The following is a collection of simple macros for generating
   the implementation wrapper functions. */
#define DEF_IMPL_FUNC1(func_name)                                                           \
BOOLEAN func_name (const CHAR *key)                                                         \
{                                                                                           \
    return impl-> func_name (key);                                                          \
}

#define DEF_IMPL_FUNC2(func_name, value_type)                                               \
STATUS func_name (const CHAR *key, const CHAR *sub_key, value_type value)                   \
{                                                                                           \
    STATUS  status = REG_BAD_PATH;                                                          \
                                                                                            \
                                                                                            \
    if (sub_key)                                                                            \
    {                                                                                       \
        CHAR        reg_path[REG_MAX_KEY_LENGTH];                                           \
                                                                                            \
                                                                                            \
        (VOID)strncpy(reg_path, key, strlen(key) + 1);                                      \
        (VOID)strncat(reg_path, sub_key, REG_MAX_KEY_LENGTH-strlen(reg_path));              \
                                                                                            \
        status = impl-> func_name (reg_path, value);                                        \
    }                                                                                       \
    else                                                                                    \
    {                                                                                       \
        status = impl-> func_name (key, value);                                             \
    }                                                                                       \
                                                                                            \
    return (status);                                                                        \
}

#define DEF_IMPL_FUNC3(func_name, value_type)                                               \
STATUS func_name (const CHAR *key, const CHAR *sub_key, value_type value, UINT length)      \
{                                                                                           \
    STATUS  status = REG_BAD_PATH;                                                          \
                                                                                            \
                                                                                            \
    if (sub_key)                                                                            \
    {                                                                                       \
        CHAR        reg_path[REG_MAX_KEY_LENGTH];                                           \
                                                                                            \
                                                                                            \
        (VOID)strncpy(reg_path, key, strlen(key) + 1);                                      \
        (VOID)strncat(reg_path, sub_key, REG_MAX_KEY_LENGTH-strlen(reg_path));              \
                                                                                            \
        status = impl-> func_name (reg_path, value, length);                                \
    }                                                                                       \
    else                                                                                    \
    {                                                                                       \
        status = impl-> func_name (key, value, length);                                     \
    }                                                                                       \
                                                                                            \
    return (status);                                                                        \
}


/*************************************************************************
*
*   FUNCTION
*
*       REG_Has_Key
*
*   DESCRIPTION
*
*       This function determines whether a given key exist in the registry
*       or not.
*
*   INPUT
*
*       key - the key being checked for existence.
*
*   OUTPUT
*
*       NU_TRUE - the key exist.
*       NU_FALSE - the key does not exist. 
*
*************************************************************************/
DEF_IMPL_FUNC1(REG_Has_Key)


/*************************************************************************
*
*   FUNCTION
*
*       REG_Key_Is_Writable_Value
*
*   DESCRIPTION
*
*       This function determines whether a given key is writable or not.
*
*   INPUT
*
*       key - the key being checked for writability.
*
*   OUTPUT
*
*       NU_TRUE - the key is writable.
*       NU_FALSE - the key is not writable. 
*
*************************************************************************/
DEF_IMPL_FUNC1(REG_Key_Is_Writable)


/*************************************************************************
*
*   FUNCTION
*
*       REG_Num_Child_Keys_Value
*
*   DESCRIPTION
*
*       This function returns the number of child keys that a given
*       key has.
*
*   INPUT
*
*       key - the key whose number of children is being calculated.
*       sub_key - the key whose value is being retrieved sub directory
*       num - a pointer to an unsigned integer which will hold the
*             child count, once computed.
*
*   OUTPUT
*
*       NU_SUCCESS - the number of children was computed successfully.
*       REG_BAD_PATH - a bad key path was given.
*
*************************************************************************/
DEF_IMPL_FUNC2(REG_Num_Child_Keys_Value, UINT*)


/*************************************************************************
*
*   FUNCTION
*
*       REG_Get_Boolean
*
*   DESCRIPTION
*
*       This function returns the boolean value associated with
*       the given key.
*
*   INPUT
*
*       key - the key whose value is being retrieved.
*       sub_key - the key whose value is being retrieved sub directory
*       value - a pointer where the retrieved value is put.
*
*   OUTPUT
*
*       NU_SUCCESS - the value was retrieved successfully.
*       REG_BAD_PATH - a bad key path was given.
*
*************************************************************************/
DEF_IMPL_FUNC2(REG_Get_Boolean_Value, BOOLEAN*)


/*************************************************************************
*
*   FUNCTION
*
*       REG_Set_Boolean_Value
*
*   DESCRIPTION
*
*       This function sets the value associated with the given key
*       to the given value.
*
*   INPUT
*
*       key - the key whose value is being set.
*       sub_key - the key whose value is being retrieved sub directory
*       value - the new value to associate the key with.
*
*   OUTPUT
*
*       NU_SUCCESS - the value was set successfully.
*       REG_BAD_PATH - a bad key path was given.
*
*************************************************************************/
DEF_IMPL_FUNC2(REG_Set_Boolean_Value, BOOLEAN)


/*************************************************************************
*
*   FUNCTION
*
*       REG_Get_UINT8_Value
*
*   DESCRIPTION
*
*       This function returns the 8-bit unsigned integer value associated 
*       with the given key.
*
*   INPUT
*
*       key - the key whose value is being retrieved.
*       sub_key - the key whose value is being retrieved sub directory
*       value - a pointer where the retrieved value is put.
*
*   OUTPUT
*
*       NU_SUCCESS - the value was retrieved successfully.
*       REG_BAD_PATH - a bad key path was given.
*
*************************************************************************/
DEF_IMPL_FUNC2(REG_Get_UINT8_Value, UINT8*)


/*************************************************************************
*
*   FUNCTION
*
*       REG_Set_UINT8_Value
*
*   DESCRIPTION
*
*       This function sets the value associated with the given key
*       to the given value.
*
*   INPUT
*
*       key - the key whose value is being set.
*       value - the new value to associate the key with.
*
*   OUTPUT
*
*       NU_SUCCESS - the value was set successfully.
*       REG_BAD_PATH - a bad key path was given.
*
*************************************************************************/
DEF_IMPL_FUNC2(REG_Set_UINT8_Value, UINT8)


/*************************************************************************
*
*   FUNCTION
*
*       REG_Get_UINT16_Value
*
*   DESCRIPTION
*
*       This function returns the 16-bit unsigned integer value associated 
*       with the given key.
*
*   INPUT
*
*       key - the key whose value is being retrieved.
*       value - a pointer where the retrieved value is put.
*
*   OUTPUT
*
*       NU_SUCCESS - the value was retrieved successfully.
*       REG_BAD_PATH - a bad key path was given.
*
*************************************************************************/
DEF_IMPL_FUNC2(REG_Get_UINT16_Value,  UINT16*)


/*************************************************************************
*
*   FUNCTION
*
*       REG_Set_UINT16_Value
*
*   DESCRIPTION
*
*       This function sets the value associated with the given key
*       to the given value.
*
*   INPUT
*
*       key - the key whose value is being set.
*       value - the new value to associate the key with.
*
*   OUTPUT
*
*       NU_SUCCESS - the value was set successfully.
*       REG_BAD_PATH - a bad key path was given.
*
*************************************************************************/
DEF_IMPL_FUNC2(REG_Set_UINT16_Value,  UINT16)


/*************************************************************************
*
*   FUNCTION
*
*       REG_Get_UINT32_Value
*
*   DESCRIPTION
*
*       This function returns the 32-bit unsigned integer value associated 
*       with the given key.
*
*   INPUT
*
*       key - the key whose value is being retrieved.
*       value - a pointer where the retrieved value is put.
*
*   OUTPUT
*
*       NU_SUCCESS - the value was retrieved successfully.
*       REG_BAD_PATH - a bad key path was given.
*
*************************************************************************/
DEF_IMPL_FUNC2(REG_Get_UINT32_Value,  UINT32*)


/*************************************************************************
*
*   FUNCTION
*
*       REG_Set_UINT32_Value
*
*   DESCRIPTION
*
*       This function sets the value associated with the given key
*       to the given value.
*
*   INPUT
*
*       key - the key whose value is being set.
*       value - the new value to associate the key with.
*
*   OUTPUT
*
*       NU_SUCCESS - the value was set successfully.
*       REG_BAD_PATH - a bad key path was given.
*
*************************************************************************/
DEF_IMPL_FUNC2(REG_Set_UINT32_Value,  UINT32)


/*************************************************************************
*
*   FUNCTION
*
*       REG_Get_String_Value
*
*   DESCRIPTION
*
*       This function returns the character string value associated with
*       the given key.
*
*   INPUT
*
*       key - the key whose value is being retrieved.
*       value - a pointer where the retrieved value is put.
*
*   OUTPUT
*
*       NU_SUCCESS - the value was retrieved successfully.
*       REG_BAD_PATH - a bad key path was given.
*
*************************************************************************/
DEF_IMPL_FUNC3(REG_Get_String_Value,  CHAR*)


/*************************************************************************
*
*   FUNCTION
*
*       REG_Set_String_Value
*
*   DESCRIPTION
*
*       This function sets the value associated with the given key
*       to the given value.
*
*   INPUT
*
*       key - the key whose value is being set.
*       value - the new value to associate the key with.
*
*   OUTPUT
*
*       NU_SUCCESS - the value was set successfully.
*       REG_BAD_PATH - a bad key path was given.
*
*************************************************************************/
DEF_IMPL_FUNC3(REG_Set_String_Value,  const CHAR*)


/*************************************************************************
*
*   FUNCTION
*
*       REG_Get_Bytes_Value
*
*   DESCRIPTION
*
*       This function returns the byte sequence associated with
*       the given key.
*
*   INPUT
*
*       key - the key whose value is being retrieved.
*       value - a pointer where the retrieved value is put.
*
*   OUTPUT
*
*       NU_SUCCESS - the value was retrieved successfully.
*       REG_BAD_PATH - a bad key path was given.
*
*************************************************************************/
DEF_IMPL_FUNC3(REG_Get_Bytes_Value,   UNSIGNED_CHAR*)


/*************************************************************************
*
*   FUNCTION
*
*       REG_Set_Bytes_Value
*
*   DESCRIPTION
*
*       This function sets the value associated with the given key
*       to the given value.
*
*   INPUT
*
*       key - the key whose value is being set.
*       value - the new value to associate the key with.
*
*   OUTPUT
*
*       NU_SUCCESS - the value was set successfully.
*       REG_BAD_PATH - a bad key path was given.
*
*************************************************************************/
DEF_IMPL_FUNC3(REG_Set_Bytes_Value, const UNSIGNED_CHAR*)


/*************************************************************************
*
*   FUNCTION
*
*       REG_Set_Writable_Value
*
*   DESCRIPTION
*
*       This function marks the value associated with the given key
*       as writable.
*
*   INPUT
*
*       key - the key whose writability is being changed.
*       is_writable - specifies whether the given key should be writable
*                     or not.
*
*   OUTPUT
*
*       NU_SUCCESS - the write access for the key was modified 
*                     successfully.
*       REG_BAD_PATH - a bad key path was given.
*
*************************************************************************/
DEF_IMPL_FUNC2(REG_Set_Writable_Value, BOOLEAN)

