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
*       os_mark.c
*
*   COMPONENT
*
*       Nucleus OS Markers
*
*   DESCRIPTION
*
*       Implements the Nucleus Trace Mark APIs
*
*************************************************************************/
#include    "nucleus.h"
#include    "kernel/nu_kernel.h"
#include    "os/services/trace/core/trace.h"
#include    "services/nu_trace_os_mark.h"

/* Build configuration that enables or disables kernel markers */
#if (CFG_NU_OS_SVCS_TRACE_CORE_TRACE_SUPPORT == NU_TRUE)

#if (CFG_NU_OS_SVCS_TRACE_CORE_PC_HOTSPOT_SUPPORT == NU_TRUE)
void            *Trace_PC_Sample;
#endif /* (CFG_NU_OS_SVCS_TRACE_CORE_PC_HOTSPOT_SUPPORT == NU_TRUE) */

unsigned int	idle_flag = 0;

/***********************************************************************
*
*   MACRO
*
*       LOG_ONEPARAM
*
***********************************************************************/
#define LOG_ONEPARAM(func_name, d_type)                                                 \
void func_name(unsigned short evt_id, d_type param)                                     \
{                                                                                       \
    char*           p_buff;                                                             \
    int             retval = -1;                                                        \
    TRACE_MARK_HDR* p_pkt;                                                              \
    unsigned short  size = sizeof(TRACE_MARK_HDR) + sizeof(d_type);                     \
    retval = Acquire_Buffer_Space(size, &p_buff);                                       \
    if(retval > 0)                                                                      \
    {                                                                                   \
        p_pkt = (TRACE_MARK_HDR*)p_buff;                                                \
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id, size);                             \
        memcpy(&(p_pkt->payload[0]), (char*)&param, sizeof(d_type));                    \
    }                                                                                   \
    Release_Buffer_Space(retval);                                                       \
}

/***********************************************************************
*
*   MACRO
*
*       LOG_ONEPC
*
***********************************************************************/
#define LOG_ONEPC(func_name)                                                            \
void func_name(unsigned short evt_id, char* evt_str)                                    \
{                                                                                       \
    char*           p_buff;                                                             \
    int             retval = -1;                                                        \
    TRACE_MARK_HDR* p_pkt;                                                              \
    int             str_size = strlen(evt_str) + 1;                                     \
    retval = Acquire_Buffer_Space((TH_SIZE + str_size), &p_buff);                       \
    if(retval > 0)                                                                      \
    {                                                                                   \
        p_pkt = (TRACE_MARK_HDR*)p_buff;                                                \
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id, (TH_SIZE + str_size));             \
        memcpy(&(p_pkt->payload[0]), evt_str, str_size);                                \
    }                                                                                   \
    Release_Buffer_Space(retval);                                                       \
}

/***********************************************************************
*
*   MACRO
*
*       LOG_ONEPCONEPARAM
*
***********************************************************************/
#define LOG_ONEPCONEPARAM(func_name, d_type)                                            \
void func_name(unsigned short evt_id, char* evt_str, d_type param)                      \
{                                                                                       \
    char*           p_buff;                                                             \
    int             retval = -1;                                                        \
    TRACE_MARK_HDR* p_pkt;                                                              \
    int             str_size = strlen(evt_str) + 1;                                     \
    retval = Acquire_Buffer_Space((TH_SIZE + str_size + sizeof(d_type)), &p_buff);      \
    if(retval > 0)                                                                      \
    {                                                                                   \
        p_pkt = (TRACE_MARK_HDR*)p_buff;                                                \
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id, (TH_SIZE + str_size + sizeof(d_type)));    \
        memcpy(&(p_pkt->payload[0]), evt_str, str_size);                                \
        memcpy(&(p_pkt->payload[str_size]), (char*)&param, sizeof(d_type));              \
    }                                                                                   \
    Release_Buffer_Space(retval);                                                       \
}

/***********************************************************************
*
*   MACRO
*
*       LOG_TWOPARAMS
*
***********************************************************************/
#define LOG_TWOPARAMS(func_name, d1_type, d2_type)                                      \
void func_name(unsigned short evt_id, d1_type param1, d2_type param2)                   \
{                                                                                       \
    char*           p_buff;                                                             \
    int             retval = -1;                                                        \
    TRACE_MARK_HDR* p_pkt;                                                              \
    retval = Acquire_Buffer_Space((TH_SIZE + sizeof(d1_type) + sizeof(d2_type)), &p_buff);            \
    if(retval > 0)                                                                      \
    {                                                                                   \
        p_pkt = (TRACE_MARK_HDR*)p_buff;                                                \
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id, (TH_SIZE + sizeof(d1_type) + sizeof(d2_type)));  \
        memcpy(&(p_pkt->payload[0]), (char*)&param1, sizeof(d1_type));                  \
        memcpy(&(p_pkt->payload[sizeof(d1_type)]), (char*)&param2, sizeof(d2_type));    \
    }                                                                                   \
    Release_Buffer_Space(retval);                                                       \
}

/***********************************************************************
*
*   MACRO
*
*       LOG_THREEPARAMS
*
***********************************************************************/
#define LOG_THREEPARAMS(func_name, d1_type, d2_type, d3_type)                               \
void func_name(unsigned short evt_id, d1_type param1, d2_type param2, d3_type param3)       \
{                                                                                           \
    char*           p_buff;                                                                 \
    int             retval = -1;                                                            \
    TRACE_MARK_HDR* p_pkt;                                                                  \
    retval = Acquire_Buffer_Space((TH_SIZE + sizeof(d1_type) + sizeof(d2_type) + sizeof(d3_type)), &p_buff);            \
    if(retval > 0)                                                                          \
    {                                                                                       \
        p_pkt = (TRACE_MARK_HDR*)p_buff;                                                    \
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id,(TH_SIZE + sizeof(d1_type) +            \
                            sizeof(d2_type) + sizeof(d3_type)));                            \
        memcpy(&(p_pkt->payload[0]), (char*)&param1, sizeof(d1_type));                      \
        memcpy(&(p_pkt->payload[sizeof(d1_type)]), (char*)&param2, sizeof(d2_type));        \
        memcpy(&(p_pkt->payload[(sizeof(d1_type)+sizeof(d2_type))]), (char*)&param3, sizeof(d3_type));                  \
    }                                                                                       \
    Release_Buffer_Space(retval);                                                           \
}

/***********************************************************************
*
*   MACRO
*
*       LOG_FOURPARAMS
*
***********************************************************************/
#define LOG_FOURPARAMS(func_name, d1_type, d2_type, d3_type, d4_type)                                                   \
void func_name(unsigned short evt_id, d1_type param1, d2_type param2, d3_type param3, d4_type param4)                   \
{                                                                                           \
    char*           p_buff;                                                                 \
    int             retval = -1;                                                            \
    TRACE_MARK_HDR* p_pkt;                                                                  \
    retval = Acquire_Buffer_Space((TH_SIZE + sizeof(d1_type) + sizeof(d2_type) +            \
                                   sizeof(d3_type) + sizeof(d4_type)), &p_buff);            \
    if(retval > 0)                                                                          \
    {                                                                                       \
        p_pkt = (TRACE_MARK_HDR*)p_buff;                                                    \
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id,(TH_SIZE + sizeof(d1_type) +            \
                            sizeof(d2_type) + sizeof(d3_type) + sizeof(d4_type)));          \
        memcpy(&(p_pkt->payload[0]), (char*)&param1, sizeof(d1_type));                      \
        memcpy(&(p_pkt->payload[sizeof(d1_type)]), (char*)&param2, sizeof(d2_type));        \
        memcpy(&(p_pkt->payload[(sizeof(d1_type)+sizeof(d2_type))]), (char*)&param3, sizeof(d3_type));                  \
        memcpy(&(p_pkt->payload[(sizeof(d1_type)+sizeof(d2_type)+sizeof(d3_type))]), (char*)&param4, sizeof(d4_type));  \
    }                                                                                       \
    Release_Buffer_Space(retval);                                                           \
}

/***********************************************************************
*
*   MACRO
*
*       LOG_FIVEPARAMS
*
***********************************************************************/
#define LOG_FIVEPARAMS(func_name, d1_type, d2_type, d3_type, d4_type, d5_type)                                          \
void func_name(unsigned short evt_id, d1_type param1, d2_type param2, d3_type param3, d4_type param4, d5_type param5)   \
{                                                                                           \
    char*           p_buff;                                                                 \
    int             retval = -1;                                                            \
    TRACE_MARK_HDR* p_pkt;                                                                  \
    retval = Acquire_Buffer_Space((TH_SIZE + sizeof(d1_type) + sizeof(d2_type) +            \
                                   sizeof(d3_type) + sizeof(d4_type) + sizeof(d5_type)),    \
                                   &p_buff);                                                \
    if(retval > 0)                                                                          \
    {                                                                                       \
        p_pkt = (TRACE_MARK_HDR*)p_buff;                                                    \
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id,(TH_SIZE + sizeof(d1_type) +            \
                            sizeof(d2_type) + sizeof(d3_type) + sizeof(d4_type) +           \
                            sizeof(d5_type)));                                              \
        memcpy(&(p_pkt->payload[0]), (char*)&param1, sizeof(d1_type));                      \
        memcpy(&(p_pkt->payload[sizeof(d1_type)]), (char*)&param2, sizeof(d2_type));        \
        memcpy(&(p_pkt->payload[(sizeof(d1_type)+sizeof(d2_type))]), (char*)&param3, sizeof(d3_type));                  \
        memcpy(&(p_pkt->payload[(sizeof(d1_type)+sizeof(d2_type)+sizeof(d3_type))]), (char*)&param4, sizeof(d4_type));  \
        memcpy(&(p_pkt->payload[(sizeof(d1_type)+sizeof(d2_type)+sizeof(d3_type)+sizeof(d4_type))]), (char*)&param5, sizeof(d5_type));  \
    }                                                                                       \
    Release_Buffer_Space(retval);                                                           \
}

