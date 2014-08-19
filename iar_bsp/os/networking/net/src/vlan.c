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

/**************************************************************************
*
*   FILENAME
*
*       vlan.c
*
*   DESCRIPTION
*
*       This include file will contains VLAN functions.
*
*   FUNCTIONS
*
*       VLAN_Initialize
*       VLAN_Ether_Send
*       VLAN_Search_VID
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       nu_net.h
*       vlan_defs.h
*       802.h
*
****************************************************************************/

#include "networking/nu_net.h"
#include "networking/vlan_defs.h"
#include "networking/802.h"

#if ( (INCLUDE_VLAN == NU_TRUE) && (USE_SW_VLAN_METHOD == NU_TRUE) )

/************************************************************************
*
*   FUNCTION
*
*       VLAN_Initialize
*
*   DESCRIPTION
*
*       This function
*
*   INPUTS
*
*       device
*
*   OUTPUTS
*
*       status
*
*************************************************************************/
STATUS VLAN_Initialize (DV_DEVICE_ENTRY *device)
{
    STATUS              status = NU_SUCCESS;
    DV_DEVICE_ENTRY     *hw_device;
    VLAN_INIT_DATA      *init_data;

    /* Get the VLAN initialization information setup by the application */
    init_data = (VLAN_INIT_DATA *)(device->dev_driver_options);

    /* Get the hardware device this VLAN device will tx and rx through */
    hw_device = DEV_Get_Dev_By_Name((const char *)(init_data->vlan_device));

    /* Verify that all needed information is valid */
    if((hw_device == NU_NULL) || (init_data->vlan_vid > VLAN_VID_CHECK))
    {
       status = NU_INVALID_PARM;
    }
    else
    {
        /* Fill in the device table */
        device->dev_output     = VLAN_Ether_Send;
        device->dev_input      = ETH_Ether_Input;
        device->dev_start      = hw_device->dev_start;
        device->dev_receive    = hw_device->dev_receive;
        device->dev_addrlen    = hw_device->dev_addrlen;
        device->dev_hdrlen     = hw_device->dev_hdrlen + VLAN_HEADER_SIZE;
        device->dev_mtu        = (ETHERNET_MTU - VLAN_HEADER_SIZE);

        /* Because the Ioctl function accesses the real ethernet driver the */
        /* virtual device must get a copy the actual Shared Memory and IO   */
        /* space memory pointers.                                           */

        device->dev_ioctl      = hw_device->dev_ioctl;
        device->dev_sm_addr    = hw_device->dev_sm_addr;
        device->dev_io_addr    = hw_device->dev_io_addr;


        device->dev_type       = DVT_VLAN;

#if (USE_SW_VLAN_METHOD == NU_TRUE)
        /* Setup specific VLAN information */
        device->dev_vlan_vid    = init_data->vlan_vid;
        device->dev_vlan_associated_device = hw_device;
#endif

        /* Copy real Ethernet device's MAC address */

        device->dev_mac_addr[0] = hw_device->dev_mac_addr[0];
        device->dev_mac_addr[1] = hw_device->dev_mac_addr[1];
        device->dev_mac_addr[2] = hw_device->dev_mac_addr[2];
        device->dev_mac_addr[3] = hw_device->dev_mac_addr[3];
        device->dev_mac_addr[4] = hw_device->dev_mac_addr[4];
        device->dev_mac_addr[5] = hw_device->dev_mac_addr[5];

        /* This Virtual device is dependent upon the Real ethernet device */
        /* so assign real device flags to the virtual device.             */

        device->dev_flags |= hw_device->dev_flags;

        device->dev_flags |= DV_UP;   /* Set the device is UP flag */

    }

    return(status);
}

