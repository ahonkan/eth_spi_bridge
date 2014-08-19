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
*       lcp.c
*
*   COMPONENT
*
*       LCP - Link Control Protocol
*
*   DESCRIPTION
*
*       This file contains the link control protocol used by PPP to
*       establish and negotiate the configuration options that will
*       be over the link.
*
*   DATA STRUCTURES
*
*       StateStrings
*
*   FUNCTIONS
*
*       LCP_Init
*       LCP_Reset
*       LCP_Interpret
*       LCP_Send_Config_Req
*       LCP_Check_Config_Req
*       LCP_Check_Config_Ack
*       LCP_Check_Config_Nak
*       LCP_Check_Terminate_Req
*       LCP_Check_Terminate_Ack
*       LCP_Process_Request
*       LCP_Process_Nak
*       LCP_Send_Protocol_Reject
*       LCP_Send_Code_Reject
*       LCP_Process_Code_Reject
*       LCP_Process_Protocol_Reject
*       LCP_Random_Number
*       LCP_Random_Number32
*       LCP_Send_Echo_Req
*       LCP_Process_Echo_Req
*       LCP_Send_Terminate_Req
*       LCP_Send_Terminate_Ack
*       LCP_Nak_Option
*       LCP_Reject_Option
*       LCP_New_Buffer
*       LCP_Send
*       LCP_Append_Option
*
*   DEPENDENCIES
*
*       nu_ppp.h
*
*************************************************************************/
#include "drivers/nu_ppp.h"

#if LCP_DEBUG_PRINT_OK || NCP_DEBUG_PRINT_OK || CCP_DEBUG_PRINT_OK
CHAR StateStrings[10][10] = {PPP_DEBUG_STATES};
#endif

#if LCP_DEBUG_PRINT_OK
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
*     LCP_Init
*
* DESCRIPTION
*
*     Initializes the LCP layer of PPP. In general, it sets up
*     the default values that will be used each time the link
*     is reset. If SNMP is used, these defaults can be changed
*     from a remote manager.
*
* INPUTS
*
*     *dev_ptr                  Pointer to the device that this
*                               LCP layer belongs to.
*
* OUTPUTS
*
*     STATUS
*
*************************************************************************/
STATUS LCP_Init(DV_DEVICE_ENTRY *dev_ptr)
{
    LINK_LAYER      *link_layer;

    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

#if (INCLUDE_SEC_MIB == NU_TRUE)

#if (PPP_USE_CHAP == NU_TRUE)
    if (PMSC_NewEntry(dev_ptr->dev_index, PPP_CHAP_PROTOCOL, 1) == NU_NULL)
    {
        /* Log the error. */
        NLOG_Error_Log("Failed to add entry to PMCS table.", NERR_FATAL, __FILE__, __LINE__);
    }
#endif

#if (PPP_USE_PAP == NU_TRUE)
    if (PMSC_NewEntry(dev_ptr->dev_index, PPP_PAP_PROTOCOL, 2) == NU_NULL)
    {
        /* Log the error. */
        NLOG_Error_Log("Failed to add entry to PMCS table.", NERR_FATAL, __FILE__, __LINE__);
    }
#endif

#endif

    /* These are the options that we can allow the peer to negotiate, while
       not necessarily negotiating them locally. */
    link_layer->lcp.options.remote.default_flags = (PPP_FLAG_MRU | PPP_FLAG_ACCM
                                | PPP_FLAG_MAGIC | PPP_FLAG_PFC | PPP_FLAG_ACC);

    /* Initialize the LCP negotiation flags based on initial configuration. */
    link_layer->lcp.options.local.default_flags = 0;

#if (PPP_USE_MRU == NU_TRUE)
        link_layer->lcp.options.local.default_flags |= PPP_FLAG_MRU;
#endif

#if (PPP_USE_ACCM == NU_TRUE)
        link_layer->lcp.options.local.default_flags |= PPP_FLAG_ACCM;
#endif

#if (PPP_USE_MAGIC == NU_TRUE)
        link_layer->lcp.options.local.default_flags |= PPP_FLAG_MAGIC;
#endif

#if (PPP_USE_PFC == NU_TRUE)
        link_layer->lcp.options.local.default_flags |= PPP_FLAG_PFC;
#endif

#if (PPP_USE_ACC == NU_TRUE)
        link_layer->lcp.options.local.default_flags |= PPP_FLAG_ACC;
#endif

    /* If we are a server, then the PAP and CHAP flags identify the
       options we intend to request from the client. If we are a client,
       then these flags identify what we are capable of satisfying
       for a server. */
#if (PPP_USE_CHAP == NU_TRUE)
        link_layer->lcp.options.local.default_flags  |= PPP_FLAG_CHAP;
        link_layer->lcp.options.remote.default_flags |= PPP_FLAG_CHAP;

#if (PPP_USE_CHAP_MS1 == NU_TRUE)
        link_layer->lcp.options.local.default_flags  |= PPP_FLAG_CHAP_MS1;
        link_layer->lcp.options.remote.default_flags |= PPP_FLAG_CHAP_MS1;
#endif

#if (PPP_USE_CHAP_MS2 == NU_TRUE)
        link_layer->lcp.options.local.default_flags  |= PPP_FLAG_CHAP_MS2;
        link_layer->lcp.options.remote.default_flags |= PPP_FLAG_CHAP_MS2;
#endif

#endif /* PPP_USE_CHAP */


#if (PPP_USE_PAP == NU_TRUE)
        link_layer->lcp.options.local.default_flags |= PPP_FLAG_PAP;
        link_layer->lcp.options.remote.default_flags |= PPP_FLAG_PAP;
#endif

/* If PPP Multilink Protocol is enabled */
#if (INCLUDE_PPP_MP == NU_TRUE)
    /* These options are allowed to be negotiated on our side */
    link_layer->lcp.options.remote.default_flags |= PPP_FLAG_ENDPOINT_DISC;

#if (PPP_MP_DEFAULT_USE_SHORT_SEQ_NUM == NU_TRUE)
    /* set the use of short sequence number header format in MP */
    link_layer->lcp.options.remote.default_flags |= PPP_FLAG_SHORT_SEQ_NUM;
#endif

#endif /* INCLUDE_PPP_MP */

    /* Set the initial defaults for this link. */
    link_layer->lcp.options.local.default_mru           = (UINT16)dev_ptr->dev_mtu;
    link_layer->lcp.options.local.default_accm          = HDLC_LOCAL_DEFAULT_ACCM;
    link_layer->lcp.options.local.default_fcs_size      = 16;

    link_layer->lcp.options.remote.default_mru           = (UINT16)dev_ptr->dev_mtu;
    link_layer->lcp.options.remote.default_accm          = HDLC_FOREIGN_DEFAULT_ACCM;
    link_layer->lcp.options.remote.default_fcs_size      = 16;

    return NU_SUCCESS;
}



/*************************************************************************
*
*   FUNCTION
*
*       LCP_Reset
*
*   DESCRIPTION
*
*       Resets the LCP configuration options to their default values.
*       This is done in preparation for a new PPP session.
*
*   INPUTS
*
*       LINK_LAYER  *link_layer     Pointer to the LCP structure to
*                                   be reset.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID LCP_Reset(LINK_LAYER *link_layer)
{
    LCP_LAYER *lcp = &link_layer->lcp;
#if (INCLUDE_SEC_MIB == NU_TRUE)
    STATUS status;
    UINT16 protocol;
#endif

    /* Reset the options we will request during negotiation. */
    lcp->options.local.flags = lcp->options.local.default_flags;

    /* Reset the remote options to zero. They will be filled in as we
       acknowledge a peer's request. */
    lcp->options.remote.flags = 0;

    /* Reset the default TX LCP options into the structure that holds them.
       We don't fill them all in here because our peer will let us know
       which ones it can handle. */
    lcp->options.remote.accm = lcp->options.remote.default_accm;
    lcp->options.remote.mru = lcp->options.local.default_mru;
    lcp->options.remote.fcs_size = lcp->options.remote.default_fcs_size;

    /* Reset the local default LCP options. */
    lcp->options.local.accm = lcp->options.local.default_accm;
    lcp->options.local.mru = lcp->options.local.default_mru;
    lcp->options.local.fcs_size = lcp->options.local.default_fcs_size;

    /* Reset the authentication protocols for the link. */
    lcp->options.local.flags &= ~PPP_AUTH_MASK;
    link_layer->authentication.state = 0;

    if (link_layer->mode == PPP_SERVER)
    {
#if (INCLUDE_SEC_MIB == NU_TRUE)
        /* Clear our default protocol. */
        lcp->options.local.default_flags &= ~PPP_AUTH_MASK;

        /* Get the default protocol from the PPP MIB. */
        status = PMSC_GetDefaultProtocol(link_layer->hwi.dev_ptr->dev_index, &protocol);
        if (status == NU_SUCCESS)
        {
            if (protocol == PPP_CHAP_PROTOCOL)
                lcp->options.local.default_flags |= PPP_FLAG_CHAP;
            else if (protocol == PPP_PAP_PROTOCOL)
                lcp->options.local.default_flags |= PPP_FLAG_PAP;
        }

#else

#if (PPP_USE_CHAP == NU_TRUE)
        lcp->options.local.default_flags &= ~PPP_FLAG_PAP;
    #if (PPP_USE_CHAP_MS2 == NU_TRUE)
        lcp->options.local.default_flags &= ~PPP_FLAG_CHAP_MS1;
    #elif(PPP_USE_CHAP_MS1 == NU_TRUE)
        lcp->options.local.default_flags &= ~PPP_FLAG_CHAP_MS2;
    #endif

#elif (PPP_USE_PAP == NU_TRUE)
        lcp->options.local.default_flags &= ~PPP_FLAG_CHAP;
#endif /* PPP_USE_CHAP */

#endif


        /* Set local flags with the authentication protocols we intend
           to request from the client. */
#if (PPP_USE_CHAP == NU_TRUE)
        lcp->options.local.flags |= PPP_FLAG_CHAP;

    #if (PPP_USE_CHAP_MS1 == NU_TRUE)
        lcp->options.local.flags |= PPP_FLAG_CHAP_MS1;
    #endif

    #if (PPP_USE_CHAP_MS2 == NU_TRUE)
        lcp->options.local.flags |= PPP_FLAG_CHAP_MS2;
    #endif

#endif /* PPP_USE_CHAP */

#if (PPP_USE_PAP == NU_TRUE)
        lcp->options.local.flags |= PPP_FLAG_PAP;
#endif

        if (lcp->options.local.flags & PPP_AUTH_MASK)
                link_layer->authentication.state = AUTH_REQUIRED;

/* If PPP Multilink Protocol is enabled */
#if (INCLUDE_PPP_MP == NU_TRUE)

        /* Set the flags for using Multilink Protocol on our side */
        lcp->options.local.flags |= PPP_FLAG_MRRU;
        lcp->options.local.flags |= PPP_FLAG_ENDPOINT_DISC;

        lcp->options.remote.default_flags |= PPP_FLAG_MRRU;

        /* Set the default MRRU to be negotiated. */
        lcp->options.local.mp_mrru = PPP_MP_DEFAULT_MRRU_SIZE;

#if (PPP_MP_DEFAULT_USE_SHORT_SEQ_NUM == NU_TRUE)
        /* set the use of short sequence number header format in MP */
        lcp->options.local.flags |= PPP_FLAG_SHORT_SEQ_NUM;
#endif

#endif /* INCLUDE_PPP_MP */

    } /* link_layer->mode == PPP_SERVER */

    /* Set the initial state of the LCP layer. */
    lcp->state = REQ_SENT;
    lcp->echo_counter = 0;

} /* LCP_Reset */



