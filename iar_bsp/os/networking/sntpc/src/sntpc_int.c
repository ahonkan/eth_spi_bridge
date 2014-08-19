/*************************************************************************
*
*            Copyright Mentor Graphics Corporation 2012
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
*       sntpc_int.c
*
*   DESCRIPTION
*
*       Nucleus Simple Network Time Protocol Client internal functions
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       nu_os_net_sntpc_init
*       SNTPC_Init_On_First_Server
*       SNTPC_Deinit_On_Last_Server
*       SNTPC_Server_List_Query
*       SNTPC_Send_Request
*       SNTPC_Decode_Reply
*       SNTPC_Process_Reply
*       SNTPC_Get_Timestamp_From_Server
*       SNTPC_Log2
*       SNTPC_Send_Request_Handler
*       SNTPC_Generate_Request
*       SNTPC_Server_Update_Task
*       SNTPC_On_Wire_Protocol
*
*   DEPENDENCIES
*
*       sntpc.h
*       sntpc_int.h
*       reg_api.h
*
*************************************************************************/

#include "networking/sntpc.h"
#include "os/networking/sntpc/inc/sntpc_int.h"
#include "services/reg_api.h"

/* Global variable for the SNTP Client Configuration. */
SNTPC_CONFIG_STRUCT *SNTPC_Config = NU_NULL;

/* Function prototypes. */
VOID SNTPC_Server_Update_Task(UNSIGNED argc, VOID *argv);

/*************************************************************************
*
*   FUNCTION
*
*       nu_os_net_sntpc_init
*
*   DESCRIPTION
*
*       This function initializes the SNTP Client. It is the run-time
*       entry point of the product initialization sequence.
*
*   INPUTS
*
*       *path                   Path to the configuration settings.
*       startstop               Whether product is being started or
*                               being stopped.
*
*   OUTPUTS
*
*       status                  NU_SUCCESS is returned if initialization
*                               has completed successfully; otherwise, an
*                               operating system specific error code is
*                               returned.
*
*************************************************************************/
STATUS nu_os_net_sntpc_init(CHAR *path, INT startstop)
{
    STATUS      status = NU_SUCCESS;
    STATUS      tmp_status;
    NU_MEMORY_POOL  *sntpc_memory;
    CHAR tmp_path[REG_MAX_KEY_LENGTH];
#if CFG_NU_OS_NET_SNTPC_USE_DEFAULT_SERVER
    SNTPC_SERVER    server;
#endif

    if (startstop)
    {
        /* Initialize the SNTP Client module. */

        /* Get memory pool pointer */
        status = NU_System_Memory_Get(&sntpc_memory, NU_NULL);
        if (status == NU_SUCCESS)
        {
            /* Allocate memory for the config structure */
            status = NU_Allocate_Memory(sntpc_memory,
                                        (VOID **)(&(SNTPC_Config)),
                                        (UNSIGNED)(sizeof(SNTPC_CONFIG_STRUCT)),
                                        (UNSIGNED)NU_NO_SUSPEND);
            if (status == NU_SUCCESS)
            {
                /* Zero out the config structure. */
                memset(SNTPC_Config, 0, sizeof(SNTPC_CONFIG_STRUCT));

                /* Set the UDP socket to an uninitialized value. */
                SNTPC_Config->sntpc_socket = -1;

                /* Memory pool used by the Nucleus SNTP Client. */
                SNTPC_Config->sntpc_memory = sntpc_memory;

                /* Retrieve run-time initialization constants. */
                status = (REG_Has_Key(path) ? NU_SUCCESS : NU_INVAL);
                if (status == NU_SUCCESS)
                {
                    /* Set the maximum number of servers */
                    strncpy(tmp_path, path, strlen(path)+1);
                    strncat(tmp_path, SNTPC_REG_OPT_MAX_SERVERS,
                            REG_MAX_KEY_LENGTH-strlen(tmp_path));

                    status = REG_Get_UINT16(tmp_path,
                        &(SNTPC_Config->sntpc_server_list.sntpc_max_servers));
                    if (status == NU_SUCCESS)
                    {
                        /* Create the semaphore */
                        status = NU_Create_Semaphore(
                                        &SNTPC_Config->sntpc_semaphore,
                                        "SNTPC_Semaphore", 1, NU_FIFO);

#if CFG_NU_OS_NET_SNTPC_USE_DEFAULT_SERVER
                        if ((status == NU_SUCCESS) &&
                            strlen(CFG_NU_OS_NET_SNTPC_DEFAULT_SERVER) != 0)
                        {
                            server.sntpc_poll_interval = 1800;
                            server.sntpc_server_addr.port = 123;
                            server.sntpc_server_hostname =
                                CFG_NU_OS_NET_SNTPC_DEFAULT_SERVER;
                            server.sntpc_server_addr.family = NU_FAMILY_IP;

                            status = SNTPC_Add_Server(&server);
                        }
#endif /* CFG_NU_OS_NET_SNTPC_USE_DEFAULT_SERVER */
                    }
                }
            }
        }
    }
    else
    {
        /* Stop requested. */

        if (SNTPC_Config != NU_NULL)
        {
            /* Remove all servers */
            SNTPC_Purge_Server_List();

            status = NU_Delete_Semaphore(&SNTPC_Config->sntpc_semaphore);

            /* Clean-up after server discovery failure */
            tmp_status = NU_Deallocate_Memory(SNTPC_Config);
            if (tmp_status == NU_SUCCESS)
            {
                SNTPC_Config = NU_NULL;
            }
        }
    }

    return (status);

} /* nu_os_net_sntpc_init */

