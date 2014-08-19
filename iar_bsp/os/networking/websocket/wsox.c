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
*       wsox.c
*
*   COMPONENT
*
*       Nucleus WebSocket
*
*   DESCRIPTION
*
*       This file holds the Nucleus WebSocket routines common between the
*       client and server.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       nu_os_net_wsox_init
*       WSOX_Create_Context
*       WSOX_Find_Listener_Context
*       WSOX_Find_Context_By_Handle
*       WSOX_Search_List_By_Handle
*       WSOX_Search_List
*       WSOX_Compress_Whitespace
*       WSOX_Create_Accept_Key
*       WSOX_Compare_Protocol_Lists
*       WSOX_Setup_Recv_Handle
*       WSOX_TX_Close
*       WSOX_Mask_Data
*       WSOX_Build_Header
*       WSOX_Cleanup_Connection_Entry
*       WSOX_Send_Frame
*       WSOX_SSL_ZC_Send
*       WSOX_Send
*       WSOX_Recv
*       WSOX_Fill_Buffer_Chain
*       WSOX_Check_For_Data
*
*   DEPENDENCIES
*
*       nu_networking.h
*       wsox_int.h
*
************************************************************************/

#include "networking/nu_networking.h"
#include "os/networking/websocket/wsox_int.h"
#include "services/reg_api.h"
#include "services/runlevel_init.h"
#if (WSOX_INCLUDE_CYASSL == NU_TRUE)
#include "os/networking/ssl/lite/cyassl/ctaocrypt/sha.h"
#include "os/networking/ssl/lite/cyassl/ctaocrypt/coding.h"
#endif

UINT32                  WSOX_Handle_Counter = 1;
WSOX_CONTEXT_LIST       WSOX_Listener_List;
NU_SEMAPHORE            WSOX_Resource;
WSOX_PENDING_CLIENTS    WSOX_Pending_List;

extern  WSOX_CONTEXT_LIST   WSOX_Connection_List;
extern  BOOLEAN             WSOX_Server_State;
extern  NU_TASK             WSOX_Master_Task_CB;
extern  VOID                *WSOX_Master_Task_Memory;
extern  INT                 WSOX_Socketd;

STATIC WSOX_CONTEXT_STRUCT *WSOX_Search_List_By_Handle(WSOX_CONTEXT_LIST *list,
                                                       UINT32 handle);
STATIC STATUS WSOX_Fill_Buffer_Chain(NET_BUFFER *buf_ptr, INT socketd,
                                     CHAR *data_ptr, UINT16 first_seg_len,
                                     UINT16 data_len, CHAR *buffer, CHAR *mask,
                                     UINT64 encode_count);
#if (WSOX_INCLUDE_CYASSL == NU_TRUE)
STATIC STATUS WSOX_SSL_ZC_Send(VOID *ssl_ptr, INT socketd, NET_BUFFER *buf_ptr,
                               UINT16 *tx_len);
#endif

/************************************************************************
*
*   FUNCTION
*
*       nu_os_net_wsox_init
*
*   DESCRIPTION
*
*       This function initializes the WebSocket module.  It is the
*       run-time entry point of the product initialization sequence.
*
*   INPUTS
*
*       *path                   Path to the configuration settings.
*       startstop               Whether product is being started or
*                               being stopped.
*
*   OUTPUTS
*
*       status                  NU_SUCCESS is returned if initialization
*                               has completed successfully; otherwise, an
*                               operating system specific error code is
*                               returned.
*
************************************************************************/
STATUS nu_os_net_wsox_init(CHAR *path, INT startstop)
{
    STATUS              status;

    /* Initialize the WebSocket module. */
    if (startstop == RUNLEVEL_START)
    {
        WSOX_Listener_List.head = NU_NULL;
        WSOX_Listener_List.tail = NU_NULL;

        WSOX_Pending_List.head = NU_NULL;
        WSOX_Pending_List.tail = NU_NULL;

        /* Create the synchronization semaphore. */
        status = NU_Create_Semaphore(&WSOX_Resource, "WSOX", (UNSIGNED)1,
                                     NU_PRIORITY_INHERIT);
    }

    /* Shut down the WebSocket module. */
    else if (startstop == RUNLEVEL_STOP)
    {
        /* Get the WebSocket semaphore. */
        status = NU_Obtain_Semaphore(&WSOX_Resource, NU_SUSPEND);

        /* Delete all allocated resources. */
        if (status == NU_SUCCESS)
        {
            /* Clean up the connection list. */
            while (WSOX_Connection_List.head)
            {
                WSOX_Cleanup_Connection_Entry(WSOX_Connection_List.head);
            }

            /* Clean up the listener list. */
            while (WSOX_Listener_List.head)
            {
                WSOX_Cleanup_Connection_Entry(WSOX_Listener_List.head);
            }

            /* Delete the server task. */
            if (WSOX_Server_State == NU_TRUE)
            {
                /* Close the control socket. */
                NU_Close_Socket(WSOX_Socketd);

                /* Stop the task. */
                status = NU_Terminate_Task(&WSOX_Master_Task_CB);

                if (status == NU_SUCCESS)
                {
                    /* Delete the task. */
                    status = NU_Delete_Task(&WSOX_Master_Task_CB);

                    if (status == NU_SUCCESS)
                    {
                        /* Clean up the memory. */
                        NU_Deallocate_Memory(WSOX_Master_Task_Memory);
                    }
                }

                WSOX_Server_State = NU_FALSE;
            }

            NU_Release_Semaphore(&WSOX_Resource);

            /* Delete the semaphore. */
            NU_Delete_Semaphore(&WSOX_Resource);
        }
    }

    else
    {
        status = NU_SUCCESS;
    }

    return (status);

} /* nu_os_net_wsox_init */

