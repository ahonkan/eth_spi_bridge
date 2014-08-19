/*************************************************************************
*
*              Copyright 2010 Mentor Graphics Corporation
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
*       nussh_rtl.h
*
*   DESCRIPTION
*
*       This file defines an OS compatibility layer for Nucleus OS.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       nucleus.h
*
************************************************************************/
#ifndef NUSSH_RTL_H
#define NUSSH_RTL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Define Application data structures.  */
extern NU_MEMORY_POOL          *SSHS_Mem_Pool;
extern NU_TASK                 *sshsrv_task;
extern STATUS                   sshsrv_running;
extern NU_SEMAPHORE             SSH_Timer_Semaphore;

/* Various macros, structures and function definitions for porting of
 * ANSI C/POSIX code to Nucleus equivalents.
 */

/*
 * General libc definitions and mappings to Nucleus equivalents.
 */

/* Contains the difference in seconds from UTC and local standard time */
#ifndef CFG_NU_OS_NET_SSH_DB_SSHSERVER_TIMEZONE
#define CFG_NU_OS_NET_SSH_DB_SSHSERVER_TIMEZONE 0
#endif

#ifndef CFG_NU_OS_SVCS_POSIX_CORE_ENABLE

/* Alarms/signals related macros. */
#define SIGHUP                          1
#define SIGINT                          2
#define SIGQUIT                         3
#define SIGILL                          4
#define SIGTRAP                         5
#define SIGABRT                         6
#define SIGFPE                          8
#define SIGKILL                         9
#define SIGSEGV                         11
#define SIGPIPE                         13
#define SIGALRM                         14
#define SIGTERM                         15
#define SIGUSR1                         30
#define SIGUSR2                         31
#define ITIMER_REAL                     100

/* Error macros. */
#define EINTR           4       /* Interrupted system call. */
#define EBADF           9       /* Bad file number. */
#define EINVAL          22      /* Invalid argument. */
#define ERANGE          34      /* Math result not representable. */
#define EINPROGRESS     36      /* Operation now in progress. */
#define EADDRNOTAVAIL   49      /* Cannot assign requested address. */

#endif  /* CFG_NU_OS_SVCS_POSIX_CORE_ENABLE */

/* Signal flags. */
#define SA_NOCLDSTOP    0x00000001

/* Map memory and other general functions to the OS layer. */
#define strdup nussh_strdup
#define snprintf nussh_snprintf
#define alarm nussh_alarm
#define getpid    nussh_getpid
#define exit(x) nussh_exit(x)
#define atexit nussh_atexit
#define perror nussh_perror
#define sleep  nussh_sleep
#define usleep  nussh_usleep
#define gmtime nussh_gmtime
#define time nussh_time
#define localtime nussh_localtime
#define clock nussh_clock
#define gettimeofday nussh_gettimeofday

/* Error number, needed to be signed. */
extern INT32 SSH_errno;
#define errno SSH_errno

char *nussh_strdup(const char *s);
int nussh_snprintf(char *str, size_t size, const char *format, ...);
unsigned nussh_alarm(unsigned seconds);
void *nussh_signal(int signum, void *handler);
void nussh_exit(int status);
int nussh_atexit(void (*function)(void));
int nussh_sleep(int);
int nussh_usleep(int);
struct tm * nussh_localtime (const time_t * timer);
void nussh_perror(const char *msg);
struct tm *nussh_gmtime(const time_t *timer);
time_t nussh_time(time_t* t);
clock_t nussh_clock(void);
int nussh_getpid(void);
int gettimeofday(struct timeval *tp, void *tz);

#ifdef __cplusplus
}
#endif

#endif /* NUSSH_RTL_H */
