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
*       ccp.c
*
*   COMPONENT
*
*       CCP - Compression Control Protocol
*
*   DESCRIPTION
*
*       This file contains the compression control protocol used by PPP to
*       establish and negotiate the configuration options that will
*       be used for the link.
*
*   DATA STRUCTURES
*
*       PPP_CCP_Event
*
*   FUNCTIONS
*
*       CCP_Init
*       CCP_Reset
*       CCP_Interpret
*       CCP_Data_Interpret
*       CCP_Send_Config_Req
*       CCP_Check_Config_Req
*       CCP_Check_Config_Ack
*       CCP_Check_Config_Nak
*       CCP_Check_Reset_Req
*       CCP_Check_Terminate_Req
*       CCP_Check_Terminate_Ack
*       CCP_Event_Handler
*       CCP_Process_Request
*       CCP_Process_Nak
*       CCP_Send_Protocol_Reject
*       CCP_Send_Code_Reject
*       CCP_Process_Code_Reject
*       CCP_Process_Protocol_Reject
*       CCP_Send_Echo_Req
*       CCP_Process_Echo_Req
*       CCP_Send_Terminate_Req
*       CCP_Send_Terminate_Ack
*
*   DEPENDENCIES
*
*       nu_ppp.h
*       ccp_defs.h
*
*************************************************************************/
#include "drivers/nu_ppp.h"
#include "drivers/ccp_defs.h"

#if (PPP_ENABLE_MPPE == NU_TRUE)

#if (CCP_DEBUG_PRINT_OK == NU_TRUE)
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

TQ_EVENT PPP_CCP_Event;

/*************************************************************************
* FUNCTION
*
*     CCP_Init
*
* DESCRIPTION
*
*     Initializes the CCP layer of PPP. In general, it sets up
*     the default values that will be used each time the device
*     is initialized. 
*
* INPUTS
*
*     *dev_ptr                          Pointer to the device that this
*                                       CCP layer belongs to.
*
* OUTPUTS
*
*     NU_PPP_INIT_FAILURE               Initialization failed.
*     NU_SUCCESS                        Success.
*
*************************************************************************/
STATUS CCP_Init(DV_DEVICE_ENTRY *dev_ptr)
{
    LINK_LAYER      *link_layer;
    static CHAR     init_once = 0;
    STATUS          status = NU_SUCCESS;

    if (init_once == 0)
    {
        init_once++;

        /* Register CCP Event */
        status = EQ_Register_Event(PPP_Event_Forwarding, &PPP_CCP_Event);

        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to register PPP_CCP event.", 
                           NERR_SEVERE, __FILE__, __LINE__);

            return (NU_PPP_INIT_FAILURE);
        }
    }

    /* Get the link_layer pointer */
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

    /* Clearing out the data structures. */
    UTL_Zero(&link_layer->ccp.options.local, sizeof(CCP_OPTIONS));
    UTL_Zero(&link_layer->ccp.options.remote, sizeof(CCP_OPTIONS));

    /* Initialize CCP flag. */
    link_layer->ccp.options.local.mppe.mppe_require_encryption = 
        PPP_REQUIRE_ENCRYPTION;

    /* Initialize the CCP negotiation flags based on 
     * initial configuration. */
#if (PPP_USE_40_BIT_ENCRYPTION == NU_TRUE)
    link_layer->ccp.options.local.mppe.mppe_default_flags |= 
                                            CCP_FLAG_L_BIT;
#endif

#if (PPP_USE_56_BIT_ENCRYPTION == NU_TRUE)
    link_layer->ccp.options.local.mppe.mppe_default_flags |= 
                                            CCP_FLAG_M_BIT;
#endif

#if (PPP_USE_128_BIT_ENCRYPTION == NU_TRUE)
    link_layer->ccp.options.local.mppe.mppe_default_flags |= 
                                            CCP_FLAG_S_BIT;
#endif

    /* Set the H flag according to the encryption mode macro. */
#if ((CCP_ENCRYPTION_MODE == CCP_USE_BOTH_ENCRYPTIONS) || \
     (CCP_ENCRYPTION_MODE == CCP_USE_STATELESS_ONLY))
    link_layer->ccp.options.local.mppe.mppe_default_flags |= 
                                            CCP_FLAG_H_BIT;
#endif

    /* Initialize the concluded bits to default flags. */
    link_layer->ccp.options.local.mppe.mppe_ccp_supported_bits = 
        link_layer->ccp.options.local.mppe.mppe_default_flags;

    return (status);

} /* CCP_Init */

/*************************************************************************
*
*   FUNCTION
*
*       CCP_Reset
*
*   DESCRIPTION
*
*       Resets the CCP configuration options to their default values.
*       This is done in preparation for a new PPP connection.
*
*   INPUTS
*
*       *link_layer                    Pointer to the CCP structure to
*                                      be reset.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID CCP_Reset(LINK_LAYER *link_layer)
{
    CCP_LAYER *ccp = &link_layer->ccp;

    /* Reset the options we will request during negotiation. */
    ccp->options.local.mppe.mppe_ccp_supported_bits = 
        ccp->options.local.mppe.mppe_default_flags;

    /* Reset the various CCP flags */
    ccp->options.local.mppe.mppe_encrypt_packets = 0;
    link_layer->ccp.options.local.mppe.mppe_reset_requested = 0;

} /* CCP_Reset */

/*************************************************************************
* FUNCTION
*
*     CCP_Interpret
*
* DESCRIPTION
*
*     Passes the incoming packet to its associated handler. After
*     the packet has been processed it is deallocated from the
*     buffer list.
*
* INPUTS
*
*     *buf_ptr                          Pointer to NET_BUFFER.
*
* OUTPUTS
*
*     None
*
*************************************************************************/
VOID CCP_Interpret(NET_BUFFER *buf_ptr)
{
    LINK_LAYER      *link_layer;
    CCP_FRAME       *ccp_frame;
    DV_DEVICE_ENTRY *dev_ptr = buf_ptr->mem_buf_device;

    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

    PrintInfo("CCP_Interpret\n");

    /* Swap the 2-byte packet length. */
    ccp_frame = (CCP_FRAME*)buf_ptr->data_ptr;
    ccp_frame->len = INTSWAP(ccp_frame->len);

    /* Switch on the CCP type. */
    switch (ccp_frame->code)
    {
    case CCP_CONFIGURE_REQUEST:
        CCP_Check_Config_Req(buf_ptr);
        break;

    case CCP_CONFIGURE_ACK:
        CCP_Check_Config_Ack(buf_ptr);
        break;

    case CCP_CONFIGURE_NAK:
        CCP_Check_Config_Nak(PPP_FLAG_NAK, buf_ptr);
        break;

    case CCP_CONFIGURE_REJECT:
        CCP_Check_Config_Nak(PPP_FLAG_REJECT, buf_ptr);
        break;

    case CCP_TERMINATE_REQUEST:
        link_layer->term_id = ccp_frame->id;
        CCP_Check_Terminate_Req(buf_ptr);
        break;

    case CCP_TERMINATE_ACK:
        CCP_Check_Terminate_Ack(buf_ptr);
        break;

    case CCP_CODE_REJECT:
        CCP_Process_Code_Reject(buf_ptr);
        break;

    case CCP_RESET_REQUEST:
        CCP_Check_Reset_Req(buf_ptr);
        break;

    case CCP_RESET_ACK:
        break;

    default:

        PrintInfo("    CCP Unknown code!\n");

        /* Reject any unknown CCP codes. */
        CCP_Send_Code_Reject(buf_ptr, PPP_CCP_PROTOCOL);

        break;
    }

    /* Release the buffer space. */
    MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

} /* CCP_Interpret */

