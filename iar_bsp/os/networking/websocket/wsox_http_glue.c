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
*       wsox_http_glue.c
*
*   COMPONENT
*
*       Nucleus WebSocket/HTTP glue layer
*
*   DESCRIPTION
*
*       This file holds the routines to process an upgrade request from HTTP
*       in order to invoke a new connection over the WebSocket stack.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_WSOX_Process_Upgrade_Request
*       WSOX_Parse_Header_Field
*       WSOX_Send_HTTP_Status_Message
*
*   DEPENDENCIES
*
*       nu_networking.h
*       wsox_int.h
*
************************************************************************/

#include "networking/nu_networking.h"
#include "os/networking/websocket/wsox_int.h"

STATIC STATUS WSOX_Send_HTTP_Status_Message(INT socketd, UINT16 status,
                                            CHAR *string, CHAR *extra,
                                            VOID *ssl_ptr);

/************************************************************************
*
*   FUNCTION
*
*       NU_WSOX_Process_Upgrade_Request
*
*   DESCRIPTION
*
*       Routine to parse the components of an HTTP upgrade request into
*       the appropriate data structure format for the WebSocket module
*       to complete and accept the incoming connection.
*
*   INPUTS
*
*       method                  The method(s) being used by the request;
*                               HTTP_LITE_PUT, HTTP_LITE_POST,
*                               HTTP_LITE_DELETE, HTTP_LITE_GET
*       *uri                    The uri associated with the operation.
*       *buf_ptr                A pointer to the HTTP header buffer.
*       buf_len                 The length of the HTTP header buffer.
*       socketd                 The socketd for the connection.
*       *ssl                    The SSL structure associated with the
*                               new connection.
*
*   OUTPUTS
*
*       NU_SUCCESS                  The upgrade request was successfully
*                                   processed.
*
*       Otherwise, a WSOX error code is returned indicating that the
*       connection was not accepted.  The caller does not need to do
*       anything in response to an error code.
*
*************************************************************************/
STATUS NU_WSOX_Process_Upgrade_Request(INT method, CHAR *uri, CHAR *buf_ptr,
                                       INT buf_len, INT socketd, VOID *ssl)
{
    NU_WSOX_CONTEXT_STRUCT  new_handle;
    STATUS                  status, temp_status;
    CHAR                    *version, *code, *extra, *keys;

    /* Zero out the new handle structure. */
    memset(&new_handle, 0, sizeof(new_handle));

    /* Save a pointer to the resource. */
    new_handle.resource = uri;

    /* Initialize status. */
    status = WSOX_PROTO_BAD_REQUEST;
    code = WSOX_BAD_REQUEST;
    extra = NU_NULL;

    /* RFC 2616 section 19.3 - The line terminator for message-header
     * fields is the sequence CRLF. However, we recommend that applications,
     * when parsing such headers, recognize a single LF as a line terminator
     * and ignore the leading CR.
     */
    if (buf_ptr[buf_len - 1] == '\n')
    {
        /* The LF is not needed any longer.  Replace it with a null-terminator
         * so this buffer can be treated as a string.
         */
        buf_ptr[buf_len - 1] = '\0';

        /* Parse out the host name, null-terminate it and store it in the data
         * structure.
         */
        new_handle.host = WSOX_Parse_Header_Field(buf_ptr, buf_len, WSOX_HOST,
                                                  &temp_status);

        /* RFC 6455 - section 4.2.1 - A host header field must be present. */
        if ( (new_handle.host) && (temp_status == NU_SUCCESS) )
        {
            /* Parse out the key, null-terminate it and store it in the data
             * structure.
             */
            keys = WSOX_Parse_Header_Field(buf_ptr, buf_len, WSOX_SEC_KEY,
                                           &temp_status);

            /* RFC 6455 - section 4.2.1 - A key must be present. */
            if ( (keys) && (temp_status == NU_SUCCESS) )
            {
                /* Parse out the version. */
                version = WSOX_Parse_Header_Field(buf_ptr, buf_len, WSOX_SEC_VERSION,
                                                  &temp_status);

                /* RFC 6455 - section 4.2.1 - A version header field must be
                 * present.
                 */
                if ( (version) && (temp_status == NU_SUCCESS) )
                {
                    /* RFC 6455 - section 4.2.2 - If this version does not
                     * match a version understood by the server, the server
                     * MUST abort the WebSocket handshake and instead send
                     * an appropriate HTTP error code (such as 426 Upgrade
                     * Required) and a |Sec-WebSocket-Version| header field
                     * indicating the version(s) the server is capable of
                     * understanding.
                     */
                     if (strcmp(version, WSOX_VERSION) == 0)
                     {
                        status = NU_SUCCESS;
                     }

                     else
                     {
                         status = WSOX_PROTO_UPDATE_REQUIRED;
                         code = WSOX_UPDATE_REQUIRED;
                         extra = "Sec-WebSocket-Version: 13\r\n";
                     }
                }
            }
        }
    }

    /* If a key and version field were found, parse out the rest of the
     * headers.
     */
    if (status == NU_SUCCESS)
    {
        /* RFC 6455 section 1.3 - The |Origin| header field [RFC6454]
         * is used to protect against unauthorized cross-origin use
         * of a WebSocket server by scripts using the WebSocket API
         * in a web browser.  The server is informed of the script
         * origin generating the WebSocket connection request.  If
         * the server does not wish to accept connections from this
         * origin, it can choose to reject the connection by sending
         * an appropriate HTTP error code.
         */
        new_handle.origins = WSOX_Parse_Header_Field(buf_ptr, buf_len,
                                                     WSOX_ORIGIN, &status);

        if (status == NU_SUCCESS)
        {
            /* RFC 6455 section 1.3 - The |Sec-WebSocket-Protocol|
             * request-header field can be used to indicate what
             * subprotocols (application-level protocols layered
             * over the WebSocket Protocol) are acceptable to the
             * client. The server selects one or none of the
             * acceptable protocols and echoes that value in its
             * handshake to indicate that it has selected that protocol.
             */
            new_handle.protocols = WSOX_Parse_Header_Field(buf_ptr, buf_len,
                                                           WSOX_SEC_PROTOCOL,
                                                           &status);
        }

        if (status == NU_SUCCESS)
        {
            /* Parse out the extensions. */
            new_handle.extensions = WSOX_Parse_Header_Field(buf_ptr, buf_len,
                                                            WSOX_SEC_EXT, &status);
        }
    }

    if (status == NU_SUCCESS)
    {
        /* Invoke the routine to accept the new connection. */
        status = NU_WSOX_Accept(&new_handle, socketd, keys, ssl);

        if (status != NU_SUCCESS)
        {
            /* RFC 6455 - section 4.2.2 - If the server does not wish to accept
             * this connection, it MUST return an appropriate HTTP error code
             * (e.g., 403 Forbidden) and abort the WebSocket handshake.
             */
            if (status == WSOX_BAD_ORIGIN)
            {
                status = WSOX_PROTO_FORBIDDEN;
                code = WSOX_FORBIDDEN;
            }

            /* RFC 6455 - section 4.2.2 - If the requested service is not
             * available, the server MUST send an appropriate HTTP error code
             * (such as 404 Not Found) and abort the WebSocket handshake.
             */
            else if (status == WSOX_NO_RESOURCE)
            {
                status = WSOX_PROTO_NOT_FOUND;
                code = WSOX_NOT_FOUND;
            }

            /* The maximum number of connections have been reached for this
             * listening handle.
             */
            else if (status == WSOX_MAX_CONXNS)
            {
                status = WSOX_PROTO_SRVC_UNAVAILABLE;
                code = WSOX_SRVC_UNAVAILABLE;
            }

            /* An internal error occurred. */
            else
            {
                status = WSOX_PROTO_SERVER_ERROR;
                code = WSOX_SERVER_ERROR;
            }
        }
    }

    /* If the client request has failed, send a status message to the
     * client.
     */
    if (status != NU_SUCCESS)
    {
        /* Ignore the return value since it is irrelevant to the HTTP
         * module.
         */
        (VOID)WSOX_Send_HTTP_Status_Message(socketd, status, code, extra, ssl);
    }

    /* Return an error for now until full implementation is in place.  Otherwise,
     * the connection stays open on the other side indefinitely.
     */
    return (status);

} /* NU_WSOX_Process_Upgrade_Request */

