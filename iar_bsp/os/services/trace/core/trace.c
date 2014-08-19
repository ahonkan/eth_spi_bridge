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
*       trace.c
*
*   COMPONENT
*
*       Trace Buffering System
*
*   DESCRIPTION
*
*       This is a stand-alone component that implements the back-end
*       buffering infrastructure for software tracing.
*
*************************************************************************/
/********************/
/* Include headers  */
/********************/
#include    "trace.h"

/********************/
/* Global variables */
/********************/

/* Data structure to manage trace data buffering */
static TRACE_BUFF_MGMT  T_Buff_Mgt;

/* Dummy link packet to enable read head traversal */
static LINK_PKT         Lnk_Pkt;

#define GET_USED_BUFFER_SPACE  ((T_Buff_Mgt.rd_i == T_Buff_Mgt.wr_i) ? 0 :    \
                               ((T_Buff_Mgt.rd_i < T_Buff_Mgt.wr_i) ?         \
                               (T_Buff_Mgt.wr_i - T_Buff_Mgt.rd_i) :          \
                               (T_Buff_Mgt.size - T_Buff_Mgt.rd_i) + T_Buff_Mgt.wr_i))

/* Global to maintain interrupt state of the system */
CRITICAL_SECTION_DATA;

/***********************************************************************
*
*   FUNCTION
*
*       Init_Trace_Buffer
*
*   DESCRIPTION
*
*       Clear trace buffer memory and initialize the buffer management
*       data structure
*
*    INPUTS
*
*       p_mem                Pointer to caller allocated memory
*       size                Size of trace buffer
*
*   OUTPUTS
*
*       TRACE_SUCCESS        Trace component successfully initialized
*       TRACE_INIT_ERR        Initialization error
*
***********************************************************************/
int   Initialize_Trace_Buffer(char* p_mem, unsigned int size)
{
    int ret_val= 0;

    /* If a valid memory is provided clear memory and
     * initialize the buffer management data structure */
    if(p_mem == 0)
    {
        ret_val  = TRACE_INIT_ERR;
    }
    else
    {
        T_Buff_Mgt.p_buff = p_mem;
        T_Buff_Mgt.size = (size - sizeof(LINK_PKT));
        T_Buff_Mgt.rd_i = 0;
        T_Buff_Mgt.wr_i = 0;
        T_Buff_Mgt.rd_dirty = 0;
        T_Buff_Mgt.trace_initialized = NU_TRUE;
        T_Buff_Mgt.buffer_overflow = NU_FALSE;

        /* Initialize link Packet marker */
        Lnk_Pkt.mrkr_type = LNK_MARK;
    }

    return ret_val;
}

/***********************************************************************
*
*   FUNCTION
*
*       Acquire_Buffer_Space
*
*   DESCRIPTION
*
*       This function acquires and locks the next available buffer space
*       of size requested by the user.
*       NOTE: Release_Buffer_Space *HAS* to be called after the buffer
*       is filled with trace data to return the buffer to the system.
*       This function implements logic to handle non-overwrite-mode
*       and over-write mode build configurations.
*
*    INPUTS
*
*       size                Size of trace buffer required
*
*   OUTPUTS
*
*       TRACE_SUCCESS        Successfully acquired buffer space
*       TRACE_BUF_FULL_ERR    Trace buffer full error
*       TRACE_RD_I_DIRTY    Trace buffer management read index is dirty
*
***********************************************************************/
#if(CFG_NU_OS_SVCS_TRACE_CORE_OVERWRITE_OLD_DATA == 1)

