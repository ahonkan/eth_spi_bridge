/*************************************************************************
*
*              Copyright 2012 Mentor Graphics Corporation
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
*       mdns.c
*
*   DESCRIPTION
*
*       This file contains the Multicast Domain Name System (DNS) component.
*       Given a host name or IP address, this component will discover the
*       matching IP address or host name using the mDNS protocol.
*
*   DATA STRUCTURES
*
*       NONE
*
*   FUNCTIONS
*
*       MDNS_Initialize
*       MDNS_Master_Task
*       MDNS_Is_Sender_On_Link
*       MDNS_Wake_Task
*       MDNS_Query_Handler
*       MDNS_Event_Handler
*       MDNS_Send_Query
*       MDNS_Send_Response
*       MDNS_Add_Additional_Answers
*       MDNS_Add_Known_Answers
*       MDNS_Continuous_Query
*       MDNS_TX_All_Interfaces
*       MDNS_TX_Packet
*       MDNS_Handle_Conflict
*       MDNS_Process_Incoming_Packet
*       MDNS_Find_Any_Record
*       MDNS_Process_Incoming_Probe_Query
*       MDNS_Process_Incoming_Response
*       MDNS_Find_Matching_Record
*       MDNS_Invoke_Probing
*       MDNS_Sort_Records
*       MDNS_Compare_Records
*       MDNS_Send_Announcement
*       MDNS_Send_Partial_Response
*       MDNS_Build_Probe
*       MDNS_Build_Query
*       MDNS_Initialize_Hostname
*       MDNS_Create_Default_Hostname
*       MDNS_Register_Local_Host
*       MDNS_Perform_Registration
*       MDNS_Remove_Local_Host
*       MDNS_Find_Query_By_Id
*       MDNS_Add_Query
*       MDNS_Find_Matching_Query_By_Name
*       MDNS_Find_Matching_Query_By_Data
*       MDNS_Set_Device_Hostname
*       NU_Set_MDNS_Hostname_Callback
*
*   DEPENDENCIES
*
*       nu_net.h
*       nu_net6.h
*       dns.h
*       dns4.h
*       dns6.h
*
*************************************************************************/

#include "networking/nu_net.h"
#include "networking/dns.h"

#if (INCLUDE_IPV4 == NU_TRUE)
#include "networking/dns4.h"
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/nu_net6.h"
#include "networking/dns6.h"
#endif

#if (INCLUDE_MDNS == NU_TRUE)

NU_TASK         NU_MDNS_Task_Ptr, NU_MDNS_Wake_Task_Ptr;
NU_QUEUE        MDNS_Queue;
INT             MDNS_Socket = -1;
NU_SEMAPHORE    MDNS_Resource;
DNS_SERVER_LIST MDNS_Servers;
MDNS_QUERY_LIST MDNS_Query_List;
NU_TIMER        MDNS_Response_Timer;
CHAR*           (*MDNS_Create_Hostname_Callback)(UINT32 index, BOOLEAN conflict);

#if (INCLUDE_IPV4 == NU_TRUE)
UINT8   MDNS_IPv4_Mcast_Addr[] = {224, 0, 0, 251};
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
UINT8   MDNS_IPv6_Mcast_Addr[] = {0xff, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xfb};
#endif

extern UINT8            SCK_Host_Name[];
extern DNS_HOST_LIST    DNS_Hosts;
extern UNSIGNED         DNS_Id;

VOID                MDNS_Master_Task(UNSIGNED argc, VOID *argv);
VOID                MDNS_Wake_Task(UNSIGNED argc, VOID *argv);
INT                 MDNS_Build_Probe(CHAR **buf_ptr, DNS_HOST *dns_ptr,
                                     DNS_HOST *dnsv4_ptr, DNS_HOST *dnsv6_ptr,
                                     CHAR *hostname);
STATUS              MDNS_Send_Announcement(INT socketd, struct addr_struct *addr,
                                           DNS_HOST *dnsv4_ptr, DNS_HOST *dnsv6_ptr,
                                           CHAR *hostname);
VOID                MDNS_Handle_Conflict(DNS_HOST *dns_ptr);
VOID                MDNS_Process_Incoming_Packet(CHAR *recv_buffer, struct addr_struct *addr);
MDNS_SORT_INDEX     *MDNS_Sort_Records(CHAR *record, INT16 count, CHAR *recv_buffer);
INT16               MDNS_Send_Partial_Response(DNS_HOST *dns_ptr, INT dev_index,
                                               CHAR *host_name, INT16 offset, CHAR **buffer,
                                               UINT16 type, UINT16 class, CHAR *data,
                                               UINT16 *ans_count, UINT16 *add_ans_count,
                                               INT16 total_size, UINT16 hdr_flags,
                                               struct addr_struct *addr);
INT                 MDNS_Compare_Records(MDNS_SORT_INDEX *a, MDNS_SORT_INDEX *b);
INT                 MDNS_Build_Query(CHAR **buf_ptr, CHAR *question, INT16 type, INT16 family);
VOID                MDNS_Continuous_Query(MDNS_QUERY *dns_ptr);
STATUS              MDNS_TX_All_Interfaces(CHAR *buffer, UINT16 size);
UINT16              MDNS_Process_Incoming_Response(DNS_HOST **dns_ptr, MDNS_QUERY *qry_ptr,
                                                   DNS_PKT_HEADER *pkt, CHAR *recv_buffer,
                                                   CHAR *name, INT count);
DNS_HOST            *MDNS_Find_Matching_Record(CHAR *recv_buffer, CHAR *pkt, CHAR *name);
UINT16              MDNS_Process_Incoming_Probe_Query(CHAR *recv_buffer, CHAR *name,
                                                      struct addr_struct *addr);
MDNS_QUERY          *MDNS_Find_Query_By_ID(UINT32 id);
VOID                MDNS_Send_Query(MDNS_QUERY *qry_ptr, DNS_HOST *dns_ptr);
INT32               MDNS_Add_Known_Answers(CHAR **buffer, DNS_HOST *dns_ptr, INT16 offset,
                                           UINT16 *answer_count, UINT16 *add_answer_count,
                                           INT32 total_size, INT dev_index,
                                           struct addr_struct *addr, UINT16 hdr_flags);
MDNS_QUERY          *MDNS_Find_Matching_Query_By_Name(CHAR *name, INT type);
STATUS              MDNS_TX_Packet(CHAR *buffer, UINT16 size, INT dev_index);
STATUS              MDNS_Is_Sender_On_Link(msghdr *msg, struct addr_struct *mdns_addr);
INT32               MDNS_Add_Additional_Answers(DNS_HOST *dns_ptr, CHAR **buffer,
                                                UINT16 *answer_count, UINT16 *add_answer_count,
                                                INT dev_index, struct addr_struct *addr,
                                                INT32 total_size);
DNS_HOST            *MDNS_Find_Any_Record(CHAR *recv_buffer, CHAR *pkt, CHAR *name);
CHAR                *MDNS_Create_Default_Hostname(UINT32 index, BOOLEAN conflict);

/****************************************************************************
*
*   FUNCTION
*
*       MDNS_Initialize
*
*   DESCRIPTION
*
*       This is the initialization routine for mDNS.  It creates all data
*       structures and tasks used by the mDNS module.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       NU_SUCCESS upon successful completion; otherwise, an operating-system
*       specific error.
*
******************************************************************************/
STATUS MDNS_Initialize(VOID)
{
    STATUS      status;
    VOID        *pointer;
    DNS_SERVER  *server_ptr;

    /* Initialize the mDNS server list. */
    MDNS_Servers.dnss_head = NU_NULL;
    MDNS_Servers.dnss_tail = NU_NULL;

    /* Initialize the mDNS query list. */
    MDNS_Query_List.head = NU_NULL;
    MDNS_Query_List.tail = NU_NULL;

    /* Set the default routine for creating new host names for interfaces. */
    MDNS_Create_Hostname_Callback = MDNS_Create_Default_Hostname;

    /* Set up the mDNS servers. */
#if (INCLUDE_IPV4 == NU_TRUE)
    status = NU_Allocate_Memory(MEM_Cached, (VOID**)&server_ptr,
                                (UNSIGNED)sizeof(DNS_SERVER),
                                (UNSIGNED)NU_NO_SUSPEND);

    if (status == NU_SUCCESS)
    {
        server_ptr->family = NU_FAMILY_IP;
        memcpy(server_ptr->dnss_ip, MDNS_IPv4_Mcast_Addr, IP_ADDR_LEN);

        DLL_Enqueue(&MDNS_Servers, server_ptr);
    }

    else
    {
        NLOG_Error_Log("Failed to add IPv4 mDNS server.", NERR_SEVERE,
                       __FILE__, __LINE__);
    }
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
    status = NU_Allocate_Memory(MEM_Cached, (VOID**)&server_ptr,
                                (UNSIGNED)sizeof(DNS_SERVER),
                                (UNSIGNED)NU_NO_SUSPEND);

    if (status == NU_SUCCESS)
    {
        server_ptr->family = NU_FAMILY_IP6;
        memcpy(server_ptr->dnss_ip, MDNS_IPv6_Mcast_Addr, IP6_ADDR_LEN);

        DLL_Enqueue(&MDNS_Servers, server_ptr);
    }

    else
    {
        NLOG_Error_Log("Failed to add IPv6 mDNS server.", NERR_SEVERE,
                       __FILE__, __LINE__);
    }
#endif

    /* Create the semaphore to protect the mDNS resources from simultaneous
     * access.  The mDNS thread does not run in the context of the networking
     * stack and should not use the TCP semaphore for protection.
     */
    status = NU_Create_Semaphore(&MDNS_Resource, "MDNS", (UNSIGNED)1, NU_FIFO);

    if (status == NU_SUCCESS)
    {
        memset(&MDNS_Response_Timer, 0, sizeof(NU_TIMER));

        /* Create a timer used to send out multicast responses. This timer gets set
         * in MDNS_Process_Incoming_Response after receiving a query and will
         * then send out all responses 500ms after the first query that started the timer.
         */
        status = NU_Create_Timer(&MDNS_Response_Timer, "MDNS_RT",
                                 MDNS_Response_Handler, 0, 0xffffffff, 0,
                                 NU_DISABLE_TIMER);

        if (status == NU_SUCCESS)
        {
            /* Create the MDNS Event queue that will be used to process expired timers
             * for retransmission of mDNS queries.
             */
            status = NU_Allocate_Memory(MEM_Cached, &pointer,
                                        (UNSIGNED)(MDNS_EVENT_Q_NUM_ELEMENTS *
                                        MDNS_EVENT_Q_ELEMENT_SIZE * sizeof(UNSIGNED)),
                                        (UNSIGNED)NU_NO_SUSPEND);

            if (status == NU_SUCCESS)
            {
                pointer = TLS_Normalize_Ptr(pointer);

                status = NU_Create_Queue(&MDNS_Queue, "mDNSQueue", pointer,
                                         (UNSIGNED)MDNS_EVENT_Q_NUM_ELEMENTS *
                                         MDNS_EVENT_Q_ELEMENT_SIZE, NU_FIXED_SIZE,
                                         (UNSIGNED)MDNS_EVENT_Q_ELEMENT_SIZE, NU_FIFO);

                if (status == NU_SUCCESS)
                {
                    /* Create mDNS master task that will perform probing, announcing and
                     * continuous resolution.
                     */
                    status = NU_Allocate_Memory(MEM_Cached, &pointer, (UNSIGNED)MDNS_STACK_SIZE,
                                                (UNSIGNED)NU_NO_SUSPEND);

                    if (status == NU_SUCCESS)
                    {
                        /* Create a thread that will perform probing, announcing and continuous
                         * resolution.
                         */
                        status = NU_Create_Task(&NU_MDNS_Task_Ptr, "mDNS_T1",
                                                MDNS_Master_Task, (UNSIGNED)0,
                                                NU_NULL, pointer, (UNSIGNED)MDNS_STACK_SIZE,
                                                MDNS_MASTER_TASK_PRIO, (UNSIGNED)MDNS_TIME_SLICE,
                                                NU_PREEMPT, NU_NO_START);

                        if (status == NU_SUCCESS)
                        {
                            /* Start the task */
                            status = NU_Resume_Task(&NU_MDNS_Task_Ptr);

                            if (status != NU_SUCCESS)
                                NLOG_Error_Log("Failed to resume task.", NERR_SEVERE,
                                               __FILE__, __LINE__);

                            /* Allocate memory for the wake task. */
                            status = NU_Allocate_Memory(MEM_Cached, &pointer,
                                                        (UNSIGNED)MDNS_WAKE_STACK_SIZE,
                                                        (UNSIGNED)NU_NO_SUSPEND);

                            if (status == NU_SUCCESS)
                            {
                                /* Create a thread that will wake the main task when a message is
                                 * sent to the queue.
                                 */
                                status = NU_Create_Task(&NU_MDNS_Wake_Task_Ptr, "mDNS_T2",
                                                        MDNS_Wake_Task, (UNSIGNED)0,
                                                        NU_NULL, pointer, (UNSIGNED)MDNS_WAKE_STACK_SIZE,
                                                        MDNS_WAKE_TASK_PRIO, (UNSIGNED)MDNS_TIME_SLICE,
                                                        NU_PREEMPT, NU_NO_START);

                                if (status == NU_SUCCESS)
                                {
                                    /* Start the task */
                                    status = NU_Resume_Task(&NU_MDNS_Wake_Task_Ptr);

                                    if (status != NU_SUCCESS)
                                        NLOG_Error_Log("Failed to resume task.", NERR_SEVERE,
                                                       __FILE__, __LINE__);
                                }
                            }
                        }

                        else
                        {
                            NLOG_Error_Log("Failed to create the mDNS master task.",
                                           NERR_FATAL, __FILE__, __LINE__);
                        }
                    }

                    else
                    {
                        NLOG_Error_Log("Failed to allocate memory for the mDNS master task.",
                                       NERR_FATAL, __FILE__, __LINE__);
                    }
                }

                else
                {
                    NLOG_Error_Log("Failed to create the mDNS queue.", NERR_FATAL,
                                   __FILE__, __LINE__);

                    return (status);
                }
            }
            else
            {
                NLOG_Error_Log("Failed to allocate memory for mDNS queue.",
                               NERR_FATAL, __FILE__, __LINE__);

                return (NU_MEM_ALLOC);
            }
        }
        else
        {
            NLOG_Error_Log("Failed to create responder timer.",
                                   NERR_FATAL, __FILE__, __LINE__);
        }

    }

    else
    {
        NLOG_Error_Log("Failed to create mDNS resource.",
                       NERR_FATAL, __FILE__, __LINE__);

        return (NU_MEM_ALLOC);
    }

    return (status);
} /* MDNS_Initialize */

