/*************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
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
*       tftpc.c
*
*   COMPONENT
*
*       TFTPC -- Trivial File Transfer Protocol (client)
*
*   DESCRIPTION
*
*       This file contains the TFTP routines necessary to get and put
*       a file to a TFTP server.
*
*   DATA STRUCTURES
*
*       tftpc_errors[]
*
*   FUNCTIONS
*
*       TFTPC_Request             Send a request to a TFTP server.
*       TFTPC_Recv                Recv a packet.
*       TFTPC_Process_Data        Process a data packet.
*       TFTPC_Ack                 Send a TFTP ack.
*       TFTPC_Process_Ack         Process an ack packet.
*       TFTPC_Send_Data           Send a TFTP data packet.
*       TFTPC_Retransmit          Retransmit the last TFTP packet.
*       TFTPC_Error               Send a TFTP error packet.
*       TFTPC_Check_Options       Evaluate server acknowledged options
*       TFTPC_Set_Options         Set options to be sent to the server.
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_networking.h"

/* TFTP defines 9 error codes, with values 0 through 8.  In the event that one
 * of these errors is received it will be passed back to the application.  Since
 * a value of 0 is defined to be NU_SUCCESS in the Nucleus PLUS OS it is
 * necessary to redefine the first error code, and since the convention at
 * Mentor Graphics is to return negative numbers for error codes all were
 * redefined. */
static const INT16  tftpc_errors[] = {TFTP_ERROR, TFTP_FILE_NFOUND, TFTP_ACCESS_VIOLATION,
                                      TFTP_DISK_FULL, TFTP_BAD_OPERATION, TFTP_UNKNOWN_TID,
                                      TFTP_FILE_EXISTS, TFTP_NO_SUCH_USER, TFTP_BAD_OPTION};



