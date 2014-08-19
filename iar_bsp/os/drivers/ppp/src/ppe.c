/*************************************************************************
*
*               Copyright 2002 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************
****************************************************************************
* FILE NAME                                         
*
*       ppe.c                                     
*
* COMPONENT
*
*       PPE - PPPoE Driver Common Routines
*
* DESCRIPTION
*
*       These routines are necessary for all applications that require
*       a PPPoE Host, Access Concentrator, or Relay Agent.
*
* DATA STRUCTURES
*
*       PPE_DevList         - A semaphore for mutual exclusion of the
*                              virtual device structures between the
*                              timer task and application tasks.
*       PPE_Next_Session_Id - A global counter for the session ids.
*
* FUNCTIONS
*
*       PPE_Create_Session
*       PPE_Find_Virtual_Device
*       PPE_Get_Unique_Id
*       PPE_Initialize
*       PPE_Input
*       PPE_Output
*       PPE_Process_PADT
*       PPE_Reserve_Device
*       PPE_Reset_State
*       PPE_Return_Device
*       PPE_Send
*       PPE_Session_Terminate
*       PPE_Transmit
*
* DEPENDENCIES
*
*       NERRS.H
*       TCP_ERRS.H
*
***************************************************************************/
#include "networking/nu_net.h"
#include "networking/nerrs.h"
#include "networking/tcp_errs.h"
#include "networking/ncl.h"
#include "drivers/nu_drivers.h"
#include "drivers/ppe_defs.h"
#include "drivers/ppe_extr.h"

extern STATUS   PPP_Terminate_Link(DV_DEVICE_ENTRY*);
extern VOID     (*ppe_process_packet)(UINT16);

/* These global variables are counters used by PPE_Get_Unique_Id()
   to get a new unique number for the next device or session id. */
UINT16          PPE_Next_Session_Id = 0;
UINT16          PPE_Next_VDevice_Id = 0;

#if (NET_VERSION_COMP > NET_4_5)
TQ_EVENT        PPE_Event;
#endif


/************************************************************************
* FUNCTION
*
*       PPE_Find_Virtual_Device
*
* DESCRIPTION
*
*       Given a vdevice identifier, or session id, this function will
*       search the device table for a PPPoE device that has a 
*       matching entry, and will return a pointer to that device.
*
* INPUTS
*
*       sid                 - A 16 bit number used as an identifier of
*                              a virtual device.
*
* OUTPUTS
*
*       vdevice             - A pointer to the matching PPPoE virtual
*                              device, or NU_NULL if not found.
*
************************************************************************/
DV_DEVICE_ENTRY *PPE_Find_Virtual_Device(UINT16 sid, UINT16 mode)
{
    DV_DEVICE_ENTRY         *vdevice;
    LINK_LAYER              *link_layer;
    PPE_LAYER               *ppe_layer;
    
    /* Search the device list for a device with matching session id. */
    vdevice = DEV_Table.dv_head;
    while (vdevice)
    {
        /* Only check virtual PPPoE devices. Skip other types. */
        if (vdevice->dev_type == DVT_PPP)
        {
            link_layer = (LINK_LAYER*)vdevice->dev_ppp_layer;
            if (link_layer->hwi_itype & PPP_ITYPE_PPPOE)
            {
                ppe_layer = (PPE_LAYER*)link_layer->link;

                /* Check the id based on which type of id we're looking for. */
                if (mode == PPE_VDEVICE_ID)
                {
                    /* See if this is the vdevice id we're looking for. */
                    if (ppe_layer->ppe_vdevice_id == sid &&
                        ppe_layer->ppe_session_id == 0)
                    {
                        /* This device matches the session. Return it. */
                        break;
                    }
                }
                else if (mode == PPE_SESSION_ID)
                {
                    /* Make sure this is the session id we're looking for. */
                    if (ppe_layer->ppe_session_id == sid && 
                        ppe_layer->ppe_vdevice_id != PPE_EMPTY_SESSION_ID)
                    {
                        /* The PPE_ACTIVE flag should be set for all sessions. */
                        if (ppe_layer->ppe_status & (PPE_ACTIVE | PPE_SESSION))
                        {
                            /* This device matches the session. Return it. */
                            break;
                        }
                    }
                }
            }
        }

        /* No match, or it's not a PPPoE device. Try the next device. */
        vdevice = vdevice->dev_next;
    }

    return vdevice;

} /* PPE_Find_Virtual_Device */



