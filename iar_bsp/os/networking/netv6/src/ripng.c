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
*       ripng.c                                      
*
*   DESCRIPTION
*
*       Routing Information Protocol for IPv6 per RFC 2080
*
*   DATA STRUCTURES
*
*       RIPng_List
*       RIPng_Socket
*
*   FUNCTIONS
*
*       NU_Ripng_Initialize
*       RIPng_Task_Entry
*       RIPng_Update_Table
*       RIPng_Find_Entry
*       RIPng_Delete_Old
*       RIPng_Gather_Old
*       RIPng_Gather_All
*       RIPng_Send_Updates
*       RIPng_Send_Table
*       RIPng_Broadcast
*       RIPng_Send_Routes
*       RIPng_Request_Routing_Table
*
*   DEPENDENCIES
*
*       nu_net.h
*       nu_net6.h
*
*************************************************************************/

#include "networking/nu_net.h"
#include "networking/nu_net6.h"

#if (INCLUDE_UDP == NU_FALSE)
#error UDP must be included in order to use RIPng.  If you do not wish to use RIPng, remove the file from the build.
#endif

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)
/* Collect RIPNG_MAX_DELETES old routes for deletion at any one time. */
static  RTAB6_ROUTE_ENTRY    *RIPng_delete_nodes[RIPNG_MAX_DELETES];
static  RTAB6_ROUTE_ENTRY    *RIPng_updated_nodes[RIPNG_MAX_PER_PACKET];

static  NU_TASK              RIPng_Tcb;

static  INT         RIPng_Socket;       /* Send or receive socket for broadcasts */
static  INT         RIPng_Delete_Num = 0;
static  CHAR        *RIPng_Buffer;

static  UINT8       RIPng_Multi_Addr[IP6_ADDR_LEN] = 
                        {0xff, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9};

/*  Local prototypes.  */
STATIC  VOID            RIPng_Task_Entry(UNSIGNED, VOID *);
STATIC  VOID            RIPng_Broadcast(INT16);
STATIC  VOID            RIPng_Request_Routing_Table(const RIPNG_LIST_NODE *);
STATIC  VOID            RIPng_Gather_Old(UINT32, INT16);
STATIC  INT             RIPng_Delete_Old(VOID);
STATIC  VOID            RIPng_Gather_All(const UINT32 , const UINT8 *, 
                                         UINT8 *, UINT8, INT16);
STATIC  VOID            RIPng_Send_Table(const RIPNG_LIST_NODE *, 
                                         const struct addr_struct *);
STATIC  VOID            RIPng_Send_Updates(RIPNG_HEADER *, INT, 
                                           const struct addr_struct *);
STATIC RIPNG_LIST_NODE  *RIPng_Find_Entry(UINT32);
STATIC  INT             RIPng_Update_Table(const RIPNG_LIST_NODE *, 
                                           RIPNG_HEADER *, INT, 
                        const struct addr_struct *);
STATIC  VOID            RIPng_Send_Routes(const RIPNG_LIST_NODE *, INT16,
                                          const struct addr_struct *);
#endif

RIPNG_LIST_STRUCT    RIPng_List;

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)
static  INT16        RIPng_Triggered_Update;
#endif

/*************************************************************************
*
*   FUNCTION
*
*       NU_Ripng_Initialize
*
*   DESCRIPTION
*
*       This function initializes the RIPng module.
*
*   INPUTS
*
*       *ripng                  A pointer to the beginning node.
*       num                     The number of nodes in the array.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful
*       NU_INVALID_PARM         ripng is NU_NULL, num is less than or 
*                               equal to 0 or multicasting support
*                               is not included.
*       NU_MEM_ALLOC            Memory allocation failed
*       NU_INVAL                Generic invalid return
*
*************************************************************************/
STATUS NU_Ripng_Initialize(RIPNG_STRUCT *ripng, INT num)
{
#if (INCLUDE_IP_MULTICASTING == NU_TRUE)
    STATUS              status;
    VOID                *pointer;
    IP6_MREQ            mgroup;
    DV_DEVICE_ENTRY     *dev_ptr;
    struct addr_struct  bcastaddr;
    INT                 i;
    RIPNG_LIST_NODE     *node;
#endif

    NU_SUPERV_USER_VARIABLES

    if ( (ripng == NU_NULL) || (num <= 0) )
        return (NU_INVALID_PARM);

    /* Switch to supervisor mode */    
    NU_SUPERVISOR_MODE();

#if (INCLUDE_IP_MULTICASTING == NU_FALSE)

    NLOG_Error_Log("RIPng Send and Receive mode requires multicasting support", 
                   NERR_FATAL, __FILE__, __LINE__);

    NU_USER_MODE();

    return (NU_INVALID_PARM);

#else

    /* Setup the socket that will be used by RIPng to send and receive. */   
    RIPng_Socket = NU_Socket(NU_FAMILY_IP6, NU_TYPE_DGRAM, 0);

    if (RIPng_Socket < 0)
    {
        NLOG_Error_Log("NU_Ripng_Initialize could not create socket", 
                       NERR_FATAL, __FILE__, __LINE__);
        
        /* Return to user mode */
        NU_USER_MODE();
        
        return (NU_INVAL);
    }

    /* Set the socket option to receive the Hop Limit for packets received
     * on this socket.
     */
    if (NU_Setsockopt_IPV6_RECVHOPLIMIT(RIPng_Socket, 1) != NU_SUCCESS)
        NLOG_Error_Log("NU_Ripng_Initialize could not set socket option to receive Hop Limit of incoming packets", 
                       NERR_SEVERE, __FILE__, __LINE__);

    /* Set the socket option to receive the interface index of the interface
     * on which a packet is received.
     */
    if (NU_Setsockopt_IPV6_RECVPKTINFO(RIPng_Socket, 1) != NU_SUCCESS)
        NLOG_Error_Log("NU_Ripng_Initialize could not set socket option to receive interface index of incoming packets", 
                       NERR_SEVERE, __FILE__, __LINE__);

    /* Set the socket option to set the multicast Hop Limit to 255. */
    if (NU_Setsockopt_IPV6_MULTICAST_HOPS(RIPng_Socket, 255) != NU_SUCCESS)
        NLOG_Error_Log("NU_Ripng_Initialize could not set socket option for multicast hop limit", 
                       NERR_SEVERE, __FILE__, __LINE__);
   
    /* Set up the listening address */
    bcastaddr.id = IP6_ADDR_ANY;
    bcastaddr.name = "RIPng";
    bcastaddr.family = NU_FAMILY_IP6;
    bcastaddr.port = RIPNG_PORT;

    /* Bind to the RIPng Port */
    status = NU_Bind(RIPng_Socket, &bcastaddr, 0);

    if (status < 0)
    {
        NLOG_Error_Log("NU_Ripng_Initialize NU_Bind failed", NERR_FATAL, 
                       __FILE__, __LINE__);
        
        /* Return to user mode */
        NU_USER_MODE();
        
        return (NU_INVAL);
    }
  
    /* Create the list of devices and their configurations to be used with
     * RIPng. 
     */
    status = NU_Allocate_Memory(MEM_Cached, (VOID**)&node, 
                                (UNSIGNED)((UNSIGNED)num * sizeof(RIPNG_LIST_NODE)), 
                                (UNSIGNED)NU_NO_SUSPEND);

    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("NU_RIPng_Initialize no memory for list of devices", 
                       NERR_FATAL, __FILE__, __LINE__);

        /* Return to user mode */
        NU_USER_MODE();
        
        return (NU_MEM_ALLOC);
    }

    /* Initialize the triggered update flag */
    RIPng_Triggered_Update = 0;

    /* Setup the multicast group structure.  Only copy the multicast address
     * into the multicast group structure once since the same group will
     * be joined for each interface on which RIPng is enabled.
     */
    NU_BLOCK_COPY(mgroup.ip6_mreq_multiaddr, RIPng_Multi_Addr, IP6_ADDR_LEN);
    
    for (i = 0; i < num; i++)
    {
        /* Grab the semaphore so we can get a pointer to the device */
        status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log("Unable to obtain TCP semaphore", NERR_RECOVERABLE, 
                           __FILE__, __LINE__);

            /* Return to user mode */
            NU_USER_MODE();

            return (status);
        }

        /* Look up the device the user wishes to use RIPng with. */
        dev_ptr = DEV_Get_Dev_By_Name(ripng[i].ripng_device_name);
        
        /* Check that the device exists and is IPv6-Enabled */
        if ( (dev_ptr == NU_NULL) || (!(dev_ptr->dev_flags & DV6_IPV6)) )
        {
            NLOG_Error_Log("NU_RIPng_Initialize device invalid", 
                           NERR_FATAL, __FILE__, __LINE__);

            /* Set an error status for this interface */
            ripng[i].ripng_status = NU_INVALID_PARM;

            /* Release the TCP semaphore */
            if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE, 
                               __FILE__, __LINE__);

            continue;
        }

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)

        /* Set the interface index of the multicast group structure to the 
         * interface index of the interface on which to join the multicast
         * group.
         */
        mgroup.ip6_mreq_dev_index = dev_ptr->dev_index;

        /* Extract the necessary parameters from the device since we are
         * about to release the semaphore.
         */
        node[i].ripng_device.ripng_dev_index = dev_ptr->dev_index;
        node[i].ripng_device.ripng_dev_mtu = dev_ptr->dev6_link_mtu;

        /* If the user specified a new metric then use it. Otherwise continue
         * to use the default metric associated with this device. 
         */
        if (ripng[i].ripng_metric)
            node[i].ripng_device.ripng_dev_metric = ripng[i].ripng_metric;
        else
            node[i].ripng_device.ripng_dev_metric = (UINT8)dev_ptr->dev_metric;

        /* Release the TCP semaphore */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE, 
                           __FILE__, __LINE__);

        /* Join the All-RIP-Routers multicast group on the interface. */
        if (NU_Setsockopt_IPV6_JOIN_GROUP(RIPng_Socket, &mgroup) != NU_SUCCESS)
        {
            NLOG_Error_Log("NU_RIPng_Initialize could not join multicast group", 
                           NERR_RECOVERABLE, __FILE__, __LINE__);

            /* Set an error status for this interface */
            ripng[i].ripng_status = NU_INVAL;

            continue;
        }