/*************************************************************************
* FUNCTION
*
*     LCP_Interpret
*
* DESCRIPTION
*
*     Passes the incoming packet to its associated handler. After
*     the packet has been processed it is deallocated from the
*     buffer list.
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
VOID LCP_Interpret(NET_BUFFER *buf_ptr)
{
    DV_DEVICE_ENTRY *dev_ptr = buf_ptr->mem_buf_device;
    LINK_LAYER      *link_layer;
    LCP_FRAME       *lcp_frame;
    LCP_LAYER       *lcp;

    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
    lcp = &link_layer->lcp;

    PrintInfo("LCP_Interpret\n");

    /* Swap the 2-byte packet length. */
    lcp_frame = (LCP_FRAME*)buf_ptr->data_ptr;
    lcp_frame->len = INTSWAP(lcp_frame->len);

    /* Switch on the LCP type. */
    switch (lcp_frame->code)
    {
    case LCP_CONFIGURE_REQUEST:
        LCP_Check_Config_Req(buf_ptr);
        break;

    case LCP_CONFIGURE_ACK:
        LCP_Check_Config_Ack(buf_ptr);
        break;

    case LCP_CONFIGURE_NAK:
        LCP_Check_Config_Nak(PPP_FLAG_NAK, buf_ptr);
        break;

    case LCP_CONFIGURE_REJECT:
        LCP_Check_Config_Nak(PPP_FLAG_REJECT, buf_ptr);
        break;

    case LCP_TERMINATE_REQUEST:
        link_layer->term_id = lcp_frame->id;
        LCP_Check_Terminate_Req(buf_ptr);
        break;

    case LCP_TERMINATE_ACK:
        LCP_Check_Terminate_Ack(buf_ptr);
        break;

    case LCP_CODE_REJECT:
        LCP_Process_Code_Reject(buf_ptr);
        break;

    case LCP_PROTOCOL_REJECT:
        LCP_Process_Protocol_Reject(buf_ptr);
        break;

    case LCP_ECHO_REQUEST:
        LCP_Process_Echo_Req(buf_ptr);
        break;

    case LCP_ECHO_REPLY:

        /* Make sure this is a response to our request by comparing the
        frame id and the remote magic number. */
        if ((lcp_frame->id == lcp->identifier) &&
            (((GET32(lcp_frame->data, 0)) == lcp->options.remote.magic_number)))
        {
            /* Reset the echo counter. */
            lcp->echo_counter = 0;
        }
        break;

    case LCP_DISCARD_REQUEST:

        PrintInfo("    Discard request\n");

        /* Just silently discard this packet. */
        PML_SilentDiscards_Inc(dev_ptr);

        break;

    default:

        PrintInfo("    Unknown code!\n");

        /* Reject any unknown LCP codes. */
        LCP_Send_Code_Reject(buf_ptr, PPP_LINK_CONTROL_PROTOCOL);

        break;
    }

    /* Release the buffer space */
    MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);

} /* LCP_Interpret */



/*************************************************************************
* FUNCTION
*
*     LCP_Send_Config_Req
*
* DESCRIPTION
*
*     Sends an LCP configure request packet to the peer with the
*     options we want to negotiate.
*
* INPUTS
*
*     *dev_ptr
*
* OUTPUTS
*
*     None
*
*************************************************************************/
VOID LCP_Send_Config_Req(DV_DEVICE_ENTRY *dev_ptr)
{
    NET_BUFFER          *buf_ptr;
    LCP_FRAME   HUGE    *lcp_frame;
    LINK_LAYER          *link_layer;
    UINT8               temp_ptr[8];
    LCP_LAYER           *lcp;
    UINT32              auth;
    STATUS              status;

	/* Zero out memory to remove KW error. */
	memset(temp_ptr, 0, 8);
			
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
    lcp = &link_layer->lcp;

    /* If this function is not called as a result of a timeout, then there
       may be a lingering RESEND event in the TQ. Remove it now. */
    TQ_Timerunset(PPP_Event, TQ_CLEAR_EXACT, (UNSIGNED)dev_ptr, LCP_SEND_CONFIG);

    /* Check if this device needs to do proxy LCP at the LNS. */
    if(link_layer->hwi.itype & PPP_ITYPE_L2TP_LNS)
    {
        status = PPP_L2TP_LNS_Do_Proxy(dev_ptr);

        if(status == NU_SUCCESS)
            return;
    }

    PrintInfo("   LCP_Send_Config_Req\n");

    /* Create an outgoing request packet. Most of the fields will
       be pre-filled. If we fail to allocate a buffer, we can still set
       the timer event to try again later. */
    buf_ptr = LCP_New_Buffer(dev_ptr, LCP_CONFIGURE_REQUEST, -1);
    if (buf_ptr != NU_NULL)
    {
        /* Get a pointer to the data part of the LCP packet. */
        lcp_frame = (LCP_FRAME*)buf_ptr->data_ptr;

        /* Save the identifier that was assigned to this packet. */
        lcp->identifier = lcp_frame->id;

        /* Add the Max Receive Unit option. */
        if (lcp->options.local.flags & PPP_FLAG_MRU)
        {
            PrintOpt("      LCP Local Req: Max RX Unit(%d)\n", lcp->options.local.mru);			
            PUT16(temp_ptr, 0, lcp->options.local.mru);
            LCP_Append_Option((LCP_FRAME *)lcp_frame, LCP_OPTION_MRU,
                              2, temp_ptr);
        }

        /* Add the ACCM option. */
        if (lcp->options.local.flags & PPP_FLAG_ACCM)
        {
            PrintOpt("      LCP Local Req: ACCM(%.08x)\n", lcp->options.local.accm);
            PUT32(temp_ptr, 0, lcp->options.local.accm);
            LCP_Append_Option((LCP_FRAME *)lcp_frame, LCP_OPTION_ACCM,
                              4, temp_ptr);
        }

        /* Add the authentication protocol. We will only send this option
           if we are in server mode and need to authenticate the client. */
        if (link_layer->mode == PPP_SERVER)
        {
            /* Do we need to request authentication? */
            if (link_layer->authentication.state == AUTH_REQUIRED)
            {
                /* Start with our default protocol. */
                auth = lcp->options.local.flags & lcp->options.local.default_flags & PPP_AUTH_MASK;
                if (auth == 0)
                    /* Default protocol has already been NAKed. Try another. */
                    auth = lcp->options.local.flags & PPP_AUTH_MASK;

                if (auth == PPP_FLAG_CHAP)
                {
                    PUT16(temp_ptr, 0, PPP_CHAP_PROTOCOL);

#if(PPP_USE_CHAP_MS1 == NU_TRUE)
                    if(lcp->options.local.flags & PPP_FLAG_CHAP_MS1 &
                        lcp->options.local.default_flags)
                    {
                        PrintOpt("      LCP Local Req: Requesting MSCHAPv%d\n", 1);
                        temp_ptr[2] = LCP_CHAP_MS1;
                    }
                    else
#endif
#if(PPP_USE_CHAP_MS2 == NU_TRUE)
                    if(lcp->options.local.flags & PPP_FLAG_CHAP_MS2 &
                        lcp->options.local.default_flags)
                    {
                        PrintOpt("      LCP Local Req: Requesting MSCHAPv%d\n", 2);
                        temp_ptr[2] = LCP_CHAP_MS2;
                    }
                    else
#endif
#if(PPP_USE_CHAP_MS1 == NU_TRUE)
                    if(lcp->options.local.flags & PPP_FLAG_CHAP_MS1)
                    {
                        PrintOpt("      LCP Local Req: Requesting MSCHAPv%d\n", 1);
                        temp_ptr[2] = LCP_CHAP_MS1;
                    }
                    else
#endif
#if(PPP_USE_CHAP_MS2 == NU_TRUE)
                    if(lcp->options.local.flags & PPP_FLAG_CHAP_MS2)
                    {
                        PrintOpt("      LCP Local Req: Requesting MSCHAPv%d\n", 2);
                        temp_ptr[2] = LCP_CHAP_MS2;
                    }
                    else
#endif
                    {
                        PrintOpt("      LCP Local Req: Requesting %s\n", "CHAP");
                        temp_ptr[2] = LCP_CHAP_MD5;
                    }

                    lcp->options.local.chap_protocol = temp_ptr[2];

                    LCP_Append_Option((LCP_FRAME *)lcp_frame,
                                      LCP_OPTION_AUTH, 3, temp_ptr);
                }
                /* If CHAP was NAKed, maybe we can use PAP. */
                else if (auth == PPP_FLAG_PAP)
                {
                    PrintOpt("      LCP Local Req: Requesting %s\n", "PAP");
                    PUT16(temp_ptr, 0, PPP_PAP_PROTOCOL);
                    LCP_Append_Option((LCP_FRAME *)lcp_frame,
                                      LCP_OPTION_AUTH, 2, temp_ptr);
                }
            }
        }

        /* Add the Magic Number option. */
        if (lcp->options.local.flags & PPP_FLAG_MAGIC)
        {
            if (lcp->options.local.magic_number == 0)
                lcp->options.local.magic_number = LCP_Random_Number32();

            PUT32(temp_ptr, 0, lcp->options.local.magic_number);
            LCP_Append_Option((LCP_FRAME *)lcp_frame, LCP_OPTION_MAGIC,
                              4, temp_ptr);
        }

        /* Protocol Field Compression. */
        if (lcp->options.local.flags & PPP_FLAG_PFC)
            LCP_Append_Option((LCP_FRAME *)lcp_frame, LCP_OPTION_PFC,
                              0, temp_ptr);

        /* Address and Control Compression. */
        if (lcp->options.local.flags & PPP_FLAG_ACC)
            LCP_Append_Option((LCP_FRAME *)lcp_frame, LCP_OPTION_ACC,
                              0, temp_ptr);

#if (INCLUDE_PPP_MP == NU_TRUE)

        /* Add the Max Receive Reconstructed Unit option. */
        if (lcp->options.local.flags & PPP_FLAG_MRRU)
        {
            PrintOpt("      LCP Local Req: MRRU(%d)\n", lcp->options.local.mp_mrru);

            PUT16(temp_ptr, 0, lcp->options.local.mp_mrru);
            LCP_Append_Option((LCP_FRAME *)lcp_frame, LCP_OPTION_MRRU,
                              2, temp_ptr);
        }

        /* Add short sequence number header format option. */
        if(lcp->options.local.flags & PPP_FLAG_SHORT_SEQ_NUM)
        {
            PrintOpt("      LCP Local Req: %s MP Short Sequence number header \n", "Enable");
            LCP_Append_Option((LCP_FRAME *)lcp_frame,
                              LCP_OPTION_SHORT_SEQ_NUM, 0, temp_ptr);
        }

        /* Add endpoint discriminator option along with the endpoint
        discriminator value. */
        if(lcp->options.local.flags & PPP_FLAG_ENDPOINT_DISC)
        {
            temp_ptr[0] = lcp->options.local.mp_endpoint_class;

            memcpy(&temp_ptr[1], lcp->options.local.mp_endpoint_disc,
                lcp->options.local.mp_endpoint_disc_len);

            LCP_Append_Option((LCP_FRAME *)lcp_frame, LCP_OPTION_ENDPOINT_DISC,
                (UINT8)(lcp->options.local.mp_endpoint_disc_len + 1), temp_ptr);
        }
#endif

        /* If this device is being used with the Nucleus L2TP LAC then we need
        to store the LCP request. */
        if(link_layer->hwi.itype & PPP_ITYPE_L2TP_LAC)
        {
            /* store the LCP request we are sending to the remote end */
            NU_BLOCK_COPY(lcp->options.local.last_cfg_req, lcp_frame, lcp_frame->len);
        }

        /* Send the configure request packet. */
        LCP_Send(dev_ptr, buf_ptr, PPP_LINK_CONTROL_PROTOCOL);
    }

    /* Set a timer event for retransmitting if necessary. */
    TQ_Timerset(PPP_Event, (UNSIGNED)dev_ptr, LCP_TIMEOUT_VALUE, LCP_SEND_CONFIG);

} /* LCP_Send_Config_Req */



