/*************************************************************************
*
*               Copyright 1997 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************
**************************************************************************
*
*   FILE NAME
*
*       ppp.c
*
*   COMPONENT
*
*       PPP - Core component of PPP
*
*   DESCRIPTION
*
*       This file contains the main PPP services and controls
*       negotiation of a PPP link with a peer.
*
*   DATA STRUCTURES
*
*       PPP_Null_Ip
*       PPP_Event
*       PPP_Memory
*
*   FUNCTIONS
*
*       PPP_Initialize
*       PPP_Task_Entry
*       PPP_Event_Forwarding
*       PPP_Lower_Layer_Up
*       NU_Set_PPP_Login
*       PPP_Set_Login
*       PPP_Hangup
*       PPP_Still_Connected
*       PPP_Two_To_Power
*       PPP_Output
*       PPP_Input
*       PPP_Dial
*       PPP_Wait_For_Client
*       PPP_Abort_Wait_For_Client
*       PPP_Abort
*       PPP_Abort_Connection
*       PPP_Set_Link_Options
*       PPP_Get_Link_Options
*       PPP_Event_Handler
*       PPP_Obtain_Connection_Status
*       PPP_Negotiation_Timeout
*       PPP_Last_Activity_Time
*       PPP_Reset
*       PPP_Attach_IP_Address
*       PPP_Attach_DNS_Address
*       PPP_Protocol_Reject
*       PPP_Add_Protocol
*       PPP_PrintPkt
*       PPP_Ioctl
*
*   DEPENDENCIES
*
*       nu_ppp.h
*
*************************************************************************/
#include "drivers/ppp.h"
#include "drivers/ppe_extr.h"

#if (PPP_DEBUG_PRINT_OK == NU_TRUE)
#define PrintInfo(s)       PPP_Printf(s)
#define PrintErr(s)        PPP_Printf(s)
#else
#define PrintInfo(s)
#define PrintErr(s)
#endif

UINT8       PPP_Null_Ip[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
TQ_EVENT    PPP_Event;

NU_MEMORY_POOL *PPP_Memory = NU_NULL;

/***********************************************************************
*
* FUNCTION
*
*       nu_os_drvr_ppp_init
*
* DESCRIPTION
*
*       This function initializes ppp driver
*
* INPUTS
*
*       CHAR    *key              - Path to registry
*       INT     startstop         - Option to Register or Unregister
*
* OUTPUTS
*
*       None
*
***********************************************************************/
VOID nu_os_drvr_ppp_init (const CHAR * key, INT startstop)
{
    STATUS              status;
    NU_DEVICE           PPP_Devices[2];
    INT                 device_count=0;
#if CFG_NU_OS_DRVR_PPP_ENABLE_SERIAL
    DV_DEV_LABEL        *ser_reg_label;
#endif
#if CFG_NU_OS_DRVR_PPP_ENABLE_ETHERNET
    DV_DEV_LABEL        *eth_reg_label;
#endif

#if CFG_NU_OS_DRVR_PPP_ENABLE_SERIAL
      /* Initialize device structure for PPP driver */
    PPP_Devices[device_count].dv_name               = CFG_NU_OS_DRVR_PPP_DEV_NAME_PPP;

#if CFG_NU_OS_DRVR_PPP_DC_PROTOCOL_ENABLE
    PPP_Devices[device_count].dv_init               = PPP_DC_Initialize;
#else
    PPP_Devices[device_count].dv_init               = HDLC_Initialize;
#endif

    status = NU_Allocate_Memory(&System_Memory, (VOID *)&ser_reg_label, sizeof(DV_DEV_LABEL), NU_NO_SUSPEND);
    if(status == NU_SUCCESS)
    {
        status = REG_Get_Bytes_Value(key, "/ser_device", (UINT8 *)(ser_reg_label), sizeof(DV_DEV_LABEL));
        if(status == NU_SUCCESS)
        {
            PPP_Devices[device_count].dv_driver_options = (UINT32)(ser_reg_label);
        }
        else
        {
            NLOG_Error_Log("Unable to get the label of serial device from the registry\n", NERR_FATAL, __FILE__, __LINE__);
        }
    }
    else
    {
        NLOG_Error_Log("Failed to allocate memory for serial device registry label\n", NERR_FATAL, __FILE__, __LINE__);
    }

    PPP_Devices[device_count].dv_flags = (DV_POINTTOPOINT | DV_NOARP);

#ifdef CFG_NU_OS_NET_IPV6_ENABLE
#if CFG_NU_OS_DRVR_PPP_SER_ENABLE_IPV6
    /* If this device will be transmitting IPv6, enable its flag. */
    PPP_Devices[device_count].dv_flags |= DV6_IPV6;
#endif
#endif

    device_count++;
#endif  /* CFG_NU_OS_DRVR_PPP_ENABLE_SERIAL */

#if CFG_NU_OS_DRVR_PPP_ENABLE_ETHERNET
    /* Initialize the virtual PPPoE device. */
    PPP_Devices[device_count].dv_name = CFG_NU_OS_DRVR_PPP_DEV_NAME_PPPOE;

    PPP_Devices[device_count].dv_init = PPE_Initialize;

    /* Set this option to the name of the hardware device that this virtual
       device will be bound to. */
    status = NU_Allocate_Memory(&System_Memory, (VOID *)&eth_reg_label, sizeof(DV_DEV_LABEL), NU_NO_SUSPEND);
    if(status == NU_SUCCESS)
    {
        status = REG_Get_Bytes_Value(key, "/eth_device", (UINT8 *)(eth_reg_label), sizeof(DV_DEV_LABEL));
        if(status == NU_SUCCESS)
        {
            PPP_Devices[device_count].dv_driver_options = (UINT32)(eth_reg_label);
        }
        else
        {
            NLOG_Error_Log("Unable to get the label of serial device from the registry\n", NERR_FATAL, __FILE__, __LINE__);
        }
    }
    else
    {
        NLOG_Error_Log("Failed to allocate memory for serial device registry label\n", NERR_FATAL, __FILE__, __LINE__);
    }

    PPP_Devices[device_count].dv_flags = (DV_POINTTOPOINT | DV_NOARP);

#ifdef CFG_NU_OS_NET_IPV6_ENABLE
#if CFG_NU_OS_DRVR_PPP_ETH_ENABLE_IPV6
    /* If this device will be transmitting IPv6, enable its flag. */
    PPP_Devices[device_count].dv_flags |= DV6_IPV6;
#endif
#endif

    device_count++;
#endif  /* CFG_NU_OS_DRVR_PPP_ENABLE_ETHERNET */

    /* Register the device. */
    status = NU_Init_Devices(PPP_Devices, device_count);
    if(status != NU_SUCCESS)
    {
        NLOG_Error_Log("NU_Init_Devices failed in ppp init function\n", NERR_FATAL, __FILE__, __LINE__);
    }
}  /* nu_os_drvr_ppp_init */

/*************************************************************************
* FUNCTION
*
*     PPP_Initialize
*
* DESCRIPTION
*
*     This function initializes the LCP and NCP layers of the protocol
*     stack. After exiting from this function PPP will be in a state
*     to begin link negotiation.
*
* INPUTS
*
*     DV_DEVICE_ENTRY   *dev_ptr        Pointer to the device structure
*                                        for this device.
*
* OUTPUTS
*
*     STATUS                            Returns NU_SUCCESS if
*                                        initialization is successful,
*                                        else a value less than 0 is
*                                        returned.
*
*************************************************************************/
STATUS PPP_Initialize(DV_DEVICE_ENTRY *dev_ptr)
{
    STATUS          status;
    LINK_LAYER      *ppp_layer_ptr;
    void            *ptr;
#if (PPP_LOG_TO_FILE == NU_TRUE)
    FILE *fp;
#endif

    dev_ptr->dev_ioctl = PPP_Ioctl;

    ppp_layer_ptr = (LINK_LAYER*)(dev_ptr->dev_link_layer);

    /* Set the MTU if it hasn't been set yet, or it is set to
       a higher value. */
    if (dev_ptr->dev_mtu == 0 || dev_ptr->dev_mtu > PPP_MTU)
        dev_ptr->dev_mtu = PPP_MTU;

    dev_ptr->dev_type = DVT_PPP;

    /* Each link protocol adds a header size to a total for Net to use. */
    dev_ptr->dev_hdrlen += PPP_PROTOCOL_HEADER_2BYTES;

    /* Set the default timeout for link negotiation */
    ppp_layer_ptr->negotiation_timeout = (UINT16)PPP_DEFAULT_NEG_TIMEOUT;

    /* Allocate a block of memory for the PPP task stack and queue. */
    status = NU_Allocate_Memory(PPP_Memory, &(ppp_layer_ptr->ppp_init_mem),
                                PPP_TASK_STACK_SIZE +
                                (PPP_EVENT_MESSAGE_SIZE *
                                PPP_EVENT_QUEUE_SIZE *
                                sizeof(UNSIGNED)), NU_NO_SUSPEND);

    if(status != NU_SUCCESS)
    {
        return (status);
    }

    /* Create the event queue. */
    status = NU_Create_Queue(&(ppp_layer_ptr->ppp_event_queue), "PPP_Q", (ppp_layer_ptr->ppp_init_mem),
                             (PPP_EVENT_MESSAGE_SIZE *
                             PPP_EVENT_QUEUE_SIZE),
                             NU_FIXED_SIZE, PPP_EVENT_MESSAGE_SIZE,
                             NU_FIFO);

    if(status != NU_SUCCESS)
    {
        return (status);
    }

    /* Set pointer to stack memory allocated
       after the queue memory. */
    ptr = (VOID *)(((UINT8 *)(ppp_layer_ptr->ppp_init_mem)) + (PPP_EVENT_MESSAGE_SIZE *
                                             PPP_EVENT_QUEUE_SIZE *
                                             sizeof(UNSIGNED)));

    /* Create PPP task which will handle all PPP events. */
    status = NU_Create_Task(&(ppp_layer_ptr->ppp_task_cb), "PPP_EVT", PPP_Task_Entry, 0,
                            dev_ptr, ptr, PPP_TASK_STACK_SIZE,
                            PPP_TASK_PRIORITY, PPP_TASK_TIME_SLICE,
                            PPP_TASK_PREEMPT, NU_NO_START);

    if(status != NU_SUCCESS)
    {
        return (status);
    }
#if (HDLC_POLLED_TX == NU_FALSE)

    status = NU_Create_Event_Group(&(ppp_layer_ptr->ppp_tx_event),"PPP_Tx_EVENT");

    if(status != NU_SUCCESS)
    {
        return (status);
    }
#endif

    /* Start the PPP task. */
    status = NU_Resume_Task(&(ppp_layer_ptr->ppp_task_cb));

    if(status != NU_SUCCESS)
    {
        return (status);
    }

    /* Register the main PPP event handler with the dispatcher. */
    status = EQ_Register_Event(PPP_Event_Forwarding, &PPP_Event);
    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to register the PPP event.", NERR_FATAL, __FILE__, __LINE__);

        return(NU_PPP_INIT_FAILURE);
    }

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)
    /* Enable multicasting on the device. */
    dev_ptr->dev_flags |= DV_MULTICAST;
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
    if (dev_ptr->dev_flags & DV6_IPV6)
        /* Disable Duplicate Address Detection. */
        dev_ptr->dev_flags |= DV6_NODAD;
#endif

    /* Initialize the LCP layer. */
    if (LCP_Init(dev_ptr) != NU_SUCCESS)
        return (NU_PPP_INIT_FAILURE);

    /* Initialize the authentication structures. */
    if (AUTH_Init(dev_ptr) != NU_SUCCESS)
        return (NU_PPP_INIT_FAILURE);

#if (PPP_ENABLE_MPPE == NU_TRUE)
    /* Initialize the CCP layer. */
    if (CCP_Init(dev_ptr) != NU_SUCCESS)
        return (NU_PPP_INIT_FAILURE);
#endif

    /* Initialize the NCP layer. */
    if (NCP_Init(dev_ptr) != NU_SUCCESS)
        return (NU_PPP_INIT_FAILURE);

    status = NU_Create_Event_Group(&ppp_layer_ptr->negotiation_progression, dev_ptr->dev_net_if_name);
    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to create PPP negotiation event group.", NERR_FATAL, __FILE__, __LINE__);

        return(NU_PPP_INIT_FAILURE);
    }

#if (PPP_LOG_TO_FILE == NU_TRUE)
    fp = fopen(PPP_LOG_FILENAME, "w");
    if (fp == NULL)
        PPP_Printf("Failed to create a PPP log file.\n");
    else
    {
        fprintf(fp, "Nucleus PPP connection log:\n===========================\n\n");
        fclose(fp);
    }
#endif

#if (INCLUDE_PPP_MP == NU_TRUE)
    if (PPP_MP_Initialize() != NU_SUCCESS)
    {
        return(NU_PPP_INIT_FAILURE);
    }
#endif

    return (NU_SUCCESS);

} /* PPP_Initialize */

/*************************************************************************
* FUNCTION
*
*     PPP_Task_Entry
*
* DESCRIPTION
*
*     This function is the entry point of the PPP task. The
*     PPP task is responsible for dispatching all PPP events
*     to their corresponding handlers. All events generated
*     by the NET event component are forwarded to this task
*     and are processed in it's context, to reduce load on
*     the NET event handler task.
*
* INPUTS
*
*     argc                      Argument count. This is an
*                               unused parameter.
*     *argv                     Vector containing pointers
*                               to task arguments. 
*
* OUTPUTS
*
*     None
*
*************************************************************************/
VOID PPP_Task_Entry(UNSIGNED argc, VOID *argv)
{
    STATUS          status;
    UNSIGNED        actual_size;
    UNSIGNED        event_msg[PPP_EVENT_MESSAGE_SIZE];
    TQ_EVENT        event;
    DV_DEVICE_ENTRY *dev_ptr = argv;
    LINK_LAYER      *ppp_layer_ptr;

    NU_SUPERV_USER_VARIABLES;

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    ppp_layer_ptr = (LINK_LAYER*)(dev_ptr->dev_link_layer);

    for(;;)
    {
        /* Wait for and receive a message from the event queue. */
        status = NU_Receive_From_Queue(&(ppp_layer_ptr->ppp_event_queue), event_msg,
                                       PPP_EVENT_MESSAGE_SIZE,
                                       &actual_size, NU_SUSPEND);

        if(status != NU_SUCCESS)
        {
            /* Log the failure. */
            NLOG_Error_Log("Failed to receive PPP event from queue.",
                           NERR_FATAL, __FILE__, __LINE__);
        }
        else
        {
            /* Get the event type from the message. */
            event = event_msg[PPP_MSG_EVENT_INDEX];

            /* Grab the semaphore. */
            if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to obtain TCP semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);

                /* Discard this event and move to the next one. */
                continue;
            }
            
            /* Determine the event type. */
#if (INCLUDE_IPV4 == NU_TRUE)
            if(event == IPCP_Event)
            {
                /* Call the NCP event handler. */
                NCP_Event_Handler(event, (UNSIGNED)dev_ptr,
                                  event_msg[PPP_MSG_EXTDAT_INDEX]);
            }
            else
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
            if(event == IPV6CP_Event)
            {
                /* Call the NCP event handler. */
                NCP_Event_Handler(event, (UNSIGNED)dev_ptr,
                                  event_msg[PPP_MSG_EXTDAT_INDEX]);
            }
            else
#endif
#if (PPP_ENABLE_MPPE == NU_TRUE)
            if(event == PPP_CCP_Event)
            {
                /* Call the PPP event handler. */
                CCP_Event_Handler(event, (UNSIGNED)dev_ptr,
                                  event_msg[PPP_MSG_EXTDAT_INDEX]);
            }
            else
#endif
            if(event == PPP_Event)
            {
                /* Call the PPP event handler. */
                PPP_Event_Handler(event, (UNSIGNED)dev_ptr,
                    event_msg[PPP_MSG_EXTDAT_INDEX]);
            }
            else
            {
                /* Log message for invalid event type. */
                NLOG_Error_Log("Invalid event type in event message.",
                               NERR_SEVERE, __FILE__, __LINE__);
            }

            /* Release the semaphore. */
            if(NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release TCP semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }        
    }


    /* Note this instruction is commented out to remove a compiler
       warning. The while loop above should never be exited and this
       instruction never executed.

    NU_USER_MODE();

    */

} /* PPP_Task_Entry */