/************************************************************************
*
*   FUNCTION
*
*       WSOX_Create_Context
*
*   DESCRIPTION
*
*       Allocate memory for a new context handle based on the incoming
*       handle. All data related to the structure must be passed into
*       this routine at the time of creation.  The user cannot modify
*       the strings contained in this structure after creation since
*       one chunk of memory is allocated for the structure based on
*       the input.
*
*   INPUTS
*
*       *user_ptr               The handle on which to base the new
*                               handle.
*       *status                 The status of the operation.
*
*   OUTPUTS
*
*       A pointer to a new context structure or NU_NULL if memory could
*       not be allocated or there are no available handles in the
*       system.
*
*************************************************************************/
WSOX_CONTEXT_STRUCT *WSOX_Create_Context(NU_WSOX_CONTEXT_STRUCT *user_ptr,
                                         STATUS *status)
{
    NU_MEMORY_POOL          *memory_ptr;
    WSOX_CONTEXT_STRUCT     *new_handle = NU_NULL;
    UINT32                  temp_handle, handle_index;
    INT                     next_offset = 0;

    /* Find a unique handle for the new structure. */
    temp_handle = WSOX_Handle_Counter;

    do
    {
        handle_index = WSOX_Handle_Counter ++;

        /* Check if this handle is unique. */
        if (WSOX_Find_Context_By_Handle(handle_index) == NU_NULL)
        {
            break;
        }

    } while (temp_handle != WSOX_Handle_Counter);

    /* If a handle was found. */
    if (temp_handle != WSOX_Handle_Counter)
    {
        /* Get a pointer to the system memory pool. */
        *status = NU_System_Memory_Get(&memory_ptr, NU_NULL);

        if (*status == NU_SUCCESS)
        {
            /* Allocate a block of memory for the entire structure. */
            *status = NU_Allocate_Memory(MEM_Cached, (VOID**)&new_handle,
                                         sizeof(WSOX_CONTEXT_STRUCT) +
                                         /* Only listening handles need to
                                          * allocate memory for and store these
                                          * values.
                                          */
                                         ((user_ptr->flag & WSOX_LISTENER) ?
                                          sizeof(WSOX_SERVER_STRUCT) +
                                          (strlen(user_ptr->resource) + 1 +
                                          (user_ptr->host ? (strlen(user_ptr->host) + 1) : 0) +
                                          (((user_ptr->protocols) && (strlen(user_ptr->protocols) > 0)) ?
                                           (strlen(user_ptr->protocols) + 1) : 0) +
                                          (user_ptr->origins ? (strlen(user_ptr->origins) + 1) : 0) +
                                          (user_ptr->extensions ? (strlen(user_ptr->extensions) + 1) : 0)) : 0),
                                         NU_SUSPEND);
        }

        if (*status == NU_SUCCESS)
        {
            memset(new_handle, 0, sizeof(WSOX_CONTEXT_STRUCT));

            /* If this is a listener, copy the values into the data structure. */
            if (user_ptr->flag & WSOX_LISTENER)
            {
                /* Set the memory pointer for the server structure. */
                new_handle->wsox_server = (WSOX_SERVER_STRUCT*)&new_handle[1];
                next_offset = sizeof(WSOX_SERVER_STRUCT);

                /* Zero the memory for the server. */
                memset(new_handle->wsox_server, 0, sizeof(WSOX_SERVER_STRUCT));

                /* Store the max connections. */
                new_handle->wsox_server->connections = user_ptr->max_connections;

                /* Set up the memory for host and copy it into the structure. */
                if (user_ptr->host)
                {
                    new_handle->wsox_server->host =
                        (CHAR*)(&new_handle[1]) + next_offset;
                    strcpy(new_handle->wsox_server->host, user_ptr->host);

                    next_offset += (strlen(new_handle->wsox_server->host) + 1);
                }

                /* Set up the memory for resource and copy it into the structure. */
                new_handle->wsox_server->resource =
                    (CHAR*)(&new_handle[1]) + next_offset;
                strcpy(new_handle->wsox_server->resource, user_ptr->resource);

                next_offset += (strlen(new_handle->wsox_server->resource) + 1);

                /* Set up the memory for protocols and copy it into the structure.
                 * The pointer may not be NULL, but the length of the string could
                 * be zero, because the protocols requested by the client do not
                 * match those supported by the server if this is a new context
                 * structure for a connecting client.
                 */
                if ( (user_ptr->protocols) && (strlen(user_ptr->protocols) > 0) )
                {
                    /* Set the pointer and copy the value. */
                    new_handle->wsox_server->protocols =
                        (CHAR*)(&new_handle[1]) + next_offset;
                    strcpy(new_handle->wsox_server->protocols, user_ptr->protocols);

                    /* Remove all the whitespace from the list. */
                    WSOX_Compress_Whitespace(new_handle->wsox_server->protocols);

                    next_offset += (strlen(new_handle->wsox_server->protocols) + 1);
                }

                /* Set up the memory for origins and copy it into the structure. */
                if (user_ptr->origins)
                {
                    /* Set the pointer and copy the value. */
                    new_handle->wsox_server->origins =
                        (CHAR*)(&new_handle[1]) + next_offset;
                    strcpy(new_handle->wsox_server->origins, user_ptr->origins);

                    /* Remove all the whitespace from the list. */
                    WSOX_Compress_Whitespace(new_handle->wsox_server->origins);

                    next_offset += (strlen(new_handle->wsox_server->origins) + 1);
                }

                /* Set up the memory for extensions and copy it into the structure. */
                if (user_ptr->extensions)
                {
                    /* Set the pointer and copy the value. */
                    new_handle->wsox_server->extensions =
                        (CHAR*)(&new_handle[1]) + next_offset;
                    strcpy(new_handle->wsox_server->extensions, user_ptr->extensions);
                }
            }

            /* Copy the remaining data. */
            new_handle->flag = user_ptr->flag;

#if (WSOX_ENABLE_HNDL_NOTIFY == NU_TRUE)
            /* Enable receive notifications by default. */
            new_handle->flag |= WSOX_NOTIFY_RX;
#endif

            /* Assign the handle index. */
            new_handle->user_handle = handle_index;

            /* Copy the callback routines. */
            new_handle->onclose = user_ptr->onclose;
            new_handle->onerror = user_ptr->onerror;
            new_handle->onmessage = user_ptr->onmessage;
            new_handle->onopen = user_ptr->onopen;
        }
    }

    else
    {
        *status = WSOX_NO_HANDLES;
    }

    return (new_handle);

} /* WSOX_Create_Context */

/************************************************************************
*
*   FUNCTION
*
*       WSOX_Find_Listener_Context
*
*   DESCRIPTION
*
*       Find a context structure that has been set up to process a
*       WebSocket connection for the respective parameters.
*
*   INPUTS
*
*       *resource               The resource associated with the pending
*                               connection.
*       *origin                 The node originating the request.
*       *status                 The status of the operation.
*
*   OUTPUTS
*
*       A matching context structure or NU_NULL if no match could be
*       found.
*
*************************************************************************/
WSOX_CONTEXT_STRUCT *WSOX_Find_Listener_Context(CHAR *resource, CHAR *origin,
                                                STATUS *status)
{
    WSOX_CONTEXT_STRUCT *temp_ptr;
    STATUS              local_status;

    /* RFC 6455 section 4.2.2 - If the requested service is not available,
     * the server MUST send an appropriate HTTP error code (such as 404 Not Found)
     * and abort the WebSocket handshake.
     */
    local_status = WSOX_NO_RESOURCE;

    /* Get a pointer to the first entry in the list. */
    temp_ptr = WSOX_Listener_List.head;

    /* Search the list of listeners for a matching context entry. */
    while (temp_ptr)
    {
        /* If the resource matches. */
        if (strcmp(temp_ptr->wsox_server->resource, resource) == 0)
        {
            /* If the client is a web browser, it will supply an origin. */
            if (origin)
            {
                /* If there is no matching origin, this client is ok to access
                 * the resource.
                 */
                if (WSOX_Search_List(temp_ptr->wsox_server->origins, origin) == NU_FALSE)
                {
                    /* This client can access the resource through this context. */
                    local_status = NU_SUCCESS;
                    break;
                }

                else
                {
                    /* RFC 6455 section 4.2.2 - If the server does not wish
                     * to accept this connection, it MUST return an appropriate
                     * HTTP error code (e.g., 403 Forbidden) and abort the
                     * WebSocket handshake described in this section.
                     */
                    local_status = WSOX_BAD_ORIGIN;
                }
            }

            else
            {
                local_status = NU_SUCCESS;
                break;
            }
        }

        /* Get the next entry in the list. */
        temp_ptr = temp_ptr->flink;
    }

    *status = local_status;

    return (temp_ptr);

} /* WSOX_Find_Listener_Context */

