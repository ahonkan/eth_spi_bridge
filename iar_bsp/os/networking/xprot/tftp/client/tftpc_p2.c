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
*       tftpc_p2.c
*
*   DESCRIPTION
*
*       This file contains the implementation of TFTPC_Put2.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       TFTPC_Put2
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_networking.h"


/*************************************************************************
*
*   FUNCTION
*
*       TFTPC_Put2
*
*   DESCRIPTION
*
*       This function is called from a user application.  It puts a file
*       to a TFTP server.
*
*   INPUTS
*
*       *remote_ip              Pointer to the target ip address.
*       *rpath                  Pointer to what the file will be called
*                               on the remote host.
*       *lpath                  Pointer to the local file to be transferred.
*       *ops                    Pointer to options the client requests of
*                               the server.
*       family                  The family - either ipv4 or ipv6
*
*   OUTPUTS
*
*       The number of bytes put to the server when successful.
*       Else various error codes from specified FILE system and
*       TFTPC specific errors:
*
*       TFTP_DISK_FULL
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
INT32 TFTPC_Put2(const UINT8 *remote_ip, const CHAR *rpath, CHAR *lpath,
                 TFTP_OPTIONS *ops, INT16 family)
{
    TFTPC_CB    *tftp_con = NU_NULL;
    STATUS      status;
    INT16       retransmits;
    INT32       bytes_sent;
    INT32       total_bytes, bytes_received;
    UINT32      file_size;
    INT         file_desc;
    INT32       original_location;

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
    /* Declare memory for the TFTP Control Block */
    CHAR tftp_control_memory[sizeof(TFTPC_CB)];

    /* Declare memory for the Receive Buffer */
    CHAR tftp_recv_memory[TFTP_BUFFER_SIZE_MIN + TFTP_HEADER_SIZE];

#if (TFTP_BLOCK_SIZE_MAX > TFTP_BLOCK_SIZE_DEFAULT)
    /*Declare memory for the Transmission Buffer*/
    CHAR tftp_trans_memory[TFTP_BLOCK_SIZE_MAX + TFTP_HEADER_SIZE];
#else
    /* Declare memory for the TFTP tx buffer if the block size is less than the default */
    CHAR tftp_trans_memory[TFTP_BLOCK_SIZE_DEFAULT + TFTP_HEADER_SIZE];
#endif
#endif

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Open the requested file - if an error occurs, return the error */
    file_desc = NU_Open(lpath, PO_RDWR|PO_BINARY, PS_IWRITE);

    if (file_desc < 0)
    {
        /* Return to user mode */
        NU_USER_MODE();

        return (file_desc);
    }

    /* Get the file size of the file */
    file_size = 0;

    /* Save off the current location of the file pointer - seek 0 bytes
     * from the current position.
     */
    original_location = NU_Seek(file_desc, 0, PSEEK_CUR);

    if (original_location >= 0)
    {
        /* Get the end location of the file pointer - seek to the end of
         * the file.
         */
        file_size = (UINT32)NU_Seek(file_desc, 0, PSEEK_END);

        /* Restore the original position of the file pointer - seek
         * original_location bytes from the beginning of the file.
         */
        NU_Seek(file_desc, original_location, PSEEK_SET);
    }


    /* Check that the requested options are valid */
    TFTPC_Set_Options(file_size, ops);

#if (INCLUDE_STATIC_BUILD == NU_FALSE)

    /* Allocate memory for the TFTP Control Block */
    status = NU_Allocate_Memory(MEM_Cached, (VOID **)&tftp_con,
                                sizeof(TFTPC_CB), NU_NO_SUSPEND);
#else

    if (ops->blksize > TFTP_BLOCK_SIZE_MAX)
        status = NU_NO_MEMORY;
    else
    {
        /* Assign memory to the tftp control block */
        tftp_con = (TFTPC_CB*)tftp_control_memory;

        /* Assign memory to the tftp transmission buffer */
        tftp_con->trans_buf = tftp_trans_memory;

        /* Set the length of the transmission buffer */
#if (TFTP_BLOCK_SIZE_MAX > TFTP_BLOCK_SIZE_DEFAULT)

        tftp_con->trans_buf_length =
            TFTP_BLOCK_SIZE_MAX + TFTP_HEADER_SIZE;

#else
        tftp_con->trans_buf_length =
            TFTP_BLOCK_SIZE_DEFAULT + TFTP_HEADER_SIZE;
#endif

        /* Assign memory to the tftp receive buffer */
        tftp_con->input_buf = tftp_recv_memory;

        /* Set the length of the receive buffer. */
        tftp_con->recv_buf_length =
            TFTP_BUFFER_SIZE_MIN + TFTP_HEADER_SIZE;

        status = NU_SUCCESS;
    }