/*************************************************************************
*
*   FUNCTION
*
*       SNTPC_Init_On_First_Server
*
*   DESCRIPTION
*
*       Initialization performed when the first time source is added
*       to the SNTP database. This is relatively delayed initialization
*       to save system resources until a time source is available.
*       This function creates the SNTP task and opens the UDP socket
*       of the client.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       NU_SUCCESS              Upon successful initialization.
*       OS error code           On failure.
*
*************************************************************************/
STATUS SNTPC_Init_On_First_Server(VOID)
{
    STATUS              status;
    INT                 sock;
    struct addr_struct  myaddr;

    /* Populate the "my address" structure. Use a random local port for
     * the client-side of the UDP communication. */
    myaddr.port = 0;
    myaddr.name = "SNTPC Client";
#ifdef CFG_NU_OS_NET_IPV6_ENABLE
    myaddr.family = NU_FAMILY_IP6;
    myaddr.id     = IP6_ADDR_ANY;
#else
    myaddr.family = NU_FAMILY_IP;
    PUT32(myaddr.id.is_ip_addrs, 0, IP_ADDR_ANY);
#endif

    /* If a previous SNTP Client resources are still allocated and
     * are likely scheduled for de-initialization but have not been deallocated
     * yet, then do not allocate any resources here. The de-initialization
     * code will detect addition of this server to the list and will
     * therefore abort the de-initialization sequence. */
    if (SNTPC_Config->sntpc_server_update_task != NU_NULL)
    {
        return NU_SUCCESS;
    }

    /* Open a UDP socket for communication with SNTP servers. */
    sock = NU_Socket(myaddr.family, NU_TYPE_DGRAM, 0);

    if (sock >= 0)
    {
        /* Bind to the ANY address. */
        status = NU_Bind(sock, &myaddr, 0);
        if (status == sock)
        {
            SNTPC_Config->sntpc_socket = sock;
            status = NU_SUCCESS;
        }
    }
    else
    {
        status = (STATUS)sock;
    }

    if(status == NU_SUCCESS)
    {
        /* Now create the SNTP update task. */
        status = NU_Create_Auto_Clean_Task(
                        &SNTPC_Config->sntpc_server_update_task,
                        "SNTPC_S", SNTPC_Server_Update_Task, 0,
                        NU_NULL, SNTPC_Config->sntpc_memory,
                        SNTPC_TASK_STACK_SIZE,
                        SNTPC_TASK_PRIO, SNTPC_TASK_TIME_SLICE,
                        NU_PREEMPT, NU_NO_START);
    }

    if (status == NU_SUCCESS)
    {
        /* Allocate memory for the queue */
        status = NU_Allocate_Aligned_Memory(SNTPC_Config->sntpc_memory,
                        &SNTPC_Config->sntpc_queue_mem,
                        (SNTPC_Config->sntpc_server_list.sntpc_max_servers *
                         sizeof(SNTPC_SERVER_LIST *)), sizeof(UNSIGNED),
                        NU_NO_SUSPEND);
        if(status == NU_SUCCESS)
        {
            /* Register the "send request" queue. */
            status = NU_Create_Queue(&SNTPC_Config->sntpc_send_request_queue,
                         "SNTPC_Q",
                         SNTPC_Config->sntpc_queue_mem,
                         SNTPC_Config->sntpc_server_list.sntpc_max_servers,
                         NU_FIXED_SIZE, 1, NU_FIFO);
        }
    }

    if(status == NU_SUCCESS)
    {
        /* Start the task. */
        status = NU_Resume_Task(SNTPC_Config->sntpc_server_update_task);
    }

    return status;
}

/*************************************************************************
*
*   FUNCTION
*
*       SNTPC_Deinit_On_Last_Server
*
*   DESCRIPTION
*
*       De-initialization performed when the last time source is removed
*       from the SNTP database. This function destroys the SNTP task and
*       closes the UDP socket of the client.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       NU_SUCCESS              Upon successful de-initialization.
*
*************************************************************************/
STATUS SNTPC_Deinit_On_Last_Server(VOID)
{
    VOID *null_ptr = NU_NULL;

    /* Send a NULL address to the queue, which will signal the
     * "server update task" to free all SNTP Client resources and
     * then exit. */
    NU_Send_To_Queue(&SNTPC_Config->sntpc_send_request_queue,
                    (VOID *)&null_ptr, 1, NU_NO_SUSPEND);

    return NU_SUCCESS;
}

