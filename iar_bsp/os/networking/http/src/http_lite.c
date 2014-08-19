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
*       http_lite.c
*
*   COMPONENT
*
*       Nucleus HTTP Lite
*
*   DESCRIPTION
*
*       This file holds routines shared between the HTTP Lite Client
*       and Server.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       nu_os_net_http_init
*       HTTP_Lite_Select
*       HTTP_Lite_Get_Chunk_Data
*       HTTP_Lite_Put_Chunk_Data
*       HTTP_Lite_Write
*       HTTP_Lite_Receive
*       HTTP_Lite_Read_Line
*       HTTP_Lite_Read_Buffer
*       HTTP_Lite_Write_Buffer
*
*   DEPENDENCIES
*
*       nu_networking.h
*       nu_http_lite.h
*
************************************************************************/

#include "networking/nu_networking.h"
#include "os/networking/http/inc/http_lite_int.h"
#include "services/reg_api.h"

CHAR    HTTP_Lite_Server_Registry_Path[REG_MAX_KEY_LENGTH] = {0};

/************************************************************************
*
*   FUNCTION
*
*       nu_os_net_http_init
*
*   DESCRIPTION
*
*       This function initializes the HTTP Lite Client and/or Server.
*       It is the run-time entry point of the product initialization
*       sequence.
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
STATUS nu_os_net_http_init(CHAR *path, INT startstop)
{
    STATUS      status = NU_SUCCESS;

#if ( (CFG_NU_OS_NET_HTTP_CLIENT_ENABLE == NU_TRUE) || (CFG_NU_OS_NET_HTTP_SERVER_ENABLE == NU_TRUE) )

    if (path)
    {
        /* Save a copy locally. */
        strcpy(HTTP_Lite_Server_Registry_Path, path);
    }

#endif

#if (CFG_NU_OS_NET_HTTP_CLIENT_ENABLE == NU_TRUE)

    if (startstop)
    {
        /* Initialize the HTTP Lite Client module. */
        status = NU_HTTP_Lite_Client_Init();
    }

#endif

#if (CFG_NU_OS_NET_HTTP_SERVER_ENABLE == NU_TRUE)

    if (status == NU_SUCCESS)
    {
        if (startstop)
        {
            /* Initialize the HTTP Lite Server module. */
            status = NU_HTTP_Lite_Server_Init();

            if (status != NU_SUCCESS)
            {
                NLOG_Error_Log("Error at call to nu_os_net_http_init().\n",
                               NERR_FATAL, __FILE__, __LINE__);
            }
        }

        else
        {
            /* Stop requested */
            NU_HTTP_Lite_Server_Shutdown();
        }
    }

#endif

    return (status);

} /* nu_os_net_http_init */

/************************************************************************
*
*   FUNCTION
*
*       HTTP_Lite_Select
*
*   DESCRIPTION
*
*       This function selects on a socket for reading.  It waits
*       HTTP_SVR_TIMEOUT seconds before timing out and returning
*       an error.
*
*   INPUTS
*
*       socketd                 The socket to wait for data on.
*
*   OUTPUTS
*
*       NU_SUCCESS              There is data ready for reading.
*       HTTP_SOCKET_NOT_READY   The call timed out.
*
*************************************************************************/
STATUS HTTP_Lite_Select(INT socketd, VOID *ssl)
{
    FD_SET              readfs;
    STATUS              status;

#if (HTTP_INCLUDE_CYASSL == NU_TRUE)
    if (ssl)
        status = NU_SUCCESS;

    else
#endif
    {
        /* Initialize the bitmap */
        NU_FD_Init(&readfs);

        /* Set the read bit. */
        NU_FD_Set(socketd, &readfs);

        /* RFC 2616 - section 8.1.4 - Servers will usually have some
         * time-out value beyond which they will no longer maintain
         * an inactive connection.
         */
        status = NU_Select(socketd + 1, &readfs, NU_NULL, NU_NULL,
                           (HTTP_TIMEOUT * NU_TICKS_PER_SECOND));

        if (status == NU_SUCCESS)
        {
            /* Ensure this socket has data ready for reading. */
            if (NU_FD_Check(socketd, &readfs) != NU_TRUE)
                status = HTTP_SOCKET_NOT_READY;
        }
    }

    return (status);

} /* HTTP_Lite_Select */