#endif

        /* Indicate that the interface was successfully initialized for RIPng */
        ripng[i].ripng_status = NU_SUCCESS;
       
        /* Add this one to the list. */
        DLL_Enqueue(&RIPng_List, &node[i]);
    }
    
    /* If at least one valid IPv6-enabled interface was passed in */
    if (RIPng_List.ripng_head)
    {
        /* Create RIPng task */
        status = NU_Allocate_Memory(MEM_Cached, &pointer, 
                                    (UNSIGNED)RIPNG_STACK_SIZE,
                                    (UNSIGNED)NU_NO_SUSPEND);

        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log("NU_RIPng_Initialize no memory for RIPng task", 
                           NERR_FATAL, __FILE__, __LINE__);

            /* Return to user mode */
            NU_USER_MODE();
        
            return (NU_MEM_ALLOC);
        }
    
        pointer = TLS_Normalize_Ptr(pointer);
       
        status = NU_Create_Task(&RIPng_Tcb, "RIPngTASK", RIPng_Task_Entry,
                                (UNSIGNED)0, NU_NULL, pointer,
                                (UNSIGNED)RIPNG_STACK_SIZE, RIPNG_PRIORITY, 
                                (UNSIGNED)RIPNG_TIME_SLICE, RIPNG_PREEMPT, 
                                 NU_NO_START);
    
        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log("NU_RIPng_Initialize could not create task", 
                           NERR_FATAL, __FILE__, __LINE__);  

            /* Return to user mode */
            NU_USER_MODE();
        
            return (NU_INVAL);
        }

        /* Start the RIPng task */
        status = NU_Resume_Task(&RIPng_Tcb);
    }

    /* Return an error - no interfaces were valid */
    else
        status = NU_INVALID_PARM;

    /* Return to user mode */
    NU_USER_MODE();
    
    return (status);

#endif
    
} /* NU_RIPng_Initialize */

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)