/****************************************************************************
*
*   FUNCTION
*
*       MDNS_Master_Task
*
*   DESCRIPTION
*
*       This is the mDNS master task that handles receiving data on the
*       mDNS port and performing the work of events that have been triggered.
*
*   INPUTS
*
*       argc                    Unused.
*       argv                    Unused.
*
*   OUTPUTS
*
*       None.
*
******************************************************************************/
VOID MDNS_Master_Task(UNSIGNED argc, VOID *argv)
{
    STATUS              status;
    struct addr_struct  mdns_addr;
    FD_SET              readfs;
    UNSIGNED            receive_message[3], actual_size;
    CHAR                *buffer;
    INT32               byte_count;
    DNS_HOST            *dnsv4_ptr = NU_NULL, *dnsv6_ptr = NU_NULL, *dns_ptr;
    MDNS_QUERY          *qry_ptr;
    SCK_IOCTL_OPTION    ioctl_opt;
#if (INCLUDE_IPV4 == NU_TRUE)
    UINT8               ip_address[IP_ADDR_LEN];
#endif
    msghdr              msg;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Wait for the networking stack and drivers to complete initialization. */
    if (NETBOOT_Wait_For_Network_Up(NU_SUSPEND) != NU_SUCCESS)
        return;

    /* Set the family for the listening socket.  Note that IPv4 data can
     * be received over an IPv6 socket, but IPv6 data cannot be received
     * over an IPv4 socket.  If IPv6 is included, the socket should default
     * to IPv6 in order to receive both IPv4 and IPv6 data.
     */
#if (INCLUDE_IPV6 == NU_TRUE)
    mdns_addr.family = NU_FAMILY_IP6;
#else
    mdns_addr.family = NU_FAMILY_IP;
#endif

    /* Ensure the event handler doesn't try to wake up the socket before
     * it is initialized properly.
     */
    status = NU_Obtain_Semaphore(&MDNS_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Create a socket for sending and receiving mDNS data. */
        MDNS_Socket = NU_Socket(mdns_addr.family, NU_TYPE_DGRAM, 0);

        if (MDNS_Socket >= 0)
        {
#if (INCLUDE_IPV6 == NU_TRUE)
            /* Set the socket option to receive the destination address and
             * receive interface via NU_Recvmsg() for IPv6 data.
             */
            NU_Setsockopt_IPV6_RECVPKTINFO(MDNS_Socket, 1);
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
            /* Set the socket option to receive the destination address and
             * receive interface via NU_Recvmsg() for IPv4 data.
             */
            NU_Setsockopt_IP_PKTINFO(MDNS_Socket, 1);
#endif

            /* The msghdr is used by NU_Recvmsg() to receive data. */
            msg.msg_name = &mdns_addr;
            msg.msg_namelen = sizeof(mdns_addr);

            /* An ioctl will be used later to determine the number of bytes
             * on the receive socket.
             */
            ioctl_opt.s_optval = (UINT8*)&MDNS_Socket;

            /* Bind to the mDNS port and the wildcard address. */
            mdns_addr.port = MDNS_PORT;
            memset(mdns_addr.id.is_ip_addrs, 0, MAX_ADDRESS_SIZE);

            status = NU_Bind(MDNS_Socket, &mdns_addr, 0);

            /* NU_Bind returns the socket that was bound to upon success instead
             * of NU_SUCCESS.
             */
            if (status >= 0)
            {
                status = NU_Listen(MDNS_Socket, 10);
            }
        }

        else
        {
            status = NU_INVALID_SOCKET;
        }

        NU_Release_Semaphore(&MDNS_Resource);
    }

    /* If the socket was set up properly. */
    if (status == NU_SUCCESS)
    {
        /* Process events and/or receive data on the listening socket. */
        for (;;)
        {
            /* Check if there is an event pending on the queue.  If there is
             * not, the task will suspend on NU_Select until data is received
             * on the socket or until a timer expires that will wake up the
             * socket by setting an error on it.
             */
            status = NU_Receive_From_Queue(&MDNS_Queue, receive_message,
                                           (UNSIGNED)3, &actual_size,
                                           NU_NO_SUSPEND);

            if (status == NU_SUCCESS)
            {
                /* The Ethernet driver could go up and down at any point, in
                 * which case we would need to ensure the driver is all the
                 * way up and running before processing the event.
                 */
                if (NETBOOT_Wait_For_Network_Up(NU_SUSPEND) != NU_SUCCESS)
                {
                    continue;
                }

                /* Get the semaphore so another error does not get set on the
                 * socket after we clear it.  The event handler sets an error
                 * on the socket when an event is put on the queue to wake the
                 * listening socket up from this thread.
                 */
                status = NU_Obtain_Semaphore(&MDNS_Resource, NU_SUSPEND);

                if (status == NU_SUCCESS)
                {
                    /* Clear any error from the socket.  Keep the semaphore so
                     * another error is not set while processing; otherwise, any
                     * call to NU_Send or NU_Recv will return the error.
                     */
                    SCK_Clear_Socket_Error(MDNS_Socket);

                    /* If the message is for registering a multicast address. */
                    if (receive_message[0] == MDNS_REGISTER_DEVICE)
                    {
#if (INCLUDE_IPV4 == NU_TRUE)

                        /* If this is an IPv4 request. */
                        if (receive_message[1] == NU_FAMILY_IP)
                        {
                            /* Store the IP address as a pointer. */
                            PUT32(ip_address, 0, receive_message[2]);

                            /* Join the IPv4 multicast group for this socket on this interface. */
                            status = NU_IP_Multicast_Listen(MDNS_Socket, ip_address,
                                                            MDNS_IPv4_Mcast_Addr,
                                                            MULTICAST_FILTER_EXCLUDE,
                                                            NU_NULL, 0);

                            /* When the interface goes up and down, the TTL that was
                             * previously set for the socket is reverted.  Reset the
                             * The IP TTL to 255 per the RFC.
                             */
                            NU_Setsockopt_IP_MULTICAST_TTL(MDNS_Socket, 255);
                        }

#if (INCLUDE_IPV6 == NU_TRUE)
                        else
#endif
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
                        {
                            /* Join the IPv6 multicast group for this socket on this interface. */
                            status = NU_IP6_Multicast_Listen(MDNS_Socket,
                                                             receive_message[2],
                                                             MDNS_IPv6_Mcast_Addr,
                                                             MULTICAST_FILTER_EXCLUDE,
                                                             NU_NULL, 0);

                            /* When the interface goes up and down, the Hop Limit that was
                             * previously set for the socket is reverted.  Reset the
                             * The IP TTL to 255 per the RFC.
                             */
                            NU_Setsockopt_IPV6_MULTICAST_HOPS(MDNS_Socket, 255);
                        }
#endif
                    }

                    else if (receive_message[0] == MDNS_PROCESS_RECORD)
                    {
                        /* Obtain the semaphore. */
                        status = NU_Obtain_Semaphore(&DNS_Resource, NU_SUSPEND);

                        if (status != NU_SUCCESS)
                        {
                            NU_Release_Semaphore(&MDNS_Resource);
                            continue;
                        }

                        /* Get a pointer to the DNS entry. */
                        dns_ptr = DNS_Find_Host_By_ID(receive_message[1]);

                        /* The entry could have been removed.  This is not a reason
                         * to flag an error.
                         */
                        if (!dns_ptr)
                        {
                            NU_Release_Semaphore(&DNS_Resource);
                            NU_Release_Semaphore(&MDNS_Resource);
                            continue;
                        }

                        /* If the event was for a single query. */
                        if (dns_ptr->mdns_state == MDNS_QUERYING)
                        {
                            /* Retransmit the query four times. */
                            if (dns_ptr->mdns_retrans_count < 4)
                            {
                                /* Send a query for this one record. */
                                MDNS_Send_Query(dns_ptr->mdns_query, dns_ptr);

                                dns_ptr->mdns_retrans_count ++;
                            }

                            /* If after four queries no answer is received, the record is
                             * deleted when it reaches 100% of its lifetime.
                             */
                            else
                            {
                                DNS_Delete_Host(dns_ptr);
                            }
                        }

                        else
                        {
#if (INCLUDE_IPV6 == NU_TRUE)
                            /* If this is an IPv6 host timer. */
                            if (dns_ptr->dns_h_length == IP6_ADDR_LEN)
                            {
                                dnsv6_ptr = dns_ptr;

                                /* Set up the destination address. */
                                memcpy(mdns_addr.id.is_ip_addrs, MDNS_IPv6_Mcast_Addr, IP6_ADDR_LEN);
                                mdns_addr.family = NU_FAMILY_IP6;

                                /* Set the interface for the message to be transmitted from. */
                                NU_Setsockopt_IPV6_MULTICAST_IF(MDNS_Socket,
                                                                dnsv6_ptr->mdns_dev_index);
                            }

                            else
                            {
                                /* Get a pointer to the IPv6 host entry. */
                                dnsv6_ptr = DNS_Find_Matching_Host_By_Name(dns_ptr->dns_name,
                                                                           NU_NULL, DNS_TYPE_AAAA);
                            }
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
                            /* If this is an IPv4 host timer. */
                            if (dns_ptr->dns_h_length == IP_ADDR_LEN)
                            {
                                dnsv4_ptr = dns_ptr;

                                /* Set up the destination address. */
                                memcpy(mdns_addr.id.is_ip_addrs, MDNS_IPv4_Mcast_Addr, IP_ADDR_LEN);
                                mdns_addr.family = NU_FAMILY_IP;

                                /* Set the interface for the message to be transmitted from. */
                                NU_Setsockopt_IP_MULTICAST_IF(MDNS_Socket,
                                                              (UINT8*)dnsv4_ptr->dns_record_data);
                            }

                            else
                            {
                                /* Get a pointer to the IPv4 host entry. */
                                dnsv4_ptr = DNS_Find_Matching_Host_By_Name(dns_ptr->dns_name,
                                                                           NU_NULL, DNS_TYPE_A);
                            }
#endif

                            /* The outgoing port is always 5353, but since we are reusing the
                             * address structure to receive data, we must reset the value each
                             * time around the loop.
                             */
                            mdns_addr.port = MDNS_PORT;

                            /* If the state of the record is probing. */
                            if (dns_ptr->mdns_state == MDNS_PROBING)
                            {
                                /* If the max retransmissions have been sent. */
                                if (dns_ptr->mdns_retrans_count == 3)
                                {
                                    /* Change the state to announcing. */
                                    dns_ptr->mdns_state = MDNS_ANNOUNCING;

                                    /* Transmit 2 responses during announcing. */
                                    dns_ptr->mdns_retrans_count = 0;
                                }
                            }

                            /* If the record is still in the probing state. */
                            if (dns_ptr->mdns_state == MDNS_PROBING)
                            {
                                /* Build the outgoing query for the probe. */
                                byte_count = MDNS_Build_Probe(&buffer, dns_ptr, dnsv4_ptr,
                                                              dnsv6_ptr, dns_ptr->dns_name);

                                /* Increment retransmission count. */
                                if (byte_count > 0)
                                {
                                    /* Send the DNS query. */
                                    byte_count = NU_Send_To(MDNS_Socket, buffer, (UINT16)byte_count,
                                                            0, &mdns_addr, 0);

                                    /* Deallocate the buffer. */
                                    NU_Deallocate_Memory(buffer);
                                    buffer = NU_NULL;

                                    /* If the probe was successfully sent, increment the
                                     * retransmission counter.
                                     */
                                    if (byte_count > 0)
                                    {
                                        dns_ptr->mdns_retrans_count ++;
                                    }

                                    /* Validate the timer and reset it to transmit the
                                     * next query in 250 ms.  If the query could not be built,
                                     * give up on probing.
                                     */
                                    if (dns_ptr->mdns_timer)
                                        NU_Reset_Timer(dns_ptr->mdns_timer, MDNS_Event_Handler,
                                                       MDNS_PROBE_DELAY, 0, NU_ENABLE_TIMER);
                                }

                                else
                                {
                                    /* Clear the host name. */
                                    MDNS_Set_Device_Hostname(NU_NULL, dns_ptr->mdns_dev_index);
                                }
                            }

                            /* If the record is not unique. */
                            else if (dns_ptr->mdns_state == MDNS_CONFLICTED)
                            {
                                MDNS_Handle_Conflict(dns_ptr);
                            }

                            /* If the record is in the announcing state. */
                            else if (dns_ptr->mdns_state == MDNS_ANNOUNCING)
                            {
                                /* If the other family's record already started the
                                 * announcing phase, don't duplicate announcing for
                                 * this family's record since the announcement sent
                                 * by the other record includes this record.
                                 */
#if ( (INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE) )
                                if ( ((dns_ptr == dnsv4_ptr) &&
                                      ((dnsv6_ptr == NU_NULL) ||
                                       (dnsv6_ptr->mdns_state != MDNS_ANNOUNCING) ||
                                       (dnsv6_ptr->mdns_retrans_count == 0))) ||
                                     ((dns_ptr == dnsv6_ptr) &&
                                      ((dnsv4_ptr == NU_NULL) ||
                                       (dnsv4_ptr->mdns_state != MDNS_ANNOUNCING) ||
                                       (dnsv4_ptr->mdns_retrans_count == 0))) )
#endif
                                {
                                    /* Send the outgoing response over both IPv4 and IPv6. */
                                    if (MDNS_Send_Announcement(MDNS_Socket, NU_NULL, dnsv4_ptr,
                                                               dnsv6_ptr, dns_ptr->dns_name) == NU_SUCCESS)
                                    {
                                        dns_ptr->mdns_retrans_count ++;
                                    }

                                    /* Send two announcements. */
                                    if ( (dns_ptr->mdns_retrans_count < 2) &&
                                         (dns_ptr->mdns_timer) )
                                    {
                                        /* Set the timer to send a second announcement. */
                                        NU_Reset_Timer(dns_ptr->mdns_timer, MDNS_Event_Handler,
                                                       MDNS_ONE_SEC_DELAY, 0, NU_ENABLE_TIMER);
                                    }
                                }
                            }
                        }

                        NU_Release_Semaphore(&DNS_Resource);
                    }

                    /* If this is a message to transmit a query. */
                    else if (receive_message[0] == MDNS_PROCESS_QUERY)
                    {
                        /* Obtain the semaphore. */
                        status = NU_Obtain_Semaphore(&DNS_Resource, NU_SUSPEND);

                        if (status == NU_SUCCESS)
                        {
                            /* Get a pointer to the query entry. */
                            qry_ptr = MDNS_Find_Query_By_ID(receive_message[1]);

                            /* If the state is continuous query. */
                            if (qry_ptr)
                            {
                                MDNS_Continuous_Query(qry_ptr);
                            }

                            NU_Release_Semaphore(&DNS_Resource);
                        }
                    }

                    /* If this message is to process all our responses flagged as DNS_DELAY_RESPONSE*/
                    else if (receive_message[0] == MDNS_PROCESS_RESPONSES)
                    {
                        /* Obtain the semaphore. */
                        status = NU_Obtain_Semaphore(&DNS_Resource, NU_SUSPEND);

                        if (status == NU_SUCCESS)
                        {
                            /* Send out all responses to queries we have received within the last
                             * 500ms.
                             * NOTE: MDNS_Response_Timer is used to send this message and its timer
                             * doesn't start until it receives a query.
                             */
                            MDNS_Send_Response(NU_NULL, NU_NULL);
                            NU_Release_Semaphore(&DNS_Resource);
                        }
                    }

                    /* Release the semaphore. */
                    NU_Release_Semaphore(&MDNS_Resource);
                }
            }

            else
            {
                /* Initially all the bits should be cleared. */
                NU_FD_Init(&readfs);

                /* Specify which socket we want to select on. */
                NU_FD_Set(MDNS_Socket, &readfs);

                status = NU_Select(MDNS_Socket + 1, &readfs, NU_NULL, NU_NULL, NU_SUSPEND);

                /* Process the incoming data. */
                if (status == NU_SUCCESS)
                {
                    /* Ensure the socket is ready for reading. */
                    if (NU_FD_Check(MDNS_Socket, &readfs) == NU_TRUE)
                    {
                        /* Determine the number of bytes on the socket. */
                        status = NU_Ioctl_FIONREAD(&ioctl_opt);

                        /* Get the semaphore so the event handler cannot set an
                         * error until after we are in NU_Select again; otherwise
                         * any send/receive will return the error.
                         */
                        status = NU_Obtain_Semaphore(&MDNS_Resource, NU_SUSPEND);

                        if (status == NU_SUCCESS)
                        {
                            /* Clear any error from the socket. */
                            SCK_Clear_Socket_Error(MDNS_Socket);

                            /* If there is data on the socket. */
                            if (ioctl_opt.s_ret.sck_bytes_pending > 0)
                            {
                                /* Allocate memory for the buffer. */
                                status = NU_Allocate_Memory(MEM_Cached, (VOID **)&buffer,
                                                            ioctl_opt.s_ret.sck_bytes_pending,
                                                            NU_SUSPEND);

                                if (status == NU_SUCCESS)
                                {
                                    /* Set up the msghdr structure. */
                                    msg.msg_iov = buffer;
                                    msg.msg_iovlen = ioctl_opt.s_ret.sck_bytes_pending;

                                    /* Initialize the length. */
                                    msg.msg_controllen = 0;

#if (INCLUDE_IPV6 == NU_TRUE)
                                    /* Set the length of the ancillary data */
                                    msg.msg_controllen = NU_CMSG_SPACE(sizeof(in6_pktinfo));
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
                                    /* Set the length of the ancillary data */
                                    msg.msg_controllen += NU_CMSG_SPACE(sizeof(in_pktinfo));
#endif

                                    /* Allocate memory for the ancillary data */
                                    NU_Allocate_Memory(&System_Memory, (VOID**)&msg.msg_control,
                                                       msg.msg_controllen, NU_NO_SUSPEND);

                                    /* Zero out the memory. */
                                    memset(msg.msg_control, 0, msg.msg_controllen);

                                    /* Receive the incoming data. */
                                    byte_count = NU_Recvmsg(MDNS_Socket, &msg, 0);

                                    if (byte_count > 0)
                                    {
                                        /* Verify that the sender is on-link. */
                                        status = MDNS_Is_Sender_On_Link(&msg, &mdns_addr);

                                        /* Do not process the packet if the sender is not
                                         * on-link.
                                         */
                                        if (status == NU_TRUE)
                                        {
                                            /* Obtain the semaphore. */
                                            status = NU_Obtain_Semaphore(&DNS_Resource, NU_SUSPEND);

                                            if (status == NU_SUCCESS)
                                            {
                                                /* Process the packet. */
                                                MDNS_Process_Incoming_Packet(buffer, &mdns_addr);

                                                NU_Release_Semaphore(&DNS_Resource);
                                            }
                                        }
                                    }

                                    NU_Deallocate_Memory(buffer);
                                    buffer = NU_NULL;

                                    /* Deallocate the memory for the ancillary data. */
                                    NU_Deallocate_Memory(msg.msg_control);
                                }
                            }

                            NU_Release_Semaphore(&MDNS_Resource);
                        }
                    }
                }
            }
        }
    }

    NU_USER_MODE();

} /* MDNS_Master_Task */

/****************************************************************************
*
*   FUNCTION
*
*       MDNS_Is_Sender_On_Link
*
*   DESCRIPTION
*
*       This routine checks that an incoming packet was sent from a node
*       on the same link as the local host.
*
*   INPUTS
*
*       *msg                    A pointer to the ancillary data returned by
*                               NU_Recvmsg().
*       *mdns_addr              A pointer to the address structure containing
*                               the sender's address information.
*
*   OUTPUTS
*
*       NU_TRUE                 The sender is on-link.
*       NU_FALSE                The sender is not on-link.
*
******************************************************************************/
STATUS MDNS_Is_Sender_On_Link(msghdr *msg, struct addr_struct *mdns_addr)
{
    STATUS              status = NU_FALSE;
    cmsghdr             *cmsg;
    ROUTE_ENTRY         *rt_ptr;
#if (INCLUDE_IPV4 == NU_TRUE)
    in_pktinfo          *pktv4_info_ptr;
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
    in6_pktinfo         *pkt_info_ptr;
#endif

    /* If ancillary data was returned. */
    if (msg->msg_controllen)
    {
        /* Process the ancillary data. */
        for (cmsg = NU_CMSG_FIRSTHDR(msg);
             cmsg != NU_NULL;
             cmsg = NU_CMSG_NXTHDR(msg, cmsg))
        {
#if (INCLUDE_IPV6 == NU_TRUE)
            if ( (cmsg->cmsg_level == IPPROTO_IPV6) &&
                 (cmsg->cmsg_type == IPV6_PKTINFO) )
            {
                /* Extract the destination address and interface index on
                 * which this packet was received.
                 */
                pkt_info_ptr = (in6_pktinfo*)NU_CMSG_DATA(cmsg);

                /* If the destination address is the mDNS multicast address
                 * or the source address is link-local.
                 */
                if ( (memcmp(pkt_info_ptr->ipi6_addr,
                             MDNS_IPv6_Mcast_Addr, IP6_ADDR_LEN) == 0) ||
                     (IPV6_IS_ADDR_LINKLOCAL(mdns_addr->id.is_ip_addrs)) )
                {
                    status = NU_TRUE;
                }

                /* Check if the source address is on-link. */
                else
                {
                    /* Find a route to the source through the destination address. */
                    rt_ptr = NU_Find_Route_By_Gateway(mdns_addr->id.is_ip_addrs,
                                                      pkt_info_ptr->ipi6_addr,
                                                      NU_FAMILY_IP6, 0);

                    /* If there is a route, and the route does not use the default
                     * gateway, this address is on-link.
                     */
                    if ( (rt_ptr) &&
                         (!(IP_IS_NULL_ADDR(rt_ptr->rt_route_node->rt_ip_addr))) )
                    {
                        status = NU_TRUE;
                    }
                }
            }
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
            if ( (cmsg->cmsg_level == IPPROTO_IP) &&
                 (cmsg->cmsg_type == IP_PKTINFO) )
            {
                /* Extract the destination address and interface
                 * index on which this packet was received.
                 */
                pktv4_info_ptr = (in_pktinfo*)NU_CMSG_DATA(cmsg);

                /* If the destination address is the IPv4 multicast address. */
                if (memcmp(pktv4_info_ptr->ipi_addr, MDNS_IPv4_Mcast_Addr,
                           IP_ADDR_LEN) == 0)
                {
                    status = NU_TRUE;
                }

                else
                {
                    /* Find a route to the source through the destination address. */
                    rt_ptr = NU_Find_Route_By_Gateway(mdns_addr->id.is_ip_addrs,
                                                      pktv4_info_ptr->ipi_addr,
                                                      NU_FAMILY_IP, 0);

                    /* If there is a route, and the route does not use the default
                     * gateway, this address is on-link.
                     */
                    if ( (rt_ptr) && (!(IP_ADDR(rt_ptr->rt_route_node->rt_ip_addr) == 0)) )
                    {
                        status = NU_TRUE;
                    }
                }
            }
#endif
        }
    }

    return (status);

} /* MDNS_Is_Sender_On_Link */

/****************************************************************************
*
*   FUNCTION
*
*       MDNS_Wake_Task
*
*   DESCRIPTION
*
*       This thread handles waking the mDNS socket if it is suspended on
*       NU_Select by setting an error on the socket.  This thread is
*       resumed by MDNS_Event_Handler, which expires when a Nucleus timer
*       expires.  Since the callback routine for a Nucleus timer cannot
*       suspend, this thread is required to set the error on the socket.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       None.
*
******************************************************************************/
VOID MDNS_Wake_Task(UNSIGNED argc, VOID *argv)
{
    struct sock_struct  *sockptr;

    for (;;)
    {
        /* Suspend until data is sent to the queue. */
        NU_Suspend_Task(&NU_MDNS_Wake_Task_Ptr);

        /* If the Master_Task has the semaphore, the socket error has been
         * cleared and the thread will loop back around to the queue before
         * suspending in NU_Select.  If the Master_Task does not have the
         * semaphore, it is either checking the queue or suspended in
         * NU_Select, so there is no need to suspend on the semaphore here.
         */
        if (NU_Obtain_Semaphore(&MDNS_Resource, NU_NO_SUSPEND) == NU_SUCCESS)
        {
            /* Get the TCP semaphore since we are about to manipulate the
             * internal socket structure.
             */
            if ( (MDNS_Socket >= 0) &&
                 (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS) )
            {
                /* Get a pointer to the socket list entry. */
                sockptr = SCK_Sockets[MDNS_Socket];

                /* Resume the thread that is suspend on select. */
                SCK_Set_Socket_Error(sockptr, NU_NO_DATA_TRANSFER);

                NU_Release_Semaphore(&TCP_Resource);
            }

            NU_Release_Semaphore(&MDNS_Resource);
        }
    }

} /* MDNS_Wake_Task */

/****************************************************************************
*
*   FUNCTION
*
*       MDNS_Query_Handler
*
*   DESCRIPTION
*
*       This function handles sending data to the Master_Task queue.  This
*       function is called from the context of a Nucleus timer and therefore
*       cannot suspend.
*
*   INPUTS
*
*       dns_id                  The DNS host index of the expiring entry.
*
*   OUTPUTS
*
*       None.
*
******************************************************************************/
VOID MDNS_Query_Handler(UNSIGNED dns_id)
{
    UNSIGNED    data[3];

    data[0] = MDNS_PROCESS_QUERY;
    data[1] = dns_id;
    data[2] = 0; /* Unused for this event. */

    /* Send the data to the queue. */
    if (NU_Send_To_Queue(&MDNS_Queue, data, (UNSIGNED)3,
                         (UNSIGNED)NU_NO_SUSPEND) == NU_SUCCESS)
    {
        /* Resume the task that can wake up the socket.  Since we need the
         * TCP semaphore to wake up a socket, and we will have to suspend
         * on that semaphore, this has to be done from another task.
         */
        NU_Resume_Task(&NU_MDNS_Wake_Task_Ptr);
    }

} /* MDNS_Query_Handler */

/****************************************************************************
*
*   FUNCTION
*
*       MDNS_Event_Handler
*
*   DESCRIPTION
*
*       This function handles sending data to the Master_Task queue.  This
*       function is called from the context of a Nucleus timer and therefore
*       cannot suspend.
*
*   INPUTS
*
*       dns_id                  The DNS host index of the expiring entry.
*
*   OUTPUTS
*
*       None.
*
******************************************************************************/
VOID MDNS_Event_Handler(UNSIGNED dns_id)
{
    UNSIGNED    data[3];

    data[0] = MDNS_PROCESS_RECORD;
    data[1] = dns_id;
    data[2] = 0; /* Unused for this event. */

    /* Send the data to the queue. */
    if (NU_Send_To_Queue(&MDNS_Queue, data, (UNSIGNED)3,
                         (UNSIGNED)NU_NO_SUSPEND) == NU_SUCCESS)
    {
        /* Resume the task that can wake up the socket.  Since we need the
         * TCP semaphore to wake up a socket, and we will have to suspend
         * on that semaphore, this has to be done from another task.
         */
        NU_Resume_Task(&NU_MDNS_Wake_Task_Ptr);
    }

} /* MDNS_Event_Handler */

/****************************************************************************
*
*   FUNCTION
*
*       MDNS_Response_Handler
*
*   DESCRIPTION
*
*       This function is executed 500ms after receiving a query that needs a multicast
*       response. This is done to help alleviate network traffic by not flooding the network
*       everytime we receive a query. All the query's responses that need to be executed
*       during that 500ms will have their database records flaged with
*       records that need to be mulitcast as a response. Pending database records will have
*       the DNS_DELAY_RESPONSE flag set in their DNS_HOST->dns_flags member.
*
*   INPUTS
*
*       dns_id                  The DNS host index of the expiring entry.
*
*   OUTPUTS
*
*       None.
*
******************************************************************************/
VOID MDNS_Response_Handler(UNSIGNED dns_id)
{
    UNSIGNED    data[3];

    data[0] = MDNS_PROCESS_RESPONSES;
    data[1] = dns_id;
    data[2] = 0; /* Unused for this event. */

    /* Send the data to the queue. */
    if (NU_Send_To_Queue(&MDNS_Queue, data, (UNSIGNED)3,
                         (UNSIGNED)NU_NO_SUSPEND) == NU_SUCCESS)
    {
        /* Resume the task that can wake up the socket.  Since we need the
         * TCP semaphore to wake up a socket, and we will have to suspend
         * on that semaphore, this has to be done from another task.
         */
        NU_Resume_Task(&NU_MDNS_Wake_Task_Ptr);
    }

} /* MDNS_Response_Handler */

