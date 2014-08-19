/*************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
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
*       net_dbg.h
*
*   DESCRIPTION
*
*       This file contains support information for the Debug Module.
*
*   DATA STRUCTURES
*
*       NET_DEBUG_PARENT_LIST_STRUCT
*       NET_DEBUG_BUFFER_STRUCT
*       NET_DEBUG_BUFFER_INFO_STRUCT
*       NET_NTFY_STRUCT
*
*   FUNCTIONS
*
*       None
*
*   DEPENDENCIES
*
*       os.h
*       nu_net.h
*
*************************************************************************/

#ifndef _NET_DBG_
#define _NET_DBG_

#include "networking/nu_net.h"

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

extern NU_QUEUE                    NET_NTFY_Msg_Queue;

#define NET_NTFY_ETHERNET       1
#define NET_NTFY_GENERAL        2

#define NET_TOO_MANY_BUFFERS    1
#define NET_TOO_FEW_BUFFERS     2
#define NET_CIRCULAR_LIST       3
#define NET_DUP_BUFF            4
#define NET_CIRCULAR_CHAIN      5

#define NET_DBG_NO_ACTION       0
#define NET_DBG_SOCKET          1
#define NET_DBG_PORT            2
#define NET_DBG_DEV             3

#define NET_NTFY_NOTIFY         0x1

/* Size of the message sent to the application */
typedef struct NET_NTFY_DEBUG_STRUCT
{
    INT     net_ntfy_type;
    VOID    *net_ntfy_info;
    INT     net_ntfy_length;
} NET_NTFY_Debug_Struct;

typedef struct NET_Debug_Parent_List_Struct
{
    UINT32  *net_dbg_list;
    INT     net_dbg_buf_socket;
    INT     net_dbg_buf_tcp_port;
    INT     net_dbg_buf_dev_index;
} NET_DEBUG_PARENT_LIST_STRUCT;

typedef struct NET_Debug_Buffer_Struct
{
    NET_BUFFER                      *net_dbg_buffer;
    NET_DEBUG_PARENT_LIST_STRUCT    net_dbg_list1;
    NET_DEBUG_PARENT_LIST_STRUCT    net_dbg_list2;
} NET_DEBUG_BUFFER_STRUCT;

typedef struct NET_Debug_Buffer_Info_Struct
{
    NET_DEBUG_BUFFER_STRUCT         *net_dbg_buf_list;
    STATUS                          net_dbg_buf_status;
    INT                             net_dbg_buf_ooo_count;
    INT                             net_dbg_buf_unp_count;
    INT                             net_dbg_buf_recv_count;
    INT                             net_dbg_buf_driver_count;
    INT                             net_dbg_buf_out_count;
    INT                             net_dbg_buf_free_count;
    INT                             net_dbg_buf_arp_count;
    INT                             net_dbg_buf_re_count;
    INT                             net_dbg_buf_total_count;
    INT                             net_dbg_buf_buffers_used;
} NET_DEBUG_BUFFER_INFO_STRUCT;

typedef struct NET_Ntfy_Struct
{
    STATUS                          net_ntfy_status;

    union
    {
#if (NU_DEBUG_NET == NU_TRUE)
        NET_DEBUG_BUFFER_INFO_STRUCT    net_ntfy_buffer_info;
#endif
        DV_DEVICE_ENTRY                 net_ntfy_dev_info;
    } net_ntfy_info;

    CHAR                            net_ntfy_file[NLOG_MAX_BUFFER_SIZE];
    INT                             net_ntfy_line;
    INT                             net_ntfy_flags;
    NU_TASK                         *net_ntfy_task;
    INT                             net_ntfy_type;
} NET_NTFY_STRUCT;

#define NET_DBG_QUEUE_ELEMENT_SIZE  (sizeof(NET_NTFY_STRUCT) / sizeof(UNSIGNED))

#if (NU_DEBUG_NET == NU_FALSE)

#define NET_DBG_Notify(a, b, c, d, e)
#define NET_DBG_Validate_MEM_Buffers_Used

#else

#define NET_DBG_Notify      NET_Notify

STATUS  NET_DBG_Validate_MEM_Buffers_Used(VOID);

extern NET_DEBUG_BUFFER_STRUCT      NET_DBG_Buffer_Ptr_List[];

#endif

#if ( (NU_ENABLE_NOTIFICATION == NU_TRUE) || (NU_DEBUG_NET == NU_TRUE) )
VOID    NET_Notify(INT32, const CHAR *, const INT, NU_TASK *,
                   const NET_NTFY_Debug_Struct *);
#else
#define	NET_Notify(a, b, c, d, e)
#endif

STATUS  NET_DBG_Count_Buffers(NET_DEBUG_BUFFER_INFO_STRUCT *);
STATUS  NET_NTFY_Init(VOID);

#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* _NET_DBG_ */