/***********************************************************************
*
*   MACRO
*
*       LOG_SIXPARAMS
*
***********************************************************************/
#define LOG_SIXPARAMS(func_name, d1_type, d2_type, d3_type, d4_type, d5_type, d6_type)                                                 \
void func_name(unsigned short evt_id, d1_type param1, d2_type param2, d3_type param3, d4_type param4, d5_type param5, d6_type param6)  \
{                                                                                           \
    char*           p_buff;                                                                 \
    int             retval = -1;                                                            \
    TRACE_MARK_HDR* p_pkt;                                                                  \
    retval = Acquire_Buffer_Space((TH_SIZE + sizeof(d1_type) + sizeof(d2_type) +            \
                                   sizeof(d3_type) + sizeof(d4_type) + sizeof(d5_type) +    \
                                   sizeof(d6_type)), &p_buff);                              \
    if(retval > 0)                                                                          \
    {                                                                                       \
        p_pkt = (TRACE_MARK_HDR*)p_buff;                                                    \
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id,(TH_SIZE + sizeof(d1_type) +            \
                            sizeof(d2_type) + sizeof(d3_type) + sizeof(d4_type) +           \
                            sizeof(d5_type) + sizeof(d6_type)));                            \
        memcpy(&(p_pkt->payload[0]), (char*)&param1, sizeof(d1_type));                      \
        memcpy(&(p_pkt->payload[sizeof(d1_type)]), (char*)&param2, sizeof(d2_type));        \
        memcpy(&(p_pkt->payload[(sizeof(d1_type)+sizeof(d2_type))]), (char*)&param3, sizeof(d3_type));                  \
        memcpy(&(p_pkt->payload[(sizeof(d1_type)+sizeof(d2_type)+sizeof(d3_type))]), (char*)&param4, sizeof(d4_type));  \
        memcpy(&(p_pkt->payload[(sizeof(d1_type)+sizeof(d2_type)+sizeof(d3_type)+sizeof(d4_type))]), (char*)&param5, sizeof(d5_type));                  \
        memcpy(&(p_pkt->payload[(sizeof(d1_type)+sizeof(d2_type)+sizeof(d3_type)+sizeof(d4_type)+sizeof(d5_type))]), (char*)&param6, sizeof(d6_type));  \
    }                                                                                       \
    Release_Buffer_Space(retval);                                                           \
}

/***********************************************************************
*
*   MACRO
*
*       LOG_SEVENPARAMS
*
***********************************************************************/
#define LOG_SEVENPARAMS(func_name, d1_type, d2_type, d3_type, d4_type, d5_type, d6_type, d7_type)                                                       \
void func_name(unsigned short evt_id, d1_type param1, d2_type param2, d3_type param3, d4_type param4, d5_type param5, d6_type param6, d7_type param7)   \
{                                                                                           \
    char*           p_buff;                                                                 \
    int             retval = -1;                                                            \
    TRACE_MARK_HDR* p_pkt;                                                                  \
    retval = Acquire_Buffer_Space((TH_SIZE + sizeof(d1_type) + sizeof(d2_type) +            \
                                   sizeof(d3_type) + sizeof(d4_type) + sizeof(d5_type) +    \
                                   sizeof(d6_type) + sizeof(d7_type)), &p_buff);            \
    if(retval > 0)                                                                          \
    {                                                                                       \
        p_pkt = (TRACE_MARK_HDR*)p_buff;                                                    \
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id,(TH_SIZE + sizeof(d1_type) +            \
                            sizeof(d2_type) + sizeof(d3_type) + sizeof(d4_type) +           \
                            sizeof(d5_type) + sizeof(d6_type) + sizeof(d7_type)));          \
        memcpy(&(p_pkt->payload[0]), (char*)&param1, sizeof(d1_type));                      \
        memcpy(&(p_pkt->payload[sizeof(d1_type)]), (char*)&param2, sizeof(d2_type));        \
        memcpy(&(p_pkt->payload[(sizeof(d1_type)+sizeof(d2_type))]), (char*)&param3, sizeof(d3_type));                  \
        memcpy(&(p_pkt->payload[(sizeof(d1_type)+sizeof(d2_type)+sizeof(d3_type))]), (char*)&param4, sizeof(d4_type));  \
        memcpy(&(p_pkt->payload[(sizeof(d1_type)+sizeof(d2_type)+sizeof(d3_type)+sizeof(d4_type))]), (char*)&param5, sizeof(d5_type));                  \
        memcpy(&(p_pkt->payload[(sizeof(d1_type)+sizeof(d2_type)+sizeof(d3_type)+sizeof(d4_type)+sizeof(d5_type))]), (char*)&param6, sizeof(d6_type));  \
        memcpy(&(p_pkt->payload[(sizeof(d1_type)+sizeof(d2_type)+sizeof(d3_type)+sizeof(d4_type)+sizeof(d5_type)+sizeof(d6_type))]), (char*)&param7, sizeof(d7_type));  \
    }                                                                                       \
    Release_Buffer_Space(retval);                                                           \
}

/***********************************************************************
*
*   MACRO
*
*       LOG_EIGHTPARAMS
*
***********************************************************************/
#define LOG_EIGHTPARAMS(func_name, d1_type, d2_type, d3_type, d4_type, d5_type, d6_type, d7_type, d8_type)                                                              \
void func_name(unsigned short evt_id, d1_type param1, d2_type param2, d3_type param3, d4_type param4, d5_type param5, d6_type param6, d7_type param7, d8_type param8)   \
{                                                                                           \
    char*           p_buff;                                                                 \
    int             retval = -1;                                                            \
    TRACE_MARK_HDR* p_pkt;                                                                  \
    retval = Acquire_Buffer_Space((TH_SIZE + sizeof(d1_type) + sizeof(d2_type) +            \
                                   sizeof(d3_type) + sizeof(d4_type) + sizeof(d5_type) +    \
                                   sizeof(d6_type) + sizeof(d7_type) + sizeof(d8_type)),    \
                                   &p_buff);                                                \
    if(retval > 0)                                                                          \
    {                                                                                       \
        p_pkt = (TRACE_MARK_HDR*)p_buff;                                                    \
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id,(TH_SIZE + sizeof(d1_type) +            \
                            sizeof(d2_type) + sizeof(d3_type) + sizeof(d4_type) +           \
                            sizeof(d5_type) + sizeof(d6_type) + sizeof(d7_type) +           \
                            sizeof(d8_type)));                                              \
        memcpy(&(p_pkt->payload[0]), (char*)&param1, sizeof(d1_type));                      \
        memcpy(&(p_pkt->payload[sizeof(d1_type)]), (char*)&param2, sizeof(d2_type));        \
        memcpy(&(p_pkt->payload[(sizeof(d1_type)+sizeof(d2_type))]), (char*)&param3, sizeof(d3_type));                  \
        memcpy(&(p_pkt->payload[(sizeof(d1_type)+sizeof(d2_type)+sizeof(d3_type))]), (char*)&param4, sizeof(d4_type));  \
        memcpy(&(p_pkt->payload[(sizeof(d1_type)+sizeof(d2_type)+sizeof(d3_type)+sizeof(d4_type))]), (char*)&param5, sizeof(d5_type));                  \
        memcpy(&(p_pkt->payload[(sizeof(d1_type)+sizeof(d2_type)+sizeof(d3_type)+sizeof(d4_type)+sizeof(d5_type))]), (char*)&param6, sizeof(d6_type));  \
        memcpy(&(p_pkt->payload[(sizeof(d1_type)+sizeof(d2_type)+sizeof(d3_type)+sizeof(d4_type)+sizeof(d5_type)+sizeof(d6_type))]), (char*)&param7, sizeof(d7_type));                  \
        memcpy(&(p_pkt->payload[(sizeof(d1_type)+sizeof(d2_type)+sizeof(d3_type)+sizeof(d4_type)+sizeof(d5_type)+sizeof(d6_type)+sizeof(d7_type))]), (char*)&param8, sizeof(d8_type));  \
    }                                                                                       \
    Release_Buffer_Space(retval);                                                           \
}

/***********************************************************************
*
*   FUNCTION
*
*       Log_pV
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, void* pV> record
*
***********************************************************************/
LOG_ONEPARAM(Log_pV, void*)

/***********************************************************************
*
*   FUNCTION
*
*       Log_I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, int int_val> record
*
***********************************************************************/
LOG_ONEPARAM(Log_I, int)

/***********************************************************************
*
*   FUNCTION
*
*       Log_U32
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, unsigned long u32> record
*
***********************************************************************/
LOG_ONEPARAM(Log_U32, unsigned long)

/***********************************************************************
*
*   FUNCTION
*
*       Log_I32
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, signed long i32> record
*
***********************************************************************/
LOG_ONEPARAM (Log_I32, signed long)

/***********************************************************************
*
*   FUNCTION
*
*       Log_U8
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, unsigned char u8> record
*
***********************************************************************/
LOG_ONEPARAM(Log_U8, unsigned char)

/***********************************************************************
*
*   FUNCTION
*
*       Log_U16
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, unsigned short u16> record
*
***********************************************************************/
LOG_ONEPARAM(Log_U16, unsigned short)

/***********************************************************************
*
*   FUNCTION
*
*       Log_pC
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, char* pC> record
*
***********************************************************************/
LOG_ONEPC(Log_pC)

/***********************************************************************
*
*   FUNCTION
*
*       Log_pCI
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, char* pC, int status> record
*
***********************************************************************/
LOG_ONEPCONEPARAM(Log_pCI, int)

/***********************************************************************
*
*   FUNCTION
*
*       Log_pCU32
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, char* pC, unsigned long u32> record
*
***********************************************************************/
LOG_ONEPCONEPARAM(Log_pCU32, unsigned long)