/*************************************************************************
*
*   FUNCTION
*
*       TFTPC_Request
*
*   DESCRIPTION
*
*       This function is responsible for sending a TFTP read request to
*       a TFTP server.
*
*   INPUTS
*
*       *remote_ip              Pointer to the server's ip address.
*       *remote_fname           Pointer to the remote file to read.
*       *tftp_con               Pointer to the TFTP Control Block
*       family                  The family - either ipv4 or ipv6
*
*   OUTPUTS
*
*       NU_SUCCESS
*       TFTP_CON_FAILURE
*       NU_NO_PORT_NUMBER
*       NU_NOT_CONNECTED
*       NU_INVALID_PARM
*       NU_WOULD_BLOCK
*       NU_INVALID_SOCKET
*       NU_INVALID_ADDRESS
*       NU_NO_DATA_TRANSFER
*       NU_DEVICE_DOWN
*
*************************************************************************/
STATUS TFTPC_Request(const UINT8 *remote_ip, const CHAR *remote_fname,
                     TFTPC_CB *tftp_con, INT16 family)
{
    INT         bytes_sent, send_size;
    CHAR        temp[10];
    STATUS      status;

    /* Create a socket. */
    tftp_con->socket_number = (INT16)(NU_Socket(family, NU_TYPE_DGRAM, 0));

    if (tftp_con->socket_number < 0)
        return (TFTP_CON_FAILURE);

    /* fill in a structure with the server address */
    tftp_con->server_addr.family    = family;
    tftp_con->server_addr.port      = 69;
    tftp_con->server_addr.name      = "tftp";

    /* Set the server's address */
#if (INCLUDE_IPV6 == NU_TRUE)

    if (family == NU_FAMILY_IP6)
        memcpy(tftp_con->server_addr.id.is_ip_addrs, remote_ip, IP6_ADDR_LEN);
    else
        memcpy(tftp_con->server_addr.id.is_ip_addrs, remote_ip, IP_ADDR_LEN);

#else
    memcpy(tftp_con->server_addr.id.is_ip_addrs, remote_ip, IP_ADDR_LEN);
#endif

    /* Initialize block_number (to 0 per RFC 2347) and transfer status. */
    tftp_con->block_number = 0;
    tftp_con->status = TRANSFERRING_FILE;

    /* First, compute the total length of the request buffer, so we
     * can allocate memory for it.
     */

    /* Calculate the length of the remote filename plus the NULL
     * terminator and "octet" type plus NULL terminator.
     */
    send_size = (INT)(strlen(remote_fname)) + 2 + (INT)strlen("octet") + 2;

    /* If a block size was specified by the application, set it in
     * the request packet.
     */
    if (tftp_con->options.blksize != TFTP_BLOCK_SIZE_DEFAULT)
    {
        NU_ITOA((INT)tftp_con->options.blksize, temp, 10);
        send_size += (INT)strlen("blksize") + (INT)strlen(temp) + 2;
    }

    /* If a timeout was specified by the application, set it in
     * the request packet.
     */
    if (tftp_con->options.timeout != 0)
    {
        NU_ITOA((INT)tftp_con->options.timeout, temp, 10);
        send_size += (INT)strlen("timeout") + (INT)strlen(temp) + 2;
    }

    /* If the tsize option was specified by the application, set it in
     * the request packet.
     */
    if ((tftp_con->options.tsize == 0) && (tftp_con->type == READ_TYPE))
    {
        send_size += (INT)strlen("tsize") + 3;
    }

    if ((tftp_con->options.tsize != 0) && (tftp_con->type == WRITE_TYPE))
    {
        NU_ULTOA(tftp_con->options.tsize, temp, 10);
        send_size += (INT)strlen("tsize") + (INT)strlen(temp) + 2;
    }

#if (INCLUDE_STATIC_BUILD == NU_FALSE)

    /* Allocate memory for the Transmission Buffer.  If this is a GET request,
     * this buffer will be used only to send the initial request and ACK
     * each corresponding DATA packet.
     */
    if (tftp_con->type == READ_TYPE)
    {
        tftp_con->trans_buf_length = (UINT16)(send_size + TFTP_HEADER_SIZE);

        status = NU_Allocate_Memory(MEM_Cached,
                                    (VOID**)&tftp_con->trans_buf,
                                    tftp_con->trans_buf_length,
                                    NU_NO_SUSPEND);
    }

    /* If this is a WRITE request, this buffer will be used to send the
     * initial request and each subsequent chunk of data from the file
     * being written. Make sure there is enough room in the buffer for
     * the initial request.  If the blocksize is less than the length
     * of the initial request, allocate more memory than the blocksize
     * requires.
     */
    else
    {
        /* If the blksize specified by the application is greater than
         * the length of the initial request packet.
         */
        if (tftp_con->options.blksize > (UINT16)send_size)
        {
            /* If the blocksize is greater than the default blocksize,
             * use the specified blocksize.
             */
            if (tftp_con->options.blksize > TFTP_BLOCK_SIZE_DEFAULT)
                tftp_con->trans_buf_length = tftp_con->options.blksize;

            /* Otherwise, use the default blocksize in case the server
             * does not support RFC 2347 and responds with a default
             * block size packet.
             */
            else
                tftp_con->trans_buf_length = TFTP_BLOCK_SIZE_DEFAULT;
        }

        /* Otherwise, the send_size is greater than the blocksize */
        else
        {
            /* Is the send_size is less than the default block size,
             * use the default blocksize in case the server does not
             * support RFC 2347 and responds with a default block size
             * packet.
             */
            if (send_size < TFTP_BLOCK_SIZE_DEFAULT)
                tftp_con->trans_buf_length = TFTP_BLOCK_SIZE_DEFAULT;

            /* Otherwise, use the send_size */
            else
                tftp_con->trans_buf_length = (UINT16)send_size;
        }

        /* Add bytes for the TFTP header */
        tftp_con->trans_buf_length += TFTP_HEADER_SIZE;

        status = NU_Allocate_Memory(MEM_Cached, (VOID **)&tftp_con->trans_buf,
                                    (UNSIGNED)(tftp_con->trans_buf_length),
                                    NU_NO_SUSPEND);
    }

#else

    /* The static memory allocated for this buffer is only TFTP_BUFFER_SIZE_MIN
     * bytes plus the TFTP header.  If the initial request exceeds this buffer,
     * return an error.
     */
    if ( ((tftp_con->type == READ_TYPE) && (send_size > TFTP_BUFFER_SIZE_MIN)) ||
#if (TFTP_BLOCK_SIZE_MAX > TFTP_BLOCK_SIZE_DEFAULT)
         ((tftp_con->type == WRITE_TYPE) && (send_size > TFTP_BLOCK_SIZE_MAX)) )
#else
         ((tftp_con->type == WRITE_TYPE) && (send_size > TFTP_BLOCK_SIZE_DEFAULT)) )
#endif
    {
        status = TFTP_NO_MEMORY;
    }

    else
        status = NU_SUCCESS;

