/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 1998 - 2007
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************
*
* Copyright (c) 1992, 1993
*   The Regents of the University of California.  All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
* 3. All advertising materials mentioning features or use of this software
*    must display the following acknowledgement:
*   This product includes software developed by the University of
*   California, Berkeley and its contributors.
* 4. Neither the name of the University nor the names of its contributors
*    may be used to endorse or promote products derived from this software
*    without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
*

**************************************************************************
*
*   FILENAME                                               
*
*       snmp_prt.c                                               
*
*   DESCRIPTION
*
*       This file contains miscellaneous utilities.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       x_realloc
*       x_gettimeofday
*       x_timemsec
*       x_timerinit
*       x_timeout
*       x_untimeout
*       timeout_function
*       x_insque
*       x_remque
*
*   DEPENDENCIES
*
*       ctype.h
*       nucleus.h
*       snmp_prt.h
*       xtypes.h
*       externs.h
*
************************************************************************/

#include <ctype.h>
#include "nucleus.h"
#include "networking/target.h"
#include "networking/externs.h"
#include "networking/snmp_prt.h"
#include "networking/xtypes.h"

/*------------------ Specific support requirements --------------------*/
#ifdef  IP_H
#undef  IP_H
#endif

#define IP_H             /* Exclude the Nucleus NET IP.H header file. */

#define MAX_TIMERS      25

extern  NU_MEMORY_POOL  System_Memory;
extern  VOID            timeout_function(UNSIGNED arg);


tmr_t       *tmr_free;
tmr_t       *tmr_active;

#if (INCLUDE_MIB_RMON1 == NU_TRUE)
NU_QUEUE                Timer_Queue;
NU_TASK                 SNMP_Timer_Handler;
parm_list_t plist[SNMP_SPRINTF_NUM_PARMS];
#endif

VOID    x_remque(VOID *);
VOID    x_insque(VOID *, VOID *);

#if (INCLUDE_MIB_RMON1 == NU_TRUE)
VOID SNMP_Timer_Task(UNSIGNED argc, VOID *argv);
/************************************************************************
*
*   FUNCTION
*
*       x_realloc
*
*   DESCRIPTION
*
*       This function allocates a new block of memory, copies the
*       contents of the passed in memory block into the new block,
*       then deallocates the memory block passed in.
*
*   INPUTS
*
*       *memblock       A pointer to a block of memory.
*       size            The size of the new block of memory and the
*                       number of bytes of the old block of memory to
*                       copy into the new block of memory.
*
*   OUTPUTS
*
*       A pointer to the new block of memory.
*
*************************************************************************/
VOID *x_realloc (VOID *memblock, UINT32 size)
{
    VOID    *cp = NU_NULL;

    if (!memblock)
    {
        if (NU_Allocate_Memory(&System_Memory, (VOID**)&cp, size,
                               NU_NO_SUSPEND) == NU_SUCCESS)
        {
            cp = TLS_Normalize_Ptr(cp);
            UTL_Zero(cp, size);
        }
    }

    else if (NU_Allocate_Memory(&System_Memory, (VOID**)&cp, size,
                                NU_NO_SUSPEND) == NU_SUCCESS)
    {
        cp = TLS_Normalize_Ptr(cp);
        UTL_Zero(cp, size);
        NU_BLOCK_COPY(cp, memblock, (unsigned int)size);
        NU_Deallocate_Memory(memblock);
    }

    return (cp);

} /* x_realloc */

/*---------------------- Library Functions (XLIB Defines Apply) --------*/

/************************************************************************
*
*   FUNCTION
*
*       x_gettimeofday
*
*   DESCRIPTION
*
*       This function returns the time of day.
*
*   INPUTS
*
*       *tp     The data structure which will hold the time of day
*               information.
*       *p      Unused parameter.
*
*   OUTPUTS
*
*       0
*       gettimeofday
*
*************************************************************************/
INT32 x_gettimeofday (struct x_timeval *tp, VOID *p)
{
    UNUSED_PARAMETER(p);

#ifdef WINNT
    time_t ltime;
    time_t sysStartTime=-1;

    if (sysStartTime < 0)
    {
        time(&sysStartTime);
        tp->tv_sec=0;
    }
    else
    {
        time(&ltime);
        tp->tv_usec=0;
        tp->tv_sec=ltime-sysStartTime;
    }
    return (0);

#else
    UNUSED_PARAMETER(tp);
    return (0);
#endif

} /* x_gettimeofday */

