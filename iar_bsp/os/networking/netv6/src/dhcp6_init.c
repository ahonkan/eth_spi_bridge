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
*   FILENAME                                         
*
*       dhcp6_init.c                                       
*
*   DESCRIPTION
*
*       This file contains all the DHCP routines for initialization of
*       the DHCPv6 client module and the necessary tasks for processing
*       DHCPv6 client events and incoming packets.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       DHCP6_Init
*       DHCP6_Find_TX_Struct_By_ID
*       DHCP6_Find_Struct_By_ID
*       DHCP6_Find_Struct_By_Dev
*       DHCP6_Free_TX_Struct
*       DHCP6_RX_Task
*       DHCP6_Event_Handler
*       DHCP6_Event_Handler_Task
*
*   DEPENDENCIES
*
*       nu_net.h
*       nu_net6.h
*
**************************************************************************/

#include "networking/nu_net.h"
#include "networking/nu_net6.h"

#if (INCLUDE_DHCP6 == NU_TRUE)

UINT8           DHCP6_All_DHCP_Relay_Agents_and_Servers[IP6_ADDR_LEN] = 
                    {0xff, 02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 2};

DHCP6_TX_STRUCT     DHCP6_Tx_List[DHCP6_SIM_TX_COUNT];
NU_SEMAPHORE        DHCP6_Cli_Resource;
TQ_EVENT            DHCP6_Retrans_Event;
TQ_EVENT            DHCP6_Renew_IA_NA_Event;
TQ_EVENT            DHCP6_Stateful_Config_Event;
TQ_EVENT            DHCP6_Stateless_Config_Event;
TQ_EVENT            DHCP6_Release_Event;
TQ_EVENT            DHCP6_Decline_Event;
NU_TASK             DHCP6_RX_Task_Ptr;
NU_TASK             DHCP6_Event_Task_Ptr;
INT                 DHCP6_Client_Socket;
UINT8               DHCP6_RX_Task_Init = 0;
NU_QUEUE            DCHP6_Queue;
DHCP6_OPT_STRUCT    DHCP6_Options[DHCP6_MAX_OPT_CODES];
DHCP6_STRUCT_LIST   DHCP6_Structs;
struct addr_struct  DHCP6_Server_Addr;

VOID DHCP6_RX_Task(UNSIGNED, VOID *);
VOID DHCP6_Event_Handler_Task(UNSIGNED, VOID *);
STATIC  DHCP6_TX_STRUCT *DHCP6_Find_TX_Struct_By_ID(UINT8);

extern UINT32   NET_Initialized_Modules;