/****************************************************************************
*
*   FUNCTION
*
*       MDNS_Send_Query
*
*   DESCRIPTION
*
*       This function builds and transmits a query out each interface for a
*       local host record that is undergoing a continuous query.
*
*   INPUTS
*
*       *qry_ptr                A pointer to the record to query.
*       *dns_ptr                A pointer to the known-answer.
*
*   OUTPUTS
*
*       None.
*
******************************************************************************/
VOID MDNS_Send_Query(MDNS_QUERY *qry_ptr, DNS_HOST *dns_ptr)
{
    CHAR                *buffer;
    INT32               total_size;
    UINT16              answer_count = 0;
    INT16               offset;

    /* If the query type does not match the known-answer type, this is a
     * DNS-SD record pairing - there will be just one query associated with
     * many DNS records of different types.  Use the name in the known-answer
     * for the query.
     */
    if (qry_ptr->mdns_type != dns_ptr->dns_type)
    {
        /* Build the header and query section of the DNS query packet. */
        total_size = MDNS_Build_Query(&buffer, dns_ptr->dns_name, dns_ptr->dns_type,
                                      dns_ptr->dns_family);

        /* Use header compression in the answer section. */
        offset = DNS_FIXED_HDR_LEN;
    }

    else
    {
        /* Build the header and query section of the DNS query packet. */
        total_size = MDNS_Build_Query(&buffer, qry_ptr->mdns_data, qry_ptr->mdns_type,
                                      qry_ptr->mdns_family);

        /* If the query name matches the hostname of this record, use header
         * compression.
         */
        if (strcmp(qry_ptr->mdns_data, dns_ptr->dns_name) == 0)
        {
            offset = DNS_FIXED_HDR_LEN;
        }
        else
        {
            offset = 0;
        }
    }

    /* Add any known answers to the query and transmit the packet. */
    if (total_size > 0)
    {
        /* Add this record to the answer section. */
        total_size = MDNS_Add_Known_Answers(&buffer, dns_ptr, offset, &answer_count,
                                            NU_NULL, total_size, -1, NU_NULL, 0);

        /* If a buffer needs to be deallocated. */
        if (buffer)
        {
            /* If there is still data left to send and an error did not occur. */
            if ( (total_size > 0) && (total_size != DNS_FIXED_HDR_LEN) )
            {
                /* Set the number of answers. */
                PUT16(buffer, DNS_ANCOUNT_OFFSET, answer_count);

                /* Send the data out all interfaces. */
                MDNS_TX_All_Interfaces(buffer, (UINT16)total_size);
            }

            /* Deallocate the buffer allocated by MDNS_Build_Query(). */
            NU_Deallocate_Memory(buffer);
        }
    }

} /* MDNS_Send_Query */

/****************************************************************************
*
*   FUNCTION
*
*       MDNS_Send_Response
*
*   DESCRIPTION
*
*       This function builds and transmits a response.
*
*   INPUTS
*
*       *dns_ptr                A pointer to the record to transmit or NU_NULL
*                               to transmit all records stored in the database.
*       *addr                   If it is NU_NULL then multicast response, otherwise
*                               send response to specific address.
*
*   OUTPUTS
*
*       None.
*
******************************************************************************/
VOID MDNS_Send_Response(DNS_HOST *dns_ptr, struct addr_struct *addr)
{
    CHAR                *buffer;
    INT32               total_size;
    UINT16              answer_count = 0, add_answer_count = 0;
    DNS_HOST            *l_host, *tmp_ptr = NU_NULL;
    DNS_HOST            *dns_to_del;
    UNSIGNED            time_left = 0;

    /* Build the response header.  If the record is unique, add the AA flag. */
    total_size = DNS_Build_Header((VOID **)&buffer, 0, DNS_QR |
                                  (((dns_ptr) && (dns_ptr->dns_flags & DNS_UNIQUE_RECORD)) ? DNS_AA : 0));

    /* Add the answers to the response and transmit the packet. */
    if (total_size > 0)
    {
        /* The fixed header is the only thing in the packet at this time. */
        total_size = DNS_FIXED_HDR_LEN;

        /* If the dns_ptr is NU_NULL then transmit all the records that are flagged
         * for transmission.
         */
        if (dns_ptr == NU_NULL)
        {
            for (l_host = DNS_Hosts.dns_head; l_host; l_host = l_host->dns_next)
            {
                if ((l_host->dns_flags & DNS_DELAY_RESPONSE) == DNS_DELAY_RESPONSE)
                {
                    /* Add this record to the answer section. */
                    total_size = MDNS_Add_Known_Answers(&buffer, l_host, 0, &answer_count,
                                                        NU_NULL, total_size, -1,
                                                        addr, DNS_QR);

                    /* If there was an error sending our response, abort sending the
                     * remaining responses and reset the response timer. Hopefully the
                     * responses that didn't get sent will be sent next time.
                     */
                    if (total_size < 0)
                    {
                        /* dns_ptr should only be NU_NULL when called from
                         * MDNS_Response_Timer, so MDNS_Response_Timer will be disabled,
                         * however best to double check just to be on the safe side.
                         * NU_Get_Remaining_Time will return an error if our timer is
                         * currently disabled.
                         */
                        if ( (NU_Get_Remaining_Time(&MDNS_Response_Timer, &time_left) != NU_SUCCESS) ||
                             (time_left <= 0) )
                        {
                            /* Reset the timer to fire in 500ms. */
                            NU_Reset_Timer(&MDNS_Response_Timer, MDNS_Response_Handler,
                                           MDNS_RESPONSE_DELAY, 0, NU_ENABLE_TIMER);
                        }
                        break;
                    }

                    /* If there wasn't an error sending the response, mark as sent. */
                    else
                    {
                        /* Update our ttl update timestamp. */
                        l_host->mdns_last_local_resp_time = NU_Retrieve_Clock();

                        /* If this is a PTR record for a service, we will need to add
                         * some additional answers to the packet below.  Do not unflag
                         * this record yet.  Do not send additional records for PTR
                         * records that are being timed out since it would be a waste
                         * of network resources.
                         */
                        if ( (l_host->dns_ttl == 0) ||
                             (l_host->dns_type != DNS_TYPE_PTR) ||
                             (l_host->dns_family != NU_FAMILY_UNSPEC) )
                        {
                            /* Clear flag indicating the response was sent. */
                            l_host->dns_flags ^= DNS_DELAY_RESPONSE;
                        }

                        else
                        {
                            /* Save a pointer to the first PTR service record
                             * found so we don't have to traverse the entire list
                             * later when adding additional records.
                             */
                            if (!tmp_ptr)
                            {
                                tmp_ptr = l_host;
                            }
                        }
                    }

                    /* If the dns record's ttl is zero then it needs to be removed from
                     * the database(DNS_HOSTS).
                     */
                    if (l_host->dns_ttl == 0)
                    {
                        /* A pointer is needed to the record that will be removed so that
                         * the place in DNS_HOST doesn't get lost. This is needed because
                         * DLL_Remove returns the next dns record in the list and our loop
                         * will increment to the next record, which means l_host's pointer
                         * will need to be set back one. However, we still need pointer to
                         * record that was removed so that it can be deleted.
                         */
                        dns_to_del = l_host;

                        /* Remove this record from the database. */
                        l_host = DLL_Remove(&DNS_Hosts, dns_to_del);

                        /* Deallocate the memory for this host. */
                        NU_Deallocate_Memory(dns_to_del);

                        /* Because DLL_Remove returns the next dns record in the list
                         * and our loop will increment to the next record we need to
                         * move this pointer back one otherwise this record will be skipped.
                         */
                        if (l_host)
                        {
                            l_host = l_host->dns_previous;
                        }

                        else
                        {
                            break;
                        }
                    }
                }
            }

            /* If any of the records that were added to the packet are PTR records
             * for a service, add the SRV, TXT, A and AAAA records too.
             */
            if ( (total_size > 0) && (tmp_ptr) )
            {
                /* Go back through the database looking for the PTR record(s) that
                 * needs to have its related records added to the packet.
                 */
                for (l_host = tmp_ptr; (l_host) && (total_size > 0);
                     l_host = l_host->dns_next)
                {
                    /* If this is a PTR record for a service. */
                    if (l_host->dns_flags & DNS_DELAY_RESPONSE)
                    {
                        /* Add any records relating to the service to the packet. */
                        total_size = MDNS_Add_Additional_Answers(l_host, &buffer, &answer_count,
                                                                 &add_answer_count, -1,
                                                                 addr, total_size);

                        /* Clear the flag indicating the response was sent. */
                        l_host->dns_flags ^= DNS_DELAY_RESPONSE;
                    }
                }
            }
        }

        else
        {
            /* Add this record to the answer section. */
            total_size = MDNS_Add_Known_Answers(&buffer, dns_ptr, 0, &answer_count,
                                                NU_NULL, total_size, -1,
                                                addr, DNS_QR);

            /* If this is a PTR record for a service, we will need to add
             * additional answers to the packet.
             */
            if ( (dns_ptr->dns_type == DNS_TYPE_PTR) &&
                 (dns_ptr->dns_family == NU_FAMILY_UNSPEC) )
            {
                total_size = MDNS_Add_Additional_Answers(dns_ptr, &buffer, &answer_count,
                                                         &add_answer_count, -1, addr,
                                                         total_size);
            }

            if (total_size > 0)
            {
                /* Update our ttl update timestamp. */
                dns_ptr->mdns_last_local_resp_time = NU_Retrieve_Clock();
            }
        }

        /* If a buffer needs to be deallocated. */
        if (buffer)
        {
            /* If there is still data left to send and an error did not occur. */
            if ( (total_size > 0) && (total_size != DNS_FIXED_HDR_LEN) )
            {
                /* Set the number of answers. */
                PUT16(buffer, DNS_ANCOUNT_OFFSET, answer_count);
                PUT16(buffer, DNS_ARCOUNT_OFFSET, add_answer_count);

                /* If the addr is null then send a multicast response. */
                if (addr != NU_NULL)
                {
                   /* Send the data back to node that performed Query. */
                   total_size = NU_Send_To(MDNS_Socket, buffer, (UINT16)total_size, 0, addr, 0);
                }
                else
                {
                    /* Send the data out all interfaces. */
                    MDNS_TX_All_Interfaces(buffer, (UINT16)total_size);
                }
            }

            /* Deallocate the buffer allocated by DNS_Build_Header() and
             * MDNS_Add_Known_Answers().
             */
            NU_Deallocate_Memory(buffer);
        }
    }

} /* MDNS_Send_Response */

/****************************************************************************
*
*   FUNCTION
*
*       MDNS_Add_Additional_Answers
*
*   DESCRIPTION
*
*       This function adds additional answers to the answer section of the
*       packet.  Currently, the routine is only used to add the additional
*       records relating to a PTR service record, but can be extended for
*       other purposes.
*
*   INPUTS
*
*       *dns_ptr                A pointer to the record to add additional
*                               answers for.
*       **buffer                A pointer to the buffer being transmitted.
*       *answer_count           The number of answers in the current buffer.
*       dev_index               The interface index out which to transmit
*                               the partial packet or -1 if the packet should
*                               be transmitted out all interfaces.
*       *addr                   If it is NU_NULL then multicast response, otherwise
*                               send response to specific address.
*       total_size              The total size of the current buffer.
*
*   OUTPUTS
*
*       The total size of the data in the buffer or an operating-system specific
*       error upon failure.
*
******************************************************************************/
INT32 MDNS_Add_Additional_Answers(DNS_HOST *dns_ptr, CHAR **buffer,
                                  UINT16 *answer_count, UINT16 *add_answer_count,
                                  INT dev_index, struct addr_struct *addr,
                                  INT32 total_size)
{
    DNS_HOST    *srv_ptr, *temp_ptr;
    INT32       len = total_size;
    INT16       offset;

    /* If this is a PTR record for a service, add the additional records that
     * apply to the service.
     */
    if ( (dns_ptr->dns_type == DNS_TYPE_PTR) &&
         (dns_ptr->dns_family == NU_FAMILY_UNSPEC) )
    {
        /* Find the SRV record from the data portion of the PTR record. */
        srv_ptr = DNS_Find_Matching_Host_By_Name(dns_ptr->dns_record_data,
                                                 NU_NULL, DNS_TYPE_SRV);

        /* The SRV record holds the hostname as the data portion.  If no
         * SRV record exists, we cannot add the A or AAAA records.
         */
        if (srv_ptr)
        {
            /* Add the SRV record to the packet. */
            len = MDNS_Add_Known_Answers(buffer, srv_ptr, 0, answer_count,
                                         add_answer_count, len, dev_index, addr,
                                         DNS_QR);

            if (len > 0)
            {
                /* Update our ttl update timestamp. */
                srv_ptr->mdns_last_local_resp_time = NU_Retrieve_Clock();

                /* If the SRV record is in the same packet as the A record. */
                if (*add_answer_count == 1)
                {
                    /* Set the offset to the data found in the target section of the
                     * SRV record for header compression.
                     */
                    offset = len - strlen(srv_ptr->dns_record_data) - 2;
                }

                else
                {
                    offset = 0;
                }

                /* Find the A record from the data portion of the SRV record. */
                temp_ptr = DNS_Find_Matching_Host_By_Name(srv_ptr->dns_record_data,
                                                          NU_NULL, DNS_TYPE_A);

                /* Add all A records. */
                if (temp_ptr)
                {
                    len = MDNS_Add_Known_Answers(buffer, temp_ptr, offset, answer_count,
                                                 add_answer_count, len, dev_index, addr,
                                                 DNS_QR);

                    if (len > 0)
                    {
                        /* Update our ttl update timestamp. */
                        temp_ptr->mdns_last_local_resp_time = NU_Retrieve_Clock();
                    }
                }

                if (len > 0)
                {
                    /* Find the AAAA record from the data portion of the SRV record. */
                    temp_ptr = DNS_Find_Matching_Host_By_Name(srv_ptr->dns_record_data,
                                                              NU_NULL, DNS_TYPE_AAAA);

                    /* Add all AAAA records. */
                    if (temp_ptr)
                    {
                        /* If the SRV and A record did not fit in the packet, do
                         * not use an offset.
                         */
#if (INCLUDE_IPV4 == NU_TRUE)
                        if (*add_answer_count != 2)
#else
                        if (*add_answer_count != 1)
#endif
                        {
                            offset = 0;
                        }

                        len = MDNS_Add_Known_Answers(buffer, temp_ptr, offset,
                                                     answer_count, add_answer_count, len,
                                                     dev_index, addr, DNS_QR);

                        if (len > 0)
                        {
                            /* Update our ttl update timestamp. */
                            temp_ptr->mdns_last_local_resp_time = NU_Retrieve_Clock();
                        }
                    }
                }
            }
        }

        /* Find the TXT record from the data portion of the PTR record. */
        temp_ptr = DNS_Find_Matching_Host_By_Name(dns_ptr->dns_record_data,
                                                  NU_NULL, DNS_TYPE_TXT);

        if (temp_ptr)
        {
            /* If all the answers fit in a single buffer, use header compression. */
#if ( (INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE) )
            if (*add_answer_count == 3)
#else
            if (*add_answer_count == 2)
#endif
            {
                /* Set the offset to the data found in the name section of the
                 * SRV record for header compression.
                 */
                offset = total_size;
            }

            else
            {
                offset = 0;
            }

            /* Add the TXT record. */
            len = MDNS_Add_Known_Answers(buffer, temp_ptr, offset, answer_count,
                                         add_answer_count, len, dev_index, addr, DNS_QR);

            if (len > 0)
            {
                /* Update our ttl update timestamp. */
                temp_ptr->mdns_last_local_resp_time = NU_Retrieve_Clock();
            }
        }
    }

    return (len);

} /* MDNS_Add_Additional_Answers */

/****************************************************************************
*
*   FUNCTION
*
*       MDNS_Add_Known_Answers
*
*   DESCRIPTION
*
*       This function adds a record to the answer section of an outgoing
*       query and transmits the packet if it reaches the maximum length.
*
*   INPUTS
*
*       **buffer                A pointer to the buffer being transmitted.
*       *dns_ptr                A pointer to the record to add.
*       *answer_count           The number of answers in the current buffer.
*       *add_answer_count       The number of additional answers in the
*                               current buffer.
*       total_size              The total size of the current buffer.
*       dev_index               The interface index out which to transmit
*                               the partial packet or -1 if the packet should
*                               be transmitted out all interfaces.
*       *addr                   If it is NU_NULL then multicast response, otherwise
*                               send response to specific address.
*       hdr_flags               Flags to set in the header if the answer spans
*                               multiple buffers and a new header must be created.
*
*   OUTPUTS
*
*       The total size of the data in the buffer or an operating-system specific
*       error upon failure.
*
******************************************************************************/
INT32 MDNS_Add_Known_Answers(CHAR **buffer, DNS_HOST *dns_ptr, INT16 offset,
                             UINT16 *answer_count, UINT16 *add_answer_count,
                             INT32 total_size, INT dev_index,
                             struct addr_struct *addr, UINT16 hdr_flags)
{
    INT     i, j;

     /* When a Multicast DNS Querier sends a query to which it already
      * knows some answers, it populates the Answer Section of the DNS
      * query message with those answers.
      */
     if ( (dns_ptr->dns_type == DNS_TYPE_A) ||
          (dns_ptr->dns_type == DNS_TYPE_AAAA) )
     {
         for (i = 0, j = 0;
              (i < DNS_MAX_IP_ADDRS) && (!(IP_IS_NULL_ADDR(&dns_ptr->dns_record_data[j])));
              i ++, j += MAX_ADDRESS_SIZE)
         {

             /* Add this record to the known-answer section. */
             total_size = MDNS_Send_Partial_Response(dns_ptr, dev_index,
                                                     dns_ptr->dns_name,
                                                     offset, buffer,
                                                     dns_ptr->dns_type, DNS_CLASS_IN,
                                                     &dns_ptr->dns_record_data[j],
                                                     answer_count, add_answer_count,
                                                     total_size, hdr_flags, addr);

             /* If buffer is NULL upon return, an error occurred in allocating a new
              * buffer for the next transmission.
              */
             if (!(*buffer))
                 break;
         }
     }

     /* This is a PTR, SRV or TXT record. */
     else
     {
         /* Add the record to the known-answer section. */
         total_size = MDNS_Send_Partial_Response(dns_ptr, dev_index,
                                                 dns_ptr->dns_name, offset, buffer,
                                                 dns_ptr->dns_type, DNS_CLASS_IN,
                                                 dns_ptr->dns_record_data,
                                                 answer_count, add_answer_count,
                                                 total_size, hdr_flags, addr);
     }

     return (total_size);

} /* MDNS_Add_Known_Answers */

/****************************************************************************
*
*   FUNCTION
*
*       MDNS_Continuous_Query
*
*   DESCRIPTION
*
*       This function builds and transmits a query out each interface for a
*       local host record that is undergoing a continuous query.
*
*   INPUTS
*
*       *qry_ptr                A pointer to the query record.
*
*   OUTPUTS
*
*       None.
*
******************************************************************************/
VOID MDNS_Continuous_Query(MDNS_QUERY *qry_ptr)
{
    CHAR                *buffer;
    INT32               total_size;
    UINT16              answer_count = 0;
    MDNS_RESPONSE       *dns_ptr;
    INT16               offset;

    /* Build the header and query section of the DNS query packet. */
    total_size = MDNS_Build_Query(&buffer, qry_ptr->mdns_data, qry_ptr->mdns_type,
                                  qry_ptr->mdns_family);

    /* Add any known answers to the query and transmit the packet. */
    if (total_size > 0)
    {
        /* Get a pointer to the first host in the list. */
        dns_ptr = qry_ptr->mdns_host_list.dns_head;

        /* Add all the known answers to the packet, transmitting the packet
         * if it gets too big.
         */
        while (dns_ptr)
        {
            /* If the query name matches the hostname of this record, use header
             * compression.
             */
            if (strcmp(qry_ptr->mdns_data, dns_ptr->mdns_host->dns_name) == 0)
            {
                offset = DNS_FIXED_HDR_LEN;
            }
            else
            {
                offset = 0;
            }

            total_size = MDNS_Add_Known_Answers(&buffer, dns_ptr->mdns_host, offset,
                                                &answer_count, NU_NULL, total_size, -1,
                                                NU_NULL, 0);

            /* If an error occurred. */
            if (total_size <= 0)
                break;

            /* Get the next host record. */
            dns_ptr = dns_ptr->mdns_next;
        }

        /* If a buffer needs to be deallocated. */
        if (buffer)
        {
            /* If there is still data left to send and an error did not occur. */
            if ( (total_size > 0) && (total_size != DNS_FIXED_HDR_LEN) )
            {
                /* Set the number of answers. */
                PUT16(buffer, DNS_ANCOUNT_OFFSET, answer_count);

                /* Send the data out all interfaces. */
                MDNS_TX_All_Interfaces(buffer, (UINT16)total_size);
            }

            /* Deallocate the buffer allocated by MDNS_Build_Query(). */
            NU_Deallocate_Memory(buffer);
        }
    }

    /* If this is not the first transmission.  The first transmission
     * is < 120ms.  The second transmission is 1 second, and each
     * transmission increases thereafter.
     */
    if (qry_ptr->mdns_qry_expire >= MDNS_ONE_SEC_DELAY)
    {
        /* The intervals between successive queries MUST increase by
         * at least a factor of two.
         */
        qry_ptr->mdns_qry_expire <<= 1;

        /* When the interval between queries reaches or exceeds 60
         * minutes, a querier MAY cap the interval to a maximum of
         * 60 minutes, and perform subsequent queries at a steady-state
         * rate of one query per hour.
         */
        if (qry_ptr->mdns_qry_expire >= MDNS_QUERY_MAX)
        {
            qry_ptr->mdns_qry_expire = MDNS_QUERY_MAX;
        }
    }

    else
    {
        /* The interval between the first two queries MUST be at least
         * one second.
         */
        qry_ptr->mdns_qry_expire = MDNS_ONE_SEC_DELAY;
    }

    /* Reset the timer. */
    NU_Reset_Timer(&qry_ptr->mdns_qry_timer, MDNS_Query_Handler,
                   qry_ptr->mdns_qry_expire, 0, NU_ENABLE_TIMER);

} /* MDNS_Continuous_Query */