/************************************************************************
* FUNCTION
*
*        PPE_Initialize
*
* DESCRIPTION
*
*        Initialize the device as a PPPoE virtual device.
*
* INPUTS
*
*       vdevice             - A pointer to the PPPoE virtual device.
*
* OUTPUTS
*
*       NU_SUCCESS          - Initialization completed with no errors.
*       NU_PPE_ERROR        - An error specific to PPPoE occured.
*       -1                  - A general system error occurred.
*
************************************************************************/
STATUS PPE_Initialize(DV_DEVICE_ENTRY *vdevice)
{
    PPE_LAYER HUGE      *ppe_layer;
    LINK_LAYER HUGE     *ppp_layer_ptr;
    CHAR                linkname[10] = "PPE";
    CHAR                *nameptr = &linkname[3];
    DV_DEVICE_ENTRY     *hdevice;
    STATUS              status;
    VOID                *pointer;
    static INT          PPE_Init_Once = NU_FALSE;

#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    DV_DEV_LABEL        *eth_label;
    DV_DEV_ID           eth_device_id;
    INT                 dev_count = 4; /* There is currently no macro to
                                        * determine the number of ethernet
                                        * devices present in the system,
                                        * hence using this hard-coded value
                                        */
#endif

#if (NET_VERSION_COMP <= NET_4_5)
    NU_LCP_OPTIONS      lcpopts;
#endif

#if (NET_VERSION_COMP > NET_4_5)
    /* Set the PPP Memory Pool to the same pool as Net. */
    if (PPP_Memory == NU_NULL)
        PPP_Memory = MEM_Cached;
#endif

    /* Append the device index number to the linkname, differentiating
       some of the named Nucleus structures from other links. */
    NU_ITOA((INT)vdevice->dev_index, nameptr, 10);

    /* Store the address of the hardware device structure for the ppp layer. */
    hdevice = DEV_Get_Dev_By_Name(CFG_NU_OS_DRVR_PPP_ETH_DEV_NAME);
    if (hdevice == NU_NULL)
        return NU_PPE_ERROR;

    /* Allocate memory for the link layer structures. The PPP structure will
       be pointed to by the device structure for this device, and the PPE
       structure will be pointed to by the PPP structure. */
    status = NU_Allocate_Memory (PPP_Memory, &pointer, sizeof(LINK_LAYER) +
                                sizeof(PPE_LAYER), NU_NO_SUSPEND);
    if (status != NU_SUCCESS)
    {
        NERRS_Log_Error (TCP_FATAL, __FILE__, __LINE__);
        return(-1);
    }

    /* Mark the boundaries of the allocated memory. */
    ppp_layer_ptr = (LINK_LAYER*)pointer;
    ppe_layer = (PPE_LAYER HUGE *)(ppp_layer_ptr + 1);

    /* Zero out the ppp layer information. */
    UTL_Zero (ppp_layer_ptr, sizeof(LINK_LAYER) + sizeof(PPE_LAYER));

    /* Connect the ppp_layer structure to the device structure. */
    vdevice->dev_ppp_layer = ppp_layer_ptr;

    /* Fill in the device */
    vdevice->dev_output = PPP_Output;  /* Called by stack to send packet */
    vdevice->dev_start = PPE_Transmit;   /* Called by PPP to send packet */

    /* Set the device's MTU to PPE_MTU only if it is a smaller value. */
    if (vdevice->dev_mtu == 0 || vdevice->dev_mtu > PPE_MTU)
        vdevice->dev_mtu = PPE_MTU;

    /* Each link protocol adds a header size to a total for Net to use. 
       Since this device is on top of the ethernet device, we'll add the
       hardware device's header length as well. */
    vdevice->dev_hdrlen += (UINT8)(hdevice->dev_hdrlen + PPE_HEADER_SIZE);
    
    /* Store the address of the virtual device structure for the ppp layer. */
    ppp_layer_ptr->hwi_dev_ptr = vdevice;

    /* Assign the type of PPP hardware interface */
    ppp_layer_ptr->hwi_itype = PPP_ITYPE_PPPOE;
    
    /* Store the address of the hardware device structure in the ppe layer. */
    ppe_layer->ppe_hdevice = hdevice;

    /* Init the connection status. */
    ppp_layer_ptr->connection_status = NU_PPP_DISCONNECTED;

    /* Connect the ppe_layer structure to the ppp_layer structure. */
    ppp_layer_ptr->link = (VOID*)ppe_layer;

    /* Pointers to hardware interface functions. */
    ppp_layer_ptr->hwi_connect = PPEH_Discovery;
    ppp_layer_ptr->hwi_passive = PPEA_Wait_For_Client;
    ppp_layer_ptr->hwi_disconnect = PPE_Session_Terminate;

    /* Create the event group for the connection/timeout events. */
    status = NU_Create_Event_Group(&ppe_layer->ppe_event, linkname);
    if (status != NU_SUCCESS)
    {
        NERRS_Log_Error(TCP_FATAL, __FILE__, __LINE__);
        return(NU_PPE_ERROR);
    }

    /* Only execute these tasks once. */
    if (PPE_Init_Once == NU_FALSE)
    {
        /* Assign the Rx packet handler to ppe_process_packet, which
           is declared in Net.c. */
        ppe_process_packet = PPE_Input;

#if (NET_VERSION_COMP > NET_4_5)
        /* Register the PPPoE event handler with the dispatcher. */
        status = EQ_Register_Event(PPEH_Timer_Expire, &PPE_Event);
        if (status != NU_SUCCESS)
        {
            NERRS_Log_Error (TCP_FATAL, __FILE__, __LINE__);
            return(NU_PPE_ERROR);
        }
#endif

        PPE_Init_Once = NU_TRUE;
    }
    
#ifdef CFG_NU_OS_SVCS_PWR_ENABLE
    /* Obtain the Label of the physical device to be used. */
    eth_label = (DV_DEV_LABEL *)vdevice->dev_driver_options;

    /* Get the PPPoE device ID */
    status = DVC_Dev_ID_Get(eth_label, 1, &eth_device_id, &dev_count);

    if ((status == NU_SUCCESS) && (dev_count > 0))
    {
        /* Make sure we place a min state request for the Ethernet
         * driver to be on, or PPP will not function over Ethernet.
         */
        status = NU_PM_Min_Power_State_Request(eth_device_id, POWER_ON_STATE,
                                               &(ppp_layer_ptr->ppp_pm_handle));

        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log("PPE_Initialize failed to request min Power state\n",
                           NERR_FATAL, __FILE__, __LINE__);
        }
    }
    else
    {
        NLOG_Error_Log("PPE_Initialize failed to retrieve PPPoE device ID.\n",
                       NERR_FATAL, __FILE__, __LINE__);
    }