/*************************************************************************
*
*   FUNCTION
*
*       SNTPC_Server_List_Query
*
*   DESCRIPTION
*
*       Return a server list structure that matches the address.
*
*   INPUTS
*
*       *addr                   A pointer to the server address.
*       *server_hostname        The server's hostname. This is optional
*                               and can be NU_NULL if not specified.
*       **ret_server            A pointer-pointer to a server list structure
*                                that will contain the server details upon
*                                success.
*
*   OUTPUTS
*
*       NU_SUCCESS upon successful completion or an operating-system
*       specific error if no data was sent.
*
*       **ret_server            Upon success, server information will be
*                               returned.
*
*************************************************************************/
STATUS SNTPC_Server_List_Query(struct addr_struct *addr,
                               CHAR *server_hostname,
                               SNTPC_SERVER_LIST **ret_server)
{
    STATUS  status = NU_FAILED_QUERY;
    SNTPC_SERVER_LIST *temp;
    INT     addr_size;
#ifdef CFG_NU_OS_NET_IPV6_ENABLE
    UINT32  addr_param;
    UINT32  addr_db;
#endif

    /* Search server list for matching entry */
    temp = SNTPC_Config->sntpc_server_list.sntpc_head;

    while (temp != NU_NULL)
    {
        if (temp->sntpc_server.sntpc_server_addr.port == addr->port)
        {
            /* If the "server_hostname" parameter is non-NULL, then only
             * try to compare the hostnames and ignore the IP address. */
            if (server_hostname != NU_NULL)
            {
                /* Compare the hostnames and the family type. */
                if ((temp->sntpc_server.sntpc_server_hostname != NU_NULL) &&
                    (temp->sntpc_server.sntpc_server_addr.family == addr->family) &&
                    (strcmp(server_hostname,
                        temp->sntpc_server.sntpc_server_hostname)
                        == 0))
                {
                    *ret_server = temp;
                    status = NU_SUCCESS;
                    break;
                }
            }
            /* Otherwise, compare the IP addresses. But only if the IP
             * address has been resolved. */
            else if (temp->sntpc_hostname_resolved == NU_TRUE)
            {
#if (CFG_NU_OS_NET_STACK_INCLUDE_IPV4) && defined(CFG_NU_OS_NET_IPV6_ENABLE)
                if (temp->sntpc_server.sntpc_server_addr.family == NU_FAMILY_IP)
                    addr_size = IP_ADDR_LEN;
                else
#endif
                    addr_size = MAX_ADDRESS_SIZE;

                /* If the address families are same then the comparison
                 * is straight forward. */
                if (temp->sntpc_server.sntpc_server_addr.family == addr->family)
                {
                    if (0 == memcmp(
                        temp->sntpc_server.sntpc_server_addr.id.is_ip_addrs,
                        addr->id.is_ip_addrs, addr_size))
                    {
                        *ret_server = temp;
                        status = NU_SUCCESS;
                        break;
                    }
                }
#ifdef CFG_NU_OS_NET_IPV6_ENABLE
                /* Otherwise, if the address families are different, then
                 * there is a possibility that the IPv6 address is actually
                 * an IPv4-mapped-IPv6 address which must be compared
                 * accordingly. */
                else
                {
                    /* Convert first IP address to IPv4 form. */
                    if (addr->family == NU_FAMILY_IP)
                    {
                        addr_param = IP_ADDR(&addr->id.is_ip_addrs[0]);
                    }
                    else if (IPV6_IS_ADDR_V4MAPPED(addr->id.is_ip_addrs))
                    {
                        addr_param = IP_ADDR(&addr->id.is_ip_addrs[12]);
                    }
                    else
                        continue;

                    /* Convert second IP address to IPv4 form. */
                    if (temp->sntpc_server.sntpc_server_addr.family
                            == NU_FAMILY_IP)
                    {
                        addr_db = IP_ADDR(
                            &temp->sntpc_server.sntpc_server_addr.
                            id.is_ip_addrs[0]);
                    }
                    else if (IPV6_IS_ADDR_V4MAPPED(
                                temp->sntpc_server.sntpc_server_addr.
                                id.is_ip_addrs))
                    {
                        addr_db = IP_ADDR(
                            &temp->sntpc_server.sntpc_server_addr.
                            id.is_ip_addrs[12]);
                    }
                    else
                        continue;

                    /* Compare the two IP addresses. */
                    if (addr_param == addr_db)
                    {
                        *ret_server = temp;
                        status = NU_SUCCESS;
                        break;
                    }
                }
#endif
            }
        }
        temp = temp->sntpc_next;
    }

    return(status);
}

