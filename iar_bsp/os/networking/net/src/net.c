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
*       net.c
*
*   DESCRIPTION
*
*       This file contains the Nucleus Net core functions and global
*       data structures.
*
*   DATA STRUCTURES
*
*       TCP_Resource
*       Buffers_Available
*       SCK_Ticks_Per_Second
*       SCK_Host_Name[]
*       SCK_Domain_Name[]
*       eQueue
*       EQ_Table
*       NU_EventsDispatcher_ptr
*       timer_task_ptr
*       NET_Initialized_Modules
*       NET_Unused_Parameter
*       Saved_hp
*
*   FUNCTIONS
*
*       NU_Init_Net
*       NET_Init_Net
*       NET_Init
*       NET_Demux
*
*   DEPENDENCIES
*
*       nu_net.h
*       ld_extr.h
*       nc6.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/ld_extr.h"

#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/nc6.h"
#endif

#if (INCLUDE_IPSEC == NU_TRUE)
#include "networking/ips_externs.h"
#endif

NU_SEMAPHORE      TCP_Resource;
NU_EVENT_GROUP    Buffers_Available;

/* This global will contain just what it is named, the number
   of Nucleus PLUS timer ticks in one second. This variable will
   be initialized by the NU_Init_Net function and the value
   should be supplied by PLUS. See TARGET.H for how this value
   is determined, look for TICKS_PER_SECOND. */
UINT32  SCK_Ticks_Per_Second;

/*  next_socket_no is used to record the last position searched in the
    socket list.  The next time the socket list is searched an unused socket
    should be found immediately.  The alternative is to begin searching from the
    start of the list every time.  Chances are a lot of used sockets would be
    found before finding an unused one.
*/
extern INT next_socket_no;

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
/* This is a pointer to the memory pool from which Nucleus NET will
   for all internal memory allocations, outside of the frame buffers. */
NU_MEMORY_POOL      *MEM_Cached;
#endif

/*  Holds the local host name */
UINT8 SCK_Host_Name[MAX_HOST_NAME_LENGTH];

/* Holds the local domain name */
UINT8 SCK_Domain_Name[NET_MAX_DOMAIN_NAME_LENGTH];

NU_PROTECT      SCK_Protect;

NU_QUEUE        eQueue;
NU_TASK         NU_EventsDispatcher_ptr;
NU_TASK         timer_task_ptr;
TQ_EVENT        NET_Resume_Event;

extern VOID     NU_EventsDispatcher(UNSIGNED argc, VOID *argv);

VOID            NET_Demux(UNSIGNED argc, VOID *argv);
STATIC STATUS   NET_Init_Net(const NU_NET_INIT_DATA *init_struct);

#if (INCLUDE_LOOPBACK_DEVICE == NU_TRUE)
    /* Declare and init the global to hold the name of the
       loopback device. */
    CHAR   loopback_device_name[] = "loopback";
#endif

/* Maintain an account of which modules are initialized. */
UINT32 NET_Initialized_Modules = 0;

/* The only purpose of this variable is to facilitate the removal of warnings. */
UNSIGNED    NET_Unused_Parameter;

STATIC STATUS   NET_Init(VOID);

NU_HOSTENT        *Saved_hp = NU_NULL;

#if (INCLUDE_IPV6 == NU_TRUE)
extern UINT8   IP6_Loopback_Address[];
#endif

#if ( (INCLUDE_DHCP == NU_TRUE) || (INCLUDE_DHCP6 == NU_TRUE) )
DHCP_DUID_STRUCT    DHCP_Duid;
#endif

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
/* Declare memory for Net Event queue */
CHAR NET_Event_Memory[NET_EVENT_MEMORY_SIZE];

/* Declare memory for Event Queue Dispatcher task */
CHAR NET_Event_Disp_Memory[EV_STACK_SIZE];

/* Declare memory for Net timer task */
CHAR NET_Timer_Memory[TM_STACK_SIZE];
#endif

