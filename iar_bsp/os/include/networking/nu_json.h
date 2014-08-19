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
*       nu_json.h
*
*   COMPONENT
*
*       JSON
*
*   DESCRIPTION
*
*       This file contains JSON Generator and Parser's public constants
*       and function declarations.
*
*   DATA STRUCTURES
*
*       JSON_GEN_HANDLE         Opaque structure used to keep track of
*                               the current state of a JSON Generator.
*
*   DEPENDENCIES
*
*       None
*
************************************************************************/
#ifndef NU_JSON_H
#define NU_JSON_H

#include "nucleus.h"

#ifdef          __cplusplus
/* C declarations in C++     */
extern          "C" {
#endif

/* Public JSON Token types */
#define JSON_TOKEN_TYPE_OBJECT      1
#define JSON_TOKEN_TYPE_ARRAY       2

/* JSON pair types */
#define JSON_TYPE_BOOLEAN           0
#define JSON_TYPE_STRING            1
#define JSON_TYPE_INTEGER           2
#define JSON_TYPE_FLOAT             3
#define JSON_TYPE_NULL              4

/* More types used by the JSON Parser in addition to the above ones. */
#define JSON_TYPE_UINTEGER          5
#define JSON_TYPE_OBJECT_START      6
#define JSON_TYPE_OBJECT_END        7
#define JSON_TYPE_ARRAY_START       8
#define JSON_TYPE_ARRAY_END         9
#define JSON_TYPE_UNKNOWN           10
#define JSON_TYPE_NONE              11

/* Flag taken by the "add string" API function. */
#define JSON_IS_PARTIAL             1

/* Maximum depth of nesting in the JSON structure. */
#define JSON_MAX_DEPTH_LEVEL        CFG_NU_OS_NET_JSON_MAX_NESTING_DEPTH

/* Maximum lengths of other data types. */
#define JSON_MAX_FLOAT_LENGTH       100
#define JSON_MAX_NAME_LENGTH        100

/* The minimum length of the JSON Parser buffer size. Please do not
 * modify this value as it is a hard limit due to the logic used
 * in the code. */
#define JSON_PARSER_MIN_BUFFER_SIZE 6

struct json_gen_struct;
struct json_parser_struct;

/* Opaque pointers to JSON Parser and Generator handles.
 * The elements of this data structure are inconsequential to the user. */
typedef struct json_gen_struct *JSON_GEN_HANDLE;
typedef struct json_parser_struct *JSON_PARSER_HANDLE;

typedef struct _json_string
{
    CHAR            *str;
    INT             length;
} JSON_STRING;

/* JSON Status Values.  */
#define NUF_JSON_BUFFER_FULL        -3000   /* The buffer used by the generator
                                             * or parser is full or would exceed
                                             * its size if data was added. */
#define NUF_JSON_DEPTH_EXCEEDED     -3001   /* Nesting/stack depth of JSON
                                             * data was exceeded. */
#define NUF_JSON_PARSING_ERROR      -3002   /* Invalid JSON given to parser. */
#define NUF_NAME_LENGTH_EXCEEDED    -3003   /* Maximum name length exceeded. */
#define NUF_JSON_UNEXPECTED_TYPE    -3004   /* Unexpected type encountered. */
#define NUF_FLOAT_LENGTH_EXCEEDED   -3003   /* Maximum name length exceeded. */
#define NUF_INVALID_UTF8_NAME       -3004   /* Name data is invalid UTF-8. */
#define NUF_INVALID_UTF8_DATA       -3005   /* String data is invalid UTF-8. */

/* JSON Generator User API. */
STATUS NU_JSON_Generator_Create(UINT32 buffer_size, JSON_GEN_HANDLE *handle);
STATUS NU_JSON_Generator_Destroy(JSON_GEN_HANDLE *handle);
STATUS NU_JSON_Generator_Start_Token(JSON_GEN_HANDLE *handle, CHAR *name, UINT8 token_type);
STATUS NU_JSON_Generator_End_Token(JSON_GEN_HANDLE *handle, UINT8 token_type);
STATUS NU_JSON_Generator_Add_Name(JSON_GEN_HANDLE *handle, CHAR *name);
STATUS NU_JSON_Generator_Add_String(JSON_GEN_HANDLE *handle, CHAR *jstr, UINT8 flags);
STATUS NU_JSON_Generator_Add_Boolean(JSON_GEN_HANDLE *handle, BOOLEAN value);
STATUS NU_JSON_Generator_Add_Int(JSON_GEN_HANDLE *handle, INT64 value);
STATUS NU_JSON_Generator_Add_UInt(JSON_GEN_HANDLE *handle, UINT64 value);
STATUS NU_JSON_Generator_Add_Float(JSON_GEN_HANDLE *handle, CHAR *value);
STATUS NU_JSON_Generator_Add_Null(JSON_GEN_HANDLE *handle);
STATUS NU_JSON_Generator_Get_Buffer(JSON_GEN_HANDLE *handle, CHAR **buffer, INT *buffer_length);
STATUS NU_JSON_Generator_Clear_Buffer(JSON_GEN_HANDLE *handle);

/* JSON Parser User API. */
STATUS NU_JSON_Parser_Create(INT buffer_size, JSON_PARSER_HANDLE *handle);
STATUS NU_JSON_Parser_Destroy(JSON_PARSER_HANDLE *handle);
STATUS NU_JSON_Parser_Set_Data(JSON_PARSER_HANDLE *handle, CHAR *json_data,
                               INT *json_data_length);
STATUS NU_JSON_Parser_Reset(JSON_PARSER_HANDLE *handle);
STATUS NU_JSON_Parser_Next(JSON_PARSER_HANDLE *handle, UINT8 *type,
                           UINT8 *stack_level, CHAR *name);
STATUS NU_JSON_Parser_Get_Info(JSON_PARSER_HANDLE *handle,
                               BOOLEAN compress,
                               INT *current_type_index,
                               INT *next_type_index,
                               INT *buffer_left);
STATUS NU_JSON_Parser_Get_Null(JSON_PARSER_HANDLE *handle);
STATUS NU_JSON_Parser_Get_Boolean(JSON_PARSER_HANDLE *handle, BOOLEAN *value);
STATUS NU_JSON_Parser_Get_Int(JSON_PARSER_HANDLE *handle, INT64 *value);
STATUS NU_JSON_Parser_Get_UInt(JSON_PARSER_HANDLE *handle, UINT64 *value);
STATUS NU_JSON_Parser_Get_Float(JSON_PARSER_HANDLE *handle, JSON_STRING *value);
STATUS NU_JSON_Parser_Get_String(JSON_PARSER_HANDLE *handle, JSON_STRING *value,
                                 UINT8 *flags);

/* Utility functions for string to double conversions. */
STATUS JSON_String_To_Double(CHAR *str, double *num);
STATUS JSON_Double_To_String(double num, JSON_STRING *result, INT decimal_places);

#ifdef          __cplusplus
/* End of C declarations */
}
#endif  /* __cplusplus */


#endif /* NU_JSON_H */