/*************************************************************************
* FUNCTION
*
*     PPP_Event_Forwarding
*
* DESCRIPTION
*
*     This function forwards all events to the PPP event
*     task. The task dispatches these events to their
*     corresponding event handlers.
*
* INPUTS
*
*     event                     Event identifier.
*     dat                       First event parameter.
*     extdat                    Second event parameter.
*
* OUTPUTS
*
*     None
*
*************************************************************************/
VOID PPP_Event_Forwarding(TQ_EVENT event, UNSIGNED dat, UNSIGNED extdat)
{
    DV_DEVICE_ENTRY *dev_ptr = (DV_DEVICE_ENTRY *)dat;
    UNSIGNED        event_msg[PPP_EVENT_MESSAGE_SIZE];
    LINK_LAYER      *ppp_layer_ptr;
    ppp_layer_ptr = (LINK_LAYER*)(dev_ptr->dev_link_layer);

    /* Create the event message. The device pointer (dat) is
       converted to the device index to make sure the pointer
       is not invalidated while it is in the event queue. It
       is converted back to the device pointer by the PPP event
       task. */
    event_msg[PPP_MSG_EVENT_INDEX]  = (UNSIGNED)event;
    event_msg[PPP_MSG_DAT_INDEX]    = (UNSIGNED)dev_ptr->dev_index;
    event_msg[PPP_MSG_EXTDAT_INDEX] = (UNSIGNED)extdat;

    /* Send message to the event queue. */
    if(NU_Send_To_Queue(&(ppp_layer_ptr->ppp_event_queue), event_msg,
                        PPP_EVENT_MESSAGE_SIZE,
                        NU_NO_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to send message to PPP event queue",
                       NERR_SEVERE, __FILE__, __LINE__);
    }

} /* PPP_Event_Forwarding */

/*************************************************************************
* FUNCTION
*
*     PPP_Lower_Layer_Up
*
* DESCRIPTION
*
*     This function is responsible for controlling the movement of PPP
*     through each phase of the link negotiation.
*
* INPUTS
*
*     UINT8             *ip_address4     Pointer to the IPv4 address to be
*                                        used during IPCP negotiation.
*                                        This address will either be the
*                                        address to assign to a calling
*                                        client or will be the address
*                                        to suggest to a PPP server
*                                        depending on if the local side
*                                        is functioning as a PPP server
*                                        or a PPP client respectively.
*
*     UINT8             *ip_address6     Pointer to the IPv6 address to be
*                                        used during IPCP negotiation.
*                                        This address will either be the
*                                        address to assign to a calling
*                                        client or will be the address
*                                        to suggest to a PPP server
*                                        depending on if the local side
*                                        is functioning as a PPP server
*                                        or a PPP client respectively.
*     DV_DEVICE_ENTRY   *dev_ptr         Pointer to device structure.
*
* OUTPUTS
*
*     STATUS                             How the link negotiation ended
*
*************************************************************************/
STATUS PPP_Lower_Layer_Up(UINT8 *ip_address4, UINT8 *ip_address6,
                          DV_DEVICE_ENTRY *dev_ptr)
{
    STATUS                  status;
    UNSIGNED                bufs_ava;
    LINK_LAYER              *link_layer;
    UINT32                  index_save;

#if (INCLUDE_IPV6 == NU_FALSE)
    UNUSED_PARAMETER(ip_address6);
#endif

    /* Grab a pointer to the ppp link layer structure. */
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

#if (INCLUDE_IPV4 == NU_TRUE)
    if(ip_address4)
        /* Write the local address to the NCP structures. */
        memcpy(link_layer->ncp.options.ipcp.local_address, ip_address4, IPCP_ADDR_LEN);
    else
        memset(link_layer->ncp.options.ipcp.local_address, 0, IPCP_ADDR_LEN);
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
    if(ip_address6)
       memcpy(link_layer->ncp6.options.ipv6cp.local_address, ip_address6, IPV6CP_ADDR_LEN );
    else
       memset(link_layer->ncp6.options.ipv6cp.local_address, 0, IPV6CP_ADDR_LEN);
#endif

    /* Eat any events that may be around from the last session. */
    NU_Retrieve_Events(&link_layer->negotiation_progression,
                      (PPP_LCP_FAIL | PPP_CCP_FAIL | PPP_AUTH_FAIL | PPP_NCP_FAIL |
                      PPP_CONFIG_SUCCESS | PPP_NEG_ABORT | PPP_NEG_TIMEOUT),
                      NU_OR_CONSUME, &bufs_ava, NU_NO_SUSPEND);


    /* Set the status of the PPP connection attempt. */
    link_layer->connection_status = NU_PPP_LINK_NEGOTIATION;

    /* Set the LCP open event to start link negotiation. */
    EQ_Put_Event(PPP_Event, (UNSIGNED)dev_ptr, LCP_OPEN_REQUEST);

    /* Set the total negotiation timeout event. If this event goes off
       before the link is up we will abort the attempt. */
    TQ_Timerset(PPP_Event, (UNSIGNED)dev_ptr, link_layer->negotiation_timeout, PPP_STOP_NEGOTIATION);

    /* Save the index before releasing the semaphore. */
    index_save = dev_ptr->dev_index;

    /* Release the semaphore. */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                   __FILE__, __LINE__);
    }

    /* We wait for the event to tell us that the link is 1) up, 2) aborted,
       or 3) timed out. */
    status = NU_Retrieve_Events(&link_layer->negotiation_progression,
                                (PPP_LCP_FAIL | PPP_CCP_FAIL |PPP_AUTH_FAIL | PPP_NCP_FAIL |
                                PPP_CONFIG_SUCCESS | PPP_NEG_ABORT | PPP_NEG_TIMEOUT),
                                NU_OR_CONSUME, &bufs_ava, NU_SUSPEND);
    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to retrieve the PPP negotiation event.", NERR_FATAL, __FILE__, __LINE__);
        return(NU_PPP_INIT_FAILURE);
    }

    /* Grab the semaphore again. */
    if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
           __FILE__, __LINE__);
    }

    /* Check if the device has not been removed. */
    if(DEV_Get_Dev_By_Index(index_save) == NU_NULL)
    {
        /* Device has been removed, so return from this function. */
        return NU_INVALID_LINK;
    }

    /* At this point, all negotiations (LCP, PAP or CHAP, and NCP) have
       completed. Now find out what the outcome is and return a status
       to the application. */

    /* If negotiation was successful, then the NCP layer is up and the
       device has an IP address. Ready to go. */
    if (bufs_ava == PPP_CONFIG_SUCCESS)
    {
        /* We didn't timeout so clear the negotiation timer event. */
        TQ_Timerunset(PPP_Event, TQ_CLEAR_EXACT, (UNSIGNED)dev_ptr, PPP_STOP_NEGOTIATION);

        /* Set the status of the PPP connection attempt. */
        link_layer->connection_status = NU_PPP_CONNECTED;
        status = NU_SUCCESS;
    }

    else
    {
        if (bufs_ava & PPP_NEG_TIMEOUT)
        {
            PrintInfo("PPP Negotiation timed out!\n");
            status = NU_NEGOTIATION_TIMEOUT;
        }

        /* Aborted by the application? */
        else if (bufs_ava & PPP_NEG_ABORT)
        {
            PrintInfo("PPP Negotiation aborted!\n");
            status = NU_NEGOTIATION_ABORTED;
        }
        else
        {
            /* One of the protocols failed to negotiate. */
            if (bufs_ava & PPP_LCP_FAIL)
            {
                PrintErr("LCP Negotiation failed!\n");
                status = NU_LCP_FAILED;
            }
            else if (bufs_ava & PPP_AUTH_FAIL)
            {
                PrintErr("Authentication failed!\n");
                status = NU_AUTH_FAILED;
            }
            else if (bufs_ava & PPP_NCP_FAIL)
            {
                PrintErr("NCP Negotiation failed!\n");
                status = NU_NCP_FAILED;
            }
            else if (bufs_ava & PPP_CCP_FAIL)
            {
                PrintErr("CCP Negotiation failed!\n");
                status = NU_CCP_FAILED;
            }
        }

        /* Stop any PPP timer that may still be running. */
        TQ_Timerunset(PPP_Event, TQ_CLEAR_ALL_EXTRA, (UNSIGNED)dev_ptr, 0);
    }

    return status;

} /* PPP_Lower_Layer_Up */

/*************************************************************************
* FUNCTION
*
*     NU_Set_PPP_Login
*
* DESCRIPTION
*
*     This function calls the function which Sets the ID and password to
*     be used when dialing up a PPP server.
*
* INPUTS
*
*     CHAR              id[]            ID to be used
*     CHAR              pw[]            Password to be used
*     CHAR              *link_name      Pointer to the PPP link name.
*
* OUTPUTS
*
*     STATUS
*
*************************************************************************/
STATUS NU_Set_PPP_Login(CHAR id[PPP_MAX_ID_LENGTH],
                        CHAR pw[PPP_MAX_PW_LENGTH], CHAR *link_name)
{
    DV_DEVICE_ENTRY         *dev_ptr;
    STATUS                  status = NU_INVALID_LINK;
    NU_SUPERV_USER_VARIABLES;

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Grab the semaphore. */
    if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Find the device. */
    dev_ptr = DEV_Get_Dev_By_Name(link_name);

    /* Make sure the device was found and that it is a PPP device. */
    if ((dev_ptr) && (dev_ptr->dev_type == DVT_PPP))
        status = PPP_Set_Login(id, pw, dev_ptr);

    /* Release the semaphore. */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Return from function in user mode. */
    NU_USER_MODE();
    return status;

} /* NU_Set_PPP_Login */

/*************************************************************************
* FUNCTION
*
*     PPP_Set_Login
*
* DESCRIPTION
*
*     Sets the ID and password to be used when dialing up a PPP server.
*
* INPUTS
*
*     CHAR              id[]            ID to be used
*     CHAR              pw[]            Password to be used
*     CHAR              *dev_ptr        Pointer to the PPP device.
*
* OUTPUTS
*
*     STATUS
*
*************************************************************************/
STATUS PPP_Set_Login(CHAR id[PPP_MAX_ID_LENGTH],
                     CHAR pw[PPP_MAX_PW_LENGTH],
                     DV_DEVICE_ENTRY *dev_ptr)
{
    STATUS                  status;
    AUTHENTICATION_LAYER    *auth;
    LINK_LAYER              *link_layer;

    /* Get a pointer to the authentication layer structure. */
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
    auth = &link_layer->authentication;

    status = NU_SUCCESS;

#if (INCLUDE_SEC_MIB == NU_TRUE)
    /* add entry to the MIB database */
    status = PMSS_SetClientLogin(dev_ptr->dev_index, id, pw);
    if (status != NU_SUCCESS)
    {
        /* unable to find entry - just log message and keep PPP alive */
        NLOG_Error_Log("PMSS_SetClientLogin failed.", NERR_SEVERE, __FILE__, __LINE__);
    }
#endif

    /* Simply copy the ID and PW pair into our internal variables. */
    auth->name_len = (UINT8)strlen(id);
    strcpy(auth->login_name, id);
    auth->pw_len = (UINT8)strlen(pw);
    strcpy(auth->login_pw, pw);

    return status;

} /* PPP_Set_Login */



/*************************************************************************
* FUNCTION
*
*     PPP_Hangup
*
* DESCRIPTION
*
*     Starts the link termination phase and hangs up the physical device.
*
* INPUTS
*
*     UINT8             mode            Should the link be closed even
*                                        if it is active.
*     CHAR              *link_name      Pointer to the PPP link name.
*
* OUTPUTS
*
*     STATUS                            Was the link closed or not
*
*************************************************************************/
STATUS PPP_Hangup(UINT8 mode, CHAR *link_name)
{
    STATUS          status = NU_TRUE;
    DV_DEVICE_ENTRY *dev_ptr;
    LINK_LAYER      *link_layer = NU_NULL;
    UINT32          index_save;
    NU_SUPERV_USER_VARIABLES;

    /* switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    PrintInfo("PPP_Hangup\n");

    /* Grab the semaphore. */
    if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Find the device that we want to hangup. */
    dev_ptr = DEV_Get_Dev_By_Name(link_name);

    /* Make sure the device is found */
    if(dev_ptr)
    {
        /* Grab a pointer to the LCP layer structure. */
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

#if(INCLUDE_PPP_MP == NU_TRUE)

        /* Check if this device has Multilink Protocol on */
        if(link_layer->lcp.options.remote.flags & PPP_FLAG_MRRU)
        {
            /* Remove this link from the bundle */
            PPP_MP_Remove_Link(dev_ptr->dev_index);
        }
#endif
    }

    /* Make sure the device was found. */
    if ((dev_ptr) && (dev_ptr->dev_type == DVT_PPP))
    {
        /* If the modem has already disconnected, return true. */
        if (link_layer->hwi.state == INITIAL)
        {
            /* Release the semaphore. */
            if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                               __FILE__, __LINE__);
            }

            /* Return from function in user mode. */
            NU_USER_MODE();

            return status;
        }

        /* If the caller wants to force it, then we will close the whole network
           without warning. */
        if (mode & PPP_FORCE)
        {
            link_layer->hwi.state = STOPPING;
            link_layer->hwi.disconnect(link_layer, NU_FALSE);
            link_layer->lcp.state = STOPPING;

            /* Set the LCP close event to start link termination. */
            EQ_Put_Event(PPP_Event, (UNSIGNED)dev_ptr, LCP_CLOSE_REQUEST);
            status = NU_FALSE;
        }
        else

        /* If it is ok to hangup then we will start by terminating the link
           and we will hangup the modem. */
        if (mode & PPP_NO_FORCE)
        {
#if (INCLUDE_IPV4 == NU_TRUE)
            if(!(link_layer->ncp.options.ipcp.flags & PPP_FLAG_NO_IPV4))
            {
                if (link_layer->ncp.state != CLOSED && link_layer->ncp.state != STOPPED)
                {
                    link_layer->ncp.state = CLOSING;

                    /* Set the NCP close event to start link termination. */
                    EQ_Put_Event(IPCP_Event, (UNSIGNED)dev_ptr, NCP_CLOSE_REQUEST);
                }
            }
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
            if (dev_ptr->dev_flags & DV6_IPV6)
            {
                if (link_layer->ncp6.state != CLOSED && link_layer->ncp6.state != STOPPED)
                {
                    link_layer->ncp6.state = CLOSING;
                    EQ_Put_Event(IPV6CP_Event, (UNSIGNED)dev_ptr, NCP_CLOSE_REQUEST);
                }
            }
#endif
            status = NU_FALSE;
        }

        /* Optionally suspend here until the modem is disconnected. */
        if (mode & PPP_BLOCK)
        {
#if (HDLC_POLLED_TX == NU_FALSE)
            /* Check the transq. If there is anything on we will wait until
               it has all been sent before hanging up the modem. */
            while (dev_ptr->dev_transq.head != NULL
                    || link_layer->hwi.state != INITIAL)
#else
            while (link_layer->hwi.state != INITIAL)
#endif
            {
                /* Save the index before releasing the semaphore. */
                index_save = dev_ptr->dev_index;

                /* Release the semaphore. */
                if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                               __FILE__, __LINE__);
                }

                /* Wait before retrying. */
                NU_Sleep(TICKS_PER_SECOND >> 2);

                /* Grab the semaphore again. */
                if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
                }

                /* Check if the device has not been removed. */
                if(DEV_Get_Dev_By_Index(index_save) == NU_NULL)
                {
                    /* Release the semaphore. */
                    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                                   __FILE__, __LINE__);
                    }

                    /* Device has been removed, so return from this
                    function. */
                    return NU_INVALID_LINK;
                }

            }

            status = NU_TRUE;
        }

    } /* if a device was found */
    else
        status = NU_INVALID_LINK;

    /* Release the semaphore. */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Return from function in user mode. */
    NU_USER_MODE();
    return status;

} /* PPP_Hangup */