/************************************************************************
*
*   FUNCTION
*
*       HTTP_Lite_Get_Chunk_Data
*
*   DESCRIPTION
*
*       This function parses all chunks out of an incoming packet
*       stream and returns a newly allocated buffer of memory that
*       contains the data minus the chunk information.
*
*   INPUTS
*
*       *rx_buffer              A pointer to the buffer into which data
*                               can be received for parsing the chunk
*                               header.
*       **data_ptr              A pointer that will be filled in with all
*                               the data in the packet.
*       sd                      The socket over which to receive data.
*       max_len                 The maximum length of rx_buffer.
*       *ssl                    If this is a secure connection, the SSL
*                               structure associated with the connection.
*       *data_len               Input: This is the maximum size of the data buffer.
*                               Output: The number of bytes not received due to max buffer size.
*
*   OUTPUTS
*
*       The number of bytes read into data_ptr or zero if an error occurred.
*
*************************************************************************/
UINT32 HTTP_Lite_Get_Chunk_Data(CHAR *rx_buffer, CHAR **data_ptr, INT sd,
                                UINT32 max_len, VOID *ssl, UINT32 *data_len)
{
    UINT32  chunk_len, total_len = 0;
    STATUS  status;
    INT     bytes_recv;
    CHAR    *temp_ptr;
    UINT32  max_data_len = 0;
    UINT32  temp_len = 0;

    /* Initialize data pointer */
    *data_ptr = NU_NULL;

    if ((data_len != NU_NULL) && (*data_len > 0))
    {
        max_data_len = *data_len;
    }

    do
    {
        /* Read the current chunk header. */
        if ((bytes_recv = HTTP_Lite_Read_Line(rx_buffer, sd, max_len, NU_TRUE, ssl)) > 0)
        {
            /* If chunk extensions are provided, the chunk size is terminated
             * by a semicolon followed by the extension name and an optional
             * equal sign and value.
             */
            temp_ptr = HTTP_Lite_Find_Token(HTTP_SEMICOLON, rx_buffer,
                                            &rx_buffer[bytes_recv],
                                            HTTP_TOKEN_CASE_INSENS);

            if (temp_ptr)
            {
                /* This implementation does not support extensions, so change the
                 * semi-colon to a NULL-terminator so the length can be extracted.
                 */
                temp_ptr[0] = '\0';
            }

            /* Convert the ASCII length to a long. */
            chunk_len = NCL_Ahtol(rx_buffer);

            /* If a zero chunk length is received, we have received all
             * chunks.
             */
            if (chunk_len)
            {
                /* Add the two bytes for the CRLF so it gets received
                 * with the call to receive data.
                 */
                temp_len = chunk_len += strlen(HTTP_CRLF);

                if (data_len != NU_NULL)
                {
                    /* Check for max buffer size */
                    if ((total_len + chunk_len) > max_data_len)
                    {
                        if ((max_data_len - total_len) > 0)
                        {
                            /* Only receive max_data_len bytes */
                            temp_len = max_data_len - total_len;
                        }
                        else
                        {
                            /* No more data can be received.
                             * Save off the number of bytes
                             * remaining and return. */
                            *data_len = chunk_len;
                            break;
                        }
                    }
                }

                /* Allocate memory for the buffer.  The buffer will be a total
                 * of 2 bytes too long to account for the CRLF at the end
                 * of each chunk.  This is a trade off to keep the code simple.
                 * Otherwise, the data and CRLF will have to be received
                 * separately into separate buffers.
                 */
                status = NU_Reallocate_Memory(MEM_Cached, (VOID**)data_ptr,
                                              total_len + temp_len, NU_SUSPEND);

                /* If memory was successfully allocated, read the chunk into the
                 * buffer and discard the CRLF characters.
                 */
                if (status == NU_SUCCESS)
                {
                    /* Receive the full chunk.  This routine will always return
                     * a positive value.
                     */
                    bytes_recv = HTTP_Lite_Receive(sd, &((*data_ptr)[total_len]),
                                                   temp_len, ssl, NU_FALSE);

                    if (temp_len != chunk_len)
                    {
                        /* Partial receive due to max buffer */
                        if (bytes_recv != temp_len)
                        {
                            status = HTTP_ERROR_DATA_READ;
                        }
                        else
                        {
                            total_len += temp_len;

                            if (data_len != NU_NULL)
                            {
                                /* Save off the number of bytes remaining */
                                *data_len = chunk_len - temp_len;
                            }
                        }

                        break;
                    }
                    else
                    {
                        if (bytes_recv > 0)
                        {
                            /* Update the total number of bytes received, removing
                             * the trailing CRLF.
                             */
                            total_len += (bytes_recv - strlen(HTTP_CRLF));
                        }

                        /* If the entire chunk could not be received or there is no
                         * CRLF at the end of the data.
                         */
                        if ((bytes_recv != temp_len) ||
                             (memcmp(&((*data_ptr)[total_len]), HTTP_CRLF, strlen(HTTP_CRLF)) != 0) )
                        {
                            status = HTTP_ERROR_DATA_READ;
                        }
                    }
                }
            }

            /* This is not an error condition - we are just done reading
             * the chunks.
             */
            else
            {
                if (data_len != NU_NULL)
                {
                    *data_len = chunk_len;
                }
                status = NU_SUCCESS;
                break;
            }
        }

        /* The chunk header could not be read. */
        else
        {
            status = HTTP_ERROR_DATA_READ;
        }

    } while (status == NU_SUCCESS);

    /* If an error occurred, do not save any data. */
    if (status != NU_SUCCESS)
    {
        if (*data_ptr != NU_NULL)
        {
            NU_Deallocate_Memory(*data_ptr);
        }
        total_len = 0;
    }

    return (total_len);

} /* HTTP_Lite_Get_Chunk_Data */