/*************************************************************************
* FUNCTION
*
*     LCP_Check_Config_Req
*
* DESCRIPTION
*
*     Handle an incoming Configure Request packet according to the PPP
*     state machine.
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
VOID LCP_Check_Config_Req(NET_BUFFER *buf_ptr)
{
    NET_BUFFER      *ack_buf = NU_NULL;
    DV_DEVICE_ENTRY *dev_ptr;
    LINK_LAYER      *link_layer;
    LCP_LAYER       *lcp;
    STATUS          status;
    UINT8           state;

    /* Get some pointers to our data. */
    dev_ptr = buf_ptr->mem_buf_device;
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
    lcp = &link_layer->lcp;

    /* Save current state. */
    state = lcp->state;

    PrintInfo("   LCP_Check_Config_Req\n");

    status = LCP_Process_Request(buf_ptr, &ack_buf);
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
                lcp->state = REQ_SENT;
            else
                lcp->state = ACK_SENT;
            /* Send the ack/nak/rej. */
            if (ack_buf != NU_NULL)
                LCP_Send(dev_ptr, ack_buf, PPP_LINK_CONTROL_PROTOCOL);

            break;

        case ACK_RCVD:
            if (status == PPP_FLAG_ACK)
                lcp->state = OPENED;
            /* Send the ack/nak/rej. */
            if (ack_buf != NU_NULL)
                LCP_Send(dev_ptr, ack_buf, PPP_LINK_CONTROL_PROTOCOL);
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


        PrintState("   LCP state == %s --> %s\n", StateStrings[state], StateStrings[lcp->state]);

        /* Perform response routine based on current state. */
        switch (state)
        {
        case CLOSED:
            LCP_Send_Terminate_Ack(dev_ptr, PPP_LINK_CONTROL_PROTOCOL);
            break;

        case STOPPED:
            lcp->num_transmissions = LCP_MAX_CONFIGURE;
            LCP_Send_Config_Req(dev_ptr);
            break;

        case ACK_RCVD:
            if (status == PPP_FLAG_ACK)
            {
                /* If the peer did not request authentication, and we do not
                   require authentication, then consider ourselves authenticated
                   for the move into NCP negotiation. */
                if (link_layer->authentication.state != AUTH_REQUIRED)
                    link_layer->authentication.state = AUTHENTICATED;

                /* Set the LCP up event to start using the link. */
                EQ_Put_Event(PPP_Event, (UNSIGNED)dev_ptr, LCP_LAYER_UP);
            }

            break;

        case OPENED:
            if(lcp->state == ACK_SENT)
                lcp->state = OPENED;
            else
            {
                lcp->num_transmissions = LCP_MAX_CONFIGURE;
                LCP_Send_Config_Req(dev_ptr);
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
}



/*************************************************************************
* FUNCTION
*
*     LCP_Check_Config_Ack
*
* DESCRIPTION
*
*     Handle an incoming Configure ACK packet according to the PPP
*     state machine.
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
VOID LCP_Check_Config_Ack(NET_BUFFER *buf_ptr)
{
    DV_DEVICE_ENTRY *dev_ptr;
    LINK_LAYER      *link_layer;
    LCP_LAYER       *lcp;
    UINT8           state;

    /* Get some pointers to our data. */
    dev_ptr = buf_ptr->mem_buf_device;
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
    lcp = &link_layer->lcp;

    lcp->num_transmissions = LCP_MAX_CONFIGURE;
    TQ_Timerunset(PPP_Event, TQ_CLEAR_EXACT, (UNSIGNED)dev_ptr, LCP_SEND_CONFIG);

    /* Save current state. */
    state = lcp->state;

    PrintInfo("   LCP_Check_Config_Ack\n");

    /* Change to next state. */
    switch (state)
    {
    case REQ_SENT:
        lcp->state = ACK_RCVD;
        break;

    case ACK_SENT:
        lcp->state = OPENED;
        break;

    case ACK_RCVD:
    case OPENED:
        lcp->state = REQ_SENT;
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

    PrintState("   LCP state == %s --> %s\n", StateStrings[state], StateStrings[lcp->state]);

    /* Perform response routine based on current state. */
    switch (state)
    {
    case CLOSED:
    case STOPPED:
        LCP_Send_Terminate_Ack(dev_ptr, PPP_LINK_CONTROL_PROTOCOL);
        break;

    case REQ_SENT:
        break;

    case ACK_SENT:

        /* If the peer did not request authentication, and we do not
           require authentication, then consider ourselves authenticated
           for the move into NCP negotiation. */
        if (link_layer->authentication.state != AUTH_REQUIRED)
            link_layer->authentication.state = AUTHENTICATED;

        /* Set the LCP up event to start using the link. */
        EQ_Put_Event(PPP_Event, (UNSIGNED)dev_ptr, LCP_LAYER_UP);

        break;

    case ACK_RCVD:
    case OPENED:
        lcp->num_transmissions = LCP_MAX_CONFIGURE;
        LCP_Send_Config_Req(dev_ptr);

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



/*************************************************************************
* FUNCTION
*
*     LCP_Check_Config_Nak
*
* DESCRIPTION
*
*     Handle an incoming Configure NAK packet according to the PPP
*     state machine.
*
* INPUTS
*
*     nak_type
*     *buf_ptr
*
* OUTPUTS
*
*     None
*
*************************************************************************/
VOID LCP_Check_Config_Nak(UINT8 nak_type, NET_BUFFER *buf_ptr)
{
    DV_DEVICE_ENTRY *dev_ptr;
    LINK_LAYER      *link_layer;
    LCP_LAYER       *lcp;
    STATUS          status;
    UINT8           state;

    /* Get some pointers to our data. */
    dev_ptr = buf_ptr->mem_buf_device;
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
    lcp = &link_layer->lcp;

    lcp->num_transmissions = LCP_MAX_CONFIGURE;
    TQ_Timerunset(PPP_Event, TQ_CLEAR_EXACT, (UNSIGNED)dev_ptr, LCP_SEND_CONFIG);

    /* Save current state. */
    state = lcp->state;

    PrintInfo("   LCP_Check_Config_Nak\n");

    status = LCP_Process_Nak(nak_type, buf_ptr);
    if (status != PPP_FLAG_DROP)
    {
        /* Change to next state. */
        switch (state)
        {
        case REQ_SENT:
            if (status == PPP_FLAG_ABORT)
                lcp->state = STOPPING;
            break;

        case ACK_RCVD:
        case OPENED:
            lcp->state = REQ_SENT;
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

    PrintState("   LCP state == %s --> %s\n", StateStrings[state], StateStrings[lcp->state]);

        /* Perform response routine based on current state. */
        switch (state)
        {
        case CLOSED:
        case STOPPED:
            LCP_Send_Terminate_Ack(dev_ptr, PPP_LINK_CONTROL_PROTOCOL);
            break;

        case REQ_SENT:
        case ACK_SENT:
            if (status == PPP_FLAG_ABORT)
            {
                /* Terminate the connection */
                link_layer->lcp.state = CLOSING;

                /* Set the LCP close event to close the link. */
                EQ_Put_Event(PPP_Event, (UNSIGNED)dev_ptr, LCP_CLOSE_REQUEST);
            }
            else
                LCP_Send_Config_Req(dev_ptr);

            break;

        case ACK_RCVD:
        case OPENED:
            lcp->num_transmissions = LCP_MAX_CONFIGURE;
            LCP_Send_Config_Req(dev_ptr);

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
}



/*************************************************************************
* FUNCTION
*
*     LCP_Check_Terminate_Req
*
* DESCRIPTION
*
*     Handle an incoming Terminate Request packet according to the PPP
*     state machine.
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
VOID LCP_Check_Terminate_Req(NET_BUFFER *buf_ptr)
{
    DV_DEVICE_ENTRY *dev_ptr;
    LINK_LAYER      *link_layer;
    LCP_LAYER       *lcp;
    UINT8           state;

    /* Get some pointers to our data. */
    dev_ptr = buf_ptr->mem_buf_device;
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
    lcp = &link_layer->lcp;

    /* Save current state. */
    state = lcp->state;

    PrintInfo("   LCP_Check_Terminate_Req\n");

    /* Change to next state. */
    switch (state)
    {
    case REQ_SENT:
    case ACK_RCVD:
    case ACK_SENT:
        lcp->state = REQ_SENT;
        break;

    case OPENED:
        lcp->state = STOPPING;
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

    PrintState("   LCP state == %s --> %s\n", StateStrings[state], StateStrings[lcp->state]);

    /* Perform response routine based on current state. */
    switch (state)
    {
    case OPENED:
        EQ_Put_Event(PPP_Event, (UNSIGNED)dev_ptr, LCP_CLOSE_REQUEST);
        break;

    case CLOSED:
    case STOPPED:
    case CLOSING:
    case STOPPING:
    case REQ_SENT:
    case ACK_RCVD:
    case ACK_SENT:
        EQ_Put_Event(PPP_Event, (UNSIGNED)dev_ptr, LCP_SEND_TERMINATE);
        break;

    case INITIAL:
    case STARTING:
    default:
        break;
    }

}



/*************************************************************************
* FUNCTION
*
*     LCP_Check_Terminate_Ack
*
* DESCRIPTION
*
*     Handle an incoming Terminate ACK packet according to the PPP
*     state machine.
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
VOID LCP_Check_Terminate_Ack(NET_BUFFER *buf_ptr)
{
    DV_DEVICE_ENTRY *dev_ptr;
    LINK_LAYER      *link_layer;
    LCP_LAYER       *lcp;
    UINT8           state;

    /* Get some pointers to our data. */
    dev_ptr = buf_ptr->mem_buf_device;
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
    lcp = &link_layer->lcp;

    TQ_Timerunset(PPP_Event, TQ_CLEAR_EXACT, (UNSIGNED)dev_ptr, LCP_SEND_TERMINATE);

    /* Save current state. */
    state = lcp->state;

    PrintInfo("   LCP_Check_Terminate_Ack\n");

    /* Change to next state. */
    switch (state)
    {
    case CLOSING:
        lcp->state = CLOSED;
        break;

    case STOPPING:
        lcp->state = STOPPED;
        break;

    case ACK_RCVD:
    case OPENED:
        lcp->state = REQ_SENT;
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

    PrintState("   LCP state == %s --> %s\n", StateStrings[state], StateStrings[lcp->state]);

    /* Perform response routine based on current state. */
    switch (state)
    {
    case OPENED:
        lcp->num_transmissions = LCP_MAX_CONFIGURE;
        LCP_Send_Config_Req(dev_ptr);
        break;

    case CLOSING:
    case STOPPING:
        EQ_Put_Event(PPP_Event, (UNSIGNED)dev_ptr, LCP_LAYER_DOWN);
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

}



/*************************************************************************
* FUNCTION
*
*     LCP_Process_Request
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
*     *in_buf_ptr
*     **ack_buf
*
* OUTPUTS
*
*     STATUS
*
*************************************************************************/
STATUS LCP_Process_Request(NET_BUFFER *in_buf_ptr, NET_BUFFER **ack_buf)
{
    STATUS          status = PPP_FLAG_ACK;
    LINK_LAYER      *link_layer;
    LCP_FRAME       *lcp_frame, *ack_frame;
    LCP_OPTION      *option;
    DV_DEVICE_ENTRY *dev_ptr;
    UINT32          len = 0, magic;
    UINT8           temp_ptr[16];
    UINT16          mru, type;
    LCP_LAYER       *lcp;
    STATIC UINT8    retx_nak = 0;

    /* Get some pointers to our data. */
    dev_ptr = in_buf_ptr->mem_buf_device;
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
    lcp = &link_layer->lcp;
    lcp_frame = (LCP_FRAME*)in_buf_ptr->data_ptr;

    /* If this device is being used with the Nucleus L2TP then we need to
    store the LCP request. */
   if(link_layer->hwi.itype & PPP_ITYPE_L2TP_LAC)
   {
        /* check if initial LCP request has already arrived */
        if(((LCP_FRAME *)lcp->options.remote.init_cfg_req)->len == 0)
        {
          /* store the initial LCP request from the remote end */
          NU_BLOCK_COPY(lcp->options.remote.init_cfg_req, lcp_frame, lcp_frame->len);
        }

        /* store the incoming LCP request from the remote end */
        NU_BLOCK_COPY(lcp->options.remote.last_cfg_req, lcp_frame, lcp_frame->len);
    }

    PrintInfo("   LCP_Process_Request\n");

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
        option = (LCP_OPTION*)(&lcp_frame->data[(INT)len]);
        if (option->len == 0)
            break;

        /* Check out what the option is and see if we can handle it */
        switch (option->code)
        {
        case LCP_OPTION_MRU:

            PrintOpt("      LCP Peer Req: Max RX Unit(%d)\n", GET16(option->data, 0));

            if (lcp->options.remote.default_flags & PPP_FLAG_MRU)
            {
                /* Pull the MRU from the packet. */
                mru = GET16(option->data, 0);

                /* Update the MRU for this device. The default MRU represents
                   the minimum requirement. */
                if (mru >= lcp->options.remote.default_mru)
                {
                    /* The requested MTU is accepted. */
                    lcp->options.remote.mru = mru;

                    /* If the remote MRU is less than what is written to the
                       device, then it needs to be written to the device, since
                       that is the only way the Net stack will know its TX
                       limitations. */
                    if (mru < dev_ptr->dev_mtu)
                        dev_ptr->dev_mtu = mru;
                }
                else
                {
                    /* The requested MRU is smaller than minimum default. NAK it
                       with our own MRU. */
                    PUT16(option->data, 0, lcp->options.remote.default_mru);
                    status = LCP_Nak_Option(ack_frame, option->code, (UINT8)(option->len - LCP_OPTION_HDRLEN), option->data);
                }
            }
            else
            {
                /* Not negotiating this option. Reject it. */
                status = LCP_Reject_Option(ack_frame, option->code, (UINT8)(option->len - LCP_OPTION_HDRLEN), option->data);
            }

            break;


        case LCP_OPTION_ACCM:

            PrintOpt("      LCP Peer Req: ACCM(%08x)\n", GET32(option->data, 0));

            if (lcp->options.remote.default_flags & PPP_FLAG_ACCM)
            {
                /* Store off the char map for our peer. */
                lcp->options.remote.accm = GET32(option->data, 0);
            }
            else
            {
                /* Not negotiating this option. Reject it. */
                status = LCP_Reject_Option(ack_frame, option->code, (UINT8)(option->len - LCP_OPTION_HDRLEN), option->data);
            }

            break;

        case LCP_OPTION_AUTH:

            type = GET16(option->data, 0);
            PrintOpt("      LCP Peer Req: Authentication (%.04x)\n", type);

            link_layer->authentication.state = AUTH_REQUIRED;

            /* If we are capable of PAP or CHAP authentication, then
               negotiate this option. Otherwise reject it. */
            if (lcp->options.remote.default_flags & PPP_AUTH_MASK)
            {
                /* Validate the authentication protocol type. */
                if (type == PPP_CHAP_PROTOCOL && lcp->options.remote.default_flags & PPP_FLAG_CHAP 
                    && retx_nak < 5)
                {
                    /* CHAP requested, make sure it is with MD5/MSCHAPv1/MSCHAPv2. */
                    if (option->len < 5 || (
#if(PPP_USE_CHAP_MS2 == NU_TRUE)
                                (option->data[2] != LCP_CHAP_MS2) &&
#endif
#if(PPP_USE_CHAP_MS1 == NU_TRUE)
                                 (option->data[2] != LCP_CHAP_MS1) &&
#endif
                                 (option->data[2] != LCP_CHAP_MD5)))

                    {
                        /* At this point the peer is using CHAP but not with
                           MD5/MSCHAPv1/MSCHAPv2. We must NAK and state that
                           we must use one of them.
                        */
                        PUT16(temp_ptr, 0, PPP_CHAP_PROTOCOL);
#if(PPP_USE_CHAP_MS2 == NU_TRUE)
                        if(lcp->options.remote.flags & PPP_FLAG_CHAP_MS2)
                        {
                            temp_ptr[2] = LCP_CHAP_MS2;
                            lcp->options.remote.flags &= ~PPP_FLAG_CHAP_MS2;
                        }

                        else
#endif
#if(PPP_USE_CHAP_MS1 == NU_TRUE)
                        if(lcp->options.remote.flags & PPP_FLAG_CHAP_MS1)
                        {
                            temp_ptr[2] = LCP_CHAP_MS1;
                            lcp->options.remote.flags &= ~PPP_FLAG_CHAP_MS1;
                        }
                        else
#endif
                            temp_ptr[2] = LCP_CHAP_MD5;

                        /* store the chap protocol we'll be using */
                        lcp->options.remote.chap_protocol = temp_ptr[2];

                        /* Put it in the NAK packet */
                        status = LCP_Nak_Option(ack_frame, option->code, (UINT8)3, temp_ptr);

                        /* Increment the number of times we have sent this NAK. */
                        retx_nak++;

                    }
                    else
                    {
                        /* If we make it here then our peer is using
                           MD5-CHAP, which is fine with us. Put this in
                           in the options structure. */
                        lcp->options.remote.flags |= PPP_FLAG_CHAP;
                        lcp->options.remote.chap_protocol = option->data[2];

                        /* Reset the counter. */
                        retx_nak = 0;
                    }

                }

                /* See if it is PAP */
                else if (type == PPP_PAP_PROTOCOL && lcp->options.remote.default_flags & PPP_FLAG_PAP)
                {
                    /* This is fine. Put it in the options structure. */
                    lcp->options.remote.flags |= PPP_FLAG_PAP;

                    /* Reset the counter. */
                    retx_nak = 0;
                }

                /* Unknown authentication protocol. NAK it in the hopes that
                   a supported protocol will be received in the next request. */

                /* If we can do CHAP, then add it to the NAK. */
                else if ((lcp->options.remote.default_flags & PPP_FLAG_CHAP)
                    && retx_nak < 5)
                {
                    PUT16(temp_ptr, 0, PPP_CHAP_PROTOCOL);
                    temp_ptr[2] = LCP_CHAP_MD5;
                    lcp->options.remote.chap_protocol = temp_ptr[2];

                    /* Put it in the NAK packet */
                    status = LCP_Nak_Option(ack_frame, option->code, (UINT8)3, temp_ptr);

                    /* Increment the number of times we have sent this NAK. */
                    retx_nak++;
                }

                /* If we can do PAP, then add it to the NAK. */
                else if ((lcp->options.remote.default_flags & PPP_FLAG_PAP)
                     && retx_nak < 5)
                {
                    /* All we have left is PAP. */
                    PUT16(temp_ptr, 0, PPP_PAP_PROTOCOL);

                    /* Put it in the NAK packet */
                    status = LCP_Nak_Option(ack_frame, option->code, (UINT8)2, temp_ptr);

                    /* Increment the number of times we have sent this NAK. */
                    retx_nak++;
                }

                else
                {
                    /* Not negotiating this option. Reject it. */
                    status = LCP_Reject_Option(ack_frame, option->code, (UINT8)(option->len - LCP_OPTION_HDRLEN), option->data);

                    /* Reset the counter. */
                    retx_nak = 0;
                }

            }
            else
            {
                /* Not negotiating this option. Reject it. */
                status = LCP_Reject_Option(ack_frame, option->code, (UINT8)(option->len - LCP_OPTION_HDRLEN), option->data);
            }

            break;

        case LCP_OPTION_MAGIC:

            PrintOpt("      LCP Peer Req: Magic Number (%.08x)\n", GET32(option->data, 0));

            if (lcp->options.remote.default_flags & PPP_FLAG_MAGIC)
            {
                /* Extract the magic number from the packet */
                magic = GET32(option->data, 0);

                /* Make sure they do not match our magic number. */
                if (magic == lcp->options.local.magic_number)
                {
                    /* Get a new number for the remote side. */
                    lcp->options.remote.magic_number = LCP_Random_Number32();

                    /* Add the requested number to the data string. */
                    PUT32(temp_ptr, 0, magic);

                    /* Append the new recommended number. */
                    PUT32(temp_ptr, 4, lcp->options.remote.magic_number);

                    /* Add it to the NAK packet. */
                    status = LCP_Nak_Option(ack_frame, option->code, 8, temp_ptr);
                }
                else
                    /* Their number is ok. Write it into the LCP options structure. */
                    lcp->options.remote.magic_number = magic;
            }
            else
            {
                /* Not negotiating this option. Reject it. */
                status = LCP_Reject_Option(ack_frame, option->code, (UINT8)(option->len - LCP_OPTION_HDRLEN), option->data);
            }

            break;

        case LCP_OPTION_PFC:

            PrintOpt("      LCP Peer Req: %s Protocol compression\n", "Enable");

            lcp->options.remote.flags |= PPP_FLAG_PFC;

            break;

        case LCP_OPTION_ACC:

            PrintOpt("      LCP Peer Req: %s Address/Control compression\n", "Enable");

            lcp->options.remote.flags |= PPP_FLAG_ACC;

            break;

#if (INCLUDE_PPP_MP == NU_TRUE)

        /* Multilink MRRU option*/
        case LCP_OPTION_MRRU:

            PrintOpt("      LCP Peer Req: MP MRRU (%d)\n", GET16(option->data, 0));

            if (lcp->options.remote.default_flags & PPP_FLAG_MRRU)
            {
                /* Pull the MRRU from the packet. */
                mru = GET16(option->data, 0);

                /* Update the MRRU for this device. The default MRRU
                   represents the minimum requirement. */
                if (mru >= lcp->options.remote.default_mru)
                {
                    /* The requested MRRU is accepted. */
                    lcp->options.remote.mp_mrru = mru;

                    /* The peer has negotiated Multilink Protocol which is
                    specified by the sending of MRRU */
                    lcp->options.remote.flags |= PPP_FLAG_MRRU;
                }

                else
                {
                    /* The requested MRRU is smaller than minimum default.
                    NAK it with our own MRRU. */
                    PUT16(option->data, 0, lcp->options.remote.default_mru);

                    status = LCP_Nak_Option(ack_frame, option->code,
                        (UINT8)(option->len - LCP_OPTION_HDRLEN), option->data);
                }
            }
            else
            {
                /* Not negotiating this option. Reject it. */
                status = LCP_Reject_Option(ack_frame, option->code,
                    (UINT8)(option->len - LCP_OPTION_HDRLEN), option->data);
            }

            break;

        /* Multilink endpoint discriminator option */
        case LCP_OPTION_ENDPOINT_DISC:

            PrintOpt("      LCP Peer Req: %s \n", "MP Endpoint Discriminator");

            if (lcp->options.remote.default_flags & PPP_FLAG_ENDPOINT_DISC)
            {
                /* check if endpoint discriminator received is from a
                valid class of endpoint discriminators */
                if( (option->data[0] == LCP_ENDPOINT_DISC_NULL) ||
                    (option->data[0] == LCP_ENDPOINT_DISC_MAC) ||
                    (option->data[0] == LCP_ENDPOINT_DISC_IP) ||
                    (option->data[0] == LCP_ENDPOINT_DISC_DIRECTORY) )
                {
                    /* get the endpoint discriminator class  */
                    lcp->options.remote.mp_endpoint_class = option->data[0];

                    /* Check if the endpoint discriminator length does not
                    exceed the maximum allowed */
                    if((option->len - LCP_OPTION_HDRLEN - 1) <=
                        LCP_ENDPOINT_DISC_DIRECTORY_MAX_SIZE)
                    {
                        /* copy the discriminator */
                        memcpy(lcp->options.remote.mp_endpoint_disc,
                            &option->data[1], (option->len - LCP_OPTION_HDRLEN - 1));
                    }
                }

                else
                {
                    /* Not negotiating this option. Reject it. */
                    status = LCP_Reject_Option(ack_frame, option->code,
                        (UINT8)(option->len - LCP_OPTION_HDRLEN), option->data);
                }
            }
            break;

        /* Multilink short sequence number header option */
        case LCP_OPTION_SHORT_SEQ_NUM:

            PrintOpt("      LCP Peer Req: %s MP Short Sequence number header \n", "Enable");

            /* Add short sequence number header format option */
            if(lcp->options.remote.default_flags & PPP_FLAG_SHORT_SEQ_NUM)
            {
                lcp->options.remote.flags |= PPP_FLAG_SHORT_SEQ_NUM;
            }
            break;
#endif

        default:
            PrintOpt("      LCP Peer Req: Unknown option (%.02x)\n", option->code);

            /* If we make it here then this is an option that we do not
               support, so send a reject. */
            status = LCP_Reject_Option(ack_frame, option->code, (UINT8)(option->len - LCP_OPTION_HDRLEN), option->data);

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
}

/*************************************************************************
* FUNCTION
*
*     LCP_Process_Nak
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
*     nak_type
*     *in_buf_ptr
*
* OUTPUTS
*
*     STATUS
*
*************************************************************************/
STATUS LCP_Process_Nak(UINT8 nak_type, NET_BUFFER *in_buf_ptr)
{
    DV_DEVICE_ENTRY *dev_ptr;
    LINK_LAYER      *link_layer;
    LCP_FRAME       *lcp_frame;
    LCP_OPTION      *option;
    STATUS          status = PPP_FLAG_OK;
    UINT16          len = 0, mru;
    LCP_LAYER       *lcp;

    /* Get a pointer to the IPCP structure for this device. */
    dev_ptr = (DV_DEVICE_ENTRY *)in_buf_ptr->mem_buf_device;
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
    lcp = &link_layer->lcp;
    lcp_frame = (LCP_FRAME*)in_buf_ptr->data_ptr;

#if (PPP_DEBUG_PRINT_OK == NU_TRUE)
    if (nak_type == PPP_FLAG_NAK)
        PrintInfo("   LCP_Process_Nak\n");
    else
        PrintInfo("   LCP_Process_Reject\n");
#endif

    /* Make sure this is a response to our request. */
    if (lcp_frame->id == lcp->identifier)
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
            case LCP_OPTION_MRU:

                /* The MRU should not be NAKed or REJECTed, but if it is,
                   handle it appropriately. */
                if (nak_type == PPP_FLAG_REJECT)
                {
                    PrintOpt("      LCP Peer REJECT: %s\n", "Max RX Unit");

                    /* This means the MRU must remain the standard 1500. */
                    lcp->options.local.flags &= ~PPP_FLAG_MRU;
                }
                else
                {
                    /* Pull the MRU from the packet. */
                    mru = GET16(option->data, 0);

                    PrintOpt("      LCP Peer NAK: Max RX Unit(%d)\n", mru);

                    /* Update the MRU for this device if it is a
                       smaller value. Otherwise, just bump up the
                       pointers and move on. */
                    if (mru <= lcp->options.local.default_mru)
                    {
                        /* The requested MTU is accepted. */
                        lcp->options.local.mru = mru;
                    }
                }

                break;

            case LCP_OPTION_ACCM:

                if (nak_type == PPP_FLAG_REJECT)
                {
                    PrintOpt("      LCP Peer REJECT: ACCM(%.08x)\n", lcp->options.local.accm);
                    lcp->options.local.flags &= ~PPP_FLAG_ACCM;
                }
                else
                {
                    /* What this means is that our peer knows of some
                       chars that should be mapped when we RX. Put
                       them in the options structure. */
                    lcp->options.local.accm = GET32(option->data, 0);
                    PrintOpt("      LCP Peer NAK: ACCM(%.08x)\n", lcp->options.local.accm);
                }

                break;

            case LCP_OPTION_AUTH:

                if (nak_type == PPP_FLAG_REJECT)
                {
                    PrintOpt("      LCP Peer REJECT: Auth (%.04x)\n", 0);

                    /* A peer can't reject authentication. This will
                       terminate negotiation. */
                    lcp->options.local.flags &= ~PPP_AUTH_MASK;
                }
                else
                {
                    /* The preferred authentication protocol was NAKed, so clear its
                       flag from the current options. */
                    if (lcp->options.local.flags & lcp->options.local.default_flags & PPP_AUTH_MASK)
                    {
                        if(lcp->options.local.flags & lcp->options.local.default_flags & PPP_FLAG_CHAP_MS1)
                            lcp->options.local.flags &= ~(lcp->options.local.default_flags & PPP_FLAG_CHAP_MS1);
                        else if(lcp->options.local.flags & lcp->options.local.default_flags & PPP_FLAG_CHAP_MS2)
                            lcp->options.local.flags &= ~(lcp->options.local.default_flags & PPP_FLAG_CHAP_MS2);
                        else if(lcp->options.local.flags & PPP_FLAG_CHAP_MS2)
                            lcp->options.local.flags &= ~PPP_FLAG_CHAP_MS2;
                        else if(lcp->options.local.flags & PPP_FLAG_CHAP_MS1)
                            lcp->options.local.flags &= ~PPP_FLAG_CHAP_MS1;
                        else
                            lcp->options.local.flags &= ~(lcp->options.local.default_flags & PPP_AUTH_MASK);
                    }
                    else
                        lcp->options.local.flags &= ~PPP_AUTH_MASK;
                }

                /* Make sure we can still authenticate the peer if needed. */
                if ((link_layer->authentication.state == AUTH_REQUIRED)
                    && !(lcp->options.local.flags & PPP_AUTH_MASK))
                {
                    /* Client unable to authenticate. */
                    status = PPP_FLAG_ABORT;
                }

                break;

            case LCP_OPTION_MAGIC:

                if (nak_type == PPP_FLAG_REJECT)
                {
                    PrintOpt("      LCP Peer REJECT: Magic number (%.08x)\n", lcp->options.local.magic_number);
                    lcp->options.local.flags &= ~PPP_FLAG_MAGIC;
                }
                else
                {
                    /* Being NAKed with this option means that we might
                       be in a looped-back condition. Check to see if
                       they are the same, this could be a response to a
                       previous magic number conflict. */

                    /* Extract the magic number from the packet */
                    lcp->options.local.magic_number = GET32(option->data, 0);

                    PrintOpt("      LCP Peer NAK: Magic Number(%.08x)\n", lcp->options.local.magic_number);

                    /* Make sure they are not the same. */
                    if (lcp->options.remote.magic_number == lcp->options.local.magic_number)
                    {
                        /* Since they are the same we need to create a new number
                           for the next config request packet. */
                        lcp->options.local.magic_number = LCP_Random_Number32();
                    }
                }

                break;

            case LCP_OPTION_PFC:

                if (nak_type == PPP_FLAG_REJECT)
                    lcp->options.local.flags &= ~PPP_FLAG_PFC;
                break;

            case LCP_OPTION_ACC:

                if (nak_type == PPP_FLAG_REJECT)
                    lcp->options.local.flags &= ~PPP_FLAG_ACC;
                break;

#if (INCLUDE_PPP_MP == NU_TRUE)

            /* Multilink Protocol's MRRU option */
            case LCP_OPTION_MRRU:

                /* The MRRU should not be NAKed or REJECTed, but if it is,
                   handle it appropriately. */
                if (nak_type == PPP_FLAG_REJECT)
                {
                    PrintOpt("      LCP Peer REJECT: %s\n", "MP MRRU");

                    /* Multilink Operation not required. */
                    lcp->options.local.flags &= ~PPP_FLAG_MRRU;
                }

                break;

            /* MP short sequence number header option */
            case LCP_OPTION_SHORT_SEQ_NUM:

                /* if short sequence number should not be used */
                if (nak_type == PPP_FLAG_REJECT)
                {
                    PrintOpt("      LCP Peer REJECT: %s\n", "Short Seq Num");

                    /* short sequence number should not be used. */
                    lcp->options.local.flags &= ~PPP_FLAG_SHORT_SEQ_NUM;
                }

                break;

            /* MP endpoint discriminator option */
            case LCP_OPTION_ENDPOINT_DISC:

                if (nak_type == PPP_FLAG_REJECT)
                {
                    PrintOpt("      LCP Peer REJECT: %s\n", "MP Endpoint Discriminator");

                    /* local endpoint discriminator should not be used */
                    lcp->options.local.flags &= ~PPP_FLAG_ENDPOINT_DISC;
                }

#endif /* INCLUDE_PPP_MP */

            default:

                /* If we make it here then this is an option that we did not
                   request or was incorrectly naked/rejected. */
                break;

            } /* switch */

            /* Setup for next option. */
            len = (UINT16)(len + option->len);
        }
    }
    else
        status = PPP_FLAG_DROP;

    return status;
}



/*************************************************************************
*
*   FUNCTION
*
*       LCP_Send_Protocol_Reject
*
*   DESCRIPTION
*
*       Send a protocol reject packet to our peer. This is in response to
*       having received a packet destined for some unknown network protocol.
*       Since a protocol reject packet is just the unknown packet with a
*       protocol reject header, we can reuse this buffer.
*
*   INPUTS
*
*       NET_BUFFER  *in_buf_ptr     Pointer to the unknown protocol
*                                   packet.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID LCP_Send_Protocol_Reject(NET_BUFFER *in_buf_ptr)
{
    DV_DEVICE_ENTRY *dev_ptr;
    NET_BUFFER      *buf_ptr;
    LCP_FRAME       *lcp_frame;

    /* Get some pointers to our data. */
    dev_ptr = in_buf_ptr->mem_buf_device;

    in_buf_ptr->data_ptr -= 2;
    in_buf_ptr->data_len += 2;

    /* Create an outgoing packet. Most of the fields will
       be pre-filled. */
    buf_ptr = LCP_New_Buffer(dev_ptr, LCP_PROTOCOL_REJECT, -1);
    if (buf_ptr == NU_NULL)
        /* This error is logged within LCP_New_Buffer, so just return. */
        return;

    lcp_frame = (LCP_FRAME*)buf_ptr->data_ptr;

    /* Copy the rejected packet to the new buffer. */
    memcpy(lcp_frame->data, in_buf_ptr->data_ptr,
           (INT)in_buf_ptr->data_len);
    lcp_frame->len = (UINT16)(lcp_frame->len + in_buf_ptr->data_len);

    /* Send it. */
    LCP_Send(dev_ptr, buf_ptr, PPP_LINK_CONTROL_PROTOCOL);

} /* LCP_Send_Protocol_Reject */



/*************************************************************************
*
*   FUNCTION
*
*       LCP_Send_Code_Reject
*
*   DESCRIPTION
*
*       Send a code reject packet to our peer. This is in response to
*       having received an unknown code for a known protocol type.
*
*   INPUTS
*
*       NET_BUFFER  *in_buf_ptr     Pointer to the unknown protocol
*                                   packet.
*       UINT16      protocol        Protocol type.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID LCP_Send_Code_Reject(NET_BUFFER *in_buf_ptr, UINT16 protocol)
{
    DV_DEVICE_ENTRY *dev_ptr;
    NET_BUFFER      *buf_ptr;
    LCP_FRAME       *lcp_frame;

    /* Get some pointers to our data. */
    dev_ptr = in_buf_ptr->mem_buf_device;

    /* Create an outgoing packet. Most of the fields will
       be pre-filled. */
    buf_ptr = LCP_New_Buffer(dev_ptr, LCP_CODE_REJECT, -1);
    if (buf_ptr == NU_NULL)
        /* This error is logged within LCP_New_Buffer, so just return. */
        return;

    lcp_frame = (LCP_FRAME*)buf_ptr->data_ptr;

    /* Copy the rejected packet to the new buffer. */
    memcpy(lcp_frame->data, in_buf_ptr->data_ptr,
           (INT)in_buf_ptr->data_len);
    lcp_frame->len = (UINT16)(lcp_frame->len + in_buf_ptr->data_len);

    /* Send it. */
    LCP_Send(dev_ptr, buf_ptr, protocol);

} /* LCP_Send_Code_Reject */



/*************************************************************************
* FUNCTION
*
*       LCP_Process_Code_Reject
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
*     *buf_ptr
*
* OUTPUTS
*
*     None
*
*************************************************************************/
VOID LCP_Process_Code_Reject(NET_BUFFER *buf_ptr)
{
    DV_DEVICE_ENTRY *dev_ptr = buf_ptr->mem_buf_device;
    LINK_LAYER      *link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

    PrintInfo("   LCP_Process_Code_Reject\n");

    /* We currently don't send any optional codes, so terminate the link. */
    EQ_Put_Event(PPP_Event, (UNSIGNED)dev_ptr, LCP_CLOSE_REQUEST);
    NU_Set_Events(&link_layer->negotiation_progression, PPP_LCP_FAIL, NU_OR);

} /* LCP_Process_Code_Reject */



/*************************************************************************
* FUNCTION
*
*       LCP_Process_Protocol_Reject
*
* DESCRIPTION
*
*       This function checks a code reject packet. If it is rejecting any
*       codes that PPP needs to work NU_FALSE is returned. If all rejected
*       codes are allowed by this implementation then NU_TRUE is returned.
*       Note that most codes specified in RFC1661 are required. Any codes
*       that are not required by this implementation are never transmitted
*       and thus will never be rejected.
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
VOID LCP_Process_Protocol_Reject(NET_BUFFER *buf_ptr)
{
    DV_DEVICE_ENTRY      *dev_ptr;
    LINK_LAYER           *link_layer;
    LCP_FRAME            *lcp_frame;
    UINT16                protocol;

#if(INCLUDE_PPP_MP == NU_TRUE)
    MIB2_IF_STACK_STRUCT *stack_entry;
#endif

    /* Get some pointers to our data. */
    dev_ptr = buf_ptr->mem_buf_device;
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
    lcp_frame = (LCP_FRAME*)buf_ptr->data_ptr;

    PrintInfo("   LCP_Process_Protocol_Reject\n");

    /* Verify that LCP is up. */
    if (link_layer->lcp.state != OPENED)
    {
        PrintInfo("Discarded - LCP not open.\n");
        PML_SilentDiscards_Inc(dev_ptr);
        return;
    }

#if(INCLUDE_PPP_MP == NU_TRUE)
    /* Get the stack entry if this device is associated with any MP
     * device. NCP is only negotiated at MP device level rather than
     * on lower PPP device.
     */
    stack_entry = MIB2_If_Stack_Get_LI_Entry(dev_ptr->dev_index + 1, NU_TRUE);

    /* If a stack entry is found */
    if((stack_entry) && (stack_entry->mib2_higher_layer))
    {
        /* Point to the link layer of the MP device. */
        dev_ptr = stack_entry->mib2_higher_layer;
        link_layer =(LINK_LAYER *) dev_ptr->dev_link_layer;

    }
#endif

    /* Check the protocol and handle it. */
    protocol = GET16(lcp_frame->data, 0);
    switch (protocol)
    {
    case PPP_CHAP_PROTOCOL:
        break;

    case PPP_PAP_PROTOCOL:
        break;

#if (INCLUDE_IPV4 == NU_TRUE)
    case PPP_IP_CONTROL_PROTOCOL:

        link_layer->ncp.options.ipcp.flags |= PPP_FLAG_NO_IPV4;
        link_layer->ncp.state = INITIAL;
        TQ_Timerunset(link_layer->ncp.event_id, TQ_CLEAR_ALL_EXTRA, (UNSIGNED)dev_ptr, 0);

#if (INCLUDE_IPV6 == NU_TRUE)
        if (!(dev_ptr->dev_flags & DV6_IPV6))
        {
            EQ_Put_Event(PPP_Event, (UNSIGNED)dev_ptr, LCP_CLOSE_REQUEST);
            NU_Set_Events(&link_layer->negotiation_progression, PPP_NCP_FAIL, NU_OR);
        }
#else

        NU_Set_Events(&link_layer->negotiation_progression, PPP_NCP_FAIL, NU_OR);
#endif

        TQ_Timerunset(link_layer->ncp.event_id, TQ_CLEAR_EXACT, (UNSIGNED)dev_ptr, NCP_SEND_CONFIG);
        EQ_Put_Event(link_layer->ncp.event_id, (UNSIGNED)dev_ptr, NCP_LAYER_UP);

        break;
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
    case PPP_IPV6_CONTROL_PROTOCOL:

        link_layer->ncp6.state = INITIAL;

        /* Cleanup the already allocated IPv6 resources since IPv6 is
         * not going to be used for this PPP interface
         */
        DEV6_Cleanup_Device(dev_ptr, 0);

        dev_ptr->dev_flags &= ~DV6_IPV6;

        TQ_Timerunset(link_layer->ncp6.event_id, TQ_CLEAR_ALL_EXTRA, (UNSIGNED)dev_ptr, 0);

#if (INCLUDE_IPV4 == NU_TRUE)
        if ((link_layer->ncp.state == STOPPED) || (link_layer->ncp.state == INITIAL))
        {
            EQ_Put_Event(PPP_Event, (UNSIGNED)dev_ptr, LCP_CLOSE_REQUEST);
            NU_Set_Events(&link_layer->negotiation_progression, PPP_NCP_FAIL, NU_OR);
        }
#endif

    break;
#endif

#if (PPP_ENABLE_MPPE == NU_TRUE)
    case PPP_CCP_PROTOCOL:

        /* Terminate the connection */
        link_layer->lcp.state = CLOSING;

        EQ_Put_Event(PPP_Event, (UNSIGNED)dev_ptr, LCP_CLOSE_REQUEST);
        NU_Set_Events(&link_layer->negotiation_progression, PPP_CCP_FAIL, NU_OR);

        break;
#endif

    default:
        break;
    }

} /* LCP_Process_Protocol_Reject */



/*************************************************************************
*
*   FUNCTION
*
*       LCP_Random_Number
*
*   DESCRIPTION
*
*       This routine will handle Pseudo-random sequence generation.  The
*       algorithms and code was taken from the draft of the C standards
*       document dated November 11, 1985.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       Returns a value in the range of 0 to RAND_MAX.
*
*************************************************************************/
UINT8 LCP_Random_Number(VOID)
{
    static UINT32 next = 1;

    next = next * NU_Retrieve_Clock() + 12345;

    return((UINT8)((next / 32) %(LCP_RAND_MAX + 1)));

}  /* LCP_Random_Number */



/*************************************************************************
*
*   FUNCTION
*
*       LCP_Random_Number32
*
*   DESCRIPTION
*
*       This routine will handle Pseudo-random sequence generation.  The
*       algorithms and code was taken from the draft of the C standards
*       document dated November 11, 1985.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       Returns a value in the range of 0 to RAND_MAX.
*
*************************************************************************/
UINT32 LCP_Random_Number32(VOID)
{
    static UINT32 next = 1;

    next = next * NU_Retrieve_Clock() + 12345;

    return(next %(LCP_RAND_MAX32 + 1));

}  /* LCP_Random_Number32 */



/*************************************************************************
*
*   FUNCTION
*
*       LCP_Send_Echo_Req
*
*   DESCRIPTION
*
*       This is expiration for the echo timer. If we have already sent
*       LCP_MAX_ECHO echo requests then the link will be considered to be down
*       and the upper layers will be notified. If not then an echo request will
*       be sent.
*
*   INPUTS
*
*       *dev_ptr
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID LCP_Send_Echo_Req(DV_DEVICE_ENTRY *dev_ptr)
{
    NET_BUFFER      *buf_ptr;
    LINK_LAYER      *link_layer;
    LCP_FRAME       *lcp_frame;
    LCP_LAYER       *lcp;

    /* Get a pointer to the lcp layer for this packet. */
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
    lcp = &link_layer->lcp;

    PrintInfo("   LCP_Send_Echo_Req\n");
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

    /* If this function is not called as a result of a timeout, then there
       may be a lingering RESEND event in the TQ. Remove it now. */
    TQ_Timerunset(PPP_Event, TQ_CLEAR_EXACT, (UNSIGNED)dev_ptr, LCP_ECHO_REQ);

    /* Make sure that we have been getting a reply */
    if (lcp->echo_counter++ < LCP_MAX_ECHO)
    {
        /* Create an outgoing request packet. Most of the fields will
           be pre-filled. */
        buf_ptr = LCP_New_Buffer(dev_ptr, LCP_ECHO_REQUEST, -1);
        if (buf_ptr == NU_NULL)
        {
            /* Just set the timer event to try this again later. */
            TQ_Timerset(PPP_Event, (UNSIGNED)dev_ptr, LCP_ECHO_VALUE, LCP_ECHO_REQ);

            /* This error is logged within LCP_New_Buffer, so just return. */
            return;
        }

        /* Get a pointer to the data part of the LCP packet. */
        lcp_frame = (LCP_FRAME*)buf_ptr->data_ptr;

        /* Save the identifier that was assigned to this packet. */
        lcp->identifier = lcp_frame->id;

        /* Put our magic number into the packet. */
        PUT32(lcp_frame->data, 0, lcp->options.local.magic_number);
        lcp_frame->len += (UINT16)LCP_MAGIC_LEN;

        /* Send the echo req packet. */
        LCP_Send(dev_ptr, buf_ptr, PPP_LINK_CONTROL_PROTOCOL);

        /* Set a timer event for the next echo. */
        TQ_Timerset(PPP_Event, (UNSIGNED)dev_ptr, LCP_ECHO_VALUE, LCP_ECHO_REQ);
    }
    else
    {
        /* Assume the modem connection is down and abort the link. */
        link_layer->hwi.state = STOPPING;
        lcp->state = STOPPING;

        /* Stop any PPP timer that may still be running */
        TQ_Timerunset(PPP_Event, TQ_CLEAR_ALL_EXTRA, (UNSIGNED)dev_ptr, 0);

        /* Disconnect the modem if it is still connected. */
        link_layer->hwi.disconnect(link_layer, NU_FALSE);

        /* Close PPP link from the bottom layer up. */
        EQ_Put_Event(PPP_Event, (UNSIGNED)dev_ptr, LCP_CLOSE_REQUEST);
    }

} /* LCP_Send_Echo_Req */



/*************************************************************************
* FUNCTION
*
*     LCP_Process_Echo_Req
*
* DESCRIPTION
*
*     Handle an incoming Echo Request packet.
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
VOID LCP_Process_Echo_Req(NET_BUFFER *buf_ptr)
{
    DV_DEVICE_ENTRY *dev_ptr;
    LINK_LAYER      *link_layer;
    LCP_LAYER       *lcp;
    LCP_FRAME       *lcp_frame;

    /* Get some pointers to our data. */
    dev_ptr = buf_ptr->mem_buf_device;
    link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
    lcp = &link_layer->lcp;

    /* Verify that LCP is up. */
    if (lcp->state != OPENED)
    {
        PrintInfo("Discarded - LCP not open.\n");
        PML_SilentDiscards_Inc(dev_ptr);
        return;
    }

    PrintInfo("   LCP_Process_Echo_Req\n");

    /* Get a pointer to the data part of the LCP packet. */
    lcp_frame = (LCP_FRAME*)buf_ptr->data_ptr;

    if (!(lcp->options.remote.flags & PPP_FLAG_MAGIC)
        || (GET32(lcp_frame->data, 0) == lcp->options.remote.magic_number))
    {
        /* Create an echo reply packet. Most of the fields will
           be pre-filled. */
        buf_ptr = LCP_New_Buffer(dev_ptr, LCP_ECHO_REPLY, lcp_frame->id);
        if (buf_ptr == NU_NULL)
            /* This error is logged within LCP_New_Buffer, so just return. */
            return;

        /* Get a pointer to the data part of the new packet. */
        lcp_frame = (LCP_FRAME*)buf_ptr->data_ptr;

        /* Put local magic number into the packet. */
        PUT32(lcp_frame->data, 0, lcp->options.local.magic_number);
        lcp_frame->len += (UINT16)LCP_MAGIC_LEN;

        /* Send the echo response. */
        LCP_Send(dev_ptr, buf_ptr, PPP_LINK_CONTROL_PROTOCOL);
    }

} /* LCP_Process_Echo_Req */



