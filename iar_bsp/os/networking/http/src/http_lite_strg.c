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
*       http_lite_strg.c
*
*   COMPONENT
*
*       Nucleus HTTP Lite
*
*   DESCRIPTION
*
*       This file holds the in-memory storage routines for the HTTP Lite
*       module.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_HTTP_Lite_Create_File
*       NU_HTTP_Lite_Delete_File
*       NU_HTTP_Lite_Write_File
*       HTTP_Lite_Create_File
*       HTTP_Lite_Delete_File
*       HTTP_Lite_Find_File
*       HTTP_Lite_Write_File
*       HTTP_Lite_Svr_Get
*       HTTP_Lite_Svr_Put
*       HTTP_Lite_Svr_Delete
*       HTTP_Lite_Get_Mime_Type
*
*   DEPENDENCIES
*
*       http_lite.h
*       nu_networking.h
*
************************************************************************/

#include "networking/nu_networking.h"
#include "os/networking/http/inc/http_lite_int.h"

/* The mime table maps file types to mime extensions.
 * if the file type does not require a header sent with
 * the reply, NU_NULL is associated with it.
 */
const HTTP_MIME_TABLE HTTP_Mime_Table[]=
{
    {"txt",  "text/plain"},
    {"text", "text/plain"},
    {"html", "text/html"},
    {"htm",  "text/html"},
    {"gif",  "image/gif"},
    {"jpg",  "image/jpeg"},
    {"jpeg", "image/jpeg"},
    {"jar",  "application/octet-stream"},
    {NU_NULL, NU_NULL}
};

extern NU_MEMORY_POOL           *MEM_Cached;

extern HTTP_FILE_LIST           HTTP_Lite_File_List;
extern NU_SEMAPHORE             HTTP_Lite_Resource;
extern HTTP_SVR_SESSION_STRUCT  *HTTP_Session;

STATIC CHAR                 *HTTP_Lite_Get_Mime_Type(CHAR *);

/************************************************************************
*
*   FUNCTION
*
*       NU_HTTP_Lite_Create_File
*
*   DESCRIPTION
*
*       Creates a new file with the given file name of the specified
*       length.
*
*   INPUTS
*
*       *fname                  Name of the new file.
*
*   OUTPUTS
*
*       NU_SUCCESS                  The file was successfully created.
*       HTTP_INVALID_PARAMETER      The input is invalid.
*       HTTP_FILE_ALREADY_EXISTS    The filename already exists.
*
************************************************************************/
STATUS NU_HTTP_Lite_Create_File(CHAR *fname)
{
    HTTP_FILE_STRUCT    *fptr;
    STATUS              status;

    /* Validate the input parameters. */
    if (fname)
    {
        /* Get the semaphore. */
        status = NU_Obtain_Semaphore(&HTTP_Lite_Resource, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Ensure the file does not already exist. */
            fptr = HTTP_Lite_Find_File(fname);

            if (!fptr)
            {
                /* Create the file. */
                fptr = HTTP_Lite_Create_File(fname, &status);
            }

            else
                status = HTTP_FILE_ALREADY_EXISTS;

            /* Release the semaphore. */
            NU_Release_Semaphore(&HTTP_Lite_Resource);
        }
    }

    else
        status = HTTP_INVALID_PARAMETER;

    return (status);

} /* NU_HTTP_Lite_Create_File */

/************************************************************************
*
*   FUNCTION
*
*       NU_HTTP_Lite_Write_File
*
*   DESCRIPTION
*
*       Writes data to the end of an existing file.
*
*   INPUTS
*
*       *fname                  Name of the file to write to.
*       len                     Length of the new data.
*       *buffer                 Pointer to the content.
*
*   OUTPUTS
*
*       NU_SUCCESS              The data was written to the file.
*       HTTP_INVALID_PARAMETER  The input is invalid.
*
************************************************************************/
STATUS NU_HTTP_Lite_Write_File(CHAR *fname, UINT32 len, CHAR *buffer)
{
    STATUS              status;
    HTTP_FILE_STRUCT    *fptr;

    if ( (fname) && (buffer) )
    {
        /* Get the semaphore. */
        status = NU_Obtain_Semaphore(&HTTP_Lite_Resource, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Get a pointer to the file. */
            fptr = HTTP_Lite_Find_File(fname);

            if (fptr)
            {
                /* Write to the file. */
                status = HTTP_Lite_Write_File(fptr, len, buffer);
            }

            /* The file does not exist. */
            else
                status = HTTP_INVALID_PARAMETER;

            /* Release the semaphore. */
            NU_Release_Semaphore(&HTTP_Lite_Resource);
        }
    }

    else
        status = HTTP_INVALID_PARAMETER;

    return (status);

} /* NU_HTTP_Lite_Write_File */

