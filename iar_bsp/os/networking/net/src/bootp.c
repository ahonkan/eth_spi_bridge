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

/**************************************************************************
*
*   FILENAME
*
*       bootp.c
*
*   DESCRIPTION
*
*      This file will contain all the BOOTP routines.
*
*   DATA STRUCTURES
*
*      None.
*
*   FUNCTIONS
*
*      NU_Bootp
*      BOOTP_Process_Request
*      BOOTP_Init
*      BOOTP_Process_Packet
*
*   DEPENDENCIES
*
*      nu_net.h
*
****************************************************************************/

#include "networking/nu_net.h"

static UINT32 bootp_xid;

/* Local functions */

STATIC UINT16 BOOTP_Init (BOOTPLAYER *pkt, const BOOTP_STRUCT *out_bp);
STATIC STATUS BOOTP_Process_Packets(INT socketd, BOOTP_STRUCT *bp, UINT16 timeout);
STATIC STATUS BOOTP_Process_Request(INT socketd, UINT8 HUGE *work_buffer, UINT32 pkt_size,
                                    BOOTP_STRUCT *bp_ptr, const CHAR *dv_name,
                                    UINT8 hdrlen);

/* This list is used to hold on to the BOOTP request that will be sent. By using
   this list the need to build the request each time it is transmitted is
   avoided. Instead the request is built once and then reused if
   retransmissions are necessary. */
static  NET_BUFFER_HEADER BOOTP_List;