/***********************************************************************
*
*   FUNCTION
*
*       Log_II
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, int int_val1, int int_val2> record
*
***********************************************************************/
LOG_TWOPARAMS(Log_II, int, int)

/***********************************************************************
*
*   FUNCTION
*
*       Log_U32I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, unsigned long u32, int status> record
*
***********************************************************************/
LOG_TWOPARAMS(Log_U32I, unsigned long, int)

/***********************************************************************
*
*   FUNCTION
*
*       Log_pVU32
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, void* pV, unsigned long u32> record
*
***********************************************************************/
LOG_TWOPARAMS(Log_pVU32, void*, unsigned long)

/***********************************************************************
*
*   FUNCTION
*
*       Log_pVI
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, void* pV, int status> record
*
***********************************************************************/
LOG_TWOPARAMS(Log_pVI, void*, int)

/***********************************************************************
*
*   FUNCTION
*
*       Log_pVpV
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, void* pV1, void* pV2> record
*
***********************************************************************/
LOG_TWOPARAMS(Log_pVpV, void*, void*)

/***********************************************************************
*
*   FUNCTION
*
*       Log_U8I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, unsigned char u8, int status> record
*
***********************************************************************/
LOG_TWOPARAMS(Log_U8I, unsigned char, int)

/***********************************************************************
*
*   FUNCTION
*
*       Log_I32I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, signed long i32, int status>
*       record
*
***********************************************************************/
LOG_TWOPARAMS(Log_I32I, signed long, int)

/***********************************************************************
*
*   FUNCTION
*
*       Log_pVpVI
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, void* pV1, void* pV2, int status> record
*
***********************************************************************/
LOG_THREEPARAMS(Log_pVpVI, void*, void*, int)

/***********************************************************************
*
*   FUNCTION
*
*       Log_U32U32I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, unsigned long u32_1,
*               unsigned long u32_2, int status> record
*
***********************************************************************/
LOG_THREEPARAMS(Log_U32U32I, unsigned long, unsigned long, int)

/***********************************************************************
*
*   FUNCTION
*
*       Log_U16II
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, unsigned short u16, int sd,
*               int status> record
*
***********************************************************************/
LOG_THREEPARAMS(Log_U16II, unsigned short, int, int)

/***********************************************************************
*
*   FUNCTION
*
*       Log_pVU8I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, void* pV, unsigned char u8, int status> record
*
***********************************************************************/
LOG_THREEPARAMS(Log_pVU8I, void*, unsigned char, int)

/***********************************************************************
*
*   FUNCTION
*
*       Log_pVU32I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, void* pV, unsigned long u32, int status> record
*
***********************************************************************/
LOG_THREEPARAMS(Log_pVU32I, void*, unsigned long, int)

/***********************************************************************
*
*   FUNCTION
*
*       Log_pVU8U8
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, void* pV, unsigned char u8_1, unsigned char u8_2> record
*
***********************************************************************/
LOG_THREEPARAMS(Log_pVU8U8, void*, unsigned char, unsigned char)

/***********************************************************************
*
*   FUNCTION
*
*       Log_pVU32U32
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, void* pV, unsigned long u32_1, unsigned long u32_2> record
*
***********************************************************************/
LOG_THREEPARAMS(Log_pVU32U32, void*, unsigned long, unsigned long)

/***********************************************************************
*
*   FUNCTION
*
*       Log_I32I16I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, signed long i32_1,
*               signed short i16, int fd> record
*
***********************************************************************/
LOG_THREEPARAMS(Log_I32I16I, signed long, signed short, int)

/***********************************************************************
*
*   FUNCTION
*
*       Log_I32U32I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, signed long i32, unsigned long u32,
*       int status> record
*
***********************************************************************/
LOG_THREEPARAMS(Log_I32U32I, signed long, unsigned long, int)

/***********************************************************************
*
*   FUNCTION
*
*       Log_I32II
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, signed long i32, int fd, int status>
*       record
*
***********************************************************************/
LOG_THREEPARAMS(Log_I32II, signed long, int, int)

/***********************************************************************
*
*   FUNCTION
*
*       Log_I32II32
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, signed long i32_1, int fd,
*       signed long i32_2> record
*
***********************************************************************/
LOG_THREEPARAMS(Log_I32II32, signed long, int, signed long)

/***********************************************************************
*
*   FUNCTION
*
*       Log_U16II32
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, unsigned short u16, int sd,
*               signed long i32> record
*
***********************************************************************/
LOG_THREEPARAMS(Log_U16II32, unsigned short, int, signed long)

/***********************************************************************
*
*   FUNCTION
*
*       Log_I32I16II32
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, signed long i32_1,
*               signed short i16, int fd, signed long i32_2> record
*
***********************************************************************/
LOG_FOURPARAMS(Log_I32I16II32, signed long, signed short, int, signed long)

/***********************************************************************
*
*   FUNCTION
*
*       Log_pVU32U8I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, void* pV, unsigned long u32,
*               unsigned char u8, int status> record
*
***********************************************************************/
LOG_FOURPARAMS(Log_pVU32U8I, void*, unsigned long, unsigned char, int)

/***********************************************************************
*
*   FUNCTION
*
*       Log_pVpVU8I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, void* pV1, void* pV2,
*               unsigned char u8, int status> record
*
***********************************************************************/
LOG_FOURPARAMS(Log_pVpVU8I, void*, void*, unsigned char, int)

/***********************************************************************
*
*   FUNCTION
*
*       Log_pVpVU32I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, signed short i16_1,
*               signed short i16_2, signed short i16_3, int status> record
*
***********************************************************************/
LOG_FOURPARAMS(Log_pVpVU32I, void*, void*, unsigned long, int)

/***********************************************************************
*
*   FUNCTION
*
*       Log_I16I16I16I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, void* pV1, void* pV2,
*               unsigned long u32, int status> record
*
***********************************************************************/
LOG_FOURPARAMS(Log_I16I16I16I, signed short, signed short, signed short, int)

/***********************************************************************
*
*   FUNCTION
*
*       Log_pVpVU32U8I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, void* pV1, void* pV2,
*               unsigned long u32, unsigned char u8, int status> record
*
***********************************************************************/
LOG_FIVEPARAMS(Log_pVpVU32U8I, void*, void*, unsigned long, unsigned char, int)

/***********************************************************************
*
*   FUNCTION
*
*       Log_pVpVU32U32I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, void* pV1, void* pV2,
*               unsigned long u32_1, unsigned long u32_2, int status> record
*
***********************************************************************/
LOG_FIVEPARAMS(Log_pVpVU32U32I, void*, void*, unsigned long, unsigned long, int)

/***********************************************************************
*
*   FUNCTION
*
*       Log_pVpVpVU32I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, void* pV1, void* pV2, void* pV3,
*               unsigned long u32_1, int status> record
*
***********************************************************************/
LOG_FIVEPARAMS(Log_pVpVpVU32I, void*, void*, void*, unsigned long, int)

/***********************************************************************
*
*   FUNCTION
*
*       Log_I32pVU32U8I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, signed long i32, void *pV,
*       unsigned long u32, unsigned char u8, int status> record
*
***********************************************************************/
LOG_FIVEPARAMS(Log_I32pVU32U8I, signed long, void*, unsigned long, unsigned char, int)

/***********************************************************************
*
*   FUNCTION
*
*       Log_U8U8U16U8U32
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, unsigned char u8_1,
*               unsigned char u8_2, unsigned short u16,
*               unsigned char u8_3, unsigned long u32> record
*
***********************************************************************/
LOG_FIVEPARAMS(Log_U8U8U16U8U32, unsigned char, unsigned char, unsigned short, unsigned char, unsigned long)

/***********************************************************************
*
*   FUNCTION
*
*       Log_pVpVpVU32U32I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, void* pV1, void* pV2, void* pV3,
*               unsigned long u32_1, unsigned long u32_2, int status> record
*
***********************************************************************/
LOG_SIXPARAMS(Log_pVpVpVU32U32I, void*, void*, void*, unsigned long, unsigned long, int)

/***********************************************************************
*
*   FUNCTION
*
*       Log_pVpVU32U32U8I
*
*   DESCRIPTION
*
*       Logs a <unsigned long evt_id, void* pV1, void* pV2, unsigned long u32_1,
*               unsigned long u32_2, unsigned char u8, int status> record
*
***********************************************************************/
LOG_SIXPARAMS(Log_pVpVU32U32U8I, void*, void*, unsigned long, unsigned long, unsigned char, int)

/***********************************************************************
*
*   FUNCTION
*
*       Log_pVU32U32U32U8I
*
*   DESCRIPTION
*
*       Logs a <unsigned long evt_id, void* pV1, void* pV, unsigned long u32_1,
*               unsigned long u32_2, unsigned long u32_3, unsigned char u8,
*               int status> record
*
***********************************************************************/
LOG_SIXPARAMS(Log_pVU32U32U32U8I, void*, unsigned long, unsigned long, unsigned long, unsigned char, int)

/***********************************************************************
*
*   FUNCTION
*
*       Log_pVpVU32U32U32I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, void* pV1, void* pV2, void* pV3,
*               unsigned long u32_1, unsigned long u32_2, unsigned char u8_1,
*               unsigned char u8_2, int status> record
*
***********************************************************************/
LOG_SIXPARAMS(Log_pVpVU32U32U32I, void*, void*, unsigned long, unsigned long, unsigned long, int)

/***********************************************************************
*
*   FUNCTION
*
*       Log_pVpVpVU32U32U32I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, void* pV1, void* pV2, void* pV3,
*               unsigned long u32_1, unsigned long u32_2, unsigned long u32_3,
*               int status> record
*
***********************************************************************/
LOG_SEVENPARAMS(Log_pVpVpVU32U32U32I, void*, void*, void*, unsigned long, unsigned long, unsigned long, int)

