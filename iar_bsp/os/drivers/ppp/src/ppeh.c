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
*       ppeh.c                                    
*
* COMPONENT
*
*       PPEH - PPPoE Driver Host Routines.
*
* DESCRIPTION
*
*       These routines provide the functionality specific to a
*       PPPoE Host or Relay Agent application.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       PPEH_Build_PADI_Packet
*       PPEH_Clean_RBuf
*       PPEH_Control_Timer
*       PPEH_Discovery
*       PPEH_Find_Client_Device
*       PPEH_Process_PADO
*       PPEH_Process_PADS
*       PPEH_Resend
*       PPEH_Send_PADI
*       PPEH_Timer_Expire
*       PPEH_Validate_ACName
*
* DEPENDENCIES
*
*       None.
*
***************************************************************************/
#include "drivers/ppe_defs.h"
#include "drivers/ppe_extr.h"


/************************************************************************
* FUNCTION
*
*       PPEH_Process_PADO
*
* DESCRIPTION
*
*       Process an incoming PADO packet and send a PADR packet. This
*       involves decoding the HOSTUNIQ tag that was previously sent,
*       finding the associated device that is making the request,
*       reading and validating the tags in the packet, and making sure
*       the requested service tag exists in the PADO. If everything is
*       successful, send a PADR request packet to the peer.
*
* INPUTS
*
*       *ppe                - A pointer to the PPPoE header information.
*
* OUTPUTS
*
*       NU_SUCCESS          - 
*
************************************************************************/
STATUS PPEH_Process_PADO(PPE_FRAME *ppe)
{
    STATUS                  status = NU_SUCCESS;
    PPE_TAG                 intag;
    NET_BUFFER              *padrbuffer, *buffer;
    DV_DEVICE_ENTRY         *vdevice;
    UINT16                  taglen;
    LINK_LAYER              *link_layer;
    PPE_LAYER               *ppe_layer;

    /* Get the incoming buffer. */
    buffer = MEM_Buffer_List.head;

    /* Move past the header to begin reading tags. */
    PPEU_Adjust_Header(buffer, PPE_HEADER_SIZE);

    /* Find a virtual device using the information we encoded in the
       HOSTUNIQ tag that went out with the PADI packet. */
    vdevice = PPEH_Find_Client_Device(ppe);
    if (vdevice)
    {
        link_layer = (LINK_LAYER*)vdevice->dev_ppp_layer;
        ppe_layer = (PPE_LAYER*)link_layer->link;

        /* The packet now belongs to the virtual device. */
        buffer->mem_buf_device = vdevice;

        /* If we did not send a PADI packet, then this should not be our
           PADO response packet. This would be an error. */
        if (ppe_layer->ppe_status & PPE_PADI_SENT)
        {
            /* Stop the discovery timer that was started when the service
               request PADI packet was sent. */
            PPEH_Control_Timer(vdevice, PPE_STOP_TIMER);

            /* Allocate a Net buffer for sending a PADR request packet. */
            padrbuffer = PPEU_New_Buffer(vdevice);
            if (padrbuffer)
            {
                /* Write a HOSTUNIQ tag at the next tag position. This tag will
                   be the first thing we will look for to resolve the virtual
                   device on arrival of a PADS packet. */
                PPEC_Encode_Cookie(ppe->ppe_macaddr, ppe_layer->ppe_vdevice_id, 
                    ppe_layer->ppe_node.ppe_cookie);
                
                /* Add the cookie tag into the outgoing PADR packet. */
                PPEU_Append_Tag(padrbuffer, PPE_TAG_HOSTUNIQ, 
                    PPE_COOKIE_SIZE, ppe_layer->ppe_node.ppe_cookie);

                /* Add the service this host requires to the temp buffer. */
                PPEU_Append_Tag(padrbuffer, PPE_TAG_SERVICE, 
                    (UINT16)strlen((CHAR*)ppe_layer->ppe_service.name),
                    ppe_layer->ppe_service.name);

                /* Set the starting offset of the first tag and get it. */
                intag.ppe_offset = 0;
                taglen = PPEU_Get_Tag(buffer, &intag);

                /* Extract and process all tags in this PADO. */
                while (intag.ppe_type != PPE_TAG_END)
                {
                    switch (intag.ppe_type)
                    {

                    case PPE_TAG_HOSTUNIQ:

                        /* Ignore it since it has already been processed. */
                        break;

                    case PPE_TAG_SERVICE:

                        /* If the service is specific, then verify that this is 
                           the one we requested. Otherwise we will accept the 
                           service provided by the AC. */
                        if (*ppe_layer->ppe_service.name != 0)
                        {
                            if (!(ppe_layer->ppe_status & PPE_SERVICE_FOUND))
                            {
                                /* See if this service is the one we 
                                   requested. */
                                status = memcmp(intag.ppe_string, 
                                                ppe_layer->ppe_service.name,
                                                (unsigned int)intag.ppe_length);
                                if (status == NU_SUCCESS)
                                {
                                    ppe_layer->ppe_status |= PPE_SERVICE_FOUND;
                                }
                                else
                                    status = NU_PPE_INVALID_PADx;
                            }
                        }
                        else
                        {
                            /* Ignore the services, since the request is the 
                               null string. Alternatively, the application 
                               developer can add code here that will "decide" 
                               which service to use. */
                            ppe_layer->ppe_status |= PPE_SERVICE_FOUND;
                        }

                        break;

                    case PPE_TAG_ACNAME:

                        /* Check the AC Name if we are requesting a connection 
                           with a particular AC. */
                        if (ppe_layer->ppe_status & PPE_REQUEST_BY_NAME)
                        {
                            /* See if this is the AC we want to connect with. */
                            status = memcmp(intag.ppe_string, ppe_layer->ppe_node.ppe_fname,
                                            (unsigned int)intag.ppe_length);
                            if (status == NU_SUCCESS)
                                ppe_layer->ppe_status |= PPE_AC_NAME_FOUND;
                            else
                                status = NU_PPE_INVALID_PADx;
                        }

                        break;

                    case PPE_TAG_ACCOOKIE:
                    case PPE_TAG_RELAYID:
                        
                        /* Copy this tag into the outgoing PADR packet. */
                        PPEU_Append_Tag(padrbuffer, intag.ppe_type, intag.ppe_length,
                                        intag.ppe_string);
                        

                        break;

                    default:;
                        /* Ignore tag. */
                    }

                    if (status == NU_SUCCESS)
                    {
                        /* Add the string length to the offset so we can read 
                           another tag. */
                        intag.ppe_offset += taglen;

                        /* Make sure not to go past the total packet length. */
                        if (ppe->ppe_length > intag.ppe_offset && status == NU_SUCCESS)
                            taglen = PPEU_Get_Tag(buffer, &intag);
                        else
                            break;
                    }
                    else
                        break;
                }

                /* At this point the packet has been read. If everything is 
                   valid, send the PADR request packet. */
                if (status == NU_SUCCESS && ppe_layer->ppe_status & 
                                    (PPE_SERVICE_FOUND | PPE_COOKIE_VERIFIED))
                {
                    /* Get the peer's hardware address. */
                    memcpy(ppe_layer->ppe_node.ppe_mac_addr, ppe->ppe_macaddr, 6);

                    /* Send the PADR response packet. */
                    status = PPE_Send(vdevice, PPE_PADR, padrbuffer);
                    if (status == NU_SUCCESS)
                    {
                        /* Restart the discovery timer. */
                        status = PPEH_Control_Timer(vdevice, PPE_RESET_TIMER);

                        /* Set the PPPoE state to PPE_PADR_SENT. */
                        ppe_layer->ppe_status &= ~PPE_DISCOVERY_MASK;
                        ppe_layer->ppe_status |= PPE_PADR_SENT;
                    }
                }
                else
                {
#if (NU_DEBUG_PPE == NU_TRUE)        
                    if (ppe_layer->ppe_status & PPE_SERVICE_ERROR)
                    {
                        PPE_Printf("PPEH_Process_PADO() - No service tag present in PADO\r\n");
                    }
#endif

                    /* Make sure we don't save the state of this device, so it
                       will be reset for new discovery. */
                    ppe_layer->ppe_status &= ~PPE_DISCOVERY_MASK;

                    /* Dump the PADR buffer. */
                    MEM_One_Buffer_Chain_Free(padrbuffer, &MEM_Buffer_Freelist);
                }

                /* Reset the status bits in preparation for the next stage. */
                PPE_Reset_State(vdevice, PPE_HOST_MODE);
            }
            else
            {
#if (NU_DEBUG_PPE == NU_TRUE)        
                PPE_Printf("PPEH_Process_PADO() - Could not allocate a response buffer.\r\n");
#endif

                /* Make sure we don't save the state of this device, so it
                   will be reset for new discovery. */
                ppe_layer->ppe_status &= ~PPE_DISCOVERY_MASK;

                /* Dump the PADR buffer. */
                MEM_One_Buffer_Chain_Free(padrbuffer, &MEM_Buffer_Freelist);
                status = NU_PPE_INVALID_PADx;
            }
        }
        else
            status = NU_PPE_INVALID_PADx;
    }
    else
    {
#if (NU_DEBUG_PPE == NU_TRUE)        
        PPE_Printf("PPEH_Process_PADO() - No device for this connection.\r\n");
#endif
        status = NU_PPE_INVALID_PADx;
    }

    return status;

} /* PPEH_Process_PADO */



