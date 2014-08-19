/*************************************************************************
*
*               Copyright 2005 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       ike_task.c
*
* COMPONENT
*
*       IKE - Task
*
* DESCRIPTION
*
*       This file implements the functions specific to the
*       IKE task.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IKE_Create_UDP_Socket
*       IKE_Get_Datagram_Group
*       IKE_Get_Datagram_Group6
*       IKE_Task_Entry
*
* DEPENDENCIES
*
*       nu_net.h
*       nu_net6.h
*       ips_api.h
*       ike_api.h
*       ike_buf.h
*       ike_task.h
*
*************************************************************************/
#include "networking/nu_net.h"
#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/nu_net6.h"
#endif
#include "networking/ips_api.h"
#include "networking/ike_api.h"
#include "networking/ike_buf.h"
#include "networking/ike_task.h"
#include "storage/pcdisk.h"

/* Local function prototypes. */
STATIC VOID   IKE_Create_Sockets(VOID);
STATIC STATUS IKE_Create_UDP_Socket(INT *socket, UINT16 port_num);

#if (INCLUDE_IPV4 == NU_TRUE)
STATIC STATUS IKE_Get_Datagram_Group(UINT8 *if_addr,
                                     UINT32 *if_index,
                                     IKE_POLICY_GROUP **group);
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
STATIC STATUS IKE_Get_Datagram_Group6(UINT32 if_index,
                                      IKE_POLICY_GROUP **group);
#endif

#if (IKE_INCLUDE_RSA == NU_TRUE)
STATIC VOID IKE_Setup_FS(VOID);
#endif

extern VOID   ERC_System_Error (INT);