/***********************************************************************
*
*   FUNCTION
*
*       Log_pVpVpVU32U32U8U8I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, void* pV1, void* pV2, void* pV3,
*               unsigned long u32_1, unsigned long u32_2, unsigned char u8_1,
*               unsigned char u8_2, int status> record
*
***********************************************************************/
LOG_EIGHTPARAMS(Log_pVpVpVU32U32U8U8I, void*, void*, void*, unsigned long, unsigned long, unsigned char, unsigned char, int)

/***********************************************************************
*
*   FUNCTION
*
*       Log_pVpVpVU32U32U32U32I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, void* pV1, void* pV2, void* pV3,
*               unsigned long u32_1, unsigned long u32_2, unsigned long u32_3,
*               unsigned long u32_4, int status> record
*
***********************************************************************/
LOG_EIGHTPARAMS(Log_pVpVpVU32U32U32U32I, void*, void*, void*, unsigned long, unsigned long, unsigned long, unsigned long, int)

/***********************************************************************
*
*   FUNCTION
*
*       Log_V
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id> record
*
***********************************************************************/
void Log_V(unsigned short evt_id)
{
    char*           p_buff;
    int             retval = -1;
    TRACE_MARK_HDR* p_pkt;

    /* Acquire pointer to next available buffer */
    retval = Acquire_Buffer_Space(TH_SIZE, &p_buff);

    /* Fill up the buffer */
    if(retval > 0)
    {
        p_pkt = (TRACE_MARK_HDR*) p_buff;
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id, TH_SIZE);
    }

    /* Release trace buffer */
    Release_Buffer_Space(retval);
}

/***********************************************************************
*
*   FUNCTION
*
*       Log_pVpCI
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, void* pV, char* pC, int status> record
*
***********************************************************************/
void Log_pVpCI(unsigned short evt_id, void* pV, char* pC, int status)
{
    int             str_size = (strlen(pC) + 1);
    int             pkt_size = (TH_pVI_SIZE + str_size);
    char*           p_buff;
    int             retval = -1;
    TRACE_MARK_HDR* p_pkt;

    /* Acquire pointer to next available buffer */
    retval = Acquire_Buffer_Space(pkt_size, &p_buff);

    /* Fill up the buffer */
    if(retval > 0)
    {
        p_pkt = (TRACE_MARK_HDR*)p_buff;
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id, pkt_size);
        memcpy(&(p_pkt->payload[0]), (char*)&pV, pV_SIZE);
        memcpy(&(p_pkt->payload[pV_SIZE]), (char*)pC, str_size);
        memcpy(&(p_pkt->payload[(pV_SIZE+str_size)]), (char*)&status, I_SIZE);
    }

    /* Release trace buffer */
    Release_Buffer_Space(retval);
}

/***********************************************************************
*
*   FUNCTION
*
*       Log_pVpCU32U8I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, void* pV, char* pC, unsigned long u32,
*               unsigned char u8, int status> record
*
***********************************************************************/
void    Log_pVpCU32U8I(unsigned short evt_id, void* pV, char* pC, unsigned long u32,
                             unsigned char u8, int status)
{
    int             str_size = (strlen(pC) + 1);
    char*           p_buff;
    int             retval = -1;
    TRACE_MARK_HDR* p_pkt;

    /* Acquire pointer to next available buffer */
    retval = Acquire_Buffer_Space((TH_pVU32U8I_SIZE+str_size), &p_buff);

    /* Fill up the buffer */
    if(retval > 0)
    {
        p_pkt = (TRACE_MARK_HDR*)p_buff;
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id, (TH_pVU32U8I_SIZE+str_size));
        memcpy(&(p_pkt->payload[0]), (char*)&pV, pV_SIZE);
        memcpy(&(p_pkt->payload[pV_SIZE]), (char*)pC, str_size);
        memcpy(&(p_pkt->payload[(pV_SIZE+str_size)]), (char*)&u32, UL_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+str_size+UL_SIZE)]), (char*)&u8, UC_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+str_size+UL_SIZE+UC_SIZE)]), (char*)&status, I_SIZE);
    }

    /* Release trace buffer */
    Release_Buffer_Space(retval);
}

/***********************************************************************
*
*   FUNCTION
*
*       Log_pVpCU8I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, void* pV, char* pC,
*               unsigned long u8, int status> record
*
***********************************************************************/
void    Log_pVpCU8I(unsigned short evt_id, void* pV, char* pC,
                          unsigned char u8, int status)
{
    int             str_size = (strlen(pC) + 1);
    char*           p_buff;
    int             retval = -1;
    TRACE_MARK_HDR* p_pkt;

    /* Acquire pointer to next available buffer */
    retval = Acquire_Buffer_Space((TH_pVU8I_SIZE+str_size), &p_buff);

    /* Fill up the buffer */
    if(retval > 0)
    {
        p_pkt = (TRACE_MARK_HDR*)p_buff;
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id, (TH_pVU8I_SIZE+str_size));
        memcpy(&(p_pkt->payload[0]), (char*)&pV, pV_SIZE);
        memcpy(&(p_pkt->payload[pV_SIZE]), (char*)pC, str_size);
        memcpy(&(p_pkt->payload[(pV_SIZE+str_size)]), (char*)&u8, UC_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+str_size+UC_SIZE)]), (char*)&status, I_SIZE);
    }

    /* Release trace buffer */
    Release_Buffer_Space(retval);
}

/***********************************************************************
*
*   FUNCTION
*
*       Log_pVpCU32I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, void* pV, char* pC,
*               unsigned long u32, int status> record
*
***********************************************************************/
void    Log_pVpCU32I(unsigned short evt_id, void* pV, char* pC,
                           unsigned long u32, int status)
{
    int             str_size = (strlen(pC) + 1);
    char*           p_buff;
    int             retval = -1;
    TRACE_MARK_HDR* p_pkt;

    /* Acquire pointer to next available buffer */
    retval = Acquire_Buffer_Space((TH_pVU32I_SIZE+str_size), &p_buff);

    /* Fill up the buffer */
    if(retval > 0)
    {
        p_pkt = (TRACE_MARK_HDR*)p_buff;
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id, (TH_pVU32I_SIZE+str_size));
        memcpy(&(p_pkt->payload[0]), (char*)&pV, pV_SIZE);
        memcpy(&(p_pkt->payload[pV_SIZE]), (char*)pC, str_size);
        memcpy(&(p_pkt->payload[(pV_SIZE+str_size)]), (char*)&u32, UL_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+str_size+UL_SIZE)]), (char*)&status, I_SIZE);
    }

    /* Release trace buffer */
    Release_Buffer_Space(retval);
}

/***********************************************************************
*
*   FUNCTION
*
*       Log_pVpVpCU32I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, void* pV1, void* pV2, char* pC,
*               unsigned long u32, int status> record
*
***********************************************************************/
void    Log_pVpVpCU32I(unsigned short evt_id, void* pV1, void* pV2,
                             char* pC, unsigned long u32, int status)
{
    int             str_size = (strlen(pC) + 1);
    char*           p_buff;
    int             retval = -1;
    TRACE_MARK_HDR* p_pkt;

    /* Acquire pointer to next available buffer */
    retval = Acquire_Buffer_Space((TH_pVpVU32I_SIZE+str_size), &p_buff);

    /* Fill up the buffer */
    if(retval > 0)
    {
        p_pkt = (TRACE_MARK_HDR*)p_buff;
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id, (TH_pVpVU32I_SIZE+str_size));
        memcpy(&(p_pkt->payload[0]), (char*)&pV1, pV_SIZE);
        memcpy(&(p_pkt->payload[pV_SIZE]), (char*)&pV2, pV_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE)]), (char*)pC, str_size);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE+str_size)]), (char*)&u32, UL_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE+str_size+UL_SIZE)]), (char*)&status, I_SIZE);
    }

    /* Release trace buffer */
    Release_Buffer_Space(retval);
}

/***********************************************************************
*
*   FUNCTION
*
*       Log_pVpVpCU32U8I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, void* pV1, void* pV2, char* pC,
*               unsigned long u32, unsigned char u8, int status> record
*
***********************************************************************/
void    Log_pVpVpCU32U8I(unsigned short evt_id, void* pV1, void* pV2,
                               char* pC, unsigned long u32, unsigned char u8, int status)
{
    int             str_size = (strlen(pC) + 1);
    char*           p_buff;
    int             retval = -1;
    TRACE_MARK_HDR* p_pkt;

    /* Acquire pointer to next available buffer */
    retval = Acquire_Buffer_Space((TH_pVpVU32U8I_SIZE+str_size), &p_buff);

    /* Fill up the buffer */
    if(retval > 0)
    {
        p_pkt = (TRACE_MARK_HDR*)p_buff;
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id, (TH_pVpVU32U8I_SIZE+str_size));
        memcpy(&(p_pkt->payload[0]), (char*)&pV1, pV_SIZE);
        memcpy(&(p_pkt->payload[pV_SIZE]), (char*)&pV2, pV_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE)]), (char*)pC, str_size);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE+str_size)]), (char*)&u32, UL_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE+str_size+UL_SIZE)]), (char*)&u8, UC_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE+str_size+UL_SIZE+UC_SIZE)]), (char*)&status, I_SIZE);
    }

    /* Release trace buffer */
    Release_Buffer_Space(retval);
}

