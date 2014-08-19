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

/**************************************************************************
*
*   FILENAME
*
*       cmsg_defs.h
*
*   DESCRIPTION
*
*       This file defines the macros used at the application layer and
*       by the stack to create and traverse a list of ancillary data
*       objects.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       nu_net.h
*
****************************************************************************/

#ifndef CMSG_DEFS_H
#define CMSG_DEFS_H

#include "networking/nu_net.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

#define CMSG_FIRSTHDR(mhdr) \
   ( (mhdr)->msg_controllen >= sizeof(cmsghdr) ? \
     (cmsghdr *)(mhdr)->msg_control : (cmsghdr *)NU_NULL)

#define CMSG_NXTHDR(mhdr, cmsg) \
        ( ((cmsg) == NU_NULL) ? CMSG_FIRSTHDR(mhdr) : \
          (((CHAR *)(cmsg) + (UINT8)(NET_ALIGN((cmsg)->cmsg_len)) + \
          (UINT8)(NET_ALIGN(sizeof(cmsghdr)))) > \
          ((CHAR *)((mhdr)->msg_control) + (UINT8)((mhdr)->msg_controllen)) ? \
          (cmsghdr *)NU_NULL : (cmsghdr *)((CHAR *)(cmsg) + \
          (UINT8)(NET_ALIGN((cmsg)->cmsg_len)))) )

#define CMSG_DATA(cmsg) ((CHAR *)(cmsg) + (UINT16)(NET_ALIGN(sizeof(cmsghdr))))

#define CMSG_SPACE(length) ((UINT16)(NET_ALIGN(sizeof(cmsghdr))) + (UINT16)(NET_ALIGN(length)))

#define CMSG_LEN(length) ((UINT16)(NET_ALIGN(sizeof(cmsghdr))) + length)

#define NU_CMSG_FIRSTHDR    CMSG_FIRSTHDR
#define NU_CMSG_NXTHDR      CMSG_NXTHDR
#define NU_CMSG_DATA        CMSG_DATA
#define NU_CMSG_SPACE       CMSG_SPACE
#define NU_CMSG_LEN         CMSG_LEN

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* CMSG_DEFS_H */