/*************************************************************************
*
* FUNCTION
*
*       IKE_Create_UDP_Socket
*
* DESCRIPTION
*
*       This is a utility function called by the IKE task. It
*       first closes the given socket, if it is greater than
*       zero and then creates a new UDP socket for sending and
*       receiving IKE packets.
*
* INPUTS
*
*       *socket                 On return, this contains a socket.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_NO_SOCK_MEMORY       No memory for creating socket.
*       NU_INVALID_SOCKET       Bind failed due to invalid socket.
*       NU_INVALID_PARM         Bind failed due to invalid parameter.
*       NU_INVALID_ADDRESS      Bind failed due to invalid address.
*
*************************************************************************/
STATIC STATUS IKE_Create_UDP_Socket(INT *socket, UINT16 port_num)
{
    STATUS              status;
    struct addr_struct  addr;
    INT16               family;

    /* Log debug message. */
    IKE_DEBUG_LOG("Creating IKE UDP socket");

    /* Check if the socket is already open. */
    if((*socket) >= 0)
    {
        /* Close the socket. */
        if(NU_Close_Socket((*socket)) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to close IKE socket",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    /* Set the address family. */
#if (INCLUDE_IPV6 == NU_FALSE)
    family = NU_FAMILY_IP;
#else
    family = NU_FAMILY_IP6;
#endif

    /* Create a new socket. */
    (*socket) = NU_Socket(family, NU_TYPE_DGRAM, 0);

    /* If socket creation failed. */
    if((*socket) < 0)
    {
        NLOG_Error_Log("Failed to create IKE socket",
                       NERR_RECOVERABLE, __FILE__, __LINE__);

        /* Set error status. */
        status = (*socket);
    }

    else
    {
        /* Set socket options to allow obtaining interface address. */
#if (INCLUDE_IPV4 == NU_TRUE)
        status = NU_Setsockopt_IP_RECVIFADDR((*socket), NU_TRUE);

        if(status == NU_SUCCESS)
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
        {
            status = NU_Setsockopt_IPV6_RECVPKTINFO((*socket), NU_TRUE);
        }

        if(status == NU_SUCCESS)
#endif

        {
            /* Initialize the address struct. */
            addr.family = family;
            addr.name = "IKE_Sock";
            addr.port = port_num;

            /* Set IP address to zero. */
#if (INCLUDE_IPV6 == NU_FALSE)
            *(UINT32 *)addr.id.is_ip_addrs = IP_ADDR_ANY;
#else
            addr.id = IP6_ADDR_ANY;
#endif

            /* Bind the address to the socket. */
            status = NU_Bind((*socket), &addr, 0);

            if(status < 0)
            {
                NLOG_Error_Log("Failed to bind address to IKE socket",
                               NERR_RECOVERABLE, __FILE__, __LINE__);

                /* Close the socket because the bind failed. */
                if(NU_Close_Socket((*socket)) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to close IKE socket",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            else
            {
                /* On success, socket is returned by the bind call
                 * so explicitly set status to success.
                 */
                status = NU_SUCCESS;
            }
        }

        else
        {
            NLOG_Error_Log("Unable to set required socket options",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    /* Return the status. */
    return (status);

} /* IKE_Create_UDP_Socket */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Create_Sockets
*
* DESCRIPTION
*
*       This is a utility function which creates the necessary sockets
*       needed to be used for receiving IKE and IKEv2 packets. If NAT
*       traversal is enabled, this function creates the port 4500 which
*       is used specifically with NAT traversal in IKE.
*
* INPUTS
*
*       None.
*
* OUTPUTS
*
*       None.
*
*************************************************************************/
STATIC VOID IKE_Create_Sockets(VOID)
{
    /* Create a UDP socket. */
    while(IKE_Create_UDP_Socket(&IKE_Data.ike_socket, IKE_RECV_UDP_PORT)
          != NU_SUCCESS)
    {
        /* Wait a while before trying again. */
        NU_Sleep(TICKS_PER_SECOND);
        continue;
    }

#if (IKE_ENABLE_NAT_TRAVERSAL == NU_TRUE)
    /* Create a UDP socket to be used for NAT traversal. */
    while(IKE_Create_UDP_Socket(&IKE_Data.ike_natt_socket,
        IKE_RECV_NATT_UDP_PORT) != NU_SUCCESS)
    {
        /* Wait a while before trying again. */
        NU_Sleep(TICKS_PER_SECOND);
        continue;
    }
#endif

}

#if (INCLUDE_IPV4 == NU_TRUE)
/*************************************************************************
*
* FUNCTION
*
*       IKE_Get_Datagram_Group
*
* DESCRIPTION
*
*       This is a utility function which returns a pointer to
*       the group containing the interface which received the
*       last datagram on the IKE socket. The index of the
*       interface which received the packet is also returned.
*
* INPUTS
*
*       *if_addr                Address of interface which received
*                               last datagram on IKE socket.
*       *if_index               On return, this contains index
*                               of the device which received the
*                               last UDP packet.
*       **group                 On return, this contains a pointer
*                               to the group.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_TIMEOUT              Timed-out waiting for a resource.
*       IKE_NOT_FOUND           Either the interface was not
*                               found or the interface has
*                               not been added to any group.
*
*************************************************************************/
STATIC STATUS IKE_Get_Datagram_Group(UINT8 *if_addr,
                                     UINT32 *if_index,
                                     IKE_POLICY_GROUP **group)
{
    STATUS          status;
    DV_DEVICE_ENTRY *dev_entry;

    /* Grab the NET semaphore. */
    status = NU_Obtain_Semaphore(&TCP_Resource, IKE_TIMEOUT);

    if(status == NU_SUCCESS)
    {
        /* Initialize group pointer to NULL. */
        *group = NU_NULL;

        /* Get device by IP address. */
        dev_entry = DEV_Get_Dev_By_Addr(if_addr);

        if(dev_entry != NU_NULL)
        {
            /* Return group and index. */
            *group = (IKE_POLICY_GROUP*)
                     dev_entry->dev_physical->dev_phy_ike_group;

            *if_index = dev_entry->dev_index;
        }

        /* If group is still NULL then return error. */
        if(*group == NU_NULL)
        {
            /* Set error status. */
            status = IKE_NOT_FOUND;
        }

        /* Release the NET semaphore. */
        if(NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release the semaphore",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to obtain NET semaphore",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_Get_Datagram_Group */
#endif /* (INCLUDE_IPV4 == NU_TRUE) */

#if (INCLUDE_IPV6 == NU_TRUE)
/*************************************************************************
*
* FUNCTION
*
*       IKE_Get_Datagram_Group6
*
* DESCRIPTION
*
*       This is a utility function which returns a pointer to
*       the group containing the interface which received the
*       last datagram on the IKE IPv6 socket.
*
* INPUTS
*
*       if_index                Index of interface which received
*                               last datagram.
*       **group                 On return, this contains a pointer
*                               to the group.
*
* OUTPUTS
*
*       NU_SUCCESS              On success.
*       NU_TIMEOUT              Timed-out waiting for a resource.
*       IKE_NOT_FOUND           Either the interface was not
*                               found or the interface has
*                               not been added to any group.
*
*************************************************************************/
STATIC STATUS IKE_Get_Datagram_Group6(UINT32 if_index,
                                      IKE_POLICY_GROUP **group)
{
    STATUS          status;
    DV_DEVICE_ENTRY *dev_entry;

    /* Grab the NET semaphore. */
    status = NU_Obtain_Semaphore(&TCP_Resource, IKE_TIMEOUT);

    if(status == NU_SUCCESS)
    {
        /* Initialize group pointer to NULL. */
        *group = NU_NULL;

        /* Get device by IP address. */
        dev_entry = DEV_Get_Dev_By_Index(if_index);

        if(dev_entry != NU_NULL)
        {
            *group = (IKE_POLICY_GROUP*)
                dev_entry->dev_physical->dev_phy_ike_group;
        }

        /* If group is still NULL then return error. */
        if(*group == NU_NULL)
        {
            /* Set error status. */
            status = IKE_NOT_FOUND;
        }

        /* Release the NET semaphore. */
        if(NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release the semaphore",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to obtain NET semaphore",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    /* Return the status. */
    return (status);

} /* IKE_Get_Datagram_Group6 */
#endif /* (INCLUDE_IPV6 == NU_TRUE) */

/*************************************************************************
*
* FUNCTION
*
*       IKE_Setup_FS
*
* DESCRIPTION
*
*       This function setups the disk and path to get certificate files
*       from, as specified in the metadata file.
*
* INPUTS
*
*       None.
*
* OUTPUTS
*
*       None.
*
*************************************************************************/
#if (IKE_INCLUDE_RSA == NU_TRUE)
STATIC VOID IKE_Setup_FS(VOID)
{
    STATUS status;
    CHAR disk[4];

    disk[0] = CFG_NU_OS_NET_IKE_DISK_DRIVE[0];
    disk[1] = ':';
    disk[2] = '\0';

    /* Wait for maximum storage devices to be initialized. */
    status = NU_Storage_Device_Wait(disk, NU_SUSPEND);
    if ((status != NU_SUCCESS) && (status != NU_TIMEOUT))
    {
        NLOG_Error_Log("Error waiting for run-level completion",
                       NERR_SEVERE, __FILE__, __LINE__);
        ERC_System_Error(status);
    }

    status = NU_Become_File_User();
    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("Error becoming file user",
                       NERR_SEVERE, __FILE__, __LINE__);
        ERC_System_Error(status);
    }

    /* Explore the mounted file systems. */

    status = NU_Open_Disk(disk);
    if (status == NU_SUCCESS)
    {
        status = NU_Set_Default_Drive(disk[0] - 'A');
        if (status == NU_SUCCESS)
        {
            status = NU_Set_Current_Dir(CFG_NU_OS_NET_IKE_CERT_DIR);
            if (status != NU_SUCCESS)
            {
                NU_Close_Disk(disk);
            }
        }
        else
        {
            NU_Close_Disk(disk);
        }
    }

    if (status != NU_SUCCESS)
    {
        NU_Release_File_User();
    }

} /* IKE_Setup_FS */
#endif

/*************************************************************************
*
* FUNCTION
*
*       IKE_Task_Entry
*
* DESCRIPTION
*
*       This function is the entry point for the IKE task.
*
* INPUTS
*
*       argc                    Argument count.
*       *argv                   Argument vector containing pointers
*                               to task arguments.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID IKE_Task_Entry(UNSIGNED argc, VOID *argv)
{
    STATUS              status;
    FD_SET              readfs;
    INT                 socket = 0;
    INT32               bytes;
    IKE_PACKET          pkt;
    IKE_POLICY_GROUP    *group = NU_NULL;
    msghdr              msg;
#if (INCLUDE_IPV6 == NU_TRUE)
    cmsghdr             *cmsg;
    in6_pktinfo         *pkt_info_ptr;
    UINT8               *pkt_info_buffer;
#if (INCLUDE_IPV4 == NU_TRUE)
    UINT32              ipv4_addr;
#endif
#endif
    NU_SUPERV_USER_VARIABLES

    UNUSED_PARAMETER(argc);
    UNUSED_PARAMETER(argv);

#if (IKE_INCLUDE_RSA == NU_TRUE)
    IKE_Setup_FS();
#endif

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Allocate the receive buffer. */
    if(IKE_Receive_Buffer == NU_NULL)
    {
        /* Memory allocation failed. */
        NLOG_Error_Log("Receive buffer has not been allocated",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

#if (INCLUDE_IPV6 == NU_TRUE)
    /* Allocate memory for ancillary data. */
    else if(NU_Allocate_Memory(IKE_Data.ike_memory,
                               (VOID**)&pkt_info_buffer,
                               NU_CMSG_SPACE(sizeof(in6_pktinfo)),
                               NU_NO_SUSPEND) != NU_SUCCESS)
    {
        /* Memory allocation failed. */
        NLOG_Error_Log("Failed to allocate ancillary data buffer",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }
#endif

    /* Otherwise, no error occurred. */
    else
    {
        /* Set packet data to the receive buffer. */
        pkt.ike_data = IKE_Receive_Buffer;

#if (INCLUDE_IPV6 == NU_TRUE)
        /* Normalize the pointer. */
        pkt_info_buffer = TLS_Normalize_Ptr(pkt_info_buffer);
#endif

        /* Loop for as long as stopping state is not reached. */
        while(IKE_Daemon_State != IKE_DAEMON_STOPPING_LISTEN)
        {
            IKE_Create_Sockets();

            /* Log debug message. */
            IKE_DEBUG_LOG("IKE task listening for incoming messages");

            /* Loop for as long as stopping state is not reached. */
            while(IKE_Daemon_State != IKE_DAEMON_STOPPING_LISTEN)
            {
                /* Empty the read descriptor set. */
                NU_FD_Init(&readfs);

                /* Add UDP socket to read descriptor set. */
                NU_FD_Set(IKE_Data.ike_socket, &readfs);
#if (IKE_ENABLE_NAT_TRAVERSAL == NU_TRUE)
                NU_FD_Set(IKE_Data.ike_natt_socket, &readfs);
#endif

#if (IKE_ENABLE_NAT_TRAVERSAL == NU_TRUE)
                /* Wait for data on the socket. */
                status = NU_Select(IKE_Data.ike_natt_socket + 1,
                    &readfs, NU_NULL, NU_NULL,
                    IKE_DAEMON_POLL_INTERVAL);

#else
                /* Wait for data on the socket. */
                status = NU_Select(IKE_Data.ike_socket + 1,
                                   &readfs, NU_NULL, NU_NULL,
                                   IKE_DAEMON_POLL_INTERVAL);
#endif

                if(status != NU_SUCCESS)
                {
                    /* If this is only a timeout, start over again. */
                    if(status == NU_NO_DATA)
                    {
                        continue;
                    }

                    else
                    {
                        /* Break out of inner loop and re-initialize
                         * the sockets.
                         */
                        break;
                    }
                }

                if(NU_FD_Check(IKE_Data.ike_socket, &readfs) == NU_TRUE)
                {
                    socket = IKE_Data.ike_socket;
                }
#if (IKE_ENABLE_NAT_TRAVERSAL == NU_TRUE)
                else if (NU_FD_Check(IKE_Data.ike_natt_socket, &readfs)
                         == NU_TRUE)
                {
                    socket = IKE_Data.ike_natt_socket;
                }
#endif

                if((socket == IKE_Data.ike_socket)
#if (IKE_ENABLE_NAT_TRAVERSAL == NU_TRUE)
                    || (socket == IKE_Data.ike_natt_socket)
#endif
                    )
                {
                    /* Fill the message header structure. */
                    msg.msg_name        = &pkt.ike_remote_addr;
                    msg.msg_namelen     = sizeof(pkt.ike_remote_addr);
                    msg.msg_iov         = (CHAR*)pkt.ike_data;
                    msg.msg_iovlen      = IKE_MAX_INBOUND_PACKET_LEN;
#if (INCLUDE_IPV6 == NU_TRUE)
                    msg.msg_control     = pkt_info_buffer;
                    msg.msg_controllen  =
                        NU_CMSG_SPACE(sizeof(in6_pktinfo));

                    /* Zero out the ancillary data buffer. */
                    UTL_Zero(pkt_info_buffer, msg.msg_controllen);
#else
                    msg.msg_control     = NU_NULL;
                    msg.msg_controllen  = 0;
#endif

                    /* Receive a UDP packet. */
                    bytes = NU_Recvmsg(socket, &msg, 0);

                    if(bytes < 0)
                    {
                        NLOG_Error_Log("Failed to receive UDP packet",
                            NERR_RECOVERABLE, __FILE__, __LINE__);

                        /* Break out of loop and re-create the socket. */
                        break;
                    }

                    else
                    {
                        /* Log debug message. */
                        IKE_DEBUG_LOG("--> Received IKE message");

#if (INCLUDE_IPV6 == NU_TRUE)
                        /* Initialize packet info pointer. */
                        pkt_info_ptr = NU_NULL;

#if (INCLUDE_IPV4 == NU_TRUE)
                        /* If remote address is an IPv4-mapped IPv6
                         * address.
                         */
                        if(NU_Is_IPv4_Mapped_Addr(
                            pkt.ike_remote_addr.id.is_ip_addrs) == NU_TRUE)
                        {
                            /* Override IP family of the remote address. */
                            pkt.ike_remote_addr.family = NU_FAMILY_IP;

                            /* Also convert the IPv4-mapped IPv6 address
                             * to simply an IPv4 address.
                             */
                            IP6_EXTRACT_IPV4_ADDR(ipv4_addr,
                                pkt.ike_remote_addr.id.is_ip_addrs);

                            PUT32(pkt.ike_remote_addr.id.is_ip_addrs, 0,
                                  ipv4_addr);

                            /* Get IPv4 address of interface which
                             * received the packet.
                             */
                            if(NU_Recv_IF_Addr(socket,
                                   pkt.ike_local_addr.id.is_ip_addrs)
                                   != NU_SUCCESS)
                            {
                                NLOG_Error_Log("Failed to get IF address",
                                    NERR_RECOVERABLE, __FILE__, __LINE__);

                                continue;
                            }
                        }

                        else
#endif
                        {
                            /* Get first (and only) item from the
                             * ancillary data buffer.
                             */
                            cmsg = NU_CMSG_FIRSTHDR(&msg);

                            /* If this is the interface address. */
                            if((cmsg             != NU_NULL) &&
                               (cmsg->cmsg_level == IPPROTO_IPV6) &&
                               (cmsg->cmsg_type  == IPV6_PKTINFO))
                            {
                                /* Extract interface address from
                                 * the buffer.
                                 */
                                pkt_info_ptr =
                                    (in6_pktinfo*)CMSG_DATA(cmsg);
                            }

                            else
                            {
                                /* Otherwise, ignore the packet. */
                                NLOG_Error_Log(
                                    "Ancillary data missing in IKE packet",
                                    NERR_RECOVERABLE, __FILE__, __LINE__);

                                continue;
                            }

                            /* Set address of interface which received
                             * the packet.
                             */
                            NU_BLOCK_COPY(
                                pkt.ike_local_addr.id.is_ip_addrs,
                                pkt_info_ptr->ipi6_addr, IP6_ADDR_LEN);

                            /* Set the interface index. */
                            pkt.ike_if_index = pkt_info_ptr->ipi6_ifindex;
                        }
#else
                        /* Get address of interface which received
                         * the packet.
                         */
                        if(NU_Recv_IF_Addr(socket,
                               pkt.ike_local_addr.id.is_ip_addrs)
                               != NU_SUCCESS)
                        {
                            NLOG_Error_Log("Failed to get IF address",
                                NERR_RECOVERABLE, __FILE__, __LINE__);

                            continue;
                        }
#endif

                        /* Set family and port of interface address. */
                        pkt.ike_local_addr.family =
                            pkt.ike_remote_addr.family;

                        pkt.ike_local_addr.port   = pkt.ike_local_addr.port;

                        /* Set length of the packet. */
                        pkt.ike_data_len = (UINT16)bytes;
#if (IKE_INCLUDE_VERSION_2 == NU_TRUE)
                        /* Remember the socket where the packet came from.*/
                        pkt.ike2_socket = socket;
#endif

                        /* Grab the IKE semaphore. */
                        if(NU_Obtain_Semaphore(&IKE_Data.ike_semaphore,
                                               IKE_TIMEOUT) != NU_SUCCESS)
                        {
                            NLOG_Error_Log(
                                "Failed to obtain IKE semaphore",
                                NERR_RECOVERABLE, __FILE__, __LINE__);
                        }

                        else
                        {
                             /* Get IKE group from the network interface
                             * which received the packet.
                             */
#if ((INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE))
                            if(pkt_info_ptr == NU_NULL)
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
                            {
                                status =
                                    IKE_Get_Datagram_Group(
                                        pkt.ike_local_addr.id.is_ip_addrs,
                                        &pkt.ike_if_index,
                                        &group);
                            }
#endif

#if ((INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE))
                            else
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
                            {
                                status = IKE_Get_Datagram_Group6(
                                             pkt_info_ptr->ipi6_ifindex,
                                             &group);
                            }
#endif

                            if(status == NU_SUCCESS)
                            {
                                /* Call the dispatch function. */
                                if(IKE_Dispatch(&pkt, group) != NU_SUCCESS)
                                {
                                    IKE_DEBUG_LOG(
                                        "Error in IKE packet processing");
                                }
                            }

                            else
                            {
                                NLOG_Error_Log("Failed to get IKE group",
                                    NERR_RECOVERABLE, __FILE__, __LINE__);
                            }

                            /* Release the IKE semaphore. */
                            if(NU_Release_Semaphore(
                                   &IKE_Data.ike_semaphore) != NU_SUCCESS)
                            {
                                NLOG_Error_Log(
                                    "Failed to release IKE semaphore",
                                    NERR_SEVERE, __FILE__, __LINE__);
                            }
                        }
                    }
                }

                socket = 0;
            }

            /* Close the socket if it is open. */
            if(IKE_Data.ike_socket >= 0)
            {
                if(NU_Close_Socket(IKE_Data.ike_socket) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to close IKE socket",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }

                /* Set socket value to indicate that it is not open. */
                IKE_Data.ike_socket = -1;
            }

#if (IKE_ENABLE_NAT_TRAVERSAL == NU_TRUE)
            if(IKE_Data.ike_natt_socket >= 0)
            {
                if(NU_Close_Socket(IKE_Data.ike_natt_socket) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to close IKE NATT socket",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
                }

                /* Set socket value to indicate that it is not open. */
                IKE_Data.ike_natt_socket = -1;
            }
#endif
        }

#if (INCLUDE_IPV6 == NU_TRUE)
        /* Deallocate the packet info buffer. */
        if(NU_Deallocate_Memory(pkt_info_buffer) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate memory",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
#endif

        /* If shutdown of this task was requested. */
        if(IKE_Daemon_State == IKE_DAEMON_STOPPING_LISTEN)
        {
            /* Request completed. Now move to next stop state. */
            IKE_Daemon_State = IKE_DAEMON_STOPPING_EVENTS;
        }
    }

    /* Log debug message. */
    IKE_DEBUG_LOG("IKE task terminated");

    /* Switch back to user mode. */
    NU_USER_MODE();

} /* IKE_Task_Entry */