/***********************************************************************
*
*   FUNCTION
*
*       Log_pVpVpVpCU32U8I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, void* pV1, void* pV2, void* pV3, char* pC,
*               unsigned long u32, unsigned char u8, int status> record
*
***********************************************************************/
void Log_pVpVpVpCU32U8I(unsigned short evt_id, void* pV1, void* pV2, void* pV3, char* pC,
                        unsigned long u32, unsigned char u8, int status)
{
    int             str_size = (strlen(pC) + 1);
    char*           p_buff;
    int             retval = -1;
    TRACE_MARK_HDR* p_pkt;

    /* Acquire pointer to next available buffer */
    retval = Acquire_Buffer_Space((TH_pVpVpVU32U8I_SIZE+str_size), &p_buff);

    /* Fill up the buffer */
    if(retval > 0)
    {
        p_pkt = (TRACE_MARK_HDR*)p_buff;
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id, (TH_pVpVpVU32U8I_SIZE+str_size));
        memcpy(&(p_pkt->payload[0]), (char*)&pV1, pV_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE)]), (char*)&pV2, pV_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE)]), (char*)&pV3, pV_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE+pV_SIZE)]), (char*)pC, str_size);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE+pV_SIZE+str_size)]), (char*)&u32, UL_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE+pV_SIZE+str_size+UL_SIZE)]), (char*)&u8, UC_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE+pV_SIZE+str_size+UL_SIZE+UC_SIZE)]), (char*)&status, I_SIZE);
    }

    /* Release trace buffer */
    Release_Buffer_Space(retval);
}

/***********************************************************************
*
*   FUNCTION
*
*       Log_pVpVpCU32U32U8I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, void* pV1, void* pV3, char* pC,
*               unsigned long u32_1, unsigned long u32_2, unsigned char u8,
*               int status> record
*
***********************************************************************/
void    Log_pVpVpCU32U32U8I(unsigned short evt_id, void* pV1, void* pV2, char* pC,
                            unsigned long u32_1, unsigned long u32_2, unsigned char u8,
                            int status)
{
    int             str_size = (strlen(pC) + 1);
    char*           p_buff;
    int             retval = -1;
    TRACE_MARK_HDR* p_pkt;

    /* Acquire pointer to next available buffer */
    retval = Acquire_Buffer_Space((TH_pVpVU32U32U8I_SIZE+str_size), &p_buff);

    /* Fill up the buffer */
    if(retval > 0)
    {
        p_pkt = (TRACE_MARK_HDR*)p_buff;
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id, (TH_pVpVU32U32U8I_SIZE+str_size));
        memcpy(&(p_pkt->payload[0]), (char*)&pV1, pV_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE)]), (char*)&pV2, pV_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE)]), (char*)pC, str_size);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE+str_size)]), (char*)&u32_1, UL_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE+str_size+UL_SIZE)]), (char*)&u32_2, UL_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE+str_size+UL_SIZE+UL_SIZE)]), (char*)&u8, UC_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE+str_size+UL_SIZE+UL_SIZE+UC_SIZE)]), (char*)&status, I_SIZE);
    }

    /* Release trace buffer */
    Release_Buffer_Space(retval);
}

/***********************************************************************
*
*   FUNCTION
*
*       Log_pVpVpCU32U32U32U8I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, void* pV1, void* pV2, char* pC,
*               unsigned long u32_1, unsigned long u32_2, unsigned long u32_3,
*               unsigned char u8, int status> record
*
***********************************************************************/
void    Log_pVpVpCU32U32U32U8I(unsigned short evt_id, void* pV1, void* pV2, char* pC,
                                     unsigned long u32_1, unsigned long u32_2, unsigned long u32_3,
                                     unsigned char u8, int status)
{
    int             str_size = (strlen(pC) + 1);
    char*           p_buff;
    int             retval = -1;
    TRACE_MARK_HDR* p_pkt;

    /* Acquire pointer to next available buffer */
    retval = Acquire_Buffer_Space((TH_pVpVU32U32U32U8I_SIZE+str_size), &p_buff);

    /* Fill up the buffer */
    if(retval > 0)
    {
        p_pkt = (TRACE_MARK_HDR*)p_buff;
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id, (TH_pVpVU32U32U32U8I_SIZE+str_size));
        memcpy(&(p_pkt->payload[0]), (char*)&pV1, pV_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE)]), (char*)&pV2, pV_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE)]), (char*)pC, str_size);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE+str_size)]), (char*)&u32_1, UL_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE+str_size+UL_SIZE)]), (char*)&u32_2, UL_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE+str_size+UL_SIZE+UL_SIZE)]), (char*)&u32_3, UL_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE+str_size+UL_SIZE+UL_SIZE+UL_SIZE)]), (char*)&u8, UC_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE+str_size+UL_SIZE+UL_SIZE+UL_SIZE+UC_SIZE)]), (char*)&status, I_SIZE);
    }

    /* Release trace buffer */
    Release_Buffer_Space(retval);
}

/***********************************************************************
*
*   FUNCTION
*
*       Log_pVpVpCU32U32U8U8I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, void* pV1, void* pV2, char* pC,
*               unsigned long u32_1, unsigned long u32_2, unsigned char u8_1,
*               unsigned char u8_2, int status> record
*
***********************************************************************/
void    Log_pVpVpCU32U32U8U8I(unsigned short evt_id, void* pV1, void* pV2, char* pC,
                                    unsigned long u32_1, unsigned long u32_2, unsigned char u8_1,
                                    unsigned char u8_2, int status)
{
    int             str_size = (strlen(pC) + 1);
    char*           p_buff;
    int             retval = -1;
    TRACE_MARK_HDR* p_pkt;

    /* Acquire pointer to next available buffer */
    retval = Acquire_Buffer_Space((TH_pVpVU32U32U8U8I_SIZE+str_size), &p_buff);

    /* Fill up the buffer */
    if(retval > 0)
    {
        p_pkt = (TRACE_MARK_HDR*)p_buff;
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id, (TH_pVpVU32U32U8U8I_SIZE+str_size));
        memcpy(&(p_pkt->payload[0]), (char*)&pV1, pV_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE)]), (char*)&pV2, pV_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE)]), (char*)pC, str_size);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE+str_size)]), (char*)&u32_1, UL_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE+str_size+UL_SIZE)]), (char*)&u32_2, UL_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE+str_size+UL_SIZE+UL_SIZE)]), (char*)&u8_1, UC_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE+str_size+UL_SIZE+UL_SIZE+UC_SIZE)]), (char*)&u8_2, UC_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE+str_size+UL_SIZE+UL_SIZE+UC_SIZE+UC_SIZE)]), (char*)&status, I_SIZE);
    }

    /* Release trace buffer */
    Release_Buffer_Space(retval);
}


/***********************************************************************
*
*   FUNCTION
*
*       Log_pVpVpVpCU32U32U8U8U8I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, void* pV1,void* pV2, void* pV3,
*               char* pC, unsigned long u32_1, unsigned long u32_2,
*               unsigned char u8_1, unsigned char u8_2, unsigned char u8_3,
*               int status> record
*
***********************************************************************/
void Log_pVpVpVpCU32U32U8U8U8I(unsigned short evt_id, void* pV1,void* pV2, void* pV3,
                                     char* pC, unsigned long u32_1, unsigned long u32_2,
                                     unsigned char u8_1, unsigned char u8_2, unsigned char u8_3,
                                     int status)
{
    int             str_size = (strlen(pC) + 1);
    char*           p_buff;
    int             retval = -1;
    TRACE_MARK_HDR* p_pkt;

    /* Acquire pointer to next available buffer */
    retval = Acquire_Buffer_Space((TH_pVpVpVU32U32U8U8U8I_SIZE+str_size), &p_buff);

    /* Fill up the buffer */
    if(retval > 0)
    {
        p_pkt = (TRACE_MARK_HDR*)p_buff;
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id, (TH_pVpVpVU32U32U8U8U8I_SIZE+str_size));
        memcpy(&(p_pkt->payload[0]), (char*)&pV1, pV_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE)]), (char*)&pV2, pV_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE)]), (char*)&pV3, pV_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE+pV_SIZE)]), (char*)pC, str_size);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE+pV_SIZE+str_size)]), (char*)&u32_1, UL_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE+pV_SIZE+str_size+UL_SIZE)]), (char*)&u32_2, UL_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE+pV_SIZE+str_size+UL_SIZE+UL_SIZE)]), (char*)&u8_1, UC_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE+pV_SIZE+str_size+UL_SIZE+UL_SIZE+UC_SIZE)]), (char*)&u8_2, UC_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE+pV_SIZE+str_size+UL_SIZE+UL_SIZE+UC_SIZE+UC_SIZE)]), (char*)&u8_3, UC_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE+pV_SIZE+str_size+UL_SIZE+UL_SIZE+UC_SIZE+UC_SIZE+UC_SIZE)]), (char*)&status, I_SIZE);
    }

    /* Release trace buffer */
    Release_Buffer_Space(retval);
}

/***********************************************************************
*
*   FUNCTION
*
*       Log_pVpVpVU32U32U32U32U32I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, void* pV1, void* pV2, void* pV3,
*               unsigned long u32_1, unsigned long u32_2, unsigned long u32_3,
*               unsigned long u32_4, unsigned long u32_5, int status> record
*
***********************************************************************/
void    Log_pVpVpVU32U32U32U32U32I(unsigned short evt_id, void* pV1, void* pV2, void* pV3,
                                         unsigned long u32_1, unsigned long u32_2, unsigned long u32_3,
                                         unsigned long u32_4, unsigned long u32_5, int status)
{
    char*           p_buff;
    int             retval = -1;
    TRACE_MARK_HDR* p_pkt;

    /* Acquire pointer to next available buffer */
    retval = Acquire_Buffer_Space(TH_pVpVpVU32U32U32U32U32I_SIZE, &p_buff);

    /* Fill up the buffer */
    if(retval > 0)
    {
        p_pkt = (TRACE_MARK_HDR*)p_buff;
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id, TH_pVpVpVU32U32U32U32U32I_SIZE);
        memcpy(&(p_pkt->payload[0]), (char*)&pV1, pV_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE)]), (char*)&pV2, pV_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE)]), (char*)&pV3, pV_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE+pV_SIZE)]), (char*)&u32_1, UL_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE+pV_SIZE+UL_SIZE)]), (char*)&u32_2, UL_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE+pV_SIZE+UL_SIZE+UL_SIZE)]), (char*)&u32_3, UL_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE+pV_SIZE+UL_SIZE+UL_SIZE+UL_SIZE)]), (char*)&u32_4, UL_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE+pV_SIZE+UL_SIZE+UL_SIZE+UL_SIZE+UL_SIZE)]), (char*)&u32_5, UL_SIZE);
        memcpy(&(p_pkt->payload[(pV_SIZE+pV_SIZE+pV_SIZE+UL_SIZE+UL_SIZE+UL_SIZE+UL_SIZE+UL_SIZE)]), (char*)&status, I_SIZE);
    }

    /* Release trace buffer */
    Release_Buffer_Space(retval);
}

