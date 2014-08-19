/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2012
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
*     http_lite_client.c
*
*   COMPONENT
*
*     Nucleus HTTP Lite Client
*
*   DESCRIPTION
*
*     This file holds the HTTP Lite Client routines.
*
*   FUNCTIONS
*
*     NU_HTTP_Lite_Client_Init
*     NU_HTTP_Lite_Create_Session_Handle
*     NU_HTTP_Lite_Delete_Session_Handle
*     HTTP_Client_Get_Session_Handle
*     HTTP_Lite_Parse_URL
*     HTTP_Lite_Client_Query
*     NU_HTTP_Lite_Client_Get
*     NU_HTTP_Lite_Client_Put
*     NU_HTTP_Lite_Client_Delete
*     NU_HTTP_Lite_Client_Post
*
*   DEPENDENCIES
*
*     nucleus.h
*     nu_networking.h
*     http_lite.h
*     reg_api.h
*     ctype.h
*     http_lite_int.h
*
************************************************************************/

#include "nucleus.h"
#include "networking/nu_networking.h"
#include "services/reg_api.h"
#include <ctype.h>
#include "os/networking/http/inc/http_lite_int.h"

INT32           HTTP_Cli_Id = 0;
HTTP_SH_S_LIST  HTTP_Cli_Hdl_List;
NU_SEMAPHORE    HTTP_Client_Resource;
UINT8           HTTP_Client_Init = NU_FALSE;

extern NU_MEMORY_POOL   *MEM_Cached;

STATIC NU_HTTP_SH_S     *HTTP_Client_Get_Session_Handle(INT32);

#if (HTTP_INCLUDE_CYASSL == NU_TRUE)
#define HTTP_LITE_CLI_SSL_STRUCT    handle->ssl
#else
#define HTTP_LITE_CLI_SSL_STRUCT    NU_NULL
#endif

/************************************************************************
*
*   FUNCTION
*
*     NU_HTTP_Lite_Client_Init
*
*   DESCRIPTION
*
*     Initialize the data structures used by the HTTP Client module.
*
*   INPUTS
*
*     None.
*
*   OUTPUTS
*
*     NU_SUCCESS or an operating-system specific error upon failure.
*
************************************************************************/
STATUS NU_HTTP_Lite_Client_Init(VOID)
{
    STATUS  status = NU_SUCCESS;

    /* Ensure the client has not already been initialized. */
    if (HTTP_Client_Init == NU_FALSE)
    {
        /* Create the system resource. */
        status = NU_Create_Semaphore(&HTTP_Client_Resource, "HTTPCLI",
                                     (UNSIGNED)1, NU_FIFO);

        if (status == NU_SUCCESS)
        {
            /* Obtain the semaphore. */
            status = NU_Obtain_Semaphore(&HTTP_Client_Resource, NU_NO_SUSPEND);

            if (status == NU_SUCCESS)
            {
                /* Indicate that the client has been initialized. */
                HTTP_Client_Init = NU_TRUE;

                /* Initialize the ID counter. */
                HTTP_Cli_Id = 0;

                /* Initialize the list that will hold the session handles. */
                HTTP_Cli_Hdl_List.flink = NU_NULL;
                HTTP_Cli_Hdl_List.blink = NU_NULL;

                /* Release the semaphore. */
                NU_Release_Semaphore(&HTTP_Client_Resource);
            }
        }
    }

    return (status);

} /* NU_HTTP_Lite_Client_Init */

/************************************************************************
*
*   FUNCTION
*
*     NU_HTTP_Lite_Create_Session_Handle
*
*   DESCRIPTION
*
*     This function allocates and initializes a session handle
*     structure.
*
*   INPUTS
*
*     None.
*
*   OUTPUTS
*
*     Session handle ID or an operating-system specific error code
*     if an error occurred.
*
************************************************************************/
INT32 NU_HTTP_Lite_Create_Session_Handle(VOID)
{
    STATUS          status;
    NU_HTTP_SH_S    *session_handle, *temp_ptr;
    INT32           i = 0;

    /* Allocate memory for the session handle. */
    status = NU_Allocate_Memory(MEM_Cached, (VOID**)&session_handle,
                                sizeof(NU_HTTP_SH_S), NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Clear out the new structure */
        memset(session_handle, 0, sizeof(NU_HTTP_SH_S));

        /* Initialize socket descriptor to invalid value */
        session_handle->socketd = -1;

        /* Obtain the system semaphore so we can access the global list
         * of handles.
         */
        status = NU_Obtain_Semaphore(&HTTP_Client_Resource, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Assign a session ID that has not been used yet.  Bound
             * this loop by the maximum number of session handles that
             * could possibly exist at one time, however unlikely.
             */
            for (i = 0; i <= 2147483647; i ++)
            {
                /* Set the ID of the session handle.  This is how the user
                 * will identify this session handle.
                 */
                session_handle->id = HTTP_Cli_Id++;

                /* If the ID has wrapped, re-initialize to zero. */
                if (HTTP_Cli_Id < 0)
                    HTTP_Cli_Id = 0;

                /* Find a handle with this ID. */
                temp_ptr = HTTP_Client_Get_Session_Handle(session_handle->id);

                if (!temp_ptr)
                    break;
            }

            /* Ensure a handle index was available. */
            if (!temp_ptr)
            {
                /* Create the synchronization semaphore for this handle. */
                status = NU_Create_Semaphore(&(session_handle->lock), "HTTP_CLI",
                                         (UNSIGNED)1, NU_FIFO);
                if (status == NU_SUCCESS)
                {
                    /* Add this session handle structure to the internal list
                     * that is maintained by the client module.
                     */
                    DLL_Enqueue(&HTTP_Cli_Hdl_List, session_handle);

                    /* Set the return value. */
                    status = session_handle->id;
                }
            }

            else
            {
                status = NU_NO_MEMORY;
            }

            /* Release the system semaphore. */
            NU_Release_Semaphore(&HTTP_Client_Resource);
        }

        /* If an error is being returned, deallocate the memory
         * for the session handle.
         */
        if (status < 0)
        {
            /* Deallocate structure memory */
            NU_Deallocate_Memory((VOID*)session_handle);
        }
    }

    return (status);

} /* NU_HTTP_Lite_Create_Session_Handle */

