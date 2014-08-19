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
*       ncp.c
*
*   COMPONENT
*
*       NCP - Network Control Protocol
*
*   DESCRIPTION
*
*       This file contains the Internet Protocol Control Protocol (IPCP)
*       for use with PPP to negotiate IPv4 network settings to be used
*       over the link. IPCP is one of several possible Network Control
*       Protocols (NCPs) that can be used over the same LCP link.
*
*   DATA STRUCTURES
*
*       IPCP_Event
*       IPV6CP_Event
*
*   FUNCTIONS
*
*       NCP_Init
*       NCP_Reset
*       NCP_Change_IP_Mode
*       NCP_Set_Client_IP_Address
*       NCP_Interpret
*       NCP_Check_Config_Req
*       NCP_Check_Config_Ack
*       NCP_Check_Config_Nak
*       NCP_Check_Terminate_Req
*       NCP_Check_Terminate_Ack
*       NCP_Check_Code_Reject
*       NCP_Process_Request
*       NCP_Process_Nak
*       NCP_Send_Config_Req
*       NCP_Event_Handler
*       NCP_Close_IP_Layer
*       NCP_Clean_Up_Link
*
*   DEPENDENCIES
*
*       nu_ppp.h
*
*************************************************************************/
#include "drivers/nu_ppp.h"

#if (INCLUDE_IPV4 == NU_TRUE)
TQ_EVENT IPCP_Event;
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
TQ_EVENT IPV6CP_Event;
#endif

#if NCP_DEBUG_PRINT_OK
#define PrintInfo(s)        PPP_Printf(s)
#define PrintErr(s)         PPP_Printf(s)
#define PrintOpt(s,v)       PPP_Printf(s,v)
#define PrintState(s,a,b)   PPP_Printf(s,a,b)
#else
#define PrintInfo(s)
#define PrintErr(s)
#define PrintOpt(s,v)
#define PrintState(s,a,b)
#endif


/*************************************************************************
* FUNCTION
*
*     NCP_Init
*
* DESCRIPTION
*
*     This function initializes the NCP layer of PPP. It creates the needed
*     timers and initializes the NCP structure for the passed in device.
*
* INPUTS
*
*     DV_DEVICE_ENTRY     *dev_ptr     Pointer to the device that this
*                                       LCP layer belongs to.
*
* OUTPUTS
*
*     STATUS              NU_SUCCESS will be returned on if NCP is
*                         correctly initialized otherwise -1 will be
*                         returned or a Nucleus PLUS status will be
*                         returned.
*
*************************************************************************/
STATUS NCP_Init(DV_DEVICE_ENTRY *dev_ptr)
{
    LINK_LAYER      *link_layer;
    STATUS          status = NU_SUCCESS;
    static CHAR     init_once = 0;

    /* Get a pointer to the link layer for this device. */
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

    if (init_once++ == 0)
    {
#if (INCLUDE_IPV4 == NU_TRUE)
        /* Register the IPCP event handler with the dispatcher. */
        status = EQ_Register_Event(PPP_Event_Forwarding, &IPCP_Event);
        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to register IPCP event.", NERR_FATAL, __FILE__, __LINE__);
            return(NU_PPP_INIT_FAILURE);
        }
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
        /* Register the IPCP event handler with the dispatcher. */
        status = EQ_Register_Event(PPP_Event_Forwarding, &IPV6CP_Event);
        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to register IPV6CP event.", NERR_FATAL, __FILE__, __LINE__);
            return(NU_PPP_INIT_FAILURE);
        }
#endif
    }

    /* Initialize the associated NCP structures based on the type
       of device. */
#if (INCLUDE_IPV6 == NU_TRUE)
    if (dev_ptr->dev_flags & DV6_IPV6)
    {
        /* Device type: IPv6 */
        link_layer->ncp6.protocol_code = PPP_IPV6_CONTROL_PROTOCOL;
        link_layer->ncp6.event_id = IPV6CP_Event;
    }
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    if(!(link_layer->ncp.options.ipcp.flags & PPP_FLAG_NO_IPV4))
    {
        /* Default device type: IPv4 */
        link_layer->ncp.protocol_code = PPP_IP_CONTROL_PROTOCOL;
        link_layer->ncp.event_id = IPCP_Event;

        /* Initialize the default flags. */
        link_layer->ncp.options.ipcp.default_flags = 0;

#if (INCLUDE_DNS == NU_TRUE)

#if (PPP_USE_DNS1 == NU_TRUE)
    link_layer->ncp.options.ipcp.default_flags |= PPP_FLAG_DNS1;

#if (PPP_USE_DNS2 == NU_TRUE)
    link_layer->ncp.options.ipcp.default_flags |= PPP_FLAG_DNS2;
#endif
#endif
#endif
    }
#endif /* #if (INCLUDE_IPV4 == NU_TRUE) */


    /* Set the default options. */
    NCP_Reset(link_layer);

    return (status);

} /* NCP_Init */



/*************************************************************************
*
* FUNCTION
*
*     NCP_Reset
*
* DESCRIPTION
*
*     Resets the IPCP negotiation options to their defaults. This
*     is done in preparation for a new PPP session.
*
* INPUTS
*
*     LINK_LAYER       *link   Pointer to the link layer.
*
* OUTPUTS
*
*     None
*
*************************************************************************/
VOID NCP_Reset(LINK_LAYER *link)
{
#if (INCLUDE_IPV4 == NU_TRUE)
    /* Reset the flags to default values. */
    link->ncp.options.ipcp.flags = link->ncp.options.ipcp.default_flags;

    /* Zero out the primary and secondary DNS servers. */
    memset(link->ncp.options.ipcp.primary_dns_server, 0, 4);
    memset(link->ncp.options.ipcp.secondary_dns_server, 0, 4);

    /* Set the initial state of the NCP layer. */
    link->ncp.state = REQ_SENT;
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
    link->ncp6.state = REQ_SENT;
#endif

} /* NCP_Reset */



/*************************************************************************
* FUNCTION
*
*     NCP_IP_Change_Mode
*
* DESCRIPTION
*
*     This function sets the mode of operation for NCP and the rest of PPP.
*     The main difference between modes is in CLIENT mode PPP requests an
*     IP address and in SERVER mode PPP assigns an IP address.
*
* INPUTS
*
*     INT8              new_mode        CLIENT or SERVER
*     DV_DEVICE_ENTRY   *dev_ptr        Pointer to PPP device.
*
* OUTPUTS
*
*     STATUS                            NU_SUCCESS if the passed in mode
*                                       was acceptable NU_UNAVAILABLE if
*                                       the passed in mode was
*                                       unacceptable
*
*************************************************************************/
STATUS NCP_Change_IP_Mode(UINT8 new_mode, DV_DEVICE_ENTRY *dev_ptr)
{
    STATUS          ret_val;

    /* Make sure it is a valid mode */
    if ((new_mode == PPP_CLIENT) || (new_mode == PPP_SERVER))
    {
        /* Change the mode to the one that was passed in */
        ((LINK_LAYER*)dev_ptr->dev_link_layer)->mode = new_mode;

        ret_val = NU_SUCCESS;
    }
    else
        ret_val = NU_UNAVAILABLE;

    return (ret_val);

} /* NCP_IP_Change_Mode */