/**************************************************************************
*
*   FUNCTION
*
*       NU_Bootp
*
*   DESCRIPTION
*
*       This routine will do the main processing of bootp lookup request
*       calls.  The function will build the entire packet from the mac
*       layer to the BOOTP layer. It will send a bootp request packet,
*       and process the BOOTP REPLY that is sent from the BOOTP Server.
*       The code will also handle retries.
*
*   INPUTS
*
*       *dv_name                    Pointer to Devices name.
*       *bp_ptr                     Pointer to BOOTP structure.
*
* OUTPUTS
*
*       NU_SUCCESS                  Call was successful.
*       NU_INVALID_SEMAPHORE        Could not get the semaphore.
*       NU_INVALID_PARM             Invalid parameter.
*       NU_BOOTP_INIT_FAILED        General init failure.
*       NU_NO_BUFFERS               No buffers available for packet.
*       NU_BOOTP_ATTACH_IP_FAILED   Could not attach IP to device.
*       NU_BOOTP_SEND_FAILED        Could not send bootp packet.
*
****************************************************************************/
STATUS NU_Bootp(const CHAR *dv_name, BOOTP_STRUCT *bp_ptr)
{
    DV_DEVICE_ENTRY     *int_face;
    INT                 socketd;
    UINT8 HUGE          *buffer;
    struct addr_struct  clientaddr;
    STATUS              status;
    UINT32              pkt_size;
    UINT8               hdrlen;

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
    /*  Declare memory for the buffer space */
    struct bootp        boot_memory;
#endif

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Validate the input parameters. */
    if ( (dv_name != NU_NULL) && (bp_ptr != NU_NULL) )
    {
        /* Create a socket and bind to it, so that we can receive packets. */
        socketd = NU_Socket(NU_FAMILY_IP, NU_TYPE_DGRAM, NU_NONE);

        /* If the socket is valid */
        if (socketd >= 0)
        {
#if (INCLUDE_STATIC_BUILD == NU_FALSE)

            /* Allocate memory for the buffer space */
            status = NU_Allocate_Memory(MEM_Cached,(VOID **)&buffer,
                                        sizeof(BOOTPLAYER), NU_NO_SUSPEND);

#else
            /* Assign memory to the buffer */
            buffer = (UINT8 *)&boot_memory;
#endif

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
            if (status == NU_SUCCESS)
#endif
            {
                /* build local address and port to bind to. */
                clientaddr.family = NU_FAMILY_IP;
                clientaddr.port   = IPPORT_BOOTPC;
                clientaddr.id.is_ip_addrs[0] = (UINT8) 0;
                clientaddr.id.is_ip_addrs[1] = (UINT8) 0;
                clientaddr.id.is_ip_addrs[2] = (UINT8) 0;
                clientaddr.id.is_ip_addrs[3] = (UINT8) 0;
                clientaddr.name = "MGC";

                /* Bind the socket to the wildcard address */
                if (NU_Bind(socketd, &clientaddr, 0) == socketd)
                {
                    UTL_Zero(bp_ptr->bp_mac_addr, sizeof(bp_ptr->bp_mac_addr));
                    UTL_Zero(bp_ptr->bp_siaddr, sizeof(bp_ptr->bp_siaddr));
                    UTL_Zero(bp_ptr->bp_giaddr, sizeof(bp_ptr->bp_giaddr));
                    UTL_Zero(bp_ptr->bp_file, sizeof(bp_ptr->bp_file));

                    /* Get the semaphore before accessing the device list */
                    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
                    {
                        /* Get the device by name to be used on BOOTP for this
                         * iteration.
                         */
                        int_face = DEV_Get_Dev_By_Name(dv_name);

                        /* Ensure this interface exists. */
                        if (int_face)
                        {
                            pkt_size =
                                (UINT32)(int_face->dev_hdrlen + BOOTP_MAX_HEADER_SIZE);

                            /* If the interface MTU is greater than or equal to the
                             * size of the outgoing BOOTP packet, continue.
                             */
                            if (int_face->dev_mtu >= pkt_size)
                            {
                                /* copy the hardware address from the device structure */
                                memcpy(bp_ptr->bp_mac_addr, int_face->dev_mac_addr,
                                       DADDLEN);

                                hdrlen = int_face->dev_hdrlen;

                                /* Release the semaphore */
                                if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                                    NLOG_Error_Log("Failed to release semaphore",
                                                   NERR_SEVERE, __FILE__, __LINE__);

                                /* Make sure that the MAC address is a good. */
                                if (memcmp(bp_ptr->bp_mac_addr, "\0\0\0\0\0\0",
                                           DADDLEN) != 0)
                                {
                                    status = BOOTP_Process_Request(socketd, buffer,
                                                                   pkt_size, bp_ptr,
                                                                   dv_name, hdrlen);
                                }

                                else
                                    status = NU_BOOTP_INIT_FAILED;

                            } /* Check the device MTU */

                            else
                            {
                                /* Release the semaphore */
                                if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                                    NLOG_Error_Log("Failed to release semaphore",
                                                   NERR_SEVERE, __FILE__, __LINE__);

                                status = NU_BOOTP_INIT_FAILED;
                            }
                        }

                        else
                        {
                            /* Release the semaphore */
                            if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                                NLOG_Error_Log("Failed to release semaphore",
                                               NERR_SEVERE, __FILE__, __LINE__);

                            status = NU_BOOTP_INIT_FAILED;
                        }

                    } /* Get the semaphore */

                    else
                        status = NU_BOOTP_INIT_FAILED;

                } /* Bind the socket to the wildcard address */

                else
                    status = NU_BOOTP_INIT_FAILED;

#if (INCLUDE_STATIC_BUILD == NU_FALSE)

                if (NU_Deallocate_Memory(buffer) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to deallocate memory for buffer",
                                   NERR_SEVERE, __FILE__, __LINE__);
#endif
            } /* Allocate memory for the buffer */

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
            else
                status = NU_BOOTP_INIT_FAILED;
#endif

            if (NU_Close_Socket(socketd) != NU_SUCCESS)
                NLOG_Error_Log("Error closing socket", NERR_SEVERE,
                               __FILE__, __LINE__);

        } /* Create a socket */

        else
            status = NU_BOOTP_INIT_FAILED;

    } /* Validate the input parameters */

    else
        status = NU_INVALID_PARM;

    NU_USER_MODE();

    return (status);

} /* NU_Bootp */