/************************************************************************
*
*   FUNCTION
*
*     NU_HTTP_Lite_Delete_Session_Handle
*
*   DESCRIPTION
*
*     This function deallocates a session handle structure.
*
*   INPUTS
*
*     session_id                The ID associated with the internal
*                               handle structure.
*
*   OUTPUTS
*
*     NU_SUCCESS or an operating-system specific error if unable
*     to delete the session handle.
*
************************************************************************/
STATUS NU_HTTP_Lite_Delete_Session_Handle(INT32 session_id)
{
    NU_HTTP_SH_S    *session_handle;
    STATUS          status;

    /* Obtain the system semaphore so we can access the global handle
     * list safely.
     */
    status = NU_Obtain_Semaphore(&HTTP_Client_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Get a pointer to the session handle structure. */
        session_handle = HTTP_Client_Get_Session_Handle(session_id);

        /* Return NU_SUCCESS even if the session handle was not found. */
        if (session_handle)
        {
            /* Obtain the session handle semaphore. */
            status = NU_Obtain_Semaphore(&session_handle->lock, NU_SUSPEND);

            if (status == NU_SUCCESS)
            {
                /* Remove this session handle from the list. */
                DLL_Remove(&HTTP_Cli_Hdl_List, session_handle);

                /* Release the session handle semaphore. */
                NU_Release_Semaphore(&session_handle->lock);

                /* Delete the semaphore. */
                NU_Delete_Semaphore(&session_handle->lock);

                /* Deallocate structure memory */
                NU_Deallocate_Memory((VOID*)session_handle);
            }
        }

        /* Release the system semaphore. */
        NU_Release_Semaphore(&HTTP_Client_Resource);
    }

    return (status);

} /* NU_HTTP_Lite_Delete_Session_Handle */

/************************************************************************
*
*   FUNCTION
*
*     HTTP_Client_Get_Session_Handle
*
*   DESCRIPTION
*
*     Returns the session handle associated with the given session ID.
*
*   INPUTS
*
*     session_id                The session ID of the target handle.
*
*   OUTPUTS
*
*     A pointer to the session handle or NU_NULL if no matching handle
*     exists.
*
************************************************************************/
STATIC NU_HTTP_SH_S *HTTP_Client_Get_Session_Handle(INT32 session_id)
{
    NU_HTTP_SH_S    *handle_ptr;

    /* Look for the session handle by the given index. */
    for (handle_ptr = HTTP_Cli_Hdl_List.flink;
         handle_ptr;
         handle_ptr = handle_ptr->flink)
    {
        if (handle_ptr->id == session_id)
            break;
    }

    return (handle_ptr);

} /* HTTP_Client_Get_Session_Handle */

/************************************************************************
*
*   FUNCTION
*
*     HTTP_Lite_Parse_URL
*
*   DESCRIPTION
*
*     Scan a uri for its constituents
*
*   INPUTS
*
*     name              points to a document name which may be
*                        incomplete.
*     parts             structure pointer filled in with URL parts
*                        from name. Any fields that are non-null point
*                        to zero terminated strings.
*
*   OUTPUTS
*
*     None
*
************************************************************************/
STATIC VOID HTTP_Lite_Parse_URL (CHAR *name, HTURI *parts)
{
    CHAR *p;
    CHAR *after_access = name;
    memset(parts, '\0', sizeof(HTURI));

    if ((p = strchr(name, ' ')) != NULL)
    {
        *p++ = '\0';
    }

    for(p=name; *p; p++)
    {

        /*
         * Look for any whitespace. This is very bad for pipelining as it
         * makes the request invalid
         */
        if (isspace((int) *p))
        {
            char *orig=p, *dest=p+1;
            while (orig != '\0')
            {
                *orig++ = *dest++;
            }
            p = p-1;
        }

        if (*p=='/' || *p=='#' || *p=='?')
        {
            break;
        }

        if (*p==':')
        {
            *p = 0;

            /* Scheme has been specified */
            parts->access = after_access;

            after_access = p + 1;

            if (0==NU_STRICMP("URL", parts->access))
            {
                /* Ignore IETF's URL: pre-prefix */
                parts->access = NULL;
            }
            else
            {
                break;
            }
        }
    }

    p = after_access;

    if (*p=='/')
    {
        if (p[1]=='/')
        {
            /* host has been specified */
            parts->host = p+2;

            /* Terminate access */
            *p=0;

            /* look for end of host name if any */
            p=strchr(parts->host,'/');

            if(p)
            {
                /* Terminate host */
                *p=0;

                /* Root has been found */
                parts->absolute = p+1;
            }
        }
        else
        {
            /* Root found but no host */
            parts->absolute = p+1;
        }
    }
}


