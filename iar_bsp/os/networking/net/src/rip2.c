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
*       rip2.c
*
*   COMPONENT
*
*       Routing
*
*   DESCRIPTION
*
*       Routing Information Protocol 2 per RFC 2453
*
*   DATA STRUCTURES
*
*       debug_string
*       RIP2_List
*       RIP2_Socket
*
*   FUNCTIONS
*
*       NU_Rip2_Initialize
*       RIP2_Task_Entry
*       RIP2_Update_Table
*       RIP2_IsNetworkClass_ABC
*       RIP2_IsBroadcast_Address
*       RIP2_IsFromMe
*       RIP2_Find_Entry
*       RIP2_Delete_Old
*       RIP2_Gather_Old
*       RIP2_Gather_All
*       RIP2_Send_Updates
*       RIP2_Send_Table
*       RIP2_Broadcast
*       RIP2_Send_Routes
*       RIP2_Request_Routing_Table
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV4 == NU_TRUE)

#if (INCLUDE_UDP == NU_FALSE)
#warning UDP must be included in order to use RIP2.  RIP2 will be excluded from the build.
#else

/* Change to NU_TRUE to add debugging output */
#define NU_DEBUG_RIP2    NU_FALSE

#if (NU_DEBUG_RIP2 == NU_TRUE)

/* This debugging function is only an example. You will need to
set this macro to your debug printing routine. */
#define ATI_DEBUG_PRINT(x)  NU_Printf(x);

CHAR debug_string[250];     /* String ATI_DEBUG_PRINT passes */

#endif

/* delete n at a any one time. */
static RTAB4_ROUTE_ENTRY    *delete_nodes[RIP2_MAX_DELETES];
static RTAB4_ROUTE_ENTRY    *updated_nodes[RIP2_MAX_PER_PACKET];

static  INT         RIP2_Socket;       /* Send or receive socket for broadcasts */
static  INT         RIP2_Delete_Num = 0;
static  CHAR        *RIP2_Buffer;
static  NU_TASK     RIP2_Tcb;

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)
static  UINT8       RIP2_Multi_Addr[IP_ADDR_LEN] = {224, 0, 0, 9};
#endif

/*  Local prototypes.  */
STATIC  VOID            RIP2_Task_Entry(UNSIGNED, VOID *);
STATIC  VOID            RIP2_Broadcast(INT16);
STATIC  VOID            RIP2_Request_Routing_Table(RIP2_LIST_NODE *);
STATIC  VOID            RIP2_Gather_Old(UINT32, INT16);
STATIC  INT             RIP2_Delete_Old(VOID);
STATIC  VOID            RIP2_Gather_All(const UINT32, const UINT8 *, INT *,
                                        INT16);
STATIC  VOID            RIP2_Send_Table(const RIP2_LIST_NODE *,
                                        const struct addr_struct *, UINT8);
STATIC  VOID            RIP2_Send_Updates(RIP2_PACKET *, INT,
                                          const struct addr_struct *);
STATIC  RIP2_LIST_NODE  *RIP2_Find_Entry(const UINT8 *);
STATIC  INT16           RIP2_IsNetworkClass_ABC(UINT32);
STATIC  INT16           RIP2_IsBroadcast_Address(const UINT8 *, INT16);
STATIC  INT16           RIP2_IsFromMe(UINT32, const struct addr_struct *);
STATIC  INT             RIP2_Update_Table(RIP2_LIST_NODE *, RIP2_PACKET *,
                                          INT, const struct addr_struct *);
STATIC  VOID            RIP2_Send_Routes(const RIP2_LIST_NODE *, INT16,
                                         const struct addr_struct *, UINT8);

RIP2_LIST_STRUCT    RIP2_List;
static  INT16       RIP2_Triggered_Update;

#if (INCLUDE_STATIC_BUILD == NU_TRUE)

/* Declare memory for RIP2 list nodes */
RIP2_LIST_NODE   NET_RIP2_Memory[NET_MAX_RIP_TREE_NODES];

/* Declare memory for RIP2 Task */
CHAR             NET_RIP2_Task_Memory[RIP2_STACK_SIZE];
#endif

/*************************************************************************
*
*   FUNCTION
*
*       NU_Rip2_Initialize
*
*   DESCRIPTION
*
*       This function initializes the RIP 2 module.
*
*   INPUTS
*
*       *rip2                   A pointer to the beginning node.
*       num                     The number of nodes in the tree.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful
*       NU_INVALID_PARM         rip2 is NU_NULL, num is less than or
*                               equal to 0 or the RIP send or receive
*                               mode is RIP-2, but multicasting support
*                               is not included.
*       NU_MEM_ALLOC            Memory allocation failed
*       NU_INVAL                Generic invalid return
*
*************************************************************************/
STATUS NU_Rip2_Initialize(const RIP2_STRUCT *rip2, INT num)
{
    STATUS              status;
    VOID                *pointer;
    struct addr_struct  bcastaddr;
    INT                 i;
    RIP2_LIST_NODE      *node = NU_NULL;
    DV_DEVICE_ENTRY     *dev_ptr;

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)
    NU_IOCTL_OPTION     option;
#endif

    NU_SUPERV_USER_VARIABLES

    if ( (rip2 == NU_NULL) || (num <= 0) )
        return (NU_INVALID_PARM);

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

#if (INCLUDE_IP_MULTICASTING == NU_FALSE)

    for (i = 0; i < num; i++)
    {
        /* If multicasting is not included and the Receive mode is set to
         * receive RIP-2 packets, return an error.
         */
        if (rip2[i].rip2_recvmode & RECV_RIP2)
        {
            NLOG_Error_Log("RIP2 Send and Receive mode requires multicasting support",
                           NERR_FATAL, __FILE__, __LINE__);

            return (NU_INVALID_PARM);
        }
    }
#endif

    /* Setup the socket that will be used by RIP to send and receive. */

    /* open a socket interface that we will listen and send on. */
    RIP2_Socket = NU_Socket(NU_FAMILY_IP, NU_TYPE_DGRAM, 0);

    if (RIP2_Socket < 0)
    {
        NLOG_Error_Log("NU_Rip2_Initialize could not create socket",
                       NERR_FATAL, __FILE__, __LINE__);

        /* Return to user mode */
        NU_USER_MODE();

        return (NU_INVAL);
    }

    /* fill in a structure with the listening address */
    bcastaddr.family = NU_FAMILY_IP;
    bcastaddr.port = RIP_PORT;

    /* RIP does not care which address message came from. */
    ((UINT32 *)bcastaddr.id.is_ip_addrs)[0] = IP_ADDR_ANY;

    bcastaddr.name = "RIP2";

    status = NU_Bind(RIP2_Socket, &bcastaddr, 0);

    if (status < 0)
    {
        NLOG_Error_Log("NU_Rip2_Initialize NU_Bind failed", NERR_FATAL,
                       __FILE__, __LINE__);

        /* Return to user mode */
        NU_USER_MODE();

        return (NU_INVAL);
    }

    /* turn on broadcasting for the transmitting socket */
    status = NU_Setsockopt_SO_BROADCAST(RIP2_Socket, 1);

    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("NU_Rip2_Initialize could not turn on broadcasting for socket",
                       NERR_FATAL, __FILE__, __LINE__);

        /* Return to user mode */
        NU_USER_MODE();

        return (NU_INVAL);
    }

    /* Create the list of devices and their configurations to be used with
     * RIP2.
     */
#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    status = NU_Allocate_Memory(MEM_Cached, (VOID**)&node,
                                (UNSIGNED)((UNSIGNED)num * sizeof(RIP2_LIST_NODE)),
                                (UNSIGNED)NU_NO_SUSPEND);
#else
    /* Assign memory to the list */
    if(num > NET_MAX_RIP_TREE_NODES)
        status = NU_NO_MEMORY;
    else
    {
        node   = NET_RIP2_Memory;
        status = NU_SUCCESS;
    }
#endif

    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("NU_Rip2_Initialize no memory for list of devices",
                       NERR_FATAL, __FILE__, __LINE__);

        /* Return to user mode */
        NU_USER_MODE();

        return (NU_MEM_ALLOC);
    }

    /* Initialize the triggered update flag */
    RIP2_Triggered_Update = 0;

    for (i = 0; i < num; i++)
    {
        /* Look up the device the user wishes to use RIP2 with. */
        dev_ptr = DEV_Get_Dev_By_Name(rip2[i].rip2_device_name);

        if (dev_ptr == NU_NULL)
        {
            NLOG_Error_Log("NU_Rip2_Initialize device does not exist",
                           NERR_FATAL, __FILE__, __LINE__);

            /* Return to user mode */
            NU_USER_MODE();

            return (NU_INVALID_PARM);
        }

        /* Store off the device index */
        node[i].r2_device.rip2_dev_index = dev_ptr->dev_index;

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)

        /* Does this device want to RX RIP2 messages? If so join the
         * multicast group for this device.
         */
        if (rip2[i].rip2_recvmode & RECV_RIP2)
        {
            /* Store the devices name. */
            option.s_optval = (UINT8*)rip2[i].rip2_device_name;

            /* Get the IP address of the device for which we desire to join a
             * multicast group.
             */
            if (NU_Ioctl(SIOCGIFADDR, &option, sizeof(option)) == NU_SUCCESS)
            {
                /* Join a multicast group. */
                if (NU_IP_Multicast_Listen(RIP2_Socket, option.s_ret.s_ipaddr,
                                           RIP2_Multi_Addr,
                                           MULTICAST_FILTER_EXCLUDE,
                                           NU_NULL, 0) != NU_SUCCESS)
                    NLOG_Error_Log("NU_Rip2_Initialize could not join multicast group",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        } /* end if RX multicast packets */

#endif
        /* If the user specified a new metric then use it. */
        if (rip2[i].rip2_metric)
            node[i].r2_device.rip2_dev_metric = (UINT8)rip2[i].rip2_metric;

        /* Otherwise, use the metric associated with the interface. */
        else
            node[i].r2_device.rip2_dev_metric = (UINT8)dev_ptr->dev_metric;

        node[i].r2_sendmode = rip2[i].rip2_sendmode;
        node[i].r2_recvmode = rip2[i].rip2_recvmode;

        /* Add this one to the list. */
        DLL_Enqueue(&RIP2_List, &node[i]);
    }

    /* Create RIP2 task */
#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    status = NU_Allocate_Memory(MEM_Cached, &pointer,
                                (UNSIGNED)RIP2_STACK_SIZE,
                                (UNSIGNED)NU_NO_SUSPEND);
    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("NU_Rip2_Initialize no memory for RIP2 task",
                       NERR_FATAL, __FILE__, __LINE__);

        /* Return to user mode */
        NU_USER_MODE();

        return (NU_MEM_ALLOC);
    }
#else
    /* Assign memory to the RIP2 Task */
    pointer = (VOID *)NET_RIP2_Task_Memory;
#endif

    pointer = TLS_Normalize_Ptr(pointer);

    status = NU_Create_Task(&RIP2_Tcb, "RIP2TASK", RIP2_Task_Entry,
                            (UNSIGNED)0, NU_NULL, pointer,
                            (UNSIGNED)RIP2_STACK_SIZE, RIP2_PRIORITY,
                            (UNSIGNED)RIP2_TIME_SLICE, RIP2_PREEMPT,
                             NU_NO_START);

    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("NU_Rip2_Initialize could not create task",
                       NERR_FATAL, __FILE__, __LINE__);

        /* Return to user mode */
        NU_USER_MODE();

        return (NU_INVAL);
    }

    /* Start the RIP2 task */
    status = NU_Resume_Task(&RIP2_Tcb);

    /* Return to user mode */
    NU_USER_MODE();

    return (status);

} /* NU_Rip2_Initialize */