/**************************************************************************
*
*   FUNCTION
*
*       BOOTP_Process_Request
*
*   DESCRIPTION
*
*       The function will build the entire packet from the mac
*       layer to the BOOTP layer. It will send a bootp request packet,
*       and process the BOOTP REPLY that is sent from the BOOTP Server.
*       The code will also handle retries.
*
*   INPUTS
*
*       socketd                     The UDP socket over which to
*                                   communicate.
*       *int_face                   A pointer to the device through which
*                                   to communicate.
*       *buffer                     The memory in which to build the
*                                   packet.
*       pkt_size                    The total size of the packet.
*       *bp_ptr                     Pointer to BOOTP structure.
*       *dv_name                    Pointer to Devices name.
*
* OUTPUTS
*
*       NU_SUCCESS                  Call was successful.
*       NU_INVALID_SEMAPHORE        Could not get the semaphore.
*       NU_NO_BUFFERS               No buffers available for packet.
*       NU_BOOTP_ATTACH_IP_FAILED   Could not attach IP to device.
*       NU_BOOTP_SEND_FAILED        Could not send bootp packet.
*
****************************************************************************/
STATIC STATUS BOOTP_Process_Request(INT socketd, UINT8 HUGE *work_buffer,
                                    UINT32 pkt_size, BOOTP_STRUCT *bp_ptr,
                                    const CHAR *dv_name, UINT8 hdrlen)
{
    DV_DEVICE_ENTRY     *int_face = NU_NULL;
    NET_BUFFER          *buf_ptr;
    UINT16              delay, delay_mask = 3;
    BOOTPLAYER          *bootp_ptr;
    IPLAYER             *ip_ptr;
    UDPLAYER            *udp_pkt;
    NET_BUFFER          *next_buf;
    struct pseudotcp    tcp_chk;
    UINT32              flags;
    UINT32              temp_data_len, temp_total_data_len;
    UINT8               *ether_header;
    DEV_IF_ADDR_ENTRY   *target_entry;
    INT                 i;
    INT32               total_size, current_size;
    STATUS              found = 0;
    SCK_SOCKADDR        sa;
    UINT16              nbytes;
    INT16               hlen = IP_HEADER_LEN;
    STATUS              status;
    STATUS              retval = NU_SUCCESS;

    /* Get the semaphore before accessing the device list */
    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status != NU_SUCCESS)
        return (status);

    /* Get a buffer chain to transmit the packet */
    buf_ptr =
        MEM_Buffer_Chain_Dequeue(&MEM_Buffer_Freelist, pkt_size);

    /* Release the semaphore */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

    if (buf_ptr == NU_NULL)
        return (NU_NO_BUFFERS);

    /* Set the list to which the driver should free this buffer chain upon
     * completion of transmission.
     */
    buf_ptr->mem_dlist = &BOOTP_List;

    /* Zero out the packet */
    UTL_Zero(buf_ptr->mem_parent_packet, sizeof(NET_PARENT_BUFFER_SIZE));

    /* Zero out the next packet */
    if (buf_ptr->next_buffer != NU_NULL)
    {
        next_buf = buf_ptr->next_buffer;

        UTL_Zero(next_buf->mem_packet, sizeof(NET_MAX_BUFFER_SIZE));
    }
    else
        next_buf = NU_NULL;

    /* Ensure the parent packet pointer can be set up to the appropriate
     * position, depending on the size of the first buffer.
     */
    if (pkt_size >= BOOTP_HEADER_LEN)
    {
        bootp_ptr = (BOOTPLAYER *)(buf_ptr->mem_parent_packet +
            (UINT32)(pkt_size - BOOTP_HEADER_LEN));
    }

    /* NET_PARENT_BUFFER_SIZE is too small to handle this operation.
     * Return with an error.
     */
    else
    {
        /* Get the semaphore before returning the buffers. */
        status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Set the status to indicate that buffers could not
             * be allocated for the operation.
             */
            status = NU_NO_BUFFERS;

            /* Free the allocated buffer chain. */
            MEM_One_Buffer_Chain_Free(buf_ptr, &MEM_Buffer_Freelist);

            NLOG_Error_Log("Buffer chain too small to complete operation", NERR_SEVERE,
                           __FILE__, __LINE__);

            /* Release the semaphore */
            if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                               __FILE__, __LINE__);
        }

        return (status);
    }

    udp_pkt   = (UDPLAYER *) (((CHAR *)bootp_ptr) - UDP_HEADER_LEN);
    ip_ptr    = (IPLAYER *) (((CHAR *)udp_pkt) - IP_HEADER_LEN);

    /*  Build the BOOTP Request Packet  */
    nbytes = BOOTP_Init((BOOTPLAYER *)work_buffer, bp_ptr);

    /*  Initialize the local and foreign port numbers  */
    PUT16(udp_pkt, UDP_SRC_OFFSET, IPPORT_BOOTPC);
    PUT16(udp_pkt, UDP_DEST_OFFSET, IPPORT_BOOTPS);

    /* Set up the UDP Header  */
    PUT16(udp_pkt, UDP_LENGTH_OFFSET, (UINT16)(nbytes + UDP_HEADER_LEN));

    PUT16(udp_pkt, UDP_CHECK_OFFSET, 0);

    buf_ptr->data_ptr =
        buf_ptr->mem_parent_packet + NET_MAX_UDP_HEADER_SIZE + hdrlen;

    /*  Chain the Bootp Request Packet */
    if (nbytes > NET_PARENT_BUFFER_SIZE)
    {
        current_size = (NET_PARENT_BUFFER_SIZE - IP_HEADER_LEN) - hdrlen;
        total_size = nbytes - current_size;
    }
    else
    {
        total_size = nbytes;
        current_size = total_size;
    }

    /*  Copy Bootp Packet into first Buffer */
    memcpy(buf_ptr->data_ptr, work_buffer, (unsigned int)current_size);

    /*  Set the Data Length to the Size of bytes copied.  */
    buf_ptr->data_len = (UINT32)current_size;

    /*  Set the Total data length to the Number of bytes in a Bootp Packet. */
    buf_ptr->mem_total_data_len = nbytes;

    /*  Increment Bootp Buffer to be at the number of bytes copied.  */
    work_buffer = work_buffer + current_size;

    /*  Check to make sure there is data to store in the mem_packet */
    while ( (total_size) && (next_buf != NU_NULL) )
    {
        if (total_size > (INT32)NET_MAX_BUFFER_SIZE)
        {
            current_size = NET_MAX_BUFFER_SIZE;
        }
        else
        {
            current_size = total_size;
        }

        total_size = total_size - current_size;

        /*  Copy the remaining data in the chaining packets  */
        memcpy(next_buf->mem_packet, work_buffer, (unsigned int)current_size);

        /*  Set the Data pointer to the remainder of the packets.  */
        next_buf->data_ptr = next_buf->mem_packet;
        next_buf->next = NU_NULL;
        next_buf->data_len = (UINT32)current_size;

        work_buffer = work_buffer + current_size;

        if (next_buf->next_buffer != NU_NULL)
        {
            next_buf = next_buf->next_buffer;
        }
    }

    /* Increment the Packet data pointer to the UDP layer */
    buf_ptr->data_ptr -= UDP_HEADER_LEN;

    /*  Copy the udp_pkt into the parent_packet.  */
    memcpy(buf_ptr->data_ptr,(UINT8 *)udp_pkt,sizeof(UDPLAYER));

    /*  Increment the total data length */
    buf_ptr->mem_total_data_len += UDP_HEADER_LEN;

    /*  Increment the data length of this packet.  */
    buf_ptr->data_len += UDP_HEADER_LEN;

    /*  Calculate UDP Checksum  */
    tcp_chk.source  = 0x0;
    tcp_chk.dest    = 0xFFFFFFFFUL;
    tcp_chk.z       = 0;
    tcp_chk.proto   = IP_UDP_PROT;
    tcp_chk.tcplen  = INTSWAP((UINT16)buf_ptr->mem_total_data_len);

    PUT16(udp_pkt, UDP_CHECK_OFFSET, TLS_TCP_Check((UINT16*)&tcp_chk, buf_ptr));

    /* If a checksum of zero is computed it should be replaced with 0xffff. */
    if (GET16(udp_pkt, UDP_CHECK_OFFSET) == 0)
        PUT16(udp_pkt, UDP_CHECK_OFFSET, 0xFFFF);

    /*  Set up the IP header  */
    PUT8(ip_ptr, IP_VERSIONANDHDRLEN_OFFSET, (UINT8)(((hlen >> 2)) | (IP_VERSION << 4)));

    /*  Set the IP header to no fragments. */
    PUT16(ip_ptr, IP_FRAGS_OFFSET, 0);

    /* Set the IP packet ID.  */
    PUT16(ip_ptr, IP_IDENT_OFFSET, 0);

    /* Set the type of service. */
    PUT8(ip_ptr, IP_SERVICE_OFFSET, 0);

    /* Set the total length( data and IP header) for this packet. */
    PUT16(ip_ptr, IP_TLEN_OFFSET, (UINT16)(nbytes + UDP_HEADER_LEN + hlen));

    /*  Set the time to live. */
    PUT8(ip_ptr, IP_TTL_OFFSET, IP_Time_To_Live);

    /*  Set the protocol. */
    PUT8(ip_ptr, IP_PROTOCOL_OFFSET, IP_UDP_PROT);

    /* We are doing a broadcast, so we do not need this fields. */
    PUT32(ip_ptr, IP_SRC_OFFSET, 0);
    PUT32(ip_ptr, IP_DEST_OFFSET, IP_ADDR_BROADCAST);

    /*  Compute the IP checksum. */
    PUT16(ip_ptr, IP_CHECK_OFFSET, 0);
    PUT16(ip_ptr, IP_CHECK_OFFSET, TLS_IP_Check((UINT16 *)ip_ptr, (UINT16)(hlen >> 1)));

    /*  Set the buffer pointer to the IP Layer.  */
    buf_ptr->data_ptr -= IP_HEADER_LEN;

    /*  Add the IPLAYER to the total data length */
    buf_ptr->mem_total_data_len += IP_HEADER_LEN;
    temp_total_data_len =  buf_ptr->mem_total_data_len;

    /*  Set the data length of the current packet.  */
    buf_ptr->data_len += IP_HEADER_LEN;
    temp_data_len =  buf_ptr->data_len;

    /*  Copy the IP header into the parent packet of the buffer chain.  */
    memcpy(buf_ptr->data_ptr, (UINT8 *)ip_ptr, IP_HEADER_LEN);

    /*  Set initial Delay for Processing packets.  */
    delay = (UINT16)((delay_mask & UTL_Rand()) + 1);

    /*  Initialize the ethernet header.  */
    ether_header = (UINT8 *)sa.sck_data;
    PUT_STRING(ether_header, ETHER_DEST_OFFSET, NET_Ether_Broadaddr, DADDLEN);
    PUT_STRING(ether_header, ETHER_ME_OFFSET, bp_ptr->bp_mac_addr, DADDLEN);
    PUT16(ether_header, ETHER_TYPE_OFFSET, INTSWAP(EIP));

    sa.sck_family = SK_FAM_UNSPEC;
    sa.sck_len = sizeof(sa);

    /* Get the semaphore before accessing the device list */
    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Get the device by name to be used on BOOTP for this
         * iteration.
         */
        int_face = DEV_Get_Dev_By_Name(dv_name);

        if (int_face)
        {
            /* Set the flag to indicate that address configuration is in progress
             * on this interface.
             */
            int_face->dev_flags |= DV_ADDR_CFG;
        }

        else
            status = NU_BOOTP_SEND_FAILED;

        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* If we were able to obtain the semaphore, but the interface name is
     * invalid, we need to free the buffers and return also.
     */
    if (!int_face)
    {
        MEM_One_Buffer_Chain_Free(buf_ptr, &MEM_Buffer_Freelist);

        return (status);
    }

    /*  Transmit the packet  */
    for (i = 0; i < BOOTP_RETRIES; i++)
    {
        /* Grab the semaphore because we are about to change the interface
           that the BOOTP request will be sent over. */
        status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

        if (status != NU_SUCCESS)
        {
            /* Clear the flag to indicate that address configuration is finished
             * for this interface.
             */
            int_face->dev_flags &= ~DV_ADDR_CFG;

            MEM_Buffer_Remove(&BOOTP_List, buf_ptr);

            MEM_One_Buffer_Chain_Free(buf_ptr, &MEM_Buffer_Freelist);

            return (status);
        }

        /* The device will not send packets until an IP address is attached.
           Temporarily trick it into thinking an IP address is attached so this
           request can be sent.  Then set it back. */
        flags = int_face->dev_flags;
        int_face->dev_flags |= (DV_UP | DV_RUNNING);

        /* Send the packet. */
        status = (*(int_face->dev_output))(buf_ptr, int_face,
                                           (SCK_SOCKADDR_IP *)&sa, NU_NULL);
        int_face->dev_flags = flags;

        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);

        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log("Error occurred while sending BOOTP request", NERR_FATAL,
                           __FILE__,__LINE__);

            retval = NU_BOOTP_SEND_FAILED;
            break;
        }

        /*  Look at each packet on the buffer list to see if it is my reply */
        found = BOOTP_Process_Packets(socketd, bp_ptr, delay);

        if (found == NU_TRUE)
            break;                           /*  Found the Packet. */

        delay_mask++;

        delay = (UINT16)((delay_mask & UTL_Rand()) + 1);

        /*  Get the the packet off the Transmitted list and retransmit again. */
        MEM_Buffer_Remove(&BOOTP_List, buf_ptr);

        buf_ptr->data_ptr = buf_ptr->mem_parent_packet + hdrlen;
        buf_ptr->data_len = temp_data_len;
        buf_ptr->mem_total_data_len = temp_total_data_len;
    }  /*  End For Loop */

    /* Get the semaphore before accessing the device list */
    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Get the device by name to be used on BOOTP for this
         * iteration.
         */
        int_face = DEV_Get_Dev_By_Name(dv_name);

        /* Ensure a matching interface exists. */
        if (int_face)
        {
            /* Clear the flag to indicate that address configuration is
             * finished for this interface.
             */
            int_face->dev_flags &= ~DV_ADDR_CFG;
        }

        /* There was an error during address configuration. */
        else
        {
            retval = NU_BOOTP_ATTACH_IP_FAILED;
        }

        /* Have we errored out? */
        if (retval == NU_SUCCESS)
        {
            /* We did not error out. Did we get a response? */
            if (found == NU_TRUE)
            {
                /* Look for an empty address structure entry to use */
                target_entry = DEV_Find_Target_Address(int_face, 0);

                /* If an empty address structure entry was used, fill it in with
                 * the bootp information.
                 */
                if (target_entry)
                    status = DEV_Initialize_IP(int_face, bp_ptr->bp_yiaddr,
                                               bp_ptr->bp_net_mask, target_entry);

                /* Create a new entry */
                else
                    status = DEV_Attach_IP_To_Device(dv_name, bp_ptr->bp_yiaddr,
                                                     bp_ptr->bp_net_mask);

                if ( (status != NU_SUCCESS) && (status != NU_NO_ACTION) )
                {
                    NLOG_Error_Log("Unable to attach BOOTP addr to device",
                                   NERR_FATAL, __FILE__, __LINE__);

                    retval = NU_BOOTP_ATTACH_IP_FAILED;
                }
                else
                    retval = NU_SUCCESS;
            }
            else
                retval = found;

        } /* end if we errored out */

        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    else
        retval = status;

    /* Free this buffer by pulling it off of the bootp list and
       putting it onto the free list. */
    MEM_Buffer_Remove(&BOOTP_List, buf_ptr);

    MEM_One_Buffer_Chain_Free(buf_ptr, &MEM_Buffer_Freelist);

    return (retval);

} /* BOOTP_Process_Request */