/************************************************************************
*
*   FUNCTION
*
*       WSOX_Find_Context_By_Handle
*
*   DESCRIPTION
*
*       Find a context structure by the handle from the list of
*       connections that can send/receive data.
*
*   INPUTS
*
*       handle                  The handle associated with the context
*                               structure.
*
*   OUTPUTS
*
*       A matching context structure or NU_NULL if no match could be
*       found.
*
*************************************************************************/
WSOX_CONTEXT_STRUCT *WSOX_Find_Context_By_Handle(UINT32 handle)
{
    WSOX_CONTEXT_STRUCT *wsox_ptr;

    /* First search the list of active connections. */
    wsox_ptr = WSOX_Search_List_By_Handle(&WSOX_Connection_List, handle);

    /* If no matching context structure was found from the list of
     * connections, check the list of listeners.
     */
    if (!wsox_ptr)
    {
        wsox_ptr = WSOX_Search_List_By_Handle(&WSOX_Listener_List, handle);
    }

    return (wsox_ptr);

} /* WSOX_Find_Context_By_Handle */

/************************************************************************
*
*   FUNCTION
*
*       WSOX_Search_List_By_Handle
*
*   DESCRIPTION
*
*       Find a context structure by the handle from the provided
*       list of context structures.
*
*   INPUTS
*
*       *list                   The list to search.
*       handle                  The handle associated with the context
*                               structure.
*
*   OUTPUTS
*
*       A matching context structure or NU_NULL if no match could be
*       found.
*
*************************************************************************/
STATIC WSOX_CONTEXT_STRUCT *WSOX_Search_List_By_Handle(WSOX_CONTEXT_LIST *list,
                                                       UINT32 handle)
{
    WSOX_CONTEXT_STRUCT *temp_ptr;

    /* Get a pointer to the first entry in the list. */
    temp_ptr = list->head;

    /* Search the list for a matching context entry. */
    while (temp_ptr)
    {
        /* If the handle matches. */
        if (temp_ptr->user_handle == handle)
        {
            break;
        }

        /* Get the next entry in the list. */
        temp_ptr = temp_ptr->flink;
    }

    return (temp_ptr);

} /* WSOX_Search_List_By_Handle */

/************************************************************************
*
*   FUNCTION
*
*       WSOX_Search_List
*
*   DESCRIPTION
*
*       This routine searches a comma-separated list for an occurrence of
*       the target string.
*
*   INPUTS
*
*       *list                   A pointer to the null-terminated, comma
*                               separated list.
*       *target                 A pointer to the target string to find in
*                               the list.
*
*   OUTPUTS
*
*       NU_TRUE                 The string is contained in the list.
*       NU_FALSE                The string is not contained in the list.
*
*************************************************************************/
BOOLEAN WSOX_Search_List(CHAR *list, CHAR *target)
{
    CHAR    *str_start, *str_end;
    BOOLEAN match = NU_FALSE;

    /* Get a pointer to the first entry in the list. */
    str_start = list;

    /* We do not need to bound the loop by the length of the list, because when
     * the context was created, we made sure the list was properly formed.
     */
    while (match == NU_FALSE)
    {
        /* Find the next comma in the list.  There will be no whitespace before
         * this comma since we removed all whitespace from the list when the
         * context was created.
         */
        str_end = strchr(str_start, ',');

        if (str_end)
        {
            /* Null-terminate this string. */
            str_end[0] = '\0';
        }

        /* Check if these two strings match. */
        if (NCL_Stricmp(target, str_start) == 0)
        {
            match = NU_TRUE;
        }

        if (str_end)
        {
            /* Put the comma back in the list. */
            str_end[0] = ',';

            /* Move to the next entry. */
            str_start = &str_end[1];
        }

        /* This is the last entry in the list. */
        else
        {
            break;
        }
    }

    return (match);

} /* WSOX_Search_List */

/************************************************************************
*
*   FUNCTION
*
*       WSOX_Compress_Whitespace
*
*   DESCRIPTION
*
*       This routine removes all whitespace from a null-terminated
*       string.
*
*   INPUTS
*
*       *string_ptr             A pointer to the null-terminated string.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID WSOX_Compress_Whitespace(CHAR *string_ptr)
{
    CHAR *src;
    CHAR *dest;

    src = dest = string_ptr;

    while (*src)
    {
        /* Skip over any whitespace. */
        while (*src == ' ')
        {
            src++;
        }

        /* Overwrite this byte with the non-whitespace byte of data. */
        if (src != dest)
        {
            *dest = *src;
        }

        /* Move forward one byte in each string. */
        if (*src)
        {
            src++;
            dest++;
        }
    }

    /* Null-terminate the string. */
    *dest = '\0';

} /* WSOX_Compress_Whitespace */

/************************************************************************
*
*   FUNCTION
*
*       WSOX_Create_Accept_Key
*
*   DESCRIPTION
*
*       Creates Sec-WebSocket-Key per RFC 6455:
*
*       RFC 6455 - section 1.3 - Sec-WebSocket-Key - For this header field,
*       the server has to take the value (as present in the header field...)
*       and concatenate this with the Globally Unique Identifier
*       "258EAFA5-E914-47DA-95CA-C5AB0DC85B11" in string form, ...
*       A SHA-1 hash (160 bits), base64-encoded of this concatenation is
*       then returned in the server's handshake.
*
*   INPUTS
*
*       *key                    The key to encode.
*       key_len                 The length of key.
*       *buffer                 The buffer into which to encode the key.
*       *buf_len                The length of the buffer on input.  On
*                               return, the number of bytes written to
*                               the buffer.
*
*   OUTPUTS
*
*       NU_SUCCESS              The key was successfully encoded.
*
*       Otherwise, an operating-system specific error is returned.
*
*************************************************************************/
STATUS WSOX_Create_Accept_Key(CHAR *key, INT key_len, CHAR *buffer,
                              INT *buf_len)
{
    STATUS      status;
#if (WSOX_INCLUDE_CYASSL == NU_TRUE)
    Sha         sha;
    byte        shaSum[SHA_DIGEST_SIZE];
#endif

#if (WSOX_INCLUDE_CYASSL == NU_TRUE)
    /* Initialize the hash value. */
    InitSha(&sha);

    /* Add the key to the hash. */
    ShaUpdate(&sha, (byte*)key, key_len);

    /* Add the fixed GUID to the hash. */
    ShaUpdate(&sha, (byte*)WSOX_GUID_CONST, WSOX_GUID_CONST_LEN);

    /* Compute the hash value. */
    ShaFinal(&sha, shaSum);

    /* Base64 encode the hashed value. */
    status = Base64_Encode(shaSum, SHA_DIGEST_SIZE, (byte*)buffer, (word32*)buf_len);
#else
    status = -1;
#endif

    return (status);

} /* WSOX_Create_Accept_Key */

