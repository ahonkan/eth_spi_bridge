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
*   FILE NAME                                                              
*                                                                               
*       mld6.c                                       
*                                                                       
*   DESCRIPTION                                                           
*              
*       This file contains those functions specific to Multicast Listener
*       Discovery Version 2 Internet Draft version 7.
*
*       The purpose of Multicast Listener Discovery is to enable each 
*       IPv6 router to discover the presence of multicast listeners 
*       (that is, nodes wishing to receive multicast packets) on its 
*       directly attached links, and to discover specifically which 
*       multicast addresses are of interest to those neighboring nodes.  
*       This information is then provided to whichever multicast routing 
*       protocol is being used by the router in order to ensure that 
*       multicast packets are delivered to all links where there are 
*       interested receivers.
*                           
*                                            
*   FUNCTIONS                                                             
*             
*       MLD6_Start_Listening 
*       MLD6_Send_Startup_Report
*       MLD6_Stop_Listening_Addr
*       MLD6_Stop_Listening_Multi
*       MLD6_Timer_Expired
*       MLD6_Input
*       MLD6_Output
*       MLD6_Build_Packet
*       MLD6_Header_Init
*       MLD6_Calculate_Max_Response_Delay
*       MLD6_Update_Host_Compatibility
*                                                                       
*   DEPENDENCIES                                                          
*                                                                    
*       externs.h
*       mld6.h
*       externs6.h   
*                                                                       
*************************************************************************/

#include "networking/externs.h"
#include "networking/mld6.h"
#include "networking/externs6.h"

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)

UNSIGNED    Multicast_MLDv1_Timer_Index = MLDV1_MIN_TMR_INDEX;
#if (MLD6_DEFAULT_COMPATIBILTY_MODE == MLDV2_COMPATIBILITY)
UNSIGNED    Multicast_MLDv2_Mode_Include_Timer_Index = 
                                            MLDV2_MODE_INC_MIN_TMR_INDEX;
UNSIGNED    Multicast_MLDv2_Mode_Exclude_Timer_Index = 
                                            MLDV2_MODE_EXC_MIN_TMR_INDEX;
UNSIGNED    Multicast_MLDv2_Change_To_Include_Timer_Index = 
                                            MLDV2_CHANGE_TO_INC_MIN_TMR_INDEX;
UNSIGNED    Multicast_MLDv2_Change_To_Exclude_Timer_Index = 
                                            MLDV2_CHANGE_TO_EXC_MIN_TMR_INDEX;
UNSIGNED    Multicast_MLDv2_Allow_New_Sources_Timer_Index = 
                                            MLDV2_ALLOW_NEW_SRCS_MIN_TMR_INDEX; 
UNSIGNED    Multicast_MLDv2_Block_Old_Sources_Timer_Index = 
                                            MLDV2_BLOCK_OLD_SRCS_MIN_TMR_INDEX;
#endif

static  TQ_EVENT    MLD6_Send_Startup_Report_Event;

#if (MLD6_DEFAULT_COMPATIBILTY_MODE == MLDV2_COMPATIBILITY)
TQ_EVENT    MLDv1_Compatibility_Timer_Event;
#endif

/* IP Address to which all MLDv2 reports are sent and all MLDv2-capable
 *  routers listen. 
 */
UINT8       IP6_MLDv2_All_Routers_Multi[16] = {0xFF, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x16};

extern  TQ_EVENT    Multicast_General_Query_Timer_Event;
extern  TQ_EVENT    Multicast_Address_Specific_Timer_Event;
extern UINT8    IP6_All_Routers_Multi[];
extern UINT8    IP6_All_Nodes_Multi[];

