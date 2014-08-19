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
*       dev_rd.c
*
*   DESCRIPTION
*
*       This file contains the implementation of NU_Remove_Device
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Remove_Device
*       DEV_Remove_Device
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "services/nu_trace_os_mark.h"

#if (INCLUDE_NAT == NU_TRUE)
#include "networking/nat_extr.h"
#endif

#if (INCLUDE_STATIC_BUILD == NU_TRUE)

/* Declare Memory for Devices */
extern DV_DEVICE_ENTRY     NET_Device_Memory[];

/* Declare memory flags for the above declared array (memory) */
extern UINT8               NET_Device_Memory_Flags[];

#endif /* (INCLUDE_STATIC_BUILD == NU_TRUE) */

#if ( (INCLUDE_ARP == NU_TRUE) || (INCLUDE_RARP == NU_TRUE) )
extern ARP_RESOLVE_LIST  ARP_Res_List;
#endif

/*************************************************************************
*
*   FUNCTION
*
*       NU_Remove_Device
*
*   DESCRIPTION
*
*       This function removes the specified device from the system.
*
*   INPUTS
*
*       *name                   A pointer to the name of the device.
*       flags                   Flags regarding how to remove the device.
*                               This parameter is currently unused.
*
*   OUTPUTS
*
*       NU_SUCCESS              Success
*       NU_INVALID_PARM         The device does not exist in the system.
*
*************************************************************************/
STATUS NU_Remove_Device(const CHAR *name, UINT32 flags)
{
    DV_DEVICE_ENTRY     *dev;
    STATUS              status;

    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();

    /* We must grab the NET semaphore */
    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status != NU_SUCCESS)
    {
        NU_USER_MODE();
        return (status);
    }

    /* Get the device using the IP Address */
    dev = DEV_Get_Dev_By_Name(name);

    if (dev)
    {
        /* Remove the device from the networking stack. */
        DEV_Remove_Device(dev, flags);

        /* Delete the device from the cache. */
        NU_Ifconfig_Delete_Interface(name);
    }
    else
        status = NU_INVALID_PARM;

    /* Release the semaphore */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

    NU_USER_MODE();
    
    /* Trace log */
    T_DEV_REMOVAL((char*)name, status);

    return (status);

} /* NU_Remove_Device */

/*************************************************************************
*
*   FUNCTION
*
*       DEV_Remove_Device
*
*   DESCRIPTION
*
*       This is the actual function that removes the device.
*
*   INPUTS
*
*       *dev                    A pointer to the device.
*       flags                   Flags regarding how to remove the device.
*                               This parameter is currently unused.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID DEV_Remove_Device(DV_DEVICE_ENTRY *dev, UINT32 flags)
{
    DV_REQ              d_req;
    INT                 irq_level;
    NET_BUFFER          *current_buf, *saved_buf = NU_NULL;

#if ( (INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IP_MULTICASTING == NU_TRUE) )
    IP_MULTI            *ipm;
    INT                 i;
#endif

#if ( (INCLUDE_IPV4 == NU_TRUE) && \
      ((INCLUDE_ARP == NU_TRUE) || (INCLUDE_RARP == NU_TRUE)) )
    ARP_RESOLVE_ENTRY               *ar_entry;
#endif

#if ( (INCLUDE_IP_MULTICASTING == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
    /* Leave all IPv4 multicast groups to which the interface
     * belongs
     */
    for (i = 0, ipm = dev->dev_addr.dev_multiaddrs;
         (i < IP_MAX_MEMBERSHIPS) && (ipm);
         i++)
    {
        /* Leave the group */
        IP_Delete_Multi(ipm);

        /* If the multicast group was not successfully deleted, get
         * a pointer to the next multicast group for the interface.
         */
        if (ipm == dev->dev_addr.dev_multiaddrs)
            ipm = ipm->ipm_next;
        else
            ipm = dev->dev_addr.dev_multiaddrs;
    }
#endif

#if (INCLUDE_IPV6 == NU_TRUE)

    /* If this is an IPv6 enabled device, remove all IPv6 resources */
    if (dev->dev_flags & DV6_IPV6)
        DEV6_Cleanup_Device(dev, flags);

#else

    UNUSED_PARAMETER(flags);

#endif

    UTL_Zero(&d_req, sizeof(DV_REQ));

    /* Notify the driver to stop accepting packets and to deallocate
     * any resources for the interface.
     */
    if (dev->dev_ioctl)
    {
        /* Instruct the driver to deallocate all resources for the
         * device
         */
        if ((*dev->dev_ioctl)(dev, DEV_REMDEV, &d_req) != NU_SUCCESS)
            return;
    }

#if ( (INCLUDE_IPV4 == NU_TRUE) && \
      ((INCLUDE_ARP == NU_TRUE) || (INCLUDE_RARP == NU_TRUE)) )

    /* Stop transmitting ARP requests out this interface */
    for (ar_entry = ARP_Res_List.ar_head;
         ar_entry != NU_NULL;
         ar_entry = ar_entry->ar_next)
    {
        /* Set the send count to the maximum so the next time the timer goes
         * off for this entry, all packets will be freed and the structure
         * deallocated.
         */
        if (ar_entry->ar_device == dev)
            ar_entry->ar_send_count = 255;
    }