/************************************************************************
*
*   FUNCTION
*
*     HTTP_Lite_Client_Query
*
*   DESCRIPTION
*
*     Send a command and additional headers to the http server.
*
*   INPUTS
*     command               Command to send
*     url                   URL / filename queried
*     additional_header     Additional header
*     mode                  Type of query
*     data                  Data to send after header. If NULL, no
*                            data is sent.
*     length                Size of data
*     psd                   Pointer to variable where socket
*                            descriptor value is set.
*     *handle               Pointer to the session handle.
*
*   OUTPUTS
*
*     status                server response status
*
************************************************************************/
STATIC INT HTTP_Lite_Client_Query(CHAR *command, CHAR *url,
                                  CHAR *additional_header,
                                  HTTP_QUERYMODE mode, CHAR *data,
                                  INT length, INT *psd, NU_HTTP_SH_S *handle)
{
    STATUS      status;
    HTURI       uri;
    CHAR        test_ip[MAX_ADDRESS_SIZE] = {0};
    INT         s, newsock, url_size;
#if (HTTP_INCLUDE_CYASSL == NU_TRUE)
    INT         type;
#endif
    NU_HOSTENT  *hentry = NU_NULL;
    struct addr_struct server;
    CHAR        *loc_url = NU_NULL;
    CHAR        rmtsvr_name[] = "rmtsvr";

    /* Initialize the file pointer */
    if (psd)
    {
        *psd = -1;
    }

    /* Allocate string for copying URL - include an extra byte for the
     * trailing null terminator.
     */
    url_size = strlen(url);
    status = NU_Allocate_Memory(MEM_Cached, (VOID**)&loc_url,
                                url_size + 1, NU_NO_SUSPEND);
    if (status == NU_SUCCESS)
    {
        /* Parse the host name from the URL */
        strncpy(loc_url, url, url_size);
        loc_url[url_size] = '\0'; /* url_size is ok because url_size + 1 was allocated */
        HTTP_Lite_Parse_URL(loc_url, &uri);
    }

    /* Ensure all parts of the URI are present. */
    if ((status == NU_SUCCESS) && (uri.host) && (uri.access) && (uri.absolute) )
    {
        /* Initialize the port and name of the server structure. */
#if (HTTP_INCLUDE_CYASSL == NU_TRUE)
        if (0 == strncmp(uri.access, HTTP_SECURE, strlen(HTTP_SECURE)))
        {
            /* If this is a secure session */
            server.port = HTTP_SSL_PORT;
        }
        else
#endif
        {
            server.port = HTTP_SVR_PORT;
        }
        server.name = rmtsvr_name;

#if (INCLUDE_IPV6 == NU_TRUE)
        /* Search for a ':' to determine if the address is IPv4 or IPv6. */
        if (strchr(uri.host, ':') != NU_NULL)
        {
            server.family = NU_FAMILY_IP6;

            /* Convert the string to an array. */
            status = NU_Inet_PTON(NU_FAMILY_IP6, uri.host, test_ip);
        }

        else
#if (INCLUDE_IPV4 == NU_FALSE)
        {
            /* An IPv6 address was not passed into the routine. */
            status = -1;
        }
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
        {
            server.family = NU_FAMILY_IP;

            /* Attempt to convert the string to an array.  If this is not an IPv4
             * address, the routine will return an error and we will assume the
             * user passed in a host name.
             */
            status = NU_Inet_PTON(NU_FAMILY_IP, uri.host, test_ip);
        }
#endif

        /* If the URI contains an IP address, copy it into the server structure. */
        if (status == NU_SUCCESS)
        {
            memcpy(&server.id.is_ip_addrs, test_ip, MAX_ADDRESS_SIZE);
        }

        /* If the application did not pass in an IP address, resolve the host
         * name into a valid IP address.
         */
        else
        {
            /* If IPv6 is enabled, default to IPv6.  If the host does not have
             * an IPv6 address, an IPv4-mapped IPv6 address will be returned that
             * can be used as an IPv6 address.
             */
#if (INCLUDE_IPV6 == NU_TRUE)
            server.family = NU_FAMILY_IP6;
#else
            server.family = NU_FAMILY_IP;
#endif

            /* Try getting host info by name */
            hentry = NU_Get_IP_Node_By_Name(uri.host, server.family, DNS_V4MAPPED, &status);

            if (hentry)
            {
                /* Copy the hentry data into the server structure */
                memcpy(&server.id.is_ip_addrs, *hentry->h_addr_list, hentry->h_length);
                server.family = hentry->h_addrtype;

                /* Free the memory associated with the host entry returned */
                NU_Free_Host_Entry(hentry);
            }

            /* If the host name could not be resolved, return an error. */
            else
            {
                status = HTTP_NO_IP_NODE;
            }
        }

        /* Initialize the header pointer to NULL. */
        handle->header = NU_NULL;

        if (status == NU_SUCCESS)
        {
            /* Allocate memory for the header. */
            status = NU_Allocate_Memory(MEM_Cached, (VOID **)&handle->header,
                                        HTTP_CLI_MAXBUF, (UNSIGNED)NU_SUSPEND);
        }

        if (status == NU_SUCCESS)
        {
            /* create socket */
            if ((s = NU_Socket(server.family, NU_TYPE_STREAM, 0)) < 0)
            {
                status = s;
            }
        }

        if (status == NU_SUCCESS)
        {
            /* connect to server */
            newsock = NU_Connect(s, &server, sizeof(server));

#if (HTTP_INCLUDE_CYASSL == NU_TRUE)
            /* If this is a secure session, then enable CyaSSL. */
            if ((newsock == s) && (server.port == HTTP_SSL_PORT))
            {
                /* Create a context structure for CyaSSL. */
#if defined(CYASSL_DTLS)
                handle->ctx = CyaSSL_CTX_new(CyaDTLSv1_client_method());
#elif  !defined(NO_TLS)
                handle->ctx = CyaSSL_CTX_new(CyaSSLv23_client_method());
#else
                handle->ctx = CyaSSL_CTX_new(CyaSSLv3_client_method());
#endif
                if (handle->ctx != NU_NULL)
                {
#if (CFG_NU_OS_NET_SSL_LITE_NO_FILESYSTEM == NU_FALSE)
                    /* If the CA is pointing to a file. */
                    if (handle->ssl_struct->ssl_flags & NU_HTTP_CA_FILE)
                    {
                        /* Load the CA certificates. */
#if (CFG_NU_OS_NET_SSL_LITE_CYASSL_DER_LOAD == NU_TRUE)

                        /* If the CA file has a "DER" extension, set the type
                         * accordingly.
                         */
                        if ( (NU_STRICMP(&handle->ssl_struct->ssl_ca[strlen(handle->ssl_struct->ssl_ca) - 3], "der") == 0) ||
                             (NU_STRICMP(&handle->ssl_struct->ssl_ca[strlen(handle->ssl_struct->ssl_ca) - 3], "cer") == 0) )
                        {
                            type = SSL_FILETYPE_ASN1;
                        }
                        else
                        {
                            type = SSL_FILETYPE_PEM;
                        }

                        status = CyaSSL_CTX_der_load_verify_locations(handle->ctx, handle->ssl_struct->ssl_ca,
                                                                      type);
#else
                        status = CyaSSL_CTX_load_verify_locations(handle->ctx, handle->ssl_struct->ssl_ca, 0);
#endif
                    }

                    else
#endif
                    if (handle->ssl_struct->ssl_ca_size > 0)
                    {
                        /* Determine whether this is a PEM or DER CA. */
                        if (handle->ssl_struct->ssl_flags & NU_HTTP_CA_PEM_PTR)
                        {
                            type = SSL_FILETYPE_PEM;
                        }

                        else if (handle->ssl_struct->ssl_flags & NU_HTTP_CA_DER_PTR)
                        {
                            type = SSL_FILETYPE_ASN1;
                        }

                        else
                        {
                            status = NU_INVALID_PARM;
                        }

                        if (status != NU_INVALID_PARM)
                        {
                            /* Load CA */
                            status = CyaSSL_CTX_load_verify_buffer(handle->ctx,
                                                                   (const unsigned char *)handle->ssl_struct->ssl_ca,
                                                                   handle->ssl_struct->ssl_ca_size, type);
                        }
                    }

                    /* If the client passed in a certificate to be used with the server. */
                    if ( (status == SSL_SUCCESS) && (handle->ssl_struct->ssl_cert) )
                    {
#if (CFG_NU_OS_NET_SSL_LITE_NO_FILESYSTEM == NU_FALSE)
                        /* If the certificate is in a file, load it from the file. */
                        if (handle->ssl_struct->ssl_flags & NU_HTTP_CERT_FILE)
                        {
                            /* If the certificate file has a binary extension, set the type
                             * accordingly.
                             */
                            if ( (NU_STRICMP(&handle->ssl_struct->ssl_cert
                                             [strlen(handle->ssl_struct->ssl_cert) - 3], "der") == 0) ||
                                 (NU_STRICMP(&handle->ssl_struct->ssl_cert
                                             [strlen(handle->ssl_struct->ssl_cert) - 3], "cer") == 0) )
                            {
                                type = SSL_FILETYPE_ASN1;
                            }
                            else
                            {
                                type = SSL_FILETYPE_PEM;
                            }

                            /* Load the certificate. */
                            status = CyaSSL_CTX_use_certificate_file(handle->ctx, handle->ssl_struct->ssl_cert,
                                                                     type);
                        }

                        else
#endif
                        if (handle->ssl_struct->ssl_cert_size > 0)
                        {
                            /* Determine whether this is a PEM or DER certificate. */
                            if (handle->ssl_struct->ssl_flags & NU_HTTP_CERT_PEM_PTR)
                            {
                                type = SSL_FILETYPE_PEM;
                            }

                            else if (handle->ssl_struct->ssl_flags & NU_HTTP_CERT_DER_PTR)
                            {
                                type = SSL_FILETYPE_ASN1;
                            }

                            else
                            {
                                status = NU_INVALID_PARM;
                            }

                            if (status != NU_INVALID_PARM)
                            {
                                /* Load the certificate that will be sent to the server
                                 * for verification.
                                 */
                                status = CyaSSL_CTX_use_certificate_buffer(handle->ctx,
                                                                           (const unsigned char *)handle->ssl_struct->ssl_cert,
                                                                           handle->ssl_struct->ssl_cert_size, type);
                            }
                        }

                        else
                        {
                            status = NU_INVALID_PARM;
                        }
                    }

                    /* If the client passed in a key to be used with the server. */
                    if ( (status == SSL_SUCCESS) && (handle->ssl_struct->ssl_key) )
                    {
#if (CFG_NU_OS_NET_SSL_LITE_NO_FILESYSTEM == NU_FALSE)
                        /* If the key is in a file, load it from the file. */
                        if (handle->ssl_struct->ssl_flags & NU_HTTP_KEY_FILE)
                        {
                            /* If the key file has a binary extension, set the type accordingly. */
                            if ( (NU_STRICMP(&handle->ssl_struct->ssl_key
                                             [strlen(handle->ssl_struct->ssl_key) - 3], "der") == 0) ||
                                 (NU_STRICMP(&handle->ssl_struct->ssl_key
                                             [strlen(handle->ssl_struct->ssl_key) - 3], "cer") == 0) )
                            {
                                type = SSL_FILETYPE_ASN1;
                            }
                            else
                            {
                                type = SSL_FILETYPE_PEM;
                            }

                            /* Load the key. */
                            status = CyaSSL_CTX_use_PrivateKey_file(handle->ctx, handle->ssl_struct->ssl_key,
                                                                    type);
                        }

                        else
#endif
                        if (handle->ssl_struct->ssl_key_size > 0)
                        {
                            /* Determine whether this is a PEM or DER key. */
                            if (handle->ssl_struct->ssl_flags & NU_HTTP_KEY_PEM_PTR)
                            {
                                type = SSL_FILETYPE_PEM;
                            }

                            else if (handle->ssl_struct->ssl_flags & NU_HTTP_KEY_DER_PTR)
                            {
                                type = SSL_FILETYPE_ASN1;
                            }

                            else
                            {
                                status = NU_INVALID_PARM;
                            }

                            if (status == SSL_SUCCESS)
                            {
                                /* Load the server key. */
                                status = CyaSSL_CTX_use_PrivateKey_buffer(handle->ctx,
                                                                          (const unsigned char *)handle->ssl_struct->ssl_key,
                                                                          handle->ssl_struct->ssl_key_size, type);
                            }
                        }

                        else
                        {
                            status = NU_INVALID_PARM;
                        }
                    }

#if CFG_NU_OS_NET_SSL_LITE_HAVE_OCSP
                    if(status == SSL_SUCCESS)
                    {
                        if(CyaSSL_CTX_OCSP_set_options(handle->ctx, CYASSL_OCSP_ENABLE) != 1)
                        {
                            status = HTTP_SSL_ERROR;
                        }
                    }
#if CFG_NU_OS_NET_SSL_LITE_OCSP_OVERRIDE
                    if (status == SSL_SUCCESS)
                    {
                        if(CyaSSL_CTX_OCSP_set_override_url(handle->ctx, CFG_NU_OS_NET_SSL_LITE_OCSP_OVERRIDE_URL) != 1)
                        {
                            status = HTTP_SSL_ERROR;
                        }
                    }
#endif
#endif

                    if (status == SSL_SUCCESS)
                    {
                        SSL_CTX_set_verify(handle->ctx, SSL_VERIFY_NONE, 0);

                        handle->ssl = CyaSSL_new(handle->ctx);
                        if (handle->ssl != NULL)
                        {
                            CyaSSL_set_fd(handle->ssl, s);

                            /* If domain checking has not been disabled. */
                            if (!(handle->ssl_struct->ssl_flags & NU_HTTP_NO_DOMAIN_CHECK))
                            {
                                status = CyaSSL_check_domain_name(handle->ssl, uri.host);
                            }

                            if (status == SSL_SUCCESS)
                            {
                                newsock = CyaSSL_connect(handle->ssl);
                                if (newsock == SSL_SUCCESS)
                                {
                                    newsock = s;
                                }
                            }
                        }

                        else
                        {
                            status = HTTP_SSL_ERROR;
                        }
                    }

                    if (status != SSL_SUCCESS)
                    {
                        newsock = status;
                    }
                }
                else
                {
                    newsock = HTTP_SSL_ERROR;
                }
            }
#endif
            if (newsock == s)
            {
                if (psd)
                {
                    *psd = s;
                }

                /* Create the header. */
                snprintf(handle->header, HTTP_CLI_MAXBUF,
                        "%s /%.256s HTTP/1.1\r\nUser-Agent: %s\r\n%s%s\r\n%s\r\n",
                        command,
                        uri.absolute,
                        HTTP_USER_AGENT,
                        "Host: ", uri.host,
                        additional_header);

                /* Send the header and the chunked data. */
                status = HTTP_Lite_Write_Buffer(s, handle->header, HTTP_CLI_MAXBUF, data,
                                                length, HTTP_LITE_CLI_SSL_STRUCT);

                if (status == NU_SUCCESS)
                {
                    /* read result & check */
                    status = HTTP_Lite_Read_Line(handle->header, s, HTTP_CLI_MAXBUF,
                                                 NU_TRUE, HTTP_LITE_CLI_SSL_STRUCT);
                    if (status <= 0)
                    {
                        status = HTTP_INVALID_HEADER_READ;
                    }
                    else if (sscanf(handle->header,
                                    "HTTP/1.%*d %03d", (int*)&status) != 1)
                    {
                        status = HTTP_INVALID_HEADER_PARSE;
                    }
                }
            }
            else
            {
                /* error connecting */
                status = newsock;
            }

            if ((mode != HTTP_KEEP_OPEN) || (status != HTTP_PROTO_OK))
            {
#if (HTTP_INCLUDE_CYASSL == NU_TRUE)
                if (handle->ssl)
                {
                    CyaSSL_free(handle->ssl);
                    CyaSSL_CTX_free(handle->ctx);
                    handle->ssl = NU_NULL;
                    handle->ctx = NU_NULL;
                }
#endif
                /* close socket */
                NU_Close_Socket(s);

                /* Deallocate the memory for the header. */
                NU_Deallocate_Memory(handle->header);

                handle->header = NU_NULL;
            }
        }
    }

    else
    {
        status = HTTP_INVALID_URI;
    }

    /* Deallocate memory for the URL parsing */
    NU_Deallocate_Memory(loc_url);

    return status;
}