/************************************************************************
*
*   FUNCTION
*
*       HTTP_Lite_Put_Chunk_Data
*
*   DESCRIPTION
*
*       This function adds a chunked data section to a buffer.
*
*   INPUTS
*
*       *buf_ptr                A pointer to the buffer into which to
*                               add the chunked data segment.
*       buffer_len              The available length of the buffer.
*       data_len                The length of the data to add.
*
*   OUTPUTS
*
*       The length of data added to the buffer.
*
*************************************************************************/
UINT32 HTTP_Lite_Put_Chunk_Data(CHAR *buf_ptr, UINT32 buffer_len, UINT32 data_len)
{
    CHAR    ah_ptr[12] = {0};
    UINT32  len;

    /* Convert the length to ASCII. */
    NCL_Ultoa(data_len, ah_ptr, 16);

    /* Get the length of the ASCII string. */
    len = strlen(ah_ptr);

    /* Ensure the chunked data option will fit in the buffer. */
    if ((len + strlen(HTTP_CRLF)) <= buffer_len)
    {
        /* Copy the ASCII length into the buffer. */
        strcpy(buf_ptr, ah_ptr);

        /* If this is not the last chunk, insert just one CRLF. */
        if (data_len)
        {
            /* Add the CRLF after the length. */
            strcpy(&buf_ptr[len], HTTP_CRLF);

            len += strlen(HTTP_CRLF);
        }

        /* Otherwise, if an additional CRLF will fit in the buffer, insert
         * two CRLF to indicate the end of the chunked-body.
         */
        else if ((len + strlen(HTTP_CRLFCRLF)) <= buffer_len)
        {
            /* Add the CRLFCRLF sequence after the zero length. */
            strcpy(&buf_ptr[len], HTTP_CRLFCRLF);

            len += strlen(HTTP_CRLFCRLF);
        }

        /* The buffer isn't big enough. */
        else
        {
            len = 0;
        }
    }

    else
    {
        len = 0;
    }

    return (len);

} /* HTTP_Lite_Put_Chunk_Data */