#endif

    /* Remove all IP addresses from the interface */
    DEV_Detach_Addrs_From_Device(dev);

    /* Resume all sockets that are bound to an address on this interface
     * or are using an address on this interface as the source address
     * for communications.
     */
    DEV_Resume_All_Open_Sockets();

    /* Remove all packets from the transmit list. */
    while (dev->dev_transq.head)
        DEV_Recover_TX_Buffers(dev);

    /* Remove all buffers from the Buffer List using this device */
    irq_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

    current_buf = MEM_Buffer_Dequeue(&MEM_Buffer_List);

    /* While there are buffers on the list and we have not looped
     * through the entire list of buffers.
     */
    while ( (current_buf) && (current_buf != saved_buf) )
    {
        /* If this buffer was received on the device being deleted,
         * free the buffer.
         */
        if (current_buf->mem_buf_device == dev)
            MEM_One_Buffer_Chain_Free(current_buf, &MEM_Buffer_Freelist);

        /* Otherwise, put the buffer back in the Buffer List */
        else
        {
            /* If a buffer not using the deleted device has not already
             * been found, save a pointer to this buffer.
             */
            if (saved_buf == NU_NULL)
                saved_buf = current_buf;

            MEM_Buffer_Enqueue(&MEM_Buffer_List, current_buf);
        }

        /* Get the next buffer on the list */
        current_buf = MEM_Buffer_Dequeue(&MEM_Buffer_List);
    }

    /* If saved_buf is not NULL, and current_buf is equal to saved_buf, put
     * saved_buf back on the Free List since it was removed in the above
     * loop but never freed or put back on the Free List.
     */
    if ( (saved_buf) && (saved_buf == current_buf) )
    {
        MEM_Buffer_Enqueue(&MEM_Buffer_List, saved_buf);
    }

    /* Re-enable interrupts */
    NU_Local_Control_Interrupts(irq_level);

    /* Remove the interface from the list of interfaces */
    DLL_Remove(&DEV_Table, dev);

#if (INCLUDE_IF_STACK == NU_TRUE)

    /* Remove all stack entries that are associated with this device. */
    MIB2_If_Stack_Remove_Dev(dev);

#endif

#if ( (INCLUDE_STATIC_BUILD == NU_FALSE) && (MIB2_IF_INCLUDE == NU_TRUE) )

    /* If memory had been allocated to the Ether group in MIB-II,
       deallocate that memory. */
    if (dev->dev_mibInterface.eth != NU_NULL)
    {
        if (NU_Deallocate_Memory(dev->dev_mibInterface.eth) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate memory",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

#if (INCLUDE_IF_EXT == NU_TRUE)

    /* If memory had been allocated to the extension table for MIB-II,
       deallocate that memory. */
    if (dev->dev_mibInterface.mib2_ext_tbl != NU_NULL)
    {
        /* Also deallocate memory for the High-Capacity counters. */
        if (dev->dev_mibInterface.mib2_ext_tbl->mib2_hc != NU_NULL)
        {
            if (NU_Deallocate_Memory(dev->dev_mibInterface.mib2_ext_tbl->mib2_hc) != NU_SUCCESS)
                NLOG_Error_Log("Failed to deallocate memory",
                               NERR_SEVERE, __FILE__, __LINE__);
        }

        if (NU_Deallocate_Memory(dev->dev_mibInterface.mib2_ext_tbl) != NU_SUCCESS)
            NLOG_Error_Log("Failed to deallocate memory",
                           NERR_SEVERE, __FILE__, __LINE__);
    }

#endif
#endif

#if (INCLUDE_NAT == NU_TRUE)
#ifdef NAT_1_4
    /* Notify NAT that this interface has been removed.  NAT may need to
     * do some clean up if this interface was previously initialized to
     * be used for NAT processing.
     */
    NAT_Remove_Interface(dev);
#endif
#endif

#if (INCLUDE_STATIC_BUILD == NU_FALSE)

#if (INCLUDE_MDNS == NU_TRUE)

    /* Deallocate the memory for the local host name.  All host name entries
     * for this interface were removed when the IP addresses were deleted.
     */
    if (dev->dev_host_name)
    {
        if (NU_Deallocate_Memory((VOID*)dev->dev_host_name) != NU_SUCCESS)
            NLOG_Error_Log("Failed to deallocate memory for local host name",
                           NERR_SEVERE, __FILE__, __LINE__);
    }
#endif

    /* Deallocate the memory being used by this interface */
    if (NU_Deallocate_Memory((VOID*)dev) != NU_SUCCESS)
        NLOG_Error_Log("Failed to deallocate memory",
                       NERR_SEVERE, __FILE__, __LINE__);
#else

    /* Mark the memory used by the device as unused. */
    NET_Device_Memory_Flags[(UINT8)(dev - NET_Device_Memory)] = NU_FALSE;

#endif

} /* DEV_Remove_Device */