/**************************************************************************
*
*   FUNCTION
*
*       BOOTP_Init
*
*   DESCRIPTION
*
*       This routine will handle the initing of the bootp packet.
*
*   INPUTS
*
*       pkt                     Packet to be transmitted.
*       out_bp                  The structure that contains global data.
*
*   OUTPUTS
*
*       len                     The length of the BOOTP Request packet.
*
****************************************************************************/
STATIC UINT16 BOOTP_Init(BOOTPLAYER *pkt, const BOOTP_STRUCT *out_bp)
{
    UINT8       bootp_cookie[4] = BOOTP_COOKIE;

    /* Initially, there are no buffers on the BOOTP_List. */
    BOOTP_List.head = NU_NULL;
    BOOTP_List.tail = NU_NULL;

    /* get a unique transaction ID */
    bootp_xid = NU_Retrieve_Clock();

    /*  Set the Packet to all 0's */
    memset(pkt, 0, sizeof(BOOTPLAYER) );

    /* This is a bootp request. */
    pkt->bp_op = BOOTREQUEST;

    /* Initialize the hardware dependencies */
    pkt->bp_htype = HARDWARE_TYPE;
    pkt->bp_hlen = DADDLEN;        /* Hardware Address 1 octet */

    /* Initialize the unique ID. */
    pkt->bp_xid = bootp_xid;

    /* The number of seconds the bootp client has been running. */
    pkt->bp_secs = 1;
    memcpy(pkt->bp_chaddr, out_bp->bp_mac_addr,DADDLEN);

    /* Initialize the server's ip address to the broadcast address.
     *  The server will fill in his correct address. */
    PUT32(pkt->bp_siaddr, 0, IP_ADDR_BROADCAST);

    /* Copy the requested server name into the header */
    memcpy(pkt->bp_sname, out_bp->bp_sname, sizeof(pkt->bp_sname));

    /* Copy the client's known IP address into the header */
    memcpy(pkt->bp_ciaddr, out_bp->bp_ip_addr, IP_ADDR_LEN);

    /* Check to see if the application has set up the vendor-specific array.
        If not, just copy the default magic cookie and the BOOTP_END tag
        into the array.  This is suggested by RFC 1542. */
    if (out_bp->bp_vend_opt[0] == 0)
    {
        /* Copy the magic cookie into the array */
        memcpy(pkt->bp_vend, bootp_cookie, 4);

        /* Copy the BOOTP_END tag into the array */
        PUT8(pkt->bp_vend, 4, BOOTP_END);
    }
    else
    {
        /* Copy the requested vendor-specific options into the packet */
        NU_BLOCK_COPY(pkt->bp_vend, out_bp->bp_vend_opt, sizeof(pkt->bp_vend));
    }

    return (sizeof(*pkt));

} /* BOOTP_Init */