/************************************************************************
* FUNCTION
*
*       PPEH_Process_PADS
*
* DESCRIPTION
*
*       Process an incoming PADS packet and create the session if all
*       is ok. This involves finding an available device that can make 
*       the connection, reading and validating the tags in the packet,
*       and making sure the session was created for the requested
*       service. Then set the connection event to initiate the PPP 
*       layer for continued negotiation.
*
* INPUTS
*
*       *ppe                - A pointer to the PPPoE header information.
*
* OUTPUTS
*
*       NU_SUCCESS          - PADS ok and a session is established.
*       NU_PPE_INVALID_PADx - An error in the PADS packet.
*
************************************************************************/
STATUS PPEH_Process_PADS(PPE_FRAME *ppe)
{
    STATUS                  status = NU_SUCCESS;
    PPE_TAG                 intag;
    NET_BUFFER             *buffer;
    DV_DEVICE_ENTRY        *vdevice;
    LINK_LAYER             *link_layer;
    PPE_LAYER              *ppe_layer;
    UINT16                  taglen;
    
    /* Get the incoming buffer. */
    buffer = MEM_Buffer_List.head;

    /* Move past the header to begin reading tags. */
    PPEU_Adjust_Header(buffer, PPE_HEADER_SIZE);

    /* Find a virtual device using the information we encoded in the
       HOSTUNIQ tag that went out with the PADR packet. */
    vdevice = PPEH_Find_Client_Device(ppe);
    if (vdevice)
    {
        link_layer = (LINK_LAYER*)vdevice->dev_ppp_layer;
        ppe_layer = (PPE_LAYER*)link_layer->link;

        /* The packet now belongs to the virtual device. */
        buffer->mem_buf_device = vdevice;

        /* If we did not send a PADR packet, then this should not be our
           PADS response packet. This would be an error. */
        if (ppe_layer->ppe_status & PPE_PADR_SENT)
        {
            /* Stop the discovery timer that was started when the service
               request PADS packet was sent. */
            PPEH_Control_Timer(vdevice, PPE_STOP_TIMER);

            /* Set the starting offset of the first tag and get it. */
            intag.ppe_offset = 0;
            taglen = PPEU_Get_Tag(buffer, &intag);

            /* Extract and process all tags in this PADS. */
            while (intag.ppe_type != PPE_TAG_END)
            {
                switch (intag.ppe_type)
                {
                case PPE_TAG_SERVICE:

                    /* Exactly one service tag is required from this packet. */
                    if (ppe_layer->ppe_status & PPE_SERVICE_FOUND)
                        break;

                    if (strlen((CHAR*)ppe_layer->ppe_service.name) != 0)
                    {
                        /* Make sure it matches the service we requested. */
                        status = memcmp(intag.ppe_string,
                                        ppe_layer->ppe_service.name,
                                        (unsigned int)intag.ppe_length);
                        if  (status == NU_SUCCESS)
                            ppe_layer->ppe_status |= PPE_SERVICE_FOUND;
                        else
                            status = NU_PPE_INVALID_PADx;
                    }
                    else
                        /* Ignore the services, since the request is the null 
                           string. */
                        ppe_layer->ppe_status |= PPE_SERVICE_FOUND;
                    
                    break;


                case PPE_TAG_HOSTUNIQ:

                    /* Ignore it since it has already been processed. */
                    break;

                case PPE_TAG_SERVICERR:

                    /* Optionally add a call to a custom error handler
                       and pass in the ppe_layer pointer. */
                    status = PPE_AC_ERROR;
                    break;

                case PPE_TAG_SYSTEMERR:

                    /* Optionally add a call to a custom error handler
                       and pass in the ppe_layer pointer. */
                    status = PPE_AC_ERROR;
                    break;

                case PPE_TAG_GENERR:

                    /* Optionally add a call to a custom error handler
                       and pass in the ppe_layer pointer. */
                    status = PPE_AC_ERROR;
                    break;

                default:
                    /* Ignore tag. */;
                }

                if (status == NU_SUCCESS)
                {
                    /* Add the string length to the offset so we can read 
                       another tag. */
                    intag.ppe_offset += taglen;

                    /* Make sure not to go past the total packet length. */
                    if (ppe->ppe_length > intag.ppe_offset && status == NU_SUCCESS)
                        taglen = PPEU_Get_Tag(buffer, &intag);
                    else
                        break;
                }
                else
                    break;
            }

            /* At this point the packet has been read. If everything is valid,
               create a session and start PPP. */
            if (status == NU_SUCCESS)
            {
                /* Get the peer's hardware address. */
                memcpy(ppe_layer->ppe_node.ppe_mac_addr, ppe->ppe_macaddr, 6);

                /* Clean up the ppe_layer and set up the session. */
                PPE_Create_Session(vdevice, ppe->ppe_sid);

                /* Set the discovery event, signifying that the connection has 
                   been made, and PPP can start negotiations. 
                   Note: PPEH_Discovery() is waiting on this event. */
                NU_Set_Events(&ppe_layer->ppe_event, PPE_CONNECT, NU_OR);
            }
            else
            {
                /* If any error results at this stage, we just need to 
                   start over. */
                ppe_layer->ppe_status &= ~PPE_DISCOVERY_MASK;
                
                PPE_Reset_State(vdevice, PPE_HOST_MODE);
                
#if (NU_DEBUG_PPE == NU_TRUE)        
                PPE_Printf("PPEH_Process_PADS() - Error in PADS packet.\r\n");
#endif
            }
        }
        else
            status = NU_PPE_INVALID_PADx;
    }
    else
    {
#if (NU_DEBUG_PPE == NU_TRUE)        
        PPE_Printf("PPEH_Process_PADS() - No device for this packet.\r\n");
#endif
        status = NU_PPE_INVALID_PADx;
    }

    return status;

} /* PPEH_Process_PADS */



