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
*       zc_ab.c
*
*   DESCRIPTION
*
*       This file contains the implementation of NU_ZC_Allocate_Buffer.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_ZC_Allocate_Buffer
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/************************************************************************
*
*   FUNCTION
*
*       NU_ZC_Allocate_Buffer
*
*   DESCRIPTION
*
*       This function provides the caller with a chain of NET buffers
*       of length equal to the size provided to the function.  If
*       the socket type provided is a TCP socket, a connection must
*       be established on the socket.
*
*   INPUTS
*
*       **zc_buf                Pointer to a pointer to a NET Buffer
*       size                    The size of the buffer chain to allocate.
*       socketd                 The socket descriptor for the
*                               communication.
*
*   OUTPUTS
*
*       The number of bytes that will fit in the buffer chain returned.
*
*       NU_INVALID_SOCKET       The socket descriptor provided is
*                               invalid.
*       NU_INVALID_PARM         If the socket is TCP, the connection
*                               is not established, or the caller has
*                               passed in zero as the size parameter.
*       NU_NO_BUFFERS           There are not enough buffers in the
*                               system to fulfill the request, and the
*                               socket is a non-blocking socket.
*       NU_SOCKET_CLOSED        The socket was closed by another task
*                               while this task was suspended waiting
*                               for buffers.
*
*************************************************************************/
INT32 NU_ZC_Allocate_Buffer(NET_BUFFER **zc_buf, UINT16 size, INT socketd)
{
    UINT16                          hdr_size = 0;
    INT32                           bytes_allocated;
    SOCKET_STRUCT                   *sck_ptr;
    NET_BUFFER                      *buf_ptr, *last_buffer = NU_NULL;
    UINT16                          total_size = size;
    STATUS                          status;
    NET_BUFFER_SUSPENSION_ELEMENT   waiting_for_buffer;
    struct SCK_TASK_ENT             task_entry;
    UINT32                          socket_id;

    /* Obtain the semaphore and validate the socket */
    status = SCK_Protect_Socket_Block(socketd);

    if (status == NU_SUCCESS)
    {
        /* Initialize the buffer to NULL in case an error occurs */
        *zc_buf = NU_NULL;

        /* Get a pointer to the socket structure */
        sck_ptr = SCK_Sockets[socketd];

        /* If this is a TCP socket, check the state of the connection and
         * the MSS of the other side.
         */
        if (sck_ptr->s_protocol == NU_PROTO_TCP)
        {
            /* If the port index is negative or the status of the connection
             * is something other than ESTABLISHED, return an error.
             */
            if ( (sck_ptr->s_port_index < 0) ||
                 (TCP_Ports[sck_ptr->s_port_index]->state != SEST) )
            {
                bytes_allocated = 0;
            }

            /* If the number of bytes to store in this buffer exceeds the MSS
             * of the other side of the connection, set the number of bytes
             * to the MSS of the other side of the connection.
             */
            else
            {
                hdr_size = TCP_HEADER_LEN + TCP_Ports[sck_ptr->s_port_index]->p_opt_len;

                /* If the number of bytes to allocated exceeds the MSS, reset
                 * the number of bytes to allocate.
                 */
                if (size > TCP_Ports[sck_ptr->s_port_index]->p_smss)
                {
                    bytes_allocated = TCP_Ports[sck_ptr->s_port_index]->p_smss;
                }

                else
                {
                    bytes_allocated = size;
                }
            }
        }

        /* This is not a TCP socket, so there is no restriction on the number
         * of bytes the sender may send per buffer.
         */
        else
        {
            /* Set the UDP header length */
            if (sck_ptr->s_protocol == NU_PROTO_UDP)
            {
                hdr_size = UDP_HEADER_LEN;
            }

            /* This is an IPRAW socket, there is no transport layer header */
            else
            {
                hdr_size = 0;
            }

            bytes_allocated = size;
        }

        /* If an error has not occurred, get the buffers */
        if (bytes_allocated > 0)
        {
            *zc_buf = NU_NULL;

            while (total_size)
            {
                buf_ptr = NU_NULL;

                /* Attempt to reserve buffers for the application. */
                while (!buf_ptr)
                {
                    /* Do not let the application use all the buffers in the
                     * system.  Some buffers must be reserved for receiving
                     * data.  Otherwise, deadlock could occur.
                     */
                    if ((MAX_BUFFERS - MEM_Buffers_Used) > NET_FREE_BUFFER_THRESHOLD)
                    {
                        buf_ptr = MEM_Buffer_Chain_Dequeue(&MEM_Buffer_Freelist,
                                                           (bytes_allocated + hdr_size));
                    }

                    /* If there are not enough buffers in the system and this
                     * is a blocking socket, suspend for buffers.
                     */
                    if ( (!buf_ptr) && (sck_ptr->s_flags & SF_BLOCK) )
                    {
                        NLOG_Error_Log("Suspending for buffers in NU_ZC_Allocate_Buffer",
                                       NERR_INFORMATIONAL, __FILE__, __LINE__);

                        /* Add this task to the element. */
                        waiting_for_buffer.waiting_task = NU_Current_Task_Pointer();

                        /* Set up the reference to the suspending task */
                        task_entry.task = waiting_for_buffer.waiting_task;

                        /* Get a reference to the memory buffer element */
                        task_entry.buff_elmt = &waiting_for_buffer;

                        /* Put this on the suspended TX list */
                        DLL_Enqueue(&sck_ptr->s_TXTask_List, &task_entry);

                        /* Setup a reference to the socket */
                        waiting_for_buffer.socketd = socketd;

                        /* Setup a reference to the task entry */
                        waiting_for_buffer.list_entry = &task_entry;

                        /* Put this element on the buffer suspension list */
                        DLL_Enqueue(&MEM_Buffer_Suspension_List, &waiting_for_buffer);

                        /* Get the socket ID to verify the socket after suspension */
                        socket_id = sck_ptr->s_struct_id;

                        /* Suspend this task. */
                        SCK_Suspend_Task(waiting_for_buffer.waiting_task);

                        /* Make sure this entry is removed from the buffer
                         * suspension list.
                         */
                        DLL_Remove(&MEM_Buffer_Suspension_List, &waiting_for_buffer);

                        /* If this is a different socket, or the TCP connection has
                         * been closed while suspended, handle it appropriately
                         */
                        if ( (!SCK_Sockets[socketd]) ||
                             (SCK_Sockets[socketd]->s_struct_id != socket_id) ||
                             ((sck_ptr->s_protocol == NU_PROTO_TCP) &&
                              ((sck_ptr->s_port_index < 0) ||
                               (TCP_Ports[sck_ptr->s_port_index]->state != SEST))) )
                        {
                            NLOG_Error_Log("Socket closed while suspending for buffers in NU_ZC_Allocate_Buffer",
                                           NERR_INFORMATIONAL, __FILE__, __LINE__);

                            status = NU_SOCKET_CLOSED;
                            break;
                        }
                    }

                    /* Do not suspend if this is not a blocking socket. */
                    else
                    {
                        break;
                    }
                }

                /* If a buffer was successfully allocated. */
                if (buf_ptr)
                {
                    /* Set the next_buffer pointer of the last buffer in the
                     * previous chain of buffers to the parent of the next
                     * chain of buffers.
                     */
                    if (last_buffer)
                    {
                        last_buffer->next_buffer = buf_ptr;
                    }

                    /* If this is the first buffer chain to be allocated, set
                     * the caller's buffer pointer to the head of this chain.
                     */
                    else
                    {
                        *zc_buf = buf_ptr;
                    }

                    /* Decrement total_size */
                    if ((UINT16)bytes_allocated < total_size)
                    {
                        total_size = (UINT16)(total_size - bytes_allocated);
                    }

                    else
                    {
                        total_size = 0;
                    }

                    /* Set the data pointer of the parent buffer */
                    buf_ptr->data_ptr = buf_ptr->mem_parent_packet + hdr_size;

                    /* Flag this first buffer as a parent in the chain of chains */
                    buf_ptr->mem_flags |= NET_PARENT;

                    /* If the next_buffer pointer is NULL, set last_buffer to this
                     * buffer.  It is the only buffer in the chain.
                     */
                    if (buf_ptr->next_buffer == NU_NULL)
                    {
                        last_buffer = buf_ptr;
                    }

                    else
                    {
                        /* Get a pointer to the next buffer */
                        buf_ptr = buf_ptr->next_buffer;

                        /* Set up the data pointers for the rest of the buffer
                         * chain
                         */
                        while (buf_ptr)
                        {
                            buf_ptr->data_ptr = buf_ptr->mem_parent_packet;

                            if (buf_ptr->next_buffer == NU_NULL)
                            {
                                last_buffer = buf_ptr;
                            }

                            /* Get a pointer to the next buffer */
                            buf_ptr = buf_ptr->next_buffer;
                        }
                    }
                }

                /* Otherwise, return an error */
                else
                {
                    /* Return an error. */
                    status = NU_NO_BUFFERS;

                    break;
                }
            }
        }

        else
        {
            /* Return an error. */
            status = NU_INVALID_PARM;
        }

        /* Release the semaphore */
        SCK_Release_Socket();
    }

    /* If an error did not occur. */
    if (status == NU_SUCCESS)
    {
        /* Return the number of bytes of data that were allocated */
        status = size - total_size;
    }

    return (status);

} /* NU_ZC_Allocate_Buffer */
