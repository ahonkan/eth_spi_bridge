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
*   FILENAME
*
*       802_input.c
*
*   DESCRIPTION
*
*       This file implements the functions used by the Ethernet and
*       Wireless LAN to interpret data.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       EightZeroTwo_Input
*
*   DEPENDENCIES
*
*       nu_net.h
*       vlan_defs.h
*       802.h
*
*************************************************************************/

#include "networking/nu_net.h"
#include "networking/802.h"
#include "nucleus_gen_cfg.h"

#if ( (INCLUDE_VLAN == NU_TRUE) && (USE_SW_VLAN_METHOD) )
#include "networking/vlan_defs.h"
#endif

#if (INCLUDE_SNMP == NU_TRUE)
#include SNMP_GLUE
#endif

#ifdef CFG_NU_OS_NET_WPA_SUPP_SRC_ENABLE
extern VOID WPA_Supplicant_EAPOL_Rx(CHAR *src_addr, CHAR *dst_addr,
                            NET_BUFFER *buf_ptr, DV_DEVICE_ENTRY *device);
#endif

/* Declare the global function pointer for SPAN. When SPAN is initialized
   it will set this to point to the correct function. Otherwise this
   will point to zero and will not be used. */
extern UINT32 (*span_process_packet) (UINT8 *, UINT32, UINT32);

/* Declare the global function pointer for PPPoE. */
extern VOID   (*ppe_process_packet)(UINT16);

/*************************************************************************
*
*   FUNCTION
*
*       EightZeroTwo_Input
*
*   DESCRIPTION
*
*       Interpret the packet received on the basis of the protocol type
*       field.
*
*   INPUTS
*
*       *device             A pointer to the device on which the packet
*                           was received.
*       protocol_type       Type of the packet received.
*       *ether_pkt          A pointer to the incoming packet.
*
*   OUTPUTS
*
*       NU_SUCCESS          Successful
*
*************************************************************************/
STATUS EightZeroTwo_Input(DV_DEVICE_ENTRY *device, UINT16 protocol_type,
                          UINT8 *ether_pkt)
{

#if ( (INCLUDE_VLAN == NU_TRUE) && (USE_SW_VLAN_METHOD) )
    UINT16              getcode;
    DV_DEVICE_ENTRY     *vdevice;
#else
    UNUSED_PARAMETER(ether_pkt);
#endif

    switch (protocol_type)
    {             /* what to do with it? */
#if (INCLUDE_IPV4 == NU_TRUE)
        case EIP:

            /* This is an IP packet. */
            IP_Interpret((IPLAYER *)MEM_Buffer_List.head->data_ptr,
                         device, MEM_Buffer_List.head);

            break;

        case EARP:
        case ERARP:

            /* This is an ARP packet */
            ARP_Interpret((ARP_LAYER *)MEM_Buffer_List.head->data_ptr,
                          device);

            /* We are finished with the ARP packet. */
            /* Drop the packet by placing it back on the buffer_freelist. */
            MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);
            break;
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
        case EIP6:

            /* This is an IP packet. */
            IP6_Interpret((IP6LAYER *)MEM_Buffer_List.head->data_ptr,
                          device, MEM_Buffer_List.head);

            break;
#endif

#if ( (INCLUDE_VLAN == NU_TRUE) && (USE_SW_VLAN_METHOD) )

        case EVLAN:

            /* Search linked list of real devices for any VLAN devices
             * attached to this device
             */
            vdevice = VLAN_Search_VID((UINT16)((GET16(ether_pkt, VLAN_TAG_OFFSET)) & 0x0FFF),
                                      device);

            if (vdevice)
            {
                /* VLAN packets have additional bytes in the ENET header that
                 * must be removed.
                 */
                MEM_Buffer_List.head->data_ptr           += VLAN_HEADER_SIZE;
                MEM_Buffer_List.head->data_len           -= VLAN_HEADER_SIZE;
                MEM_Buffer_List.head->mem_total_data_len -= VLAN_HEADER_SIZE;

                /* Set a pointer to the interface. */
                MEM_Buffer_List.head->mem_buf_device = vdevice;

                /* use virtual device pointer returned in VLAN_Search_VID */

                /* Now read the real packet type to determine how to process this packet */
                getcode = GET16(ether_pkt, ETHER_TYPE_OFFSET + VLAN_HEADER_SIZE);

#if (INCLUDE_IPV4 == NU_TRUE)
                if (getcode == EARP)
                {
                    /* This is an ARP packet */
                    ARP_Interpret((ARP_LAYER *)MEM_Buffer_List.head->data_ptr,
                                  vdevice);

                    /* We are finished with the ARP packet. */
                    /* Drop the packet by placing it back on the buffer_freelist. */
                    MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);
                }

                if (getcode == EIP)
                {
                    /* This is an IP packet */
                    IP_Interpret((IPLAYER *)MEM_Buffer_List.head->data_ptr,
                                 vdevice, MEM_Buffer_List.head);
                }
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
                if (getcode == EIP6)
                {
                    /* This is an IP packet. */
                    IP6_Interpret((IP6LAYER *)MEM_Buffer_List.head->data_ptr,
                                  vdevice, MEM_Buffer_List.head);
                }
#endif
            }
            else
            {
                /* ADD MIB STATISTICS HERE FOR DROPPED VLAN PACKETS */

                /* Drop the packet by placing it back on the buffer_freelist. */
                MEM_Buffer_Chain_Free (&MEM_Buffer_List, &MEM_Buffer_Freelist);
            }

            break;
