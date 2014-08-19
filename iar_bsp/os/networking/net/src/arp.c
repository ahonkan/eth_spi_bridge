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
*       arp.c
*
*   DESCRIPTION
*
*       This file contains the implementation of ARP (Address Resolution
*       Protocol).
*
*   DATA STRUCTURES
*
*       ARP_Cache
*       ARP_Res_List
*       ARP_Res_Count
*
*   FUNCTIONS
*
*       ARP_Build_Pkt
*       ARP_Cache_Update
*       ARP_Check_Events
*       ARP_Event
*       ARP_Cleanup_Entry
*       ARP_Find_Entry
*       ARP_Init
*       ARP_Interpret
*       ARP_Reply
*       ARP_Request
*       ARP_Resolve
*       ARP_LL_Event_Handler
*       ARP_Probe
*
*   DEPENDENCIES
*
*       nu_net.h
*       net_extr.h
*
*************************************************************************/

#include "networking/nu_net.h"
#include "networking/net_extr.h"

#if (INCLUDE_IPV4 == NU_TRUE)

/* This is our ARP cache. */
ARP_ENTRY ARP_Cache[ARP_CACHE_LENGTH];

/* This is the resolve list.  An item is placed on this list when a MAC address
   needs to be resolved.  It is removed once the address has been resolved or
   when failure occurs.
 */
ARP_RESOLVE_LIST    ARP_Res_List;
UINT16              ARP_Res_Count;

#if (INCLUDE_LL_CONFIG == NU_TRUE)
TQ_EVENT            ARP_LL_Event;
#endif

#if (INCLUDE_STATIC_BUILD == NU_TRUE)

/* Declare Memory for ARPs */
ARP_RESOLVE_ENTRY   NET_ARP_Memory[NET_MAX_ARPS];

/* Declare flag array for the above memory */
UINT8               NET_ARP_Memory_Flags[NET_MAX_ARPS] = {0};

#endif

STATIC NET_BUFFER *ARP_Build_Pkt(const DV_DEVICE_ENTRY *, UINT32,
                                 const UINT8 *, UINT32, UINT16);
STATIC VOID       ARP_Check_Events(ARP_LAYER *);

/*************************************************************************
*
*   FUNCTION
*
*       ARP_Build_Pkt
*
*   DESCRIPTION
*
*       This function will get a free buffer and fill in as much of the
*       ARP packet fields as possible.  All other fields must be updated
*       by the calling function.
*
*   INPUTS
*
*       *dev                    The device to transmit the packet on.
*       tipnum                  The target IP number.
*       *thardware              The target hardware address.
*       pkt_type                The ARP packet type, reply or request.
*       sipnum                  The source IP number
*
*   OUTPUTS
*
*       *NET_BUFFER             Pointer to the buffer in which the packet
*                               was constructed.
*       NU_NULL                 When no buffer pointer for the arp packet
*                               is allocated
*
*************************************************************************/
STATIC NET_BUFFER *ARP_Build_Pkt(const DV_DEVICE_ENTRY *dev, UINT32 tipnum,
                                 const UINT8 *thardware, UINT32 sipnum,
                                 UINT16 pkt_type)
{
    NET_BUFFER      *buf_ptr;
    UINT8           *arp_pkt;

    /* Allocate a buffer to place the arp packet in. */
    buf_ptr = MEM_Buffer_Dequeue(&MEM_Buffer_Freelist);

    if (buf_ptr == NU_NULL)
    {
        return (NU_NULL);
    }

    /* Initialize each field in the allocated buffer. */
    buf_ptr->mem_total_data_len = buf_ptr->data_len = ARP_HEADER_LEN;

    buf_ptr->data_ptr = buf_ptr->mem_parent_packet + dev->dev_hdrlen;
    buf_ptr->mem_dlist      = (NET_BUFFER_HEADER *) &MEM_Buffer_Freelist;

    /* Set up a pointer to the packet. */
    arp_pkt = (UINT8 *)buf_ptr->data_ptr;

    PUT16(arp_pkt, ARP_HRD_OFFSET, HARDWARE_TYPE);  /* Ether = 1 */
    PUT16(arp_pkt, ARP_PRO_OFFSET, ARPPRO);         /* IP protocol = 0x0800 */
    PUT8(arp_pkt, ARP_HLN_OFFSET, DADDLEN);         /* Ethernet hardware length */
    PUT8(arp_pkt, ARP_PLN_OFFSET, IP_ADDR_LEN);     /* IP length = 4 */

    /* sender's hardware addr */
    PUT_STRING(arp_pkt, ARP_SHA_OFFSET, dev->dev_mac_addr, DADDLEN);

    PUT_STRING(arp_pkt, ARP_THA_OFFSET, thardware, DADDLEN);

    /* sender's IP addr */
    PUT32(arp_pkt, ARP_SPA_OFFSET, sipnum);

    /* put in IP address we want */
    PUT32(arp_pkt, ARP_TPA_OFFSET, tipnum);

    /* Set the packet type.  Either a request or a response. */
    PUT16(arp_pkt, ARP_OP_OFFSET, pkt_type);

    return(buf_ptr);

} /* ARP_Build_Pkt */

/*************************************************************************
*
*   FUNCTION
*
*       ARP_Reply
*
*   DESCRIPTION
*
*       This function sends an ARP reply.  Called whenever an ARP request
*       is received.
*
*   INPUTS
*
*       *thardware              The target hardware address.
*       tipnum                  The target IP number.
*       *dev                    Pointer to the device the reply will be
*                               sent from.
*       sipnum                  The source IP number.
*
*   OUTPUTS
*
*       NU_SUCCESS              Indicates successful operation.
*       NU_NO_BUFFERS           Indicates failure to allocate a buffer.
*
*************************************************************************/
STATUS ARP_Reply(const UINT8 *thardware, UINT32 tipnum, UINT32 sipnum,
                 DV_DEVICE_ENTRY *dev)
{
    NET_BUFFER              *buf_ptr;
    ARP_MAC_HEADER          mh;
    STATUS                  status;

#if ((INCLUDE_ARP_INFO_LOGGING == NU_TRUE) || (PRINT_ARP_MSG == NU_TRUE))
    ARP_LAYER               *arp_hdr;
#endif

    buf_ptr = ARP_Build_Pkt(dev, tipnum, thardware, sipnum, ARPREP);

    if (buf_ptr == NU_NULL)
    {
        return (NU_NO_BUFFERS);
    }

#if ((INCLUDE_ARP_INFO_LOGGING == NU_TRUE) || (PRINT_ARP_MSG == NU_TRUE))
    /* Get a pointer to the ARP header */
    arp_hdr = (ARP_LAYER *)buf_ptr->data_ptr;

    /* Store/Print the ARP header info */
    NLOG_ARP_Info(arp_hdr, NLOG_TX_PACK);
#endif

    /* This is a pseudo MAC header that is passed to the MAC layer send function.
       The MAC layer information that is known is filled in. A family of
       SK_FAM_UNSPEC lets the MAC layer know that this is not an IP datagram and
       it should not try to resolve a hardware address. */
    memcpy(mh.ar_mac.ar_mac_ether.dest, thardware, DADDLEN);
    memset(mh.ar_mac.ar_mac_ether.me, 0, DADDLEN);
    mh.ar_mac.ar_mac_ether.type = EARP;
    mh.ar_family = SK_FAM_UNSPEC;
    mh.ar_len = sizeof(mh);

    /* Send the ARP Packet. */
    status = (*dev->dev_output)(buf_ptr, dev, (VOID *)&mh, NU_NULL);

    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to send ARP Reply packet", NERR_SEVERE,
                       __FILE__, __LINE__);

        MEM_One_Buffer_Chain_Free(buf_ptr, &MEM_Buffer_Freelist);

        NET_DBG_Notify(status, __FILE__, __LINE__,
                       NU_Current_Task_Pointer(), NU_NULL);
    }

    return (NU_SUCCESS);

} /* ARP_Reply */

