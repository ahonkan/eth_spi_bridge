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
*   FILENAME
*
*       eth.c
*
*   DESCRIPTION
*
*       This file will hold all the routines which are used to interface
*       with the ethernet hardware.  They will handle the basic functions
*       of setup, xmit, receive, etc.
*
*   DATA STRUCTURES
*
*       ETH_Ether_Broadaddr[DADDLEN]
*
*   FUNCTIONS
*
*       ETH_Ether_Input
*       ETH_Ether_Send
*       ETH_Add_Multi
*       ETH_Del_Multi
*
*   DEPENDENCIES
*
*       nu_net.h
*       net4.h
*       802.h
*       net6.h
*
*************************************************************************/

#include "networking/nu_net.h"
#include "networking/802.h"
#include "services/nu_trace_os_mark.h"

#if (INCLUDE_IPV4 == NU_TRUE)
#include "networking/net4.h"
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/net6.h"
#endif

#if (INCLUDE_SNMP == NU_TRUE)
#include SNMP_GLUE
#endif

#if (INCLUDE_STATIC_BUILD == NU_TRUE)

/* Declare memory for the ethernet multicast addresses */
NET_MULTI  NET_Eth_Address_Memory[NET_MAX_MULTICAST_GROUPS];

/* Declare flag array for the memory declared above */
UINT8      NET_Eth_Address_Memory_Flags[NET_MAX_MULTICAST_GROUPS] = {0};
#endif

/*************************************************************************
*
*   FUNCTION
*
*       ETH_Ether_Input
*
*   DESCRIPTION
*
*       Examines input ethernet packet and determines what type of
*       packet it is.  It then calls the proper interpret routine.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       NU_SUCCESS
*
*************************************************************************/
STATUS ETH_Ether_Input(VOID)
{
    UINT8               *ether_pkt;
    DV_DEVICE_ENTRY     *device;

    /* Point to the packet. */
    ether_pkt = (UINT8 *)(MEM_Buffer_List.head->data_ptr);

    /* Get a pointer to the device. */
    device = MEM_Buffer_List.head->mem_buf_device;

    /* If this packet was addressed to either a broadcast or multicast address
       set the appropriate flag. */
    if (!memcmp((VOID *)(ether_pkt + ETHER_DEST_OFFSET),
                (VOID *)NET_Ether_Broadaddr, DADDLEN))
    {
        MEM_Buffer_List.head->mem_flags |= NET_BCAST;
        MIB2_InBroadcast_Pkts_Inc(device);

#if (INCLUDE_SNMP == NU_TRUE)
#if (INCLUDE_MIB_RMON1 == NU_TRUE)
        MIB2_BroadcastPkts_Inc(device);
#endif
#endif
    }
    else if (GET8(ether_pkt, ETHER_DEST_OFFSET) & 1)
    {
        MEM_Buffer_List.head->mem_flags |= NET_MCAST;
        MIB2_InMulticast_Pkts_Inc(device);

#if (INCLUDE_SNMP == NU_TRUE)
#if (INCLUDE_MIB_RMON1 == NU_TRUE)
        MIB2_MulticastPkts_Inc(device);
#endif
#endif
    }
    else
        MIB2_ifInUcastPkts_Inc(device);

    /* If the buffer includes the running total of the data to
     * be used for the checksum, remove the bytes for the ethernet header
     */
    if (MEM_Buffer_List.head->mem_flags & NET_BUF_SUM)
    {
        MEM_Buffer_List.head->chk_sum =
            TLS_Header_Memsum(MEM_Buffer_List.head->data_ptr, device->dev_hdrlen);
    }
    
    /* Trace log */
    T_DEV_RX_ACT(device->dev_net_if_name, MEM_Buffer_List.head->mem_total_data_len, NU_SUCCESS);

    /* Strip the ethernet header and size off of the packet */
    MEM_Buffer_List.head->data_ptr           += device->dev_hdrlen;
    MEM_Buffer_List.head->data_len           -= device->dev_hdrlen;
    MEM_Buffer_List.head->mem_total_data_len -= device->dev_hdrlen;

    /* Interpret the packet depending upon the protocol type field. */
    EightZeroTwo_Input(device, GET16(ether_pkt, ETHER_TYPE_OFFSET),
                       ether_pkt);

    return (NU_SUCCESS);

} /* ETH_Ether_Input */