/*************************************************************************
*
*   FUNCTION
*
*       LCP_Send_Terminate_Req
*
*   DESCRIPTION
*
*       Sends a terminate request packet to the peer.
*
*   INPUTS
*
*       *dev_ptr
*       protocol
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID LCP_Send_Terminate_Req(DV_DEVICE_ENTRY *dev_ptr, UINT16 protocol)
{
    NET_BUFFER  *buf_ptr;
    LCP_FRAME   *lcp_frame;
    LCP_LAYER       *lcp;

    /* Get some pointers to our data. */
    lcp = &((LINK_LAYER*)dev_ptr->dev_link_layer)->lcp;

    PrintInfo("   LCP_Send_Terminate_Req\n");

    /* Create an outgoing request packet. Most of the fields will
       be pre-filled. */
    buf_ptr = LCP_New_Buffer(dev_ptr, LCP_TERMINATE_REQUEST, -1);
    if (buf_ptr == NU_NULL)
        /* This error is logged within LCP_New_Buffer, so just return. */
        return;

    /* Get a pointer to the data part of the LCP packet. */
    lcp_frame = (LCP_FRAME*)buf_ptr->data_ptr;

    /* Save the identifier that was assigned to this packet. */
    lcp->identifier = lcp_frame->id;

    /* Nothing else goes into the packet, so just send it. */
    LCP_Send(dev_ptr, buf_ptr, protocol);

} /* LCP_Send_Terminate_Req */



