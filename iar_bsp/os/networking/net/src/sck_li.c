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
*   FILE NAME
*
*       sck_li.c
*
*    DESCRIPTION
*
*       This file contains the implementation of NU_Listen.
*
*    DATA STRUCTURES
*
*       None.
*
*    FUNCTIONS
*
*       NU_Listen
*
*    DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_SNMP == NU_TRUE)
#include SNMP_GLUE
#endif

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
/* Declare Memory for listen sockets */
struct TASK_TABLE_STRUCT  NET_Listen_Memory[NET_MAX_SERVER_SOCKETS];

INT   NET_Stat_Entry_Memory[NET_MAX_SERVER_BACKLOG * NET_MAX_SERVER_SOCKETS];
INT   NET_Socket_Index_Memory[NET_MAX_SERVER_BACKLOG * NET_MAX_SERVER_SOCKETS];

/* Declare flag array for the array (memory) declared above */
UINT8 NET_Listen_Memory_Flags[NET_MAX_SERVER_SOCKETS] = {0};
#endif

/*************************************************************************
*
*   FUNCTION
*
*       NU_Listen
*
*   DESCRIPTION
*
*       This function is responsible for indicating that the server is
*       willing to accept connection requests from clients.
*
*   INPUTS
*
*       socketd                 Specifies the server's socket descriptor
*       backlog                 Specifies the number of requests, which
*                               can be queued for this server
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful listen.
*       NU_INVALID_SOCKET       The socket parameter was not a valid
*                               socket value or it had not been
*                               previously allocated via the NU_Socket
*                               call.
*       NU_NO_SOCK_MEMORY       There was not enough memory for Nucleus
*                               to allocate the necessary structures to
*                               be ready for a connection.
*
*************************************************************************/
STATUS NU_Listen(INT socketd, UINT16 backlog)
{
    STATUS                      status;        /* initialize to SUCCESS */
    struct TASK_TABLE_STRUCT    *task_entry = NU_NULL;
                                                /* structure of connections
                                                    for this server/task_id */
    UINT16                      counter;       /* used for initialization of
                                                  Task_Entry */
    UINT16                      backlog_num;

#if (INCLUDE_SR_SNMP == NU_TRUE)
    UINT8                       tcp_laddr[IP_ADDR_LEN];
    UINT8                       tcp_faddr[IP_ADDR_LEN];
    UINT16                      in_port;
    UINT16                      out_port;
#endif

    /* Obtain the semaphore and validate the socket */
    status = SCK_Protect_Socket_Block(socketd);

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    if (status != NU_SUCCESS)
        return (status);

#endif

    /* If NU_Listen has already been called for the socket, return
     * NU_SUCCESS.
     */
    if (SCK_Sockets[socketd]->s_accept_list != NU_NULL)
    {
        /* Release the semaphore */
        SCK_Release_Socket();

        return (NU_SUCCESS);
    }

    /* If the application does not want to queue up connections, set
     * the memory to be allocated to one.
     */
    if (backlog == 0)
        backlog_num = 1;
    else
        backlog_num = backlog;

    /* Allocate memory required for a Task_Entry structure, connection status
       array, and TCP_Ports entry array all at once.  This is done for efficiency
       (1 service call as opposed to 3).  Also, each of the 3 independent
       allocations was less than the default min allocation size (50 bytes) of
       PLUS, so memory was being wasted.
    */
#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    status = NU_Allocate_Memory(MEM_Cached, (VOID **) &task_entry,
                                (UNSIGNED)(sizeof(struct TASK_TABLE_STRUCT) +
                                (backlog_num * sizeof(task_entry->stat_entry)) +
                                (backlog_num * sizeof(task_entry->socket_index))),
                                (UNSIGNED)NU_NO_SUSPEND);

    /* verify a successful allocation */
    if (status == NU_SUCCESS)
    {
        task_entry = (struct TASK_TABLE_STRUCT *)TLS_Normalize_Ptr(task_entry);

        /* Break up the block of memory allocated into three separate chunks.
           Step past the chunk used for the main structure. */
        task_entry->stat_entry = (INT *)(task_entry + 1);

        /* Point past the chunk of memory used for the list of stat_entry's. */
        task_entry->socket_index = (INT *)(task_entry->stat_entry + backlog_num);
#else
    /* traverse the array to find free memory location */
    for(counter = 0; (NET_Listen_Memory_Flags[counter] != NU_FALSE) &&
        (counter != NET_MAX_SERVER_SOCKETS); counter++)
         ;
    if(counter != NET_MAX_SERVER_SOCKETS)
    {
        /* assign the free memory */
        task_entry = (struct TASK_TABLE_STRUCT *)&NET_Listen_Memory[counter];
        task_entry->stat_entry   = &NET_Stat_Entry_Memory[counter * NET_MAX_SERVER_BACKLOG];
        task_entry->socket_index = &NET_Socket_Index_Memory[counter * NET_MAX_SERVER_BACKLOG];

        /* turn the memory flag on */
        NET_Listen_Memory_Flags[counter] = NU_TRUE;
        status = NU_SUCCESS;
    }
    else
        status = NU_NO_MEMORY;

    if (status == NU_SUCCESS)
    {

#endif /* INCLUDE_STATIC_BUILD */

        /* retrieve the local port number from the socket descriptor
           for the task table */
        task_entry->local_port_num = SCK_Sockets[socketd]->s_local_addr.port_num;

        /* record the socket number in the socketd field of task_entry */
        task_entry->socketd = socketd;

        /* Clear the task suspension list. */
        task_entry->ssp_task_list.blink = task_entry->ssp_task_list.flink = NU_NULL;
        task_entry->ssp_task_list.buff_elmt = NU_NULL;
        task_entry->ssp_task_list.task = NU_NULL;

        /* initialize the port_entries and stat_entries */
        for (counter = 0; counter < backlog_num; counter++)
        {
            SCK_Clear_Accept_Entry(task_entry, (INT)counter);
        }

        /* initialize the current connection pointer to the first
           space in the table */
        task_entry->current_idx = 0;

        /* store the number of backlog queues possible */
        task_entry->total_entries = backlog;

        /* initialize the next pointer to NU_NULL */
        task_entry->next = NU_NULL;

        /* Associate the socket with its accept list. */
        SCK_Sockets[socketd]->s_accept_list = task_entry;

        /* Mark this socket as a listener. */
        SCK_Sockets[socketd]->s_flags |= SF_LISTENER;

#if ( (INCLUDE_SR_SNMP == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )

        if (SCK_Sockets[socketd]->s_family == NU_FAMILY_IP)
        {
            PUT32(tcp_laddr, 0, IP_ADDR_ANY);
            PUT32(tcp_faddr, 0, IP_ADDR_ANY);
            in_port  = SCK_Sockets[socketd]->s_local_addr.port_num;
            out_port = SCK_Sockets[socketd]->s_foreign_addr.port_num;

            SNMP_tcpConnTableUpdate(SNMP_ADD, SLISTEN, tcp_laddr, in_port,
                                    tcp_faddr, out_port);
        }
#endif
    }
    else
    {
        NLOG_Error_Log("Unable to alloc memory for task entry struct",
                       NERR_RECOVERABLE, __FILE__, __LINE__);

        /* Tell them no more memory. */
        status = NU_NO_SOCK_MEMORY;
    }

    /* Release the semaphore */
    SCK_Release_Socket();

    /* return to the caller */
    return (status);

} /* NU_Listen */
