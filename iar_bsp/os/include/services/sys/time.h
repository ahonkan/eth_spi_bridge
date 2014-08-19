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
*       time.h
*
*   COMPONENT
*
*       TIC - Timer Control.
*
*   DESCRIPTION
*
*       This file contains the various time types.
*
*       NOTE: Standard C RTL header.
*
*   DATA STRUCTURES
*
*       tm
*       timespec
*       itimerspec
*       clock_t
*       clockid_t
*       time_t
*       timer_t
*
*   DEPENDENCIES
*
*       config.h            Nucleus POSIX configuration definitions
*       compiler.h          Nucleus POSIX compiler-specific definitions
*       types.h             POSIX time.h definitions
*       unistd.h            POSIX unistd.h definitions
*       signal.h            POSIX signal.h defintions
*
*************************************************************************/

#ifndef NU_PSX_TIME_H
#define NU_PSX_TIME_H

#include "services/config.h"
#include "services/compiler.h"
#include "services/sys/types.h"
#include "services/unistd.h"

/* For Metaware Metrowerks and KMC GNU Tools.  */
#ifndef _TIME_H
#define _TIME_H

/* For ADS Tools.  */
#ifndef __time_h
#define __time_h

/* For Hitachi Tools and TI Tools.  */
#ifndef _TIME
#define _TIME

/* For Paradigm Tools and Microtec Tools.  */
#ifndef __TIME_H
#define __TIME_H

/* For Microsoft Visual C.  */
#ifndef _INC_TIME
#define _INC_TIME

#ifndef __TIME_H_
#define __TIME_H_

/* For Code Sourcery ARM GNU (sys/time.h) */
#ifndef _SYS_TIME_H_
#define _SYS_TIME_H_

/* For MinGW and other GNU */
#ifndef _TIME_H_
#define _TIME_H_

/* For DIAB tools */
#ifndef __Itime
#define __Itime

/* Null pointer constant. */
#ifndef NULL
#define NULL            0
#endif /* NULL */

#ifndef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC          100
#endif /* CLOCKS_PER_SEC */

/* The identifier of the system-wide real time clock.  */
#define     CLOCK_REALTIME      0

/* The identifier for the system wide monotonic clock.  */
#define     CLOCK_MONOTONIC     1

/* Flag indicating time is absolute. For functions taking timer objects,
   this refers to the clock associated with the timer.  */
#define     TIMER_ABSTIME       1

/* timeval in sec and microsec */
struct timeval
{
    time_t         tv_sec;      /* Seconds. */
    suseconds_t    tv_usec;     /* Microseconds. */
};

struct tm
{
    /* date and time components.  */
    int tm_sec;                             /* Seconds [0,59].  */
    int tm_min;                             /* Minutes [0,59].  */
    int tm_hour;                            /* Hour [0,23].  */
    int tm_mday;                            /* Day of month [1,31].  */
    int tm_mon;                             /* Month of year [0,11].  */
    int tm_year;                            /* Years since 1900.  */
    int tm_wday;                            /* Day of week [0,6]
                                            (Sunday =0).  */
    int tm_yday;                            /* Day of year [0,365].  */
    int tm_isdst;                           /* Daylight Savings flag.  */
};

struct timespec
{
    time_t tv_sec;                          /* Seconds.  */
    long tv_nsec;                           /* NanoSeconds.  */
};

struct itimerspec
{
    struct timespec it_interval;            /* Timer Period.  */
    struct timespec it_value;               /* Timer Expiration.  */
};

#ifdef __cplusplus
extern "C" {
#endif /*  __cplusplus  */

char*       asctime(const struct tm* timeptr);

clock_t     clock(void);
char *      ctime(const time_t *clock);

#if (_POSIX_THREAD_SAFE_FUNCTIONS != -1)

char *      asctime_r(const struct tm * timeptr,char * buf);
char *      ctime_r(const time_t * clock, char *buf);
struct tm * gmtime_r(const time_t * timer, struct tm * result);
struct tm * localtime_r(const time_t * timer,struct tm * result);

#endif /* _POSIX_THREAD_SAFE_FUNCTIONS */

double      difftime(time_t time1, time_t time0);
struct tm * gmtime(const time_t * timer);
struct tm * localtime(const time_t* timer);
time_t      mktime(struct tm * timeptr);
size_t      strftime(char * s, size_t maxsize, const char *format,
                     const struct tm *timeptr);
time_t      time(time_t * tloc);

#if (_POSIX_TIMERS != -1)

#include "services/signal.h"

int         clock_gettime(clockid_t clock_id, struct timespec *tp);
int         clock_getres(clockid_t clock_id, struct timespec *res);
int         clock_settime(clockid_t clock_id, const struct timespec *tp);
int         nanosleep(const struct timespec *rqtp, struct timespec *rmtp);
int         timer_create(clockid_t clock_id,struct sigevent *evp,timer_t *timerid);
int         timer_delete(timer_t timerid);
int         timer_gettime(timer_t timerid, struct itimerspec *value);
int         timer_settime(timer_t timerid, int flags,
                          const struct itimerspec *value,struct itimerspec *ovalue);
int         timer_getoverrun(timer_t timerid);
int         clock_nanosleep(clockid_t clock_id, int flags,
                            const struct timespec * rqtp, struct timespec *rmtp);

#endif /* _POSIX_TIMERS */

extern  int     daylight;
extern  long    timezone;
extern  char *  tzname[];

#ifdef __cplusplus
}
#endif  /*  __cplusplus  */

#endif /* __Itime */
#endif /* __TIME_H_ */
#endif /* _SYS_TIME_H_ */
#endif /* _TIME_H_ */
#endif /* _INC_TIME */
#endif /* __TIME_H */
#endif /* _TIME */
#endif /* __time_h */
#endif /* _TIME_H */

#endif /* NU_PSX_TIME_H */