/************************************************************************
*
*   FUNCTION
*
*       WSOX_Compare_Protocol_List
*
*   DESCRIPTION
*
*       This routine checks that all elements in list1 are also
*       found in list 2.  If the prune value is set to NU_TRUE, the
*       routine removes all protocols from list1 that do not match a
*       protocol in list2.
*
*   INPUTS
*
*       *list1                  The list that contains all required
*                               elements.
*       *list2                  The list checked against list1.
*       prune                   NU_TRUE to prune elements from list1
*                               that are not found in list2.
*
*   OUTPUTS
*
*       NU_TRUE                 The two lists contain the same values.
*       NU_FALSE                The two lists do not contain the same
*                               values.
*
*************************************************************************/
BOOLEAN WSOX_Compare_Protocol_Lists(CHAR *list1, CHAR *list2, BOOLEAN prune)
{
    CHAR        *list1_ptr = list1;
    INT         str_len, i;
    STATUS      status = NU_SUCCESS;
    BOOLEAN     match, return_match = NU_TRUE;

    /* Search the local list for a matching protocol in the foreign
     * list.
     */
    do
    {
        /* Move past any leading white space. */
        while (list1_ptr[0] == ' ')
        {
            list1_ptr ++;
        }

        /* Get the remaining length of the protocol header. */
        str_len = strlen(list1_ptr);

        /* While we have not encountered white space or a comma. */
        for (i = 0; (i < str_len) &&
             ((list1_ptr[i] != ' ') && (list1_ptr[i] != ',')); i++)
        {
            ;;
        }

        /* Ensure this is not the last entry in the list. */
        if (i < str_len)
        {
            /* If a comma was encountered, null-terminate the string here
             * and move on to the comparison.
             */
            if (list1_ptr[i] == ',')
            {
                list1_ptr[i] = '\0';
            }

            /* Otherwise, continue to parse the bytes until the end of the
             * string is reached or a comma is found.
             */
            else
            {
                /* Null-terminate the string. */
                list1_ptr[i++] = '\0';

                /* Find the comma and remove it. */
                for (; (i < str_len) && (list1_ptr[i] != ','); i++)
                {
                    ;;
                }

                /* If a comma was encountered, replace it with white space. */
                if ( (i < str_len) && (list1_ptr[i] == ',') )
                {
                    list1_ptr[i] = ' ';
                }
            }
        }

        if (i >= str_len)
        {
            status = WSOX_END_OF_LIST;
        }

        /* Check if the protocol is in the list. */
        match = WSOX_Search_List(list2, list1_ptr);

        if (match == NU_FALSE)
        {
            return_match = NU_FALSE;
        }

        /* If this protocol is not in list2, and list1 should be pruned. */
        if ( (match == NU_FALSE) && (prune == NU_TRUE) )
        {
            /* If this is the last entry in the client's list. */
            if (status == WSOX_END_OF_LIST)
            {
                /* Find the last comma in the string of supported
                 * protocols.
                 */
                list1_ptr = strrchr(list1, ',');

                if (list1_ptr)
                {
                    /* Remove the last comma to indicate the end of
                     * the list.
                     */
                    list1_ptr[0] = '\0';
                }

                /* There are no matching protocols. */
                else
                {
                    /* NULL terminate protocols at the first byte.  Do
                     * not just set the pointer to NULL, as the caller
                     * might need to deallocate this memory.
                     */
                    list1[0] = '\0';
                }
            }

            /* If this isn't the last entry in the client's list, remove
             * the protocol and the comma by moving the rest of the
             * protocol list (including the null-terminator) into the
             * space held by the unsupported protocol.
             */
            else
            {
                memmove(list1_ptr, &list1_ptr[i+1], strlen(&list1_ptr[i+1]) + 1);
            }
        }

        /* Put the comma back in the packet. */
        else if ( (status != WSOX_END_OF_LIST) &&
                  ((match == NU_TRUE) || (prune == NU_FALSE)) )
        {
            /* Add the comma back into the list. */
            list1_ptr[strlen(list1_ptr)] = ',';

            /* Move past this entry, the comma, and any trailing white
             * space that may have come before the comma.
             */
            list1_ptr = &list1_ptr[i+1];
        }

    } while (status == NU_SUCCESS);

    return (return_match);

} /* WSOX_Compare_Protocol_Lists */

/************************************************************************
*
*   FUNCTION
*
*       WSOX_Setup_Recv_Handle
*
*   DESCRIPTION
*
*       This routine informs the server thread to begin processing
*       incoming data for the handle.
*
*   INPUTS
*
*       *wsox_ptr               The handle for which the server should
*                               begin handling incoming data.
*
*   OUTPUTS
*
*       NU_SUCCESS              The handle has been set up for receive
*                               functionality.
*
*       Otherwise, an operating-system specific error code is returned
*       indicating that the server could not be started.
*
*************************************************************************/
STATUS WSOX_Setup_Recv_Handle(WSOX_CONTEXT_STRUCT *wsox_ptr)
{
    STATUS      status;


    /* Notify the server master task that a new connection has been
     * accepted.
     */
    status = WSOX_Resume_Server();

    /* If the server could not be started, fail the WebSocket connection. */
    if (status == NU_SUCCESS)
    {
        /* Set the state of the connection to open and denote that this
         * is a new connection, and the application needs to be notified.
         */
        wsox_ptr->flag |= (WSOX_OPEN | WSOX_NEW_CNXN);

        /* Add the connection to the list of open connections. */
        DLL_Enqueue(&WSOX_Connection_List, wsox_ptr);
    }

    return (status);

} /* WSOX_Setup_Recv_Handle */