/*************************************************************************
*
*   FUNCTION
*
*       RIPng_Task_Entry
*
*   DESCRIPTION
*
*       The RIPng task.  This task processes incoming RIPng packets and
*       handles the garbage collection and deletion of Routing Table
*       entries.
*
*   INPUTS
*
*       argc                    Unused
*       *argv                   Unused
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
STATIC VOID RIPng_Task_Entry(UNSIGNED argc, VOID *argv)
{
    struct addr_struct  fromaddr;
    UINT32              curtime, garbage_time = 0,
                        triggered_update = 0;
    UINT32              del_time;
    STATUS              status;
    SIGNED              bytes_received;
    UINT32              wtime, broadcast_timer = 0;
    INT                 entry_count;
    RIPNG_HEADER        *rpkt;
    FD_SET              readfs;
    RIPNG_LIST_NODE     *node;
    CHAR                RIPng_buf[RIPNG_BUFFER_SIZE];
    msghdr              msg;
    cmsghdr             *cmsg;
    INT                 hop_limit;
    INT                 *hop_limit_ptr;
    in6_pktinfo         *pktinfo_ptr = NU_NULL;

    NU_SUPERV_USER_VARIABLES

    UNUSED_PARAMETER(argc);
    UNUSED_PARAMETER(argv);

    /* Switch to supervisor mode */    
    NU_SUPERVISOR_MODE();

    /* Setup the buffer */
    RIPng_Buffer = RIPng_buf;  
   
    /* RFC 2080 - section 2.4.1 - When a router first comes up, it 
     * multicasts a Request on every connected network asking for a
     * complete routing table.
     */
    for (node = RIPng_List.ripng_head; node; node = node->ripng_next)
        RIPng_Request_Routing_Table(node);

    /* Initialize the next time the Routing Table should be checked for
     * expired entries.
     */
    del_time = NU_Retrieve_Clock() + RIPNG_DELETE_INTERVAL;

    wtime = 0;

    /* Set up the pointer to the client data structure */
    msg.msg_name = &fromaddr;
    msg.msg_namelen = sizeof(fromaddr);

    /* Set up the pointer to the buffer to use for receiving */
    msg.msg_iov = RIPng_Buffer;
    msg.msg_iovlen = RIPNG_BUFFER_SIZE;

    /* Set the length of the ancillary data to hold the in6_pktinfo
     * data structure and the hop limit. 
     */
    msg.msg_controllen = 
        CMSG_SPACE(sizeof(INT)) + CMSG_SPACE(sizeof(in6_pktinfo));

    /* Allocate memory for the two pieces of ancillary data */
    if (NU_Allocate_Memory(&System_Memory, (VOID**)&msg.msg_control,
                           msg.msg_controllen, NU_NO_SUSPEND) != NU_SUCCESS)
        NLOG_Error_Log("RIPng_Task_Entry could not allocate memory for message block", 
                       NERR_FATAL, __FILE__, __LINE__);

    else
    {
        memset(msg.msg_control, 0, msg.msg_controllen);

        for (;;)
        {
            /* If the last broadcast was done as a result of the 30 second
             * broadcast timer expiring, reset the broadcast timer.
             */
            if (wtime == 0)
                broadcast_timer = NU_Retrieve_Clock() + RIPNG_BTIME;

            /* Continue waiting for data until it is time to send out a 
             * broadcast on the link or a Triggered Update has been set.
             */
            do
            {
                /* Reset wtime to the amount of time left to wait until a 
                 * Broadcast needs to be sent. 
                 */
                wtime = TQ_Check_Duetime(broadcast_timer);

                /* If it is not time to send a Broadcast, listen for an
                 * incoming packet on the RIPng port.
                 */
                if (wtime)
                {
                    NU_FD_Init(&readfs);
                    NU_FD_Set(RIPng_Socket, &readfs);

                    /* The first parameter in this call is normally NSOCKETS. This is an 
                     * optimization so that the select call does not search the entire
                     * socket list. Instead it will only search through the sockets up to 
                     * the RIPng_Socket. The RIPng_Socket should be among the first created. 
                     */
                    status = NU_Select((RIPng_Socket + 1), &readfs, NU_NULL, NU_NULL, 
                                       (UNSIGNED)wtime);

                    if (status == NU_NO_DATA)
                        continue;
        
                    /* Check for data on the socket */
                    if (NU_FD_Check(RIPng_Socket, &readfs) == NU_FALSE)
                        continue;
        
                    /* If the select returns NU_SUCCESS then a packet is 
                     * waiting to be read on the socket.
                     */

                    /* Retrieve the data pending on the socket. */
                    bytes_received = NU_Recvmsg(RIPng_Socket, &msg, 0);

                    /* Process the packet */
                    if (bytes_received >= 0)
                    {
                        rpkt = (RIPNG_HEADER*)RIPng_Buffer;

                        /* remove header size */
                        bytes_received -= RIPNG_HEADER_LEN; 

                        /* Determine the number of RTEs in the packet */
                        entry_count = ((INT)bytes_received / RIPNG_RTE_LEN);

                        /* Check that there is not extra garbage in the packet */
                        if ( (entry_count * RIPNG_RTE_LEN) != bytes_received)
                        {
                            NLOG_Error_Log("RIPng_Task_Entry received invalid sized packet", 
                                           NERR_FATAL, __FILE__, __LINE__);

                            continue;
                        }

                        /* Search through the ancillary data returned for the in6_pktinfo
                         * structure which contains the interface index of the interface on
                         * which the packet was received.
                         */
                        for (cmsg = CMSG_FIRSTHDR(&msg);
                             cmsg != NU_NULL;
                             cmsg = CMSG_NXTHDR(&msg, cmsg))
                        {
                            /* If this is the in6_pktinfo structure */
                            if ( (cmsg->cmsg_level == IPPROTO_IPV6) &&
                                 (cmsg->cmsg_type == IPV6_PKTINFO) )
                            {
                                /* Extract the interface index from the buffer */
                                pktinfo_ptr = (in6_pktinfo*)CMSG_DATA(cmsg);
                                break;
                            }
                        }

                        /* If the stack did not return an in6_pktinfo pointer, log 
                         * an error and go to the top of the loop.
                         */
                        if (!pktinfo_ptr)
                        {
                            NLOG_Error_Log("RIPng_Task_Entry no in6_pktinfo ancillary data returned", 
                                           NERR_SEVERE, __FILE__, __LINE__);
                            continue;
                        }

                        /* Check that this packet came from a router on one of the 
                         * directly connected networks.
                         */
                        node = RIPng_Find_Entry(pktinfo_ptr->ipi6_ifindex);

                        /* If the interface on which this packet was received is
                         * not RIP-enabled or the version of RIPng is not supported,
                         * discard the packet.
                         */
                        if ( (node == NU_NULL) || (rpkt->version != RIPNG_VERSION1) )
                        {
                            NLOG_Error_Log("RIPng_Task_Entry rejected packet", 
                                           NERR_INFORMATIONAL, __FILE__, __LINE__);

                            continue;
                        }

                        /* Process a RIPng request */
                        if (rpkt->command == RIPNG_REQUEST)
                        {
                            /* RFC 2080 - section 2.4.1 - If there is exactly one entry 
                             * in the request, and it has a destination prefix of zero, a
                             * prefix length of zero, and a metric of infinity (i.e., 16), 
                             * then this is a request to send the entire routing table.
                             */
                            if ( (entry_count == 1) && 
                                 (IPV6_IS_ADDR_UNSPECIFIED(rpkt->ripng_prefix)) &&
                                 (rpkt->ripng_prefix_len == 0) )
                            {
                                if (rpkt->ripng_metric == RT_INFINITY)
                                    RIPng_Send_Table(node, &fromaddr);

                                else
                                    NLOG_Error_Log("RIPng_Task_Entry received invalid metric in request for routing table", 
                                                   NERR_INFORMATIONAL, __FILE__, __LINE__);
                            }

                            /* RFC 2080 - section 2.4.1 - ... the router responds 
                             * directly to the requester's address and port with 
                             * a globally valid source address since the requester 
                             * may not reside on the directly attached network.
                             */
                            else
                                RIPng_Send_Updates(rpkt, entry_count, &fromaddr);
                        }

                        /* Process a RIP response */
                        else if (rpkt->command == RIPNG_RESPONSE)
                        {
                            /* RFC 2080 - section 2.4.2 - The Response must be 
                             * ignored if it is not from the RIPng port.  The
                             * datagram's IPv6 source address should be checked 
                             * to see whether the datagram is from a valid neighbor; 
                             * the source of the datagram must be a link-local address.  
                             * It is also worth checking to see whether the response 
                             * is from one of the router's own addresses.
                             */
                            if ( (fromaddr.port == RIPNG_PORT) &&
                                 (IPV6_IS_ADDR_LINKLOCAL(fromaddr.id.is_ip_addrs)) &&
                                 (!(DEV6_Get_Dev_By_Addr(fromaddr.id.is_ip_addrs))) )
                            {
                                /* Initialize hop_limit to the valid value */
                                hop_limit = 255;

                                /* RFC 2080 - section 2.4.2 - As an additional check, 
                                 * periodic advertisements must have their hop counts
                                 * set to 255, and inbound, multicast packets sent from 
                                 * the RIPng port (i.e. periodic advertisement or 
                                 * triggered update packets) must be examined to ensure 
                                 * that the hop count is 255.
                                 */
                                if (IPV6_IS_ADDR_MULTICAST(pktinfo_ptr->ipi6_addr))
                                {
                                    for (cmsg = CMSG_FIRSTHDR(&msg);
                                         cmsg != NU_NULL;
                                         cmsg = CMSG_NXTHDR(&msg, cmsg))
                                    {
                                        /* If this is the hop limit structure */
                                        if ( (cmsg->cmsg_level == IPPROTO_IPV6) &&
                                             (cmsg->cmsg_type == IPV6_HOPLIMIT) )
                                        {
                                            /* Extract the hop limit from the buffer */
                                            hop_limit_ptr = (INT*)CMSG_DATA(cmsg);
                                            hop_limit = *hop_limit_ptr;

                                            break;
                                        }
                                    }
                                }

                                if (hop_limit == 255)
                                {
                                    /* Update the Routing Table with the RTEs in the
                                     * update packet.
                                     */
                                    if (RIPng_Update_Table(node, rpkt, entry_count, &fromaddr))
                                        NLOG_Error_Log("RIPng_Task_Entry error at call to update table", 
                                                       NERR_INFORMATIONAL, __FILE__, __LINE__);
                                }

                                else
                                    NLOG_Error_Log("RIPng_Task_Entry received periodic update with hop limit not 255", 
                                                   NERR_INFORMATIONAL, __FILE__, __LINE__);
                            }

                        } /* else if */

                        else
                        {
                            NLOG_Error_Log("RIPng_Task_Entry received invalid command", 
                                           NERR_INFORMATIONAL, __FILE__, __LINE__);
                        }

                    } /* process the packet */

                    /* We got an error */
                    else
                        NLOG_Error_Log("RIPng_Task_Entry NU_Recv_From error", 
                                       NERR_FATAL, __FILE__, __LINE__);

                } /* if (wtime) */

            } while ( (wtime) && (RIPng_Triggered_Update == 0) );

            curtime = NU_Retrieve_Clock();

            /* If we broke out of the loop to send a Triggered Update, do not
             * bother checking the delete and garbage collection timers. 
             */
            if (!RIPng_Triggered_Update)
            {
                /* RFC 2080 - section 2.3 - Every 30 seconds, the RIP process is 
                 * awakened to send an unsolicited Response message containing 
                 * the complete routing table to every neighboring router.
                 */        
                RIPng_Broadcast(0);

                /* RFC 2080 - section 2.3 - If 180 seconds elapse from the last 
                 * time the timeout was initialized, the route is considered to 
                 * have expired, and the deletion process begins for that route.
                 */
                if (TQ_Check_Duetime(del_time) == 0)
                {
                    del_time = curtime + RIPNG_DELETE_INTERVAL;

                    /* This function will only gather RIPNG_MAX_DELETES per call. */
                    RIPng_Delete_Num = 0;

                    /* Gather the old entries from the routing table */
                    RIPng_Gather_Old(curtime, 1);

                    /* If entries were found to be deleted and the garbage collection
                     * timer is not already running.
                     */
                    if (RIPng_Delete_Num)
                    {
                        /* Set the garbage collection timer to 120 seconds */
                        garbage_time = curtime + RIPNG_GARBAGE_COLLECTION_INTERVAL;

                        /* Set the flag to send a Triggered Update */
                        RIPng_Triggered_Update = 1;
                    }
                }

                /* RFC 2080 - section 2.3 - Upon expiration of the 
                 * garbage-collection timer, the route is finally removed from 
                 * the routing table.
                 */
                if ( (garbage_time) && (TQ_Check_Duetime(garbage_time) == 0) )
                {
                    RIPng_Delete_Old();

                    garbage_time = 0;
                }
            }

            /* If a Triggered Update has been invoked from a received packet or
             * by the deletion process.
             */
            if (RIPng_Triggered_Update)
            {
                /* Triggered updates must be limited to once every 1-5 
                 * seconds. 
                 */
                if (((curtime - triggered_update) * SCK_Ticks_Per_Second) >= 1)
                {
                    triggered_update = curtime;

                    /* Send a Triggered Update */
                    RIPng_Broadcast(1);

                    /* Reset the Triggered Update flag */
                    RIPng_Triggered_Update = 0;
                }
            }
        }
    }

    if (NU_Close_Socket(RIPng_Socket) != NU_SUCCESS)
    {
        NLOG_Error_Log("RIPng_Task_Entry could not close socket", 
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }
    
    NU_USER_MODE();

} /* RIPng_Task_Entry */