/*************************************************************************
*
*   FUNCTION
*
*       RIP2_Task_Entry
*
*   DESCRIPTION
*
*       The RIP2 task insertion
*
*   INPUTS
*
*       argc                    Unused
*       *argv                   Unused
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
STATIC VOID RIP2_Task_Entry(UNSIGNED argc, VOID *argv)
{
    struct addr_struct  fromaddr;
    UINT32              curtime, garbage_time = 0,
                        triggered_update = 0;
    UINT32              del_time;
    STATUS              status;
    SIGNED              bytes_received;
    INT16               flen;
    UINT32              wtime, broadcast_timer = 0;
    INT                 entry_count;
    RIP2_PACKET         *rpkt;
    FD_SET              readfs;
    RIP2_LIST_NODE      *node, *next_node;
    CHAR                rip2_buf[RIP2_BUFFER_SIZE];

    NU_SUPERV_USER_VARIABLES

#if (NU_DEBUG_RIP2 == NU_TRUE)
    CHAR                temp[10];
#endif

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

#if (NU_DEBUG_RIP2 == NU_TRUE)
    strcpy(debug_string, "in RIP2_Task_Entry\n\r");
    ATI_DEBUG_PRINT(debug_string);
#endif

    UNUSED_PARAMETER(argc);
    UNUSED_PARAMETER(argv);

    /* Setup the buffer */
    RIP2_Buffer = rip2_buf;

    /* The sending socket can do both broadcast and multicast sends,
     * you just have to enable them through setsockopt.
     */

    /* RFC 2453 - section 3.9.1 - When a router first comes up, it
     * multicasts a Request on every connected network asking for a
     * complete routing table.
     */
    node = RIP2_List.r2_head;

    while (node)
    {
        /* Save a pointer to the next node since RIP2_Request_Routing_Table
         * could delete the node if the interface has been removed.
         */
        next_node = node->r2_next;

        RIP2_Request_Routing_Table(node);

        /* Get a pointer to the next node */
        node = next_node;
    }

    del_time = NU_Retrieve_Clock() + RIP2_DELETE_INTERVAL;

    wtime = 0;

    for (;;)
    {
        /* If the last broadcast was done as a result of the 30 second
         * broadcast timer expiring, reset the broadcast timer.
         */
        if (wtime == 0)
            broadcast_timer = NU_Retrieve_Clock() + RIP2_BTIME;

        /* Continue waiting for data until it is time to send out a
         * broadcast on the link or a Triggered Update has been set.
         */
        do
        {
            /* Reset wtime to the amount of time left to wait until a
             * Broadcast needs to be sent.
             */
            wtime = TQ_Check_Duetime(broadcast_timer);

            if (wtime)
            {
                NU_FD_Init(&readfs);
                NU_FD_Set(RIP2_Socket, &readfs);

                /* The first parameter in this call is normally NSOCKETS. This is an
                 * optimization so that the select call does not search the entire
                 * socket list. Instead it will only search through the sockets up to
                 * the RIP2_Socket. The RIP2_Socket should be among the first created.
                 */
                status = NU_Select((RIP2_Socket + 1), &readfs, NU_NULL, NU_NULL,
                                   (UNSIGNED)wtime);

                if (status == NU_NO_DATA)
                    continue;

                /* Check for data on the socket */
                if (NU_FD_Check(RIP2_Socket, &readfs) == NU_FALSE)
                    continue;

                /* If the select returns NU_SUCCESS then a packet is
                 * waiting to be read on the socket.
                 */

                fromaddr.family = NU_FAMILY_IP;
                fromaddr.port = 0;

                /* Go get the router packets. */
                bytes_received = NU_Recv_From(RIP2_Socket, RIP2_Buffer,
                                              (UINT16)RIP2_BUFFER_SIZE,
                                              (INT16)0, &fromaddr, &flen);
                /* We got an error */
                if (bytes_received < 0)
                {
                    NLOG_Error_Log("RIP2_Task_Entry NU_Recv_From error",
                                   NERR_FATAL, __FILE__, __LINE__);

                    NET_DBG_Notify(bytes_received, __FILE__, __LINE__,
                                   NU_Current_Task_Pointer(), NU_NULL);
                }

                /* Process the packet */
                else
                {
                    rpkt = (RIP2_PACKET*)RIP2_Buffer;

                    bytes_received -= 4;        /* remove header size */
                    entry_count = ((INT)bytes_received / RIP_PKT_SIZE);

                    /* Check that there is not extra garbage in the packet */
                    if ( (entry_count * RIP_PKT_SIZE) != bytes_received)
                    {
#if (NU_DEBUG_RIP2 == NU_TRUE)
                        strcpy(debug_string, "Rejecting packet due to invalid packet size.\n\r");
                        ATI_DEBUG_PRINT(debug_string);
#endif
                        NLOG_Error_Log("RIP2_Task_Entry received invalid sized packet",
                                       NERR_FATAL, __FILE__, __LINE__);

                        continue;
                    }

                    /* RFC 1058 - section 3.4 - All fields that are described
                     * above as "must be zero" are to be checked.  If any such
                     * field contains a non-zero value, the entire message is
                     * to be ignored.
                     */
                    if ( (rpkt->version == 1) && (rpkt->unused != 0) )
                    {
#if (NU_DEBUG_RIP2 == NU_TRUE)
                        strcpy(debug_string, "Rejecting RIP-1 packet due to non-zero MBZ field.\n\r");
                        ATI_DEBUG_PRINT(debug_string);
#endif
                        NLOG_Error_Log("RIP2_Task_Entry received RIP-1 packet with non-zero MBZ field",
                                       NERR_FATAL, __FILE__, __LINE__);

                        continue;
                    }

#if (NU_DEBUG_RIP2 == NU_TRUE)

                    /* sprintf(debug_string, "bytes_received = %d  entry_count = %d\n\r",
                    bytes_received, entry_count); */
                    strcpy(debug_string, "bytes_received = ");
                    NU_ITOA((int)bytes_received, temp, 10);
                    strcat(debug_string, temp);
                    strcat(debug_string, " ");
                    strcat(debug_string, "entry_count = ");
                    NU_ITOA(entry_count, temp, 10);
                    strcat(debug_string, temp);
                    strcat(debug_string, "\n\r");

                    ATI_DEBUG_PRINT(debug_string);

                    strcpy(debug_string, "fromaddr       = ");
                    NU_ITOA(fromaddr.id.is_ip_addrs[0], temp, 10);
                    strcat(debug_string, temp);
                    strcat(debug_string, ".");
                    NU_ITOA(fromaddr.id.is_ip_addrs[1], temp, 10);
                    strcat(debug_string, temp);
                    strcat(debug_string, ".");
                    NU_ITOA(fromaddr.id.is_ip_addrs[2], temp, 10);
                    strcat(debug_string, temp);
                    strcat(debug_string, ".");
                    NU_ITOA(fromaddr.id.is_ip_addrs[3], temp, 10);
                    strcat(debug_string, temp);
                    strcat(debug_string, ":");
                    NU_ITOA(fromaddr.port, temp, 10);
                    strcat(debug_string, temp);

                    ATI_DEBUG_PRINT(debug_string);

                    if (rpkt->command == RIP2_REQUEST)
                        strcpy(debug_string, "   and it's a REQUEST packet\n\r");
                    else
                        strcpy(debug_string, "   and it's a RESPONSE packet\n\r");

                    ATI_DEBUG_PRINT(debug_string);

#endif
                    /* Check that this packet came from a router on one of the
                     * directly connected networks.
                     */
                    node = RIP2_Find_Entry(fromaddr.id.is_ip_addrs);

                    /* If this packet is from an invalid router or this node does not
                     * support the version of RIP.
                     */
                    if ( (node == NU_NULL) ||
                         ((!(node->r2_recvmode & RECV_RIP2)) && (rpkt->version == 2)) ||
                         ((!(node->r2_recvmode & RECV_RIP1)) && (rpkt->version == 1)) ||
                         (rpkt->version == 0) )
                    {
                        NLOG_Error_Log("RIP2_Task_Entry rejected packet",
                                       NERR_INFORMATIONAL, __FILE__, __LINE__);

#if (NU_DEBUG_RIP2 == NU_TRUE)
                        strcpy(debug_string, "Rejecting packet due to version.\n\r");
                        ATI_DEBUG_PRINT(debug_string);
#endif
                        continue;
                    }

                    /* Process a RIP request */
                    if (rpkt->command == RIP2_REQUEST)
                    {
                        /* RFC 1058 - section 3.4.1 - If a request comes from port 520,
                         * silent processes do not respond.  If the request comes from
                         * any other port, processes must respond even if they are
                         * silent.
                         */
                        if ( (node->r2_sendmode != SEND_NONE) ||
                             ((node->r2_sendmode == SEND_NONE) &&
                              (fromaddr.port != RIP_PORT)) )
                        {
                            /* RFC 2453 - section 3.9.1 - If there is exactly one entry
                             * in the request, and it has an address family identifier of
                             * zero and a metric of infinity (i.e., 16), then this is a
                             * request to send the entire routing table.
                             */
                            if ( (entry_count == 1) && (INTSWAP(rpkt->af_id) == 0) )
                            {
                                if (GET32(&rpkt->metric, 0) == RT_INFINITY)
                                {
#if (NU_DEBUG_RIP2 == NU_TRUE)
                                    strcpy(debug_string, "Sending complete table.\n\r");
                                    ATI_DEBUG_PRINT(debug_string);
#endif
                                    RIP2_Send_Table(node, &fromaddr, rpkt->version);
                                }
                                else
                                {
#if (NU_DEBUG_RIP2 == NU_TRUE)
                                    strcpy(debug_string, "Received invalid metric in request for routing table.\n\r");
                                    ATI_DEBUG_PRINT(debug_string);
#endif
                                    NLOG_Error_Log("RIP2_Task_Entry received invalid metric in request for routing table",
                                                   NERR_INFORMATIONAL, __FILE__, __LINE__);
                                }
                            }

                            else
                            {
#if (NU_DEBUG_RIP2 == NU_TRUE)
                                strcpy(debug_string, "Sending only requested entries.\n\r");
                                ATI_DEBUG_PRINT(debug_string);
#endif
                                RIP2_Send_Updates(rpkt, entry_count, &fromaddr);
                            }
                        }
                    }

                    /* Process a RIP response */
                    else if (rpkt->command == RIP2_RESPONSE)
                    {
#if (NU_DEBUG_RIP2 == NU_TRUE)
                        strcpy(debug_string, "Received update entries.\n\r");
                        ATI_DEBUG_PRINT(debug_string);
#endif

                        /* The Response must be ignored if it is not from the RIP port.
                         * The datagram's IPv4 source address should be checked to see
                         * whether the datagram is from a valid neighbor; the source of
                         * the datagram must be on a directly-connected network.  It is
                         * also worth checking to see whether the response is from one
                         * of the router's own addresses.
                         */
                        if ( (fromaddr.port == RIP_PORT) &&
                             (RIP2_IsFromMe(node->r2_device.rip2_dev_index, &fromaddr) == 0) )
                        {
#if (NU_DEBUG_RIP2 == NU_TRUE)
                            strcpy(debug_string, "Updating table.\n\r");
                            ATI_DEBUG_PRINT(debug_string);
#endif
                            if (RIP2_Update_Table(node, rpkt, entry_count, &fromaddr))
                                NLOG_Error_Log("RIP2_Task_Entry error in RIP2_Update_Table",
                                               NERR_INFORMATIONAL, __FILE__, __LINE__);
                        }

#if (NU_DEBUG_RIP2 == NU_TRUE)
                        else
                        {
                            strcpy(debug_string, "Rejecting packet to update table.\n\r");
                            ATI_DEBUG_PRINT(debug_string);
                        }
#endif
                    } /* else if */

                    else
                    {
#if (NU_DEBUG_RIP2 == NU_TRUE)
                        strcpy(debug_string, "Received invalid command.\n\r");
                        ATI_DEBUG_PRINT(debug_string);
#endif
                        NLOG_Error_Log("RIP2_Task_Entry received invalid command",
                                       NERR_INFORMATIONAL, __FILE__, __LINE__);
                    }
                } /* else */
            } /* if (wtime) */
        } while ( (wtime) && (RIP2_Triggered_Update == 0) );

        curtime = NU_Retrieve_Clock();

        /* If we broke out of the loop to send a Triggered Update, do not
         * bother checking the delete and garbage collection timers.
         */
        if (!RIP2_Triggered_Update)
        {
            /* RFC 2453 - section 3.8 - Every 30 seconds, the RIP process is
             * awakened to send an unsolicited Response message containing
             * the complete routing table to every neighboring router.
             */
            RIP2_Broadcast(0);

            /* RFC 2453 - section 3.8 - If 180 seconds elapse from the last
             * time the timeout was initialized, the route is considered to
             * have expired, and the deletion process begins for that route.
             */
            if (TQ_Check_Duetime(del_time) == 0)
            {
                del_time = curtime + RIP2_DELETE_INTERVAL;

                /* This function will only gather MAX_DELETES per call. */
                RIP2_Delete_Num = 0;

                /* Gather the old entries from the routing table */
                RIP2_Gather_Old(curtime, 1);

                /* If entries were found to be deleted and the garbage collection
                 * timer is not already running.
                 */
                if (RIP2_Delete_Num)
                {
                    /* Set the garbage collection timer to 120 seconds */
                    garbage_time = curtime + RIP2_GARBAGE_COLLECTION_INTERVAL;

                    /* Set the flag to send a Triggered Update */
                    RIP2_Triggered_Update = 1;
                }
            }

            /* RFC 2453 - section 3.8 - Upon expiration of the
             * garbage-collection timer, the route is finally removed from
             * the routing table.
             */
            if ( (garbage_time) && (TQ_Check_Duetime(garbage_time) == 0) )
            {
                RIP2_Delete_Old();

                garbage_time = 0;
            }
        }

        /* If a Triggered Update has been invoked from a received packet or
         * by the deletion process.
         */
        if (RIP2_Triggered_Update)
        {
            /* Triggered updates must be limited to once every 1-5
             * seconds.
             */
            if (((curtime - triggered_update) * SCK_Ticks_Per_Second) >= 1)
            {
                triggered_update = curtime;

                /* Send a Triggered Update */
                RIP2_Broadcast(1);

                /* Reset the Triggered Update flag */
                RIP2_Triggered_Update = 0;
            }
        }
    }

    /* Note this instruction is commented out to remove a compiler
       warning. The while loop above should never be exited and this
       instruction never executed. Thus the reason for the compiler
       warning and the justification to comment out this instruction.

       This line is left in just for completeness and so that in the
       future it is not overlooked.


    if (NU_Close_Socket(RIP2_Socket) != NU_SUCCESS)
    {
        NLOG_Error_Log("RIP2_Task_Entry could not close socket",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }


    NU_USER_MODE();
    */

} /* RIP2_Task_Entry */