/************************************************************************
*
*    FUNCTION
*
*     NU_HTTP_Lite_Client_Get
*
*   DESCRIPTION
*
*     Retrieves information identified in the Request-URI.
*     Returns the code from the server in the http_session field.
*
*
*   INPUTS
*
*     session_id            The ID associated with an internally
*                            managed session ID structure.
*     *cli_struct           Data structure that holds information
*                           regarding the GET operation:
*                               uri         Name of the resource to read
*                               pdata       Address of a pointer variable
*                                           which will be set to point
*                                           to allocated memory containing
*                                           read data.
*                               plength     Input:  Pointer to maximum size
*                                                   of the buffer that can
*                                                   be allocated and returned.
*                                           Output: Address of integer variable
*                                                   which will be set to length
*                                                   of the read data.
*                               ptype       Allocated buffer where the read
*                                           data type is returned. If NULL, the
*                                           type is not returned.
*                               type_size   The size of the ptype buffer.
*     http_status           The status code returned by the server.
*
*   OUTPUTS
*
*     NU_SUCCESS                The operation was successfully completed.
*     HTTP_INVALID_PARAMETER    One of the input parameters is invalid.
*     HTTP_INVALID_URI          The URI is not formed properly.
*     HTTP_NO_IP_NODE           The host name is invalid.
*     HTTP_SSL_ERROR            CyaSSL could not be initialized.
*     HTTP_ERROR_DATA_WRITE     Data could not be sent to the server.
*     HTTP_INVALID_HEADER_READ  No response from the server.
*     HTTP_INVALID_HEADER_PARSE The server is not using a compatible
*                               HTTP version.
*     HTTP_ERROR_UNREAD_DATA    Some data was received, but more data
*                               may be pending on the socket.
*
*     Otherwise, an operating-system specific error is returned,
*     indicating the nature of the failure; out of memory, cannot
*     connect to server, etc.
*
************************************************************************/
STATUS NU_HTTP_Lite_Client_Get(INT32 session_id, NU_HTTP_CLIENT *cli_struct,
                               STATUS *http_status)
{
    STATUS          status;
    CHAR            *pc;
    INT             sd;
    UINT32          n;
    UINT32          length = 0;
    UINT32          total_len = 0;
    CHAR            token[24] = "content-type: %";
    CHAR            ts[8];
    NU_HTTP_SH_S    *handle;
    UINT8           chunked = NU_FALSE;
    BOOLEAN         http_hdr_close = NU_FALSE;
    UINT32          max_recv_len;
    UINT32          temp;

    /* Parameter checking */
    if ( (cli_struct) && (cli_struct->uri) && (http_status) &&
         ((cli_struct->ptype) || (cli_struct->type_size == 0)) )
    {
        /* Set up the token string for finding Content Type header */
        snprintf(ts, sizeof(ts), "%d", cli_struct->type_size);
        strcat(token, ts);
        strcat(token, "s\0");

        cli_struct->pdata = NU_NULL;

        if (cli_struct->plength == 0)
        {
            max_recv_len = HTTP_CLI_MAXBUF;
        }
        else
        {
            max_recv_len = cli_struct->plength;
        }

        if (cli_struct->ptype)
        {
            cli_struct->ptype[0] = '\0';
        }

        /* Get the system semaphore so we can safely access the internal
         * list of session handles.
         */
        status = NU_Obtain_Semaphore(&HTTP_Client_Resource, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Get a pointer to the session handle. */
            handle = HTTP_Client_Get_Session_Handle(session_id);

            if (handle)
            {
                /* Lock the session structure */
                status = NU_Obtain_Semaphore(&handle->lock, NU_SUSPEND);

                if (status == NU_SUCCESS)
                {
                    /* Release the system semaphore since the session semaphore
                     * is now locked.
                     */
                    NU_Release_Semaphore(&HTTP_Client_Resource);

#if (HTTP_INCLUDE_CYASSL == NU_TRUE)
                    handle->ssl_struct = &cli_struct->ssl_struct;
#endif

                    /* If the socket is not open, then issue a new request */
                    if (handle->socketd < 0)
                    {

                        /* Send the request to the server.  Note that the caller is
                         * responsible for closing the socket that will be opened by
                         * this routine and deallocating the memory for the HTTP header
                         * since the caller is specifying HTTP_KEEP_OPEN.
                         */
                        *http_status = HTTP_Lite_Client_Query("GET", cli_struct->uri, "",
                                                              HTTP_KEEP_OPEN, NULL, 0, &sd,
                                                              handle);

#if (HTTP_INCLUDE_CYASSL == NU_TRUE)
                        handle->ssl_struct = NU_NULL;
#endif

                        if (*http_status == HTTP_PROTO_OK)
                        {
                            /* Parse through the headers returned by the server */
                            while (1)
                            {
                                n = HTTP_Lite_Read_Line(handle->header, sd, HTTP_CLI_MAXBUF,
                                                        NU_TRUE, HTTP_LITE_CLI_SSL_STRUCT);

                                if (n <= 0)
                                {
                                    status = HTTP_INVALID_HEADER_READ;
                                    break;
                                }

                                /* empty line ? (=> end of header) */
                                if ((*handle->header) == '\0')
                                {
                                    break;
                                }

                                /* try to parse some keywords : */
                                /* convert to lower case until ':' is found or end of string */
                                for (pc = handle->header; ((*pc != ':') && *pc); pc++)
                                {
                                    *pc = tolower((UINT8)*pc);
                                }

                                sscanf(handle->header, "content-length: %lu", &length);

                                /* Check if the data is chunked. */
                                if (HTTP_Lite_Find_Token(HTTP_CHUNKED, handle->header,
                                                         &handle->header[n], HTTP_TOKEN_CASE_INSENS))
                                {
                                    handle->flag |= HTTP_SH_FLAG_CHUNKED;
                                    chunked = NU_TRUE;
                                }

                                /* Check for a "Connection: " general-header field. */
                                if(HTTP_Lite_Find_Token(HTTP_CONNECTION,
                                                        handle->header,
                                                        &handle->header[n],
                                                        HTTP_TOKEN_CASE_INSENS))
                                {
                                    /* Move past the "Connection: " */
                                    handle->header += (sizeof(HTTP_CONNECTION) - 1);

                                    /* Check for "close" within the bounds of the Connection: header. */
                                    if (HTTP_Lite_Find_Token("close", handle->header,
                                                             &handle->header[n],
                                                             HTTP_TOKEN_CASE_INSENS))
                                    {
                                        /* Close the connection after completing the
                                         * respective method.
                                         */
                                        http_hdr_close = NU_TRUE;
                                        handle->flag |= HTTP_SH_FLAG_CLOSE;
                                    }
                                }

                                if (cli_struct->ptype)
                                {
                                    sscanf(handle->header, token, cli_struct->ptype);
                                }
                            }

                            if (status == NU_SUCCESS)
                            {
                                do
                                {
                                    /* check for a "connection:close" command */
                                    if ((!length && !chunked) && http_hdr_close)
                                    {
                                        if (max_recv_len > total_len)
                                        {
                                            /* Set to length to remaining buffer size. */
                                            length = max_recv_len - total_len;
                                        }
                                        else
                                        {
                                            /* Return a notification that there is more data available */
                                            status = HTTP_ERROR_UNREAD_DATA;
                                            break;
                                        }
                                    }

                                    /* If a content-length or chunked header is present. */
                                    if ( (length) || (chunked) )
                                    {
                                        /* Check for max length */
                                        if (length > max_recv_len)
                                        {
                                            handle->num_remaining = length - max_recv_len;
                                            length = max_recv_len;
                                        }

                                        /* Allocate a new buffer for the data and read data
                                         * into it.
                                         */
                                        temp = max_recv_len;
                                        n = HTTP_Lite_Read_Buffer(handle->header, HTTP_CLI_MAXBUF, sd,
                                                                  &cli_struct->pdata, length,
                                                                  HTTP_LITE_CLI_SSL_STRUCT,
                                                                  total_len, &temp, handle);

                                        /* If a length was specified, but that number of bytes
                                         * could not be received, or no data was received,
                                         * return an error.
                                         */
                                        if ( (n == 0) || ((!http_hdr_close) && (length) && (n != length)) )
                                        {
                                            status = HTTP_ERROR_DATA_READ;
                                        }

                                        /* Set total number of bytes received */
                                        total_len += n;

                                        if (http_hdr_close)
                                        {
                                            /* set length for next iteration */
                                            length = 0;
                                        }
                                        else
                                        {
                                            /* There was only one data segment */

                                            /* If chunked and there was more data to be read */
                                            if ((chunked) && (temp > 0))
                                            {
                                                handle->num_remaining = temp;
                                            }

                                            /* If there was more data to be read */
                                            if(handle->num_remaining > 0)
                                            {
                                                /* Store the socket in the client handle */
                                                handle->socketd = sd;

                                                /* Return a notification that there is more data available */
                                                status = HTTP_ERROR_UNREAD_DATA;
                                            }
                                            else
                                            {
                                                /* Clear the socket in the client handle */
                                                handle->socketd = -1;
                                            }
                                            break;
                                        }
                                    }
                                    else
                                    {
                                        status = HTTP_INVALID_LENGTH;
                                        break;
                                    }
                                } while(NU_TRUE == NU_Is_Connected(sd));

                                /* Return the length of the buffer. */
                                cli_struct->plength = total_len;
                            }
                        }

                        else if (*http_status >= 0)
                        {
                            status = HTTP_RESPONSE_ERROR;
                        }

                        else
                        {
                            /* http_session contains a negative error status */
                            status = *http_status;
                        }

#if (HTTP_INCLUDE_CYASSL == NU_TRUE)
                        if (handle->ssl)
                        {
                            CyaSSL_free(handle->ssl);
                            CyaSSL_CTX_free(handle->ctx);
                            handle->ssl = NU_NULL;
                            handle->ctx = NU_NULL;
                        }
#endif
                    }
                    else
                    {
                        sd = handle->socketd;

                        /* The handle has pending data from a previous call. */
                        length = handle->num_remaining;

                        /* Check for max length */
                        if (length > max_recv_len)
                        {
                            length = max_recv_len;
                            status = HTTP_ERROR_UNREAD_DATA;
                        }

                        /* Allocate a new buffer for the data and read data
                         * into it.
                         */
                        temp = max_recv_len;
                        n = HTTP_Lite_Read_Buffer(handle->header, HTTP_CLI_MAXBUF, handle->socketd,
                                                  &cli_struct->pdata, length, HTTP_LITE_CLI_SSL_STRUCT,
                                                  total_len, &temp, (VOID*)handle);

                        if (handle->num_remaining > 0)
                        {
                            /* Update num bytes remaining */
                            handle->num_remaining -= n;
                        }

                        total_len = n;

                        if ((handle->flag & HTTP_SH_FLAG_CHUNKED) != NU_TRUE)
                        {
                            /* If a length was specified, but that number of bytes
                             * could not be received, or no data was received,
                             * return an error.
                             */
                            if ((n == 0) || ((length) && (n != length)) )
                            {
                                status = HTTP_ERROR_DATA_READ;
                            }
                            else
                            {
                                /* Return the length of the buffer. */
                                cli_struct->plength = total_len;
                            }
                        }
                        else
                        {

                            if (handle->num_remaining == 0)
                            {
                                /* During a chunked transfer there are three possible outcomes: */

                                if (temp > 0)
                                {
                                    /* Buffer was full. Return and indicate more data remaining. */
                                    status = HTTP_ERROR_UNREAD_DATA;
                                    handle->num_remaining = temp;
                                }
                                else
                                {
                                    /* This chunk is complete. If it was the "last" chunk,
                                     * then no data was received and there is no remaining data */

                                    if (n == 0)
                                    {
                                        /* Chunk transmit was completed successfully. No more chunks. */
                                        status = NU_SUCCESS;
                                    }
                                    else
                                    {
                                        /* Update the total number of bytes received, removing
                                         * the trailing CRLF.
                                         */
                                        total_len -= strlen(HTTP_CRLF);

                                        /* The entire chunk was received. Need to check for another chunk. */
                                        status = HTTP_ERROR_UNREAD_DATA;
                                    }
                                }
                            }

                            /* Return the length of the buffer. */
                            cli_struct->plength = total_len;
                        }
                    }

                    if (status != HTTP_ERROR_UNREAD_DATA)
                    {
                        /* Close the socket */
                        NU_Close_Socket(sd);

                        /* Invalidate the socket descriptor field */
                        handle->socketd = -1;

                        /* Clear control fields */
                        handle->flag = 0;
                        handle->num_remaining = 0;

                        /* Deallocate the header memory. */
                        if (handle->header)
                        {
                            NU_Deallocate_Memory(handle->header);

                            handle->header = NU_NULL;
                        }
                    }

                    /* Release the session handle semaphore. */
                    NU_Release_Semaphore(&handle->lock);
                }

                /* Could not obtain semaphore for session handle. */
                else
                {
                    /* Release the system semaphore. */
                    NU_Release_Semaphore(&HTTP_Client_Resource);
                }
            }

            /* The session ID is not valid. */
            else
            {
                status = HTTP_INVALID_PARAMETER;

                /* Release the system semaphore. */
                NU_Release_Semaphore(&HTTP_Client_Resource);
            }
        }
    }
    else
    {
        status = HTTP_INVALID_PARAMETER;
    }

    return (status);
}