/***********************************************************************
*
*   FUNCTION
*
*       Log_I32U32U8I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, signed long i32,
*       unsigned long u32, unsigned char u8, int status> record
*
***********************************************************************/
void Log_I32U32U8I(unsigned short evt_id, signed long i32,
                         unsigned long u32, unsigned char u8, int status)
{
    char*           p_buff;
    int             retval = -1;
    TRACE_MARK_HDR* p_pkt;

    /* Acquire pointer to next available buffer */
    retval = Acquire_Buffer_Space(TH_I32U32U8I_SIZE, &p_buff);

    /* Fill up the buffer */
    if(retval > 0)
    {
        p_pkt = (TRACE_MARK_HDR*)p_buff;
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id, TH_I32U32U8I_SIZE);
        memcpy(&(p_pkt->payload[0]), (char*)&i32, IL_SIZE);
        memcpy(&(p_pkt->payload[(IL_SIZE)]), (char*)&u32, UL_SIZE);
        memcpy(&(p_pkt->payload[(IL_SIZE+UL_SIZE)]), (char*)&u8, UC_SIZE);
        memcpy(&(p_pkt->payload[(IL_SIZE+UL_SIZE+UC_SIZE)]), (char*)&status, I_SIZE);
    }

    /* Release trace buffer */
    Release_Buffer_Space(retval);
}

/***********************************************************************
*
*   FUNCTION
*
*       Log_pCU32U8I32
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, char* pC, unsigned long u32,
*       unsigned char u8, signed long i32> record
*
***********************************************************************/
void Log_pCU32U8I32(unsigned short evt_id, char* pC, unsigned long u32,
                          unsigned char u8, signed long i32)
{
    int             str_size = (strlen(pC) + 1);
    char*           p_buff;
    int             retval = -1;
    TRACE_MARK_HDR* p_pkt;

    /* Acquire pointer to next available buffer */
    retval = Acquire_Buffer_Space((TH_U32U8I32_SIZE+str_size), &p_buff);

    /* Fill up the buffer */
    if(retval > 0)
    {
        p_pkt = (TRACE_MARK_HDR*)p_buff;
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id, (TH_U32U8I32_SIZE+str_size));
        memcpy(&(p_pkt->payload[0]), (char*)pC, str_size);
        memcpy(&(p_pkt->payload[(str_size)]), (char*)&u32, UL_SIZE);
        memcpy(&(p_pkt->payload[(str_size+UL_SIZE)]), (char*)&u8, UC_SIZE);
        memcpy(&(p_pkt->payload[(str_size+UL_SIZE+UC_SIZE)]), (char*)&i32, IL_SIZE);
    }

    /* Release trace buffer */
    Release_Buffer_Space(retval);
}


/***********************************************************************
*
*   FUNCTION
*
*       Log_pCU32U32I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, char* pC, unsigned long u32_1,
*       unsigned char u32_2, int status> record
*
***********************************************************************/
void Log_pCU32U32I(unsigned short evt_id, char* pC, unsigned long u32_1,
                    unsigned long u32_2, int status)
{
    int             str_size = (strlen(pC) + 1);
    char*           p_buff;
    int             retval = -1;
    TRACE_MARK_HDR* p_pkt;

    /* Acquire pointer to next available buffer */
    retval = Acquire_Buffer_Space((TH_U32U32I_SIZE+str_size), &p_buff);

    /* Fill up the buffer */
    if(retval > 0)
    {
        p_pkt = (TRACE_MARK_HDR*)p_buff;
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id, (TH_U32U32I_SIZE+str_size));
        memcpy(&(p_pkt->payload[0]), (char*)pC, str_size);
        memcpy(&(p_pkt->payload[(str_size)]), (char*)&u32_1, UL_SIZE);
        memcpy(&(p_pkt->payload[(str_size+UL_SIZE)]), (char*)&u32_2, UL_SIZE);
        memcpy(&(p_pkt->payload[(str_size+UL_SIZE+UL_SIZE)]), (char*)&status, I_SIZE);
    }

    /* Release trace buffer */
    Release_Buffer_Space(retval);
}

/***********************************************************************
*
*   FUNCTION
*
*       Log_pCU32I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, char* pC, unsigned long u32_1,
*       int status> record
*
***********************************************************************/
void Log_pCU32I(unsigned short evt_id, char* pC, unsigned long u32, int status)
{
    int             str_size = (strlen(pC) + 1);
    char*           p_buff;
    int             retval = -1;
    TRACE_MARK_HDR* p_pkt;

    /* Acquire pointer to next available buffer */
    retval = Acquire_Buffer_Space((TH_U32I_SIZE+str_size), &p_buff);

    /* Fill up the buffer */
    if(retval > 0)
    {
        p_pkt = (TRACE_MARK_HDR*)p_buff;
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id, (TH_U32I_SIZE+str_size));
        memcpy(&(p_pkt->payload[0]), (char*)pC, str_size);
        memcpy(&(p_pkt->payload[(str_size)]), (char*)&u32, UL_SIZE);
        memcpy(&(p_pkt->payload[(str_size+UL_SIZE)]), (char*)&status, I_SIZE);
    }

    /* Release trace buffer */
    Release_Buffer_Space(retval);
}

/***********************************************************************
*
*   FUNCTION
*
*       Log_pCU8I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, char* pC1, unsigned char u8,
*               int status> record
*
***********************************************************************/
void Log_pCU8I(unsigned short evt_id, char* pC, unsigned char u8, int status)
{
    int             str_size = (strlen(pC) + 1);
    char*           p_buff;
    int             retval = -1;
    TRACE_MARK_HDR* p_pkt;

    /* Acquire pointer to next available buffer */
    retval = Acquire_Buffer_Space((TH_U8I_SIZE+str_size), &p_buff);

    /* Fill up the buffer */
    if(retval > 0)
    {
        p_pkt = (TRACE_MARK_HDR*)p_buff;
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id, (TH_U8I_SIZE+str_size));
        memcpy(&(p_pkt->payload[0]), (char*)pC, str_size);
        memcpy(&(p_pkt->payload[(str_size)]), (char*)&u8, UC_SIZE);
        memcpy(&(p_pkt->payload[(str_size+UC_SIZE)]), (char*)&status, I_SIZE);
    }

    /* Release trace buffer */
    Release_Buffer_Space(retval);
}

/***********************************************************************
*
*   FUNCTION
*
*       Log_pCpU8U8U32I_pU8ArraySize
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, char* pC, unsigned char* pU8,
*               unsigned char u8, unsigned long u32, int status
*               unsigned char pu8_array_size> record
*
***********************************************************************/
void    Log_pCpU8U8U32I_pU8ArraySize(unsigned short evt_id, char* pC, unsigned char* pU8,
                                  unsigned char u8, unsigned long u32, int status,
                                  unsigned char pu8_array_size)
{
    int             str_size = (strlen(pC) + 1);
    char*           p_buff;
    int             retval = -1;
    TRACE_MARK_HDR* p_pkt;
    int             pu8_size = sizeof(unsigned char)*pu8_array_size;

    /* Acquire pointer to next available buffer */
    retval = Acquire_Buffer_Space((TH_U8U32I_SIZE+str_size+pu8_size), &p_buff);

    /* Fill up the buffer */
    if(retval > 0)
    {
        p_pkt = (TRACE_MARK_HDR*)p_buff;
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id, (TH_U8U32I_SIZE+str_size+pu8_size));
        memcpy(&(p_pkt->payload[0]), (char*)pC, str_size);
        memcpy(&(p_pkt->payload[(str_size)]), (char*)pU8, pu8_size);
        memcpy(&(p_pkt->payload[(str_size+pu8_size)]), (char*)&u8, UC_SIZE);
        memcpy(&(p_pkt->payload[(str_size+pu8_size+UC_SIZE)]), (char*)&u32, UL_SIZE);
        memcpy(&(p_pkt->payload[(str_size+pu8_size+UC_SIZE+UL_SIZE)]), (char*)&status, I_SIZE);
    }

    /* Release trace buffer */
    Release_Buffer_Space(retval);
}

/***********************************************************************
*
*   FUNCTION
*
*       Log_pCpU8pU8I_pU8ArraySize
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, char* pC, unsigned char* pU8_1,
*               unsigned char* pU8_2, int status, unsigned char pu8_array_size> record
*
***********************************************************************/
void    Log_pCpU8pU8I_pU8ArraySize(unsigned short evt_id, char* pC, unsigned char* pU8_1,
                                unsigned char* pU8_2, int status, unsigned char pu8_array_size)
{
    int             str_size = (strlen(pC) + 1);
    char*           p_buff;
    int             retval = -1;
    TRACE_MARK_HDR* p_pkt;
    int             pu8_size = sizeof(unsigned char)*pu8_array_size;

    /* Acquire pointer to next available buffer */
    retval = Acquire_Buffer_Space((TH_I_SIZE+str_size+pu8_size+pu8_size), &p_buff);

    /* Fill up the buffer */
    if(retval > 0)
    {
        p_pkt = (TRACE_MARK_HDR*)p_buff;
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id, (TH_I_SIZE+str_size+pu8_size+pu8_size));
        memcpy(&(p_pkt->payload[0]), (char*)pC, str_size);
        memcpy(&(p_pkt->payload[(str_size)]), (char*)pU8_1, pu8_size);
        memcpy(&(p_pkt->payload[(str_size+pu8_size)]), (char*)pU8_2, pu8_size);
        memcpy(&(p_pkt->payload[(str_size+pu8_size+pu8_size)]), (char*)&status, I_SIZE);
    }

    /* Release trace buffer */
    Release_Buffer_Space(retval);
}