/************************************************************************
*
*   FUNCTION
*
*       WSOX_Parse_Header_Field
*
*   DESCRIPTION
*
*       This routine parses the string associated with a header from the
*       packet, and null-terminates the string in the provided buffer.
*       All extra whitespace found before the start of the string and
*       after the end of the string but before the null-terminator is
*       removed from the string.  Extra whitespace within the fields
*       of a comma-separated list is not removed from the string.
*
*   INPUTS
*
*       *buf_ptr                The buffer in which to find the target
*                               string.
*       buf_len                 The length of the buffer.
*       *target                 The target header type to find.
*       *status                 If a malformed header of the target type
*                               is found, this value is set to
*                               HTTP_PROTO_BAD_REQUEST; otherwise, the
*                               value is set to NU_SUCCESS.  This value
*                               should not be used to determine whether
*                               a header was found, but only if the header
*                               that was found is properly formed.
*
*   OUTPUTS
*
*       A pointer to the string associated with the target header.
*
*************************************************************************/
CHAR *WSOX_Parse_Header_Field(CHAR *buf_ptr, INT buf_len, CHAR *target,
                              STATUS *status)
{
    CHAR    *target_ptr, *temp_ptr;
    INT     i, str_len;
    STATUS  local_status;

    /* Initialize status to success.  Success will be returned unless a malformed
     * header is found.  This status does not indicate whether a header was found.
     */
    local_status = NU_SUCCESS;

    /* Look for the target keyword. */
    target_ptr = WSOX_Find_Token(target, buf_ptr, &buf_ptr[buf_len],
                                 WSOX_TOKEN_CASE_INSENS);

    /* If the keyword is present. */
    if (target_ptr)
    {
        /* Move past the keyword. */
        target_ptr += strlen(target);

        /* Get the remaining length of the buffer so we know what value to use
         * to bound the search.
         */
        str_len = buf_len - ((UNSIGNED)(target_ptr - buf_ptr));

        /* Move past any leading white space. */
        for (i = 0; (i < str_len) && (target_ptr[0] == ' '); i++)
        {
            target_ptr ++;
        }

        if (i < str_len)
        {
            /* Find the terminating character for this header. */
            temp_ptr = WSOX_Find_Token("\r\n", target_ptr, &buf_ptr[buf_len],
                                       WSOX_TOKEN_CASE_INSENS);

            if (temp_ptr)
            {
                /* Null-terminate the string. */
                temp_ptr[0] = '\0';

                /* Back up one byte. */
                temp_ptr --;

                /* Remove any whitespace between the null-terminator and the final
                 * character in the string.
                 */
                while ( (target_ptr != temp_ptr) && (temp_ptr[0] == ' ') )
                {
                    temp_ptr[0] = '\0';
                    temp_ptr --;
                }
            }

            /* This header is malformed.  The header must end in "\r\n". */
            else
            {
                local_status = WSOX_PROTO_BAD_REQUEST;
                target_ptr = NU_NULL;
            }
        }

        /* This header is malformed.  The header must end in "\r\n". */
        else
        {
            local_status = WSOX_PROTO_BAD_REQUEST;
            target_ptr = NU_NULL;
        }
    }

    *status = local_status;

    return (target_ptr);

} /* WSOX_Parse_Header_Field */