#endif

    /* Initialize upper PPP layer for this device. */
    status = PPP_Initialize(vdevice);
    if (status == NU_SUCCESS)
    {
#if (NET_VERSION_COMP > NET_4_5)
        MIB2_ifType_Seti(vdevice->dev_index, 23);
        MIB2_ifSpecific_Set(vdevice->dev_index, (UINT8*)"ppp");
        MIB2_ifDescr_Seti(vdevice->dev_index, "Nucleus PPPoE interface v1.2");
        MIB2_ifMtu_Set(vdevice->dev_index, vdevice->dev_mtu);
        MIB2_ifSpeed_Seti(vdevice->dev_index, vdevice->dev_baud_rate);
#else
        /* Set LCP and NCP link options for this link. */
        lcpopts.magic_number               = PPE_DEFAULT_MAGIC_NUMBER;
        lcpopts.accm                       = 0;
        lcpopts.max_rx_unit                = (UINT16)vdevice->dev_mtu;
        lcpopts.authentication_protocol    = LCP_DEFAULT_AUTH_PROTOCOL;
        lcpopts.protocol_field_compression = NU_FALSE;
        lcpopts.address_field_compression  = NU_FALSE;
        lcpopts.use_accm                   = NU_FALSE;
        lcpopts.use_max_rx_unit            = PPE_USE_MRU;
        lcpopts.Initial_max_rx_unit        = (UINT16)vdevice->dev_mtu;
        lcpopts.Initial_magic_number       = PPE_DEFAULT_MAGIC_NUMBER;

        status = PPP_Set_Link_Options (vdevice->dev_net_if_name, &lcpopts, NU_NULL);

        /* Get the PPP event handler function pointer and save it. */
        ppe_layer->ppe_event_handler = vdevice->dev_event;
#endif
    }
    else
    {
#if (NU_DEBUG_PPE == NU_TRUE)        
        PPE_Printf("PPP_Initialize failed...\r\n");
#endif
        NERRS_Log_Error (TCP_FATAL, __FILE__, __LINE__);
        return status;
    }

    /* Now place the device into a pool of unused devices, waiting
       to be allocated by an application process. */
    PPE_Return_Device(vdevice);

    return status;

} /* PPE_Initialize */