/*************************************************************************
*
*   FUNCTION
*
*       ETH_Ether_Send
*
*   DESCRIPTION
*
*       This function is the hardware layer transmission function for
*       Ethernet.  Other physical mediums (serial, token ring, etc.)
*       will have their own transmit functions.  Given a packet to
*       transmit this function first resolves the hardware layer address
*       and then queues the packet for transmission.
*
*   INPUTS
*
*       *buf_ptr                Pointer to the net buffer
*       *device                 Pointer to the device
*       *dest                   Pointer to the destination to send to
*       *ro                     Pointer to the routing information
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful
*       NU_HOST_UNREACHABLE     Host cannot be reached, wrong information
*       -1                      Invalid socket family
*
*************************************************************************/
STATUS ETH_Ether_Send(NET_BUFFER *buf_ptr, DV_DEVICE_ENTRY *device,
                      VOID *dest, VOID *ro)
{
#ifndef PACKET
    INT                 old_int;
#endif
    UINT8               mac_dest[DADDLEN];
    UINT16              type;
    STATUS              status;

    /* Verify that the device is up. */
    if ( (device->dev_flags & (DV_UP | DV_RUNNING)) != (DV_UP | DV_RUNNING) )
        return (NU_HOST_UNREACHABLE);

    status = EightZeroTwo_Output(buf_ptr, device, dest, ro, mac_dest, &(type));

    if (status == NU_SUCCESS)
    {
        /* Move back the data pointer to make room for the MAC layer and adjust the
           packet length. */
        buf_ptr->data_ptr           -= device->dev_hdrlen;
        buf_ptr->data_len           += device->dev_hdrlen;
        buf_ptr->mem_total_data_len += device->dev_hdrlen;

        /* Initialize the ethernet header. */
        PUT16(buf_ptr->data_ptr, ETHER_TYPE_OFFSET, type);

        memcpy((buf_ptr->data_ptr + ETHER_DEST_OFFSET),
               ((unsigned char *)(mac_dest)), DADDLEN);

        memcpy((buf_ptr->data_ptr + ETHER_ME_OFFSET),
               ((unsigned char *)(device->dev_mac_addr)), DADDLEN);

        /* Is this a broadcast, multicast or unicast packet. */
        if (buf_ptr->mem_flags & NET_BCAST)
            MIB2_OutBroadcast_Pkts_Inc(device);

        else if (buf_ptr->mem_flags & NET_MCAST)
            MIB2_OutMulticast_Pkts_Inc(device);

        else
            MIB2_ifOutUcastPkts_Inc(device);

#ifdef PACKET

        /* Pass the packet to the device. */
        status = device->dev_start(device, buf_ptr);

        if (status != NU_SUCCESS)
            NLOG_Error_Log("Device failed to transmit packet", NERR_SEVERE,
                            __FILE__, __LINE__);

        /* Free the buffers onto the appropriate free lists */
        else
            MEM_Multiple_Buffer_Chain_Free(buf_ptr);

#else

        old_int = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

        /* Place the buffer on the device's transmit queue. */
        MEM_Buffer_Enqueue(&device->dev_transq, buf_ptr);

        ++device->dev_transq_length;

        /* Trace log */
        T_DEV_TRANSQ_LEN(device->dev_net_if_name, device->dev_transq_length);

        /* If this is the first buffer in the transmit queue we need to resume the
         * transmit task.  If not another packet is already being transmitted. */
        if (device->dev_transq.head == buf_ptr)
        {
            NU_Local_Control_Interrupts(old_int);

            /* Resume the the transmit task. */
            status = device->dev_start(device, buf_ptr);

            /* If the buffer could not be transmitted, remove it from the transmit
             * queue.
             */
            if (status != NU_SUCCESS)
            {
                MEM_Buffer_Dequeue(&device->dev_transq);

                /* Decrement the Q length for the device transmit Q.*/
                --(device->dev_transq_length);

                NLOG_Error_Log("Driver failed to transmit packet", NERR_SEVERE,
                               __FILE__, __LINE__);
            }
        }
        else
        {
            NU_Local_Control_Interrupts(old_int);
            status = NU_SUCCESS;
        }

#endif
    }

    else
    {
        if (status == NU_UNRESOLVED_ADDR)
        {
            /* To the upper layer protocols this is the same as a
               successful send. */
            status = NU_SUCCESS;
        }
    }

    /* Trace log */
    T_DEV_TRANSQ_LEN(device->dev_net_if_name, device->dev_transq_length);

    return (status);

} /* ETH_Ether_Send */