/*************************************************************************
*
*   FUNCTION
*
*       RIPng_Update_Table
*
*   DESCRIPTION
*
*       Update the routing table with routes received from other routers.
*
*   INPUTS
*
*       *node                   Pointer to the node on the RIPng list 
*                               containing the pertinent parameters
*                               of the interface on which this packet
*                               was received.
*       *rpkt                   Pointer to the RIPng packet received.
*       pkt_cnt                 The number of RTEs in the packet.
*       *fromaddr               A pointer to the source address of the
*                               node from which this packet was 
*                               received.
*
*   OUTPUTS
*
*       NU_SUCCESS              Success
*
*************************************************************************/
STATIC INT RIPng_Update_Table(const RIPNG_LIST_NODE *node, RIPNG_HEADER *rpkt, 
                             INT pkt_cnt, const struct addr_struct *fromaddr)
{
    INT                 i;
    INT                 ret;
    UINT8               next_hop[IP6_ADDR_LEN];
    RTAB6_ROUTE_ENTRY   *current_rt_entry;
    RIPNG_ENTRY         *ep;
    UPDATED_ROUTE_NODE  updated_route;
    UINT32              metric;
    UINT16              route_tag;
       
    /* Get a pointer to the first RTE in the packet */
    ep = (RIPNG_ENTRY*)&rpkt->ripng_prefix;

    /* Initialize the next-hop to the sender's address.  Unless a
     * Next-Hop RTE is encountered, use the sender as the next-hop
     * for all destination RTE's.
     */
    NU_BLOCK_COPY(next_hop, fromaddr->id.is_ip_addrs, IP6_ADDR_LEN);
 
    /* Process each RTE in the packet */
    for (i = 0; i < pkt_cnt; i++)
    {
        /* RFC 2080 - section 2.4.2 - Is the destination prefix valid 
         * (e.g., not a multicast prefix and not a link-local address).  
         * A link-local address should never be present in an RTE.
         * Is the prefix length valid (i.e., between 0 and 128, inclusive).
         * Is the metric valid (i.e., between 1 and 16, inclusive).
         */
        if ( (IPV6_IS_ADDR_LINKLOCAL(ep->ripng_prefix)) || 
             (IPV6_IS_ADDR_MULTICAST(ep->ripng_prefix)) || 
             (ep->ripng_prefix_len > 128) ||
             ((ep->ripng_metric < 1) || (ep->ripng_metric > RT_INFINITY)) )
        {
            NLOG_Error_Log("RIPng_Update_Table received invalid RTE", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);

            /* RFC 2080 - section 2.4.2 - If any check fails, ignore 
             * that entry and proceed to the next. Again, logging the 
             * error is probably a good idea.
             */
            ep++;
            continue;
        }

        /* RFC 2080 - section 2.1.1 - A next hop RTE is identified by 
         * a value of 0xFF in the metric field of an RTE.
         */
        if (ep->ripng_metric == RIPNG_NEXTHOP_METRIC)
        {
            /* RFC 2080 - section 2.1.1 - Specifying a value of 
             * 0:0:0:0:0:0:0:0 in the prefix field of a next hop RTE 
             * indicates that the next hop address should be the 
             * originator of the RIPng advertisement.  If the received 
             * next hop address is not a link-local address, it should 
             * be treated as 0:0:0:0:0:0:0:0.
             */
            if ( (IPV6_IS_ADDR_UNSPECIFIED(ep->ripng_prefix)) ||
                 (!(IPV6_IS_ADDR_LINKLOCAL(ep->ripng_prefix))) )
                NU_BLOCK_COPY(next_hop, fromaddr->id.is_ip_addrs, 
                              IP6_ADDR_LEN);

            /* RFC 2080 - section 2.1.1 - The prefix field specifies 
             * the IPv6 address of the next hop.
             */
            else
                NU_BLOCK_COPY(next_hop, ep->ripng_prefix, IP6_ADDR_LEN);

            /* Process the next RTE */
            ep++;
            continue;
        }

        /* RFC 2080 - section 2.4.2 - Once the entry has been validated, 
         * update the metric by adding the cost of the network on which 
         * the message arrived.  If the result is greater than infinity, 
         * use infinity. 
         */
        metric = ep->ripng_metric + node->ripng_device.ripng_dev_metric;

        if (metric >= RT_INFINITY)
            metric = RT_INFINITY;

        route_tag = GET16(&ep->ripng_routetag, 0);

        /* If the address is all zeros, update the default route for this 
         * network. 
         */
        if (IPV6_IS_ADDR_UNSPECIFIED(ep->ripng_prefix))
        {
            /* Check if a default route already exists in the system.
             * Only update the default route if one does not already 
             * exist.
             */
            if (NU_Get_Default_Route(NU_FAMILY_IP6) == NU_NULL)
            {
                /* Only set this as the default route if the metric is less
                 * than infinity.
                 */
                if (metric < RT_INFINITY)
                {
                    /* Add the default route */
                    if (NU_Add_Route6(ep->ripng_prefix, next_hop, 
                                      ep->ripng_prefix_len) != NU_SUCCESS)
                        NLOG_Error_Log("Failed to add route", 
                                       NERR_SEVERE, __FILE__, __LINE__);

                    memset(&updated_route, -1, sizeof(UPDATED_ROUTE_NODE));

                    /* Set the new metric */
                    updated_route.urt_metric.urt6_metric = (INT16)metric;

                    /* Set the new route tag */
                    updated_route.urt_routetag = route_tag;

                    /* Set the interface index of the interface to use for
                     * this route.
                     */
                    updated_route.urt_dev.urt_dev_index = 
                        (INT32)node->ripng_device.ripng_dev_index;

                    /* Update the metric of the default route */
                    if (NU_Update_Route(ep->ripng_prefix, next_hop, &updated_route, 
                                        NU_FAMILY_IP6) != NU_SUCCESS)
                        NLOG_Error_Log("Failed to update route", 
                                       NERR_SEVERE, __FILE__, __LINE__);
                }
            }
        }

        /* Otherwise, the route specified is not a default route */
        else
        {
            /* RFC 2080 - section 2.4.2 - Check to see whether there is 
             * already an explicit route for the destination address.  
             * If there is no such route, add this route to the routing 
             * table, unless the metric is infinity (there is no point in 
             * adding a route which is unusable).  
             */
            current_rt_entry = 
                (RTAB6_ROUTE_ENTRY*)NU_Find_Route_By_Gateway(ep->ripng_prefix, 
                                                             next_hop, 
                                                             NU_FAMILY_IP6,
                                                             RT_OVERRIDE_METRIC |
                                                             RT_OVERRIDE_RT_STATE |
                                                             RT_OVERRIDE_DV_STATE);

            /* Check if we found a network route for an advertised host route */
            if (current_rt_entry)
            {
                current_rt_entry->rt_entry_parms.rt_parm_refcnt --;

                /* RFC 1058 - section 3.4.2 - avoid adding routes to hosts 
                 * if the host is part of a net or subnet for which we have 
                 * at least as good a route.
                 */
                if ( (ep->ripng_prefix_len == 128) && 
                     (current_rt_entry->rt_route_node->rt_submask_length != 128) )
                {
                    /* The network route is better than the host route */
                    if ( (!(IPV6_IS_ADDR_UNSPECIFIED(current_rt_entry->rt_route_node->rt_ip_addr))) &&
                         (current_rt_entry->rt_entry_parms.rt_parm_metric <= metric) )
                    {
                        /* Process the next RTE */
                        ep++;
                        continue;
                    }

                    /* The host route is better than the network route.  Add
                     * the route to the routing table.
                     */
                    else
                        current_rt_entry = NU_NULL;
                }
            }

            /* Initialize the structure used to update the route */
            memset(&updated_route, -1, sizeof(UPDATED_ROUTE_NODE));

            /* If there is not an explicit route and the new metric is
             * less than infinity, add a new route.
             */
            if ( (current_rt_entry == NU_NULL) && (metric < RT_INFINITY) )
            {
                /* Add the route to the routing table */
                ret = NU_Add_Route6(ep->ripng_prefix, next_hop, 
                                    ep->ripng_prefix_len);

                if (ret == NU_SUCCESS)
                {
                    /* Set up the modified parameters of the route entry */
                    updated_route.urt_metric.urt6_metric = (INT16)metric;
                    updated_route.urt_flags = RT_UP | RT_CHANGED | RT_RIPNG;

                    /* Set the new route tag */
                    updated_route.urt_routetag = route_tag;

                    /* Set the interface index of the interface to use for
                     * this route.
                     */
                    updated_route.urt_dev.urt_dev_index = 
                        (INT32)node->ripng_device.ripng_dev_index;

                    /* Update the parameters of the route that cannot be specified
                     * using NU_Add_Route.
                     */
                    if (NU_Update_Route(ep->ripng_prefix, next_hop, 
                                        &updated_route, NU_FAMILY_IP6) != NU_SUCCESS)
                        NLOG_Error_Log("Failed to update route", 
                                       NERR_SEVERE, __FILE__, __LINE__);

                    /* Signal the output process to trigger an update */     
                    RIPng_Triggered_Update = 1;
                }

                else
                {
                    NLOG_Error_Log("RIPng_Update_Table could not create new route node", 
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
            }

            /* If there is a route */
            else if (current_rt_entry)
            {
                /* RFC 2080 - section 2.4.2 - If there is an existing route, 
                 * compare the next hop address to the address of the router
                 * from which the datagram came.  If this datagram is from 
                 * the same router as the existing route, reinitialize the 
                 * timeout.
                 */
                if (memcmp(next_hop, fromaddr->id.is_ip_addrs, IP6_ADDR_LEN) == 0)
                {
                    /* Reinitialize the timeout. */
                    updated_route.urt_age = 0;
                    
                    /* If the advertised metric is infinity, set the metric of
                     * the route to infinity and time out the route.
                     */
                    if (ep->ripng_metric == RT_INFINITY)
                        metric = RT_INFINITY;

                    /* If the new metric is different than the old one, do the 
                     * following actions:
                     */
                    if (metric != current_rt_entry->rt_entry_parms.rt_parm_metric)
                    {
                        /* Adopt the route from the datagram (i.e., put the new 
                         * metric in and adjust the next hop address, if necessary).
                         */
                        updated_route.urt_metric.urt6_metric = (INT16)metric;

                        /* Set the new route tag */
                        updated_route.urt_routetag = route_tag;

                        /* Set the route change flag and signal the output process 
                         * to trigger an update
                         */
                        updated_route.urt_flags =
                            (INT32)(current_rt_entry->rt_entry_parms.rt_parm_flags | RT_CHANGED);

                        /* RFC 2080 - section 2.4.2 - If the new metric is infinity, 
                         * the deletion process begins for the route, which is no 
                         * longer used for routing packets.  
                         */
                        if (metric == RT_INFINITY)
                        {
                            /* Set the age to an expired time */
                            updated_route.urt_age = 
                                (INT32)(NU_Retrieve_Clock() / NU_PLUS_Ticks_Per_Second);
                        }

                        RIPng_Triggered_Update = 1;
                    }

                    /* RFC 2080 - section 2.4.2 - Note that the deletion process 
                     * is started only when the metric is first set to infinity.  
                     * If the metric was already infinity, then a new deletion
                     * process is not started.
                     */
                    if ( (metric < RT_INFINITY) ||
                         (current_rt_entry->rt_entry_parms.rt_parm_metric < RT_INFINITY) )
                    {
                        if (NU_Update_Route(current_rt_entry->rt_route_node->rt_ip_addr,
                                            next_hop, &updated_route, 
                                            NU_FAMILY_IP6) != NU_SUCCESS)
                            NLOG_Error_Log("Failed to update route", 
                                           NERR_SEVERE, __FILE__, __LINE__);
                    }
                }

                /* If the new metric is lower than the old one */
                else if (metric < current_rt_entry->rt_entry_parms.rt_parm_metric)
                {
                    /*  - Adopt the route from the datagram (i.e., put the new 
                     *    metric in and adjust the next hop address, if necessary).
                     */
                    updated_route.urt_metric.urt6_metric = (INT16)metric;

                    /*  - Set the route change flag and signal the output process 
                     *    to trigger an update
                     */
                    updated_route.urt_flags =
                        (INT32)(current_rt_entry->rt_entry_parms.rt_parm_flags | RT_CHANGED);

                    /* Set the new route tag */
                    updated_route.urt_routetag = route_tag;

                    RIPng_Triggered_Update = 1;

                    /* Update the route */
                    if (NU_Update_Route(current_rt_entry->rt_route_node->rt_ip_addr,
                                        next_hop, &updated_route, 
                                        NU_FAMILY_IP6) != NU_SUCCESS)
                        NLOG_Error_Log("Failed to update route", 
                                       NERR_SEVERE, __FILE__, __LINE__);
                }
            }
        }

        ep++;
    }
   
    return (NU_SUCCESS);

} /* RIPng_Update_Table */

/*************************************************************************
*
*   FUNCTION
*
*       RIPng_Find_Entry
*
*   DESCRIPTION
*
*       Finds a RIP-enabled device by interface index.
*
*   INPUTS
*
*       dev_index               The interface index of the interface the 
*                               most recent packet was received on.
*
*   OUTPUTS
*
*       RIPNG_LIST_NODE*        A pointer to the requested entry.
*       NU_NULL                 No matching device exists.
*
*************************************************************************/
STATIC RIPNG_LIST_NODE *RIPng_Find_Entry(UINT32 dev_index)
{
    RIPNG_LIST_NODE     *node;
  
    /* Search the list of RIP-enabled interfaces for the interface on
     * which the packet was received.
     */
    for (node = RIPng_List.ripng_head; node; node = node->ripng_next)
    {
        if (node->ripng_device.ripng_dev_index == dev_index)
            break;
    }
    
    return (node);
    
} /* RIPng_Find_Entry */

/*************************************************************************
*
*   FUNCTION
*
*       RIPng_Delete_Old
*
*   DESCRIPTION
*
*       Permanently deletes routes which have not been updated in 
*       RIPNG_RT_LIFE_TIME seconds and routes with a metric greater than
*       or equal to infinity (unreachable routes) from the Routing Table.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       > 0                     Index to the next node
*       0                       There are no nodes to delete
*       < 0                     There was an error
*
*************************************************************************/
STATIC INT RIPng_Delete_Old(VOID)
{
    INT         i;
    UINT32      curtime;

    /* If there are no nodes to delete, return */
    if (RIPng_Delete_Num <= 0)
    {
        RIPng_Delete_Num = 0;
        return (0);
    }

    curtime = NU_Retrieve_Clock();

    /* Delete in reverse order */
    for (i = (RIPng_Delete_Num - 1); i >= 0; i--)
    {
        /* RFC 2453 - section 3.8 - Should a new route to this network be 
         * established while the garbage-collection timer is running, the 
         * new route will replace the one that is about to be deleted.  
         * In this case the garbage-collection timer must be cleared.
         */
        if ( ((curtime - RIPng_delete_nodes[i]->rt_entry_parms.rt_parm_clock) >= RIPNG_RT_LIFE_TIME) ||
             (RIPng_delete_nodes[i]->rt_entry_parms.rt_parm_metric >= RT_INFINITY) )
        {       
            /* Delete the entry from the NET routing table */
            if (NU_Delete_Route2(RIPng_delete_nodes[i]->rt_route_node->rt_ip_addr, 
                                 RIPng_delete_nodes[i]->rt_next_hop.sck_addr, 
                                 NU_FAMILY_IP6) != NU_SUCCESS)
                NLOG_Error_Log("Failed to delete route", 
                               NERR_SEVERE, __FILE__, __LINE__);
        }

        RIPng_delete_nodes[i] = 0;
    }
    
    RIPng_Delete_Num = 0;
    
    return (i);

} /* RIPng_Delete_Old */

/*************************************************************************
*
*   FUNCTION
*
*       RIPng_Gather_Old
*
*   DESCRIPTION
*
*       This function traverses the entire NET routing table.  If an
*       entry is found that has not been used in RIPNG_RT_LIFE_TIME 
*       seconds or the route metric has been set to INFINITY, the route 
*       is placed on a list to be deleted from the NET routing table.
*
*   INPUTS
*
*       curtime                 Current time
*       flag                    Flag indicating whether to reset the 
*                               number of old routes gathered.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
STATIC VOID RIPng_Gather_Old(UINT32 curtime, INT16 flag)
{
    RTAB6_ROUTE_ENTRY   *current_route;
    UPDATED_ROUTE_NODE  updated_route;

#if ( (!defined(NET_5_3)) || (NET_VERSION_COMP < NET_5_3) )
    UINT8               null_addr[IP6_ADDR_LEN] = 
                            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#endif

    memset(&updated_route, -1, sizeof(UPDATED_ROUTE_NODE));

    if (flag)
        RIPng_Delete_Num = 0;

    /* Get a pointer to the first route in the Routing Table. */

#if ( (defined(NET_5_3)) && (NET_VERSION_COMP >= NET_5_3) )

    current_route = 
        (RTAB6_ROUTE_ENTRY*)NU_Find_Next_Route_Entry(NU_NULL, NU_FAMILY_IP6);

#else

    current_route = 
        (RTAB6_ROUTE_ENTRY*)NU_Find_Next_Route(null_addr, NU_FAMILY_IP6);

#endif

    /* Check each route in the table to determine if it should be 
     * deleted.
     */
    while (current_route)        
    {
        /* If the route has not been used in RT_LIFE_TIME amount of time 
         * or the metric is infinity, flag the node to be deleted.  STATIC and
         * SILENT routes are not affected by RIPng.
         */
        if ( (((curtime - 
                current_route->rt_entry_parms.rt_parm_clock) >= 
                RIPNG_RT_LIFE_TIME) ||
              (current_route->rt_entry_parms.rt_parm_metric >= RT_INFINITY)) &&
              (!(current_route->rt_entry_parms.rt_parm_flags & RT_STATIC)) &&
              (!(current_route->rt_entry_parms.rt_parm_flags & RT_SILENT)) )
        {
            /* If the delete list is not full. */
            if (RIPng_Delete_Num < RIPNG_MAX_DELETES)
            {
                /* Set the metric for the route to infinity */
                updated_route.urt_metric.urt6_metric = RT_INFINITY;

                /* Set the RT_CHANGE flag */
                updated_route.urt_flags = 
                    (INT32)(current_route->rt_entry_parms.rt_parm_flags | RT_CHANGED);

                /* The age must be set to remain the current age; 
                 * otherwise, NU_Update_Route will update the age to the
                 * current time.  If it is allowed to do so, the route will
                 * not be deleted by RIPng_Delete_Old.
                 */
                updated_route.urt_age = 
                    (INT32)((curtime - current_route->rt_entry_parms.
                            rt_parm_clock) / TICKS_PER_SECOND);

                /* Make the changes to the route */
                if (NU_Update_Route(current_route->rt_route_node->rt_ip_addr, 
                                    current_route->rt_next_hop.sck_addr, 
                                    &updated_route, NU_FAMILY_IP6) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to update route", NERR_SEVERE, 
                                   __FILE__, __LINE__);

                RIPng_delete_nodes[RIPng_Delete_Num] = current_route;
                RIPng_Delete_Num++;
            }

            /* The maximum number of nodes to delete has been put on the 
             * delete list.  Stop searching for nodes to delete.
             */
            else
                break;
        }

        /* Get a pointer to the next route in the Routing Table. */

#if ( (defined(NET_5_3)) && (NET_VERSION_COMP >= NET_5_3) )

        current_route = 
            (RTAB6_ROUTE_ENTRY*)
            NU_Find_Next_Route_Entry((ROUTE_ENTRY*)current_route, 
                                     NU_FAMILY_IP6);
#else
        current_route = 
            (RTAB6_ROUTE_ENTRY*)
            NU_Find_Next_Route(current_route->rt_route_node->rt_ip_addr, 
                               NU_FAMILY_IP6);
#endif
               
    }

} /* RIPng_Gather_Old */

/*************************************************************************
*
*   FUNCTION
*
*       RIPng_Gather_All
*
*   DESCRIPTION
*
*       Gather all route entries in route table that are not silent and
*       not link-local.
*
*   INPUTS
*
*       dev_index               The interface index of the device out 
*                               which the RIPng packet will be sent.
*       *current_addr           A pointer to the address of which to get
*                               the next address.
*       *num                    A pointer to the amount in the packet
*       max_entries             The maximum number of entries to gather.
*       flags                   A flag to indicate that the packets to be
*                               gathered are for a Triggered Update, in 
*                               which case only routes with the RT_CHANGED
*                               flag set will be returned.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
STATIC VOID RIPng_Gather_All(const UINT32 dev_index, const UINT8 *current_addr, 
                             UINT8 *num, UINT8 max_entries, INT16 flags)
{
    RTAB6_ROUTE_ENTRY   *current_route = NU_NULL;
    ROUTE_NODE          *default_route;

#if (RIPNG_DISABLE_POISONED_REVERSE != NU_TRUE)
    UNUSED_PARAMETER(dev_index);
#endif

    /* If the first route is desired, check for a default route */
    if (IPV6_IS_ADDR_UNSPECIFIED(current_addr))
    {
        default_route = NU_Get_Default_Route(NU_FAMILY_IP6);

        /* If a default route exists, put it in the packet */
        if (default_route)
            current_route = default_route->rt_route_entry_list.rt_entry_head;
    }

    /* Get a pointer to the first route in the route table */
    if (!current_route)
        current_route = 
            (RTAB6_ROUTE_ENTRY*)NU_Find_Next_Route(current_addr, NU_FAMILY_IP6);

    /* Traverse the entire routing table, gathering each valid route */
    while (current_route)
    {
        /* RFC 2453 - section 3.4.3 - ... it is never useful to claim 
         * reachability for a destination network to the neighbor(s) 
         * from which the route was learned.  "Split horizon" is a scheme 
         * for avoiding problems caused by including routes in updates sent 
         * to the router from which they were learned.  The "simple split 
         * horizon" scheme omits routes learned from one neighbor in updates 
         * sent to that neighbor.  "Split horizon with poisoned reverse" 
         * includes such routes in updates, but sets their metrics to 
         * infinity ... In general, split horizon with poisoned reverse is 
         * safer than simple split horizon ... The router requirements RFC 
         * specifies that all implementations of RIP must use split horizon 
         * and should also use split horizon with poisoned reverse, although 
         * there may be a knob to disable poisoned reverse.
         * RFC 2080 - section 2.4.2 - A link-local address should never be 
         * present in an RTE.
         */
        if ( (!(current_route->rt_entry_parms.rt_parm_flags & RT_SILENT)) &&
             (!(IPV6_IS_ADDR_LINKLOCAL(current_route->rt_route_node->rt_ip_addr)))

#if (RIPNG_DISABLE_POISONED_REVERSE == NU_TRUE)
             && 
             ((!(current_route->rt_entry_parms.rt_parm_flags & RT_RIPNG)) ||
              (current_route->rt_entry_parms.rt_parm_device->dev_index != dev_index))
#endif
             )
        {
            /* Check if we are only gathering changed routes */
            if ( (!flags) || 
                 (current_route->rt_entry_parms.rt_parm_flags & RT_CHANGED) )
            {
                /* If the maximum number of nodes has not been found */
                if (*num < max_entries)
                {
                    /* Add the node to the list of nodes */
                    RIPng_updated_nodes[*num] = current_route;

                    /* Increment the number of nodes on the list */
                    *num = (UINT8)(*num + 1);

                    /* Clear the route changed flag */
                    current_route->rt_entry_parms.rt_parm_flags &= ~RT_CHANGED;
                }

                /* The maximum number of nodes has been found.  Do not add
                 * any more nodes.
                 */
                else
                    break;
            }
        }

        /* Get a pointer to the next route */
        current_route = 
            (RTAB6_ROUTE_ENTRY*)NU_Find_Next_Route(current_route->rt_route_node->rt_ip_addr,
                                                   NU_FAMILY_IP6);
    }

} /* RIPng_Gather_All */