/************************************************************************
* FUNCTION
*
*       PPE_Input
*
* DESCRIPTION
*
*       This is the main input routine for PPPoE packets. It is
*       checked for validity and then passed to the appropriate
*       handler. It handles both discovery and session packets.
*
* INPUTS
*
*       eth_type            - The ethernet packet type (2 types).
*
* OUTPUTS
*
*       None
*
************************************************************************/
VOID PPE_Input(UINT16 eth_type)
{
    NET_BUFFER          *buffer = MEM_Buffer_List.head;
    LINK_LAYER          *link_layer;
    CHAR                *peer_addr;
    DV_DEVICE_ENTRY     *vdevice, *hdevice;
    PPE_FRAME           ppe;

    /* Get a handle on all the relevant data. */
    ppe.ppe_vertype = GET8(buffer->data_ptr, 0);
    ppe.ppe_dcode = GET8(buffer->data_ptr, 1);
    ppe.ppe_sid = GET16(buffer->data_ptr, 2);
    ppe.ppe_length = GET16(buffer->data_ptr, 4);
    memcpy(ppe.ppe_macaddr, buffer->data_ptr - 8, 6);

    hdevice = MEM_Buffer_List.head->mem_buf_device;

    /* Verify that we can process this frame by checking the PPPoE
       version and type fields. If the frame has a different version,
       then we can't process it. */
    if(ppe.ppe_vertype != PPE_VERTYPE)
    {
        /* Drop the packet by placing it back on the buffer_freelist. */
        MEM_Buffer_Chain_Free (&MEM_Buffer_List, &MEM_Buffer_Freelist);
    }

    /* If this is a session packet, then check the session id and send the 
       frame up. */
    else if (eth_type == PPPOES)
    {
        /* Validate the session packet */
        if ((ppe.ppe_sid == 0x0000) || (ppe.ppe_sid == 0xffff))
        {
            /* Invalid Packet... drop it! */
#if (NU_DEBUG_PPE == NU_TRUE)        
            PPE_Printf("PPE_Input() - Received invalid session packet.\r\n");
#endif         
            /* Drop the packet by placing it back on the buffer_freelist. */
            MEM_Buffer_Chain_Free (&MEM_Buffer_List, &MEM_Buffer_Freelist);
        }
        else
        {
            /* Get the virtual device that is associated with this hardware
               device and session id. */
            vdevice = PPE_Find_Virtual_Device(ppe.ppe_sid, PPE_SESSION_ID);
            if (vdevice)
            {
                link_layer = (LINK_LAYER*)vdevice->dev_ppp_layer;
                peer_addr = (CHAR*)((PPE_LAYER*)link_layer->link)
                            ->ppe_node.ppe_mac_addr;

                /* Security: Make sure the source address is valid. */
                if (memcmp(ppe.ppe_macaddr, peer_addr, DADDLEN) == 0)
                {
                    /* Switch to the virtual device for this buffer. */
                    MEM_Buffer_List.head->mem_buf_device = vdevice;

                    /* Remove the PPPoE header and deliver the packet to PPP */
                    PPEU_Adjust_Header(buffer, PPE_HEADER_SIZE);

#if (INCLUDE_PPPOE_TCP_MSS_FIX == NU_TRUE)
                    /* Check the packet type and update the TCP MSS option
                       if one is present. */
                    if (buffer->data_ptr[0] == 0)
                        PPEU_Fix_TCP_MSS(buffer);
#endif

#if (NU_DEBUG_PPE == NU_TRUE)
                    /* Truncate a packet to the PPPoE max length, if it total
                       data length is too large, or is incorrect. */
                    if (MEM_Buffer_List.head->mem_total_data_len > ppe.ppe_length)
                        MEM_Buffer_List.head->mem_total_data_len = ppe.ppe_length;
#endif
                    /* Deliver to the PPP layer. */
                    PPP_Input();
                }
                else
                {
                    /* Drop the packet by placing it back on the buffer_freelist. */
                    MEM_Buffer_Chain_Free (&MEM_Buffer_List, &MEM_Buffer_Freelist);
                }
            }
            else
            {
                /* No device matched that session id. Just drop the packet. */
#if (NU_DEBUG_PPE == NU_TRUE)        
                PPE_Printf("PPE_Input() - No device for this session id.\r\n");
#endif                
                /* Drop the packet by placing it back on the buffer_freelist. */
                MEM_Buffer_Chain_Free (&MEM_Buffer_List, &MEM_Buffer_Freelist);
            }
        }
    }
    else
    {
        /* If this is a discovery packet, get the PADx type. */
        switch(ppe.ppe_dcode)
        {
#if (INCLUDE_PPE_AC == NU_TRUE)
            case PPE_PADI:
                
#if (INCLUDE_PPE_HOST == NU_TRUE)
                /* If we just sent a broadcast PADI packet, we don't want our
                   AC side to process it. */
                if (memcmp(ppe.ppe_macaddr, hdevice->dev_mac_addr, DADDLEN) != 0)
                {
#endif
#if (NU_DEBUG_PPE == NU_TRUE)        
                    PPE_Printf("PPE_Input() - Received a PADI.\r\n");
#endif
                    PPEA_Process_PADI(&ppe);
#if (INCLUDE_PPE_HOST == NU_TRUE)
                }
#endif
                
                break;

            case PPE_PADR:
                
#if (NU_DEBUG_PPE == NU_TRUE)        
                PPE_Printf("PPE_Input() - Received a PADR.\r\n");
#endif
                PPEA_Process_PADR(&ppe);

                break;
#endif


                
#if (INCLUDE_PPE_HOST == NU_TRUE)
            case PPE_PADO:

#if (NU_DEBUG_PPE == NU_TRUE)        
                PPE_Printf("PPE_Input() - Received a PADO.\r\n");
#endif
                PPEH_Process_PADO(&ppe);

                break;

            case PPE_PADS:

#if (NU_DEBUG_PPE == NU_TRUE)        
                PPE_Printf("PPE_Input() - Received a PADS.\r\n");
#endif
                PPEH_Process_PADS(&ppe);

                break;
#endif

            case PPE_PADT:

#if (NU_DEBUG_PPE == NU_TRUE)        
                PPE_Printf("PPE_Input() - Received a PADT.\r\n");
#endif
                PPE_Process_PADT(&ppe);

                break;

            default:;
                /* Drop the packet */
#if (NU_DEBUG_PPE == NU_TRUE)        
                PPE_Printf("PPE_Input() - Received invalid discovery packet.\r\n");
#endif
        }

        /* Drop the packet by placing it back on the buffer_freelist. */
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);
    }

} /* PPE_Input */