/*************************************************************************
*
*   FUNCTION
*
*       RIP2_Update_Table
*
*   DESCRIPTION
*
*       Update the routing table with routes received from other routers.
*
*   INPUTS
*
*       *node                   Pointer to a node on the RIP2 list
*       *rpkt                   Pointer to the RIP2 packet passed in
*       pkt_cnt                 How many packets
*       *fromaddr               A pointer to the source address
*
*   OUTPUTS
*
*       NU_SUCCESS              Success
*       -1                      RTAB_Insert_Node failed
*
*************************************************************************/
STATIC INT RIP2_Update_Table(RIP2_LIST_NODE *node, RIP2_PACKET *rpkt,
                             INT pkt_cnt, const struct addr_struct *fromaddr)
{
    INT                 i;
    INT                 ret;
    UINT8               default_mask[IP_ADDR_LEN];
    UINT8               next_hop[IP_ADDR_LEN];
    UINT8               subnet_mask[IP_ADDR_LEN];
    INT16               c;
    RTAB4_ROUTE_ENTRY   *current_rt_entry;
    RIP2_HEADER         *rh;
    RIP2_ENTRY          *ep;
    UPDATED_ROUTE_NODE  updated_route;
    UINT32              metric;
    STATUS              status;
    DV_DEVICE_ENTRY     *dev_ptr;

#if (NU_DEBUG_RIP2 == NU_TRUE)

    CHAR                temp[10];

    strcpy(debug_string, "in RIP2_Update_Table\n\r");
    ATI_DEBUG_PRINT(debug_string);

#endif

    /* Grab the semaphore so we can get a pointer to the device */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* Look up the device */
        dev_ptr = DEV_Get_Dev_By_Index(node->r2_device.rip2_dev_index);

        /* Set the default mask according to the subnet mask of the device */
        if (dev_ptr)
        {
            default_mask[0] =
                (UINT8)((dev_ptr->dev_addr.dev_netmask >> 24) & 0x000000FF);

            default_mask[1] =
                (UINT8)((dev_ptr->dev_addr.dev_netmask >> 16) & 0x000000FF);

            default_mask[2] =
                (UINT8)((dev_ptr->dev_addr.dev_netmask >> 8) & 0x000000FF);

            default_mask[3] =
                (UINT8)(dev_ptr->dev_addr.dev_netmask & 0x000000FF);
        }

        else
        {
            /* Release the TCP semaphore */
            if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                               __FILE__, __LINE__);

            /* Remove this node from the list since the interface has been
             * deleted.
             */
            DLL_Remove(&RIP2_List, node);

            /* Deallocate the memory */
            if (NU_Deallocate_Memory(node) != NU_SUCCESS)
                NLOG_Error_Log("Failed to deallocate memory", NERR_SEVERE,
                               __FILE__, __LINE__);

            return (-1);
        }

        /* Release the TCP semaphore */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    else
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        return (-1);
    }

    /* point to header information */
    rh = (RIP2_HEADER*)rpkt;

#if (NU_DEBUG_RIP2 == NU_TRUE)

    strcpy(debug_string, "Before");
    ATI_DEBUG_PRINT(debug_string);

    strcpy(debug_string,
        "    -----------------------------------"
        "--------------------------------\n\r");

    ATI_DEBUG_PRINT(debug_string);

#endif

    ep = (RIP2_ENTRY*)&rpkt->af_id;

#if (NU_DEBUG_RIP2 == NU_TRUE)

    strcat(debug_string, "RIP Version: ");
    NU_ITOA(rh->version, temp, 10);
    strcat(debug_string, temp);
    strcat(debug_string, "\n\r");

    ATI_DEBUG_PRINT(debug_string);