/************************************************************************
*
*   FUNCTION
*
*       NU_HTTP_Lite_Delete_File
*
*   DESCRIPTION
*
*       Deletes an existing file.
*
*   INPUTS
*
*       *fname                  The name of the file to delete.
*
*   OUTPUTS
*
*       NU_SUCCESS              The file was deleted.
*       HTTP_INVALID_PARAMETER  The file does not exist or fname is
*                               invalid.
*
************************************************************************/
STATUS NU_HTTP_Lite_Delete_File(CHAR *fname)
{
    STATUS              status;
    HTTP_FILE_STRUCT    *fptr;

    /* Validate the input parameter. */
    if (fname)
    {
        /* Get the semaphore. */
        status = NU_Obtain_Semaphore(&HTTP_Lite_Resource, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Get a pointer to the file. */
            fptr = HTTP_Lite_Find_File(fname);

            if (fptr)
            {
                /* Delete the file. */
                status = HTTP_Lite_Delete_File(fptr);
            }

            else
                status = HTTP_INVALID_PARAMETER;

            /* Release the semaphore. */
            NU_Release_Semaphore(&HTTP_Lite_Resource);
        }
    }

    else
        status = HTTP_INVALID_PARAMETER;

    return (status);

} /* NU_HTTP_Lite_Delete_File */

/************************************************************************
*
*   FUNCTION
*
*       HTTP_Lite_Create_File
*
*   DESCRIPTION
*
*       Creates a new file.  This routine does not check if a file of
*       the same name already exists - it is up to the user to ensure
*       exclusive naming.
*
*   INPUTS
*
*       *fname                  Name of the new file.
*       *status                 Pointer to status that will be filled in.
*
*   OUTPUTS
*
*       Pointer to the new file structure, or NU_NULL upon failure.
*
************************************************************************/
HTTP_FILE_STRUCT *HTTP_Lite_Create_File(CHAR *fname, STATUS *status)
{
    HTTP_FILE_STRUCT    *new_file = NU_NULL;

    /* Allocate memory for the new file structure.  Account for the size of the
     * structure, the file name plus null-terminator, and the memory for the
     * actual file.
     */
    *status = NU_Allocate_Memory(MEM_Cached, (VOID **)&new_file,
                                 sizeof(HTTP_FILE_STRUCT) +
                                 (strlen(fname) + 1),
                                 (UNSIGNED)NU_SUSPEND);

    if (*status == NU_SUCCESS)
    {
        /* Set the file name pointer. */
        new_file->http_fname = (CHAR*)(&new_file[1]);

        /* Set the file name. */
        strcpy(new_file->http_fname, fname);

        /* Initialize the file pointer to NULL. */
        new_file->http_fptr = NU_NULL;

        /* There are currently zero bytes in the file. */
        new_file->http_flen = 0;

        /* Add the file to the list of file pointers. */
        DLL_Enqueue(&HTTP_Lite_File_List, new_file);
    }

    return (new_file);

} /* HTTP_Lite_Create_File */