/************************************************************************
* FUNCTION
*
*       PPE_Output
*
* DESCRIPTION
*
*       Low-level output routine, which adds the PPPoE header to the
*       buffer based on type information from the calling function. It
*       will then send the packet through the ethernet device.
*
*
* INPUTS
*
*       type                - The PPPoE packet type.
*       *macaddr            - Pointer to ethernet hardware address.
*       sid                 - A 16 bit number used as an identifier of
*                              a virtual device.
*       *hdevice            - Pointer to the ethernet device.
*       *buffer             - Pointer to the Net buffer to send.
*
* OUTPUTS
*
*       NU_SUCCESS          - The packet is successfully transmitted.
*
************************************************************************/
STATUS PPE_Output(UINT8 type, UINT8 *macaddr, UINT16 sid, 
                  DV_DEVICE_ENTRY *hdevice, NET_BUFFER* buffer)
{
    PPE_ADDR    dest;
    STATUS      status;
    INT32       dv_up;

    /* Set up the destination address information for Net_Ether_Send(). */
    dest.sck.sck_family = SK_FAM_UNSPEC;
    memcpy(&dest.mac.ar_mac.ar_mac_ether, macaddr, DADDLEN);

    /* Set the ethernet type field based on type argument. */
    if (type == 0x00)
        dest.mac.ar_mac.ar_mac_ether.type = PPPOES;
    else
        dest.mac.ar_mac.ar_mac_ether.type = PPPOED;

    /* Add the PPPoE header to the packet. */
    PPEU_Add_Header(buffer, type, sid);

    /* Grab the DV_UP flag from the ethernet device. If the ethernet
       device is down, that may simply mean that it has been turned
       off. We will return it to that state when we're done. */
    dv_up = hdevice->dev_flags & DV_UP;

    /* Enable the ethernet device so this packet can be sent. */
    hdevice->dev_flags |= DV_UP;

    /* Send it to the driver */
    status = hdevice->dev_output(buffer, hdevice, (SCK_SOCKADDR_IP*)&dest, 
                                NU_NULL);
#if (NU_DEBUG_PPE == NU_TRUE)        
    if (status != NU_SUCCESS)
    {
        PPE_Printf("PPE_Output() - Failed to send packet through ethernet device\r\n");
    }
#endif

    /* Return the ethernet device to its original DV_UP state. */
    hdevice->dev_flags &= ~DV_UP;
    hdevice->dev_flags |= dv_up;

    return status;

} /* PPE_Output */



/************************************************************************
* FUNCTION
*
*       PPE_Process_PADT
*
* DESCRIPTION
*
*       Find the virtual device that needs to be terminated, verify
*       that the session is valid, and call the terminate routine.
*
* INPUTS
*
*       *ppe                - A pointer to the PPPoE header information.
*
* OUTPUTS
*
*       NU_SUCCESS          - The link was terminated successfully.
*       NU_PPE_INVALID_PADx - The PADT is invalid.
*
************************************************************************/
STATUS PPE_Process_PADT(PPE_FRAME *ppe)
{
    LINK_LAYER      *link_layer;
    PPE_LAYER       *ppe_layer;
    DV_DEVICE_ENTRY *vdevice;
    STATUS          status;

    /* Verify that the PADT matches a valid session. */
    if (ppe->ppe_sid != 0)
    {
        /* Find the device matching the session id. */
        vdevice = PPE_Find_Virtual_Device(ppe->ppe_sid, PPE_SESSION_ID);
        if (vdevice)
        {
            link_layer = (LINK_LAYER*)vdevice->dev_ppp_layer;
            ppe_layer = (PPE_LAYER*)link_layer->link;

            /* The packet now belongs to the virtual device. */
            MEM_Buffer_List.head->mem_buf_device = vdevice;

            /* Security issue. Make sure the peer mac address is correct 
               as well. */
            if (memcmp(ppe_layer->ppe_node.ppe_mac_addr, ppe->ppe_macaddr, DADDLEN) == 0)
            {
                /* Terminate the session. This will reset the DV_UP flag
                   in the device structure, signifying that the link is
                   no longer usable by PPP. */
                vdevice->dev_flags &= ~DV_UP;
                
#if (NET_VERSION_COMP > NET_4_5)
                /* Reset the state of the modem. */
                link_layer->hwi.state = INITIAL;

                /* Signal an open PPP link that the medium is no longer available. */
                if (link_layer->lcp.state == OPENED)
                {
                    link_layer->lcp.state = STOPPING;

                    /* Force PPP to remove the link, since the device is down. */
                    EQ_Put_Event(PPP_Event, (UNSIGNED)vdevice, LCP_CLOSE_REQUEST);
                }

                /* Remove all session information, essentially making the device
                   available for a new discovery. */
                PPE_Return_Device(vdevice);

                status = NU_SUCCESS;
#else
                link_layer->lcp.state = STOPPED;
                link_layer->lcp.echo_counter = INITIAL;
                
                /* Force PPP to remove the link, since the device is down. */
                status = PPP_Terminate_Link(vdevice);
#endif
            }
            else
                status = NU_PPE_INVALID_PADx;
        }
        else
            status = NU_PPE_INVALID_PADx;
    }
    else
        status = NU_PPE_INVALID_PADx;

    return status;

} /* PPE_Process_PADT */