/************************************************************************
*
*   FUNCTION
*
*       WSOX_TX_Close
*
*   DESCRIPTION
*
*       Builds and sends a close frame to the other side of the
*       connection.
*
*   INPUTS
*
*       socketd                 The socket over which to send the close.
*       mask                    NU_TRUE if the data should be masked.
*       status_code             The status code to insert in the close
*                               frame.
*       *reason                 The optional reason to insert in the close
*                               frame.
*       *ssl_ptr                A pointer to the SSL structure if this is
*                               a secure connection.
*
*   OUTPUTS
*
*       NU_SUCCESS              The close frame was successfully sent.
*       WSOX_MSG_TOO_BIG        The close frame is too large.  Try
*                               reducing the length of the reason and
*                               resubmit.
*
*       Otherwise, an operating-system specific error code is returned.
*
*************************************************************************/
STATUS WSOX_TX_Close(INT socketd, BOOLEAN mask, UINT16 status_code,
                     CHAR *reason, VOID *ssl_ptr)
{
    NET_BUFFER      *buf_ptr;
    CHAR            *data_ptr;
    UINT64          pkt_len = 0;
    UINT8           hdr_len = WSOX_MIN_HDR_LEN;
    INT32           bytes_sent;
    STATUS          status;

#if (WSOX_INCLUDE_CYASSL == NU_TRUE)
    if (!ssl_ptr)
#endif
    {
        /* Enable Zero Copy on the socket. */
        NU_Fcntl(socketd, NU_SET_ZC_MODE, NU_ZC_ENABLE);
    }

    /* If a status code is included, include memory for the two-byte
     * status code and the optional reason.
     */
    if (status_code)
    {
        /* Compute the payload length. */
        pkt_len =
            ((reason != NU_NULL ? strlen(reason) : 0) + WSOX_STATUS_CODE_LEN);
    }

    /* RFC 6455 - section 5.5.1 - Close frames sent from client to server
     * must be masked.
     */
    if ( (mask) && (status_code) )
    {
        hdr_len += WSOX_MASK_LEN;
    }

    /* RFC 6455 - section 5.5 - All control frames MUST have a payload
     * length of 125 bytes or less.
     */
    if ((pkt_len + hdr_len) <= WSOX_MAX_CTRL_FRAME_LEN)
    {
        /* Allocate a buffer chain for this transmission. */
        status = NU_ZC_Allocate_Buffer(&buf_ptr, (UINT16)(pkt_len + hdr_len), socketd);

        if (status > 0)
        {
            /* The control frame cannot be bigger than 125 bytes, and the minimum
             * size of a segment is 128 bytes.  Check that the user did not
             * decrease this minimum below the recommended value.  If they did,
             * we cannot send the close message of this size.
             */
            if (NU_ZC_SEGMENT_BYTES_LEFT(buf_ptr, buf_ptr, socketd) >=
                (pkt_len + hdr_len))
            {
                /* Get the first segment in the buffer chain. */
                data_ptr = NU_ZC_SEGMENT_DATA(buf_ptr);

                /* Build the header. */
                WSOX_Build_Header(NU_NULL, data_ptr, WSOX_CLOSE_FRAME, pkt_len,
                                  mask, hdr_len, 0);

                /* Add the status code and reason to the packet. */
                if (status_code)
                {
                    /* Add the two byte status code. */
                    PUT16(data_ptr, hdr_len, status_code);

                    /* If the user has specified a string to send. */
                    if (reason)
                    {
                        memcpy(&data_ptr[hdr_len + WSOX_STATUS_CODE_LEN],
                               reason, strlen(reason));
                    }

                    /* If this is a client, the payload must be masked. */
                    if (mask)
                    {
                        /* Mask the data. */
                        WSOX_Mask_Data(&data_ptr[hdr_len], &data_ptr[hdr_len],
                                       &data_ptr[hdr_len - WSOX_MASK_LEN], pkt_len, 0);
                    }
                }

                /* Don't delay sending this data. */
                NU_Push(socketd);

#if (WSOX_INCLUDE_CYASSL == NU_TRUE)
                if (ssl_ptr)
                {
                    /* Set the number of bytes for transmission. */
                    bytes_sent = (UINT16)(pkt_len + hdr_len);

                    status = WSOX_SSL_ZC_Send(ssl_ptr, socketd, buf_ptr, (UINT16*)&bytes_sent);
                }

                else
#endif
                {
                    /* Send the close frame. */
                    bytes_sent = NU_ZC_Send(socketd, buf_ptr, (UINT16)(pkt_len + hdr_len), 0);

                    if (bytes_sent > 0)
                    {
                        status = NU_SUCCESS;
                    }

                    /* If an error occurred during transmission, deallocate the network
                     * buffer chain.
                     */
                    else
                    {
                        status = bytes_sent;
                    }
                }
            }

            else
            {
                status = WSOX_MSG_TOO_BIG;
            }

            if (status < 0)
            {
                NU_ZC_Deallocate_Buffer(buf_ptr);
            }
        }
    }

    else
    {
        status = WSOX_MSG_TOO_BIG;
    }

#if (WSOX_INCLUDE_CYASSL == NU_TRUE)
    if (!ssl_ptr)
#endif
    {
        /* Disable Zero Copy mode on the socket. */
        NU_Fcntl(socketd, NU_SET_ZC_MODE, NU_ZC_DISABLE);
    }

    return (status);

} /* WSOX_TX_Close */

/************************************************************************
*
*   FUNCTION
*
*       WSOX_Mask_Data
*
*   DESCRIPTION
*
*       Masks the WebSocket data per RFC 6455, section 5.3.
*
*   INPUTS
*
*       *dest                   The destination buffer of the masked
*                               data.
*       *src                    The data being masked.
*       *mask                   The 32-bit mask to use to mask the
*                               data.
*       len                     The length of the data being masked.
*       encode_count            The number of bytes already masked in
*                               the frame.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID WSOX_Mask_Data(CHAR *dest, CHAR *src, CHAR *mask, UINT64 len,
                    UINT64 encode_count)
{
    UINT64      j;

    /* Add each byte to the buffer, and mask it. */
    for (j = 0; j < len; encode_count ++, j ++, src ++)
    {
        dest[j] = (CHAR)(src[0] ^ mask[encode_count % 4]);
    }

} /* WSOX_Mask_Data */

/************************************************************************
*
*   FUNCTION
*
*       WSOX_Build_Header
*
*   DESCRIPTION
*
*       Builds a WebSocket header in the provided buffer.
*
*   INPUTS
*
*       *wsox_ptr               A pointer to the handle structure for
*                               which the header is being built or
*                               NU_NULL if there is no connected handle
*                               associated with this packet.
*       *buffer                 The buffer into which to build the header.
*       opcode                  The data type of the payload.
*       data_len                The total payload length.
*       mask                    NU_TRUE if the payload is masked.
*                               NU_FALSE if the payload is not masked.
*       hdr_len                 The total length of the header being
*                               built.
*       flags                   Flags associated with the frame being
*                               built.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID WSOX_Build_Header(WSOX_CONTEXT_STRUCT *wsox_ptr, CHAR *buffer,
                       INT opcode, UINT64 data_len, BOOLEAN mask,
                       UINT8 hdr_len, UINT8 flags)
{
    /* If the fragment flag is clear. */
    if (!(flags & NU_WSOX_FRAGMENT))
    {
        /* If this is the final fragment in the stream. */
        if ( (wsox_ptr) && (wsox_ptr->cli_frag_opcode != 0) )
        {
            opcode = 0;
            wsox_ptr->cli_frag_opcode = 0;
        }

        /* Set the final fragment bit. */
        opcode |= 0x80;
    }

    /* RFC 6455 - section 5.4 - A fragmented message consists of a single
     * frame with the FIN bit clear and an opcode other than 0, followed
     * by zero or more frames with the FIN bit clear and the opcode set
     * to 0, and terminated by a single frame with the FIN bit set and
     * an opcode of 0.
     */
    else
    {
        /* This is the first fragment in the stream. */
        if ( (wsox_ptr) && (wsox_ptr->cli_frag_opcode == 0) )
        {
            /* Store the opcode to use for the rest of the fragments. */
            wsox_ptr->cli_frag_opcode = opcode;
        }

        else
        {
            opcode = 0;
        }
    }

    /* Add the opcode to the field in the buffer. */
    buffer[WSOX_FIN_RSV_OPCODE_OFFSET] = opcode;

    /* Put the payload length in the buffer. */
    if (data_len <= 125)
    {
        PUT8(buffer, WSOX_MSK_LEN_OFFSET, data_len);
    }

    /* The data length is 2 bytes. */
    else if (data_len <= 65535)
    {
        PUT8(buffer, WSOX_MSK_LEN_OFFSET, 126);
        PUT16(buffer, WSOX_EXT_MSK_LEN_OFFSET, data_len);
    }

    /* The data length is 8 bytes. */
    else
    {
        PUT8(buffer, WSOX_MSK_LEN_OFFSET, 127);
        PUT64(buffer, WSOX_EXT_MSK_LEN_OFFSET, data_len);
    }

    /* Ensure the caller is not generating a mask for no data. */
    if ( (mask) && (data_len) )
    {
        /* Set the mask bit to TRUE. */
        buffer[WSOX_MSK_LEN_OFFSET] |= 0x80;

        /* Ensure random number generator is seeded. */
        NU_RTL_Rand_Seed();

        /* Put the masking key in the buffer. */
        PUT32(buffer, (hdr_len - WSOX_MASK_LEN), (UINT32)rand());
    }

} /* WSOX_Build_Header */