/************************************************************************
*
*   FUNCTION
*
*       HTTP_Lite_Write_File
*
*   DESCRIPTION
*
*       Writes data to the end of an existing file.
*
*   INPUTS
*
*       *fptr                   Pointer to the file.
*       len                     Length of the new data.
*       *buffer                 Pointer to the content.
*
*   OUTPUTS
*
*       NU_SUCCESS              The data was written to the file.
*       NU_NO_MEMORY            There is not enough room in the file
*                               to write the data.
*
************************************************************************/
STATUS HTTP_Lite_Write_File(HTTP_FILE_STRUCT *fptr, UINT32 len,
                            CHAR *buffer)
{
    STATUS  status = NU_SUCCESS;

    /* If more than zero bytes is being written. */
    if (len)
    {
        /* If the file has already been written to, reallocate memory
         * for the new data.
         */
        if (fptr->http_fptr)
        {
            status = NU_Reallocate_Memory(MEM_Cached, (VOID**)&fptr->http_fptr,
                                          fptr->http_flen + len, NU_SUSPEND);
        }

        else
        {
            status = NU_Allocate_Memory(MEM_Cached, (VOID**)&fptr->http_fptr,
                                        len, NU_SUSPEND);
        }

        /* If memory was allocated successfully. */
        if (status == NU_SUCCESS)
        {
            /* Copy the new contents. */
            NU_Block_Copy(&fptr->http_fptr[fptr->http_flen], buffer, len);

            /* Update the length of the file. */
            fptr->http_flen += len;
        }

        else
            status = NU_NO_MEMORY;
    }

    return (status);

} /* HTTP_Lite_Write_File */

/************************************************************************
*
*   FUNCTION
*
*       HTTP_Lite_Delete_File
*
*   DESCRIPTION
*
*       Deletes an existing file.
*
*   INPUTS
*
*       *fptr                   Pointer to the file to delete.
*
*   OUTPUTS
*
*       NU_SUCCESS upon success or an operating-system specific error
*       code upon failure.
*
************************************************************************/
STATUS HTTP_Lite_Delete_File(HTTP_FILE_STRUCT *fptr)
{
    STATUS  status;

    if (fptr)
    {
        /* Remove the file from the list. */
        DLL_Remove(&HTTP_Lite_File_List, fptr);

        /* Deallocate the memory. */
        status = NU_Deallocate_Memory((VOID*)fptr);
    }

    else
        status = NU_SUCCESS;

    return (status);

} /* HTTP_Lite_Delete_File */

/************************************************************************
*
*   FUNCTION
*
*       HTTP_Lite_Find_File
*
*   DESCRIPTION
*
*       Finds a file with the specific name.
*
*   INPUTS
*
*       *fname                  The name of the file to find.
*
*   OUTPUTS
*
*       A pointer to the file structure or NU_NULL if the file does not
*       exist.
*
************************************************************************/
HTTP_FILE_STRUCT *HTTP_Lite_Find_File(CHAR *fname)
{
    HTTP_FILE_STRUCT    *fptr = NU_NULL;

    /* Traverse through the list of files looking for a file of the
     * specified name.
     */
    for (fptr = HTTP_Lite_File_List.flink; fptr; fptr = fptr->flink)
    {
        /* If this is the matching file. */
        if (strcmp(fptr->http_fname, fname) == 0)
            break;
    }

    return (fptr);

} /* HTTP_Lite_Find_File */

/************************************************************************
*
*   FUNCTION
*
*       HTTP_Lite_Svr_Get
*
*   DESCRIPTION
*
*       Default function to execute GET operation which will retrieve
*       a file matching the uri.  This routine will be registered at
*       start up as the default routine for GET operations if
*       get_file_enable is set to true in the .metadata file for HTTP
*       Lite.
*
*   INPUTS
*
*       method                      The method being requested by the
*                                   client.
*       *uri                        The URI in the request.
*       *http_ptr                   A pointer to the HTTP Server
*                                   structure.
*
*   OUTPUTS
*
*       NU_SUCCESS or an operating-system specific error upon failure.
*
************************************************************************/
STATUS HTTP_Lite_Svr_Get(INT method, CHAR *uri, HTTP_SVR_SESSION_STRUCT *http_ptr)
{
    HTTP_FILE_STRUCT    *fptr;
    STATUS              status;
    CHAR                *mime;

    /* Find the file. */
    fptr = HTTP_Lite_Find_File(uri);

    /* If the file exists. */
    if (fptr)
    {
        /* Get the mime type of the filename. */
        mime = HTTP_Lite_Get_Mime_Type(fptr->http_fname);

        if (*mime)
        {
            /* Generate the "OK" response header. */
            HTTP_Lite_Response_Header(HTTP_PROTO_OK);

            /* Insert the proper header field. */
            strcat(http_ptr->buffer, " Document Follows");
            strcat(http_ptr->buffer, "\r\n");

            /* Insert the file name. */
            HTTP_Lite_Header_Name_Insert(HTTP_CONTENT_TYPE, mime);

            /* Insert the Transfer encoding. */
            HTTP_Lite_Header_Name_Insert(HTTP_TRANSFER_ENCODING, HTTP_CHUNKED);

            strcat(http_ptr->buffer, "\r\n");

            /* Send the header and the chunks. */
            status = HTTP_Lite_Write_Buffer(http_ptr->socketd,
                                            http_ptr->buffer,
                                            HTTP_SVR_RCV_SIZE,
                                            fptr->http_fptr,
                                            fptr->http_flen,
                                            HTTP_LITE_SVR_SSL_STRUCT);
        }

        else
        {
            status = HTTP_INVALID_HEADER_PARSE;
        }
    }

    /* Otherwise, send an error. */
    else
    {
        status = HTTP_Lite_Send_Status_Message(HTTP_PROTO_NOT_FOUND,
                                               HTTP_NOT_FOUND);
    }

    return (status);

} /* HTTP_Lite_Svr_Get */