/************************************************************************
* FUNCTION
*
*       PPE_Transmit
*
* DESCRIPTION
*
*       This is the transmit entry function from PPP and is only used
*       for existing sessions. On entry, the buffer is removed from the
*       virtual device's transmit queue, and is sent through the
*       associated hardware device via PPE_Output().
*
* INPUTS
*
*       vdevice             - Pointer to the PPPoE virtual device.
*       *buffer             - Pointer to the Net buffer to send.
*
* OUTPUTS
*
*       NU_SUCCESS          - The packet was transmitted successfully.
*
************************************************************************/
STATUS PPE_Transmit(DV_DEVICE_ENTRY *vdevice, NET_BUFFER *buffer)
{
    STATUS              status;
    DV_DEVICE_ENTRY    *hdevice;
    LINK_LAYER         *link_layer;
    PPE_LAYER          *ppe_layer;
    UINT16              sid;
#if (NET_VERSION_COMP <= NET_4_5)
    UINT16              pkt_type;
#endif

#if (NET_VERSION_COMP <= NET_4_5)
    /* Get the type of packet we are encapsulating. */
    if (buffer->mem_flags & NET_IP)
        pkt_type = PPP_IP_PROTOCOL;
    else
        if (buffer->mem_flags & NET_LCP)
            pkt_type = PPP_LINK_CONTROL_PROTOCOL;
        else
            if (buffer->mem_flags & NET_IPCP)
                pkt_type = PPP_IP_CONTROL_PROTOCOL;
            else
                if (buffer->mem_flags & NET_PAP)
                    pkt_type = PPP_PAP_PROTOCOL;
                else
                    if (buffer->mem_flags & NET_CHAP)
                        pkt_type = PPP_CHAP_PROTOCOL;
                    else
                        return NU_PPP_INVALID_PROTOCOL;
                    
    /* Since we are not in the opened state we must be sending
       configure pkts, none of these are compressed. */
    buffer->data_ptr           -= PPP_HEADER_SIZE;
    buffer->data_len           += PPP_HEADER_SIZE;
    buffer->mem_total_data_len += PPP_HEADER_SIZE;
    
    /* Add the packet type in MSB LSB order. */
    PUT16 (buffer->data_ptr, 0, pkt_type);
#endif

    link_layer = (LINK_LAYER*)vdevice->dev_ppp_layer;
    ppe_layer = (PPE_LAYER*)link_layer->link;
    hdevice = ppe_layer->ppe_hdevice;
    sid = ppe_layer->ppe_session_id;

    /* Send the ppp session packet. Make sure that the virtual device
       is still connected, i.e. the session flag is set. */
    if (ppe_layer->ppe_status & (PPE_ACTIVE | PPE_SESSION))
    {
#if (INCLUDE_PPPOE_TCP_MSS_FIX == NU_TRUE)
#if (NET_VERSION_COMP > NET_4_5)
    if (buffer->mem_flags & (NET_IP | NET_IP6))
#else
    if (buffer->mem_flags & NET_IP)
#endif
        /* Check the packet type and update the TCP MSS option
           if one is present. */
        PPEU_Fix_TCP_MSS(buffer);
#endif
        /* Remove the buffer from the transmit queue. It should be
           the same as the parameter buffer. */
        MEM_Buffer_Dequeue(&vdevice->dev_transq);
        vdevice->dev_transq_length--;

        /* Call the ethernet output device. */
        status = PPE_Output(0, ppe_layer->ppe_node.ppe_mac_addr, sid, hdevice, 
                            buffer);
#if (NU_DEBUG_PPE == NU_TRUE)        
        if (status != NU_SUCCESS)
        {
            PPE_Printf("PPE_Transmit() - Failed to send through hardware device.\n");
        }
#endif
    }
    else
        /* The session must have been terminated. */
        status = NU_PPE_ERROR;

    return status;

} /* PPE_Transmit */



/************************************************************************
* FUNCTION
*
*       PPE_Send
*
* DESCRIPTION
*
*       A high level transmit routine that sends only discovery
*       packets based on their type. Appropriate information is
*       added as necessary for sending through the hardware device
*       via the PPE_Output() routine.
*
* INPUTS
*
*       vdevice             - A pointer to the PPPoE virtual device.
*       type                - PPPoE packet type.
*       *buffer             - Pointer to the Net buffer to send.
*
* OUTPUTS
*
*       NU_SUCCESS          - The packet was transmitted successfully.
*
************************************************************************/
STATUS PPE_Send(DV_DEVICE_ENTRY *vdevice, UINT8 type, NET_BUFFER *buffer)
{
    STATUS              status;
    DV_DEVICE_ENTRY     *hdevice;
    LINK_LAYER          *link_layer;
    PPE_LAYER           *ppe_layer;
    UINT16              sid;

    link_layer = (LINK_LAYER*)vdevice->dev_ppp_layer;
    ppe_layer = (PPE_LAYER*)link_layer->link;
    hdevice = ppe_layer->ppe_hdevice;

    /* Assign the session id only if this is a PADS or PADT packet.
       Other discovery packets must have a session id of 0. */
    sid = ppe_layer->ppe_session_id;

    /* Send through the hardware device. */
    status = PPE_Output(type, ppe_layer->ppe_node.ppe_mac_addr, sid,
                            hdevice, buffer);
#if (NU_DEBUG_PPE == NU_TRUE)        
    if (status != NU_SUCCESS)
    {
        PPE_Printf("PPE_Send() - Failed to send through hardware device.\r\n");
    }
#endif
    
    return status;

} /* PPE_Send */