/****************************************************************************
*
*   FUNCTION
*
*       MDNS_TX_All_Interfaces
*
*   DESCRIPTION
*
*       This function transmits the mDNS packet out all interfaces over both
*       IPv4 and IPv6 on the node.
*
*   INPUTS
*
*       *buffer                 A pointer to the buffer to transmit.
*       size                    The length of the outgoing buffer.
*
*   OUTPUTS
*
*       NU_SUCCESS upon successful completion or NU_NO_ACTION if no data was
*       sent out any interface.
*
******************************************************************************/
STATUS MDNS_TX_All_Interfaces(CHAR *buffer, UINT16 size)
{
    struct if_nameindex *dev_list, *dev_ptr;
    STATUS              status = NU_NO_ACTION;

    /* Get a list of all interfaces on the node. */
    dev_list = NU_IF_NameIndex();

    if (dev_list)
    {
        dev_ptr = dev_list;

        /* Send the message out each interface. */
        while (dev_ptr->if_name != NU_NULL)
        {
#if (INCLUDE_LOOPBACK_DEVICE == NU_TRUE)
            /* Don't send data out the loopback interface. */
            if (dev_ptr->if_index != 0)
#endif
            {
                /* If data can be sent from any interface, this routine should
                 * return success.
                 */
                if (MDNS_TX_Packet(buffer, size, dev_ptr->if_index) == NU_SUCCESS)
                {
                    status = NU_SUCCESS;
                }
            }

            /* Move the pointer ahead in memory */
            dev_ptr = (struct if_nameindex*)((CHAR*)dev_ptr +
                      (sizeof(struct if_nameindex) + DEV_NAME_LENGTH));
        }

        NU_IF_FreeNameIndex(dev_list);
    }

    return (status);

} /* MDNS_TX_All_Interfaces */

/****************************************************************************
*
*   FUNCTION
*
*       MDNS_TX_Packet
*
*   DESCRIPTION
*
*       This function transmits the mDNS packet over the mDNS socket using
*       both IPv4 and IPv6 if enabled.
*
*   INPUTS
*
*       *buffer                 A pointer to the buffer to transmit.
*       size                    The length of the outgoing buffer.
*       dev_index               The interface index out which to send
*                               the packet.
*
*   OUTPUTS
*
*       NU_SUCCESS upon successful completion or an operating-system specific
*       error if no data was sent.
*
******************************************************************************/
STATUS MDNS_TX_Packet(CHAR *buffer, UINT16 size, INT dev_index)
{
#if (INCLUDE_IPV4 == NU_TRUE)
    SCK_IOCTL_OPTION    multi_ioctl_opt;
    CHAR                dev_name[DEV_NAME_LENGTH];
#endif
    UINT16              bytes_sent;
    STATUS              status = NU_NO_ACTION, mcast_status;
    struct addr_struct  mdns_addr;
    DNS_SERVER          *cur_server;

    /* Set the destination port. */
    mdns_addr.port = MDNS_PORT;

    /* Get a pointer to the first server in the list. */
    cur_server = MDNS_Servers.dnss_head;

    while (cur_server)
    {
        /* Set the family and address of the destination. */
        mdns_addr.family = cur_server->family;
        memcpy(mdns_addr.id.is_ip_addrs, cur_server->dnss_ip, MAX_ADDRESS_SIZE);

        /* Set the multicast interface for IPv4 and/or IPv6 to use this
         * interface.
         */
#if ( (INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE) )
        if (mdns_addr.family == NU_FAMILY_IP)
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
        {
            /* Convert the index to the name. */
            if (NU_IF_IndexToName(dev_index, dev_name))
            {
                /* Set up the ioctl structure to pass in the device name. */
                multi_ioctl_opt.s_optval = (UINT8*)dev_name;

                /* Get the IP address associated with the name. */
                mcast_status = NU_Ioctl_SIOCGIFADDR(&multi_ioctl_opt,
                                                    sizeof(SCK_IOCTL_OPTION));

                if (mcast_status == NU_SUCCESS)
                {
                    /* Set the IPv4 multicast interface. */
                    mcast_status = NU_Setsockopt_IP_MULTICAST_IF(MDNS_Socket,
                                                                 multi_ioctl_opt.s_ret.
                                                                 s_ipaddr);
                }
            }

            else
            {
                mcast_status = NU_NO_ACTION;
            }
        }

#if (INCLUDE_IPV6 == NU_TRUE)
        else
#endif
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
        {
            /* Set the IPv6 multicast interface. */
            mcast_status = NU_Setsockopt_IPV6_MULTICAST_IF(MDNS_Socket, dev_index);
        }
#endif

        if (mcast_status == NU_SUCCESS)
        {
            /* Send the DNS query to the multicast address. */
            bytes_sent = NU_Send_To(MDNS_Socket, buffer, size, 0, &mdns_addr, 0);

            if (bytes_sent < 0)
            {
                status = bytes_sent;
                break;
            }

            else
            {
                status = NU_SUCCESS;
            }
        }

        /* Get the next server in the list. */
        cur_server = cur_server->dnss_next;
    }

    return (status);

} /* MDNS_TX_Packet */

/****************************************************************************
*
*   FUNCTION
*
*       MDNS_Handle_Conflict
*
*   DESCRIPTION
*
*       This routine handles the situation when probing determines that the
*       generated hostname is not unique on the link.  A new hostname is
*       created, and probing is restarted on the link.
*
*   INPUTS
*
*       *dns_ptr                A pointer to the record for which the conflict
*                               was found.
*
*   OUTPUTS
*
*       None.
*
******************************************************************************/
VOID MDNS_Handle_Conflict(DNS_HOST *dns_ptr)
{
    CHAR    *new_name;

    /* Create a new hostname for this record. */
    new_name = MDNS_Create_Hostname_Callback(dns_ptr->mdns_dev_index, NU_TRUE);

    if (new_name)
    {
        /* Assign the new hostname to the interface and kick off probing
         * and announcing.
         */
        MDNS_Set_Device_Hostname(new_name, dns_ptr->mdns_dev_index);
    }

} /* MDNS_Handle_Conflict */

/****************************************************************************
*
*   FUNCTION
*
*       MDNS_Process_Incoming_Packet
*
*   DESCRIPTION
*
*       This routine processes an incoming packet destined to port 5353.
*
*   INPUTS
*
*       *recv_buffer            A pointer to the incoming packet to process.
*       *addr                   Address of who sent the packet.
*
*   OUTPUTS
*
*       None.
*
******************************************************************************/
VOID MDNS_Process_Incoming_Packet(CHAR *recv_buffer, struct addr_struct *addr)
{
    CHAR                *name;
    DNS_HOST            *dns_ptr, *dns_ptr_ptr = NU_NULL;
    MDNS_QUERY          *qry_ptr;
    UINT16              count, data_len, rec_count;
    INT                 offset, ip_idx, i, j, n;
    UNSIGNED            time_left = 0;
    STATUS              status;

    /* Allocate memory for parsing out the questions/responses. */
    status = NU_Allocate_Memory(MEM_Cached, (VOID **)&name, DNS_MAX_NAME_SIZE,
                                NU_NO_SUSPEND);

    if (status != NU_SUCCESS)
    {
        return;
    }

    /* If this is a Response packet. */
    if (GET16(recv_buffer, DNS_FLAGS_OFFSET) & DNS_QR)
    {
        /* Move past the fixed DNS header. */
        offset = DNS_FIXED_HDR_LEN;

        /* Process answers and additional answers. */
        for (n = 0; n < 2; n ++)
        {
            /* Process regular answers first. */
            if (n == 0)
            {
                /* Determine the number of answers in the packet.  Note that Multicast DNS
                 * Responses MUST NOT contain any questions in the Question Section.
                 */
                count = GET16(recv_buffer, DNS_ANCOUNT_OFFSET);
            }

            /* Check additional answers next. */
            else
            {
                count = GET16(recv_buffer, DNS_ARCOUNT_OFFSET);
            }

            /* If there are answers in the packet. */
            if (count > 0)
            {
                /* Process each of the records in the packet. */
                for (i = 0, rec_count = 0; i < count; i ++)
                {
                    /* Move past the name to the fixed portion of the packet. */
                    offset += DNS_Unpack_Domain_Name(name, DNS_MAX_NAME_SIZE,
                                                     &recv_buffer[offset], recv_buffer);

                    /* Find a matching record in the local database. */
                    dns_ptr = MDNS_Find_Matching_Record(&recv_buffer[offset], recv_buffer, name);

                    /* If a record was found, and it is already associated with a query,
                     * get a pointer to the query.
                     */
                    if ( (dns_ptr) && (dns_ptr->mdns_query) )
                    {
                        qry_ptr = dns_ptr->mdns_query;
                    }

                    /* Otherwise, look up the query associated with the record. */
                    else
                    {
                        qry_ptr = MDNS_Find_Matching_Query_By_Name(name, GET16(recv_buffer,
                                                                               offset + DNS_TYPE_OFFSET));

                        /* If there is no Query and no matching record, and if this packet
                         * contained a PTR record specific to DNS-SD, use the Query associated
                         * with that record.
                         */
                        if ( (!qry_ptr) && (dns_ptr_ptr) )
                        {
                            qry_ptr = dns_ptr_ptr->mdns_query;
                        }
                    }

                    /* If this is the response to a continuous query, or a response to a record we are NOT
                     * authoritative on then process all records in this packet.
                     */
                    if ( (qry_ptr) ||
                         ((dns_ptr) &&
                          ((dns_ptr->mdns_state == MDNS_UNINITIALIZED) ||
                           (dns_ptr->mdns_state == MDNS_QUERYING))) )
                    {
                        /* Process the response to the continuous query. */
                        rec_count += MDNS_Process_Incoming_Response(&dns_ptr, qry_ptr,
                                                                    (DNS_PKT_HEADER *)recv_buffer,
                                                                    &recv_buffer[offset], name, count);

                        /* If we have not already found a PTR record for a DNS-SD service. */
                        if ( (!dns_ptr_ptr) && (qry_ptr) && (dns_ptr) )
                        {
                            /* If this is a PTR record for a DNS-SD service. */
                            if ( (dns_ptr->dns_family == NU_FAMILY_UNSPEC) &&
                                 (dns_ptr->dns_type == DNS_TYPE_PTR) )
                            {
                                /* Save a pointer to this record to be used if there is a SRV
                                 * or TXT record present in the packet.
                                 */
                                dns_ptr_ptr = dns_ptr;
                            }
                        }

                        /* If all the records answered an outstanding continuous query, exit
                         * the loop.  Otherwise, the responder has placed more records in the
                         * packet than pertain to a single query - process the next record.
                         */
                        if (count == rec_count)
                        {
                            /* Move past all these answers to the next set of
                             * answers.
                             */
                            while (i != (count - 1))
                            {
                                /* Move past the name to the fixed portion of the next record. */
                                offset += DNS_Unpack_Domain_Name(NU_NULL, DNS_MAX_NAME_SIZE,
                                                                 &recv_buffer[offset], recv_buffer);

                                i ++;
                            }

                            /* Move past the last record in this section. */
                            offset += (GET16(recv_buffer, offset + DNS_RDLENGTH_OFFSET) + DNS_RDATA_OFFSET);

                            break;
                        }
                    }

                    /* If this is our record, and it is not already in the conflicted
                     * state.
                     */
                    else if ( (dns_ptr) && (dns_ptr->mdns_state != MDNS_CONFLICTED) )
                    {
                        /* Extract the data length from the packet. */
                        data_len = GET16(&recv_buffer[offset], DNS_RDLENGTH_OFFSET);

                        /* Check each of our addresses. */
                        for (j = 0, ip_idx = 0; (j < DNS_MAX_IP_ADDRS) &&
                             (!(IP_IS_NULL_ADDR(&dns_ptr->dns_record_data[ip_idx])));
                             ip_idx += MAX_ADDRESS_SIZE, j ++)
                        {
                            /* If the data does not match our data. */
                            if (memcmp(&dns_ptr->dns_record_data[ip_idx],
                                       &recv_buffer[offset + DNS_RDATA_OFFSET], data_len) != 0)
                            {
                                /* If we are in the probing state, this is a conflict. */
                                if (dns_ptr->mdns_state == MDNS_PROBING)
                                {
                                    dns_ptr->mdns_state = MDNS_CONFLICTED;
                                    break;
                                }

                                /* Otherwise, restart probing so the other node knows
                                 * this address is in use.
                                 */
                                else
                                {
                                    MDNS_Invoke_Probing(dns_ptr, (UTL_Rand() % MDNS_PROBE_DELAY) + 1);
                                    break;
                                }
                            }
                        }
                    }

                    /* Move past this record. */
                    offset += (GET16(recv_buffer, offset + DNS_RDLENGTH_OFFSET) + DNS_RDATA_OFFSET);
                }
            }
        }
    }

    /* If this is a Query packet. */
    else
    {
        /* Get the number of queries in the packet. */
        count = GET16(recv_buffer, DNS_QDCOUNT_OFFSET);

        /* If there are queries in the packet. */
        if (count > 0)
        {
            /* Move past the DNS header. */
            offset = DNS_FIXED_HDR_LEN;

            /* Extract the name from the first query. */
            offset += DNS_Unpack_Domain_Name(name, DNS_MAX_NAME_SIZE, &recv_buffer[offset],
                                             recv_buffer);

            /* If the type of the first query is ANY, and there are Authority Resource
             * Records present, this is a probe query.
             */
            if ( (GET16(&recv_buffer[offset], DNS_TYPE_OFFSET) == DNS_TYPE_ANY) &&
                 (GET16(recv_buffer, DNS_NSCOUNT_OFFSET) > 0) )
            {
                /* This routine returns the number of queries left to be processed. */
                count = MDNS_Process_Incoming_Probe_Query(recv_buffer, name,
                                                          GET16(&recv_buffer[offset], DNS_CLASS_OFFSET) ?
                                                          addr : NU_NULL);
            }

            /* If this is not a query sent by a foreign node in the probing state
             * or it is a query sent during probing, but we are not in the probing
             * state, process the queries.
             */
            if (count != 0)
            {
                i = 0;

                /* Process each query in the packet. */
                do
                {
                    /* Find a matching record in the local database. */
                    dns_ptr = MDNS_Find_Matching_Record(&recv_buffer[offset], recv_buffer, name);

                    /* If the record wasn't found see if a negative response needs to be sent. */
                    if(dns_ptr == NU_NULL)
                    {
                        dns_ptr = MDNS_Find_Any_Record(&recv_buffer[offset], recv_buffer, name);

                        /* If there is a negative response record to send, then send it. */
                        if(dns_ptr != NU_NULL)
                        {
                            MDNS_Send_Response(dns_ptr, NU_NULL);

                            /* Clear the negative response flag. */
                            dns_ptr->dns_flags ^= DNS_NSEC_RECORD;
                        }
                    }
                    else
                    {
                        /* If this record is local to the node.  Do not answer queries
                         * for records that are not local to this node.
                         */
                        if (dns_ptr->dns_flags & DNS_LOCAL_RECORD)
                        {
                            /* If the unicast bit is set. */
                            if( (GET16(&recv_buffer[offset], DNS_CLASS_OFFSET) & DNS_UNI_FLAG) == DNS_UNI_FLAG)
                            {
                                MDNS_Process_Unicast_Query(dns_ptr, addr);
                            }
                            else
                            {
                                /* Flag this record to be multicast the next time the multicast timer fires. */
                                dns_ptr->dns_flags |= DNS_DELAY_RESPONSE;

                                /* If the timer isn't already set then set it.
                                   NU_Get_Remaining_Time will return an error if our timer is currently disabled.
                                 */
                                if((NU_Get_Remaining_Time(&MDNS_Response_Timer, &time_left) != NU_SUCCESS) || (time_left <= 0))
                                {
                                    /* Start response timer. */
                                    NU_Reset_Timer(&MDNS_Response_Timer, MDNS_Response_Handler, MDNS_RESPONSE_DELAY, 0,
                                                   NU_ENABLE_TIMER);
                                }
                            }
                        }

                        /* This is a foreign record so check for Passive Observation of Failure(POOF) condition.
                         * RFC 10.5
                         *     After seeing two or more of these queries, and seeing no multicast
                         *     response containing the expected answer within ten seconds, then even
                         *     though its TTL may indicate that it is not yet due to expire, that
                         *     record SHOULD be flushed from the cache. The host SHOULD NOT perform
                         *     its own queries to re-confirm that the record is truly gone.
                         */
                        else
                        {
                            dns_ptr->mdns_foreign_query_cnt += 1;

                            /* If this foreign query has been seen twice, invoke POOF to delete
                             * stale entries.
                             */
                            if (dns_ptr->mdns_foreign_query_cnt == 2)
                            {
                                /* Set the TTL to 10 seconds. */
                                dns_ptr->dns_ttl = (10 * SCK_Ticks_Per_Second);

                                /* If the record is being actively queried, adjust the Querier
                                 * Cache Maintenance timer to 10 seconds.  Otherwise, just set
                                 * the TTL to 10 seconds and let the entry get overwritten if
                                 * it times out.  There is no need to start a timer for an
                                 * entry that is not being actively queried by an application.
                                 */
                                if (dns_ptr->mdns_query)
                                {
                                    /* There should always be a timer allocated if the entry
                                     * is associated with a query pointer, but check the pointer
                                     * for safety.
                                     */
                                    if (dns_ptr->mdns_timer)
                                    {
                                        /* Stop the timer. */
                                        NU_Control_Timer(dns_ptr->mdns_timer, NU_DISABLE_TIMER);

                                        /* Set the state to querying so Querier Cache Maintenance will
                                         * take over timing out this record.
                                         */
                                        dns_ptr->mdns_state = MDNS_QUERYING;

                                        /* Set the retrans count so the entry will be deleted upon
                                         * expiration of the Querier Cache Maintenance timer.
                                         */
                                        dns_ptr->mdns_retrans_count = 5;

                                        /* Set the timer to expire in 10 seconds. */
                                        NU_Reset_Timer(dns_ptr->mdns_timer, MDNS_Event_Handler,
                                                       dns_ptr->dns_ttl, 0, NU_ENABLE_TIMER);
                                    }
                                }
                            }
                        }
                    }

                    /* Get the next query.  Move past the rest of the query data. */
                    offset += 4;

                    /* Increment the count of the number of records we have processed. */
                    i ++;

                    /* If there is another query in the packet. */
                    if (i < count)
                    {
                        /* Extract the name from the next query. */
                        offset += DNS_Unpack_Domain_Name(name, DNS_MAX_NAME_SIZE,
                                                         &recv_buffer[offset],
                                                         recv_buffer);
                    }

                } while (i < count);
            }
        }
    }

    /* Deallocate the memory that was used to parse the question/response. */
    NU_Deallocate_Memory(name);

} /* MDNS_Process_Incoming_Packet */

/****************************************************************************
*
*   FUNCTION
*
*       MDNS_Find_Any_Record
*
*   DESCRIPTION
*
*       Find an A or AAAA record where the other type is known to not exist in
*       the system.
*
*   INPUTS
*
*       *recv_buffer            A pointer to the beginning of the fixed portion
*                               of the response record.
*       *pkt                    A pointer to the head of the packet.
*       *name                   The name corresponding to the response.
*
*   OUTPUTS
*
*       A pointer to the local DNS record or NU_NULL if no record exists.
*
******************************************************************************/
DNS_HOST *MDNS_Find_Any_Record(CHAR *recv_buffer, CHAR *pkt, CHAR *name)
{
    DNS_HOST *dns_ptr = NU_NULL; /* Assume both IPV4 and IPV6 are NOT enabled. */
    UINT16    type;

    /* Get the record type of the request. */
    type = GET16(recv_buffer, DNS_TYPE_OFFSET);

    /* Negative responses are only sent for A or AAAA records when IPv4 or
     * IPv6 is disabled, respectively.  If the query is for an A record,
     * check if a AAAA record exists.
     */
    if (type == DNS_TYPE_A)
    {
        dns_ptr = DNS_Find_Matching_Host_By_Name(name, NU_NULL, DNS_TYPE_AAAA);
    }

    /* If the query is for an AAAA record, check if an A record exists. */
    else if (type == DNS_TYPE_AAAA)
    {
        dns_ptr = DNS_Find_Matching_Host_By_Name(name, NU_NULL, DNS_TYPE_A);
    }

    /* If a record was found, flag that record for sending an NSEC. */
    if (dns_ptr != NU_NULL)
    {
        dns_ptr->dns_flags |= DNS_NSEC_RECORD;
    }

    return (dns_ptr);

} /* MDNS_Find_Any_Record */

