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
* FILE NAME
*
*       arp_ra.c
*
* DESCRIPTION
*
*       This file contains the implementation of RARP.
*
* DATA STRUCTURES
*
*       none
*
* FUNCTIONS
*
*       ARP_Rarp
*
* DEPENDENCIES
*
*       nu_net.h
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_RARP == NU_TRUE)

extern ARP_RESOLVE_LIST  ARP_Res_List;
extern UINT16            ARP_Res_Count;

/*************************************************************************
*
*   FUNCTION
*
*       ARP_Rarp
*
*   DESCRIPTION
*
*       This function is responsible for resolving the IP address of
*       this host.  It is the entry point for applications that need to
*       use RARP (Reverse Address Resolution Protocol).  RARP is
*       typically required by disk-less workstations.  Such workstations
*       have no means to store their IP address locally.
*
*   INPUTS
*
*       *device_name            Name of device to resolve.
*
*   OUTPUTS
*
*       NU_MEM_ALLOC            Memory allocation failure.
*       NU_NO_BUFFERS           No buffers a re available to build the
*                               pkt in.
*       NU_RARP_INIT_FAILED     A response was not received to the
*                               request.
*       NU_SUCCESS              The IP was successfully resolved.
*       NU_INVALID_PARM         Device name is invalid.
*       NU_HOST_UNREACHABLE     Device is not initialized.
*       status                  The reason the Semaphore cannot be
*                               obtained
*
*************************************************************************/
STATUS ARP_Rarp(const CHAR *device_name)
{
    ARP_RESOLVE_ENTRY   *ar_entry;
    DV_DEVICE_ENTRY     *device;
    UINT8               mask[IP_ADDR_LEN];
    UINT8               dev_addr[IP_ADDR_LEN];
    STATUS              status;
    UINT32              flags;
    DEV_IF_ADDR_ENTRY   *dev_addr_entry;

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
    /* Declare memory for an ARP resolve entry */
    ARP_RESOLVE_ENTRY   rarp_memory;
#endif

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /*  Don't let any other users in until we are done.  */
    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status != NU_SUCCESS)
    {
        /* Switch back to user mode. */
        NU_USER_MODE();
        return (status);
    }

    /* Look up the device for which an IP address needs to be resolved. */
    device = DEV_Get_Dev_By_Name(device_name);

    if (device == NU_NULL)
    {
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);

        /* Switch back to user mode. */
        NU_USER_MODE();

        return (NU_INVALID_PARM);
    }

    /* At this point the device is not completely up yet because there is no IP
       address attached to the device.  However, the device must have been
       initialized before Rarp is called. */
    if (!(device->dev_flags & DV_RUNNING))
    {
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);

        /* Switch back to user mode. */
        NU_USER_MODE();

        return (NU_HOST_UNREACHABLE);
    }

    /* Get a pointer to the address entry being resolved */
    dev_addr_entry = DEV_Find_Target_Address(device, 0);

    if (dev_addr_entry == NU_NULL)
    {
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);

        /* Switch back to user mode. */
        NU_USER_MODE();

        return (NU_INVALID_PARM);
    }

    /* Save off the flags for the interface */
    flags = device->dev_flags;

    /* At this point the device is DV_RUNNING (It has been initialized) but
       not DV_UP (There is no IP address attached.  In order to send a
       packet the device must be up and running.  So temporarily set the
       device to up so the RARP request can be sent. */
    device->dev_flags |= DV_UP;

    /* Send the RARP request. */
    if (ARP_Request(device, (UINT32 *)IP_Null,  device->dev_mac_addr, ERARP,
                    RARPQ) != NU_SUCCESS)
    {
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);

        /* Switch back to user mode. */
        NU_USER_MODE();

        return (NU_NO_BUFFERS);
    }

    /* Restore the flags */
    device->dev_flags = flags;

#if (INCLUDE_STATIC_BUILD == NU_FALSE)

    /* Allocate memory for an ARP resolve entry.  This structure is used to keep
       track of this resolution attempt. */
    if (NU_Allocate_Memory(MEM_Cached, (VOID **)&ar_entry,
                           sizeof(*ar_entry),
                           (UNSIGNED)NU_NO_SUSPEND) != NU_SUCCESS)
    {
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);

        /* Switch back to user mode. */
        NU_USER_MODE();

        return (NU_MEM_ALLOC);
    }

#else
    /* Assign memory to the ARP resolve entry */
    ar_entry = &rarp_memory;

#endif

    /* Initialize the entry structure. */
    ar_entry->ar_id         = ARP_Res_Count++;
    ar_entry->ar_device     = device;
    ar_entry->ar_dest       = 0xffffffffUL;
    ar_entry->ar_send_count = 1;            /* Already sent once above. */
    ar_entry->ar_task       = NU_Current_Task_Pointer();
    ar_entry->ar_buf_ptr    = NU_NULL;
    ar_entry->ar_pkt_type   = RARPQ;

    /* Order is not important.  Simply add the new entry to the end of the
       list. */
    DLL_Enqueue(&ARP_Res_List, ar_entry);

    /* Transmit the next one in a second. */
    if (TQ_Timerset(RARP_REQUEST, (UNSIGNED)ar_entry->ar_id,
                    SCK_Ticks_Per_Second, 0) != NU_SUCCESS)
        NLOG_Error_Log("Failed to set timer to transmit RARP request", NERR_SEVERE,
                       __FILE__, __LINE__);

    /* Suspend this task pending the resolution of our IP address or a
       timeout. */
    SCK_Suspend_Task(NU_Current_Task_Pointer());

    if (dev_addr_entry->dev_entry_ip_addr != IP_ADDR_ANY)
    {
        /* Get the address into a local array. */
        PUT32(dev_addr, 0, dev_addr_entry->dev_entry_ip_addr);

        /* Get the mask associated with an address of this type. */
        if (IP_Get_Net_Mask(dev_addr, mask) != NU_SUCCESS)
            NLOG_Error_Log("Failed to get NET mask associated with address",
                           NERR_SEVERE, __FILE__, __LINE__);
        else
        {
            if (DEV_Initialize_IP(device, dev_addr, mask,
                                  dev_addr_entry) != NU_SUCCESS)
                NLOG_Error_Log("Failed to attach IP to device", NERR_SEVERE,
                               __FILE__, __LINE__);
        }
    }

#if (INCLUDE_STATIC_BUILD == NU_FALSE)

    /* Deallocate the ARP resolve entry structure. */
    if (NU_Deallocate_Memory(ar_entry) != NU_SUCCESS)
        NLOG_Error_Log("Failed to deallocate memory for ARP entry", NERR_SEVERE,
                       __FILE__, __LINE__);
#endif

    /* If we did not find an IP address, return an error. */
    if (!device->dev_addr.dev_addr_list.dv_head->dev_entry_ip_addr)
        status = NU_RARP_INIT_FAILED;

    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* ARP_Rarp */

#endif /* (INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_RARP == NU_TRUE) */
