/*************************************************************************
*
*             Copyright 1995-2007 Mentor Graphics Corporation
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
*       tftpdefs.h                                     
*
*   COMPONENT
*
*       Nucleus TFTP Server
*
*   DESCRIPTION
*
*       This file contains data structure definitions used by the
*       Nucleus TFTP server routines.
*
*   DATA STRUCTURES
*
*       TFTPS_CB        TFTP Server Control Block
*
*   DEPENDENCIES
*
*       pcdisk.h
*
************************************************************************/

#ifndef NU_TFTPDEFS_H
#define NU_TFTPDEFS_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#include "storage/pcdisk.h"


/*************************************************************/

/* TFTP Server Control Block  */
typedef struct tftps_cb
{
    INT             socket_number;          /* The socket that is being used. */
    struct          addr_struct server_addr;/* Server's address. */
    TFTP_OPTIONS    options;                /* Options requested of the server */
    UINT16          tid;                    /* Server's Transfer ID */
    INT16           status;                 /* Status of communication. */
    INT16           type;
    UINT16          block_number;           /* Block number of expected packet. */
    CHAR            *trans_buf;             /* Output buffer */
    CHAR            *tftp_input_buffer;     /* Input buffer */
    INT             file_desc;              /* File descriptor */
    UINT32          cur_buf_size;           /* How much space is left in buffer. */
    CHAR            *mode;                  /* File Transfer Mode Requested by Client. */
    INT32           bytes_sent;             /* The number of bytes last transmitted */
    INT             session_socket_number;
} TFTPS_CB;


/* TFTP_Unused_Parameter is defined in xprot/tftpserv/src/tftps.c */
extern UINT32 TFTP_Unused_Parameter;

/* this macro is used to remove compilation warnings. */
#define TFTP_UNUSED_PARAMETER(x)  TFTP_Unused_Parameter = ((UINT32)(x))


#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* NU_TFTPDEFS_H */