/*************************************************************************
*
*   FUNCTION
*
*       DHCP6_Init
*
*   DESCRIPTION
*
*       This routine initializes all DHCPv6 client data structures, 
*       timers and events.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       NU_SUCCESS              Initialization was successful.
*       NU_NO_MEMORY            Not enough memory to create client task
*                               or event.
*       NU_INVALID_TASK         Failed to create task.
*
*************************************************************************/
STATUS DHCP6_Init(VOID)
{
    STATUS  status;
    VOID    *pointer;
    UINT32  i;

    /* Create the DHCPv6 client semaphore. */
    status = NU_Create_Semaphore(&DHCP6_Cli_Resource, "DHCP6CLI", 
                                 (UNSIGNED)1, NU_FIFO);

    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to create DHCPv6 client semaphore", 
                       NERR_FATAL, __FILE__, __LINE__);

        return (status);
    }

    /* Initialize each index to 0xffffffff. */
    for (i = 0; i < DHCP6_MAX_OPT_CODES; i++)
    {
        DHCP6_Options[i].dhcp6_opt_addr = 0xffffffff;
        DHCP6_Options[i].dhcp6_opt_count = 0;        
    }

    /* Set each function pointer.  Some pointers are NU_NULL because they
     * are unsupported, and some are NU_NULL because there is no processing
     * to be done on this option when received by the server. 
     */
    DHCP6_Options[1].dhcp6_parse_opt = DHCP6_Parse_Client_ID_Option;
    DHCP6_Options[2].dhcp6_parse_opt = DHCP6_Parse_Server_ID_Option;
    DHCP6_Options[3].dhcp6_parse_opt = DHCP6_Parse_IA_NA_Option;
    DHCP6_Options[4].dhcp6_parse_opt = NU_NULL;
    DHCP6_Options[5].dhcp6_parse_opt = DHCP6_Parse_IA_Addr_Option;
    DHCP6_Options[6].dhcp6_parse_opt = DHCP6_Parse_Option_Request_Option;
    DHCP6_Options[7].dhcp6_parse_opt = DHCP6_Parse_Preference_Option;
    DHCP6_Options[8].dhcp6_parse_opt = NU_NULL;
    DHCP6_Options[9].dhcp6_parse_opt = NU_NULL;
    DHCP6_Options[10].dhcp6_parse_opt = NU_NULL;
    DHCP6_Options[11].dhcp6_parse_opt = NU_NULL;
    DHCP6_Options[12].dhcp6_parse_opt = DHCP6_Parse_Server_Unicast_Option;
    DHCP6_Options[13].dhcp6_parse_opt = DHCP6_Parse_Status_Code_Option;
    DHCP6_Options[14].dhcp6_parse_opt = NU_NULL;
    DHCP6_Options[15].dhcp6_parse_opt = DHCP6_Parse_User_Class_Option;
    DHCP6_Options[16].dhcp6_parse_opt = NU_NULL;
    DHCP6_Options[17].dhcp6_parse_opt = NU_NULL;
    DHCP6_Options[18].dhcp6_parse_opt = NU_NULL;
    DHCP6_Options[19].dhcp6_parse_opt = DHCP6_Parse_Reconfigure_Msg_Option;
    DHCP6_Options[20].dhcp6_parse_opt = DHCP6_Parse_Reconfigure_Accept_Option;
    DHCP6_Options[21].dhcp6_parse_opt = NU_NULL;
    DHCP6_Options[22].dhcp6_parse_opt = NU_NULL;
    DHCP6_Options[23].dhcp6_parse_opt = DHCP6_Parse_DNS_Server_Option;
    DHCP6_Options[24].dhcp6_parse_opt = NU_NULL;

    /* Initialize each TX structure. */
    for (i = 0; i < DHCP6_SIM_TX_COUNT; i++)
    {
        memset(&DHCP6_Tx_List[i], 0, sizeof(DHCP6_TX_STRUCT));
    }

    /* Initialize the DHCPv6 client structure list. */
    DHCP6_Structs.dhcp6_head = NU_NULL;
    DHCP6_Structs.dhcp6_tail = NU_NULL;

    /* Set up the server structure for sending multicast packets. */
    DHCP6_Server_Addr.family = NU_FAMILY_IP6;
    DHCP6_Server_Addr.port = DHCP6_SERVER_PORT;
    DHCP6_Server_Addr.name = "DHCP6Rtr";

    /* Copy the multicast address into the address structure. */
    NU_BLOCK_COPY(DHCP6_Server_Addr.id.is_ip_addrs, 
                  DHCP6_All_DHCP_Relay_Agents_and_Servers, IP6_ADDR_LEN);

    /* Create the DHCPv6 client event to handle retransmissions. */
    if (EQ_Register_Event(DHCP6_Event_Handler, 
                          &DHCP6_Retrans_Event) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to register DHCPv6 Solicit retransmission event", 
                       NERR_FATAL, __FILE__, __LINE__);
    }

    /* Create the DHCPv6 client event to renew IAs when T1 expires. */
    if (EQ_Register_Event(DHCP6_Event_Handler, 
                          &DHCP6_Renew_IA_NA_Event) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to register DHCPv6 address renewal timer", 
                       NERR_FATAL, __FILE__, __LINE__);
    }

    /* Create the DHCPv6 client event to invoke DHCPv6. */
    if (EQ_Register_Event(DHCP6_Event_Handler, 
                          &DHCP6_Stateful_Config_Event) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to register DHCPv6 address renewal timer", 
                       NERR_FATAL, __FILE__, __LINE__);
    }

    /* Create the DHCPv6 client event to invoke stateless address 
     * configuration. 
     */
    if (EQ_Register_Event(DHCP6_Event_Handler, 
                          &DHCP6_Stateless_Config_Event) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to register DHCPv6 address renewal timer", 
                       NERR_FATAL, __FILE__, __LINE__);
    }
	
    /* Create the DHCPv6 client event to release an IPv6 address. */
    if (EQ_Register_Event(DHCP6_Event_Handler, 
                          &DHCP6_Release_Event) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to register DHCPv6 release event", 
                       NERR_FATAL, __FILE__, __LINE__);
    }

    /* Create the DHCPv6 client event to decline an IPv6 address. */
    if (EQ_Register_Event(DHCP6_Event_Handler, 
                          &DHCP6_Decline_Event) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to register DHCPv6 release event", 
                       NERR_FATAL, __FILE__, __LINE__);
    }

    /* Create the DHCPv6 client queue for handling events. */
    status = NU_Allocate_Memory(MEM_Cached, &pointer,
                                (UNSIGNED)(DHCP6_EVENT_Q_NUM_ELEMENTS * 
                                sizeof(DHCP6_QUEUE_ELEMENT)),
                                (UNSIGNED)NU_SUSPEND);

    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to allocate memory for the DHCPv6 client queue", 
                       NERR_FATAL, __FILE__, __LINE__);

        return (NU_MEM_ALLOC);
    }

    pointer = TLS_Normalize_Ptr(pointer);

    /* The DHCP6_QUEUE_ELEMENT contains 2 UNSIGNED variables. */
    status = NU_Create_Queue(&DCHP6_Queue, "DHCP6Q", pointer, 
                             (UNSIGNED)(DHCP6_EVENT_Q_NUM_ELEMENTS * 
                             sizeof(DHCP6_QUEUE_ELEMENT)),
                             NU_FIXED_SIZE, 2, NU_FIFO);

    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to create the DHCPv6 client queue", 
                       NERR_FATAL, __FILE__, __LINE__);
    }
    
    /* Allocate memory for the DHCPv6 receive task. */
    status = NU_Allocate_Memory(MEM_Cached, &pointer, 
                                (UNSIGNED)DHCP6_CLIENT_RX_TASK_SIZE,
                                (UNSIGNED)NU_NO_SUSPEND);

    /* If memory was allocated, create the task. */
    if (status == NU_SUCCESS)
    {
        pointer = TLS_Normalize_Ptr(pointer);

        /* Create the DHCPv6 client task to receive data from DHCPv6 
         * servers. 
         */
        status = NU_Create_Task(&DHCP6_RX_Task_Ptr, "DHCP6CLI",
                                DHCP6_RX_Task, (UNSIGNED)0, NU_NULL, pointer,
                                (UNSIGNED)DHCP6_CLIENT_RX_TASK_SIZE, 
                                DHCP6_CLIENT_RX_TASK_PRIORITY, 
                                (UNSIGNED)DHCP6_CLIENT_RX_TASK_SLICE, 
                                DHCP6_CLIENT_RX_TASK_PREEMPT, 
                                NU_START);

        if (status != NU_SUCCESS)
        {   
            NLOG_Error_Log("Failed to create the DHCPv6 client data task", 
                           NERR_FATAL, __FILE__, __LINE__);

            return (NU_INVALID_TASK);
        }
    }
    
    /* Log an error and return an error code. */
    else
    {
        NLOG_Error_Log("Failed to allocate memory for the DHCPv6 client task", 
                       NERR_FATAL, __FILE__, __LINE__);

        return (NU_NO_MEMORY);
    }

    /* Allocate memory for the DHCPv6 event handler task. */
    status = NU_Allocate_Memory(MEM_Cached, &pointer, 
                                (UNSIGNED)DHCP6_CLIENT_EVENT_TASK_SIZE,
                                (UNSIGNED)NU_NO_SUSPEND);

    /* If memory was allocated, create the task. */
    if (status == NU_SUCCESS)
    {
        pointer = TLS_Normalize_Ptr(pointer);

        /* Create the DHCPv6 event handler client task. */
        status = NU_Create_Task(&DHCP6_Event_Task_Ptr, "DHCP6EVNT",
                                DHCP6_Event_Handler_Task, (UNSIGNED)0, 
                                NU_NULL, pointer,
                                (UNSIGNED)DHCP6_CLIENT_EVENT_TASK_SIZE, 
                                DHCP6_CLIENT_EVENT_TASK_PRIORITY, 
                                (UNSIGNED)DHCP6_CLIENT_EVENT_TASK_SLICE, 
                                DHCP6_CLIENT_EVENT_TASK_PREEMPT, 
                                NU_START);

        if (status != NU_SUCCESS)
        {   
            status = NU_INVALID_TASK;

            NLOG_Error_Log("Failed to create the DHCPv6 event handler task", 
                           NERR_FATAL, __FILE__, __LINE__);
        }
    }
    
    /* Log an error and return an error code. */
    else
    {
        status = NU_NO_MEMORY;

        NLOG_Error_Log("Failed to allocate memory for the DHCPv6 event handler task", 
                       NERR_FATAL, __FILE__, __LINE__);
    }

    return (status);

} /* DHCP6_Init */