STATIC  NET_BUFFER *MLD6_Header_Init(UINT32 size);

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       MLD6_Start_Listening
*                                                                         
*   DESCRIPTION                                                           
*
*       This function invokes Multicast Listener Discovery for a given
*       Multicast Address.
*
*   INPUTS                                                                
*                                              
*       *ip6_multi              A pointer to the Multicast data structure
*                               for which to start listening
*
*       message_to_send         Type of message that is to be sent
*                                                                         
*   OUTPUTS                                                               
*                                           
*       None
*
*************************************************************************/
VOID MLD6_Start_Listening(IP6_MULTI *ip6_multi, UINT8 message_to_send)
{
    STATUS              ret_status;
    MULTI_DEV_STATE     *mld_dev_state;
    
    /* The Link-Local All-Nodes Multicast address starts in Idle Listener 
     * state for that address on every interface, never transitions to 
     * another state, and never sends a Report or Done for that address.
     */
    if (memcmp(ip6_multi->ipm6_addr, IP6_All_Nodes_Multi, IP6_ADDR_LEN) == 0)
        ip6_multi->ipm6_data.multi_state = MLD6_IDLE_LISTENER;

    /* If the address is not node-local, join the group and send a 
     * Report.
     */
    else if (!(IPV6_IS_ADDR_MC_NODELOCAL(ip6_multi->ipm6_addr)))
    {
        /* Set the state to Delaying Listener */
        ip6_multi->ipm6_data.multi_state = MLD6_DELAYING_LISTENER;

        /* Send the report */
        if (ip6_multi->ipm6_data.multi_device->dev6_mld_compat_mode == 
            MLDV1_COMPATIBILITY)
            message_to_send = MLDV1_LISTENER_REPORT;


        if (message_to_send == MLDV1_LISTENER_REPORT)
            ret_status = 
                MLD6_Output(ip6_multi, message_to_send, 
                            ip6_multi->ipm6_addr, 0);

        else
            ret_status = 
                MLD6_Output(ip6_multi, message_to_send, 
                            IP6_MLDv2_All_Routers_Multi, 0);

        if (ret_status != NU_SUCCESS)
            NLOG_Error_Log("Send the MLD report", NERR_SEVERE, 
                           __FILE__, __LINE__);

        /* Initialize the number of startup packets transmitted to 1 */
        ip6_multi->ipm6_data.multi_startup_count = 1;

        /* If another report should be sent, set the timer to transmit 
         * another report.
         */
        if (ip6_multi->ipm6_data.multi_startup_count < 
            MLD6_STARTUP_QUERY_COUNT)
        {
            /* Generate a random timer value */
            ip6_multi->ipm6_data.multi_timer = 
                MULTI_RANDOM_DELAY(MLD6_UNSOLICITED_REPORT_INTERVAL);

            /* Get the device filter state */
            mld_dev_state = 
                Multi_Get_Device_State_Struct(ip6_multi->ipm6_data.multi_device, 
                                              ip6_multi->ipm6_addr, 
                                              IP6_ADDR_LEN);

            /* Set the timer index */
            ip6_multi->ipm6_data.multi_timer_index = 
                  Multi_Get_Timer_Index(ip6_multi->ipm6_data.multi_device, 
                                        mld_dev_state, MULTICAST_TYPE_MLD, 
                                        message_to_send);

            /* Start the timer */
            if (TQ_Timerset(Multicast_General_Query_Timer_Event, 
                 (UNSIGNED)(ip6_multi->ipm6_data.multi_device->dev_index),
                            ip6_multi->ipm6_data.multi_timer, 
                            ip6_multi->ipm6_data.multi_timer_index) != NU_SUCCESS)
                NLOG_Error_Log("Failed to start the MLD timer", NERR_SEVERE, 
                               __FILE__, __LINE__);
        }
    }
    else
        NLOG_Error_Log("Multicast Address cannot participate in MLD", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);

} /* MLD6_Start_Listening */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       MLD6_Send_Startup_Report
*                                                                         
*   DESCRIPTION                                                           
*
*       This function is called when the startup_timer associated
*       with an multicast address expires.  When a device joins a
*       multicast group, it transmits MLD6_STARTUP_QUERY_COUNT
*       reports each MLD6_STARTUP_QUERY_INTERVAL seconds apart.
*
*   INPUTS                                                                
*                             
*       event                   The event being handled.       
*       mld_index               The index of the multicast address in 
*                               the list.
*       dev_index               The index of the device on which the 
*                               multicast address is located.
*                                                                         
*   OUTPUTS                                                               
*                                   
*       None.        
*
*************************************************************************/
VOID MLD6_Send_Startup_Report(TQ_EVENT event, UNSIGNED mld_index, 
                              UNSIGNED dev_index)
{
    DV_DEVICE_ENTRY     *device;
    IP6_MULTI           *ip6_multi;
    MULTI_DEV_STATE     *mld_dev_state;
    STATUS              ret_status;

    UNUSED_PARAMETER(event);

    device = DEV_Get_Dev_By_Index((UINT32)dev_index);

    if (device)
    {
        /* Get a pointer to the first Multicast address in the list */
        ip6_multi = device->dev6_multiaddrs;
    
        /* Traverse the list of Multicast addresses searching for the
         * matching data structure that triggered the event.
         */
        while (ip6_multi)
        {
            /* If this is the target, break */
            if (ip6_multi->ipm6_data.multi_startup_index == mld_index)
                break;

            /* Otherwise, get the next Multicast address in the list */
            ip6_multi = ip6_multi->ipm6_next;
        }

        /* If the target entry was found */
        if (ip6_multi)
        {
            /* Send the report */
            if (device->dev6_mld_compat_mode == MLDV1_COMPATIBILITY)
                ret_status = MLD6_Output(ip6_multi, MLDV1_LISTENER_REPORT, 
                                         ip6_multi->ipm6_addr, 0);

            else
                ret_status = MLD6_Output(ip6_multi, MLDV2_LISTENER_REPORT, 
                                         IP6_MLDv2_All_Routers_Multi, 0);

            /* Log an error message */
            if (ret_status != NU_SUCCESS)
                NLOG_Error_Log("Failed to send the MLD report", 
                               NERR_SEVERE, __FILE__, __LINE__);

            /* Increment the number of startup packets transmitted */
            ip6_multi->ipm6_data.multi_startup_count++;
    
            /* If another report should be sent, set the timer to transmit 
             * another report.
             */
            if (ip6_multi->ipm6_data.multi_startup_count < MLD6_STARTUP_QUERY_COUNT)
            {

                /* Get the device filter state */
                mld_dev_state = 
                    Multi_Get_Device_State_Struct(ip6_multi->ipm6_data.multi_device, 
                                                  ip6_multi->ipm6_addr, 
                                                  IP6_ADDR_LEN);
                
                /* Set the timer index */
                ip6_multi->ipm6_data.multi_timer_index = 
                    Multi_Get_Timer_Index(ip6_multi->ipm6_data.multi_device, 
                                          mld_dev_state, MULTICAST_TYPE_MLD, 
                                          NU_NULL);

                if (TQ_Timerset(MLD6_Send_Startup_Report_Event, 
                    (UNSIGNED)(ip6_multi->ipm6_data.multi_device->dev_index),
                                MLD6_STARTUP_QUERY_INTERVAL, 
                                ip6_multi->ipm6_data.multi_startup_index) != 
                                NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to set the MLD timer", 
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
            }
        }
    }

} /* MLD6_Send_Startup_Report */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       MLD6_Stop_Listening_Addr
*                                                                         
*   DESCRIPTION                                                           
*
*       This function halts Multicast Listener Discovery for a given
*       Multicast Address on the specified interface or for all interfaces
*       if no interface is specified.
*
*   INPUTS                                                                
*                                    
*       *multi_address          The Multicast Address for which to halt
*                               Multicast Listener Discovery.
*       *device                 The device for which to halt Multicast 
*                               Listener Discovery or NU_NULL for all
*                               devices.
*                                                                         
*   OUTPUTS                                                               
*                                   
*       None.        
*
*************************************************************************/
VOID MLD6_Stop_Listening_Addr(const UINT8 *multi_address, 
                              const DV_DEVICE_ENTRY *device)
{
    IP6_MULTI   *ip6_multi;

    /* If a device was passed in, find the Multicast Address data structure
     * for that interface and halt Multicast Listener Discovery.
     */
    if (device != NU_NULL)
    {
        ip6_multi = IP6_Lookup_Multi(multi_address, device->dev6_multiaddrs);

        if (ip6_multi != NU_NULL)
            MLD6_Stop_Listening_Multi(ip6_multi);
    }

    /* If a device was not passed in, halt Multicast Listener Discovery
     * for the Multicast address on each interface in the system.
     */
    else
    {
        /* Get a pointer to the first interface in the list */
        device = DEV_Table.dv_head;
    
        /* Traverse the list and halt Multicast Listener Discovery */
        while (device)
        {
            if (device->dev_flags & DV6_IPV6)
            {
                /* Get a pointer to the Multicast data structure */
                ip6_multi = IP6_Lookup_Multi(multi_address, 
                                             device->dev6_multiaddrs);

                if (ip6_multi != NU_NULL)
                    MLD6_Stop_Listening_Multi(ip6_multi);
            }

            device = device->dev_next;
        }
    }

} /* MLD6_Stop_Listening_Addr */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       MLD6_Stop_Listening_Multi
*                                                                         
*   DESCRIPTION                                                           
*
*       This function halts Multicast Listener Discovery for a given
*       Multicast data structure.
*
*   INPUTS                                                                
*                                    
*       *ip6_multi              A pointer to the Multicast data structure 
*                               for which to halt Multicast Listener 
*                               Discovery.
*                                                                         
*   OUTPUTS                                                               
*                                   
*       None.        
*
*************************************************************************/
VOID MLD6_Stop_Listening_Multi(IP6_MULTI *ip6_multi)
{
    /* If the current state is DELAYING_LISTENER, stop the timer */
    if (ip6_multi->ipm6_data.multi_state == MLD6_DELAYING_LISTENER)
    {
        if (TQ_Timerunset(Multicast_General_Query_Timer_Event, 
                          TQ_CLEAR_EXACT, (UNSIGNED)(ip6_multi->ipm6_data.
                          multi_device->dev_index),
                          ip6_multi->ipm6_data.multi_timer_index) != 
                          NU_SUCCESS)
            NLOG_Error_Log("Failed to stop the MLD timer", 
                           NERR_SEVERE, __FILE__, __LINE__);

        if (TQ_Timerunset(Multicast_Address_Specific_Timer_Event, 
                          TQ_CLEAR_EXACT, (UNSIGNED)(ip6_multi->ipm6_data.
                          multi_device->dev_index), 
                          ip6_multi->ipm6_data.multi_timer_index) != 
                          NU_SUCCESS)
            NLOG_Error_Log("Failed to stop the MLD timer", 
                           NERR_SEVERE, __FILE__, __LINE__);
    }

    /* Change the state to the Non-Listening mode */
    ip6_multi->ipm6_data.multi_state = MLD6_NON_LISTENER;

    /* Send a done report to all Routers.  If the flag saying we were the
     * last node to report is cleared, this action may be skipped.
     */
    if (ip6_multi->ipm6_data.multi_device->dev6_mld_compat_mode == 
        MLDV1_COMPATIBILITY)
    {
#if (MLD6_OPTIMIZE_DONE_REPORTS == NU_TRUE)
        if (ip6_multi->ipm6_data.multi_sent_last_report == NU_TRUE)
#endif
        {
            if (MLD6_Output(ip6_multi, MLD6_LISTENER_DONE, 
                            IP6_All_Routers_Multi, 0) != NU_SUCCESS)
                NLOG_Error_Log("Failed to send the MLD report", 
                               NERR_SEVERE, __FILE__, __LINE__);
        }
    }

} /* MLD6_Stop_Listening_Multi */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       MLD6_Input
*                                                                         
*   DESCRIPTION                                                           
*
*       This function processes an incoming MLD message accordingly.
*
*   INPUTS                                                                
*                                                 
*       *pkt                    A pointer to the IPv6 Header.
*       *device                 A pointer to the device on which the 
*                               message was received.
*       *buf_ptr                A pointer to the buffer.
*                                                                         
*   OUTPUTS                                                               
*                                           
*       0
*
*************************************************************************/
STATUS MLD6_Input(const IP6LAYER *pkt, DV_DEVICE_ENTRY *device, 
                  const NET_BUFFER *buf_ptr)
{
    STATUS      status;

    switch (buf_ptr->data_ptr[MULTICAST_MESSAGE_TYPE_OFFSET])
    {
        case MLD6_LISTENER_QUERY:

            MIB_ipv6IfIcmpInGrpMemQur_Inc(device);
            break;

        case MLDV1_LISTENER_REPORT:
        case MLDV2_LISTENER_REPORT:

            MIB_ipv6IfIcmpInGrpMemRes_Inc(device);
            break;

        case MLD6_LISTENER_DONE:

            MIB_ipv6IfIcmpInGrpMemRed_Inc(device);
            break;

        default:

            break;
    }

    /* The MLD message must originate from a link-local address */
    if (IPV6_IS_ADDR_LINKLOCAL(pkt->ip6_src))
    {
        /* The MLD message must be at least MLD6_MIN_PKT_LENGTH octets long */
        if (buf_ptr->mem_total_data_len >= MLD6_MIN_PKT_LENGTH)
        {
            /* Call the central Multicast input function */
            status = Multi_Input(buf_ptr, device, MLD6_MULTI_ADDR_OFFSET);
        }            
        
        else
        {
            MIB_ipv6IfIcmpInErrors_Inc(device);

            NLOG_Error_Log("MLD message received with invalid length", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);
            
            status = NU_INVAL;
        }
    }
    else
    {
        MIB_ipv6IfIcmpInErrors_Inc(device);

        NLOG_Error_Log("MLD message received with invalid Source address", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);

        status = NU_INVAL;
    }

    return (status);

} /* MLD6_Input */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       MLD6_Output
*                                                                         
*   DESCRIPTION                                                           
*
*       This function generates and transmits an MLD message.
*
*   INPUTS                                                                
*                          
*       *ip6_multi              A pointer to the IPv6 Multicast data
*                               structure.
*       message_to_send         The type of MLD message to send:
*
*       *dest_addr              A pointer to the Destination address of
*                               the MLD message.
*       max_response_delay      The Maximum Response Delay field is
*                               meaningful only in Query messages, and
*                               specifies the maximum allowed delay
*                               before sending a responding Report, in
*                               units of milliseconds.  In all other
*                               messages, it is set to zero by the 
*                               sender and ignored by receivers.                       
*                                                                         
*   OUTPUTS                                                               
*                                           
*       NU_INVALID_ADDRESS      A link-local address does not exist for
*                               the device.
*       NU_NO_BUFFERS           Insufficient NET Buffers.
*       NU_HOST_UNREACHABLE     A route to the host does not exist.
*       NU_MSGSIZE              The size of the message is wrong.
*
*************************************************************************/
STATUS MLD6_Output(IP6_MULTI *ip6_multi, UINT8 message_to_send, 
                   UINT8 *dest_addr, UINT16 max_response_delay)
{
    DEV6_IF_ADDRESS     *source_address;
    UINT16              checksum;
    NET_BUFFER          *buf_ptr;
    STATUS              status;
    MULTI_SCK_OPTIONS   multi_opts;
    IP6_S_OPTIONS       ip6_options;

    /* All MLD messages are sent with a link-local IPv6 Source Address
     */
    source_address = IP6_Find_Link_Local_Addr(ip6_multi->
                                              ipm6_data.multi_device);

    /* If the interface does not have a link-local source address,
     * it cannot participate in MLD.
     */
    if (source_address)
    {
        /* Build the message */
        buf_ptr = MLD6_Build_Packet(ip6_multi, message_to_send, 
                                    max_response_delay, 
                                    ip6_multi->ipm6_addr);

        /* Initialize the IP6_S_OPTIONS structure */
        memset(&ip6_options, 0, sizeof(IP6_S_OPTIONS));
        
        ip6_options.tx_source_address = source_address->dev6_ip_addr;
        ip6_options.tx_dest_address = dest_addr;
        ip6_options.tx_hop_limit = MLD6_HOP_LIMIT;

        if (buf_ptr != NU_NULL)
        {
            /* If we are about to send a MLD report, then set the last 
             * report flag, so that we will know to send a DONE message 
             * when we leave the group
             */
            if ( (message_to_send != MLD6_LISTENER_DONE) &&
                 (message_to_send != MLD6_LISTENER_QUERY) )
                ip6_multi->ipm6_data.multi_sent_last_report = NU_TRUE;
                    
            /* Compute the Checksum */
            checksum = UTL6_Checksum(buf_ptr, source_address->dev6_ip_addr, 
                                     dest_addr, buf_ptr->mem_total_data_len, 
                                     IPPROTO_ICMPV6, IPPROTO_ICMPV6);


            PUT16(buf_ptr->data_ptr, IP6_ICMP_CKSUM_OFFSET, checksum);
            
            /* Initialize the options that will be passed to IP6_Send */
            UTL_Zero(&multi_opts, sizeof(MULTI_SCK_OPTIONS));
            multi_opts.multio_device = ip6_multi->ipm6_data.multi_device;
            multi_opts.multio_ttl = 1;
            multi_opts.multio_loop = 0;            

            buf_ptr->data_ptr -= MLD6_ROUTER_ALERT_LENGTH;

            buf_ptr->mem_total_data_len += MLD6_ROUTER_ALERT_LENGTH;
            buf_ptr->data_len += MLD6_ROUTER_ALERT_LENGTH;

            /* Increment the number of ICMP messages sent. */
            MIB_ipv6IfIcmpOutMsgs_Inc(multi_opts.multio_device);

            status = IP6_Send(buf_ptr, &ip6_options, IP6_HOP_BY_HOP, 
                              NU_NULL, NU_NULL, &multi_opts);

            if (status != NU_SUCCESS)
            {
                /* Increment the number of send errors. */
                MIB_ipv6IfIcmpOutErrors_Inc(multi_opts.multio_device);

                NLOG_Error_Log("MLD message not sent", NERR_SEVERE, 
                               __FILE__, __LINE__);

                /* The message was not sent.  Deallocate the buffer.  If the 
                 * message was transmitted it will be deallocated when the 
                 * transmit complete interrupt occurs. 
                 */
                MEM_One_Buffer_Chain_Free(buf_ptr, buf_ptr->mem_dlist);
            }

            else
            {
                switch (message_to_send)
                {
                    case MLD6_LISTENER_DONE:

                        MIB_ipv6IfIcmpOutGrpMemRed_Inc(multi_opts.multio_device);
                        break;

                    case MLD6_LISTENER_QUERY:

                        MIB_ipv6IfIcmpOutGrpMemQur_Inc(multi_opts.multio_device);
                        break;

                    case MLDV1_LISTENER_REPORT:
                    case MLDV2_LISTENER_REPORT:

                        MIB_ipv6IfIcmpOutGrpMemRes_Inc(multi_opts.multio_device);
                        break;

                    default:

                        break;
                }
            }
        }
        else
        {
            /* Increment the number of send errors. */
            MIB_ipv6IfIcmpOutErrors_Inc(ip6_multi->ipm6_data.multi_device);

            NLOG_Error_Log("MLD message could not be built", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);

            status = NU_NO_BUFFERS;
        }
    }
    else
    {
        NLOG_Error_Log("MLD message not sent to due no valid link-local address on the interface", 
                       NERR_SEVERE,  __FILE__, __LINE__);
        status = NU_INVALID_ADDRESS;
    }

    return (status);

} /* MLD6_Output */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       MLD6_Build_Packet
*                                                                         
*   DESCRIPTION                                                           
*
*       This function builds an MLD message of the specified type.
*
*   INPUTS                                                                
*                                                 
*       *ip6_multi              A ptr to the IPv6 Multicast data
*                               structure.
*       message_to_send         The type of MLD message to build.
*       max_resp_delay          The Maximum Response Delay to put in 
*                               the packet.  This value is only set for 
*                               Query messages.
*       *multi_addrs            A pointer to the multicast address to put 
*                               in the Multicast Address field of the 
*                               message.
*                                                                         
*   OUTPUTS                                                               
*                                           
*       If successful, a pointer to the new NET_BUFFER.
*       If unsuccessful, NU_NULL.
*
*************************************************************************/
NET_BUFFER *MLD6_Build_Packet(const IP6_MULTI *ip6_multi, UINT8 message_to_send, 
                              UINT16 max_resp_delay, const UINT8 *multi_addrs)
{
    NET_BUFFER          *buf_ptr = NU_NULL;
    DV_DEVICE_ENTRY     *device = ip6_multi->ipm6_data.multi_device;
    MULTI_DEV_STATE     *mld_state;

#if (MLD6_DEFAULT_COMPATIBILTY_MODE == MLDV2_COMPATIBILITY)
    UINT8               *record_ptr; 
    UINT16              i;
#endif

    /* Get a pointer to the device's state structure */
    mld_state = Multi_Get_Device_State_Struct(device, multi_addrs, 
                                              IP6_ADDR_LEN);
    
    if (mld_state)
    {
        /* Build the common fields of the ICMP header */
        if ( (message_to_send == MLDV1_LISTENER_REPORT) || 
             (message_to_send == MLD6_LISTENER_DONE) )
            buf_ptr = MLD6_Header_Init(MLDV1_HEADER_SIZE);

#if (MLD6_DEFAULT_COMPATIBILTY_MODE == MLDV2_COMPATIBILITY)
        else
            buf_ptr = MLD6_Header_Init(MLDV2_HEADER_SIZE + MLDV2_RECORD_SIZE + 
                                       (mld_state->dev_num_src_to_report * 
                                        IP6_ADDR_LEN));
#endif

        /* If a net buffer was created */
        if (buf_ptr != NU_NULL)
        {
            /* Put the MLD Type in the packet. */
            if ( (message_to_send == MLDV1_LISTENER_REPORT) ||
                 (message_to_send == MLD6_LISTENER_DONE) )
                PUT8(buf_ptr->data_ptr, IP6_ICMP_TYPE_OFFSET, message_to_send);
        
            else
                PUT8(buf_ptr->data_ptr, IP6_ICMP_TYPE_OFFSET, 
                     MLDV2_LISTENER_REPORT);

            /* Put the Code in the packet. */
            PUT8(buf_ptr->data_ptr, IP6_ICMP_CODE_OFFSET, 0);
        
            /* Initialize the checksum to 0 */
            PUT16(buf_ptr->data_ptr, MLD6_CHECKSUM_OFFSET, 0);

            /* Put the Maximum Response Delay field in the message */
            if (message_to_send == MLD6_LISTENER_QUERY)
                PUT16(buf_ptr->data_ptr, MLD6_MAX_RESP_OFFSET, max_resp_delay);

            else
                PUT16(buf_ptr->data_ptr, MLD6_MAX_RESP_OFFSET, 0);

#if (MLD6_DEFAULT_COMPATIBILTY_MODE == MLDV2_COMPATIBILITY)
            /* For MLDv2 reports, add the number of group records to be 
             * reported 
             */
            if (device->dev6_mld_compat_mode == MLDV2_COMPATIBILITY) 
            {
                PUT16(buf_ptr->data_ptr, MLD6_REPORT_RESERVED2_OFFSET, 0);
                /* Each multicast group report will be sent separately.  
                 * Therefore, each report will only contain one record. 
                 */
                PUT16(buf_ptr->data_ptr, 
                        MLD6_REPORT_NUM_MULTI_ADDR_RECORDS_OFFSET, 1);
            }
#endif
            /* If this is a MLDv1 report or we are in MLDv1 compatibility mode, 
                zero out the reserved field and copy the multicast address. */
            if ( (ip6_multi->ipm6_data.multi_device->dev6_mld_compat_mode == 
                  MLDV1_COMPATIBILITY) || (message_to_send == MLD6_LISTENER_DONE) )
            {
                PUT16(buf_ptr->data_ptr, MLD6_RES_OFFSET, 0);

                /* Copy the multicast address into the header */
                NU_BLOCK_COPY(&buf_ptr->data_ptr[MLD6_MULTI_ADDR_OFFSET],
                              multi_addrs, IP6_ADDR_LEN);       
            }
#if (MLD6_DEFAULT_COMPATIBILTY_MODE == MLDV2_COMPATIBILITY)
            /* Only MLDv2 reports contain group records.  Format the group 
             * record here 
             */
            else
            {
                /* Set a pointer to the location to start copying the 
                 * group records 
                 */
                record_ptr = 
                    (UINT8 *)(buf_ptr->data_ptr + MLD6_MULTICAST_ADDRESS_RECORD);

                /* Construct each of the Group Records */
                PUT8(record_ptr, MLD6_RECORD_TYPE_OFFSET, message_to_send);
                PUT8(record_ptr, MLD6_RECORD_AUX_DATA_LEN_OFFSET, 0);
                PUT16(record_ptr, MLD6_RECORD_NUM_OF_SOURCES, 
                      mld_state->dev_num_src_to_report);
            
                /* Copy the multicast address into the record */
                memcpy((record_ptr + MLD6_RECORD_MULTICAST_ADDR_OFFSET), 
                       ip6_multi->ipm6_addr, IP6_ADDR_LEN);
            
                /* Copy the source addresses to report into the packet */
                for (i = 0; i < mld_state->dev_num_src_to_report; i++)
                    memcpy((record_ptr + MLD6_RECORD_SOURCE_ADDR_OFFSET + 
                           (i * IP6_ADDR_LEN)), (mld_state->dev_src_to_report + 
                           (i * IP6_ADDR_LEN)), IP6_ADDR_LEN);
            }
#endif
        }
        else
            NLOG_Error_Log("MLD message cannot be built to due lack of buffers", 
                           NERR_SEVERE, __FILE__, __LINE__);
    }
    else
        NLOG_Error_Log("Unable to find MLD device structure", NERR_SEVERE, 
                       __FILE__, __LINE__);
    return (buf_ptr);

} /* MLD6_Build_Packet */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       MLD6_Header_Init
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function initializes an MLD buffer.
*                                                                         
*   INPUTS                                                                
*                                               
*       size                    The total length of the MLD packet.
*                                                                         
*   OUTPUTS                                                               
*                                                                         
*       Upon successful completion, a pointer to a NET buffer containing
*       the MLD packet is returned.
*
*       Otherwise, NU_NULL is returned.
*
*************************************************************************/
STATIC NET_BUFFER *MLD6_Header_Init(UINT32 size)
{
    NET_BUFFER  *buf_ptr;
   
    /* Get a free buffer chain. */
    buf_ptr = 
        MEM_Buffer_Chain_Dequeue(&MEM_Buffer_Freelist, 
                                 (INT32)(size + MLD6_ROUTER_ALERT_LENGTH));
    
    if (buf_ptr != NU_NULL)
    {
        /* Initialize each field in the allocated buffer. */
        buf_ptr->mem_total_data_len = size;

        buf_ptr->mem_dlist = (NET_BUFFER_HEADER *) &MEM_Buffer_Freelist;

        buf_ptr->data_ptr = buf_ptr->mem_parent_packet;

        /* Put the Next-Header type in the Hop by Hop options header */
        PUT8(buf_ptr->data_ptr, IP6_EXTHDR_NEXTHDR_OFFSET, IPPROTO_ICMPV6);

        /* Put the length in the Hop by Hop options header */
        PUT8(buf_ptr->data_ptr, IP6_EXTHDR_LENGTH_OFFSET, 0);

        /* Put the Hop-By-Hop option type in the packet. */
        PUT8(buf_ptr->data_ptr, 2 + IP6_TYPE_OFFSET, IP6_ROUTER_ALERT_OPT);

        /* Put the data length of two in the packet */
        PUT8(buf_ptr->data_ptr, 2 + IP6_LENGTH_OFFSET, 2);

        /* Zero out the other field */
        PUT16(buf_ptr->data_ptr, 2 + IP6_DATA_OFFSET, 0);

        /* Add a padN option to pad the header by two bytes to keep it 
         * word-aligned.
         */
        PUT8(buf_ptr->data_ptr, 6 + IP6_TYPE_OFFSET, IP6_PADN_OPTION);

        PUT8(buf_ptr->data_ptr, 6 + IP6_LENGTH_OFFSET, 
             IP6_PADN_OPTION_LENGTH(2));

        /* Increment past the Router Alert Option */
        buf_ptr->data_ptr += MLD6_ROUTER_ALERT_LENGTH;

        buf_ptr->data_len = size;
    }

    return (buf_ptr);

} /* MLD6_Header_Init */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       MLD6_Calculate_Max_Response_Delay
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       Used to calculate the Maximum Response Delay from the 
*         received  max response code.
*                                                                         
*   INPUTS                                                                
*                                               
*       max_resp_code           The received max response code.  
*                                                                         
*   OUTPUTS                                                               
*                                                                         
*       The calculated maximum response delay.
*
*************************************************************************/
UINT32 MLD6_Calculate_Max_Response_Delay(UINT16 max_resp_code)
{
    UINT32  response_delay;

    /* Since the Nucleus clock is a 10ms clock and the RFC specifies that
     *  a maximum response delay of 10000 be equal to 10 seconds, we must
     *  divide the max_resp_code by 10.
     */
    max_resp_code = (UINT16)(max_resp_code / 10);

    /* If the maximum response code is greater than 32768, then we will 
        need to use the RFC-prescribed algorithm */
    if (max_resp_code & 0x8000)
    {
        response_delay = ((max_resp_code & 0x0FFF) | 0x1000) <<  \
                            (((max_resp_code >> 12) & 0x0007) + 3);
    }

    else
        response_delay = max_resp_code;

    return (response_delay);

} /* MLD6_Calculate_Max_Response_Delay */

#if (MLD6_DEFAULT_COMPATIBILTY_MODE == MLDV2_COMPATIBILITY)

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       MLD6_Update_Host_Compatibility
*                                                                         
*   DESCRIPTION                                                           
*
*       This function determines the which compatibility mode the device 
*       should be in and sets the device to that mode.
*
*   INPUTS                                                                
*                          
*       *buf_ptr                A ptr to the received query 
*
*       *multi_ptr              A ptr to the multi structure
* 
*   OUTPUTS                                                               
*                                                  
*       VOID
*
*************************************************************************/
VOID MLD6_Update_Host_Compatibility(const NET_BUFFER *buf_ptr, 
                                    MULTI_IP *multi_ptr)
{
    /* Since this is a MLD query, verify whether it is from a router 
     * using MLDv1 or MLDv2 
     */
    if (multi_ptr != NU_NULL)
    {
        /* Determine the message type */
        if (buf_ptr->data_ptr[MLD6_TYPE_OFFSET] == 
            MLD6_LISTENER_QUERY)
        {
            /* If the total length of the query is exactly 24 octets, 
             * then this is a MLDv1 query 
             */
            if (buf_ptr->data_len == MULTICAST_MLDV1_QUERY_LENGTH)
            {
                /* Since this is a MLDv1 query, set the device as 
                 * being in MLDv1 compatibility mode */

                /* If there is currently a MLDv1 timer running, reset 
                 * the timer. 
                 */
                if (multi_ptr->multi_data.multi_device->
                    dev6_mld_compat_timers & MLDV1_COMPAT_TIMER_RUNNING)
                {
                    if (TQ_Timerunset(MLDv1_Compatibility_Timer_Event, 
                                      TQ_CLEAR_EXACT, (UNSIGNED)(multi_ptr->
                                      multi_data.multi_device->dev_index),
                                      NU_NULL) != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to stop the multicast timer", 
                                       NERR_SEVERE, __FILE__, 
                                       __LINE__);
                    }

                    /* Restart the timer with a fresh timeout value */
                    if (TQ_Timerset(MLDv1_Compatibility_Timer_Event, 
                                    (UNSIGNED)(multi_ptr->multi_data.
                                    multi_device->dev_index), 
                                    MLD6_OLDER_VERSION_QUERIER_PRESENT_TIMEOUT(multi_ptr->
                                    multi_data.multi_device), 
                                    NU_NULL) != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to set the compatibility timer", 
                                       NERR_SEVERE, __FILE__, 
                                       __LINE__);
                    }
                }

#if (MLD6_DEFAULT_COMPATIBILTY_MODE != MLDV1_COMPATIBILITY)
                /* If the default compatibility mode is not MLDv1, 
                 * set the compatibility timer. 
                 */
                else 
                {
                    /* Set the device as being in MLDv1 compatibility 
                     * mode 
                     */
                    multi_ptr->multi_data.multi_device->
                                    dev6_mld_compat_mode = 
                                        MLDV1_COMPATIBILITY;

                    /* Set the compatibility timer */
                    if (TQ_Timerset(MLDv1_Compatibility_Timer_Event, 
                                    (UNSIGNED)(multi_ptr->multi_data.
                                    multi_device->dev_index), 
                                    MLD6_OLDER_VERSION_QUERIER_PRESENT_TIMEOUT(multi_ptr->
                                    multi_data.multi_device), 
                                    NU_NULL) != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to set the compatibility timer", 
                                       NERR_SEVERE, __FILE__, 
                                       __LINE__);
                    }
                    
                    /* Set the flag to indicate the the MLDv1 timer 
                     * has been set. 
                     */
                    else
                        multi_ptr->multi_data.multi_device->
                            dev6_mld_compat_timers = 
                                (multi_ptr->multi_data.multi_device->
                                dev6_mld_compat_timers & MLDV1_COMPAT_TIMER_RUNNING);
                }
#endif
            }
        }

    }
} /* MLD6_Update_Host_Compatibility */ 

#endif
#endif