int    Acquire_Buffer_Space(unsigned short size, char** p_buff)
{
    MIN_PKT_HDR*    p_pkt;
    unsigned short    bytes_freed = 0;
    int             retval = TRACE_SUCCESS;

    /* Critical section begins */
    CRITICAL_SECTION_START;

    /* Make sure buffer size reserved is word aligned */
    size = WORD_ALIGN(size);

#if(CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL == DBG_INTERFACE)

    /* Entering buff management critical section clear rd_dirty bit
     * to facilitate asynchronous buffer access from host over debug interface */
    T_Buff_Mgt.rd_dirty = 0;

#endif

    /* Fix rd_idx and Insert link packet if we do not have contiguous
     * memory available in the buffer */
    if(GET_UPDATED_IDX(T_Buff_Mgt.wr_i, size, T_Buff_Mgt.size) == 0)
    {
        /* Insert link packet */
        p_pkt = (MIN_PKT_HDR*) &T_Buff_Mgt.p_buff[T_Buff_Mgt.wr_i];
        p_pkt->mrkr_type = LNK_MARK;
        p_pkt->size = T_Buff_Mgt.size - T_Buff_Mgt.wr_i;

        /* Fix up read pointer - if (rd_idx = 0) { moved rd_idx
         * to next packet}, if(rd_idx > wrt_idx) { wrap first
         * and than move rd_idx to next available packet */
        if ((T_Buff_Mgt.rd_i > T_Buff_Mgt.wr_i) || (T_Buff_Mgt.rd_i == 0))
        {
            T_Buff_Mgt.rd_i = 0;
            p_pkt =    (MIN_PKT_HDR*)&T_Buff_Mgt.p_buff[T_Buff_Mgt.rd_i];
            T_Buff_Mgt.rd_i = GET_UPDATED_IDX(T_Buff_Mgt.rd_i,
                              (WORD_ALIGN(p_pkt->size)), T_Buff_Mgt.size);
        }

        /* Wrap write index */
        T_Buff_Mgt.wr_i = 0;
    }


    /* If write crosses over read, move read forward to provide required buffer space to caller */
    if(GET_UPDATED_IDX(T_Buff_Mgt.wr_i, size, T_Buff_Mgt.size) >= T_Buff_Mgt.rd_i)
    {
        if(T_Buff_Mgt.rd_i != T_Buff_Mgt.wr_i)
        {
            if(T_Buff_Mgt.wr_i < T_Buff_Mgt.rd_i)
            {
                T_Buff_Mgt.buffer_overflow = NU_TRUE;
                bytes_freed += (T_Buff_Mgt.rd_i - T_Buff_Mgt.wr_i);

                /* Move read index to accommodate memory space requested by the caller */
                for(;;)
                {
                    /* If we have freed up enough memory for buffer requested break loop */
                    if(bytes_freed > size)
                    {
                        break;
                    }

                    /* Obtain pointer to current packet, update bytes freed variable, and
                     * update read index to point to next available packet */
                    p_pkt =    (MIN_PKT_HDR*)&T_Buff_Mgt.p_buff[T_Buff_Mgt.rd_i];
                    bytes_freed += WORD_ALIGN(p_pkt->size);
                    T_Buff_Mgt.rd_i = GET_UPDATED_IDX(T_Buff_Mgt.rd_i,
                                     (WORD_ALIGN(p_pkt->size)), T_Buff_Mgt.size);
                }
            }
        }
    }

    /* Update output arguments */
    *p_buff = &T_Buff_Mgt.p_buff[T_Buff_Mgt.wr_i];
    retval = size;

#if(CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL == DBG_INTERFACE)

    /* If buffer management structure is marked dirty by debugger return error */
    if(T_Buff_Mgt.rd_dirty == 1)
    {
        /* Set buffer state to be empty by making rd_idx equal to wrt_idx */
        T_Buff_Mgt.rd_i = T_Buff_Mgt.wr_i;

        /* Return error to caller */
        retval = TRACE_RD_I_DIRTY;
    }

#endif

    return retval;
}

#else