/*************************************************************************
*
*   FUNCTION
*
*       LCP_Send_Terminate_Ack
*
*   DESCRIPTION
*
*       Sends a terminate ack packet to the peer.
*
*   INPUTS
*
*       UINT8       *dev_ptr        Pointer to the device structure.
*       UINT16      protocol        Which protocol, LCP or NCP, is
*                                   sending the ack
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID LCP_Send_Terminate_Ack(DV_DEVICE_ENTRY *dev_ptr, UINT16 protocol)
{
    LINK_LAYER          *link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
    NET_BUFFER          *buf_ptr;

    PrintInfo("   LCP_Send_Terminate_Ack\n");

    /* Create an outgoing request packet. Most of the fields will
       be pre-filled. */
    buf_ptr = LCP_New_Buffer(dev_ptr, LCP_TERMINATE_ACK, link_layer->term_id);
    if (buf_ptr == NU_NULL)
        /* This error is logged within LCP_New_Buffer, so just return. */
        return;

    /* Nothing else goes into the packet, so just send it. */
    LCP_Send(dev_ptr, buf_ptr, protocol);

    /* Reset the terminate id to -1 for the next ack. */
    link_layer->term_id = -1;

} /* LCP_Send_Terminate_Ack */



/*************************************************************************
*
* FUNCTION
*
*     LCP_Nak_Option
*
* DESCRIPTION
*
*     Append a NAKed option to the given buffer. If the buffer is
*     currently an ACK, then it is reset as a NAK before the option is
*     appended. However, if it is currently a REJECT packet, then the
*     option will not be added. Status is returned either NAK or REJECT
*     depending on the outcome.
*
* INPUTS
*
*     *frame
*     type
*     len
*     *data
*
* OUTPUTS
*
*     STATUS
*
*************************************************************************/
STATUS LCP_Nak_Option(LCP_FRAME *frame, UINT8 type, UINT8 len, UINT8 *data)
{
    /* If we are already building an ACK packet, reset it and make it
       a NAK packet. */
    if (frame->code == LCP_CONFIGURE_ACK)
    {
        /* Set the code to NAK and reset the length to the beginning. */
        frame->code = LCP_CONFIGURE_NAK;
        frame->len = LCP_HEADER_LEN;
    }

    /* If this is a NAK packet, then go ahead and append the option. */
    if (frame->code == LCP_CONFIGURE_NAK)
    {
        /* Add option to the NAK packet. */
        LCP_Append_Option(frame, type, len, data);
        return PPP_FLAG_NAK;
    }
    else
        /* If for some reason this is a REJECT packet, then the
           option is skipped. */
        return PPP_FLAG_REJECT;

} /* LCP_Nak_Option */