/************************************************************************
* FUNCTION
*
*       PPEH_Build_PADI_Packet
*
* DESCRIPTION
*
*       Get a new buffer from the buffer freelist and add the
*       HOSTUNIQ tag and service tag before returning it to the
*       calling function.
*
* INPUTS
*
*       *vdevice            - Pointer to the device structure.
*
* OUTPUTS
*
*       NET_BUFFER*         - Pointer to the PADI packet Net buffer.
*
************************************************************************/
NET_BUFFER *PPEH_Build_PADI_Packet(DV_DEVICE_ENTRY *vdevice)
{
    NET_BUFFER      *buffer;
    UINT8           *service;
    LINK_LAYER      *link_layer;
    PPE_LAYER       *ppe_layer;
    UINT16          len;
    
    /* Get a pointer to the ppe_layer structure. */
    link_layer = (LINK_LAYER*)vdevice->dev_ppp_layer;
    ppe_layer = (PPE_LAYER*)link_layer->link;
    service = ppe_layer->ppe_service.name;

    /* Make sure the service is not too long. This check may only be
       necessary if the service string is created dynamically. */
    len = (UINT16)strlen((CHAR*)service);
    if (len > PPE_PADx_MAXSIZE)
    {
#if (NU_DEBUG_PPE == NU_TRUE)
        PPE_Printf("Build_PADI_Packet() - Service name too long.\r\n");
#endif
        buffer = NU_NULL;
    }
    else
    {
        /* Get a buffer for building the packet. */
        buffer = PPEU_New_Buffer(vdevice);
        if (buffer)
        {
            /* Write a HOSTUNIQ tag at the next tag position. This tag will
               be the first thing we will look for to resolve the virtual
               device on arrival of a PADS packet. */
            PPEC_Encode_Cookie(ppe_layer->ppe_node.ppe_mac_addr, 
                        ppe_layer->ppe_vdevice_id, ppe_layer->ppe_node.ppe_cookie);

            /* Add the cookie tag into the outgoing PADI packet. */
            PPEU_Append_Tag(buffer, PPE_TAG_HOSTUNIQ, PPE_COOKIE_SIZE, 
                ppe_layer->ppe_node.ppe_cookie);

            /* Add the service this host requires to the buffer. */
            PPEU_Append_Tag(buffer, PPE_TAG_SERVICE, len, service);
        }
#if (NU_DEBUG_PPE == NU_TRUE)
        else
        {
            PPE_Printf("Build_PADI_Packet() - Could not build packet.\r\n");
        }
#endif
    }

    return buffer;

} /* PPEH_Build_PADI_Packet */