/************************************************************************
*
*   FUNCTION
*
*       HTTP_Lite_Svr_Put
*
*   DESCRIPTION
*
*       Default function to execute PUT operation which will store
*       a file named by the uri.  This routine will be registered at
*       start up as the default routine for PUT operations if
*       put_file_enable is set to true in the .metadata file for HTTP
*       Lite.
*
*   INPUTS
*
*       method                      The method being requested by the
*                                   client.
*       *uri                        The URI in the request.
*       *http_ptr                   A pointer to the HTTP Server
*                                   structure.
*
*   OUTPUTS
*
*       NU_SUCCESS or an operating-system specific error upon failure.
*
************************************************************************/
STATUS HTTP_Lite_Svr_Put(INT method, CHAR *uri, HTTP_SVR_SESSION_STRUCT *http_ptr)
{
    HTTP_FILE_STRUCT    *fptr;
    UINT16              code;
    STATUS              status = NU_SUCCESS;
    CHAR                *string, *size;
    UINT32              length;

    /* RFC 2616 - section 9.6 - If the Request-URI refers to an already
     * existing resource, the enclosed entity SHOULD be considered as a
     * modified version of the one residing on the origin server.
     */
    fptr = HTTP_Lite_Find_File(uri);

    /* If the file already exists, delete it so the new one can
     * replace it.
     */
    if (fptr)
    {
        HTTP_Lite_Delete_File(fptr);

        /* RFC 2616 - section 9.6 - If an existing resource is modified,
         * either the 200 (OK) or 204 (No Content) response codes SHOULD
         * be sent to indicate successful completion of the request.
         */
        code = HTTP_PROTO_OK;
        string = HTTP_OK;
    }

    else
    {
        /* RFC 2616 - section 9.6 - If a new resource is created,
         * the origin server MUST inform the user agent via the
         * 201 (Created) response.
         */
        code = HTTP_PROTO_CREATED;
        string = HTTP_CREATED;
    }

    /* Create the file. */
    fptr = HTTP_Lite_Create_File(uri, &status);

    if (fptr)
    {
        /* Search for the chunked header. */
        if (HTTP_Lite_Find_Token(HTTP_CHUNKED, http_ptr->buffer,
                                 &http_ptr->buffer[http_ptr->in_hdr_size],
                                 HTTP_TOKEN_CASE_INSENS))
        {
            /* The length of the file is not known. */
            length = 0;
        }

        /* Search for the Content-Length header. */
        else if ((size = HTTP_Lite_Find_Token(HTTP_CONTENT_LENGTH, http_ptr->buffer,
                                              &http_ptr->buffer[http_ptr->in_hdr_size],
                                              HTTP_TOKEN_CASE_INSENS)) != NU_NULL)
        {
            /* Increment Pointer to Content Length within the buffer */
            size += strlen(HTTP_CONTENT_LENGTH);

            /* Get the total length of the data */
            length = NU_ATOL(size);
        }

        /* There is no content-length header or chunked header. */
        else
        {
            status = HTTP_INVALID_HEADER_PARSE;
            code = HTTP_PROTO_SERVER_ERROR;
            string = HTTP_SERVER_ERROR;
        }

        if (status == NU_SUCCESS)
        {
            /* Allocate a new buffer for the data and read data into it. */
            fptr->http_flen = HTTP_Lite_Read_Buffer(http_ptr->buffer,
                                                    HTTP_SVR_RCV_SIZE,
                                                    http_ptr->socketd,
                                                    &(fptr->http_fptr),
                                                    length,
                                                    HTTP_LITE_SVR_SSL_STRUCT,
                                                    0,
                                                    NU_NULL, NU_NULL);

            /* If the data could not be read, return an error. */
            if (fptr->http_flen == 0)
            {
                status = HTTP_ERROR_DATA_READ;
                code = HTTP_PROTO_SERVER_ERROR;
                string = HTTP_SERVER_ERROR;
            }
        }
    }

    else
    {
        /* The system is out of memory. */
        status = NU_NO_MEMORY;
        code = HTTP_PROTO_SERVER_ERROR;
        string = HTTP_SERVER_ERROR;
    }

    /* Send the status back to the client. */
    if (HTTP_Lite_Send_Status_Message(code, string) != NU_SUCCESS)
        status = HTTP_ERROR_DATA_WRITE;

    return (status);

} /* HTTP_Lite_Svr_Put */

