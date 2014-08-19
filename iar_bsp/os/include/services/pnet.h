/************************************************************************
*
*                  Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
*************************************************************************

*************************************************************************
* FILE NAME
*
*       pnet.h
*
* COMPONENT
*
*       Nucleus POSIX - Networking
*
*
* DESCRIPTION
*
*       POSIX Networking initialization and utility functions.
*
*
* DATA STRUCTURES
*
*       p_socket
*
* DEPENDENCIES
*
*       None.
*
************************************************************************/
#ifndef _PNET_PNET_H
#define _PNET_PNET_H

/* POSIX RTL standard library header files conflicting with Nucleus NET
toolset header files from net/target.h. Including Nucleus POSIX
stdio.h, string.h and stdlib.h first before the toolset in order to
avoid toolset library conflict within Nucleus POSIX library.
(defect#215) */
#if ((defined(POSIX_INCLUDE_NET)) && (POSIX_INCLUDE_NET > 0))
#if ( (defined(INCLUDE_NU_POSIX)) && (INCLUDE_NU_POSIX == NU_FALSE) )
#include "services/stdio.h"
#include "services/string.h"
#include "services/stdlib.h"
#endif /* INCLUDE_NU_POSIX == NU_FALSE */
#endif

/* POSIX Socket Control Block for open socket */
typedef struct p_socket
{
    INT8            domain;                 /* Domain Name                  */
    INT16           type;                   /* Address Type                 */
    INT16           protocol;               /* Protocol Type                */
    INT8            connect;                /* 1-connected 0-Not connected  */
    INT8            listen;                 /* Socket is listening or not   */
    INT             port;                   /* Port                         */
    VOID            *buff_ptr;              /* Pointer to temporary buffer  */
    UINT16          length;                 /* Length of the buffer         */
    UINT16          p_errno;                /* Socket errno                 */
    UINT8           padding[1];             /* 1 byte Padding               */

}POSIX_SOCKET;


#ifdef __cplusplus
extern "C" {
#endif

STATUS  pnet_auto_buffer(pid_t pid, VOID **buffer, UINT16 *oldlen, size_t oldsize,
                        UINT16 *newlen, size_t newsize);
INT     pnet_write_dns_entry(INT fd);
VOID    PNET_Net_Maps_Error_Values(pid_t pid,POSIX_SOCKET* sock_ptr,STATUS errval);
CHAR*   POSIX_SYS_NET_Get_Inet_Ntoa_Buff(pid_t pid);

#ifdef __cplusplus
}
#endif

#endif /* _PNET_PNET_H */