/***********************************************************************
*
*   FUNCTION
*
*       Log_pCpU8pU8pU8I_pU8ArraySize
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, char* pC, unsigned char* pU8_1,
*               unsigned char* pU8_2, unsigned char* pU8_3, int status,
*               unsigned char pu8_array_size> record
*
***********************************************************************/
void    Log_pCpU8pU8pU8I_pU8ArraySize(unsigned short evt_id, char* pC, unsigned char* pU8_1,
                                      unsigned char* pU8_2, unsigned char* pU8_3, int status,
                                      unsigned char pu8_array_size)
{
    int             str_size = (strlen(pC) + 1);
    char*           p_buff;
    int             retval = -1;
    TRACE_MARK_HDR* p_pkt;
    int             pu8_size = sizeof(unsigned char)*pu8_array_size;

    /* Acquire pointer to next available buffer */
    retval = Acquire_Buffer_Space((TH_I_SIZE+str_size+pu8_size+pu8_size+pu8_size), &p_buff);

    /* Fill up the buffer */
    if(retval > 0)
    {
        p_pkt = (TRACE_MARK_HDR*)p_buff;
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id, (TH_I_SIZE+str_size+pu8_size+pu8_size+pu8_size));
        memcpy(&(p_pkt->payload[0]), (char*)pC, str_size);
        memcpy(&(p_pkt->payload[(str_size)]), (char*)pU8_1, pu8_size);
        memcpy(&(p_pkt->payload[(str_size+pu8_size)]), (char*)pU8_2, pu8_size);
        memcpy(&(p_pkt->payload[(str_size+pu8_size+pu8_size)]), (char*)pU8_3, pu8_size);
        memcpy(&(p_pkt->payload[(str_size+pu8_size+pu8_size+pu8_size)]), (char*)&status, I_SIZE);
    }

    /* Release trace buffer */
    Release_Buffer_Space(retval);
}

/***********************************************************************
*
*   FUNCTION
*
*       Log_pU8pU8I_pU8ArraySize
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, unsigned char* pU8_1,
*               unsigned char* pU8_2, int status, unsigned char pu8_array_size> record
*
***********************************************************************/
void    Log_pU8pU8I_pU8ArraySize(unsigned short evt_id, unsigned char* pU8_1,
                              unsigned char* pU8_2, int status, unsigned char pu8_array_size)
{
    char*           p_buff;
    int             retval = -1;
    TRACE_MARK_HDR* p_pkt;
    int             pu8_size = sizeof(unsigned char)*pu8_array_size;

    /* Acquire pointer to next available buffer */
    retval = Acquire_Buffer_Space((TH_I_SIZE+pu8_size+pu8_size), &p_buff);

    /* Fill up the buffer */
    if(retval > 0)
    {
        p_pkt = (TRACE_MARK_HDR*)p_buff;
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id, (TH_I_SIZE+pu8_size+pu8_size));
        memcpy(&(p_pkt->payload[(0)]), (char*)pU8_1, pu8_size);
        memcpy(&(p_pkt->payload[(pu8_size)]), (char*)pU8_2, pu8_size);
        memcpy(&(p_pkt->payload[(pu8_size+pu8_size)]), (char*)&status, I_SIZE);
    }

    /* Release trace buffer */
    Release_Buffer_Space(retval);
}

/***********************************************************************
*
*   FUNCTION
*
*       Log_pCpU8I_pU8ArraySize
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, char* pC, unsigned char* pU8,
                int status, unsigned char pu8_array_size> record
*
***********************************************************************/
void    Log_pCpU8I_pU8ArraySize(unsigned short evt_id, char* pC, unsigned char* pU8,
                             int status, unsigned char pu8_array_size)
{
    int             str_size = (strlen(pC) + 1);
    char*           p_buff;
    int             retval = -1;
    TRACE_MARK_HDR* p_pkt;
    int             pu8_size = sizeof(unsigned char)*pu8_array_size;

    /* Acquire pointer to next available buffer */
    retval = Acquire_Buffer_Space((TH_I_SIZE+str_size+pu8_size), &p_buff);

    /* Fill up the buffer */
    if(retval > 0)
    {
        p_pkt = (TRACE_MARK_HDR*)p_buff;
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id, (TH_I_SIZE+str_size+pu8_size));
        memcpy(&(p_pkt->payload[0]), (char*)pC, str_size);
        memcpy(&(p_pkt->payload[(str_size)]), (char*)pU8, pu8_size);
        memcpy(&(p_pkt->payload[(str_size+pu8_size)]), (char*)&status, I_SIZE);
    }

    /* Release trace buffer */
    Release_Buffer_Space(retval);
}

/***********************************************************************
*
*   FUNCTION
*
*       Log_pCpU8U8I_pU8ArraySize
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, char* pC, unsigned char* pU8,
*               unsigned char u8, int status, unsigned char pu8_array_size> record
*
***********************************************************************/
void    Log_pCpU8U8I_pU8ArraySize(unsigned short evt_id, char* pC, unsigned char* pU8,
                                  unsigned char u8, int status, unsigned char pu8_array_size)
{
    int             str_size = (strlen(pC) + 1);
    char*           p_buff;
    int             retval = -1;
    TRACE_MARK_HDR* p_pkt;
    int             pu8_size = sizeof(unsigned char)*pu8_array_size;

    /* Acquire pointer to next available buffer */
    retval = Acquire_Buffer_Space((TH_U8I_SIZE+str_size+pu8_size), &p_buff);

    /* Fill up the buffer */
    if(retval > 0)
    {
        p_pkt = (TRACE_MARK_HDR*)p_buff;
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id, (TH_U8I_SIZE+str_size+pu8_size));
        memcpy(&(p_pkt->payload[0]), (char*)pC, str_size);
        memcpy(&(p_pkt->payload[(str_size)]), (char*)pU8, pu8_size);
        memcpy(&(p_pkt->payload[(str_size+pu8_size)]), (char*)&u8, UC_SIZE);
        memcpy(&(p_pkt->payload[(str_size+pu8_size+UC_SIZE)]), (char*)&status, I_SIZE);
    }

    /* Release trace buffer */
    Release_Buffer_Space(retval);
}

/***********************************************************************
*
*   FUNCTION
*
*       Log_pCpU8pU8U8I_pU8ArraySize
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, char* pC, unsigned char* pU8_1, unsigned char* pU8_2,
*               unsigned char u8, int status, unsigned char pu8_array_size> record
*
***********************************************************************/
void    Log_pCpU8pU8U8I_pU8ArraySize(unsigned short evt_id, char* pC, unsigned char* pU8_1,
                                     unsigned char* pU8_2, unsigned char u8, int status,
                                     unsigned char pu8_array_size)
{
    int             str_size = (strlen(pC) + 1);
    char*           p_buff;
    int             retval = -1;
    TRACE_MARK_HDR* p_pkt;
    int             pu8_size = sizeof(unsigned char)*pu8_array_size;

    /* Acquire pointer to next available buffer */
    retval = Acquire_Buffer_Space((TH_U8I_SIZE+str_size+pu8_size+pu8_size), &p_buff);

    /* Fill up the buffer */
    if(retval > 0)
    {
        p_pkt = (TRACE_MARK_HDR*)p_buff;
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id, (TH_U8I_SIZE+str_size+pu8_size+pu8_size));
        memcpy(&(p_pkt->payload[0]), (char*)pC, str_size);
        memcpy(&(p_pkt->payload[(str_size)]), (char*)pU8_1, pu8_size);
        memcpy(&(p_pkt->payload[(str_size+pu8_size)]), (char*)pU8_2, pu8_size);
        memcpy(&(p_pkt->payload[(str_size+pu8_size+pu8_size)]), (char*)&u8, UC_SIZE);
        memcpy(&(p_pkt->payload[(str_size+pu8_size+pu8_size+UC_SIZE)]), (char*)&status, I_SIZE);
    }

    /* Release trace buffer */
    Release_Buffer_Space(retval);
}

/***********************************************************************
*
*   FUNCTION
*
*       Log_pCpU8I16I_pU8ArraySize
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, char* pC, unsigned char* pU8,
*               signed short i16, int status, unsigned char pu8_array_size> record
*
***********************************************************************/
void    Log_pCpU8I16I_pU8ArraySize(unsigned short evt_id, char* pC, unsigned char* pU8,
                                signed short i16, int status, unsigned char pu8_array_size)

{
    int             str_size = (strlen(pC) + 1);
    char*           p_buff;
    int             retval = -1;
    TRACE_MARK_HDR* p_pkt;
    int             pu8_size = sizeof(unsigned char)*pu8_array_size;

    /* Acquire pointer to next available buffer */
    retval = Acquire_Buffer_Space((TH_I16I_SIZE+str_size+pu8_size), &p_buff);

    /* Fill up the buffer */
    if(retval > 0)
    {
        p_pkt = (TRACE_MARK_HDR*)p_buff;
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id, (TH_I16I_SIZE+str_size+pu8_size));
        memcpy(&(p_pkt->payload[0]), (char*)pC, str_size);
        memcpy(&(p_pkt->payload[(str_size)]), (char*)pU8, pu8_size);
        memcpy(&(p_pkt->payload[(str_size+pu8_size)]), (char*)&i16, IS_SIZE);
        memcpy(&(p_pkt->payload[(str_size+pu8_size+IS_SIZE)]), (char*)&status, I_SIZE);
    }

    /* Release trace buffer */
    Release_Buffer_Space(retval);
}