#endif

    if (status == NU_SUCCESS)
    {
        /* Indicate whether this is a READ or WRITE operation */
        if (tftp_con->type == READ_TYPE)
            PUT16(tftp_con->trans_buf, 0, TFTP_RRQ_OPCODE);
        else
            PUT16(tftp_con->trans_buf, 0, TFTP_WRQ_OPCODE);

        /* Fill in the file name and mode - both null terminated */
        strcpy(&(tftp_con->trans_buf[2]), remote_fname);
        send_size = (INT)(strlen(remote_fname)) + 2;
        tftp_con->trans_buf[send_size++] = 0;

        strcpy(&(tftp_con->trans_buf[send_size]), "octet");
        send_size += (INT)strlen("octet");
        tftp_con->trans_buf[send_size++] = 0;

        /* Check if there is a blksize, timeout or tsize option indicated, if so,
         * append the option name and option value to the end of the packet -
         * all null terminated
         */
        if (tftp_con->options.blksize != TFTP_BLOCK_SIZE_DEFAULT)
        {
            strcpy(&(tftp_con->trans_buf[send_size]), "blksize");
            send_size = (INT)strlen("blksize") + send_size;
            tftp_con->trans_buf[send_size++] = 0;

            strcpy(&(tftp_con->trans_buf[send_size]),
                   (CHAR *)(NU_ITOA((INT)tftp_con->options.blksize, temp, 10)) );
            send_size += (INT)strlen(temp);
            tftp_con->trans_buf[send_size++] = 0;
        }

        if (tftp_con->options.timeout != 0)
        {
            strcpy(&(tftp_con->trans_buf[send_size]), "timeout");
            send_size = (INT)strlen("timeout") + send_size;
            tftp_con->trans_buf[send_size++] = 0;

            strcpy(&(tftp_con->trans_buf[send_size]),
                   (CHAR *)(NU_ITOA((INT)tftp_con->options.timeout, temp, 10)) );
            send_size += (INT)strlen(temp);
            tftp_con->trans_buf[send_size++] = 0;
        }

        /* If the user set the timeout to 0, do not send a timeout option in
         * the request, but do use the default timeout as the internal
         * timeout value.  The server side will use its default too.
         */
        else
            tftp_con->options.timeout = TFTP_TIMEOUT_DEFAULT;

        if ((tftp_con->options.tsize == 0) && (tftp_con->type == READ_TYPE))
        {
            strcpy(&(tftp_con->trans_buf[send_size]), "tsize");
            send_size = (INT)strlen("tsize") + send_size;
            tftp_con->trans_buf[send_size++] = 0;
            tftp_con->trans_buf[send_size++] = 0;
            tftp_con->trans_buf[send_size++] = 0;
        }

        if ((tftp_con->options.tsize != 0) && (tftp_con->type == WRITE_TYPE))
        {
            strcpy(&(tftp_con->trans_buf[send_size]), "tsize");
            send_size = (INT)strlen("tsize") + send_size;
            tftp_con->trans_buf[send_size++] = 0;

            strcpy(&(tftp_con->trans_buf[send_size]),
                   (CHAR *)(NU_ULTOA(tftp_con->options.tsize, temp, 10)) );
            send_size += (INT)strlen(temp);
            tftp_con->trans_buf[send_size++] = 0;
        }

        /* Send the request. */
        bytes_sent = (INT)(NU_Send_To(tftp_con->socket_number,
                                        tftp_con->trans_buf, (UINT16)(send_size),
                                        0, &tftp_con->server_addr, 0));

        /* If the packet could not be sent, clean up all resources
         * allocated by this routine.
         */
        if (bytes_sent <= 0)
        {
            /* Close the socket. */
            if (NU_Close_Socket(tftp_con->socket_number) != NU_SUCCESS)
                NLOG_Error_Log("Failed to close socket", NERR_SEVERE,
                               __FILE__, __LINE__);

#if (INCLUDE_STATIC_BUILD == NU_FALSE)

            /* Deallocate the memory allocated for the transmission
             * buffer.
             */
            if (NU_Deallocate_Memory(tftp_con->trans_buf) != NU_SUCCESS)
                NLOG_Error_Log("Failed to deallocate memory for the TFTP request buffer",
                               NERR_SEVERE, __FILE__, __LINE__);
#endif
        }
    }

    /* Otherwise, return the memory allocation error */
    else
    {
        /* Close the socket. */
        if (NU_Close_Socket(tftp_con->socket_number) != NU_SUCCESS)
            NLOG_Error_Log("Failed to close socket", NERR_SEVERE,
                           __FILE__, __LINE__);

        bytes_sent = TFTP_NO_MEMORY;
    }

    return (bytes_sent);

} /* TFTPC_Request */