/*************************************************************************
*
*   FUNCTION
*
*       RIPng_Send_Updates
*
*   DESCRIPTION
*
*       Send all updated route table entries since last send.
*
*   INPUTS
*
*       *rpkt                   A pointer to the RIPng packet
*       entries                 The number of entries
*       *faddr                  A pointer to the destination the address
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
STATIC VOID RIPng_Send_Updates(RIPNG_HEADER *rpkt, INT entries, 
                               const struct addr_struct *faddr)
{
    SIGNED              ret;
    INT                 i;
    UINT16              len;
    RIPNG_ENTRY         *pkt;
    RTAB6_ROUTE_ENTRY   *rn;
    UINT8               next_hop[IP6_ADDR_LEN];
    struct addr_struct  baddr;

    /* Validate the socket since the response will be sent using the
     * same socket on which the request was received.
     */
    if (RIPng_Socket < 0)
    {
        NLOG_Error_Log("RIPng_Send_Updates invalid socket", 
                       NERR_SEVERE, __FILE__, __LINE__);

        return;
    }

    /* Check that there are entries to process */
    if (entries > 0)
    {      
        baddr.family = NU_FAMILY_IP6;
        baddr.port = faddr->port;

        NU_BLOCK_COPY(baddr.id.is_ip_addrs, faddr->id.is_ip_addrs, 
                      IP6_ADDR_LEN);

        baddr.name = "RIPng_RESP";

        /* RFC 2080 - section 2.5.2 - Set the command to response */
        rpkt->command = RIPNG_RESPONSE;
    
        /* For each rip struct in RIPNG_PACKET, see if route exists and
         * update metric in packet or set metric to RT_INFINITY and send the
         * packet back to sender 
         */
        pkt = (RIPNG_ENTRY *)&rpkt->ripng_prefix;

        /* Set the next-hop to the source of the REQUEST in case one is
         * not specified in the packet.
         */
        NU_BLOCK_COPY(next_hop, faddr->id.is_ip_addrs, IP6_ADDR_LEN);

        /* Process each entry in the packet */
        for (i = 0; i < entries; i++)
        {
            /* RFC 2080 - section 2.1.1 - A next hop RTE is identified by 
             * a value of 0xFF in the metric field of an RTE.
             */
            if (pkt->ripng_metric == RIPNG_NEXTHOP_METRIC)
            {
                /* RFC 2080 - section 2.1.1 - Specifying a value of 
                 * 0:0:0:0:0:0:0:0 in the prefix field of a next hop RTE 
                 * indicates that the next hop address should be the 
                 * originator of the RIPng advertisement. 
                 */
                if ( (IPV6_IS_ADDR_UNSPECIFIED(pkt->ripng_prefix)) ||
                     (!(IPV6_IS_ADDR_LINKLOCAL(pkt->ripng_prefix))) )
                    NU_BLOCK_COPY(next_hop, faddr->id.is_ip_addrs, 
                                  IP6_ADDR_LEN);

                /* RFC 2080 - section 2.1.1 - The prefix field specifies 
                 * the IPv6 address of the next hop.
                 */
                else
                    NU_BLOCK_COPY(next_hop, pkt->ripng_prefix, IP6_ADDR_LEN);

                /* Process the next RTE in the packet */
                pkt++;
                continue;
            }

            /* RFC 2080 - section 2.4.1 - If the request is for specific
             * entries, they are looked up in the routing table and the 
             * information is returned as is; no Split Horizon processing 
             * is done.
             */
            rn = 
                (RTAB6_ROUTE_ENTRY*)NU_Find_Route_By_Gateway(pkt->ripng_prefix, 
                                                             next_hop, 
                                                             NU_FAMILY_IP6,
                                                             RT_OVERRIDE_METRIC |
                                                             RT_HOST_MATCH);

            /* RFC 2080 - section 2.4.1 - If there is no explicit route 
             * to the specified destination, put infinity in the metric field.
             */
            if (rn == NU_NULL)
                pkt->ripng_metric = RT_INFINITY;

            /* RFC 2080 - section 2.4.1 - ... if there is a route, put that 
             * route's metric in the metric field of the RTE.
             */
            else
            {
                pkt->ripng_metric = (UINT8)rn->rt_entry_parms.rt_parm_metric;

                rn->rt_entry_parms.rt_parm_refcnt --;
            }

            /* Process the next RTE */
            pkt++;
        }

        /* Calculate the length of the packet to be transmitted */
        len = (UINT16)((entries * RIPNG_RTE_LEN) + RIPNG_HEADER_LEN);

        /* Send the packet directly to the original requester */
        ret = NU_Send_To(RIPng_Socket, (char *)rpkt, len, 0, &baddr, 
                         sizeof(baddr));

        if (ret <= 0)
        {
            /* If an ICMP error message was returned, try to send again */
            if (ICMP_ERROR_CODE(ret) == NU_TRUE)
            {
                NLOG_Error_Log("RIPng_Send_Updates NU_Send_To returned ICMP Error Code", 
                               NERR_RECOVERABLE, __FILE__, __LINE__);

                ret = NU_Send_To(RIPng_Socket, (char *)rpkt, len, 0, &baddr, 
                                 sizeof(baddr));
            }

            if (ret <= 0)
                NLOG_Error_Log("RIPng_Send_Updates NU_Send_To returned negative value", 
                               NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    return;

} /* RIPng_Send_Updates */

/*************************************************************************
*
*   FUNCTION
*
*       RIPng_Send_Table
*
*   DESCRIPTION
*
*       Send the complete route table.
*
*   INPUTS
*
*       *node                   Pointer to the RIPng list node.
*       *faddr                  Pointer to the destination address 
*                               structure.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
STATIC VOID RIPng_Send_Table(const RIPNG_LIST_NODE *node, 
                             const struct addr_struct *faddr)
{
    struct addr_struct  baddr;
 
    /* do not care which address message came from so set to zero */
    baddr.family = NU_FAMILY_IP6;
    baddr.port = faddr->port;

    NU_BLOCK_COPY(baddr.id.is_ip_addrs, faddr->id.is_ip_addrs, IP6_ADDR_LEN);

    baddr.name = "RIPng_RESP";

    RIPng_Send_Routes(node, 0, &baddr);

    return;

} /* RIPng_Send_Table */

/*************************************************************************
*
*   FUNCTION
*
*       RIPng_Broadcast
*
*   DESCRIPTION
*
*       Broadcast route entries for each interface in system.
*
*   INPUTS
*
*       flags                   Value indicating whether this is a 
*                               Triggered Update.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
STATIC VOID RIPng_Broadcast(INT16 flags)
{
    RIPNG_LIST_NODE     *node;
    struct addr_struct  baddr;

    baddr.family = NU_FAMILY_IP6;
    NU_BLOCK_COPY(baddr.id.is_ip_addrs, RIPng_Multi_Addr, IP6_ADDR_LEN);
    baddr.name = "RIPng_Response";
    baddr.port = RIPNG_PORT;
   
    /* Send a RIPng broadcast out each interface for all routes not on
     * that interface.
     */
    for (node = RIPng_List.ripng_head; node; node = node->ripng_next)
    {
#if (INCLUDE_IP_MULTICASTING == NU_TRUE)

        /* Set this device as the Multicast Interface */
        NU_Setsockopt_IPV6_MULTICAST_IF(RIPng_Socket, 
                                        (INT32)node->ripng_device.ripng_dev_index);

#endif

        RIPng_Send_Routes(node, flags, &baddr);
    }

    return;

} /* RIPng_Broadcast */