/*************************************************************************
* FUNCTION
*
*     PPP_Still_Connected
*
* DESCRIPTION
*
*     Checks the state of the PPP link and returns TRUE if it is still
*     open. FALSE if it is not open.
*
* INPUTS
*
*     CHAR              *link_name      Pointer to the PPP link name.
*
* OUTPUTS
*
*     STATUS                            Is the link open or not
*
*************************************************************************/
STATUS PPP_Still_Connected(CHAR *link_name)
{
    STATUS          status;
    DV_DEVICE_ENTRY *dev_ptr;
    LINK_LAYER      *link_layer;
    NU_SUPERV_USER_VARIABLES;

    /* switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Grab the semaphore. */
    if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Find the device that we want to hangup. */
    dev_ptr = DEV_Get_Dev_By_Name(link_name);

    /* Make sure a device was found and that it is a PPP device. */
    if ((dev_ptr) && (dev_ptr->dev_type == DVT_PPP))
    {
        /* Grab a pointer to the LCP layer structure. */
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

        if (link_layer->hwi.state != INITIAL)
            status = NU_TRUE;
        else
            status = NU_FALSE;
    }
    else
        status = NU_INVALID_LINK;

    /* Release the semaphore. */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Return from function in user mode. */
    NU_USER_MODE();
    return status;

} /* PPP_Still_Connected */



/*************************************************************************
* FUNCTION
*
*     PPP_Two_To_Power
*
* DESCRIPTION
*
*     Raises two to the power of the passed in variable. This function
*     is used instead of POW so that math and floating point libraries
*     do not have to be linked in.
*
* INPUTS
*
*     UINT8             exponent        The exponent to raise two too
*
* OUTPUTS
*
*     UINT32                            The computed power
*
*************************************************************************/
UINT32 PPP_Two_To_Power(UINT8 exponent)
{
    UINT32  answer;

#ifdef NU_NON_REENTRANT_CLIB
    INT     old_state;
#endif

    answer = PPP_ONE;

    /* Make sure the exponent is not zero */
    if (exponent != 0)
    {
        /* The bit shifting operation below has been seen to cause
           problems with non-reentrant C library's. To fix this
           turn interrupts off so that the PPP LISR does not
           interrupt the shift operation. */
#ifdef NU_NON_REENTRANT_CLIB
        old_state = NU_Control_Interrupts (NU_DISABLE_INTERRUPT);
#endif

        answer <<= (UINT32) exponent;

#ifdef NU_NON_REENTRANT_CLIB
        NU_Control_Interrupts (old_state);
#endif
    }

    return (answer);
} /* PPP_Two_To_Power */



/*************************************************************************
* FUNCTION
*
*     PPP_Output
*
* DESCRIPTION
*
*     Makes sure that the device is active and calls the transmit routine
*     for the device. Then deallocates the buffer to the appropriate list.
*
* INPUTS
*
*     NET_BUFFER        *buf_ptr        Pointer to the packet buffer of
*                                        the packet to send.
*     DV_DEVICE_ENTRY   *dev_ptr        Pointer to the device to send
*                                        the packet over.
*     VOID              *unused1        Not used by this routine. Only
*                                        included in order to comply
*                                        with NET 4.0.
*     VOID              *unused2        Not used by this routine. Only
*                                        included in order to comply
*                                        with NET 4.0.
*
* OUTPUTS
*
*     STATUS                            Was the packet sent?
*
*************************************************************************/
STATUS PPP_Output(NET_BUFFER *buf_ptr, DV_DEVICE_ENTRY *dev_ptr,
                  VOID *unused1, VOID *unused2)
{
    STATUS          status = NU_SUCCESS;
    LINK_LAYER      *link_layer;
    UINT16          pkt_type = 0;

#if (PPP_ENABLE_MPPE == NU_TRUE)
    UINT16          pkt_range;
    CCP_LAYER       *ccp;
    UINT16          header;
#endif

#if (HDLC_POLLED_TX == NU_FALSE)
    INT     old_val;
#endif

    /* Remove warnings. */
    UNUSED_PARAMETER(unused1);
    UNUSED_PARAMETER(unused2);

    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

#if (PPP_ENABLE_MPPE == NU_TRUE)
    ccp = &link_layer->ccp;
#endif

    /* If the packet to be sent is IP data then the device must also be
       up and running. If its not an IP packet then it is a PPP negotiation
       packet, in which case we do not care that the device is not up and
       running. That is what this packet is trying to accomplish. */
    if (buf_ptr->mem_flags & (NET_IP | NET_IP6))
    {
        /* Verify that the device is up and running. */
        if (((dev_ptr->dev_flags & (DV_UP | DV_RUNNING)) == (DV_UP | DV_RUNNING))
            && (link_layer->lcp.state == OPENED))
        {
#if (INCLUDE_IPV4 == NU_TRUE)
            if (buf_ptr->mem_flags & NET_IP && link_layer->ncp.state == OPENED)
                pkt_type = PPP_IP_PROTOCOL;
            else
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
            if (buf_ptr->mem_flags & NET_IP6 && link_layer->ncp6.state == OPENED)
                pkt_type = PPP_IPV6_PROTOCOL;
            else
#endif
              status = NU_HOST_UNREACHABLE;

            if (status == NU_SUCCESS)
            {
                PPP_Add_Protocol(buf_ptr, pkt_type);

                /* If it is, update the last_activity_time variable which maintains
                   the time of the last IP activity through the PPP link. */
                link_layer->last_activity_time = NU_Retrieve_Clock();
            }
        }
        else
            status = NU_HOST_UNREACHABLE;
    }

    /* If the PPP packet code is between 0x0021 to 0x00FA, then 
     * encrypt these packets. (Only if encryption is enabled.) */
#if (PPP_ENABLE_MPPE == NU_TRUE)

    if (ccp->options.local.mppe.mppe_encrypt_packets == NU_TRUE &&
        (ccp->options.local.mppe.mppe_ccp_supported_bits &
        (CCP_FLAG_M_BIT | CCP_FLAG_L_BIT | CCP_FLAG_S_BIT)))
    {
        pkt_range = GET16(buf_ptr->data_ptr, 0);
        if ((pkt_range >= MPPE_RANGE_LOWER) && 
            (pkt_range <= MPPE_RANGE_HIGHER))
        {
            /* Encrypt outgoing packet. */
            status = MPPE_Encrypt(buf_ptr, dev_ptr);

            if (status != NU_SUCCESS)
            {
                NLOG_Error_Log("Error Encrypting the packet", NERR_SEVERE, 
                                __FILE__, __LINE__);
            }
 
            if (ccp->options.local.mppe.mppe_stateless)
            {
                header = ccp->options.local.mppe.mppe_send_coherency_count
                    | (CCP_ENCRYPTED_PACKET_FLAG | CCP_FLUSHED_BIT);
            }
            else
            {
                /* If a reset request was received. */
                if (ccp->options.local.mppe.mppe_reset_requested)
                {
                    /* Set the flushed bit and clear the flag. */
                    header = ccp->options.local.mppe.mppe_send_coherency_count |
                        (CCP_ENCRYPTED_PACKET_FLAG | CCP_FLUSHED_BIT);
                    ccp->options.local.mppe.mppe_reset_requested = 0;
                }
                else
                {
                    header = ccp->options.local.mppe.mppe_send_coherency_count |
                        CCP_ENCRYPTED_PACKET_FLAG;
                }
            }

            PPP_Add_Protocol(buf_ptr, header);
            PPP_Add_Protocol(buf_ptr, CCP_ENCRYPTED_PACKET_HDR);

            ccp->options.local.mppe.mppe_send_coherency_count = 
                (ccp->options.local.mppe.mppe_send_coherency_count + 1) & 
                 CCP_MAX_CCOUNT; 

        }
    }
#endif

    if (status == NU_SUCCESS)
    {
#if (HDLC_POLLED_TX == NU_FALSE)
        /* Disable interrupts. */
        old_val = NU_Control_Interrupts(NU_DISABLE_INTERRUPTS);

        /* Put this packet onto the TX queue. */
        MEM_Buffer_Enqueue(&dev_ptr->dev_transq, buf_ptr);

        /* Bump up the counter. */
        dev_ptr->dev_transq_length++;

        /* If using SNMP, this will update the MIB */
        MIB2_ifOutQLen_Inc(dev_ptr->dev_index);

        /* If the packet just added to the TX queue is the only one in the
           queue then send it. */
        if (dev_ptr->dev_transq.head == buf_ptr)
        {
            /* Restore interrupt state */
            NU_Control_Interrupts (old_val);
#endif
            /* If using SNMP, this will update the MIB */
            MIB2_ifOutNUcastPkts_Inci(dev_ptr->dev_index);

            /* Send the packet */
            status = dev_ptr->dev_start(dev_ptr, buf_ptr);
            
#if (HDLC_POLLED_TX == NU_FALSE)         

           if( status == NU_SUCCESS)
           {
               status = NU_Set_Events(&(link_layer->ppp_tx_event),1,NU_OR);
           }
#endif

#if (HDLC_POLLED_TX == NU_FALSE)
        }
        else
        {
            /* Restore interrupt state */
            NU_Control_Interrupts(old_val);

        }
#endif

    }

#if (HDLC_POLLED_TX == NU_TRUE)
    /* Free the buffers onto the appropriate free lists */
    if ((link_layer->hwi.itype != PPP_ITYPE_PPPOE) &&
        (link_layer->hwi.itype != PPP_ITYPE_VIRTUAL))
        MEM_Multiple_Buffer_Chain_Free(buf_ptr);

#endif


    return (status);

} /* PPP_Output */



