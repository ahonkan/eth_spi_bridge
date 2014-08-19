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
*       tftp.h
*
*   COMPONENT
*
*       TFTP -  Trivial File Transfer Protocol
*
*   DESCRIPTION
*
*       This file contains declarations for data structures that are
*       shared between the TFTP client and server.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       tftpc4.h
*
*************************************************************************/

#ifndef NU_TFTP_H
#define NU_TFTP_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* Error codes for the TFTP Client component. */
#define TFTP_ERROR              -1751  /*   Not defined, see error message    */
#define TFTP_FILE_NFOUND        -1752  /*   File not found                    */
#define TFTP_ACCESS_VIOLATION   -1753  /*   Access violation                  */
#define TFTP_DISK_FULL          -1754  /*   Disk full or allocation exceeded */
#define TFTP_FILE_ERROR         -1755
#define TFTP_BAD_OPERATION      -1756  /*   Illegal TFTP operation            */
#define TFTP_UNKNOWN_TID        -1757  /*   Unknown transfer ID               */
#define TFTP_FILE_EXISTS        -1758  /*   File already exists               */
#define TFTP_NO_SUCH_USER       -1759  /*   No such user                      */
#define TFTP_BAD_OPTION         -1760  /*   Illegal option negotiation       */
#define TFTP_NO_MEMORY          -1761
#define TFTP_CON_FAILURE        -1762
#define TFTP_DUPLICATE_DATA     -1763
#define TFTP_DUPLICATE_ACK      -1764

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* NU_TFTP_H */