#endif  /* INCLUDE_STATIC_BUILD == NU_FALSE*/

    /* If any of the memory allocations failed, close the file and return an error */
    if (status != NU_SUCCESS)
    {
        if (NU_Close(file_desc) != NU_SUCCESS)
            NLOG_Error_Log("Failed to close file", NERR_SEVERE,
                           __FILE__, __LINE__);

        /* Return to user mode */
        NU_USER_MODE();

        return (TFTP_NO_MEMORY);
    }

    /* Set the options that will be transmitted to the server in the request
     * packet
     */
    tftp_con->options.timeout = ops->timeout;
    tftp_con->options.blksize = ops->blksize;
    tftp_con->options.tsize = file_size;

    /* Setup the file descriptor, path and READ_TYPE */
    tftp_con->file_desc = file_desc;
    tftp_con->type = WRITE_TYPE;

    /* Send a write request to the server. */
    bytes_sent = TFTPC_Request(remote_ip, rpath, tftp_con, family);

    if (bytes_sent <= 0)
    {
        if (NU_Close(tftp_con->file_desc) != NU_SUCCESS)
            NLOG_Error_Log("Failed to close file", NERR_SEVERE,
                           __FILE__, __LINE__);

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
        if (NU_Deallocate_Memory((VOID*)tftp_con) != NU_SUCCESS)
            NLOG_Error_Log("Failed to deallocate memory for TFTP control block",
                           NERR_SEVERE, __FILE__, __LINE__);
#endif
        /* Return to user mode */
        NU_USER_MODE();

        return (TFTP_CON_FAILURE);
    }

#if (INCLUDE_STATIC_BUILD == NU_FALSE)

    /* The receive buffer needs to be as big as the initial request that
     * was sent in order to receive OACKs for all options and to
     * accommodate for the filename length.
     */
    tftp_con->recv_buf_length = (UINT16)bytes_sent;

    /* Allocate memory for the Receive Buffer.  This buffer will be
     * used only to receive ACK packets.
     */
    status = NU_Allocate_Memory(MEM_Cached, (VOID **)&tftp_con->input_buf,
                                tftp_con->recv_buf_length, NU_NO_SUSPEND);

    if (status != NU_SUCCESS)
    {
        if (NU_Close(tftp_con->file_desc) != NU_SUCCESS)
            NLOG_Error_Log("Failed to close file", NERR_SEVERE,
                           __FILE__, __LINE__);

        if (NU_Close_Socket(tftp_con->socket_number) != NU_SUCCESS)
            NLOG_Error_Log("Failed to close socket", NERR_SEVERE,
                           __FILE__, __LINE__);

        if (NU_Deallocate_Memory((VOID*)tftp_con->trans_buf) != NU_SUCCESS)
            NLOG_Error_Log("Failed to deallocate memory for TFTP control block",
                           NERR_SEVERE, __FILE__, __LINE__);

        if (NU_Deallocate_Memory((VOID*)tftp_con) != NU_SUCCESS)
            NLOG_Error_Log("Failed to deallocate memory for TFTP control block",
                           NERR_SEVERE, __FILE__, __LINE__);

        /* Return to user mode */
        NU_USER_MODE();

        return (TFTP_NO_MEMORY);
    }