/****************************************************************************
*
*   FUNCTION
*
*       MDNS_Process_Unicast_Query
*
*   DESCRIPTION
*
*       This routine processes an sends a response to a unicast query.
*
*   INPUTS
*
*       *dns_ptr                A pointer to the record for which the conflict
*                               was found.
*       *addr                   The address of who sent the unicast query.
*
*   OUTPUTS
*
*       None
*
******************************************************************************/
VOID MDNS_Process_Unicast_Query(DNS_HOST *dns_ptr, struct addr_struct *addr)
{
    /* Assume we are sending a multicast response. */
    struct addr_struct  *mdns_addr = NU_NULL;
    UNSIGNED            delta_time_wrap = 0;
    UNSIGNED            quarter_ticks;


    /* The reason for sending a multicast response to a the unicast query is
       because of the following which is stated in the RFC(Section 5.4):
           When receiving a question with the "unicast response" bit set, a
           Responder SHOULD usually respond with a unicast packet directed back
           to the querier. However, if the Responder has not multicast that
           record recently (within one quarter of its TTL), then the Responder
           SHOULD instead multicast the response so as to keep all the peer
           caches up to date, and to permit passive conflict detection.
    */

    /* If it is a dynamic record then we need to remove the clock(mdns_ttl_start) from the ttl,
       in order to get the ttl back into units of ticks.
       The ttl's current unit is clock time(timestamp).
     */
   if(!dns_ptr->dns_flags & DNS_PERMANENT_ENTRY)
   {
       /* If the ttl is less then the start ttl then that means the timer has wrapped.
          NOTE: In this case ttl's units are time(expiration timestamp) and mdns_ttl_start_time is
                in units of time(start timestamp). */
       if(dns_ptr->dns_ttl < dns_ptr->mdns_ttl_start_time)
       {
           delta_time_wrap = (0xFFFFFFFFUL - dns_ptr->mdns_ttl_start_time);

           /* If subtracting the delta time wrap will cause the clock to loop backwards,
            * then first subtract the delta_time_wrap from the max clock time and then add
            * in the ttl value to remove the start time.
            */
           if(delta_time_wrap > dns_ptr->dns_ttl)
           {
               quarter_ticks = (0xFFFFFFFFUL - delta_time_wrap) + dns_ptr->dns_ttl;
           }
           /* Remove that start time. */
           else
           {
               quarter_ticks = dns_ptr->dns_ttl - delta_time_wrap;
           }
       }
       else
       {
           quarter_ticks = (dns_ptr->dns_ttl - dns_ptr->mdns_ttl_start_time);
       }
   }
   else
   {
       /* This is a permanent entry so the ttl's unit is ticks. */
       quarter_ticks = (dns_ptr->dns_ttl);
   }

   /* Figure out what time is within 25% of the ttl. */
   quarter_ticks = quarter_ticks >> 2;

   /* Check to see when the last multicast was for this record.
      If the current clock time is within 25% of the ttl
      then this response should be multicast instead of unicast.
   */
   if(TQ_Check_Duetime(dns_ptr->mdns_last_local_resp_time + quarter_ticks) == NU_NO_SUSPEND)
   {
       mdns_addr = addr;
   }

   MDNS_Send_Response(dns_ptr, mdns_addr);

} /* MDNS_Process_Unicast_Query */

/****************************************************************************
*
*   FUNCTION
*
*       MDNS_Process_Incoming_Probe_Query
*
*   DESCRIPTION
*
*       This routine processes an incoming query sent by a foreign node
*       during probing.  If the record matches one of our local records
*       and we are also probing, tie-breaking will be performed.  Otherwise,
*       the record is not processed by this routine at this time.  Code
*       can be added here to handle that case, or if it makes sense, this
*       code can be added to the generic query processing logic yet to
*       be written.
*
*   INPUTS
*
*       *recv_buffer            A pointer to the head of the incoming
*                               DNS packet.
*       *name                   The hostname being probed by the foreign
*                               node.
*       *addr                   The address associated with the sender of
*                               the query.
*
*   OUTPUTS
*
*       The number of queries remaining to be processed in this packet.
*
******************************************************************************/
UINT16 MDNS_Process_Incoming_Probe_Query(CHAR *recv_buffer, CHAR *name,
                                         struct addr_struct *addr)
{
    INT                 i, offset, compare, ip_idx;
    UINT16              count;
    DNS_HOST            *dns_ptr;
#if (INCLUDE_IPV6 == NU_TRUE)
    DNS_HOST            *dnsv6_ptr;
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
    DNS_HOST            *dnsv4_ptr;
#endif
    MDNS_SORT_INDEX     *sorted_ptr, local_record;

#if (INCLUDE_IPV6 == NU_TRUE)
    /* Determine if there is a matching IPv6 record. */
    dns_ptr = dnsv6_ptr = DNS_Find_Matching_Host_By_Name(name, NU_NULL, DNS_TYPE_AAAA);
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    /* Determine if there is a matching IPv4 record.   The records are sorted based
     * on class, type, data.  The class will always be the same for IPv4 and IPv6
     * records, but the type of the IPv4 record is lower.  If there is an IPv4
     * record, use it as the first comparison.
     */
    dns_ptr = dnsv4_ptr = DNS_Find_Matching_Host_By_Name(name, NU_NULL, DNS_TYPE_A);
#endif

    /* If either record is in the probing state, perform tie breaking. */
    if (
#if (INCLUDE_IPV6 == NU_TRUE)
         ((dnsv6_ptr) && (dnsv6_ptr->mdns_state == MDNS_PROBING))
#if (INCLUDE_IPV4 == NU_TRUE)
         ||
#endif
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
         ((dnsv4_ptr) && (dnsv4_ptr->mdns_state == MDNS_PROBING))
#endif
        )
    {
        /* Get the number of authority name servers in the packet. */
        count = GET16(recv_buffer, DNS_NSCOUNT_OFFSET);

        /* Move past the header. */
        offset = DNS_FIXED_HDR_LEN;

        /* Move past the questions to the Authority RRs. */
        for (i = 0; i < count; i ++)
        {
            /* Move past the name. */
            offset += DNS_Unpack_Domain_Name(NU_NULL, DNS_MAX_NAME_SIZE,
                                             &recv_buffer[offset], recv_buffer);

            /* Move past the type and class. */
            offset += 4;
        }

        /* Sort the incoming data according to the RFC. */
        sorted_ptr = MDNS_Sort_Records(&recv_buffer[offset], count, recv_buffer);

        if (sorted_ptr)
        {
            /* Compare each of the records to find the winner. */
            for (i = 0, ip_idx = 0; (i < count) && (dns_ptr); i ++)
            {
                /* Set up the local record for the comparison. */
                local_record.r_class = DNS_CLASS_IN;
                local_record.type = dns_ptr->dns_type;
                local_record.data_ptr = &dns_ptr->dns_record_data[ip_idx];
                local_record.data_len = dns_ptr->dns_h_length;

                /* Determine whether these records differ. */
                compare = MDNS_Compare_Records(&sorted_ptr[i], &local_record);

                /* If their record is lexicographically less than mine, I
                 * win the tie breaker.  Ignore this packet.
                 */
                if (compare == -1)
                {
                    break;
                }

                /* Otherwise, they win the tie-breaker. */
                else if (compare == 1)
                {
                    dns_ptr->mdns_state = MDNS_TIE_BREAKER;
                    break;
                }

                /* Increment the IP address index. */
                ip_idx += MAX_ADDRESS_SIZE;

                /* If there is not another IP address in this record, move to
                 * the next local record.
                 */
                if (IP_IS_NULL_ADDR(&dns_ptr->dns_record_data[ip_idx]))
                {
                    ip_idx = 0;

#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )

                    /* If we have not already evaluated the IPv6 records. */
                    if (dns_ptr != dnsv6_ptr)
                    {
                        dns_ptr = dnsv6_ptr;
                    }

                    else
                    {
                        /* This is the last record. */
                        dns_ptr = NU_NULL;
                    }
#else
                    dns_ptr = NU_NULL;
#endif
                }
            }

            /* The other side won the tie-breaker if our record is in the
             * tie breaker state or we ran out of records and they still
             * have records remaining.  Delay 1 second and restart probing.
             */
            if ( ((dns_ptr) && (dns_ptr->mdns_state == MDNS_TIE_BREAKER)) ||
                 ((!dns_ptr) && (i < count)) )
            {
#if (INCLUDE_IPV6 == NU_TRUE)
                if (dnsv6_ptr)
                {
                    /* Reset the timer for probing. */
                    MDNS_Invoke_Probing(dnsv6_ptr, MDNS_ONE_SEC_DELAY);
                }
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
                if (dnsv4_ptr)
                {
                    /* Reset the timer for probing. */
                    MDNS_Invoke_Probing(dnsv4_ptr, MDNS_ONE_SEC_DELAY);
                }
#endif
            }

            /* Deallocate the memory allocated to sort the list of
             * incoming records.
             */
            NU_Deallocate_Memory(sorted_ptr);
        }

        count = 0;
    }

    /* The local record is not in the probing state, but owns this record.  We
     * should answer with a response so the other side knows their record is in
     * conflict.
     */
    else if (dns_ptr)
    {
        /* Send the IPv4, IPv6 and PTR records. */
        (VOID)MDNS_Send_Announcement(MDNS_Socket, addr, dnsv4_ptr, dnsv6_ptr,
                                     dns_ptr->dns_name);

        /* Probe queries are not allowed to have queries for any other
         * records.
         */
        count = 0;
    }

    else
    {
        count = GET16(recv_buffer, DNS_QDCOUNT_OFFSET);
    }

    return (count);

} /* MDNS_Process_Incoming_Probe_Query */

/****************************************************************************
*
*   FUNCTION
*
*       MDNS_Process_Incoming_Response
*
*   DESCRIPTION
*
*       This routine processes an incoming response record intended for a
*       continuous query.
*
*   INPUTS
*
*       **dns_ptr_ptr           The record associated with the response.
*       *qry_ptr                The query associated with the response.
*       *pkt                    A pointer to the DNS header.
*       *recv_buffer            A pointer to the fixed portion of the
*                               incoming record.
*       *name                   The name associated with the response.
*       count                   The number of records in the packet.
*
*   OUTPUTS
*
*       The number of records processed from the packet.
*
******************************************************************************/
UINT16 MDNS_Process_Incoming_Response(DNS_HOST **dns_ptr_ptr, MDNS_QUERY *qry_ptr,
                                      DNS_PKT_HEADER *pkt, CHAR *recv_buffer,
                                      CHAR *name, INT count)
{
    MDNS_NTFY_STRUCT    *mdns_ptr = NU_NULL;
    CHAR                *data = NU_NULL;
    STATUS              status = NU_SUCCESS;
    UINT16              type, rec_count = 0;
    INT                 i, ip_idx, j, family;
    UINT16              data_len;
    UNSIGNED            ttl;
    MDNS_RESPONSE       *res_ptr;
    DNS_HOST            *dns_ptr = *dns_ptr_ptr, *new_dns_host;
    DNS_SRV_DATA        srv_data;

    /* Extract the TTL from the packet. */
    ttl = GET32(recv_buffer, DNS_TTL_OFFSET);

    /* If the ttl is zero at this point then this is a GoodBye Packet(mDNS RFC 10.1).
     * The calling function MDNS_Process_Incoming_Packet ensured that this
     * record matched one of our existing records in our database and that
     * this record is NOT authoritative, which are the other two conditions
     * that must be satisfied for a GoodBye Packet.
     */
    if (ttl == 0)
    {
        /* The ttl is being set to one per mDNS RFC 10.1:
         * Queriers receiving a Multicast DNS Response with a TTL of zero SHOULD
         * NOT immediately delete the record from the cache, but instead record
         * a TTL of 1 and then delete the record one second later.
         */
        ttl = 1;
    }

    /* Always update the TTL when a response is received. */
    if (dns_ptr)
    {
        /* Store the time at which we received this, because when
           processing unicast responses we need to be able to
           remove the start timer at which a record was received.
         */
        dns_ptr->mdns_ttl_start_time = NU_Retrieve_Clock();

        /* Convert the ttl to ticks. */
        dns_ptr->dns_ttl = (ttl * SCK_Ticks_Per_Second);

        /* Calculate the ttl expiration time. */
        dns_ptr->dns_ttl += dns_ptr->mdns_ttl_start_time;
    }

    /* If this response is for a continuous query. */
    if (qry_ptr)
    {
        /* Extract the type from the packet. */
        type = GET16(recv_buffer, DNS_TYPE_OFFSET);

        switch (type)
        {
#if (INCLUDE_IPV4 == NU_TRUE)
        case DNS_TYPE_A :

            family = NU_FAMILY_IP;
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
        case DNS_TYPE_AAAA :

            if (type == DNS_TYPE_AAAA)
            {
                family = NU_FAMILY_IP6;
            }
#endif
            /* Allocate memory for the IP addresses. */
            status = NU_Allocate_Memory(MEM_Cached, (VOID **)&data,
                                        count * MAX_ADDRESS_SIZE,
                                        NU_NO_SUSPEND);

            if (status == NU_SUCCESS)
            {
                /* Zero out the memory. */
                memset(data, 0, count * MAX_ADDRESS_SIZE);

                /* Let the DNS module extract the IP addresses into a local
                 * buffer; otherwise, we will have to process the addresses
                 * in the packet twice; once to determine whether the data
                 * has been updated since the last query and once to extract
                 * the data and update the record.
                 */
                status = DNS_Extract_Data((DNS_PKT_HEADER *)pkt, data,
                                          name, &ttl, type, family, (INT16*)&rec_count);

                /* If the records were successfully extracted. */
                if (status == NU_SUCCESS)
                {
                    /* If a record already exists, check if it has changed. */
                    if (dns_ptr)
                    {
                        /* Determine whether the record entry has changed from
                         * the last time it was queried.
                         */
                        for (i = 0, ip_idx = 0; i < rec_count; ip_idx += MAX_ADDRESS_SIZE, i ++)
                        {
                            /* If we have reached the end of IP addresses in the
                             * old record, there are new IP addresses to be stored.
                             */
                            if (IP_IS_NULL_ADDR(&dns_ptr->dns_record_data[ip_idx]))
                            {
                                break;
                            }

                            /* Check if this IP address is present in the old
                             * record.
                             */
                            for (j = 0; j < rec_count; j ++)
                            {
                                /* If this IP is present in the old record, exit. */
                                if (memcmp(&data[j * MAX_ADDRESS_SIZE],
                                           (CHAR*)&dns_ptr->dns_record_data[ip_idx],
                                           dns_ptr->dns_h_length) == 0)
                                {
                                    break;
                                }
                            }

                            /* If this IP address isn't present in the old record,
                             * then the entry has been updated - exit the loop.
                             */
                            if (j == rec_count)
                                break;
                        }

                        /* The entry has changed if there are more or less IP
                         * addresses or an IP address is present in the new entry
                         * that was not present in the old entry.
                         */
                        if ( (i != rec_count) ||
                             (!(IP_IS_NULL_ADDR(&dns_ptr->dns_record_data[ip_idx]))) )
                        {
                            /* Zero out the old record since there may be less IP addresses
                             * in the new record.
                             */
                            memset((CHAR*)dns_ptr->dns_record_data, 0,
                                   DNS_MAX_IP_ADDRS * MAX_ADDRESS_SIZE);

                            /* Update the record. */
                            memcpy((CHAR*)dns_ptr->dns_record_data, data,
                                    rec_count * MAX_ADDRESS_SIZE);

                            /* Notify the application of the changes. */
                            mdns_ptr = qry_ptr->mdns_callback.head;
                        }
                    }

                    else
                    {
                        /* Create a new record for this response. */
                        dns_ptr = DNS_Add_Host(name, data, ttl, family, type,
                                               rec_count, 0, NU_NULL);
                    }
                }
            }

            break;

        case DNS_TYPE_SRV :

            /* Extract the data length from the packet. */
            data_len = GET16(recv_buffer, DNS_RDLENGTH_OFFSET) + 1;

            /* Ensure the name will fit in the record. */
            if (data_len <= DNS_MAX_NAME_SIZE)
            {
                if (data_len < DNS_MIN_NAME_ALLOC)
                {
                    data_len = DNS_MIN_NAME_ALLOC;
                }

                /* Allocate memory for the name. */
                status = NU_Allocate_Memory(MEM_Cached, (VOID **)&data,
                                            data_len, NU_NO_SUSPEND);

                if (status == NU_SUCCESS)
                {
                    /* Extract the data from the pointer record. */
                    data_len = DNS_Unpack_Domain_Name(data, data_len,
                                                      &recv_buffer[DNS_SRV_DATA_OFFSET], (CHAR*)pkt);

                    /* If a record already exists. */
                    if (dns_ptr)
                    {
                        /* Determine if the data has changed. */
                        if (NU_STRICMP(dns_ptr->dns_record_data, data) != 0)
                        {
                            /* Copy the new data. */
                            strcpy(dns_ptr->dns_record_data, data);

                            /* Set the new data length. */
                            dns_ptr->dns_h_length = strlen(data);

                            /* Notify the application of the change. */
                            mdns_ptr = qry_ptr->mdns_callback.head;
                        }
                    }

                    /* Create a new record. */
                    else
                    {
                        /* Set up the SRV record specific data. */
                        srv_data.dns_srv_prio = GET16(recv_buffer, DNS_SRV_PRIO_OFFSET);
                        srv_data.dns_srv_weight = GET16(recv_buffer, DNS_SRV_WEIGHT_OFFSET);
                        srv_data.dns_srv_port = GET16(recv_buffer, DNS_SRV_PORT_OFFSET);

                        /* Create a new record for this response. */
                        dns_ptr = DNS_Add_Host(name, data, ttl, NU_FAMILY_UNSPEC, type,
                                               strlen(data), 0, &srv_data);
                    }
                }
            }

            else
            {
                status = NU_NO_MEMORY;
            }

            /* Invalidate this record so it isn't processed again by a subsequent
             * call to DNS_Extract_Data if there are A or AAAA records present that
             * have not been processed yet.
             */
            PUT16(recv_buffer, DNS_TYPE_OFFSET, 0xffff);

            /* Consider this record processed even if memory allocation failed. */
            rec_count ++;

            break;

        case DNS_TYPE_PTR :

            /* A PTR record's data cannot change from one response to the next,
             * so the only thing to do for PTR records is to create a new one if
             * an existing one was not found.
             */
            if (!dns_ptr)
            {
                /* Allocate memory for the name. */
                status = NU_Allocate_Memory(MEM_Cached, (VOID **)&data,
                                            DNS_MAX_NAME_SIZE, NU_NO_SUSPEND);

                if (status == NU_SUCCESS)
                {
#if (INCLUDE_IPV4 == NU_TRUE)
                    /* If this is an IPv4 address PTR record. */
                    if ( (strlen(name) >= 13) &&
                         (NU_STRICMP(&name[strlen(name) - 13], ".IN-ADDR.ARPA") == 0) )
                    {
                        family = NU_FAMILY_IP;
                    }

#if (INCLUDE_IPV6 == NU_TRUE)
                    /* If this is an IPv6 address PTR record. */
                    else
#endif
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
                    if ( (strlen(name) >= 9) &&
                         (NU_STRICMP(&name[strlen(name) - 9], ".IP6.ARPA") == 0) )
                    {
                        family = NU_FAMILY_IP6;
                    }
#endif

                    else
                    {
                        family = NU_FAMILY_UNSPEC;
                    }

                    /* Extract the data from the pointer record.  Only process this one
                     * record, since there is only one name that will match the given
                     * IP address.  Any other PTR records in the packet are for other
                     * host records.
                     */
                    data_len = DNS_Unpack_Domain_Name(data, DNS_MAX_NAME_SIZE,
                                                      &recv_buffer[DNS_RDATA_OFFSET],
                                                      (CHAR*)pkt);

                    /* If this is a reverse-address mapped PTR record. */
                    if (family != NU_FAMILY_UNSPEC)
                    {
                        /* Create a new record for this response. */
                        dns_ptr = DNS_Add_Host(data, qry_ptr->mdns_data, ttl, family,
                                               type, 1, 0, NU_NULL);
                    }

                    else
                    {
                        /* Create a new record for this response. */
                        dns_ptr = DNS_Add_Host(name, data, ttl, family, type,
                                               strlen(data), 0, NU_NULL);
                    }
                }
            }

            /* Invalidate this record so it isn't processed again by a subsequent
             * call to DNS_Extract_Data if there are A or AAAA records present that
             * have not been processed yet.
             */
            PUT16(recv_buffer, DNS_TYPE_OFFSET, 0xffff);

            /* Consider this record processed even if memory allocation failed. */
            rec_count ++;

            break;

        case DNS_TYPE_TXT:

            /* Determine the number of bytes in the data portion of the packet. */
            data_len = GET16(recv_buffer, DNS_RDLENGTH_OFFSET);

            /* If there is data in the packet, set a pointer to the data. */
            if (data_len)
            {
                /* Get a pointer to the data. */
                data = &recv_buffer[DNS_RDATA_OFFSET];
            }

            else
            {
                data = NU_NULL;
            }

            /* If there is already a matching TXT record, check if it has
             * changed.
             */
            if (dns_ptr)
            {
                if (memcmp(dns_ptr->dns_record_data, data, data_len) != 0)
                {
                    /* Update the existing record with the new data. */
                    new_dns_host = DNS_Add_Host(dns_ptr->dns_name, data,
                                                dns_ptr->dns_ttl, dns_ptr->dns_family,
                                                DNS_TYPE_TXT, data_len,
                                                dns_ptr->dns_flags, NU_NULL);

                    /* Flag the host as unique so a Goodbye packet is not
                     * sent when the record is deleted.
                     */
                    dns_ptr->dns_flags |= DNS_UNIQUE_RECORD;

                    /* Delete the old entry. */
                    DNS_Delete_Host(dns_ptr);

                    dns_ptr = new_dns_host;
                }
            }

            /* Otherwise, add a new record. */
            else
            {
                /* Create a new record for this response. */
                dns_ptr = DNS_Add_Host(name, data, ttl, NU_FAMILY_UNSPEC, type, data_len,
                                       0, NU_NULL);
            }

            /* Set this pointer back to NULL so the data is not improperly
             * deallocated below.
             */
            data = NU_NULL;

            /* Invalidate this record so it isn't processed again by a subsequent
             * call to DNS_Extract_Data if there are A or AAAA records present that
             * have not been processed yet.
             */
            PUT16(recv_buffer, DNS_TYPE_OFFSET, 0xffff);

            /* Consider this record processed even if memory allocation failed. */
            rec_count ++;

            break;

        default:

            /* This should never happen since we would not be querying for any
             * type of record that we don't support.
             */
            break;
        }

        /* If processing was successful. */
        if ( (status == NU_SUCCESS) && (dns_ptr) )
        {
            /* Return a pointer to the new record. */
            *dns_ptr_ptr = dns_ptr;

            /* If this record was just created, or just transitioned from a
             * one-shot record to a continuous query record.
             */
            if (dns_ptr->mdns_query == NU_NULL)
            {
                /* Link the query to the host. */
                dns_ptr->mdns_query = qry_ptr;

                /* Allocate memory for the timer. */
                status = NU_Allocate_Memory(MEM_Cached, (VOID **)&dns_ptr->mdns_timer,
                                            sizeof(NU_TIMER), NU_NO_SUSPEND);

                if (status == NU_SUCCESS)
                {
                    /* Zero out the memory. */
                    memset(dns_ptr->mdns_timer, 0, sizeof(NU_TIMER));

                    /* Create a new timer.  The initial expiration does not matter, but cannot
                     * be 0 due to the routine's requirements.
                     */
                    NU_Create_Timer(dns_ptr->mdns_timer, "QCM", MDNS_Event_Handler,
                                    dns_ptr->dns_id, 0xffffffff, 0, NU_DISABLE_TIMER);
                }
            }

            else if (dns_ptr->mdns_timer)
            {
                /* Stop the timer. */
                NU_Control_Timer(dns_ptr->mdns_timer, NU_DISABLE_TIMER);
            }

            /* If the cache-flush bit is set, there is no need for the querier
             * to continue issuing a stream of queries with exponentially-increasing
             * intervals, since the receipt of a unique answer is a good indication
             * that no other answers will be forthcoming. In this case, the Multicast
             * DNS Querier SHOULD plan to issue its next query for this record at
             * 80-82% of the record's TTL...
             */
            if ((GET16(recv_buffer, DNS_CLASS_OFFSET)) & DNS_CACHE_FLUSH_FLAG)
            {
                /* Stop the query timer. */
                NU_Control_Timer(&qry_ptr->mdns_qry_timer, NU_DISABLE_TIMER);

                /* Set the expiration time to zero. */
                qry_ptr->mdns_qry_expire = 0;
            }

            /* Set the state to querying. */
            dns_ptr->mdns_state = MDNS_QUERYING;

            /* Reset querier cache maintenance. */
            dns_ptr->mdns_retrans_count = 0;

            /* Reset the POOF counter - this should only be reset if there is
             * an application interested in the record.
             */
            dns_ptr->mdns_foreign_query_cnt = 0;

            /* Convert the TTL to ticks. */
            ttl *= SCK_Ticks_Per_Second;

            /* The Querier should plan to issue a query at 80% of the record
             * lifetime.
             */
            dns_ptr->mdns_expire = (ttl - (ttl / 5));

            /* Validate the timer and set the timer to expire at 80% and then
             * again at 85%, 90% and 95% via the reschedule input parameter.
             */
            if (dns_ptr->mdns_timer)
            {
                NU_Reset_Timer(dns_ptr->mdns_timer, MDNS_Event_Handler,
                               dns_ptr->mdns_expire, (ttl - dns_ptr->mdns_expire) >> 2,
                               NU_ENABLE_TIMER);
            }

            /* Get a pointer to the query's response list. */
            res_ptr = qry_ptr->mdns_host_list.dns_head;

            /* Search for this entry in the query list. */
            while (res_ptr)
            {
                if (res_ptr->mdns_host == dns_ptr)
                    break;

                res_ptr = res_ptr->mdns_next;
            }

            /* If an entry was not found, add it. */
            if (!res_ptr)
            {
                /* Allocate memory for the response structure. */
                status = NU_Allocate_Memory(MEM_Cached, (VOID **)&res_ptr,
                                            sizeof(MDNS_RESPONSE), NU_NO_SUSPEND);

                if (status == NU_SUCCESS)
                {
                    /* Zero out the memory. */
                    memset(res_ptr, 0, sizeof(MDNS_RESPONSE));

                    /* Link the response to the query. */
                    res_ptr->mdns_host = dns_ptr;

                    /* Add this response to the list of responses for this query. */
                    DLL_Enqueue(&qry_ptr->mdns_host_list, res_ptr);

                    /* Notify the application that a new record has been found. */
                    mdns_ptr = qry_ptr->mdns_callback.head;
                }
            }
        }

        /* If memory was allocated, deallocate it now. */
        if (data)
        {
            NU_Deallocate_Memory(data);
        }

        /* Signal each interested task. */
        while (mdns_ptr)
        {
            NU_Send_Signals(mdns_ptr->callback_ptr, (1UL << MDNS_SIGNAL));
            mdns_ptr = mdns_ptr->next;
        }
    }

    return (rec_count);

} /* MDNS_Process_Incoming_Response */