/*----------------- Stack Functions (XSTK defines apply)---------------*/

/************************************************************************
*
*   FUNCTION
*
*       x_timemsec
*
*   DESCRIPTION
*
*       This is a user-specific function which has not been implemented.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       0
*
*************************************************************************/
UINT32 x_timemsec (VOID)
{
    return(0);

} /* x_timemsec */

#endif

/************************************************************************
*
*   FUNCTION
*
*       x_timerinit
*
*   DESCRIPTION
*
*       This function initializes the list of timers.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       NU_TRUE     The timer was successfully initialized.
*       NU_FALSE    The timer was not successfully initialized, probably
*                   having to do with lack of resources.
*
*************************************************************************/
BOOLEAN x_timerinit (VOID)
{
    tmr_t   *tp;
    INT     i;
    INT     j;
#if (INCLUDE_MIB_RMON1 == NU_TRUE)
    VOID    *queue_space;
    /* SNMP_Timer_Task */
    VOID    *snmp_timer_mem;
#endif

#if (INCLUDE_MIB_RMON1 == NU_TRUE)
    if (NU_Allocate_Memory(&System_Memory, &queue_space,
                           sizeof(UNSIGNED) * 20, NU_NO_SUSPEND)
                                                            != NU_SUCCESS)
        return (NU_FALSE);

    queue_space = TLS_Normalize_Ptr(queue_space);

    if (NU_Allocate_Memory(&System_Memory, &snmp_timer_mem, 1000,
                            NU_NO_SUSPEND) != NU_SUCCESS)
    {
        NU_Deallocate_Memory(queue_space);
        return (NU_FALSE);
    }

    snmp_timer_mem = TLS_Normalize_Ptr(snmp_timer_mem);

    if (NU_Create_Task(&SNMP_Timer_Handler, "snmp_timer",
                        SNMP_Timer_Task, 0, NU_NULL, snmp_timer_mem,
                        1000, 3,0,NU_NO_PREEMPT, NU_NO_START)
                != NU_SUCCESS)
    {
        NU_Deallocate_Memory(queue_space);
        NU_Deallocate_Memory(snmp_timer_mem);

        return (NU_FALSE);
    }

    else if (NU_Create_Queue(&Timer_Queue, "snmp_timers", queue_space, 20,
                            NU_FIXED_SIZE, 1, NU_FIFO) != NU_SUCCESS)
    {
        NU_Deallocate_Memory(queue_space);
        NU_Delete_Task(&SNMP_Timer_Handler);
        NU_Deallocate_Memory(snmp_timer_mem);

        return (NU_FALSE);
    }

    else
#endif

    if (NU_Allocate_Memory(&System_Memory, (VOID**)&tmr_active,
                           sizeof(tmr_t), NU_NO_SUSPEND) != NU_SUCCESS)
        return (NU_FALSE);

    else
    {
        tmr_active = TLS_Normalize_Ptr(tmr_active);
        UTL_Zero(tmr_active, sizeof(tmr_t));
        tmr_active->forw=tmr_active;
        tmr_active->back=tmr_active;

        if (NU_Allocate_Memory(&System_Memory, (VOID**)&tp,
                               (sizeof(tmr_t) * MAX_TIMERS),
                               NU_NO_SUSPEND) != NU_SUCCESS)
        {
            if (NU_Deallocate_Memory(tmr_active) != NU_SUCCESS)
            {
                NLOG_Error_Log("SNMP: failed to deallocate memory",
                                NERR_SEVERE, __FILE__, __LINE__);
            }

#if (INCLUDE_MIB_RMON1 == NU_TRUE)
            NU_Delete_Queue(&Timer_Queue);
            NU_Deallocate_Memory(queue_space);
            NU_Delete_Task(&SNMP_Timer_Handler);
            NU_Deallocate_Memory(snmp_timer_mem);
#endif
            return (NU_FALSE);
        }

        else
        {
            tp = TLS_Normalize_Ptr(tp);
            UTL_Zero(tp, sizeof(tmr_t) * MAX_TIMERS);

            tmr_free=tp;
            tmr_free->forw=tmr_free;
            tmr_free->back=tmr_free;

            for(i=0; i < MAX_TIMERS; i++)
            {
                if (NU_Allocate_Memory(&System_Memory,
                                       (VOID**)(&(tp->tmr_ptr)),
                                       sizeof(NU_TIMER),
                                       NU_NO_SUSPEND) != NU_SUCCESS)
                {
                    if (NU_Deallocate_Memory(tmr_active) != NU_SUCCESS)
                    {
                        NLOG_Error_Log("SNMP: Failed to deallocate memory",
                                        NERR_SEVERE, __FILE__, __LINE__);
                    }

                    if (NU_Deallocate_Memory(tp) != NU_SUCCESS)
                    {
                        NLOG_Error_Log("SNMP: Failed to deallocate memory",
                                        NERR_SEVERE, __FILE__, __LINE__);
                    }

#if (INCLUDE_MIB_RMON1 == NU_TRUE)
                    NU_Delete_Queue(&Timer_Queue);
                    NU_Deallocate_Memory(queue_space);
                    NU_Delete_Task(&SNMP_Timer_Handler);
                    NU_Deallocate_Memory(snmp_timer_mem);
#endif
                    for (j = 0; j < i; j++)
                    {
                        if (NU_Deallocate_Memory(tp->tmr_ptr) != NU_SUCCESS)
                        {
                            NLOG_Error_Log("SNMP: Failed to deallocate "
                                            "memory", NERR_SEVERE,
                                            __FILE__, __LINE__);
                        }
                    }

                    return (NU_FALSE);
                }

                else
                {
                    tp->tmr_ptr = TLS_Normalize_Ptr(tp->tmr_ptr);
                    UTL_Zero(tp->tmr_ptr, sizeof(NU_TIMER));

                    x_insque( tp, tmr_free->back );
                    tp++;
                }
            }
        }
    }

#if (INCLUDE_MIB_RMON1 == NU_TRUE)
    NU_Resume_Task(&SNMP_Timer_Handler);
#endif

    return (NU_TRUE);

} /* x_timerinit */

