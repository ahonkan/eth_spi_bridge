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
*       sck.c
*
*   DESCRIPTION
*
*       This file defines our interface to the functions using the Berkeley
*       socket standard TCP/IP interface.
*
*   DATA STRUCTURES
*
*       next_socket_no
*       *SCK_Sockets[]
*
*   FUNCTIONS
*
*       SCK_Create_Socket
*       SCK_Suspend_Task
*       SCK_Resume_All
*       SCK_Clear_Socket_Error
*
*   DEPENDENCIES
*
*       nu_net.h
*
**************************************************************************/

#include "networking/nu_net.h"

/* next_socket_no is used to record the last position searched in the
   socket list.  The next time the socket list is searched an unused socket
   should be found immediately.  The alternative is to begin searching from the
   start of the list every time.  Chances are a lot of used sockets would be
   found before finding an unused one. */
INT next_socket_no;

/* The following structure is used to define the socket list for
   Nucleus NET. Sockets are required for all TCP/UDP/IPRaw connections. */
struct sock_struct *SCK_Sockets[NSOCKETS];

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
/* Declare memory for all sockets */
SOCKET_STRUCT NET_Socket_Memory[NSOCKETS];

/* Declare memory for the sticky options associated with the sockets */
UINT8   NET_Sticky_Options_Memory[NSOCKETS * STICKY_OPTIONS_MAX_SIZE];
UINT8   NET_Sticky_Options_Memory_Flags[NSOCKETS] = {0};
#endif