/****************************************************************************
*
*   FUNCTION
*
*       MDNS_Find_Matching_Record
*
*   DESCRIPTION
*
*       This routine finds a record matching an incoming response.
*
*       The determination of whether a given record answers a given question
*       is done using the standard DNS rules: The record name must match the
*       question name, the record rrtype must match the question qtype unless
*       the qtype is "ANY" (255) or the rrtype is "CNAME" (5), and the record
*       rrclass must match the question qclass unless the qclass is "ANY"
*       (255). As with unicast DNS, generally only DNS class 1 ("Internet")
*       is used, but should client software use classes other than 1 the
*       matching rules described above MUST be used.
*
*   INPUTS
*
*       *recv_buffer            A pointer to the beginning of the fixed portion
*                               of the response record.
*       *pkt                    A pointer to the head of the packet.
*       *name                   The name corresponding to the response.
*
*   OUTPUTS
*
*       A pointer to the local DNS record or NU_NULL if no record exists.
*
******************************************************************************/
DNS_HOST *MDNS_Find_Matching_Record(CHAR *recv_buffer, CHAR *pkt, CHAR *name)
{
    UINT16      type, class;
    DNS_HOST    *dns_ptr = NU_NULL;
    INT16       family;
    CHAR        ip_addr[MAX_ADDRESS_SIZE];
    CHAR        *data;

    /* Extract the class. */
    class = GET16(recv_buffer, DNS_CLASS_OFFSET);

    /* Ensure the class is type IN or ANY. */
    if ( (class & DNS_CLASS_IN) || (class & DNS_CLASS_ANY) )
    {
        /* Extract the type.  Note that type ANY records are processed by the
         * code that processes an incoming probe query.
         */
        type = GET16(recv_buffer, DNS_TYPE_OFFSET);

        /* If this is a PTR or SRV record. */
        switch (type)
        {
        case DNS_TYPE_PTR:
        case DNS_TYPE_SRV:
        {
#if (INCLUDE_IPV4 == NU_TRUE)
            /* If this is an IPv4 address PTR record. */
            if ( (strlen(name) >= 13) &&
                 (NU_STRICMP(&name[strlen(name) - 13], ".IN-ADDR.ARPA") == 0) )
            {
                family = NU_FAMILY_IP;

                /* Extract the address from the name. */
                DNS4_String_To_Addr(ip_addr, name);
            }

#if (INCLUDE_IPV6 == NU_TRUE)
            /* If this is an IPv6 address PTR record. */
            else
#endif
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
            if ( (strlen(name) >= 9) &&
                 (NU_STRICMP(&name[strlen(name) - 9], ".IP6.ARPA") == 0) )
            {
                family = NU_FAMILY_IP6;

                /* Extract the address from the name. */
                DNS6_String_To_Addr(ip_addr, name);
            }
#endif
            /* This is not a reverse-address mapped PTR record.  Try to find the
             * record by name and data.
             */
            else
            {
                /* This record does not hold an IP address. */
                family = NU_FAMILY_UNSPEC;

                /* If this is a query, find the record by name only. */
                if (!(GET16(pkt, DNS_FLAGS_OFFSET) & DNS_QR))
                {
                    /* Find a pointer to the record. */
                    dns_ptr = DNS_Find_Matching_Host_By_Name(name, NU_NULL, type);
                }

                /* Otherwise, match the name and data.  The name for a PTR record is
                 * not unique, so we could have multiple PTR records for the same
                 * name in the database.  We need to find the one associated with
                 * this host.
                 */
                else
                {
                    /* Allocate memory for the data that must be null-terminated to pass
                     * into DNS_Find_Matching_Host_By_Name().
                     */
                    if (NU_Allocate_Memory(MEM_Cached, (VOID **)&data, DNS_MAX_NAME_SIZE,
                                           NU_NO_SUSPEND) == NU_SUCCESS)
                    {
                        /* Extract the data from the pointer record. */
                        DNS_Unpack_Domain_Name(data,
                                               DNS_MAX_NAME_SIZE,
                                               &recv_buffer[type == DNS_TYPE_PTR ? DNS_RDATA_OFFSET : DNS_SRV_DATA_OFFSET],
                                               pkt);

                        /* Find a pointer to the record. */
                        dns_ptr = DNS_Find_Matching_Host_By_Name(name, data, type);

                        NU_Deallocate_Memory(data);
                    }
                }
            }

            /* Try to find the reverse-address mapped PTR record. */
            if (family != NU_FAMILY_UNSPEC)
            {
                /* Find a matching record. */
                dns_ptr = DNS_Find_Matching_Host_By_Addr((UINT8*)ip_addr, family);
            }

            break;
        }

#if (INCLUDE_IPV4 == NU_TRUE)
        case DNS_TYPE_A:
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
        case DNS_TYPE_AAAA:
#endif
        case DNS_TYPE_TXT:
        {
            /* Find the record by name only.  Only one record of this name will
             * exist in the system, but the data could have changed since the
             * last response was received.
             */
            dns_ptr = DNS_Find_Matching_Host_By_Name(name, NU_NULL, type);

            break;
        }

        default:
            break;
        }
    }

    return (dns_ptr);

} /* MDNS_Find_Matching_Record */

/****************************************************************************
*
*   FUNCTION
*
*       MDNS_Invoke_Probing
*
*   DESCRIPTION
*
*       This routine transitions the given DNS record to the probing state.
*
*   INPUTS
*
*       *dns_ptr                The record to transition to probing.
*       delay                   The time to delay sending the first probe.
*
*   OUTPUTS
*
*       None.
*
******************************************************************************/
VOID MDNS_Invoke_Probing(DNS_HOST *dns_ptr, UINT32 delay)
{
    STATUS status =  NU_SUCCESS;
    dns_ptr->mdns_state = MDNS_PROBING;
    dns_ptr->mdns_retrans_count = 0;

    /* Disable the timer so it can be reset. */
    status = NU_Control_Timer(dns_ptr->mdns_timer, NU_DISABLE_TIMER);

    /* Reset the probing timer.  Zero is an invalid initial timeout, so
     * ensure the timer can never be set to zero.
     */
    if(status == NU_SUCCESS)
    {
        (VOID)NU_Reset_Timer(dns_ptr->mdns_timer, MDNS_Event_Handler, delay, 0,
                            NU_ENABLE_TIMER);
    }


} /* MDNS_Invoke_Probing */

/****************************************************************************
*
*   FUNCTION
*
*       MDNS_Sort_Records
*
*   DESCRIPTION
*
*       This routine sorts incoming records according to the mDNS RFC:
*       Section 8.2.1 : The records are sorted using the same lexicographical
*       order as described above, that is: if the record classes differ, the
*       record with the lower class number comes first. If the classes are
*       the same but the rrtypes differ, the record with the lower rrtype
*       number comes first. If the class and rrtype match, then the rdata
*       is compared bytewise until a difference is found.
*
*       A bubble sort is used because there will most likely be only 1-4
*       records to sort.
*
*   INPUTS
*
*       *record                 A pointer to the head of the records.
*       count                   The number of records in the list.
*       *recv_buffer            A pointer to the DNS header of the incoming
*                               packet.
*
*   OUTPUTS
*
*       A pointer to memory allocated that holds the sorted list of pointers,
*       or NU_NULL if memory could not be allocated.
*
******************************************************************************/
MDNS_SORT_INDEX *MDNS_Sort_Records(CHAR *record, INT16 count, CHAR *recv_buffer)
{
    MDNS_SORT_INDEX saved_record;
    MDNS_SORT_INDEX *ptr_list = NU_NULL;
    INT             offset, i, j, name_len;

    /* Allocate memory for the list that will be returned to the caller. */
    if (NU_Allocate_Memory(MEM_Cached, (VOID **)&ptr_list,
                           (UNSIGNED)(sizeof(MDNS_SORT_INDEX) * count),
                           NU_SUSPEND) == NU_SUCCESS)
    {
        /* Zero out the memory. */
        memset(ptr_list, 0, (sizeof(MDNS_SORT_INDEX) * count));

        offset = 0;

        /* Populate the pointer list initially. */
        for (i = 0; i < count; i ++)
        {
            /* Determine the length of the name. */
            name_len = DNS_Unpack_Domain_Name(NU_NULL, DNS_MAX_NAME_SIZE, &record[offset],
                                              recv_buffer);

            /* Set up the data variable pointers. */
            ptr_list[i].r_class = GET16(&record[offset], DNS_CLASS_OFFSET + name_len);
            ptr_list[i].type = GET16(&record[offset], DNS_TYPE_OFFSET + name_len);
            ptr_list[i].data_ptr = &record[offset + DNS_RDATA_OFFSET + name_len];
            ptr_list[i].data_len = GET16(&record[offset], DNS_RDLENGTH_OFFSET + name_len);

            /* Add bytes for the variable data in the packet. */
            offset += (name_len + GET16(&record[offset], DNS_RDLENGTH_OFFSET + name_len));

            /* Add bytes for the fixed length of the packet. */
            offset += 10;
        }
    }

    /* Sort the records. */
    for (i = 0; i < (count - 1); i ++)
    {
        for (j = 0; j < (count - 1); j ++)
        {
            /* If this record is lexicographically greater than the next record,
             * swap the records.
             */
            if (MDNS_Compare_Records(&ptr_list[j], &ptr_list[j + 1]) == 1)
            {
                /* Make a copy of the record about to be overwritten. */
                memcpy(&saved_record, &ptr_list[j], sizeof(MDNS_SORT_INDEX));

                /* Swap the records in memory. */
                memcpy(&ptr_list[j], &ptr_list[j + 1], sizeof(MDNS_SORT_INDEX));
                memcpy(&ptr_list[j + 1], &saved_record, sizeof(MDNS_SORT_INDEX));
            }
        }
    }

    return (ptr_list);

} /* MDNS_Sort_Records */

/****************************************************************************
*
*   FUNCTION
*
*       MDNS_Compare_Records
*
*   DESCRIPTION
*
*       This routine compares records according to the mDNS RFC:
*       Section 8.2.1 : ... if the record classes differ, the record with
*       the lower class number comes first. If the classes are the same but
*       the rrtypes differ, the record with the lower rrtype number comes
*       first. If the class and rrtype match, then the rdata is compared
*       bytewise until a difference is found.
*
*   INPUTS
*
*       *a                      A record to compare.
*       *b                      A record to compare.
*
*   OUTPUTS
*
*       -1                      a is lexicographically less than b.
*       1                       b is lexicographically less than a.
*       0                       The records are the same.
*
******************************************************************************/
INT MDNS_Compare_Records(MDNS_SORT_INDEX *a, MDNS_SORT_INDEX *b)
{
    INT             j, offset;

    /* Compare class. */
    if (a->r_class < b->r_class)
        return (-1);
    else if (a->r_class > b->r_class)
        return (1);
    else
    {
        /* Compare type. */
        if (a->type < b->type)
            return (-1);
        else if (a->type > b->type)
            return (1);
        else
        {
            /* Determine which entry has less data. */
            if (a->data_len < b->data_len)
                offset = a->data_len;
            else
                offset = b->data_len;

            /* Compare bytes until out of bytes to compare. */
            for (j = 0; j < offset; j ++)
            {
                if (a->data_ptr[j] < b->data_ptr[j])
                    return (-1);
                else if (a->data_ptr[j] > b->data_ptr[j])
                    return (1);
            }

            /* Whichever entry has less data remaining wins. */
            if (a->data_len < b->data_len)
                return (-1);
            if (a->data_len > b->data_len)
                return (1);
        }
    }

    /* The records are the same. */
    return (0);

} /* MDNS_Compare_Records */

/****************************************************************************
*
*   FUNCTION
*
*       MDNS_Send_Announcement
*
*   DESCRIPTION
*
*       This routine sends an outgoing response message for announcing
*       ownership of a local record.
*
*   INPUTS
*
*       socketd                 The socket out which to send the response.
*       *addr                   The address to which to transmit the
*                               response, or NU_NULL to send the response
*                               via multicast out all interfaces.
*       *dnsv4_ptr              The IPv4 record to add to the response.
*       *dnsv6_ptr              The IPv6 record to add to the response.
*       *hostname               The hostname for the response.
*
*   OUTPUTS
*
*       NU_SUCCESS or an operating-system specific error upon failure.
*
******************************************************************************/
STATUS MDNS_Send_Announcement(INT socketd, struct addr_struct *addr, DNS_HOST *dnsv4_ptr,
                              DNS_HOST *dnsv6_ptr, CHAR *hostname)
{
    INT         q_size, total_size = 0, i, j, ip_idx;
    UINT16      auth_count = 0;
    CHAR        *buffer;
    STATUS      status;
    DNS_HOST    *dns_ptrs[] = {dnsv4_ptr, dnsv6_ptr};

    /* Build the response header. */
    q_size = DNS_Build_Header((VOID **)&buffer, 0, DNS_QR | DNS_AA);

    if (q_size > 0)
    {
        /* The current size of the packet is just the DNS header. */
        total_size = DNS_FIXED_HDR_LEN;

        /* Add any IPv4 and IPv6 records to the packet. */
        for (j = 0; j < 2; j++)
        {
            /* If there are no records of this family type, move on to
             * the next family type.
             */
            if (!dns_ptrs[j])
                continue;

            /* Add each IP address to the Answer Section. */
            for (i = 0, ip_idx = 0;
                 (i < DNS_MAX_IP_ADDRS) && (buffer) &&
                 (!(IP_IS_NULL_ADDR(&dns_ptrs[j]->dns_record_data[ip_idx])));
                 i++, ip_idx += MAX_ADDRESS_SIZE)
            {
                /* Insert the hostname record. */
                total_size = MDNS_Send_Partial_Response(dns_ptrs[j], dns_ptrs[j]->mdns_dev_index,
                                                        hostname, DNS_FIXED_HDR_LEN, &buffer,
                                                        dns_ptrs[j]->dns_type,
                                                        DNS_CLASS_IN | DNS_CACHE_FLUSH_FLAG,
                                                        &dns_ptrs[j]->dns_record_data[ip_idx],
                                                        &auth_count, 0, total_size,
                                                        DNS_QR | DNS_AA, addr);

                /* A partial response was either unsuccessfully sent, or a new buffer for
                 * the rest of the response could not be allocated.  Exit the loop.
                 */
                if (!buffer)
                    break;

                /* Send the PTR record. */
                total_size = MDNS_Send_Partial_Response(dns_ptrs[j], dns_ptrs[j]->mdns_dev_index,
                                                        hostname, DNS_FIXED_HDR_LEN, &buffer,
                                                        DNS_TYPE_PTR,
                                                        DNS_CLASS_IN | DNS_CACHE_FLUSH_FLAG,
                                                        &dns_ptrs[j]->dns_record_data[ip_idx],
                                                        &auth_count, 0, total_size,
                                                        DNS_QR | DNS_AA, addr);
            }
        }

        /* Check if an error needs to be set. */
        if (total_size > 0)
            status = NU_SUCCESS;
        else
            status = total_size;

        if (buffer)
        {
            if (auth_count != 0)
            {
                /* Record the number of authority records. */
                PUT16(buffer, DNS_ANCOUNT_OFFSET, auth_count);

                /* If this is a multicast response. */
                if (!addr)
                {
                    /* Send the DNS response.  The index will be the same for both
                     * IPv4 and IPv6, so use either.
                     */
                    status = MDNS_TX_Packet(buffer, (UINT16)total_size, dnsv4_ptr ?
                                            dnsv4_ptr->mdns_dev_index :
                                            dnsv6_ptr->mdns_dev_index);
                }

                else
                {
                    /* Send the response to a unicast address. */
                    status = NU_Send_To(MDNS_Socket, buffer, (UINT16)total_size,
                                        0, addr, 0);

                    if (status > 0)
                    {
                        status = NU_SUCCESS;
                    }
                }
            }

            NU_Deallocate_Memory(buffer);
        }
    }

    return (status);

} /* MDNS_Send_Announcement */