/**************************************************************************
*
*   FUNCTION
*
*       BOOTP_Process_Packets
*
*   DESCRIPTION
*
*       This routine will handle the parsing of the incoming bootp
*       packets.
*
*   INPUTS
*
*       socketd                 Socket Descriptor to retrieve data from.
*       *bp                     Pointer to the bootp packet that we are
*                               to process.
*       timeout                 Timeout value used for NU_Select.
*
*   OUTPUTS
*
*      NU_NO_SOCK_MEMORY        Unable to allocate memory.
*      NU_BOOTP_FAILED          No data to process.
*      NU_BOOTP_SELECT_FAILED   No bootp packet to process.
*      NU_BOOTP_RECV_FAILED     Error retrieving data.
*      NU_TRUE                  Packet was processed.
*      0                        Packet was not found.
*
****************************************************************************/
STATIC STATUS BOOTP_Process_Packets(INT socketd, BOOTP_STRUCT *bp,
                                    UINT16 timeout)
{
    STATUS              ret;
    INT                 x, items, len;
    INT16               flen;
    CHAR                *inbuf;
    INT16               found = 0;
    UINT32              local_xid;
    BOOTPLAYER          *bootp_ptr;          /* Bootp struct pointer*/
    FD_SET              readfs;
    struct addr_struct  fromaddr;
    UINT8               *ptr;
    STATUS              retval = 0;
    CHAR                host_name[MAX_HOST_NAME_LENGTH];
#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    STATUS              status;
#endif


#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    /*   Allocate Memory for input Buffer */
    status = NU_Allocate_Memory(MEM_Cached, (VOID **)&inbuf, IOSIZE,
                                (UNSIGNED)NU_NO_SUSPEND);

    /*  Check if an error occurred during NU_Allocate_Memory */
    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("Error occurred during memory alloc", NERR_FATAL,
                       __FILE__, __LINE__);

        return (NU_NO_SOCK_MEMORY);
    }