/*************************************************************************
*
*   FUNCTION
*
*       SNTPC_Send_Request
*
*   DESCRIPTION
*
*       Sends an SNTP request packet to the specified server.
*       The UDP socket to the specified server should already be
*       open when this function is called.
*
*   INPUTS
*
*       *server                 The SNTP server to be requested.
*       *pkt                    The request packet to be sent to server.
*
*   OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_DATA_TRANSFER     If request failed to be sent.
*       OS error code           On failure.
*
*************************************************************************/
STATUS SNTPC_Send_Request(SNTPC_SERVER_LIST *server, SNTPC_PACKET *pkt)
{
    STATUS  status = NU_SUCCESS;
    UINT8   buffer[SNTPC_MIN_PACKET_LENGTH];
    INT     value;
    INT32   bytes_sent;

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if ((server == NU_NULL) || (pkt == NU_NULL))
    {
        return NU_INVALID_PARM;
    }
#endif

    /* Encode all the fields in the buffer. */

    value =  (pkt->sntpc_leap      << 30);
    value |= (pkt->sntpc_version   << 27);
    value |= (pkt->sntpc_mode      << 24);
    value |= (pkt->sntpc_stratum   << 16);
    value |= (pkt->sntpc_poll      << 8);
    value |= (pkt->sntpc_precision);

    PUT32(buffer, 0, value);

    PUT32(buffer, SNTPC_ROOT_DELAY_OFFSET, pkt->sntpc_root_delay);
    PUT32(buffer, SNTPC_ROOT_DISP_OFFSET, pkt->sntpc_root_disp);
    PUT32(buffer, SNTPC_REF_TS_OFFSET, pkt->sntpc_ref_ts.sntpc_seconds);
    PUT32(buffer, SNTPC_REF_TS_OFFSET + 4, pkt->sntpc_ref_ts.sntpc_fraction);
    PUT32(buffer, SNTPC_ORIG_TS_OFFSET, pkt->sntpc_orig_ts.sntpc_seconds);
    PUT32(buffer, SNTPC_ORIG_TS_OFFSET + 4, pkt->sntpc_orig_ts.sntpc_fraction);
    PUT32(buffer, SNTPC_RX_TS_OFFSET, pkt->sntpc_rx_ts.sntpc_seconds);
    PUT32(buffer, SNTPC_RX_TS_OFFSET + 4, pkt->sntpc_rx_ts.sntpc_fraction);
    PUT32(buffer, SNTPC_TX_TS_OFFSET, pkt->sntpc_tx_ts.sntpc_seconds);
    PUT32(buffer, SNTPC_TX_TS_OFFSET + 4, pkt->sntpc_tx_ts.sntpc_fraction);

    /* Copy the reference ID also. */
    memcpy(buffer + SNTPC_REF_ID_OFFSET, pkt->sntpc_ref_id, SNTPC_REF_ID_LENGTH);

    /* Send this packet. */
    bytes_sent = NU_Send_To(SNTPC_Config->sntpc_socket, (CHAR *)buffer,
                            SNTPC_MIN_PACKET_LENGTH, 0,
                        &server->sntpc_server.sntpc_server_addr, 0);

    if (bytes_sent != sizeof(buffer))
    {
        status = NU_NO_DATA_TRANSFER;
    }

    return status;
}