/************************************************************************
*
*    FUNCTION
*
*     NU_HTTP_Lite_Client_Put
*
*   DESCRIPTION
*
*     This function sends data to the http data server.
*     The data will be stored under the resource name filename.
*     Returns the code from the server in the http_session field.
*
*   INPUTS
*
*     session_id            The ID associated with an internally
*                            managed session ID structure.
*     *cli_struct           Data structure that holds information
*                           regarding the PUT operation:
*                               uri         Name of the resource to create.
*                               pdata       Pointer to the data to send.
*                               plength     Length of the data to send.
*                               ptype       Type of the data.  If NULL, the
*                                           default type is used.
*                               overwrite   Flag to request to overwrite the
*                                           resource if it already exists.
*     http_status           The status returned by the server.
*
*   OUTPUTS
*
*     NU_SUCCESS                The operation was successfully completed.
*     HTTP_INVALID_PARAMETER    One of the input parameters is invalid.
*     HTTP_INVALID_URI          The URI is not formed properly.
*     HTTP_NO_IP_NODE           The host name is invalid.
*     HTTP_SSL_ERROR            CyaSSL could not be initialized.
*     HTTP_ERROR_DATA_WRITE     Data could not be sent to the server.
*     HTTP_INVALID_HEADER_READ  No response from the server.
*     HTTP_INVALID_HEADER_PARSE The server is not using a compatible
*                               HTTP version.
*
*     Otherwise, an operating-system specific error is returned,
*     indicating the nature of the failure; out of memory, cannot
*     connect to server, etc.
*
************************************************************************/
STATUS NU_HTTP_Lite_Client_Put(INT32 session_id, NU_HTTP_CLIENT *cli_struct,
                               STATUS *http_status)
{
    CHAR ow_str[] = "Control: overwrite=1\r\n";
    CHAR additional_header[sizeof(HTTP_TRANSFER_ENCODING) + sizeof(HTTP_CHUNKED) + sizeof(HTTP_CRLF) +
                           sizeof(HTTP_CONTENT_TYPE) + 64 /* max content type */ + sizeof(HTTP_CRLF) +
                           sizeof(ow_str) + sizeof(HTTP_CRLF)];
    STATUS          status;
    NU_HTTP_SH_S    *session_handle;

    /* Parameter checking */
    if ( (cli_struct) && (cli_struct->uri) && (http_status) &&
         ((cli_struct->pdata) || (cli_struct->plength == 0)) )
    {
        /* Prepare the request header */
        if (cli_struct->ptype)
        {
            snprintf(additional_header, sizeof(additional_header),
                     "Content-type: %.64s\r\n%s Transfer-Encoding: chunked\r\n",
                     cli_struct->ptype, cli_struct->overwrite ? ow_str : "");
        }

        else
        {
            /* The Transfer-Encoding header should be last. */
            snprintf(additional_header, sizeof(additional_header), "Transfer-Encoding: chunked\r\n");
        }

        /* Get the system semaphore so we can safely access the list of
         * internal session handles.
         */
        status = NU_Obtain_Semaphore(&HTTP_Client_Resource, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Get a pointer to the session handle. */
            session_handle = HTTP_Client_Get_Session_Handle(session_id);

            if (session_handle)
            {
                /* Lock the session structure */
                status = NU_Obtain_Semaphore(&session_handle->lock, NU_SUSPEND);

                if (status == NU_SUCCESS)
                {
                    /* Release the system semaphore since the session handle is now
                     * locked.
                     */
                    NU_Release_Semaphore(&HTTP_Client_Resource);

#if (HTTP_INCLUDE_CYASSL == NU_TRUE)
                    session_handle->ssl_struct = &cli_struct->ssl_struct;
#endif

                    /* Send the request */
                    *http_status = HTTP_Lite_Client_Query("PUT", cli_struct->uri,
                                                          additional_header,
                                                          HTTP_CLOSE, cli_struct->pdata,
                                                          cli_struct->plength, NULL,
                                                          session_handle);

#if (HTTP_INCLUDE_CYASSL == NU_TRUE)
                    session_handle->ssl_struct = NU_NULL;
#endif

                    if ( (*http_status == HTTP_PROTO_OK) ||
                         (*http_status == HTTP_PROTO_CREATED) ||
                         (*http_status == HTTP_PROTO_NO_RESPONSE) )
                    {
                        status = NU_SUCCESS;
                    }

                    else if (*http_status >= 0)
                    {
                        status = HTTP_RESPONSE_ERROR;
                    }

                    else
                    {
                        /* http_session contains a negative error status */
                        status = *http_status;
                    }

                    /* Release the semaphore. */
                    NU_Release_Semaphore(&session_handle->lock);
                }

                else
                {
                    /* Release the system semaphore. */
                    NU_Release_Semaphore(&HTTP_Client_Resource);
                }
            }

            /* The session ID is not valid. */
            else
            {
                status = HTTP_INVALID_PARAMETER;

                /* Release the system semaphore. */
                NU_Release_Semaphore(&HTTP_Client_Resource);
            }
        }
    }

    else
    {
        status = HTTP_INVALID_PARAMETER;
    }

    return (status);
}