/*************************************************************************
*
*   FUNCTION
*
*       ARP_Interpret
*
*   DESCRIPTION
*
*       Interpret ARP packets.
*       Look at incoming ARP packet and make required assessment of
*       usefulness, check to see if we requested this packet, clear
*       all appropriate flags.
*
*   INPUTS
*
*       *a_pkt                  Pointer to the ARP packet to process.
*       *device                 Pointer to the device the packet was
*                               received on.
*
*   OUTPUTS
*
*       NU_SUCCESS              Indicates successful operation.
*       NU_NO_ACTION            Indicates that the packet was determined
*                               to be unacceptable.
*
*************************************************************************/
STATUS ARP_Interpret(ARP_LAYER *a_pkt, DV_DEVICE_ENTRY *device)
{
    UINT8                   sha[DADDLEN];
    UINT32                  target_ip_addr;
    UINT32                  source_ip_addr;
    DEV_IF_ADDR_ENTRY       *src_addr, *target_addr;
    SCK_SOCKADDR_IP         dest;

#if (INCLUDE_RARP == NU_TRUE)
    ARP_RESOLVE_ENTRY       *ar_entry;
    DEV_IF_ADDR_ENTRY       *my_ip;
    STATUS                  status;
#endif

    target_ip_addr = GET32(a_pkt, ARP_TPA_OFFSET);
    source_ip_addr = GET32(a_pkt, ARP_SPA_OFFSET);

    /* Check if the source address is an address on this device */
    src_addr = DEV_Find_Target_Address(device, source_ip_addr);

    /* Check if the source address is an address on this device */
    target_addr = DEV_Find_Target_Address(device, target_ip_addr);

    GET_STRING(a_pkt, ARP_SHA_OFFSET, sha, DADDLEN);

#if (INCLUDE_LL_CONFIG == NU_TRUE)

    /* If this interface has a link-local address assigned to it. */
    if ( (device->dev_flags & DV_CFG_IPV4_LL_ADDR) &&
         (device->dev_addr.dev_link_local_addr != IP_ADDR_ANY) )
    {
        /* Trigger the state machine. */
        switch (device->dev_ll_state)
        {
            case LL_STATE_PROBE_WAIT:
            case LL_STATE_ANNOUNCE_WAIT:
            case LL_STATE_ARP_ANNOUNCE:

                /* RFC 3927 - section 2.2.1 - If ... the host receives
                 * any ARP packet (Request *or* Reply) on the interface
                 * where the probe is being performed where the packet's
                 * 'sender IP address' is the address being probed for ...
                 * In addition, if during this period the host receives
                 * any ARP Probe where the packet's 'target IP address'
                 * is the address being probed for, and the packet's
                 * 'sender hardware address' is not the hardware address
                 * of the interface the host is attempting to configure ...
                 */
                if ( (memcmp(device->dev_mac_addr, sha, DADDLEN) != 0) &&
                     ((device->dev_addr.dev_link_local_addr == source_ip_addr) &&
                      ((GET16(a_pkt, ARP_OP_OFFSET) == ARPREQ) ||
                       (GET16(a_pkt, ARP_OP_OFFSET) == ARPREP))) ||
                     ((device->dev_addr.dev_link_local_addr == target_ip_addr) &&
                      (GET16(a_pkt, ARP_OP_OFFSET) == ARPREQ)) )
                {
                    /* RFC 3927 - section 2.2.1 - A host should maintain
                     * a counter of the number of address conflicts it has
                     * experienced in the process of trying to acquire an
                     * address.
                     */
                    device->dev_ll_conflict_count++;

                    /* RFC 3927 - section 2.2.1 - ... the host MUST treat
                     * this address as being in use by some other host,
                     * and MUST select a new pseudo-random address and
                     * repeat the process.
                     */
                    device->dev_ll_state = LL_STATE_CONFLICT;

                    return (NU_SUCCESS);
                }

                break;

            case LL_STATE_CLAIMED:

                /* RFC 3927 - section 2.5 - At any time, if a host
                 * receives an ARP packet (request *or* reply) on an
                 * interface where the 'sender IP address' is the IP
                 * address the host has configured for that interface,
                 * but the 'sender hardware address' does not match the
                 * hardware address of that interface, then this is a
                 * conflicting ARP packet, indicating an address conflict.
                 */
                if ( (memcmp(device->dev_mac_addr, sha, DADDLEN) != 0) &&
                     ((device->dev_addr.dev_link_local_addr == source_ip_addr) &&
                      ((GET16(a_pkt, ARP_OP_OFFSET) == ARPREQ) ||
                       (GET16(a_pkt, ARP_OP_OFFSET) == ARPREP))) )
                {
                    /* RFC 3927 - section 2.2.1 - A host should maintain
                     * a counter of the number of address conflicts it has
                     * experienced in the process of trying to acquire an
                     * address.
                     */
                    device->dev_ll_conflict_count++;

                    /* RFC 3927 - section 2.5 - Upon receiving a
                     * conflicting ARP packet, a host MAY elect to
                     * immediately configure a new IPv4 Link-Local
                     * address as described above.
                     */
                    device->dev_ll_state = LL_STATE_CONFLICT;

                    /* Since there is no timer running for the state
                     * machine, set one now so the address can be
                     * released and a new address configured.
                     */
                    TQ_Timerset(ARP_LL_Event, device->dev_index, 0, 0);

                    return (NU_SUCCESS);
                }

                break;

            case LL_STATE_CONFLICT:

                /* We are already in conflict state.  We can ignore this
                 * collision.
                 */
                return (NU_SUCCESS);

            default:

                /* Unknown state, ignore. */
                break;
        }
    }

#endif

#if (INCLUDE_RARP == NU_TRUE)

    /* Search for an address list entry with a null IP address */
    my_ip = DEV_Find_Target_Address(device, 0);

    /* Is this a RARP response to my RARP request. */
    if ( (my_ip) && (GET16(a_pkt, ARP_OP_OFFSET) == RARPR) &&
         EQ_STRING(a_pkt, ARP_THA_OFFSET, device->dev_mac_addr, DADDLEN) )
    {
        /* Search the ARP_Res_List for a match. */
        for (ar_entry = ARP_Res_List.ar_head;
             ar_entry != NU_NULL;
             ar_entry = ar_entry->ar_next)
        {
            /* A match is found when we find an entry for a RARP request. */
            if ( (ar_entry->ar_pkt_type == RARPQ) &&
                 (ar_entry->ar_device == device) )
                break;
        }

        /* Was a match found. */
        if (ar_entry)
        {
            /* Copy the newly discovered IP address. */
            my_ip->dev_entry_ip_addr = target_ip_addr;

            /* Clear the timer event. */
            TQ_Timerunset(RARP_REQUEST, TQ_CLEAR_EXACT,
                          (UNSIGNED)ar_entry->ar_id, 0);

            /* Remove from the list. */
            DLL_Remove(&ARP_Res_List, ar_entry);

            /* Resume the task. */
            status = NU_Resume_Task(ar_entry->ar_task);

            if (status != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to resume task", NERR_RECOVERABLE,
                               __FILE__, __LINE__);

                NET_DBG_Notify(status, __FILE__, __LINE__,
                               NU_Current_Task_Pointer(), NU_NULL);
            }
        }
    }
    else