/*************************************************************************
*
*   FUNCTION
*
*       DHCP6_Find_TX_Struct_By_ID
*
*   DESCRIPTION
*
*       This routine finds a transmission structure based on the ID of the
*       DHCP6_TX_STRUCT.
*
*   INPUTS
*
*       ds_id                   The ID of the target DHCP6_TX_STRUCT.
*
*   OUTPUTS
*
*       A pointer to the DHCP6_TX_STRUCT or NU_NULL if no matching structure
*       was found.
*
*************************************************************************/
STATIC DHCP6_TX_STRUCT *DHCP6_Find_TX_Struct_By_ID(UINT8 ds_id)
{
    DHCP6_TX_STRUCT    *tx_ptr = NU_NULL;
    INT                 i;

    /* Loop through the list of structures until a match is found. */
    for (i = 0; i < DHCP6_SIM_TX_COUNT; i++)
    {
        /* If this is the target entry, return it. */
        if (DHCP6_Tx_List[i].dhcp6_id == ds_id)
        {
            tx_ptr = &DHCP6_Tx_List[i];

            break;
        }
    }

    return (tx_ptr);

} /* DHCP6_Find_TX_Struct_By_ID */

/*************************************************************************
*
*   FUNCTION
*
*       DHCP6_Find_Struct_By_ID
*
*   DESCRIPTION
*
*       This routine finds a DHCPv6 structure based on the ID.
*
*   INPUTS
*
*       target_id           The target ID of the structure to find.
*
*   OUTPUTS
*
*       A pointer to the DHCP6_STRUCT or NU_NULL if no matching structure
*       was found.
*
*************************************************************************/
DHCP6_STRUCT *DHCP6_Find_Struct_By_ID(UINT8 target_id)
{
    DHCP6_STRUCT    *ds_ptr;

    /* Get a pointer to the first entry in the list. */
    ds_ptr = DHCP6_Structs.dhcp6_head;

    /* Check each entry in the list for a match on transaction ID. */
    while (ds_ptr)
    {
        /* If this entry matches, return it. */
        if (ds_ptr->dhcp6_id == target_id)
            break;

        /* Get a pointer to the next entry in the list. */
        ds_ptr = ds_ptr->dhcp6_next;
    }

    return (ds_ptr);

} /* DHCP6_Find_Struct_By_ID */

