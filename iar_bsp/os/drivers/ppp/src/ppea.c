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
*       ppea.c                                    
*
* COMPONENT
*
*       PPEA - PPPoE Driver Access Concentrator (AC) Routines.
*
* DESCRIPTION
*
*       These routines provide the functionality specific to an
*       Access Concentrator or Relay Agent application.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       PPEA_Accept
*       PPEA_Process_PADI
*       PPEA_Process_PADR
*       PPEA_Wait_For_Client
*
* DEPENDENCIES
*
*       PPEC.C  - Configuration information for the AC Services.
*
***************************************************************************/
#include "drivers/ppe_defs.h"
#include "drivers/ppe_extr.h"

extern PPE_SERVICE  PPEC_AC_Services[PPE_NUM_SERVICES];

UINT8 NullService[1] = "";


/************************************************************************
* FUNCTION
*
*        PPEA_Accept
*
* DESCRIPTION
*
*        Wait for a connection event that will occur when a host
*        completes the discovery process successfully. This call will
*        not time out.
*
* INPUTS
*
*        vdevice            - A virtual device to bind connection to.
*
* OUTPUTS
*
*        NU_SUCCESS         - Since it can only return when a connection
*                             occurs. This behavior may change in
*                             future implementations.
*
************************************************************************/
UINT32 PPEA_Accept(DV_DEVICE_ENTRY* vdevice)
{
    UINT32     event;
    LINK_LAYER *link_layer;
    PPE_LAYER  *ppe_layer;
    STATUS      status;
    
    link_layer = (LINK_LAYER*)vdevice->dev_ppp_layer;
    ppe_layer = (PPE_LAYER*)link_layer->link;

    /* Wait for the connection. */
    status = NU_Retrieve_Events(&ppe_layer->ppe_event, PPE_CONNECT,
                        NU_OR_CONSUME, (UNSIGNED *)&event, NU_SUSPEND);
    if (status != NU_SUCCESS)
    {
        /* Return null event, signifying an error. */
        event = 0;
    }

    return event;

} /* PPEA_Accept */