#if (INCLUDE_MIB_RMON1 == NU_TRUE)

/************************************************************************
*
*   FUNCTION
*
*       x_timeout
*
*   DESCRIPTION
*
*       This function creates a new timer.
*
*   INPUTS
*
*       *func       The func to be associated with the new timer.
*       *arg        The arg to be associated with the new timer.
*       ticks       The initial time of the timer.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID x_timeout (VOID(*func)(VOID *), VOID *arg, INT32 ticks)
{
    STATUS    status;
    tmr_t    *tp;

    if ((tp = tmr_free->forw) != tmr_free)
    {
        x_remque(tp);

        tp->forw = tp;
        tp->back = tp;
        tp->func = func;
        tp->arg  = arg;

        ticks = (INT32)(ticks * NU_PLUS_Ticks_Per_Second / 1000);

        status = NU_Create_Timer((NU_TIMER *)tp->tmr_ptr,
                                 (CHAR *)tp->name,
                                 timeout_function,
                                 (UNSIGNED)tp, (UNSIGNED)ticks,
                                 (UNSIGNED)ticks, NU_ENABLE_TIMER);

        if (status != NU_SUCCESS)
        {
            tp->func = 0;
            tp->arg  = 0;
            x_insque( tp, tmr_free->back );
        }

        else
            x_insque( tp, tmr_active->back );
    }

} /* x_timeout */

/************************************************************************
*
*   FUNCTION
*
*       x_untimeout
*
*   DESCRIPTION
*
*       This function removes the timer associated with the func and
*       arg parameters from the system.
*
*   INPUTS
*
*       *func       A pointer to the function associated with the timer
*                   to delete.
*       *arg        A pointer to the arg associated with the timer to
*                   delete.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID x_untimeout (VOID(*func)(VOID *), VOID *arg)
{
    tmr_t    *tp;

    if (tmr_active->forw != tmr_active)
    {
        for (tp = tmr_active->forw; tp != tmr_active; tp = tp->forw)
        {
            if ( (tp->func == func) && (tp->arg == arg) )
            {
                x_remque(tp);
                tp->func = 0;
                tp->arg = 0;
                (VOID)NU_Control_Timer(tp->tmr_ptr, NU_DISABLE_TIMER);
                (VOID)NU_Delete_Timer(tp->tmr_ptr);
                x_insque(tp, tmr_free->back);
                break;
            }
        }
    }

} /* x_untimeout */