/*************************************************************************
*
*   FUNCTION
*
*       NU_Init_Net
*
*   DESCRIPTION
*
*       This function initializes the network for Nucleus NET TCP/IP
*       operations.  It should only be called once.
*
*   INPUTS
*
*       *init_struct            Pointer to the structure containing
*                               initialization data.
*
*   OUTPUTS
*
*       NU_SUCCESS              Success
*       NU_INVALID_PARM         One of the input parameters is NULL.
*       NU_INVAL                A general-purpose error condition. This
*                               generally indicates that a required
*                               resource (task, semaphore, etc.) could not
*                               be created.
*       NU_MEM_ALLOC            The System_Memory pool has been exhausted.
*       NU_DHCP_INIT_FAILED     Failed to initialize the DHCP module.
*
*************************************************************************/
STATUS NU_Init_Net(const NU_NET_INIT_DATA *init_struct)
{
    STATUS      status;

    NU_SUPERV_USER_VARIABLES

    /* Validate the input parameter. */
    if ( (init_struct == NU_NULL) ||
         (init_struct->net_buffered_mem == NU_NULL) ||
         (init_struct->net_internal_mem == NU_NULL) )
        return (NU_INVALID_PARM);

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Create synchronization semaphore.  This semaphore is used internally
     * by the NET stack to protect global resources such as the Sockets list,
     * Port lists for all transport layer protocols, Routing Table, NET
     * buffers, etc ...
     */
    status = NU_Create_Semaphore(&TCP_Resource, "TCP", (UNSIGNED)1, NU_FIFO);

    /* If the semaphore could not be created, then processing cannot continue,
     * because the semaphore is critical to proper operation of the stack.
     */
    if (status == NU_SUCCESS)
    {
        /* Obtain the semaphore throughout the remaining initialization to
         * ensure that no task is kicked off before the corresponding globals
         * have been created and initialized.
         */
        status = NU_Obtain_Semaphore(&TCP_Resource, NU_NO_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Finish initialization.  This work was separated into another
             * function to reduce the number of calls to release the
             * semaphore if a critical event occurs.
             */
            status = NET_Init_Net(init_struct);

            /* Release the semaphore. */
            if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                               __FILE__, __LINE__);
        }

        else
            NLOG_Error_Log("Failed to obtain the TCP semaphore", NERR_FATAL,
                           __FILE__, __LINE__);
    }

    else
        NLOG_Error_Log("Failed to create the TCP semaphore", NERR_FATAL,
                       __FILE__, __LINE__);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* NU_Init_Net */

/*************************************************************************
*
*   FUNCTION
*
*       NET_Init_Net
*
*
*   DESCRIPTION
*
*       This function initializes the network for Nucleus NET TCP/IP
*       operations.  It should only be called once.
*
*   INPUTS
*
*       *init_struct            Pointer to the structure containing
*                               initialization data.
*
*   OUTPUTS
*
*       NU_SUCCESS              Success
*       NU_INVAL                A general-purpose error condition. This
*                               generally indicates that a required
*                               resource (task, semaphore, etc.) could not
*                               be created.
*       NU_MEM_ALLOC            The System_Memory pool has been exhausted.
*       NU_DHCP_INIT_FAILED     Failed to initialize the DHCP module.
*
*************************************************************************/
STATIC STATUS NET_Init_Net(const NU_NET_INIT_DATA *init_struct)
{
    STATUS                      status;

#if (INCLUDE_LOOPBACK_DEVICE == NU_TRUE)

    NU_DEVICE                   loopback_device;

#if (INCLUDE_IPV6 == NU_TRUE)
    DV_DEVICE_ENTRY             *dev_ptr;
    IP6_NEIGHBOR_CACHE_ENTRY    *nc_entry;
#endif

#endif

    VOID                        *pointer;

    /* Initialize the pointer to the non-cached memory pointer.  This pool is
     * used by the stack to create the NET buffers.
     */
    MEM_Non_Cached = init_struct->net_buffered_mem;

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    /* Initialize the pointer to the cached memory pointer.  This pool is used
     * by the stack to allocate memory for all other internal resources than
     * the NET buffers.
     */
    MEM_Cached = init_struct->net_internal_mem;
#endif

#if (NU_ENABLE_NOTIFICATION == NU_TRUE)

    /* Initialize the notification module.  This module is used by other tasks
     * in the stack, so initialize it before any tasks are created.
     */
    status = NET_NTFY_Init();

    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to initialize the notification module.",
                       NERR_FATAL, __FILE__, __LINE__);

        return (status);
    }
#endif