/*************************************************************************
*
* FUNCTION
*
*       SNTPC_Decode_Reply
*
* DESCRIPTION
*
*       Decodes an incoming SNTP packet from the server and returns
*       the decoded packet in the form of a packet structure.
*
* INPUTS
*
*       *buffer                 Packet to be decoded in raw form.
*       buffer_len              Length of data in buffer.
*       *pkt                    On return, this contains the decoded
*                               packet data.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       OS error code           On failure.
*
*************************************************************************/
STATUS SNTPC_Decode_Reply(UINT8 *buffer, INT buffer_len, SNTPC_PACKET *pkt)
{
    STATUS      status = NU_SUCCESS;
    INT         value;

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if ((buffer == NU_NULL) || (buffer_len == 0) || (pkt == NU_NULL))
    {
        return NU_INVALID_PARM;
    }
#endif

    /* Make sure the buffer contains enough data for an SNTP packet. */
    if (buffer_len >= SNTPC_MIN_PACKET_LENGTH)
    {
        /* Get multiple fields from the first 32-bits and decode them. */
        value = GET32(buffer, 0);

        pkt->sntpc_leap         = (value >> 30);
        pkt->sntpc_version      = (value >> 27) & 0x07;
        pkt->sntpc_mode         = (value >> 24) & 0x07;
        pkt->sntpc_stratum      = (value >> 16) & 0xff;
        pkt->sntpc_poll         = (value >> 8 ) & 0xff;
        pkt->sntpc_precision    = (value & 0xff);

        pkt->sntpc_root_delay = GET32(buffer, SNTPC_ROOT_DELAY_OFFSET);
        pkt->sntpc_root_disp = GET32(buffer, SNTPC_ROOT_DISP_OFFSET);
        pkt->sntpc_ref_ts.sntpc_seconds   = GET32(buffer, SNTPC_REF_TS_OFFSET);
        pkt->sntpc_ref_ts.sntpc_fraction  = GET32(buffer, SNTPC_REF_TS_OFFSET + 4);
        pkt->sntpc_orig_ts.sntpc_seconds  = GET32(buffer, SNTPC_ORIG_TS_OFFSET);
        pkt->sntpc_orig_ts.sntpc_fraction = GET32(buffer, SNTPC_ORIG_TS_OFFSET + 4);
        pkt->sntpc_rx_ts.sntpc_seconds    = GET32(buffer, SNTPC_RX_TS_OFFSET);
        pkt->sntpc_rx_ts.sntpc_fraction   = GET32(buffer, SNTPC_RX_TS_OFFSET + 4);
        pkt->sntpc_tx_ts.sntpc_seconds    = GET32(buffer, SNTPC_TX_TS_OFFSET);
        pkt->sntpc_tx_ts.sntpc_fraction   = GET32(buffer, SNTPC_TX_TS_OFFSET + 4);

        /* Copy the reference ID and null-terminate it. */
        memcpy(pkt->sntpc_ref_id, buffer + SNTPC_REF_ID_OFFSET, SNTPC_REF_ID_LENGTH);
        pkt->sntpc_ref_id[SNTPC_REF_ID_LENGTH] = '\0';
    }
    else
    {
        /* Not enough data in buffer. */
        status = NU_INVALID_PARM;
    }

    return status;
}

/*************************************************************************
*
* FUNCTION
*
*       SNTPC_Process_Reply
*
* DESCRIPTION
*
*       Processes a decoded SNTP packet coming from the server and
*       updates the internal "time source" variables accordingly
*       within the local database. This function applies the
*       "on-wire protocol" to the incoming packet.
*
* INPUTS
*
*       *server                 Server from which the packet is received.
*       *packet                 Decoded SNTP packet.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       OS error code           On failure.
*
*************************************************************************/
STATUS SNTPC_Process_Reply(SNTPC_SERVER_LIST *server, SNTPC_PACKET *packet)
{
    STATUS      status = NU_SUCCESS;

    /* Run on-wire protocol */
    SNTPC_On_Wire_Protocol(server, packet);

    return status;
}

/*************************************************************************
*
*   FUNCTION
*
*       SNTPC_Get_Timestamp_From_Server
*
*   DESCRIPTION
*
*       Returns the current time relative to the specified
*       server in SMTP UTC timestamp format. This time is not
*       adjusted for timezone and DST.
*
*   INPUTS
*
*       *current_time           Contains the current time on return.
*       *server                 Server with respect to which the time
*                               should be returned.
*
*   OUTPUTS
*
*       NU_SUCCESS              Upon successful completion.
*       SNTPC_NOT_SYNCED        If time with the remote server has not
*                               been synced for the given server.
*
*************************************************************************/
STATUS SNTPC_Get_Timestamp_From_Server(SNTPC_TIMESTAMP *current_time,
                                       SNTPC_SERVER_LIST *server)
    {
    STATUS              status = NU_SUCCESS;
    UNSIGNED            ticks;

#if INCLUDE_NET_API_ERR_CHECK
    if ((current_time == NU_NULL) || (server == NU_NULL))
    {
        return NU_INVALID_PARM;
    }
#endif

    /* Store the server's timestamp into the current time structure. */
    current_time->sntpc_seconds = server->sntpc_last_server_time.sntpc_seconds;
    current_time->sntpc_fraction = server->sntpc_last_server_time.sntpc_fraction;

    /* Now calculate the time since the last server update and add that
     * to the current time. */
    ticks = NU_Retrieve_Clock() - server->sntpc_last_plus_ticks;
    current_time->sntpc_seconds += ticks / NU_PLUS_Ticks_Per_Second;
    ticks = ticks % NU_PLUS_Ticks_Per_Second;

    /* Convert ticks to fraction and add to the current time. */
    current_time->sntpc_fraction +=
            ticks * (0xffffffff / NU_PLUS_Ticks_Per_Second);

    /* Adjust time after calculations if fraction overflowed. */
    if (current_time->sntpc_fraction <
        server->sntpc_last_server_time.sntpc_fraction)
    {
        current_time->sntpc_seconds += 1;
    }

    /* If this time source is not synchronized then return the
     * time but also return a status indicating this condition. */
    if (server->sntpc_synced == NU_FALSE)
    {
        status = SNTPC_NOT_SYNCED;
    }

    return(status);
}