#endif

/************************************************************************
*
*   FUNCTION
*
*       x_insque
*
*   DESCRIPTION
*
*       This function adds the xq_t ein to the list of xq_t elements.
*
*   INPUTS
*
*       *ein        The element to add to the list.
*       *previn     The previous element in the list.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID x_insque (VOID *ein, VOID *previn)
{
    xq_t    *e, *prev;

    e = (xq_t *)ein;
    prev = (xq_t *)previn;

    e->xq_prev = prev;
    e->xq_next = prev->xq_next;
    prev->xq_next->xq_prev = e;
    prev->xq_next = e;

} /* x_insque */

#if (INCLUDE_MIB_RMON1 == NU_TRUE)

static  INT32       pcount = 0;
static  INT32       outcnt = 0;
static  INT32       neg;
static  INT8        fill_char;


#define PBUF(c) outcnt++; \
                *buf++ = (INT8)c


/************************************************************************
*
*   FUNCTION
*
*       x_sprintf
*
*   DESCRIPTION
*
*       This function places the contents of the global plist into buf.
*
*   INPUTS
*
*       *buf        A pointer to the buffer into which to place the new
*                   data.
*       *fmt        The flag describing the behavior of the copy.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
void x_sprintf (CHAR *buf, CHAR *fmt)
{
    register    CHAR    *p;
    register    INT32   ch;
    UINT32      ul;
    INT32       lflag, width, len, *il;
    CHAR        *pp, buff[(sizeof(long) * 8 / 3) + 1];

    for (;;)
    {
        while ((ch = *fmt++) != '%')
        {
            if (ch == '\0')
            {
                outcnt = 0;
                pcount = 0;
                *buf = '\0';
                UTL_Zero(plist,
                         (sizeof(parm_list_t) * SNMP_SPRINTF_NUM_PARMS));
                return;
            }
            PBUF(ch);
        }

        lflag = 0;
        neg = 0;
        width = 0;
        fill_char = ' ';
reswitch:
        switch (ch = *fmt++)
        {
        case 'l':
            lflag = 1;
            goto reswitch;

        case 'c':
            ch = (INT32)(plist[pcount++].charval);
            PBUF((ch & 0x7f));
            break;
        case 's':
            p = (CHAR*)plist[pcount++].charptr;
            len =(INT32) strlen(p);
            if (width && !neg)
            {
                if (width > len)
                {
                    width = width - len;

                    while (width--)
                    {
                        PBUF(' ');
                    }
                }
            }

            ch = *p++;
            while (ch)
            {
                PBUF(ch);
                ch = *p++;
            }

            if (width && neg)
            {
                if (width > len)
                {
                    width = width - len;

                    while (width--)
                    {
                        PBUF(' ');
                    }
                }
            }
            break;
        case 'n':
            il  = plist[pcount++].intptr;
            *il = outcnt;
            break;
        case 'd':
            ul = lflag ?
                plist[pcount++].longval : plist[pcount++].intval;

            if ((INT32)ul < 0)
            {
                PBUF('-');
                ul = -(INT32)ul;
            }

            pp = buff;

            do
            {
                *pp++ = "0123456789abcdef"[ul % 10];
                ul = ul / 10;
            } while (ul);

            *pp = 0;

            if (width && !neg)
            {
                if (strlen(buff) < (UINT16)width)
                {
                    width = (INT32)(width - strlen(buff));

                    while (width--)
                    {
                        PBUF(fill_char);
                    }
                }
            }

            do
            {
                PBUF(*--pp);
            } while (pp > buff);

            if (width && neg)
            {
                if (strlen(buff) < (UINT16)width)
                {
                    width = (INT32)(width - strlen(buff));

                    while (width--)
                    {
                        PBUF(' ');
                    }
                }
            }
            break;
        case 'o':
            ul = plist[pcount++].longval;
            pp = buff;

            do {
                *pp++ = "0123456789abcdef"[ul % 8];
                ul = ul / 8;
            } while (ul);

            *pp = 0;

            if (width && !neg)
            {
                if (strlen(buff) < (UINT16)width)
                {
                    width = (INT32)(width - strlen(buff));

                    while (width--)
                        PBUF(fill_char);
                }
            }

            do
            {
                PBUF(*--pp);
            } while (pp > buff);

            if (width && neg)
            {
                if (strlen(buff) < (UINT16)width)
                {
                    width = (INT32)(width - strlen(buff));

                    while (width--)
                        PBUF(' ');
                }
            }
            break;
        case 'u':
            ul = plist[pcount++].longval;
            pp = buff;

            do
            {
                *pp++ = "0123456789abcdef"[ul % 10];
                ul = ul / 10;
            } while (ul);

            *pp = 0;

            if (width && !neg)
            {
                if (strlen(buff) < (UINT16)width)
                {
                    width = (INT32)(width - strlen(buff));

                    while (width--)
                        PBUF(fill_char);
                }
            }

            do
            {
                PBUF(*--pp);
            } while (pp > buff);

            if (width && neg)
            {
                if (strlen(buff) < (UINT16)width)
                {
                    width = (INT32)(width - strlen(buff));

                    while(width--)
                        PBUF(' ');
                }
            }
            break;
        case 'x':
            ul = plist[pcount++].longval;
            pp = buff;

            do
            {
                *pp++ = "0123456789abcdef"[ul % 16];
                ul = ul / 16;
            } while (ul);

            *pp = 0;

            if (width && !neg)
            {
                if (strlen(buff) < (UINT16)width)
                {
                    width = (INT32)(width - strlen(buff));

                    while(width--)
                        PBUF(fill_char);
                }
            }

            do
            {
                PBUF(*--pp);
            } while (pp > buff);

            if (width && neg)
            {
                if (strlen(buff) < (UINT16)width)
                {
                    width = (INT32)(width - strlen(buff));

                    while(width--)
                        PBUF(' ');
                }
            }
            break;
        case '%':
            PBUF('%');
            break;
        case '-':
            neg = 1;
            goto reswitch;
        default:
            if ( (ch == '*') || ((ch >= '0') && (ch <= '9')) )
            {
                if (ch == '0')
                    fill_char = '0';

                for (;;)
                {
                    if ( (ch < '0') || (ch > '9') )
                        break;

                    width = width * 10;
                    width = width + (ch - '0');
                    ch = *fmt++;
                }

                    fmt--;
                goto reswitch;
            }

            if (lflag)
                PBUF('l');

            PBUF(ch);
            break;
        }
    }

} /* x_sprintf */