#else
    /*   Declare Memory for an input Buffer */
    CHAR    input_buffer_memory[IOSIZE];

    /*   Assign Memory to the input Buffer */
    inbuf  = input_buffer_memory;
#endif

    /*   Do While to process received data */
    do
    { /*  do while */

        NU_FD_Init(&readfs);
        NU_FD_Set(socketd, &readfs);

        ret = NU_Select(NSOCKETS, &readfs, NU_NULL, NU_NULL,
                        (timeout * SCK_Ticks_Per_Second));

        if (ret == NU_NO_DATA)
        {
            retval = NU_BOOTP_FAILED;
            break;
        }

        if (NU_FD_Check(socketd, &readfs) == NU_FALSE)
        {
            retval = NU_BOOTP_SELECT_FAILED;
            break;
        }

        fromaddr.family = NU_FAMILY_IP;
        fromaddr.port = 0;

        ret = (INT)NU_Recv_From(socketd, inbuf, IOSIZE, 0, &fromaddr, &flen);

        if (ret < 0)
        {
            NLOG_Error_Log("Did not receive a BOOTP response", NERR_FATAL,
                           __FILE__, __LINE__);

            retval = NU_BOOTP_RECV_FAILED;

            break;
        }

        /* get pointer to BOOTP packet. */
        bootp_ptr = (BOOTPLAYER *)inbuf;

        /*  Retrieve the unique ID from the packet  */
        memcpy(&local_xid, &bootp_ptr->bp_xid, sizeof(UINT32));

        /* see if packet is the returning response to the packet I sent */
        if (local_xid != bootp_xid)
            continue;

        /* Accept the very first packet */

        /*  Get the Server address */
        bp->bp_siaddr[0] = bootp_ptr->bp_siaddr[0];
        bp->bp_siaddr[1] = bootp_ptr->bp_siaddr[1];
        bp->bp_siaddr[2] = bootp_ptr->bp_siaddr[2];
        bp->bp_siaddr[3] = bootp_ptr->bp_siaddr[3];

        /*  Get my IP address */
        bp->bp_yiaddr[0] = bootp_ptr->bp_yiaddr[0];
        bp->bp_yiaddr[1] = bootp_ptr->bp_yiaddr[1];
        bp->bp_yiaddr[2] = bootp_ptr->bp_yiaddr[2];
        bp->bp_yiaddr[3] = bootp_ptr->bp_yiaddr[3];

        /*  Get the client IP address */
        bp->bp_ip_addr[0] = bootp_ptr->bp_ciaddr[0];
        bp->bp_ip_addr[1] = bootp_ptr->bp_ciaddr[1];
        bp->bp_ip_addr[2] = bootp_ptr->bp_ciaddr[2];
        bp->bp_ip_addr[3] = bootp_ptr->bp_ciaddr[3];

        /*  Get the Server Name */
        memcpy(bp->bp_sname, bootp_ptr->bp_sname, sizeof(bp->bp_sname));

        /* Get the Bootp file name.  */
        memcpy(bp->bp_file, bootp_ptr->bp_file, sizeof(bp->bp_file));

        /* Get the entire vendor options array */
        memcpy(bp->bp_vend_opt, bootp_ptr->bp_vend, sizeof(bp->bp_vend_opt));

        /* Test to see if any info is in the vendor-specific array.  Since
            the server may be using a magic cookie that is different from
            the default magic cookie, we will just see if any data is in
            the array. */
        ptr = bootp_ptr->bp_vend + 4;

        if ((*ptr != BOOTP_PAD) || (*ptr != BOOTP_END))
        {
            /* Now loop through vendor options, passing them to user */
            /* callback function. */

            found = NU_TRUE;

            while ( (*ptr != BOOTP_END) && ((ptr - bootp_ptr->bp_vend) < 64) )
            {
                switch (*ptr)
                {
                    case BOOTP_PAD: /* no-op padding, used to align fields to word */

                        ptr++;      /* boundaries. */
                        break;

                    case BOOTP_SUBNET:       /* subnet mask */

                        len = (*(ptr + 1));
                        ptr += 2;

                        bp->bp_net_mask[0] = (*ptr);
                        bp->bp_net_mask[1] = (*(ptr + 1));
                        bp->bp_net_mask[2] = (*(ptr + 2));
                        bp->bp_net_mask[3] = (*(ptr + 3));

                        ptr += len;
                        break;

                    case BOOTP_TIMEOFF:       /* time offset */

                        ptr += 3;
                        break;

                    case BOOTP_GATEWAY:       /* gateways  */

                        len = (*(ptr + 1));
                        ptr += 2;

                        bp->bp_giaddr[0] = (*ptr);
                        bp->bp_giaddr[1] = (*(ptr + 1));
                        bp->bp_giaddr[2] = (*(ptr + 2));
                        bp->bp_giaddr[3] = (*(ptr + 3));

                        ptr += len;
                        break;

                    case BOOTP_TIMESERV:    /* time servers */
                    case BOOTP_NAMESERV:    /* IEN = 116 name server */

                        ptr += 3;
                        break;

                    case BOOTP_DNS_SERVER:  /* domain name server */

                        len = (*(ptr + 1));
                        items = len / 4;
                        ptr += 2;

                        for (x = 0; x < items; x++)
                        {
                            if (NU_Add_DNS_Server(ptr, DNS_ADD_TO_END) != NU_SUCCESS)
                                NLOG_Error_Log("Error adding DNS server", NERR_SEVERE,
                                               __FILE__, __LINE__);

                            ptr += 4;
                        }

                        break;

                    case BOOTP_LOGSERV:       /* log server */
                        /* Place your code here. */

                    case BOOTP_COOKSRV:       /* cookie server */
                        /* Place your code here. */

                    case BOOTP_LPRSRV:        /* lpr server */
                        /* Place your code here. */

                    case BOOTP_IMPRSRV:       /* impress server */
                        /* Place your code here. */

                    case BOOTP_RLPSRV:        /* rlp server */
                        /* Place your code here. */

                        ptr += 3;
                        break;

                    case BOOTP_HOSTNAME:      /* client host name */

                        len = (*(ptr + 1));

                        /* Get the current host name for the system */
                        SCK_Get_Host_Name(host_name, MAX_HOST_NAME_LENGTH);

                        /* Accept hostname only if we don't already have one. */
                        if ( (*host_name == 0) && (len < MAX_HOST_NAME_LENGTH) )
                        {
                            SCK_Set_Host_Name((CHAR*)(&ptr[2]), len);
                        }

                        ptr += len+2;
                        break;

                    case BOOTP_BFILSZ:        /* Bootp File Size */

                        ptr += 2;
                        break;

                    case BOOTP_VEND_SPEC:     /* Bootp Vendor Specific Info */

                        len = (*(ptr + 1));

                        /* Increment past the option, option tag, and
                            the option length */
                        ptr += len + 2;
                        break;

                    case BOOTP_END:

                        break;

                    default:

                        /* Get the length of the option */
                        len = (*(ptr + 1));

                        /* Increment past the option, option tag, and
                            the option length */
                        ptr += len + 2;
                        break;

                }   /* end switch */
            }   /* end while    */
        }

    } while (found == 0);

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    if (NU_Deallocate_Memory(inbuf) != NU_SUCCESS)
        NLOG_Error_Log("Failed to deallocate memory for buffer", NERR_SEVERE,
                       __FILE__, __LINE__);
#endif

    if (retval == 0)
    {
        if (found == 0)
            retval = NU_BOOTP_RECV_FAILED;
        else
            retval = found;
    }

    return (retval);

} /* BOOTP_Process_Packets */