/************************************************************************
* FUNCTION
*
*       PPE_Session_Terminate
*
* DESCRIPTION
*
*       Terminate the PPP/PPPoE link. If this is called as a result
*       of receiving a PADT packet from the peer, then the PPP link
*       will be terminated before removing device information. If PPP
*       terminates the link, a PADT packet will be sent to the peer.
*
* INPUTS
*
*       *link_layer         - Pointer to the LINK_LAYER structure.
*       flags               - Values specify how this link is terminated.
*
* OUTPUTS
*
*       NU_SUCCESS          - The link has terminated successfully.
*
************************************************************************/
STATUS PPE_Session_Terminate(LINK_LAYER *link_layer, UINT8 flags)
{
    STATUS              status = NU_SUCCESS;
    DV_DEVICE_ENTRY     *vdevice;
    NET_BUFFER          *buffer;
    PPE_LAYER           *ppe_layer;

    UNUSED_PARAMETER(flags);
    vdevice = link_layer->hwi_dev_ptr;
    ppe_layer = (PPE_LAYER*)link_layer->link;

#if (NET_VERSION_COMP > NET_4_5)
    /* No need to do anything if the modem is already disconnected. */
    if (link_layer->hwi.state == INITIAL)
        return NU_SUCCESS;

    /* Reset the state of the modem. */
    link_layer->hwi.state = INITIAL;
#endif

    /* See if this device has already been terminated. */
    if (ppe_layer->ppe_vdevice_id == PPE_EMPTY_SESSION_ID ||
        ppe_layer->ppe_session_id == PPE_INIT_SESSION_ID)
    {
        return NU_SUCCESS;
    }

#if (NET_VERSION_COMP > NET_4_5)
    /* Set the status of the PPP connection attempt. */
    link_layer->connection_status = NU_PPP_HANGING_UP;
#endif

    /* Send a PADT packet to the peer to notify it that we have
       terminated the link. */
    buffer = PPEU_New_Buffer(vdevice);
    if (buffer)
        PPE_Send(vdevice, PPE_PADT, buffer);

    /* Remove all session information, essentially making the device
       available for a new discovery. */
    PPE_Return_Device(vdevice);

#if (NET_VERSION_COMP <= NET_4_5)
    /* Reset the LCP state. */
    link_layer->lcp.state   = link_layer->lcp.echo_counter;
#endif

    return status;

} /* PPE_Session_Terminate */



/************************************************************************
* FUNCTION
*
*       PPE_Reset_State
*
* DESCRIPTION
*
*       Reset the information for the specified device based on
*       whether it is acting as an AC or a host. If acting as a host
*       and a discovery packet has been sent, then the state will be
*       preserved so as not to get multiple discovery stages confused
*       between devices. Otherwise, the state will be returned to an
*       initial state.
*
* INPUTS
*
*       *vdevice            - A pointer to the PPPoE virtual device.
*       mode                - Either AC mode or Host mode.
*
* OUTPUTS
*
*       None.
*
************************************************************************/
VOID PPE_Reset_State(DV_DEVICE_ENTRY* vdevice, UINT32 mode)
{
    LINK_LAYER      *link_layer;
    PPE_LAYER       *ppe_layer;

    link_layer = (LINK_LAYER*)vdevice->dev_ppp_layer;
    ppe_layer = (PPE_LAYER*)link_layer->link;

    /* Reset the link information specific to the acting mode. */
    if (mode == PPE_AC_MODE)
    {
        /* Reset the status to null. The appropriate settings will
           be added below. */
        ppe_layer->ppe_status = 0;

        /* Reset the hardware address to broadcast. */
        memcpy(ppe_layer->ppe_node.ppe_mac_addr, NET_Ether_Broadaddr, DADDLEN);

        /* Initialize the vdevice id. */
        ppe_layer->ppe_vdevice_id = PPE_INIT_SESSION_ID;

        /* Set the AC device name into the ppe_layer structure. */
        ppe_layer->ppe_lname = (UINT8*)PPE_AC_NAME;
    }
    else
    {
        /* If the host is in the middle of discovery, save the discovery
           state to make sure the order is preserved. */
        if (ppe_layer->ppe_status & PPE_DISCOVERY_MASK)
        {
            /* Save the current state of discovery. */
            ppe_layer->ppe_status &= PPE_DISCOVERY_MASK;
        }

        /* Otherwise, clean up the ppe_layer information completely
           for new discovery. */
        else
        {
            /* Reset the status to null. The appropriate settings will
               be added below. */
            ppe_layer->ppe_status = 0;

            /* Clean up any unsent PADx buffers. */
            PPEH_Clean_RBuf(ppe_layer);
            
            /* Reset the hardware address to ensure that nothing will be sent
            or received from this device. */
            memcpy(ppe_layer->ppe_node.ppe_mac_addr, NET_Ether_Broadaddr, DADDLEN);

            /* Get another host device identifier for the next discovery. */
            ppe_layer->ppe_vdevice_id = PPE_Get_Unique_Id(PPE_VDEVICE_ID);
        }

        /* Set the host device name into the ppe_layer structure. */
        ppe_layer->ppe_lname = (UINT8*)PPE_HOST_NAME;

#if (NET_VERSION_COMP <= NET_4_5)
        /* Set the device event handler to PPEH_Timer_Expire. */
        vdevice->dev_event = PPEH_Timer_Expire;
#endif
    }
    
    /* Initialize the session id. */
    ppe_layer->ppe_session_id = PPE_INIT_SESSION_ID;
    
    /* Make the device active and assign it the given acting mode. */
    ppe_layer->ppe_status |= (mode | PPE_ACTIVE);

#if (NET_VERSION_COMP > NET_4_5)
    /* Reset the state of the modem. */
    link_layer->hwi.state = INITIAL;
#endif

} /* PPE_Reset_State */