#if (INCLUDE_SOCKETS == NU_TRUE)
    /* Initialize the SCK_Sockets pointer-array. */
    UTL_Zero((CHAR *)SCK_Sockets, sizeof(SCK_Sockets));

    /* Initialize next_socket_no */
    next_socket_no = 0;
#endif

    /* Initialize the host name to zeros */
    UTL_Zero(SCK_Host_Name, MAX_HOST_NAME_LENGTH);

    /* Initialize the net_stack_name to the default name */
    memcpy(SCK_Host_Name, HOSTNAME, strlen(HOSTNAME));

    /* Initialize the domain name to zeros */
    UTL_Zero(SCK_Domain_Name, NET_MAX_DOMAIN_NAME_LENGTH);

    /* Initialize the net_stack_name to the default name */
    memcpy(SCK_Domain_Name, DOMAINNAME, strlen(DOMAINNAME));

    /* Initialized the number of Nucleus PLUS timer ticks in one second.
       This is used for various time related functions of Nucleus NET. */
    SCK_Ticks_Per_Second = NU_PLUS_Ticks_Per_Second;

    /* Create the Event queue.  This queue is used internally by the stack
     * to schedule an event to occur.  The Events Dispatcher receives the
     * events from this queue and processes them accordingly.
     */
#if (INCLUDE_STATIC_BUILD == NU_FALSE)

    status = NU_Allocate_Memory(MEM_Cached, &pointer,
                                (UNSIGNED)(SCK_EVENT_Q_NUM_ELEMENTS *
                                SCK_EVENT_Q_ELEMENT_SIZE * sizeof(UNSIGNED)),
                                (UNSIGNED)NU_NO_SUSPEND);

    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to allocate memory for the Event Queue",
                       NERR_FATAL, __FILE__, __LINE__);

        return (NU_MEM_ALLOC);
    }

#else

    /* Assign memory to event queue */
    pointer = (VOID *)NET_Event_Memory;

#endif

    pointer = TLS_Normalize_Ptr(pointer);

    status = NU_Create_Queue(&eQueue, "EvtQueue", pointer,
                             (UNSIGNED)SCK_EVENT_Q_NUM_ELEMENTS *
                             SCK_EVENT_Q_ELEMENT_SIZE,
                             NU_FIXED_SIZE,
                             (UNSIGNED)SCK_EVENT_Q_ELEMENT_SIZE, NU_FIFO);

    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to create the Event Queue", NERR_FATAL,
                       __FILE__, __LINE__);

        return (status);
    }

    /* The Buffers Available event is used by the link-layer to notify the
     * stack that a received buffer is ready for processing.  NET_Demux
     * suspends on this event until the link-layer sets the event indicating
     * there is a packet to process on the MEM_Buffer_List.
     */
    status = NU_Create_Event_Group(&Buffers_Available, "BUFAVA");

    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to create the Buffers Available Event Group",
                       NERR_FATAL, __FILE__, __LINE__);

        return (status);
    }

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    /* Create Event Queue Dispatcher task.  The Events Dispatcher handles
     * all events set internally by the stack.
     */
    status = NU_Allocate_Memory(MEM_Cached, &pointer, (UNSIGNED)EV_STACK_SIZE,
                                (UNSIGNED)NU_NO_SUSPEND);

    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to allocate memory for the Events Dispatcher",
                       NERR_FATAL, __FILE__, __LINE__);

        return (NU_MEM_ALLOC);
    }
#else
    /* Assign memory to the event queue dispatcher task */
    pointer = (VOID*)NET_Event_Disp_Memory;
#endif

    pointer = TLS_Normalize_Ptr(pointer);

    status = NU_Create_Task(&NU_EventsDispatcher_ptr, "EvntDisp",
                            NU_EventsDispatcher, (UNSIGNED)0, NU_NULL, pointer,
                            (UNSIGNED)EV_STACK_SIZE, EV_PRIORITY,
                            (UNSIGNED)EV_TIME_SLICE, EV_PREEMPT, NU_NO_START);

    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to create the Events Dispatcher task",
                       NERR_FATAL, __FILE__, __LINE__);

        return (status);
    }

    /* Start the task */
    if (NU_Resume_Task(&NU_EventsDispatcher_ptr) != NU_SUCCESS)
        NLOG_Error_Log("Failed to resume task", NERR_SEVERE,
                       __FILE__, __LINE__);

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    /* Create timer task.  This task handles the processing of all incoming
     * packets.
     */
    status = NU_Allocate_Memory(MEM_Cached, &pointer, (UNSIGNED)TM_STACK_SIZE,
                                (UNSIGNED)NU_NO_SUSPEND);

    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to allocate memory for the Timer Task",
                       NERR_FATAL, __FILE__, __LINE__);

        return (NU_MEM_ALLOC);
    }