/*************************************************************************
*
*   FUNCTION
*
*       TFTPC_Recv
*
*   DESCRIPTION
*
*       This function is responsible for receiving data from a TFTP
*       server.  NU_Select is used to timeout.
*
*   INPUTS
*
*       *tftp_con               Pointer to the TFTP Control Block
*
*   OUTPUTS
*
*       The number of bytes received when successful.
*       NU_NO_DATA  when NU_Select fails to find a data ready socket.
*
*************************************************************************/
INT32 TFTPC_Recv(TFTPC_CB *tftp_con)
{
    FD_SET      readfs;
    INT32       bytes_received;
    INT16       status, clilen;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Do a select on this socket.  In the case that the foreign port
     * fails to respond we don't want to suspend on receive forever.
     */
    NU_FD_Init(&readfs);
    NU_FD_Set(tftp_con->socket_number, &readfs);

    status = (INT16)(NU_Select(NSOCKETS, &readfs, NU_NULL, NU_NULL,
                               (tftp_con->options.timeout *
                                SCK_Ticks_Per_Second)));

    if (status != NU_SUCCESS)
    {
        /* Return to user mode */
        NU_USER_MODE();

        return (status);
    }

    /*  We must have received something.  Go get the server's response. */
    bytes_received = (INT32)(NU_Recv_From(tftp_con->socket_number,
                                          tftp_con->input_buf,
                                          tftp_con->recv_buf_length, 0,
                                          &tftp_con->server_addr, &clilen));

    /* Return to user mode */
    NU_USER_MODE();

    return (bytes_received);

} /* TFTPC_Recv */

/*************************************************************************
*
*   FUNCTION
*
*       TFTPC_Process_Data
*
*   DESCRIPTION
*
*       This function is responsible for processing a data packet
*       whenever a read request is in progress.
*
*   INPUTS
*
*       *tftp_con               Pointer to the TFTP Control Block.
*       *bytes_received         Number of bytes in the packet.
*
*   OUTPUTS
*
*       NU_SUCCESS              whenever the expected data was received,
*                               otherwise the error messages from the
*                               specified FILE system used.
*       TFTP_BAD_OPTION         A bad option was specified by the other
*                               side of the connection.
*       TFTP_DUPLICATE_DATA     Duplicate data was received.
*
*************************************************************************/
STATUS TFTPC_Process_Data(TFTPC_CB *tftp_con, INT32 bytes_received)
{
    UINT16  data_size, error_code;
    INT32   bytes = 0;
    STATUS  status;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* What kind of packet is this. */
    switch(GET16(tftp_con->input_buf, 0))
    {
    case TFTP_OACK_OPCODE:

        /* Check that the returned options are valid */
        if (TFTPC_Check_Options(tftp_con, bytes_received) != NU_SUCCESS)
            return (TFTP_BAD_OPTION);

        /* Acknowledge that we received the OACK */
        TFTPC_Ack(tftp_con);

        /* Increment the block number */
        tftp_con->block_number++;

        break;

    case TFTP_DATA_OPCODE:

        /* Received a DATA packet that has already be acknowledged,
         * because our current block number is greater than the block
         * number of the packet - we do not want to exit, error, or
         * confirm this packet (because it has already been confirmed),
         * but go get the next packet
         */
        if ( (tftp_con->block_number > GET16(tftp_con->input_buf, 2))
             && (tftp_con->tid == tftp_con->server_addr.port) )
            return (TFTP_DUPLICATE_DATA);

        /* If data was received make sure block number and TID are
         * correct.
         */
        if ( (tftp_con->block_number == GET16(tftp_con->input_buf, 2))
             && (tftp_con->tid == tftp_con->server_addr.port) )
        {
            /* Calculate the amount of data in this packet. */
            data_size = (UINT16)(bytes_received - TFTP_HEADER_SIZE);

            if (data_size > 0)
            {
                /* Write the data to the file */
                bytes = NU_Write(tftp_con->file_desc, &(tftp_con->input_buf[4]), data_size);

                if (bytes <= 0)
                {
                    /* Return to user mode */
                    NU_USER_MODE();

                    return ((STATUS)bytes);
                }
            }

            /* If blksize bytes of data were copied, send an ACK.  We know
             * the other side will send at least one more data packet
             * and that all data in the current packet was accepted.
             */
            if (bytes == (INT32) tftp_con->options.blksize)
            {
                TFTPC_Ack(tftp_con);
            }

            /* Else if less data was copied than was received, we have
             * filled the user's buffer and can accept no more data.
             * Send an error condition indicating that no more data can
             * be accepted.
             */
            else if (bytes < (bytes_received - TFTP_HEADER_SIZE))
            {
                tftp_con->status = TRANSFER_COMPLETE;
                TFTPC_Error(tftp_con, 3, "Buffer Full. ");
            }

            /* Else the last data packet has been received.
             * We are done. Send the last ACK.
             */
            else
            {
                tftp_con->status = TRANSFER_COMPLETE;
                TFTPC_Ack(tftp_con);
            }

            /* Increment the block number. */
            tftp_con->block_number++;
        }
        else
        {
            /* Return to user mode */
            NU_USER_MODE();

            return(TFTP_CON_FAILURE);
        }
        break;

    case TFTP_ERROR_OPCODE:

        if (GET16(tftp_con->input_buf, 2) <= 8)
        {
            /* Extract the error code from the packet. */
            error_code = GET16(tftp_con->input_buf, 2);

            /* If the error code is valid, return it to the upper layer. */
            if (error_code < TFTPC_ERROR_CODE_COUNT)
                status = tftpc_errors[error_code];

            /* Otherwise, return an error. */
            else
                status = TFTP_CON_FAILURE;

            /* Return to user mode */
            NU_USER_MODE();

            return (status);
        }
        else
        {
            /* Return to user mode */
            NU_USER_MODE();

            return(TFTP_CON_FAILURE);
        }

    case TFTP_ACK_OPCODE:
    case TFTP_RRQ_OPCODE:
    case TFTP_WRQ_OPCODE:
    default:
        {
            /* Return to user mode */
            NU_USER_MODE();

            return (TFTP_CON_FAILURE);
        }
    }

    /* Return to user mode */
    NU_USER_MODE();

    return (NU_SUCCESS);

} /* TFTPC_Process_Data */