/************************************************************************
* FUNCTION
*
*       PPE_Reserve_Device
*
* DESCRIPTION
*
*       Find a virtual PPPoE device that is not being used, and
*       initialize the device information to prepare it for use by
*       this task.
*
* INPUTS
*
*       options             - Reserve device as an AC or Host device.
*
* OUTPUTS
*
*       *vdevice            - A pointer to the PPPoE virtual device.
*
************************************************************************/
DV_DEVICE_ENTRY *PPE_Reserve_Device(UINT32 options)
{
    DV_DEVICE_ENTRY        *vdevice;

    /* Find an available device. */
    vdevice = PPE_Find_Virtual_Device(PPE_EMPTY_SESSION_ID, PPE_VDEVICE_ID);
    if (vdevice)
    {
        /* Reset the link information to its initial state. */
        PPE_Reset_State(vdevice, options);
    }

    return vdevice;

} /* PPE_Reserve_Device */



/************************************************************************
* FUNCTION
*
*       PPE_Return_Device
*
* DESCRIPTION
*
*       Remove all information pertaining to the previous PPPoE link
*       and return the device to the pool of unused PPPoE devices, 
*       ready to be reallocated by another task.
*
* INPUTS
*
*       *vdevice            - A pointer to the PPPoE virtual device.
*
* OUTPUTS
*
*       None.
*
************************************************************************/
VOID PPE_Return_Device(DV_DEVICE_ENTRY *vdevice)
{
    LINK_LAYER             *link_layer;
    PPE_LAYER              *ppe_layer;

    link_layer = (LINK_LAYER*)vdevice->dev_ppp_layer;
    ppe_layer = (PPE_LAYER*)link_layer->link;
    
    /* First things first. Mark the device as not up. This will stop
       IP packets from trying to be transmitted. */
    vdevice->dev_flags &= ~DV_UP;

#if (NET_VERSION_COMP > NET_4_5)
    /* Reset the state of the modem. */
    link_layer->hwi.state = INITIAL;
#else
    /* Set the device event handler to PPEH_Timer_Expire. */
    vdevice->dev_event = PPEH_Timer_Expire;
#endif

    /* Reset the hardware address to broadcast. */
    memcpy(ppe_layer->ppe_node.ppe_mac_addr, NET_Ether_Broadaddr, DADDLEN);

    /* Null the resend buffer pointer. */
    PPEH_Clean_RBuf(ppe_layer);

    /* Set the session id to invalid, thus returning it to the pool
    of available devices to be used by another process. */
    ppe_layer->ppe_status = 0;
    ppe_layer->ppe_session_id = PPE_INIT_SESSION_ID;
    ppe_layer->ppe_vdevice_id = PPE_EMPTY_SESSION_ID;

} /* PPE_Return_Device */



/************************************************************************
* FUNCTION
*
*       PPE_Create_Session
*
* DESCRIPTION
*
*       Prepare the virtual device for PPP communication by setting
*       the PPP event handler pointer into the device structure,
*       cleaning up any discovery information, and setting the status
*       bits to mark the device as an active session.
*
* INPUTS
*
*       *vdevice            - A pointer to the PPPoE virtual device.
*
* OUTPUTS
*
*       NU_SUCCESS          - Session was created successfully (always).
*
************************************************************************/
STATUS PPE_Create_Session(DV_DEVICE_ENTRY *vdevice, UINT16 sid)
{
    LINK_LAYER *link_layer;
    PPE_LAYER  *ppe_layer;

    link_layer = (LINK_LAYER*)vdevice->dev_ppp_layer;
    ppe_layer = (PPE_LAYER*)link_layer->link;

    /* Assign the session ID from the frame to the PPPoE structure. */
    ppe_layer->ppe_session_id = sid;

#if (NET_VERSION_COMP <= NET_4_5)
    /* Set the event handler for this device to PPP_Event_Handler()
       This is done since the device is used by both PPP and PPPoE,
       so vdevice->dev_event needs to handle both protocols. */
    vdevice->dev_event = ppe_layer->ppe_event_handler;
#endif

    /* Clean up any PADx buffer that hasn't been resent. */
    PPEH_Clean_RBuf(ppe_layer);

    /* Set the state for this link as a valid session. */
    ppe_layer->ppe_status = (PPE_ACTIVE | PPE_SESSION);

#if (NET_VERSION_COMP > NET_4_5)
    /* Set the state of the modem. */
    link_layer->hwi.state = OPENED;
#endif

    return NU_SUCCESS;

} /* PPE_Create_Session */



/************************************************************************
* FUNCTION
*
*       PPE_Get_Unique_Id
*
* DESCRIPTION
*
*       Calculate a new unique identifier by incrementing a global
*       counter variable and checking the device table to see if it
*       is currently in use.
*
* INPUTS
*
*       UINT16              - The mode, or type of id required.
*
* OUTPUTS
*
*       UINT16              - A 16 bit number used as an identifier of
*                              a virtual device.
*
************************************************************************/
UINT16 PPE_Get_Unique_Id(UINT16 mode)
{
    UINT16 *gcounter;

    if (mode == PPE_SESSION_ID)
        gcounter = &PPE_Next_Session_Id;
    else
        gcounter = &PPE_Next_VDevice_Id;
    
    /* If the id already exists in a device, then skip it. It is not
       likely that a match will occur until after 65535 when it loops
       back around. */
    do
    {
        /* Increment the global counter. */
        (*gcounter)++;

        /* Make sure it is between 1 and 65535, inclusive. */
        if (*gcounter == 0xFFFF || *gcounter == 0)
            *gcounter = 1;

    } while (PPE_Find_Virtual_Device(*gcounter, mode) != NU_NULL);

    return *gcounter;

} /* PPE_Get_Unique_Id */

