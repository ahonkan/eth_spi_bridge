/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2013
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/************************************************************************
*
*   FILE NAME
*
*       json_defs.h
*
*   COMPONENT
*
*       JSON
*
*   DESCRIPTION
*
*       This file contains JSON Generator and Parser private constants and
*       function declarations.
*
*   DATA STRUCTURES
*
*       _JSON_GEN_HANDLE        Private structure used to keep up with
*                               the current state of a JSON Generator.
*                               All API's will pass in the public type
*                               JSON_GEN_HANDLE which points to this
*                               structure.
*       _JSON_PARSER_HANDLE     Private structure used to keep up with
*                               the current state of a JSON Parser.
*
*   DEPENDENCIES
*
*       nu_json.h
*       nucleus.h
*       nu_kernel.h
*
************************************************************************/
#ifndef JSON_DEFS_H
#define JSON_DEFS_H

#include "networking/nu_json.h"
#include "nucleus.h"
#include "kernel/nu_kernel.h"

#ifdef          __cplusplus
/* C declarations in C++     */
extern          "C" {
#endif

/* Maximum length of a 64-bit integer including the sign and the
 * null-terminator. */
#define JSON_MAX_INTEGER_LENGTH     25

/* The start state for UTF-8 validation. */
#define JSON_UTF8_ACCEPT_STATE      0

/* This is a private structure for the JSON Generator. */
typedef struct json_gen_struct
{
    NU_SEMAPHORE    json_semaphore;     /* Semaphore to protect this structure. */
    INT             buffer_pos;         /* Pointer to null terminator location,
                                           which is the next place to start adding data
                                           assuming this is less than "buffer_size". */
    INT             buffer_size;        /* Total size of the "buffer". */
    CHAR            *buffer;            /* Pointer to the JSON data buffer. */
    CHAR            *name_buffer;       /* Buffer for the name. */
    INT             stack_level;        /* The current stack level the JSON
                                         * document is at. */
    INT             prev_data_index;    /* Used to keep up with the data's current state.
                                           This is JSON data passed in by the user. */
    UINT8           prev_prefix_index;  /* Used to keep up with the prefixes current state.
                                           This is generally some structural character or a quote. */
    UINT8           prev_suffix_index;  /* Used to keep up with the suffixes current state.
                                           This is generally some structural character or a quote. */
    BOOLEAN         name_is_present;    /* Used to indicate if a value is part of a pair or not */
    BOOLEAN         is_partial_string;  /* Use by our API's to remember if the user is adding a partial string. */

    /* Used indicate if a comma needs to be appended to the data at
     * the given stack level. */
    BOOLEAN         should_prepend_comma[JSON_MAX_DEPTH_LEVEL];

} _JSON_GEN_HANDLE;

/* Private structure to keep track of JSON Parser stack items. */
typedef struct json_parser_stack_item
{
    UINT8           prev_state;         /* Previous state before push. */
    UINT8           next_state;         /* Next state after push. */
} JSON_PARSER_STACK_ITEM;

/* This is a private structure for the JSON Parser. */
typedef struct json_parser_struct
{
    NU_SEMAPHORE    json_semaphore;     /* Semaphore to protect this structure. */
    INT             data_size;          /* Size of data in the buffer. */
    INT             data_pos;           /* Index of current position in data. */
    INT             buffer_size;        /* Total size of the "buffer". */
    CHAR            *buffer;            /* Pointer to the JSON data buffer. */
    INT             stack_level;        /* The current stack level the JSON
                                         * document is at. */
    INT             string_start_pos;   /* Start of string position in buffer. */
    UINT32          utf8_state;         /* UTF-8 state for string data. */
    UINT8           current_type;       /* Current token type. */
    UINT8           current_state;      /* Current state of the state machine. */
    BOOLEAN         name_is_present;    /* Is NU_TRUE if a name is present. */
    BOOLEAN         string_in_progress; /* String data is in progress. */
    BOOLEAN         record_value;       /* If enabled, value is recorded. */

    /* Current name/value read from the input. */
    CHAR            current_name[JSON_MAX_NAME_LENGTH + 1];
    CHAR            current_value[JSON_MAX_FLOAT_LENGTH + 1];

    /* Stack of states. */
    JSON_PARSER_STACK_ITEM stack_items[JSON_MAX_DEPTH_LEVEL];
} _JSON_PARSER_HANDLE;

/* JSON Generator Utility/Helper functions. */
STATUS JSON_Gen_Handle_To_Internal_Handle(JSON_GEN_HANDLE *handle, _JSON_GEN_HANDLE **internal_handle);
STATUS JSON_Append_Data(_JSON_GEN_HANDLE *internal_handle, CHAR *prefix, CHAR *data, CHAR *suffix);
STATUS JSON_Append_String(_JSON_GEN_HANDLE *internal_handle, CHAR *data, INT data_len);
CHAR *JSON_UInt64_To_String(UINT64 value, CHAR *string);
CHAR *JSON_Int64_To_String(INT64 value, CHAR *string);
#if CFG_NU_OS_KERN_PLUS_SUPPLEMENT_STATIC_TEST
BOOLEAN JSON_Is_Valid_Float(CHAR *value);
#endif

/* Macros for obtaining/releasing the semaphore. */
#define JSON_GEN_LCK_ENTER(internal_handle)   NU_Obtain_Semaphore(&((internal_handle)->json_semaphore), NU_SUSPEND)
#define JSON_GEN_LCK_EXIT(internal_handle)    NU_Release_Semaphore(&((internal_handle)->json_semaphore))

/* JSON Parser Utility/Helper functions. */
UINT64 JSON_String_To_UInt64(CHAR *string);
INT64 JSON_String_To_Int64(CHAR *string);
UINT32 JSON_Decode_Utf8(UINT32 *state, UINT32 *codep, UINT32 byte);
INT JSON_Widechar_To_Utf8(UINT32 wide_char, CHAR *dest);

#ifdef          __cplusplus
/* End of C declarations */
}
#endif  /* __cplusplus */


#endif /* JSON_DEFS_H */