/****************************************************************************
*
*   FUNCTION
*
*       MDNS_Send_Partial_Response
*
*   DESCRIPTION
*
*       This routine tries to insert a record into the buffer.  If the record
*       cannot be inserted, it sends the current buffer and allocates a new
*       buffer for the record.
*
*   INPUTS
*
*       *dns_ptr                A pointer to the record to transmit.
*       dev_index               The interface out which to transmit the
*                               partial packet or -1 to transmit the
*                               partial packet out all interfaces.
*       *hostname               The hostname for the response.
*       offset                  The byte offset in the packet that the
*                               hostname can be found if using header
*                               compression.
*       **buffer                A pointer to the buffer for the response.
*       type                    The type of record.
*       class                   The class of the record.
*       *data                   A pointer to the data to insert into the
*                               data portion of the record.
*       *ans_count              The number of answers in the packet.
*       *add_ans_count          The number of additional answers in the
*                               packet.
*       total_size              The number of bytes in the buffer already.
*       hdr_flags               Flags to set in the DNS header.
*       *addr                   If it is NU_NULL then multicast response, otherwise
*                               send response to specific address.
*
*   OUTPUTS
*
*       The number of bytes in the buffer or an operating system specific
*       error.
*
******************************************************************************/
INT16 MDNS_Send_Partial_Response(DNS_HOST *dns_ptr, INT dev_index,
                                 CHAR *host_name, INT16 offset, CHAR **buffer,
                                 UINT16 type, UINT16 class, CHAR *data,
                                 UINT16 *ans_count, UINT16 *add_ans_count,
                                 INT16 total_size, UINT16 hdr_flags,
                                 struct addr_struct *addr)
{
    INT     q_size, send_size;
    UINT32  dest_len;

    /* The total bytes available in the current packet is the max message size
     * minus the bytes already contained in the packet.  If the record cannot
     * be added to the packet, this value must be updated to reflect the newly
     * allocated buffer with just a header.
     */
    dest_len = DNS_MAX_MESSAGE_SIZE - total_size;

    do
    {
        /* Add the record to the packet.  The routine will use header compression
         * to compress the host name in A, AAAA and PTR records only if an offset
         * to that name was provided and the size indicates that another record
         * already exists that contains the full name.
         */
        q_size = DNS_Insert_Record(dns_ptr,
                                   (total_size == DNS_FIXED_HDR_LEN) || (offset == 0) ? host_name : NU_NULL,
                                   offset, &((*buffer)[total_size]), type, class,
                                   data, dest_len);

        /* If the record could be added, increment the number of records, update
         * the total size, and exit the loop.
         */
        if (q_size > 0)
        {
            /* Increment the count of whichever type of answer was added. */
            if (!add_ans_count)
            {
                (*ans_count) ++;
            }

            else
            {
                (*add_ans_count) ++;
            }

            total_size += q_size;
        }

        /* If the buffer has reached its size limit, and there is a question or
         * response to send, transmit the packet.  total_size will only be equal
         * to DNS_FIXED_HDR_LEN if we failed to insert a single query/response in
         * the packet.  We shouldn't send a packet with just a generic header on it.
         */
        else if (total_size != DNS_FIXED_HDR_LEN)
        {
            /* Insert the number of records into the packet. */
            PUT16(*buffer, DNS_ANCOUNT_OFFSET, *ans_count);

            if (add_ans_count)
            {
                /* Insert the number of additional answers into the packet. */
                PUT16(*buffer, DNS_ARCOUNT_OFFSET, *add_ans_count);
            }

            if (dns_ptr->mdns_state == MDNS_QUERYING)
            {
                /* It MUST then set the TC bit in the header before sending the Query. */
                PUT16(*buffer, DNS_FLAGS_OFFSET, DNS_TC);
            }

            /* Transmit the packet via unicast to the address specified. */
            if (addr)
            {
                /* Send the data back to the host that performed Query. */
                send_size = NU_Send_To(MDNS_Socket, *buffer, (UINT16)total_size,
                                       0, addr, 0);
            }

            else
            {
                /* If no interface or address information was provided, send the packet
                 * out all interfaces over multicast.
                 */
                if (dev_index == -1)
                {
                    /* Send the data out all interfaces. */
                    if (MDNS_TX_All_Interfaces(*buffer, (UINT16)total_size) == NU_SUCCESS)
                    {
                        send_size = total_size;
                    }

                    else
                    {
                        send_size = NU_NO_ACTION;
                    }
                }

                /* If an address was not provided, but a device index was provided,
                 * multicast the packet out the specified interface over IPv4 and
                 * IPv6.
                 */
                else
                {
                    /* Send the DNS response. */
                    if (MDNS_TX_Packet(*buffer, (UINT16)total_size, dev_index) == NU_SUCCESS)
                    {
                        send_size = total_size;
                    }

                    else
                    {
                        send_size = NU_NO_ACTION;
                    }
                }

                if (send_size > 0)
                {
                    /* Store the last time this recored was sent via multicast. */
                    dns_ptr->mdns_last_local_resp_time = NU_Retrieve_Clock();
                }
            }

            /* Deallocate the buffer. */
            NU_Deallocate_Memory(*buffer);
            *buffer = NU_NULL;

            /* If the last packet could be sent, allocate a new buffer for
             * the record that could not be added to the last packet.
             */
            if (send_size > 0)
            {
                /* Allocate a buffer and build the response header. */
                if (DNS_Build_Header((VOID **)buffer, 0, hdr_flags) > 0)
                {
                    /* The current size of the packet is just the DNS header. */
                    total_size = DNS_FIXED_HDR_LEN;

                    /* Do not try to use header compression. */
                    offset = 0;

                    /* Reset the number of answers and additional answers. */
                    *ans_count = 0;

                    if (add_ans_count)
                    {
                        *add_ans_count = 0;
                    }

                    /* If the record that could not be added to the last packet
                     * would not fit in a packet with just a DNS header,  we cannot
                     * send this record, because the DNS buffer size is too small.
                     * We will skip this record and move on to the next.
                     */
                    if (dest_len == (DNS_MAX_MESSAGE_SIZE - DNS_FIXED_HDR_LEN))
                        break;

                    /* Update the number of bytes available in the new
                     * buffer.
                     */
                    dest_len = DNS_MAX_MESSAGE_SIZE - DNS_FIXED_HDR_LEN;
                }

                else
                {
                    /* A new buffer could not be allocated. */
                    total_size = NU_NO_MEMORY;
                    break;
                }
            }

            else
            {
                /* Return the error generated by NU_Send_To(). */
                total_size = send_size;
                break;
            }
        }

        else
        {
            total_size = NU_NO_MEMORY;
            break;
        }

    } while (q_size <= 0);

    return (total_size);

} /* MDNS_Send_Partial_Response */

/****************************************************************************
*
*   FUNCTION
*
*       MDNS_Build_Probe
*
*   DESCRIPTION
*
*       This routine builds an outgoing query message used in probing.  Note
*       that the entire query must fit in one message.  If the user has defined
*       the buffer size to be less than the requirement for a query, no query
*       will be transmitted.
*
*   INPUTS
*
*       **buf_ptr               A pointer to the buffer that will be allocated
*                               that holds the query.
*       *dns_ptr                The record that prompted the query.  This will
*                               match either dnsv4_ptr or dnsv6_ptr.
*       *dnsv4_ptr              The IPv4 record to add to the query.
*       *dnsv6_ptr              The IPv6 record to add to the query.
*       *hostname               The hostname for the query.
*
*   OUTPUTS
*
*       The size of the buffer or an operating-system specific error upon
*       failure.
*
******************************************************************************/
INT MDNS_Build_Probe(CHAR **buf_ptr, DNS_HOST *dns_ptr, DNS_HOST *dnsv4_ptr,
                     DNS_HOST *dnsv6_ptr, CHAR *hostname)
{
    CHAR        *buffer;
    INT         total_size, q_size, i, j, ip_idx, record_count = 0;
    DNS_HOST    *dns_ptrs[] = {dnsv4_ptr, dnsv6_ptr};

    /* Build the header. */
    q_size = DNS_Build_Header((VOID **)&buffer, 0, 0);

    if (q_size > 0)
    {
        /* The total number of bytes in the packet is currently
         * just the length of the header.
         */
        total_size = DNS_FIXED_HDR_LEN;

        /* Add a query section for each of the hostname record
         * types that will be added to the Authority Section.
         */
#if (INCLUDE_IPV4 == NU_TRUE)
        if (dnsv4_ptr)
        {
            /* Build the query portion. */
            q_size = DNS_Build_Query(hostname, DNS_FIXED_HDR_LEN,
                                     &buffer[total_size], DNS_TYPE_ANY,
                                     NU_FAMILY_IP,
                                     dns_ptr->mdns_retrans_count == 0 ?
                                     DNS_CLASS_IN | DNS_UNI_FLAG : DNS_CLASS_IN,
                                     DNS_MAX_MESSAGE_SIZE - total_size);

            if (q_size > 0)
            {
                record_count ++;
                total_size += q_size;
            }
        }
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
        if ( (dnsv6_ptr) && (q_size > 0) )
        {
            /* Build the query portion. */
            q_size = DNS_Build_Query(
#if (INCLUDE_IPV4 == NU_TRUE)
                                     dnsv4_ptr != NU_NULL ? NU_NULL :
#endif
                                     hostname, DNS_FIXED_HDR_LEN,
                                     &buffer[total_size], DNS_TYPE_ANY,
                                     NU_FAMILY_IP6,
                                     dns_ptr->mdns_retrans_count == 0 ?
                                     DNS_CLASS_IN | DNS_UNI_FLAG : DNS_CLASS_IN,
                                     DNS_MAX_MESSAGE_SIZE - total_size);

            if (q_size > 0)
            {
                record_count ++;
                total_size += q_size;
            }
        }
#endif

        /* Insert the number of queries in the packet. */
        PUT16(buffer, DNS_QDCOUNT_OFFSET, record_count);

        record_count = 0;

        /* Add any IPv4 and IPv6 records to the packet. */
        for (j = 0; j < 2; j++)
        {
            /* If there are no records of this family type, move on to
             * the next family type.
             */
            if (!dns_ptrs[j])
                continue;

            /* If we have not exceeded the buffer size. */
            if (q_size > 0)
            {
                /* Add each IP address to the Authority Section. */
                for (i = 0, ip_idx = 0; (i < DNS_MAX_IP_ADDRS) && (q_size > 0) &&
                     (!(IP_IS_NULL_ADDR(&dns_ptrs[j]->dns_record_data[ip_idx])));
                     i++, record_count ++, total_size += q_size, ip_idx += MAX_ADDRESS_SIZE)
                {
                    /* Add the record to the packet. */
                    q_size = DNS_Insert_Record(dns_ptrs[j], NU_NULL, DNS_FIXED_HDR_LEN,
                                               &buffer[total_size], dns_ptrs[j]->dns_type,
                                               DNS_CLASS_IN,
                                               &dns_ptrs[j]->dns_record_data[ip_idx],
                                               DNS_MAX_MESSAGE_SIZE - total_size);
                }
            }
        }
    }

    /* If all entries will not fit in one query, this query cannot be sent. */
    if (q_size <= 0)
    {
        total_size = NU_NO_MEMORY;

        /* Deallocate the buffer. */
        NU_Deallocate_Memory(buffer);
    }

    else
    {
        /* Insert the number of authority records in the packet. */
        PUT16(buffer, DNS_NSCOUNT_OFFSET, record_count);

        *buf_ptr = buffer;
    }

    return (total_size);

} /* MDNS_Build_Probe */

/****************************************************************************
*
*   FUNCTION
*
*       MDNS_Build_Query
*
*   DESCRIPTION
*
*       This routine builds an outgoing query message used for continuous
*       resolution.
*
*   INPUTS
*
*       **buf_ptr               A pointer to the buffer that will be allocated
*                               that holds the query.
*       *question               The query question.
*       type                    The type of record to query.
*       family                  The family type of the record.
*
*   OUTPUTS
*
*       The size of the buffer or an operating-system specific error upon
*       failure.
*
******************************************************************************/
INT MDNS_Build_Query(CHAR **buf_ptr, CHAR *question, INT16 type, INT16 family)
{
    CHAR    *buffer;
    INT     total_size, q_size;

    /* Build the header. */
    q_size = DNS_Build_Header((VOID **)&buffer, 0, 0);

    if (q_size > 0)
    {
        /* The total number of bytes in the packet is currently
         * just the length of the header.
         */
        total_size = DNS_FIXED_HDR_LEN;

        /* Add a query section for the record. */
        q_size = DNS_Build_Query(question, DNS_FIXED_HDR_LEN,
                                 &buffer[total_size], type, family,
                                 DNS_CLASS_IN,
                                 DNS_MAX_MESSAGE_SIZE - total_size);

        if (q_size > 0)
        {
            total_size += q_size;
        }

        /* Insert the number of queries in the packet. */
        PUT16(buffer, DNS_QDCOUNT_OFFSET, 1);
    }

    /* If all entries will not fit in one query, this query cannot be sent. */
    if (q_size <= 0)
    {
        total_size = NU_NO_MEMORY;

        /* Deallocate the buffer. */
        NU_Deallocate_Memory(buffer);
    }

    else
    {
        *buf_ptr = buffer;
    }

    return (total_size);

} /* MDNS_Build_Query */

/****************************************************************************
*
*   FUNCTION
*
*       MDNS_Initialize_Hostname
*
*   DESCRIPTION
*
*       This function creates initial local host name for an interface based
*       on the interface index and the domain '.local'.
*
*   INPUTS
*
*       *device                 A pointer to the device for which to create
*                               a hostname or NU_NULL.
*
*   OUTPUTS
*
*       A pointer to the newly created host name upon success.  NU_NULL
*       upon error.
*
******************************************************************************/
CHAR *MDNS_Initialize_Hostname(UINT32 index)
{
    CHAR    *name_ptr;

    name_ptr = MDNS_Create_Hostname_Callback(index, NU_FALSE);

    return (name_ptr);

} /* MDNS_Initialize_Hostname */

/****************************************************************************
*
*   FUNCTION
*
*       MDNS_Create_Default_Hostname
*
*   DESCRIPTION
*
*       This function creates a local host name for an interface based on
*       the interface index and the domain '.local'.
*
*   INPUTS
*
*       *device                 A pointer to the device for which to create
*                               a hostname or NU_NULL.
*       conflict                NU_TRUE if the host name is being reconfigured
*                               due to a conflict.
*                               NU_FALSE is this is the first host name assigned
*                               to the interface.
*
*   OUTPUTS
*
*       A pointer to the newly created host name upon success.  NU_NULL
*       upon error.
*
******************************************************************************/
CHAR *MDNS_Create_Default_Hostname(UINT32 index, BOOLEAN conflict)
{
    CHAR        int_index[20];
    UINT32      numeric_portion;
    STATUS      status;
    CHAR        *name_ptr = NU_NULL;

    /* If a device is not being passed into the routine, a random host name
     * should be generated.
     */
    if (conflict)
    {
        /* Instead of using the interface index, generate a random number. */
        numeric_portion = UTL_Rand();
    }

    else
    {
        /* Try to use the interface index for the first round of probing. */
        numeric_portion = index;
    }

    /* Convert the numeric portion to an ASCII string and add the
     * ".local" to the end.
     */
    NCL_Ultoa(numeric_portion, int_index, 10);
    strcat(int_index, ".local");

    /* Allocate a block of memory for the full host name. */
    status = NU_Allocate_Memory(MEM_Cached, (VOID **)&name_ptr,
                                (UNSIGNED)(strlen((CHAR*)SCK_Host_Name) +
                                strlen(int_index) + 1), NU_NO_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Copy the base part of the host name. */
        strcpy(name_ptr, (CHAR*)SCK_Host_Name);

        /* Append the numerical portion and .local to the name. */
        strcat(name_ptr, int_index);
    }

    return (name_ptr);

} /* MDNS_Create_Default_Hostname */

/****************************************************************************
*
*   FUNCTION
*
*       MDNS_Register_Local_Host
*
*   DESCRIPTION
*
*       This function creates and registers a local host name for an
*       interface.  If the name already exists, the IP address is
*       added to the record.
*
*   INPUTS
*
*       *device                 A pointer to the device for which to create
*                               the new local host record.
*       *ip_addr                The IP address to add.
*       family                  The family type of the IP address.
*
*   OUTPUTS
*
*       NU_SUCCESS upon successful completion, or an operating-system
*       specific error upon failure.
*
******************************************************************************/
STATUS MDNS_Register_Local_Host(DV_DEVICE_ENTRY *device, UINT8 *ip_addr, INT16 family)
{
    STATUS      status;

    /* Obtain the mDNS semaphore. */
    status = NU_Obtain_Semaphore(&MDNS_Resource, NU_NO_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Obtain the DNS semaphore. */
        status = NU_Obtain_Semaphore(&DNS_Resource, NU_NO_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Register the interface. */
            MDNS_Perform_Registration(device, ip_addr, family);

            NU_Release_Semaphore(&DNS_Resource);
        }

        NU_Release_Semaphore(&MDNS_Resource);
    }

    return (status);

} /* MDNS_Register_Local_Host */

/****************************************************************************
*
*   FUNCTION
*
*       MDNS_Perform_Registration
*
*   DESCRIPTION
*
*       This function creates and registers a local host name for an
*       interface.  If the name already exists, the IP address is
*       added to the record.
*
*   INPUTS
*
*       *device                 A pointer to the device for which to create
*                               the new local host record.
*       *ip_addr                The IP address to add.
*       family                  The family type of the IP address.
*
*   OUTPUTS
*
*       NU_SUCCESS upon successful completion, or an operating-system
*       specific error upon failure.
*
******************************************************************************/
STATUS MDNS_Perform_Registration(DV_DEVICE_ENTRY *device, UINT8 *ip_addr,
                                 INT16 family)
{
    DNS_HOST    *dns_ptr;
    STATUS      status = NU_SUCCESS;
    UNSIGNED    data[3];

#if (INCLUDE_IPV6 == NU_TRUE)
    if (family == NU_FAMILY_IP6)
    {
        /* Get a pointer to the IPv6 host entry if it already exists. */
        dns_ptr = DNS_Find_Matching_Host_By_Name(device->dev_host_name, NU_NULL,
                                                 DNS_TYPE_AAAA);

        if (!dns_ptr)
        {
            /* Store the device index to send to the task that will join the
             * IP multicast group.
             */
            data[2] = device->dev_index;
        }
    }

#if (INCLUDE_IPV4 == NU_TRUE)
    else
#endif
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
    {
        /* Get a pointer to the IPv4 host entry if it already exists. */
        dns_ptr = DNS_Find_Matching_Host_By_Name(device->dev_host_name, NU_NULL,
                                                 DNS_TYPE_A);

        /* If this is the first time an IPv4 hostname has been configured for
         * the interface, join the multicast group and set a timer to start
         * probing and announcing.
         */
        if (!dns_ptr)
        {
            /* Store the IP address to send to the task that will join the
             * IP multicast group.
             */
            data[2] = device->dev_addr.dev_addr_list.dv_head->dev_entry_ip_addr;
        }
    }
#endif

    /* If the host entry already exists, add the IP address to the
     * record.
     */
    if (dns_ptr)
    {
        status = DNS_Update_Host(dns_ptr, (CHAR*)ip_addr);

        if (status == NU_SUCCESS)
        {
            /* If this record is not already in the probing state,
             * validate and reset the timer
             */
            if ( (dns_ptr->mdns_timer) &&
                 (dns_ptr->mdns_state != MDNS_PROBING) )
            {
                /* Reset the probing timer.  Zero is an invalid initial timeout, so
                 * ensure the timer can never be set to zero.
                 */
                status = NU_Reset_Timer(dns_ptr->mdns_timer, MDNS_Event_Handler,
                                        (UTL_Rand() % MDNS_PROBE_DELAY) + 1, 0,
                                        NU_ENABLE_TIMER);
            }
        }
    }

    /* Otherwise, add a new host entry. */
    else
    {
        data[0] = MDNS_REGISTER_DEVICE;
        data[1] = family;

        /* Send the data to the queue to join the multicast group once the mDNS
         * socket is created.
         */
        if (NU_Send_To_Queue(&MDNS_Queue, data, (UNSIGNED)3,
                             (UNSIGNED)NU_NO_SUSPEND) == NU_SUCCESS)
        {
            /* Resume the task that can wake up the socket. */
            NU_Resume_Task(&NU_MDNS_Wake_Task_Ptr);
        }

        dns_ptr = DNS_Add_Host(device->dev_host_name, (CHAR*)ip_addr, MDNS_LOCAL_TTL,
                               family,
#if (INCLUDE_IPV4 == NU_TRUE)
#if (INCLUDE_IPV6 == NU_TRUE)
                               family == NU_FAMILY_IP ?
#endif
                               DNS_TYPE_A
#if (INCLUDE_IPV6 == NU_TRUE)
                               :
#endif
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
                               DNS_TYPE_AAAA
#endif
                               , 1, DNS_AUTHORITATIVE_RECORD, NU_NULL);

        if (dns_ptr)
        {
            /* Store the device index of the interface associated with this entry. */
            dns_ptr->mdns_dev_index = device->dev_index;

            /* Allocate memory for the timer. */
            status = NU_Allocate_Memory(MEM_Cached, (VOID **)&dns_ptr->mdns_timer,
                                        sizeof(NU_TIMER), NU_NO_SUSPEND);

            if (status == NU_SUCCESS)
            {
                /* Zero out the memory. */
                memset(dns_ptr->mdns_timer, 0, sizeof(NU_TIMER));

                /* Create the probing timer.  Zero is an invalid initial timeout, so
                 * ensure the timer can never be set to zero.
                 */
                status = NU_Create_Timer(dns_ptr->mdns_timer, "MDNS",
                                         MDNS_Event_Handler, dns_ptr->dns_id,
                                         (UTL_Rand() % MDNS_PROBE_DELAY) + 1, 0,
                                         NU_ENABLE_TIMER);
            }
        }

        else
        {
            status = NU_NO_MEMORY;
        }
    }

    if (dns_ptr)
    {
        /* A new IP address is being added to the interface.  Restart probing. */
        dns_ptr->mdns_retrans_count = 0;
        dns_ptr->mdns_state = MDNS_PROBING;
    }

    return (status);

} /* MDNS_Perform_Registration */