/*************************************************************************
*
*   FUNCTION
*
*       VLAN_Ether_Send
*
*   DESCRIPTION
*
*       This function is the hardware layer transmission function for
*       VLAN.  This function is an exact duplicate of ETH_Ether_Send
*       with the exception of the additional 4 bytes necessary for the
*       VLAN tags.  It was thought that duplicating the code would be
*       better than slowing each transmission with a check whether the
*       packet was VLAN or not.
*
*   INPUTS
*
*       *buf_ptr                Pointer to the net buffer
*       *device                 Pointer to the device
*       *dest                   Pointer to the destination to send to
*       *ro                     Pointer to the routing information
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful
*       NU_HOST_UNREACHABLE     Host cannot be reached, wrong information
*       -1                      Invalid socket family
*
*************************************************************************/
STATUS VLAN_Ether_Send(NET_BUFFER *buf_ptr, DV_DEVICE_ENTRY *device,
                      VOID *dest, VOID *ro)
{
#ifndef PACKET
    INT                 old_int;
#endif
    UINT8               mac_dest[DADDLEN];
    UINT16              type;
    STATUS              status;
    UINT8               vlp;
    UINT16              tag;
    DV_DEVICE_ENTRY     *real_device;   /* Real HW device associated with virtual device */

    /* Verify that the device is up. */
    if ( (device->dev_flags & (DV_UP | DV_RUNNING)) != (DV_UP | DV_RUNNING) )
        return (NU_HOST_UNREACHABLE);

    status = EightZeroTwo_Output(buf_ptr, device, dest, ro, mac_dest, &(type));

    if (status == NU_SUCCESS)
    {
        /* Move back the data pointer to make room for the MAC and VLAN layer */
        buf_ptr->data_ptr           -= device->dev_hdrlen;
        buf_ptr->data_len           += device->dev_hdrlen;
        buf_ptr->mem_total_data_len += device->dev_hdrlen;

        /* Initialize the ethernet header. */

        PUT_STRING(buf_ptr->data_ptr, ETHER_DEST_OFFSET, mac_dest, DADDLEN);
        PUT_STRING(buf_ptr->data_ptr, ETHER_ME_OFFSET, device->dev_mac_addr, DADDLEN);
        PUT16(buf_ptr->data_ptr, ETHER_TYPE_OFFSET, VLAN_TYPE);

        /* Get the Type of Service field from the IP TOS field set at socket creation time */

        /* IP layer is now offset by VLAN_HEADER_SIZE (4) bytes as shown below:            */

        /* ETHERNET HEADER with VLAN FORMAT          ETHERNET HEADER FORMAT   */
        /* [DEST ADDRESS] bytes 00..05               [DEST ADDRESS] bytes 00..05  */
        /* [SRC  ADDRESS] bytes 06..11               [SRC  ADDRESS] bytes 06..11  */
        /* [VLAN TYPE   ] bytes 12..13               [PKT  TYPE   ] bytes 12..13  */
        /* [VLAN TAG    ] bytes 14..15                                            */
        /* [PKT  TYPE   ] bytes 16..17                                            */

        /* Set the VLAN priority. */
        vlp = device->dev_vlan_vprio;

#if (VLAN_CANONICAL_FORM == NU_TRUE)

        tag = 0;   /* leave Canonical Format Indication (CFI) bit 12 set to 0 */

#else

        tag = (1 << 12);  /* set CFI bit 12 to a 1 */

#endif

        /* Now set the VLP and VID subfields in the VLAN tag field */

        tag |= ((vlp << 13) | (device->dev_vlan_vid));

        /* Insert the VLAN tag */
        PUT16(buf_ptr->data_ptr, VLAN_TAG_OFFSET, tag);

        /* Add the original type value moved down by VLAN_HEADER_SIZE bytes */
        PUT16(buf_ptr->data_ptr, ETHER_TYPE_OFFSET + VLAN_HEADER_SIZE, type);

        /* Is this a unicast or a non-unicast packet. */
        if ( (buf_ptr->mem_flags & NET_BCAST) || (buf_ptr->mem_flags & NET_MCAST) )
            MIB2_ifOutNUcastPkts_Inc(device);
        else
            MIB2_ifOutUcastPkts_Inc(device);

    /*** all transmission requests MUST use the real hardware device pointer ***/

        real_device = device->dev_vlan_associated_device;

#ifdef PACKET

        /* Pass the packet to the device. */
        status = device->dev_start(real_device, buf_ptr);

        if (status != NU_SUCCESS)
            NLOG_Error_Log("Device failed to transmit packet", NERR_SEVERE,
                            __FILE__, __LINE__);

        /* Free the buffers onto the appropriate free lists */
        else
            MEM_Multiple_Buffer_Chain_Free(buf_ptr);

#else

        old_int = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

        /* Place the buffer on the REAL device's transmit queue. */
        MEM_Buffer_Enqueue(&real_device->dev_transq, buf_ptr);

        /* Increment the number of packet on the outgoing queue */
        ++real_device->dev_transq_length;
        
        /* Trace log */
        T_DEV_TRANSQ_LEN(real_device->dev_net_if_name, real_device->dev_transq_length);

        /* If this is the first buffer in the transmit queue we need to resume the
         * transmit task.  If not another packet is already being transmitted. */
        if (real_device->dev_transq.head == buf_ptr)
        {
            NU_Local_Control_Interrupts(old_int);

            /* Resume the the transmit task. */
            status = device->dev_start(real_device, buf_ptr);

            /* If the buffer could not be transmitted, remove it from the transmit
             * queue.
             */
            if (status != NU_SUCCESS)
            {
                MEM_Buffer_Dequeue(&real_device->dev_transq);

                NLOG_Error_Log("Driver failed to transmit packet", NERR_SEVERE,
                               __FILE__, __LINE__);
            }
        }
        else
        {
            NU_Local_Control_Interrupts(old_int);
            status = NU_SUCCESS;
        }

#endif
    }

    else
    {
        if (status == NU_UNRESOLVED_ADDR)
        {
            /* To the upper layer protocols this is the same as a
               successful send. */
            status = NU_SUCCESS;
        }
    }

    return (status);

} /* VLAN_Ether_Send */

/************************************************************************
*
*   FUNCTION
*
*       VLAN_Search_VID
*
*   DESCRIPTION
*
*       This function finds the VLAN device associated with the real
*       Ethernet device.  The VLAN group ID for the real device is
*       compared to the VLAN group ID extracted from the incoming packet
*       and the result of this comparison is returned
*
*   INPUTS
*       vt           incoming packet VLAN group ID
*       device       pointer to real Ethernet device
*
*   OUTPUTS
*
*       A pointer to the device or NU_NULL if no match is found.
*
*************************************************************************/
DV_DEVICE_ENTRY *VLAN_Search_VID(UINT16 vt,
                                 const DV_DEVICE_ENTRY *hw_device)
{
    DV_DEVICE_ENTRY *devptr;

    /* set working device pointer to first device in linked list */
    devptr = DEV_Table.dv_head;

    while (devptr)
    {
        /* If this device has a VLAN device association, this is
         * the actual device that received the packet and the
         * incoming VID matches the VID for this device.
         */
        if ( (devptr->dev_type == DVT_VLAN) &&
             (devptr->dev_vlan_associated_device == hw_device) &&
             (vt == devptr->dev_vlan_vid) )
        {
            break;
        }

        /* get the next device from the list */
        devptr = devptr->dev_next;

    } /* end while loop */

    return (devptr);
}

#endif
