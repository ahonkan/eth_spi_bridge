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
*   FILE NAME
*
*       ldc.c
*
*   COMPONENT
*
*       LD - Loopback Device
*
*   DESCRIPTION
*
*       This module supplies the functionality needed to implement a
*       loopback device. It is responsible for looping all transmitted
*       packets back up the stack. It also provides an Ioctl function
*       to be used by the stack for multicasting support. This way
*       multicast packets can be looped back too.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       LDC_Init
*       LDC_TX_Packet
*       LDC_Ioctl
*
*   DEPENDENCIES
*
*       nu_net.h
*       ld_defs.h
*       ld_extr.h
*
*************************************************************************/

#include "networking/nu_net.h"
#include "networking/ld_defs.h"
#include "networking/ld_extr.h"

#if (INCLUDE_SNMP == NU_TRUE)
#include SNMP_GLUE
#endif

/*************************************************************************
*
*   FUNCTION
*
*       LDC_Init
*
*   DESCRIPTION
*
*       Initializes the loopback device. Sets up the correct values in
*       the device structure, including the function pointers that hook
*       this driver to the stack.
*
*   INPUTS
*
*       *dev_ptr                Pointer to the device structure for the
*                               loopback device.
*
*   OUTPUTS
*
*       NU_SUCCESS
*
************************************************************************/
STATUS LDC_Init(DV_DEVICE_ENTRY *dev_ptr)
{
    /* Fill in device specific information. */
    dev_ptr->dev_type       = DVT_LOOP;
    dev_ptr->dev_addrlen    = LD_ADDRESS_LENGTH;
    dev_ptr->dev_hdrlen     = LD_HEADER_LENGTH;
    dev_ptr->dev_mtu        = LD_MTU;

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)
    dev_ptr->dev_flags      |= (DV_MULTICAST);

#if (INCLUDE_IPV6 == NU_TRUE)
    /* The loopback device does not need to perform Duplicate Address
     * Detection on the IPv6 link-local address.
     */
    dev_ptr->dev_flags      |= (DV6_NODAD);
#endif

#endif

    /* The vector is not important for the loopback device
       since it does not generate interrupts. It does, though,
       need to be different from any other registered NIC so that
       they can be independently identified. Here we will just make
       it a large value that should not make sense for any interrupt
       controller.
    */
    dev_ptr->dev_vect       = (UINT16)LD_INVALID_VECTOR;

    /* Initialize the functions pointers that link the stack
       and the driver together. Since this is a loop back device
       we do not make use of all of these. */
    dev_ptr->dev_start      = LDC_TX_Packet;

    dev_ptr->dev_ioctl      = LDC_Ioctl;
    dev_ptr->dev_output     = ETH_Ether_Send;
    dev_ptr->dev_input      = ETH_Ether_Input;

    /* Init the basic interface information. */
    MIB2_ifDescr_Set(dev_ptr, "Nucleus NET Software Loopback Interface");
    MIB2_ifType_Set(dev_ptr, 24);
    MIB2_ifSpeed_Set(dev_ptr, 10000000UL);

    return (NU_SUCCESS);

} /* LDC_Init */

