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
* FILE NAME
*
*       tftpc_g2.c
*
* DESCRIPTION
*
*       This file contains the implementation of TFTPC_Get2.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       TFTPC_Get2
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_networking.h"


/*************************************************************************
*
*   FUNCTION
*
*       TFTPC_Get2
*
*   DESCRIPTION
*
*       This function is called from a user application.  It retrieves a
*       file from a TFTP server - compatible with RFC 2347 compliant and
*       non-compliant servers.
*
*   INPUTS
*
*       *remote_ip              Pointer to the target ip address.
*       *rpath                  Pointer to the remote file to get.
*       *lpath                  Pointer to the local name to give the
*                               file.
*       *ops                    Pointer to options the user requests of
*                               the server.
*       family                  The family - IPv4 or IPv6
*
*   OUTPUTS
*
*       The number of bytes received when successful.
*       Else various error codes from specified FILE system, NET API
*       errors, and TFTPC specific errors:
*
*       TFTP_NO_MEMORY
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
INT32 TFTPC_Get2(const UINT8 *remote_ip, const CHAR *rpath, CHAR *lpath,
                 TFTP_OPTIONS *ops, INT16 family)
{
    TFTPC_CB    *tftp_con = NU_NULL;
    STATUS      status;
    INT16       retransmits;
    INT32       total_bytes, bytes_received;
    UINT32      file_size = 0;
    INT         file_desc;
    INT32       original_location;

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
    /* Declare memory for the TFTP Control Block */
    CHAR tftp_control_memory[sizeof(TFTPC_CB)];

    /* Declare memory for the Transmission Buffer */
    CHAR tftp_trans_memory[TFTP_BUFFER_SIZE_MIN + TFTP_HEADER_SIZE];

#if (TFTP_BLOCK_SIZE_MAX > TFTP_BLOCK_SIZE_DEFAULT)
    /* Declare memory for the Receive Buffer */
    CHAR tftp_recv_memory[TFTP_BLOCK_SIZE_MAX + TFTP_HEADER_SIZE];
#else
    /* Declare memory for the Receive buffer if the block size is less
     * than the default
     */
    CHAR tftp_recv_memory[TFTP_BLOCK_SIZE_DEFAULT + TFTP_HEADER_SIZE];
#endif
#endif

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Open the file and get the length for the tsize option */
    file_desc = NU_Open(lpath, (PO_RDWR | PO_CREAT | PO_TRUNC | PO_BINARY),
                         PS_IWRITE);

    if (file_desc < 0)
    {
        /* Return to user mode */
        NU_USER_MODE();

        return (file_desc);
    }

    /* Check that the requested options are valid */
    TFTPC_Set_Options(file_size, ops);

#if (INCLUDE_STATIC_BUILD == NU_FALSE)

    /* Allocate memory for the TFTP Control Block */
    status = NU_Allocate_Memory(MEM_Cached, (VOID**)&tftp_con,
                                sizeof(TFTPC_CB), NU_NO_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* If the blocksize is less than the RFC default block size,
         * allocate TFTP_BLOCK_SIZE_DEFAULT in case the other side is
         * not RFC 2347 compliant and responds with a
         * TFTP_BLOCK_SIZE_DEFAULT sized packet.  Do not change the
         * value of the actual blocksize now.  This is just a
         * precaution for receiving the ACK of the request.  The
         * blocksize will be changed later if the server does not
         * support options.
         */
        if (ops->blksize < TFTP_BLOCK_SIZE_DEFAULT)
            tftp_con->recv_buf_length =
                TFTP_BLOCK_SIZE_DEFAULT + TFTP_HEADER_SIZE;
        else
            tftp_con->recv_buf_length = ops->blksize + TFTP_HEADER_SIZE;

        /* Allocate memory for the Receive Buffer. */
        status = NU_Allocate_Memory(MEM_Cached, (VOID**)&tftp_con->input_buf,
                                    (UNSIGNED)tftp_con->recv_buf_length,
                                    NU_NO_SUSPEND);

        if (status != NU_SUCCESS)
        {
            /* Deallocate the memory allocated for the control block and the
             * transmission buffer.
             */
            if (NU_Deallocate_Memory((VOID*)tftp_con) != NU_SUCCESS)
                NLOG_Error_Log("Failed to deallocate memory for the TFTP control block",
                               NERR_SEVERE, __FILE__, __LINE__);
        }
    }