/*************************************************************************
* FUNCTION
*
*     PPP_Input
*
* DESCRIPTION
*
*     This function pulls the first packet off of the incoming packet
*     list and calls the correct protocol routine to handle the
*     encapsulated packet.
*
* INPUTS
*
*     None
*
* OUTPUTS
*
*     STATUS                     NU_SUCCESS                            
*
*************************************************************************/
STATUS PPP_Input(VOID)
{
    NET_BUFFER          *buf_ptr;
    DV_DEVICE_ENTRY     *dev_ptr;
    LINK_LAYER          *link_layer;
    LCP_LAYER           *lcp;
    NCP_LAYER           *ncp;
    UINT16              ppp_protocol;
    UINT32              *last_activity_time;
    
#if(INCLUDE_IF_STACK == NU_TRUE)
    MIB2_IF_STACK_STRUCT *stack_entry;
#endif

    /* Get a pointer to the incoming buffer we are handling. */
    buf_ptr = MEM_Buffer_List.head;

    /* Get a pointer to the various PPP layers. */
    dev_ptr = buf_ptr->mem_buf_device;
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

#if(INCLUDE_IF_STACK == NU_TRUE)
    /* Get the stack entry if this device is associated with any MP
     * device. NCP is only negotiated at MP device level rather than
     * on lower PPP device.
     */
    stack_entry = MIB2_If_Stack_Get_LI_Entry(dev_ptr->dev_index + 1, NU_TRUE);

    /* If a stack entry is found */
    if((stack_entry) && (stack_entry->mib2_higher_layer))
    {
        /* Check whether the upper layer is L2TP. */
        if((((LINK_LAYER *)stack_entry->mib2_higher_layer->dev_link_layer)->hwi.itype & PPP_ITYPE_L2TP) ||
           (((LINK_LAYER *)stack_entry->mib2_higher_layer->dev_link_layer)->hwi.itype & PPP_ITYPE_PPTP))
        {
            /* Set buffer device. */
            buf_ptr->mem_buf_device = stack_entry->mib2_higher_layer;

            /* Point to the link layer of the MP device. */
            return (*(stack_entry->mib2_higher_layer->dev_input))();
        }

        else
        {
            /* Point to the link layer of the MP device. */
            link_layer =(LINK_LAYER *) stack_entry->mib2_higher_layer->dev_link_layer;
            dev_ptr = stack_entry->mib2_higher_layer;
        }
    }
#endif

    /* Get a pointer to the various PPP layers. */
    lcp = &link_layer->lcp;
#if (INCLUDE_IPV4 == NU_TRUE)
    ncp = &link_layer->ncp;
#endif

    /* Get a pointer to the last_activity_time variable which maintains
       the time of the last IP activity through the PPP link. */
    last_activity_time = &link_layer->last_activity_time;

    /* If the protocol field is compressed, then convert it to the
       16-bit protocol id and move the data pointer appropriately
       before entering the switch. */
    if (buf_ptr->data_ptr[0] & 1)
    {
        ppp_protocol = (UINT16)buf_ptr->data_ptr[0];
        buf_ptr->data_ptr += PPP_PROTOCOL_HEADER_1BYTE;
        buf_ptr->data_len -= PPP_PROTOCOL_HEADER_1BYTE;
        buf_ptr->mem_total_data_len -= PPP_PROTOCOL_HEADER_1BYTE;
    }
    else
    {
        ppp_protocol = GET16(buf_ptr->data_ptr, 0);

#if NU_PPP_DEBUG
    if (ppp_protocol & 0xFF00)
    {
        PPP_PrintPkt((UINT8 *)buf_ptr->data_ptr, buf_ptr->data_len, "IN:");
    }
#endif

        buf_ptr->data_ptr += PPP_PROTOCOL_HEADER_2BYTES;
        buf_ptr->data_len -= PPP_PROTOCOL_HEADER_2BYTES;
        buf_ptr->mem_total_data_len -= PPP_PROTOCOL_HEADER_2BYTES;
    }

    switch (ppp_protocol)
    {
#if (INCLUDE_IPV6 == NU_TRUE)
    case PPP_IPV6_PROTOCOL:
        if (dev_ptr->dev_flags & DV6_IPV6)
            ncp = &link_layer->ncp6;
        else
            PPP_Protocol_Reject(buf_ptr);
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    case PPP_IP_PROTOCOL:
        if((link_layer->ncp.options.ipcp.flags & PPP_FLAG_NO_IPV4) &&
            (ppp_protocol == PPP_IP_PROTOCOL))
        {
            PPP_Protocol_Reject(buf_ptr);
        }
#endif
#if ((INCLUDE_IPV4 == NU_TRUE) || (INCLUDE_IPV6 == NU_TRUE))
        /* Only allow IP packets if LCP and NCP are in the open state
           and the device is up. */
        if ( (lcp->state == OPENED) && (ncp->state == OPENED) &&
            (dev_ptr->dev_flags & DV_UP) )
        {
            /* Update the last_activity_time variable which maintains the
               time of the last IP activity through the PPP link. */
            *last_activity_time = NU_Retrieve_Clock();

            /* If using SNMP, this will update the MIB */
            MIB2_ifInNUcastPkts_Inc(dev_ptr);

            /* Deliver the payload to the IP layer. */
#if (INCLUDE_IPV6 == NU_TRUE)
            if (ppp_protocol == PPP_IPV6_PROTOCOL && ncp->protocol_code == PPP_IPV6_CONTROL_PROTOCOL)
                IP6_Interpret((IP6LAYER*)buf_ptr->data_ptr, dev_ptr, buf_ptr);
#if (INCLUDE_IPV4 == NU_TRUE)
            else
#endif
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
                IP_Interpret((IPLAYER*)buf_ptr->data_ptr, dev_ptr, buf_ptr);
#endif
        }
        else
        {
            /* Bump the number of silent discards and free the buffers */
            PML_SilentDiscards_Inc(dev_ptr);
            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

            /* If using SNMP, this will update the MIB */
            MIB2_ifInUnknownProtos_Inc(dev_ptr);
        }

        break;
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
    case PPP_IPV6_CONTROL_PROTOCOL:
        if (dev_ptr->dev_flags & DV6_IPV6)
        {
#if(INCLUDE_PPP_MP == NU_TRUE)
            if(stack_entry && stack_entry->mib2_higher_layer)
            {
                /* Set the device on which this packet came, to MP device. */
                buf_ptr->mem_buf_device = stack_entry->mib2_higher_layer;
            }
#endif
            NCP_Interpret(&link_layer->ncp6, buf_ptr);
        }

        else
            PPP_Protocol_Reject(buf_ptr);
        break;
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    case PPP_IP_CONTROL_PROTOCOL:
        if(link_layer->ncp.options.ipcp.flags & PPP_FLAG_NO_IPV4)
        {
            PPP_Protocol_Reject(buf_ptr);
            break;
        }

#if(INCLUDE_PPP_MP == NU_TRUE)
        /* If stack entry was found. */
        if(stack_entry && stack_entry->mib2_higher_layer)
        {
            /* Set the device on which this packet came, to MP device. */
            buf_ptr->mem_buf_device = stack_entry->mib2_higher_layer;
        }
#endif
        NCP_Interpret(&link_layer->ncp, buf_ptr);
        break;
#endif

#if(PPP_USE_CHAP == NU_TRUE)
    case PPP_CHAP_PROTOCOL:
        CHAP_Interpret(buf_ptr);
        break;
#endif

    case PPP_PAP_PROTOCOL:
        PAP_Interpret(buf_ptr);
        break;

    case PPP_LINK_CONTROL_PROTOCOL:
        LCP_Interpret(buf_ptr);
        break;

#if(INCLUDE_PPP_MP == NU_TRUE)

    case PPP_MP_PROTOCOL:

        /* If stack entry was found. */
        if((stack_entry) && (stack_entry->mib2_higher_layer))
        {
            /* Call the input function of the MP interface */
            (*(stack_entry->mib2_higher_layer->dev_input))();
        }

        break;
#endif

#if (PPP_ENABLE_MPPE == NU_TRUE)
    case PPP_CCP_PROTOCOL:
        CCP_Interpret(buf_ptr);
        break;

    case PPP_COMPRESSED_DATA:
        CCP_Data_Interpret(buf_ptr);
        break;
#endif

    default:
        PPP_Protocol_Reject(buf_ptr);
       break;

    } /* switch */

    return (NU_SUCCESS);
} /* PPP_Input */

/*************************************************************************
* FUNCTION
*
*     PPP_Dial
*
* DESCRIPTION
*
*     Dials a PPP server and attempts to establish a connection.
*
* INPUTS
*
*     *ppp_options      Pointer to the PPP_OPTIONS structure to
*                       make a PPP call.
* OUTPUTS
*
*     STATUS            Was the connection made. If not which
*                       protocol failed.
*
*************************************************************************/
STATUS PPP_Dial(PPP_OPTIONS *ppp_options)
{
    STATUS                status = NU_SUCCESS;
    DV_DEVICE_ENTRY       *ppp_dev = NU_NULL;
    LINK_LAYER            *link_layer;
    UINT32                index_save;

/* If PPP Multilink Protocol is enabled. */
#if(INCLUDE_PPP_MP == NU_TRUE)
    DV_DEVICE_ENTRY       *mp_dev = NU_NULL;
    MIB2_IF_STACK_STRUCT  *stack_entry;
#endif

    NU_SUPERV_USER_VARIABLES;

    /* switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Grab the semaphore. */
    if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Get the device which matches the PPP device name passed in. */
    if(ppp_options->ppp_link)
        ppp_dev = DEV_Get_Dev_By_Name(ppp_options->ppp_link);

/* If PPP Multilink Protocol is enabled. */
#if(INCLUDE_PPP_MP == NU_TRUE)

    /* Check if the device is to be associated to some MP device. */
    if(ppp_options->ppp_mp_link)
    {
        /* Get the MP device. */
        mp_dev = DEV_Get_Dev_By_Name(ppp_options->ppp_mp_link);

        /* Attach the PPP device with the MP device. */
        status = PPP_MP_Attach_Link(mp_dev, ppp_dev);
    }
#endif

    /* Make sure the device was found and that it is a PPP device. */
    if ((status == NU_SUCCESS) && (ppp_dev) && (ppp_dev->dev_type == DVT_PPP))
    {
        /* Get pointers to the lower layers */
        link_layer = (LINK_LAYER*)ppp_dev->dev_link_layer;

        /* Set the status of the PPP connection attempt. */
        link_layer->connection_status = NU_PPP_DIALING;

        /* Change to CLIENT mode, we must be a client if we are dialing. */
        NCP_Change_IP_Mode(PPP_CLIENT, ppp_dev);

        /* Initialize the PPP state machine. */
        PPP_Reset(ppp_dev);

        PrintInfo("Establishing physical connection to remote server.\n");

        /* Save the index of the device before releasing the semaphore. */
        index_save = ppp_dev->dev_index;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
        }

        /* Connect to server. */
        status = link_layer->hwi.connect(ppp_options->ppp_num_to_dial, ppp_dev);

        if (status == NU_SUCCESS)
        {

            /* Link was successfully connected grab the semaphore. */
            if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
                               __FILE__, __LINE__);
            }

            /* Check if the device has not been removed. */
            if(DEV_Get_Dev_By_Index(index_save) == NU_NULL)
            {
                /* Device has been removed, so set the status. */
                status = NU_INVALID_LINK;
            }

            if (status == NU_SUCCESS)
            {
                PrintInfo("   Connected to remote server.\n");

#if (INCLUDE_IPV4 == NU_TRUE)
                /* Check if a flag is set to not negotiate IPv4 address. */
                if(ppp_options->ppp_flags & NU_PPP_NO_IPV4)
                {
                    /* Set the flag that not to negotiate IPv4 address. */
                    link_layer->ncp.options.ipcp.flags |= PPP_FLAG_NO_IPV4;
                }
#endif

#if ((INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE))
                /* Start the PPP negotiation */
                status = PPP_Lower_Layer_Up(ppp_options->ppp_local_ip4, ppp_options->ppp_local_ip6, ppp_dev);
#elif (INCLUDE_IPV6 == NU_TRUE)
                /* Start the PPP negotiation */
                status = PPP_Lower_Layer_Up(NU_NULL, ppp_options->ppp_local_ip6, ppp_dev);
#else
                /* Start the PPP negotiation */
                status = PPP_Lower_Layer_Up(ppp_options->ppp_local_ip4, NU_NULL, ppp_dev);
#endif
                if (status == NU_SUCCESS)
                {
                    /* Initialize the last activity time to the current system clock
                       when the link is established. */
                    link_layer->last_activity_time = NU_Retrieve_Clock();
#if (INCLUDE_IPV4 == NU_TRUE)
#if (INCLUDE_DNS == NU_TRUE)
                    /* If we have been assigned DNS servers, add routes to them also. */
                    if (IP_ADDR(link_layer->ncp.options.ipcp.primary_dns_server) != NU_NULL)
                        PPP_Attach_DNS_Address(link_layer, 1);

                    if (IP_ADDR(link_layer->ncp.options.ipcp.secondary_dns_server) != NU_NULL)
                        PPP_Attach_DNS_Address(link_layer, 2);
#endif
#endif
                }
            }
        }
        else
            PrintInfo("   Connection to remote server failed.\n");
    }
    else
        status = NU_INVALID_LINK;

/* If PPP Multilink Protocol is enabled. */
#if(INCLUDE_PPP_MP == NU_TRUE)
    if((status != NU_SUCCESS) && mp_dev)
    {
        /* Get stack entry for the PPP device and MP device. */
        stack_entry = MIB2_If_Stack_Get_Entry(mp_dev->dev_index,
            ppp_dev->dev_index);

        /* Check if we have got the entry. */
        if(stack_entry)
        {
            /* Remove the stack entry. */
            MIB2_If_Stack_Remove_Entry(stack_entry);
        }
    }
#endif

    /* Release the semaphore. */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Reset PPP link if we have an error. */
    if (status != NU_SUCCESS)
    {
        /* Hangup PPP link to change link state back to initial. */
        PPP_Hangup(PPP_FORCE, ppp_options->ppp_link);
    }

    /* Return from function in user mode. */
    NU_USER_MODE();
    return status;

} /* PPP_Dial */

/*************************************************************************
* FUNCTION
*
*     PPP_Wait_For_Client
*
* DESCRIPTION
*
*     Puts the modem in answer mode and waits for a caller. When
*     the modem has connected with another modem a PPP session is
*     attempted to be established.
*
* INPUTS
*
*      *ppp_options             Pointer to the PPP_OPTIONS structure
*
* OUTPUTS
*
*     STATUS                            NU_SUCCESS
*
*************************************************************************/
STATUS PPP_Wait_For_Client(PPP_OPTIONS *ppp_options)
{
    STATUS          status;
    DV_DEVICE_ENTRY *dev_ptr;
    LINK_LAYER      *link_layer;
    UINT32          index_save;

    NU_SUPERV_USER_VARIABLES;

    /* switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Grab the semaphore. */
    if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Get the device for the link name. */
    dev_ptr = DEV_Get_Dev_By_Name(ppp_options->ppp_link);

    /* Make sure we got a device and that it is a PPP device. */
    if ((dev_ptr) && (dev_ptr->dev_type == DVT_PPP))
    {
        /* Get a pointer to the PPP structure. */
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

        /* Change to SERVER mode */
        NCP_Change_IP_Mode(PPP_SERVER, dev_ptr);

        do
        {
            /* Set the status of the PPP connection attempt. */
            link_layer->connection_status = NU_PPP_WAITING;

            /* Initialize the state of the LCP and NCP structures. */
            PPP_Reset(dev_ptr);

            PrintInfo("Waiting for remote client to connect.\n");

            /* Save the device index before releasing the semaphore. */
            index_save = dev_ptr->dev_index;

            /* Release the semaphore. */
            if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                               __FILE__, __LINE__);
            }

            /* Start waiting for a client to call. */
            status = link_layer->hwi.passive(dev_ptr);

            /* Grab the semaphore. */
            if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
                               __FILE__, __LINE__);
            }

            /* Check if the device has not been removed. */
            if(DEV_Get_Dev_By_Index(index_save) == NU_NULL)
            {
                /* Device has been removed, so return from this function. */
                status = NU_INVALID_LINK;
                break;
            }

            if (status == NU_SUCCESS)
            {
                PrintInfo("   Remote client connected.\n");
#if (INCLUDE_IPV4 == NU_TRUE)
            /* Check if a flag is set to not negotiate IPv4 address. */
            if(ppp_options->ppp_flags & NU_PPP_NO_IPV4)
            {
                /* Set the flag that not to negotiate IPv4 address. */
                link_layer->ncp.options.ipcp.flags |= PPP_FLAG_NO_IPV4;
            }
#endif


#if ((INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IPV6 == NU_TRUE))
                /* Start the PPP negotiation */
                status = PPP_Lower_Layer_Up(ppp_options->ppp_local_ip4, ppp_options->ppp_local_ip6, dev_ptr);
#elif (INCLUDE_IPV6 == NU_TRUE)
                /* Start the PPP negotiation */
                status = PPP_Lower_Layer_Up(NU_NULL, ppp_options->ppp_local_ip6, dev_ptr);
#else
                /* Start the PPP negotiation */
                status = PPP_Lower_Layer_Up(ppp_options->ppp_local_ip4, NU_NULL, dev_ptr);
#endif
                if (status == NU_SUCCESS)
                {
                    /* Initialize the last activity time to the current system clock
                       when the link is established. */
                    link_layer->last_activity_time = NU_Retrieve_Clock();
                }
            }
          /* Loop while a client is not connected. and the option is not set to
           * return the error whenever it occurs. */
        } while ((!(ppp_options->ppp_flags & NU_PPP_RETURN_ERROR)) &&
            ((status != NU_SUCCESS) && (status != NU_PPP_ATTEMPT_ABORTED)));
    }
    else
        status = NU_INVALID_LINK;

    /* Release the semaphore. */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Return from function in user mode. */
    NU_USER_MODE();
    return status;

} /* PPP_Wait_For_Client */