/****************************************************************************
*
*   FUNCTION
*
*       MDNS_Remove_Local_Host
*
*   DESCRIPTION
*
*       This function deletes the IP address from all local host records.
*       If this is the last IP address, the entire record is deleted.
*
*   INPUTS
*
*       *ip_addr                The IP address to remove.
*       family                  The family type of the IP address.
*
*   OUTPUTS
*
*       None.
*
******************************************************************************/
VOID MDNS_Remove_Local_Host(UINT8 *ip_addr, INT16 family)
{
    DNS_HOST    *dns_ptr, *next_ptr;
    INT         i, ip_idx;
    INT16       h_length;

    /* Obtain the semaphore. */
    if (NU_Obtain_Semaphore(&DNS_Resource, NU_NO_SUSPEND) == NU_SUCCESS)
    {
#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
        if (family == NU_FAMILY_IP6)
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
            h_length = IP6_ADDR_LEN;
#if (INCLUDE_IPV4 == NU_TRUE)
        else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
            h_length = IP_ADDR_LEN;
#endif

        /* Search for a matching host. */
        dns_ptr = DNS_Hosts.dns_head;

        while (dns_ptr)
        {
            /* Save a pointer to the next entry in case this entry is deleted. */
            next_ptr = dns_ptr->dns_next;

            /* Ensure the IP addresses are the same family. */
            if (dns_ptr->dns_h_length == h_length)
            {
                /* Loop through all the records, removing this IP address from the record. */
                for (i = 0, ip_idx = 0; i < DNS_MAX_IP_ADDRS; i++, ip_idx += MAX_ADDRESS_SIZE)
                {
                    /* Is this one we're looking for. */
                    if (memcmp((UINT8 *)&dns_ptr->dns_record_data[ip_idx], ip_addr,
                               (unsigned int)h_length) == 0)
                    {
                        /* If this is the last entry the list can hold, or the next entry
                         * in the list marks the end of the list, then this is the last valid
                         * IP address in the list.
                         */
                        if ( ((i + 1) == DNS_MAX_IP_ADDRS) ||
                             (IP_IS_NULL_ADDR(&dns_ptr->dns_record_data[ip_idx + MAX_ADDRESS_SIZE])) )
                        {
                            /* If this is the only entry in the list. */
                            if (i == 0)
                            {
                                /* Delete the entire record from the database. */
                                DNS_Delete_Host(dns_ptr);
                            }

                            else
                            {
                                /* Zero out this entry. */
                                memset(&dns_ptr->dns_record_data[ip_idx], 0,
                                       (unsigned int)MAX_ADDRESS_SIZE);
                            }
                        }

                        /* Otherwise, move the entries up in memory to fill in
                         * this deleted space.
                         */
                        else
                        {
                            memmove(&dns_ptr->dns_record_data[ip_idx],
                                    &dns_ptr->dns_record_data[ip_idx + MAX_ADDRESS_SIZE],
                                    (DNS_MAX_IP_ADDRS - i) * MAX_ADDRESS_SIZE);
                        }

                        break;
                    }
                }
            }

            /* Get a pointer to the next entry */
            dns_ptr = next_ptr;
        }

        NU_Release_Semaphore(&DNS_Resource);
    }

} /* MDNS_Remove_Local_Host */

/****************************************************************************
*
*   FUNCTION
*
*       MDNS_Find_Query_By_ID
*
*   DESCRIPTION
*
*       This function finds a matching query structure by ID.
*
*   INPUTS
*
*       id                      The ID of the target entry.
*
*   OUTPUTS
*
*       A pointer to the query structure or NU_NULL if no matching entry
*       exists.
*
******************************************************************************/
MDNS_QUERY *MDNS_Find_Query_By_ID(UINT32 id)
{
    MDNS_QUERY  *qry_ptr;

    qry_ptr = MDNS_Query_List.head;

    while (qry_ptr)
    {
        /* If the type and family match, check the data. */
        if (qry_ptr->mdns_id == id)
        {
            break;
        }

        /* Get the next entry. */
        qry_ptr = qry_ptr->mdns_next;
    }

    return (qry_ptr);

} /* MDNS_Find_Query_By_ID */

/****************************************************************************
*
*   FUNCTION
*
*       MDNS_Find_Matching_Query_By_Data
*
*   DESCRIPTION
*
*       This function finds a matching query structure matching the incoming
*       type and data.
*
*   INPUTS
*
*       type                    The type of the target entry.
*       *data                   The data of the target entry.
*       family                  The family of the query.
*
*   OUTPUTS
*
*       A pointer to the query structure or NU_NULL if no matching entry
*       exists.
*
******************************************************************************/
MDNS_QUERY *MDNS_Find_Matching_Query_By_Data(CHAR *data, INT type, INT16 family)
{
    MDNS_QUERY  *qry_ptr;
    INT         addr_len;

    qry_ptr = MDNS_Query_List.head;

    while (qry_ptr)
    {
        /* If the type and family match, check the data. */
        if (qry_ptr->mdns_type == type)
        {
            /* If the data matches. */
            if ( (type == DNS_TYPE_PTR) && (family != NU_FAMILY_UNSPEC) )
            {
#if (INCLUDE_IPV4 == NU_TRUE)
#if (INCLUDE_IPV6 == NU_TRUE)
                /* If this is an IPv4 address PTR record. */
                if (family == NU_FAMILY_IP)
#endif
                {
                    addr_len = IP_ADDR_LEN;
                }

#if (INCLUDE_IPV6 == NU_TRUE)
                /* If this is an IPv6 address PTR record. */
                else
#endif
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
                {
                    addr_len = IP6_ADDR_LEN;
                }
#endif

                /* If the addresses match. */
                if (memcmp(data, qry_ptr->mdns_data, addr_len) == 0)
                {
                    break;
                }
            }

            /* If the data fields match. */
            else if (NU_STRICMP(qry_ptr->mdns_data, data) == 0)
            {
                break;
            }
        }

        /* Get the next entry. */
        qry_ptr = qry_ptr->mdns_next;
    }

    return (qry_ptr);

} /* MDNS_Find_Matching_Query_By_Data */

/****************************************************************************
*
*   FUNCTION
*
*       MDNS_Find_Matching_Query_By_Name
*
*   DESCRIPTION
*
*       This function finds a matching query structure matching the incoming
*       type and name.
*
*   INPUTS
*
*       *name                   The name of the target entry.
*       type                    The type of the target entry.
*
*   OUTPUTS
*
*       A pointer to the query structure or NU_NULL if no matching entry
*       exists.
*
******************************************************************************/
MDNS_QUERY *MDNS_Find_Matching_Query_By_Name(CHAR *name, INT type)
{
    MDNS_QUERY  *qry_ptr;
    CHAR        ip_addr[MAX_ADDRESS_SIZE];
    INT         addr_len = 0;

    qry_ptr = MDNS_Query_List.head;

    while (qry_ptr)
    {
        /* If the type and family match, check the data. */
        if (qry_ptr->mdns_type == type)
        {
            /* If the data matches. */
            if (type == DNS_TYPE_PTR)
            {
#if (INCLUDE_IPV4 == NU_TRUE)
                /* If this is an IPv4 address PTR record. */
                if ( (strlen(name) >= 13) &&
                     (NU_STRICMP(&name[strlen(name) - 13], ".IN-ADDR.ARPA") == 0) )
                {
                    addr_len = IP_ADDR_LEN;

                    /* Extract the address from the name. */
                    DNS4_String_To_Addr(ip_addr, name);
                }

#if (INCLUDE_IPV6 == NU_TRUE)
                /* If this is an IPv6 address PTR record. */
                else
#endif
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
                if ( (strlen(name) >= 9) &&
                     (NU_STRICMP(&name[strlen(name) - 9], ".IP6.ARPA") == 0) )
                {
                    addr_len = IP6_ADDR_LEN;

                    /* Extract the address from the name. */
                    DNS6_String_To_Addr(ip_addr, name);
                }
#endif

                /* If the addresses match. */
                if ( (addr_len != 0) &&
                     (memcmp(ip_addr, qry_ptr->mdns_data, addr_len) == 0) )
                {
                    break;
                }
            }

            /* If this is an address look up, check if the host names match. */
            if ( (addr_len == 0) && (NU_STRICMP(qry_ptr->mdns_data, name) == 0) )
            {
                break;
            }
        }

        /* Get the next entry. */
        qry_ptr = qry_ptr->mdns_next;
    }

    return (qry_ptr);

} /* MDNS_Find_Matching_Query_By_Name */

/****************************************************************************
*
*   FUNCTION
*
*       MDNS_Add_Query
*
*   DESCRIPTION
*
*       This function creates a new query structure.
*
*   INPUTS
*
*       *data                   The data of the new entry.
*       type                    The type of the new entry.
*       family                  The family of the new entry.
*
*   OUTPUTS
*
*       A pointer to the query structure or NU_NULL if a new structure could
*       not be created.
*
******************************************************************************/
MDNS_QUERY *MDNS_Add_Query(CHAR *data, INT type, INT16 family)
{
    MDNS_QUERY  *qry_ptr = NU_NULL;
    INT         str_len;

    /* If this is a reverse-address mapped PTR record, determine the length of
     * the IP address.
     */
    if ( (type == DNS_TYPE_PTR) && (family != NU_FAMILY_UNSPEC) )
    {
#if (INCLUDE_IPV4 == NU_TRUE)
        if (family == NU_FAMILY_IP)
        {
            str_len = IP_ADDR_LEN;
        }
#if (INCLUDE_IPV6 == NU_TRUE)
        else
#endif
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
        {
            str_len = IP6_ADDR_LEN;
        }
#endif
    }

    /* This query does not hold an IP address as its data. */
    else
    {
        str_len = strlen(data) + 1;
    }

    /* Allocate a block of memory for the new query structure and for the
     * data.
     */
    if (NU_Allocate_Memory(MEM_Cached, (VOID **)&qry_ptr,
                           (UNSIGNED)((UNSIGNED)str_len + sizeof(MDNS_QUERY)),
                           NU_NO_SUSPEND) == NU_SUCCESS)
    {
        memset(qry_ptr, 0, str_len + sizeof(MDNS_QUERY));

        /* Ensure the ID is not zero. */
        qry_ptr->mdns_id = DNS_Id ++;

        /* Ensure the ID is not zero. */
        if (qry_ptr->mdns_id == 0)
        {
            qry_ptr->mdns_id = DNS_Id ++;
        }

        qry_ptr->mdns_type = type;
        qry_ptr->mdns_family = family;

        /* Set the pointer to memory where the data will be stored. */
        qry_ptr->mdns_data = (CHAR *)(qry_ptr + 1);

        /* Copy the data. */
        memcpy(qry_ptr->mdns_data, data, str_len);

        /* Null-terminate the string. */
        if (type != DNS_TYPE_PTR)
            qry_ptr->mdns_data[str_len] = 0;

        /* Initialize the callback list. */
        qry_ptr->mdns_callback.head = NU_NULL;
        qry_ptr->mdns_callback.tail = NU_NULL;

        /* Initialize the host list. */
        qry_ptr->mdns_host_list.dns_head = NU_NULL;
        qry_ptr->mdns_host_list.dns_head = NU_NULL;

        /* Create a new timer.  The initial expiration does not matter, but cannot
         * be 0 due to the routine's requirements.
         */
        NU_Create_Timer(&qry_ptr->mdns_qry_timer, "mDNSquery", MDNS_Query_Handler,
                        qry_ptr->mdns_id, 0xffffffff, 0, NU_DISABLE_TIMER);

        /* Add this entry to the list. */
        DLL_Enqueue(&MDNS_Query_List, qry_ptr);
    }

    return (qry_ptr);

} /* MDNS_Add_Query */

/*************************************************************************
*
*   FUNCTION
*
*       MDNS_Set_Device_Hostname
*
*   DESCRIPTION
*
*       This function changes the host name of the interface associated
*       with the respective interface index.  The host name was configured
*       initially when the interface was initialized using the metadata
*       option "hostname" and the interface index. The interface's host
*       name is used only when mDNS is enabled to claim ownership of the
*       A and AAAA records for the respective IP address(es) assigned to
*       the interface and to answer A, AAAA and PTR queries for the host
*       name. If the interface has already been configured with an IP
*       address(es) before this routine is invoked, the respective local
*       DNS record(s) will be updated with the new host name, and probing and
*       announcing will be initiated.
*
*   INPUTS
*
*       *name                   New host name or NU_NULL to clear the host
*                               name.
*       dev_index               The index of the interface being configured.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful completion of the service.
*       NU_INVALID_PARM         There is no interface with a matching
*                               index.
*
*       Otherwise, an operating-system specific error is returned.
*
*************************************************************************/
STATUS MDNS_Set_Device_Hostname(CHAR *name, UINT32 dev_index)
{
#if (INCLUDE_MDNS == NU_TRUE)
    DV_DEVICE_ENTRY     *dev_ptr;
    DNS_HOST            *l_host, *next_ptr;
    UINT8               ip_addr[IP_ADDR_LEN];
    DNS_SRV_DATA        extra_data;
#if (INCLUDE_IPV4 == NU_TRUE)
    DNS_HOST            *dnsv4_ptr;
    DEV_IF_ADDR_ENTRY   *current_v4addr;
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
    DNS_HOST            *dnsv6_ptr;
    DEV6_IF_ADDRESS     *current_v6addr;
#endif
#endif
    STATUS              status;

    /* Obtain the TCP semaphore. */
    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Get a pointer to the device structure. */
        dev_ptr = DEV_Get_Dev_By_Index(dev_index);

        if (dev_ptr)
        {
            /* Get the local host address records for this device. */
#if (INCLUDE_IPV6 == NU_TRUE)
            dnsv6_ptr = DNS_Find_Matching_Host_By_Name(dev_ptr->dev_host_name,
                                                       NU_NULL, DNS_TYPE_AAAA);
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
            dnsv4_ptr = DNS_Find_Matching_Host_By_Name(dev_ptr->dev_host_name,
                                                       NU_NULL, DNS_TYPE_A);
#endif

            /* Reconfigure the local DNS host entries for this device. */
            if (name)
            {
                /* If there is a name currently configured on the interface. */
                if (dev_ptr->dev_host_name)
                {
                    /* Search through the DNS host list for SRV records that use this
                     * host name in the data field.
                     */
                    for (l_host = DNS_Hosts.dns_head; l_host; l_host = next_ptr)
                    {
                        /* Save a pointer to the next entry. */
                        next_ptr = l_host->dns_next;

                        /* If the record matches, and it is not in the process of being
                         * deleted, update it.
                         */
                        if ( (l_host->dns_type == DNS_TYPE_SRV) && (l_host->dns_ttl != 0) &&
                             (NU_STRICMP((const char *)l_host->dns_record_data,
                                         dev_ptr->dev_host_name) == 0) )
                        {
                            /* Set up the extra data parameters. */
                            extra_data.dns_srv_prio = l_host->dns_prio;
                            extra_data.dns_srv_weight = l_host->dns_weight;
                            extra_data.dns_srv_port = l_host->dns_port;

                            /* Add the new host entry. */
                            DNS_Add_Host(l_host->dns_name, name, DNS_SD_DEFAULT_TTL,
                                         l_host->dns_family, DNS_TYPE_SRV, strlen(name),
                                         DNS_LOCAL_RECORD | DNS_PERMANENT_ENTRY,
                                         (VOID*)&extra_data);

                            /* Remove the old entry.  Always delete the old entry instead
                             * of trying to update it with the new data so a goodbye
                             * packet is sent for this record.
                             */
                            DNS_Delete_Host(l_host);
                        }
                    }

                    /* Deallocate the memory for the interface's host name. */
                    NU_Deallocate_Memory(dev_ptr->dev_host_name);

                    dev_ptr->dev_host_name = NU_NULL;
                }

                /* Set the host name to the memory passed into the routine. */
                dev_ptr->dev_host_name = name;

#if (INCLUDE_IPV4 == NU_TRUE)
                if (dnsv4_ptr)
                {
                    /* Remove the old entry. */
                    DNS_Delete_Host(dnsv4_ptr);
                }

                current_v4addr = dev_ptr->dev_addr.dev_addr_list.dv_head;

                /* Add each IP address to the new local DNS entry. */
                while (current_v4addr)
                {
                    /* If this is not an empty entry awaiting DHCP resolution. */
                    if (current_v4addr->dev_entry_ip_addr != 0)
                    {
                        PUT32(ip_addr, 0, current_v4addr->dev_entry_ip_addr);

                        /* Register this address. */
                        MDNS_Perform_Registration(dev_ptr, ip_addr, NU_FAMILY_IP);
                    }

                    current_v4addr = current_v4addr->dev_entry_next;
                }
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
                if (dnsv6_ptr)
                {
                    /* Remove the old entry. */
                    DNS_Delete_Host(dnsv6_ptr);
                }

                current_v6addr = dev_ptr->dev6_addr_list.dv_head;

                /* Add each IP address to the new local DNS entry. */
                while (current_v6addr)
                {
                    /* Register this address. */
                    MDNS_Perform_Registration(dev_ptr, current_v6addr->dev6_ip_addr,
                                              NU_FAMILY_IP6);

                    current_v6addr = current_v6addr->dev6_next;
                }
#endif
            }

            /* The name is being deleted. */
            else if (dev_ptr->dev_host_name)
            {
                /* Search through the DNS host list for SRV records that use this
                 * host name in the data field.
                 */
                for (l_host = DNS_Hosts.dns_head; l_host; l_host = next_ptr)
                {
                    next_ptr = l_host->dns_next;

                    /* If the record matches, and it is not in the process of being
                     * deleted, delete it.
                     */
                    if ( (l_host->dns_type == DNS_TYPE_SRV) && (l_host->dns_ttl != 0) &&
                         (NU_STRICMP((const char *)l_host->dns_record_data,
                                     dev_ptr->dev_host_name) == 0) )
                    {
                        /* Remove the old entry. */
                        DNS_Delete_Host(l_host);
                    }
                }

                /* Delete the records. */
#if (INCLUDE_IPV6 == NU_TRUE)
                if (dnsv6_ptr)
                {
                    DNS_Delete_Host(dnsv6_ptr);
                }
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
                if (dnsv4_ptr)
                {
                    DNS_Delete_Host(dnsv4_ptr);
                }
#endif

                /* Deallocate the memory for the interface's host name. */
                NU_Deallocate_Memory(dev_ptr->dev_host_name);

                dev_ptr->dev_host_name = NU_NULL;
            }
        }

        else
        {
            status = NU_INVALID_PARM;
        }

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release TCP semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    return (status);

} /* MDNS_Set_Device_Hostname */

#endif

/*************************************************************************
*
*   FUNCTION
*
*       NU_Set_MDNS_Hostname_Callback
*
*   DESCRIPTION
*
*       This API routine sets the callback routine that will be used to
*       create host names for interfaces used in mDNS upon registration
*       of a new interface or upon conflict detection.
*
*       Note that the routine registered to set the host name must use
*       dynamically allocated memory when returning the host name to
*       the caller.  This memory will be used for the new host name
*       and must not be deallocated by the application.  The mDNS
*       module will deallocate the memory when the interface is
*       removed or the corresponding interface records deleted.
*
*   INPUTS
*
*       *set_hostname           A pointer to the callback routine to
*                               invoke whenever the mDNS module needs
*                               to create a host name for a registered
*                               interface.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful completion of the service.
*       NU_INVALID_PARM         The input parameter is invalid.
*
*       Otherwise, an operating-system specific error is returned.
*
*************************************************************************/
STATUS NU_Set_MDNS_Hostname_Callback(CHAR* (set_hostname(UINT32 int_index,
                                     BOOLEAN conflict)))
{
    STATUS      status;

#if (INCLUDE_MDNS == NU_TRUE)

    if (set_hostname)
    {
        status = NU_Obtain_Semaphore(&MDNS_Resource, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Set the callback routine. */
            MDNS_Create_Hostname_Callback = set_hostname;

            NU_Release_Semaphore(&MDNS_Resource);
        }
    }

    else
    {
        status = NU_INVALID_PARM;
    }

#else

    /* mDNS must be included to configure the callback routine. */
    status = NU_INVALID_PARM;

#endif

    return (status);

} /* NU_Set_MDNS_Hostname_Callback */