/************************************************************************
* FUNCTION
*
*       PPEA_Process_PADI
*
* DESCRIPTION
*
*       Process an incoming PADI packet and send a response PADO. This
*       involves finding an available device that can make the
*       connection, reading and validating the tags in the packet,
*       and making sure the device can accommodate the requested
*       service. If everything is successful, send a PADO packet
*       to the peer.
*
* INPUTS
*
*       *ppe                - A pointer to the PPPoE header information.
*
* OUTPUTS
*
*       NU_SUCCESS          - PADI ok and a PADO was sent.
*       NU_PPE_INVALID_PADx - An error in the PADI packet.
*
************************************************************************/
STATUS PPEA_Process_PADI(PPE_FRAME *ppe)
{
    STATUS                  status = NU_SUCCESS;
    PPE_TAG                 intag;
    NET_BUFFER              *padobuffer, *buffer;
    DV_DEVICE_ENTRY         *vdevice;
    UINT16                  index;
    UINT16                  taglen;
    LINK_LAYER              *link_layer;
    PPE_LAYER               *ppe_layer;

    /* Get the incoming buffer. */
    buffer = MEM_Buffer_List.head;

    /* Move past the header to begin reading tags. */
    PPEU_Adjust_Header(buffer, PPE_HEADER_SIZE);

    /* Find any listening virtual device. */
    vdevice = PPE_Find_Virtual_Device(PPE_INIT_SESSION_ID, PPE_VDEVICE_ID);
    if (vdevice)
    {
        link_layer = (LINK_LAYER*)vdevice->dev_ppp_layer;
        ppe_layer = (PPE_LAYER*)link_layer->link;

        /* The packet now belongs to the virtual device. */
        buffer->mem_buf_device = vdevice;

        /* Create a response PADO buffer. */
        padobuffer = PPEU_New_Buffer(vdevice);
        if (padobuffer)
        {
            /* Add the AC name to this buffer. */
            PPEU_Append_Tag(padobuffer, PPE_TAG_ACNAME, 
                (UINT16)strlen((CHAR*)ppe_layer->ppe_lname),
                ppe_layer->ppe_lname);
            
            /* Set the starting offset of the first tag and get it. */
            intag.ppe_offset = 0;
            taglen = PPEU_Get_Tag(buffer, &intag);

            /* Extract and process all tags in this PADI. */
            while (intag.ppe_type != PPE_TAG_END)
            {
                switch (intag.ppe_type)
                {
                case PPE_TAG_SERVICE:

                    /* The service tag cannot appear twice in this packet. 
                       Verify it. */
                    if (ppe_layer->ppe_status & PPE_SERVICE_FOUND)
                    {
                        status = NU_PPE_INVALID_PADx;
                    }
                    else
                    {
                        /* Check to see if this requested service is in the AC 
                           list of services. A zero length service indicates 
                           that any service is acceptable. */
                        if (intag.ppe_length != 0)
                        {
                            /* This is a required tag and must exist in the 
                               service list. Search the list for a matching 
                               entry. While searching, might as well dump the 
                               services to the response packet. */
                            for (index = 0; index < PPE_NUM_SERVICES; index++)
                            {
                                /* Copy the tag to the response PADO packet. */
                                PPEU_Append_Tag(padobuffer, PPE_TAG_SERVICE, 
                                    (UINT16)strlen((CHAR*)PPEC_AC_Services[index].name),
                                    PPEC_AC_Services[index].name);

                                /* Only check the AC list of services if it 
                                   hasn't already been found. */
                                if (!(ppe_layer->ppe_status & PPE_SERVICE_FOUND))
                                {
                                    status = memcmp(intag.ppe_string, 
                                                    PPEC_AC_Services[index].name,
                                                    (unsigned int)intag.ppe_length);
                                    if (status == NU_SUCCESS)
                                    {
                                        ppe_layer->ppe_status |= PPE_SERVICE_FOUND;
                                    }
                                }
                            }

                            /* If the requested service was not found, then we 
                               ignore the packet and dump the PADO buffer 
                               (end of this function). */
                            if (!(ppe_layer->ppe_status & PPE_SERVICE_FOUND))
                            {
                                ppe_layer->ppe_status |= PPE_SERVICE_ERROR;
                                status = NU_PPE_INVALID_PADx;
                            }
                        }

                        /* Any service was requested, so just copy all services 
                           into the PADO. */
                        else
                        {
                            /* Copy all services into the PADO packet. */
                            for (index = 0; index < PPE_NUM_SERVICES; index++)
                            {
                                /* Copy the tag to the response PADO packet. */
                                PPEU_Append_Tag(padobuffer, PPE_TAG_SERVICE,
                                    (UINT16)strlen((CHAR*)PPEC_AC_Services[index].name),
                                    PPEC_AC_Services[index].name);
                            }

                            ppe_layer->ppe_status |= PPE_SERVICE_FOUND;
                        }
                    }

                    break;

                case PPE_TAG_ACNAME:

                    /* Make sure we are the AC they are requesting. */
                    if (memcmp(ppe_layer->ppe_lname, intag.ppe_string, 
                        intag.ppe_length) != 0)
                        status = NU_PPE_INVALID_PADx;

                    break;
                    
                case PPE_TAG_HOSTUNIQ:
                case PPE_TAG_RELAYID:
                    
                    /* Copy this tag into the outgoing PADO packet. */
                    PPEU_Append_Tag(padobuffer, intag.ppe_type, intag.ppe_length,
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

            /* At this point the packet has been read. If everything is valid,
               send the PADO packet response. */
            if (status == NU_SUCCESS && ppe_layer->ppe_status & PPE_SERVICE_FOUND)
            {
                /* Get the peer's hardware address. */
                memcpy(ppe_layer->ppe_node.ppe_mac_addr, ppe->ppe_macaddr, 6);

                /* Send the PADO response packet. */
                status = PPE_Send(vdevice, PPE_PADO, padobuffer);
                if (status != NU_SUCCESS)
                {
#if (NU_DEBUG_PPE == NU_TRUE)        
                    PPE_Printf("PPEA_Process_PADI() - Failed to send PADO packet...\r\n");
#endif
                    status = NU_PPE_ERROR;
                }
            }
            else
            {
#if (NU_DEBUG_PPE == NU_TRUE)        
                PPE_Printf("PPEA_Process_PADI() - Tag error in PADI\r\n");
#endif

                /* Dump the PADO buffer. */
                MEM_One_Buffer_Chain_Free(padobuffer, &MEM_Buffer_Freelist);
            }
        }
        
        /* The AC doesn't save states between discovery stages, 
        so reset the status bits. */
        PPE_Reset_State(vdevice, PPE_AC_MODE);
    }
    else
    {
#if (NU_DEBUG_PPE == NU_TRUE)        
        PPE_Printf("PPEA_Process_PADI() - Could not find a listening device.\r\n");
#endif
        status = NU_PPE_INVALID_PADx;
    }

    return status;

} /* PPEA_Process_PADI */



/************************************************************************
* FUNCTION
*
*       PPEA_Process_PADR
*
* DESCRIPTION
*
*       Process an incoming PADR packet and create the session if all
*       is ok. This involves finding an available device that can make 
*       the connection, reading and validating the tags in the packet,
*       and making sure the device can accommodate the requested
*       service. Send a response PADS with either an error message or
*       the session id that the host will use.
*
* INPUTS
*
*       *ppe                - A pointer to the PPPoE header information.
*
* OUTPUTS
*
*       NU_SUCCESS          - PADR ok and a session is established.
*       NU_PPE_INVALID_PADx - An error in the PADR packet.
*
************************************************************************/
STATUS PPEA_Process_PADR(PPE_FRAME *ppe)
{
    STATUS                  status = NU_SUCCESS;
    PPE_TAG                 intag;
    NET_BUFFER              *padsbuffer, *buffer;
    DV_DEVICE_ENTRY         *vdevice;
    UINT16                  index;
    UINT16                  taglen;
    LINK_LAYER              *link_layer;
    PPE_LAYER               *ppe_layer;

    /* Get the incoming buffer. */
    buffer = MEM_Buffer_List.head;

    /* Move past the header to begin reading tags. */
    PPEU_Adjust_Header(buffer, PPE_HEADER_SIZE);

    /* Find a listening virtual device. */
    vdevice = PPE_Find_Virtual_Device(PPE_INIT_SESSION_ID, PPE_VDEVICE_ID);
    if (vdevice)
    {
        link_layer = (LINK_LAYER*)vdevice->dev_ppp_layer;
        ppe_layer = (PPE_LAYER*)link_layer->link;

        /* The packet now belongs to the virtual device. */
        buffer->mem_buf_device = vdevice;

        /* Allocate a Net buffer for sending a PADS response packet. */
        padsbuffer = PPEU_New_Buffer(vdevice);
        if (padsbuffer)
        {
            /* Add the AC name to this buffer. */
            PPEU_Append_Tag(padsbuffer, PPE_TAG_ACNAME, 
                (UINT16)strlen((CHAR*)ppe_layer->ppe_lname),
                ppe_layer->ppe_lname);
            
            /* Set the starting offset of the first tag and get it. */
            intag.ppe_offset = 0;
            taglen = PPEU_Get_Tag(buffer, &intag);

            /* Extract and process all tags in this PADR. */
            while (intag.ppe_type != PPE_TAG_END)
            {
                switch (intag.ppe_type)
                {
                case PPE_TAG_SERVICE:

                    /* The service tag cannot appear twice in this packet. 
                       Verify it. */
                    if (ppe_layer->ppe_status & PPE_SERVICE_FOUND)
                    {
                        ppe_layer->ppe_status |= PPE_SERVICE_ERROR;
                        status = NU_PPE_INVALID_PADx;
                    }
                    else
                    {
                        /* Check to see if this requested service is in the AC 
                           list of services. A zero length service indicates 
                           that any service is acceptable. */
                        if (intag.ppe_length != 0)
                        {
                            /* Search the list for a matching service. */
                            for (index = 0; index < PPE_NUM_SERVICES; index++)
                            {
                                /* Check to see if this requested service is in 
                                   the AC list of services. */
                                status = memcmp(intag.ppe_string, 
                                                PPEC_AC_Services[index].name,
                                                (unsigned int)intag.ppe_length);
                                if (status == NU_SUCCESS)
                                {
                                    /* Add this service to the temp 
                                       ppe_layer->ppe_service so that it can be 
                                       referenced when sending a response 
                                       packet. */
                                    ppe_layer->ppe_service.name = 
                                        PPEC_AC_Services[index].name;
                                    ppe_layer->ppe_status |= PPE_SERVICE_FOUND;

                                    /* Add the service as a tag into the 
                                       outgoing PADS packet. */
                                    PPEU_Append_Tag(padsbuffer, PPE_TAG_SERVICE,
                                        (UINT16)strlen((CHAR*)ppe_layer->ppe_service.name),
                                        ppe_layer->ppe_service.name);
                                    break;
                                }
                            }
                        }
                        else
                        {
                            /* Add null service to the temp ppe_layer->ppe_service 
                               so that it can be referenced when sending a 
                               response packet. */
                            ppe_layer->ppe_service.name = NullService;
                            ppe_layer->ppe_status |= PPE_SERVICE_FOUND;

                            /* Add the service as a tag into the outgoing PADS 
                               packet. */
                            PPEU_Append_Tag(padsbuffer, PPE_TAG_SERVICE,
                                (UINT16)strlen((CHAR*)ppe_layer->ppe_service.name),
                                ppe_layer->ppe_service.name);
                        }
                    }

                    break;

                case PPE_TAG_ACNAME:

                    /* Verify that this is the correct AC name. */
                    status = memcmp(intag.ppe_string, PPE_AC_NAME, 
                                    (unsigned int)intag.ppe_length);
                    if (status == NU_SUCCESS)
                    {
                        ppe_layer->ppe_status |= PPE_AC_NAME_FOUND;
                    }
                    else
                    {
                        /* Wrong AC Name. Send an error message. */
                        PPEU_Append_Tag(padsbuffer, PPE_TAG_GENERR, 
                            (UINT16)strlen((CHAR*)PPE_BAD_ACNAME_MSG),
                            (UINT8*)PPE_BAD_ACNAME_MSG);

                        status = NU_PPE_INVALID_PADx;
                    }

                    break;

                case PPE_TAG_ACCOOKIE:

                    /* Ignore it since it has already been processed. */
                    break;

                case PPE_TAG_RELAYID:
                case PPE_TAG_HOSTUNIQ:
                    
                    /* Copy this tag into the outgoing PADS packet. */
                    PPEU_Append_Tag(padsbuffer, intag.ppe_type, intag.ppe_length,
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

            /* At this point the packet has been read. If everything is valid,
               create a session and send the PADS packet response. */
            if (status == NU_SUCCESS && ppe_layer->ppe_status & PPE_SERVICE_FOUND)
            {
                /* Get the peer's hardware address. */
                memcpy(ppe_layer->ppe_node.ppe_mac_addr, ppe->ppe_macaddr, 6);

                /* Bind the session to this device. */
                PPE_Create_Session(vdevice, PPE_Get_Unique_Id(PPE_SESSION_ID));

                /* Send the PADS response packet. */
                status = PPE_Send(vdevice, PPE_PADS, padsbuffer);
                if (status == NU_SUCCESS)
                {
#if (NET_VERSION_COMP > NET_4_5)
                    /* Set the hwi state. This tells PPP that the PPPoE device
                       (modem) is up. */
                    link_layer->hwi.state = OPENED;
#endif
                    /* Let the server application task know that a session has been
                       established. */
                    NU_Set_Events(&ppe_layer->ppe_event, PPE_CONNECT, NU_OR);
                }
            }
            else
            {
                if (ppe_layer->ppe_status & PPE_SERVICE_ERROR)
                {
#if (NU_DEBUG_PPE == NU_TRUE)        
                    PPE_Printf("PPEA_Process_PADR() - Bad service tag present in PADR\r\n");
#endif

                    /* Add an error tag into the outgoing PADS packet. */
                    PPEU_Append_Tag(padsbuffer, PPE_TAG_SERVICERR,
                                    (UINT16)strlen((CHAR*)PPE_SERVICE_ERROR_MSG),
                                    (UINT8*)PPE_SERVICE_ERROR_MSG);

                    /* Get the peer's hardware address. */
                    memcpy(ppe_layer->ppe_node.ppe_mac_addr, ppe->ppe_macaddr, 6);

                    /* Send the PADS response packet. */
                    PPE_Send(vdevice, PPE_PADS, padsbuffer);
                }
                else
                {
#if (NU_DEBUG_PPE == NU_TRUE)        
                    PPE_Printf("PPEA_Process_PADR() - No service tag present in PADR\r\n");
#endif

                    /* Dump the PADS buffer. */
                    MEM_One_Buffer_Chain_Free(padsbuffer, &MEM_Buffer_Freelist);
                }

                /* Since discovery has to start over for this host, reset the 
                   status for a new host discovery. */
                PPE_Reset_State(vdevice, PPE_AC_MODE);
            }
        }
    }
    else
    {
#if (NU_DEBUG_PPE == NU_TRUE)        
        PPE_Printf("PPEA_Process_PADR() - Could not find a listening device.\r\n");
#endif
        status = NU_PPE_INVALID_PADx;
    }

    return status;

} /* PPEA_Process_PADR */



/************************************************************************
* FUNCTION
*
*       PPEA_Wait_For_Client
*
* DESCRIPTION
*
*       Obtain a virtual device, either from the argument or find any
*       available. Set up the initial information for establishing a
*       connection, and wait for the connection. This function will
*       not return without connection with a peer.
*
* INPUTS
*
*       vdevice         - A pointer to the PPPoE virtual device.
*
* OUTPUTS
*
*       UINT16          - The PPPoE session ID for this device.
*       0               - A connection could not been made.
*
************************************************************************/
STATUS PPEA_Wait_For_Client(DV_DEVICE_ENTRY *vdevice)
{
    STATUS      status;
    UINT32      session;
    
    /* Reserve the TCP_Resource semaphore for exclusive access to the
       list of PPPoE devices. */
    NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);
    
    /* Make sure that the device is available and set up for use
       by this process. */
    if (vdevice == NU_NULL)
    {
        /* Look for an available device, and set it up as an AC. */
        vdevice = PPE_Reserve_Device(PPE_AC_MODE);
    }
    else
    {
        /* Set up the given device as an AC. */
        PPE_Reset_State(vdevice, PPE_AC_MODE);
    }

    /* Done with access to the virtual devices. Release the semaphore. */
    NU_Release_Semaphore(&TCP_Resource);

    if (vdevice != NU_NULL)
    {
        /* Suspend here until a connection is established. */
        session = PPEA_Accept(vdevice);
        if (!session)
        {
            /* Get the network resource. */
            NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);
            
            /* Return the device to the pool so it can be used by
               another process. */
            PPE_Return_Device(vdevice);

            /* Done with access to the virtual devices. Release the semaphore. */
            NU_Release_Semaphore(&TCP_Resource);
            
#if (NU_DEBUG_PPE == NU_TRUE)        
            PPE_Printf("DEMOI_Wait_For_Client() - Error in PPPoE link negotiation.\r\n");
#endif
            status = NU_PPE_ERROR;
        }
        else
            status = NU_SUCCESS;
    }
    else
        status = NU_PPE_ERROR;

    /* At this point, either a physical connection has been established
       on the virtual device with a client host, or an error occurred and
       will return a null value. PPP negotiation can start after this function
       returns successfully. */

    return status;

} /* PPEA_Wait_For_Client */