/************************************************************************
*
*   FUNCTION
*
*       WSOX_Cleanup_Connection_Entry
*
*   DESCRIPTION
*
*       This function cleans up the data associated with an internal
*       connection handle structure.
*
*   INPUTS
*
*       *wsox_ptr               A pointer to the handle structure
*                               to clean up.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID WSOX_Cleanup_Connection_Entry(WSOX_CONTEXT_STRUCT *wsox_ptr)
{
    WSOX_CONTEXT_STRUCT *cnxn_ptr;

    /* If this is not a listener handle. */
    if (!(wsox_ptr->flag & WSOX_LISTENER))
    {
        /* Remove the connection handle from the list of active connections. */
        DLL_Remove(&WSOX_Connection_List, wsox_ptr);

        /* If this connection was created from a listener handle. */
        if (wsox_ptr->wsox_server)
        {
            /* Increment the max number of connections this listener can
             * accept.
             */
            wsox_ptr->wsox_server->connections ++;
        }

        /* Close the socket. */
        NU_Close_Socket(wsox_ptr->socketd);
    }

    else
    {
        /* Invalidate the server pointer in any open connections that were
         * created from this listener.
         */
        cnxn_ptr = WSOX_Connection_List.head;

        while (cnxn_ptr)
        {
            /* If this connection was created from the listener being
             * deleted.
             */
            if (cnxn_ptr->wsox_server == wsox_ptr->wsox_server)
            {
                cnxn_ptr->wsox_server = NU_NULL;
            }

            cnxn_ptr = cnxn_ptr->flink;
        }

        /* Remove the entry from the list of listeners. */
        DLL_Remove(&WSOX_Listener_List, wsox_ptr);

        /* Deallocate the listener specific memory. */
        NU_Deallocate_Memory(wsox_ptr->wsox_server);
    }

#if (WSOX_INCLUDE_CYASSL == NU_TRUE)
    if (wsox_ptr->ssl)
    {
        /* Free this SSL context structure. */
        CyaSSL_free(wsox_ptr->ssl);
    }

    if (wsox_ptr->ctx)
    {
        CyaSSL_CTX_free(wsox_ptr->ctx);
    }
#endif

    /* If this handle was in the process of receiving a close frame, deallocate
     * the memory that was allocated for the data.
     */
    if (wsox_ptr->close_reason)
    {
        NU_Deallocate_Memory(wsox_ptr->close_reason);
    }

    /* Deallocate memory for the connection handle. */
    NU_Deallocate_Memory(wsox_ptr);

} /* WSOX_Cleanup_Connection_Entry */