/*************************************************************************
* FUNCTION
*
*     PPP_Abort_Wait_For_Client
*
* DESCRIPTION
*
*     This routine aborts the wait for a client to connect.  If PPP
*     negotiation has started for the link, this function returns
*     an error.
*
* INPUTS
*
*     CHAR      *link_name              Pointer to the PPP link name.
*
* OUTPUTS
*
*     NU_SUCCESS            The wait for a client was aborted.
*     NU_INVALID_LINK       The link_name specified is not a valid
*                           PPP link.
*     NU_BUSY               A client is currently connecting.
*
*************************************************************************/
STATUS PPP_Abort_Wait_For_Client(CHAR *link_name)
{
    STATUS          status;
    DV_DEVICE_ENTRY *dev_ptr;
    LINK_LAYER      *link_layer;
    NU_SUPERV_USER_VARIABLES;

    /* switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Grab the semaphore. */
    if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Get the device for the link name. */
    dev_ptr = DEV_Get_Dev_By_Name(link_name);

    /* Make sure we got a device and that it is a PPP device. */
    if ((dev_ptr) && (dev_ptr->dev_type == DVT_PPP))
    {
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
        if (link_layer->connection_status == NU_PPP_WAITING)
        {
            /* Set the flag which will cause the modem layer to abort the wait for a client */
            link_layer->connection_status = NU_PPP_WAITING_ABORTED;

            status = NU_SUCCESS;
        }
        else
            status = NU_BUSY;
    }
    else
        status = NU_INVALID_LINK;

    /* Release the semaphore. */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Return from function in user mode. */
    NU_USER_MODE();
    return (status);

} /* PPP_Abort_Wait_For_Client */

/*************************************************************************
* FUNCTION
*
*     PPP_Abort
*
* DESCRIPTION
*
*     This routine aborts the client and server wait for
*     negotiation completion.  It also aborts a client dial
*     phase.  If PPP negotiation has started for the link,
*     all layers are brought down.
*
* INPUTS
*
*     *link_name                Pointer to the PPP link name.
*
* OUTPUTS
*
*     NU_SUCCESS                The wait for a client was aborted.
*     NU_INVALID_MODE           Link has been established. Use
*                               PPP_Hangup to disconnect now.
*     NU_INVALID_LINK           The link_name specified is not a
*                               valid PPP link.
*
*************************************************************************/
STATUS PPP_Abort(CHAR *link_name)
{
    STATUS          status = NU_SUCCESS;
    DV_DEVICE_ENTRY *dev_ptr;
    LINK_LAYER      *link_layer;
    NU_SUPERV_USER_VARIABLES;

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Grab the semaphore. */
    if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain a semaphore",
                       NERR_SEVERE, __FILE__, __LINE__);
    }

    /* Get the device for the link name. */
    dev_ptr = DEV_Get_Dev_By_Name(link_name);

    /* Make sure we got a device and that it is a PPP device. */
    if((dev_ptr) && (dev_ptr->dev_type == DVT_PPP))
    {
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

        switch(link_layer->connection_status)
        {
        case NU_PPP_DIALING:
        case NU_PPP_WAITING:
            /* Set the flag which will cause the modem
               layer to abort the waiting for a client. */
            link_layer->connection_status = NU_PPP_WAITING_ABORTED;
            break;

        case NU_PPP_LINK_NEGOTIATION:
        case NU_PPP_AUTHENTICATION:
            /* Send the negotiation abortion event. This works
               for both client and server negotiations. */
            EQ_Put_Event(PPP_Event, (UNSIGNED)dev_ptr,
                         PPP_ABORT_NEGOTIATION);
            break;

        case NU_PPP_NETWORK_NEGOTIATION:
            /* If LCP layer is still up. */
            if(link_layer->lcp.state == OPENED)
            {
                /* Send the negotiation abortion event. This works
                   for both client and server negotiations. */
                EQ_Put_Event(PPP_Event, (UNSIGNED)dev_ptr,
                             PPP_ABORT_NEGOTIATION);
            }
            /* Otherwise, the link is hanging-up and the
               NCP layer is currently being brought down. */
            else
            {
                /* Cannot abort this negotiation. */
                status = NU_INVALID_MODE;
            }
            break;

        case NU_PPP_CONNECTED:
        case NU_PPP_WAITING_ABORTED:
        case NU_PPP_DISCONNECTED:
        case NU_PPP_HANGING_UP:
        default:
            /* Link is not in negotiation or waiting mode
               so cannot be aborted. PPP_Hangup() must be
               used for disconnecting. */
            status = NU_INVALID_MODE;
            break;
        }

#if(INCLUDE_PPP_MP == NU_TRUE)
        if(status == NU_SUCCESS)
        {
            /* Check if this device has Multilink Protocol on */
            if(link_layer->lcp.options.remote.flags & PPP_FLAG_MRRU)
            {
                /* Remove this link from the bundle */
                PPP_MP_Remove_Link(dev_ptr->dev_index);
            }
        }
#endif
    }
    else
    {
        /* Device not found. */
        status = NU_INVALID_LINK;
    }

    /* Release the semaphore. */
    if(NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release a semaphore",
                       NERR_SEVERE, __FILE__, __LINE__);
    }

    /* Return from function in user mode. */
    NU_USER_MODE();

    return (status);

} /* PPP_Abort */

/*************************************************************************
* FUNCTION
*
*     PPP_Abort_Connection
*
* DESCRIPTION
*
*     This function aborts a PPP link when it is in the
*     connected state.  This is a non-conforming function and
*     must only be called when an immediate reset of the PPP
*     connection is required.  The caller is responsible for
*     resetting the modem.
*
* INPUTS
*
*     *link_name                Pointer to the PPP link name.
*
* OUTPUTS
*
*     NU_SUCCESS                Connection re-initialized.
*     NU_INVALID_MODE           Link is not in connected state.
*     NU_INVALID_LINK           The link_name specified is not a
*                               valid PPP link.
*
*************************************************************************/
STATUS PPP_Abort_Connection(CHAR *link_name)
{
    STATUS          status = NU_SUCCESS;
    UNSIGNED        bufs_ava;
    DV_DEVICE_ENTRY *dev_ptr;
    LINK_LAYER      *link_layer;
#if (INCLUDE_PPP_MP == NU_TRUE)
    MIB2_IF_STACK_STRUCT    *stack_entry;
#endif
    NU_SUPERV_USER_VARIABLES;

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    PrintInfo("PPP_Abort_Connection\n");

    /* Grab the semaphore. */
    if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain a semaphore",
                       NERR_SEVERE, __FILE__, __LINE__);
    }

    /* Find the device that we want to reset. */
    dev_ptr = DEV_Get_Dev_By_Name(link_name);

    /* Make sure the device was found. */
    if((dev_ptr) && (dev_ptr->dev_type == DVT_PPP))
    {
        /* Grab a pointer to the LCP layer structure. */
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

        /* If link is not in the connected state. */
        if(link_layer->connection_status != NU_PPP_CONNECTED)
        {
            status = NU_INVALID_MODE;
        }
        else
        {
#if (INCLUDE_PPP_MP == NU_TRUE)
            /* Check if this device has Multilink Protocol. */
            if(link_layer->lcp.options.remote.flags & PPP_FLAG_MRRU)
            {
                /* Remove this link from the bundle */
                PPP_MP_Remove_Link(dev_ptr->dev_index);
            }

            /* Get the stack entry associated with this MP device. */
            stack_entry = MIB2_If_Stack_Get_LI_Entry(dev_ptr->dev_index +
                                                     1, NU_TRUE);

            /* Check if we have got the entry and a PPP device. */
            if(stack_entry && stack_entry->mib2_higher_layer)
            {
                if(((PPP_MP_BUNDLE *)stack_entry->mib2_higher_layer->
                    dev_extension)->mp_num_links == 1)
                {
                    /* Close any IP communications that use this link. */
                    NCP_Close_IP_Layer(stack_entry->mib2_higher_layer);
                }
            }
            else
#endif
                /* Close any IP communications that use this link. */
                NCP_Close_IP_Layer(dev_ptr);

            /* Clear the transmit queue. */
            MEM_Buffer_Cleanup(&dev_ptr->dev_transq);
            dev_ptr->dev_transq_length = 0;

            /* Clear all PPP timer events for this link. */
            TQ_Timerunset(PPP_Event, TQ_CLEAR_ALL_EXTRA,
                          (UNSIGNED)dev_ptr, 0);
#if (INCLUDE_IPV4 == NU_TRUE)
            TQ_Timerunset(IPCP_Event, TQ_CLEAR_ALL_EXTRA,
                          (UNSIGNED)dev_ptr, 0);
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
            TQ_Timerunset(IPV6CP_Event, TQ_CLEAR_ALL_EXTRA,
                          (UNSIGNED)dev_ptr, 0);
#endif

            /* Eat all negotiation events. */
            NU_Retrieve_Events(&link_layer->negotiation_progression,
                               (PPP_LCP_FAIL  | PPP_AUTH_FAIL      |
                                PPP_NCP_FAIL  | PPP_CCP_FAIL |
                                PPP_CONFIG_SUCCESS | PPP_NEG_ABORT |
                                PPP_NEG_TIMEOUT),
                               NU_OR_CONSUME, &bufs_ava, NU_NO_SUSPEND);

            /* Reset current hardware state but do not reset
               modem as caller is responsible for that. */
            link_layer->hwi.state = INITIAL;

            /* Set the status of the PPP connection. */
            link_layer->connection_status = NU_PPP_DISCONNECTED;

            /* Set NCP state. */
            link_layer->ncp.state = STOPPED;
#if (INCLUDE_IPV6 == NU_TRUE)
            link_layer->ncp6.state = STOPPED;
#endif
        }
    }
    else
    {
        /* Device not found. */
        status = NU_INVALID_LINK;
    }

    /* Release the semaphore. */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release a semaphore",
                       NERR_SEVERE, __FILE__, __LINE__);
    }

    /* Return from function in user mode. */
    NU_USER_MODE();

    return (status);

} /* PPP_Abort_Connection */

/*************************************************************************
* FUNCTION
*
*     PPP_Set_Link_Options
*
* DESCRIPTION
*
*     Initializes the LCP and NCP configuration options to be used
*     for a particular PPP link.
*
* INPUTS
*
*     CHAR              *link_name      Pointer to the name of the PPP
*                                       link to which the options will
*                                       be applied.
*
*     NU_PPP_CFG        *ppp_cfg        Pointer to the PPP configuration
*                                       structure that will be applied
*                                       to the PPP link.
*
* OUTPUTS
*
*     STATUS                            NU_SUCCESS if the options are
*                                       successfully applied, otherwise
*                                       NU_INVALID_LINK is returned
*
*************************************************************************/
STATUS PPP_Set_Link_Options(CHAR *link_name, NU_PPP_CFG *ppp_cfg)
{
    LCP_LAYER       *lcp;
#if (INCLUDE_IPV4 == NU_TRUE)
    NCP_LAYER       *ncp;
#endif

#if (PPP_ENABLE_MPPE == NU_TRUE)
    CCP_LAYER       *ccp;
#endif

    STATUS          status;
    DV_DEVICE_ENTRY *dev_ptr;
    LINK_LAYER      *link_layer;
    NU_SUPERV_USER_VARIABLES;

    /* switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Init */
    status = PPP_Validate_Link_Options(ppp_cfg);

    if (status != NU_SUCCESS)
    {
        return status; 
    }

    /* Grab the semaphore. */
    if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Find the device for this link name. */
    dev_ptr = DEV_Get_Dev_By_Name(link_name);

    /* Make sure the device was found and that it is a PPP device. */
    if ((dev_ptr != NU_NULL) && (dev_ptr->dev_type == DVT_PPP))
    {
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

        /* Make sure the options pointer is valid */
        if (ppp_cfg != NU_NULL)
        {
            /* Get pointers to the LCP and NCP structures. */
            lcp = &link_layer->lcp;
#if (INCLUDE_IPV4 == NU_TRUE)
            ncp = &link_layer->ncp;

            /* Reset the default flags. */
            ncp->options.ipcp.default_flags = 0;
#endif

#if (PPP_ENABLE_MPPE == NU_TRUE)
            ccp = &link_layer->ccp;
#endif

            /* Reset the default flags. */
            lcp->options.local.default_flags = 0;

            /* negotiate all options as defined by the use flags. */
            lcp->options.local.default_flags |= ppp_cfg->use_flags;

            /* Negotiate the ACCM. */
            if (ppp_cfg->use_flags & PPP_FLAG_ACCM)
            {
                lcp->options.local.default_accm = ppp_cfg->default_accm;
            }

            lcp->options.local.default_flags |= PPP_FLAG_MRU;
            lcp->options.local.default_mru = ppp_cfg->default_mru;

#if(INCLUDE_PPP_MP == NU_TRUE)
            lcp->options.local.mp_mrru = ppp_cfg->mp_mrru;
#endif
            /* Set the default authentication protocol. */

            /* authentication protocol, MSCHAPv2 */
            if (ppp_cfg->default_auth_protocol == PPP_CHAP_MS2_PROTOCOL)
                lcp->options.local.default_flags |= PPP_FLAG_CHAP_MS2;

            /* authentication protocol, MSCHAPv1 */
            else if (ppp_cfg->default_auth_protocol == PPP_CHAP_MS1_PROTOCOL)
                lcp->options.local.default_flags |= PPP_FLAG_CHAP_MS1;

            /* authentication protocol, CHAP */
            else if (ppp_cfg->default_auth_protocol == PPP_CHAP_PROTOCOL)
                lcp->options.local.default_flags |= PPP_FLAG_CHAP;

            /* authentication protocol, PAP */
            else if (ppp_cfg->default_auth_protocol == PPP_PAP_PROTOCOL)
                lcp->options.local.default_flags |= PPP_FLAG_PAP;

            /* Define the FCS size. */
            lcp->options.local.default_fcs_size = ppp_cfg->default_fcs_size;

#if (INCLUDE_IPV4 == NU_TRUE)
            /* Request DNS server addresses. */
            if (ppp_cfg->num_dns_servers == 1)
                ncp->options.ipcp.default_flags |= PPP_FLAG_DNS1;

            else if (ppp_cfg->num_dns_servers == 2)
                ncp->options.ipcp.default_flags |= (PPP_FLAG_DNS1 | PPP_FLAG_DNS2);

            else if (ppp_cfg->num_dns_servers == 0)
                ncp->options.ipcp.default_flags &= ~(PPP_FLAG_DNS1 | PPP_FLAG_DNS2);
            
#endif

            /* The variables use_pf_compression, use_ac_compression,
             * use_magic_number and use_accm are left here for compatibility
             * with older versions of PPP */

            /* Negotiate protocol field compression. */
            if (ppp_cfg->use_pf_compression == NU_TRUE)
                lcp->options.local.default_flags |= PPP_FLAG_PFC;

            /* Negotiate address and control field compression. */
            if (ppp_cfg->use_ac_compression == NU_TRUE)
                lcp->options.local.default_flags |= PPP_FLAG_ACC;

            /* Negotiate the magic number option. */
            if (ppp_cfg->use_magic_number == NU_TRUE)
                lcp->options.local.default_flags |= PPP_FLAG_MAGIC;

            /* Negotiate the ACCM */
            if (ppp_cfg->use_accm == NU_TRUE)
            {
                lcp->options.local.default_flags |= PPP_FLAG_ACCM;
                lcp->options.local.default_accm = ppp_cfg->default_accm;
            }

#if (PPP_ENABLE_MPPE == NU_TRUE)
            if (ppp_cfg->require_encryption == NU_TRUE)
                ccp->options.local.mppe.mppe_require_encryption = 1;
            else 
                ccp->options.local.mppe.mppe_require_encryption = 0;

            if (ppp_cfg->use_40_bit_encryption == NU_TRUE)
                ccp->options.local.mppe.mppe_default_flags |= CCP_FLAG_L_BIT;
            else 
                ccp->options.local.mppe.mppe_default_flags &= ~CCP_FLAG_L_BIT;

            if (ppp_cfg->use_56_bit_encryption == NU_TRUE)
                ccp->options.local.mppe.mppe_default_flags |= CCP_FLAG_M_BIT;
            else 
                ccp->options.local.mppe.mppe_default_flags &= ~CCP_FLAG_M_BIT;

            if (ppp_cfg->use_128_bit_encryption == NU_TRUE)
                ccp->options.local.mppe.mppe_default_flags |= CCP_FLAG_S_BIT;
            else 
                ccp->options.local.mppe.mppe_default_flags &= ~CCP_FLAG_S_BIT;

#endif

            /* Set the return value. */
            status = NU_SUCCESS;
        }
    }
    else
    {
        status = NU_INVALID_LINK;
    }

    /* Release the semaphore. */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Return from function in user mode. */
    NU_USER_MODE();
    return (status);

} /* PPP_Set_Link_Options */