/************************************************************************
* FUNCTION
*
*       PPEH_Discovery
*
* DESCRIPTION
*
*       Obtain an available PPPoE device if one hasn't been provided
*       as an argument, initialize it with the necessary information
*       as a Host device, and start the discovery process by sending
*       a PADI packet to the broadcast address. The thread will remain
*       suspended here until a connection is made, or a timeout occurs.
*
* INPUTS
*
*       *service            - Pointer to the requested service string.
*       *vdevice            - A pointer to the PPPoE virtual device.
*
* OUTPUTS
*
*       NU_SUCCESS          - A connection has been established.
*       NU_NO_CONNECT       - No connection could be made.
*
************************************************************************/
STATUS PPEH_Discovery(CHAR *service, DV_DEVICE_ENTRY *vdevice)
{
    STATUS          status;
    UNSIGNED        event;
    LINK_LAYER      *link_layer;
    PPE_LAYER       *ppe_layer;

    /* Reserve the TCP_Resource semaphore for exclusive access to the
       list of PPPoE devices. */
    NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);
    
    /* Make sure the device is available, or if any device is available. */
    if (vdevice == NU_NULL)
    {
        /* If the caller doesn't care which device to use, find one. */
        vdevice = PPE_Reserve_Device(PPE_HOST_MODE);
        if (vdevice == NU_NULL)
        {
#if (NU_DEBUG_PPE == NU_TRUE)
            PPE_Printf("PPEH_Discovery() - Virtual device not available.\r\n");
#endif
            return NU_NO_CONNECT;
        }
    }
    else
    {
        /* The device was provided to us, so reset it to initial status. */
        PPE_Reset_State(vdevice, PPE_HOST_MODE);
    }
	
    link_layer = (LINK_LAYER*)vdevice->dev_ppp_layer;
    ppe_layer = (PPE_LAYER*)link_layer->link;	

    /* Done with access to the virtual devices. Release the semaphore. */
    NU_Release_Semaphore(&TCP_Resource);
    
    /* Save the service requested into the ppe_layer structure. */
    ppe_layer->ppe_service.name = (UINT8*)service;

    /* Send a PADI packet. Then suspend on the response. */
    status = PPEH_Send_PADI(vdevice);
    if (status == NU_SUCCESS)
    {
        /* Start the discovery timer (PADx). */
        PPEH_Control_Timer(vdevice, PPE_RESET_TIMER);

        /* The application thread will suspend until a connection is made,
           or timeout. */
        NU_Retrieve_Events(&ppe_layer->ppe_event, (PPE_CONNECT | PPE_TIMEOUT), 
                            NU_OR_CONSUME, &event, NU_SUSPEND);
        if (event & PPE_CONNECT)
        {
#if (NU_DEBUG_PPE == NU_TRUE)
            PPE_Printf("Connection established...\r\n");
#endif

#if (NET_VERSION_COMP > NET_4_5)
            /* Set the hwi state. This tells PPP that the PPPoE device is up. */
            link_layer->hwi.state = OPENED;
#endif
            status = NU_SUCCESS;
        }
        else
        {
            /* Reserve the TCP_Resource semaphore for exclusive access to the
               list of PPPoE devices. */
            NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);
            
            /* Discovery was not successful, so return the device to its
               initial unused state. */
            PPE_Return_Device(vdevice);

            /* Done with access to the virtual devices. Release the semaphore. */
            NU_Release_Semaphore(&TCP_Resource);
            
#if (NU_DEBUG_PPE == NU_TRUE)
            PPE_Printf("Received a timeout... Failed to connect...\r\n");
#endif
            status = NU_NO_CONNECT;
        }
    }
    else
    {
        /* The initial PADI packet could not be sent. */
#if (NU_DEBUG_PPE == NU_TRUE)
        PPE_Printf("PPEH_Discovery() - Failed sending PADI...\r\n");
#endif
        status = NU_NO_CONNECT;
    }

    return status;

} /* PPEH_Discovery */