#endif /* INCLUDE_RARP == NU_TRUE */

    /* Check to see if the packet was for me or if it was sent by someone using
       my IP address.  If neither then return.  Most ARP packets should fall
       into this category.
     */
    if ( (!src_addr) && (!target_addr) )
    {
        /* Is proxy ARP enabled on this device? */
        if (device->dev_flags & DV_PROXYARP)
        {
            /* Let proxy ARP take care of responding if passes check. */
            ARP_Proxy_ARP (a_pkt, device, source_ip_addr, target_ip_addr);
        }

        /* If this is a gratuitous ARP, update the ARP cache if there is an
         * existing entry for the sender.
         */
        if (source_ip_addr == target_ip_addr)
        {
            dest.sck_addr = source_ip_addr;

            /* If the entry already exists in the ARP cache, update it.
             * Otherwise, do not add it to the ARP cache since this node
             * has not communicated with the target node.
             */
            if (ARP_Find_Entry(&dest))
            {
                ARP_Cache_Update(source_ip_addr, sha, 0,
                                 (INT32)device->dev_index);
            }
        }

        /* This packet was destined for someone else, so return. */
        return (NU_SUCCESS);
    }

    /* If the source IP address is the same as mine then someone may be trying
     * to use my IP address.
     */
    if ( (src_addr) && (source_ip_addr != 0) )
    {
        /* Check the source hardware address to make sure this packet
           is not from me. Some NICs see their own broadcasts. */
        if (memcmp(sha, device->dev_mac_addr, DADDLEN))
        {
            /* They are not the same so this is from some other node. */
            NLOG_Error_Log ("Rcvd ARP Response from another node using our addr",
                                NERR_RECOVERABLE, __FILE__, __LINE__);

            /* Mark that a duplicate address has been detected for this
               device. */
            src_addr->dev_entry_dup_addr_detections ++;
        }
    }

#if ((INCLUDE_ARP_INFO_LOGGING == NU_TRUE) || (PRINT_ARP_MSG == NU_TRUE))
    /* Store/Print the ARP header info */
    NLOG_ARP_Info(a_pkt, NLOG_RX_PACK);
#endif

    /*
    *  check packet's desired IP address translation to see if it wants
    *  me to answer.
    */
    if ( (GET16(a_pkt, ARP_OP_OFFSET) == ARPREQ) && (target_addr) )
    {
        /* keep the address for me */
        if (ARP_Cache_Update(source_ip_addr, sha, 0,
                             (INT32)device->dev_index) < 0)
        {
            NLOG_Error_Log("Failed to update ARP Cache entry",
                           NERR_INFORMATIONAL, __FILE__, __LINE__);
        }

        /* Check to see if there are any packets pending the resolution of
           this address. */
        ARP_Check_Events(a_pkt);

        /* Send a reply to the request. */
        if (ARP_Reply(sha, source_ip_addr, target_ip_addr, device) != NU_SUCCESS)
            NLOG_Error_Log("Failed to send ARP Reply", NERR_SEVERE,
                           __FILE__, __LINE__);

        return (NU_SUCCESS);
    }

    /*
     *  Check for a reply that I probably asked for.
     */
    if ( (target_addr)
          && (GET16(a_pkt, ARP_OP_OFFSET) == ARPREP)
          && (GET16(a_pkt, ARP_HRD_OFFSET) == HARDWARE_TYPE)
          && (GET8(a_pkt, ARP_HLN_OFFSET) == DADDLEN)
          && (GET8(a_pkt, ARP_PLN_OFFSET) == 4) )
    {
        if (ARP_Cache_Update (source_ip_addr, sha, 0,
                              (INT32)device->dev_index) < 0)
        {
            NLOG_Error_Log("Failed to update ARP Cache entry",
                           NERR_INFORMATIONAL, __FILE__, __LINE__);
        }

        /* Check to see if there are any packets pending the resolution of
           this address. */
        ARP_Check_Events(a_pkt);

        return (NU_SUCCESS);

    } /* end if this is a reply that we asked for */

    return (NU_NO_ACTION);

} /* ARP_Interpret */