#else

    /* Assign memory to the timer task */
    pointer = (VOID *)NET_Timer_Memory;
#endif

    pointer = TLS_Normalize_Ptr(pointer);

    status = NU_Create_Task(&timer_task_ptr, "TIMER", NET_Demux, (UNSIGNED)0,
                            NU_NULL, pointer, (UNSIGNED)TM_STACK_SIZE,
                            TM_PRIORITY, (UNSIGNED)TM_TIME_SLICE, TM_PREEMPT,
                            NU_NO_START);

    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to create the Timer Task", NERR_FATAL,
                       __FILE__, __LINE__);

        return (status);
    }

    /* Start the task */
    if (NU_Resume_Task(&timer_task_ptr) != NU_SUCCESS)
        NLOG_Error_Log("Failed to resume task", NERR_SEVERE,
                       __FILE__, __LINE__);

    /* Initialize the protocols of the NET stack. */
    status = NET_Init();

#if (INCLUDE_LOOPBACK_DEVICE == NU_TRUE)

    /* Did we successfully init the stack? */
    if (status == NU_SUCCESS)
    {
        /* Now that the stack is completely initialized setup the
           loopback device. */
        loopback_device.dv_name             = loopback_device_name;
        loopback_device.dv_flags            = DV_NOARP;
        loopback_device.dv_driver_options   = 0;
        loopback_device.dv_init             = LDC_Init;

        loopback_device.dv_hw.ether.dv_irq          = 0;
        loopback_device.dv_hw.ether.dv_io_addr      = 0;
        loopback_device.dv_hw.ether.dv_shared_addr  = 0;

#if (INCLUDE_IPV4 == NU_TRUE)
        *(UINT32 *)loopback_device.dv_gw    = 0x00000000;

        loopback_device.dv_ip_addr[0]       = 127;
        loopback_device.dv_ip_addr[1]       = 0;
        loopback_device.dv_ip_addr[2]       = 0;
        loopback_device.dv_ip_addr[3]       = 1;

        loopback_device.dv_subnet_mask[0]   = 255;
        loopback_device.dv_subnet_mask[1]   = 000;
        loopback_device.dv_subnet_mask[2]   = 000;
        loopback_device.dv_subnet_mask[3]   = 000;
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
        /* Zero out the IPv6 loopback address and put a 1 in the last
         * byte.
         */
        NU_BLOCK_COPY(loopback_device.dv6_ip_addr, IP6_Loopback_Address, IP6_ADDR_LEN);

        loopback_device.dv_flags |= DV6_IPV6;
#endif

        /* Add the loopback interface to the system. */
        status = DEV_Init_Devices(&loopback_device, 1);

        /* If the device was attached. */
        if (status == NU_SUCCESS)
        {
#if ( (INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_ARP == NU_TRUE) )

            /* Now add an ARP entry for this device. This will
               be a permanent entry since there is no need to ARP
               a loopback device. About the MAC address used: since
               what the MAC address is is not important just */
            ARP_Cache_Update(IP_ADDR (loopback_device.dv_ip_addr),
                             (UINT8 *)"\0\0\0\0\0\0", ARP_PERMANENT,
                             (INT32)DEV_Table.dv_head->dev_index);
#endif

#if (INCLUDE_IPV6 == NU_TRUE)

            /* If IPv6 is included, add a permanent entry to the Neighbor Cache for
             * the Loopback device.
             */
            dev_ptr = DEV_Get_Dev_By_Name(loopback_device.dv_name);

            if (dev_ptr)
            {
                nc_entry = dev_ptr->dev6_add_neighcache_entry(dev_ptr, IP6_Loopback_Address,
                                                              (UINT8*)"\0\0\0\0\0\0",
                                                              NC_PERMANENT, NU_NULL,
                                                              NC_NEIGH_REACHABLE);

                /* Add a route for the Loopback device. */
                if (RTAB6_Add_Route(dev_ptr, IP6_Loopback_Address,
                                    nc_entry->ip6_neigh_cache_ip_addr, 128,
                                    (RT_LOCAL | RT_STATIC | RT_UP)) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to add IPv6 route for loopback",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
            }
#endif
        }
    }

#endif

#if (INCLUDE_MDNS)
    /* Initialize the mDNS module. */
    MDNS_Initialize();
#endif

    return (status);

} /* NET_Init_Net */