/************************************************************************
*
*   FUNCTION
*
*       SCK_Create_Socket
*
*   DESCRIPTION
*
*       This function creates a new socket.
*
*   INPUTS
*
*       protocol                The protocol specified should be
*                               NU_proto_list[family][type] or the
*                               protocol received from a Raw socket
*       family                  The family type
*
*   OUTPUTS
*
*       > = 0                   Socket descriptor
*       NU_NO_SOCK_MEMORY       No socket memory to allocate from
*       NU_NO_SOCKET_SPACE      No socket descriptors are available.
*
*************************************************************************/
INT SCK_Create_Socket(INT protocol, INT16 family)
{
    struct sock_struct *sockptr;                              /* pointer to current socket */
    INT                 return_status = NU_NO_SOCKET_SPACE;
    INT                 counter;                              /* to traverse the socket list */

    /* search the socket list to be sure there is room for another connection */
    for (counter = 0; counter < NSOCKETS; counter++)
    {
        if (SCK_Sockets[next_socket_no] == NU_NULL)
        {
#if (INCLUDE_STATIC_BUILD == NU_FALSE)

            /* allocate a socket structure */
            return_status = NU_Allocate_Memory(MEM_Cached, (VOID **) &sockptr,
                                               (UNSIGNED)sizeof(struct sock_struct),
                                               (UNSIGNED)NU_NO_SUSPEND);

            if (return_status == NU_SUCCESS)
#else
            /* Assign memory to the new socket */
            sockptr = &NET_Socket_Memory[next_socket_no];
#endif
            {
                sockptr = (struct sock_struct *)TLS_Normalize_Ptr(sockptr);
                SCK_Sockets[next_socket_no] = sockptr;

                /* Zero out the socket structure */
                UTL_Zero(sockptr, sizeof(struct sock_struct));

                /* fill only the protocol portion of the socket structure */
                sockptr->s_protocol = (UINT16)protocol;

                sockptr->s_family = family;

                /* Set the initial state. */
                sockptr->s_state = 0;

                /* Initialize the socket options.  The ability to transmit
                   broadcast messages is enabled by default. */
                sockptr->s_options = SO_BROADCAST_OP;

                /* Disable the wait for close, SO_LINGER, option by default. */
                sockptr->s_linger.linger_on = NU_FALSE;

                /* Initialize the linger time. */
                sockptr->s_linger.linger_ticks = 0;

                /* Sockets are blocking by default */
                sockptr->s_flags = SF_BLOCK;

                /* Initially there is no multicast option data. */
#if (INCLUDE_IPV4 == NU_TRUE)
                sockptr->s_moptions_v4 = NU_NULL;
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
                sockptr->s_moptions_v6 = NU_NULL;
#endif

                /* No port is yet associated with this socket. */
                sockptr->s_port_index = -1;

                /* return the SCK_Sockets index */
                return_status = next_socket_no;

                next_socket_no++;

                if (next_socket_no >= NSOCKETS)
                  next_socket_no = 0;

                /* A pseudo unique ID number is associated with the socket */
                sockptr->s_struct_id = EQ_ID_VALUE;

            }  /* end status == NU_SUCCESS */

#if (INCLUDE_STATIC_BUILD == NU_FALSE)

            else
            {
                NLOG_Error_Log("Unable to alloc memory for socket struct",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }  /* end status != NU_SUCCESS */
#endif

            /* discontinue after we find an empty space in the socket list */
            break;

        }  /* end SCK_Sockets slot is null */

        next_socket_no++;
        if (next_socket_no >= NSOCKETS)
          next_socket_no = 0;
    }

    return (return_status);

} /* SCK_Create_Socket */

/************************************************************************
*
*   FUNCTION
*
*       SCK_Suspend_Task
*
*   DESCRIPTION
*
*       This function suspends the calling task.  First it releases the
*       TCP Resource.  Upon resumption it grabs the TCP Resource.
*
*   INPUTS
*
*       task_id                 Task to suspend.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID SCK_Suspend_Task(NU_TASK *task_id)
{
    OPTION   old_preempt;

    old_preempt = NU_Change_Preemption(NU_NO_PREEMPT);

    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

    NU_Suspend_Task(task_id);

    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

    NU_Change_Preemption(old_preempt);

} /* SCK_Suspend_Task */

/************************************************************************
*
*   FUNCTION
*
*       SCK_Resume_All
*
*   DESCRIPTION
*
*       This function resumes all tasks in a socket suspension list.
*       An optional parameter is given to include removing items
*       from a buffer suspension list.
*
*   INPUTS
*
*       *list                   Pointer to list of socket task entries.
*       flags                   Used to indicate additional actions when
*                               resuming tasks.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID SCK_Resume_All(struct SCK_TASK_ENT *list, INT flags)
{
    struct SCK_TASK_ENT *task_entry_ptr;    /* task entry for list
                                               operations */

    if (list != NU_NULL)
    {
        while (list->flink != NU_NULL)
        {
            /* Remove entry from list */
            task_entry_ptr = DLL_Dequeue (list);

            /* Check if we need to remove the buffer suspension
               entry */
            if (flags & SCK_RES_BUFF)
            {
                if (task_entry_ptr->buff_elmt != NU_NULL)
                {
                    DLL_Remove(&MEM_Buffer_Suspension_List,
                               task_entry_ptr->buff_elmt);
                }
            }

            /* Resume the task */
            if (NU_Resume_Task(task_entry_ptr->task) != NU_SUCCESS)
                NLOG_Error_Log("Failed to resume task", NERR_INFORMATIONAL,
                               __FILE__, __LINE__);
        }
    }

} /* SCK_Resume_All */

/************************************************************************
*
*   FUNCTION
*
*       SCK_Clear_Socket_Error
*
*   DESCRIPTION
*
*       This function clears any error on the socket.
*
*   INPUTS
*
*       socketd                 The socket for which to clear the error.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID SCK_Clear_Socket_Error(INT socketd)
{
    /* Obtain the semaphore and validate the socket */
    if (SCK_Protect_Socket_Block(socketd) == NU_SUCCESS)
    {
        /* Clear the error. */
        SCK_Sockets[socketd]->s_error = 0;

        /* Release the semaphore */
        SCK_Release_Socket();
    }

} /* SCK_Clear_Socket_Error */

