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

/***********************************************************************
*
*   FILE NAME
*
*       igmp_s.c
*
*   COMPONENT
*
*       IGMP - Internet Group Management Protocol
*
*   DESCRIPTION
*
*       This file contains the IGMP (Internet Group Management Protocol)
*       transmission module. This function will be used to transmit all
*       IGMP messages.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       IGMP_Send
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

#if ( (INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_IP_MULTICASTING == NU_TRUE) )

/*************************************************************************
*
*   FUNCTION
*
*       IGMP_Send
*
*   DESCRIPTION
*
*       Send an IGMP group membership report.
*
*   INPUTS
*
*       *ipm                    A pointer to the multicast group
*                               structure.
*       dest_addr               Destination IP address
*       message_to_send         Type of message that is to be sent
*       max_response_code       The Maximum Response Delay field is
*                               meaningful only in Query messages, and
*                               specifies the maximum allowed delay
*                               before sending a responding Report, in
*                               units of milliseconds.  In all other
*                               messages, it is set to zero by the
*                               sender and ignored by receivers.
*
*   OUTPUTS
*
*       NU_SUCCESS              The IGMP message was successfully sent
*       NU_NO_BUFFERS           No NET buffers available for sending the
*                                   message
*       NU_INVALID_PARM         One of the passed-in parameters was invalid
*
*************************************************************************/
STATUS IGMP_Send(IP_MULTI *ipm, UINT32 dest_addr, UINT8 message_to_send,
                 UINT8 max_response_code)
{
    NET_BUFFER              *buf_ptr = NU_NULL;
    IGMP_LAYER              *igmp;
    MULTI_SCK_OPTIONS       mopt;
    MULTI_DEV_STATE         *igmp_state = NU_NULL;
    STATUS                  status = NU_SUCCESS;
    DV_DEVICE_ENTRY         *device = ipm->ipm_data.multi_device;

#if (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV3_COMPATIBILITY)
    UINT16                  i;
    UINT8                   *record_ptr;
#endif

    if ( (device->dev_igmp_compat_mode == IGMPV1_COMPATIBILITY) ||
         (device->dev_igmp_compat_mode == IGMPV2_COMPATIBILITY) )
    {
        /* Allocate a buffer chain to build the IGMP report in. */
        buf_ptr = MEM_Buffer_Chain_Dequeue(&MEM_Buffer_Freelist,
                                           IGMP_HEADER_LEN);

        if (buf_ptr)
            /* Initialize the size of the data in this buffer. */
            buf_ptr->mem_total_data_len = buf_ptr->data_len = IGMP_HEADER_LEN;
        else
            status = NU_NO_BUFFERS;
    }

    else
    {
        /* Get the device state structure that matches the multicast
         * address */
        igmp_state = Multi_Get_Device_State_Struct(device, ipm->ipm_addr,
                                                   IP_ADDR_LEN);

        if (igmp_state != NU_NULL)
        {
            /* Allocate a buffer chain to build the IGMP report in. */
            buf_ptr =
                MEM_Buffer_Chain_Dequeue(&MEM_Buffer_Freelist,
                                         (IGMP_HEADER_LEN +
                                          IGMP_RECORD_HEADER_SIZE +
                                         (igmp_state->dev_num_src_to_report *
                                          IP_ADDR_LEN)));
            if (buf_ptr)
            {
                /* Initialize the size of the data in this buffer. */
                buf_ptr->mem_total_data_len = buf_ptr->data_len =
                    (IGMP_HEADER_LEN + IGMP_RECORD_HEADER_SIZE +
                    (igmp_state->dev_num_src_to_report * IP_ADDR_LEN));
            }
            else
                status = NU_NO_BUFFERS;
        }
        else
            status = NU_INVALID_PARM;
    }

    if (status == NU_SUCCESS)
    {
        /* Set the deallocation list pointer. */
        buf_ptr->mem_dlist = &MEM_Buffer_Freelist;

        /* Point to the location within the buffer where the IGMP header will
           be placed. */
        buf_ptr->data_ptr = buf_ptr->mem_parent_packet;

        /* Overlay the IGMP header. */
        igmp = (IGMP_LAYER *)buf_ptr->data_ptr;

        /* Initialize the IGMP header. */

        /* Add the report type to the header */
        if ( (message_to_send == IGMPV1_HOST_MEMBERSHIP_REPORT) ||
             (message_to_send == IGMPV2_HOST_MEMBERSHIP_REPORT) ||
             (message_to_send == IGMP_GROUP_LEAVE_MESSAGE) )
            PUT8(igmp, IGMP_TYPE_OFFSET, message_to_send);

        else
            PUT8(igmp, IGMP_TYPE_OFFSET, IGMPV3_HOST_MEMBERSHIP_REPORT);

        /* If we are sending a report, we need to set the sent_last_report
         * flag in the device so that we will know to send a leave
         * message if we leave the group
         */
        if (message_to_send != IGMP_GROUP_LEAVE_MESSAGE)
             ipm->ipm_data.multi_sent_last_report = NU_TRUE;

        /* Add the max response code to the header */
        if (message_to_send == IGMP_HOST_MEMBERSHIP_QUERY)
            PUT8(igmp, IGMP_MAX_RESP_CODE_OFFSET, max_response_code);

        else
            PUT8(igmp, IGMP_MAX_RESP_CODE_OFFSET, 0);

        /* Zero out the checksum field so that it can be correctly calculated */
        PUT16(igmp, IGMP_CKSUM_OFFSET, 0);

        /* For IGMPv3 reports, add the number of group records to be reported */
        if (device->dev_igmp_compat_mode == IGMPV3_COMPATIBILITY)
        {
            PUT16(igmp, IGMP_REPORT_RESERVED2_OFFSET, 0);

            /* Each multicast group report will be sent separately.
             * Therefore, each report will only contain one record.
             */
            PUT16(igmp, IGMP_REPORT_NUMBER_OF_GROUP_RECORDS, 1);
        }

        /* Add the multicast address to the buffer for IGMPv1 and IGMPv2
         * reports.
         */
        if ( (device->dev_igmp_compat_mode == IGMPV1_COMPATIBILITY) ||
             (device->dev_igmp_compat_mode == IGMPV2_COMPATIBILITY) )
            PUT32(igmp, IGMP_GROUP_OFFSET, GET32(ipm->ipm_addr, 0));

        /* Only IGMPv3 reports contain group records.  Format the group
         * record here
         */
#if (IGMP_DEFAULT_COMPATIBILTY_MODE == IGMPV3_COMPATIBILITY)
        else
        {
            /* Set a pointer to the location to start copying the group
             * records
             */
            record_ptr = ((UINT8 *)igmp + IGMP_REPORT_GROUP_RECORD);

            /* Construct each of the Group Records */
            PUT8(record_ptr, IGMP_RECORD_TYPE_OFFSET, message_to_send);

            PUT8(record_ptr, IGMP_RECORD_AUX_DATA_LENGTH_OFFSET, 0);

            PUT16(record_ptr, IGMP_RECORD_NUMBER_OF_SRC_ADDRS_OFFSET,
                  igmp_state->dev_num_src_to_report);

            /* Copy the multicast address into the record */
            PUT32(record_ptr, IGMP_RECORD_MULTICAST_ADDR_OFFSET,
                  GET32(ipm->ipm_addr, 0));

            /* Copy the source addresses to report into the packet */
            for (i = 0; i < igmp_state->dev_num_src_to_report; i++)
                memcpy((record_ptr + IGMP_RECORD_SOURCE_ADDR_OFFSET +
                        (i * IP_ADDR_LEN)),
                       (igmp_state->dev_src_to_report + (i * IP_ADDR_LEN)),
                       IP_ADDR_LEN);
        }
#endif

        /* Compute the checksum */
        PUT16(igmp, IGMP_CKSUM_OFFSET, TLS_IP_Check((VOID *)igmp,
              (UINT16)(buf_ptr->data_len / sizeof(UINT16))));

        /* Set up the multicast options that will be passed to the IP Layer */
        UTL_Zero(&mopt, sizeof(mopt));

        mopt.multio_device = ipm->ipm_data.multi_device;
        mopt.multio_ttl = 1;
        mopt.multio_loop = 0;

        /* If the report is IGMPv1 or IGMPv2, then the report will be sent
         * to the group address.
         */
        if ( (device->dev_igmp_compat_mode == IGMPV1_COMPATIBILITY) ||
             (device->dev_igmp_compat_mode == IGMPV2_COMPATIBILITY) )
        {
            /* Send the message */
            status = IP_Send(buf_ptr, NU_NULL, dest_addr, 0, 0, 1,
                             IP_IGMP_PROT, 0, &mopt);
        }

        else
        {
            /* General Queries will be sent to the all-systems multicast
             * address
             */
            status = IP_Send(buf_ptr, NU_NULL, IGMPV3_ALL_ROUTERS_GROUP,
                             0, 0, 1, IP_IGMP_PROT, 0, &mopt);
        }

        if (status != NU_SUCCESS)
        {
            /* The packet was not sent.  Deallocate the buffer.  If the
             * packet was transmitted it will be deallocated later by TCP.
             */
            MEM_One_Buffer_Chain_Free(buf_ptr, &MEM_Buffer_Freelist);
        }
    }

    return (status);

} /* IGMP_Send */

#endif
