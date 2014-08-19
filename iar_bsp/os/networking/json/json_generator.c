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
*       json_generator.c
*
* COMPONENT
*
*       JSON
*
* DESCRIPTION
*
*       The JSON Generator is used to convert data into JSON text.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       nu_os_net_json_init
*       NU_JSON_Generator_Create
*       NU_JSON_Generator_Destroy
*       NU_JSON_Generator_Start_Token
*       NU_JSON_Generator_End_Token
*       NU_JSON_Generator_Add_Name
*       NU_JSON_Generator_Add_String
*       NU_JSON_Generator_Add_Boolean
*       NU_JSON_Generator_Add_Int
*       NU_JSON_Generator_Add_UInt
*       NU_JSON_Generator_Add_Float
*       NU_JSON_Generator_Add_Null
*       NU_JSON_Generator_Get_Buffer
*       NU_JSON_Generator_Clear_Buffer
*
*       JSON_Append_Data
*       JSON_Append_String
*       JSON_Gen_Handle_To_Internal_Handle
*       JSON_Is_Valid_Float
*
* DEPENDENCIES
*
*       nu_json.h
*       json_defs.h
*       nu_kernel.h
*       sockdefs.h
*       ctype.h
*       stdio.h
*
*************************************************************************/
#include "networking/nu_json.h"
#include "os/networking/json/json_defs.h"
#include "kernel/nu_kernel.h"
#include "os/include/networking/sockdefs.h"
#include <ctype.h>
#include <stdio.h>

/* Pointer to the JSON memory pool. */
extern NU_MEMORY_POOL System_Memory;
NU_MEMORY_POOL *JSON_Memory;

#define JSON_GEN_SET_NULL_TERMINATOR(internal_handle) internal_handle->buffer[internal_handle->buffer_pos] = '\0'
/* Determine how much of the name will fit -1 is to subtract out the null terminator. */
#define JSON_GEN_SPACE_LEFT(internal_handle) (internal_handle->buffer_size - 1 - internal_handle->buffer_pos)
#define JSON_GEN_HAS_SPACE(internal_handle, data) JSON_GEN_SPACE_LEFT > strlen(data)
#define JSON_GEN_IS_FULL(internal_handle) (internal_handle->buffer_pos == (internal_handle->buffer_size-1))

/**************************************************************************
*
*   FUNCTION
*
*       nu_os_net_json_init
*
*   DESCRIPTION
*
*       Component initialization function. Starts JSON initialization
*
*  INPUTS
*       *reg_path                Unused
*       startstop
*
*  OUTPUTS
*       None
*
****************************************************************************/
STATUS nu_os_net_json_init(const CHAR * reg_path, INT startstop)
{
    STATUS status = NU_SUCCESS;
    JSON_Memory = &System_Memory;
    return status;
} /* nu_os_net_json_init */

#if CFG_NU_OS_NET_JSON_INCLUDE_GENERATOR

