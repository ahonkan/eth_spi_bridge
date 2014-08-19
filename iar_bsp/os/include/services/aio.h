/*************************************************************************
*
*                  Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
* FILE NAME
*
*       aio.h
*
* COMPONENT
*
*       Nucleus POSIX - file system
*
* DESCRIPTION
*
*       This file contains the asynchronous input and output related
*		definitions.
*
* DATA STRUCTURES
*
*		aiocb								Asynchronous I/O Control
*											block.
*
* DEPENDENCIES
*
*		"fcntl.h"							Contains the requests and
*											arguments for use by the
*											functions fcntl( ) and
*											open( ).
*		"sys/types.h"						Contains various data types.
*		"signal.h"							Contains the various signal
*											related definitions.
*		"time.h"							Contains the various time
*											types.
*
*************************************************************************/
#ifndef __AIO_H_
#define __AIO_H_

#if (_POSIX_ASYNCHRONOUS_IO != -1)

#include "services/fcntl.h"
#include "services/sys/types.h"
#include "services/signal.h"
#include "services/sys/time.h"



/* Return value indicating that all requested operations have been
   canceled.  */
#define AIO_CANCELED        0x001

/* Return value indicating that some of the requested operations count not
   be canceled since they are in progress.  */
#define AIO_NOTCANCELED     0x002


/* Return value indicating none of the requested operations could be
   canceled since they had already completed.  */
#define AIO_ALLDONE         0x004

/* A lio_listio() element operation option indicating that no transfer is
   requested.  */
#define LIO_NOP             0x001

/* A lio_listio() synchronization operation indicating that the calling
   thread is to continue operation while the lio_listio() operation is
   being performed and no notification when the operation is complete.  */
#define LIO_NOWAIT          0x002

/* A lio_listio() operation option requesting a read.  */
#define LIO_READ            0x004

/* A lio_listio() synchronization operation option indicating that the
   calling thread is to suspend until lio_listio operation is
   complete.  */
#define LIO_WAIT            0x008

/* A lio_listio() element operation option requesting a write.  */
#define LIO_WRITE           0x010


typedef struct aiocb
{
    int             aio_fildes;             /* File descriptor.  */
    off_t           aio_offset;             /* File offset.  */
    volatile void*  aio_buf;                /* Location of buffer.  */
    size_t          aio_nbytes;             /* Length of transfer. */
    int             aio_reqprio;            /* Request priority offset. */
    struct sigevent aio_sigevent;           /* Signal number and value. */
    int             aio_lio_opcode;         /* Operation to be
                                               performed.  */
    unsigned long   reserved[6];            /* Used for the internal
                                               operations.  */

}aiocb;

/* Function Declarations.  */

#ifdef __cplusplus
extern "C" {
#endif

/* Tries to cancel an Asynchronous operation.  */
int aio_cancel(int , struct aiocb*  );

/* Retrieves error status for an asynchronous operation.  */
int aio_error(const struct aiocb*   aiocbp);

/* Asynchronously reads from a file.  */
int aio_read(struct aiocb*  );

/* Retrieves return status for an asynchronous operation.  */
ssize_t aio_return(struct aiocb*    );

/* Wait for an asynchronous operation to complete.  */
int aio_suspend(const struct aiocb* const[], int, const struct timespec*);

/* Asynchronously write to a file.  */
int aio_write(struct aiocb* );

/* lio_listio Performs a list of IO operations asynchronously or
   synchronously.  */
int lio_listio(int , struct aiocb* const list[],int, struct sigevent*  );

/* Enables the asynchronous capability for devices */
int aio_enable(char *devname,int priority);

/* Disable the asynchronous capability for devices */
int aio_disable(char *devname);

#ifdef __cplusplus
}
#endif

#endif  /*  _POSIX_ASYNCHRONOUS_IO  */

#endif  /*  __AIO_H_  */




