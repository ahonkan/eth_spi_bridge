/*************************************************************************
*
*              Copyright 2013 Mentor Graphics Corporation
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
*       sshnu_rtl.c
*
*   DESCRIPTION
*
*       This file defines functions of an OS compatibility layer
*       for the Nucleus OS.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*
*   DEPENDENCIES
*
*
************************************************************************/
#include "nussh_includes.h"
#include "session.h"

NU_MEMORY_POOL *SSH_Mem_Pool = &System_Memory;
NU_SEMAPHORE SSH_Timer_Semaphore;
static INT32 SSH_Last_Tick;
static INT32 SSH_Time = 0x4d08c14e; /* 15th Dec 2010, 1823 GMT+5 */

INT32 SSH_errno;

CHAR SSH_Print_Buf2[500];

char *nussh_strdup(const char *s)
{
    char *res;
    size_t len;
    if (s == NULL)
        return NULL;
    len = strlen(s);
    res = malloc(len + 1);
    if (res)
        memcpy(res, s, len + 1);
    return res;
}

int nussh_snprintf(char *str, size_t size, const char *format, ...)
{
    va_list ap;
    int ret;

    va_start(ap, format);
    ret = vsnprintf(str, size, format, ap);
    va_end(ap);
    if (size > 0)
        str[size - 1] = '\0';
    return ret;
}

unsigned nussh_alarm(unsigned seconds)
{
    /* Do nothing. */
    return seconds;
}

void *nussh_signal(int signum, void *handler)
{
    return NULL;
}

void nussh_exit(int status)
{
    if (status != NU_SUCCESS)
    {
        TRACE(("\n!!! Fatal Error !!!\n"));
    }
    NU_Terminate_Task(NU_Current_Task_Pointer());
}

int nussh_atexit(void (*function)(void))
{
    return 0;
}

void nussh_perror(const char *msg)
{
    printf("%s\n", msg);
}

#define DAYS_OF_YR(y) (((y)%4 || ((y)%100==0 && (y)%400)) ? 365 : 366)

struct tm *nussh_gmtime(const time_t *timer)
{
    int t;
    int d;
    int year;
    int m;
    char days_of_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    static struct tm time_struct;

    /* Break down the timer */
    t = (int)*timer;
    time_struct.tm_sec = t % 60;
    t /= 60;
    time_struct.tm_min = t % 60;
    t /= 60;
    time_struct.tm_hour = t % 24;
    t /= 24;
    time_struct.tm_wday = (t + 4) % 7;

    /* Calculate the year and also account for possible leap years. */
    year = 1970;
    while (t >= (d = DAYS_OF_YR(year)))
    {
        t -= d;
        year++;
    }

    /* Set the year value */
    time_struct.tm_year = year - 1900;

    /* Set up day of the year */
    time_struct.tm_yday = t;

    /* Determine if this is a leap year */
    if (d == 366)
        days_of_month[1] = 29;
    else
        days_of_month[1] = 28;

    /* Figure out month and day */
    for(m = 0; t >= days_of_month[m]; m++)
        t -= days_of_month[m];

    /* Set month and day */
    time_struct.tm_mon = m;
    time_struct.tm_mday = t + 1;

    return &time_struct;
}

time_t nussh_time(time_t* t)
{
    UNSIGNED diff;
    UINT32 current_tick;

    /* Make all calculations atomic */

    NU_Obtain_Semaphore(&SSH_Timer_Semaphore, NU_SUSPEND);

    current_tick = NU_Retrieve_Clock();

    /* Since all quantities are unsigned, wrap around will also generate
       correct absolute difference. */
    diff = current_tick - SSH_Last_Tick;
    SSH_Time += (diff / NU_PLUS_TICKS_PER_SEC);
    SSH_Last_Tick = current_tick;

    NU_Release_Semaphore(&SSH_Timer_Semaphore);

    if (t)
        *t = SSH_Time;

    return ((time_t) SSH_Time);
}

struct tm * nussh_localtime (const time_t * timer)
{
    time_t          time;

    /* Make a local copy in case *timer is changing asynchronously. */
    time = *timer;

    /*  Perform the timezone adjustment */
    time += CFG_NU_OS_NET_SSH_DB_SSHSERVER_TIMEZONE;

    return(nussh_gmtime(&time));
}

clock_t nussh_clock(void)
{
    return ((clock_t) NU_Retrieve_Clock());
}

/*
** Sleep for a little while.  Return the amount of time slept.
*/
int nussh_usleep(int microsec){

    /*The actuall formula is NU_TICKS_PER_SECOND*(microsec/1000000).
     * (microsec/1000000) will convert time from micro sec to seconds.
     *  Modified to microsec/(1000000/NU_TICKS_PER_SECOND), so .
     */
    unsigned int ticks = microsec/(1000000/NU_PLUS_TICKS_PER_SEC);

    /* Still if ticks is 0, sleep for minimal granulate possible. */
    NU_Sleep(ticks?ticks:1);

    return 0;
}

/*
** Sleep for a little while.  Return the amount of time slept.
*/
int nussh_sleep(int sec){

    /* Still if ticks is 0, sleep for minimal granulate possible. */
    NU_Sleep(NU_PLUS_TICKS_PER_SEC*sec);

    return 0;
}

int nussh_getpid(void)
{
	return (int)NU_Current_Task_Pointer();
}

/*
** This routein will return EPOCH time according to SSH internal timer.
*/
int nussh_gettimeofday(struct timeval *tp, void *tz)
{
    time_t tv;

    UNUSED_PARAMETER(tz);

    /* Populate the time structure. */
    nussh_time(&tv);
    tv += CFG_NU_OS_NET_SSH_DB_SSHSERVER_TIMEZONE;
    if (tp != NU_NULL)
    {
        tp->tv_sec = tv;
        tp->tv_usec = ((NU_Retrieve_Clock() % NU_PLUS_TICKS_PER_SEC) * 10000);
    }

    return 0;
}
