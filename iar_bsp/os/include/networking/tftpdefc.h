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
*       tftpdefc.h
*
*   COMPONENT
*
*       Net - TFTP Client
*
*   DESCRIPTION
*
*       This file contains data structure definitions used by the TFTP
*       client and server routines.
*
*   DATA STRUCTURES
*
*       TFTPC_CB        TFTP Control Block
*       TFTP_OPTIONS    User indicated options requested of the server
*
*   DEPENDENCIES
*
*       pcdisk.h
*
*************************************************************************/

#ifndef NU_TFTPDEFC_H
#define NU_TFTPDEFC_H

#include "storage/pcdisk.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* The number of error codes defined by the TFTP specification. */
#define TFTPC_ERROR_CODE_COUNT  9

/* TFTP Opcodes defined by RFC 783 */
#define TFTP_RRQ_OPCODE       1
#define TFTP_WRQ_OPCODE       2
#define TFTP_DATA_OPCODE      3
#define TFTP_ACK_OPCODE       4
#define TFTP_ERROR_OPCODE     5

/* TFTP Opcode defined by RFC 2347 */
#define TFTP_OACK_OPCODE      6

/* Connection Status Values */
#define TRANSFERRING_FILE     100
#define TRANSFER_COMPLETE     102

/* RFC 1350 defaults */
#define TFTP_BLOCK_SIZE_DEFAULT     512
#define TFTP_HEADER_SIZE            4
#define TFTP_ACK_SIZE               4

#define TFTP_BLOCK_SIZE_MAX         65464UL
#define TFTP_BLOCK_SIZE_MIN         8
#define TFTP_BUFFER_SIZE_MIN        100
#define TFTP_PARSING_LENGTH         128

/* Type Of File Transfer */
#define READ_TYPE   0
#define WRITE_TYPE  1

/* User defined defaults */
#define TFTP_TIMEOUT_DEFAULT        30
#define TFTP_NUM_RETRANS   3

/* RFC 2348, 2349 options */
typedef struct tftp_options
{
    UINT32  tsize;                  /* If sent as 0 on a RRQ, server returns the size
                                       of the file being requested - if value
                                       specified on a WRQ, server accepts/declines
                                       the size of the file to be transmitted */
    UINT16  timeout;                /* Specify the server's timeout value */
    UINT16  blksize;                /* Specify the size of the data block
                                       to be transmitted at a time */
    INT16   timeout_acknowledged;   /* NU_TRUE or NU_FALSE if the server
                                       acknowledges the timeout requested */
    INT16   blksize_acknowledged;   /* NU_TRUE or NU_FALSE if the server
                                       acknowledges the blksize requested */
    INT16   tsize_acknowledged;     /* NU_TRUE or NU_FALSE if the server
                                       acknowledges the tsize requested */
    UINT8   padN[2];
} TFTP_OPTIONS;

/* TFTP Control Block  */
typedef struct tftpc_cb
{
    INT             socket_number;          /* The socket that is being used. */
    struct          addr_struct server_addr;/* Server's address. */
    TFTP_OPTIONS    options;                /* Options requested of the server */
    UINT16          tid;                    /* Server's Transfer ID */
    INT16           status;                 /* Status of communication. */
    INT16           type;
    UINT16          recv_buf_length;        /* The number of bytes that will fit
                                               in the receive buffer. */
    UINT16          trans_buf_length;       /* The number of bytes that will fit
                                               in the transmission buffer. */
    UINT16          block_number;           /* Block number of expected packet. */
    CHAR            *trans_buf;             /* Last packet sent. */
    CHAR            *input_buf;             /* Last packet received */
    INT             file_desc;              /* File descriptor */
    UINT32          cur_buf_size;           /* How much space is left in buffer. */
    CHAR            *mode;                  /* File Transfer Mode Requested by Client. */
} TFTPC_CB;

/* Maintain backward compatibility with previous versions of TFTP Server */
#define TFTP_CB TFTPC_CB

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* NU_TFTPDEFC_H */