/*************************************************************************
*
*   FUNCTION
*
*       DHCP6_Find_Struct_By_Dev
*
*   DESCRIPTION
*
*       This routine finds a DHCPv6 structure based on the interface 
*       index.
*
*   INPUTS
*
*       dev_index           The target interface index of the structure to 
*                           find.
*
*   OUTPUTS
*
*       A pointer to the DHCP6_STRUCT or NU_NULL if no matching structure
*       was found.
*
*************************************************************************/
DHCP6_STRUCT *DHCP6_Find_Struct_By_Dev(UINT32 dev_index)
{
    DHCP6_STRUCT    *ds_ptr;

    /* Get a pointer to the first entry in the list. */
    ds_ptr = DHCP6_Structs.dhcp6_head;

    /* Check each entry in the list for a match on transaction ID. */
    while (ds_ptr)
    {
        /* If this entry matches, return it. */
        if (ds_ptr->dhcp6_dev_index == dev_index)
            break;

        /* Get a pointer to the next entry in the list. */
        ds_ptr = ds_ptr->dhcp6_next;
    }

    return (ds_ptr);

} /* DHCP6_Find_Struct_By_Dev */

/*************************************************************************
*
*   FUNCTION
*
*       DHCP6_Free_TX_Struct
*
*   DESCRIPTION
*
*       This routine frees a DHCP transmission structure.
*
*   INPUTS
*
*       *tx_ptr                 Pointer to the transmission structure to
*                               free.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID DHCP6_Free_TX_Struct(DHCP6_TX_STRUCT *tx_ptr)
{
    /* Free the transmission structure. */
    tx_ptr->dhcp6_use = 0;

    /* Get the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* Clear the timer that is running to retransmit the message. */
        TQ_Timerunset(DHCP6_Retrans_Event, TQ_CLEAR_EXACT, tx_ptr->dhcp6_id, 
                      0);

        /* Clear the timer that is running to release the IP address. */
        TQ_Timerunset(DHCP6_Release_Event, TQ_CLEAR_EXACT, tx_ptr->dhcp6_id, 
                      0);

        /* Clear the timer that is running to decline the address. */
        TQ_Timerunset(DHCP6_Decline_Event, TQ_CLEAR_EXACT, tx_ptr->dhcp6_id, 
                      0);

        /* Release the semaphore. */
        NU_Release_Semaphore(&TCP_Resource);

        /* Clear out the contents of the structure. */
        memset(tx_ptr, 0, sizeof(DHCP6_TX_STRUCT));
    }

    else
    {
        NLOG_Error_Log("Failed to obtain TCP semaphore",  NERR_SEVERE, 
                       __FILE__, __LINE__);
    }

} /* DHCP6_Free_TX_Struct */