/***********************************************************************
*
*   FUNCTION
*
*       Log_pCpCpCU8U32U32I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, char* pC1,
*               char* pC2, char* pC3,unsigned char u8,
*               unsigned long u32_1, unsigned long u32_2,
*               int status> record
*
***********************************************************************/
void Log_pCpCpCU8U32U32I(unsigned short evt_id, char* pC1,
                               char* pC2, char* pC3,unsigned char u8,
                               unsigned long u32_1, unsigned long u32_2,
                               int status)
{
    int             str_size1 = (strlen(pC1) + 1);
    int             str_size2 = (strlen(pC2) + 1);
    int             str_size3 = (strlen(pC3) + 1);
    char*           p_buff;
    int             retval = -1;
    TRACE_MARK_HDR* p_pkt;

    /* Acquire pointer to next available buffer */
    retval = Acquire_Buffer_Space((TH_U8U32U32I_SIZE+str_size1+str_size2+str_size3), &p_buff);

    /* Fill up the buffer */
    if(retval > 0)
    {
        p_pkt = (TRACE_MARK_HDR*)p_buff;
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id, (TH_U8U32U32I_SIZE+str_size1+str_size2+str_size3));
        memcpy(&(p_pkt->payload[0]), (char*)pC1, str_size1);
        memcpy(&(p_pkt->payload[str_size1]), (char*)pC2, str_size2);
        memcpy(&(p_pkt->payload[str_size1+str_size2]), (char*)pC3, str_size3);
        memcpy(&(p_pkt->payload[(str_size1+str_size2+str_size3)]), (char*)&u8, UC_SIZE);
        memcpy(&(p_pkt->payload[(str_size1+str_size2+str_size3+UC_SIZE)]), (char*)&u32_1, UL_SIZE);
        memcpy(&(p_pkt->payload[(str_size1+str_size2+str_size3+UC_SIZE+UL_SIZE)]), (char*)&u32_2, UL_SIZE);
        memcpy(&(p_pkt->payload[(str_size1+str_size2+str_size3+UC_SIZE+UL_SIZE+UL_SIZE)]), (char*)&status, I_SIZE);
    }

    /* Release trace buffer */
    Release_Buffer_Space(retval);
}

/***********************************************************************
*
*   FUNCTION
*
*       Log_pCpCpC
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, char* pC1, char* pC2,
*               char* pC3> record
*
***********************************************************************/
void Log_pCpCpC(unsigned short evt_id, char* pC1, char* pC2, char* pC3)
{
    int             str_size1 = (strlen(pC1) + 1);
    int             str_size2 = (strlen(pC2) + 1);
    int             str_size3 = (strlen(pC3) + 1);
    char*           p_buff;
    int             retval = -1;
    TRACE_MARK_HDR* p_pkt;

    /* Acquire pointer to next available buffer */
    retval = Acquire_Buffer_Space((TH_SIZE+str_size1+str_size2+str_size3), &p_buff);

    /* Fill up the buffer */
    if(retval > 0)
    {
        p_pkt = (TRACE_MARK_HDR*)p_buff;
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id, (TH_SIZE+str_size1+str_size2+str_size3));
        memcpy(&(p_pkt->payload[0]), (char*)pC1, str_size1);
        memcpy(&(p_pkt->payload[(str_size1)]), (char*)pC2, str_size2);
        memcpy(&(p_pkt->payload[(str_size1+str_size2)]), (char*)pC3, str_size3);
    }

    /* Release trace buffer */
    Release_Buffer_Space(retval);
}

/***********************************************************************
*
*   FUNCTION
*
*       Log_pCpCI
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, char* pC1, char* pC2,
*               int fd> record
*
***********************************************************************/
void Log_pCpCI(unsigned short evt_id, char* pC1, char* pC2, int fd)
{
    int             str_size1 = (strlen(pC1) + 1);
    int             str_size2 = (strlen(pC2) + 1);
    char*           p_buff;
    int             retval = -1;
    TRACE_MARK_HDR* p_pkt;

    /* Acquire pointer to next available buffer */
    retval = Acquire_Buffer_Space((TH_I_SIZE+str_size1+str_size2), &p_buff);

    /* Fill up the buffer */
    if(retval > 0)
    {
        p_pkt = (TRACE_MARK_HDR*)p_buff;
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id, (TH_I_SIZE+str_size1+str_size2));
        memcpy(&(p_pkt->payload[0]), (char*)pC1, str_size1);
        memcpy(&(p_pkt->payload[(str_size1)]), (char*)pC2, str_size2);
        memcpy(&(p_pkt->payload[(str_size1+str_size2)]), (char*)&fd, I_SIZE);
    }

    /* Release trace buffer */
    Release_Buffer_Space(retval);
}

/***********************************************************************
*
*   FUNCTION
*
*       Log_pCpCpCU16U16I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, char* pC1, char* pC2,
*               char* pC3, unsigned short u16_1, unsigned short u16_2
*               int fd> record
*
***********************************************************************/
void Log_pCpCpCU16U16I(unsigned short evt_id, char* pC1, char* pC2,
                       char* pC3, unsigned short u16_1,
                       unsigned short u16_2, int fd)
{
    int             str_size1 = (strlen(pC1) + 1);
    int             str_size2 = (strlen(pC2) + 1);
    int             str_size3 = (strlen(pC3) + 1);
    char*           p_buff;
    int             retval = -1;
    TRACE_MARK_HDR* p_pkt;

    /* Acquire pointer to next available buffer */
    retval = Acquire_Buffer_Space((TH_U16U16I_SIZE+str_size1+str_size2+str_size3), &p_buff);

    /* Fill up the buffer */
    if(retval > 0)
    {
        p_pkt = (TRACE_MARK_HDR*)p_buff;
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id, (TH_U16U16I_SIZE+str_size1+str_size2+str_size3));
        memcpy(&(p_pkt->payload[0]), (char*)pC1, str_size1);
        memcpy(&(p_pkt->payload[str_size1]), (char*)pC2, str_size2);
        memcpy(&(p_pkt->payload[str_size1+str_size2]), (char*)pC3, str_size3);
        memcpy(&(p_pkt->payload[(str_size1+str_size2+str_size3)]), (char*)&u16_1, US_SIZE);
        memcpy(&(p_pkt->payload[(str_size1+str_size2+str_size3+US_SIZE)]), (char*)&u16_2, US_SIZE);
        memcpy(&(p_pkt->payload[(str_size1+str_size2+str_size3+US_SIZE+US_SIZE)]), (char*)&fd, I_SIZE);
    }

    /* Release trace buffer */
    Release_Buffer_Space(retval);
}

/***********************************************************************
*
*   FUNCTION
*
*       Log_pCU16U16I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, char* pC, unsigned short u16_1,
*               unsigned short u16_2, int fd> record
*
***********************************************************************/
void Log_pCU16U16I(unsigned short evt_id, char* pC, unsigned short u16_1,
                         unsigned short u16_2, int fd)
{
    int             str_size = (strlen(pC) + 1);
    char*           p_buff;
    int             retval = -1;
    TRACE_MARK_HDR* p_pkt;

    /* Acquire pointer to next available buffer */
    retval = Acquire_Buffer_Space((TH_U16U16I_SIZE+str_size), &p_buff);

    /* Fill up the buffer */
    if(retval > 0)
    {
        p_pkt = (TRACE_MARK_HDR*)p_buff;
        Write_Packet_Header(p_pkt, KRN_MARK, evt_id, (TH_U16U16I_SIZE+str_size));
        memcpy(&(p_pkt->payload[0]), (char*)pC, str_size);
        memcpy(&(p_pkt->payload[(str_size)]), (char*)&u16_1, US_SIZE);
        memcpy(&(p_pkt->payload[(str_size+US_SIZE)]), (char*)&u16_2, US_SIZE);
        memcpy(&(p_pkt->payload[(str_size+US_SIZE+US_SIZE)]), (char*)&fd, I_SIZE);
    }

    /* Release trace buffer */
    Release_Buffer_Space(retval);
}

#if (CFG_NU_OS_SVCS_TRACE_CORE_TRACK_TRACE_OVERHEAD == NU_TRUE)
/***********************************************************************
*
*   FUNCTION
*
*       Log_U64U64I
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, unsigned long long u64_1,
*               unsigned long long u64_2, int status> record
*
***********************************************************************/
LOG_THREEPARAMS(Log_U64U64I, unsigned long long, unsigned long long, int)

/***********************************************************************
*
*   FUNCTION
*
*       Log_pCU64U64
*
*   DESCRIPTION
*
*       Logs a <unsigned short evt_id, string evt_str, long u64_1, long u64_2>
*
***********************************************************************/
void Log_pCU64U64(unsigned short evt_id, char* evt_str, unsigned long long u64_1, unsigned long long u64_2)
{
    int             str_size = strlen(evt_str) + 1;
    unsigned short  size = sizeof(TRACE_MARK_HDR) + str_size + sizeof(unsigned long long) + sizeof(unsigned long long);
    TRACE_MARK_HDR* p_tm;
    int             retval = -1;
    char*           p_buff;

    /* Acquire pointer to next available buffer */
    retval = Acquire_Buffer_Space(size, &p_buff);

    /* Fill up the buffer */
    if (retval > 0)
    {
        p_tm = (TRACE_MARK_HDR*) p_buff;
        Write_Packet_Header(p_tm, KRN_MARK, evt_id, size);
        memcpy(&(p_tm->payload[0]), evt_str, str_size);
        memcpy(&(p_tm->payload[str_size]), (char*)&u64_1, sizeof(unsigned long long));
        memcpy(&(p_tm->payload[(str_size + sizeof(unsigned long long))]), (char*)&u64_2, sizeof(unsigned long long));
    }

    /* Release trace buffer */
    Release_Buffer_Space(retval);
}
#endif /* (CFG_NU_OS_SVCS_TRACE_CORE_TRACK_TRACE_OVERHEAD == NU_TRUE) */

#endif /* (CFG_NU_OS_SVCS_TRACE_CORE_TRACE_SUPPORT == NU_TRUE) */