/*************************************************************************
* FUNCTION
*
*     PPP_Get_Link_Options
*
* DESCRIPTION
*
*     Gets the LCP and NCP configuration options which are being
*     used for a particular PPP link.
*
* INPUTS
*
*     CHAR              *link_name      Pointer to the name of the PPP
*                                       link
*
*     NU_PPP_CFG        *ppp_cfg        Pointer to the PPP configuration
*                                       structure that will be returned
*
* OUTPUTS
*
*     STATUS                            NU_SUCCESS if the options are
*                                       successfully applied, otherwise
*                                       NU_INVALID_LINK is returned
*
*************************************************************************/
STATUS PPP_Get_Link_Options(CHAR *link_name, NU_PPP_CFG *ppp_cfg)
{
    LCP_LAYER       *lcp;
#if (INCLUDE_IPV4 == NU_TRUE)
    NCP_LAYER       *ncp;
#endif
#if (PPP_ENABLE_MPPE == NU_TRUE)
    CCP_LAYER       *ccp;
#endif
    DV_DEVICE_ENTRY *dev_ptr;
    LINK_LAYER      *link_layer;
    STATUS          status = NU_INVALID_PARM;

    NU_SUPERV_USER_VARIABLES;

    /* switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Grab the semaphore. */
    if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Find the device for this link name. */
    dev_ptr = DEV_Get_Dev_By_Name(link_name);

    /* Make sure the device was found and that it is a PPP device. */
    if ((dev_ptr != NU_NULL) && (dev_ptr->dev_type == DVT_PPP))
    {
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

        /* Make sure the options pointer is valid */
        if (ppp_cfg != NU_NULL)
        {
            /* Get pointers to the LCP and NCP structures. */
            lcp = &link_layer->lcp;
#if (INCLUDE_IPV4 == NU_TRUE)
            ncp = &link_layer->ncp;
#endif

#if (PPP_ENABLE_MPPE == NU_TRUE)
            ccp = &link_layer->ccp;
#endif
            /* ACCM */
            ppp_cfg->default_accm = lcp->options.local.default_accm;

            /* authentication protocol, MSCHAPv2 */
            if(lcp->options.local.default_flags & PPP_FLAG_CHAP_MS2)
                ppp_cfg->default_auth_protocol = PPP_CHAP_MS2_PROTOCOL;

            /* authentication protocol, MSCHAPv1 */
            else if(lcp->options.local.default_flags & PPP_FLAG_CHAP_MS1)
                ppp_cfg->default_auth_protocol = PPP_CHAP_MS1_PROTOCOL;

            /* authentication protocol, CHAP */
            else if(lcp->options.local.default_flags & PPP_FLAG_CHAP)
                ppp_cfg->default_auth_protocol = PPP_CHAP_PROTOCOL;

            /* authentication protocol, PAP */
            else if(lcp->options.local.default_flags & PPP_FLAG_PAP)
                ppp_cfg->default_auth_protocol = PPP_PAP_PROTOCOL;

            /* FCS */
            ppp_cfg->default_fcs_size = lcp->options.local.default_fcs_size;

            /* MRU */
            ppp_cfg->default_mru = lcp->options.local.default_mru;

            /* Protocol field compression. */
            if (lcp->options.local.default_flags & PPP_FLAG_PFC)
                ppp_cfg->use_pf_compression = NU_TRUE;

            /* Address and control field compression. */
            if (lcp->options.local.default_flags & PPP_FLAG_ACC)
                ppp_cfg->use_ac_compression = NU_TRUE;

#if (INCLUDE_IPV4 == NU_TRUE)
            /* Request DNS server addresses. */
            if(ncp->options.ipcp.default_flags & (PPP_FLAG_DNS1 | PPP_FLAG_DNS2))
            {
                ppp_cfg->num_dns_servers = 2;
            }

            else if(ncp->options.ipcp.default_flags & PPP_FLAG_DNS1)
            {
                ppp_cfg->num_dns_servers = 1;
            }
            else
            {
                ppp_cfg->num_dns_servers = 0;
            }
#endif

#if(INCLUDE_PPP_MP == NU_TRUE)
            /* MRRU */
            ppp_cfg->mp_mrru = lcp->options.local.mp_mrru;
#endif

#if (PPP_ENABLE_MPPE == NU_TRUE)
            if (ccp->options.local.mppe.mppe_require_encryption == 1)
                ppp_cfg->require_encryption = NU_TRUE;

            else if (ccp->options.local.mppe.mppe_require_encryption == 0)
                ppp_cfg->require_encryption = NU_FALSE;

            if (ccp->options.local.mppe.mppe_default_flags & CCP_FLAG_L_BIT)
                ppp_cfg->use_40_bit_encryption = NU_TRUE;
            else
                ppp_cfg->use_40_bit_encryption = NU_FALSE;

            if (ccp->options.local.mppe.mppe_default_flags & CCP_FLAG_M_BIT)
                ppp_cfg->use_56_bit_encryption = NU_TRUE;
            else
                ppp_cfg->use_56_bit_encryption = NU_FALSE;

            if (ccp->options.local.mppe.mppe_default_flags & CCP_FLAG_S_BIT)
                ppp_cfg->use_128_bit_encryption = NU_TRUE;
            else
                ppp_cfg->use_128_bit_encryption = NU_FALSE;

#endif

            /* return all the other negotiation options. */
            ppp_cfg->use_flags = lcp->options.local.default_flags;

            /* Set the return value. */
            status = NU_SUCCESS;
        }
    }
    else
        status = NU_INVALID_LINK;

    /* Release the semaphore. */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Return from function in user mode. */
    NU_USER_MODE();

    return (status);

} /* PPP_Get_Link_Options */


/*************************************************************************
* FUNCTION
*
*     PPP_Event_Handler
*
* DESCRIPTION
*
*     Handles processing of LCP, PAP, and CHAP events.
*
* INPUTS
*
*     evt                               Event identifier.
*     dat                               Pointer to PPP device structure.
*     subevt                            PPP event to be serviced.
*
* OUTPUTS
*
*     None
*
*************************************************************************/
VOID PPP_Event_Handler(TQ_EVENT evt, UNSIGNED dat, UNSIGNED subevt)
{
    DV_DEVICE_ENTRY             *dev_ptr = (DV_DEVICE_ENTRY*)dat;
    LINK_LAYER                  *link_layer;
    LCP_LAYER                   *lcp;
    AUTHENTICATION_LAYER        *auth;
    UNSIGNED                    events = PPP_NEG_ABORT;

#if(INCLUDE_PPP_MP == NU_TRUE)
    MIB2_IF_STACK_STRUCT        *stack_entry;
    DV_DEVICE_ENTRY             *mp_dev = NU_NULL;
#endif

    /* Get a pointer to the link structures. */
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
    lcp = &link_layer->lcp;
    auth = &link_layer->authentication;

    /* Switch on the event and do the appropriate processing. */
    switch (subevt)
    {
    case LCP_OPEN_REQUEST:

        PrintInfo("Starting LCP negotiation...\n");

        /* Initialize the number of retransmit attempts. */
        lcp->num_transmissions = LCP_MAX_CONFIGURE;

    case LCP_SEND_CONFIG:

        if (lcp->num_transmissions-- > 0)
        {
            PrintInfo("   Sending LCP config request...\n");

            /* Send a configure request packet */
            LCP_Send_Config_Req(dev_ptr);
        }
        else
            NU_Set_Events(&link_layer->negotiation_progression, PPP_LCP_FAIL, NU_OR);

        break;

    case LCP_LAYER_UP:

        PrintInfo("   LCP layer up.\n");

#if (LCP_MAX_ECHO > 0)
        /* Start the timed event for sending an echo request. */
        TQ_Timerset(PPP_Event, (UNSIGNED)dev_ptr, LCP_ECHO_VALUE, LCP_ECHO_REQ);
#endif

        /* Fall through to start authentication. */

    case AUTH_REQUEST:

        /* If LCP is not open, open it. */
        if (lcp->state != OPENED)
            break;

        /* If already authenticated, go straight to NCP negotiation. */
        if (auth->state == AUTHENTICATED)
            goto START_NCP_NEGOTIATION;

        /* Set the status of the PPP connection attempt. */
        link_layer->connection_status = NU_PPP_AUTHENTICATION;

        /* Do we need to authenticate? */
        if ((lcp->options.local.flags & PPP_AUTH_MASK)
            || (lcp->options.remote.flags & PPP_AUTH_MASK))
        {
            /* Initialize the number of retransmit attempts. */
            auth->num_transmissions = LCP_MAX_AUTHENTICATE;

#if(PPP_USE_CHAP == NU_TRUE)
            /* If we are a SERVER using CHAP then we need to challenge the
               peer for a password. */
            if (lcp->options.local.flags & PPP_FLAG_CHAP)
            {
                PrintInfo("Starting CHAP authentication...\n");

                /* Start the authentication. */
                goto CHAP_AUTHENTICATION;
            }
#endif
            /* If we are using PAP then initiate the authentication. We
               will only send a PAP packet if we are a CLIENT. */
            if (lcp->options.remote.flags & PPP_FLAG_PAP)
            {
                PrintInfo("Starting PAP authentication...\n");

                /* Start the authentication. */
                goto PAP_AUTHENTICATION;
            }

            /* In all other cases, just wait for the peer to initiate
               authentication. */
        }
        else
        {
            /* Neither side requires authentication. */
            auth->state = AUTHENTICATED;
        }

        /* If already authenticated, go straight to NCP negotiation. */
        if (auth->state == AUTHENTICATED)
            goto START_NCP_NEGOTIATION;

        break;

#if(PPP_USE_CHAP == NU_TRUE)
    case CHAP_SEND_CHALLENGE:
        CHAP_AUTHENTICATION:

        if (auth->num_transmissions-- > 0)
        {
            PrintInfo("Send CHAP challenge...\n");

            /* Send the CHAP challenge */
            CHAP_Send_Challenge(dev_ptr);
        }
        else
            NU_Set_Events(&link_layer->negotiation_progression, PPP_AUTH_FAIL, NU_OR);

        break;
#endif

    case PAP_SEND_AUTH:
        PAP_AUTHENTICATION:

        if (auth->num_transmissions-- > 0)
        {
            PrintInfo("Send PAP authentication...\n");

            /* Retransmit the PAP authentication packet */
            PAP_Send_Authentication(dev_ptr);
        }
        else
            NU_Set_Events(&link_layer->negotiation_progression, PPP_AUTH_FAIL, NU_OR);

        break;


    case AUTHENTICATED:

        PrintInfo("   Authentication successful.\n");

        /* Start NCP negotiation. */
        START_NCP_NEGOTIATION:


        if(link_layer->hwi.itype & PPP_ITYPE_L2TP_LAC)
        {
            /* Clear any remaining timeout events. */
            TQ_Timerunset(PPP_Event, TQ_CLEAR_EXACT, (UNSIGNED)dev_ptr, PPP_STOP_NEGOTIATION);

            /* Disable echo for LAC. */
            TQ_Timerunset(PPP_Event, TQ_CLEAR_EXACT, (UNSIGNED)dev_ptr, LCP_ECHO_REQ);

            NU_Set_Events(&link_layer->negotiation_progression, PPP_CONFIG_SUCCESS, NU_OR);
            return;
        }


#if(INCLUDE_PPP_MP == NU_TRUE)

        /* Check if Multilink Protocol is negotiated at LCP */
        if(link_layer->lcp.options.remote.flags & PPP_FLAG_MRRU)
        {
            /* Get the stack entry associated with this PPP link. */
            stack_entry = MIB2_If_Stack_Get_LI_Entry(dev_ptr->dev_index + 1, NU_FALSE);

            /* If we have got a stack entry. */
            if(stack_entry)
            {
                mp_dev = stack_entry->mib2_higher_layer;
            }

            /* Associate the link with the MP device. */
            if(PPP_MP_Add_Link(dev_ptr, mp_dev) == NU_SUCCESS)
            {
                /* If we have a pointer to MP device. */
                if(!mp_dev)
                {
                    /* Get the stack entry associated with this PPP link. */
                    stack_entry = MIB2_If_Stack_Get_LI_Entry(dev_ptr->dev_index + 1, NU_TRUE);

                    /* If we have got a stack entry. */
                    if(stack_entry)
                    {
                        /* Point to the MP device. */
                        mp_dev = stack_entry->mib2_higher_layer;
                    }
                }

                /* Clear any remaining timeout events. */
                TQ_Timerunset(PPP_Event, TQ_CLEAR_EXACT, (UNSIGNED)dev_ptr, PPP_STOP_NEGOTIATION);

                /* If NCP has already been negotiated at MP level then
                 * there is no need to negotiate it again.
                 */
                if(mp_dev)
                {
#if (INCLUDE_IPV4 == NU_TRUE)
                    if(((LINK_LAYER *)mp_dev->dev_link_layer)->ncp.state == OPENED)
                    {
                        /* Set the state of the NCP as open. */
                        link_layer->ncp.state = OPENED;

                        /* Just set the event that PPP negotiation has been
                         * completed.
                         */
                        NU_Set_Events(&link_layer->negotiation_progression, PPP_CONFIG_SUCCESS, NU_OR);
                        break;
                    }
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
                    if(((LINK_LAYER *)mp_dev->dev_link_layer)->ncp6.state == OPENED)
                    {
                        /* Set the state of the NCP as open. */
                        link_layer->ncp6.state = OPENED;

                        /* Just set the event that PPP negotiation has been
                         * completed.
                         */
                        NU_Set_Events(&link_layer->negotiation_progression, PPP_CONFIG_SUCCESS, NU_OR);
                        break;
                    }
#endif
                }

                /* Set the MP device for further negotiations. */
                dev_ptr = mp_dev;
            }
        }

#endif

#if (PPP_ENABLE_MPPE == NU_TRUE)
        link_layer->ccp.options.local.mppe.mppe_ccp_supported_bits = 
            link_layer->ccp.options.local.mppe.mppe_default_flags;

        if(!(link_layer->ncp.options.ipcp.flags & PPP_FLAG_NO_IPV4))
            EQ_Put_Event(PPP_CCP_Event, (UNSIGNED)dev_ptr, CCP_OPEN_REQUEST);
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
        if(!(link_layer->ncp.options.ipcp.flags & PPP_FLAG_NO_IPV4))
            EQ_Put_Event(IPCP_Event, (UNSIGNED)dev_ptr, NCP_OPEN_REQUEST);
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
        /* Also set event to start IPV6CP negotiation. */
        if (dev_ptr->dev_flags & DV6_IPV6)
            EQ_Put_Event(IPV6CP_Event, (UNSIGNED)dev_ptr, NCP_OPEN_REQUEST);
#endif
        break;

    case LCP_ECHO_REQ:

        /* Send an echo request packet */
        LCP_Send_Echo_Req(dev_ptr);
        break;


    case LCP_CLOSE_REQUEST:

        TQ_Timerunset(PPP_Event, TQ_CLEAR_EXACT, (UNSIGNED)dev_ptr, LCP_ECHO_REQ);

        if (lcp->state == STOPPING)
        {
#if (INCLUDE_IPV6 == NU_TRUE)
            if (link_layer->ncp6.state == OPENED)
            {
                link_layer->ncp6.state = STOPPING;
                EQ_Put_Event(IPV6CP_Event, (UNSIGNED)dev_ptr, NCP_CLOSE_REQUEST);
#if (INCLUDE_IPV4 == NU_TRUE)
                if (link_layer->ncp.state != OPENED)
                                break;
#endif
            }
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
            if (link_layer->ncp.state == OPENED)
            {
                link_layer->ncp.state = STOPPING;
                EQ_Put_Event(IPCP_Event, (UNSIGNED)dev_ptr, NCP_CLOSE_REQUEST);
                break;
            }
#endif
        }

        /* Initialize the restart counter. */
        lcp->num_transmissions = LCP_MAX_TERMINATE;

    case LCP_SEND_TERMINATE:

        /* Only send terminate requests during a normal close. */
        if (link_layer->hwi.state == OPENED)
        {
            if (lcp->state == CLOSING)
            {
                if (lcp->num_transmissions-- > 0)
                {
                    PrintInfo("   Sending LCP terminate request...\n");

                    /* Send a terminate request for LCP. */
                    LCP_Send_Terminate_Req(dev_ptr, PPP_LINK_CONTROL_PROTOCOL);

                    /* Set up a timeout event for LCP close. */
                    TQ_Timerset(PPP_Event, (UNSIGNED)dev_ptr, LCP_TIMEOUT_VALUE, LCP_SEND_TERMINATE);

                    break;
                }
                else
                    lcp->state = CLOSED;
            }
            else
            {
                /* Send a terminate ack. */
                LCP_Send_Terminate_Ack(dev_ptr, PPP_LINK_CONTROL_PROTOCOL);
                lcp->state = STOPPED;
            }
        }

        /* Fall through to hangup. */

    case LCP_LAYER_DOWN:

        if (link_layer->hwi.state != OPENED)
        {
#if (INCLUDE_IPV4 == NU_TRUE)
            if(!(link_layer->ncp.options.ipcp.flags & PPP_FLAG_NO_IPV4))
            {
                /* Modem is already down. Abort all NCP links. */
                if (link_layer->ncp.state != CLOSED && link_layer->ncp.state != STOPPED)
                {
                    link_layer->ncp.state = STOPPING;
                    EQ_Put_Event(IPCP_Event, (UNSIGNED)dev_ptr, NCP_CLOSE_REQUEST);
                }
            }
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
            if (dev_ptr->dev_flags & DV6_IPV6)
            {
                if (link_layer->ncp6.state != CLOSED && link_layer->ncp6.state != STOPPED)
                {
                    link_layer->ncp6.state = STOPPING;
                    EQ_Put_Event(IPV6CP_Event, (UNSIGNED)dev_ptr, NCP_CLOSE_REQUEST);
                }
            }
#endif
            break;
        }

        else
        {
            /* Wait one timeout period before disconnecting modem. */
            link_layer->hwi.state = STOPPING;
            TQ_Timerset(PPP_Event, (UNSIGNED)dev_ptr, LCP_TIMEOUT_VALUE, MDM_HANGUP);
            break;
        }


    case MDM_HANGUP:

        PrintInfo("   Modem disconnecting...\n");

        /* LCP has closed normally, so disconnect the physical link. */
        link_layer->hwi.disconnect(link_layer, NU_FALSE);

        /* Set the status of the PPP connection. */
        link_layer->connection_status = NU_PPP_HANGING_UP;

#if(INCLUDE_PPP_MP == NU_TRUE)
        /* Check if this device has Multilink Protocol on */
        if(link_layer->lcp.options.remote.flags & PPP_FLAG_MRRU)
        {
            /* Remove this link from the bundle */
            PPP_MP_Remove_Link(dev_ptr->dev_index);
        }
#endif

        break;

    case PPP_STOP_NEGOTIATION:

        /* Add timeout event to event flags. */
        events = (events | PPP_NEG_TIMEOUT);

    case PPP_ABORT_NEGOTIATION:

        PrintInfo("   Request to stop negotiations...\n");

        /* Set the status of the PPP connection attempt. */
        link_layer->connection_status = NU_PPP_DISCONNECTED;

        /* Clear all PPP timer events for this link. */
        TQ_Timerunset(PPP_Event, TQ_CLEAR_ALL_EXTRA, (UNSIGNED)dev_ptr, 0);
#if (INCLUDE_IPV4 == NU_TRUE)
        TQ_Timerunset(IPCP_Event, TQ_CLEAR_ALL_EXTRA, (UNSIGNED)dev_ptr, 0);
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
        TQ_Timerunset(IPV6CP_Event, TQ_CLEAR_ALL_EXTRA, (UNSIGNED)dev_ptr, 0);
#endif
        /* Disconnect. No need to wait for it. */
        lcp->state = STOPPED;
        EQ_Put_Event(PPP_Event, (UNSIGNED)dev_ptr, LCP_LAYER_DOWN);

        /* Set the negotiation as aborted. */
        NU_Set_Events(&link_layer->negotiation_progression, events, NU_OR);
        break;

    default:
        UNUSED_PARAMETER(evt);
        break;

    } /* end switch */

    return;
} /* PPP_Event_Handler */