/*************************************************************************
* FUNCTION
*
*     NCP_Set_Client_IP_Address
*
* DESCRIPTION
*
*     This function sets the IP addresses to be assigned to a calling
*     client. It is only used in SERVER mode.
*
* INPUTS
*
*     *ppp_options                  Pointer to the PPP_OPTIONS structure.
*
* OUTPUTS
*
*     STATUS                        Was the addresses passed in set
*
*************************************************************************/
STATUS NCP_Set_Client_IP_Address(PPP_OPTIONS *ppp_options)
{
    LINK_LAYER          *link;
    DV_DEVICE_ENTRY     *dev_ptr;
    STATUS              ret_status = NU_SUCCESS;
    NU_SUPERV_USER_VARIABLES;

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Grab the semaphore. */
    if(NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Find the device that we want to hangup. */
    dev_ptr = DEV_Get_Dev_By_Name(ppp_options->ppp_link);

    /* Make sure the device was found and that it is a PPP device. */
    if ((dev_ptr) && (dev_ptr->dev_type == DVT_PPP))
    {
        /* Get a pointer to the authentication layer structure. */
        link = (LINK_LAYER*)dev_ptr->dev_link_layer;

#if (INCLUDE_IPV4 == NU_TRUE)
        /* If remote IPv4 address is passed in. */
        if(!(link->ncp.options.ipcp.flags & PPP_FLAG_NO_IPV4))
        {
            if (ppp_options->ppp_remote_ip4)
                memcpy(link->ncp.options.ipcp.remote_address,
                                           ppp_options->ppp_remote_ip4, 4);
        }
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
        if (dev_ptr->dev_flags & DV6_IPV6)
        {
            /* If remote IPv6 address is passed in. */
            if(ppp_options->ppp_remote_ip6)
                memcpy(link->ncp6.options.ipv6cp.remote_address,
                                            ppp_options->ppp_remote_ip6, 8);

#if (INCLUDE_IPV4 == NU_TRUE)
            /* Otherwise if IPv6 address is not passed in, then make a
             * IPv6 address out of IPv4 address
             */
            else if(ppp_options->ppp_remote_ip4)
            {
                /* Clear the first four bytes. */
                memset(link->ncp6.options.ipv6cp.remote_address, 0, 4);

                memcpy(&link->ncp6.options.ipv6cp.remote_address[4],
                                            ppp_options->ppp_remote_ip4, 4);
            }
#endif
        }

#endif

    }
    else
        ret_status = NU_INVALID_LINK;

    /* Release the semaphore. */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to release a semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Return from function in user mode. */
    NU_USER_MODE();

    return (ret_status);

} /* NCP_Set_Client_IP_Address */


/*************************************************************************
* FUNCTION
*
*     NCP_Interpret
*
* DESCRIPTION
*
*     Distribute incoming IPCP packets according to their LCP code.
*     All packet types are multiplexed to a corresponding NCP_Check_
*     function, which will handle it according to the PPP automaton
*     state.
*
* INPUTS
*
*     *ncp
*     *buf_ptr
*
* OUTPUTS
*
*     None
*
*************************************************************************/
VOID NCP_Interpret(NCP_LAYER *ncp, NET_BUFFER *buf_ptr)
{
    DV_DEVICE_ENTRY *dev_ptr = buf_ptr->mem_buf_device;
    LINK_LAYER      *link_layer;
    LCP_FRAME       *lcp_frame;

    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

    /* LCP must be in the opened state for NCP negotiation */
    if (link_layer->lcp.state != OPENED || link_layer->authentication.state != AUTHENTICATED)
    {
        PrintInfo("Discarded - LCP not open or link not authenticated.\n");
        PML_SilentDiscards_Inc(dev_ptr);
    }
    else
    {
        PrintInfo("NCP_Interpret\n");

        /* Swap the 2-byte packet length. */
        lcp_frame = (LCP_FRAME*)buf_ptr->data_ptr;
        lcp_frame->len = INTSWAP(lcp_frame->len);

        /* Switch on the LCP type. */
        switch (buf_ptr->data_ptr[0])
        {
        case LCP_CONFIGURE_REQUEST:
            NCP_Check_Config_Req(ncp, buf_ptr);
            break;

        case LCP_CONFIGURE_ACK:
            NCP_Check_Config_Ack(ncp, buf_ptr);
            break;

        case LCP_CONFIGURE_NAK:
            NCP_Check_Config_Nak(ncp, PPP_FLAG_NAK, buf_ptr);
            break;

        case LCP_CONFIGURE_REJECT:
            NCP_Check_Config_Nak(ncp, PPP_FLAG_REJECT, buf_ptr);
            break;

        case LCP_TERMINATE_REQUEST:
            link_layer->term_id = lcp_frame->id;
            NCP_Check_Terminate_Req(ncp, buf_ptr);
            break;

        case LCP_TERMINATE_ACK:
            NCP_Check_Terminate_Ack(ncp, buf_ptr);
            break;

        case LCP_CODE_REJECT:
            NCP_Check_Code_Reject(ncp, buf_ptr);
            break;

        default:

            PrintInfo("    Unknown code!\n");

            /* Reject any unknown LCP codes. */
            LCP_Send_Code_Reject(buf_ptr, PPP_IP_CONTROL_PROTOCOL);

            break;
        }
    }

    /* Release the buffer space */
    MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

} /* NCP_Interpret */



/*************************************************************************
* FUNCTION
*
*     NCP_Check_Config_Req
*
* DESCRIPTION
*
*     Handle an incoming Configure Request packet according to the PPP
*     state machine.
*
* INPUTS
*
*     None
*
* OUTPUTS
*
*     None
*
*************************************************************************/
VOID NCP_Check_Config_Req(NCP_LAYER *ncp, NET_BUFFER *buf_ptr)
{
    NET_BUFFER      *ack_buf = NU_NULL;
    DV_DEVICE_ENTRY *dev_ptr;
    STATUS          status;
    UINT8           state;

    /* Get some pointers to our data. */
    dev_ptr = buf_ptr->mem_buf_device;

    PrintInfo("   NCP_Check_Config_Req\n");

    state = ncp->state;

    status = NCP_Process_Request(ncp, buf_ptr, &ack_buf);
    if (status != PPP_FLAG_DROP)
    {
        /* Change to next state. */
        switch (state)
        {
        case STOPPED:
        case REQ_SENT:
        case ACK_SENT:
        case OPENED:
            if (status != PPP_FLAG_ACK)
                ncp->state = REQ_SENT;
            else
                ncp->state = ACK_SENT;
            /* Send the ack/nak/rej. */
            if (ack_buf != NU_NULL)
                LCP_Send(dev_ptr, ack_buf, ncp->protocol_code);
            break;

        case ACK_RCVD:
            if (status == PPP_FLAG_ACK)
                ncp->state = OPENED;
            /* Send the ack/nak/rej. */
            if (ack_buf != NU_NULL)
                LCP_Send(dev_ptr, ack_buf, ncp->protocol_code);

            break;

        case CLOSED:
        case CLOSING:
        case STOPPING:
        case INITIAL:
        case STARTING:
        default:
            /* Free the buffer if it was allocated above. */
            if(ack_buf != NU_NULL)
                MEM_Multiple_Buffer_Chain_Free(ack_buf);
            break;
        }


        PrintState("   NCP state == %s --> %s\n", StateStrings[state], StateStrings[ncp->state]);

        /* Perform response routine based on current state. */
        switch (state)
        {
        case CLOSED:
            LCP_Send_Terminate_Ack(dev_ptr, ncp->protocol_code);
            break;

        case STOPPED:
            ncp->num_transmissions = LCP_MAX_CONFIGURE;
            NCP_Send_Config_Req(ncp, dev_ptr);
            break;

        case ACK_RCVD:
            if (status == PPP_FLAG_ACK)
            {
                /* Set the NCP up event to start using the link. */
                EQ_Put_Event(ncp->event_id, (UNSIGNED)dev_ptr, NCP_LAYER_UP);
            }

            break;

        case OPENED:
            ncp->num_transmissions = LCP_MAX_CONFIGURE;
            NCP_Send_Config_Req(ncp, dev_ptr);
            break;

        case INITIAL:
        case STARTING:
        case CLOSING:
        case STOPPING:
        case REQ_SENT:
        case ACK_SENT:
        default:
            break;
        }
    }

} /* NCP_Check_Config_Req */


/*************************************************************************
* FUNCTION
*
*     NCP_Check_Config_Ack
*
* DESCRIPTION
*
*     Handle an incoming Configure ACK packet according to the PPP
*     state machine.
*
* INPUTS
*
*     *ncp
*     *buf_ptr
*
* OUTPUTS
*
*     None
*
*************************************************************************/
VOID NCP_Check_Config_Ack(NCP_LAYER *ncp, NET_BUFFER *buf_ptr)
{
    DV_DEVICE_ENTRY *dev_ptr;
    UINT8           state;

    /* Get some pointers to our data. */
    dev_ptr = buf_ptr->mem_buf_device;

    ncp->num_transmissions = LCP_MAX_CONFIGURE;
    TQ_Timerunset(ncp->event_id, TQ_CLEAR_EXACT, (UNSIGNED)dev_ptr, NCP_SEND_CONFIG);

    /* Save the current state. */
    state = ncp->state;

    PrintInfo("   NCP_Check_Config_Ack\n");

    /* Change to next state. */
    switch (state)
    {
    case REQ_SENT:
        ncp->state = ACK_RCVD;
        break;

    case ACK_SENT:
        ncp->state = OPENED;
        break;

    case ACK_RCVD:
    case OPENED:
        ncp->state = REQ_SENT;
        break;
    case INITIAL:
    case STARTING:
    case CLOSED:
    case STOPPED:
    case CLOSING:
    case STOPPING:
    default:
        break;
    }

    PrintState("   NCP state == %s --> %s\n", StateStrings[state], StateStrings[ncp->state]);

    /* Perform response routine based on current state. */
    switch (state)
    {
    case CLOSED:
    case STOPPED:
        LCP_Send_Terminate_Ack(dev_ptr, ncp->protocol_code);
        break;

    case REQ_SENT:
        break;

    case ACK_SENT:
        /* Set the NCP up event to start using the link. */
        EQ_Put_Event(ncp->event_id, (UNSIGNED)dev_ptr, NCP_LAYER_UP);

        break;

    case ACK_RCVD:
    case OPENED:
        ncp->num_transmissions = LCP_MAX_CONFIGURE;
        NCP_Send_Config_Req(ncp, dev_ptr);

        NLOG_Error_Log("Possibly received a cross-connection packet.",
                       NERR_INFORMATIONAL, __FILE__, __LINE__);
        break;
    case INITIAL:
    case STARTING:
    case CLOSING:
    case STOPPING:
    default:
        break;
    }

} /* NCP_Check_Config_Ack */


/*************************************************************************
* FUNCTION
*
*     NCP_Check_Config_Nak
*
* DESCRIPTION
*
*     Handle an incoming Configure NAK packet according to the PPP
*     state machine.
*
* INPUTS
*
*     *ncp
*     nak_type
*     *buf_ptr
*
* OUTPUTS
*
*     None
*
*************************************************************************/
VOID NCP_Check_Config_Nak(NCP_LAYER *ncp, UINT8 nak_type, NET_BUFFER *buf_ptr)
{
    DV_DEVICE_ENTRY *dev_ptr;
    STATUS status;
    UINT8           state;

    /* Get some pointers to our data. */
    dev_ptr = buf_ptr->mem_buf_device;

    ncp->num_transmissions = LCP_MAX_CONFIGURE;
    TQ_Timerunset(ncp->event_id, TQ_CLEAR_EXACT, (UNSIGNED)dev_ptr, NCP_SEND_CONFIG);

    /* Save the current state. */
    state = ncp->state;

    PrintInfo("   NCP_Check_Config_Nak\n");

    status = NCP_Process_Nak(ncp, nak_type, buf_ptr);
    if (status != PPP_FLAG_DROP)
    {
        /* Change to next state. */
        switch (state)
        {
        case REQ_SENT:
            if (status == PPP_FLAG_ABORT)
                ncp->state = STOPPING;
            break;

        case ACK_RCVD:
        case OPENED:
            ncp->state = REQ_SENT;
            break;

        case ACK_SENT:
        case INITIAL:
        case STARTING:
        case CLOSED:
        case STOPPED:
        case CLOSING:
        case STOPPING:
        default:
            break;
        }

        PrintState("   NCP state == %s --> %s\n", StateStrings[state], StateStrings[ncp->state]);

        /* Perform response routine based on current state. */
        switch (state)
        {
        case CLOSED:
        case STOPPED:
            LCP_Send_Terminate_Ack(dev_ptr, ncp->protocol_code);
            break;

        case REQ_SENT:
        case ACK_SENT:
            if (status == PPP_FLAG_ABORT)
            {
                /* Initialize the number of retransmit attempts. */
                EQ_Put_Event(ncp->event_id, (UNSIGNED)dev_ptr, NCP_CLOSE_REQUEST);
            }
            else
                NCP_Send_Config_Req(ncp, dev_ptr);

            break;

        case ACK_RCVD:
        case OPENED:
        ncp->num_transmissions = LCP_MAX_CONFIGURE;
        NCP_Send_Config_Req(ncp, dev_ptr);

        NLOG_Error_Log("Possibly received a cross-connection packet.",
                       NERR_INFORMATIONAL, __FILE__, __LINE__);
        break;
        case INITIAL:
        case STARTING:
        case CLOSING:
        case STOPPING:
        default:
            break;
        }
    }

} /* NCP_Check_Config_Nak */



/*************************************************************************
* FUNCTION
*
*     NCP_Check_Terminate_Req
*
* DESCRIPTION
*
*     Handle an incoming Terminate Request packet according to the PPP
*     state machine.
*
* INPUTS
*
*     None
*
* OUTPUTS
*
*     None
*
*************************************************************************/
VOID NCP_Check_Terminate_Req(NCP_LAYER *ncp, NET_BUFFER *buf_ptr)
{
    DV_DEVICE_ENTRY *dev_ptr;
    UINT8           state;

    /* Get some pointers to our data. */
    dev_ptr = buf_ptr->mem_buf_device;

    /* Save the current state. */
    state = ncp->state;

    PrintInfo("   NCP_Check_Terminate_Req\n");

    /* Change to next state. */
    switch (state)
    {
    case REQ_SENT:
    case ACK_RCVD:
    case ACK_SENT:
        ncp->state = REQ_SENT;
        break;

    case OPENED:
        ncp->state = STOPPING;
        break;

    case INITIAL:
    case STARTING:
    case CLOSED:
    case STOPPED:
    case CLOSING:
    case STOPPING:
    default:
        break;
    }

    PrintState("   NCP state == %s --> %s\n", StateStrings[state], StateStrings[ncp->state]);

    /* Perform response routine based on current state. */
    switch (state)
    {
    case OPENED:
        EQ_Put_Event(ncp->event_id, (UNSIGNED)dev_ptr, NCP_CLOSE_REQUEST);
        LCP_Send_Terminate_Ack(dev_ptr, ncp->protocol_code);
        break;

    case CLOSED:
    case STOPPED:
    case CLOSING:
    case STOPPING:
    case REQ_SENT:
    case ACK_RCVD:
    case ACK_SENT:
        LCP_Send_Terminate_Ack(dev_ptr, ncp->protocol_code);
        break;

    case INITIAL:
    case STARTING:
    default:
        break;
    }

} /* NCP_Check_Terminate_Req */


/*************************************************************************
* FUNCTION
*
*     NCP_Check_Terminate_Ack
*
* DESCRIPTION
*
*     Handle an incoming Terminate ACK packet according to the PPP
*     state machine.
*
* INPUTS
*
*     *ncp
*     *buf_ptr
*
* OUTPUTS
*
*     None
*
*************************************************************************/
VOID NCP_Check_Terminate_Ack(NCP_LAYER *ncp, NET_BUFFER *buf_ptr)
{
    DV_DEVICE_ENTRY *dev_ptr;
    UINT8           state;

    /* Get some pointers to our data. */
    dev_ptr = buf_ptr->mem_buf_device;

    TQ_Timerunset(ncp->event_id, TQ_CLEAR_EXACT, (UNSIGNED)dev_ptr, NCP_SEND_TERMINATE);

    /* Save the current state. */
    state = ncp->state;

    PrintInfo("   NCP_Check_Terminate_Ack\n");

    /* Change to next state. */
    switch (state)
    {
    case CLOSING:
        ncp->state = CLOSED;
        break;

    case STOPPING:
        ncp->state = STOPPED;
        break;

    case ACK_RCVD:
    case OPENED:
        ncp->state = REQ_SENT;
        break;

    case INITIAL:
    case STARTING:
    case CLOSED:
    case STOPPED:
    case REQ_SENT:
    case ACK_SENT:
    default:
        break;
    }

    PrintState("   NCP state == %s --> %s\n", StateStrings[state], StateStrings[ncp->state]);

    /* Perform response routine based on current state. */
    switch (state)
    {
    case OPENED:
        ncp->num_transmissions = LCP_MAX_CONFIGURE;
        NCP_Send_Config_Req(ncp, dev_ptr);
        break;

    case CLOSING:
    case STOPPING:
        EQ_Put_Event(ncp->event_id, (UNSIGNED)dev_ptr, NCP_LAYER_DOWN);
        break;

    case INITIAL:
    case STARTING:
    case CLOSED:
    case STOPPED:
    case REQ_SENT:
    case ACK_RCVD:
    case ACK_SENT:
    default:
        break;
    }

} /* NCP_Check_Terminate_Ack */



/*************************************************************************
* FUNCTION
*
*     NCP_Check_Code_Reject
*
* DESCRIPTION
*
*     Handle an incoming Code Reject packet according to the PPP
*     state machine.
*
* INPUTS
*
*     *ncp
*     *buf_ptr
*
* OUTPUTS
*
*     None
*
*************************************************************************/
VOID NCP_Check_Code_Reject(NCP_LAYER *ncp, NET_BUFFER *buf_ptr)
{
    DV_DEVICE_ENTRY *dev_ptr;
    STATUS          status = PPP_FLAG_OK;
    UINT8           state;

    /* Get some pointers to our data. */
    dev_ptr = buf_ptr->mem_buf_device;

    /* Save the current state. */
    state = ncp->state;

    PrintInfo("   NCP_Check_Code_Reject\n");

    /* Change to next state. */
    switch (state)
    {
    case REQ_SENT:
    case ACK_SENT:
        if (status != PPP_FLAG_OK)
            ncp->state = STOPPED;
        break;

    case INITIAL:
    case STARTING:
    case CLOSED:
    case STOPPED:
    case CLOSING:
    case STOPPING:
    case ACK_RCVD:
    case OPENED:
    default:
        break;
    }

    PrintState("   NCP state == %s --> %s\n", StateStrings[state], StateStrings[ncp->state]);

    /* Perform response routine based on current state. */
    switch (state)
    {
    case REQ_SENT:
    case ACK_SENT:

        /* Initialize the number of retransmit attempts. */
        ncp->num_transmissions = LCP_MAX_TERMINATE;

        /* We currently don't send any optional codes, so terminate the link. */
        EQ_Put_Event(ncp->event_id, (UNSIGNED)dev_ptr, NCP_SEND_TERMINATE);

        break;

    case INITIAL:
    case STARTING:
    case CLOSED:
    case STOPPED:
    case CLOSING:
    case STOPPING:
    case ACK_RCVD:
    case OPENED:
    default:
        break;
    }

} /* NCP_Check_Code_Reject */



/*************************************************************************
* FUNCTION
*
*     NCP_Process_Request
*
* DESCRIPTION
*
*     Check the options in an incoming configure request packet. If we
*     need to NAK any options, they will be added to a NAK packet. Or,
*     if we need to REJECT any options, they will be added to a REJECT
*     packet. If all options are ok, they are copied to an ACK packet.
*     In any case, the buffer is given to the calling function, but if
*     the request is to be ignored, the buffer pointer will be NU_NULL.
*
* INPUTS
*
*     *ncp
*     *in_buf_ptr
*     **ack_buf
*
* OUTPUTS
*
*     STATUS
*
*************************************************************************/
STATUS NCP_Process_Request(NCP_LAYER *ncp, NET_BUFFER *in_buf_ptr,
                           NET_BUFFER **ack_buf)
{
    STATUS status = PPP_FLAG_ACK;
    LCP_FRAME       *lcp_frame, *ack_frame;
    LCP_OPTION      *option;
    DV_DEVICE_ENTRY *dev_ptr;
    UINT16          len = 0;
    LINK_LAYER      *link_layer;

#if ((INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_DNS == NU_TRUE))
    UINT8           nservers = 2, num_dns_servers;
    UINT8           temp_address[IPV6CP_ADDR_LEN];
#endif

    /* Get some pointers to our data. */
    dev_ptr = in_buf_ptr->mem_buf_device;
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
    lcp_frame = (LCP_FRAME*)in_buf_ptr->data_ptr;

    PrintInfo("   NCP_Process_Request\n");

    /* Allocate a new LCP response packet for the ack/nak/reject that
       results from this request. */
    *ack_buf = LCP_New_Buffer(dev_ptr, LCP_CONFIGURE_ACK, lcp_frame->id);
    if (*ack_buf == NU_NULL)
        /* This error is logged within LCP_New_Buffer, so just return. */
        return PPP_FLAG_DROP;

    /* Set ack_frame to the buffer's data_ptr. */
    ack_frame = (LCP_FRAME*)((*ack_buf)->data_ptr);

    while (len + LCP_HEADER_LEN < lcp_frame->len)
    {
        /* Move to the next option. */
        option = (LCP_OPTION*)(&lcp_frame->data[len]);
        if (option->len == 0)
            break;

        /* Check out what the option is and see if we can handle it */
        switch (option->code)
        {
#if (INCLUDE_IPV4 == NU_TRUE)
        case NCP_IP_ADDRESS:

            PrintOpt("      IPCP Peer Req: IP Address\n", 0);

            if (ncp->protocol_code != PPP_IP_CONTROL_PROTOCOL)
                goto reject;

            if (link_layer->mode == PPP_CLIENT
                && memcmp((CHAR*)option->data, PPP_Null_Ip, IP_ADDR_LEN) != 0)
            {

                PrintInfo("    Server assigning its remote IP.\n");

                /* Get the IP address out. This is the IP address of
                   the server. Store it in the device structure
                   for this link. */
                dev_ptr->dev_addr.dev_dst_ip_addr = IP_ADDR(option->data);

                /* Add the remote address to the NCP data structure. */
                memcpy(ncp->options.ipcp.remote_address, option->data, IP_ADDR_LEN);

            }
            else if (link_layer->mode == PPP_SERVER)
            {
                if (memcmp(ncp->options.ipcp.remote_address, (CHAR*)option->data, IP_ADDR_LEN) != 0)
                {
                    PrintInfo("    NAKing with client IP.\n");

                    /* This will be done by a NAK packet. So all we need to
                       do is to replace the zero IP address with the
                       assigned one. */
                    memcpy((CHAR*)option->data, ncp->options.ipcp.remote_address, (UINT8)(option->len - LCP_OPTION_HDRLEN));

                    /* Add this option to the NAK packet. */
                    status = LCP_Nak_Option(ack_frame, option->code, (UINT8)(option->len - LCP_OPTION_HDRLEN), option->data);
                }
                else
                    PrintInfo("    Requested client IP OK.\n");
            }

            break;

#if (INCLUDE_DNS == NU_TRUE)
        case NCP_PRIMARY_DNS_ADDRESS:
            nservers = 1;

        case NCP_SECONDARY_DNS_ADDRESS:

            /* Could fall through from above case - Check to see if the
            Secondary DNS is requested. If so set count to 2. */
            if (option->code == NCP_SECONDARY_DNS_ADDRESS)
                nservers = 2;

            if (ncp->protocol_code != PPP_IP_CONTROL_PROTOCOL)
                goto reject;

            /* If the IP is zero, then the host is requesting a DNS server address. */
            if (memcmp((CHAR*)option->data, PPP_Null_Ip, IP_ADDR_LEN) == 0)
            {
                /* Get the first DNS server in our DNS server list. */
                num_dns_servers = (UINT8)NU_Get_DNS_Servers(temp_address, nservers * IP_ADDR_LEN);

                /* Make sure we got one. */
                if ( (num_dns_servers == nservers) &&
					 (nservers <= IPV6CP_ADDR_LEN) )
                {
                    PrintInfo("    Assigning client DNS\n");

                    /* Replace the null IP with our DNS IP. */
                    memcpy((CHAR*)option->data, &temp_address[nservers-1], (UINT8)(option->len - LCP_OPTION_HDRLEN));

                    /* Add this option to the NAK packet. */
                    status = LCP_Nak_Option(ack_frame, option->code, (UINT8)(option->len - LCP_OPTION_HDRLEN), option->data);
                }
                else
                {
                    /* We don't have a DNS server to assign so we must reject this option. */
                    status = LCP_Reject_Option(ack_frame, option->code, (UINT8)(option->len - LCP_OPTION_HDRLEN), option->data);
                }
            }

            break;
#endif
#endif /* #if (INCLUDE_IPV4 == NU_TRUE) */

#if (INCLUDE_IPV6 == NU_TRUE)
        case NCP_IPV6_ADDRESS:
            /* This is only valid for IPV6CP. */
            if (ncp->protocol_code != PPP_IPV6_CONTROL_PROTOCOL)
                goto reject;

            /* Make sure this is a valid peer address. */
            if (memcmp(option->data, ncp->options.ipv6cp.local_address, IPV6CP_ADDR_LEN) != 0
                && memcmp(option->data, PPP_Null_Ip, IPV6CP_ADDR_LEN) != 0)
            {
                /* Add the remote address to the NCP data structure. */
                memcpy(ncp->options.ipv6cp.remote_address, option->data, IPV6CP_ADDR_LEN);
            }
            else
            {
                /* If we are not already filling a REJECT packet, then it is safe
                   to NAK this option. */
                if (status != PPP_FLAG_REJECT)
                {
                    /* The remote address is invalid, so generate another. */
                    memcpy(option->data, ncp->options.ipv6cp.remote_address, IPV6CP_ADDR_LEN);

                    /* Append this option to the outgoing NAK packet. If we are
                       currently filling an ACK, then it will be converted to
                       a NAK. */
                    status = LCP_Nak_Option(ack_frame, option->code, (UINT8)(option->len - LCP_OPTION_HDRLEN), option->data);
                }
            }

            break;
#endif

        default:
        reject:
            /* If we make it here then this is an option that we do not
               support, so send a reject. */
            status = LCP_Reject_Option(ack_frame, option->code, (UINT8)(option->len - LCP_OPTION_HDRLEN), option->data);

            break;

        } /* switch */

        /* Setup for next option. */
        len = len + option->len;
    }

#if (INCLUDE_IPV6 == NU_TRUE)
    /* Make sure the peer tried to negotiate the address. */
    if (ncp->protocol_code == PPP_IPV6_CONTROL_PROTOCOL && status != PPP_FLAG_REJECT)
    {
        if (memcmp(PPP_Null_Ip, ncp->options.ipv6cp.remote_address, IPV6CP_ADDR_LEN) == 0)
        {
            /* The remote address was not included in the request, so send a
               NAK with this option to solicit a request. */
            status = LCP_Nak_Option(ack_frame, NCP_IPV6_ADDRESS, IPV6CP_ADDR_LEN, ncp->options.ipv6cp.remote_address);
        }
    }
#endif

    /* If we need to send a response, then *ack_buf will contain
       the buffer to send. */
    if (status == PPP_FLAG_REJECT)
    {
        /* REJECT packet is already built and ready to send. */
        PrintInfo("      Send Reject.\n");
    }
    else if (status == PPP_FLAG_NAK)
    {
        /* NAK packet is already built and ready to send. */
        PrintInfo("      Send NAK.\n");
    }
    else
    {
        PrintInfo("      Send ACK.\n");

        /* Everything in the configuration packet was ok, so copy the whole
           thing into the response buffer and change the code to ACK. */
        memcpy(ack_frame->data, lcp_frame->data, lcp_frame->len - 4);
        ack_frame->len = lcp_frame->len;
    }

    return status;

} /* NCP_Process_Request */



/*************************************************************************
* FUNCTION
*
*     NCP_Process_Nak
*
* DESCRIPTION
*
*     Check an incoming NAK or REJECT packet to see if we can handle
*     the NAKed or REJECTed options. If an option is NAKed, we will
*     adjust the settings in the ipcp structure so the next request
*     will be correct. If an option is NAKed, we will either disable
*     the option if it is not needed, or abort negotiations if it is.
*
* INPUTS
*
*     *ncp
*     nak_type
*     *in_buf_ptr
*
* OUTPUTS
*
*     STATUS
*
*************************************************************************/
STATUS NCP_Process_Nak(NCP_LAYER *ncp, UINT8 nak_type, NET_BUFFER *in_buf_ptr)
{
    DV_DEVICE_ENTRY *dev_ptr;
    LINK_LAYER      *link_layer;
    LCP_FRAME       *lcp_frame;
    LCP_OPTION      *option;
    STATUS          status = PPP_FLAG_OK;
    UINT16          len = 0;

    /* Get a pointer to the IPCP structure for this device. */
    dev_ptr = (DV_DEVICE_ENTRY *)in_buf_ptr->mem_buf_device;
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
    lcp_frame = (LCP_FRAME*)in_buf_ptr->data_ptr;

#if (PPP_DEBUG_PRINT_OK == NU_TRUE)
    if (nak_type == PPP_FLAG_NAK)
        PrintInfo("   NCP_Process_Nak\n");
    else
        PrintInfo("   NCP_Process_Reject\n");
#endif

    /* Make sure this is a response to our request. */
    if (lcp_frame->id == ncp->identifier)
    {
        /* Process each option in the nak/rej appropriately. */
        while (len + LCP_HEADER_LEN < lcp_frame->len)
        {
            /* Move to the next option. */
            option = (LCP_OPTION*)(&lcp_frame->data[len]);
            if (option->len == 0)
                break;

            /* Check out what the option is and see if we can handle it. */
            switch (option->code)
            {
#if (INCLUDE_IPV4 == NU_TRUE)
            case NCP_IP_ADDRESS:

                PrintOpt("      IPCP IP Address\n", 0);

                if (ncp->protocol_code == PPP_IP_CONTROL_PROTOCOL)
                {
                    /* If this is a NAK, then we must be a client receiving an IP
                       address assignment from a server. */
                    if (nak_type == PPP_FLAG_NAK)
                    {
                        if (link_layer->mode == PPP_CLIENT && memcmp(option->data, PPP_Null_Ip, IP_ADDR_LEN) != 0)
                        {
                            /* Copy the IP into the ipcp structure. */
                            memcpy(ncp->options.ipcp.local_address, option->data, IP_ADDR_LEN);
                        }
                        else
                        {
                            /* This means the peer is rejecting the address we
                               sent. Abort negotiations. */
                            status = PPP_FLAG_ABORT;
                            break;
                        }
                    }
                    else if (nak_type == PPP_FLAG_REJECT)
                    {
                        /* Without an IP address, no sense in continuing.
                           Abort negotiations. */
                        status = PPP_FLAG_ABORT;
                        break;
                    }
                }

                break;

            case NCP_PRIMARY_DNS_ADDRESS:
            case NCP_SECONDARY_DNS_ADDRESS:

            PrintOpt("      IPCP DNS Address\n", 0);

#if (INCLUDE_DNS == NU_TRUE)
                if (ncp->protocol_code == PPP_IP_CONTROL_PROTOCOL)
                {
                    if (link_layer->mode == PPP_CLIENT)
                    {
                        if (nak_type == PPP_FLAG_NAK)
                        {
                            /* Copy the address over to the IPCP structure. */
                            if (option->code == NCP_PRIMARY_DNS_ADDRESS)
                                memcpy(&ncp->options.ipcp.primary_dns_server, (CHAR*)option->data, IP_ADDR_LEN);
                            else
                                memcpy(&ncp->options.ipcp.secondary_dns_server, (CHAR*)option->data, IP_ADDR_LEN);
                        }
                        else if (nak_type == PPP_FLAG_REJECT)
                        {
                            /* Disable DNS usage in the IPCP structure. */
                            if (option->code == NCP_PRIMARY_DNS_ADDRESS)
                                ncp->options.ipcp.flags &= ~PPP_FLAG_DNS1;
                            else
                                ncp->options.ipcp.flags &= ~PPP_FLAG_DNS2;
                        }
                    }
                }
#endif
            break;
#endif /* #if (INCLUDE_IPV4 == NU_TRUE) */
#if (INCLUDE_IPV6 == NU_TRUE)
            case NCP_IPV6_ADDRESS:

                PrintOpt("      IPV6CP IP Address\n", 0);

                /* This is only valid for IPV6CP. */
                if (ncp->protocol_code != PPP_IPV6_CONTROL_PROTOCOL)
                    break;

                /* Make sure this is a valid local address. */
                if (memcmp(option->data, ncp->options.ipv6cp.remote_address, IPV6CP_ADDR_LEN) != 0
                    && memcmp(option->data, PPP_Null_Ip, IPV6CP_ADDR_LEN) != 0)
                {
                    /* Add the local address to the NCP data structure. */
                    memcpy(ncp->options.ipv6cp.local_address, option->data, IPV6CP_ADDR_LEN);
                }
                else if (nak_type == PPP_FLAG_REJECT || memcmp(option->data,
                         PPP_Null_Ip, IPV6CP_ADDR_LEN) == 0)
                {
                    /* Without an IP address, no sense in continuing.
                       Abort negotiations. */
                    status = PPP_FLAG_ABORT;
                    break;
                }

                break;
#endif
            default:

                PrintOpt("      Unrequested option (%.02x)\n", option->code);

                /* If we make it here then this is an option that we did not
                   request. Drop it. */
                break;

            } /* switch */

            /* Setup for next option. */
            len = len + option->len;
        }
    }
    else
        status = PPP_FLAG_DROP;

    return status;

} /* NCP_Process_Nak */



/*************************************************************************
* FUNCTION
*
*     NCP_Send_Config_Req
*
* DESCRIPTION
*
*     Sends a configure request packet to the peer. This packet contains
*     either an IP address that is being assigned to the peer or an IP
*     address of zero, stating that we need to be assigned an IP address.
*
* INPUTS
*
*     *ncp
*     *dev_ptr
*
* OUTPUTS
*
*     None
*
*************************************************************************/
VOID NCP_Send_Config_Req(NCP_LAYER *ncp, DV_DEVICE_ENTRY *dev_ptr)
{
    NET_BUFFER          *buf_ptr;
    LCP_FRAME   HUGE    *lcp_frame;
#if (INCLUDE_DNS == NU_TRUE)
    LINK_LAYER          *link_layer;

    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
#endif

    PrintInfo("   NCP_Send_Config_Req\n");

    /* If this function is not called as a result of a timeout, then there
       may be a lingering RESEND event in the TQ. Remove it now. */
    TQ_Timerunset(ncp->event_id, TQ_CLEAR_EXACT, (UNSIGNED)dev_ptr, NCP_SEND_CONFIG);

    /* Create an outgoing request packet. Most of the fields will
       be pre-filled. */
    buf_ptr = LCP_New_Buffer(dev_ptr, LCP_CONFIGURE_REQUEST, -1);

    if (buf_ptr == NU_NULL)
        /* This error is logged within LCP_New_Buffer, so just return. */
        return;

    /* Get a pointer to the data part of the NCP packet. */
    lcp_frame = (LCP_FRAME*)buf_ptr->data_ptr;

    /* Save the identifier that was assigned to this packet. */
    ncp->identifier = lcp_frame->id;

#if (INCLUDE_IPV6 == NU_TRUE)
    if (ncp->protocol_code == PPP_IPV6_CONTROL_PROTOCOL)
    {
        /* Add the local IP address configuration option. */
        LCP_Append_Option((LCP_FRAME *)lcp_frame, NCP_IPV6_ADDRESS,
                          IPV6CP_ADDR_LEN,
                          ncp->options.ipv6cp.local_address);
    }
#if (INCLUDE_IPV4 == NU_TRUE)
    else if (ncp->protocol_code == PPP_IP_CONTROL_PROTOCOL)
#endif
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
    {
        /* Add the local IP address configuration option. */
        LCP_Append_Option((LCP_FRAME *)lcp_frame, NCP_IP_ADDRESS,
                          IPCP_ADDR_LEN, ncp->options.ipcp.local_address);

#if (INCLUDE_DNS == NU_TRUE)
        /* If we are a client then we may want to get some DNS
           server addresses. */
        if (link_layer->mode == PPP_CLIENT)
        {
            /* Do we want to request a primary DNS server address? */
            if (ncp->options.ipcp.flags & PPP_FLAG_DNS1)
                LCP_Append_Option((LCP_FRAME *)lcp_frame,
                                  NCP_PRIMARY_DNS_ADDRESS,
                                  IPCP_ADDR_LEN,
                                  ncp->options.ipcp.primary_dns_server);

            /* Do we want to request a secondary DNS server address? */
            if (ncp->options.ipcp.flags & PPP_FLAG_DNS2)
                LCP_Append_Option((LCP_FRAME *)lcp_frame,
                                  NCP_SECONDARY_DNS_ADDRESS, 4,
                                  ncp->options.ipcp.secondary_dns_server);
        }
#endif

    }
#endif /* #if (INCLUDE_IPV4 == NU_TRUE) */

    /* Send the configure request packet. */
    LCP_Send(dev_ptr, buf_ptr, ncp->protocol_code);

    /* Set a timer event for retransmitting if necessary. */
    TQ_Timerset(ncp->event_id, (UNSIGNED)dev_ptr, LCP_TIMEOUT_VALUE, NCP_SEND_CONFIG);

} /* NCP_IP_Send_Config_Req */




/*************************************************************************
* FUNCTION
*
*     NCP_Event_Handler
*
* DESCRIPTION
*
*     Handles processing of NCP events.
*
* INPUTS
*
*     TQ_EVENT          evt             Event identifier.
*     DV_DEVICE_ENTRY   dat             Pointer to PPP device structure.
*     UNSIGNED          subevt          PPP event to be serviced.
*
* OUTPUTS
*
*     None
*
*************************************************************************/
VOID NCP_Event_Handler(TQ_EVENT evt, UNSIGNED dat, UNSIGNED subevt)
{
    DV_DEVICE_ENTRY         *dev_ptr = (DV_DEVICE_ENTRY*)dat;
    LINK_LAYER              *link_layer;
    LCP_LAYER               *lcp;
    NCP_LAYER               *ncp = NU_NULL;
    AUTHENTICATION_LAYER    *auth;
    STATUS                  status;

#if(INCLUDE_PPP_MP == NU_TRUE)
    MIB2_IF_STACK_STRUCT    *stack_entry;
#endif

    /* Get a pointer to the link structures. */
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
    lcp = &link_layer->lcp;
    auth = &link_layer->authentication;

#if (INCLUDE_IPV4 == NU_TRUE)
    if (evt == IPCP_Event)
        ncp = &link_layer->ncp;

#if (INCLUDE_IPV6 == NU_TRUE)
	else	
#endif
#endif

    /* Get a pointer to the NCP structure. */
#if (INCLUDE_IPV6 == NU_TRUE)
    if (evt == IPV6CP_Event)
        ncp = &link_layer->ncp6;
		
	else
		return;
#endif


#if ((INCLUDE_IPV4 != NU_TRUE) || (INCLUDE_IPV6 != NU_TRUE))
   
	/* Adding this check to remove Klocworks error. */
    if (ncp == NU_NULL)
	    return;
#endif

    /* Switch on the event and do the appropriate processing. */
    switch (subevt)
    {
    case NCP_OPEN_REQUEST:

        /* Only negotiate this NCP if it is not already open. */
        if (ncp->state == OPENED)
            break;

        PrintInfo("Starting NCP negotiation...\n");

        /* Set the status of the PPP connection attempt. */
        link_layer->connection_status = NU_PPP_NETWORK_NEGOTIATION;

        /* Initialize the number of retransmit attempts. */
        ncp->num_transmissions = LCP_MAX_CONFIGURE;

    case NCP_SEND_CONFIG:

        /* If LCP is not open, open it. */
        if (lcp->state != OPENED)
        {
            PrintErr("   NCP event entered before LCP is open...\n");
            break;
        }

        /* If not authenticated, do so now. */
        else if (auth->state != AUTHENTICATED)
        {
            PrintErr("   NCP event entered without authentication...\n");
            break;
        }
        else if (ncp->num_transmissions-- > 0)
        {
            PrintInfo("   Sending NCP config request...\n");

            /* Send request to the host */
            NCP_Send_Config_Req(ncp, dev_ptr);
        }
        else
        {
            ncp->state = STOPPED;
            EQ_Put_Event(evt, (UNSIGNED)dev_ptr, NCP_LAYER_DOWN);
#if (INCLUDE_IPV4 == NU_TRUE)
            if (evt == IPCP_Event)
                NU_Set_Events(&link_layer->negotiation_progression, PPP_NCP_FAIL, NU_OR);
#endif
        }

        break;

    case NCP_LAYER_UP:

        /* Clear any remaining timeout events for this event only. */
        TQ_Timerunset(evt, TQ_CLEAR_EXACT, (UNSIGNED)dev_ptr, NCP_SEND_CONFIG);

#if (INCLUDE_IPV6 == NU_TRUE)
        if (evt == IPV6CP_Event)
        {
            PrintInfo("   IPV6CP layer up.\n");

            /* Attach IP addresses to this device and set the route. */
            status = PPP6_Attach_IP_Address(dev_ptr);
            if (status != NU_SUCCESS)
            {
                /* Log error. */
                NLOG_Error_Log("Failed to bind IPv6 address to device.", NERR_SEVERE, __FILE__, __LINE__);
                EQ_Put_Event(evt, (UNSIGNED)dev_ptr, NCP_CLOSE_REQUEST);
            }
        }

#if (INCLUDE_IPV4 == NU_TRUE)
        else if (evt == IPCP_Event)
#endif
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
        if(!(ncp->options.ipcp.flags & PPP_FLAG_NO_IPV4))
        {
            PrintInfo("   IPCP layer up.\n");

            /* Check to see if we have been able to negotiate remote address. */
            if(memcmp(ncp->options.ipcp.remote_address, PPP_Null_Ip, IP_ADDR_LEN) != 0)
            {
                /* Attach IP addresses to this device and set the route. */
                status = PPP_Attach_IP_Address(dev_ptr);
            }

            else
                status = NU_NOT_PRESENT;

            if (status != NU_SUCCESS)
            {
                /* Log error. */
                NLOG_Error_Log("Failed to bind IPv4 address to device.", NERR_SEVERE, __FILE__, __LINE__);
                EQ_Put_Event(evt, (UNSIGNED)dev_ptr, NCP_CLOSE_REQUEST);
                break;
            }
        }

        if ((link_layer->ncp.state == OPENED)
            || (link_layer->ncp.options.ipcp.flags & PPP_FLAG_NO_IPV4))
        {
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
            if (!(dev_ptr->dev_flags & DV6_IPV6) || link_layer->ncp6.state == OPENED)
#endif
            {
#if(INCLUDE_PPP_MP == NU_TRUE)
                /* Check if this device is an MP device (virtual). */
                if(link_layer->hwi.itype & PPP_ITYPE_VIRTUAL)
                {
                    /* Get the stack entry associated with this MP device. */
                    stack_entry = MIB2_If_Stack_Get_HI_Entry(dev_ptr->dev_index + 1, NU_TRUE);

                    /* Check if we have got the entry and a PPP device. */
                    if(stack_entry && stack_entry->mib2_lower_layer)
                    {
                        /* Point to the link layer of the PPP device. */
                        link_layer = stack_entry->mib2_lower_layer->dev_link_layer;
#if (INCLUDE_IPV4 == NU_TRUE)
                        /* Set the ncp state of the PPP device. */
                        link_layer->ncp.state = OPENED;
#endif
                    }
                }
#endif
                /* Clear any remaining timeout events. */
                TQ_Timerunset(PPP_Event, TQ_CLEAR_EXACT, (UNSIGNED)dev_ptr, PPP_STOP_NEGOTIATION);

                /* Let the application know that the network is up. */
                NU_Set_Events(&link_layer->negotiation_progression, PPP_CONFIG_SUCCESS, NU_OR);
            }
#if (INCLUDE_IPV4 == NU_TRUE)
        }
#endif
        break;

    case NCP_CLOSE_REQUEST:

        PrintInfo("Closing NCP layer...\n");

        /* Set the status of the PPP connection attempt. */
        link_layer->connection_status = NU_PPP_NETWORK_NEGOTIATION;

#if(INCLUDE_PPP_MP == NU_TRUE)
        /* Get the stack entry associated with this MP device. */
        stack_entry = MIB2_If_Stack_Get_LI_Entry(dev_ptr->dev_index + 1, NU_TRUE);

        /* Check if we have got the entry and a PPP device. */
        if(stack_entry && stack_entry->mib2_higher_layer)
        {
            if(((PPP_MP_BUNDLE *)stack_entry->mib2_higher_layer->dev_extension)->mp_num_links == 1)
            {
                /* Close any IP communications that use this link. */
                NCP_Close_IP_Layer(stack_entry->mib2_higher_layer);
            }
        }
        else
#endif
            /* Close any IP communications that use this link. */
            NCP_Close_IP_Layer(dev_ptr);

        /* Initialize the number of retransmit attempts. */
        ncp->num_transmissions = LCP_MAX_TERMINATE;

    case NCP_SEND_TERMINATE:

        /* Only send terminate requests during a normal close. */
        if (lcp->state == OPENED)
        {
            if (ncp->state == CLOSING)
            {
                if (ncp->num_transmissions-- > 0)
                {
                    PrintInfo("   Sending NCP terminate request...\n");

                    /* Send a terminate request for NCP. */
                    LCP_Send_Terminate_Req(dev_ptr, ncp->protocol_code);

                    /* Set up a timeout event for NCP close. */
                    TQ_Timerset(evt, (UNSIGNED)dev_ptr, LCP_TIMEOUT_VALUE, NCP_SEND_TERMINATE);
                    break;
                }
                else
                    ncp->state = CLOSED;
            }
            else
            {
                /* Send a terminate ack. */
                LCP_Send_Terminate_Ack(dev_ptr, ncp->protocol_code);
                ncp->state = STOPPED;
            }
        }

    case NCP_LAYER_DOWN:

        PrintInfo("   NCP layer down.\n");

        if (lcp->state == OPENED)
            lcp->state = CLOSING;

        /* If the LCP layer is stopped then stop the NCP layer as well */
        if(lcp->state == STOPPED)
            ncp->state = STOPPED;

        if (lcp->state == CLOSING)
            /* Close LCP if no more NCPs are using it. */
            EQ_Put_Event(PPP_Event, (UNSIGNED)dev_ptr, LCP_CLOSE_REQUEST);
        else
        {
            /* Send a terminate ack. */
            lcp->state = STOPPED;
            EQ_Put_Event(PPP_Event, (UNSIGNED)dev_ptr, LCP_SEND_TERMINATE);
        }


    default:
        UNUSED_PARAMETER(evt);
        break;

    } /* end switch */

    return;

} /* NCP_Event_Handler */



/*************************************************************************
* FUNCTION
*
*     NCP_Close_IP_Layer
*
* DESCRIPTION
*
*     This function takes care of shutting down any upper layer
*     connections before disconnecting the device.
*
* INPUTS
*
*     DV_DEVICE_ENTRY   *dev_ptr        Pointer to the device structure
*                                        for this device.
*
* OUTPUTS
*
*     None
*
*************************************************************************/
VOID NCP_Close_IP_Layer(DV_DEVICE_ENTRY *dev_ptr)
{
#if (INCLUDE_SOCKETS == NU_TRUE)
    /* Close the sockets layer. */
    SCK_Kill_All_Open_Sockets(dev_ptr);
#endif

    /* Close the IP layer. */
    NCP_Clean_Up_Link(dev_ptr);

    /* Make sure the transmit queue is flushed. */
    MEM_Buffer_Cleanup(&dev_ptr->dev_transq);
    dev_ptr->dev_transq_length = 0;

} /* NCP_Close_IP_Layer */



/*************************************************************************
* FUNCTION
*
*     NCP_Clean_Up_Link
*
* DESCRIPTION
*
*     This function removes all routes that were added when a PPP device
*     became active. It will also remove any DNS servers added by the PPP
*     device. This is used when the link is going down.
*
* INPUTS
*
*     DV_DEVICE_ENTRY   *dev_ptr        Pointer to device structure
*
* OUTPUTS
*
*    None
*
*************************************************************************/
VOID NCP_Clean_Up_Link(DV_DEVICE_ENTRY *dev_ptr)
{
#if (INCLUDE_DNS == NU_TRUE || INCLUDE_IPV4 == NU_TRUE)
    LINK_LAYER *link_layer;

    /* Get a pointer to the PPP structure. */
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
    /* Delete all routes using this device */
    if (dev_ptr->dev_flags & DV6_IPV6)
        RTAB6_Delete_Routes_For_Device(dev_ptr);
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    if(!(link_layer->ncp.options.ipcp.flags & PPP_FLAG_NO_IPV4))
    {
        /* Delete all routes using this device */
        RTAB4_Delete_Routes_For_Device(dev_ptr->dev_net_if_name);

#if (INCLUDE_DNS == NU_TRUE)
        /* Remove primary DNS server. */
        if (IP_ADDR(link_layer->ncp.options.ipcp.primary_dns_server) != NU_NULL)
        {
            NU_Delete_DNS_Server(link_layer->ncp.options.ipcp.primary_dns_server);
            memset(&link_layer->ncp.options.ipcp.primary_dns_server, 0, IPCP_ADDR_LEN);
        }

        /* Remove secondary DNS server. */
        if (IP_ADDR(link_layer->ncp.options.ipcp.secondary_dns_server) != NU_NULL)
        {
            NU_Delete_DNS_Server(link_layer->ncp.options.ipcp.secondary_dns_server);
            memset(&link_layer->ncp.options.ipcp.secondary_dns_server, 0, IPCP_ADDR_LEN);
        }
#endif
    }
#endif /* INCLUDE_IPV4 == NU_TRUE */

#if (INCLUDE_IPV6 == NU_TRUE)
    /* Detach the IPv6 address from this device. */
    if (dev_ptr->dev_flags & DV6_IPV6)
        PPP6_Detach_IP_Address(dev_ptr);
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
    if(!(link_layer->ncp.options.ipcp.flags & PPP_FLAG_NO_IPV4))
    {
        /* Detach the IPv4 address from this device. */
        DEV_Detach_IP_From_Device(dev_ptr->dev_net_if_name);
    }
#endif

} /* NCP_Clean_Up_Link */