/*************************************************************************
*
*   FUNCTION
*
*       ETH_Add_Multi
*
*   DESCRIPTION
*
*       When an ethernet multicast group is joined this function adds an
*       entry to the list of ethernet multicast addresses a device should
*       receive.  If the specified address is already registered with the
*       device the reference count is incremented.
*
*   INPUTS
*
*       *dev                    Pointer to the device
*       *d_req                  Pointer to the device requirements
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful
*       NU_MEM_ALLOC            No memory to allocate from
*       NU_RESET                Multicasting is reset
*       -1                      Multicast address is invalid
*
*************************************************************************/
STATUS ETH_Add_Multi(DV_DEVICE_ENTRY *dev, const DV_REQ *d_req)
{
    UINT8       multi_addr[DADDLEN];
    INT         irq_level;
    NET_MULTI   *em;

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
    INT         i;                          /* Counter to traverse an array */
#endif

    irq_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

#if (INCLUDE_IPV6 == NU_TRUE)

    /* If the IPv6 flag is set, map the IPv6 address */
    if (d_req->dvr_flags & DEV_MULTIV6)
    {
        /* If the address is invalid, re-enable interrupts and return
         * an error.
         */
        if (ETH6_Map_Multi(d_req, multi_addr) == -1)
        {
            NU_Local_Control_Interrupts(irq_level);
            return (-1);
        }
    }
#endif

#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
    else
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

    /* Otherwise, if the address is an IPv4 address, map the IPv4 address */
    if (d_req->dvr_dvru.dvru_addr != IP_ADDR_ANY)
    {
        /* If the address is invalid, re-enable interrupts and return
         * an error.
         */
        if (ETH4_Map_Multi(d_req, multi_addr) == -1)
        {
            NU_Local_Control_Interrupts(irq_level);
            return (-1);
        }
    }

#endif

    else
    {
        NU_Local_Control_Interrupts(irq_level);
        return(-1);
    }

    /* Has this address already been added to the list. */
    for (em = dev->dev_ethermulti;
         em != NU_NULL && (memcmp(em->nm_addr, multi_addr, DADDLEN) != 0);
         em = em->nm_next)
              ;
    if (em != NU_NULL)
    {
        /* Found a match. Increment the reference count. */
        em->nm_refcount++;
        NU_Local_Control_Interrupts(irq_level);
        return (NU_SUCCESS);
    }

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    /* This is a new address. Allocate some memory for it. */
    if (NU_Allocate_Memory(MEM_Cached, (VOID **)&em,
                            sizeof (*em), (UNSIGNED)NU_NO_SUSPEND) != NU_SUCCESS)
    {
        NU_Local_Control_Interrupts(irq_level);
        return(NU_MEM_ALLOC);
    }

#else
    /* Traverse the flag array to find free memory location*/
    for(i=0; (NET_Eth_Address_Memory_Flags[i] != NU_FALSE) && (i != NET_MAX_MULTICAST_GROUPS); i++)
        ;
    if(i == NET_MAX_MULTICAST_GROUPS)
    {
        NU_Local_Control_Interrupts(irq_level);
        return(NU_NO_MEMORY);
    }
    /* Assign memory to the new address */
    em = &NET_Eth_Address_Memory[i];
    /* Turn on the memory flag */
    NET_Eth_Address_Memory_Flags[i] = NU_TRUE;

#endif    /* INCLUDE_STATIC_BUILD */

    /* Initialize the new entry. */
    memcpy(em->nm_addr, multi_addr, DADDLEN);
    em->nm_device = dev;
    em->nm_refcount = 1;

    /* Link it into the list. */
    em->nm_next = dev->dev_ethermulti;
    dev->dev_ethermulti = em;

    /*  Restore the previous interrupt lockout level.  */
    NU_Local_Control_Interrupts(irq_level);

    return(NU_RESET);

} /* ETH_Add_Multi */