/************************************************************************
*
*   FUNCTION
*
*       HTTP_Lite_Svr_Delete
*
*   DESCRIPTION
*
*       Default function to execute DELETE operation which will delete
*       a file named by the uri.  This routine will be registered at
*       start up as the default routine for DELETE operations if
*       delete_file_enable is set to true in the .metadata file for HTTP
*       Lite.
*
*   INPUTS
*
*       method                      The method being requested by the
*                                   client.
*       *uri                        The URI in the request.
*       *http_ptr                   A pointer to the HTTP Server
*                                   structure.
*
*   OUTPUTS
*
*       NU_SUCCESS or an operating-specific error upon failure.
*
************************************************************************/
STATUS HTTP_Lite_Svr_Delete(INT method, CHAR *uri, HTTP_SVR_SESSION_STRUCT *http_ptr)
{
    STATUS  status;

    /* RFC 2616 - section 9.6 - If the Request-URI refers to an already
     * existing resource, the enclosed entity SHOULD be considered as a
     * modified version of the one residing on the origin server.
     */
    HTTP_Lite_Delete_File(HTTP_Lite_Find_File(uri));

    status = HTTP_Lite_Send_Status_Message(HTTP_PROTO_OK, HTTP_OK);

    return (status);

} /* HTTP_Lite_Svr_Delete */

/************************************************************************
*
*   FUNCTION
*
*       HTTP_Lite_Get_Mime_Type
*
*   DESCRIPTION
*
*       Function that derives a mime type from the file extension. The
*       mime type tells the browser what kind of file it's getting
*       based on the extension.
*
*   INPUTS
*
*       *fname                  Pointer to the file name.
*
*   OUTPUTS
*
*       mime_table              Returns the mime type in the
*                               table.
*
*************************************************************************/
STATIC CHAR *HTTP_Lite_Get_Mime_Type(CHAR *fname)
{
    CHAR    *ptr;
    INT     i;

    /* file name */
    ptr = fname;

    /* Find a '.' or the end of the name. */
    while ( (*ptr) && (*ptr != '.') )
        ptr++;

    /* Search the Mime table - if there is no extension, use
     * the default mime type.
     */
    if (*ptr != NU_NULL)
    {
        ptr++;

        for (i = 0; HTTP_Mime_Table[i].http_ext; i++)
        {
            /* If the type matches. */
            if (NU_STRICMP(ptr, HTTP_Mime_Table[i].http_ext) == 0)
                break;
        }

        /* If a match was not found, return the default. */
        if (!HTTP_Mime_Table[i].http_ext)
            i = HTTP_DEFAULT_MIME;
    }

    else
        i = HTTP_DEFAULT_MIME;

    return (HTTP_Mime_Table[i].http_mime_type);

} /* HTTP_Lite_Get_Mime_Type */