/*************************************************************************
*
*   FUNCTION
*
*       TFTPC_Ack
*
*   DESCRIPTION
*
*       This function is responsible for sending an acknowledgement of
*       a TFTP data packet.
*
*   INPUTS
*
*       *tftp_con               Pointer to the TFTP Control Block
*
*   OUTPUTS
*
*       The Number of bytes sent on success.
*
*************************************************************************/
STATUS TFTPC_Ack(const TFTPC_CB *tftp_con)
{
    /* Setup the ACK packet - a client always returns an ACK packet,
     * regardless of the presence of options - only servers return
     * OACK packets
     */
    PUT16(tftp_con->trans_buf, 0, TFTP_ACK_OPCODE);
    PUT16(tftp_con->trans_buf, 2, tftp_con->block_number);

    /* Send the ACK packet. */
    return ((INT)NU_Send_To(tftp_con->socket_number,
                            tftp_con->trans_buf, 4, 0,
                            &tftp_con->server_addr, 0));

} /* TFTPC_Ack */

/*************************************************************************
*
*   FUNCTION
*
*       TFTPC_Process_Ack
*
*   DESCRIPTION
*
*       This function is responsible processing an ACK packet whenever
*       a write request is in progress.
*
*   INPUTS
*
*       *tftp_con               Pointer to the TFTP Control Block.
*       bytes_received          The number of bytes received in the last
*                               packet received.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       TFTP_CON_FAILURE
*       TFTP_ERROR
*       TFTP_FILE_NFOUND
*       TFTP_ACCESS_VIOLATION
*       TFTP_DISK_FULL
*       TFTP_BAD_OPERATION
*       TFTP_UNKNOWN_TID
*       TFTP_FILE_EXISTS
*       TFTP_NO_SUCH_USER
*       TFTP_BAD_OPTION
*
*************************************************************************/
STATUS TFTPC_Process_Ack(TFTPC_CB *tftp_con, INT32 bytes_received)
{
    STATUS  status;
    UINT16  error_code;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* What kind of packet is this. */
    switch(GET16(tftp_con->input_buf, 0))
    {
    case TFTP_OACK_OPCODE:

        /* Received an OACK packet that has already be acknowledged,
         * because our block number is greater than 0, and we only
         * receive one OACK when our block number is equal to 0
         */
        if ( (tftp_con->block_number > 0) &&
             (tftp_con->tid == tftp_con->server_addr.port) )
            return (TFTP_DUPLICATE_ACK);

        /* Check that the options returned are valid */
        if ((status = TFTPC_Check_Options(tftp_con, bytes_received)) != NU_SUCCESS)
            break;

        else
        {
            /* Increment block number */
            tftp_con->block_number++;
            status = NU_SUCCESS;
        }

        break;

    case TFTP_ACK_OPCODE:

        /* Received an ACK packet that has already be acknowledged,
         * because our current block number is greater than the block
         * number of the packet - we do not want to exit, error, or
         * confirm this packet (because it has already been confirmed),
         * but go get the next packet
         */
        if ( (tftp_con->block_number > GET16(tftp_con->input_buf, 2)) &&
             (tftp_con->tid == tftp_con->server_addr.port) )
            return (TFTP_DUPLICATE_ACK);

        /* Make sure the block number and TID are correct. */
        if ( (tftp_con->block_number == GET16(tftp_con->input_buf, 2)) &&
             (tftp_con->tid == tftp_con->server_addr.port) )
            tftp_con->block_number++;

        else
        {
            status = TFTP_CON_FAILURE;
            break;
        }

        status = NU_SUCCESS;
        break;

    case TFTP_ERROR_OPCODE:

        if (GET16(tftp_con->input_buf, 2) <= 8)
        {
            /* Extract the error code from the packet. */
            error_code = GET16(tftp_con->input_buf, 2);

            /* Ensure the error code is valid. */
            if (error_code < TFTPC_ERROR_CODE_COUNT)
                status = (tftpc_errors[error_code]);

            /* Otherwise, return an error. */
            else
                status = TFTP_CON_FAILURE;
        }

        else
            status = TFTP_CON_FAILURE;

        break;

    case TFTP_RRQ_OPCODE:
    case TFTP_WRQ_OPCODE:
    case TFTP_DATA_OPCODE:
    default:
        status = TFTP_CON_FAILURE;
        break;
    }

    /* Return to user mode */
    NU_USER_MODE();

    return (status);

} /* TFTPC_Process_Ack */