/*************************************************************************
*
* FUNCTION
*
*       SNTPC_Log2
*
* DESCRIPTION
*
*       This function calculates the integer part of "log2(n)".
*
* INPUTS
*
*       n                       Number for which to calculate log2(n).
*
* OUTPUTS
*
*       Result of log2(n).
*
*************************************************************************/
INT SNTPC_Log2(UINT32 n)
{
    INT result = -1;
    while (n)
    {
        n = n >> 1;
        result++;
    }
    return result;
}

/*************************************************************************
*
* FUNCTION
*
*       SNTPC_Send_Request_Handler
*
* DESCRIPTION
*
*       This function is the timer handler for the SNTP Client
*       "send request" event. It is a callback called by Nucleus
*       when the polling timer expires. Since this is executing as a
*       HISR, the server address is added to a queue, from which
*       it will be processed by SNTPC_Server_Update_Task.
*
* INPUTS
*
*       dat                     Pointer to a server list structure.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID SNTPC_Send_Request_Handler(UNSIGNED dat)
{
    /* Queue the request. */
    NU_Send_To_Queue (&SNTPC_Config->sntpc_send_request_queue,
                      (VOID *)&dat, 1, NU_NO_SUSPEND);

    /* Resume the server update task because this task might be waiting
     * in an NU_Select() call. */
    NU_Resume_Task(SNTPC_Config->sntpc_server_update_task);
}

/*************************************************************************
*
* FUNCTION
*
*       SNTPC_Generate_Request
*
* DESCRIPTION
*
*       This function generates an SNTP request for the specified
*       SNTP Server and sends the request to the server.
*
* INPUTS
*
*       *server                 Pointer to an SNTP Server information
*                               to which the request should be sent.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       OS error code           On failure.
*
*************************************************************************/
STATUS SNTPC_Generate_Request(SNTPC_SERVER_LIST *server)
{
    SNTPC_PACKET    packet;
    SNTPC_TIMESTAMP current_time;
    NU_HOSTENT      *hentry;
    STATUS          status;

    /* Check whether address of this server has not been resolved. */
    if (server->sntpc_hostname_resolved == NU_FALSE)
    {
        /* Resolve hostname by DNS. */
        hentry = NU_Get_IP_Node_By_Name(
                            server->sntpc_server.sntpc_server_hostname,
                            server->sntpc_server.sntpc_server_addr.family,
                            DNS_DEFAULT, &status);
        if ((status == NU_SUCCESS) && (hentry != NU_NULL))
        {
            /* Just use the first address in the list. */
            memcpy(server->sntpc_server.sntpc_server_addr.id.is_ip_addrs,
                   *(hentry->h_addr_list), hentry->h_length);
            server->sntpc_server.sntpc_server_addr.family =
                    hentry->h_addrtype;

            server->sntpc_hostname_resolved = NU_TRUE;

            /* Free the host entry structure. */
            NU_Free_Host_Entry(hentry);
        }
        else
        {
            /* If we were unable to resolve the address, then do not
             * wait for the poll interval to re-attempt the resolution
             * and instead reschedule the attempt earlier than that. */
            NU_Reset_Timer(&server->sntpc_timer,
                    SNTPC_Send_Request_Handler,
                    (NU_TICKS_PER_SECOND * SNTPC_REATTEMPT_RESOLVE_TIME),
                    (NU_TICKS_PER_SECOND *
                     server->sntpc_server.sntpc_poll_interval),
                    NU_ENABLE_TIMER);

            return status;
        }
    }

    /* Obtain the current time for the given server. */
    status = SNTPC_Get_Timestamp_From_Server(&current_time, server);
    if (status == SNTPC_NOT_SYNCED)
    {
        /* Continue even with an unsynchronized time. */
        status = NU_SUCCESS;
    }

    /* Zero out all fields of the packet. */
    memset(&packet, 0, sizeof(SNTPC_PACKET));

    /* Obtain the semaphore for further processing. */
    status = NU_Obtain_Semaphore(&SNTPC_Config->sntpc_semaphore,
                                 NU_SUSPEND);
    if (status == NU_SUCCESS)
    {
        /* Configure the packet */
        packet.sntpc_version = SNTPC_VERSION;
        packet.sntpc_mode = SNTPC_MODE;
        packet.sntpc_stratum = SNTPC_STRATUM;
        packet.sntpc_poll = (UINT8)SNTPC_Log2(
                        server->sntpc_server.sntpc_poll_interval);

        /* Set the transmit timestamp */
        packet.sntpc_tx_ts.sntpc_seconds = current_time.sntpc_seconds;
        packet.sntpc_tx_ts.sntpc_fraction = current_time.sntpc_fraction;

        /* Send the packet. */
        status = SNTPC_Send_Request(server, &packet);

        NU_Release_Semaphore(&SNTPC_Config->sntpc_semaphore);
    }

    return status;
}