/************************************************************************
*
*   FUNCTION
*
*       x_remque
*
*   DESCRIPTION
*
*       This function removes the passed in xq_t element from the list of
*       xq_t elements.
*
*   INPUTS
*
*       *ein        A pointer to the element to remove.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID x_remque (VOID *ein)
{
    xq_t    *e;

    e = (xq_t *)ein;

    e->xq_prev->xq_next = e->xq_next;
    e->xq_next->xq_prev = e->xq_prev;

} /* x_remque */

/************************************************************************
*
*   FUNCTION
*
*       timeout_function
*
*   DESCRIPTION
*
*       This function performs the timeout function associated with the
*       tmr_t data structure arg.
*
*   INPUTS
*
*       arg     The tmr_t data structure of which to perform the timeout
*               function.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID timeout_function (UNSIGNED arg)
{
    NU_Send_To_Queue(&Timer_Queue, &arg, 1, NU_NO_SUSPEND);

} /* timeout_function */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Timer_Task
*
*   DESCRIPTION
*
*       This function actually performs the timeout function associated
*       with the tmr_t data structure arg.
*
*   INPUTS
*
*       argc    Unused
*       argv    Unused
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID SNMP_Timer_Task(UNSIGNED argc, VOID *argv)
{
    tmr_t       *tp;
    UNSIGNED    dummy;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    UNUSED_PARAMETER(argc);
    UNUSED_PARAMETER(argv);

    for(;;)
    {
        if (NU_Receive_From_Queue(&Timer_Queue, &tp, 1, &dummy,NU_SUSPEND)
                                                            == NU_SUCCESS)
        {
            if ((tp) && (tp->func))
                (*(tp->func))(tp->arg);
        }
    }

    /* NU_USER_MODE(); */

} /* SNMP_Timer_Task */


#endif




