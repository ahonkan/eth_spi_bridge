/*************************************************************************
*
*               Copyright 2012 Mentor Graphics Corporation
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
*       app_mark.c
*
*   COMPONENT
*
*       Nucleus Trace Internal
*
*   DESCRIPTION
*
*       Implements the Nucleus Trace Mark APIs
*
*************************************************************************/
#include 	<stdarg.h>
#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "services/nu_trace.h"
#include    "os/services/trace/core/trace.h"


/***********************************************************************
*
*   FUNCTION
*
*       Trace_Mark_I32
*
*   DESCRIPTION
*
*       Logs a <string, value> 2-tuple to trace a signed integer
*       value
*
***********************************************************************/
void Trace_Mark_I32(char* evt_str, signed int i32_val)
{
    int             str_size = strlen(evt_str) + 1;
    unsigned short  size = sizeof(TRACE_MARK_HDR) + str_size + sizeof(signed int);
    TRACE_MARK_HDR* p_tm;
    int             retval = -1;
    char*           p_buff;

    /* Acquire pointer to next available buffer */
    retval = Acquire_Buffer_Space(size, &p_buff);

    /* Fill up the buffer */
    if (retval > 0)
    {
        p_tm = (TRACE_MARK_HDR*) p_buff;
        Write_Packet_Header(p_tm, APP_MARK_I32, 0, size);
        memcpy(&(p_tm->payload[0]), evt_str, str_size);
        memcpy(&(p_tm->payload[str_size]), (char*)&i32_val, sizeof(signed int));
    }

    /* Release trace buffer */
    Release_Buffer_Space(retval);
}

/***********************************************************************
*
*   FUNCTION
*
*       Trace_Mark_U32
*
*   DESCRIPTION
*
*       Logs a <string, value> 2-tuple to trace a unsigned integer
*       value
*
***********************************************************************/
void Trace_Mark_U32(char* evt_str, unsigned int u32_val)
{
    int                 str_size = strlen(evt_str) + 1;
    unsigned short      size = sizeof(TRACE_MARK_HDR) + str_size + sizeof(unsigned int);
    TRACE_MARK_HDR*     p_tm;
    int                 retval = -1;
    char*               p_buff;

    /* Acquire pointer to next available buffer */
    retval = Acquire_Buffer_Space(size, &p_buff);

    /* Fill up the buffer */
    if (retval > 0)
    {
        p_tm = (TRACE_MARK_HDR*) p_buff;
        Write_Packet_Header(p_tm, APP_MARK_U32, 0, size);
        memcpy(&(p_tm->payload[0]), evt_str, str_size);
        memcpy(&(p_tm->payload[str_size]), (char*)&u32_val, sizeof(unsigned int));
    }

    /* Release trace buffer */
    Release_Buffer_Space(retval);
}

/***********************************************************************
*
*   FUNCTION
*
*       Trace_Mark_Float
*
*   DESCRIPTION
*
*       Logs a <string, value> 2-tuple to trace a float value
*
***********************************************************************/
void Trace_Mark_Float(char* evt_str, float float_val)
{
    int                 str_size = strlen(evt_str) + 1;
    unsigned short      size = sizeof(TRACE_MARK_HDR) + str_size + sizeof(float);
    TRACE_MARK_HDR*     p_tm;
    int                 retval = -1;
    char*               p_buff;

    /* Acquire pointer to next available buffer */
    retval = Acquire_Buffer_Space(size, &p_buff);

    /* Fill up the buffer */
    if (retval > 0)
    {
        p_tm = (TRACE_MARK_HDR*) p_buff;
        Write_Packet_Header(p_tm, APP_MARK_FLT, 0, size);
        memcpy(&(p_tm->payload[0]), evt_str, str_size);
        memcpy(&(p_tm->payload[str_size]), (char*)&float_val, sizeof(float));
    }

    /* Release trace buffer */
    Release_Buffer_Space(retval);
}

/***********************************************************************
*
*   FUNCTION
*
*       Trace_Mark_String
*
*   DESCRIPTION
*
*       Logs a <evt_str, str_val> 2-tuple to trace a string value
*
***********************************************************************/
void Trace_Mark_String(char* evt_str, char* str_val)
{
    int                 evt_str_size = strlen(evt_str) + 1;
    int                 str_val_size = strlen(str_val) + 1;
    unsigned short      size = sizeof(TRACE_MARK_HDR) + evt_str_size + str_val_size;
    TRACE_MARK_HDR*     p_tm;
    int                 retval = -1;
    char*               p_buff;

    /* Acquire pointer to next available buffer */
    retval = Acquire_Buffer_Space(size, &p_buff);

    /* Fill up the buffer */
    if (retval > 0)
    {
        p_tm = (TRACE_MARK_HDR*) p_buff;
        Write_Packet_Header(p_tm, APP_MARK_STR, 0, size);
        memcpy(&(p_tm->payload[0]), evt_str, evt_str_size);
        memcpy(&(p_tm->payload[evt_str_size]), (char*)str_val, str_val_size);
    }

    /* Release trace buffer */
    Release_Buffer_Space(retval);
}