int    Acquire_Buffer_Space(unsigned short size, char** p_buff)
{
    MIN_PKT_HDR*    p_pkt;
    int             retval = TRACE_SUCCESS;

    /* Critical section begins */
    CRITICAL_SECTION_START;

    /* Make sure buffer size reserved is word aligned */
    size = WORD_ALIGN(size);

#if(CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL == DBG_INTERFACE)

    /* Entering buff management critical section clear rd_dirty bit
     * to facilitate asynchronous buffer access from host over debug interface */
    T_Buff_Mgt.rd_dirty = 0;

#endif

    /* Fix rd_idx and Insert link packet if we do not have contiguous
     * memory available in the buffer */
    if(GET_UPDATED_IDX(T_Buff_Mgt.wr_i, size, T_Buff_Mgt.size) == 0)
    {
        if(T_Buff_Mgt.rd_i != 0)
        {
            /* Insert link packet at the end of buffer space */
            p_pkt = (MIN_PKT_HDR*) &T_Buff_Mgt.p_buff[T_Buff_Mgt.wr_i];
            p_pkt->mrkr_type = LNK_MARK;
            p_pkt->size = T_Buff_Mgt.size - T_Buff_Mgt.wr_i;

            /* Wrap write index */
            T_Buff_Mgt.wr_i = 0;
        }
        else
        {
            retval = TRACE_BUF_FULL_ERR;
        }
    }

    if(retval == TRACE_SUCCESS)
    {
        /* Determine if we have memory available, if yes, provide buffer to caller */
        if((GET_UPDATED_IDX(T_Buff_Mgt.wr_i, size, T_Buff_Mgt.size) >= T_Buff_Mgt.rd_i)
        && (T_Buff_Mgt.wr_i < T_Buff_Mgt.rd_i))
        {
            retval = TRACE_BUF_FULL_ERR;
        }
        else
        {
            *p_buff = &T_Buff_Mgt.p_buff[T_Buff_Mgt.wr_i];
            retval = size;
        }
    }

#if(CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL == DBG_INTERFACE)

    /* If buffer management structure is marked dirty by debugger return error */
    if(T_Buff_Mgt.rd_dirty == 1)
    {
        /* Set buffer state to be empty by making rd_idx equal to wrt_idx */
        T_Buff_Mgt.rd_i = T_Buff_Mgt.wr_i;

        /* Return error to caller */
        retval = TRACE_RD_I_DIRTY;
    }

#endif

    return retval;
}

#endif

/***********************************************************************
*
*   FUNCTION
*
*       Release_Buffer_Space
*
*   DESCRIPTION
*
*       This function releases the memory space placed under lock
*       by the Acquire_Buffer_Space() API.
*
***********************************************************************/
void Release_Buffer_Space(int size)
{
    /* If if successfully Acquired buffer space release it */
    if(size > 0)
    {
        T_Buff_Mgt.wr_i = GET_UPDATED_IDX(T_Buff_Mgt.wr_i, size,
                          T_Buff_Mgt.size);
    }

    /* End of critical section */
    CRITICAL_SECTION_END;

#if (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL == FILE_INTERFACE)

    if ( GET_USED_BUFFER_SPACE >= FILE_BUFFER_FLUSH_THRESHOLD )
    {
        FileIO_Start_Trace_Buffer_Flush();
    }

#endif /* (CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL == FILE_INTERFACE) */

}

/***********************************************************************
*
*   FUNCTION
*
*       Get_Trace_Packet
*
*   DESCRIPTION
*
*       This function fetches a trace packet from the trace buffer
*
***********************************************************************/
int Get_Trace_Packet(char* p_buff, unsigned short* p_size)
{
    MIN_PKT_HDR*    p_pkt;
    unsigned short  a_size;
    int             retval = TRACE_BUF_EMPTY_ERR;

    /* Critical section starts */
    CRITICAL_SECTION_START;

#if(CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL == DBG_INTERFACE)

    /* Entering buff management critical section clear rd_dirty bit
     * to facilitate asynchronous buffer access from host over debug interface */
    T_Buff_Mgt.rd_dirty = 0;

#endif

    if(T_Buff_Mgt.rd_i != T_Buff_Mgt.wr_i)
    {
        /* Obtain pointer to trace packet and its aligned size */
        p_pkt = (MIN_PKT_HDR*) &T_Buff_Mgt.p_buff[T_Buff_Mgt.rd_i];
        a_size = WORD_ALIGN(p_pkt->size);

        /* Obtain packet, size and make necessary adjustments */
        if(p_pkt->mrkr_type == LNK_MARK)
        {
            /* If marker type is link mark skip past packet */
            /* Move read pointer past link pointer */
            T_Buff_Mgt.rd_i = GET_UPDATED_IDX(T_Buff_Mgt.rd_i, a_size,
                              T_Buff_Mgt.size);

            /* Update bytes remaining, p_pkt and a_size */
            p_pkt = (MIN_PKT_HDR*)&T_Buff_Mgt.p_buff[T_Buff_Mgt.rd_i];
            a_size = WORD_ALIGN(p_pkt->size);
        }

        /* Copy trace packet to user buffer and update read pointer */
        memcpy(p_buff, (char*)p_pkt, p_pkt->size);
        *p_size = (p_pkt->size);
        retval = TRACE_SUCCESS;
        T_Buff_Mgt.rd_i = GET_UPDATED_IDX(T_Buff_Mgt.rd_i, a_size,
                          T_Buff_Mgt.size);
    }

#if(CFG_NU_OS_SVCS_TRACE_COMMS_CHANNEL == DBG_INTERFACE)

    /* If buffer management structure is marked dirty by debugger return error */
    if(T_Buff_Mgt.rd_dirty == 1)
    {
        /* Set buffer state to be empty by making rd_idx equal to wrt_idx */
        T_Buff_Mgt.rd_i = T_Buff_Mgt.wr_i;

        /* Set size to zero, and return error to caller */
        *p_size = 0;
        retval = TRACE_RD_I_DIRTY;
    }

#endif

    /* Critical section ends */
    CRITICAL_SECTION_END;

    return retval;
}