/*************************************************************************
*
*   FUNCTION
*
*       TFTPC_Send_Data
*
*   DESCRIPTION
*
*       This function is responsible for sending an acknowledgement of
*       a TFTP data packet.  The data is copied from the user's buffer
*       into the TFTP CB send buffer.  This function also updates the
*       pointer into the user's buffer and the number of bytes left
*       to send in the buffer.
*
*   INPUTS
*
*       *tftp_con               Pointer to the TFTP Control Block
*
*   OUTPUTS
*
*       The Number of bytes sent on success.
*
*************************************************************************/
INT32 TFTPC_Send_Data(TFTPC_CB *tftp_con)
{
    UINT16 num_bytes;
    INT32 bytes_read;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Fill in the opcode and block number. */
    PUT16(tftp_con->trans_buf, 0, TFTP_DATA_OPCODE);
    PUT16(tftp_con->trans_buf, 2, tftp_con->block_number);

    /* Determine the number of bytes that will be sent in this packet - if
     * we have blksize or more bytes to send, we will send blksize bytes,
     * else, we will send the remainder of the file as our last packet
     */
    bytes_read = NU_Read(tftp_con->file_desc, &(tftp_con->trans_buf[4]), tftp_con->options.blksize);

    if(bytes_read < 0)
        bytes_read = 0;

    if (bytes_read == tftp_con->options.blksize)
        num_bytes = tftp_con->options.blksize;
    else
        num_bytes = (UINT16)bytes_read;

    /* If we sent less than blksize bytes, we know it was the last packet */
    if (num_bytes < tftp_con->options.blksize)
        tftp_con->status = TRANSFER_COMPLETE;

    /* Return to user mode */
    NU_USER_MODE();

    /* Send the data. */
    return (INT32)(NU_Send_To(tftp_con->socket_number,
                              tftp_con->trans_buf,
                              (UINT16)(num_bytes + TFTP_HEADER_SIZE), 0,
                              &tftp_con->server_addr, 0));

} /* TFTPC_Send_Data */

/*************************************************************************
*
*   FUNCTION
*
*       TFTPC_Retransmit
*
*   DESCRIPTION
*
*       This function will retransmit the last packet sent.
*
*   INPUTS
*
*       *tftp_con               The pointer to TFTP Control Block.
*       nbytes                  The number of bytes to retransmit.
*
*   OUTPUTS
*
*       The Number of bytes sent on success.
*
*************************************************************************/
STATUS TFTPC_Retransmit(const TFTPC_CB *tftp_con, INT32 nbytes)
{
    return((INT)NU_Send_To(tftp_con->socket_number, tftp_con->trans_buf,
                           (UINT16)nbytes, 0, &tftp_con->server_addr, 0));

} /* TFTPC_Retransmit */