#endif

    /* Initialize the retransmit counter. */
    retransmits = 0;

    /* Current State:
     *
     * client: WRQ --->
     *
     * The client just sent the Write Request (WRQ) and is now expecting the
     * server to send an ACK, OACK, or an ERROR packet.  If a valid
     * packet is not received within the timeout period, the client
     * will retransmit the Write Request (WRQ).
     */
    while ( (((bytes_received = TFTPC_Recv(tftp_con)) == NU_NO_DATA) ||
             ((GET16(tftp_con->input_buf, 0) != TFTP_OACK_OPCODE) &&
              (GET16(tftp_con->input_buf, 0) != TFTP_ACK_OPCODE) &&
              (GET16(tftp_con->input_buf, 0) != TFTP_ERROR_OPCODE))) &&
              (retransmits < TFTP_NUM_RETRANS) )
    {
        if (TFTPC_Retransmit(tftp_con, bytes_sent) <= 0)
            NLOG_Error_Log("Failed to retransmit TFTP packet", NERR_SEVERE,
                           __FILE__, __LINE__);
        retransmits++;
    }

    /* If we received a response, then set up the server's TID.
     * Else return an error.
     */
    if ( (bytes_received > 0) && (retransmits <= TFTP_NUM_RETRANS) )
        tftp_con->tid = tftp_con->server_addr.port;
    else
    {
        if (NU_Close(tftp_con->file_desc) != NU_SUCCESS)
            NLOG_Error_Log("Failed to close file", NERR_SEVERE,
                           __FILE__, __LINE__);

        if (NU_Close_Socket(tftp_con->socket_number) != NU_SUCCESS)
            NLOG_Error_Log("Failed to close socket", NERR_SEVERE,
                           __FILE__, __LINE__);

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
        if (NU_Deallocate_Memory((VOID*)tftp_con->input_buf) != NU_SUCCESS)
            NLOG_Error_Log("Failed to deallocate memory for receive buffer",
                           NERR_SEVERE, __FILE__, __LINE__);

        if (NU_Deallocate_Memory((VOID*)tftp_con->trans_buf) != NU_SUCCESS)
            NLOG_Error_Log("Failed to deallocate memory for transmission buffer",
                           NERR_SEVERE, __FILE__, __LINE__);

        if (NU_Deallocate_Memory((VOID*)tftp_con) != NU_SUCCESS)
            NLOG_Error_Log("Failed to deallocate memory for TFTP control block",
                           NERR_SEVERE, __FILE__, __LINE__);
#endif
        /* Return to user mode */
        NU_USER_MODE();

        return (TFTP_CON_FAILURE);
    }

    /* If we did not receive an OACK, the server is not RFC 2347 compliant;
     * therefore, we must set our values to conform to RFC 1350.
     */
    if (GET16(tftp_con->input_buf, 0) != TFTP_OACK_OPCODE)
    {
        tftp_con->options.blksize = TFTP_BLOCK_SIZE_DEFAULT;
        tftp_con->options.timeout = TFTP_TIMEOUT_DEFAULT;
    }

    /* Process the server's response (ACK or OACK) This will be the first
     * ACK or OACK processed; therefore, duplicates can be ignored.
     */
    status = TFTPC_Process_Ack(tftp_con, bytes_received);

    if (status != NU_SUCCESS)
        tftp_con->status = (INT16)status;

    /* The connection is now established.  While the status is ok
     * continue to transmit the remainder of the file.
     */
    while (tftp_con->status == TRANSFERRING_FILE)
    {
        /* If the packet received does not contain a duplicate ACK,
         * send the next data packet.
         */
        if (status == NU_SUCCESS)
        {
            /* Send a data packet. */
            bytes_sent = TFTPC_Send_Data(tftp_con);

            if (bytes_sent < 0)
            {
                status = TFTP_CON_FAILURE;
                break;
            }

            /* Initialize the retransmit counter. */
            retransmits = 0;
        }

        /* Current State:
         *
         * server: ACK --->
         *
         *                  <--- client: DATA
         *
         * The client just sent a DATA packet and is expecting either
         * an ACK or an ERROR packet from the server.  While a valid packet
         * is not received and the maximum number of retransmits has not
         * been sent, retransmit the DATA packet.
         */
        while ( (((bytes_received = TFTPC_Recv(tftp_con)) == NU_NO_DATA) ||
                 ((GET16(tftp_con->input_buf, 0) != TFTP_ACK_OPCODE) &&
                  (GET16(tftp_con->input_buf, 0) != TFTP_ERROR_OPCODE))) &&
                  (retransmits < TFTP_NUM_RETRANS) )
        {
            if (TFTPC_Retransmit(tftp_con, bytes_sent) <= 0)
                NLOG_Error_Log("Failed to retransmit TFTP packet", NERR_SEVERE,
                               __FILE__, __LINE__);
            retransmits++;
        }

        /* Process the ACK. */
        if ( (bytes_received > 0) && (retransmits <= TFTP_NUM_RETRANS) )
        {
            status = TFTPC_Process_Ack(tftp_con, bytes_received);

            if (status == TFTP_CON_FAILURE)
                tftp_con->status = (INT16)status;
        }

        /* Exit the loop and return an ERROR */
        else
        {
            status = TFTP_CON_FAILURE;
            tftp_con->status = TFTP_CON_FAILURE;
        }
    }  /* while transferring file. */

    /* Close the file */
    if (NU_Close(tftp_con->file_desc) != NU_SUCCESS)
        NLOG_Error_Log("Failed to close file", NERR_SEVERE,
                       __FILE__, __LINE__);

    /* If everything went okay, then calculate the number of bytes
     * that were sent. Else we should return the last error code that was
     * received.
     */
    if (status == NU_SUCCESS)
        total_bytes = (INT32)file_size;
    else
        total_bytes = status;

    /* Close the socket, freeing any resources that were used. */
    if (NU_Close_Socket(tftp_con->socket_number) != NU_SUCCESS)
        NLOG_Error_Log("Failed to close socket", NERR_SEVERE,
                       __FILE__, __LINE__);

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    /* Deallocate the memory allocated for the session. */
    if (NU_Deallocate_Memory((VOID*)tftp_con->input_buf) != NU_SUCCESS)
        NLOG_Error_Log("Failed to deallocate memory for receive buffer",
                       NERR_SEVERE, __FILE__, __LINE__);

    if (NU_Deallocate_Memory((VOID*)tftp_con->trans_buf) != NU_SUCCESS)
        NLOG_Error_Log("Failed to deallocate memory for transmission buffer",
                       NERR_SEVERE, __FILE__, __LINE__);

    if (NU_Deallocate_Memory((VOID*)tftp_con) != NU_SUCCESS)
        NLOG_Error_Log("Failed to deallocate memory for TFTP control block",
                       NERR_SEVERE, __FILE__, __LINE__);
#endif

    /* Return to user mode */
    NU_USER_MODE();

    return (total_bytes);

} /* TFTPC_Put2 */

