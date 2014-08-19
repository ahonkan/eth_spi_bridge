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
*       mqueue.h
*
* COMPONENT
*
*		MQ - Message Queue
*
* DESCRIPTION
*
*		This file contains various definitions about Message Queues.
*
* DATA STRUCTURES
*
*		mq_attr_t							Used in getting and setting
*											the attributes of a message
*											queue.
*		sigevent							Definition of the sigevent
*											structure.
*
* DEPENDENCIES
*
*		"sys\types.h"						Contains definition for the
*											various types.
*		"signal.h"							Contains the various POSIX
*											signal related definitions.
*		"fcntl.h"							Contains the requests and
*											arguments for use by the
*											functions fcntl( ) and open( ).
*		"time.h"							Contains the various time
*											types.
*
************************************************************************/
#ifndef __MQUEUE_H_
#define __MQUEUE_H_

#if (_POSIX_MESSAGE_PASSING != -1)

#include "services/sys/types.h"
#include "services/signal.h"
#include "services/fcntl.h"
#include "services/sys/time.h"

typedef int mqd_t;

#ifndef __SIGEVENT_T_
#define __SIGEVENT_T_

struct sigevent
{
    int     sigev_notify;                   /* Notification type.  */
    int     sigev_signo;                    /* Signal number.  */
    union   sigval sigev_value;             /* Signal value.  */

    /* Notification Function.  */
    void(*) (union sigval)  sigev_notify_function;

    /* Notification Attributes.  */
    (pthread_attr_t*)       sigev_notify_attributes;
};

#endif  /*  __SIGEVENT_T  */

typedef struct mq_attr
{
    long        mq_msgsize;                 /* Maximum message size.  */
    long        mq_maxmsg;                  /* Maximum number of
                                               messages.  */
    long        mq_flags;                   /* Message queue flags.  */
    long        mq_curmsgs;                 /* Number of messages
                                               currently queued.  */
}mq_attr_t;

/* Function Declarations.  */
#ifdef __cplusplus
extern "C" {
#endif

int     mq_close(mqd_t mqdes);

int     mq_getattr(mqd_t mqdes, struct mq_attr* mqstat);

int     mq_notify(mqd_t mqdes, const struct sigevent*   notification);

mqd_t   mq_open(const char* name, int oflag, ...);

ssize_t mq_receive(mqd_t mqdes, char*   msg_ptr, size_t msg_len,
                                                       unsigned *msg_prio);

int     mq_send(mqd_t mqdes, const char*    msg_ptr, size_t msg_len,
                                                        unsigned msg_prio);

int     mq_setattr(mqd_t mqdes, const struct mq_attr*   mqstat,
                                              struct  mq_attr*    omqstat);

int     mq_unlink(const char*   name);

#if (_POSIX_TIMEOUTS != -1)

ssize_t mq_timedreceive(mqd_t   mqdes, char*    msg_ptr, size_t msg_len,
                  unsigned*   msg_prio,const struct timespec* abs_timeout);

int     mq_timedsend(mqd_t  mqdes, const char*  msg_ptr, size_t msg_len,
                     unsigned msg_prio,const struct timespec* abs_timeout);

#endif  /*  _POSIX_TIMEOUTS  */

#ifdef __cplusplus
}
#endif

#endif  /*  _POSIX_MESSAGE_PASSING  */

#endif  /*  __MQUEUE_H_  */