/*************************************************************************
*
*   FUNCTION
*
*       TFTPC_Error
*
*   DESCRIPTION
*
*       This function will send an error packet.
*
*   INPUTS
*
*       *tftp_con               Pointer to the TFTP Control Block.
*       error_code              The TFTP error code.
*       *err_string             Pointer to the error message to send.
*
*   OUTPUTS
*
*       The Number of bytes sent on success.
*
*************************************************************************/
STATUS TFTPC_Error(const TFTPC_CB *tftp_con, INT16 error_code,
                   const char *err_string)
{
    UINT16      bytes_sent, send_size;

    /* Size equals the length of the error string plus the null terminator. */
    send_size = (UINT16)((strlen(err_string) + 1) + TFTP_HEADER_SIZE);

    /* Check that the data for the error message will fit in the
     * allocated transmission buffer.
     */
    if (send_size <= tftp_con->trans_buf_length)
    {
        /* Fill in the opcode and block number. */
        PUT16(tftp_con->trans_buf, 0, TFTP_ERROR_OPCODE);
        PUT16(tftp_con->trans_buf, 2, (UINT16)error_code);

        strcpy(&tftp_con->trans_buf[4], err_string);

        /* Send the datagram. */
        bytes_sent = (INT16)(NU_Send_To(tftp_con->socket_number,
                                        tftp_con->trans_buf, send_size, 0,
                                        &tftp_con->server_addr, 0));
    }

    /* Otherwise, return no bytes sent */
    else
        bytes_sent = 0;

    return (bytes_sent);

} /* TFTPC_Error */