/*************************************************************************
*
* FUNCTION
*
*       SNTPC_Server_Update_Task
*
* DESCRIPTION
*
*       This is the entry point of the SNTP "update task" which
*       is responsible for processing all incoming SNTP responses
*       from different time sources.
*
* INPUTS
*
*       argc                    Number of parameters.
*       *argv                   Array of parameters.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID SNTPC_Server_Update_Task(UNSIGNED argc, VOID *argv)
{
    STATUS          status = NU_INVALID_PARM;
    INT16           unused_parm = 0;
    INT             udp_socket;
    INT             bytes_received;
    FD_SET          readfs;
    SNTPC_PACKET    packet;
    UINT8           rx_buffer[SNTPC_MAX_PACKET_LENGTH];
    struct addr_struct server_addr;
    SNTPC_SERVER_LIST *server = NU_NULL;
    UNSIGNED        message_size = 0;
    BOOLEAN         task_is_exiting = NU_FALSE;

    /* Loop forever. */
    while (task_is_exiting != NU_TRUE)
    {
        udp_socket = SNTPC_Config->sntpc_socket;

        /* Make sure all bits are initially set to 0 */
        NU_FD_Init(&readfs);

        /* Set the bit for the udp_socket. */
        NU_FD_Set(udp_socket, &readfs);

        /* Wait for a reply. This is a timed wait and this call may
         * be interrupted before the timeout by a timer expiring in
         * SNTPC_Send_Request_Handler() which will result in this
         * call returning with an error status. Therefore, right after
         * this NU_Select() call we need to check whether anything is
         * received in the "send request queue". */
        status = NU_Select(udp_socket + 1, &readfs, NU_NULL, NU_NULL,
                           SNTPC_RECV_TIMEOUT * NU_PLUS_Ticks_Per_Second);
        if (status == NU_SUCCESS)
        {
            bytes_received = NU_Recv_From(udp_socket, (CHAR *)rx_buffer,
                                          sizeof(rx_buffer), 0,
                                          &server_addr, &unused_parm);

            if (bytes_received > 0)
            {
                /* Set all fields of the receive packet to zero. */
                memset(&packet, 0, sizeof(packet));

                /* Decode the reply. */
                status = SNTPC_Decode_Reply(rx_buffer, bytes_received, &packet);

                if (status == NU_SUCCESS)
                {
                    /* Obtain the semaphore for further processing. */
                    status = NU_Obtain_Semaphore(&SNTPC_Config->sntpc_semaphore,
                                                 NU_SUSPEND);
                    if (status == NU_SUCCESS)
                    {
                        /* Search the database to match this response
                         * against a time source in the database. */
                        status = SNTPC_Server_List_Query(&server_addr,
                                                        NU_NULL, &server);
                        if (status == NU_SUCCESS)
                        {
                            SNTPC_Process_Reply(server, &packet);
                        }

                        NU_Release_Semaphore(&SNTPC_Config->sntpc_semaphore);
                    }
                }
            }
        }

        /* Service the "send request queue" until it is empty. */
        do
        {
            /* Check if anything is available in the "send request queue". */
            status = NU_Receive_From_Queue(&SNTPC_Config->sntpc_send_request_queue,
                                           (VOID *)&server,
                                           1,
                                           &message_size, NU_NO_SUSPEND);

            if (status == NU_SUCCESS)
            {
                /* If a valid server pointer has been specified. */
                if (server != NU_NULL)
                {
                    /* Generate/send the SNTP Request to the server. */
                    SNTPC_Generate_Request(server);
                }

                /* A NULL server pointer implies that the "server update
                 * task" is shutting down so it should clean up all
                 * SNTP Client resources. This usually happens when the
                 * last SNTP Server in the list has been removed. */
                else
                {
                    /* Obtain the semaphore for further processing. */
                    status = NU_Obtain_Semaphore(
                                &SNTPC_Config->sntpc_semaphore, NU_SUSPEND);
                    if (status == NU_SUCCESS)
                    {
                        /* If the server list is not empty then abort the
                         * de-initialization sequence. This condition may
                         * occur if the user removes a server and adds one
                         * back to the list before this code has executed. */
                        if (SNTPC_Config->sntpc_server_list.sntpc_count != 0)
                        {
                            NU_Release_Semaphore(&SNTPC_Config->sntpc_semaphore);
                            continue;
                        }

                        /* Close the UDP socket if it is open. */
                        if (SNTPC_Config->sntpc_socket >= 0)
                        {
                            NU_Close_Socket(SNTPC_Config->sntpc_socket);
                            SNTPC_Config->sntpc_socket = -1;
                        }

                        /* Delete the "send request" queue. */
                        status = NU_Delete_Queue(
                                &SNTPC_Config->sntpc_send_request_queue);
                        if (status == NU_SUCCESS)
                        {
                            memset(&SNTPC_Config->sntpc_send_request_queue,
                                    0, sizeof(NU_QUEUE));
                            /* Deallocate memory for the queue */
                            status = NU_Deallocate_Memory(
                                        SNTPC_Config->sntpc_queue_mem);
                            if (status == NU_SUCCESS)
                            {
                                SNTPC_Config->sntpc_queue_mem = NU_NULL;
                            }
                        }

                        /* Clear the best server */
                        SNTPC_Config->sntpc_server_list.sntpc_owp_best = NU_NULL;

                        /* Clear the task pointer. This is a self-cleaning
                         * task so this need not be deallocated. */
                        SNTPC_Config->sntpc_server_update_task = NU_NULL;

                        NU_Release_Semaphore(&SNTPC_Config->sntpc_semaphore);
                    }

                    /* Set the flag to exit the main loop of this task. */
                    task_is_exiting = NU_TRUE;
                    break;
                }
            }
        } while (status == NU_SUCCESS);
    }
}

