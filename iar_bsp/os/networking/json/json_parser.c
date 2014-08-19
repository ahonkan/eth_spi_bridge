/*************************************************************************
*
*               Copyright 2013 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       json_parser.c
*
* COMPONENT
*
*       JSON
*
* DESCRIPTION
*
*       The JSON Parser used to parser data which is in JSON format.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       NU_JSON_Parser_Create
*       NU_JSON_Parser_Destroy
*       NU_JSON_Parser_Set_Data
*       NU_JSON_Parser_Reset
*       NU_JSON_Parser_Next
*       NU_JSON_Parser_Get_Info
*       NU_JSON_Parser_Get_Null
*       NU_JSON_Parser_Get_Boolean
*       NU_JSON_Parser_Get_Int
*       NU_JSON_Parser_Get_UInt
*       NU_JSON_Parser_Get_Float
*       NU_JSON_Parser_Get_String
*       JSON_Parser_Handle_To_Internal_Handle
*       JSON_Parser_Get_Character_Class
*       JSON_Parser_Push_State
*       JSON_Parser_Pop_State
*       JSON_Parser_Get_Top_Of_Stack
*       JSON_Parser_Run_State_Machine
*
* DEPENDENCIES
*
*       nu_json.h
*       json_defs.h
*       nu_kernel.h
*       sockdefs.h
*       ncl.h
*       ctype.h
*
*************************************************************************/
#include "networking/nu_json.h"
#include "os/networking/json/json_defs.h"
#include "kernel/nu_kernel.h"
#include "os/include/networking/sockdefs.h"
#include "os/include/networking/ncl.h"
#include <ctype.h>

#if CFG_NU_OS_NET_JSON_INCLUDE_PARSER

/* Pointer to the JSON memory pool. */
extern NU_MEMORY_POOL *JSON_Memory;

/* Macro specifying an error state/class in the state machine. */
#define __      255

/* Character Classes to reduce the size of the state machine. */
typedef enum
{
    C_SPACE,    /* space/tab */
    C_WHITE,    /* other white-space characters */
    C_CURLO,    /* { */
    C_CURLC,    /* } */
    C_SQURO,    /* [ */
    C_SQURC,    /* ] */
    C_COLON,    /* : */
    C_COMMA,    /* , */
    C_QUOTE,    /* " */
    C_BSLSH,    /* \ */
    C_FSLSH,    /* / */
    C_DOT  ,    /* . */
    C_PLUS ,    /* + */
    C_MINUS,    /* - */
    C_ZERO ,    /* 0 */
    C_DIGIT,    /* 1-9 */
    C_ABCDF,    /* A,B,C,D,F */
    C_BIG_E,    /* E */
    C_A    ,    /* a */
    C_B    ,    /* b */
    C_E    ,    /* e */
    C_F    ,    /* f */
    C_L    ,    /* l */
    C_N    ,    /* n */
    C_R    ,    /* r */
    C_S    ,    /* s */
    C_T    ,    /* t */
    C_U    ,    /* u */
    C_ETC  ,    /* everything else */
    JSON_NUM_CLASSES
} JSON_Class;

/* Character Class Table which maps the first 128 ASCII characters
 * to one of the Character Classes defined in "JSON_Class". */
static const UINT8 JSON_Class_Table[128] = {
    __     , __     , __     , __     , __     , __     , __     , __     , /*  0 -  7 */
    C_ETC  , C_WHITE, C_WHITE, C_WHITE, C_WHITE, C_WHITE, C_ETC  , C_ETC  , /*  8 - 15 */
    __     , __     , __     , __     , __     , __     , __     , __     , /* 16 - 23 */
    __     , __     , __     , __     , __     , __     , __     , __     , /* 24 - 31 */

    C_SPACE, C_ETC  , C_QUOTE, C_ETC  , C_ETC  , C_ETC  , C_ETC  , C_ETC  , /* 32 - 39 */
    C_ETC  , C_ETC  , C_ETC  , C_PLUS , C_COMMA, C_MINUS, C_DOT  , C_FSLSH, /* 40 - 47 */
    C_ZERO , C_DIGIT, C_DIGIT, C_DIGIT, C_DIGIT, C_DIGIT, C_DIGIT, C_DIGIT, /* 48 - 55 */
    C_DIGIT, C_DIGIT, C_COLON, C_ETC  , C_ETC  , C_ETC  , C_ETC  , C_ETC  , /* 56 - 63 */

    C_ETC  , C_ABCDF, C_ABCDF, C_ABCDF, C_ABCDF, C_BIG_E, C_ABCDF, C_ETC  , /* 64 - 71 */
    C_ETC  , C_ETC  , C_ETC  , C_ETC  , C_ETC  , C_ETC  , C_ETC  , C_ETC  , /* 72 - 79 */
    C_ETC  , C_ETC  , C_ETC  , C_ETC  , C_ETC  , C_ETC  , C_ETC  , C_ETC  , /* 80 - 87 */
    C_ETC  , C_ETC  , C_ETC  , C_SQURO, C_BSLSH, C_SQURC, C_ETC  , C_ETC  , /* 88 - 95 */

    C_ETC  , C_A    , C_B    , C_ETC  , C_ETC  , C_E    , C_F    , C_ETC  , /*  96 - 103 */
    C_ETC  , C_ETC  , C_ETC  , C_ETC  , C_L    , C_ETC  , C_N    , C_ETC  , /* 104 - 111 */
    C_ETC  , C_ETC  , C_R    , C_S    , C_T    , C_U    , C_ETC  , C_ETC  , /* 112 - 119 */
    C_ETC  , C_ETC  , C_ETC  , C_CURLO, C_ETC  , C_CURLC, C_ETC  , C_ETC    /* 120 - 127 */
};

/* States of the JSON Parser state machine. */
typedef enum
{
    GO,         /* Initial start state. */
    OJ,         /* Inside an object. */
    AR,         /* Inside an array. */
    NA,         /* Match a name start. */
    NB,         /* Backslash in a name. */
    NM,         /* Match a name after it has been started. */
    CO,         /* Match a colon after the name */
    VL,         /* Match a value. */
    VE,         /* End of value. */
    ST,         /* Match a string. */
    SB,         /* Backslash in a string. */
    T1,         /* Match true: "t". */
    T2,         /* Match true: "tr". */
    T3,         /* Match true: "tru". */
    F1,         /* Match false: "f". */
    F2,         /* Match false: "fa". */
    F3,         /* Match false: "fal". */
    F4,         /* Match false: "fals". */
    N1,         /* Match null: "n". */
    N2,         /* Match null: "nu". */
    N3,         /* Match null: "nul". */
    DA,         /* Digit, minus matched. */
    DB,         /* Digit, first 1-9 digit matched. */
    DC,         /* Digit, first zero matched. */
    DD,         /* Digit, decimal-point matched. */
    DE,         /* Digit, E/e matched. */
    DF,         /* Digit, +/- matched. */
    DG,         /* Digit, optional 0-9 for DD. */
    DH,         /* Digit, optional 0-9 for DF/DE. */
    U1,         /* Unicode hex-digit 1. */
    U2,         /* Unicode hex-digit 2. */
    U3,         /* Unicode hex-digit 3. */
    U4,         /* Unicode hex-digit 4. */
    FI,         /* Finish state. */
   JSON_NUM_STATES
} JSON_State;

/* Actions of the JSON Parser state machine. */
typedef enum
{
    Xo = 200,   /* Start of object, goes to OJ. */
    XO = 201,   /* End of object, goes to VL/FI. */
    Xa = 202,   /* Start of array, goes to AR. */
    XA = 203,   /* End of array, goes to VL/FI. */
    Xn = 204,   /* Next character for name. */
    XN = 205,   /* End of name, goes to CO. */
    Xs = 206,   /* Start of string, goes to ST. */
    XS = 207,   /* End of string, goes to VE. */
    XT = 208,   /* Matched true, goes to VE. */
    XF = 209,   /* Matched false, goes to VE. */
    XL = 210,   /* Matched null, goes to VE. */
    XC = 211,   /* Matched a comma, goes to NA/VL. */
    Xm = 212,   /* Matched a minus, goes to DA. */
    Xz = 213,   /* Matched digit zero, goes to DC. */
    Xd = 214,   /* Matched digit 1-9, goes to DB. */
    XD = 215,   /* Matched end of integer/float. */
    Xe = 216,   /* Handle a backslashed escape character, goes to NM/ST. */
    XB = 217,   /* Handle the backslash character in a string, goes to SB. */
    Xu = 218,   /* Start of unicode hex-digits, goes to U2. */
    XU = 219,   /* End of unicode hex-digits, goes to NM/ST. */
} JSON_Action;