/*************************************************************************
*
*   FUNCTION
*
*       TFTPC_Check_Options
*
*   DESCRIPTION
*
*       This function compares the options sent by the client with those
*       returned from the server to be sure they are valid.
*
*   INPUTS
*
*       *tftp_con               Pointer to the TFTP Control Block.
*       bytes_received          The bytes received in the OACK packet from
*                               the server.
*
*   OUTPUTS
*
*       TFTP_BAD_OPTION,
*       TFTP_DISK_FULL
*       NU_SUCCESS
*
*************************************************************************/
STATUS TFTPC_Check_Options(TFTPC_CB *tftp_con, INT32 bytes_received)
{
    INT16   count = 2, count1;
    char    temp1[TFTP_PARSING_LENGTH];
    char    temp2[TFTP_PARSING_LENGTH];
    char    *op_holder;
    char    *value_holder;
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Set each acknowledgement field initially to false - we must track
     * acknowledged options only, and if an option was requested and
     * not acknowledged, we must reset its value to the default for
     * that option
     */
    tftp_con->options.timeout_acknowledged = NU_FALSE;
    tftp_con->options.blksize_acknowledged = NU_FALSE;
    tftp_con->options.tsize_acknowledged   = NU_FALSE;

    /* Check if the server changed our requested option values */
    while (count < bytes_received)
    {
        count1 = 0;

        /* Parse the first option from the transmission buffer */
        while (tftp_con->input_buf[count] != '\0')
        {
            temp1[count1] = tftp_con->input_buf[count];
            count ++;
            count1 ++;
        }

        /* Null terminate temp1 */
        temp1[count1] = '\0';
        op_holder = temp1;

        count++;
        count1 = 0;

        /* Parse the first value from the transmission buffer */
        while (tftp_con->input_buf[count] != '\0')
        {
            temp2[count1] = tftp_con->input_buf[count];
            count ++;
            count1 ++;
        }

        /* Null terminate temp2 */
        temp2[count1] = '\0';
        value_holder = temp2;

        count++;

        /* The server has the authority to change the value of some
         * options we specified; check each returned option's value
         * and determine if any difference is valid
         */
        if (strcmp(op_holder, "timeout") == 0)
        {
            /* The server may not change our timeout value */
            if ((UINT16)NU_ATOI(value_holder) != tftp_con->options.timeout)
            {
                /* Return to user mode */
                NU_USER_MODE();

                return(TFTP_BAD_OPTION);
            }

            /* The server acknowledged our timeout option */
            tftp_con->options.timeout_acknowledged = NU_TRUE;
        }

        else if (strcmp(op_holder, "tsize") == 0)
        {
            CHAR    drive[3];
            UINT32  free_clusters[1];
            UINT32  total_clusters[1];
            UINT8   sectors_per_cluster[1];
            UINT16  bytes_per_sector[1];
            UINT32  disk_space;

            drive[0] = (CHAR)('A' + NU_Get_Default_Drive());
            drive[1] = ':';
            drive[2] = '\0';

            disk_space = 0;
            if (NU_FreeSpace (drive, (UINT8*)sectors_per_cluster,
                 (UINT16*)bytes_per_sector, free_clusters, total_clusters) == NU_SUCCESS)
            {
                disk_space = (*sectors_per_cluster) * (*bytes_per_sector) * (*free_clusters);
            }

            /* If the file is bigger than we can hold, error */
            if ((UINT32)NU_ATOI(value_holder) > disk_space)
            {
                /* Return to user mode */
                NU_USER_MODE();

                return(TFTP_DISK_FULL);
            }
            else
            {
                tftp_con->options.tsize = (UINT32)(NU_ATOI(value_holder));
                tftp_con->options.tsize_acknowledged = NU_TRUE;
            }
        }

        else if (strcmp(op_holder, "blksize") == 0)
        {
            /* The server may not return a blksize larger than we
             * requested
             */
            if (((UINT16)NU_ATOI(value_holder) > tftp_con->options.blksize) ||
                ((UINT16)NU_ATOI(value_holder) == 0))
            {
                /* Return to user mode */
                NU_USER_MODE();

                return(TFTP_BAD_OPTION);
            }

            /* The server may return a blksize smaller than we
             * requested; therefore, set our blksize to that value
             */
            else if ((UINT16)NU_ATOI(value_holder) < tftp_con->options.blksize)
                tftp_con->options.blksize = (UINT16)(NU_ATOI(value_holder));

            /* The server acknowledged our blksize option */
            tftp_con->options.blksize_acknowledged = NU_TRUE;
        }

        /* Otherwise, the server is sending us unsupported options */
        else
            TFTPC_Error(tftp_con, 8, "Error: Option Not Supported. ");
    }

    /* We may use only those options that the server acknowledges - if
     * the server did not acknowledge an option, set it back to the default
     */
    if (tftp_con->options.timeout_acknowledged != NU_TRUE)
        tftp_con->options.timeout = TFTP_TIMEOUT_DEFAULT;

    if (tftp_con->options.blksize_acknowledged != NU_TRUE)
        tftp_con->options.blksize = TFTP_BLOCK_SIZE_DEFAULT;

    /* Return to user mode */
    NU_USER_MODE();

    return(NU_SUCCESS);

} /* TFTPC_Check_Options */

/*************************************************************************
*
*   FUNCTION
*
*       TFTPC_Set_Options
*
*   DESCRIPTION
*
*       This function verifies the validity of the blksize and timeout
*       options requested - if they are invalid, it sets them to the
*       valid defaults.
*
*   INPUTS
*
*       buf_size                Size of the user buffer.
*       *ops                    Pointer to the options structure.
*
*   OUTPUTS
*
*       NU_SUCCESS
*
*************************************************************************/
STATUS TFTPC_Set_Options(UINT32 buf_size, TFTP_OPTIONS *ops)
{
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* If the user did not specify a blksize, set it to the default
     * Also, if the user passes in a value larger than a UINT16 for
     * blksize, sets blksize to default.  Or, if the user specifies a
     * value larger than the max, set it to the default
     */
    if ((ops->blksize < TFTP_BLOCK_SIZE_MIN) ||
        (ops->blksize > TFTP_BLOCK_SIZE_MAX))
        ops->blksize = TFTP_BLOCK_SIZE_DEFAULT;

    /* If the blksize is greater than the transmission size, set it
     * to the blksize - it is wasteful to allocate more memory for a
     * transmission buffer than we need
     */
    if ((ops->blksize > buf_size) && (buf_size != 0))
        ops->blksize = (UINT16)buf_size;

    /* Per RFC 2349, the timeout interval must be between 1 and 255 - if
     * the timeout is 0, then the user did not specify a timeout, and we
     * want the server to use its default and we will use our default;
     * therefore, leave it 0 for now.
     */
    if (ops->timeout > 255)
        ops->timeout = TFTP_TIMEOUT_DEFAULT;

    /* Return to user mode */
    NU_USER_MODE();

    return (NU_SUCCESS);

} /* TFTPC_Set_Options */