/************************************************************************
* FUNCTION
*
*       PPEH_Find_Client_Device
*
* DESCRIPTION
*
*       Search the PADx packet for a HOSTUNIQ identifier, decode it
*       and verify that it is the one we sent previously. Using the sid
*       identifier, search the device table for a matching PPPoE
*       virtual device and return it.
*
* INPUTS
*
*       *ppe                - A pointer to the PPPoE header information.
*
* OUTPUTS
*
*       DV_DEVICE_ENTRY*    - Pointer to a matching virtual device, or
*                             NU_NULL if not found.
*
************************************************************************/
DV_DEVICE_ENTRY *PPEH_Find_Client_Device(PPE_FRAME *ppe)
{
    DV_DEVICE_ENTRY     *vdevice;
    NET_BUFFER          *buffer;
    LINK_LAYER          *link_layer;
    PPE_LAYER           *ppe_layer;
    PPE_TAG             tag;
    UINT16              sid, taglen;

    buffer = MEM_Buffer_List.head;

    tag.ppe_offset = 0;
    taglen = PPEU_Get_Tag(buffer, &tag);

    /* Search for the cookie tag, since this is the only true way to
       identify the virtual device associated with this packet. */
    while (tag.ppe_type != PPE_TAG_HOSTUNIQ)
    {
        /* Add the string length to the offset so we can read another tag. */
        tag.ppe_offset += taglen;

        /* Make sure not to go past the total packet length. */
        if (ppe->ppe_length > tag.ppe_offset)
            taglen = PPEU_Get_Tag(buffer, &tag);
        else
            break;
    }

    /* Decode the cookie into the source macaddr and device identifier. */
    sid = PPEC_Decode_Cookie(&tag);
    if (sid)
    {
        /* Find a virtual device that matches the session id in the cookie. */
        vdevice = PPE_Find_Virtual_Device(sid, PPE_VDEVICE_ID);
        if (vdevice)
        {
            link_layer = (LINK_LAYER*)vdevice->dev_ppp_layer;
            ppe_layer = (PPE_LAYER*)link_layer->link;

            /* Verify the peer ethernet address and make sure it is not a 
               session. These checks are just extra precautions and could 
               probably be removed for systems of low fault tolerance. */
            if (memcmp(tag.ppe_string, ppe_layer->ppe_node.ppe_mac_addr, DADDLEN) != 0 ||
                ppe_layer->ppe_status & PPE_SESSION)

                /* This device is invalid. */
                vdevice = NU_NULL;
        }
    }
    else
        vdevice = NU_NULL;

    return vdevice;

} /* PPEH_Find_Client_Device */