#else

    if (ops->blksize > TFTP_BLOCK_SIZE_MAX)
        status = NU_NO_MEMORY;
    else
    {
        /* Assign memory to the tftp control block */
        tftp_con = (TFTPC_CB*)tftp_control_memory;

        /* Assign memory to the transmission buffer */
        tftp_con->trans_buf = tftp_trans_memory;

        /* Set the length of the transmission buffer */
        tftp_con->trans_buf_length =
            TFTP_BUFFER_SIZE_MIN + TFTP_HEADER_SIZE;

        /* Assign memory to the tftp receive buffer */
        tftp_con->input_buf = tftp_recv_memory;

        /* Set the length of the receive buffer */
#if (TFTP_BLOCK_SIZE_MAX > TFTP_BLOCK_SIZE_DEFAULT)

        tftp_con->recv_buf_length =
            TFTP_BLOCK_SIZE_MAX + TFTP_HEADER_SIZE;

#else
        tftp_con->recv_buf_length =
            TFTP_BLOCK_SIZE_DEFAULT + TFTP_HEADER_SIZE;
#endif

        status = NU_SUCCESS;
    }

#endif      /* INCLUDE_STATIC_BUILD */

    /* If any memory allocations failed, close the file and return an error */
    if (status != NU_SUCCESS)
    {
        if (NU_Close(file_desc) != NU_SUCCESS)
            NLOG_Error_Log("Failed to close file", NERR_SEVERE,
                           __FILE__, __LINE__);

        /* Return to user mode */
        NU_USER_MODE();

        return (TFTP_NO_MEMORY);
    }

    /* Set the options that will be requested of the server - the tsize is
     * sent as 0, and the server will return the size of the request file
     */
    tftp_con->options.timeout = ops->timeout;
    tftp_con->options.blksize = ops->blksize;
    tftp_con->options.tsize = 0;

    /* Setup the file descriptor, path and READ_TYPE */
    tftp_con->file_desc = file_desc;
    tftp_con->type = READ_TYPE;

    /* Send a TFTP Read Request to a server */
    status = TFTPC_Request(remote_ip, rpath, tftp_con, family);

    if (status <= 0)
    {
        if (NU_Close(tftp_con->file_desc) != NU_SUCCESS)
            NLOG_Error_Log("Failed to close file", NERR_SEVERE,
                           __FILE__, __LINE__);

#if (INCLUDE_STATIC_BUILD == NU_FALSE)

        /* Deallocate the memory allocated */
        if (NU_Deallocate_Memory((VOID*)tftp_con->input_buf) != NU_SUCCESS)
            NLOG_Error_Log("Failed to deallocate memory for the TFTP receive buffer",
                           NERR_SEVERE, __FILE__, __LINE__);

        if (NU_Deallocate_Memory((VOID *)tftp_con) != NU_SUCCESS)
            NLOG_Error_Log("Failed to deallocate memory for the TFTP control block",
                           NERR_SEVERE, __FILE__, __LINE__);
#endif

        /* Return to user mode */
        NU_USER_MODE();

        return (status);
    }

    /* Initialize retransmits. */
    retransmits = 0;

    /* Current State:
     *
     * client: RRQ --->
     *
     * The client just sent the Read Request (RRQ) and is now expecting the
     * server to send an OACK, DATA, or an ERROR packet.  If a valid
     * packet is not received within the timeout period, the client
     * will retransmit the Read Request (RRQ).
     */
    while ( (((bytes_received = TFTPC_Recv(tftp_con)) == NU_NO_DATA) ||
             ((GET16(tftp_con->input_buf, 0) != TFTP_OACK_OPCODE) &&
              (GET16(tftp_con->input_buf, 0) != TFTP_DATA_OPCODE) &&
              (GET16(tftp_con->input_buf, 0) != TFTP_ERROR_OPCODE))) &&
              (retransmits < TFTP_NUM_RETRANS) )
    {
        if (TFTPC_Retransmit(tftp_con, (INT32)status) <= 0)
            NLOG_Error_Log("Failed to retransmit TFTP packet",
                           NERR_SEVERE, __FILE__, __LINE__);
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

        /* Deallocate the memory allocated */
        if (NU_Deallocate_Memory((VOID*)tftp_con->trans_buf) != NU_SUCCESS)
            NLOG_Error_Log("Failed to deallocate memory for the TFTP transmission buffer",
                           NERR_SEVERE, __FILE__, __LINE__);

        if (NU_Deallocate_Memory((VOID*)tftp_con->input_buf) != NU_SUCCESS)
            NLOG_Error_Log("Failed to deallocate memory for the TFTP receive buffer",
                           NERR_SEVERE, __FILE__, __LINE__);

        if (NU_Deallocate_Memory((VOID *)tftp_con) != NU_SUCCESS)
            NLOG_Error_Log("Failed to deallocate memory for the TFTP control block",
                           NERR_SEVERE, __FILE__, __LINE__);
#endif

        /* Return to user mode */
        NU_USER_MODE();

        return (TFTP_CON_FAILURE);
    }

    /* If we did not receive an OACK, the server is not RFC 2347 compliant;
     * therefore, we must set our values to conform to RFC 1350 and start
     * on block 1
     */
    if (GET16(tftp_con->input_buf, 0) != TFTP_OACK_OPCODE)
    {
        tftp_con->options.blksize = TFTP_BLOCK_SIZE_DEFAULT;
        tftp_con->options.timeout = TFTP_TIMEOUT_DEFAULT;
        tftp_con->block_number++;
    }

    /* Process the first data packet that was received from the server.
     * A Duplicate Data Packet cannot have been received yet since this is
     * the first DATA packet to be received from the server; therefore,
     * precautions do not need to be taken to check for Duplicate Data.
     */
    status = TFTPC_Process_Data(tftp_con, bytes_received);

    if (status != NU_SUCCESS)
        tftp_con->status = (INT16)status;

    /* Get the rest of the file and process it */
    while (tftp_con->status == TRANSFERRING_FILE)
    {
        /* If the current DATA packet is not a duplicate, set retransmits
         * to zero.
         */
        if (status != TFTP_DUPLICATE_DATA)
            retransmits = 0;

        /* Otherwise, retransmit the previous ACK and increment the number
         * of retransmits sent.
         */
        else
        {
            if (TFTPC_Retransmit(tftp_con, TFTP_ACK_SIZE) <= 0)
                NLOG_Error_Log("Failed to retransmit TFTP packet",
                               NERR_SEVERE, __FILE__, __LINE__);
            retransmits++;
        }

        /* Current State:
         *
         * server: DATA --->
         *
         *                  <--- client: ACK
         *
         * The "session" is established and the server is transmitting
         * the file.  The client has just sent an ACK packet and is
         * expecting either a DATA packet or an ERROR packet.  If a valid
         * packet is not received, the client will retransmit the ACK.
         */
        while ( (((bytes_received = TFTPC_Recv(tftp_con)) == NU_NO_DATA) ||
                 ((GET16(tftp_con->input_buf, 0) != TFTP_DATA_OPCODE) &&
                  (GET16(tftp_con->input_buf, 0) != TFTP_ERROR_OPCODE))) &&
                  (retransmits < TFTP_NUM_RETRANS) )
        {
            if (TFTPC_Retransmit(tftp_con, TFTP_ACK_SIZE) <= 0)
                NLOG_Error_Log("Failed to retransmit TFTP packet",
                               NERR_SEVERE, __FILE__, __LINE__);
            retransmits++;
        }

        /* If a data packet was received, then process it.  Else a problem
         * occurred, and we need to exit.
         */
        if ( (bytes_received > 0) && (retransmits <= TFTP_NUM_RETRANS) )
        {
            if ((status = TFTPC_Process_Data(tftp_con, bytes_received)) ==
                TFTP_CON_FAILURE)
                tftp_con->status = TFTP_CON_FAILURE;
        }

        /* We will exit the loop and return status */
        else
        {
            tftp_con->status = TFTP_CON_FAILURE;
            status = TFTP_CON_FAILURE;
        }
    } /* While transferring file */

    /* If everything went okay then calculate the number of bytes that were
     * received in this file. Else we should return the last error code that
     * was received.
     */
    if (status == NU_SUCCESS)
    {
        total_bytes = 0;

        /* Save off the current location of the file pointer - seek 0 bytes
         * from the current position.
         */
        original_location = NU_Seek(file_desc, 0, PSEEK_CUR);

        if (original_location >= 0)
        {
            /* Get the end location of the file pointer - seek to the end of
             * the file.
             */
            total_bytes = (UINT32)NU_Seek(file_desc, 0, PSEEK_END);

            /* Restore the original position of the file pointer - seek
             * original_location bytes from the beginning of the file.
             */
            NU_Seek(file_desc, original_location, PSEEK_SET);
        }
    }

    else
        total_bytes = status;

    /* Close the file */
    if (NU_Close(tftp_con->file_desc) != NU_SUCCESS)
        NLOG_Error_Log("Failed to close file", NERR_SEVERE,
                       __FILE__, __LINE__);

    /* Close the socket, freeing any resources that were used. */
    if (NU_Close_Socket(tftp_con->socket_number) != NU_SUCCESS)
        NLOG_Error_Log("Failed to close socket", NERR_SEVERE,
                       __FILE__, __LINE__);

#if (INCLUDE_STATIC_BUILD == NU_FALSE)

    /* Deallocate the memory allocated for the session. */
    if (NU_Deallocate_Memory((VOID*)tftp_con->trans_buf) != NU_SUCCESS)
        NLOG_Error_Log("Failed to deallocate memory for the TFTP control block",
                       NERR_SEVERE, __FILE__, __LINE__);

    if (NU_Deallocate_Memory((VOID*)tftp_con->input_buf) != NU_SUCCESS)
        NLOG_Error_Log("Failed to deallocate memory for the TFTP control block",
                       NERR_SEVERE, __FILE__, __LINE__);

    if (NU_Deallocate_Memory((VOID*)tftp_con) != NU_SUCCESS)
        NLOG_Error_Log("Failed to deallocate memory for the TFTP control block",
                       NERR_SEVERE, __FILE__, __LINE__);

#endif

    /* Return to user mode */
    NU_USER_MODE();

    return (total_bytes);

} /* TFTPC_Get2 */

