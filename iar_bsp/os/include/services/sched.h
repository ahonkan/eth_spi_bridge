/*************************************************************************
*
*                  Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
*************************************************************************/
/*************************************************************************
*
*   FILE NAME
*
*       sched.h
*
*   COMPONENT
*
*		TC - Thread Control.
*
*   DESCRIPTION
*
*		Contains execution scheduling related information.
*
*   DATA STRUCTURES
*
*		sched_param_t						Holds scheduling parameters.
*
*   DEPENDENCIES
*
*       config.h            Nucleus POSIX configuration definitions
*       compiler.h          Nucleus POSIX compiler-specific definitions
*       unistd.h            POSIX unistd.h definitions
*		time.h              POSIX time.h definitions
*
*************************************************************************/

#ifndef NU_PSX_SCHED_H
#define NU_PSX_SCHED_H

#include "services/config.h"
#include "services/compiler.h"
#include "services/unistd.h"

#ifndef __SCHED_H_
#define __SCHED_H_

#ifndef _SCHED_H
#define	_SCHED_H

#ifndef _SCHED_H_
#define _SCHED_H_

#if (_POSIX_PRIORITY_SCHEDULING != -1)

#include "services/sys/time.h"

/* Scheduling types */
#define SCHED_FIFO  0
#define SCHED_RR    1
#define SCHED_OTHER 2

/* Scheduling parameters.  */
typedef struct sched_param
{
    int sched_priority;                     /* Process execution scheduling
                                               priority.  */
}sched_param_t;


#ifdef __cplusplus
extern "C" {
#endif

int sched_get_priority_max(int policy);

int sched_get_priority_min(int policy);

int sched_rr_get_interval(pid_t pid, struct timespec *interval);

int sched_yield(void);

int sched_setscheduler(pid_t pid,int policy,
                                          const struct sched_param *param);

int sched_setparam(pid_t pid,const struct sched_param *param);

int sched_getscheduler(pid_t pid);

int sched_getparam(pid_t pid,struct sched_param *param);

#ifdef __cplusplus
}
#endif

#endif /* _POSIX_PRIORITY_SCHEDULING */

#endif /* _SCHED_H_ */
#endif /* _SCHED_H */
#endif /* __SCHED_H_ */

#endif /* NU_PSX_TIME_H */