/**************************************************************************
*
*   FUNCTION
*
*       ARP_Check_Events
*
*   DESCRIPTION
*
*       This function checks to see if there are any packets pending the
*       resolution of a hardware address that was provided in either a
*       received ARP response or an ARP request. If there is a pending
*       packet, it will be transmitted.
*
*   INPUTS
*
*       *a_pkt                  Pointer to a received ARP packet.
*
*   OUTPUTS
*
*       None.
*
**************************************************************************/
STATIC VOID ARP_Check_Events(ARP_LAYER *a_pkt)
{
    ARP_RESOLVE_ENTRY               *ar_entry, *ar_temp_entry;
    ARP_MAC_HEADER                  mh;
    UINT32                          arp_spa;

    /* Store off the value to avoid a GET32 every time through the loop */
    arp_spa = GET32(a_pkt, ARP_SPA_OFFSET);

    /* Are there any IP packets pending the resolution of a MAC address? If
       so check to see if this ARP response resolves any of those.
     */
    for (ar_entry = ARP_Res_List.ar_head;
         ar_entry != NU_NULL;
         ar_entry = ar_entry->ar_next)
    {
        if (ar_entry->ar_dest == arp_spa)
            break;
    }

    /* Was a match found? */
    while (ar_entry)
    {
        /* Clear the timer event. */
        TQ_Timerunset(ARPRESOLVE, TQ_CLEAR_EXACT, (UNSIGNED)ar_entry->ar_id, 0);

        /* This is a pseudo MAC header that is passed to the MAC layer send
           function.  The MAC layer information that is known is filled in.
           A family of SK_FAM_UNSPEC lets the MAC layer know that it should
           not try to resolve a hardware address. */
        GET_STRING(a_pkt, ARP_SHA_OFFSET, mh.ar_mac.ar_mac_ether.dest, DADDLEN);

        memset(mh.ar_mac.ar_mac_ether.me, 0, DADDLEN);
        mh.ar_mac.ar_mac_ether.type = EIP;
        mh.ar_family = SK_FAM_UNSPEC;
        mh.ar_len = sizeof(mh);

        /* If the packet was not sent successfully then it needs to be
           deallocated, else it will get lost. */
        if (((*ar_entry->ar_device->dev_output)(ar_entry->ar_buf_ptr,
                                                ar_entry->ar_device,
                                                (VOID *)&mh,
                                                NU_NULL)) != NU_SUCCESS)
        {
            /* The packet was not sent. Put it onto the dlist for this
               buffer so that we can try to retransmit it again. */
            MEM_Multiple_Buffer_Chain_Free(ar_entry->ar_buf_ptr);
        }

        /* Save a pointer to the next entry in the list since we are about
         * to deallocate the memory for this entry.
         */
        ar_temp_entry = ar_entry->ar_next;

        /* Deallocate this entry. */
        DLL_Remove(&ARP_Res_List, ar_entry);

#if (INCLUDE_STATIC_BUILD == NU_FALSE)

        if (NU_Deallocate_Memory(ar_entry) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate memory for ARP entry",
                           NERR_SEVERE, __FILE__, __LINE__);

            NET_DBG_Notify(NU_INVALID_POINTER, __FILE__, __LINE__,
                           NU_Current_Task_Pointer(), NU_NULL);
        }
#else
        /* Turn the memory flag off to show that memory has been freed */
        NET_ARP_Memory_Flags[(UINT8)(ar_entry - NET_ARP_Memory)] = NU_FALSE;
#endif

        /* Find another entry pending resolution of this host */
        for (ar_entry = ar_temp_entry;
             ar_entry != NU_NULL;
             ar_entry = ar_entry->ar_next)
        {
            if (ar_entry->ar_dest == arp_spa)
                break;
        }
    }

} /* ARP_Check_Events */

/*************************************************************************
*
*   FUNCTION
*
*       ARP_Init
*
*   DESCRIPTION
*
*       Initialize the ARP module.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID ARP_Init(VOID)
{
    /* Clear the ARP Cache */
    UTL_Zero(ARP_Cache, (sizeof(ARP_ENTRY) * ARP_CACHE_LENGTH));

    /* The resolve list is initially empty. */
    ARP_Res_List.ar_head = NU_NULL;
    ARP_Res_List.ar_tail = NU_NULL;

    ARP_Res_Count = 0;

#if (INCLUDE_LL_CONFIG == NU_TRUE)

    /* Register the IPv4 link-local address Event Handler. */
    EQ_Register_Event(ARP_LL_Event_Handler, &ARP_LL_Event);

#endif

} /* ARP_Init */

/*************************************************************************
*
*   FUNCTION
*
*       ARP_Resolve
*
*   DESCRIPTION
*
*       This function attempts to resolve an ethernet hardware address.
*       If unable to find an entry in the ARP cache it will queue the
*       packet, an IP packet, that needs to be sent.  An ARP event will
*       be created to send an ARP request.  The IP packet is transmitted
*       once the address is resolved.
*
*   INPUTS
*
*       *int_face               Pointer to the device to on.
*       *ip_dest                Pointer to the IP destination.
*       *mac_dest               Pointer to the hardware address.  This
*                               will be filled in if found.
*       *buf_ptr                Pointer to a buffer containing an IP packet.
*
*   OUTPUTS
*
*       NU_SUCCESS              Indicates successful operation.
*       NU_UNRESOLVED_ADDR      Indicates the address is unresolved.
*                               The IP is queued pending resolution.
*       stat                    The reason memory cannot be returned
*
*************************************************************************/
STATUS ARP_Resolve(DV_DEVICE_ENTRY *int_face, const SCK_SOCKADDR_IP *ip_dest,
                   UINT8 *mac_dest, NET_BUFFER *buf_ptr)
{
    ARP_RESOLVE_ENTRY   *ar_entry = NU_NULL, *current_ar_entry;
    STATUS              stat;
    ARP_ENTRY           *a_entry;
    UINT8               temp[4];
	
    memset (&temp,0,sizeof(UINT8)*4);

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
    /* Counter for traversing an array */
    INT                 i;
#endif

    /* If this is a broadcast packet then simply return the ethernet broadcast
       address. */
    if (buf_ptr->mem_flags & NET_BCAST)
    {
        memcpy(mac_dest, NET_Ether_Broadaddr, DADDLEN);
        return (NU_SUCCESS);
    }

    PUT32(temp, 0, ip_dest->sck_addr);

    if (buf_ptr->mem_flags & NET_MCAST)
    {
        NET_MAP_IP_TO_ETHER_MULTI(temp, mac_dest);
        return (NU_SUCCESS);
    }

    /* Check the ARP cache for the destination.  If found, return. */
    a_entry = ARP_Find_Entry(ip_dest);

    if (a_entry != NU_NULL)
    {
        memcpy(mac_dest, a_entry->arp_mac_addr, DADDLEN);

        /* It is possible that this entry was manually added without
         * an interface index.  If so, set the interface index now.
         */
        if (a_entry->arp_dev_index == -1)
            a_entry->arp_dev_index = int_face->dev_index;

        return (NU_SUCCESS);
    }

#if (INCLUDE_STATIC_BUILD == NU_FALSE)

    stat = NU_Allocate_Memory(MEM_Cached, (VOID **)&ar_entry,
                              sizeof(*ar_entry), (UNSIGNED)NU_NO_SUSPEND);

#else

    /* Check memory flags to find the unused memory */
    for (i=0; (NET_ARP_Memory_Flags[i] != NU_FALSE) && (i != NET_MAX_ARPS); i++)
        ;

    if (i != NET_MAX_ARPS)
    {
        ar_entry = &NET_ARP_Memory[i];      /* assign unused memory */
        NET_ARP_Memory_Flags[i] = NU_TRUE;  /* Turn the memory flag on */
        stat = NU_SUCCESS;
    }
    else
        stat = NU_NO_MEMORY;                /* if unused memory could not be found*/
#endif

    if (stat != NU_SUCCESS)
    {
        return (stat);
    }

    ar_entry->ar_id         = ARP_Res_Count++;
    ar_entry->ar_device     = int_face;
    ar_entry->ar_dest       = ip_dest->sck_addr;
    ar_entry->ar_send_count = 0;
    ar_entry->ar_task       = 0;              /* ar_task used only for RARP. */
    ar_entry->ar_buf_ptr    = buf_ptr;
    ar_entry->ar_pkt_type   = ARPREQ;

    /* If there is already an ARP entry for this host then we do not want to
       set an event, ie. don't send an ARP if we have already done so. */
    for (current_ar_entry = ARP_Res_List.ar_head;
         current_ar_entry != NU_NULL;
         current_ar_entry = current_ar_entry->ar_next)
    {
        if (TLS_Comparen(&ar_entry->ar_dest, &current_ar_entry->ar_dest, 4))
            break;
    }

    /* Order is not important.  Simply add the new entry to the queue. */
    DLL_Enqueue(&ARP_Res_List, ar_entry);

    /* Set the event if one was not found. */
    if (!current_ar_entry)
    {
    	/* If the event could not be set, clean up the outstanding resources. */
        if (EQ_Put_Event(ARPRESOLVE, (UNSIGNED)ar_entry->ar_id, 0) != NU_SUCCESS)
        {
        	/* Remove the entry from the ARP resolution list. */
        	DLL_Remove(&ARP_Res_List, ar_entry);

#if (INCLUDE_STATIC_BUILD == NU_FALSE)

        	/* Deallocate the memory allocated for the entry. */
        	if (NU_Deallocate_Memory(ar_entry) != NU_SUCCESS)
                NLOG_Error_Log("Failed to deallocate memory for ARP entry",
                               NERR_SEVERE, __FILE__, __LINE__);
#endif

            NLOG_Error_Log("Failed to set event to resolve address",
                           NERR_SEVERE, __FILE__, __LINE__);

            /* Return an error. */
        	return (NU_NO_MEMORY);
        }
    }

    return (NU_UNRESOLVED_ADDR);

} /* ARP_Resolve */