/***********************************************************************
*
*   FUNCTION
*
*       Trace_Mark
*
*   DESCRIPTION
*
*       Logs a <event_type, format_specifier, values ..> n-tuple to trace a
*       variable number of arguments
*
***********************************************************************/
void Trace_Mark(char *event_type, char* format, ...)
{
#if (CFG_NU_OS_SVCS_TRACE_CORE_TRACK_TRACE_OVERHEAD == NU_TRUE)
    unsigned long long _start = NU_Get_Time_Stamp();
    unsigned long long _finish;
#endif /* (CFG_NU_OS_SVCS_TRACE_CORE_TRACK_TRACE_OVERHEAD == NU_TRUE) */

    int fmt_cnt = 0;
    int data_cnt = 0;
    va_list args;
    char buff[CFG_NU_OS_SVCS_TRACE_CORE_MAX_TRACE_PKT_SIZE / 2];
    char* p_buff = buff;
    char* p_fmt;
    char* p;
    char* p_buffer;
    signed int ival;
    unsigned int uval;
    float fval;
    int length;
    int retval;
    TRACE_MARK_HDR* p_tm;
    unsigned short size;
    int event_type_length = (strlen(event_type) + 1);
    int status = 0;

    if (NU_TRACE_APP & Gbl_Trace_Mask)
    {

        va_start(args, format);
        memset(&(buff[0]), 0, sizeof(buff));

        /* Parse format string and construct pay load buffer*/
        for (p_fmt = format; *p_fmt; p_fmt++, fmt_cnt++)
        {
            if (*p_fmt == '%')
            {
                switch (*(p_fmt + 1))
                {
                    case 's':
                    {
                        p = va_arg(args, char*);
                        length = strlen(p);
                        memcpy(p_buff, p, (length + 1));
                        data_cnt += (length + 1);
                        p_buff += (length + 1);
                        break;
                    }
                    case 'd':
                    case 'i':
                    {
                        ival = va_arg(args, int);
                        data_cnt += 4;
                        memcpy(p_buff, &ival, sizeof(int));
                        p_buff += 4;
                        break;
                    }
                    case 'u':
                    case 'x':
                    case 'X':
                    {
                        uval = va_arg(args, unsigned int);
                        data_cnt += 4;
                        memcpy(p_buff, &uval, sizeof(unsigned int));
                        p_buff += 4;
                        break;
                    }
                    case 'f':
                    {
                        fval = va_arg(args, double);
                        data_cnt += 4;
                        memcpy(p_buff, &fval, sizeof(float));
                        p_buff += 4;
                        break;
                    }
                    default:
                    {
                        /* Invalid format specifier */
                        status = -1;
                        break;
                    }
                }

            }
        }

        if (status == 0)
        {
            /* Set the size of the data block */
            size = sizeof(TRACE_MARK_HDR) + data_cnt + event_type_length + (fmt_cnt + 1);

            /* Acquire pointer to next available buffer */
            retval = Acquire_Buffer_Space(size, &p_buffer);

            /* Fill up the buffer */
            if (retval > 0)
            {
                /* Put data into the buffer */
                p_tm = (TRACE_MARK_HDR*) p_buffer;
                Write_Packet_Header(p_tm, APP_MARK_VAL, 0, size);
                memcpy(&(p_tm->payload[0]), event_type, event_type_length);
                memcpy(&(p_tm->payload[event_type_length]), format, (fmt_cnt + 1));
                memcpy(&(p_tm->payload[(event_type_length + (fmt_cnt + 1))]), &(buff[0]), data_cnt);
            }

            /* Release trace buffer */
            Release_Buffer_Space(retval);
        }
    }

#if (CFG_NU_OS_SVCS_TRACE_CORE_TRACK_TRACE_OVERHEAD == NU_TRUE)
    /* Collect latency test data */
    _finish = NU_Get_Time_Stamp();
    Log_pCU64U64(0xFE, event_type, _finish, _start);
#endif /* (CFG_NU_OS_SVCS_TRACE_CORE_TRACK_TRACE_OVERHEAD == NU_TRUE) */
}