/**************************************************************************
*
*   FUNCTION
*
*       NU_JSON_Generator_Create
*
*   DESCRIPTION
*
*       Creates an instance of a JSON Generator.
*
*   INPUTS
*
*       buffer_size             The size of the buffer the JSON Generator
*                               should allocate for holding a JSON string.
*       *handle(out)            JSON Generator handle returned by this
*                               function which is to be passed into the
*                               JSON Generator APIs.
*
*   OUTPUTS
*
*       NU_SUCCESS              The JSON Generator's handle was created.
*       NU_INVALID_SIZE         The buffer size specified is zero. The
*                               buffer size must be greater then zero.
*       NU_INVALID_POINTER      The JSON Generator handle passed in is
*                               NU_NULL.
*
*       Otherwise, an operating-system specific error is returned.
*
****************************************************************************/
STATUS NU_JSON_Generator_Create(UINT32 buffer_size, JSON_GEN_HANDLE *handle)
{
    STATUS status;
    _JSON_GEN_HANDLE *internal_handle;
    INT i;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    if (buffer_size == 0)
    {
        status = NU_INVALID_SIZE;
    }
    else if (handle == NU_NULL)
    {
        status = NU_INVALID_POINTER;
    }
    else
    {
        /* Assume handle does NOT get set. */
        *handle = NU_NULL;

        /* Allocate JSON Generator handle. */
        status = NU_Allocate_Memory(JSON_Memory, (VOID**)&internal_handle,
                                   (UNSIGNED)(sizeof(_JSON_GEN_HANDLE) +
                                    buffer_size), (UNSIGNED)NU_SUSPEND);
        if (status == NU_SUCCESS)
        {
            /* Zero out the allocated memory. */
            memset(internal_handle, 0, (sizeof(_JSON_GEN_HANDLE) + buffer_size));

            /* Set buffer pointer to point after the handle's memory. */
            internal_handle->buffer = (CHAR *)(internal_handle + 1);
            internal_handle->buffer_size = buffer_size;

            /* Create our semaphore. */
            status = NU_Create_Semaphore(&(internal_handle->json_semaphore),
                                         "json_gen", (UNSIGNED)1, NU_FIFO);
            if (status == NU_SUCCESS)
            {
                for(i = 0; i < JSON_MAX_DEPTH_LEVEL; ++i)
                {
                    internal_handle->should_prepend_comma[i] = NU_FALSE;
                }
                internal_handle->stack_level = 0;
                internal_handle->is_partial_string = NU_FALSE;

                /* Assign our allocated Generator handle, so that it is
                 * returned. */
                *handle = internal_handle;
            }
            else
            {
                (VOID)NU_Deallocate_Memory(internal_handle);
            }
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
} /* NU_JSON_Generator_Create */

/**************************************************************************
*
*   FUNCTION
*
*       NU_JSON_Generator_Destroy
*
*   DESCRIPTION
*
*       Frees all resources associated with the JSON Generator's
*       handle passed in.
*
*   INPUTS
*
*       *handle                 JSON Generator handle whose resources
*                               will be freed.
*
*   OUTPUTS
*
*       NU_SUCCESS              The JSON Generator's handle was created.
*       NU_INVALID_POINTER      The JSON Generator handle passed in is
*                               NU_NULL.
*
*       Otherwise, an operating-system specific error is returned.
*
****************************************************************************/
STATUS NU_JSON_Generator_Destroy(JSON_GEN_HANDLE *handle)
{
    STATUS status;
    _JSON_GEN_HANDLE *internal_handle;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    status = JSON_Gen_Handle_To_Internal_Handle(handle, &internal_handle);
    if (status == NU_SUCCESS)
    {
        /* Delete the semaphore. If this fails we are in trouble so just
         * return the Nucleus error code. */
        status = NU_Delete_Semaphore(&(internal_handle->json_semaphore));
        if (status == NU_SUCCESS)
        {
            /* Deallocate the handle and adjoining buffer memory. */
            status = NU_Deallocate_Memory(internal_handle);

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

} /* NU_JSON_Generator_Destroy */

/**************************************************************************
*
*   FUNCTION
*
*       NU_JSON_Generator_Start_Token
*
*   DESCRIPTION
*
*       Appends a start of token type to the current JSON data buffer.
*
*   INPUTS
*
*       *handle                 JSON Generator handle that will have a
*                               start token appended to its' buffer.
*       *name                   The token's name. NU_NULL or an empty can
*                               be passed in if no name is tied to this token.
*                               *name is expected to already contain the quotes
*                               around it.
*       token_type              The type of token that is being added to the
*                               JSON String, which should be either:
*                                   JSON_TOKEN_TYPE_OBJECT or
*                                   JSON_TOKEN_TYPE_ARRAY
*
*   OUTPUTS
*
*       NU_SUCCESS              A start token and the name(if provided)
*                               was appended to the JSON buffer.
*       NU_INVALID_POINTER      The JSON Generator handle passed in is
*                               NU_NULL.
*       NUF_JSON_BUFFER_FULL    The current JSON data buffer
*                               is full.
*       NU_INVALID_PARM         The value for token_type is invalid.
*
****************************************************************************/
STATUS NU_JSON_Generator_Start_Token(JSON_GEN_HANDLE *handle, CHAR *name, UINT8 token_type)
{
    STATUS status = NU_SUCCESS;
    _JSON_GEN_HANDLE *internal_handle;
    CHAR prefix[5] = {0};
    CHAR suffix[5] = {0};

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Set name to NULL if it is a zero-length string. */
    if ((name != NU_NULL) && (*name == '\0'))
    {
        name = NU_NULL;
    }

    /* Convert the public handle to a private handle. */
    status = JSON_Gen_Handle_To_Internal_Handle(handle, &internal_handle);
    if (status == NU_SUCCESS)
    {
        if (internal_handle->stack_level >= JSON_MAX_DEPTH_LEVEL - 1)
        {
            status = NUF_JSON_DEPTH_EXCEEDED;
        }

        if (status == NU_SUCCESS)
        {
            status = JSON_GEN_LCK_ENTER(internal_handle);
        }

        if (status == NU_SUCCESS)
        {
            /* If a comma is to be prepended. */
            if ((internal_handle->should_prepend_comma[internal_handle->stack_level] == NU_TRUE) &&
                (internal_handle->name_is_present == NU_FALSE))
            {
                strcat(prefix, ",");
            }

            /* If a name has been specified. */
            if (name != NU_NULL)
            {
                strcat(prefix, "\"");
                strcat(suffix, "\":");
            }

            /* Set the opening character of the token. */
            if (token_type == JSON_TOKEN_TYPE_ARRAY)
            {
                strcat(suffix, "[");
            }
            else
            {
                strcat(suffix, "{");
            }

            /* Add this data to the buffer. */
            status = JSON_Append_Data(internal_handle, prefix, name, suffix);
            if (status == NU_SUCCESS)
            {
                /* Make sure next element starts with a comma. */
                internal_handle->should_prepend_comma[internal_handle->stack_level] = NU_TRUE;
                /* Make sure we reset flag so we know if a name was added. */
                internal_handle->name_is_present = NU_FALSE;
                /* Increment the stack depth. */
                internal_handle->stack_level++;
            }

            /* Release our lock for the JSON Generator. */
            JSON_GEN_LCK_EXIT(internal_handle);
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* NU_JSON_Generator_Start_Token */

/**************************************************************************
*
*   FUNCTION
*
*       NU_JSON_Generator_End_Token
*
*   DESCRIPTION
*
*       Appends an end of token type to the current JSON data buffer.
*
*   INPUTS
*
*       *handle                 JSON Generator handle that is to have
*                               an end of token character added to its'
*                               data buffer.
*       token_type              The type of token that is being terminated
*                               in the JSON data buffer, which should be
*                               either:
*                                   JSON_TOKEN_TYPE_OBJECT
*                                   JSON_TOKEN_TYPE_ARRAY
*
*   OUTPUTS
*
*       NU_SUCCESS              An end token was appended to the JSON buffer.
*       NU_INVALID_POINTER      The JSON Generator handle passed in is
*                               NU_NULL.
*       NUF_JSON_BUFFER_FULL    The current JSON data buffer
*                               is full.
*       NU_INVALID_PARM         The value for token_type is invalid.
*
****************************************************************************/
STATUS NU_JSON_Generator_End_Token(JSON_GEN_HANDLE *handle, UINT8 token_type)
{
    STATUS status = NU_SUCCESS;
    _JSON_GEN_HANDLE *internal_handle;
    CHAR token[] = "}";

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Convert the public handle to a private handle. */
    status = JSON_Gen_Handle_To_Internal_Handle(handle, &internal_handle);
    if (status == NU_SUCCESS)
    {
        /* Get the lock for this JSON Generator. */
        status = JSON_GEN_LCK_ENTER(internal_handle);
        if (status == NU_SUCCESS)
        {
            /* Adjust ending token if it is not an object. */
            if (token_type == JSON_TOKEN_TYPE_ARRAY)
            {
                token[0] = ']';
            }

            /* Add this data to the JSON buffer. */
            status = JSON_Append_Data(internal_handle, NU_NULL, token, NU_NULL);
            if (status == NU_SUCCESS)
            {
                /* Reduce stack depth and reset state back to
                 * no-comma-needed. */
                internal_handle->should_prepend_comma[internal_handle->stack_level] = NU_FALSE;
                internal_handle->stack_level--;
                internal_handle->name_is_present = NU_FALSE;
            }

            /* Release our lock for the JSON Generator. */
            JSON_GEN_LCK_EXIT(internal_handle);
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* NU_JSON_Generator_End_Token */

/**************************************************************************
*
*   FUNCTION
*
*       NU_JSON_Generator_Add_Name
*
*   DESCRIPTION
*
*       Appends an name to the current JSON data buffer.
*
*   INPUTS
*
*       *handle                 JSON Generator handle that is to have
*                               an end of token character added to its'
*                               data buffer.
*       *name                   The name to be appended to the current JSON
*                               data buffer.
*
*   OUTPUTS
*
*       NU_SUCCESS              An name end was appended to the JSON buffer.
*       NU_INVALID_POINTER      The JSON Generator *handle or *name passed in
*                               is NU_NULL.
*       NUF_JSON_BUFFER_FULL    The current JSON data buffer
*                               is full.
*
****************************************************************************/
STATUS NU_JSON_Generator_Add_Name(JSON_GEN_HANDLE *handle, CHAR *name)
{
    STATUS status = NU_SUCCESS;
    _JSON_GEN_HANDLE *internal_handle;
    CHAR prefix_value[] = ",\"";
    CHAR *prefix = prefix_value;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Convert the public handle to a private handle. */
    status = JSON_Gen_Handle_To_Internal_Handle(handle, &internal_handle);
    if (status == NU_SUCCESS)
    {
        /* Get the lock for this JSON Generator. */
        status = JSON_GEN_LCK_ENTER(internal_handle);
        if (status == NU_SUCCESS)
        {
            /* Make sure the name is not NU_NULL or of zero length. */
            if ((name != NU_NULL) && (*name != '\0'))
            {
                /* Check to see if a comma need not be added at this level. */
                if (internal_handle->should_prepend_comma[internal_handle->stack_level] != NU_TRUE)
                {
                    prefix++;
                }

                status = JSON_Append_Data(internal_handle, prefix, name, "\":");
                /* If all our string was added then make sure the next element starts with a comma. */
                if (status == NU_SUCCESS)
                {
                    /* Make sure next element starts with a comma. */
                    internal_handle->should_prepend_comma[internal_handle->stack_level] = NU_TRUE;
                    /* Indicates that a name was added so value part of
                     * this pair shouldn't try to add a comma. */
                    internal_handle->name_is_present = NU_TRUE;
                }
            }
            else
            {
                status = NU_INVALID_POINTER;
            }

            /* Release our lock for the JSON Generator. */
            JSON_GEN_LCK_EXIT(internal_handle);
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* NU_JSON_Generator_Add_Name */

/**************************************************************************
*
*   FUNCTION
*
*       NU_JSON_Generator_Add_String
*
*   DESCRIPTION
*
*       Appends an String to the current JSON data buffer.
*
*   INPUTS
*
*       *handle                 JSON Generator handle that is to have
*                               an end of token character added to its'
*                               data buffer.
*       *jstr                   The string to be appended to the current JSON
*                               data buffer.
*       flags                   Used to indicate if "jstr" is only a partial
*                               value and subsequent call(s) will be made
*                               to add the rest of the value.
*                               Acceptable values:
*                                   0 - Indicates end of "jstr" or that
*                                       "jstr" is NOT a partial string
*                                   JSON_IS_PARTIAL - Indicates that the
*                                   API will be called again with and the
*                                   "jstr" passed in should be appended to the
*                                   previous "jstr" value.
*
*   OUTPUTS
*
*       NU_SUCCESS              A string was appended to the JSON buffer.
*       NU_INVALID_POINTER      The parameters passed to this function are
*                               not valid or are NU_NULL.
*       NUF_JSON_BUFFER_FULL    The current JSON data buffer is full.
*
****************************************************************************/
STATUS NU_JSON_Generator_Add_String(JSON_GEN_HANDLE *handle, CHAR *jstr, UINT8 flags)
{
    STATUS status = NU_SUCCESS;
    _JSON_GEN_HANDLE *internal_handle;
    CHAR prefix_value[] = ",\"";
    CHAR *prefix = prefix_value;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Convert the public handle to a private handle. */
    status = JSON_Gen_Handle_To_Internal_Handle(handle, &internal_handle);
    if (status == NU_SUCCESS)
    {
        /* Get the lock for this JSON Generator. */
        status = JSON_GEN_LCK_ENTER(internal_handle);
        if (status == NU_SUCCESS)
        {
            /* Make sure the name is not NU_NULL. */
            if (jstr != NU_NULL)
            {
                /* Validate our flag parameter. */
                if ((flags == 0) || (flags == JSON_IS_PARTIAL))
                {
                    /* Check to see if a comma should not be added at this level. */
                    if ((internal_handle->should_prepend_comma[internal_handle->stack_level] != NU_TRUE) ||
                       (internal_handle->name_is_present == NU_TRUE))
                    {
                        prefix++;
                    }

                    /* If the value is too big to be passed in by one API call. */
                    if (flags == JSON_IS_PARTIAL)
                    {
                        /* If we are not appending to an existing string,
                         * then this is start of a new string, so make sure
                         * we add an opening quote. Also, if this is start of
                         * partial string via partial flag but the partial
                         * strings exceeds the buffer make sure we call the
                         * same JSON_Append_Data, so that the prefix, data,
                         * and suffix index history positions are maintained. */
                        if (internal_handle->is_partial_string == NU_FALSE)
                        {
                            status = JSON_Append_Data(internal_handle, prefix, jstr, NU_NULL);
                            /* If not all of the data was added then do not
                             * modify the current state so the same call is
                             * repeated later when buffer space is available. */
                            if (status == NU_SUCCESS)
                            {
                                internal_handle->is_partial_string = NU_TRUE;
                            }
                        }

                        /* "jstr" is being appended onto a previous string passed in, so
                         * don't add any starting quotes. */
                        else
                        {
                            status = JSON_Append_Data(internal_handle, NU_NULL, jstr, NU_NULL);
                        }
                    }
                    else
                    {
                        /* If the partial flag isn't set and this API wasn't previous called
                         * then include quotes around both ends of the string. */
                        if (internal_handle->is_partial_string == NU_FALSE)
                        {
                            status = JSON_Append_Data(internal_handle, prefix, jstr, "\"");
                        }
                        /* This is the last call for a multipart string, meaning the user
                         * had previously passed in JSON_PARTIAL_FLAG with a jstr and is now passing in
                         * zero indicating that the final part of "jstr" so add the closing quote. */
                        else
                        {
                            status = JSON_Append_Data(internal_handle, NU_NULL, jstr, "\"");
                            if (status == NU_SUCCESS)
                            {
                                internal_handle->is_partial_string = NU_FALSE;
                            }
                        }

                        /* If all our string was added then make sure the next
                         * element starts with a comma. */
                        if (status == NU_SUCCESS)
                        {
                            /* Make sure next element starts with a comma. */
                            internal_handle->should_prepend_comma[internal_handle->stack_level] = NU_TRUE;
                            /* Make sure we reset flag so we know if a name
                             * was added or not. */
                            internal_handle->name_is_present = NU_FALSE;
                        }
                    }
                }
                else
                {
                    status = NU_INVALID_PARM;
                }
            }
            else
            {
                status = NU_INVALID_POINTER;
            }

            /* Release our lock for the JSON Generator. */
            JSON_GEN_LCK_EXIT(internal_handle);
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* NU_JSON_Generator_Add_String */

/**************************************************************************
*
*   FUNCTION
*
*       NU_JSON_Generator_Add_Boolean
*
*   DESCRIPTION
*
*       Appends a Boolean value to the JSON data buffer.
*
*   INPUTS
*
*       *handle                 JSON Generator handle to be used.
*       value                   The boolean value to add. This can be
*                               either NU_TRUE or NU_FALSE.
*
*   OUTPUTS
*
*       NU_SUCCESS              A string was appended to the JSON buffer.
*       NU_INVALID_POINTER      The parameters passed to this function are
*                               not valid or are NU_NULL.
*       NUF_JSON_BUFFER_FULL    The current JSON data buffer is full.
*
****************************************************************************/
STATUS NU_JSON_Generator_Add_Boolean(JSON_GEN_HANDLE *handle, BOOLEAN value)
{
    STATUS status = NU_SUCCESS;
    _JSON_GEN_HANDLE *internal_handle;
    CHAR *prefix = ",";

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Convert the public handle to a private handle. */
    status = JSON_Gen_Handle_To_Internal_Handle(handle, &internal_handle);
    if (status == NU_SUCCESS)
    {
        /* Get the lock for this JSON Generator. */
        status = JSON_GEN_LCK_ENTER(internal_handle);
        if (status == NU_SUCCESS)
        {
            /* Check to see if a comma should not be added at this level. */
            if ((internal_handle->should_prepend_comma[internal_handle->stack_level] != NU_TRUE) ||
               (internal_handle->name_is_present == NU_TRUE))
            {
                prefix = NU_NULL;
            }

            if (value == NU_TRUE)
            {
                status = JSON_Append_Data(internal_handle, prefix, "true", NU_NULL);
            }
            else
            {
                status = JSON_Append_Data(internal_handle, prefix, "false", NU_NULL);
            }

            /* If the value was added successfully. */
            if (status == NU_SUCCESS)
            {
                /* Make sure next element starts with a comma. */
                internal_handle->should_prepend_comma[internal_handle->stack_level] = NU_TRUE;
                /* Make sure we reset flag so we know if a name was added. */
                internal_handle->name_is_present = NU_FALSE;
            }

            /* Release our lock for the JSON Generator. */
            JSON_GEN_LCK_EXIT(internal_handle);
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
} /* NU_JSON_Generator_Add_Boolean */

/**************************************************************************
*
*   FUNCTION
*
*       NU_JSON_Generator_Add_Int
*
*   DESCRIPTION
*
*       Adds an integer value to the JSON data buffer.
*
*   INPUTS
*
*       *handle                 JSON Generator handle to be used.
*       value                   The integer value to add.
*
*   OUTPUTS
*
*       NU_SUCCESS              A string was appended to the JSON buffer.
*       NU_INVALID_POINTER      The parameters passed to this function are
*                               not valid or are NU_NULL.
*       NUF_JSON_BUFFER_FULL    The current JSON data buffer is full.
*
****************************************************************************/
STATUS NU_JSON_Generator_Add_Int(JSON_GEN_HANDLE *handle, INT64 value)
{
    STATUS status = NU_SUCCESS;
    _JSON_GEN_HANDLE *internal_handle;
    CHAR *prefix = ",";
    CHAR str_value[JSON_MAX_INTEGER_LENGTH];

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Convert the public handle to a private handle. */
    status = JSON_Gen_Handle_To_Internal_Handle(handle, &internal_handle);
    if (status == NU_SUCCESS)
    {
        /* Get the lock for this JSON Generator. */
        status = JSON_GEN_LCK_ENTER(internal_handle);
        if (status == NU_SUCCESS)
        {
            /* Check to see if a comma should not be added at this level. */
            if ((internal_handle->should_prepend_comma[internal_handle->stack_level] != NU_TRUE) ||
               (internal_handle->name_is_present == NU_TRUE))
            {
                prefix = NU_NULL;
            }

            /* Convert our Int64 to a string. */
            (VOID)JSON_Int64_To_String(value, str_value);

            status = JSON_Append_Data(internal_handle, prefix, str_value, NU_NULL);

            /* If the value was added successfully. */
            if (status == NU_SUCCESS)
            {
                /* Make sure next element starts with a comma. */
                internal_handle->should_prepend_comma[internal_handle->stack_level] = NU_TRUE;
                /* Make sure we reset flag so we know if a name was added. */
                internal_handle->name_is_present = NU_FALSE;
            }

            /* Release our lock for the JSON Generator. */
            JSON_GEN_LCK_EXIT(internal_handle);
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
} /* NU_JSON_Generator_Add_Int */



/**************************************************************************
*
*   FUNCTION
*
*       NU_JSON_Generator_Add_UInt
*
*   DESCRIPTION
*
*       Adds an unsigned integer value to the JSON data buffer.
*
*   INPUTS
*
*       *handle                 JSON Generator handle to be used.
*       value                   The unsigned integer value to add.
*
*   OUTPUTS
*
*       NU_SUCCESS              A string was appended to the JSON buffer.
*       NU_INVALID_POINTER      The parameters passed to this function are
*                               not valid or are NU_NULL.
*       NUF_JSON_BUFFER_FULL    The current JSON data buffer is full.
*
****************************************************************************/
STATUS NU_JSON_Generator_Add_UInt(JSON_GEN_HANDLE *handle, UINT64 value)
{
    STATUS status = NU_SUCCESS;
    _JSON_GEN_HANDLE *internal_handle;
    CHAR *prefix = ",";
    CHAR str_value[JSON_MAX_INTEGER_LENGTH];

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Convert the public handle to a private handle. */
    status = JSON_Gen_Handle_To_Internal_Handle(handle, &internal_handle);
    if (status == NU_SUCCESS)
    {
        /* Get the lock for this JSON Generator. */
        status = JSON_GEN_LCK_ENTER(internal_handle);
        if (status == NU_SUCCESS)
        {
            /* Check to see if a comma should not be added at this level. */
            if ((internal_handle->should_prepend_comma[internal_handle->stack_level] != NU_TRUE) ||
               (internal_handle->name_is_present == NU_TRUE))
            {
                prefix = NU_NULL;
            }

            /* Convert our UInt64 to a string. */
            (VOID)JSON_UInt64_To_String(value, str_value);

            status = JSON_Append_Data(internal_handle, prefix, str_value, NU_NULL);

            /* If the value was added successfully. */
            if (status == NU_SUCCESS)
            {
                /* Make sure next element starts with a comma. */
                internal_handle->should_prepend_comma[internal_handle->stack_level] = NU_TRUE;
                /* Make sure we reset flag so we know if a name was added. */
                internal_handle->name_is_present = NU_FALSE;
            }

            /* Release our lock for the JSON Generator. */
            JSON_GEN_LCK_EXIT(internal_handle);
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
} /* NU_JSON_Generator_Add_UInt */

/**************************************************************************
*
*   FUNCTION
*
*       JSON_Is_Valid_Float
*
*   DESCRIPTION
*
*       Validates whether or not *value is a valid float representation.
*
*   INPUTS
*
*       value                   The float value to add in the form of
*                               a null-terminated string.
*
*   OUTPUTS
*
*       NU_TRUE                 *value is a valid representation of a float.
*       NU_FALSE                *value is NOT a valid representation of a float.
*
****************************************************************************/
STATIC BOOLEAN JSON_Is_Valid_Float(CHAR *value)
{
    CHAR *value_trav_ptr = value;
    if (*value_trav_ptr == '-' || *value_trav_ptr == '+')
    {
        value_trav_ptr++;
    }

    if (*value_trav_ptr == '0')
    {
        value_trav_ptr++;
    }
    else if ((*value_trav_ptr >= '1') && (*value_trav_ptr <= '9'))
    {
        value_trav_ptr++;
        while (isdigit((INT)*value_trav_ptr))
        {
            value_trav_ptr++;
        }
    }
    else
    {
        return NU_FALSE;
    }

    if (*value_trav_ptr == '.')
    {
        value_trav_ptr++;
        if (isdigit((INT)*value_trav_ptr))
        {
            while (isdigit((INT)*value_trav_ptr))
            {
                value_trav_ptr++;
            }
        }
        else
        {
            return NU_FALSE;
        }
    }
    else
    {
        return NU_FALSE;
    }

    if ((*value_trav_ptr == 'E') || (*value_trav_ptr == 'e'))
    {
        value_trav_ptr++;
        if ((*value_trav_ptr == '+') || (*value_trav_ptr == '-'))
        {
            value_trav_ptr++;
        }
        if (isdigit((INT)*value_trav_ptr))
        {
            while (isdigit((INT)*value_trav_ptr))
            {
                value_trav_ptr++;
            }
        }
        else
        {
            return NU_FALSE;
        }
    }

    if (*value_trav_ptr == '\0')
    {
        return NU_TRUE;
    }
    else
    {
        return NU_FALSE;
    }

} /* JSON_Is_Valid_Float */

/**************************************************************************
*
*   FUNCTION
*
*       NU_JSON_Generator_Add_Float
*
*   DESCRIPTION
*
*       Adds a float value to the JSON data buffer.
*
*   INPUTS
*
*       *handle                 JSON Generator handle to be used.
*       value                   The float value to add in the form of
*                               a null-terminated string.
*
*   OUTPUTS
*
*       NU_SUCCESS              A string was appended to the JSON buffer.
*       NU_INVALID_POINTER      The parameters passed to this function are
*                               not valid or are NU_NULL.
*       NUF_JSON_BUFFER_FULL    The current JSON data buffer is full.
*       NU_INVALID_PARM         *value is not a valid representation of a float.
*
****************************************************************************/
STATUS NU_JSON_Generator_Add_Float(JSON_GEN_HANDLE *handle, CHAR *value)
{
    STATUS status = NU_SUCCESS;
    _JSON_GEN_HANDLE *internal_handle;
    CHAR *prefix = ",";

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Convert the public handle to a private handle. */
    status = JSON_Gen_Handle_To_Internal_Handle(handle, &internal_handle);
    if (status == NU_SUCCESS)
    {
        if(JSON_Is_Valid_Float(value) == NU_TRUE)
        {
            /* Get the lock for this JSON Generator. */
            status = JSON_GEN_LCK_ENTER(internal_handle);
            if (status == NU_SUCCESS)
            {
                /* Check to see if a comma should not be added at this level. */
                if ((internal_handle->should_prepend_comma[internal_handle->stack_level] != NU_TRUE) ||
                   (internal_handle->name_is_present == NU_TRUE))
                {
                    prefix = NU_NULL;
                }

                status = JSON_Append_Data(internal_handle, prefix, value, NU_NULL);

                /* If the value was added successfully. */
                if (status == NU_SUCCESS)
                {
                    /* Make sure next element starts with a comma. */
                    internal_handle->should_prepend_comma[internal_handle->stack_level] = NU_TRUE;
                    /* Make sure we reset flag so we know if a name was added. */
                    internal_handle->name_is_present = NU_FALSE;
                }

                /* Release our lock for the JSON Generator. */
                JSON_GEN_LCK_EXIT(internal_handle);
            }
        }
        else
        {
            status = NU_INVALID_PARM;
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
} /* NU_JSON_Generator_Add_Float */

/**************************************************************************
*
*   FUNCTION
*
*       NU_JSON_Generator_Add_Null
*
*   DESCRIPTION
*
*       Adds a Null value to the JSON data buffer.
*
*   INPUTS
*
*       *handle                 JSON Generator handle to be used.
*
*   OUTPUTS
*
*       NU_SUCCESS              A string was appended to the JSON buffer.
*       NU_INVALID_POINTER      The parameters passed to this function are
*                               not valid or are NU_NULL.
*       NUF_JSON_BUFFER_FULL    The current JSON data buffer is full.
*
****************************************************************************/
STATUS NU_JSON_Generator_Add_Null(JSON_GEN_HANDLE *handle)
{
    STATUS status = NU_SUCCESS;
    _JSON_GEN_HANDLE *internal_handle;
    CHAR *prefix = ",";

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Convert the public handle to a private handle. */
    status = JSON_Gen_Handle_To_Internal_Handle(handle, &internal_handle);
    if (status == NU_SUCCESS)
    {
        /* Get the lock for this JSON Generator. */
        status = JSON_GEN_LCK_ENTER(internal_handle);
        if (status == NU_SUCCESS)
        {
            /* Check to see if a comma should not be added at this level. */
            if ((internal_handle->should_prepend_comma[internal_handle->stack_level] != NU_TRUE) ||
               (internal_handle->name_is_present == NU_TRUE))
            {
                prefix = NU_NULL;
            }

            status = JSON_Append_Data(internal_handle, prefix, "null", NU_NULL);

            /* If the value was added successfully. */
            if (status == NU_SUCCESS)
            {
                /* Make sure next element starts with a comma. */
                internal_handle->should_prepend_comma[internal_handle->stack_level] = NU_TRUE;
                /* Make sure we reset flag so we know if a name was added. */
                internal_handle->name_is_present = NU_FALSE;
            }

            /* Release our lock for the JSON Generator. */
            JSON_GEN_LCK_EXIT(internal_handle);
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);
} /* NU_JSON_Generator_Add_Null */

/**************************************************************************
*
*   FUNCTION
*
*       NU_JSON_Generator_Get_Buffer
*
*   DESCRIPTION
*
*       Returns the current JSON data buffer associated with this handle.
*
*   INPUTS
*
*       *handle                 JSON Generator handle whose buffer information
*                               is being requested.
*       **buffer(out)           A pointer to the JSON Generator's buffer
*                               associated with *handle.
*       *buffer_length(out)     The total number of bytes being used in **buffer.
*
*   OUTPUTS
*
*       NU_SUCCESS              A pointer to the JSON Data buffer
*                               and its length were returned.
*       NU_INVALID_POINTER      One of the parameters passed in is NU_NULL.
*
****************************************************************************/
STATUS NU_JSON_Generator_Get_Buffer(JSON_GEN_HANDLE *handle, CHAR **buffer, INT *buffer_length)
{
    STATUS status = NU_SUCCESS;
    _JSON_GEN_HANDLE *internal_handle;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    if ((buffer != NU_NULL) && (buffer_length != NU_NULL))
    {
        /* Convert the public handle to a private handle. */

        status = JSON_Gen_Handle_To_Internal_Handle(handle, &internal_handle);
        if (status == NU_SUCCESS)
        {
            /* Get the lock for this JSON Generator. */
            status = JSON_GEN_LCK_ENTER(internal_handle);
            if (status == NU_SUCCESS)
            {
                *buffer = internal_handle->buffer;
                *buffer_length = internal_handle->buffer_pos;

                /* Release our lock for the JSON Generator. */
                JSON_GEN_LCK_EXIT(internal_handle);
            }
        }
    }
    else
    {
        status = NU_INVALID_POINTER;
    }


    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* NU_JSON_Generator_Get_Buffer */

/**************************************************************************
*
*   FUNCTION
*
*       NU_JSON_Generator_Clear_Buffer
*
*   DESCRIPTION
*
*       Clears out the JSON data buffer associated with this handle.
*
*   INPUTS
*
*       *handle                 JSON Generator handle whose data buffer
*                               is to be cleared.
*
*   OUTPUTS
*
*       NU_SUCCESS              The JSON data buffer was cleared.
*       NU_INVALID_POINTER      The JSON Generator handle passed in is
*                               NU_NULL.
*
****************************************************************************/
STATUS NU_JSON_Generator_Clear_Buffer(JSON_GEN_HANDLE *handle)
{
    STATUS status = NU_SUCCESS;
    _JSON_GEN_HANDLE *internal_handle;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Convert the public handle to a private handle. */
    status = JSON_Gen_Handle_To_Internal_Handle(handle, &internal_handle);
    if (status == NU_SUCCESS)
    {
        /* Get the lock for this JSON Generator. */
        status = JSON_GEN_LCK_ENTER(internal_handle);
        if (status == NU_SUCCESS)
        {
            /* Reset all data members of the handle. */
            internal_handle->buffer_pos = 0;

            /* Make sure our first entry in this buffer is a null terminated string.*/
            JSON_GEN_SET_NULL_TERMINATOR(internal_handle);

            /* Release our lock for the JSON Generator. */
            JSON_GEN_LCK_EXIT(internal_handle);
        }
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* NU_JSON_Generator_Clear_Buffer */

/**************************************************************************
*
*   FUNCTION
*
*       JSON_Append_Data
*
*   DESCRIPTION
*
*       Attempts to append the prefix, data and suffix to the current JSON
*       data buffer. If not all data can be appended this function will
*       remember what data was added so that once called again the remaining
*       data can be appended to the data buffer. If not enough space is
*       available, then this function either writes all-or-none for the
*       prefix and the suffix. The data part is only written such as at
*       least one byte of the data is written.
*
*   INPUTS
*
*       *internal_handle        An JSON Generator's private/internal handle.
*       *prefix                 A string that is to be appended to the buffer.
*       *data                   A string that is to be appended to the buffer.
*       *suffix                 A string that is to be appended to the buffer.
*
*   OUTPUTS
*
*       NU_SUCCESS              An name end was appended to the JSON buffer.
*       NUF_JSON_BUFFER_FULL    The current JSON data buffer is full.
*       > 0                     The number of bytes added.
*
****************************************************************************/
STATUS JSON_Append_Data(_JSON_GEN_HANDLE *internal_handle, CHAR *prefix, CHAR *data, CHAR *suffix)
{
    STATUS status = NU_SUCCESS;
    UINT32 prefix_len = 0;
    UINT32 data_len = 0;
    UINT32 suffix_len = 0;

    /* If there is space in the buffer then add the "prefix". */
    if ((prefix != NU_NULL) && (*prefix != '\0'))
    {
        if (JSON_GEN_SPACE_LEFT(internal_handle) > 0)
        {
            prefix_len = strlen(prefix);
            /* If there is still some of the prefix to add then add it. */
            if(internal_handle->prev_prefix_index < prefix_len)
            {
                /* Append our prefix. */
                status = JSON_Append_String(internal_handle, &prefix[internal_handle->prev_prefix_index],
                                            prefix_len - internal_handle->prev_prefix_index);

                /* If all of the prefix was added then record it. */
                if(status == NU_SUCCESS)
                {
                    internal_handle->prev_prefix_index = prefix_len;
                }
                /* If all of the prefix was NOT added then remember, so
                 * that on the next call we know where to pick back up. */
                else
                {
                    internal_handle->prev_prefix_index += status;
                    status = NUF_JSON_BUFFER_FULL;
                }
            }
        }
        else
        {
            status = NUF_JSON_BUFFER_FULL;
        }
    }

    /* If there is still more space in the buffer then add the "data". */
    if ((status == NU_SUCCESS) && (data != NU_NULL) && (*data != '\0'))
    {
        if (JSON_GEN_SPACE_LEFT(internal_handle) > 0)
        {
            data_len = strlen(data);

            /* If any data needs to be added. */
            if (internal_handle->prev_data_index < data_len)
            {
                /* Append our data. */
                status = JSON_Append_String(internal_handle, &data[internal_handle->prev_data_index],
                                            data_len - internal_handle->prev_data_index);
                /* If all of the data was added then update and record it. */
                if (status == NU_SUCCESS)
                {
                    internal_handle->prev_data_index = data_len;
                }
                /* If all of the data was NOT added then remember, so
                 * that on the next call we know where to pick back up. */
                else
                {
                    internal_handle->prev_data_index += status;
                    status = NUF_JSON_BUFFER_FULL;
                }
            }
        }
        else
        {
            status = NUF_JSON_BUFFER_FULL;
        }
    }

    /* If there is still more space in the buffer then add the "suffix". */
    if ((status == NU_SUCCESS) && (suffix != NU_NULL) && (*suffix != '\0'))
    {
        if (JSON_GEN_SPACE_LEFT(internal_handle) > 0)
        {
            suffix_len = strlen(suffix);

            /* If the suffix needs to be added. */
            if (internal_handle->prev_suffix_index < suffix_len)
            {
                /* Append our suffix. */
                status = JSON_Append_String(internal_handle, &suffix[internal_handle->prev_suffix_index],
                                            suffix_len - internal_handle->prev_suffix_index);

                /* If all of the suffix was added then update then record it. */
                if (status == NU_SUCCESS)
                {
                    internal_handle->prev_suffix_index = suffix_len;
                }
                /* If all of the suffix was NOT added then remember, so
                 * that on the next call we know where to pick back up. */
                else
                {
                    internal_handle->prev_suffix_index += status;
                    status = NUF_JSON_BUFFER_FULL;
                }
            }
        }
        else
        {
            status = NUF_JSON_BUFFER_FULL;
        }
    }

    /* If the status is success and all data was added then reset the
     * different indices into the prefix, data and suffix back to zero. */
    if ((status == NU_SUCCESS) &&
       ((internal_handle->prev_prefix_index +
        internal_handle->prev_data_index +
        internal_handle->prev_suffix_index)
        == (prefix_len + data_len + suffix_len)))
    {
        /* Reset our buffer indices. */
        internal_handle->prev_prefix_index = 0;
        internal_handle->prev_data_index = 0;
        internal_handle->prev_suffix_index  = 0;
    }

    return (status);
} /* JSON_Append_Data */

/**************************************************************************
*
*   FUNCTION
*
*       JSON_Append_String
*
*   DESCRIPTION
*
*       Appends a string to the current JSON data buffer. This function
*       does not perform bounds checking and only appends the specified
*       number of bytes to the JSON output buffer.
*
*   INPUTS
*
*       *internal_handle        An JSON Generator's private/internal handle.
*       *data                   A data buffer that is to be appended to the
*                               JSON output buffer.
*       data_len                The length of our data buffer to be appended.
*
*   OUTPUTS
*
*       < 0                     An error occurred.
*       > 0                     The number of bytes added.
*
****************************************************************************/
STATUS JSON_Append_String(_JSON_GEN_HANDLE *internal_handle, CHAR *data, INT data_len)
{
    STATUS status = NU_SUCCESS;
    UINT32 data_added = data_len;
    UINT32 space_left = JSON_GEN_SPACE_LEFT(internal_handle);

    /* If there is not room for all the data then add what we can. */
    if (space_left < data_added)
    {
        data_added = space_left;
        status = data_added;
    }

    /* Copy the data into the buffer. */
    memcpy(&internal_handle->buffer[internal_handle->buffer_pos], data, data_added);
    internal_handle->buffer_pos += data_added;
    JSON_GEN_SET_NULL_TERMINATOR(internal_handle);

    return (status);
} /* JSON_Append_Str */

/**************************************************************************
*
*   FUNCTION
*
*       JSON_Gen_Handle_To_Internal_Handle
*
*   DESCRIPTION
*
*       Converts a public handle to an internal handle.
*
*   INPUTS
*
*       *handle                 A JSON Generator's public handle.
*       **internal_handle(out)  A JSON Generator's private/internal handle.
*                               token.
*   OUTPUTS
*
*       NU_SUCCESS              The handle was converted successfully.
*       NU_INVALID_POINTER      The JSON Generator handle passed in is
*                               NU_NULL.
*
****************************************************************************/
STATUS JSON_Gen_Handle_To_Internal_Handle(JSON_GEN_HANDLE *handle, _JSON_GEN_HANDLE **internal_handle)
{
    STATUS status = NU_SUCCESS;

    if ((handle != NU_NULL) && (*handle != NU_NULL))
    {
        *internal_handle = (_JSON_GEN_HANDLE*)*handle;
    }
    else
    {
        status = NU_INVALID_POINTER;
    }

    return (status);
} /* JSON_Gen_Handle_To_Internal_Handle */

/**************************************************************************
*
*   FUNCTION
*
*       JSON_UInt64_To_String
*
*   DESCRIPTION
*
*       Converts an unsigned 64 bit integer into a string.
*       *string must have enough room for the integer.
*
*   INPUTS
*
*       value                 Unsigned 64 bit integer to be converted.
*       *string               The unsigned 64 bit integer as a string.
*
*   OUTPUTS
*
*       *CHAR                 Pointer to the unsigned 64 bit integer represented
*                             as a string.
*
****************************************************************************/
CHAR *JSON_UInt64_To_String(UINT64 value, CHAR *string)
{
    CHAR    *ptr = string;
    INT64 d;
    INT64 i=1000000000000000000L;
    INT flag=0;

    if (!value)
    {
        *ptr++ = 0x30;
        *ptr = 0;
        return (string);
    }

    for(;i > 0; i/=10)
    {
        d = value / i;


        if (d || flag)
        {
            *ptr++ = (CHAR)(d + 0x30);
            value -= (d * i);
            flag = 1;
        }
    }
    *ptr = 0;


    return string;
}/* JSON_UInt64_To_String */

/**************************************************************************
*
*   FUNCTION
*
*       JSON_Int64_To_String
*
*   DESCRIPTION
*
*       Converts an signed 64 bit integer into a string.
*       *string must have enough room for the integer.
*
*   INPUTS
*
*       value                 Signed 64 bit integer to be converted.
*       *string               The signed 64 bit integer as a string.
*
*   OUTPUTS
*
*       *CHAR                 Pointer to the signed 64 bit integer represented
*                             as a string.
*
****************************************************************************/
CHAR *JSON_Int64_To_String(INT64 value, CHAR *string)
{
    CHAR    *ptr = string;
    INT64 d;
    INT64 i=1000000000000000000L;
    INT flag = 0;

    if (!value)
    {
        *ptr++ = 0x30;
        *ptr = 0;
        return (string);
    }


    /* if this is a negative value insert the minus sign. */
    if (value < 0)
    {
        *ptr++ = '-';

        /* Make the value positive. */
        value *= -1;
    }


    for(;i > 0; i/=10)
    {
        d = value / i;


        if (d || flag)
        {
            *ptr++ = (CHAR)(d + 0x30);
            value -= (d * i);
            flag = 1;
        }
    }
    *ptr = 0;

    return string;
} /* JSON_Int64_To_String */

#endif /* CFG_NU_OS_NET_JSON_INCLUDE_GENERATOR */