#endif

    if (rh->version == 1)
    {
        /* If doing version 1 packets then if the submask or nexthop field
         * is non-zero then do not do any of the packet.
         * (see RFC 1058,  section 3.4)
         */
        for (i = 0; i < pkt_cnt; i++)
        {
            if ( (ep->routetag != 0) ||
                 (memcmp(ep->submask, "\0\0\0\0", IP_ADDR_LEN) != 0) ||
                 (memcmp(ep->nexthop, "\0\0\0\0", IP_ADDR_LEN) != 0) )
            {
#if (NU_DEBUG_RIP2 == NU_TRUE)
                strcpy(debug_string, "Cannot do rip2 packets.\n\r");
                ATI_DEBUG_PRINT(debug_string);
#endif

                NLOG_Error_Log("RIP2_Update_Table received version 1 packet with non-zero subnet or next-hop field",
                               NERR_INFORMATIONAL, __FILE__, __LINE__);

                return (NU_SUCCESS);
            }
            ep++;
        }

        ep = (RIP2_ENTRY*)&rpkt->af_id;
    }

    for (i = 0; i < pkt_cnt; i++)
    {
        /* If af_id is 0xFFFF then it is an auth type entry. This
         * implementation does not support authentication.
         */
        if (ep->af_id == RIP2_FFFF)
        {
            NLOG_Error_Log("RIP2_Update_Table received authenticated packet",
                           NERR_INFORMATIONAL, __FILE__, __LINE__);

#if (NU_DEBUG_RIP2 == NU_TRUE)

            strcat(debug_string, "RIP-2 Authentication not supported");
            ATI_DEBUG_PRINT(debug_string);

#endif
            /* RFC 2453 - section 5.2 - If the router is not configured
             * to authenticate RIP-2 messages, then RIP-1 and
             * unauthenticated RIP-2 messages will be accepted;
             * authenticated RIP-2 messages shall be discarded.
             */
            return (NU_SUCCESS);
        }

        /* Entries with unrecognized family types must be ignored */
        if (INTSWAP(ep->af_id) != NU_FAMILY_IP)
        {
            NLOG_Error_Log("RIP2_Update_Table received routing entry with invalid family",
                           NERR_INFORMATIONAL, __FILE__, __LINE__);

#if (NU_DEBUG_RIP2 == NU_TRUE)

            strcat(debug_string, "RIP2_Update_Table received routing entry with invalid family");
            ATI_DEBUG_PRINT(debug_string);

#endif
            ep++;
            continue;
        }

        /* Extract the next-hop */

        /* RFC 2453 - section 4.4 - The immediate next hop IP
         * address to which packets to the destination specified
         * by this route entry should be forwarded.  Specifying a
         * value of 0.0.0.0 in this field indicates that routing
         * should be via the originator of the RIP advertisement.
         */
        if (IP_ADDR(ep->nexthop) == 0)
            memcpy(next_hop, fromaddr->id.is_ip_addrs, IP_ADDR_LEN);
        else
            memcpy(next_hop, ep->nexthop, IP_ADDR_LEN);

        /* Extract the metric */
        metric = GET32(&ep->metric, 0);

        /* RFC 2453 - section 3.9.2 - is the metric valid (between 1 and 16). */
        if ( (metric > RT_INFINITY) || (metric < 1) )
        {
#if (NU_DEBUG_RIP2 == NU_TRUE)

            strcpy(debug_string, "Invalid metric.\n\r");
            ATI_DEBUG_PRINT(debug_string);

#endif
            NLOG_Error_Log("RIP2_Update_Table packet contains invalid metric",
                           NERR_INFORMATIONAL, __FILE__, __LINE__);

            ep++;
            continue;
        }

        /* RFC 2453 - section 3.9.2 - ... update the metric by
         * adding the cost of the network on which the message
         * arrived.  If the result is greater than infinity,
         * use infinity.
         */
        metric += node->r2_device.rip2_dev_metric;

        /* If address is zero, update the default route for this network. */
        if (memcmp(ep->ip_addr, "\0\0\0\0", IP_ADDR_LEN) == 0)
        {
#if (NU_DEBUG_RIP2 == NU_TRUE)

            strcpy(debug_string, "Update the default route.\n\r");
            ATI_DEBUG_PRINT(debug_string);

#endif
            /* Check if a default route already exists in the system */
            if (NU_Get_Default_Route(NU_FAMILY_IP) == NU_NULL)
            {
                /* Only set this as the default route if the metric is less
                 * than infinity.
                 */
                if (metric < RT_INFINITY)
                {
                    /* Add the default route */
                    if (rh->version == 1)
                    {
                        status = NU_Add_Route(ep->ip_addr, default_mask,
                                              next_hop);

                        if (status != NU_SUCCESS)
                        {
                            NLOG_Error_Log("Failed to add route",
                                           NERR_SEVERE, __FILE__, __LINE__);

                            NET_DBG_Notify(status, __FILE__, __LINE__,
                                           NU_Current_Task_Pointer(), NU_NULL);
                        }
                    }

                    else
                    {
                        status = NU_Add_Route(ep->ip_addr, ep->submask,
                                              next_hop);

                        if (status != NU_SUCCESS)
                        {
                            NLOG_Error_Log("Failed to add route",
                                           NERR_SEVERE, __FILE__, __LINE__);

                            NET_DBG_Notify(status, __FILE__, __LINE__,
                                           NU_Current_Task_Pointer(), NU_NULL);
                        }
                    }

                    memset(&updated_route, -1, sizeof(UPDATED_ROUTE_NODE));

                    /* Set the new metric */
                    updated_route.urt_metric.urt4_metric = (INT32)metric;

                    /* Set the new route tag */
                    updated_route.urt_routetag = GET8(&ep->routetag, 0);

                    /* Update the metric of the default route */
                    status = NU_Update_Route(ep->ip_addr, next_hop, &updated_route,
                                             NU_FAMILY_IP);

                    if (status != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to update route",
                                       NERR_SEVERE, __FILE__, __LINE__);

                        NET_DBG_Notify(status, __FILE__, __LINE__,
                                       NU_Current_Task_Pointer(), NU_NULL);
                    }
                }
            }
        }

        else
        {
            /* RFC 2453 - section 3.9.2 - is the destination address valid
             * (e.g., unicast; not net 0 or 127)
             */
            c = RIP2_IsNetworkClass_ABC(IP_ADDR(ep->ip_addr));

            if (c == 0)
            {
#if (NU_DEBUG_RIP2 == NU_TRUE)

                strcpy(debug_string, "Invalid destination address.\n\r");
                ATI_DEBUG_PRINT(debug_string);

#endif
                NLOG_Error_Log("RIP2_Update_Table packet contains invalid destination address",
                               NERR_INFORMATIONAL, __FILE__, __LINE__);

                ep++;
                continue;
            }

            if (RIP2_IsBroadcast_Address(ep->ip_addr, c) != 0)
            {
#if (NU_DEBUG_RIP2 == NU_TRUE)

                strcpy(debug_string, "Invalid destination address.\n\r");
                ATI_DEBUG_PRINT(debug_string);

#endif
                NLOG_Error_Log("RIP2_Update_Table packet contains invalid destination address",
                               NERR_INFORMATIONAL, __FILE__, __LINE__);

                ep++;
                continue;
            }

            /* RFC 2453 - section 3.9.2 - Check to see whether there is
             * already an explicit route for the destination address.
             * If there is no such route, add this route to the routing
             * table, unless the metric is infinity (there is no point in
             * adding a route which is unusable).
             */
            current_rt_entry =
                (RTAB4_ROUTE_ENTRY*)NU_Find_Route_By_Gateway(ep->ip_addr,
                                                             next_hop,
                                                             NU_FAMILY_IP,
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
                if (memcmp(current_rt_entry->rt_route_node->rt_ip_addr,
                           ep->ip_addr, IP_ADDR_LEN) != 0)
                {
                    /* The network route is better than the host route */
                    if ( (memcmp(current_rt_entry->rt_route_node->rt_ip_addr,
                                 "\0\0\0\0", IP_ADDR_LEN) != 0) &&
                         (current_rt_entry->rt_entry_parms.rt_parm_metric <= metric) )
                    {
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
                /* Extract the subnet mask */
                if ( (rh->version == 1) || (IP_ADDR(ep->submask) == IP_ADDR_ANY))
                    memcpy(subnet_mask, default_mask, IP_ADDR_LEN);
                else
                    memcpy(subnet_mask, ep->submask, IP_ADDR_LEN);

#if (NU_DEBUG_RIP2 == NU_TRUE)

                strcpy(debug_string, "ADDING NEW ROUTE FOR: Address: ");
                NU_ITOA(ep->ip_addr[0], temp, 10);
                strcat(debug_string, temp);
                strcat(debug_string, ".");
                NU_ITOA(ep->ip_addr[1], temp, 10);
                strcat(debug_string, temp);
                strcat(debug_string, ".");
                NU_ITOA(ep->ip_addr[2], temp, 10);
                strcat(debug_string, temp);
                strcat(debug_string, ".");
                NU_ITOA(ep->ip_addr[3], temp, 10);
                strcat(debug_string, temp);
                strcat(debug_string, " -- ");
                NU_ITOA(subnet_mask[0], temp, 10);
                strcat(debug_string, temp);
                strcat(debug_string, ".");
                NU_ITOA(subnet_mask[1], temp, 10);
                strcat(debug_string, temp);
                strcat(debug_string, ".");
                NU_ITOA(subnet_mask[2], temp, 10);
                strcat(debug_string, temp);
                strcat(debug_string, ".");
                NU_ITOA(subnet_mask[3], temp, 10);
                strcat(debug_string, temp);
                strcat(debug_string, " -- ");
                NU_ITOA((int)metric, temp, 10);
                strcat(debug_string, temp);
                strcat(debug_string, "\n\r");

                ATI_DEBUG_PRINT(debug_string);
#endif

                /* Add the route to the routing table */
                ret = NU_Add_Route(ep->ip_addr, subnet_mask, next_hop);

                if (ret < 0)
                {
#if (NU_DEBUG_RIP2 == NU_TRUE)

                    strcpy(debug_string, "Could not add route to route table.\n\r");
                    ATI_DEBUG_PRINT(debug_string);

#endif
                    NLOG_Error_Log("RIP2_Update_Table could not create new route node",
                                   NERR_INFORMATIONAL, __FILE__, __LINE__);

                    NET_DBG_Notify(ret, __FILE__, __LINE__,
                                   NU_Current_Task_Pointer(), NU_NULL);

                    return (ret);
                }

                else
                {
                    /* Set up the modified parameters of the route entry */
                    updated_route.urt_metric.urt4_metric = (INT32)metric;
                    updated_route.urt_flags = RT_UP | RT_CHANGED | RT_RIP2;

                    /* Set the new route tag */
                    updated_route.urt_routetag = GET8(&ep->routetag, 0);

                    /* Update the parameters of the route that cannot be specified
                     * using NU_Add_Route.
                     */
                    status = NU_Update_Route(ep->ip_addr, next_hop, &updated_route,
                                             NU_FAMILY_IP);

                    if (status != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to update route",
                                       NERR_SEVERE, __FILE__, __LINE__);

                        NET_DBG_Notify(status, __FILE__, __LINE__,
                                       NU_Current_Task_Pointer(), NU_NULL);
                    }

                    /* Signal the output process to trigger an update */
                    RIP2_Triggered_Update = 1;
                }
            }

            /* If there is a route */
            else if (current_rt_entry)
            {
                /* RFC 2453 - section 3.9.2 - If there is an existing route,
                 * compare the next-hop address to the address of the router
                 * from which the datagram came.  If this datagram is from the
                 * same router as the existing route, reinitialize the timeout.
                 */
                if (memcmp(next_hop, fromaddr->id.is_ip_addrs, IP_ADDR_LEN) == 0)
                {
                    /* Reinitialize the timeout. */
                    updated_route.urt_age = 0;

                    /* If the advertised metric is infinity, set the metric of
                     * the route to infinity and time out the route.
                     */
                    if (GET32(&ep->metric, 0) == RT_INFINITY)
                        metric = RT_INFINITY;

                    /* If the new metric is different than the old one do the
                     * following actions:
                     */
                    if (metric != current_rt_entry->rt_entry_parms.rt_parm_metric)
                    {
                        /* Adopt the route from the datagram (i.e., put the new
                         * metric in and adjust the next hop address, if necessary).
                         */
                        updated_route.urt_metric.urt4_metric = (INT32)metric;

                        /* Set the new route tag */
                        updated_route.urt_routetag = GET8(&ep->routetag, 0);

                        /* Set the route change flag and signal the output process
                         * to trigger an update.  Also, flag the route as UP in case
                         * the deletion process is in progress for this route.
                         */
                        updated_route.urt_flags =
                            (INT32)(current_rt_entry->rt_entry_parms.rt_parm_flags |
                                    RT_CHANGED | RT_UP);

                        /* RFC 2453 - section 3.9.2 - If the new metric is infinity,
                         * the deletion process begins for the route, which is no
                         * longer used for routing packets.
                         */
                        if (metric == RT_INFINITY)
                        {
                            /* Set the age to an expired time */
                            updated_route.urt_age =
                                (INT32)(NU_Retrieve_Clock() / NU_PLUS_Ticks_Per_Second);
                        }

                        RIP2_Triggered_Update = 1;
                    }

#if (NU_DEBUG_RIP2 == NU_TRUE)

                    strcpy(debug_string, "UPDATING ROUTE FOR: Address: ");
                    NU_ITOA(current_rt_entry->rt_route_node->rt_ip_addr[0], temp, 10);
                    strcat(debug_string, temp);
                    strcat(debug_string, ".");
                    NU_ITOA(current_rt_entry->rt_route_node->rt_ip_addr[1], temp, 10);
                    strcat(debug_string, temp);
                    strcat(debug_string, ".");
                    NU_ITOA(current_rt_entry->rt_route_node->rt_ip_addr[2], temp, 10);
                    strcat(debug_string, temp);
                    strcat(debug_string, ".");
                    NU_ITOA(current_rt_entry->rt_route_node->rt_ip_addr[3], temp, 10);
                    strcat(debug_string, temp);
                    strcat(debug_string, " -- ");
                    NU_ITOA(current_rt_entry->rt_route_node->rt_submask[0], temp, 10);
                    strcat(debug_string, temp);
                    strcat(debug_string, ".");
                    NU_ITOA(current_rt_entry->rt_route_node->rt_submask[1], temp, 10);
                    strcat(debug_string, temp);
                    strcat(debug_string, ".");
                    NU_ITOA(current_rt_entry->rt_route_node->rt_submask[2], temp, 10);
                    strcat(debug_string, temp);
                    strcat(debug_string, ".");
                    NU_ITOA(current_rt_entry->rt_route_node->rt_submask[3], temp, 10);
                    strcat(debug_string, temp);
                    strcat(debug_string, " -- ");
                    NU_ITOA((int)updated_route.urt_metric.urt4_metric, temp, 10);
                    strcat(debug_string, temp);
                    strcat(debug_string, "\n\r");

                    ATI_DEBUG_PRINT(debug_string);
#endif

                    /* RFC 2453 - section 3.9.2 - Note that the deletion process
                     * is started only when the metric is first set to infinity.
                     * If the metric was already infinity, then a new deletion
                     * process is not started.
                     */
                    if ( (metric < RT_INFINITY) ||
                         (current_rt_entry->rt_entry_parms.rt_parm_metric < RT_INFINITY) )
                    {
                        /* Flag the route as up in case the deletion process is in
                         * progress for this route.
                         */
                        updated_route.urt_flags =
                            (INT32)(current_rt_entry->rt_entry_parms.rt_parm_flags | RT_UP);

                        status = NU_Update_Route(current_rt_entry->rt_route_node->rt_ip_addr,
                                                 next_hop, &updated_route,
                                                 NU_FAMILY_IP);

                        if (status != NU_SUCCESS)
                        {
                            NLOG_Error_Log("Failed to update route",
                                           NERR_SEVERE, __FILE__, __LINE__);

                            NET_DBG_Notify(status, __FILE__, __LINE__,
                                           NU_Current_Task_Pointer(), NU_NULL);
                        }
                    }
                }

                /* If the new metric is lower than the old one */
                else if (metric < current_rt_entry->rt_entry_parms.rt_parm_metric)
                {
                    /*  - Adopt the route from the datagram (i.e., put the new
                     *    metric in and adjust the next hop address, if necessary).
                     */
                    updated_route.urt_metric.urt4_metric = (INT32)metric;

                    /*  - Set the route change flag and signal the output process
                     *    to trigger an update
                     *
                     *  Also, set the route to UP in case the deletion process
                     *  has began for the route.
                     */
                    updated_route.urt_flags =
                        (INT32)(current_rt_entry->rt_entry_parms.rt_parm_flags |
                                RT_CHANGED | RT_UP);

                    /* Set the new route tag */
                    updated_route.urt_routetag = GET8(&ep->routetag, 0);

                    RIP2_Triggered_Update = 1;

#if (NU_DEBUG_RIP2 == NU_TRUE)

                    strcpy(debug_string, "UPDATING ROUTE FOR: Address: ");
                    NU_ITOA(current_rt_entry->rt_route_node->rt_ip_addr[0], temp, 10);
                    strcat(debug_string, temp);
                    strcat(debug_string, ".");
                    NU_ITOA(current_rt_entry->rt_route_node->rt_ip_addr[1], temp, 10);
                    strcat(debug_string, temp);
                    strcat(debug_string, ".");
                    NU_ITOA(current_rt_entry->rt_route_node->rt_ip_addr[2], temp, 10);
                    strcat(debug_string, temp);
                    strcat(debug_string, ".");
                    NU_ITOA(current_rt_entry->rt_route_node->rt_ip_addr[3], temp, 10);
                    strcat(debug_string, temp);
                    strcat(debug_string, " -- ");
                    NU_ITOA(current_rt_entry->rt_route_node->rt_submask[0], temp, 10);
                    strcat(debug_string, temp);
                    strcat(debug_string, ".");
                    NU_ITOA(current_rt_entry->rt_route_node->rt_submask[1], temp, 10);
                    strcat(debug_string, temp);
                    strcat(debug_string, ".");
                    NU_ITOA(current_rt_entry->rt_route_node->rt_submask[2], temp, 10);
                    strcat(debug_string, temp);
                    strcat(debug_string, ".");
                    NU_ITOA(current_rt_entry->rt_route_node->rt_submask[3], temp, 10);
                    strcat(debug_string, temp);
                    strcat(debug_string, " -- ");
                    NU_ITOA((int)metric, temp, 10);
                    strcat(debug_string, temp);
                    strcat(debug_string, "\n\r");

                    ATI_DEBUG_PRINT(debug_string);
#endif
                    /* Update the route */
                    status = NU_Update_Route(current_rt_entry->rt_route_node->rt_ip_addr,
                                             next_hop, &updated_route,
                                             NU_FAMILY_IP);

                    if (status != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to update route",
                                       NERR_SEVERE, __FILE__, __LINE__);

                        NET_DBG_Notify(status, __FILE__, __LINE__,
                                       NU_Current_Task_Pointer(), NU_NULL);
                    }
                }
            }
        }

        ep++;
    }