/* The JSON Parser State Table. */
const UINT8 JSON_State_Table[JSON_NUM_STATES][JSON_NUM_CLASSES] = {
/*                   white                                        ABCDF                                   */
/*              space |  {  }  [  ]  :  ,  "  \  /  .  +  -  0 1-9 |  E  a  b  e  f  l  n  r  s  t  u  *  */
/* GO:Start   */ {GO,GO,Xo,__,Xa,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__},
/* OJ:Object  */ {OJ,OJ,__,XO,__,__,__,__,NM,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__},
/* AR:Array   */ {AR,AR,Xo,__,Xa,XA,__,__,Xs,__,__,__,__,Xm,Xz,Xd,__,__,__,__,__,F1,__,N1,__,__,T1,__,__},
/* NA:Name-ini*/ {NA,NA,__,__,__,__,__,__,NM,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__},
/* NB:Backslsh*/ {__,__,__,__,__,__,__,__,Xe,Xe,Xe,__,__,__,__,__,__,__,__,Xe,__,Xe,__,Xe,Xe,__,Xe,Xu,__},
/* NM:Name    */ {Xn,__,Xn,Xn,Xn,Xn,Xn,Xn,XN,NB,Xn,Xn,Xn,Xn,Xn,Xn,Xn,Xn,Xn,Xn,Xn,Xn,Xn,Xn,Xn,Xn,Xn,Xn,Xn},
/* CO:Colon   */ {CO,CO,__,__,__,__,VL,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__},
/* VL:Value   */ {VL,VL,Xo,__,Xa,__,__,__,Xs,__,__,__,__,Xm,Xz,Xd,__,__,__,__,__,F1,__,N1,__,__,T1,__,__},
/* VE:Val-end */ {VE,VE,__,XO,__,XA,__,XC,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__},
/* ST:String  */ {ST,__,ST,ST,ST,ST,ST,ST,XS,XB,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST},
/* SB:Backslsh*/ {__,__,__,__,__,__,__,__,Xe,Xe,Xe,__,__,__,__,__,__,__,__,Xe,__,Xe,__,Xe,Xe,__,Xe,Xu,__},
/* T1:t       */ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,T2,__,__,__,__},
/* T2:tr      */ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,T3,__},
/* T3:tru     */ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,XT,__,__,__,__,__,__,__,__},
/* F1:f       */ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,F2,__,__,__,__,__,__,__,__,__,__},
/* F2:fa      */ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,F3,__,__,__,__,__,__},
/* F3:fal     */ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,F4,__,__,__},
/* F4:fals    */ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,XF,__,__,__,__,__,__,__,__},
/* N1:n       */ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,N2,__},
/* N2:nu      */ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,N3,__,__,__,__,__,__},
/* N3:nul     */ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,XL,__,__,__,__,__,__},
/* DA:Digit   */ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,DC,DB,__,__,__,__,__,__,__,__,__,__,__,__,__},
/* DB:Digit   */ {XD,XD,__,XD,__,XD,__,XD,__,__,__,DD,__,__,DB,DB,__,DE,__,__,DE,__,__,__,__,__,__,__,__},
/* DC:Digit   */ {XD,XD,__,XD,__,XD,__,XD,__,__,__,DD,__,__,__,__,__,DE,__,__,DE,__,__,__,__,__,__,__,__},
/* DD:Digit   */ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,DG,DG,__,__,__,__,__,__,__,__,__,__,__,__,__},
/* DE:Digit   */ {__,__,__,__,__,__,__,__,__,__,__,__,DF,DF,DH,DH,__,__,__,__,__,__,__,__,__,__,__,__,__},
/* DF:Digit   */ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,DH,DH,__,__,__,__,__,__,__,__,__,__,__,__,__},
/* DG:Digit   */ {XD,XD,__,XD,__,XD,__,XD,__,__,__,__,__,__,DG,DG,__,DE,__,__,DE,__,__,__,__,__,__,__,__},
/* DH:Digit   */ {XD,XD,__,XD,__,XD,__,XD,__,__,__,__,__,__,DH,DH,__,__,__,__,__,__,__,__,__,__,__,__,__},
/* U1:Unicode1*/ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,U2,U2,U2,U2,__,__,__,__,__,__,__,__,__,__,__},
/* U2:Unicode2*/ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,U3,U3,U3,U3,__,__,__,__,__,__,__,__,__,__,__},
/* U3:Unicode3*/ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,U4,U4,U4,U4,__,__,__,__,__,__,__,__,__,__,__},
/* U4:Unicode4*/ {__,__,__,__,__,__,__,__,__,__,__,__,__,__,XU,XU,XU,XU,__,__,__,__,__,__,__,__,__,__,__},
/* FI:Finish  */ {FI,FI,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__},
};

/*Utility function prototypes. */
STATUS JSON_Parser_Handle_To_Internal_Handle(JSON_PARSER_HANDLE *handle,
                                            _JSON_PARSER_HANDLE **parser_handle);
JSON_Class JSON_Parser_Get_Character_Class(CHAR ch);
STATUS JSON_Parser_Run_State_Machine(_JSON_PARSER_HANDLE *parser_handle);