#endif

        case PPPOED:
        case PPPOES:

            /* If PPPoE has been initialized, then ppe_process_packet will have
               been set to the handler function. */
            if (ppe_process_packet != NU_NULL)
            {
                ppe_process_packet(protocol_type);
            }
            else
            {
                /* Log the unknown protocol. */
                MIB2_ifInUnknownProtos_Inc(device);

                /* Drop the packet by placing it back on the buffer_freelist. */
                MEM_Buffer_Chain_Free (&MEM_Buffer_List, &MEM_Buffer_Freelist);

            }

            break;

#ifdef CFG_NU_OS_NET_WPA_SUPP_SRC_ENABLE

        case DOT1X:

            /* Capture the EAPOL packets. */
            WPA_Supplicant_EAPOL_Rx((CHAR *)(ether_pkt + ETHER_ME_OFFSET),
                                (CHAR *)(ether_pkt + ETHER_DEST_OFFSET),
                                MEM_Buffer_List.head, device);

            break;

#endif

        default:

            /* If SPAN has been initialized this function pointer will
               be setup. Therefore we need to check to see if this
               packet is a SPAN type packet. */
            if (span_process_packet != NU_NULL)
            {

                if ( (protocol_type == SPAN_CONFIG) || (protocol_type == SPAN_TOP_CHG) )
                {

                    /* Point to the packet. */
                    MEM_Buffer_List.head->data_ptr -= device->dev_hdrlen;
                    MEM_Buffer_List.head->data_len += device->dev_hdrlen;
                    MEM_Buffer_List.head->mem_total_data_len +=
                                                        device->dev_hdrlen;

                    span_process_packet((VOID *)MEM_Buffer_List.head->data_ptr,
                                        device->dev_index,
                                        (UINT32)protocol_type);
                }

            }
            else
            {
                /* Log the unknown protocol. */
                MIB2_ifInUnknownProtos_Inc(device);
            }

            /* Drop the packet by placing it back on the buffer_freelist. */
            MEM_Buffer_Chain_Free (&MEM_Buffer_List, &MEM_Buffer_Freelist);

            break;

    } /* end switch */

    return (NU_SUCCESS);

} /* EightZeroTwo_Input */