#if (NU_DEBUG_RIP2 == NU_TRUE)
    strcpy(debug_string,
        "    -----------------------------------"
        "--------------------------------\n\r");
    ATI_DEBUG_PRINT(debug_string);

    strcpy(debug_string, "After ");
    ATI_DEBUG_PRINT(debug_string);
#endif

    return (NU_SUCCESS);

} /* RIP2_Update_Table */

/*************************************************************************
*
*   FUNCTION
*
*       RIP2_IsNetworkClass_ABC
*
*   DESCRIPTION
*
*       If IP address is A, B or C return true.
*       A class A address, the 32 bit is off.
*       A class B address, the 32 bit is on and 31 bit is off.
*       A class C address, the 32 bit is on and 31 bit is on.
*
*   INPUTS
*
*       *ip_addr                A pointer to the IP address
*
*   OUTPUTS
*
*       0                       Unknown Class Network Address
*       1                       Class A Address
*       2                       Class B Address
*       3                       Class C Address
*
*************************************************************************/
STATIC INT16 RIP2_IsNetworkClass_ABC(UINT32 ip_addr)
{
    INT16   ret;

#if (NU_DEBUG_RIP2 == NU_TRUE)

    strcpy(debug_string, "in RIP2_IsNetworkClass_ABC\n\r");
    ATI_DEBUG_PRINT(debug_string);

#endif

    /* Multicast addresses and the loopback address cannot be advertised
     * by RIP routers.
     */
    if ( (IP_MULTICAST_ADDR(ip_addr)) ||
         ((ip_addr & 0xff000000UL) == 0x7f000000UL) )
        ret = 0;

    else if (IP_CLASSA_ADDR(ip_addr))
        ret = 1;

    else if (IP_CLASSB_ADDR(ip_addr))
        ret = 2;

    else if (IP_CLASSC_ADDR(ip_addr))
        ret = 3;

    else
        ret = 0;

    return (ret);

} /* RIP2_IsNetworkClass_ABC */

/*************************************************************************
*
*   FUNCTION
*
*       RIP2_IsBroadcast_Address
*
*   DESCRIPTION
*
*       If IP address is a broadcast address return true.
*
*   INPUTS
*
*       *ip_addr                A pointer to the IP address
*       c                       The target class
*
*   OUTPUTS
*
*       0                       Unknown Class Network Address
*       1                       Class A and Broadcast Address
*       2                       Class B and Broadcast Address
*       3                       Class C and Broadcast Address
*
*************************************************************************/
STATIC INT16 RIP2_IsBroadcast_Address(const UINT8 *ip_addr, INT16 c )
{
    INT16   ret = 0;
    UINT32  addr;

#if (NU_DEBUG_RIP2 == NU_TRUE)

    strcpy(debug_string, "in RIP2_IsBroadcast_Address\n\r");
    ATI_DEBUG_PRINT(debug_string);

#endif

    addr = IP_ADDR(ip_addr);

    switch (c)
    {
    case 1:                 /* is a class A network address */

        if ( ((addr & 0x00ffffffUL) == CLASSA_BROADCAST) )
            ret = 1;

        break;

    case 2:                 /* is a class B network address */

        if ( ((addr & 0x0000ffff) == CLASSB_BROADCAST))
            ret = 2;

        break;

    case 3:                 /* is a class C network address */

        if ( ((addr & 0x000000ff) == CLASSC_BROADCAST) )
            ret = 3;

        break;

    default:
        break;
    }

    return (ret);

} /* RIP2_IsBroadcast_Address */

/*************************************************************************
*
*   FUNCTION
*
*       RIP2_IsFromMe
*
*   DESCRIPTION
*
*       If the address is from one of my interfaces, return true.
*
*   INPUTS
*
*       dev_index               The interface index of the device.
*       *addr                   A pointer to the address structure
*
*   OUTPUTS
*
*       1                       The address is from one of my interfaces
*       0                       The address is not from one of my interfaces
*       -1                      Could not obtain the semaphore.
*
*************************************************************************/
STATIC INT16 RIP2_IsFromMe(UINT32 dev_index, const struct addr_struct *addr)
{
    DV_DEVICE_ENTRY     *dev_ptr;
    INT16               status;

#if (NU_DEBUG_RIP2 == NU_TRUE)

    strcpy(debug_string, "in RIP2_IsFromMe\n\r");
    ATI_DEBUG_PRINT(debug_string);

#endif

    /* Grab the semaphore so we can get a pointer to the device */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* Look up the device the user wishes to use RIP with. */
        dev_ptr = DEV_Get_Dev_By_Index(dev_index);

        /* Search the list of addresses on the device to determine if the
         * target address is one of the device's addresses.
         */
        if ( (dev_ptr) &&
             (DEV_Find_Target_Address(dev_ptr, IP_ADDR(addr->id.is_ip_addrs))) )
            status = 1;

        else
            status = 0;

        /* Release the TCP semaphore */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Could not get the semaphore - return an error */
    else
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        status = -1;
    }

    return (status);

} /* RIP2_IsFromMe */

/*************************************************************************
*
*   FUNCTION
*
*       RIP2_Find_Entry
*
*   DESCRIPTION
*
*       Finds a RIP-enabled device by IP address.
*
*   INPUTS
*
*       *addr                   A pointer to the IP address
*
*   OUTPUTS
*
*       RIP2_LIST_NODE*         A pointer to the requested entry.
*       NU_NULL                 No matching device exists.
*
*************************************************************************/
STATIC RIP2_LIST_NODE *RIP2_Find_Entry(const UINT8 *addr)
{
    UINT32              addr1, addr2;
    RIP2_LIST_NODE      *node, *saved_node;
    DEV_IF_ADDR_ENTRY   *dev_addr_entry;
    DV_DEVICE_ENTRY     *dev_ptr;

#if (NU_DEBUG_RIP2 == NU_TRUE)

    strcpy(debug_string, "in RIP2_Find_Entry\n\r");
    ATI_DEBUG_PRINT(debug_string);

#endif

    /* Grab the semaphore so we can get a pointer to the device */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Unable to obtain TCP semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        return (NU_NULL);
    }

    for (node = RIP2_List.r2_head; node; node = node->r2_next)
    {
        /* Look up the device the user wishes to use RIP with. */
        dev_ptr = DEV_Get_Dev_By_Index(node->r2_device.rip2_dev_index);

        /* Check that the device exists */
        if (dev_ptr == NU_NULL)
        {
            NLOG_Error_Log("RIP2_Find_Entry device invalid",
                           NERR_RECOVERABLE, __FILE__, __LINE__);

            /* Save a pointer to the previous node */
            saved_node = node->r2_prev;

            /* Remove this node from the list since the interface has been
             * deleted.
             */
            DLL_Remove(&RIP2_List, node);

            /* Deallocate the memory */
            if (NU_Deallocate_Memory(node) != NU_SUCCESS)
                NLOG_Error_Log("Failed to deallocate memory", NERR_SEVERE,
                               __FILE__, __LINE__);

            /* Set node to the previous node so the loop gets the next
             * node in the list.
             */
            node = saved_node;

            continue;
        }

        /* Get a pointer to the first address in the list of addresses
         * for the device.
         */
        dev_addr_entry = dev_ptr->dev_addr.dev_addr_list.dv_head;

        /* Traverse the list of addresses */
        while (dev_addr_entry)
        {
            addr1 = IP_ADDR(addr);
            addr2 = dev_addr_entry->dev_entry_netmask;

            addr1 = (addr1 & addr2);

            /* If this address is on the same subnet as the current
             * address in the list of addresses for the device,
             * exit the loop.
             */
            if ((addr2 & dev_addr_entry->dev_entry_ip_addr) == addr1)
                break;

            /* Get a pointer to the next address in the list of addresses
             * for the device.
             */
            dev_addr_entry = dev_addr_entry->dev_entry_next;
        }

        /* If a matching address was found, exit the loop */
        if (dev_addr_entry)
            break;
    }

    /* Release the TCP semaphore */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

    return (node);

} /* RIP2_Find_Entry */

/*************************************************************************
*
*   FUNCTION
*
*       RIP2_Delete_Old
*
*   DESCRIPTION
*
*       Deletes old or unreachable routes from routing table.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       > 0                     Index to the next node
*       0                       There are no nodes to delete
*       < 0                     There was an error
*
*************************************************************************/
STATIC INT RIP2_Delete_Old(VOID)
{
    INT         i;
    UINT32      curtime;
    UINT8       gateway[IP_ADDR_LEN];
    STATUS      status;

#if (NU_DEBUG_RIP2 == NU_TRUE)

    strcpy(debug_string, "in RIP2_Delete_Old\n\r");
    ATI_DEBUG_PRINT(debug_string);

#endif

    /* If there are no nodes to delete, return */
    if (RIP2_Delete_Num <= 0)
    {
        RIP2_Delete_Num = 0;
        return (0);
    }

    curtime = NU_Retrieve_Clock();

    /* Delete in reverse order */
    for (i = (RIP2_Delete_Num - 1); i >= 0; i--)
    {
        /* RFC 2453 - section 3.8 - Should a new route to this network be
         * established while the garbage-collection timer is running, the
         * new route will replace the one that is about to be deleted.
         * In this case the garbage-collection timer must be cleared.
         */
        if ( ((curtime - delete_nodes[i]->rt_entry_parms.rt_parm_clock) >= RIP2_RT_LIFE_TIME) ||
             (delete_nodes[i]->rt_entry_parms.rt_parm_metric >= RT_INFINITY) )
        {
            PUT32(gateway, 0, delete_nodes[i]->rt_gateway_v4.sck_addr);

            /* Delete the entry from the NET routing table */
            status = NU_Delete_Route2(delete_nodes[i]->rt_route_node->rt_ip_addr,
                                      gateway, NU_FAMILY_IP);

            if (status != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to delete route",
                               NERR_SEVERE, __FILE__, __LINE__);

                NET_DBG_Notify(status, __FILE__, __LINE__,
                               NU_Current_Task_Pointer(), NU_NULL);
            }
        }

        delete_nodes[i] = 0;
    }

    RIP2_Delete_Num = 0;

    return (i);

} /* RIP2_Delete_Old */