/*************************************************************************
* FUNCTION
*
*     CCP_Data_Interpret
*
* DESCRIPTION
*
*     Passes the incoming packet to MPPE component for decryption.
*
* INPUTS
*
*     *buf_ptr                          Pointer to NET_BUFFER
*
* OUTPUTS
*
*     None
*
*************************************************************************/
VOID CCP_Data_Interpret(NET_BUFFER *buf_ptr)
{
    LINK_LAYER      *link_layer;
    CCP_LAYER       *ccp;
    UINT16          temp, coherency_count;
    UINT8           flags;
    STATUS          status = CCP_GEN_ERROR;
    DV_DEVICE_ENTRY *dev_ptr = buf_ptr->mem_buf_device;

    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
    ccp = &link_layer->ccp;

    PrintInfo("CCP_Data_Interpret\n");

    /* Determine whether MPPE is being used or not. If it is being
     * used, then send the packet to its decryption module. */
    if (ccp->options.local.mppe.mppe_encrypt_packets == NU_TRUE)
    {
        /* Getting coherency count. */
        temp = GET16(buf_ptr->data_ptr, 0);
        coherency_count = temp & CCP_CCOUNT_MASK;

        /* Get the flags A B C D. */
        flags = (temp & CCP_FLAGS_MASK) >> CCP_FLAGS_SHIFT;

        ccp->options.remote.mppe.mppe_coherency_count = coherency_count;
        ccp->options.remote.mppe.mppe_cntrl_bits = flags;

        /* If reset flag is not set then we have to decrypt the packet.
        * Otherwise, we have to wait till a packet with A bit set 
        * is received. */
        if ((ccp->options.local.mppe.mppe_reset_flag == NU_FALSE) ||
            ((flags & CCP_FLAG_A_BIT) != 0))
        {
            /* Chop off the CCP header. */
            buf_ptr->data_ptr += PPP_PROTOCOL_HEADER_2BYTES;
            buf_ptr->data_len -= PPP_PROTOCOL_HEADER_2BYTES;
            buf_ptr->mem_total_data_len -= PPP_PROTOCOL_HEADER_2BYTES;

            /* If a packet is lost in Stateful mode then 
             * send reset request. */
            if ((ccp->options.local.mppe.mppe_stateless == NU_FALSE) &&
                (ccp->options.local.mppe.mppe_reset_flag != NU_TRUE) && 
                ((ccp->options.remote.mppe.mppe_coherency_count -
                ccp->options.local.mppe.mppe_coherency_count) > 1))
            {
                CCP_Send_Reset_Req(dev_ptr, PPP_CCP_PROTOCOL);
                ccp->options.local.mppe.mppe_reset_flag = NU_TRUE;
            }
            else if (flags & CCP_FLAG_D_BIT)
            {
                /* Decrypt the packet. */
                status = MPPE_Decrypt(buf_ptr, dev_ptr);

                if (status == NU_SUCCESS)
                {
                    /* Pass the packet to PPP_Input. */
                    PPP_Input();
                }
            }
        }
        else
        {
            CCP_Send_Reset_Req(dev_ptr, PPP_CCP_PROTOCOL);
            ccp->options.local.mppe.mppe_reset_flag = NU_TRUE;
        }
    }
    else
    {
        /* Normally we should not be here. */
        NLOG_Error_Log("Invalid packet received. ",
            NERR_INFORMATIONAL, __FILE__, __LINE__);
    }

    if (status != NU_SUCCESS)
    {
        /* Release the buffer space after dropping the packet. */
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);
    }

} /* CCP_Data_Interpret */

/*************************************************************************
* FUNCTION
*
*     CCP_Send_Config_Req
*
* DESCRIPTION
*
*     Sends a CCP configure request packet to the peer with the
*     options we want to negotiate.
*
* INPUTS
*
*     *dev_ptr                          Pointer to the Device Structure.
*
* OUTPUTS
*
*     None
*
*************************************************************************/
VOID CCP_Send_Config_Req(DV_DEVICE_ENTRY *dev_ptr)
{
    NET_BUFFER          *buf_ptr;
    CCP_FRAME   HUGE    *ccp_frame;
    LINK_LAYER          *link_layer;
    UINT8               temp_ptr[CCP_MPPC_MPPE_LENGTH];
    CCP_LAYER           *ccp;
    UINT32              enc_flags = 0;

    /* Get some pointers to data */
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
    ccp = &link_layer->ccp;

    /* If this function is not called as a result of a timeout, then 
       there may be a lingering RESEND event in the TQ. Remove it now. */
    TQ_Timerunset(PPP_CCP_Event, TQ_CLEAR_EXACT, (UNSIGNED)dev_ptr, 
                  CCP_SEND_CONFIG);

    PrintInfo("   CCP_Send_Config_Req\n");

    /* Create an outgoing request packet. Most of the fields will
       be pre-filled. If we fail to allocate a buffer, we can still set
       the timer event to try again later. */
    buf_ptr = CCP_New_Buffer(dev_ptr, CCP_CONFIGURE_REQUEST, -1);

    if (buf_ptr != NU_NULL)
    {
        /* Get a pointer to the data part of the CCP packet. */
        ccp_frame = (CCP_FRAME*)buf_ptr->data_ptr;

        /* Save the identifier that was assigned to this packet. */
        ccp->identifier = ccp_frame->id;

#if (CCP_ENCRYPTION_MODE != CCP_USE_STATEFUL_ONLY)
        if (ccp->options.local.mppe.mppe_default_flags & CCP_FLAG_H_BIT &
            ccp->options.local.mppe.mppe_ccp_supported_bits)
        {
            enc_flags |= CCP_FLAG_H_BIT;
        }
#endif

        /* Setting the appropriate flags to be sent */
        if (ccp->options.local.mppe.mppe_default_flags & CCP_FLAG_L_BIT &
            ccp->options.local.mppe.mppe_ccp_supported_bits)
        {
            enc_flags |= CCP_FLAG_L_BIT;
            ccp->options.local.mppe.mppe_key_length = MPPE_40_BIT;
        }

        if (ccp->options.local.mppe.mppe_default_flags & CCP_FLAG_M_BIT &
            ccp->options.local.mppe.mppe_ccp_supported_bits)
        {
            enc_flags |= CCP_FLAG_M_BIT;
            ccp->options.local.mppe.mppe_key_length = MPPE_56_BIT;
        }

        if (ccp->options.local.mppe.mppe_default_flags & CCP_FLAG_S_BIT &
            ccp->options.local.mppe.mppe_ccp_supported_bits)
        {
            enc_flags |= CCP_FLAG_S_BIT;
            ccp->options.local.mppe.mppe_key_length = MPPE_128_BIT;
        }

        /* Setting temporary concluded bits. */
        ccp->options.local.mppe.mppe_ccp_supported_bits = enc_flags;

        PUT32(temp_ptr, 0, enc_flags);

        /* Append the options in outgoing frame */
        CCP_Append_Option((CCP_FRAME *)ccp_frame, CCP_MICROSOFT_SPECIFIC,
                          (CCP_MPPC_MPPE_LENGTH - CCP_OPTION_HDRLEN), temp_ptr);

        /* Send the configure request packet. */
        CCP_Send(dev_ptr, buf_ptr, PPP_CCP_PROTOCOL);
    }

    /* Set a timer event for retransmitting if necessary. */
    TQ_Timerset(PPP_CCP_Event, (UNSIGNED)dev_ptr, CCP_TIMEOUT_VALUE, 
                CCP_SEND_CONFIG);

} /* CCP_Send_Config_Req */