/************************************************************************
*
*    FUNCTION
*
*     NU_HTTP_Lite_Client_Delete
*
*   DESCRIPTION
*
*     This function request a DELETE on the http data server.
*     Returns the code from the server in the http_status field.
*
*   INPUTS
*
*     session_id            The ID associated with an internally
*                            managed session ID structure.
*     *cli_struct           Data structure that holds information
*                           regarding the DELETE operation:
*                               uri         Name of the resource to delete.
*     http_status           The status returned by the server.
*
*   OUTPUTS
*
*     NU_SUCCESS                The operation was successfully completed.
*     HTTP_INVALID_PARAMETER    One of the input parameters is invalid.
*     HTTP_INVALID_URI          The URI is not formed properly.
*     HTTP_NO_IP_NODE           The host name is invalid.
*     HTTP_SSL_ERROR            CyaSSL could not be initialized.
*     HTTP_ERROR_DATA_WRITE     Data could not be sent to the server.
*     HTTP_INVALID_HEADER_READ  No response from the server.
*     HTTP_INVALID_HEADER_PARSE The server is not using a compatible
*                               HTTP version.
*
*     Otherwise, an operating-system specific error is returned,
*     indicating the nature of the failure; out of memory, cannot
*     connect to server, etc.
*
************************************************************************/
STATUS NU_HTTP_Lite_Client_Delete(INT32 session_id, NU_HTTP_CLIENT *cli_struct,
                                  STATUS *http_status)
{
    STATUS          status;
    NU_HTTP_SH_S    *session_handle;

    /* Parameter checking */
    if ( (cli_struct) && (cli_struct->uri) && (http_status) )
    {
        /* Get the system semaphore so we can safely access the internal
         * list of session handles.
         */
        status = NU_Obtain_Semaphore(&HTTP_Client_Resource, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Get a pointer to the session handle. */
            session_handle = HTTP_Client_Get_Session_Handle(session_id);

            if (session_handle)
            {
                /* Lock the session structure */
                status = NU_Obtain_Semaphore(&session_handle->lock, NU_SUSPEND);

                if (status == NU_SUCCESS)
                {
                    /* Release the system semaphore since the session handle
                     * is now locked.
                     */
                    NU_Release_Semaphore(&HTTP_Client_Resource);

#if (HTTP_INCLUDE_CYASSL == NU_TRUE)
                    session_handle->ssl_struct = &cli_struct->ssl_struct;
#endif

                    /* Send the request */
                    *http_status = HTTP_Lite_Client_Query("DELETE", cli_struct->uri, "",
                                                          HTTP_CLOSE, NULL, 0, NULL,
                                                          session_handle);

#if (HTTP_INCLUDE_CYASSL == NU_TRUE)
                    session_handle->ssl_struct = NU_NULL;
#endif

                    if ( (*http_status == HTTP_PROTO_OK) ||
                         (*http_status == HTTP_PROTO_ACCEPTED) ||
                         (*http_status == HTTP_PROTO_NO_RESPONSE) )
                    {
                        status = NU_SUCCESS;
                    }

                    else if (*http_status >= 0)
                    {
                        status = HTTP_RESPONSE_ERROR;
                    }

                    else
                    {
                        /* http_session contains a negative error status */
                        status = *http_status;
                    }

                    /* Release the semaphore. */
                    NU_Release_Semaphore(&session_handle->lock);
                }

                else
                {
                    /* Release the system semaphore. */
                    NU_Release_Semaphore(&HTTP_Client_Resource);
                }
            }

            /* The session ID is not valid. */
            else
            {
                status = HTTP_INVALID_PARAMETER;

                /* Release the system semaphore. */
                NU_Release_Semaphore(&HTTP_Client_Resource);
            }
        }
    }

    else
    {
        status = HTTP_INVALID_PARAMETER;
    }

    return (status);
}