/*************************************************************************
* FUNCTION
*
*     PPP_Obtain_Connection_Status
*
* DESCRIPTION
*
*     Handles returning the status of PPP link negotiation.
*
* INPUTS
*
*     CHAR              *link_name      Pointer to the PPP link name to
*                                        check.
*
* OUTPUTS
*
*     STATUS                            The status of the PPP connection
*                                        attempt.
*
*************************************************************************/
STATUS PPP_Obtain_Connection_Status(CHAR *link_name)
{
    DV_DEVICE_ENTRY *dev_ptr;
    STATUS          status;
    LINK_LAYER      *link_layer;
    NU_SUPERV_USER_VARIABLES;

    /* switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Grab the semaphore. */
    if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Find the device for this link name. */
    dev_ptr = DEV_Get_Dev_By_Name(link_name);

    /* Make sure the device was found and that it is a PPP device. */
    if ((dev_ptr) && (dev_ptr->dev_type == DVT_PPP))
    {
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
        status = link_layer->connection_status;
    }
    else
        status = NU_INVALID_LINK;

    /* Release the semaphore. */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Return from function in user mode. */
    NU_USER_MODE();
    return (status);

} /* PPP_Obtain_Connection_Status */



/*************************************************************************
* FUNCTION
*
*     PPP_Negotiation_Timeout
*
* DESCRIPTION
*
*     This function can set the negotiation timeout for a PPP link
*     and can retrieve the current negotiation timeout for a PPP link.
*
* INPUTS
*
*     CHAR              *link_name      Pointer to the PPP link name to
*                                        check.
*     INT               set             Set to NU_TRUE to set the timeout,
*                                        any other value will get the current
*                                        timeout.
*     UINT16            *neg_to_secs    Pointer to the new negotiation timeout
*                                        seconds or where to store
*                                        the current seconds.
*
* OUTPUTS
*
*     STATUS                            NU_SUCCESS if the timeout option
*                                        is successful, otherwise
*                                        NU_INVALID_LINK or
*                                        NU_INVALID_PARM is returned
*
*************************************************************************/
STATUS PPP_Negotiation_Timeout(CHAR *link_name, INT set,
                               UINT16 *neg_to_secs)
{
    DV_DEVICE_ENTRY *dev_ptr;
    STATUS          status = NU_SUCCESS;
    LINK_LAYER      *link_layer;
    NU_SUPERV_USER_VARIABLES;

    /* switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Grab the semaphore. */
    if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Find the device for this link name. */
    dev_ptr = DEV_Get_Dev_By_Name(link_name);

    /* Make sure the device was found and that it is a PPP device. */
    if ((dev_ptr) && (dev_ptr->dev_type == DVT_PPP))
    {
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

        /* Validate the pointer */
        if (neg_to_secs)
        {
            /* Set the timeout or get it */
            if (set == NU_TRUE)
                link_layer->negotiation_timeout = (UINT16)(*neg_to_secs * SCK_Ticks_Per_Second);
            else
                *neg_to_secs = (UINT16)(link_layer->negotiation_timeout / SCK_Ticks_Per_Second);
        }
        else
            status = NU_INVALID_PARM;
    }
    else
        status = NU_INVALID_LINK;

    /* Release the semaphore. */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    return (status);

} /* PPP_Negotiation_Timeout */



/*************************************************************************
* FUNCTION
*
*     PPP_Last_Activity_Time
*
* DESCRIPTION
*
*     Routine to return the number of seconds since the last IP activity
*     through the PPP link.  This routine can be used by the application
*     to determine if a connection can be closed due to inactivity.
*
* INPUTS
*
*     DV_DEVICE_ENTRY   *link_name     Pointer to PPP device.
*
* OUTPUTS
*
*     UINT32                           Number of seconds since last IP
*                                      activity.
*
*************************************************************************/
INT32 PPP_Last_Activity_Time(CHAR *link_name)
{
    UINT32             clock;
    INT32              return_value;
    DV_DEVICE_ENTRY    *dev_ptr;
    LINK_LAYER         *link_layer;
    NU_SUPERV_USER_VARIABLES;

    /* switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Grab the semaphore. */
    if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Get the device for the link name. */
    dev_ptr = DEV_Get_Dev_By_Name(link_name);

    /* Make sure we got a device and that it is a PPP device. */
    if ((dev_ptr) && (dev_ptr->dev_type == DVT_PPP))
    {
        /* Get a pointer to the link_layer. */
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

        /* Get current system time and check it against the last time IP
           activity was detected through the PPP link.  The current time
           should always be greater than the last_activity_time variable's
           value.  If it isn't, the system time must have wrapped around so
           adjust the current time value. */
        clock = NU_Retrieve_Clock();
        if (clock < link_layer->last_activity_time)
            clock += (0xffffffffL - link_layer->last_activity_time);
        else
            clock -= link_layer->last_activity_time;

        /* Divide by TICKS_PER_SECOND to get the number of seconds; the last
           activity time is stored in ticks.There are TICKS_PER_SECOND ticks
           in one second; therefore, divide ticks by TICKS_PER_SECOND to get
           the seconds. */
        return_value = (INT32)(clock / TICKS_PER_SECOND);
    }
    else
        return_value = NU_INVALID_LINK;

    /* Release the semaphore. */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    return return_value;

} /* PPP_Last_Activity_Time */



/*************************************************************************
* FUNCTION
*
*     PPP_Reset
*
* DESCRIPTION
*
*     Initialize the LCP and NCP states before each new connection. On
*     return from this function, the initial default negotiation options
*     for this link will have been restored.
*
* INPUTS
*
*     DV_DEVICE_ENTRY   *dev_ptr        Pointer to the device structure
*                                        for this device.
*
* OUTPUTS
*
*     None.
*
*************************************************************************/
VOID PPP_Reset(DV_DEVICE_ENTRY *dev_ptr)
{
    LINK_LAYER              *link_layer;

    /* Grab a pointer to the LCP layer structure. */
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

    /* Zero the status variables for this link. */
    link_layer->status.silent_discards = 0;
    link_layer->status.address_field_errors = 0;
    link_layer->status.control_field_errors = 0;
    link_layer->status.overrun_errors = 0;
    link_layer->status.fcs_errors = 0;

    /* Clear the transmit queue. */
    MEM_Buffer_Cleanup(&dev_ptr->dev_transq);
    dev_ptr->dev_transq_length = 0;

    /* Reset the LCP layer. This will set all configuration options
       to their default values. */
    LCP_Reset(link_layer);

    /* Reset the NCP layer. This will set all configuration options
       to their default values. */
    NCP_Reset(link_layer);

#if (PPP_ENABLE_MPPE == NU_TRUE)
    /* Reset the CCP Layer. This will set the configuration options
       to their default values. */
    CCP_Reset(link_layer);
#endif
    /* Reset the MRU in this device in case it was changed during the
       last PPP session. */
    dev_ptr->dev_mtu = link_layer->lcp.options.local.default_mru;

} /* PPP_Reset */