/*************************************************************************
* FUNCTION
*
*     CCP_Check_Config_Req
*
* DESCRIPTION
*
*     Handle an incoming Configure Request packet according to the PPP
*     state machine.
*
* INPUTS
*
*     *buf_ptr                          Pointer to NET_BUFFER
*
* OUTPUTS
*
*     None
*
*************************************************************************/
VOID CCP_Check_Config_Req(NET_BUFFER *buf_ptr)
{
    DV_DEVICE_ENTRY *dev_ptr;
    LINK_LAYER      *link_layer;
    CCP_LAYER       *ccp;
    STATUS          status;
    UINT8           state;
    NET_BUFFER      *ack_buf = NU_NULL;

    /* Get some pointers to our data. */
    dev_ptr = buf_ptr->mem_buf_device;
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
    ccp = &link_layer->ccp;

    /* Save current state. */
    state = ccp->state;

    PrintInfo("   CCP_Check_Config_Req\n");

    status = CCP_Process_Request(buf_ptr, &ack_buf);
    
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
                ccp->state = REQ_SENT;
            else
                ccp->state = ACK_SENT;

            /* Send the ACK/NAK/REJ. */
            if (ack_buf != NU_NULL)
                CCP_Send(dev_ptr, ack_buf, PPP_CCP_PROTOCOL);

            break;

        case ACK_RCVD:
            if (status == PPP_FLAG_ACK)
                ccp->state = OPENED;

            /* Send the ACK/NAK/REJ. */
            if (ack_buf != NU_NULL)
                CCP_Send(dev_ptr, ack_buf, PPP_CCP_PROTOCOL);

            break;

        case INITIAL:
            /* If encryption was not mandatory and a Config-Req was 
             * received then set the require_encryption flag for this 
             * connection and send a Config-Req. */
            if (ccp->options.local.mppe.mppe_require_encryption == NU_FALSE)
            {
                ccp->options.local.mppe.mppe_require_encryption = NU_TRUE;
                CCP_Send_Config_Req(dev_ptr);

                /* Set the next state. */
                ccp->state = REQ_SENT;
            }            
            break;

        case CLOSED:
        case CLOSING:
        case STOPPING:
        case STARTING:
        default:
            break;
        }

        PrintState("   CCP state == %s --> %s\n", StateStrings[state], 
                   StateStrings[ccp->state]);

        /* Perform response routine based on current state. */
        switch (state)
        {
        case CLOSED:
            CCP_Send_Terminate_Ack(dev_ptr, PPP_CCP_PROTOCOL);
            break;

        case STOPPED:
            ccp->num_transmissions = CCP_MAX_CONFIGURE;
            CCP_Send_Config_Req(dev_ptr);
            break;

        case ACK_RCVD:
            if (status == PPP_FLAG_ACK)
            {
                /* Set the CCP up event to start using the link. */
                EQ_Put_Event(PPP_CCP_Event, (UNSIGNED)dev_ptr, 
                             CCP_LAYER_UP);
            }

            break;

        case OPENED:
            if(ccp->state == ACK_SENT)
            {
                ccp->state = OPENED;
            }
            else
            {
                ccp->num_transmissions = CCP_MAX_CONFIGURE;
                CCP_Send_Config_Req(dev_ptr);
            }
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
} /* CCP_Check_Config_Req */