/*************************************************************************
*
*   FUNCTION
*
*       ETH_Del_Multi
*
*   DESCRIPTION
*
*       When an ethernet multicast group is dropped this function will
*       delete the entry from the list of ethernet multicast addresses
*       this device should receive if the reference count drops to 0.
*       Else the reference count is decremented.
*
*   INPUTS
*
*       *dev                    Pointer to the device
*       *d_req                  Pointer to the device requirements
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful
*       NU_INVAL                Generic invalid return
*       NU_RESET                Multicasting is reset
*       -1                      Multicast address failed
*
*************************************************************************/
STATUS ETH_Del_Multi(const DV_DEVICE_ENTRY *dev, const DV_REQ *d_req)
{
    UINT8       multi_addr[DADDLEN];
    INT         irq_level;
    NET_MULTI   *em;
    NET_MULTI   **ptr;

    irq_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

#if (INCLUDE_IPV6 == NU_TRUE)

    /* If the IPv6 flag is set, map the IPv6 address */
    if (d_req->dvr_flags & DEV_MULTIV6)
    {
        /* If the address is invalid, re-enable interrupts and return
         * an error.
         */
        if (ETH6_Map_Multi(d_req, multi_addr) == -1)
        {
            NU_Local_Control_Interrupts(irq_level);
            return (-1);
        }
    }

    else
#endif

#if (INCLUDE_IPV4 == NU_TRUE)

    /* Otherwise, if the address is an IPv4 address, map the IPv4 address */
    {
        /* If the address is invalid, re-enable interrupts and return
         * an error.
         */
        if (ETH4_Map_Multi(d_req, multi_addr) == -1)
        {
            NU_Local_Control_Interrupts(irq_level);
            return (-1);
        }
    }

#else

    {
        NU_Local_Control_Interrupts(irq_level);
        return (-1);
    }

#endif

    /* Find this address in the list. */
    for (em = dev->dev_ethermulti;
         em != NU_NULL && (memcmp(em->nm_addr, multi_addr, DADDLEN) != 0);
         em = em->nm_next)
              ;

    if (em == NU_NULL)
    {
        /* Found a match. Increment the reference count. */
        NU_Local_Control_Interrupts(irq_level);
        return (NU_INVAL);
    }

    /* If this is not the last reference then return after decrementing the
       reference count. */
    if (--em->nm_refcount != 0)
    {
        NU_Local_Control_Interrupts(irq_level);
        return(NU_SUCCESS);
    }

    /* If we made this far then there are no more references to this entry.
       So unlink and deallocate it. */
    for (ptr = &em->nm_device->dev_ethermulti;
         *ptr != em;
         ptr = &(*ptr)->nm_next)
        continue;

    *ptr = (*ptr)->nm_next;

    /*  Restore the previous interrupt lockout level.  */
    NU_Local_Control_Interrupts(irq_level);

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    /* Deallocate the structure. */
    if (NU_Deallocate_Memory(em) != NU_SUCCESS)
        NLOG_Error_Log("Failed to deallocate memory for multicast structure",
                       NERR_SEVERE, __FILE__, __LINE__);
#else
    /* Turn the memory flag off to indicate the memory is now unused */
    NET_Eth_Address_Memory_Flags[(UINT8)(em - NET_Eth_Address_Memory)] = NU_FALSE;
#endif

    return (NU_RESET);

} /* ETH_Del_Multi */