/*************************************************************************
*
*   FUNCTION
*
*       ARP_Find_Entry
*
*   DESCRIPTION
*
*       This function searches the ARP cache for a matching entry.
*
*   INPUTS
*
*       *dest                   IP address for which an ethernet address
*                               is desired.
*
*   OUTPUTS
*
*       *ARP_ENTRY              A pointer to an entry in the ARP cache.
*       NU_NULL                 If failure.
*
*************************************************************************/
ARP_ENTRY *ARP_Find_Entry(const SCK_SOCKADDR_IP *dest)
{
    INT         i;

    /* Search the cache for the target IP number. */
    for (i = 0; i < ARP_CACHE_LENGTH; i++)
    {
        if (dest->sck_addr == ARP_Cache[i].ip_addr.arp_ip_addr)
        {
            /* We found the entry. */
            if ( ((INT32_CMP((ARP_Cache[i].arp_time + CACHETO),
                             NU_Retrieve_Clock()) > 0) ||
                  (ARP_Cache[i].arp_flags & ARP_PERMANENT)) &&
                 (ARP_Cache[i].arp_flags & ARP_UP) )
                return (&(ARP_Cache[i]));
        }
    }

    return (NU_NULL);

} /* ARP_Find_Entry */

/*************************************************************************
*
*   FUNCTION
*
*       ARP_Request
*
*   DESCRIPTION
*
*       Send an ARP request.
*
*   INPUTS
*
*       *device                 Pointer to the device.
*       *tip                    The target IP number
*       *thardware              The target hardware address.
*       protocol_type           Either EARP or ERARP.
*       arp_type                Either ARPREQ or RARPQ.
*
*   OUTPUTS
*
*       NU_SUCCESS              If success.
*       -1                      If error.
*
*************************************************************************/
STATUS ARP_Request(DV_DEVICE_ENTRY *device, const UINT32 *tip,
                   const UINT8 *thardware, UINT16 protocol_type,
                   UINT16 arp_type)
{
    NET_BUFFER      *buf_ptr;
    ARP_MAC_HEADER  mh;
    STATUS          stat;

#if ((INCLUDE_ARP_INFO_LOGGING == NU_TRUE) || (PRINT_ARP_MSG == NU_TRUE))
    ARP_LAYER       *arp_hdr;
#endif

    /* Ensure the device is UP and RUNNING before sending the packet.
     * Since this routine is called from a timer, the device could have
     * been brought down before the timer expired.
     */
    if ((device->dev_flags & (DV_UP | DV_RUNNING)) == (DV_UP | DV_RUNNING))
    {
        /* Build the ARP request packet.  The target hardware address is
         * unknown at this point, so pass in a string of NULL characters
         * as the target hardware address.
         */
        buf_ptr = ARP_Build_Pkt(device, *tip, thardware,
                                device->dev_addr.dev_addr_list.dv_head->
                                dev_entry_ip_addr, arp_type);

        if (buf_ptr)
        {
#if ((INCLUDE_ARP_INFO_LOGGING == NU_TRUE) || (PRINT_ARP_MSG == NU_TRUE))
            /* Get a pointer to the ARP header */
            arp_hdr = (ARP_LAYER *)buf_ptr->data_ptr;

            /* Store/Print the ARP header info */
            NLOG_ARP_Info(arp_hdr, NLOG_TX_PACK);
#endif

            /* This is a pseudo MAC header that is passed to the MAC layer
             * send function. The MAC layer information that is known is
             * filled in. A family of SK_FAM_UNSPEC lets the MAC layer know
             * that this is not an IP datagram and it should not try to
             * resolve a hardware address.
             */
            memcpy (mh.ar_mac.ar_mac_ether.dest, NET_Ether_Broadaddr, DADDLEN);
            memset(mh.ar_mac.ar_mac_ether.me, 0, DADDLEN);
            mh.ar_mac.ar_mac_ether.type = protocol_type;
            mh.ar_family = SK_FAM_UNSPEC;
            mh.ar_len = sizeof(mh);

            /* Send the ARP Packet. */
            stat = (*device->dev_output)(buf_ptr, device, (VOID *)&mh, NU_NULL);

            /* if the packet is not sent then free up the buffer space */
            if (stat != NU_SUCCESS)
            {
                if (MEM_Buffer_Enqueue(&MEM_Buffer_Freelist,
                                    (NET_BUFFER *)buf_ptr) == NU_NULL)
                    NLOG_Error_Log("Failed to enqueue the buffer onto the Free List",
                                NERR_SEVERE, __FILE__, __LINE__);
            }
        }

        else
            stat = -1;
    }

    else
        stat = -1;

    return (stat);

} /* ARP_Request */