/*************************************************************************
*
*   FUNCTION
*
*       RIP2_Gather_Old
*
*   DESCRIPTION
*
*       This function traverses the entire NET routing table.  If an
*       entry is found that has not been used in RT_LIFE_TIME seconds or
*       the route metric has been set to INFINITY, the route is placed
*       on a list to be deleted from the NET routing table.
*
*   INPUTS
*
*       curtime                 Current time
*       flag                    Flags
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
STATIC VOID RIP2_Gather_Old(UINT32 curtime, INT16 flag)
{
    RTAB4_ROUTE_ENTRY   *current_route;
    UPDATED_ROUTE_NODE  updated_route;
    UINT8               next_hop[IP_ADDR_LEN];
    STATUS              status;

#if (NU_DEBUG_RIP2 == NU_TRUE)

    strcpy(debug_string, "in RIP2_Gather_Old\n\r");
    ATI_DEBUG_PRINT(debug_string);

#endif

    memset(&updated_route, -1, sizeof(UPDATED_ROUTE_NODE));

    if (flag)
        RIP2_Delete_Num = 0;

    /* Get a pointer to the first route in the route table */
    current_route =
        (RTAB4_ROUTE_ENTRY*)NU_Find_Next_Route_Entry(NU_NULL, NU_FAMILY_IP);

    /* Check each route in the table to determine if it should be
     * deleted.
     */
    while (current_route)
    {
        PUT32(next_hop, 0, current_route->rt_gateway_v4.sck_addr);

        /* If the route has not been used in RT_LIFE_TIME amount of time
         * or the metric is infinity, flag the node to be deleted.  STATIC and
         * SILENT routes are not affected by RIP2.
         */
        if ( (((curtime - current_route->rt_entry_parms.rt_parm_clock) >= RIP2_RT_LIFE_TIME) ||
              (current_route->rt_entry_parms.rt_parm_metric >= RT_INFINITY)) &&
              (!(current_route->rt_entry_parms.rt_parm_flags & RT_STATIC)) &&
              (!(current_route->rt_entry_parms.rt_parm_flags & RT_SILENT)) )
        {
            /* If the delete list is not full. */
            if (RIP2_Delete_Num < RIP2_MAX_DELETES)
            {
                /* Set the metric for the route to infinity */
                updated_route.urt_metric.urt4_metric = RT_INFINITY;

                /* Set the RT_CHANGE flag */
                updated_route.urt_flags =
                    (INT32)(current_route->rt_entry_parms.rt_parm_flags | RT_CHANGED);

                /* The age must be set to remain the current age;
                 * otherwise, NU_Update_Route will update the age to the
                 * current time.  If it is allowed to do so, the route will
                 * not be deleted by RIP2_Delete_Old.
                 */
                updated_route.urt_age =
                    (INT32)((curtime - current_route->rt_entry_parms.rt_parm_clock) / TICKS_PER_SECOND);

                /* Make the changes to the route */
                status = NU_Update_Route(current_route->rt_route_node->rt_ip_addr,
                                         next_hop, &updated_route, NU_FAMILY_IP);

                if (status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to update route", NERR_SEVERE,
                                   __FILE__, __LINE__);

                    NET_DBG_Notify(status, __FILE__, __LINE__,
                                   NU_Current_Task_Pointer(), NU_NULL);
                }

                delete_nodes[RIP2_Delete_Num] = current_route;
                RIP2_Delete_Num++;
            }

            /* The maximum number of nodes to delete has been put on the
             * delete list.  Stop searching for nodes to delete.
             */
            else
                break;
        }

        /* Get a pointer to the next route */
        current_route =
            (RTAB4_ROUTE_ENTRY*)
            NU_Find_Next_Route_Entry((ROUTE_ENTRY*)current_route,
                                     NU_FAMILY_IP);
    }

} /* RIP2_Gather_Old */

/*************************************************************************
*
*   FUNCTION
*
*       RIP2_Gather_All
*
*   DESCRIPTION
*
*       Gather all route entries in route table.
*
*   INPUTS
*
*       dev_index               Interface index of the device for
*                               which to gather route.
*       *current_addr           A pointer to the address of which to get
*                               the next address.
*       *num                    A pointer to the amount in the packet
*       flags                   A flag to indicate that the packets to be
*                               gathered are for a Triggered Update
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
STATIC VOID RIP2_Gather_All(const UINT32 dev_index, const UINT8 *current_addr,
                            INT *num, INT16 flags)
{
    RTAB4_ROUTE_ENTRY   *current_route = NU_NULL;
    ROUTE_NODE          *default_route;
    RIP2_LIST_NODE      *node;
    UINT8               set_change_flag = 0;

#if (NU_DEBUG_RIP2 == NU_TRUE)

    strcpy(debug_string, "in RIP2_Gather_All\n\r");
    ATI_DEBUG_PRINT(debug_string);

#endif

#if (RIP2_DISABLE_POISONED_REVERSE != NU_TRUE)
    UNUSED_PARAMETER(dev_index);
#endif

    /* If this is a triggered update, check if this is the last node in the
     * system that needs to send out the updates.
     */
    if (flags)
    {
        /* Traverse the list of RIP interfaces.  If there is another
         * interface after this one that will send out an update, do not
         * unset the RT_CHANGED flag in the route; otherwise, the next
         * interface will not send out those routes.
         */
        for (node = RIP2_List.r2_head; node; node = node->r2_next)
        {
            /* If the current entry is this interface, and there is
             * another interface in the list, do not unset the
             * RT_CHANGED flag.
             */
            if ( (node->r2_device.rip2_dev_index == dev_index) &&
                 (!node->r2_next) )
            {
                set_change_flag = 1;
                break;
            }
        }
    }

    /* If the first route is desired, check for a default route */
    if (memcmp(current_addr, "\0\0\0\0", IP_ADDR_LEN) == 0)
    {
        default_route = NU_Get_Default_Route(NU_FAMILY_IP);

        /* If a default route exists, put it in the packet */
        if (default_route)
            current_route = default_route->rt_route_entry_list.rt_entry_head;
    }

    /* Get a pointer to the first route in the route table */
    if (!current_route)
        current_route =
            (RTAB4_ROUTE_ENTRY*)NU_Find_Next_Route(current_addr, NU_FAMILY_IP);

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
         */
        if ( (!(current_route->rt_entry_parms.rt_parm_flags & RT_SILENT))
#if (RIP2_DISABLE_POISONED_REVERSE == NU_TRUE)
             &&
             ((!(current_route->rt_entry_parms.rt_parm_flags & RT_RIP2)) ||
              (current_route->rt_entry_parms.rt_parm_device->dev_index != dev_index))
#endif
             )
        {
            /* Check if we are only gathering changed routes */
            if ( (!flags) ||
                 (current_route->rt_entry_parms.rt_parm_flags & RT_CHANGED) )
            {
                /* If the maximum number of nodes has not been found */
                if (*num < RIP2_MAX_PER_PACKET)
                {
                    /* Add the node to the list of nodes */
                    updated_nodes[*num] = current_route;

                    /* Increment the number of nodes on the list */
                    *num = *num + 1;

                    /* Clear the route changed flag */
                    if (set_change_flag)
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
            (RTAB4_ROUTE_ENTRY*)
            NU_Find_Next_Route(current_route->rt_route_node->rt_ip_addr,
                               NU_FAMILY_IP);
    }

} /* RIP2_Gather_All */

/*************************************************************************
*
*   FUNCTION
*
*       RIP2_Send_Updates
*
*   DESCRIPTION
*
*       Send all updated route table entries since last send.
*
*   INPUTS
*
*       *rpkt                   A pointer to the RIP2 packet
*       entries                 The number of entries
*       *faddr                  A pointer to the destination the address
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
STATIC VOID RIP2_Send_Updates(RIP2_PACKET *rpkt, INT entries,
                              const struct addr_struct *faddr)
{
    SIGNED              ret;
    INT                 i;
    UINT16              len;
    RIP2_ENTRY          *pkt;
    RTAB4_ROUTE_ENTRY   *rn;
    UINT8               next_hop[IP_ADDR_LEN];
    struct addr_struct  baddr;

#if (NU_DEBUG_RIP2 == NU_TRUE)

    strcpy(debug_string, "in RIP2_Send_Updates\n\r");
    ATI_DEBUG_PRINT(debug_string);

#endif

    if (RIP2_Socket < 0)
    {
        NLOG_Error_Log("RIP2_Send_Updates invalid socket",
                       NERR_SEVERE, __FILE__, __LINE__);

        NET_DBG_Notify(NU_INVALID_SOCKET, __FILE__, __LINE__,
                       NU_Current_Task_Pointer(), NU_NULL);

        return;
    }

    if (entries > 0)
    {
        baddr.family = NU_FAMILY_IP;
        baddr.port = faddr->port;

        baddr.id.is_ip_addrs[0] = faddr->id.is_ip_addrs[0];
        baddr.id.is_ip_addrs[1] = faddr->id.is_ip_addrs[1];
        baddr.id.is_ip_addrs[2] = faddr->id.is_ip_addrs[2];
        baddr.id.is_ip_addrs[3] = faddr->id.is_ip_addrs[3];

        baddr.name = "RIPRESP";

        /* RFC 1058 - section 3.4.1 - Once all the entries have been filled
         * in, set the command to response and send the datagram back to
         * the port from which it came.
         */
        rpkt->command = RIP2_RESPONSE;

        /* For each rip struct in RIP2_PACKET, see if route exists and
         * update metric in packet or set metric to RT_INFINITY and send the
         * packet back to sender
         */
        pkt = (RIP2_ENTRY *)&rpkt->af_id;

        for (i = 0; i < entries; i++)
        {
            if (INTSWAP(pkt->af_id) == NU_FAMILY_IP)
            {
                if (IP_ADDR(pkt->nexthop) == 0)
                    memcpy(next_hop, faddr->id.is_ip_addrs, IP_ADDR_LEN);
                else
                    memcpy(next_hop, pkt->nexthop, IP_ADDR_LEN);

                /* RFC 2453 - section 3.9.1 - If the request is for specific
                 * entries, they are looked up in the routing table and the
                 * information is returned as is; no Split Horizon processing
                 * is done.
                 */
                rn =
                    (RTAB4_ROUTE_ENTRY*)NU_Find_Route_By_Gateway(pkt->ip_addr,
                                                                 next_hop,
                                                                 NU_FAMILY_IP,
                                                                 RT_OVERRIDE_METRIC |
                                                                 RT_HOST_MATCH);

                /* RFC 2453 - section 3.9.1 - If there is no explicit route
                 * to the specified destination, put infinity in the metric field.
                 */
                if (rn == NU_NULL)
                    PUT32(&pkt->metric, 0, RT_INFINITY);

                /* RFC 2453 - section 3.9.1 - ... if there is a route, put that
                 * route's metric in the metric field of the RTE.
                 */
                else
                {
                    PUT32(&pkt->metric, 0, rn->rt_entry_parms.rt_parm_metric);
                    rn->rt_entry_parms.rt_parm_refcnt --;
                }
            }

            pkt++;
        }

        len = (UINT16)((entries * RIP_PKT_SIZE) + 4);

        ret = NU_Send_To(RIP2_Socket, (char *)rpkt, len, 0, &baddr,
                         sizeof(baddr));

        if (ret <= 0)
        {
            /* If an ICMP error message was returned, try to send again */
            if (ICMP_ERROR_CODE(ret) == NU_TRUE)
            {
                NLOG_Error_Log("RIP2_Send_Updates NU_Send_To returned ICMP Error Code",
                               NERR_RECOVERABLE, __FILE__, __LINE__);

                ret = NU_Send_To(RIP2_Socket, (char *)rpkt, len, 0, &baddr,
                                 sizeof(baddr));
            }

            if (ret <= 0)
                NLOG_Error_Log("RIP2_Send_Updates NU_Send_To returned negative value",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    return;

} /* RIP2_Send_Updates */

/*************************************************************************
*
*   FUNCTION
*
*       RIP2_Send_Table
*
*   DESCRIPTION
*
*       Send the complete route table.
*
*   INPUTS
*
*       *node                   Pointer to the RIP2 list
*       *faddr                  Pointer to the destination address
*                               structure
*       send_mode               The type of RIP packet to send.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
STATIC VOID RIP2_Send_Table(const RIP2_LIST_NODE *node,
                            const struct addr_struct *faddr, UINT8 send_mode)
{
    struct addr_struct  baddr;

#if (NU_DEBUG_RIP2 == NU_TRUE)

    strcpy(debug_string, "in RIP2_Send_Table\n\r");
    ATI_DEBUG_PRINT(debug_string);

#endif

    /* do not care which address message came from so set to zero */
    baddr.family = NU_FAMILY_IP;
    baddr.port = faddr->port;

    baddr.id.is_ip_addrs[0] = faddr->id.is_ip_addrs[0];
    baddr.id.is_ip_addrs[1] = faddr->id.is_ip_addrs[1];
    baddr.id.is_ip_addrs[2] = faddr->id.is_ip_addrs[2];
    baddr.id.is_ip_addrs[3] = faddr->id.is_ip_addrs[3];

    baddr.name = "RIPRESP";

    RIP2_Send_Routes(node, 0, &baddr, send_mode);

    return;

} /* RIP2_Send_Table */

/*************************************************************************
*
*   FUNCTION
*
*       RIP2_Broadcast
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
*       None.
*
*************************************************************************/
STATIC VOID RIP2_Broadcast(INT16 flags)
{
    struct addr_struct  baddr;
    RIP2_LIST_NODE      *node, *saved_node;
    UINT8               send_mode;
    UINT8               dev_addr[IP_ADDR_LEN];
    DV_DEVICE_ENTRY     *dev_ptr;

#if (NU_DEBUG_RIP2 == NU_TRUE)

    strcpy(debug_string, "in RIP2_Broadcast\n\r");
    ATI_DEBUG_PRINT(debug_string);

#endif

    /* do not care which address message came from so set to zero */
    baddr.family = NU_FAMILY_IP;
    baddr.port = RIP_PORT;
    baddr.name = "RIPBCST";

    /* Send a RIP2 broadcast out each interface for all routes not on
     * that interface.
     */
    for (node = RIP2_List.r2_head; node; node = node->r2_next)
    {
        if (node->r2_sendmode == SEND_NONE)
            continue;

        /* Grab the semaphore so we can get a pointer to the device */
        if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
        {
            NLOG_Error_Log("Unable to obtain TCP semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);

            continue;
        }

        /* Look up the device the user wishes to use RIP with. */
        dev_ptr = DEV_Get_Dev_By_Index(node->r2_device.rip2_dev_index);

        /* Copy the IP address of the device */
        if (dev_ptr)
        {
            /* Ensure that the device is UP */
            if (dev_ptr->dev_flags & DV_UP)
                PUT32(dev_addr, 0,
                      dev_ptr->dev_addr.dev_addr_list.dv_head->dev_entry_ip_addr);

            /* Otherwise, the interface does not have a valid IP address.
             * Go on to the next interface in the list.
             */
            else
            {
                /* Release the TCP semaphore */
                if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                                   __FILE__, __LINE__);

                continue;
            }
        }

        else
        {
            /* Release the TCP semaphore */
            if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                               __FILE__, __LINE__);

            /* Save a pointer to the previous node */
            saved_node = node->r2_prev;

            /* Remove this node from the list since the interface has been
             * deleted.
             */
            DLL_Remove(&RIP2_List, node);

            /* Deallocate the memory */
            if (NU_Deallocate_Memory(node) != NU_SUCCESS)
                NLOG_Error_Log("Failed to deallocate memory", NERR_SEVERE,
                               __FILE__, __LINE__);

            /* Set node to the previous node so the loop gets the next
             * node in the list.
             */
            node = saved_node;

            continue;
        }

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)
        /* Create the network broadcast address */
        if (node->r2_sendmode & SEND_RIP1)
#endif
        {
            /* If the RIP1-Compatibility switch is enabled, send RIP-2
             * via broadcast.
             */
            if (node->r2_sendmode & SEND_RIP2)
                send_mode = SEND_RIP2;
            else
                send_mode= SEND_RIP1;

            /* If this is not a Point-to-Point link, create the network
             * broadcast address for the network.
             */
            if (!(dev_ptr->dev_flags & DV_POINTTOPOINT))
            {
                PUT32(baddr.id.is_ip_addrs, 0,
                      dev_ptr->dev_addr.dev_addr_list.dv_head->dev_entry_ip_addr
                      | (~dev_ptr->dev_addr.dev_netmask));

                /* Release the TCP semaphore */
                if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                                   __FILE__, __LINE__);

                /* Set this device as the Broadcast Interface */
                if (NU_Setsockopt_IP_BROADCAST_IF(RIP2_Socket,
                                                  dev_addr) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to set broadcast interface",
                                   NERR_SEVERE, __FILE__, __LINE__);
            }

            /* Otherwise, unicast this packet to the other side of the
             * link.
             */
            else
            {
                PUT32(baddr.id.is_ip_addrs, 0,
                      dev_ptr->dev_addr.dev_dst_ip_addr);

                /* Release the TCP semaphore */
                if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                                   __FILE__, __LINE__);
            }
        }

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)

        /* Store the multicast address */
        else
        {
            /* Release the TCP semaphore */
            if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                               __FILE__, __LINE__);

            memcpy(baddr.id.is_ip_addrs, RIP2_Multi_Addr, IP_ADDR_LEN);
            send_mode = SEND_RIP2;

            /* Set this device as the Multicast Interface */
            if (NU_Setsockopt_IP_MULTICAST_IF(RIP2_Socket,
                                              dev_addr) != NU_SUCCESS)
                NLOG_Error_Log("Failed to set multicast interface",
                               NERR_SEVERE, __FILE__, __LINE__);
        }
