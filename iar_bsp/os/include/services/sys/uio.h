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
*       uio.h
*
* COMPONENT
*
*       Nucleus POSIX - Networking
*
* DESCRIPTION
*
*       Contain definitions for vector I/O operations.
*
* DATA STRUCTURES
*
*       iovec
*
* DEPENDENCIES
*
*       types.h
*
************************************************************************/
#ifndef _SYS_UIO_H
#define _SYS_UIO_H

#include "services/sys/types.h"

/* Structure for scatter/gather I/O. */
struct iovec
{
    void *iov_base;                         /* Base address of a memory
                                               region for input or output   */
    size_t iov_len;                         /* The size of the memory
                                               pointed to by iov_base       */
};

#endif /* _SYS_UIO_H */