/*************************************************************************
*
*   FUNCTION
*
*       DHCP6_RX_Task
*
*   DESCRIPTION
*
*       This routine blocks on the DHCPv6 client socket and passes any
*       data received to the DHCPv6 client interpret routine.
*
*   INPUTS
*
*       argc                    Unused parameter.
*       *argv                   Unused parameter.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID DHCP6_RX_Task(UNSIGNED argc, VOID *argv)
{
    struct addr_struct  local_addr, foreign_addr;
    INT32               bytes_received;
    CHAR                dhcp_rx_buffer[DHCP6_RX_BUFFER_LEN];
    msghdr              msg;
    cmsghdr             *cmsg;
    in6_pktinfo         *pktinfo_ptr = NU_NULL;
    CHAR                anc_buf[NU_CMSG_SPACE(sizeof(in6_pktinfo))];

    UNUSED_PARAMETER(argc);
    UNUSED_PARAMETER(argv);

    /* Obtain the DHCPv6 client semaphore. */
    if (NU_Obtain_Semaphore(&DHCP6_Cli_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* Create a socket for receiving data from DHCPv6 servers. */
        DHCP6_Client_Socket = NU_Socket(NU_FAMILY_IP6, NU_TYPE_DGRAM, 0);

        /* Ensure the socket was successfully created. */
        if (DHCP6_Client_Socket >= 0)
        {
            /* Set the socket option for the call to NU_Recvmsg to return 
             * the destination address and receiving interface of the 
             * incoming packet.
             */
            NU_Setsockopt_IPV6_RECVPKTINFO(DHCP6_Client_Socket, 1);

            /* Fill in a structure with the DHCPv6 client node's 
             * information. 
             */
            local_addr.family = NU_FAMILY_IP6;
            local_addr.port = DHCP6_CLIENT_PORT;
            local_addr.id = IP6_ADDR_ANY;
            local_addr.name = "DHCP6_Cli";

            /* Bind to the socket. */
            NU_Bind(DHCP6_Client_Socket, &local_addr, 0);

            /* Set up the msghdr structure. */
            msg.msg_iov = dhcp_rx_buffer;
            msg.msg_iovlen = DHCP6_RX_BUFFER_LEN;
            msg.msg_name = &foreign_addr;
            msg.msg_namelen = sizeof(struct addr_struct);

            /* Set up the msghdr to point to the buffer of ancillary data */
            msg.msg_control = anc_buf;
            msg.msg_controllen = sizeof(anc_buf);

            /* Set the flag indicating that initialization of the RX task
             * is complete. 
             */
            DHCP6_RX_Task_Init = 1;

            /* Release the DHCPv6 client semaphore. */
            NU_Release_Semaphore(&DHCP6_Cli_Resource);

            /* Continue to receive data and pass it to the DHCPv6 interpret
             * routine indefinitely.
             */
            for (;;)
            {
                /* Suspend for data. */
                bytes_received = NU_Recvmsg(DHCP6_Client_Socket, &msg, 0);

                /* If data was received. */
                if (bytes_received > 0)
                {
                    /* Search through the ancillary data returned for the 
                     * in6_pktinfo structure which contains the interface index 
                     * of the interface on which the packet was received.
                     */
                    for (cmsg = CMSG_FIRSTHDR(&msg);
                         cmsg != NU_NULL;
                         cmsg = CMSG_NXTHDR(&msg, cmsg))
                    {
                        /* If this is the in6_pktinfo structure */
                        if ( (cmsg->cmsg_level == IPPROTO_IPV6) &&
                             (cmsg->cmsg_type == IPV6_PKTINFO) )
                        {
                            /* Save a pointer to the data. */
                            pktinfo_ptr = (in6_pktinfo*)CMSG_DATA(cmsg);
                            break;
                        }
                    }

                    /* If the ancillary data was found. */
                    if (pktinfo_ptr)
                    {
                        /* Obtain the DHCPv6 client semaphore. */
                        if (NU_Obtain_Semaphore(&DHCP6_Cli_Resource, 
                                                NU_SUSPEND) == NU_SUCCESS)
                        {
                            /* Pass the data to the DHCPv6 interpret routine. */
                            DHCP6_Interpret((UINT8*)dhcp_rx_buffer, 
                                            (UINT16)bytes_received, 
                                            pktinfo_ptr->ipi6_addr,
                                            pktinfo_ptr->ipi6_ifindex);

                            /* Release the DHCPv6 client semaphore. */
                            NU_Release_Semaphore(&DHCP6_Cli_Resource);
                        }

                        else
                        {
                            NLOG_Error_Log("Failed to obtain DHCPv6 semaphore",  
                                           NERR_SEVERE, __FILE__, __LINE__);
                        }
                    }
                }

                /* Log an error. */
                else
                {
                    NLOG_Error_Log("Error in receiving data on the DHCPv6 client socket", 
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
            }
        }

        /* Log an error. */
        else
        {
            /* Release the DHCPv6 client semaphore. */
            NU_Release_Semaphore(&DHCP6_Cli_Resource);

            NLOG_Error_Log("Could not create socket in DHCPv6 client task", 
                           NERR_FATAL, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to obtain DHCPv6 semaphore",  NERR_SEVERE, 
                       __FILE__, __LINE__);
    }

} /* DHCP6_RX_Task */

/*************************************************************************
*
*   FUNCTION
*
*       DHCP6_Event_Handler
*
*   DESCRIPTION
*
*       This routine handles DHCPv6 events for client nodes.
*
*   INPUTS
*
*       event                   The event to handle.
*       data                    Data for the event.
*       extra_data              Unused.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID DHCP6_Event_Handler(TQ_EVENT event, UNSIGNED data, UNSIGNED extra_data)
{   
    DHCP6_QUEUE_ELEMENT queue_input;

    UNUSED_PARAMETER(extra_data);

    /* Set up the queue structure. */
    queue_input.dhcp6_data = data;

    /* Copy the event. */
    queue_input.dhcp6_event = (UNSIGNED)event;

    /* Send the queue structure to the queue.  This does not need semaphore
     * protection since NU_Send_To_Queue implements protection to guard 
     * against simultaneous access on the queue.
     */
    NU_Send_To_Queue(&DCHP6_Queue, &queue_input, 2, NU_SUSPEND);

} /* DHCP6_Event_Handler */

/*************************************************************************
*
*   FUNCTION
*
*       DHCP6_Event_Handler_Task
*
*   DESCRIPTION
*
*       This routine handles DHCPv6 client events.
*
*   INPUTS
*
*       argc                    Unused parameter.
*       *argv                   Unused parameter.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID DHCP6_Event_Handler_Task(UNSIGNED argc, VOID *argv)
{
    STATUS              status;
    DHCP6_QUEUE_ELEMENT queue_input;
    UNSIGNED            act_size;
    DHCP6_STRUCT        *ds_ptr = NU_NULL;
    DHCP6_TX_STRUCT     *tx_ptr;
    DHCP6_CLIENT        cli_ptr;
    UINT8               solicit_count = 0;
    UINT8               opt_buffer[DHCP6_IA_NA_OPT_LEN];
    UINT32              iaid, delay;

    UNUSED_PARAMETER(argc);
    UNUSED_PARAMETER(argv);

    for (;;)
    {
        /* Suspend on the queue, waiting for a timer to expire. */
        status = NU_Receive_From_Queue(&DCHP6_Queue, (VOID*)&queue_input, 
                                       2, &act_size, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* If this is the event to invoke DHCPv6. */
            if (queue_input.dhcp6_event == DHCP6_Stateful_Config_Event)
            {
                /* If this is the first solicit message sent. */
                if (solicit_count == 0)
                {
                    /* Only delay the first message sent. */
                    solicit_count = 1;

                    /* RFC 3315 - section 17.1.2 - The first Solicit message 
                     * from the client on the interface MUST be delayed by a 
                     * random amount of time between 0 and SOL_MAX_DELAY.   
                     */
                    delay = UTL_Rand() % DHCP6_SOL_MAX_DELAY;

                    /* Get the NET semaphore since we are about to set a NET
                     * timer.
                     */
                    status = NU_Obtain_Semaphore(&TCP_Resource, NU_NO_SUSPEND);

                    if (status == NU_SUCCESS)
                    {
                        /* Set the delay timer to transmit the Solicit. */
                        if (TQ_Timerset(DHCP6_Stateful_Config_Event, 
                                        queue_input.dhcp6_data,
                                        delay, 0) != NU_SUCCESS)
                        {
                            NLOG_Error_Log("Cannot set timer to transmit first Solicit message", 
                                           NERR_SEVERE, __FILE__, __LINE__);
                        }

                        /* Release the NET semaphore. */
                        NU_Release_Semaphore(&TCP_Resource);
                    }

                    else
                    {
                        NLOG_Error_Log("Failed to obtain TCP semaphore",  
                                       NERR_SEVERE, __FILE__, __LINE__);
                    }
                }
                
                /* Otherwise, invoke DHCPv6. */
                else
                {
                    /* Get the IAID associated with this interfae. */
                    status = NU_Get_DHCP_IAID(queue_input.dhcp6_data, 
                                              &iaid);

                    /* If the interface is valid. */
                    if (status == NU_SUCCESS)
                    {
                        /* Build the IA_NA option to obtain an IP address 
                         * from the server.
                         */
                        cli_ptr.dhcp6_opt_length = 
                            DHCP6_Build_IA_NA_Option(opt_buffer, iaid, 0, 0,
                                                     NU_NULL, 0);

                        /* Fill in the client structure. */
                        cli_ptr.dhcp6_dev_index = queue_input.dhcp6_data;
                        cli_ptr.dhcp6_user_opts = opt_buffer;

                        /* Invoke DHCPv6. */
                        NU_Dhcp6(&cli_ptr, NU_NO_SUSPEND);
                    }
                }
            }
            /* If this is the event to invoke stateless address 
             * configuration.
             */
            else if (queue_input.dhcp6_event == DHCP6_Stateless_Config_Event)
            {
                 /* If this is the first solicit message sent. */
                if (solicit_count == 0)
                {
                    /* Only delay the first message sent. */
                    solicit_count = 1;

                    /* RFC 3315 - section 17.1.2 - The first Solicit message 
                     * from the client on the interface MUST be delayed by a 
                     * random amount of time between 0 and SOL_MAX_DELAY.   
                     */
                    delay = UTL_Rand() % DHCP6_SOL_MAX_DELAY;

                    /* Get the NET semaphore since we are about to set a NET
                     * timer.
                     */
                    status = NU_Obtain_Semaphore(&TCP_Resource, NU_NO_SUSPEND);

                    if (status == NU_SUCCESS)
                    {
                        /* Set the delay timer to transmit the Solicit. */
                        if (TQ_Timerset(DHCP6_Stateless_Config_Event, 
                                        queue_input.dhcp6_data,
                                        delay, 0) != NU_SUCCESS)
                        {
                            NLOG_Error_Log("Cannot set timer to transmit first Solicit message", 
                                           NERR_SEVERE, __FILE__, __LINE__);
                        }

                        /* Release the NET semaphore. */
                        NU_Release_Semaphore(&TCP_Resource);
                    }

                    else
                    {
                        NLOG_Error_Log("Failed to obtain TCP semaphore",  
                                       NERR_SEVERE, __FILE__, __LINE__);
                    }
                }
                
                /* Otherwise, invoke DHCPv6. */
                else
                {
                    /* Get the IAID associated with this interfae. 
                    status = NU_Get_DHCP_IAID(queue_input.dhcp6_data, 
                                              &iaid);
*/
                    /* If the interface is valid. */
                    if (status == NU_SUCCESS)
                    {
                        /* Build the IA_NA option to obtain an IP address 
                         * from the server.
                         */
/*
                        cli_ptr.dhcp6_opt_length = 
                            DHCP6_Build_IA_NA_Option(opt_buffer, iaid, 0, 0,
                                                     NU_NULL, 0);

                         Fill in the client structure.  
                        cli_ptr.dhcp6_dev_index = queue_input.dhcp6_data;
					
                        cli_ptr.dhcp6_user_opts = opt_buffer;
*/
                        /* Invoke DHCPv6. */
                        NU_Dhcp6(&cli_ptr, NU_NO_SUSPEND);
                    }
                }            
			}
            /* If this is the event to renew the addresses associated 
             * with an IA. 
             */
            else if (queue_input.dhcp6_event == DHCP6_Renew_IA_NA_Event)
            {
                /* Obtain the DHCPv6 client semaphore. */
                status = NU_Obtain_Semaphore(&DHCP6_Cli_Resource, NU_SUSPEND);

                if (status == NU_SUCCESS)
                {
                    DHCP6_Renew_Addresses(queue_input.dhcp6_data);

                    /* Release the DHCPv6 client semaphore. */
                    NU_Release_Semaphore(&DHCP6_Cli_Resource);
                }

                else
                {
                    NLOG_Error_Log("Failed to obtain DHCPv6 semaphore",  
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
            }

            /* Otherwise, this event requires a pointer to the transmission
             * structure and DHCPv6 structure.
             */
            else
            {
                /* Obtain the DHCPv6 client semaphore. */
                status = NU_Obtain_Semaphore(&DHCP6_Cli_Resource, NU_SUSPEND);

                if (status == NU_SUCCESS)
                {
                    /* Get a pointer to the transmission structure. */
                    tx_ptr = 
                        DHCP6_Find_TX_Struct_By_ID((UINT8)queue_input.dhcp6_data);

                    /* If a pointer to the transmission structure was found. */
                    if (tx_ptr)
                    {
                        /* Get a pointer to the DHCPv6 structure. */
                        ds_ptr = DHCP6_Find_Struct_By_ID(tx_ptr->dhcp6_ds_id);

	                    if (ds_ptr)
	                    {
	                        /* If this is the event to release an IPv6 address. */
	                        if (queue_input.dhcp6_event == DHCP6_Release_Event)
	                        {
	                            /* Release the address. */
	                            if (DHCP6_Release_Output(ds_ptr, tx_ptr) != NU_SUCCESS)
	                            {
	                                NLOG_Error_Log("Attempt to release IPv6 address failed", 
	                                               NERR_RECOVERABLE, __FILE__, __LINE__);
	                            }                            
	                        }
	
	                        /* If this is the event to decline an IPv6 address. */
	                        else if (queue_input.dhcp6_event == DHCP6_Decline_Event)
	                        {
	                            /* Decline the address. */
	                            if (DHCP6_Decline_Output(ds_ptr, tx_ptr) != NU_SUCCESS)
	                            {
	                                NLOG_Error_Log("Attempt to decline IPv6 address failed", 
	                                               NERR_RECOVERABLE, __FILE__, __LINE__);
	                            }                            
	                        }
	
	                        /* This is the event to retransmit a message. */
	                        else
	                        {
	                            /* Retransmit the Solicit message. */
	                            if (tx_ptr->dhcp6_type == DHCP6_SOLICIT)
	                            {
	                                /* RFC 3315 - section 17.1.2 - If the first RT
	                                 * elapses and the client has received an 
	                                 * Advertise message, the client SHOULD continue 
	                                 * with a client-initiated message exchange by 
	                                 * sending a Request message.
	                                 */
	                                if (ds_ptr->dhcp6_server.dhcp6_resp_time)
	                                {
	                                    /* Free the transmission structure. */
	                                    DHCP6_Free_TX_Struct(tx_ptr);
	
	                                    /* Initiate the transaction. */
	                                    status = DHCP6_Obtain_Address(ds_ptr);
	                                }
	
	                                /* Otherwise, retransmit the Solicit message. */
	                                else
	                                {
	                                    status = DHCP6_Solicit_Output(ds_ptr, tx_ptr);
	                                }
	                            }
	
	                            /* Retransmit the Confirm message. */
	                            else if (tx_ptr->dhcp6_type == DHCP6_CONFIRM)
	                            {
	                                status = DHCP6_Confirm_Output(ds_ptr, tx_ptr);
	                            }
	
	                            /* Retransmit the Renew message. */
	                            else if (tx_ptr->dhcp6_type == DHCP6_RENEW)
	                            {
	                                status = DHCP6_Renew_Output(ds_ptr, tx_ptr);
	                            }
	
	                            /* Retransmit the Rebind message. */
	                            else if (tx_ptr->dhcp6_type == DHCP6_REBIND)
	                            {
	                                status = DHCP6_Rebind_Output(ds_ptr, tx_ptr);
	                            }
	
	                            /* Retransmit the Request message. */
	                            else if (tx_ptr->dhcp6_type == DHCP6_REQUEST)
	                            {
	                                status = DHCP6_Request_Output(ds_ptr, tx_ptr);
	                            }
	
	                            /* Retransmit the Information Request message. */
	                            else if (tx_ptr->dhcp6_type == DHCP6_INFO_REQUEST)
	                            {
	                                status = DHCP6_Info_Request_Output(ds_ptr, tx_ptr);
	                            }
	
	                            /* Retransmit the Release message. */
	                            else if (tx_ptr->dhcp6_type == DHCP6_RELEASE)
	                            {
	                                status = DHCP6_Release_Output(ds_ptr, tx_ptr);
	                            }
	
	                            /* Retransmit the Decline message. */
	                            else if (tx_ptr->dhcp6_type == DHCP6_DECLINE)
	                            {
	                                status = DHCP6_Decline_Output(ds_ptr, tx_ptr);
	                            }
	
	                            /* This message type is not recognized. */
	                            else
	                            {
	                                status = -1;
	
	                                NLOG_Error_Log("Unrecognized DHCPv6 client packet type", 
	                                               NERR_SEVERE, __FILE__, __LINE__);
	                            }
	
	                            /* If the packet was successfully retransmitted. */
	                            if (status != NU_SUCCESS)
	                            {
	                                /* If the packet has timed out being retransmitted,
	                                 * and there is a task suspended on this action,
	                                 * resume the task now.
	                                 */
	                                if ( (status == -1) && (ds_ptr->dhcp6_task) )
	                                {
	                                    /* Set the status indicating that configuration     
	                                     * failed.
	                                     */
	                                    ds_ptr->dhcp6_status = NU_DHCP_REQUEST_FAILED;
	
	                                    /* Resume the task. */
	                                    NU_Resume_Task(ds_ptr->dhcp6_task);
	                                }
	
	                                NLOG_Error_Log("Unable to retransmit DHCPv6 message", 
	                                               NERR_SEVERE, __FILE__, __LINE__);
	                            }
	                        }  
						} 
                    }

                    /* Release the DHCPv6 client semaphore. */
                    NU_Release_Semaphore(&DHCP6_Cli_Resource);
                }

                else
                {
                    NLOG_Error_Log("Unable to obtain DHCPv6 semaphore", 
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
            }
        }

        else
        {
            NLOG_Error_Log("Error receiving from DHCPv6 client queue", 
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

} /* DHCP6_Event_Handler_Task */

#endif