#endif

        RIP2_Send_Routes(node, flags, &baddr, send_mode);
    }

    return;

} /* RIP2_Broadcast */

/*************************************************************************
*
*   FUNCTION
*
*       RIP2_Send_Routes
*
*   DESCRIPTION
*
*       This function sends all routes to the specified address.
*
*   INPUTS
*
*       *node                   A pointer to the RIP2_LIST_NODE out which
*                               to send the packet.
*       *baddr                  The IP address of the destination of the
*                               packet.
*       flags                   Value indicating whether this is a
*                               Triggered Update.
*       send_mode               The type of RIP packets to send.
*
*   OUTPUTS
*
*       VOID
*
*************************************************************************/
STATIC VOID RIP2_Send_Routes(const RIP2_LIST_NODE *node, INT16 flags,
                             const struct addr_struct *baddr, UINT8 send_mode)
{
    INT                 z;
    RIP2_ENTRY          *pkt;
    RIP2_PACKET         *rp;
    INT                 count = 0;
    UINT8               last_entry_addr[IP_ADDR_LEN] = {0, 0, 0, 0};
    UINT16              len;
    INT32               ret;
    DV_DEVICE_ENTRY     *dev_ptr;
    UINT32              iface_subnet = IP_ADDR_ANY,
                        iface_addr = IP_ADDR_ANY;
    UINT8               is_ppp = NU_FALSE;

#if (RIP2_DISABLE_POISONED_REVERSE == NU_FALSE)
    INT                 not_added = 0;
#endif
#if (NU_DEBUG_RIP2 == NU_TRUE)
    CHAR                temp[10];

    strcpy(debug_string, "in RIP2_Send_Routes\n\r");
    ATI_DEBUG_PRINT(debug_string);
#endif

    if (RIP2_Socket < 0)
    {
        NLOG_Error_Log("RIP2_Send_Table invalid socket",
                       NERR_SEVERE, __FILE__, __LINE__);

        NET_DBG_Notify(NU_INVALID_SOCKET, __FILE__, __LINE__,
                       NU_Current_Task_Pointer(), NU_NULL);

        return;
    }

    /* Grab the semaphore so we can get a pointer to the device */
    if (NU_Obtain_Semaphore(&TCP_Resource,
                            NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Unable to obtain TCP semaphore",
                       NERR_SEVERE, __FILE__, __LINE__);

        return;
    }

    /* Look up the device the user wishes to use RIP with. */
    dev_ptr = DEV_Get_Dev_By_Index(node->r2_device.rip2_dev_index);

    /* Copy the IP address of the device.  We are going to assume that it
     * won't change by the time we transmit this packet.  Otherwise, we
     * would have to grab and release the semaphore every time through
     * the proceeding loop.
     */
    if ( (dev_ptr) && (dev_ptr->dev_flags & DV_UP) )
    {
        if (!(dev_ptr->dev_flags & DV_POINTTOPOINT))
        {
            iface_addr = dev_ptr->dev_addr.dev_ip_addr;
            iface_subnet = dev_ptr->dev_addr.dev_netmask;
        }

        /* If this is a PPP link, set the flag */
        else
            is_ppp = NU_TRUE;
    }

    else
    {
        NLOG_Error_Log("RIP interface no longer valid",
                       NERR_RECOVERABLE, __FILE__, __LINE__);

        /* Release the TCP semaphore */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);

        return;
    }

    /* Release the TCP semaphore */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

    rp = (RIP2_PACKET*)RIP2_Buffer;
    rp->command = RIP2_RESPONSE;

    /* Determine the version of RIP */
    if (send_mode == SEND_RIP1)
        rp->version = RIP1_VERSION;
    else
        rp->version = RIP2_VERSION;

    rp->unused = 0;

    /* Build a packet for each 25 route entries in the route table.
     * If there are more than 25 entries, send multiple packets.
     */
    do
    {
        count = 0;

        RIP2_Gather_All(node->r2_device.rip2_dev_index, last_entry_addr,
                        &count, flags);

        /* If routes were gathered, put them in the packet and transmit the
         * packet.
         */
        if (count > 0)
        {
            /* Get a pointer to the head of the packet */
            pkt = (RIP2_ENTRY*)&RIP2_Buffer[4];

            /* Fill the packet with the route entries */
            for (z = 0; z < count; z++)
            {
#if (RIP2_DISABLE_POISONED_REVERSE == NU_FALSE)

                /* RFC 2453 - section 3.4.3 - In particular, it is never useful
                 * to claim reachability for a destination network to the
                 * neighbor(s) from which the route was learned ... "Split horizon
                 * with poisoned reverse" includes such routes in updates, but
                 * sets their metrics to infinity.
                 */
                if ( (updated_nodes[z]->rt_entry_parms.rt_parm_flags & RT_RIP2) &&
                     (updated_nodes[z]->rt_entry_parms.rt_parm_device->dev_index ==
                      node->r2_device.rip2_dev_index) )
                {
                    /* RFC 2453 - section 3.10.1 - If, after Split Horizon
                     * processing for a given network, a changed route will
                     * appear unchanged on that network (e.g., it appears
                     * with an infinite metric), the route need not be sent.
                     */
                    if ( (flags) &&
                         (updated_nodes[z]->rt_entry_parms.rt_parm_metric == RT_INFINITY) )
                    {
                        not_added++;
                        continue;
                    }
                    else
                        PUT32(&pkt->metric, 0, RT_INFINITY);
                }
                else
#endif
                    PUT32(&pkt->metric, 0, updated_nodes[z]->rt_entry_parms.rt_parm_metric);

                pkt->af_id = INTSWAP(NU_FAMILY_IP);

                pkt->routetag =
                    INTSWAP(updated_nodes[z]->rt_entry_parms.rt_parm_routetag);

                memcpy(pkt->ip_addr,
                       updated_nodes[z]->rt_route_node->rt_ip_addr, IP_ADDR_LEN);

                /* If this is RIP1, set subnet mask and next-hop to NULL */
                if (send_mode == SEND_RIP1)
                {
                    memset(pkt->submask, 0, IP_ADDR_LEN);
                    memset(pkt->nexthop, 0, IP_ADDR_LEN);
                }

                /* Otherwise, copy the subnet mask and next-hop into the
                 * packet.
                 */
                else
                {
                    memcpy(pkt->submask,
                           updated_nodes[z]->rt_route_node->rt_submask,
                           IP_ADDR_LEN);

                    PUT32(pkt->nexthop, 0, updated_nodes[z]->rt_gateway_v4.sck_addr);

                    /* RFC 2453 - section 4.4 - The immediate next hop IP address
                     * to which packets to the destination specified by this route
                     * entry should be forwarded.  Specifying a value of 0.0.0.0
                     * in this field indicates that routing should be via the
                     * originator of the RIP advertisement.  An address specified
                     * as a next hop must, per force, be directly reachable on the
                     * logical subnet over which the advertisement is made.
                     */
                    if ( (DEV_Get_Dev_By_Addr(pkt->nexthop)) ||
                         ((iface_addr & iface_subnet) !=
                          (updated_nodes[z]->rt_gateway_v4.sck_addr & iface_subnet)) ||
                         (is_ppp == NU_TRUE) )
                        memset(pkt->nexthop, 0, IP_ADDR_LEN);
                }

#if (NU_DEBUG_RIP2 == NU_TRUE)

                strcpy(debug_string, "    Sending address  = ");
                NU_ITOA(updated_nodes[z]->rt_route_node->rt_ip_addr[0], temp, 10);
                strcat(debug_string, temp);
                strcat(debug_string, ".");
                NU_ITOA(updated_nodes[z]->rt_route_node->rt_ip_addr[1], temp, 10);
                strcat(debug_string, temp);
                strcat(debug_string, ".");
                NU_ITOA(updated_nodes[z]->rt_route_node->rt_ip_addr[2], temp, 10);
                strcat(debug_string, temp);
                strcat(debug_string, ".");
                NU_ITOA(updated_nodes[z]->rt_route_node->rt_ip_addr[3], temp, 10);
                strcat(debug_string, temp);
                strcat(debug_string, "   mask = ");
                NU_ITOA(updated_nodes[z]->rt_route_node->rt_submask[0], temp, 10);
                strcat(debug_string, temp);
                strcat(debug_string, ".");
                NU_ITOA(updated_nodes[z]->rt_route_node->rt_submask[1], temp, 10);
                strcat(debug_string, temp);
                strcat(debug_string, ".");
                NU_ITOA(updated_nodes[z]->rt_route_node->rt_submask[2], temp, 10);
                strcat(debug_string, temp);
                strcat(debug_string, ".");
                NU_ITOA(updated_nodes[z]->rt_route_node->rt_submask[3], temp, 10);
                strcat(debug_string, temp);
                strcat(debug_string, "\n\r");

                ATI_DEBUG_PRINT(debug_string);
#endif
                pkt++;

            } /* for (z = 0; z < count; z++) */

            if (z > 0)
            {
                /* Save off the IP address of the last entry in the packet */
                memcpy(last_entry_addr, updated_nodes[z-1]->rt_route_node->rt_ip_addr,
                       IP_ADDR_LEN);
            }

#if (RIP2_DISABLE_POISONED_REVERSE == NU_FALSE)

            /* Subtract the entries that were not added to the packet */
            count -= not_added;
#endif

            if (count > 0)
            {
                /* Compute the length of the packet */
                len = (UINT16)((count * RIP_PKT_SIZE) + 4);

                /* Send the packet */
                ret = NU_Send_To(RIP2_Socket, RIP2_Buffer, len, 0,
                                 baddr, sizeof(baddr));

                if (ret <= 0)
                {
                    /* If an ICMP error message was returned, try to send again */
                    if (ICMP_ERROR_CODE(ret) == NU_TRUE)
                    {
                        NLOG_Error_Log("RIP2_Send_Routes NU_Send_To returned ICMP Error Code",
                                       NERR_RECOVERABLE, __FILE__, __LINE__);

                        /* Send the packet */
                        ret = NU_Send_To(RIP2_Socket, RIP2_Buffer, len, 0,
                                         baddr, sizeof(baddr));
                    }

                    if (ret <= 0)
                        NLOG_Error_Log("RIP2_Send_Routes NU_Send_To returned negative value",
                                       NERR_RECOVERABLE, __FILE__, __LINE__);
                }

            } /* if (count > 0) */

        } /* if (count > 0) */

    } while (count > 0);

    return;

} /* RIP2_Send_Routes */