/************************************************************************
* FUNCTION
*
*       PPEH_Send_PADI
*
* DESCRIPTION
*
*       Build a PADI packet and send it to the specified device.
*
* INPUTS
*
*       *vdevice            - A pointer to the PPPoE virtual device.
*
* OUTPUTS
*
*       NU_SUCCESS          - The PADI was successfully sent.
*
************************************************************************/
STATUS PPEH_Send_PADI(DV_DEVICE_ENTRY *vdevice)
{
    NET_BUFFER      *buffer;
    STATUS          status;
    LINK_LAYER      *link_layer;
    PPE_LAYER       *ppe_layer;

    link_layer = (LINK_LAYER*)vdevice->dev_ppp_layer;
    ppe_layer = (PPE_LAYER*)link_layer->link;

    /* Create a PADI packet for this service. */
    buffer = PPEH_Build_PADI_Packet(vdevice);
    if (buffer == NU_NULL)
    {
#if (NU_DEBUG_PPE == NU_TRUE)
        PPE_Printf("PPEH_Send_PADI() - Could not create a PADI packet.\r\n");
#endif
        return NU_PPE_ERROR;
    }

    /* Send the packet. */
    status = PPE_Send(vdevice, PPE_PADI, buffer);
    if (status == NU_SUCCESS)
    {
        /* Set a flag that a PADI has been sent. This will hold this
           device open to receive a PADO response. */
        ppe_layer->ppe_status |= PPE_PADI_SENT;
    }
#if (NU_DEBUG_PPE == NU_TRUE)
    else
    {
        PPE_Printf("PPEH_Send_PADI() - Could not send the PADI packet.\r\n");
    }
#endif

    return status;

} /* PPEH_Send_PADI */