/************************************************************************
*
*   FUNCTION
*
*       HTTP_Lite_Write
*
*   DESCRIPTION
*
*       This routine transmits data to the other side of a connection.
*
*   INPUTS
*
*       socketd                 The socket for transmitting data.
*       *buff                   A pointer to the buffer of data to transmit.
*       nbytes                  The number of bytes to transmit.
*       *ssl_ptr                If SSL is enabled, a pointer to the CyaSSL
*                               data structure to use for sending the data
*                               securely.
*
*   OUTPUTS
*
*       The number of bytes transmitted, or an operating-system specific
*       error if the transmission failed.
*
************************************************************************/
UINT32 HTTP_Lite_Write(INT socketd, CHAR *buff, UINT32 nbytes, VOID *ssl_ptr)
{
    INT32   bytes_sent;
    UINT32  count = 0;

    /* Transmit the data in UINT16 increments. */
    while (nbytes)
    {
#if (HTTP_INCLUDE_CYASSL == NU_TRUE)
        if (ssl_ptr)
        {
            bytes_sent = SSL_write((SSL*)ssl_ptr, &buff[count],
                                   nbytes > 65535 ? 65535 : nbytes);
        }

        else
#endif
        {
            /* Send the first chunk header. */
            bytes_sent = NU_Send(socketd, &buff[count],
                                 nbytes > 65535 ? 65535 : nbytes,
                                 NU_NULL);
        }

        /* If data was successfully transmitted. */
        if (bytes_sent > 0)
        {
            count += bytes_sent;
            nbytes -= bytes_sent;
        }

        else
        {
            break;
        }
    }

    return (count);

} /* HTTP_Lite_Write */

/************************************************************************
*
*   FUNCTION
*
*       HTTP_Lite_Receive
*
*   DESCRIPTION
*
*       This routine receives data from the other side of a connection.
*
*   INPUTS
*
*       socketd                 The socket for receiving data.
*       *buff                   A pointer to the buffer of data to receive.
*       nbytes                  The max number of bytes to receive.
*       *ssl_ptr                If SSL is enabled, a pointer to the CyaSSL
*                               data structure to use for receiving the data
*                               securely.
*       partial                 NU_TRUE: Receive less than length returns size.
*                               NU_FALSE: Receive less than length returns 0.
*
*   OUTPUTS
*
*       The number of bytes received, or zero if an error occurred.
*
************************************************************************/
UINT32 HTTP_Lite_Receive(INT socketd, CHAR *buff, UINT32 length, VOID *ssl_ptr, UINT8 partial)
{
    INT32   ret_val;
    UINT32  n = 0;

    while (length)
    {
        /* Wait for data. */
        ret_val = HTTP_Lite_Select(socketd, ssl_ptr);

        if (ret_val == NU_SUCCESS)
        {
            /* Read the data into the buffer. If a bigger value than will
             * fit into a UINT16 needs to be received, break length up into
             * segments of 65535.
             */
#if (HTTP_INCLUDE_CYASSL == NU_TRUE)
            if (ssl_ptr)
            {
                ret_val = CyaSSL_read((SSL*)ssl_ptr, &buff[n],
                                      length > 65535 ? 65535 : length);
            }
            else
#endif
            {
                ret_val = NU_Recv(socketd, &buff[n],
                                  length > 65535 ? 65535 : length, 0);
            }
        }

        if (ret_val <= 0)
        {
            if (partial == NU_TRUE)
            {
                /* In case of "close connection" header,
                 * the size is unknown. Just return the
                 * number of bytes received.
                 */
                break;
            }
            n = 0;
            break;
        }

        /* Increment the number of bytes received. */
        n += ret_val;

        /* Decrement the number of bytes left. */
        length -= ret_val;
    }

    return (n);

} /* HTTP_Lite_Receive */