/************************************************************************
*
*   FUNCTION
*
*       WSOX_Send_HTTP_Status_Message
*
*   DESCRIPTION
*
*       This routine will send an HTTP status message to the client.
*
*   INPUTS
*
*       socketd                 The socket over which to send the
*                               status message.
*       status                  The status code to insert in the
*                               message.
*       *string                 The string to insert in the message.
*       *extra                  Additional data to be inserted after
*                               the status and string.
*       *ssl_ptr                A pointer to the SSL structure if this
*                               is a secure connection.
*
*   OUTPUTS
*
*       The number of bytes sent or an error code if data could not be
*       sent.
*
*************************************************************************/
STATIC STATUS WSOX_Send_HTTP_Status_Message(INT socketd, UINT16 status,
                                            CHAR *string, CHAR *extra,
                                            VOID *ssl_ptr)
{
    NU_MEMORY_POOL  *memory_ptr;
    INT32           len, bytes_sent;
    STATUS          local_status;
    CHAR            temp[10];
    CHAR            *code_ptr, *buf_ptr;

    /* Convert the status to ASCII. */
    code_ptr = (CHAR *)NU_ITOA(status, temp, 10);

    /* Compute the length of the variable part of the outgoing message.
     * Each field must include 2 bytes for the terminating \r\n, except
     * the code_ptr which includes just 1 byte for the blank space
     * between the code and the string.
     */
    len = (strlen(string) + 2) + (strlen(code_ptr) + 1) +
          (extra ? strlen(extra) : 0) + 2;

    /* Add the fixed length portion. */
    len += WSOX_HTTP_STAT_MSG_FIX_LEN;

    /* Get a pointer to the system memory pool. */
    local_status = NU_System_Memory_Get(&memory_ptr, NU_NULL);

    if (local_status == NU_SUCCESS)
    {
        /* Allocate memory for the output buffer.  Allocate 1 extra byte for the
         * null-terminator so we can use str_x routines to build the buffer.
         */
        local_status = NU_Allocate_Memory(memory_ptr, (VOID**)&buf_ptr, len + 1,
                                          NU_NO_SUSPEND);
    }

    if (local_status == NU_SUCCESS)
    {
        /* Put the version in the buffer. */
        strcpy(buf_ptr, "HTTP/1.1 ");

        /* Add the code. */
        strcat(buf_ptr, code_ptr);
        strcat(buf_ptr, " ");

        /* Add the string. */
        strcat(buf_ptr, string);
        strcat(buf_ptr, "\r\n");

        /* Add the extra data. */
        if (extra)
        {
            /* If this data needs a \r\n terminator, the caller must have
             * included it in the string.
             */
            strcat(buf_ptr, extra);
        }

        /* Add the terminating characters. */
        strcat(buf_ptr, "\r\n");

        /* Send the data. */
        bytes_sent = WSOX_Send(socketd, buf_ptr, len, ssl_ptr);

        /* If data was sent successfully, set status to NU_SUCCESS. */
        if (bytes_sent > 0)
        {
            local_status = NU_SUCCESS;
        }

        else
        {
            local_status = bytes_sent;
        }

        NU_Deallocate_Memory(buf_ptr);
    }

    return (local_status);

} /* WSOX_Send_HTTP_Status_Message */