/*************************************************************************
* FUNCTION
*
*     LCP_Reject_Option
*
* DESCRIPTION
*
*     Append a rejected option to the given buffer. If the buffer is
*     currently an ACK or a NAK, then it is reset as a REJECT before
*     the option is appended.
*
* INPUTS
*
*     *frame
*     type
*     len
*     *data
*
* OUTPUTS
*
*     STATUS
*
*************************************************************************/
STATUS LCP_Reject_Option(LCP_FRAME *frame, UINT8 type, UINT8 len, UINT8 *data)
{
    /* If we are already building an ACK or NAK packet, then a REJECT
       will supersede it. Convert it to a REJECT packet. */
    if (frame->code == LCP_CONFIGURE_ACK || frame->code == LCP_CONFIGURE_NAK)
    {
        /* Reset the length and code because the packet is now a REJECT packet. */
        frame->code = LCP_CONFIGURE_REJECT;
        frame->len = LCP_HEADER_LEN;
    }

    /* Add option to the REJECT packet. */
    LCP_Append_Option(frame, type, len, data);

    return PPP_FLAG_REJECT;

} /* LCP_Reject_Option */

/*************************************************************************
* FUNCTION
*
*     LCP_New_Buffer
*
* DESCRIPTION
*
*     Generic routine to allocate a Net buffer for use in an outgoing
*     LCP or NCP packet. The caller supplies the code for the type of
*     packet this will be, and optionally an id. For request packets,
*     the id will be assigned a random number.
*
* INPUTS
*
*     *dev_ptr
*     code
*     id
*
* OUTPUTS
*
*     Pointer to a NET Buffer.
*
*************************************************************************/
NET_BUFFER *LCP_New_Buffer(DV_DEVICE_ENTRY *dev_ptr, UINT8 code, INT16 id)
{
    NET_BUFFER *buf_ptr;
    LCP_FRAME *lcp_frame;

    /* Allocate a buffer for the LCP packet. */
    buf_ptr = MEM_Buffer_Dequeue(&MEM_Buffer_Freelist);

    /* Make sure a buffer was available */
    if (buf_ptr != NU_NULL)
    {
        /* Set the data pointer */
        buf_ptr->data_ptr = buf_ptr->mem_parent_packet + dev_ptr->dev_hdrlen;
        lcp_frame = (LCP_FRAME*)buf_ptr->data_ptr;
        lcp_frame->code = code;
        lcp_frame->len = LCP_HEADER_LEN;

        /* Set up the configure packet. */
        if (id == -1)
        {
            /* For request packets, we create the identifier. */
            lcp_frame->id = LCP_Random_Number();
        }
        else
        {
            /* For ACKs, NAKs, and reject packets, the id must match the
               peer's incoming packet. */
            lcp_frame->id = (UINT8)id;
        }

        /* Set the dlist to free the buffer after sending. */
        buf_ptr->mem_dlist = &MEM_Buffer_Freelist;
    }

    return buf_ptr;
}