/************************************************************************
*
*   FUNCTION
*
*       HTTP_Lite_Read_Line
*
*   DESCRIPTION
*
*       Read a line from socket descriptor. Returns the number of bytes
*       read. Negative if a read error occurred before the end of line
*       or the max. Carriage returns (CR) are ignored.
*
*   INPUTS
*
*       *buf                    The buffer into which to receive the data.
*       sd                      Socket descriptor to read from.
*       max_len                 The maximum number of bytes to read into
*                               buf.
*       terminate               A boolean flag indicating whether to
*                               null-terminate the line that was read
*                               into buf or to return it intact.
*       *ssl_ptr                If the connection is secure, a pointer
*                               to the SSL structure associated with the
*                               connection.
*
*   OUTPUTS
*
*     The number of bytes read, or an operating-system specific error.
*
************************************************************************/
INT HTTP_Lite_Read_Line(CHAR *buf, INT sd, UINT32 max_len, UINT8 terminate,
                        VOID *ssl_ptr)
{
    INT     n = 0, ret_val;

    /* Receive a max of max_len bytes. */
    while (n < (max_len - 1))
    {
        /* Get one byte at a time. */
        ret_val = HTTP_Lite_Receive(sd, buf, 1, ssl_ptr, NU_FALSE);

        /* If a byte of data could not be received, break out of the loop
         * and return an error.
         */
        if (ret_val != 1)
        {
            n = ret_val;
            break;
        }

        /* Increment the number of bytes received. */
        n++;

        /* If this is a carriage return and the caller wants the data to
         * be null-terminated at the end of a CRLF, ignore it and receive
         * the next byte.
         */
        if ( (*buf == HTTP_CR) && (terminate) )
        {
            continue; /* ignore CR */
        }

        /* If this is a line feed, it is the end of the line of data.  Return
         * the data.
         */
        if (*buf == HTTP_LF)
        {
            /* If the caller wants the data as-is. */
            if (!terminate)
                buf++;

            break;    /* LF is the separator */
        }

        buf++;
    }

    /* Null-terminate the buffer if specified by the caller. */
    if (terminate)
        *buf = '\0';

    return (n);

} /* HTTP_Lite_Read_Line */

/************************************************************************
*
*   FUNCTION
*
*       HTTP_Lite_Read_Buffer
*
*   DESCRIPTION
*
*       Read data from socket descriptor. Retries reading until the
*       number of bytes requested is read. Returns the number of
*       bytes read. Negative if a read error (EOF) occurred
*       before the requested length.
*
*   INPUTS
*
*       *chunk_ptr              If the data is chunked, a pointer to a
*                               buffer into which to receive the chunk
*                               header data for parsing out the chunks.
*       max_rx_buf_len          The maximum number of bytes to read into
*                               chunk_ptr.
*       sd                      Socket descriptor to read from.
*       **buffer                A buffer pointer for which memory will be
*                               allocated and the reassembled data returned.
*       length                  Number of bytes to read if the
*                               content-length header is used instead of
*                               chunks.
*       *ssl_ptr                If the connection is secure, a pointer to
*                               the SSL data structure.
*       existing_len            This value indicates how much data
*                               already exists in the buffer.
*       *data_len (optional)    Input: This is the maximum size of the data buffer.
*                               Output: This is the number of bytes remaining on the socket.
*       *ptr (optional)         To optionally pass in an HTTP handle structure pointer.
*
*   OUTPUTS
*
*     Number of bytes read or zero if an error occurred.
*
************************************************************************/
UINT32 HTTP_Lite_Read_Buffer(CHAR *chunk_ptr, UINT32 max_rx_buf_len, INT sd,
                             CHAR **buffer, UINT32 length, VOID *ssl_ptr,
                             UINT32 existing_len, UINT32 *data_len, VOID *ptr)
{
    UINT32  n = 0;
    NU_HTTP_SH_S *handle = ptr;
    UINT8 partial = NU_FALSE;

    if (handle != NU_NULL)
    {
        partial = (handle->flag & HTTP_SH_FLAG_CLOSE) ? NU_TRUE : NU_FALSE;
    }

    /* If the content-length header was used, the total length of the data
     * is known.
     */
    if (length)
    {
        /* Allocate memory for the data. */
        if (NU_Reallocate_Memory(MEM_Cached, (VOID**)buffer, existing_len + length,
                               NU_SUSPEND) == NU_SUCCESS)
        {
            /* Receive the data. */
            n = HTTP_Lite_Receive(sd, &((*buffer)[existing_len]), length, ssl_ptr, partial);

            if ((n > 0) &&
                ((handle != NU_NULL) && ((handle->flag & HTTP_SH_FLAG_CHUNKED) == NU_TRUE)) &&
                    /* This call received the remainder of a chunk */
                (handle->num_remaining == n) &&
                    /* This was the end of the chunk. Check for error. */
                (memcmp(&((*buffer)[existing_len + n - strlen(HTTP_CRLF)]), HTTP_CRLF, strlen(HTTP_CRLF)) != 0))
            {
                /* Malformed packet. Indicate error and return. */
                handle->flag |= !HTTP_SH_FLAG_CHUNKED;

                /* If the entire chunk could not be received or there is no
                 * CRLF at the end of the data.
                 */
                n = 0;
            }

            if (n == 0)
            {
                /* If an error occurred, deallocate the memory for the buffer. */
                NU_Deallocate_Memory(*buffer);
            }
        }

        /* Clear data length */
        if (data_len != NU_NULL)
        {
            *data_len = 0;
        }
    }
    /* The transfer-encoding type is chunked; therefore, we must receive
     * each chunk separately.
     */
    else
    {
        /* Get the data.  This routine will allocate memory for buffer
         * accordingly.
         */
        n = HTTP_Lite_Get_Chunk_Data(chunk_ptr, buffer, sd, max_rx_buf_len, ssl_ptr, data_len);
    }

    return (n);

} /* HTTP_Lite_Read_Buffer */