/***********************************************************************
*
*   FUNCTION
*
*       Write_Packet_Header
*
*   DESCRIPTION
*
*       Utility function that writes a trace packet header at the
*       requested buffer pointer address.
*
***********************************************************************/
void Write_Packet_Header(TRACE_MARK_HDR* p_tm, unsigned short mrkr_type,
                           unsigned short evt_id, unsigned short size)
{
    p_tm->pkt_hdr.hdr = TRACE_PKT_HDR;
    p_tm->pkt_hdr.mrkr_type = mrkr_type;
    p_tm->pkt_hdr.size = size;
    p_tm->evt_id = evt_id;
    p_tm->cpu = GET_CPU_ID();
    p_tm->time_stamp = SCALE_FACTOR_1GHZ * TIME_STAMP();
}

/***********************************************************************
 *
 *   FUNCTION
 *
 *       Get_Used_Buffer_Space
 *
 *   DESCRIPTION
 *
 *       Provides size of used trace buffer space.
 *
 ***********************************************************************/
unsigned long Get_Used_Buffer_Space( void )
{
    long used_space =  0;

    if (T_Buff_Mgt.trace_initialized)
        used_space = GET_USED_BUFFER_SPACE;

    return used_space;
}

/***********************************************************************
 *
 *   FUNCTION
 *
 *       Get_Buffer_Read_Write_Head
 *
 *   DESCRIPTION
 *
 *       Provides trace buffer's read and write heads.
 *
 ***********************************************************************/
void Get_Buffer_Read_Write_Head( unsigned long *read_head,
                                 unsigned long *write_head )
{
    if (T_Buff_Mgt.trace_initialized)
    {
        *read_head  = T_Buff_Mgt.rd_i;
        *write_head = T_Buff_Mgt.wr_i;
    }
    else
    {
        *read_head  = 0;
        *write_head = 0;
    }
}

/***********************************************************************
 *
 *   FUNCTION
 *
 *       Get_Trace_Buffer
 *
 *   DESCRIPTION
 *
 *       Returns trace buffer pointer .
 *
 ***********************************************************************/
char * Get_Trace_Buffer( void )
{
    char *p_buff = NU_NULL;

    if (T_Buff_Mgt.trace_initialized)
        p_buff = T_Buff_Mgt.p_buff;

    return p_buff;
}

/***********************************************************************
 *
 *   FUNCTION
 *
 *       Set_Read_Head
 *
 *   DESCRIPTION
 *
 *       Updates read head to input parameter value.
 *
 ***********************************************************************/
void Set_Read_Head( unsigned long read_head )
{
    if (T_Buff_Mgt.trace_initialized)
        T_Buff_Mgt.rd_i = read_head;
}

/***********************************************************************
 *
 *   FUNCTION
 *
 *       Get_Buffer_Size
 *
 *   DESCRIPTION
 *
 *      Provides trace buffer size.
 *
 ***********************************************************************/
unsigned long Get_Buffer_Size( void )
{
    unsigned long buffer_size = 0;

    if (T_Buff_Mgt.trace_initialized)
        buffer_size = T_Buff_Mgt.size;

    return buffer_size;
}

/***********************************************************************
 *
 *   FUNCTION
 *
 *       Get_Is_Buffer_Overflowed
 *
 *   DESCRIPTION
 *
 *      Returns buffer status, whether overflowed or not.
 *
 ***********************************************************************/
unsigned long Get_Is_Buffer_Overflowed( void )
{
    unsigned long buffer_overflow = 0;

    if (T_Buff_Mgt.trace_initialized)
        buffer_overflow = T_Buff_Mgt.buffer_overflow;

    return buffer_overflow;
}