/************************************************************************
* FUNCTION
*
*       PPEH_Timer_Expire
*
* DESCRIPTION
*
*       This is the entry function called when a timer event is set.
*       The timer event is set when a PADI or PADR is sent and no
*       response is received. At this point the decision is made 
*       whether or not to resend the PADI or PADR. If a timeout has
*       occurred, then free the buffers and set a timeout event for
*       the calling task. Otherwise, resend the packet.
*
* INPUTS
*
*       *vdevice            - A pointer to the PPPoE virtual device.
*
* OUTPUTS
*
*       NU_SUCCESS          - The event is processed successfully.
*       PPE_TIMEOUT         - A discovery timeout has occurred.
*
************************************************************************/
#if (NET_VERSION_COMP > NET_4_5)
VOID PPEH_Timer_Expire(TQ_EVENT evt, UNSIGNED dat, UNSIGNED subevt)
#else
STATUS PPEH_Timer_Expire(DV_DEVICE_ENTRY *vdevice, UNSIGNED evt)
#endif
{
    STATUS      status;
#if (NET_VERSION_COMP > NET_4_5)
    DV_DEVICE_ENTRY *vdevice = (DV_DEVICE_ENTRY*)dat;
#endif

    LINK_LAYER *link_layer;
    PPE_LAYER  *ppe_layer;

    /* There is only one event for PPPoE. */
    UNUSED_PARAMETER(evt);
#if (NET_VERSION_COMP > NET_4_5)
    UNUSED_PARAMETER(subevt);
#endif
    link_layer = (LINK_LAYER*)vdevice->dev_ppp_layer;
    ppe_layer = (PPE_LAYER*)link_layer->link;

    /* We need to either restart the timer with double the timeout
       value, or determine if this was the last shot. */
    status = PPEH_Control_Timer(vdevice, PPE_RESTART_TIMER);
    if (status == PPE_TIMEOUT)
    {
        /* Free the buffer since we'll not send it again. */
        PPEH_Clean_RBuf(ppe_layer);

        /* Let the caller application know that we timed out. */
        NU_Set_Events(&ppe_layer->ppe_event, PPE_TIMEOUT, NU_OR);
    }
    else
    {
        /* Resend the packet that just timed out. Since the packet is
           fully formed and no modifications need to be made, this can
           be sent directly through the ethernet device. */
        status = PPEH_Resend(vdevice, ppe_layer->ppe_bufptr);
        if (status != NU_SUCCESS)
        {
            /* Free the buffer since we'll not send it again. */
            PPEH_Clean_RBuf(ppe_layer);

#if (NET_VERSION_COMP <= NET_4_5)
            status = NU_PPE_ERROR;
#endif
        }
#if (NU_DEBUG_PPE == NU_TRUE)
        else
            PPE_Printf("Timeout... Resent packet.\r\n");
#endif
    }

#if (NET_VERSION_COMP <= NET_4_5)
    return status;
#endif

} /* PPEH_Timer_Expire */



/************************************************************************
* FUNCTION
*
*       PPEH_Resend
*
* DESCRIPTION
*
*       This function simply removes the existing ethernet header and
*       resubmits the requested arguments to the hardware device's
*       output routine.
*
* INPUTS
*
*       vdevice             - A pointer to the PPPoE virtual device.
*       *buffer             - Pointer to the Net buffer to send.
*
* OUTPUTS
*
*       NU_SUCCESS          - The buffer was sent successfully.
*
************************************************************************/
STATUS PPEH_Resend(DV_DEVICE_ENTRY *vdevice, NET_BUFFER *buffer)
{
    STATUS              status;
    PPE_ADDR            dest;
    DV_DEVICE_ENTRY     *hdevice;
    LINK_LAYER          *link_layer;
    PPE_LAYER           *ppe_layer;
    INT32               dv_up;

    /* Get the hardware device pointer. */
    link_layer = (LINK_LAYER*)vdevice->dev_ppp_layer;
    ppe_layer = (PPE_LAYER*)link_layer->link;
    hdevice = ppe_layer->ppe_hdevice;

    /* Since the buffer already includes the correct ethernet header, we
       need to remove it, since the output routine will add it again. */
    PPEU_Adjust_Header(buffer, sizeof(DLAYER));

    /* Set up the destination address information for Net_Ether_Send(). */
    dest.sck.sck_family = SK_FAM_UNSPEC;
    memcpy(&dest.mac.ar_mac.ar_mac_ether, ppe_layer->ppe_node.ppe_mac_addr, DADDLEN);

    /* Set the ethernet type field based on type argument. */
    dest.mac.ar_mac.ar_mac_ether.type = PPPOED;

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
        PPE_Printf("PPE_Resend() - Failed to send packet through ethernet device\r\n");
    }
#endif
    
    /* Return the ethernet device to its original DV_UP state. */
    hdevice->dev_flags &= ~DV_UP;
    hdevice->dev_flags |= dv_up;

    return status;

} /* PPEH_Resend */