#if (INCLUDE_IPV4 == NU_TRUE)
/*************************************************************************
* FUNCTION
*
*     PPP_Attach_IP_Address
*
* DESCRIPTION
*
*     After IPCP has negotiated successfully, this function is called
*     to assign the IPv4 addresses to the PPP device. It will also
*     add a route to the peer.
*
* INPUTS
*
*     *dev_ptr                  Pointer to the PPP device.
*
* OUTPUTS
*
*     STATUS
*
*************************************************************************/
STATUS PPP_Attach_IP_Address(DV_DEVICE_ENTRY *dev_ptr)
{
    LINK_LAYER *link = (LINK_LAYER*)dev_ptr->dev_link_layer;
    UINT8 *local;
    UINT8 *remote;
    UINT8 subnet[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    STATUS status;

    local = link->ncp.options.ipcp.local_address;
    remote = link->ncp.options.ipcp.remote_address;

    /* Attach the IP address to this device. */
    status = DEV_Attach_IP_To_Device(dev_ptr->dev_net_if_name, local, subnet);
    if (status == NU_SUCCESS)
    {
        /* Attach the server's IP address to this device. */
        dev_ptr->dev_addr.dev_dst_ip_addr = IP_ADDR(remote);

        /* Add a route to the peer. */
        status = RTAB_Add_Route(dev_ptr, IP_ADDR(remote), IP_ADDR(subnet),
                        IP_ADDR(local), (RT_UP | RT_HOST | RT_STATIC));
    }

    return status;
}


#if (INCLUDE_DNS == NU_TRUE)
/*************************************************************************
* FUNCTION
*
*     PPP_Attach_DNS_Address
*
* DESCRIPTION
*
*     After IPCP has negotiated successfully, this function is called
*     to assign the IPv4 DNS addresses to be used for name resolution.
*     Routes are also added to the DNS servers through the PPP peer.
*
* INPUTS
*
*     *link
*     dnsid
*
* OUTPUTS
*
*     STATUS
*
*************************************************************************/
STATUS PPP_Attach_DNS_Address(LINK_LAYER *link, UINT8 dnsid)
{
    UINT8     *dns = NU_NULL;
    UINT8     *gw = link->ncp.options.ipcp.remote_address;
    UINT8     subnet[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    STATUS    status = NU_SUCCESS;

    if (dnsid == 1)
        dns = link->ncp.options.ipcp.primary_dns_server;
    else if (dnsid == 2)
        dns = link->ncp.options.ipcp.secondary_dns_server;

    if (dns)
    {
        /* Add route to DNS server. */
        RTAB_Add_Route(link->hwi.dev_ptr, IP_ADDR(dns), IP_ADDR(subnet),
                       IP_ADDR(gw), (RT_UP | RT_HOST | RT_STATIC));

        /* Avoid multiple entries of the same DNS server. */
        NU_Delete_DNS_Server(dns);

        /* Now add the DNS server to our DNS server search list. */
        NU_Add_DNS_Server(dns, DNS_ADD_TO_END);
    }

    else
    {
        status = -1;
    }

    return (status);
}
#endif
#endif  /* (INCLUDE_IPV4 == NU_TRUE) */


/*************************************************************************
* FUNCTION
*
*     PPP_Protocol_Reject
*
* DESCRIPTION
*
*     Sends a protocol reject packet to the peer and cleans up the
*     buffers used.
*
* INPUTS
*
*     *buf_ptr
*
* OUTPUTS
*
*     None
*
*************************************************************************/
VOID PPP_Protocol_Reject(NET_BUFFER *buf_ptr)
{
    DV_DEVICE_ENTRY     *dev_ptr;
    LINK_LAYER          *link_layer;
    LCP_LAYER           *lcp;

    /* Get a pointer to the various PPP layers. */
    dev_ptr = buf_ptr->mem_buf_device;
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
    lcp = &link_layer->lcp;

    if (lcp->state == OPENED)
    {
        /* Send a protocol reject packet via LCP. */
        LCP_Send_Protocol_Reject(buf_ptr);
    }
    else
    {
        /* Bump the number of silent discards and free the buffers */
        PML_SilentDiscards_Inc(dev_ptr);
    }

    /* If using SNMP, this will update the MIB */
    MIB2_ifInUnknownProtos_Inc(dev_ptr);

    /* Release the buffer. */
    MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);
}



/*************************************************************************
* FUNCTION
*
*     PPP_Add_Protocol
*
* DESCRIPTION
*
*     Adds the 2-byte PPP protocol header to an outgoing packet.
*
* INPUTS
*
*     *buf_ptr
*     protocol
*
* OUTPUTS
*
*     None
*
*************************************************************************/
VOID PPP_Add_Protocol(NET_BUFFER *buf_ptr, UINT16 protocol)
{
    /* Move the data pointer back 2 bytes for the protocol id
       and adjust the lengths. */
    buf_ptr->data_ptr           -= PPP_PROTOCOL_HEADER_2BYTES;
    buf_ptr->data_len           += PPP_PROTOCOL_HEADER_2BYTES;
    buf_ptr->mem_total_data_len += PPP_PROTOCOL_HEADER_2BYTES;

    /* Add the protocol type. */
    PUT16(buf_ptr->data_ptr, 0, protocol);
}



#if (PPP_LOG_PACKET == NU_TRUE)
/*************************************************************************
* FUNCTION
*
*     PPP_PrintPkt
*
* DESCRIPTION
*
*     Print the contents of a PPP negotiation packet to a display,
*     to memory, or to a file.
*
* INPUTS
*
*     *ptr
*     len
*     *header
*
* OUTPUTS
*
*     None
*
*************************************************************************/
VOID PPP_PrintPkt(UINT8 *ptr, UINT32 len, CHAR *header)
{
    UINT32 i;

#if (PPP_LOG_TO_FILE == NU_TRUE)
    FILE *fp;

    fp = fopen(PPP_LOG_FILENAME, "a");
    if (fp == NULL)
    {
        printf("*********** FILE ERROR ***********\n\n\n");
        return;
    }
#endif

    PPP_Printf("%s", header);
#if (PPP_LOG_TO_FILE == NU_TRUE)
    fprintf(fp, "%s", header);
#endif

    /* Write a line of 16 bytes, or up to the length of the input data. */
    for (i = 0; i < len; i++)
    {
        if (i % 16 == 0)
        {
            /* Drop to the next line. */
            PPP_Printf("\n    ");
#if (PPP_LOG_TO_FILE == NU_TRUE)
            fprintf(fp, "\n    ");
#endif
        }

        /* Print one byte in hex format. */
        PPP_Printf("%.02x ", ptr[(INT)i]);
#if (PPP_LOG_TO_FILE == NU_TRUE)
        fprintf(fp, "%.02x ", ptr[i]);
#endif
    }

    PPP_Printf("\n\n");
#if (PPP_LOG_TO_FILE == NU_TRUE)
    fprintf(fp, "\n\n");
    fclose(fp);
#endif
}


#endif



/*************************************************************************
* FUNCTION
*
*     PPP_Ioctl
*
* DESCRIPTION
*
*     The dev_ioctl function for a PPP device. In IPv6, PPP is
*     a multicast device, but there is no hardware multicasting, so this
*     function always returns NU_SUCCESS.
*
* INPUTS
*
*     *dev                      Device pointer.
*     option                    Option to specify the operation.
*     *d_req                    Unused argument.
*
* OUTPUTS
*
*     NU_SUCCESS
*
*************************************************************************/
STATUS PPP_Ioctl(DV_DEVICE_ENTRY *dev, INT option, DV_REQ *d_req)
{
#ifndef DEV_REMDEV
    UNUSED_PARAMETER(dev);
    UNUSED_PARAMETER(option);
    UNUSED_PARAMETER(d_req);
#else
    /* Declare Variables. */
#if CFG_NU_OS_DRVR_PPP_ENABLE_SERIAL
    INT del_rx_hisr = 0;
    INT del_tx_hisr = 0;
#endif

    LINK_LAYER *link_layer = (LINK_LAYER *)dev->dev_link_layer;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    DV_DEV_LABEL    *dev_label = (DV_DEV_LABEL *)dev->dev_driver_options;
    DV_DEV_ID       device_id;
    INT             dev_count = CFG_NU_OS_DRVR_SERIAL_MAX_DEVS_SUPPORTED;
#endif

    /* Remove warnings. */
    UNUSED_PARAMETER(d_req);

    /* If the device is going to be removed. */
    if(option == DEV_REMDEV)
    {
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
        /* Get the PPP device ID */
        if ((DVC_Dev_ID_Get(dev_label, 1, &device_id, &dev_count) == NU_SUCCESS) &&
            (dev_count > 0))
        {
            /* Make sure we reinstate the power state of the
             * Serial driver.
             */
            if (NU_PM_Min_Power_State_Release(device_id,
                                              link_layer->ppp_pm_handle)
               != NU_SUCCESS)
            {
                NLOG_Error_Log("PPP_Ioctl: Failed to Reinstate the Power state.",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }
#endif

#if CFG_NU_OS_DRVR_PPP_ENABLE_SERIAL
  
        if((dev->dev_driver_options) != NU_NULL)
        {
            if(NU_Deallocate_Memory((VOID *)dev->dev_driver_options) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to deallocate memory.",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }
            else
            {
                dev->dev_driver_options = NU_NULL;
            }
        }
        if(NU_Terminate_Task(&(link_layer->serial_rx_task)) != NU_SUCCESS)
        {
            NLOG_Error_Log("PPP_Ioctl: Failed to terminate the serial receive task.",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
        else
        {
            if(NU_Delete_Task(&(link_layer->serial_rx_task)) != NU_SUCCESS)
            {
                NLOG_Error_Log("PPP_Ioctl: Failed to delete the serial receive task.",
                                NERR_RECOVERABLE, __FILE__, __LINE__);
            }
            else
            {
                if(NU_Serial_Close(link_layer->uart) != NU_SUCCESS)
                {
                    NLOG_Error_Log("PPP_Ioctl: Failed to close the serial device.",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }

                /* Open the port */
                if(NU_Deallocate_Memory(link_layer->ppp_serial_rx_task_mem) != NU_SUCCESS)
                {
                    NLOG_Error_Log("PPP_Ioctl: Failed to deallocate memory.",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }
        }
#if (HDLC_POLLED_TX == NU_FALSE)
        if(NU_Terminate_Task(&(link_layer->serial_tx_task)) != NU_SUCCESS)
        {
                NLOG_Error_Log("PPP_Ioctl: Failed to terminate the serial transmit task.",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
        }
        else
        {
            if(NU_Delete_Task(&(link_layer->serial_tx_task)) != NU_SUCCESS)
            {
                NLOG_Error_Log("PPP_Ioctl: Failed to delete the serial transmit task.",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
            else
            {
                if(NU_Deallocate_Memory(link_layer->ppp_serial_tx_task_mem) != NU_SUCCESS)
                {
                    NLOG_Error_Log("PPP_Ioctl: Failed to deallocate memory.",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }
        }
#endif
        if(PPP_Open_Count == 1)
        {
            if(NU_Delete_HISR(&PPP_RX_HISR) == NU_SUCCESS)
            {
                if(NU_Deallocate_Memory(HDLC_RX_HISR_Mem) != NU_SUCCESS) 
                {
                    NLOG_Error_Log("PPP_Ioctl: Failed to deallocate memory.",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
                else
                {
                    del_rx_hisr = 1;
                }
            }
            else
            {
                NLOG_Error_Log("PPP_Ioctl: Failed to delete the PPP receive HISR.",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }  
#if (HDLC_POLLED_TX == NU_FALSE)
            if(NU_Delete_HISR(&PPP_TX_HISR) == NU_SUCCESS)
            {
                if(NU_Deallocate_Memory(HDLC_TX_HISR_Mem) != NU_SUCCESS) 
                {
                    NLOG_Error_Log("PPP_Ioctl: Failed to deallocate memory.",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
                else
                {
                    del_tx_hisr = 1;
                }
            }
            else
            {
                NLOG_Error_Log("PPP_Ioctl: Failed to delete the PPP transmit HISR.",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
#endif
            if(del_rx_hisr == 1 || del_tx_hisr == 1)
            {
                PPP_Open_Count = 0;
            }
        }
        else
        {
            PPP_Open_Count--;
        }

#endif /* CFG_NU_OS_DRVR_PPP_ENABLE_SERIAL */

        if(NU_Terminate_Task(&(link_layer->ppp_task_cb)) != NU_SUCCESS)
        {
            NLOG_Error_Log("PPP_Ioctl: Failed to Terminate the PPP task.",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
        else
        {
            if(NU_Delete_Task(&(link_layer->ppp_task_cb)) != NU_SUCCESS)
            {
                NLOG_Error_Log("PPP_Ioctl: Failed to delete the PPP task.",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
            else
            {
                if(NU_Delete_Queue(&(link_layer->ppp_event_queue)) != NU_SUCCESS)
                {
                    NLOG_Error_Log("PPP_Ioctl: Failed to delete queue.",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
#if (HDLC_POLLED_TX == NU_FALSE)
                if(NU_Delete_Event_Group(&(link_layer->ppp_tx_event)) != NU_SUCCESS)
                {
                    NLOG_Error_Log("PPP_Ioctl:Failed to delete event group",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
#endif
                if(NU_Deallocate_Memory(link_layer->ppp_init_mem) != NU_SUCCESS)
                {
                    NLOG_Error_Log("PPP_Ioctl:Failed to deallocate memory.",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }
        }

        if(link_layer->rx_ring != NU_NULL)
        {
            /* Deallocate memory used by serial buffers of the device. */
            if(NU_Deallocate_Memory(link_layer->rx_ring) != NU_SUCCESS)
            {
                NLOG_Error_Log("PPP_Ioctl: Failed to deallocate memory.",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        /* If the MDM layer structure is allocated. */
        if(link_layer->link != NU_NULL)
        {
            /* If the receive buffer in the MDM layer is allocated. */
            if(((MDM_LAYER *)link_layer->link)->recv_buffer.mdm_head != NU_NULL)
            {
                /* Deallocate a block of memory for the receive buffer. */
                if(NU_Deallocate_Memory(((MDM_LAYER *)link_layer->link)->recv_buffer.mdm_head)
                    != NU_SUCCESS)
                {
                    NLOG_Error_Log("PPP_Ioctl: Failed to deallocate memory.",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }

            /* Deallocate memory for the MDM layer structure. */
            if(NU_Deallocate_Memory(link_layer->link) != NU_SUCCESS)
            {
                NLOG_Error_Log("PPP_Ioctl: Failed to deallocate memory.",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        /* Check if this device was being used with L2TP */
        if(link_layer->hwi.itype & (PPP_ITYPE_L2TP_LAC | PPP_ITYPE_L2TP_LNS))
        {
            /* Deallocate memory used by remote initial configuration request. */
            if(NU_Deallocate_Memory(link_layer->lcp.options.remote.init_cfg_req)
                != NU_SUCCESS)
            {
                NLOG_Error_Log("PPP_Ioctl: Failed to deallocate memory.",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            /* Deallocate memory used by remote last configuration request. */
            if(NU_Deallocate_Memory(link_layer->lcp.options.remote.last_cfg_req)
                != NU_SUCCESS)
            {
                NLOG_Error_Log("PPP_Ioctl: Failed to deallocate memory.",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            /* Deallocate memory used by local initial configuration request. */
            if(NU_Deallocate_Memory(link_layer->lcp.options.local.last_cfg_req)
                != NU_SUCCESS)
            {
                NLOG_Error_Log("PPP_Ioctl: Failed to deallocate memory.",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            /* Deallocate memory used by chap extension. */
            if(NU_Deallocate_Memory(link_layer->authentication.chap.chap_ext)
                != NU_SUCCESS)
            {
                NLOG_Error_Log("PPP_Ioctl: Failed to deallocate memory.",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        /* Deallocate memory for the PPP layer structure */
        if(NU_Deallocate_Memory(dev->dev_link_layer) != NU_SUCCESS)
        {
            NLOG_Error_Log("PPP_Ioctl: Failed to deallocate memory.",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* Unregister the main PPP event handler . */
        if(EQ_Unregister_Event(PPP_Event) != NU_SUCCESS)
        {
            NLOG_Error_Log("PPP_Ioctl: Failed to Un-register the PPP event.",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }
#endif

    return (NU_SUCCESS);

} /* PPP_Ioctl */