/*************************************************************************
*
*   FUNCTION
*
*       RIPng_Send_Routes
*
*   DESCRIPTION
*
*       This function sends all routes to the specified address.
*
*   INPUTS
*
*       *node                   A pointer to the RIPNG_LIST_NODE out which
*                               to send the packet.
*       flags                   Value indicating whether this is a 
*                               Triggered Update.
*       *baddr                  The destination address structure.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
STATIC VOID RIPng_Send_Routes(const RIPNG_LIST_NODE *node, INT16 flags,
                              const struct addr_struct *baddr)
{
    UINT8               y, z;
    RIPNG_ENTRY         *pkt;
    RIPNG_HEADER        *rp;
    UINT8               next_hop_count, count, returned_count = 0;
    UINT8               last_entry_addr[IP6_ADDR_LEN] = 
                            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    UINT16              len;
    INT32               ret;
    UINT8               max_entries;
    UINT8               current_next_hop[IP6_ADDR_LEN];
    RTAB6_ROUTE_ENTRY   *saved_node;
    UINT8               first_entry, pkt_metric;

#if (RIPNG_DISABLE_POISONED_REVERSE == NU_FALSE)
    UINT8               not_added = 0;
#endif

    if (RIPng_Socket < 0)
    {
        NLOG_Error_Log("RIPng_Send_Table invalid socket", 
                       NERR_SEVERE, __FILE__, __LINE__);

        return;
    }

    rp = (RIPNG_HEADER*)RIPng_Buffer;

    /* Initialize the RIPng Header */
    rp->command = RIPNG_RESPONSE; 
    rp->version = RIPNG_VERSION1;
    rp->unused = 0;

    /* RFC 2080 - section 2.1 -  The maximum datagram size is limited 
     * by the MTU of the medium over which the protocol is being used.  
     * Since an unsolicited RIPng update is never propagated across a 
     * router, there is no danger of an MTU mismatch.  The determination 
     * of the number of RTEs which may be put into a given message is a 
     * function of the medium's MTU, the number of octets of header 
     * information preceding the RIPng message, the size of the RIPng
     * header, and the size of an RTE.  The formula is:
     *
     *             +-                                                   -+
     *             | MTU - sizeof(IPv6_hdrs) - UDP_hdrlen - RIPng_hdrlen |
     * #RTEs = INT | --------------------------------------------------- |
     *             |                      RTE_size                       |
     *             +-                                                   -+
     */
    max_entries = (UINT8)((node->ripng_device.ripng_dev_mtu - IP6_HEADER_LEN - 
                   UDP_HEADER_LEN - RIPNG_HEADER_LEN) / RIPNG_RTE_LEN);

    if (max_entries > RIPNG_MAX_PER_PACKET)
        max_entries = RIPNG_MAX_PER_PACKET;

    /* Build a packet for each max_entries route entries in the route table. 
     * If there are more than max_entries entries, send multiple packets.
     */
    do
    {
        next_hop_count = returned_count = 0;

        /* Gather the routes */
        RIPng_Gather_All(node->ripng_device.ripng_dev_index, last_entry_addr, 
                         &returned_count, max_entries, flags);

        /* If routes were gathered, put them in the packet and transmit the
         * packet.
         */
        if (returned_count > 0)
        {  
            count = returned_count;

            /* Update the IP address of the last entry to put in the 
             * packet.
             */
            NU_BLOCK_COPY(last_entry_addr, 
                          RIPng_updated_nodes[count - 1]->rt_route_node->rt_ip_addr, 
                          IP6_ADDR_LEN);

            /* Get a pointer to the head of the packet */
            pkt = (RIPNG_ENTRY*)&RIPng_Buffer[RIPNG_HEADER_LEN];

            /* Fill the packet with the route entries */
            for (z = 0; z < count; z++)
            {
                /* The route may already have been added */
                if (RIPng_updated_nodes[z])
                {
                    /* Save a pointer to this node so it can be used later */
                    saved_node = RIPng_updated_nodes[z];

                    /* RFC 2080 - section 2.1.1 - Specifying a value of 
                     * 0:0:0:0:0:0:0:0 in the prefix field of a next
                     * hop RTE indicates that the next hop address should 
                     * be the originator of the RIPng advertisement. 
                     * If the received next hop address is not a link-local 
                     * address, it should be treated as 0:0:0:0:0:0:0:0.
                     */
                    if ( (DEV6_Get_Dev_By_Addr(RIPng_updated_nodes[z]->rt_next_hop.sck_addr)) ||
                         (!(IPV6_IS_ADDR_LINKLOCAL(RIPng_updated_nodes[z]->rt_next_hop.sck_addr))) )
                        memset(current_next_hop, 0, IP6_ADDR_LEN);

                    /* Extract the next-hop from the route */
                    else
                        NU_BLOCK_COPY(current_next_hop, 
                                      RIPng_updated_nodes[z]->rt_next_hop.sck_addr, 
                                      IP6_ADDR_LEN);

                    /* Do not put this next-hop entry in the packet until 
                     * a valid route is found that will use it in case
                     * Poisoned Reverse excludes this one.
                     */
                    first_entry = 1;

                    /* Add all routes to the packet that use this next-hop as their
                     * next-hop until a route is encountered that does not use
                     * this next-hop as its next-hop.
                     */
                    for (y = z; y < count; y++)
                    {
                        /* If the route has not already been added, and the route
                         * uses the same next-hop as the current next-hop, add
                         * it to the packet.
                         */
                        if ( (RIPng_updated_nodes[y]) &&
                             ((memcmp(RIPng_updated_nodes[y]->rt_next_hop.sck_addr,
                                      saved_node->rt_next_hop.sck_addr, 
                                      IP6_ADDR_LEN) == 0) ||
                              ((IPV6_IS_ADDR_UNSPECIFIED(current_next_hop)) &&
                               (DEV6_Get_Dev_By_Addr(RIPng_updated_nodes[y]->rt_next_hop.sck_addr)))) )
                        {
#if (RIPNG_DISABLE_POISONED_REVERSE == NU_FALSE)

                            /* RFC 2453 - section 3.4.3 - In particular, it is never useful 
                             * to claim reachability for a destination network to the 
                             * neighbor(s) from which the route was learned ... "Split horizon 
                             * with poisoned reverse" includes such routes in updates, but 
                             * sets their metrics to infinity.  
                             */
                            if ( (RIPng_updated_nodes[y]->rt_entry_parms.rt_parm_flags & RT_RIPNG) &&
                                 (RIPng_updated_nodes[y]->rt_entry_parms.rt_parm_device->dev_index == node->ripng_device.ripng_dev_index) )
                            {
                                /* RFC 2080 - section 2.5.2 - If, after Split Horizon 
                                 * processing the route should not be included, skip it.
                                 */
                                if ( (flags) && 
                                     (RIPng_updated_nodes[y]->rt_entry_parms.rt_parm_metric == RT_INFINITY) )
                                {
                                    not_added++;

                                    /* Set the entry to NULL */
                                    RIPng_updated_nodes[y] = NU_NULL;

                                    /* Get the next entry in the list */
                                    continue;
                                }
                                else
                                    pkt_metric = RT_INFINITY;
                            }
                            else
#endif
                                pkt_metric = 
                                    (UINT8)RIPng_updated_nodes[y]->rt_entry_parms.rt_parm_metric;

                            /* If there is a next-hop entry pending being added
                             * to the packet, add it now. 
                             */
                            if (first_entry)
                            {
                                /* Reset the first_entry flag */
                                first_entry = 0;

                                /* If this Next-Hop entry is the last RTE that will fit
                                 * in the packet, stop processing Routing Table entries.  
                                 * There is no sense in putting a Next-Hop entry in the 
                                 * packet when the RTE for the destination cannot fit.
                                 */
                                if ( ((next_hop_count + 1) + (z + 1)) > max_entries)
                                {
                                    /* Decrement the number of retrieved entries that 
                                     * will be added to the packet. 
                                     */
                                    count --;

                                    /* Get out of the loop and send the packet */
                                    z = count;

                                    break;
                                }

                                /* Increment the number of next-hop entries in the 
                                 * packet 
                                 */
                                next_hop_count ++;

                                /* If the addition of this Next-Hop entry and its 
                                 * corresponding destination route will cause the size 
                                 * of the packet to exceed the maximum allowable size, 
                                 * reduce the number of retrieved entries to add to
                                 * the packet by one.
                                 */
                                if ( (next_hop_count + count) > max_entries)
                                {
                                    /* Decrement the number of entries in the packet */
                                    count --;
                                }

                                /* Create a next-hop RTE in the packet */
                                NU_BLOCK_COPY(pkt->ripng_prefix, current_next_hop, 
                                              IP6_ADDR_LEN);

                                /* RFC 2080 - section 2.1.1 - A next hop RTE is identified 
                                 * by a value of 0xFF in the metric field of an RTE.
                                 */
                                pkt->ripng_metric = RIPNG_NEXTHOP_METRIC;

                                /* RFC 2080 - section 2.1.1 - The route tag and prefix 
                                 * length in the next hop RTE must be set to zero on 
                                 * sending and ignored on reception.
                                 */
                                pkt->ripng_prefix_len = 0;
                                PUT16(&pkt->ripng_routetag, 0, 0);

                                /* Increment the buffer to point to the next RTE */
                                pkt++;
                            }

                            /* Copy the parameters into the RTE */
                            NU_BLOCK_COPY(pkt->ripng_prefix, 
                                          RIPng_updated_nodes[y]->rt_route_node->rt_ip_addr, 
                                          IP6_ADDR_LEN);

                            pkt->ripng_prefix_len = 
                                RIPng_updated_nodes[y]->rt_route_node->rt_submask_length;

                            pkt->ripng_metric = pkt_metric;

                            pkt->ripng_routetag = 
                                INTSWAP(RIPng_updated_nodes[y]->rt_entry_parms.rt_parm_routetag);

                            /* Set each route to NULL after added */
                            RIPng_updated_nodes[y] = NU_NULL;

                            pkt++;

                        } /* if (RIPng_updated_nodes[y]) */

                        /* The routes were returned in a specific order, and if
                         * all routes that we retrieved do not fit in this packet,
                         * we have to start back where we left off; therefore, 
                         * there cannot be any holes in the array of returned
                         * addresses.  However, if two times the number of routes
                         * returned is less than or equal to the maximum number
                         * of entries that can fit in a packet, we will not traverse
                         * the routing table again and can add the entries in any
                         * order we wish; therefore, we are able to maximize packet
                         * size.
                         */
                        else if ( (count << 1) > max_entries)
                            break;

                    } /* for (y = z; y < max_entries; y++) */

                } /* if (RIPng_updated_nodes[z]) */

            } /* for (z = 0; z < count; z++) */

            /* Add the next-hop RTEs added to the packet */
            count = (UINT8)(count + next_hop_count);
   
#if (RIPNG_DISABLE_POISONED_REVERSE == NU_FALSE)
           
            /* Subtract the entries that were not added to the packet */
            count = (UINT8)(count - not_added);
#endif

            /* If there were entries collected, send them */
            if (count > 0)
            {
                /* Go back to the beginning of the last entry put
                 * in the packet so if there are more entries in the 
                 * Routing Table than will fit in the packet we know
                 * which entry we left off with.
                 */
                if ( (returned_count + next_hop_count) > max_entries)
                {
                    pkt --;

                    /* Update the IP address of the last entry to put in the 
                     * packet.
                     */
                    NU_BLOCK_COPY(last_entry_addr, pkt->ripng_prefix, 
                                  IP6_ADDR_LEN);
                }

                /* Compute the length of the packet */
                len = (UINT16)((count * RIPNG_RTE_LEN) + RIPNG_HEADER_LEN);

                /* Send the packet */
                ret = NU_Send_To(RIPng_Socket, RIPng_Buffer, len, 0, 
                                 baddr, sizeof(baddr));

                /* If there was an error sending the packet */
                if (ret <= 0)
                {
                    /* If an ICMP error message was returned, try to send again */
                    if (ICMP_ERROR_CODE(ret) == NU_TRUE)
                    {
                        NLOG_Error_Log("RIPng_Send_Routes NU_Send_To returned ICMP Error Code", 
                                       NERR_RECOVERABLE, __FILE__, __LINE__);

                        /* Send the packet */
                        ret = NU_Send_To(RIPng_Socket, RIPng_Buffer, len, 0, 
                                         baddr, sizeof(baddr));
                    }

                    /* If the packet could not be sent the second time */
                    if (ret <= 0)
                        NLOG_Error_Log("RIPng_Send_Routes NU_Send_To returned negative value", 
                                       NERR_RECOVERABLE, __FILE__, __LINE__);
                }

            } /* if (count > 0) */

        } /* if (count > 0) */

    } while (returned_count > 0);

    return;

} /* RIPng_Send_Routes */