/************************************************************************
* FUNCTION
*
*       PPEH_Control_Timer
*
* DESCRIPTION
*
*       Based on the given options, either stop the timer, reset the
*       timer to its initial timeout value, or calculate a new restart
*       time. For the latter two options, the timer is restarted.
*
* INPUTS
*
*       *vdevice            - A pointer to the PPPoE virtual device.
*
* OUTPUTS
*
*       NU_SUCCESS          - The function completed successfully.
*       PPE_TIMEOUT         - A timeout occurred.
*       NU_PPE_ERROR           - Invalid options.
*
************************************************************************/
STATUS PPEH_Control_Timer(DV_DEVICE_ENTRY *vdevice, UINT32 options)
{
    STATUS          status;
    LINK_LAYER      *link_layer;
    PPE_LAYER       *ppe_layer;

    link_layer = (LINK_LAYER*)vdevice->dev_ppp_layer;
    ppe_layer = (PPE_LAYER*)link_layer->link;

    /* The catchall else statement below assumes the last option, so
       make sure the option is within the range. */
    if (options > 3)
        status = NU_PPE_ERROR;
    else
    {
        /* This will stop the ppe_countdown timer event and clean up. */
        if (options == PPE_STOP_TIMER)
        {
#if (NET_VERSION_COMP > NET_4_5)
            TQ_Timerunset(PPE_Event, TQ_CLEAR_EXACT, (UNSIGNED)vdevice, PPE_PADx_TIMEOUT);
#else
            UTL_Timerunset(PPE_PADx_TIMEOUT, (UNSIGNED)vdevice, NU_NULL);
#endif
            /* Stopping the timer means that we don't need to resend the
               packet, so deallocate the buffers. */
            PPEH_Clean_RBuf(ppe_layer);

            status = NU_SUCCESS;
        }
        else
        {
            /* Reset the timer to the initial values, for a new discovery
               session. If the option is not reset, then it is assumed to be
               a simple timeout. */
            if (options == PPE_RESET_TIMER)
                ppe_layer->ppe_countdown = PPE_MAX_RESTARTS;

            /* Determine whether to restart it or return a timeout. */
            if (ppe_layer->ppe_countdown-- > 0)
            {
                /* Start the timer */
#if (NET_VERSION_COMP > NET_4_5)
                TQ_Timerset(PPE_Event, (UNSIGNED)vdevice,
                            PPE_TIMEOUT_VALUE, PPE_PADx_TIMEOUT);
#else
                UTL_Timerset(PPE_PADx_TIMEOUT, (UNSIGNED)vdevice,
                             PPE_TIMEOUT_VALUE, NU_NULL);
#endif
                status = NU_SUCCESS;
            }
            else
                status = PPE_TIMEOUT;
        }
    }

    return status;

} /* PPEH_Control_Timer */



/************************************************************************
* FUNCTION
*
*       PPEH_Clean_RBuf
*
* DESCRIPTION
*
*       If there is a buffer pointer saved in the ppe_layer structure
*       for use when resending a packet, return it to the freelist
*       null the pointer.
*
* INPUTS
*
*       *ppe_layer          - Pointer to the PPPoE structure.
*
* OUTPUTS
*
*       NU_SUCCESS          - The buffer pointer is now clean (always).
*
************************************************************************/
VOID PPEH_Clean_RBuf(PPE_LAYER* ppe_layer)
{
    /* Make sure there are no pending discovery buffers. */
    if (ppe_layer->ppe_bufptr != NU_NULL)
    {
        /* If it is there, then it hasn't been returned to the free list. */
        MEM_One_Buffer_Chain_Free(ppe_layer->ppe_bufptr, &MEM_Buffer_Freelist);

        /* Now delete it. */
        ppe_layer->ppe_bufptr = NU_NULL;
    }

} /* PPEH_Clean_RBuf */



/************************************************************************
* FUNCTION
*
*       PPEH_Validate_ACName
*
* DESCRIPTION
*
*       Make sure the AC name matches the one we requested in a PADI
*       packet.
*
* INPUTS
*
*       *ppe_layer          - Pointer to the PPPoE structure.
*       *string             - Pointer to the received AC name string.
*       len                 - Length of the string.
*
* OUTPUTS
*
*       NU_SUCCESS          - The name matches.
*       NU_PPE_ERROR           - No match.
*
************************************************************************/
STATUS PPEH_Validate_ACName(PPE_LAYER *ppe_layer, UINT8 *string, UINT16 len)
{
    STATUS status;

    /* Compare the strings to see if they match. */
    if (memcmp(ppe_layer->ppe_node.ppe_fname, string, (unsigned int)len) == 0)
        status = NU_SUCCESS;
    else
        status = NU_PPE_ERROR;

    return status;

} /* PPEH_Validate_ACName */