/*************************************************************************
*
*   FUNCTION
*
*       NET_Init
*
*   DESCRIPTION
*
*       Handles all the initialization to bring up the network connection
*       Assumes that the configuration file has already been read up.
*
*       Returns 0 on successful initialization.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful
*       ret                     MEM_Init failed or PROT_Protocol_Init
*                               failed
*
*************************************************************************/
STATIC STATUS NET_Init(VOID)
{
    STATUS ret;

    /* Initialize/allocate Memory Buffers. */
    ret = MEM_Init();
    if (ret != NU_SUCCESS )
    {
        NLOG_Error_Log ("Error occurred while performing MEM_Init", NERR_RECOVERABLE,
                            __FILE__, __LINE__);

        return (ret);
    }

    /* Initialize the various protocol modules. */
    ret = PROT_Protocol_Init();
    if (ret != NU_SUCCESS )
    {
        NLOG_Error_Log ("Error occurred while performing PROT_Protocol_Init",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
        return (ret);
    }

    /* Register the event for the NET Timer */
    if (EQ_Register_Event(NET_Resume, &NET_Resume_Event) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to register event for NET Timer",
                        NERR_SEVERE, __FILE__, __LINE__);
    }

    return (NU_SUCCESS);

} /* NET_Init */

/*************************************************************************
*
*   FUNCTION
*
*       NET_Demux
*
*   DESCRIPTION
*
*       This function suspends on the Buffers_Available event waiting for
*       incoming data from the link-layer.  When the event is set, the
*       routine grabs the semaphore and passes the packet up to the next
*       layer.
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
VOID NET_Demux(UNSIGNED argc, VOID *argv)
{
    STATUS          status;
    UNSIGNED        bufs_ava;
    DV_DEVICE_ENTRY *device;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Get rid of compilation warnings. */
    UNUSED_PARAMETER(argc);
    UNUSED_PARAMETER(argv);

    for (;;)
    {
        /* Grab the stack semaphore before processing packets. */
        status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

        /* Verify that resource was available */
        if (status == NU_SUCCESS)
        {
            /* Process all of the packets which came in */
            while (MEM_Buffer_List.head)
            {
#if (INCLUDE_IPSEC == NU_TRUE)
                /* Ensure the receive count is reset. This count indicates
                 * the number of SA's applied to the current packet being
                 * processed.
                 */
                IPSEC_SA_Count = 0;
#endif

                /* Point to the device on which this packet was received. */
                device = MEM_Buffer_List.head->mem_buf_device;

                /* Call the receive function for that device. */
                (*(device->dev_input))();
            }

            /* Let other tasks use the stack. */
            if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                               __FILE__, __LINE__);

                NET_DBG_Notify(NU_INVALID_SEMAPHORE, __FILE__, __LINE__,
                               NU_Current_Task_Pointer(), NU_NULL);
            }
        }

        else
        {
            NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);

            NET_DBG_Notify(status, __FILE__, __LINE__,
                           NU_Current_Task_Pointer(), NU_NULL);
        }

        /* Wait for a device to notify the stack that a packet
           is ready for processing. */
        NU_Retrieve_Events(&Buffers_Available, 2, NU_OR_CONSUME, &bufs_ava,
                           NU_SUSPEND);

    }   /* end while for task */

    /* Switch back to user mode. */

    /* Note this instruction is commented out to remove a compiler
       warning. The while loop above should never be exited and this
       instruction never executed. Thus the reason for the compiler
       warning and the justification to comment out this instruction.

       This line is left in just for completeness and so that in the
       future it is not overlooked.

    NU_USER_MODE();

    */

}  /* NET_Demux */