/**************************************************************************
*
*   FUNCTION
*
*       NU_JSON_Parser_Create
*
*   DESCRIPTION
*
*       Creates an instance of a JSON Parser.
*
*   INPUTS
*
*       buffer_size             Size of the internal buffer used by
*                               the JSON Parser for holding JSON data.
*                               This can be a value of 6 bytes or larger.
*       *handle(out)            JSON Parser handle returned by this
*                               function which is to be passed into the
*                               rest of the JSON Parser APIs.
*
*   OUTPUTS
*
*       NU_SUCCESS              The JSON Parser's handle was created.
*       NU_INVALID_POINTER      The JSON Parser handle passed in is
*                               NU_NULL.
*       NU_INVALID_PARM         One of the parameters is invalid.
*
*       Otherwise, an operating-system specific error is returned.
*
****************************************************************************/
STATUS NU_JSON_Parser_Create(INT buffer_size, JSON_PARSER_HANDLE *handle)
{
    STATUS status;
    _JSON_PARSER_HANDLE *parser_handle;

    NU_SUPERV_USER_VARIABLES

    /* Make sure the parameters are valid. */
    if (buffer_size < JSON_PARSER_MIN_BUFFER_SIZE)
    {
        return NU_INVALID_PARM;
    }

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    if (handle == NU_NULL)
    {
        status = NU_INVALID_POINTER;
    }
    else
    {
        /* Assume handle does NOT get set. */
        *handle = NU_NULL;

        /* Allocate JSON Parser handle. */
        status = NU_Allocate_Memory(JSON_Memory, (VOID**)&parser_handle,
                                sizeof(_JSON_PARSER_HANDLE) + buffer_size,
                                NU_SUSPEND);
        if (status == NU_SUCCESS)
        {
            /* Zero out the allocated memory. */
            memset(parser_handle, 0, sizeof(_JSON_PARSER_HANDLE));

            /* Create our semaphore. */
            status = NU_Create_Semaphore(&(parser_handle->json_semaphore),
                                         "json_par", (UNSIGNED)1, NU_FIFO);
            if (status == NU_SUCCESS)
            {
                /* Set the buffer pointer. */
                parser_handle->buffer = (CHAR *)(parser_handle + 1);
                parser_handle->buffer_size = buffer_size;
                *parser_handle->buffer = '\0';

                parser_handle->current_type = JSON_TYPE_NONE;
                parser_handle->stack_level = 0;
                parser_handle->utf8_state = JSON_UTF8_ACCEPT_STATE;

                /* Assign our allocated Generator handle, so that it is
                 * returned. */
                *handle = parser_handle;
            }
            else
            {
                NU_Deallocate_Memory(parser_handle);
            }
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
} /* NU_JSON_Parser_Create */

/**************************************************************************
*
*   FUNCTION
*
*       NU_JSON_Parser_Destroy
*
*   DESCRIPTION
*
*       Frees all resources associated with the JSON Parser's
*       handle passed in.
*
*   INPUTS
*
*       *handle                 JSON Parser handle whose resources
*                               will be freed.
*
*   OUTPUTS
*
*       NU_SUCCESS              The JSON Parser's handle was created.
*       NU_INVALID_POINTER      The JSON Parser handle passed in is
*                               NU_NULL.
*
*       Otherwise, an operating-system specific error is returned.
*
****************************************************************************/
STATUS NU_JSON_Parser_Destroy(JSON_PARSER_HANDLE *handle)
{
    STATUS status;
    _JSON_PARSER_HANDLE *parser_handle;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    status = JSON_Parser_Handle_To_Internal_Handle(handle, &parser_handle);
    if (status == NU_SUCCESS)
    {
        /* Delete the semaphore. If this fails we are in trouble so just
         * return the Nucleus error code. */
        status = NU_Delete_Semaphore(&(parser_handle->json_semaphore));
        if (status == NU_SUCCESS)
        {
            /* Deallocate the handle memory. */
            status = NU_Deallocate_Memory(parser_handle);

            if (status == NU_SUCCESS)
            {
                /* Clear out our handle. */
                *handle = NU_NULL;
            }
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
} /* NU_JSON_Parser_Destroy */

/**************************************************************************
*
*   FUNCTION
*
*       NU_JSON_Parser_Set_Data
*
*   DESCRIPTION
*
*       Feeds more data into the JSON Parser.
*
*   INPUTS
*
*       *handle                 JSON Parser handle.
*       *json_data              JSON Data being fed into the Parser.
*       *json_data_length       On input the user must specify the
*                               length of data contained in the
*                               "json_data" parameter. On successful
*                               return, this contains the amount of data
*                               which was actually added to the Parser's
*                               internal buffer. If this is not equal to
*                               the total length of "json_data" then the
*                               user must add the remaining data again
*                               after processing the data currently held
*                               in the Parser's internal buffer.
*
*   OUTPUTS
*
*       NU_SUCCESS              The operation was successful.
*       NU_INVALID_POINTER      The JSON Parser handle pointer is invalid.
*
*       Otherwise, an operating-system specific error is returned.
*
****************************************************************************/
STATUS NU_JSON_Parser_Set_Data(JSON_PARSER_HANDLE *handle, CHAR *json_data,
                               INT *json_data_length)
{
    STATUS status;
    _JSON_PARSER_HANDLE *parser_handle;
    INT new_data_len;
    INT prev_data_len = 0;
    INT data_pos;

    NU_SUPERV_USER_VARIABLES

    if ((json_data == NU_NULL) || (json_data_length == NU_NULL))
    {
        return NU_INVALID_POINTER;
    }
    else if ((*json_data == '\0') || (*json_data_length <= 0))
    {
        return NU_INVALID_PARM;
    }

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    status = JSON_Parser_Handle_To_Internal_Handle(handle, &parser_handle);
    if (status == NU_SUCCESS)
    {
        new_data_len = *json_data_length;

        data_pos = parser_handle->data_pos;

        /* If we are processing a string, then leave in the data
         * which has not been returned to the user yet. */
        if ((parser_handle->current_type == JSON_TYPE_STRING) &&
                (parser_handle->string_start_pos < data_pos))
            data_pos = parser_handle->string_start_pos;

        /* If there is still unprocessed data in the buffer. */
        if (data_pos < parser_handle->data_size)
        {
            /* Length of unprocessed data. */
            prev_data_len = parser_handle->data_size - data_pos;

            /* Move unprocessed data to the start of the buffer. */
            memmove(parser_handle->buffer,
                parser_handle->buffer + data_pos, prev_data_len);
            parser_handle->data_pos -= data_pos;
            parser_handle->string_start_pos -= data_pos;
        }
        else
        {
            /* Reset the positions to the start of the buffer. */
            parser_handle->data_pos = 0;
            parser_handle->string_start_pos = 0;
        }

        /* If all the data will not fit in the remaining buffer. */
        if (parser_handle->buffer_size < new_data_len + prev_data_len + 1)
        {
            new_data_len = parser_handle->buffer_size - prev_data_len - 1;
        }

        /* If at least one byte can be written to the buffer. */
        if (new_data_len > 0)
        {
            /* Copy new data to the end of the already present
             * unprocessed data. */
            strncpy(parser_handle->buffer + prev_data_len, json_data,
                    new_data_len);
            parser_handle->buffer[prev_data_len + new_data_len] = '\0';
            parser_handle->data_size = prev_data_len + new_data_len;
        }

        /* Return the number of bytes written back to the user. */
        *json_data_length = new_data_len;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
} /* NU_JSON_Parser_Set_Data */

/**************************************************************************
*
*   FUNCTION
*
*       NU_JSON_Parser_Reset
*
*   DESCRIPTION
*
*       Resets the internal state of the JSON Parser so that all previously
*       buffered data is flushed and the Parser is ready to accept new
*       input. The "NU_JSON_Parser_Set_Data()" API must be called after
*       a reset to feed data into the Parser.
*
*   INPUTS
*
*       *handle                 JSON Parser handle which is to be reset.
*
*   OUTPUTS
*
*       NU_SUCCESS              The JSON Parser was reset successfully.
*       NU_INVALID_POINTER      The JSON Parser handle passed in is
*                               NU_NULL.
*
*       Otherwise, an operating-system specific error is returned.
*
****************************************************************************/
STATUS NU_JSON_Parser_Reset(JSON_PARSER_HANDLE *handle)
{
    STATUS status;
    _JSON_PARSER_HANDLE *parser_handle;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    status = JSON_Parser_Handle_To_Internal_Handle(handle, &parser_handle);
    if (status == NU_SUCCESS)
    {
        /* Reset state related data members. */
        parser_handle->data_size = 0;
        parser_handle->data_pos = 0;
        parser_handle->stack_level = 0;
        parser_handle->string_start_pos = 0;
        parser_handle->utf8_state = JSON_UTF8_ACCEPT_STATE;
        parser_handle->current_type = JSON_TYPE_NONE;
        parser_handle->current_state = GO;
        parser_handle->name_is_present = NU_FALSE;
        parser_handle->string_in_progress = NU_FALSE;
        parser_handle->record_value = NU_FALSE;
        *parser_handle->current_name = '\0';
        *parser_handle->current_value = '\0';
        *parser_handle->buffer= '\0';
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
} /* NU_JSON_Parser_Reset */

/**************************************************************************
*
*   FUNCTION
*
*       NU_JSON_Parser_Next
*
*   DESCRIPTION
*
*       Returns the next token/type in the JSON data stream.
*
*   INPUTS
*
*       *handle                 JSON Parser handle whose resources
*                               will be freed.
*       *type(out)              On return, contains next type which is
*                               one of the following:
*                               - JSON_TYPE_BOOLEAN
*                               - JSON_TYPE_STRING
*                               - JSON_TYPE_INTEGER
*                               - JSON_TYPE_FLOAT
*                               - JSON_TYPE_NULL
*                               - JSON_TYPE_UINTEGER
*                               - JSON_TYPE_OBJECT_START
*                               - JSON_TYPE_OBJECT_END
*                               - JSON_TYPE_ARRAY_START
*                               - JSON_TYPE_ARRAY_END
*                               - JSON_TYPE_UNKNOWN
*                               - JSON_TYPE_NONE
*       *stack_level            On return, contains current stack level.
*                               This is optional and can be NU_NULL.
*       *name                   On return, contains name of a name/value
*                               pair if applicable. Otherwise contains
*                               an empty string. This is optional and can
*                               be NU_NULL if the caller is sure that the
*                               next type cannot be a name/value pair.
*
*   OUTPUTS
*
*       NU_SUCCESS              The JSON Parser's handle was created.
*       NU_INVALID_POINTER      The JSON Parser handle passed in is
*                               NU_NULL.
*       NUF_INVALID_UTF8_NAME   Name data is invalid UTF-8.
*
*       Otherwise, an operating-system specific error is returned.
*
****************************************************************************/
STATUS NU_JSON_Parser_Next(JSON_PARSER_HANDLE *handle, UINT8 *type,
                           UINT8 *stack_level, CHAR *name)
{
    STATUS  status;
    _JSON_PARSER_HANDLE *parser_handle;
    UINT32  codepoint;
    UINT32  utf8_state = JSON_UTF8_ACCEPT_STATE;

    NU_SUPERV_USER_VARIABLES

    /* Make sure the parameters are valid. */
    if (type == NU_NULL)
    {
        return NU_INVALID_POINTER;
    }

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    status = JSON_Parser_Handle_To_Internal_Handle(handle, &parser_handle);
    if (status == NU_SUCCESS)
    {
        status = NU_Obtain_Semaphore(&(parser_handle->json_semaphore), NU_SUSPEND);
        if (status == NU_SUCCESS)
        {
            /* Reset the current type. */
            if (parser_handle->current_state == FI)
                *type = JSON_TYPE_NONE;
            else
                *type = JSON_TYPE_UNKNOWN;

            /* Reset name if present. */
            if (name)
            {
                *name = '\0';
            }
            /* Reset the stack level. */
            if (stack_level)
            {
                *stack_level = 0;
            }

            /* If there are unprocessed characters in the data stream. */
            if (parser_handle->data_pos < parser_handle->data_size)
            {
                /* Run the JSON Parser State Machine. */
                status = JSON_Parser_Run_State_Machine(parser_handle);
                if (status == NU_SUCCESS)
                {
                    /* Return the information to the caller. */
                    *type = parser_handle->current_type;

                    /* Also copy the optional parameters if present. */
                    if (stack_level)
                    {
                        /* Report the pre-container start stack-level if
                         * this is the start of a container. */
                        if ((parser_handle->current_type == JSON_TYPE_OBJECT_START) ||
                            (parser_handle->current_type == JSON_TYPE_ARRAY_START))
                        {
                            *stack_level = parser_handle->stack_level - 1;
                        }
                        else
                        {
                            *stack_level = parser_handle->stack_level;
                        }
                    }
                    /* Return the name if applicable. */
                    if ((name) && (parser_handle->current_type != JSON_TYPE_UNKNOWN))
                    {
                        strcpy(name, parser_handle->current_name);
                        *parser_handle->current_name = '\0';

                        /* Validate name for being valid UTF-8. */
                        while (*name != '\0')
                        {
                            JSON_Decode_Utf8(&utf8_state, &codepoint, *name++);
                        }

                        /* If we are not in a valid state then invalid
                         * UTF-8 data was encountered. */
                        if (utf8_state != JSON_UTF8_ACCEPT_STATE)
                        {
                            status = NUF_INVALID_UTF8_NAME;
                        }
                    }
                }
            }

            NU_Release_Semaphore(&(parser_handle->json_semaphore));
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
} /* NU_JSON_Parser_Next */

/**************************************************************************
*
*   FUNCTION
*
*       NU_JSON_Parser_Get_Info
*
*   DESCRIPTION
*
*       Gets information about the JSON Parser and its internal state.
*       The current and next indices return by this function might not
*       be exact due to the way the JSON Parser internally maintains
*       the state.
*
*   INPUTS
*
*       *handle                 JSON Parser handle.
*       compress                Determines whether "buffer_left" value
*                               represents compressed or uncompressed
*                               data. Compressed data would strip out
*                               all whitespace.
*       *current_type_index(out) On return, contains position in buffer
*                               of the current type. This will be -1
*                               if unknown. This is optional and can be
*                               NU_NULL.
*       *next_type_index(out)   On return, contains position in buffer
*                               of the next type. This will be -1
*                               if unknown. This is optional and can be
*                               NU_NULL.
*       *buffer_left(out)       On return, contains the number of bytes
*                               left which have not been processed by
*                               the parser. This is optional and can be
*                               NU_NULL.
*
*   OUTPUTS
*
*       NU_SUCCESS              The JSON Parser's handle was created.
*       NUF_JSON_UNEXPECTED_TYPE This function is called when the next
*                               expected type was not a "null".
*       NU_INVALID_POINTER      The JSON Parser handle passed in is
*                               NU_NULL.
*
*       Otherwise, an operating-system specific error is returned.
*
****************************************************************************/
STATUS NU_JSON_Parser_Get_Info(JSON_PARSER_HANDLE *handle,
                               BOOLEAN compress,
                               INT *current_type_index,
                               INT *next_type_index,
                               INT *buffer_left)
{
    STATUS  status;
    INT     i;
    INT     count = 0;
    _JSON_PARSER_HANDLE *parser_handle;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    status = JSON_Parser_Handle_To_Internal_Handle(handle, &parser_handle);
    if (status == NU_SUCCESS)
    {
        status = NU_Obtain_Semaphore(&(parser_handle->json_semaphore), NU_SUSPEND);
        if (status == NU_SUCCESS)
        {
            if (buffer_left != NU_NULL)
            {
                /* If compressed length is not requested. */
                if (!compress)
                {
                    *buffer_left = (parser_handle->data_size -
                                    parser_handle->data_pos);
                }
                else
                {
                    /* Loop for all remaining data. */
                    for (i = parser_handle->data_pos;
                         i < parser_handle->data_size; i++)
                    {
                        /* If the current character is not a space. */
                        if (!isspace((int)parser_handle->buffer[i]))
                            count++;
                    }
                    /* Return the count to the user. */
                    *buffer_left = count;
                }
            }

            if (next_type_index != NU_NULL)
            {
                /* Return the current position in the buffer. */
                *next_type_index = parser_handle->data_pos;
            }

            if (current_type_index != NU_NULL)
            {
                /* If we are parsing a string then the current type
                 * will be known. It will be unknown otherwise. */
                if (parser_handle->current_type == JSON_TYPE_STRING)
                    *current_type_index = parser_handle->string_start_pos;
                else
                    *current_type_index = -1;
            }

            NU_Release_Semaphore(&(parser_handle->json_semaphore));
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
} /* NU_JSON_Parser_Get_Info */

/**************************************************************************
*
*   FUNCTION
*
*       NU_JSON_Parser_Get_Null
*
*   DESCRIPTION
*
*       Gets the next null value encountered by the JSON parser.
*
*   INPUTS
*
*       *handle                 JSON Parser handle.
*
*   OUTPUTS
*
*       NU_SUCCESS              The operation was successful.
*       NUF_JSON_UNEXPECTED_TYPE This function is called when the next
*                               expected type was not a "null".
*       NU_INVALID_POINTER      The JSON Parser handle passed in is
*                               NU_NULL.
*
*       Otherwise, an operating-system specific error is returned.
*
****************************************************************************/
STATUS NU_JSON_Parser_Get_Null(JSON_PARSER_HANDLE *handle)
{
    STATUS status;
    _JSON_PARSER_HANDLE *parser_handle;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    status = JSON_Parser_Handle_To_Internal_Handle(handle, &parser_handle);
    if (status == NU_SUCCESS)
    {
        status = NU_Obtain_Semaphore(&(parser_handle->json_semaphore), NU_SUSPEND);
        if (status == NU_SUCCESS)
        {
            /* Make sure the current value expected is a null. If not
             * then generate an error. */
            if (parser_handle->current_type == JSON_TYPE_NULL)
            {
                parser_handle->current_type = JSON_TYPE_UNKNOWN;
            }
            else
            {
                status = NUF_JSON_UNEXPECTED_TYPE;
            }

            NU_Release_Semaphore(&(parser_handle->json_semaphore));
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
} /* NU_JSON_Parser_Get_Null */

/**************************************************************************
*
*   FUNCTION
*
*       NU_JSON_Parser_Get_Boolean
*
*   DESCRIPTION
*
*       Gets the next boolean value encountered by the JSON parser.
*
*   INPUTS
*
*       *handle                 JSON Parser handle.
*       *value(out)             On successful return, contains either
*                               NU_TRUE or NU_FALSE.
*
*   OUTPUTS
*
*       NU_SUCCESS              The operation was successful.
*       NUF_JSON_UNEXPECTED_TYPE This function is called when the next
*                               expected type was not a "null".
*       NU_INVALID_POINTER      The JSON Parser handle passed in is
*                               NU_NULL.
*
*       Otherwise, an operating-system specific error is returned.
*
****************************************************************************/
STATUS NU_JSON_Parser_Get_Boolean(JSON_PARSER_HANDLE *handle, BOOLEAN *value)
{
    STATUS status;
    _JSON_PARSER_HANDLE *parser_handle;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    status = JSON_Parser_Handle_To_Internal_Handle(handle, &parser_handle);
    if (status == NU_SUCCESS)
    {
        status = NU_Obtain_Semaphore(&(parser_handle->json_semaphore), NU_SUSPEND);
        if (status == NU_SUCCESS)
        {
            /* Make sure the current value expected is a null. If not
             * then generate an error. */
            if (parser_handle->current_type == JSON_TYPE_BOOLEAN)
            {
                if (!strcmp(parser_handle->current_value, "true"))
                {
                    *value = NU_TRUE;
                }
                else
                {
                    *value = NU_FALSE;
                }

                /* Reset the "type" to unknown now. */
                parser_handle->current_type = JSON_TYPE_UNKNOWN;
                *parser_handle->current_value = '\0';
            }
            else
            {
                status = NUF_JSON_UNEXPECTED_TYPE;
            }

            NU_Release_Semaphore(&(parser_handle->json_semaphore));
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
} /* NU_JSON_Parser_Get_Boolean */

/**************************************************************************
*
*   FUNCTION
*
*       NU_JSON_Parser_Get_Int
*
*   DESCRIPTION
*
*       Gets the next integer value encountered by the JSON parser.
*
*   INPUTS
*
*       *handle                 JSON Parser handle.
*       *value(out)             On successful return, contains the
*                               numeric value.
*
*   OUTPUTS
*
*       NU_SUCCESS              The operation was successful.
*       NUF_JSON_UNEXPECTED_TYPE This function is called when the next
*                               expected type was not an integer.
*       NU_INVALID_POINTER      The JSON Parser handle passed in is
*                               NU_NULL.
*
*       Otherwise, an operating-system specific error is returned.
*
****************************************************************************/
STATUS NU_JSON_Parser_Get_Int(JSON_PARSER_HANDLE *handle, INT64 *value)
{
    STATUS status;
    _JSON_PARSER_HANDLE *parser_handle;

    NU_SUPERV_USER_VARIABLES

    /* Make sure the parameters are valid. */
    if (value == NU_NULL)
    {
        return NU_INVALID_POINTER;
    }

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    status = JSON_Parser_Handle_To_Internal_Handle(handle, &parser_handle);
    if (status == NU_SUCCESS)
    {
        status = NU_Obtain_Semaphore(&(parser_handle->json_semaphore), NU_SUSPEND);
        if (status == NU_SUCCESS)
        {
            /* Make sure the type is as expected. */
            if (parser_handle->current_type == JSON_TYPE_INTEGER)
            {
                /* Convert the integer value from a string and return it
                 * to the caller. */
                *value = JSON_String_To_Int64(parser_handle->current_value);

                /* Reset the "type" to unknown now. */
                parser_handle->current_type = JSON_TYPE_UNKNOWN;
                *parser_handle->current_value = '\0';
            }
            else
            {
                status = NUF_JSON_UNEXPECTED_TYPE;
            }

            NU_Release_Semaphore(&(parser_handle->json_semaphore));
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
} /* NU_JSON_Parser_Get_Int */

/**************************************************************************
*
*   FUNCTION
*
*       NU_JSON_Parser_Get_UInt
*
*   DESCRIPTION
*
*       Gets the next unsigned integer value encountered by the
*       JSON parser.
*
*   INPUTS
*
*       *handle                 JSON Parser handle.
*       *value(out)             On successful return, contains the
*                               numeric value.
*
*   OUTPUTS
*
*       NU_SUCCESS              The operation was successful.
*       NUF_JSON_UNEXPECTED_TYPE This function is called when the next
*                               expected type was not an integer.
*       NU_INVALID_POINTER      The JSON Parser handle passed in is
*                               NU_NULL.
*
*       Otherwise, an operating-system specific error is returned.
*
****************************************************************************/
STATUS NU_JSON_Parser_Get_UInt(JSON_PARSER_HANDLE *handle, UINT64 *value)
{
    STATUS status;
    _JSON_PARSER_HANDLE *parser_handle;

    NU_SUPERV_USER_VARIABLES

    /* Make sure the parameters are valid. */
    if (value == NU_NULL)
    {
        return NU_INVALID_POINTER;
    }

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    status = JSON_Parser_Handle_To_Internal_Handle(handle, &parser_handle);
    if (status == NU_SUCCESS)
    {
        status = NU_Obtain_Semaphore(&(parser_handle->json_semaphore), NU_SUSPEND);
        if (status == NU_SUCCESS)
        {
            /* Make sure the type is as expected. */
            if (parser_handle->current_type == JSON_TYPE_UINTEGER)
            {
                /* Convert the integer value from a string and return it
                 * to the caller. */
                *value = JSON_String_To_UInt64(parser_handle->current_value);

                /* Reset the "type" to unknown now. */
                parser_handle->current_type = JSON_TYPE_UNKNOWN;
                *parser_handle->current_value = '\0';
            }
            else
            {
                status = NUF_JSON_UNEXPECTED_TYPE;
            }

            NU_Release_Semaphore(&(parser_handle->json_semaphore));
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
} /* NU_JSON_Parser_Get_UInt */

/**************************************************************************
*
*   FUNCTION
*
*       NU_JSON_Parser_Get_Float
*
*   DESCRIPTION
*
*       Gets the next float value encountered by the JSON parser.
*
*   INPUTS
*
*       *handle                 JSON Parser handle.
*       *value(out)             This initially contains the buffer and
*                               size of buffer passed in by the caller.
*                               On successful return, contains the string
*                               equivalent of the float number.
*
*   OUTPUTS
*
*       NU_SUCCESS              The operation was successful.
*       NUF_JSON_UNEXPECTED_TYPE This function is called when the next
*                               expected type was not an integer.
*       NU_INVALID_POINTER      The JSON Parser handle passed in is
*                               NU_NULL.
*       NUF_FLOAT_LENGTH_EXCEEDED Size of buffer provided by user not
*                               large enough to hold float value.
*
*       Otherwise, an operating-system specific error is returned.
*
****************************************************************************/
STATUS NU_JSON_Parser_Get_Float(JSON_PARSER_HANDLE *handle, JSON_STRING *value)
{
    STATUS status;
    _JSON_PARSER_HANDLE *parser_handle;

    NU_SUPERV_USER_VARIABLES

    /* Make sure the parameters are valid. */
    if ((value == NU_NULL) || (value->str == NU_NULL))
    {
        return NU_INVALID_POINTER;
    }
    else if (value->length <= 0)
    {
        return NU_INVALID_PARM;
    }

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    status = JSON_Parser_Handle_To_Internal_Handle(handle, &parser_handle);
    if (status == NU_SUCCESS)
    {
        status = NU_Obtain_Semaphore(&(parser_handle->json_semaphore), NU_SUSPEND);
        if (status == NU_SUCCESS)
        {
            /* Make sure the type is as expected. */
            if (parser_handle->current_type == JSON_TYPE_FLOAT)
            {
                /* Make sure there is enough room in the user's buffer. */
                if (value->length > strlen(parser_handle->current_value))
                {
                    strcpy(value->str, parser_handle->current_value);
                    value->length = strlen(value->str);

                    /* Reset the "type" to unknown now. */
                    parser_handle->current_type = JSON_TYPE_UNKNOWN;
                    *parser_handle->current_value = '\0';
                }
                else
                {
                    status = NUF_FLOAT_LENGTH_EXCEEDED;
                }
            }
            else
            {
                status = NUF_JSON_UNEXPECTED_TYPE;
            }

            NU_Release_Semaphore(&(parser_handle->json_semaphore));
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
} /* NU_JSON_Parser_Get_Float */

/**************************************************************************
*
*   FUNCTION
*
*       NU_JSON_Parser_Get_String
*
*   DESCRIPTION
*
*       Gets the next string value encountered by the JSON parser.
*
*   INPUTS
*
*       *handle                 JSON Parser handle.
*       *value(out)             This initially contains the buffer and
*                               size of buffer passed in by the caller.
*                               On successful return, contains the string
*                               data and length of that data.
*       *flags                  On return, contains the JSON_IS_PARTIAL
*                               flag set if more string data is pending.
*
*   OUTPUTS
*
*       NU_SUCCESS              The operation was successful.
*       NUF_JSON_UNEXPECTED_TYPE This function is called when the next
*                               expected type was not a string.
*       NU_INVALID_POINTER      The JSON Parser handle passed in is
*                               NU_NULL.
*       NUF_INVALID_UTF8_DATA   String data is invalid UTF-8.
*
*       Otherwise, an operating-system specific error is returned.
*
****************************************************************************/
STATUS NU_JSON_Parser_Get_String(JSON_PARSER_HANDLE *handle, JSON_STRING *value,
                                 UINT8 *flags)
{
    STATUS status;
    _JSON_PARSER_HANDLE *parser_handle;
    INT str_length;
    INT copy_length;
    CHAR *str;
    UINT32 codepoint;

    NU_SUPERV_USER_VARIABLES

    /* Make sure the parameters are valid. */
    if ((value == NU_NULL) || (value->str == NU_NULL) || (flags == NU_NULL))
    {
        return NU_INVALID_POINTER;
    }
    else if (value->length <= 0)
    {
        return NU_INVALID_PARM;
    }

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    status = JSON_Parser_Handle_To_Internal_Handle(handle, &parser_handle);
    if (status == NU_SUCCESS)
    {
        status = NU_Obtain_Semaphore(&(parser_handle->json_semaphore), NU_SUSPEND);
        if (status == NU_SUCCESS)
        {
            /* Reset the return values. */
            *flags = JSON_IS_PARTIAL;
            *value->str = '\0';

            /* Make sure the current value expected is an incomplete string.
             * If not then generate an error. */
            if ((parser_handle->current_type == JSON_TYPE_STRING) &&
                (parser_handle->string_in_progress == NU_TRUE))
            {
                 /* If there are unprocessed characters in the data stream. */
                if (parser_handle->data_pos < parser_handle->data_size)
                {
                    /* Run the JSON Parser State Machine. */
                    status = JSON_Parser_Run_State_Machine(parser_handle);
                    if (status == NU_SUCCESS)
                    {
                        /* Override the unknown type with string because an
                         * unknown type is returned on a partial string
                         * match. */
                        if (parser_handle->current_type == JSON_TYPE_UNKNOWN)
                            parser_handle->current_type = JSON_TYPE_STRING;

                        /* Calculate length of the string received. */
                        str_length = parser_handle->data_pos -
                                     parser_handle->string_start_pos;

                        /* If the complete string was received. */
                        if (parser_handle->string_in_progress == NU_FALSE)
                        {
                            /* Decrease the length by one to account for
                             * the closing quote. */
                            str_length--;
                        }
                        /* If the string is still in progress then see if
                         * we are on one of the intermediate Unicode
                         * related states. */
                        else if ((parser_handle->current_state == U1) ||
                                 (parser_handle->current_state == U2) ||
                                 (parser_handle->current_state == U3) ||
                                 (parser_handle->current_state == U4))
                        {
                            /* Don't process any string data until the
                             * Unicode escape sequence is completed. */
                            str_length = 0;
                        }

                        /* If some string data was received. */
                        if (str_length >= 0)
                        {
                            /* If string exceeds return buffer length. */
                            if (str_length >= value->length)
                                copy_length = value->length - 1;
                            else
                                copy_length = str_length;

                            /* Copy the string value to the user buffer. */
                            strncpy(value->str,
                                &parser_handle->buffer[parser_handle->string_start_pos],
                                copy_length);
                            value->str[copy_length] = '\0';
                            value->length = strlen(value->str);

                            /* If the current state is back-slash matching,
                             * then skip the last character which is a
                             * back-slash to be escaped. */
                            if ((parser_handle->current_state == SB) &&
                                (copy_length))
                            {
                                value->str[copy_length - 1] = '\0';
                                value->length--;
                            }

                            /* Adjust remaining string length. */
                            parser_handle->string_start_pos += copy_length;

                            /* Set the partial flag if string still in
                             * progress. */
                            if ((parser_handle->string_in_progress) ||
                                (copy_length != str_length))
                            {
                                *flags = JSON_IS_PARTIAL;
                            }
                            else
                            {
                                *flags = 0;
                            }
                        }
                    }
                }
            }
            else if ((parser_handle->current_type == JSON_TYPE_STRING) &&
                     (parser_handle->string_in_progress == NU_FALSE))
            {
                /* Check if there are processed characters of the string
                 * which have not been returned to the caller yet. */
                str_length = parser_handle->data_pos - 1 -
                             parser_handle->string_start_pos;

                if (str_length >= 0)
                {
                    /* If string exceeds return buffer length. */
                    if (str_length >= value->length)
                        copy_length = value->length - 1;
                    else
                        copy_length = str_length;

                    /* Copy the string value to the user buffer. */
                    strncpy(value->str,
                        &parser_handle->buffer[parser_handle->string_start_pos],
                        copy_length);
                    value->str[copy_length] = '\0';
                    value->length = strlen(value->str);

                    /* Adjust remaining string length. */
                    parser_handle->string_start_pos += copy_length;

                    /* Set the partial flag if string not complete. */
                    if (copy_length != str_length)
                    {
                        *flags = JSON_IS_PARTIAL;
                    }
                    else
                    {
                        *flags = 0;
                    }
                }
            }
            else
            {
                status = NUF_JSON_UNEXPECTED_TYPE;
            }

            /* If no error has occurred, then validate the UTF-8 data
             * being returned in the string. */
            if (status == NU_SUCCESS)
            {
                str = value->str;
                while (*str)
                {
                    /* Pass the data through the UTF-8 decoder. */
                    JSON_Decode_Utf8(&(parser_handle->utf8_state),
                                     &codepoint, *str++);
                }
                /* If the data is not in a valid state and if this is
                 * the end of the string. */
                if ((parser_handle->utf8_state != JSON_UTF8_ACCEPT_STATE) &&
                    ((*flags & JSON_IS_PARTIAL) == 0))
                {
                    status = NUF_INVALID_UTF8_DATA;
                }
            }

            NU_Release_Semaphore(&(parser_handle->json_semaphore));
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
} /* NU_JSON_Parser_Get_String */

/**************************************************************************
*
*   FUNCTION
*
*       JSON_Parser_Handle_To_Internal_Handle
*
*   DESCRIPTION
*
*       Converts a public handle to an internal handle.
*
*   INPUTS
*
*       *handle                 A JSON Parser's public handle.
*       **parser_handle(out)    A JSON Parser's private/internal handle.
*                               token.
*   OUTPUTS
*
*       NU_SUCCESS              The handle was converted successfully.
*       NU_INVALID_POINTER      The JSON Parser handle passed in is
*                               NU_NULL.
*
****************************************************************************/
STATUS JSON_Parser_Handle_To_Internal_Handle(JSON_PARSER_HANDLE *handle,
                                            _JSON_PARSER_HANDLE **parser_handle)
{
    STATUS status = NU_SUCCESS;

    if ((handle != NU_NULL) && (*handle != NU_NULL))
    {
        *parser_handle = (_JSON_PARSER_HANDLE*)*handle;
    }
    else
    {
        status = NU_INVALID_POINTER;
    }

    return (status);
} /* JSON_Parser_Handle_To_Internal_Handle */

/**************************************************************************
*
*   FUNCTION
*
*       JSON_Parser_Get_Character_Class
*
*   DESCRIPTION
*
*       Return the character class of the specified character.
*
*   INPUTS
*
*       ch                      Character for which to return the class.
*
*   OUTPUTS
*
*       The JSON character class for the specified character.
*
****************************************************************************/
JSON_Class JSON_Parser_Get_Character_Class(CHAR ch)
{
    /* If this is an extended ASCII character. */
    if (ch >= sizeof(JSON_Class_Table))
        return C_ETC;

    return (JSON_Class)(JSON_Class_Table[(INT)ch]);
} /* JSON_Parser_Get_Character_Class */

/**************************************************************************
*
*   FUNCTION
*
*       JSON_Parser_Push_State
*
*   DESCRIPTION
*
*       Push the older state onto the stack and set the new state.
*
*   INPUTS
*
*       *parser_handle          JSON Parser's private handle.
*       new_state               New state to set.
*
*   OUTPUTS
*
*       NU_SUCCESS              Push operation was successful.
*       NUF_JSON_DEPTH_EXCEEDED Stack depth exceeded.
*
****************************************************************************/
STATUS JSON_Parser_Push_State(_JSON_PARSER_HANDLE *parser_handle,
                              JSON_State new_state)
{
    STATUS status = NU_SUCCESS;

    /* If the stack depth is not exceeded. */
    if (parser_handle->stack_level < JSON_MAX_DEPTH_LEVEL)
    {
        /* Save the older state on the stack. */
        parser_handle->stack_items[parser_handle->stack_level].prev_state =
            parser_handle->current_state;
        parser_handle->stack_items[parser_handle->stack_level].next_state =
            new_state;
        parser_handle->stack_level++;

        /* Set the new state. */
        parser_handle->current_state = new_state;
    }
    else
    {
        status = NUF_JSON_DEPTH_EXCEEDED;
    }

    return (status);
} /* JSON_Parser_Push_State */

/**************************************************************************
*
*   FUNCTION
*
*       JSON_Parser_Pop_State
*
*   DESCRIPTION
*
*       Pops the older state from the stack and sets it as the new state.
*
*   INPUTS
*
*       *parser_handle          JSON Parser's private handle.
*
*   OUTPUTS
*
*       NU_SUCCESS              Pop operation was successful.
*       NUF_JSON_PARSING_ERROR  Stack is empty unexpectedly.
*
****************************************************************************/
STATUS JSON_Parser_Pop_State(_JSON_PARSER_HANDLE *parser_handle)
{
    STATUS status = NU_SUCCESS;

    /* Make sure the stack is not empty. */
    if (parser_handle->stack_level > 0)
    {
        /* Restore the state from the stack. */
        parser_handle->stack_level--;
        parser_handle->current_state =
            parser_handle->stack_items[parser_handle->stack_level].prev_state;
    }
    else
    {
        status = NUF_JSON_PARSING_ERROR;
    }

    return (status);
} /* JSON_Parser_Pop_State */

/**************************************************************************
*
*   FUNCTION
*
*       JSON_Parser_Get_Top_Of_Stack
*
*   DESCRIPTION
*
*       Returns the top of stack item.
*
*   INPUTS
*
*       *parser_handle          JSON Parser's private handle.
*       *top_item               Top of stack item.
*
*   OUTPUTS
*
*       NU_SUCCESS              The operation was successful.
*       NUF_JSON_PARSING_ERROR  Stack is empty unexpectedly.
*
****************************************************************************/
STATUS JSON_Parser_Get_Top_Of_Stack(_JSON_PARSER_HANDLE *parser_handle,
                                    JSON_PARSER_STACK_ITEM *top_item)
{
    STATUS status = NU_SUCCESS;

    /* Make sure the stack is not empty. */
    if (parser_handle->stack_level > 0)
    {
        /* Return the top of stack item. */
        *top_item = parser_handle->stack_items[parser_handle->stack_level - 1];
    }
    else
    {
        status = NUF_JSON_PARSING_ERROR;
    }

    return (status);
} /* JSON_Parser_Get_Top_Of_Stack */

/**************************************************************************
*
*   FUNCTION
*
*       JSON_Parser_Run_State_Machine
*
*   DESCRIPTION
*
*       Run the JSON Parser State Machine.
*
*   INPUTS
*
*       *parser_handle          JSON Parser's private handle.
*
*   OUTPUTS
*
*       NU_SUCCESS              The state machine was run successfully.
*
****************************************************************************/
STATUS JSON_Parser_Run_State_Machine(_JSON_PARSER_HANDLE *parser_handle)
{
    STATUS      status = NU_SUCCESS;
    JSON_Class  char_class;
    JSON_State  new_state;
    JSON_Action action;
    BOOLEAN     next_type_found = NU_FALSE;
    INT         length;
    JSON_PARSER_STACK_ITEM top_of_stack;
    CHAR        ch;
    UINT32      wide_char;

    /* Loop until there are unprocessed characters in the buffer or
     * until a valid type is found. */
    while ((parser_handle->data_pos < parser_handle->data_size) &&
           (next_type_found == NU_FALSE) &&
           (status == NU_SUCCESS))
    {
        char_class = JSON_Parser_Get_Character_Class(
                        parser_handle->buffer[parser_handle->data_pos]);

        /* Get the next state/action. */
        new_state = (JSON_State)(JSON_State_Table[parser_handle->current_state][char_class]);

        /* If a state transition has occurred (state value < 200). */
        if (new_state < 200)
        {
            /* Set the new state. */
            parser_handle->current_state = new_state;
        }
        /* If the new state is an error state. */
        else if (new_state == __)
        {
            status = NUF_JSON_PARSING_ERROR;
            break;
        }
        else
        {
            /* Otherwise, see what action needs to be performed. */
            action = (JSON_Action)new_state;
            switch (action)
            {
                case Xo:
                    /* Push the state onto the stack. */
                    status = JSON_Parser_Push_State(parser_handle, OJ);
                    if (status == NU_SUCCESS)
                    {
                        parser_handle->current_type = JSON_TYPE_OBJECT_START;
                        next_type_found = NU_TRUE;
                    }
                    break;
                case XO:
                    status = JSON_Parser_Get_Top_Of_Stack(parser_handle,
                                                          &top_of_stack);
                    if (status == NU_SUCCESS)
                    {
                        /* Make sure the top-most container is an object. */
                        if (top_of_stack.next_state != OJ)
                        {
                            status = NUF_JSON_PARSING_ERROR;
                        }
                        else
                        {
                            status = JSON_Parser_Pop_State(parser_handle);
                            if (status == NU_SUCCESS)
                            {
                                parser_handle->current_type = JSON_TYPE_OBJECT_END;
                                next_type_found = NU_TRUE;

                                /* If this was not the end of the top-level
                                 * object then goto the end-of-value state. */
                                if (top_of_stack.prev_state != GO)
                                {
                                    parser_handle->current_state = VE;
                                }
                                else
                                {
                                    /* Otherwise, parsing is finished. */
                                    parser_handle->current_state = FI;
                                }
                            }
                        }
                    }
                    break;
                case Xa:
                    /* Push the state onto the stack. */
                    status = JSON_Parser_Push_State(parser_handle, AR);
                    if (status == NU_SUCCESS)
                    {
                        parser_handle->current_type = JSON_TYPE_ARRAY_START;
                        next_type_found = NU_TRUE;
                    }
                    break;
                case XA:
                    status = JSON_Parser_Get_Top_Of_Stack(parser_handle,
                                                          &top_of_stack);
                    if (status == NU_SUCCESS)
                    {
                        /* Make sure the top-most container is an array. */
                        if (top_of_stack.next_state != AR)
                        {
                            status = NUF_JSON_PARSING_ERROR;
                        }
                        else
                        {
                            status = JSON_Parser_Pop_State(parser_handle);
                            if (status == NU_SUCCESS)
                            {
                                parser_handle->current_type = JSON_TYPE_ARRAY_END;
                                next_type_found = NU_TRUE;

                                /* If this was not a top-level container. */
                                if (top_of_stack.prev_state != GO)
                                {
                                    /* Then match the next value. */
                                    parser_handle->current_state = VE;
                                }
                                else
                                {
                                    /* Otherwise, parsing is finished. */
                                    parser_handle->current_state = FI;
                                }
                            }
                        }
                    }
                    break;
                case Xn:
                    length = strlen(parser_handle->current_name);
                    /* Make sure the name hasn't exceeded its limit. */
                    if (length < JSON_MAX_NAME_LENGTH)
                    {
                        /* Add the newly read character to the buffer. */
                        parser_handle->current_name[length] =
                            parser_handle->buffer[parser_handle->data_pos];
                        parser_handle->current_name[length + 1] = '\0';

                        /* We are currently at the NM state so no need to
                         * change the state. */
                    }
                    else
                    {
                        status = NUF_NAME_LENGTH_EXCEEDED;
                    }
                    break;
                case XN:
                    parser_handle->current_state = CO;
                    parser_handle->current_type = JSON_TYPE_ARRAY_END;
                    break;
                case Xs:
                    /* Go to the string matching state. */
                    parser_handle->current_state = ST;
                    parser_handle->current_type = JSON_TYPE_STRING;
                    parser_handle->string_in_progress = NU_TRUE;
                    parser_handle->string_start_pos = parser_handle->data_pos + 1;
                    parser_handle->utf8_state = JSON_UTF8_ACCEPT_STATE;
                    next_type_found = NU_TRUE;
                    break;
                case XS:
                    /* Report string completion and go to value-end state. */
                    parser_handle->current_state = VE;
                    parser_handle->current_type = JSON_TYPE_STRING;
                    parser_handle->string_in_progress = NU_FALSE;
                    next_type_found = NU_TRUE;
                    break;
                case XT:
                    /* Report boolean type and go to value-end state. */
                    parser_handle->current_state = VE;
                    parser_handle->current_type = JSON_TYPE_BOOLEAN;
                    next_type_found = NU_TRUE;
                    strcpy(parser_handle->current_value, "true");
                    break;
                case XF:
                    /* Report boolean type and go to value-end state. */
                    parser_handle->current_state = VE;
                    parser_handle->current_type = JSON_TYPE_BOOLEAN;
                    next_type_found = NU_TRUE;
                    strcpy(parser_handle->current_value, "false");
                    break;
                case XL:
                    /* Report null type and go to value-end state. */
                    parser_handle->current_state = VE;
                    parser_handle->current_type = JSON_TYPE_NULL;
                    next_type_found = NU_TRUE;
                    break;
                case XC:
                    status = JSON_Parser_Get_Top_Of_Stack(parser_handle,
                                                          &top_of_stack);
                    if (status == NU_SUCCESS)
                    {
                        /* If the top-most container is an object. */
                        if (top_of_stack.next_state == OJ)
                            /* The next item to be matched is a name. */
                            parser_handle->current_state = NA;
                        else
                            /* The next item to be matched is a value. */
                            parser_handle->current_state = VL;
                    }
                    break;
                case Xm:
                    /* Matched a minus at the start of a number. */
                    parser_handle->current_state = DA;
                    /* Start recording the number. */
                    parser_handle->record_value = NU_TRUE;
                    *parser_handle->current_value = '\0';
                    break;
                case Xz:
                    /* Matched a zero at the start of a number. */
                    parser_handle->current_state = DC;
                    /* Start recording the number. */
                    parser_handle->record_value = NU_TRUE;
                    *parser_handle->current_value = '\0';
                    break;
                case Xd:
                    /* Matched a non-zero digit at the start of a number. */
                    parser_handle->current_state = DB;
                    /* Start recording the number. */
                    parser_handle->record_value = NU_TRUE;
                    *parser_handle->current_value = '\0';
                    break;
                case XD:
                    /* Rewind back to the value-end state. */
                    parser_handle->current_state = VE;
                    parser_handle->data_pos--;
                    /* Stop recording the number. */
                    parser_handle->record_value = NU_FALSE;
                    /* Report the integer/float type. */
                    if (strchr(parser_handle->current_value, '.') ||
                            strchr(parser_handle->current_value, 'E') ||
                            strchr(parser_handle->current_value, 'e'))
                        parser_handle->current_type = JSON_TYPE_FLOAT;
                    else if (*parser_handle->current_value == '-')
                        parser_handle->current_type = JSON_TYPE_INTEGER;
                    else
                        parser_handle->current_type = JSON_TYPE_UINTEGER;
                    next_type_found = NU_TRUE;
                    break;
                case Xe:
                    /* Determine which back-slashed escape character
                     * has occurred. */
                    ch = parser_handle->buffer[parser_handle->data_pos];
                    switch (ch)
                    {
                        case 'b':
                            ch = '\b';
                            break;
                        case 'f':
                            ch = '\f';
                            break;
                        case 'n':
                            ch = '\n';
                            break;
                        case 'r':
                            ch = '\r';
                            break;
                        case 't':
                            ch = '\t';
                            break;
                    }

                    /* If we are currently reading a name. */
                    if (parser_handle->current_state == NB)
                    {
                        length = strlen(parser_handle->current_name);
                        /* Make sure the name hasn't exceeded its limit. */
                        if (length < JSON_MAX_NAME_LENGTH)
                        {
                            /* Add the newly read character to the buffer. */
                            parser_handle->current_name[length] = ch;
                            parser_handle->current_name[length + 1] = '\0';
                            /* Go back to the name matching state. */
                            parser_handle->current_state = NM;
                        }
                        else
                        {
                            status = NUF_NAME_LENGTH_EXCEEDED;
                        }
                    }
                    /* Otherwise, we are reading a value as string. */
                    else if (parser_handle->current_state == SB)
                    {
                        /* Write raw character in place of the escape
                         * sequence. */
                        parser_handle->buffer[parser_handle->data_pos] = ch;
                        /* Go back to the string matching state. */
                        parser_handle->current_state = ST;
                    }
                    break;
                case XB:
                    /* Handle the back-slash character in a string by
                     * ending this chunk of string here. */
                    parser_handle->current_state = SB;
                    parser_handle->current_type = JSON_TYPE_STRING;
                    next_type_found = NU_TRUE;
                    break;
                case Xu:
                    /* Go to the U2 state and start recording hex digits. */
                    parser_handle->current_state = U1;
                    parser_handle->record_value = NU_TRUE;
                    break;
                case XU:
                    /* Stop recording hex digits. */
                    parser_handle->record_value = NU_FALSE;

                    /* Three hex-digits are in the value buffer, now also
                     * store the forth one there. */
                    length = strlen(parser_handle->current_value);

                    /* If there is space in the value buffer. */
                    if (length < JSON_MAX_FLOAT_LENGTH)
                    {
                        /* Record the last hex-digit. */
                        parser_handle->current_value[length] =
                            parser_handle->buffer[parser_handle->data_pos];
                        parser_handle->current_value[length + 1] = '\0';
                    }

                    /* Convert the unicode hex-code to wide-chararacter.
                     * Skip the initial 'u' character in the value. */
                    wide_char = (UINT32)NCL_Ahtoi(parser_handle->current_value + 1);

                    /* Convert the wide-character to UTF-8 encoding. */
                    length = JSON_Widechar_To_Utf8(wide_char,
                                    parser_handle->current_value);
                    parser_handle->current_value[length] = '\0';

                    /* Determine if we are inside a string or a name. */
                    if (parser_handle->string_in_progress)
                    {
                        /* The start-of-string is pointing to the 'u' for
                         * the unicode escape sequence. Increment it so
                         * that we can place UTF-8 encoded characters
                         * adjoining the following data. */
                        parser_handle->string_start_pos += (5 - length);

                        /* Replace hex digits with UTF-8 characters. */
                        memcpy(&parser_handle->buffer[parser_handle->string_start_pos],
                            parser_handle->current_value, length);

                        /* Continue from the ST state. */
                        parser_handle->current_state = ST;
                        *parser_handle->current_value = '\0';
                    }
                    else
                    {
                        /* Make sure the name won't exceeded its limit. */
                        if (strlen(parser_handle->current_name) + length <=
                            JSON_MAX_NAME_LENGTH)
                        {
                            /* Add the UTF-8 bytes to the buffer. */
                            strcat(parser_handle->current_name,
                                   parser_handle->current_value);

                            /* Continue from the NM state. */
                            parser_handle->current_state = NM;
                            *parser_handle->current_value = '\0';
                        }
                        else
                        {
                            status = NUF_NAME_LENGTH_EXCEEDED;
                        }
                    }
                    break;
                default:
                    break;
            }
        }

        if (status == NU_SUCCESS)
        {
            /* If the value being parsed is to be recorded in a buffer. */
            if (parser_handle->record_value == NU_TRUE)
            {
                length = strlen(parser_handle->current_value);

                /* If there is space in the value buffer. */
                if (length < JSON_MAX_FLOAT_LENGTH)
                {
                    /* Record the current character. */
                    parser_handle->current_value[length] =
                        parser_handle->buffer[parser_handle->data_pos];
                    parser_handle->current_value[length + 1] = '\0';
                }
                else
                    status = NUF_FLOAT_LENGTH_EXCEEDED;
            }

            if (status == NU_SUCCESS)
            {
                /* Move to the next character in the input stream. */
                parser_handle->data_pos++;
            }
        }
    }

    /* If the next type was not found. */
    if (next_type_found == NU_FALSE)
    {
        parser_handle->current_type = JSON_TYPE_UNKNOWN;
    }

    return (status);
} /* JSON_Parser_Run_State_Machine */

#endif /* CFG_NU_OS_NET_JSON_INCLUDE_PARSER */