/*************************************************************************
*
*   FUNCTION
*
*       ARP_Cache_Update
*
*   DESCRIPTION
*
*       Add an entry to the ARP cache.
*
*   INPUTS
*
*       ipn                     The IP address
*       *hrdn                   The pointer to the hardware address
*       flags                   The arp flags
*       dev_index               The device index of the outgoing
*                               interface associated with this entry.
*
*   OUTPUTS
*
*       INT                     Index of entry made in ARP cache.
*       -1                      Entry was not found.
*
*************************************************************************/
INT ARP_Cache_Update(UINT32 ipn, const UINT8 *hrdn, INT flags,
                     INT32 dev_index)
{
    INT16   i, found = -1;
    UINT32  timer;

    /*
     * linear search to see if we already have this entry
     */
    for (i = 0; i < ARP_CACHE_LENGTH; i++)
    {
        if (ipn == ARP_Cache[i].ip_addr.arp_ip_addr)
        {
            found = i;
            break;
        }
    }

    /*
     *  if that IP number is not already here, take the oldest
     *  entry.  If it is already here, update the info and reset the
     *  timer.  These were pre-initialized to 0, so if any are blank,
     *  they will be taken first because they are faked to be oldest.
     */
    if (found < 0)
    {
        found = 0;
        timer = ARP_Cache[found].arp_time;

        for (i = 1; i < ARP_CACHE_LENGTH; i++)
        {
            /* If this entry has timed-out and is not permanent then
               reuse it. */
            if ( (INT32_CMP(ARP_Cache[i].arp_time, timer) < 0) &&
                ( (ARP_Cache[i].arp_flags & ARP_PERMANENT) == 0) )
            {  /* exclude gateways */
                found = i;
                timer = ARP_Cache[i].arp_time;
            }  /* end if ARP_Cache check */
        }  /* end for ARP_CACHE_LENGTH*/
    }  /* end if found < 0 */

    if (found >= 0)
    {
        /*
         *   do the update to the cache
         */
        memcpy (ARP_Cache[found].arp_mac_addr, hrdn, DADDLEN);
        ARP_Cache[found].ip_addr.arp_ip_addr = ipn;
        ARP_Cache[found].arp_time = NU_Retrieve_Clock();
        ARP_Cache[found].arp_flags = (ARP_UP | flags);
        ARP_Cache[found].arp_dev_index = dev_index;
    }

    else
        NLOG_Error_Log ("Unable to add a ARP cache entry", NERR_RECOVERABLE,
                            __FILE__, __LINE__);

    return (found);

} /* ARP_Cache_Update */

/*************************************************************************
*
*   FUNCTION
*
*       ARP_Event
*
*   DESCRIPTION
*
*       Process an ARP timer event.
*
*   INPUTS
*
*       id                      The ID of the timer event
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID ARP_Event(UINT16 id)
{
    ARP_RESOLVE_ENTRY               *ar_entry;
    STATUS                          status = NU_SUCCESS;

    for (ar_entry = ARP_Res_List.ar_head;
         ar_entry != NU_NULL;
         ar_entry = ar_entry->ar_next)
    {
        if (ar_entry->ar_id == id)
            break;
    }

    /* Was an entry found? */
    if (ar_entry)
    {
        switch (ar_entry->ar_pkt_type)
        {

        case ARPREQ :

            /* We will send at most 5 ARP requests.  After sending the 5th
               timeout, and restart the pending task. */
            if (ar_entry->ar_send_count < 5)
            {
                /* Setup a timer event to send the next one. */
                status = TQ_Timerset(ARPRESOLVE, (UNSIGNED)id, ARPTO, 0);
                if (status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to set event to resolve address",
                                   NERR_SEVERE, __FILE__, __LINE__);

                    NET_DBG_Notify(NU_NO_MEMORY, __FILE__, __LINE__,
                                   NU_Current_Task_Pointer(), NU_NULL);
                }
                else
                {
                    /* Send the ARP request. */
                    status = ARP_Request(ar_entry->ar_device, &ar_entry->ar_dest,
                                         (UINT8 *)"\0\0\0\0\0\0", EARP, ARPREQ);
                    if (status != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to transmit ARP request",
                                       NERR_SEVERE, __FILE__, __LINE__);
                    }
                 }

                /* Increment the number of tries. */
                ar_entry->ar_send_count++;
            }
            else
            {
                status = -1;
            }

            if (status != NU_SUCCESS)
            {
                /* After 5 tries we have not received a response.  Cleanup. */
                ARP_Cleanup_Entry(ar_entry);
            }

            break;

#if (INCLUDE_RARP == NU_TRUE)

        case RARPQ :

            if (ar_entry->ar_send_count < RARP_MAX_ATTEMPTS)
            {
                /* At this point the device is DV_RUNNING (It has been
                   initialized) but not DV_UP (There is no IP address attached.)
                   In order to send a packet the device must be up and running.
                   So temporarily set the device to up so the RARP request can
                   be sent. */
                ar_entry->ar_device->dev_flags |= DV_UP;

                /* Send the RARP request. */
                if (ARP_Request(ar_entry->ar_device, (UINT32 *)IP_Null,
                                ar_entry->ar_device->dev_mac_addr,
                                ERARP, RARPQ) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to transmit ARP request",
                                   NERR_SEVERE, __FILE__, __LINE__);

                /* Clear the DV_UP flag. */
                ar_entry->ar_device->dev_flags &= (~DV_UP);

                /* Setup a timer event to send the next one in one second. */
                if (TQ_Timerset(ARPRESOLVE, (UNSIGNED)id,
                                SCK_Ticks_Per_Second, 0) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to set timer to resolve address",
                                   NERR_SEVERE, __FILE__, __LINE__);

                    NET_DBG_Notify(NU_NO_MEMORY, __FILE__, __LINE__,
                                   NU_Current_Task_Pointer(), NU_NULL);
                }

                /* Increment the number of tries. */
                ar_entry->ar_send_count++;
            }
            else
            {
                /* Remove from the list. */
                DLL_Remove(&ARP_Res_List, ar_entry);

                /* Start the pending task. */
                status = NU_Resume_Task(ar_entry->ar_task);

                if (status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to resume task", NERR_RECOVERABLE,
                                   __FILE__, __LINE__);

                    NET_DBG_Notify(status, __FILE__, __LINE__,
                                   NU_Current_Task_Pointer(), NU_NULL);
                }
            }

            break;

#endif /* INCLUDE_RARP == NU_TRUE */

            /* This should never be possible, but is here to appease the
                compiler. */
        default :
            ;

        } /* switch */
    }

} /* ARP_Event */

/**************************************************************************
*
*   FUNCTION
*
*       ARP_Cleanup_Entry
*
*   DESCRIPTION
*
*       Cleans up an outstanding ARP entry pending resolution.
*
*   INPUTS
*
*       *ar_entry               A pointer to the entry being cleaned up.
*
*   OUTPUTS
*
*       None.
*
**************************************************************************/
VOID ARP_Cleanup_Entry(ARP_RESOLVE_ENTRY *ar_entry)
{
    UINT32          arp_host_ip;

    /* Save off the host's IP address */
    arp_host_ip = ar_entry->ar_dest;

    /* Free all ARPs for this host */
    do
    {
        /* Deallocate this buffer.  */
        MEM_Multiple_Buffer_Chain_Free(ar_entry->ar_buf_ptr);

        /* Deallocate the structure. */
        DLL_Remove(&ARP_Res_List, ar_entry);

#if (INCLUDE_STATIC_BUILD == NU_FALSE)

        if (NU_Deallocate_Memory(ar_entry) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate memory for ARP entry",
                           NERR_SEVERE, __FILE__, __LINE__);

            NET_DBG_Notify(NU_INVALID_POINTER, __FILE__, __LINE__,
                           NU_Current_Task_Pointer(), NU_NULL);
        }

#else
        /* Turn off the memory flag */
        NET_ARP_Memory_Flags[(UINT8)(ar_entry - NET_ARP_Memory)] = NU_FALSE;

#endif
        /* There may be more entries for this host. We need to free
           those buffers too. */
        for (ar_entry = ARP_Res_List.ar_head;
             ar_entry != NU_NULL;
             ar_entry = ar_entry->ar_next)
        {
            if (ar_entry->ar_dest == arp_host_ip)
                break;
        }

    } while (ar_entry);

} /* ARP_Cleanup_Entry */