/*************************************************************************
*
*   FUNCTION
*
*       RIP2_Request_Routing_Table
*
*   DESCRIPTION
*
*       Asks all routers to send their route tables.
*
*   INPUTS
*
*       *nodes                  Pointer to the RIP2 list nodes
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
STATIC VOID RIP2_Request_Routing_Table(RIP2_LIST_NODE *node)
{
    SIGNED              ret;
    INT16               len;
    RIP2_ENTRY          *pkt;
    RIP2_PACKET         *rp;
    struct addr_struct  baddr;
    UINT8               dev_addr[IP_ADDR_LEN];
    DV_DEVICE_ENTRY     *dev_ptr;

#if (NU_DEBUG_RIP2 == NU_TRUE)

    CHAR                temp[10];

    strcpy(debug_string, "in RIP2_Send_Table\n\r");
    ATI_DEBUG_PRINT(debug_string);

#endif

    /* If the parameters are all valid */
    if ( (RIP2_Socket >= 0) && (node != NU_NULL) &&
         (node->r2_sendmode != SEND_NONE) )
    {
        /* do not care which address message came from so set to zero */
        baddr.family = NU_FAMILY_IP;
        baddr.port = RIP_PORT;
        baddr.name = "RIPBCST";

        rp = (RIP2_PACKET*)RIP2_Buffer;
        rp->command = RIP2_REQUEST;

        rp->unused = 0;

        /* Grab the semaphore so we can get a pointer to the device */
        if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
        {
            NLOG_Error_Log("Unable to obtain TCP semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);

            return;
        }

        /* Look up the device the user wishes to use RIP with. */
        dev_ptr = DEV_Get_Dev_By_Index(node->r2_device.rip2_dev_index);

        /* Copy the IP address of the device */
        if (dev_ptr)
        {
            /* Ensure that the device is UP */
            if (dev_ptr->dev_flags & DV_UP)
                PUT32(dev_addr, 0,
                      dev_ptr->dev_addr.dev_addr_list.dv_head->dev_entry_ip_addr);

            /* Otherwise, the interface does not have a valid IP address.
             * Go on to the next interface in the list.
             */
            else
            {
                /* Release the TCP semaphore */
                if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                                   __FILE__, __LINE__);

                return;
            }
        }

        else
        {
            /* Release the TCP semaphore */
            if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                               __FILE__, __LINE__);

            /* Remove this node from the list since the interface has been
             * deleted.
             */
            DLL_Remove(&RIP2_List, node);

            /* Deallocate the memory */
            if (NU_Deallocate_Memory(node) != NU_SUCCESS)
                NLOG_Error_Log("Failed to deallocate memory", NERR_SEVERE,
                               __FILE__, __LINE__);

            return;
        }

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)
        /* If the send mode is RIP-1, send the request via broadcast */
        if (node->r2_sendmode & SEND_RIP1)
#endif
        {
            /* If RIP-1 Compatibility Switch is enabled, RIP-2 messages
             * are broadcast.
             */
            if (node->r2_sendmode & SEND_RIP2)
                rp->version = RIP2_VERSION;
            else
                rp->version = RIP1_VERSION;

            /* If this is not a Point-to-Point link, create the network
             * broadcast address for the network.
             */
            if (!(dev_ptr->dev_flags & DV_POINTTOPOINT))
            {
                PUT32(baddr.id.is_ip_addrs, 0,
                      dev_ptr->dev_addr.dev_addr_list.dv_head->dev_entry_ip_addr
                      | (~dev_ptr->dev_addr.dev_netmask));

                /* Release the TCP semaphore */
                if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                                   __FILE__, __LINE__);

                /* Set this device as the Broadcast Interface */
                if (NU_Setsockopt_IP_BROADCAST_IF(RIP2_Socket,
                                                  dev_addr) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to set broadcast interface",
                                   NERR_SEVERE, __FILE__, __LINE__);
            }

            /* Otherwise, unicast this packet to the other side of the
             * link.
             */
            else
            {
                PUT32(baddr.id.is_ip_addrs, 0,
                      dev_ptr->dev_addr.dev_dst_ip_addr);

                /* Release the TCP semaphore */
                if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                                   __FILE__, __LINE__);
            }
        }

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)

        /* If the send mode is RIP-2, send the request via multicast */
        else
        {
            /* Release the TCP semaphore */
            if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                               __FILE__, __LINE__);

            rp->version = RIP2_VERSION;
            memcpy(baddr.id.is_ip_addrs, RIP2_Multi_Addr, IP_ADDR_LEN);

            /* Set this device as the Multicast Interface */
            if (NU_Setsockopt_IP_MULTICAST_IF(RIP2_Socket,
                                              dev_addr) != NU_SUCCESS)
                NLOG_Error_Log("Failed to set multicast interface",
                               NERR_SEVERE, __FILE__, __LINE__);
        }

#endif

#if (NU_DEBUG_RIP2 == NU_TRUE)
        strcpy(debug_string, "Request from = ");
        NU_ITOA(baddr.id.is_ip_addrs[0], temp, 10);
        strcat(debug_string, temp);
        strcat(debug_string, ".");
        NU_ITOA(baddr.id.is_ip_addrs[1], temp, 10);
        strcat(debug_string, temp);
        strcat(debug_string, ".");
        NU_ITOA(baddr.id.is_ip_addrs[2], temp, 10);
        strcat(debug_string, temp);
        strcat(debug_string, ".");
        NU_ITOA(baddr.id.is_ip_addrs[3], temp, 10);
        strcat(debug_string, temp);
        strcat(debug_string, "\n\r");

        ATI_DEBUG_PRINT(debug_string);
#endif

        pkt = (RIP2_ENTRY *)&RIP2_Buffer[4];

        /* RFC 2453 section 3.9.1 - If there is exactly one entry in
         * the request, and it has an address family identifier of
         * zero and a metric of infinity (i.e., 16), then this is a
         * request to send the entire routing table.
         */
        pkt->af_id = 0;
        pkt->routetag = 0;

        memset(pkt->ip_addr, 0, IP_ADDR_LEN);
        memset(pkt->submask, 0, IP_ADDR_LEN);
        memset(pkt->nexthop, 0, IP_ADDR_LEN);

        PUT32(&pkt->metric, 0, RT_INFINITY);

        len = RIP_PKT_SIZE + 4;

        ret = NU_Send_To(RIP2_Socket, RIP2_Buffer, (UINT16)len, 0,
                         &baddr, sizeof(baddr));

        if (ret <= 0)
        {
            NLOG_Error_Log("RIP2_Request_Routing_Table NU_Send_To returned negative value",
                           NERR_RECOVERABLE, __FILE__, __LINE__);

            NET_DBG_Notify(ret, __FILE__, __LINE__,
                           NU_Current_Task_Pointer(), NU_NULL);
        }
    }

    return;

} /* RIP2_Request_Routing_Table */

#endif
#endif