/*************************************************************************
*
*   FUNCTION
*
*       LDC_TX_Packet
*
*   DESCRIPTION
*
*       Makes a copy of the packet to be transmitted and places it onto
*       the receive packet list. Then set an event to tell the stack that
*       a packet has arrived.
*
*   INPUTS
*
*       *dev_ptr                Pointer to the device structure used to
*                               send the packet.
*       *buf_ptr                Pointer to the buffer that holds the packet
*                               to send.
*
*   OUTPUTS
*
*       NU_SUCCESS              The packet was sent.
*       NU_NO_BUFFERS           There were not enough buffer to complete
*                               the transmission.
*
*************************************************************************/
STATUS LDC_TX_Packet(DV_DEVICE_ENTRY *dev_ptr, NET_BUFFER *buf_ptr)
{
    NET_BUFFER  *dest_buf_ptr;
    STATUS      ret_status = NU_SUCCESS;

    /* Allocate some buffers to hold this packet. */
    dest_buf_ptr = MEM_Buffer_Chain_Dequeue(&MEM_Buffer_Freelist,
                                            (INT32)buf_ptr->mem_total_data_len);

    /* Make sure we got the buffers. */
    if (dest_buf_ptr)
    {
        /* Setup the data pointer. */
        dest_buf_ptr->data_ptr = dest_buf_ptr->mem_parent_packet;

        /* Copy the packet to the destination buffers. */
        MEM_Chain_Copy(dest_buf_ptr, buf_ptr, 0, (INT32)buf_ptr->mem_total_data_len);

        /* The number of octets sent and received on this interface are the same. */
        MIB2_ifInOctets_Add(dev_ptr, buf_ptr->mem_total_data_len);
        MIB2_ifOutOctets_Add(dev_ptr, buf_ptr->mem_total_data_len);
    }
    else
        MIB2_ifOutDiscards_Inc(dev_ptr);

#ifndef PACKET

    /* Remove the packet from the devices transmit queue. */
    MEM_Buffer_Dequeue(&dev_ptr->dev_transq);

    /* Decrement the queue length counter. */
    --dev_ptr->dev_transq_length;
    
    /* Trace log */
    T_DEV_TRANSQ_LEN(dev_ptr->dev_net_if_name, dev_ptr->dev_transq_length);

    /* Free the buffers onto the appropriate free lists */
    MEM_Multiple_Buffer_Chain_Free(buf_ptr);

#endif

    /* Again check for success of the allocation above. This if
       statement is broken like this because the #ifdef'd code
       above needs to be executed even if the buffer allocation
       failed. */
    if (dest_buf_ptr)
    {
        /* Set the device for this just "received" packet. */
        dest_buf_ptr->mem_buf_device = dev_ptr;

        /* And the total data length. */
        dest_buf_ptr->mem_total_data_len = buf_ptr->mem_total_data_len;

        /* Put the "received" packet onto the receive buffer list. */
        MEM_Buffer_Enqueue(&MEM_Buffer_List, dest_buf_ptr);

        /* Set the event to tell the stack that a packet has arrived. */
        NU_Set_Events(&Buffers_Available, 2, NU_OR);
    }
    else
    {
        /* Log the error and return an error. */
        NLOG_Error_Log ("Unable to acquire MEM buffer from the freelist", NERR_SEVERE,
                            __FILE__, __LINE__);

        ret_status = NU_NO_BUFFERS;
    }

    return (ret_status);

} /* LDC_TX_Packet */

/*************************************************************************
*
*   FUNCTION
*
*       LDC_Ioctl
*
*   DESCRIPTION
*
*       This function processes IOCTL requests to the loopback driver.
*       Right now this only includes requests for multicasting support.
*
*   INPUTS
*
*       *dev                    Pointer to the device for this request.
*       option                  Which option to do.
*       *d_req                  Pointer to the request packet to send.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation
*       NU_INVAL                Failure
*
************************************************************************/
STATUS LDC_Ioctl(DV_DEVICE_ENTRY *dev, INT option, DV_REQ *d_req)
{
    STATUS  ret_status;

#if (INCLUDE_IP_MULTICASTING != NU_TRUE)
    UNUSED_PARAMETER(dev);
    UNUSED_PARAMETER(d_req);
#endif

    switch (option)
    {
#if (INCLUDE_IP_MULTICASTING == NU_TRUE)
        case DEV_ADDMULTI :

            /* Join the ethernet multicast group. */
            ret_status = ETH_Add_Multi(dev, d_req);

           /* A status of NU_RESET means the operation was a success and as a
              result the ethernet setup frame needs to be reset. Since this
              is a loopback device there is no receive filtering to update.
              Set the status to NU_SUCCESS and return. */
            if (ret_status == NU_RESET)
                ret_status = NU_SUCCESS;

            break;

        case DEV_DELMULTI :

            /* Join the ethernet multicast group. */
            ret_status = ETH_Del_Multi(dev, d_req);

            /* A status of NU_RESET means the operation was a success and as a
              result the ethernet setup frame needs to be reset. Since this
              is a loopback device there is no receive filtering to update.
              Set the status to NU_SUCCESS and return. */
            if (ret_status == NU_RESET)
                ret_status = NU_SUCCESS;

            break;
#endif

        default :

            ret_status = NU_INVAL;

            break;

    }

    return (ret_status);

} /* LDC_Ioctl */