/*************************************************************************
* FUNCTION
*
*     CCP_Check_Config_Ack
*
* DESCRIPTION
*
*     Handle an incoming Configure ACK packet according to the PPP
*     state machine.
*
* INPUTS
*
*     *buf_ptr                          Pointer to NET_BUFFER
*
* OUTPUTS
*
*     None
*
*************************************************************************/
VOID CCP_Check_Config_Ack(NET_BUFFER *buf_ptr)
{
    DV_DEVICE_ENTRY *dev_ptr;
    LINK_LAYER      *link_layer;
    CCP_FRAME       *ccp_frame;
    CCP_OPTION      *option;
    CCP_LAYER       *ccp;
    UINT8           state;
    UINT32          len = 0;

    /* Get some pointers to our data. */
    dev_ptr = buf_ptr->mem_buf_device;
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
    ccp = &link_layer->ccp;
    ccp_frame = (CCP_FRAME*)buf_ptr->data_ptr;

    /* Unset the CCP_SEND_CONFIG timer to stop retransmission */
    TQ_Timerunset(PPP_CCP_Event, TQ_CLEAR_EXACT, (UNSIGNED)dev_ptr, 
                  CCP_SEND_CONFIG);

    /* Save current state. */
    state = ccp->state;

    PrintInfo("   CCP_Check_Config_Ack\n");

    /* Change to next state. */
    switch (state)
    {
    case REQ_SENT:
        ccp->state = ACK_RCVD;
        break;

    case ACK_SENT:
        ccp->state = OPENED;
        break;

    case ACK_RCVD:
    case OPENED:
        ccp->state = REQ_SENT;
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

    PrintState("   CCP state == %s --> %s\n", StateStrings[state], 
               StateStrings[ccp->state]);

    if (ccp_frame->id == ccp->identifier)
    {
        /* Process each option in the ACK appropriately. */
        while ((len + CCP_HEADER_LEN) < ccp_frame->len)
        {
            option = (CCP_OPTION*)(&ccp_frame->data[(UINT16)len]);
            if (option->len == 0)
                break;

            /* Move to the next option. */
            /* Pull the supported bits from the packet. */
            ccp->options.remote.mppe.mppe_ccp_supported_bits = 
                GET32(option->data, 0);

            /* Check out what the option is and see if we can handle it.*/
            switch (option->code)
            {
            case CCP_OPTION_MPPC_MPPE:
 
                /* Set the stateless flag if the peers agreed upon 
                 * stateless mode. */
                if((ccp->options.remote.mppe.mppe_ccp_supported_bits 
                    & CCP_FLAG_H_BIT))
                {
                    ccp->options.local.mppe.mppe_stateless = NU_TRUE;
                }
                else
                {
                    ccp->options.local.mppe.mppe_stateless = NU_FALSE;
                }

            default:
                break;
            }

            /* Setup for next option. */
            len += option->len;
        }
    }

    /* Perform response routine based on current state. */
    switch (state)
    {
    case CLOSED:
    case STOPPED:
        CCP_Send_Terminate_Ack(dev_ptr, PPP_CCP_PROTOCOL);
        break;

    case REQ_SENT:
        break;

    case ACK_SENT:

        /* Set the CCP up event to start using the link. */
        EQ_Put_Event(PPP_CCP_Event, (UNSIGNED)dev_ptr, CCP_LAYER_UP);

        break;

    case ACK_RCVD:
    case OPENED:
        ccp->num_transmissions = CCP_MAX_CONFIGURE;
        CCP_Send_Config_Req(dev_ptr);

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
} /* CCP_Check_Config_Ack */

/*************************************************************************
* FUNCTION
*
*     CCP_Check_Config_Nak
*
* DESCRIPTION
*
*     Handle an incoming Configure NAK packet according to the PPP
*     state machine.
*
* INPUTS
*
*     nak_type                          Type of NAK packet
*     *buf_ptr                          Pointer to NET_BUFFER
*
* OUTPUTS
*
*     None
*
*************************************************************************/
VOID CCP_Check_Config_Nak(UINT8 nak_type, NET_BUFFER *buf_ptr)
{
    DV_DEVICE_ENTRY *dev_ptr;
    LINK_LAYER      *link_layer;
    CCP_LAYER       *ccp;
    STATUS          status;
    UINT8           state;

    /* Get some pointers to our data. */
    dev_ptr = buf_ptr->mem_buf_device;
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
    ccp = &link_layer->ccp;

    /* Unset the CCP_SEND_CONFIG timer to stop retransmission */
    TQ_Timerunset(PPP_CCP_Event, TQ_CLEAR_EXACT, (UNSIGNED)dev_ptr, 
                  CCP_SEND_CONFIG);

    /* Save current state. */
    state = ccp->state;

    PrintInfo("   CCP_Check_Config_Nak\n");

    status = CCP_Process_Nak(nak_type, buf_ptr);

    if (status != PPP_FLAG_DROP)
    {
        /* Change to next state. */
        switch (state)
        {
        case REQ_SENT:
            if (status == PPP_FLAG_ABORT)
                ccp->state = STOPPING;
            break;

        case ACK_RCVD:
        case OPENED:
            ccp->state = REQ_SENT;
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

        PrintState("   CCP state == %s --> %s\n", StateStrings[state], 
                   StateStrings[ccp->state]);

        /* Perform response routine based on current state. */
        switch (state)
        {
        case CLOSED:
        case STOPPED:
            CCP_Send_Terminate_Ack(dev_ptr, PPP_CCP_PROTOCOL);
            break;

        case REQ_SENT:
        case ACK_SENT:
            if (status == PPP_FLAG_ABORT)
            {
                /* Set the CCP up event to start using the link. */
                EQ_Put_Event(PPP_CCP_Event, (UNSIGNED)dev_ptr, 
                             CCP_CLOSE_REQUEST);
            }
            else
            {
                CCP_Send_Config_Req(dev_ptr);
            }

            break;

        case ACK_RCVD:
        case OPENED:
            ccp->num_transmissions = CCP_MAX_CONFIGURE;
            CCP_Send_Config_Req(dev_ptr);

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
} /* CCP_Check_Config_Nak */

/*************************************************************************
* FUNCTION
*
*     CCP_Check_Terminate_Req
*
* DESCRIPTION
*
*     Handle an incoming Terminate Request packet according to the PPP
*     state machine.
*
* INPUTS
*
*     *buf_ptr                          Pointer to NET_BUFFER
*
* OUTPUTS
*
*     None
*
*************************************************************************/
VOID CCP_Check_Terminate_Req(NET_BUFFER *buf_ptr)
{
    DV_DEVICE_ENTRY *dev_ptr;
    LINK_LAYER      *link_layer;
    CCP_LAYER       *ccp;
    UINT8           state;

    /* Get some pointers to our data. */
    dev_ptr = buf_ptr->mem_buf_device;
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
    ccp = &link_layer->ccp;

    /* Save current state. */
    state = ccp->state;

    PrintInfo("   CCP_Check_Terminate_Req\n");

    /* Change to next state. */
    switch (state)
    {
    case REQ_SENT:
    case ACK_RCVD:
    case ACK_SENT:
        ccp->state = REQ_SENT;
        break;

    case OPENED:
        ccp->state = STOPPING;
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

    PrintState("   CCP state == %s --> %s\n", StateStrings[state], 
               StateStrings[ccp->state]);

    /* Perform response routine based on current state. */
    switch (state)
    {
    case OPENED:
        EQ_Put_Event(PPP_CCP_Event, (UNSIGNED)dev_ptr, CCP_CLOSE_REQUEST);
        break;

    case CLOSED:
    case STOPPED:
    case CLOSING:
    case STOPPING:
    case REQ_SENT:
    case ACK_RCVD:
    case ACK_SENT:
        EQ_Put_Event(PPP_CCP_Event, (UNSIGNED)dev_ptr, 
                     CCP_SEND_TERMINATE);
        break;

    case INITIAL:
    case STARTING:
    default:
        break;
    }
} /* CCP_Check_Terminate_Req */

/*************************************************************************
* FUNCTION
*
*     CCP_Check_Terminate_Ack
*
* DESCRIPTION
*
*     Handle an incoming Terminate ACK packet according to the PPP
*     state machine.
*
* INPUTS
*   
*     *buf_ptr                          Pointer to NET_BUFFER
*
* OUTPUTS
*
*     None
*
*************************************************************************/
VOID CCP_Check_Terminate_Ack(NET_BUFFER *buf_ptr)
{
    DV_DEVICE_ENTRY *dev_ptr;
    LINK_LAYER      *link_layer;
    CCP_LAYER       *ccp;
    UINT8           state;

    /* Get some pointers to our data. */
    dev_ptr = buf_ptr->mem_buf_device;
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
    ccp = &link_layer->ccp;

    TQ_Timerunset(PPP_CCP_Event, TQ_CLEAR_EXACT, (UNSIGNED)dev_ptr, 
                  CCP_SEND_TERMINATE);

    /* Save current state. */
    state = ccp->state;

    PrintInfo("   CCP_Check_Terminate_Ack\n");

    /* Change to next state. */
    switch (state)
    {
    case CLOSING:
        ccp->state = CLOSED;
        break;

    case STOPPING:
        ccp->state = STOPPED;
        break;

    case ACK_RCVD:
    case OPENED:
        ccp->state = REQ_SENT;
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

    PrintState("   CCP state == %s --> %s\n", StateStrings[state], 
               StateStrings[ccp->state]);

    /* Perform response routine based on current state. */
    switch (state)
    {
    case OPENED:
        ccp->num_transmissions = CCP_MAX_CONFIGURE;
        CCP_Send_Config_Req(dev_ptr);
        break;

    case CLOSING:
    case STOPPING:
        EQ_Put_Event(PPP_CCP_Event, (UNSIGNED)dev_ptr, CCP_LAYER_DOWN);
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
} /* CCP_Check_Terminate_Ack */

/*************************************************************************
* FUNCTION
*
*     CCP_Check_Reset_Req
*
* DESCRIPTION
*
*     Handle an incoming Reset Request packet according to the PPP
*     state machine.
*
* INPUTS
*
*     *buf_ptr                          Pointer to NET_BUFFER
*
* OUTPUTS
*
*     None
*
*************************************************************************/
VOID CCP_Check_Reset_Req(NET_BUFFER *buf_ptr)
{
    DV_DEVICE_ENTRY *dev_ptr;
    LINK_LAYER      *link_layer;
    CCP_LAYER       *ccp;

    /* Get some pointers to our data. */
    dev_ptr = buf_ptr->mem_buf_device;
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
    ccp = &link_layer->ccp;

    PrintInfo("   CCP_Check_Reset_Req\n");

    /* Set the Reset flag to reinitialize the tables 
     * before sending the next packet */
    ccp->options.local.mppe.mppe_reset_requested = NU_TRUE;

} /* CCP_Check_Reset_Req */

/*************************************************************************
* FUNCTION
*
*     CCP_Process_Request
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
*     *in_buf_ptr                       Pointer to NET_BUFFER
*     **ack_buf                         Pointer to NET_BUFFER
*
* OUTPUTS
*
*     STATUS
*
*************************************************************************/
STATUS CCP_Process_Request(NET_BUFFER *in_buf_ptr, NET_BUFFER **ack_buf)
{
    STATUS          status = PPP_FLAG_ACK;
    LINK_LAYER      *link_layer;
    CCP_FRAME       *ccp_frame, *ack_frame;
    CCP_OPTION      *option;
    DV_DEVICE_ENTRY *dev_ptr;
    UINT32          len = 0, enc_flags = 0, supported_bits = 0;
    UINT8           temp_ptr[6];
    CCP_LAYER       *ccp;

    /* Get some pointers to our data. */
    dev_ptr = in_buf_ptr->mem_buf_device;
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
    ccp = &link_layer->ccp;
    ccp_frame = (CCP_FRAME*)in_buf_ptr->data_ptr;

    PrintInfo("   CCP_Process_Request\n");

    /* Allocate a new CCP response packet for the ACK/NAK/reject that
       results from this request. */
    *ack_buf = CCP_New_Buffer(dev_ptr, CCP_CONFIGURE_ACK, ccp_frame->id);

    if (*ack_buf == NU_NULL)
        /* This error is logged within CCP_New_Buffer, so just return. */
        return PPP_FLAG_DROP;

    /* Set ack_frame to the buffer's data_ptr. */
    ack_frame = (CCP_FRAME*)((*ack_buf)->data_ptr);

    while ((len + CCP_HEADER_LEN) < ccp_frame->len)
    {
        /* Move to the next option. */
        option = (CCP_OPTION*)(&ccp_frame->data[(INT)len]);
        if (option->len == 0)
            break;

        /* Check out what the option is and see if we can handle it. */
        switch (option->code)
        {
        case CCP_OPTION_MPPC_MPPE:

            PrintOpt("      CCP Peer Req: Supported bits(%d)\n", 
                                    GET32(option->data, 0));

            /* Pull the supported bits from the packet. */
            ccp->options.remote.mppe.mppe_ccp_supported_bits = 
                                    GET32(option->data, 0);

            /* Check whether all other bits are zero 
             * except the meaningful bits. 
             * MS Windows sometimes set the obsolete bit in CCP packet. To 
             * ignore this, L bit is shifted right once. */
            if (((ccp->options.remote.mppe.mppe_ccp_supported_bits & 
                 (~(CCP_FLAG_L_BIT | CCP_FLAG_M_BIT | CCP_FLAG_H_BIT | 
                  CCP_FLAG_S_BIT | CCP_FLAG_MPPC | CCP_FLAG_O_BIT))) == 0)
               )
            {
                /* Count the number of encryption type bits. */
                supported_bits += ((ccp->options.remote.mppe.mppe_ccp_supported_bits
                                    & CCP_FLAG_L_BIT) != 0);
                supported_bits += ((ccp->options.remote.mppe.mppe_ccp_supported_bits
                                    & CCP_FLAG_M_BIT) != 0);
                supported_bits += ((ccp->options.remote.mppe.mppe_ccp_supported_bits
                                    & CCP_FLAG_S_BIT) != 0);

                /* Setting the flag for the strongest encryption 
                 * supported */
                if ((ccp->options.remote.mppe.mppe_ccp_supported_bits 
                    & CCP_FLAG_S_BIT) &&
                    (ccp->options.local.mppe.mppe_ccp_supported_bits 
                    & CCP_FLAG_S_BIT))
                {
                    enc_flags = CCP_FLAG_S_BIT;
                }

                else if((ccp->options.remote.mppe.mppe_ccp_supported_bits 
                    & CCP_FLAG_M_BIT) &&
                    (ccp->options.local.mppe.mppe_ccp_supported_bits 
                    & CCP_FLAG_M_BIT))
                {
                    enc_flags = CCP_FLAG_M_BIT;
                }

                else if((ccp->options.remote.mppe.mppe_ccp_supported_bits 
                    & CCP_FLAG_L_BIT) &&
                    (ccp->options.local.mppe.mppe_ccp_supported_bits 
                    & CCP_FLAG_L_BIT))
                {
                    enc_flags = CCP_FLAG_L_BIT;
                }

                /* Setting stateless flag if supported */
                if ((ccp->options.remote.mppe.mppe_ccp_supported_bits 
                    & CCP_FLAG_H_BIT) &&
                    (ccp->options.local.mppe.mppe_ccp_supported_bits 
                    & CCP_FLAG_H_BIT))
                {
                    enc_flags |= CCP_FLAG_H_BIT;
                }

                /* If supported_bits is greater than 1 we need to 
                 * NAK the packet. */
                if (supported_bits > 1)
                {
                    status = PPP_FLAG_NAK;
                    ack_frame->code = CCP_CONFIGURE_NAK;
                }

                /* else if optional encryption is specified then also 
                 * we need to NAK the packet with same options again. */
                else if ((ccp->options.local.mppe.mppe_default_flags & 
                         (CCP_FLAG_L_BIT | CCP_FLAG_M_BIT | CCP_FLAG_S_BIT))
                         && supported_bits == 0)
                {
                    status = PPP_FLAG_NAK;
                    ack_frame->code = CCP_CONFIGURE_NAK;
                    enc_flags = ccp->options.local.mppe.mppe_default_flags;
                    ccp->options.local.mppe.mppe_ccp_supported_bits =
                        ccp->options.local.mppe.mppe_default_flags;
                }

                /* else if intersection of supported bits results 
                 * in a zero i.e. no agreed upon encryption strength. */
                else if ((ccp->options.local.mppe.mppe_default_flags &
                    (CCP_FLAG_L_BIT | CCP_FLAG_M_BIT | CCP_FLAG_S_BIT)) &&
                    ((enc_flags & (CCP_FLAG_L_BIT | CCP_FLAG_M_BIT 
                    | CCP_FLAG_S_BIT)) == 0))
                {
                    status = PPP_FLAG_NAK;
                    ack_frame->code = CCP_CONFIGURE_NAK;
                }
                /* The check below is meaningless if both encryption
                 * modes can be used. If not then set the H bit 
                 * accordingly. */
#if (CCP_ENCRYPTION_MODE != CCP_USE_BOTH_ENCRYPTIONS)
                else if (
#if (CCP_ENCRYPTION_MODE == CCP_USE_STATELESS_ONLY)
                    ((ccp->options.remote.mppe.mppe_ccp_supported_bits
                    & CCP_FLAG_H_BIT) == 0)
#elif (CCP_ENCRYPTION_MODE == CCP_USE_STATEFUL_ONLY)
                    (ccp->options.remote.mppe.mppe_ccp_supported_bits
                    & CCP_FLAG_H_BIT)
#endif
                    )
                {
                    status = PPP_FLAG_NAK;
                    ack_frame->code = CCP_CONFIGURE_NAK;
#if (CCP_ENCRYPTION_MODE == CCP_USE_STATELESS_ONLY)
                    enc_flags |= CCP_FLAG_H_BIT;
#elif (CCP_ENCRYPTION_MODE == CCP_USE_STATEFUL_ONLY)
                    enc_flags &= ~CCP_FLAG_H_BIT;
#endif  
                }
#endif
                /* We need to ACK the packet with the same option*/
                else
                {
                    status = PPP_FLAG_ACK;
                    if (supported_bits == 1)
                    {
                        ccp->options.local.mppe.mppe_encrypt_packets = NU_TRUE;
                    }
                }
                
                PUT32(temp_ptr, 0, enc_flags);               

                /* Append the options in the CCP frame to be sent */
                CCP_Append_Option((CCP_FRAME *)ack_frame, CCP_MICROSOFT_SPECIFIC, 
                              (CCP_MPPC_MPPE_LENGTH - CCP_OPTION_HDRLEN), temp_ptr);

            }
            else
            {
                /* Not negotiating this option. Reject it. */
                status = CCP_Reject_Option(ack_frame, option->code, 
                  (UINT8)(option->len - CCP_OPTION_HDRLEN), option->data);
            }
            break;

        default:
            PrintOpt("      CCP Peer Req: Unknown option (%.02x)\n", 
                            option->code);

            /* If we make it here then this is an option that we do not
               support, so send a reject. */
            status = CCP_Reject_Option(ack_frame, option->code, 
                                      (UINT8)(option->len - CCP_OPTION_HDRLEN),
                                      option->data);

            break;

        } /* switch */

        /* Setup for next option. */
        len += option->len;
    }

    /* If we need to send a response, then *ack_buf will contain the buffer
       to send. */
    if (status == PPP_FLAG_REJECT)
    {
        /* REJECT packet is already built and ready to send. */
        PrintInfo("      CCP Send Reject.\n");
    }

    else if (status == PPP_FLAG_NAK)
    {
        /* NAK packet is already built and ready to send. */
        PrintInfo("      CCP Send NAK.\n");
    }

    else
    {
        /* ACK packet is already built and ready to send. */
        PrintInfo("      CCP Send ACK.\n");
    }

    return status;

}/* CCP_Process_Request */

/*************************************************************************
* FUNCTION
*
*     CCP_Process_Nak
*
* DESCRIPTION
*
*     Check an incoming NAK or REJECT packet to see if we can handle
*     the NAKed or REJECTed options. If an option is NAKed, we will
*     adjust the settings in the mppe structure so the next request
*     will be correct. If an option is NAKed, we will either disable
*     the option if it is not needed, or abort negotiations if it is.
*
* INPUTS
*
*     nak_type                          Type of NAK packet
*     *in_buf_ptr                       Pointer to NET_BUFFER
*
* OUTPUTS
*
*     PPP_FLAG_OK                       Everything was OK
*     PPP_FLAG_DROP                     Drop the packet
*
*************************************************************************/
STATUS CCP_Process_Nak(UINT8 nak_type, NET_BUFFER *in_buf_ptr)
{
    DV_DEVICE_ENTRY *dev_ptr;
    LINK_LAYER      *link_layer;
    CCP_FRAME       *ccp_frame;
    CCP_OPTION      *option;
    CCP_LAYER       *ccp;
    STATUS          status = PPP_FLAG_OK;
    UINT16          len = 0;

    /* Get a pointer to the IPCP structure for this device. */
    dev_ptr = (DV_DEVICE_ENTRY *)in_buf_ptr->mem_buf_device;
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
    ccp = &link_layer->ccp;
    ccp_frame = (CCP_FRAME*)in_buf_ptr->data_ptr;

#if (PPP_DEBUG_PRINT_OK == NU_TRUE)
    if (nak_type == PPP_FLAG_NAK)
        PrintInfo("   CCP_Process_Nak\n");
    else
        PrintInfo("   CCP_Process_Reject\n");
#endif

    /* Make sure this is a response to our request. */
    if (ccp_frame->id == ccp->identifier)
    {
        /* Process each option in the NAK/reject appropriately. */
        while ((len + CCP_HEADER_LEN) < ccp_frame->len)
        {
            option = (CCP_OPTION*)(&ccp_frame->data[len]);
            if (option->len == 0)
                break;

            /* Move to the next option. */
            /* Pull the supported bits from the packet. */
            ccp->options.remote.mppe.mppe_ccp_supported_bits = 
                GET32(option->data, 0);

            /* Check out what the option is and see if we can handle it. */
            switch (option->code)
            {
            case CCP_OPTION_MPPC_MPPE:

                /* If the packet is a reject packet. */
                if (nak_type == PPP_FLAG_REJECT)
                {
                    PrintOpt("      CCP Peer REJECT: (%.04x)\n", 0);
                    if((ccp->options.remote.mppe.mppe_ccp_supported_bits 
                        & CCP_FLAG_L_BIT))
                    {
                        ccp->options.local.mppe.mppe_ccp_supported_bits 
                            &= ~(CCP_FLAG_L_BIT);
                    }

                    if((ccp->options.remote.mppe.mppe_ccp_supported_bits 
                        & CCP_FLAG_M_BIT))
                    {
                        ccp->options.local.mppe.mppe_ccp_supported_bits 
                            &= ~(CCP_FLAG_M_BIT);
                    }

                    if((ccp->options.remote.mppe.mppe_ccp_supported_bits 
                        & CCP_FLAG_S_BIT))
                    {
                        ccp->options.local.mppe.mppe_ccp_supported_bits 
                            &= ~(CCP_FLAG_S_BIT);
                    }

#if (CCP_ENCRYPTION_MODE == CCP_USE_BOTH_ENCRYPTIONS)
                    /* If we are allowed to use both encryptions then 
                     * adjust the H bit. */
                    if((ccp->options.remote.mppe.mppe_ccp_supported_bits 
                        & CCP_FLAG_H_BIT))
                    {
                        ccp->options.local.mppe.mppe_ccp_supported_bits 
                            &= ~(CCP_FLAG_H_BIT);
                    }
#else
#if (CCP_ENCRYPTION_MODE == CCP_USE_STATELESS_ONLY)
                    /* If we are allowed to use stateless only, then disconnect. */
                    if((ccp->options.remote.mppe.mppe_ccp_supported_bits 
                        & CCP_FLAG_H_BIT ))
#elif (CCP_ENCRYPTION_MODE == CCP_USE_STATEFUL_ONLY)
                    /* If we are allowed to use stateful only, then disconnect. */
                    if((ccp->options.remote.mppe.mppe_ccp_supported_bits 
                        & CCP_FLAG_H_BIT ) == 0)
#endif
                    {
                        /* The option is rejected. We need to close the link.
                         * Terminate the connection. */
                        link_layer->lcp.state = CLOSING;
                        ccp->state = CLOSING;

                        /* Send a terminate request for LCP. */
                        LCP_Send_Terminate_Req(dev_ptr, 
                                               PPP_LINK_CONTROL_PROTOCOL);

                        /* Set up a timeout event for LCP close. */
                        TQ_Timerset(PPP_Event, (UNSIGNED)dev_ptr, 
                                    LCP_TIMEOUT_VALUE,
                                    LCP_SEND_TERMINATE);

                        /* Drop the packets. */
                        status = PPP_FLAG_DROP;
                    }
#endif
                }
                else
                {
                    /* Depending in the NAK packet, adjust local bits to
                     * resend the agreed upon bits */
                    if(!(ccp->options.remote.mppe.mppe_ccp_supported_bits 
                        & CCP_FLAG_L_BIT))
                    {
                        ccp->options.local.mppe.mppe_ccp_supported_bits 
                            &= ~(CCP_FLAG_L_BIT);
                    }

                    if(!(ccp->options.remote.mppe.mppe_ccp_supported_bits 
                        & CCP_FLAG_M_BIT))
                    {
                        ccp->options.local.mppe.mppe_ccp_supported_bits 
                            &= ~(CCP_FLAG_M_BIT);
                    }

                    if(!(ccp->options.remote.mppe.mppe_ccp_supported_bits 
                        & CCP_FLAG_S_BIT))
                    {
                        ccp->options.local.mppe.mppe_ccp_supported_bits 
                            &= ~(CCP_FLAG_S_BIT);
                    }

#if (CCP_ENCRYPTION_MODE == CCP_USE_BOTH_ENCRYPTIONS)
                    /* If we are allowed to use both encryptions then 
                     * adjust the H bit. */
                    if(!(ccp->options.remote.mppe.mppe_ccp_supported_bits 
                        & CCP_FLAG_H_BIT))
                    {
                        ccp->options.local.mppe.mppe_ccp_supported_bits 
                            &= ~(CCP_FLAG_H_BIT);
                    }
#else
#if (CCP_ENCRYPTION_MODE == CCP_USE_STATELESS_ONLY)
                    /* If we are allowed to use stateless only, then disconnect. */
                    if((ccp->options.remote.mppe.mppe_ccp_supported_bits 
                        & CCP_FLAG_H_BIT ) == 0)
#elif (CCP_ENCRYPTION_MODE == CCP_USE_STATEFUL_ONLY)
                    /* If we are allowed to use stateful only, then disconnect. */
                    if((ccp->options.remote.mppe.mppe_ccp_supported_bits 
                        & CCP_FLAG_H_BIT ))
#endif
                    {
                        /* The option is rejected. We need to close the link  */
                        /* Terminate the connection */
                        link_layer->lcp.state = CLOSING;
                        ccp->state = CLOSING;

                        /* Send a terminate request for LCP. */
                        LCP_Send_Terminate_Req(dev_ptr, PPP_LINK_CONTROL_PROTOCOL);

                        /* Set up a timeout event for LCP close. */
                        TQ_Timerset(PPP_Event, (UNSIGNED)dev_ptr, 
                                    LCP_TIMEOUT_VALUE, 
                                    LCP_SEND_TERMINATE);

                        /* Drop the packets. */
                        status = PPP_FLAG_DROP;
                    }
#endif

                }

                break;

            default:

                /* If we make it here then this is an option that we did not
                   request or was incorrectly NAKed/rejected. */
                break;

            } /* switch */

            /* Setup for next option. */
            len = (UINT16)(len + option->len);
        }
    }
    else
        status = PPP_FLAG_DROP;

    return status;
} /* CCP_Process_Nak */

/*************************************************************************
*
*   FUNCTION
*
*       CCP_Send_Code_Reject
*
*   DESCRIPTION
*
*       Send a code reject packet to our peer. This is in response to
*       having received an unknown code for a known protocol type.
*
*   INPUTS
*
*       *in_buf_ptr                     Pointer to the unknown protocol
*                                       packet.
*       protocol                        Protocol type.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID CCP_Send_Code_Reject(NET_BUFFER *in_buf_ptr, UINT16 protocol)
{
    DV_DEVICE_ENTRY *dev_ptr;
    NET_BUFFER      *buf_ptr;
    CCP_FRAME       *ccp_frame;

    /* Get some pointers to our data. */
    dev_ptr = in_buf_ptr->mem_buf_device;

    /* Create an outgoing packet. Most of the fields will
       be pre-filled. */
    buf_ptr = CCP_New_Buffer(dev_ptr, CCP_CODE_REJECT, -1);
    if (buf_ptr == NU_NULL)
        /* This error is logged within CCP_New_Buffer, so just return. */
        return;

    ccp_frame = (CCP_FRAME*)buf_ptr->data_ptr;

    /* Copy the rejected packet to the new buffer. */
    NU_BLOCK_COPY(ccp_frame->data, in_buf_ptr->data_ptr,
           (INT)in_buf_ptr->data_len);
    ccp_frame->len = (UINT16)(ccp_frame->len + in_buf_ptr->data_len);

    /* Send it. */
    CCP_Send(dev_ptr, buf_ptr, protocol);

} /* CCP_Send_Code_Reject */

/*************************************************************************
* FUNCTION
*
*       CCP_Process_Code_Reject
*
* DESCRIPTION
*
*       This function checks a code reject packet. If it is rejecting any
*       codes that PPP needs to work then the link is terminated.
*       Note that most codes specified in RFC1661 are required. Any codes
*       that are not required by this implementation are never transmitted
*       and thus will never be rejected.
*
* INPUTS
*
*       *buf_ptr                        Pointer to NET_BUFFER
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID CCP_Process_Code_Reject(NET_BUFFER *buf_ptr)
{
    DV_DEVICE_ENTRY *dev_ptr = buf_ptr->mem_buf_device;
    LINK_LAYER      *link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

    PrintInfo("   CCP_Process_Code_Reject\n");

    /* We currently don't send any optional codes, so terminate the link. */
    EQ_Put_Event(PPP_CCP_Event, (UNSIGNED)dev_ptr, CCP_CLOSE_REQUEST);
    NU_Set_Events(&link_layer->negotiation_progression, PPP_CCP_FAIL, NU_OR);

} /* CCP_Process_Code_Reject */

/*************************************************************************
*
*   FUNCTION
*
*       CCP_Send_Terminate_Req
*
*   DESCRIPTION
*
*       Sends a terminate request packet to the peer.
*
*   INPUTS
*
*       *dev_ptr                        Pointer to the device structure.
*       protocol                        Which protocol is sending the ACK.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID CCP_Send_Terminate_Req(DV_DEVICE_ENTRY *dev_ptr, UINT16 protocol)
{
    NET_BUFFER  *buf_ptr;
    CCP_FRAME   *ccp_frame;
    CCP_LAYER       *ccp;

    /* Get some pointers to our data. */
    ccp = &((LINK_LAYER*)dev_ptr->dev_link_layer)->ccp;

    PrintInfo("   CCP_Send_Terminate_Req\n");

    /* Create an outgoing request packet. Most of the fields will
       be pre-filled. */
    buf_ptr = CCP_New_Buffer(dev_ptr, CCP_TERMINATE_REQUEST, -1);
    if (buf_ptr == NU_NULL)
        /* This error is logged within CCP_New_Buffer, so just return. */
        return;

    /* Get a pointer to the data part of the CCP packet. */
    ccp_frame = (CCP_FRAME*)buf_ptr->data_ptr;

    /* Save the identifier that was assigned to this packet. */
    ccp->identifier = ccp_frame->id;

    /* Nothing else goes into the packet, so just send it. */
    CCP_Send(dev_ptr, buf_ptr, protocol);

} /* CCP_Send_Terminate_Req */

/*************************************************************************
*
*   FUNCTION
*
*       CCP_Send_Terminate_ACK
*
*   DESCRIPTION
*
*       Sends a terminate ACK packet to the peer.
*
*   INPUTS
*
*       *dev_ptr                        Pointer to the device structure.
*       protocol                        Which protocol is sending the ACK.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID CCP_Send_Terminate_Ack(DV_DEVICE_ENTRY *dev_ptr, UINT16 protocol)
{
    LINK_LAYER          *link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
    NET_BUFFER          *buf_ptr;

    PrintInfo("   CCP_Send_Terminate_Ack\n");

    /* Create an outgoing request packet. Most of the fields will
       be pre-filled. */
    buf_ptr = CCP_New_Buffer(dev_ptr, CCP_TERMINATE_ACK, link_layer->term_id);
    if (buf_ptr == NU_NULL)
        /* This error is logged within CCP_New_Buffer, so just return. */
        return;

    /* Nothing else goes into the packet, so just send it. */
    CCP_Send(dev_ptr, buf_ptr, protocol);

    /* Reset the terminate id to -1 for the next ack. */
    link_layer->term_id = -1;

} /* CCP_Send_Terminate_Ack */

/*************************************************************************
*
*   FUNCTION
*
*       CCP_Send_Reset_Req
*
*   DESCRIPTION
*
*       Sends a terminate request packet to the peer.
*
*   INPUTS
*
*       *dev_ptr                        Pointer to the device structure.
*       protocol                        Which protocol is sending the ACK.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID CCP_Send_Reset_Req(DV_DEVICE_ENTRY *dev_ptr, UINT16 protocol)
{
    NET_BUFFER  *buf_ptr;
    CCP_FRAME   *ccp_frame;
    CCP_LAYER   *ccp;

    /* Get some pointers to our data. */
    ccp = &((LINK_LAYER*)dev_ptr->dev_link_layer)->ccp;

    PrintInfo("   CCP_Send_Reset_Req\n");

    /* Create an outgoing request packet. Most of the fields will
    be pre-filled. */
    buf_ptr = CCP_New_Buffer(dev_ptr, CCP_RESET_REQUEST, -1);
    if (buf_ptr == NU_NULL)
        /* This error is logged within CCP_New_Buffer, so just return. */
        return;

    /* Get a pointer to the data part of the CCP packet. */
    ccp_frame = (CCP_FRAME*)buf_ptr->data_ptr;

    /* Save the identifier that was assigned to this packet. */
    ccp->identifier = ccp_frame->id;

    /* Nothing else goes into the packet, so just send it. */
    CCP_Send(dev_ptr, buf_ptr, protocol);

} /* CCP_Send_Reset_Req */

/*************************************************************************
*
*   FUNCTION
*
*       CCP_Send_Reset_Ack
*
*   DESCRIPTION
*
*       Sends a reset ACK packet to the peer.
*
*   INPUTS
*
*       *dev_ptr                        Pointer to the device structure.
*       protocol                        Which protocol is sending the ACK.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/  
VOID CCP_Send_Reset_Ack(DV_DEVICE_ENTRY *dev_ptr, UINT16 protocol)
{
    LINK_LAYER          *link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
    NET_BUFFER          *buf_ptr;

    PrintInfo("   CCP_Send_Reset_Ack\n");

    /* Create an outgoing request packet. Most of the fields will
    be pre-filled. */
    buf_ptr = CCP_New_Buffer(dev_ptr, CCP_RESET_ACK, link_layer->term_id);
    if (buf_ptr == NU_NULL)
        /* This error is logged within CCP_New_Buffer, so just return. */
        return;

    /* Nothing else goes into the packet, so just send it. */
    CCP_Send(dev_ptr, buf_ptr, protocol);

    /* Reset the terminate id to -1 for the next ack. */
    link_layer->term_id = -1;

} /* CCP_Send_Reset_Ack */

/*************************************************************************
* FUNCTION
*
*     CCP_Event_Handler
*
* DESCRIPTION
*
*     Handles processing of CCP events.
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
VOID CCP_Event_Handler(TQ_EVENT evt, UNSIGNED dat, UNSIGNED subevt)
{
    DV_DEVICE_ENTRY         *dev_ptr = (DV_DEVICE_ENTRY*)dat;
    LINK_LAYER              *link_layer;
    CCP_LAYER               *ccp;
    LCP_LAYER               *lcp;
    AUTHENTICATION_LAYER    *auth;

    /* Get some pointers to our pointer. */
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
    ccp = &link_layer->ccp;
    lcp = &link_layer->lcp;
    auth = &link_layer->authentication;

    /* Switch on the event and do the appropriate processing. */
    switch (subevt)
    {
    case CCP_OPEN_REQUEST:

        /* Only negotiate this ncp if it is not already open. */
        if (ccp->state == OPENED)
            break;

        PrintInfo("Starting CCP negotiation...\n");

        /* Set the status of the PPP connection attempt. */
        link_layer->connection_status = NU_PPP_NETWORK_NEGOTIATION;

        /* Initialize the number of retransmit attempts. */
        ccp->num_transmissions = CCP_MAX_CONFIGURE;

    case CCP_SEND_CONFIG:

        /* We will not initiate CCP negotiation if Encryption 
         * is not mandatory. */
        if (ccp->options.local.mppe.mppe_require_encryption == 0)
        {
            break;
        }

        /* If LCP is not open, open it. */
        if (lcp->state != OPENED)
        {
            PrintErr("   CCP event entered before LCP is open...\n");
            break;
        }

        /* If not authenticated, do so now. */
        else if (auth->state != AUTHENTICATED)
        {
            PrintErr("   CCP event entered without authentication...\n");
            break;
        }

        /* If total number of retransmission are not exhausted. */
        else if (ccp->num_transmissions-- > 0)
        {
            PrintInfo("   Sending CCP config request...\n");

            /* Send request to the host */
            CCP_Send_Config_Req(dev_ptr);

            if (ccp->state == INITIAL)
            {
                /* Set the initial state of the CCP layer. */
                ccp->state = REQ_SENT;
            }
        }

        else
        {
            /* Set the Layer Down event. */
            ccp->state = STOPPED;
            EQ_Put_Event(evt, (UNSIGNED)dev_ptr, CCP_LAYER_DOWN); 
            if (evt == PPP_CCP_Event)
            {
                NU_Set_Events(&link_layer->negotiation_progression, PPP_CCP_FAIL, NU_OR);
            }
        }

        break;

    case CCP_LAYER_UP:

        /* We are about to use MPPE so initialize it. */
        MPPE_Init(dev_ptr);
        break;

    case CCP_CLOSE_REQUEST:

        PrintInfo("Closing CCP layer...\n");

        /* Set the status of the PPP connection attempt. */
        link_layer->connection_status = NU_PPP_NETWORK_NEGOTIATION;

        /* Initialize the number of retransmit attempts. */
        ccp->num_transmissions = CCP_MAX_TERMINATE;

    case CCP_SEND_TERMINATE:

        /* Only send terminate requests during a normal close. */
        if (lcp->state == OPENED)
        {
            if (ccp->state == CLOSING)
            {
                if (ccp->num_transmissions-- > 0)
                {
                    PrintInfo("   Sending CCP terminate request...\n");

                    /* Send a terminate request for CCP. */
                    CCP_Send_Terminate_Req(dev_ptr, PPP_CCP_PROTOCOL);

                    /* Set up a timeout event for CCP close. */
                    TQ_Timerset(evt, (UNSIGNED)dev_ptr, CCP_TIMEOUT_VALUE,
                        CCP_SEND_TERMINATE);
                    break;
                }
                else
                    ccp->state = CLOSED;
            }
            else
            {
                /* Send a terminate ACK. */
                CCP_Send_Terminate_Ack (dev_ptr, PPP_CCP_PROTOCOL);
                ccp->state = STOPPED;
            }
        }

    case CCP_LAYER_DOWN:

        PrintInfo("   CCP layer down.\n");

        if (lcp->state == OPENED)
            lcp->state = CLOSING;

        /* If the LCP layer is stopped then stop the CCP layer as well */
        if(lcp->state == STOPPED)
            ccp->state = STOPPED;

        if (lcp->state == CLOSING)
            /* Close LCP if CCP is not using it. */
            EQ_Put_Event(PPP_Event, (UNSIGNED)dev_ptr, LCP_CLOSE_REQUEST);
        else
        {
            /* Send a terminate ACK. */
            lcp->state = STOPPED;
            EQ_Put_Event(PPP_Event, (UNSIGNED)dev_ptr, 
                         LCP_SEND_TERMINATE);
        }

    default:
        UNUSED_PARAMETER(evt);
        break;

    } /* end switch */

    return;

} /* CCP_Event_Handler */

#endif