/*************************************************************************
*
* FUNCTION
*
*       SNTPC_On_Wire_Protocol
*
* DESCRIPTION
*
*       This function calculates the delay and offset for a server.
*       It also updates the pointer to the most accurate server,
*       "sntpc_owp_best" in the config structure.
*
* INPUTS
*
*       *server                 Pointer to a server list structure
*       *svr_packet             Pointer to a packet structure
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID SNTPC_On_Wire_Protocol(SNTPC_SERVER_LIST *server, SNTPC_PACKET *svr_packet)
{
    UINT64 T1, T2, T3, T4;
    SNTPC_TIMESTAMP current_time;
    STATUS status;
    SNTPC_SERVER_LIST *temp = SNTPC_Config->sntpc_server_list.sntpc_head;

    /* Convert to 64-bit values */
    T1 = svr_packet->sntpc_orig_ts.sntpc_seconds;
    T1 <<= 32;
    T1 |= svr_packet->sntpc_orig_ts.sntpc_fraction;

    T2 = svr_packet->sntpc_rx_ts.sntpc_seconds;
    T2 <<= 32;
    T2 |= svr_packet->sntpc_rx_ts.sntpc_fraction;

    T3 = svr_packet->sntpc_tx_ts.sntpc_seconds;
    T3 <<= 32;
    T3 |= svr_packet->sntpc_tx_ts.sntpc_fraction;

    /* Calculate T4 */
    status = SNTPC_Get_Timestamp_From_Server(&current_time, server);
    if ((status != NU_SUCCESS) && (status != SNTPC_NOT_SYNCED))
    {
        /* Unable to get the current time. Abort calculation. */
        return;
    }

    T4 = current_time.sntpc_seconds;
    T4 <<= 32;
    T4 |= current_time.sntpc_fraction;

    /* Offset calculation */
    server->sntpc_offset = ((T2 - T1) + (T3 - T4)) / 2;

    /* Round trip delay calculation */
    server->sntpc_delay = (T4 - T1) - (T3 - T2);

    /* Synchronize the clock for this server at this time. */
    server->sntpc_last_server_time.sntpc_seconds += server->sntpc_offset >> 32;
    server->sntpc_last_server_time.sntpc_fraction +=
                                        server->sntpc_offset & 0xffffffff;
    server->sntpc_last_plus_ticks = NU_Retrieve_Clock();
    server->sntpc_synced = NU_TRUE;

    if (SNTPC_Config->sntpc_server_list.sntpc_owp_best != NU_NULL)
    {
        /* Check if this server is currently the best */
        if (SNTPC_Config->sntpc_server_list.sntpc_owp_best != server)
        {
            /* Check if this server is more accurate than the current best */
            if ((server->sntpc_offset <
                 SNTPC_Config->sntpc_server_list.sntpc_owp_best->sntpc_offset) &&
                (server->sntpc_delay <
                 SNTPC_Config->sntpc_server_list.sntpc_owp_best->sntpc_delay))
            {
                SNTPC_Config->sntpc_server_list.sntpc_owp_best = server;
            }
        }
        else
        {
            /* Traverse the server list to see if any other server is better */
            while (temp)
            {
                if ((temp->sntpc_offset <
                     SNTPC_Config->sntpc_server_list.sntpc_owp_best->sntpc_offset) &&
                    (temp->sntpc_delay <
                     SNTPC_Config->sntpc_server_list.sntpc_owp_best->sntpc_delay))
                {
                    SNTPC_Config->sntpc_server_list.sntpc_owp_best = temp;
                }
                temp = temp->sntpc_next;
            }
        }
    }
    else
    {
        /* No servers currently set as best, so use this server */
        SNTPC_Config->sntpc_server_list.sntpc_owp_best = server;
    }
}