#if (INCLUDE_LL_CONFIG == NU_TRUE)

/**************************************************************************
*
*   FUNCTION
*
*       ARP_LL_Event_Handler
*
*   DESCRIPTION
*
*       This routine is the timer event callback for the Link-Local
*       state machine.  It will walk the link local algorithm through
*       various states while negotiating an IP address to use for the link.
*
*   INPUTS
*
*       dev_index               The interface index for the interface for
*                               which the timer has expired.
*
*   OUTPUTS
*
*       None.
*
**************************************************************************/
VOID ARP_LL_Event_Handler(TQ_EVENT event, UNSIGNED dev_index, UNSIGNED ext_data)
{
    DV_DEVICE_ENTRY     *device;
    UINT32              timeout;
    STATUS              ret_status;
    UINT8               link_local_addr[IP_ADDR_LEN];
    UINT8               subnet[] = {255, 255, 0, 0};
    DEV_IF_ADDR_ENTRY   *addr_struct;

    UNUSED_PARAMETER(event);
    UNUSED_PARAMETER(ext_data);

    /* Get a pointer to the device structure associated with the dev_index. */
    device = DEV_Get_Dev_By_Index((UINT32)dev_index);

    /* If the device exists in the system and is configured with a
     * link-local address.
     */
    if ( (device != NU_NULL) &&
         (device->dev_flags & DV_CFG_IPV4_LL_ADDR) )
    {
        /* Process the timer event depending on state. */
        switch (device->dev_ll_state)
        {
            case LL_STATE_PROBE_WAIT:

                /* If the link-local IP address is IP_ADDR_ANY, select
                 * a new link-local IP address candidate now.
                 */
                if (device->dev_addr.dev_link_local_addr == IP_ADDR_ANY)
                {
                    device->dev_addr.dev_link_local_addr =
                        DEV_Generate_LL_Addr();
                }

                /* Issue the probe. */
                ret_status = ARP_Probe(device, IP_ADDR_ANY);

                if (ret_status != NU_SUCCESS)
                {
                    /* Couldn't send the probe, log an error. */
                    NLOG_Error_Log("Failed to transmit the ARP Probe",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }

                else
                {
                    /* Increment the number of probes transmitted. */
                    device->dev_ll_probe_count ++;
                }

                /* RFC 3927 - section 2.2.1 - ... the host should then
                 * wait for a random time interval selected uniformly
                 * in the range zero to PROBE_WAIT seconds, and should
                 * then send PROBE_NUM probe packets, each of these
                 * probe packets spaced randomly, PROBE_MIN to PROBE_MAX
                 * seconds apart.
                 */
                if (device->dev_ll_probe_count < LL_PROBE_NUM)
                {
                    /* Stay in Probe Wait with a random probe time if we
                     * still have more probes to send.
                     */
                    timeout = ((UTL_Rand() %
                               (LL_PROBE_MAX_TIME - LL_PROBE_MIN_TIME)) +
                               LL_PROBE_MIN_TIME);
                }

                else
                {
                    /* Transition to Announce Wait with a fixed time if
                     * we have sent all the probes.
                     */
                    device->dev_ll_state = LL_STATE_ANNOUNCE_WAIT;

                    /* Initialize the number of announcements made. */
                    device->dev_ll_announce_count = 0;

                    timeout = LL_ANNOUNCE_WAIT;
                }

                /* Set the timer for the next event. */
                ret_status = TQ_Timerset(ARP_LL_Event,
                                         (UNSIGNED)device->dev_index,
                                         timeout, 0);

                if (ret_status != NU_SUCCESS)
                {
                    /* Couldn't set the timer, log an error. */
                    NLOG_Error_Log("Failed to set link-local address timer",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }

                break;

            case LL_STATE_ANNOUNCE_WAIT:

                /* We are in Announce Wait so we've sent our last probe.
                 * Set up the Announce state.
                 */
                device->dev_ll_state = LL_STATE_ARP_ANNOUNCE;

                /* Convert the 32-bit address into a 4-byte array. */
                PUT32(link_local_addr, 0, device->dev_addr.dev_link_local_addr);

                /* RFC 3927 - section 2.2.1 - If, by ANNOUNCE_WAIT
                 * seconds after the transmission of the last ARP Probe
                 * no conflicting ARP Reply or ARP Probe has been received,
                 * then the host has successfully claimed the desired IPv4
                 * Link-Local address.
                 */
                if (DEV_Attach_IP_To_Device(device->dev_net_if_name,
                                            link_local_addr,
                                            subnet) == NU_SUCCESS)
                {
                    /* RFC 3927 - section 2.4 - Having probed to determine
                     * a unique address to use, the host MUST then announce
                     * its claimed address by broadcasting ANNOUNCE_NUM ARP
                     * announcements, spaced ANNOUNCE_INTERVAL seconds apart.
                     */
                    if (ARP_Probe(device,
                                  device->dev_addr.dev_link_local_addr) != NU_SUCCESS)
                    {
                        /* If we failed to send the probe, log an error. */
                        NLOG_Error_Log("Failed to transmit ARP Probe",
                                       NERR_SEVERE, __FILE__, __LINE__);
                    }

                    else
                    {
                        /* Count the announcement. */
                        device->dev_ll_announce_count = 1;
                    }

                    /* Set a timer to send a subsequent announce. */
                    timeout = LL_ANNOUNCE_INTERVAL;

                    if (TQ_Timerset(ARP_LL_Event, (UNSIGNED)device->dev_index,
                                    timeout, 0) != NU_SUCCESS)
                    {
                        /* If we couldn't set the timer, log an error. */
                        NLOG_Error_Log("Failed to set link-local address timer",
                                       NERR_SEVERE, __FILE__, __LINE__);
                    }
                }

                /* Otherwise, a routable address must have been added to
                 * the interface while trying to probe for the link-local
                 * address.  Stop the link-local state machine now, but do
                 * not disable link-local autoconfiguration on the interface,
                 * in case the routable address is removed later.
                 */
                else
                {
                    /* Reset the state. */
                    device->dev_ll_state = 0;

                    /* Reset the link-local address to zero. */
                    device->dev_addr.dev_link_local_addr = IP_ADDR_ANY;

                    NLOG_Error_Log("Link-local address not added to interface",
                                   NERR_INFORMATIONAL, __FILE__, __LINE__);
                }

                break;

            case LL_STATE_ARP_ANNOUNCE:

                /* We are announcing our IP address.  See if we have
                 * more announcements to make before transitioning
                 */
                if (device->dev_ll_announce_count < LL_ANNOUNCE_NUM)
                {
                    /* Send the next announcement. */
                    ret_status = ARP_Probe(device, device->dev_addr.dev_link_local_addr);

                    if (ret_status != NU_SUCCESS)
                    {
                        /* If we failed to send the probe, log an error. */
                        NLOG_Error_Log("Failed to transmit ARP Probe",
                                       NERR_SEVERE, __FILE__, __LINE__);
                    }

                    else
                    {
                        /* Count the announcement. */
                        device->dev_ll_announce_count ++;
                    }

                    /* Set the timer for the next event after this
                     * announcement.
                     */
                    timeout = LL_ANNOUNCE_INTERVAL;

                    ret_status = TQ_Timerset(ARP_LL_Event,
                                             (UNSIGNED)device->dev_index,
                                             timeout, 0);

                    if (ret_status != NU_SUCCESS)
                    {
                        /* If we couldn't set the timer, log an error. */
                        NLOG_Error_Log("Failed to set link-local address timer",
                                       NERR_SEVERE, __FILE__, __LINE__);
                    }
                }

                else
                {
                    /* We've sent all the announcements, so transition to the
                     * Claimed state.
                     */
                    device->dev_ll_state = LL_STATE_CLAIMED;
                }

                break;

            case LL_STATE_CONFLICT:

                /* A collision has been detected.  So go back to Probe
                 * Wait and try another IP address.
                 */
                device->dev_ll_state = LL_STATE_PROBE_WAIT;
                device->dev_ll_probe_count = 0;

                /* Get a pointer to the address structure for this address. */
                addr_struct =
                    DEV_Find_Target_Address(device,
                                            device->dev_addr.dev_link_local_addr);

                if (addr_struct)
                {
                    /* Delete the address from the interface. */
                    DEV4_Delete_IP_From_Device(device, addr_struct);
                }

                /* Reset the state. */
                device->dev_ll_state = 0;

                /* Reset the link-local IP address. */
                device->dev_addr.dev_link_local_addr = IP_ADDR_ANY;

                /* RFC 3927 - section 2.2.1 - If the number of conflicts
                 * exceeds MAX_CONFLICTS then the host MUST limit the
                 * rate at which it probes for new addresses to no more
                 * than one new address per RATE_LIMIT_INTERVAL.
                 */
                if (device->dev_ll_conflict_count > LL_MAX_COLLISIONS)
                    timeout = LL_RATE_LIMIT_INTERVAL;
                else
                    timeout = UTL_Rand() % LL_PROBE_WAIT_TIME;

                /* Restart the link local state machine by setting the timer
                 * while we're in probe_wait state.
                 */
                ret_status = TQ_Timerset(ARP_LL_Event,
                                         (UNSIGNED) device->dev_index,
                                         timeout, 0);

                if (ret_status != NU_SUCCESS)
                {
                    /* If we couldn't set the timer, log an error. */
                    NLOG_Error_Log("Failed to set link-local timer",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }

                break;

            default:

                /* An unknown state, ignore. */
                break;
        }
    }

} /* ARP_LL_Event_Handler */

/*************************************************************************
*
*   FUNCTION
*
*       ARP_Probe
*
*   DESCRIPTION
*
*       Send an ARP Probe.  This is used for both normal probes and
*       announcements.  This routine is a lot like sending an ARP_Request
*       except that with a Probe, the source MAC Address and Source IP
*       Addresses are not necessarily the same as those that would be used
*       in the request.
*
*   INPUTS
*
*       *device                 Pointer to the interface out which to
*                               transmit the message.
*       sip                     The link-local address of the network
*                               interface.
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation was successful.
*       -1                      An error occurred.
*
*************************************************************************/
STATUS ARP_Probe(DV_DEVICE_ENTRY *device, UINT32 sip)
{
    NET_BUFFER      *buf_ptr;
    ARP_MAC_HEADER  mh;
    STATUS          stat;
    INT             flags;
    static UINT8    Ether_Anyaddr[DADDLEN] =
                        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

#if ((INCLUDE_ARP_INFO_LOGGING == NU_TRUE) || (PRINT_ARP_MSG == NU_TRUE))
    ARP_LAYER       *arp_hdr;
#endif

    /* RFC 3927 - section 2.2.1 - A host probes to see if an address is
     * already in use by broadcasting an ARP Request for the desired
     * address.  The client MUST fill in the 'sender hardware address'
     * field of the ARP Request with the hardware address of the interface
     * through which it is sending the packet.  The 'sender IP address'
     * field MUST be set to all zeroes, to avoid polluting ARP caches in
     * other hosts on the same link in the case where the address turns
     * out to be already in use by another host.  The 'target hardware address'
     * field is ignored and SHOULD be set to all zeroes.  The 'target IP
     * address' field MUST be set to the address being probed.
     *
     * RFC 3927 - section 2.4 - Having probed to determine a unique address
     * to use, the host MUST then announce its claimed address by
     * broadcasting ARP announcements...  An ARP announcement is identical
     * to the ARP Probe described above, except that now the sender and target
     * IP addresses are both set to the host's newly selected IPv4 address.
     */
    buf_ptr = ARP_Build_Pkt(device, device->dev_addr.dev_link_local_addr,
                            Ether_Anyaddr, sip, ARPREQ);

    if (buf_ptr)
    {
#if ((INCLUDE_ARP_INFO_LOGGING == NU_TRUE) || (PRINT_ARP_MSG == NU_TRUE))
        /* Get a pointer to the ARP header */
        arp_hdr = (ARP_LAYER *)buf_ptr->data_ptr;

        /* Store/Print the ARP header info */
        NLOG_ARP_Info(arp_hdr, NLOG_TX_PACK);
#endif

        /* This is a pseudo MAC header that is passed to the MAC layer
         * send function. The MAC layer information that is known is
         * filled in. A family of SK_FAM_UNSPEC lets the MAC layer know
         * that this is not an IP datagram and it should not try to
         * resolve a hardware address.
         */
        memcpy(mh.ar_mac.ar_mac_ether.dest, NET_Ether_Broadaddr, DADDLEN);
        memset(mh.ar_mac.ar_mac_ether.me, 0, DADDLEN);
        mh.ar_mac.ar_mac_ether.type = EARP;
        mh.ar_family = SK_FAM_UNSPEC;
        mh.ar_len = sizeof(mh);

        /* Send the ARP Packet. */
        flags = device->dev_flags;
        device->dev_flags |= (DV_UP | DV_RUNNING);
        stat = (*device->dev_output)(buf_ptr, device, (VOID *)&mh, NU_NULL);
        device->dev_flags = flags;

        /* if the packet is not sent then free up the buffer space */
        if (stat != NU_SUCCESS)
        {
            if (MEM_Buffer_Enqueue(&MEM_Buffer_Freelist,
                                   (NET_BUFFER *)buf_ptr) == NU_NULL)
            {
                NLOG_Error_Log("Failed to enqueue the buffer onto the Free List",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

    else
    {
        stat = -1;
    }

    return (stat);

} /* ARP_Probe */
#endif
#endif