/************************************************************************
*
*   FUNCTION
*
*       HTTP_Lite_Write_Buffer
*
*   DESCRIPTION
*
*       Writes a header and data to the other side of the connection
*       using a chunked header.
*
*   INPUTS
*
*       sd                      Socket over which to write.
*       *buffer                 Buffer containing the header and which
*                               will be used to build the chunk headers.
*       max_buf_len             The maximum bytes that will fit in
*                               buffer.
*       *data                   Pointer to the data to send.
*       data_len                The length of data to send.
*       *ssl                    If the connection is secure, a pointer
*                               to the SSL structure.
*
*   OUTPUTS
*
*     NU_SUCCESS or an operating system specific error if data could
*     not be sent.
*
************************************************************************/
STATUS HTTP_Lite_Write_Buffer(INT sd, CHAR *buffer, UINT32 max_buf_len,
                              CHAR *data, UINT32 data_len, VOID *ssl)
{
    STATUS  status = HTTP_ERROR_DATA_WRITE;
    UINT32  bytes_sent;
    UINT32  chunk_length;

    /* Send the header. */
    bytes_sent = HTTP_Lite_Write(sd, buffer, strlen(buffer), ssl);

    /* If the header was sent successfully. */
    if (bytes_sent > 0)
    {
        /* If there is data to send. */
        if (data)
        {
            /* Build the first chunk header. */
            chunk_length = HTTP_Lite_Put_Chunk_Data(buffer, max_buf_len, data_len);

            /* Send the first chunk header. */
            bytes_sent = HTTP_Lite_Write(sd, buffer, chunk_length, ssl);

            if (bytes_sent == chunk_length)
            {
                /* Send the data associated with the chunk. */
                bytes_sent = HTTP_Lite_Write(sd, data, data_len, ssl);

                if (bytes_sent == data_len)
                {
                    /* Send the CRLF to denote the end of this chunk. */
                    bytes_sent = HTTP_Lite_Write(sd, HTTP_CRLF, strlen(HTTP_CRLF), ssl);

                    if (bytes_sent == strlen(HTTP_CRLF))
                    {
                        /* Build the last chunk header. */
                        chunk_length = HTTP_Lite_Put_Chunk_Data(buffer, max_buf_len, 0);

                        /* Send the last chunk header. */
                        bytes_sent = HTTP_Lite_Write(sd, buffer, chunk_length, ssl);

                        if (bytes_sent == chunk_length)
                        {
                            status = NU_SUCCESS;
                        }
                    }
                }
            }
        }

        /* The header was sent successfully, and there is no data to send. */
        else
        {
            status = NU_SUCCESS;
        }
    }

    return (status);

} /* HTTP_Lite_Write_Buffer */