/*************************************************************************
* FUNCTION
*
*     LCP_Send
*
* DESCRIPTION
*
*     A generic routine to simplify the sending of an LCP or NCP packet.
*     The length field in the packet is swapped, mem_flags is set, and the
*     Net buffer data lengths are calculated before it is sent to the
*     output device.
*
* INPUTS
*
*     *dev_ptr
*     *buf_ptr
*     pkt_type
*
* OUTPUTS
*
*     STATUS
*
*************************************************************************/
STATUS LCP_Send(DV_DEVICE_ENTRY *dev_ptr, NET_BUFFER *buf_ptr, UINT16 pkt_type)
{
    STATUS status;
    LCP_FRAME *lcp_frame = (LCP_FRAME*)buf_ptr->data_ptr;

    /* Set the length of the data contained in the buffer. */
    buf_ptr->mem_total_data_len = buf_ptr->data_len = lcp_frame->len;
    lcp_frame->len = INTSWAP(lcp_frame->len);

    PPP_Add_Protocol(buf_ptr, pkt_type);
    PPP_PrintPkt((UINT8 *)buf_ptr->data_ptr, buf_ptr->data_len, "OUT:");

    /* Set the packet type for this buffer. */


    if (pkt_type == PPP_LINK_CONTROL_PROTOCOL)
        buf_ptr->mem_flags = NET_LCP;

    else if (pkt_type == PPP_CHAP_PROTOCOL)
        buf_ptr->mem_flags = NET_CHAP;

    else if (pkt_type == PPP_PAP_PROTOCOL)
        buf_ptr->mem_flags = NET_PAP;

#if (PPP_ENABLE_MPPE == NU_TRUE)
    else if (pkt_type == PPP_CCP_PROTOCOL)
        buf_ptr->mem_flags = NET_CCP;
#endif
#if (INCLUDE_IPV4 == NU_TRUE)
    else if (pkt_type == PPP_IP_CONTROL_PROTOCOL)
        buf_ptr->mem_flags = NET_IPCP;
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
    else if (pkt_type == PPP_IPV6_CONTROL_PROTOCOL)
        buf_ptr->mem_flags = NET_IPV6CP;
#endif

    /* Send the packet. */
    status = dev_ptr->dev_output(buf_ptr, dev_ptr, NU_NULL, NU_NULL);

    return status;
}



/*************************************************************************
* FUNCTION
*
*     LCP_Append_Option
*
* DESCRIPTION
*
*     Generic routine to append a new option to an LCP, CHAP, PAP, or
*     NCP packet. Note: The data_len field is the length of the data
*     alone and should not include the length of the option header.
*
* INPUTS
*
*     *lcp_frame
*     type
*     data_len
*     *data
*
* OUTPUTS
*
*     UINT8
*
*************************************************************************/
UINT8 LCP_Append_Option(LCP_FRAME *lcp_frame, UINT8 type, UINT8 data_len, UINT8 *data)
{
    UINT8   *data_ptr;
    UINT16  frame_len = (UINT16)(lcp_frame->len - LCP_HEADER_LEN);

    /* Move to the end of the packet data. */
    data_ptr = &lcp_frame->data[frame_len];

    /* Add option type and length. */
    data_ptr[0] = type;
    data_ptr[1] = (UINT8)(data_len + LCP_OPTION_HDRLEN);

    /* Add option data to the packet. */
    memcpy(&data_ptr[2], (CHAR*)data, data_len);

    /* Add the option length to the total frame length. */
    lcp_frame->len = lcp_frame->len + data_ptr[1];

    /* Return the total option length. */
    return data_ptr[1];
}
