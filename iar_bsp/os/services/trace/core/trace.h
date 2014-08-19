/***********************************************************************
*
*            Copyright 2012 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
************************************************************************

************************************************************************
*
*   DESCRIPTION
*
*       This file contains definitions, macros and declarations for
*       data structures used by the trace component.
*
***********************************************************************/

#ifndef _TRACE_H_
#define _TRACE_H_

#ifdef          __cplusplus
extern          "C" {   /* C declarations in C++     */
#endif

/**************/
/* OS Support */
/**************/

/***********************************/
/* Define support for Nucleus RTOS */
/***********************************/
#define    NUCLEUS_OS_SUPPORT

/* OS specific MACRO definitions */
#ifdef     NUCLEUS_OS_SUPPORT

#include   "nucleus.h"
#include   "kernel/nu_kernel.h"
#include    "os/services/trace/comms/trace_comms.h"

#define    FREQ_1GHZ                           1000000000
#define    MESA_TIME_BASE                      FREQ_1GHZ
#define    SCALE_FACTOR_1GHZ                   (MESA_TIME_BASE/(NU_HW_Ticks_Per_SW_Tick * NU_PLUS_TICKS_PER_SEC))
#define    TIME_STAMP()                        NU_Get_Time_Stamp();
#define    GET_CPU_ID()                        0

#define    CRITICAL_SECTION_DATA               static ESAL_AR_INT_CONTROL_VARS
#define    CRITICAL_SECTION_START              ESAL_GE_INT_ALL_DISABLE()
#define    CRITICAL_SECTION_END                ESAL_GE_INT_ALL_RESTORE()

#endif /* NUCLEUS_OS_SUPPORT */

/**********/
/* MACROS */
/**********/

#define    GET_UPDATED_IDX(idx,ele_size,size)  (((idx + ele_size) >= size) ? 0:(idx + ele_size))
#define    WORD_SIZE                           sizeof(unsigned long)
#define    WORD_ALIGN(a)                       (((a) & (WORD_SIZE-1)) != 0)?(((a) & (~(WORD_SIZE-1))) + 4):(a)

/***************/
/* Definitions */
/***************/

/* Error definitions */
#define TRACE_SUCCESS        0
#define TRACE_INIT_ERR      -1
#define TRACE_BUF_FULL_ERR  -2
#define TRACE_BUF_EMPTY_ERR -3
#define TRACE_RD_I_DIRTY    -4

/* Trace packet header definitions */
#define LNK_MARK            0x0001
#define KRN_MARK            0x0002
#define APP_MARK_I32        0x0003
#define APP_MARK_U32        0x0004
#define APP_MARK_FLT        0x0005
#define APP_MARK_VAL        0x0006
#define APP_MARK_STR        0x0007

/* Buffer status values */
#define BUFFER_FULL         1
#define BUFFER_DATA         2
#define BUFFER_EMPTY        3

/* Trace packet header */
#define TRACE_PKT_HDR       0xEF56A55A

/*******************/
/* Data structures */
/*******************/

struct  _link_pkt_hdr_
{
    unsigned long           hdr;
    unsigned short          mrkr_type;
    unsigned short          size;

}__attribute__((__packed__));

typedef    struct  _link_pkt_hdr_ LINK_PKT,MIN_PKT_HDR;

struct  _trace_mark_hdr_
{
    MIN_PKT_HDR             pkt_hdr;
    unsigned short          evt_id;
    unsigned short          cpu;
    unsigned long long      time_stamp;
    char                    payload[];

}__attribute__((__packed__));

typedef    struct  _trace_mark_hdr_ TRACE_MARK_HDR;

struct  _trace_buff_mgmt_
{
    char*                   p_buff;
    unsigned long           size;
    unsigned long           wr_i;
    unsigned long           rd_i;
    unsigned long           rd_dirty;
    unsigned long           trace_initialized;
    unsigned long           buffer_overflow;
};

typedef struct    _trace_buff_mgmt_    TRACE_BUFF_MGMT;

/**********************/
/* External interface */
/**********************/
int     Initialize_Trace_Buffer(char* p_mem, unsigned int size);
int     Acquire_Buffer_Space(unsigned short size, char** p_buff);
void    Release_Buffer_Space(int size);
int     Get_Trace_Packet(char* p_buff, unsigned short* p_size);
char    *Get_Trace_Buffer( void );
void    Set_Read_Head( unsigned long read_head );
void    Get_Buffer_Read_Write_Head( unsigned long *read_head, unsigned long *write_head );
unsigned long Get_Used_Buffer_Space( void );
unsigned long Get_Buffer_Size( void );
unsigned long Get_Is_Buffer_Overflowed( void );
void    Write_Packet_Header(TRACE_MARK_HDR* p_tm, unsigned short mrkr_type,
        unsigned short evt_id, unsigned short size);

#ifdef          __cplusplus
}   /* End of C declarations */
#endif  /* __cplusplus */

#endif /* _TRACE_H_ */
