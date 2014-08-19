/*************************************************************************
*
*              Copyright 2002 Mentor Graphics Corporation
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
*       icmp6_er.c                                   
*                                                                       
*   DESCRIPTION                                                           
*              
*       This file contains the function to send an echo request.
*
*   DATA STRUCTURES
*
*       None
*                                                                       
*   FUNCTIONS                                                             
*                
*       ICMP6_Send_Echo_Request
*       ICMP6_Echo_Request_Build
*                                                                       
*   DEPENDENCIES                                                          
*        
*       nu_net.h
*       externs6.h
*       in6.h
*       ip6_mib.h
*       nud6.h
*                                                                       
*************************************************************************/

#include "networking/nu_net.h"
#include "networking/externs6.h"
#include "networking/in6.h"
#include "networking/ip6_mib.h"
#include "networking/nud6.h"

extern ICMP_ECHO_LIST   ICMP_Echo_List;
extern UINT16           ICMP_Echo_Req_Seq_Num;

/***********************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*      ICMP6_Send_Echo_Request                                           
*                                                                       
*   DESCRIPTION                                                           
*
*       This function transmits an ICMPv6 Echo Request.                                                                       
*                                                                       
*   INPUTS                                                                
*                                                      
*       *dest_ip                A pointer to the destination of the 
*                               request.
*       timeout                 The timeout value to wait for an ICMPv6 
*                               Echo Reply from the destination.                 
*                                                                       
*   OUTPUTS                                                               
*                                                  
*       NU_SUCCESS              Echo Request sent successfully.               
*       NU_INVALID_PARM         The destination IP pointer is NULL.
*       NU_NO_ACTION            IP_Send failed.
*       NU_MEM_ALLOC            Insufficient memory.
*       NU_NO_BUFFERS           Insufficient NET Buffers.
*       NU_NO_ROUTE_TO_HOST		No route could be found to the destination.
*                                                                       
*************************************************************************/
STATUS ICMP6_Send_Echo_Request(UINT8 *dest_ip, UINT32 timeout)
{
    ICMP_ECHO_LIST_ENTRY        echo_entry;
    STATUS                      ret_status;
    OPTION                   old_preempt;
    NET_BUFFER                  *buf_ptr;
    RTAB6_ROUTE                 ro;
    SCK6_SOCKADDR_IP            *dest;
    UINT16                      checksum;
    IP6_S_OPTIONS               ip6_options;
    
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Grab the stack semaphore */
    ret_status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (ret_status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain the semaphore", NERR_SEVERE, 
                       __FILE__, __LINE__);

        NU_USER_MODE();

        return (ret_status);
    }

    memset(&echo_entry, 0, sizeof(ICMP_ECHO_LIST_ENTRY));
    memset(&ro, 0, sizeof(RTAB6_ROUTE));

    /* Point to the destination. */
    dest = &ro.rt_ip_dest.rtab6_rt_ip_dest;

    dest->sck_family = SK_FAM_IP6;
    dest->sck_len = sizeof(SCK6_SOCKADDR_IP);
    NU_BLOCK_COPY(dest->sck_addr, dest_ip, IP6_ADDR_LEN);

    IP6_Find_Route(&ro);

    if (ro.rt_route)
    {
        RTAB_Free((ROUTE_ENTRY*)ro.rt_route, NU_FAMILY_IP6);

        buf_ptr = ICMP6_Echo_Request_Build(ICMP_ECHO_REQ_ID, 
                                           ICMP_Echo_Req_Seq_Num);

        if (buf_ptr != NU_NULL)
        {
            /* If the timeout is zero, use the default value. */
            if (!timeout)
                timeout = ICMP6_DEFAULT_ECHO_TIMEOUT;

            memset(&ip6_options, 0, sizeof(IP6_S_OPTIONS));            

            ip6_options.tx_source_address = 
                in6_ifawithifp(ro.rt_route->rt_entry_parms.rt_parm_device, dest_ip);
    
            if (ip6_options.tx_source_address)
            {
                /* Compute the ICMP checksum. */
                checksum = UTL6_Checksum(buf_ptr, ip6_options.tx_source_address, dest_ip, 
                                         buf_ptr->mem_total_data_len, IPPROTO_ICMPV6, 
                                         IPPROTO_ICMPV6);
    
                PUT16(buf_ptr->data_ptr, IP6_ICMP_CKSUM_OFFSET, checksum);

                ip6_options.tx_dest_address = dest_ip;
                ip6_options.tx_hop_limit = ICMP6_VALID_HOP_LIMIT;

                /* Increment the number of ICMP messages sent. */
                MIB_ipv6IfIcmpOutMsgs_Inc(ro.rt_route->rt_device);

                /* Send this packet. */
                ret_status = IP6_Send(buf_ptr, &ip6_options, IP_ICMPV6_PROT, &ro, 
                                      NU_NULL, NU_NULL);
 
                if (ret_status != NU_SUCCESS)
                {
                    /* Increment the number of send errors. */
                    MIB_ipv6IfIcmpOutErrors_Inc(ro.rt_route->rt_device);

                    NLOG_Error_Log("ICMPv6 Echo Request not sent", NERR_SEVERE, 
                                   __FILE__, __LINE__);
        
                    /* Release the stack semaphore */
                    if (NU_Release_Semaphore (&TCP_Resource) != NU_SUCCESS)
                        NLOG_Error_Log("Failed to release the semaphore", NERR_SEVERE, 
                                       __FILE__, __LINE__);
                
                    /* The packet was not sent.  Deallocate the buffer.  If the packet was
                     * transmitted it will be deallocated when the transmit complete
                     * interrupt occurs. 
                     */
                    MEM_One_Buffer_Chain_Free(buf_ptr, buf_ptr->mem_dlist);
                }
                else
                {
                    /* Increment the number of ICMP echoes (pings) sent. */
                    MIB_ipv6IfIcmpOutEchos_Inc(ro.rt_route->rt_device);
   
                    /* Fill in the echo entry structure. */
                    echo_entry.icmp_requesting_task = NU_Current_Task_Pointer();
                    echo_entry.icmp_echo_seq_num = ICMP_Echo_Req_Seq_Num;
                    echo_entry.icmp_echo_status = NU_TIMEOUT;

                    /* Place it on the echo entry list. */
                    DLL_Enqueue(&ICMP_Echo_List, &echo_entry);

                    /* Set an event to wake this task up if an echo reply
                       not not RX. */
#ifdef NET_5_3
                    if (TQ_Timerset(ICMP_ECHO_TIMEOUT, 
                                    (UNSIGNED)echo_entry.icmp_echo_seq_num,
                                    timeout, NU_NULL) != NU_SUCCESS)
                        NLOG_Error_Log("Failed to set the ICMPv6 echo timeout timer", 
                                       NERR_SEVERE, __FILE__, __LINE__);

#else

                    if (TQ_Timerset(ICMP_ECHO_TIMEOUT, (UNSIGNED)&echo_entry,
                                    timeout, NU_NULL) != NU_SUCCESS)
                        NLOG_Error_Log("Failed to set the ICMPv6 echo timeout timer", 
                                       NERR_SEVERE, __FILE__, __LINE__);
#endif
    
                    /* Change to no preemption so that no other task can get in between the
                       time that we release the semaphore and self-suspend. */
                    old_preempt = NU_Change_Preemption(NU_NO_PREEMPT);
    
                    /* Release the stack semaphore */
                    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                        NLOG_Error_Log("Failed to release the semaphore", 
                                       NERR_SEVERE, __FILE__, __LINE__);
    
                    /* Self-suspend waiting on the ping reply or a timeout. */
                    if (NU_Suspend_Task(echo_entry.icmp_requesting_task) != NU_SUCCESS)
                        NLOG_Error_Log("Failed to suspend the requesting task", 
                                       NERR_SEVERE, __FILE__, __LINE__);

                    /* Restore the previous preemption state. */
                    NU_Change_Preemption(old_preempt);
    
                    /* Assign the status to that in the ICMP entry. It was
                       set by either the timeout or by the reception of
                       the reply. */
                    ret_status = echo_entry.icmp_echo_status;
    
                    /* Remove this entry from the echo entry list. */
                    DLL_Remove(&ICMP_Echo_List, &echo_entry);   

                    /* Confirm reachability of this neighbor. */
                    if (ret_status == NU_SUCCESS)
                        NUD6_Confirm_Reachability_By_IP_Addr(dest_ip);
                }
            }   
            else
            {   
                /* Increment the number of send errors due to the lack of buffers. */
                MIB_ipv6IfIcmpOutErrors_Inc(ro.rt_route->rt_device);

                NLOG_Error_Log("ICMPv6 Echo Request could not be sent - no local address", 
                               NERR_SEVERE, __FILE__, __LINE__);
        
                /* Release the stack semaphore */
                if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to release the semaphore", 
                                   NERR_SEVERE, __FILE__, __LINE__);

                /* The packet was not sent.  Deallocate the buffer.  If the packet was
                   transmitted it will be deallocated when the transmit complete
                   interrupt occurs. */
                MEM_One_Buffer_Chain_Free(buf_ptr, buf_ptr->mem_dlist);
    
       
                ret_status = NU_NO_ACTION;
            }
        }
        else
        {
            /* Increment the number of send errors due to the lack of buffers. */
            MIB_ipv6IfIcmpOutErrors_Inc(ro.rt_route->rt_device);

            NLOG_Error_Log("ICMPv6 Echo Request could not be built", NERR_SEVERE, 
                           __FILE__, __LINE__);
    
            /* Release the stack semaphore */
            if (NU_Release_Semaphore (&TCP_Resource) != NU_SUCCESS)
                NLOG_Error_Log("Failed to release the semaphore", 
                               NERR_SEVERE, __FILE__, __LINE__);
   
            ret_status = NU_NO_BUFFERS;
        }
    }
    else
    {
        NLOG_Error_Log("ICMPv6 Echo Request could not be sent due to no route", 
                       NERR_SEVERE, __FILE__, __LINE__);
    
        /* Release the stack semaphore */
        if (NU_Release_Semaphore (&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release the semaphore", 
                           NERR_SEVERE, __FILE__, __LINE__);
   
        ret_status = NU_NO_ROUTE_TO_HOST;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (ret_status);

} /* ICMP6_Send_Echo_Request */

/***********************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       ICMP6_Echo_Request_Build                                          
*                                                                       
*   DESCRIPTION                                                           
*             
*       This function builds an ICMPv6 Echo Request packet.                                                          
*                                                                       
*   INPUTS                                                                
*                                                          
*       echo_id                 The Echo ID for the packet.
*       echo_seq                The Sequence number for the packet.
*                                                                       
*   OUTPUTS                                                               
*                                              
*       A pointer to the NET_BUFFER containing the ICMPv6 Echo Request or
*       NU_NULL if the packet could not be built.                         
*                                                                       
*************************************************************************/
NET_BUFFER *ICMP6_Echo_Request_Build(UINT16 echo_id, UINT16 echo_seq)
{
    NET_BUFFER  *buf_ptr;
    UINT8       x, y;

    /* Build the common fields of the ICMP header */
    buf_ptr = ICMP6_Header_Init(ICMP6_ECHO_REQUEST, 0, 
                                IP6_ECHO_REQUEST_HDR_SIZE);

    if (buf_ptr != NU_NULL)
    {
        /* Bump the sequence number. */
        ICMP_Echo_Req_Seq_Num++;

        /* Initialize the ICMP checksum to 0 */
        PUT16(buf_ptr->data_ptr, IP6_ICMP_CKSUM_OFFSET, 0);

        /* Set the ID and sequence number fields. */
        PUT16(buf_ptr->data_ptr, IP6_ICMP_ECHO_ID, echo_id);
        PUT16(buf_ptr->data_ptr, IP6_ICMP_ECHO_SEQ, echo_seq);
    
        /* Fill in the standard 32 bytes of data. */
        for (x = 0, y = 'a'; x <= 31; x++)
        {
            /* Start back at A if we get to the end of the alphabet */
            if (y > 'w')
                y = 'a';

            /* store the byte of data */
            PUT8(buf_ptr->data_ptr, IP6_ICMP_ECHO_DATA + x, y++);
    
            /* Bump the length */
            buf_ptr->data_len++;
        }

        buf_ptr->mem_total_data_len = buf_ptr->data_len;
    }
    else
        NLOG_Error_Log("No Buffers available", NERR_SEVERE, __FILE__, __LINE__);

    return (buf_ptr);

} /* ICMP6_Echo_Request_Build */
