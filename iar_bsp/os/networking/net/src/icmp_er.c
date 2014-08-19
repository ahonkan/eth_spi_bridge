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

/***********************************************************************
*
*   FILE NAME
*
*       icmp_er.c
*
*   COMPONENT
*
*       ICMP - Internet Control Message Protocol
*
*   DESCRIPTION
*
*       This file contains the function to send an ICMP Echo Request.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       ICMP_Send_Echo_Request
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_SNMP == NU_TRUE)
#include SNMP_GLUE
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

extern ICMP_ECHO_LIST   ICMP_Echo_List;
extern UINT16           ICMP_Echo_Req_Seq_Num;

/***********************************************************************
*
*   FUNCTION
*
*       ICMP_Send_Echo_Request
*
*   DESCRIPTION
*
*       Send an ICMP echo request.
*
*   INPUTS
*
*       *dest_ip                A pointer to the address of the host
*                               to ping.
*       timeout                 The timeout value supplied in Nucleus
*                               PLUS clock ticks
*
*   OUTPUTS
*
*       NU_SUCCESS              If successful
*       NU_INVALID_PARM         dest_ip is NU_NULL
*       NU_NO_ACTION            If IP send fails
*       NU_MEM_ALLOC            No memory to allocate from
*       NU_NO_BUFFERS           No Net buffers to use
*
*************************************************************************/
STATUS ICMP_Send_Echo_Request(const UINT8 *dest_ip, UINT32 timeout)
{
    ICMP_ECHO_LIST_ENTRY    echo_entry;
    STATUS                  ret_status;
    OPTION               old_preempt;
    NET_BUFFER              *buf_ptr;
    ICMP_LAYER              *icmp_ptr;
    UINT8                   x, y;
    NU_SUPERV_USER_VARIABLES

    if (dest_ip == NU_NULL)
        return (NU_INVALID_PARM);

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Grab the stack semaphore */
    ret_status = NU_Obtain_Semaphore (&TCP_Resource, NU_SUSPEND);

    if (ret_status != NU_SUCCESS)
    {
        /* Return to user mode */
        NU_USER_MODE();

        return (ret_status);
    }

    /* Allocate a buffer for the echo request (ping) */
    buf_ptr = MEM_Buffer_Dequeue (&MEM_Buffer_Freelist);

    /* Make sure we got one. */
    if (buf_ptr != NU_NULL)
    {
        /* If the timeout is zero then used the default value. */
        if (!timeout)
            timeout = ICMP_DEFAULT_ECHO_TIMEOUT;

        /* Increment the sequence number. */
        ICMP_Echo_Req_Seq_Num++;

        /* Set the data pointer to the beginning of the buffer. */
        buf_ptr->data_ptr = buf_ptr->mem_parent_packet;

        /* Point to the ICMP layer */
        icmp_ptr = (ICMP_LAYER *)buf_ptr->data_ptr;

        /* Set the ICMP type to echo */
        PUT8(icmp_ptr, ICMP_TYPE_OFFSET, ICMP_ECHO);

        /* Set the code to zero, always zero for echo packets */
        PUT8(icmp_ptr, ICMP_CODE_OFFSET, 0);

        /* Set the ID and sequence number fields. */
        PUT16(icmp_ptr, ICMP_ID_OFFSET, ICMP_ECHO_REQ_ID);
        PUT16(icmp_ptr, ICMP_SEQ_OFFSET, ICMP_Echo_Req_Seq_Num);

        /* Fill in the standard 32 bytes of data. */
        for (x = 0, y = 'a'; x < ICMP_ECHO_REQUEST_LENGTH; x++)
        {
            /* Start back at A if we get to the end of the alphabet */
            if (y > 'w')
                y = 'a';

            /* store the byte of data */
            PUT8(icmp_ptr, ICMP_DATA_OFFSET + x, y++);
        }

        /* Set the length for this buffer. */
        buf_ptr->data_len = buf_ptr->mem_total_data_len =
            (ICMP_ECHO_REQ_HEADER_SIZE + ICMP_ECHO_REQUEST_LENGTH);

        /* Set the deallocation list */
        buf_ptr->mem_dlist = &MEM_Buffer_Freelist;

        /* Compute the checksum */
        PUT16(icmp_ptr, ICMP_CKSUM_OFFSET, 0);
        PUT16(icmp_ptr, ICMP_CKSUM_OFFSET, TLS_IP_Check_Buffer_Chain (buf_ptr) );

        /* Send this packet. */
        ret_status = IP_Send(buf_ptr, NU_NULL, IP_ADDR(dest_ip),
                             0, 0, 0, IP_ICMP_PROT, 0, NU_NULL);

        if (ret_status != NU_SUCCESS)
        {
            /* Release the stack semaphore */
            if (NU_Release_Semaphore (&TCP_Resource) != NU_SUCCESS)
                NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                                __FILE__, __LINE__);

            /* The packet was not sent.  Deallocate the buffer.  If the packet was
               transmitted it will be deallocated when the transmit complete
               interrupt occurs. */
            MEM_One_Buffer_Chain_Free(buf_ptr, buf_ptr->mem_dlist);

            /* Increment the number of send errors. */
            MIB2_icmpOutErrors_Inc;

            /* Set the status to an error. */
            ret_status = NU_NO_ACTION;
        }
        else
        {
            /* Increment the number of ICMP messages sent. */
            MIB2_icmpOutMsgs_Inc;

            /* Increment the number of ICMP echoes (pings) sent. */
            MIB2_icmpOutEchos_Inc;

            /* Fill in the echo entry structure. */
            echo_entry.icmp_requesting_task = NU_Current_Task_Pointer();
            echo_entry.icmp_echo_seq_num    = ICMP_Echo_Req_Seq_Num;
            echo_entry.icmp_echo_status = NU_TIMEOUT;

#if ((INCLUDE_ICMP_INFO_LOGGING == NU_TRUE) || (PRINT_ICMP_MSG == NU_TRUE))
            /* Store/Print the ICMP header info */
            NLOG_ICMP_Info(icmp_ptr, NLOG_TX_PACK);
#endif

            /* Place it on the echo entry list. */
            DLL_Enqueue(&ICMP_Echo_List, &echo_entry);

            /* Set an event to wake this task up if an echo reply
               is not RX. */
            if (TQ_Timerset(ICMP_ECHO_TIMEOUT, (UNSIGNED)echo_entry.icmp_echo_seq_num,
                            timeout, 0) != NU_SUCCESS)
                NLOG_Error_Log("Failed to set timer to timeout Echo Reply",
                               NERR_SEVERE, __FILE__, __LINE__);

            /* Change to no preemption so that no other task can get in between the
               time that we release the semaphore and self-suspend. */
            old_preempt = NU_Change_Preemption(NU_NO_PREEMPT);

            /* Release the stack semaphore */
            if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                                __FILE__, __LINE__);

            /* Self-suspend waiting on the ping reply or a timeout. */
            NU_Suspend_Task(echo_entry.icmp_requesting_task);

            /* Restore the previous preemption state. */
            NU_Change_Preemption(old_preempt);

            /* Assign the status to that in the ICMP entry. It was
               set by either the timeout or by the reception of
               the reply. */
            ret_status = echo_entry.icmp_echo_status;

            /* Remove this entry from the echo entry list. */
            DLL_Remove(&ICMP_Echo_List, &echo_entry);
        }
    }
    else
    {
        /* Release the stack semaphore */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);

        /* Increment the number of send errors due to the lack of buffers. */
        MIB2_icmpOutErrors_Inc;

        ret_status = NU_NO_BUFFERS;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (ret_status);

} /* ICMP_Send_Echo_Request */

#endif