/*************************************************************************
*
*   FUNCTION
*
*       RIPng_Request_Routing_Table
*
*   DESCRIPTION
*
*       This function sends a RIPng request to all routers on the link
*       requesting them to send their route tables.
*       
*   INPUTS
*
*       *node                   Pointer to the RIPng list node
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
STATIC VOID RIPng_Request_Routing_Table(const RIPNG_LIST_NODE *node)
{
    SIGNED              ret;
    INT16               len;
    RIPNG_ENTRY         *pkt;
    RIPNG_HEADER        *rp;
    struct addr_struct  baddr;

    /* If the parameters are all valid */
    if ( (RIPng_Socket >= 0) && (node != NU_NULL) )
    {
        /* Set up the destination address structure */
        baddr.family = NU_FAMILY_IP6;
        baddr.port = RIPNG_PORT;
        baddr.name = "RIPng_BCST";

        NU_BLOCK_COPY(baddr.id.is_ip_addrs, RIPng_Multi_Addr, 
                      IP6_ADDR_LEN);

        /* Get a pointer to the head of the buffer */
        rp = (RIPNG_HEADER*)RIPng_Buffer;

        rp->command = RIPNG_REQUEST; 
        rp->version = RIPNG_VERSION1;
        rp->unused = 0; 

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)

        /* Set this device as the Multicast Interface */
        NU_Setsockopt_IPV6_MULTICAST_IF(RIPng_Socket, 
                                        (INT32)node->ripng_device.ripng_dev_index);

#endif

        /* Get a pointer to the first RTE */
        pkt = (RIPNG_ENTRY *)&RIPng_Buffer[RIPNG_HEADER_LEN];

        /* RFC 2080 - section 2.4.1 - If there is exactly one entry 
         * in the request, and it has a destination prefix of zero, a
         * prefix length of zero, and a metric of infinity (i.e., 16), 
         * then this is a request to send the entire routing table. 
         */
        PUT16(&pkt->ripng_routetag, 0, 0);
        pkt->ripng_prefix_len = 0;

        memset(pkt->ripng_prefix, 0, IP6_ADDR_LEN);

        pkt->ripng_metric = RT_INFINITY;

        len = RIP_PKT_SIZE + RIPNG_HEADER_LEN;
   
        ret = NU_Send_To(RIPng_Socket, RIPng_Buffer, (UINT16)len, 0, 
                         &baddr, sizeof(baddr));

        if (ret <= 0)
            NLOG_Error_Log("RIPng_Request_Routing_Table NU_Send_To returned negative value", 
                           NERR_RECOVERABLE, __FILE__, __LINE__);   
    }
    
    return;

} /* RIPng_Request_Routing_Table */

#endif