/************************************************************************
*
*   FUNCTION
*
*       WSOX_Send_Frame
*
*   DESCRIPTION
*
*       Builds a zero copy chain of buffers for the outgoing frame and
*       transmits it to the other side of the connection.
*
*   INPUTS
*
*       *wsox_ptr               The connection over which to send the data.
*       *buffer                 A pointer to the buffer of data to
*                               send.  This buffer can be NU_NULL if
*                               data_len is zero.
*       *tx_data_len            The length of the data in the buffer on
*                               input.  On return, the number of bytes
*                               transmitted. This value can be zero if
*                               buffer is NU_NULL.
*       opcode                  The type of data in the buffer.
*       flags                   Flags associated with the transmission:
*                                   NU_WSOX_FRAGMENT - There are more bytes
*                                   to be included in the message than being
*                                   transmitted with this function call.
*       byte_position           The starting byte position of the new
*                               data, used by the masking routine.
*       *mask_ptr               For the first call to transmit a frame,
*                               this parameter will be filled in with the
*                               mask used to mask the frame data. If not
*                               all bytes can be transmitted in a single
*                               call to this routine, this same mask must be
*                               submitted by the caller.
*
*   OUTPUTS
*
*       NU_SUCCESS              At least some of the data was successfully
*                               sent.
*       WSOX_CNXN_ERROR         The connection was lost while trying to
*                               send the data.
*
*       Otherwise, an operating-system specific error is returned.
*
*************************************************************************/
STATUS WSOX_Send_Frame(WSOX_CONTEXT_STRUCT *wsox_ptr, CHAR *buffer,
                       UINT64 *tx_data_len, INT opcode, UINT8 flags,
                       UINT64 byte_position, CHAR *mask_ptr)
{
    NET_BUFFER  *buf_ptr, *seg_ptr;
    CHAR        *data_ptr;
    STATUS      status;
    UINT64      bytes_copied, data_len = *tx_data_len;
    UINT32      segment_len;
    INT32       bytes_sent;
    UINT16      current_tx_len;
    UINT8       hdr_len;

#if (WSOX_INCLUDE_CYASSL == NU_TRUE)
    if (!wsox_ptr->ssl)
#endif
    {
        /* Enable zero copy mode on the socket for transmitting data so
         * we don't have to allocate memory for encoding the data.
         */
        NU_Fcntl(wsox_ptr->socketd, NU_SET_ZC_MODE, NU_ZC_ENABLE);
    }

    /* If a header is to be included in this transmission. */
    if (!(flags & NU_WSOX_NO_HDR))
    {
        /* Initialize the header length to the minimum required size. */
        hdr_len = WSOX_MIN_HDR_LEN;

        /* Determine how many additional bytes the payload length field
         * requires, if any.
         */
        if (data_len > 125)
        {
            /* The payload length field requires 2 additional bytes. */
            if (data_len <= 65535)
            {
                hdr_len += WSOX_16BIT_PAYLOAD_LEN;
            }

            /* The payload length field requires 8 additional bytes. */
            else
            {
                hdr_len += WSOX_64BIT_PAYLOAD_LEN;
            }
        }

        /* If the handle is a client, include memory for the mask. */
        if (wsox_ptr->flag & WSOX_CLIENT)
        {
            hdr_len += WSOX_MASK_LEN;
        }
    }

    else
    {
        hdr_len = 0;
    }

    /* Initialize the number of bytes that have been copied into the buffer. */
    bytes_copied = 0;

    /* Send the data using Zero Copy buffers.  NU_ZC_Send can only send
     * UINT16 bytes of data with each call, so the data may need to be
     * sent in successive calls if the payload is greater than 65535.
     */
    do
    {
        /* Determine the number of bytes to send in the current call
         * to NU_ZC_Send.  Note that hdr_len is only non-zero the first
         * time through the loop to account for the header.
         */
        if (((data_len - bytes_copied) + hdr_len) > WSOX_MAX_DATA_IO)
        {
            current_tx_len = WSOX_MAX_DATA_IO;
        }

        else
        {
            current_tx_len = (UINT16)((data_len - bytes_copied) + hdr_len);
        }

        /* Allocate a buffer chain for this transmission. */
        status = NU_ZC_Allocate_Buffer(&buf_ptr, current_tx_len,
                                       wsox_ptr->socketd);

        if (status > 0)
        {
            status = NU_SUCCESS;

            seg_ptr = buf_ptr;

            /* Get the first segment in the buffer chain */
            data_ptr = NU_ZC_SEGMENT_DATA(seg_ptr);

            /* Determine how many bytes will fit in this segment. */
            segment_len = NU_ZC_SEGMENT_BYTES_LEFT(buf_ptr, seg_ptr,
                                                   wsox_ptr->socketd);

            if (segment_len > 0)
            {
                /* The header is only built for the first transmission. */
                if (hdr_len)
                {
                    /* Build the header.  Only mask the data if the handle is a
                     * client.
                     */
                    WSOX_Build_Header(wsox_ptr, data_ptr, opcode, data_len,
                                      wsox_ptr->flag & WSOX_CLIENT ? NU_TRUE : NU_FALSE,
                                      hdr_len, flags);

                    /* Only mask the data if this is a client handle. */
                    if ( (data_len) && (wsox_ptr->flag & WSOX_CLIENT) )
                    {
                        /* Return the mask to the caller for subsequent calls to transmit this
                         * frame of data.
                         */
                        memcpy(mask_ptr, &data_ptr[hdr_len - WSOX_MASK_LEN], WSOX_MASK_LEN);
                    }

                    /* Decrement the remaining length of the segment by the number
                     * of bytes in the header.
                     */
                    segment_len -= hdr_len;

                    /* Move the data pointer past the header. */
                    data_ptr += hdr_len;
                }

                if (data_len)
                {
                    /* Fill the buffer chain with the encoded data. */
                    status = WSOX_Fill_Buffer_Chain(buf_ptr, wsox_ptr->socketd, data_ptr,
                                                    segment_len, current_tx_len - hdr_len, buffer,
                                                    wsox_ptr->flag & WSOX_CLIENT ? mask_ptr : NU_NULL,
                                                    bytes_copied + byte_position);
                }

                if (status == NU_SUCCESS)
                {
#if (WSOX_INCLUDE_CYASSL == NU_TRUE)
                    if (wsox_ptr->ssl)
                    {
                        status = WSOX_SSL_ZC_Send(wsox_ptr->ssl, wsox_ptr->socketd,
                                                  buf_ptr, &current_tx_len);

                        /* Increment the number of bytes sent. */
                        bytes_copied += (current_tx_len - hdr_len);

                        /* Don't build the header again. */
                        hdr_len = 0;
                    }

                    else
#endif
                    {
                        /* Send this part of the frame.  There may be more data to send if
                         * the payload length is greater than UINT16.
                         */
                        bytes_sent = NU_ZC_Send(wsox_ptr->socketd, buf_ptr, current_tx_len, 0);

                        if (bytes_sent > 0)
                        {
                            /* Increment the number of bytes copied into the buffer. */
                            bytes_copied += (bytes_sent - hdr_len);

                            /* Don't build the header again. */
                            hdr_len = 0;

                            status = NU_SUCCESS;
                        }

                        else
                        {
                            status = bytes_sent;
                        }
                    }
                }
            }

            /* The connection has been closed. */
            else
            {
                status = NU_NOT_CONNECTED;
            }

            if (status != NU_SUCCESS)
            {
                NU_ZC_Deallocate_Buffer(buf_ptr);
            }
        }

    } while ( (bytes_copied < data_len) && (status == NU_SUCCESS) );

    /* Return the number of bytes transmitted. */
    *tx_data_len = bytes_copied;

#if (WSOX_INCLUDE_CYASSL == NU_TRUE)
    if (!wsox_ptr->ssl)
#endif
    {
        /* Disable zero copy mode on the socket since we are finished
         * sending data.
         */
        NU_Fcntl(wsox_ptr->socketd, NU_SET_ZC_MODE, NU_ZC_DISABLE);
    }

    return (status);

} /* WSOX_Send_Frame */

/************************************************************************
*
*   FUNCTION
*
*       WSOX_SSL_ZC_Send
*
*   DESCRIPTION
*
*       Transmits a zero copy buffer chain over the SSL connection.
*
*   INPUTS
*
*       *ssl_ptr                The SSL structure associated with the
*                               connection.
*       socketd                 The socket over which this data will
*                               be transmitted.
*       *buf_ptr                The first buffer in the chain.
*       *tx_len                 On input, the total number of bytes in
*                               the chain.  On return, the total number
*                               of bytes sent.
*
*   OUTPUTS
*
*       NU_SUCCESS              All bytes were successfully transmitted.
*       NU_WOULD_BLOCK          Not all data could be transmitted due to
*                               a need to block for buffers, window size,
*                               congestion, etc.
*       NU_NOT_CONNECTED        The other side has closed the connection.
*
*************************************************************************/
#if (WSOX_INCLUDE_CYASSL == NU_TRUE)
STATIC STATUS WSOX_SSL_ZC_Send(VOID *ssl_ptr, INT socketd, NET_BUFFER *buf_ptr,
                               UINT16 *tx_len)
{
    CHAR        *data_ptr;
    NET_BUFFER  *seg_ptr = buf_ptr;
    STATUS      status;
    UINT16      segment_len;
    INT32       bytes_sent = 0, bytes_to_send = *tx_len;

    /* Send each segment. */
    while (seg_ptr)
    {
        /* Determine how many bytes this segment could hold. */
        segment_len = NU_ZC_SEGMENT_BYTES_LEFT(buf_ptr, seg_ptr, socketd);

        if (segment_len > 0)
        {
            if (segment_len > bytes_to_send)
            {
                segment_len = bytes_to_send;
            }

            /* Get a pointer to the data buffer. */
            data_ptr = NU_ZC_SEGMENT_DATA(seg_ptr);

            bytes_sent = SSL_write((SSL*)ssl_ptr, data_ptr, segment_len);

            if (bytes_sent > 0)
            {
                bytes_to_send -= bytes_sent;
            }

            else
            {
                break;
            }

            /* Get a pointer to the next segment */
            seg_ptr = NU_ZC_SEGMENT_NEXT(seg_ptr);
        }

        else
        {
            bytes_sent = NU_NOT_CONNECTED;
            break;
        }
    }

    /* Return the number of bytes sent. */
    *tx_len = (*tx_len - bytes_to_send);

    /* If all data was sent, return success. */
    if (bytes_to_send == 0)
    {
        status = NU_SUCCESS;
        NU_ZC_Deallocate_Buffer(buf_ptr);
    }

    /* Otherwise, return an error. */
    else
    {
        if (CyaSSL_get_error(ssl_ptr, 0) == SSL_ERROR_WANT_WRITE)
        {
            status = NU_WOULD_BLOCK;
        }

        else
        {
            status = NU_NOT_CONNECTED;
        }
    }

    return (status);

} /* WSOX_SSL_ZC_Send */
#endif