/************************************************************************
*
*    FUNCTION
*
*     NU_HTTP_Lite_Client_Post
*
*   DESCRIPTION
*
*     This function sends data to the http data server.
*     The data will be stored under the resource name uri.
*     Returns the code from the server in the http_session field.
*
*   INPUTS
*
*     session_id            The ID associated with an internally
*                            managed session ID structure.
*     *cli_struct           Data structure that holds information
*                           regarding the POST operation:
*                               uri         Name of the resource to create.
*                               pdata       Pointer to the data to send.
*                               plength     Length of the data to send.
*                               ptype       Type of the data.  If NULL, the
*                                           default type is used.
*                               overwrite   Flag to request to overwrite the
*                                           resource if it already exists.
*     http_status           The status code returned by the server.
*
*   OUTPUTS
*
*     NU_SUCCESS                The operation was successfully completed.
*     HTTP_INVALID_PARAMETER    One of the input parameters is invalid.
*     HTTP_INVALID_URI          The URI is not formed properly.
*     HTTP_NO_IP_NODE           The host name is invalid.
*     HTTP_SSL_ERROR            CyaSSL could not be initialized.
*     HTTP_ERROR_DATA_WRITE     Data could not be sent to the server.
*     HTTP_INVALID_HEADER_READ  No response from the server.
*     HTTP_INVALID_HEADER_PARSE The server is not using a compatible
*                               HTTP version.
*
*     Otherwise, an operating-system specific error is returned,
*     indicating the nature of the failure; out of memory, cannot
*     connect to server, etc.
*
************************************************************************/
STATUS NU_HTTP_Lite_Client_Post(INT32 session_id, NU_HTTP_CLIENT *cli_struct,
                                STATUS *http_status)
{
    CHAR ow_str[] = "Control: overwrite=1\r\n";
    CHAR additional_header[sizeof(HTTP_TRANSFER_ENCODING) + sizeof(HTTP_CHUNKED) + sizeof(HTTP_CRLF) +
                           sizeof(HTTP_CONTENT_TYPE) + 64 /* max content type */ + sizeof(HTTP_CRLF) +
                           sizeof(ow_str) + sizeof(HTTP_CRLF)];
    STATUS          status;
    NU_HTTP_SH_S    *session_handle;

    /* Parameter checking */
    if ( (cli_struct) && (cli_struct->uri) && (http_status) &&
         ((cli_struct->pdata) || (cli_struct->plength == 0)) )
    {
        /* Prepare the request additional_header */
        if (cli_struct->ptype)
        {
            snprintf(additional_header, sizeof(additional_header),
                     "Content-type: %.64s\r\n%s Transfer-Encoding: chunked\r\n",
                     cli_struct->ptype, cli_struct->overwrite ? ow_str : "");
        }

        else
        {
            /* The Transfer-Encoding header should be last. */
            snprintf(additional_header, sizeof(additional_header), "Transfer-Encoding: chunked\r\n");
        }

        /* Get the system semaphore so we can safely access the internal
         * list of session handles.
         */
        status = NU_Obtain_Semaphore(&HTTP_Client_Resource, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Get a pointer to the session handle. */
            session_handle = HTTP_Client_Get_Session_Handle(session_id);

            if (session_handle)
            {
                /* Lock the session structure */
                status = NU_Obtain_Semaphore(&session_handle->lock, NU_SUSPEND);

                if (status == NU_SUCCESS)
                {
                    /* Release the system semaphore since the session handle is
                     * now locked.
                     */
                    NU_Release_Semaphore(&HTTP_Client_Resource);

#if (HTTP_INCLUDE_CYASSL == NU_TRUE)
                    session_handle->ssl_struct = &cli_struct->ssl_struct;
#endif

                    /* Send the request */
                    *http_status = HTTP_Lite_Client_Query("POST", cli_struct->uri,
                                                          additional_header,
                                                          HTTP_CLOSE, cli_struct->pdata,
                                                          cli_struct->plength, NU_NULL,
                                                          session_handle);

#if (HTTP_INCLUDE_CYASSL == NU_TRUE)
                    session_handle->ssl_struct = NU_NULL;
#endif

                    if ( (*http_status == HTTP_PROTO_OK) ||
                         (*http_status == HTTP_PROTO_CREATED) ||
                         (*http_status == HTTP_PROTO_NO_RESPONSE) )
                    {
                        status = NU_SUCCESS;
                    }

                    else if (*http_status >= 0)
                    {
                        status = HTTP_RESPONSE_ERROR;
                    }

                    else
                    {
                        /* http_session contains a negative error status */
                        status = *http_status;
                    }

                    /* Release the semaphore. */
                    NU_Release_Semaphore(&session_handle->lock);
                }

                else
                {
                    /* Release the system semaphore. */
                    NU_Release_Semaphore(&HTTP_Client_Resource);
                }
            }

            /* The session ID is not valid. */
            else
            {
                status = HTTP_INVALID_PARAMETER;

                /* Release the system semaphore. */
                NU_Release_Semaphore(&HTTP_Client_Resource);
            }
        }
    }
    else
    {
        status = HTTP_INVALID_PARAMETER;
    }

    return (status);
}
