/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 1998 - 2007
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILENAME                                               
*
*       timer.h                                                  
*
*   DESCRIPTION
*
*       This file contains all data structures and typedefs associated
*       with SNMP timers.
*
*   DATA STRUCTURES
*
*       timer_descr_s
*       x_timeval
*
*   DEPENDENCIES
*
*       None
*
************************************************************************/

#ifndef TIMER_H
#define TIMER_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++    */
#endif /* __cplusplus */

typedef struct timer_descr_s timer_descr_t;
typedef VOID                 (*timer_callback_t)(timer_descr_t *, UINT32,
                                                 VOID *);

struct timer_descr_s
{
    timer_callback_t        callback;
    VOID                    *parm;
    UINT32                  msecs;
    UINT32                  alarm;
    UINT32                  count;
    INT32                   type;
    struct timer_descr_s    *next;
};

/*
 * Structure returned by gettimeofday(2) system call,
 * and used in other calls.
 */
struct x_timeval
{
    long    tv_sec;     /* seconds */
    long    tv_usec;    /* and microseconds */
};

#ifdef          __cplusplus
}
#endif /* __cplusplus */

#endif