/************************************************************************
*
*   FUNCTION
*
*       WSOX_Send
*
*   DESCRIPTION
*
*       Transmits a buffer of data over the connection.
*
*   INPUTS
*
*       socketd                 The socket over which this data will
*                               be transmitted.
*       *buf_ptr                The data to transmit.
*       len                     The number of bytes in the buffer.
*       *ssl_ptr                The SSL structure associated with the
*                               connection.
*
*   OUTPUTS
*
*       The number of bytes transmitted, or an operating-system specific
*       error.
*
*************************************************************************/
INT32 WSOX_Send(INT socketd, CHAR *buf_ptr, INT32 len, VOID *ssl_ptr)
{
    INT32   bytes_sent;

#if (WSOX_INCLUDE_CYASSL == NU_TRUE)
    if (ssl_ptr)
    {
        bytes_sent = SSL_write((SSL*)ssl_ptr, buf_ptr, len);
    }

    else
#endif
    {
        /* Send the message. */
        bytes_sent = NU_Send(socketd, buf_ptr, len, 0);
    }

    return (bytes_sent);

} /* WSOX_Send */

/************************************************************************
*
*   FUNCTION
*
*       WSOX_Recv
*
*   DESCRIPTION
*
*       Receives a buffer of data over the connection.
*
*   INPUTS
*
*       socketd                 The socket over which this data will
*                               be received.
*       *buffer                 The buffer into which the data will be
*                               received.
*       len                     The length of the buffer.
*       *ssl_ptr                The SSL structure associated with the
*                               connection.
*
*   OUTPUTS
*
*       The number of bytes received, or an operating-system specific
*       error.
*
*************************************************************************/
INT32 WSOX_Recv(INT socketd, CHAR *buffer, INT32 len, VOID *ssl_ptr)
{
    INT32   bytes_rx;

#if (WSOX_INCLUDE_CYASSL == NU_TRUE)
    if (ssl_ptr)
    {
        bytes_rx = CyaSSL_read((SSL*)ssl_ptr, buffer, len);
    }
    else
#endif
    {
        bytes_rx = NU_Recv(socketd, buffer, len, 0);
    }

    return (bytes_rx);

} /* WSOX_Recv */

/************************************************************************
*
*   FUNCTION
*
*       WSOX_Fill_Buffer_Chain
*
*   DESCRIPTION
*
*       Fills the buffer chain with the indicated bytes of encoded data.
*
*   INPUTS
*
*       *buf_ptr                The first buffer in the chain.
*       socketd                 The socket over which this data will
*                               be transmitted.
*       *data_ptr               A pointer to the buffer into which to
*                               begin adding encoded data.
*       first_seg_len           The length of the first segment in the
*                               chain.
*       data_len                The number of bytes to add to the
*                               chain.
*       *buffer                 The data to encode and add to the buffer
*                               chain.
*       *mask                   The mask to use to encode the data.
*       encode_count            The value to use for indexing into mask
*                               when encoding the data.
*
*   OUTPUTS
*
*       NU_SUCCESS              The buffer chain was successfully filled.
*       WSOX_CNXN_ERROR         The TCP connection has been lost.
*
*************************************************************************/
STATIC STATUS WSOX_Fill_Buffer_Chain(NET_BUFFER *buf_ptr, INT socketd,
                                     CHAR *data_ptr, UINT16 first_seg_len,
                                     UINT16 data_len, CHAR *buffer, CHAR *mask,
                                     UINT64 encode_count)
{
    NET_BUFFER  *seg_ptr = buf_ptr;
    STATUS      status = NU_SUCCESS;
    UINT16      segment_len = first_seg_len;
    UINT16      bytes_to_copy = data_len;

    /* Fill the buffer chain with the data, one segment at a time. */
    while (bytes_to_copy)
    {
        /* If the segment can hold more data than there is left to
         * copy, decrement the segment length.
         */
        if (segment_len > bytes_to_copy)
        {
            segment_len = bytes_to_copy;
        }

        /* If the data must be masked. */
        if (mask)
        {
            WSOX_Mask_Data(data_ptr, buffer, mask, segment_len,
                           encode_count + (data_len - bytes_to_copy));
        }

        /* Otherwise, copy the data directly into the segment. */
        else
        {
            memcpy(data_ptr, buffer, segment_len);
        }

        /* Increment the buffer pointer. */
        buffer += segment_len;

        /* Decrement the number of bytes left to copy. */
        bytes_to_copy -= segment_len;

        /* If there is data left to be added to the packet. */
        if (bytes_to_copy)
        {
            /* Get a pointer to the next segment */
            seg_ptr = NU_ZC_SEGMENT_NEXT(seg_ptr);

            if (seg_ptr)
            {
                /* Get the next segment in the buffer chain */
                data_ptr = NU_ZC_SEGMENT_DATA(seg_ptr);

                /* Determine how many bytes will fit in this segment. */
                segment_len = NU_ZC_SEGMENT_BYTES_LEFT(buf_ptr, seg_ptr, socketd);

                /* The connection could have been closed while trying to fill
                 * this buffer.
                 */
                if (segment_len == 0)
                {
                    status = WSOX_CNXN_ERROR;
                    break;
                }
            }

            /* The connection could have been closed while trying to fill
             * this buffer.
             */
            else
            {
                status = WSOX_CNXN_ERROR;
                break;
            }
        }
    }

    return (status);

} /* WSOX_Fill_Buffer_Chain */

/************************************************************************
*
*   FUNCTION
*
*       WSOX_Check_For_Data
*
*   DESCRIPTION
*
*       This function checks for data on a socket.
*
*   INPUTS
*
*       socketd                 The socket to check for data.
*       *ssl_ptr                The SSL structure if this is a secure
*                               connection.
*
*   OUTPUTS
*
*       The number of bytes ready to receive on the socket.
*
*************************************************************************/
UINT32 WSOX_Check_For_Data(INT socketd, VOID *ssl_ptr)
{
    UINT32              socket_count;
    SCK_IOCTL_OPTION    opt;

    opt.s_optval = (UINT8*)&socketd;

    /* Determine the number of bytes on the socket. */
    if (NU_Ioctl_FIONREAD(&opt) == NU_SUCCESS)
    {
        socket_count = opt.s_ret.sck_bytes_pending;
    }

    else
    {
        socket_count = 0;
    }

#if (WSOX_INCLUDE_CYASSL == NU_TRUE)
    if ( (socket_count == 0) && (ssl_ptr) )
    {
        socket_count = CyaSSL_pending(ssl_ptr);
    }
#endif

    return (socket_count);

} /* WSOX_Check_For_Data */
