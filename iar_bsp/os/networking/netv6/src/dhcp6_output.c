/*************************************************************************
*
*              Copyright 2002 Mentor Graphics Corporation
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
*       dhcp6_output.c
*
*   DESCRIPTION
*
*       This file contains functions necessary for transmitting outgoing
*       DHCPv6 packets on the client.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       DHCP6_Build_Packet
*       DHCP6_Add_User_Options
*       DHCP6_Output
*       DHCP6_Add_System_IA_NA_Option
*       DHCP6_Find_Server
*       DHCP6_Solicit_Output
*       DHCP6_Confirm_Addresses
*       DHCP6_Confirm_Output
*       DHCP6_Renew_Addresses
*       DHCP6_Renew_Output
*       DHCP6_Rebind_Addresses
*       DHCP6_Rebind_Output
*       DHCP6_Obtain_Address
*       DHCP6_Request_Output
*       DHCP6_Obtain_Config_Info
*       DHCP6_Info_Request_Output
*       DHCP6_Release_Output
*       DHCP6_Release_Address
*       DHCP6_Decline_Output
*       DHCP6_Free_Address
*       DHCP6_Decline_Address
*       DHCP6_Compute_Retransmission_Time
*       DHCP6_Obtain_TX_Struct
*
*   DEPENDENCIES
*
*       target.h
*       externs.h
*       cmsg_defs.h
*       externs6.h
*       dhcp6.h
*
**************************************************************************/

#include "networking/target.h"
#include "networking/externs.h"
#include "networking/cmsg_defs.h"
#include "networking/externs6.h"
#include "networking/dhcp6.h"

#if (INCLUDE_DHCP6 == NU_TRUE)

/* Do not change the values in this array.  It is pre-loaded with numbers
 * that will enable the module to create a random number between 0.01 and 0.09.
 */
UINT8   DHCP6_Rand_Array[9] = {100, 50, 33, 25, 20, 16, 14, 12, 11};

CHAR    *DHCP6_Trans_ID_Value = DHCP6_TRANS_ID_START;
UINT8   DHCP6_Tx_Id = 1;

STATIC STATUS DHCP6_Compute_Retransmission_Time(DHCP6_TX_STRUCT *, DHCP6_STRUCT *);
STATIC STATUS DHCP6_Output(UINT8 *, INT, DHCP6_STRUCT *, DHCP6_TX_STRUCT *);
STATIC VOID DHCP6_Build_Packet(UINT8 *, DHCP6_TX_STRUCT *);
STATIC UINT16 DHCP6_Add_User_Options(UINT8 *, DHCP6_STRUCT *, DHCP6_TX_STRUCT *,
                                     UINT8);
STATIC UINT16 DHCP6_Add_System_IA_NA_Option(UINT8 *, DHCP6_STRUCT *, UINT32);

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Build_Packet
*
*   DESCRIPTION
*
*       This routine builds the common portions of an outgoing DHCPv6
*       client packet.
*
*   INPUTS
*
*       *dhcp_buffer            A pointer to the buffer in which to build
*                               the packet.
*       *tx_ptr                 A pointer to the transmission structure
*                               associated with the outgoing packet.
*
*   OUTPUTS
*
*       None.
*
**************************************************************************/
STATIC VOID DHCP6_Build_Packet(UINT8 *dhcp_buffer, DHCP6_TX_STRUCT *tx_ptr)
{
    /* Add the msg_type to the packet. */
    PUT8(dhcp_buffer, DHCP6_MSG_TYPE_OFFSET, tx_ptr->dhcp6_type);

    /* RFC 3315 - section 15.1 - A client MUST leave the transaction ID
     * unchanged in retransmissions of a message.
     */
    memcpy(&dhcp_buffer[DHCP6_TRANS_ID_OFFSET], tx_ptr->dhcp6_xid,
           DHCP6_ID_LEN);

} /* DHCP6_Build_Packet */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Add_User_Options
*
*   DESCRIPTION
*
*       This routine adds the user options outlined below to the buffer as
*       allowed by RFC 3315.  The options were built by the user.
*
*       Per Appendix A of RFC 3315, the following options are allowable in
*       the respective packet type:
*
*               Solicit Request Confirm Renew Rebind Decline Release Inform
*               -----------------------------------------------------------
*   ClientID       *       *       *      *      *      *       *      *
*   OptRequest     *       *       *      *      *      *       *      *
*   Time           *       *       *      *      *      *       *      *
*   Auth           *       *       *      *      *      *       *      *
*   UserClass      *       *       *      *      *      *       *      *
*   VendorClass    *       *       *      *      *      *       *      *
*   VendSpec       *       *       *      *      *      *       *      *
*   ReconAccept    *       *              *      *                     *
*
*   INPUTS
*
*       *dhcp_buffer            Pointer to the buffer into which to add
*                               the options.
*       *ds_ptr                 Pointer to DHCP structure associated with
*                               the message.
*       *tx_ptr                 Pointer to the DHCPv6 transmission structure
*                               associated with this transaction.
*       message_type            The type of message being built.
*
*   OUTPUTS
*
*       The number of bytes added to the packet.
*
**************************************************************************/
STATIC UINT16 DHCP6_Add_User_Options(UINT8 *dhcp_buffer, DHCP6_STRUCT *ds_ptr,
                                     DHCP6_TX_STRUCT *tx_ptr,
                                     UINT8 message_type)
{
    UINT16  opt_type, i, opt_len, nbytes = 0, valid_opts = 0;

    /* All DHCPv6 client messages include a Client ID option. */
    nbytes += DHCP6_Build_Client_ID_Option(&dhcp_buffer[nbytes]);

    /* A client MUST include an Elapsed Time option in messages to indicate
     * how long the client has been trying to complete a DHCP message
     * exchange.
     */
    nbytes += DHCP6_Build_Elapsed_Time_Option(&dhcp_buffer[nbytes], tx_ptr);

    if (ds_ptr->dhcp6_client_opts[DHCP6_OPT_ELAPSED_TIME].dhcp6_opt_addr != 0xffffffff)
    {
        /* Increment the length of the buffer. */
        nbytes += DHCP6_ELAPSED_TIME_OPT_LEN;
    }

    /* A Reconfigure Accept option is allowed in all packets except Confirm,
     * Decline and Release.  If the user/ has included a Reconfigure Accept
     * option, add it to the packet.
     */
    if ( (message_type != DHCP6_CONFIRM) &&
         (message_type != DHCP6_DECLINE) &&
         (message_type != DHCP6_RELEASE) &&
         (ds_ptr->dhcp6_client_opts[DHCP6_OPT_RECONF_ACCEPT].dhcp6_opt_addr
          != 0xffffffff) )
    {
        /* Copy the option into the packet. */
        NU_BLOCK_COPY(&dhcp_buffer[nbytes],
                      &ds_ptr->dhcp6_opts[ds_ptr->dhcp6_client_opts
                      [DHCP6_OPT_RECONF_ACCEPT].dhcp6_opt_addr],
                      DHCP6_RCNFGR_ACC_OPT_LEN);

        /* Increment the length of the buffer. */
        nbytes += DHCP6_RCNFGR_ACC_OPT_LEN;

        /* Add the flag indicating that this client will accept reconfigure
         * messages.
         */
        ds_ptr->dhcp6_flags |= DHCP6_RECON_ACCEPT;
    }

    /* A User Class option is allowed in all packet types.  If the user has
     * included a User Class option, add it to the packet.
     */
    if (ds_ptr->dhcp6_client_opts[DHCP6_OPT_USER_CLASS].dhcp6_opt_addr != 0xffffffff)
    {
        /* Copy the option into the packet. */
        NU_BLOCK_COPY(&dhcp_buffer[nbytes],
                      &ds_ptr->dhcp6_opts[ds_ptr->dhcp6_client_opts
                      [DHCP6_OPT_USER_CLASS].dhcp6_opt_addr],
                      DHCP6_USER_CLASS_OPT_LEN +
                      GET16(&ds_ptr->dhcp6_opts[ds_ptr->dhcp6_client_opts
                      [DHCP6_OPT_USER_CLASS].dhcp6_opt_addr], DHCP6_OPT_LEN_OFFSET));

        /* Increment the length of the buffer. */
        nbytes += (DHCP6_USER_CLASS_OPT_LEN +
                   GET16(&ds_ptr->dhcp6_opts[ds_ptr->dhcp6_client_opts
                         [DHCP6_OPT_USER_CLASS].dhcp6_opt_addr],
                         DHCP6_OPT_LEN_OFFSET));
    }

    /* A Vendor Class option is allowed in all packet types.  If the user
     * has included a Vendor Class option, add it to the packet.
     */
    if (ds_ptr->dhcp6_client_opts[DHCP6_OPT_VENDOR_CLASS].dhcp6_opt_addr != 0xffffffff)
    {
        /* Copy the option into the packet. */
        NU_BLOCK_COPY(&dhcp_buffer[nbytes],
                      &ds_ptr->dhcp6_opts[ds_ptr->dhcp6_client_opts
                      [DHCP6_OPT_VENDOR_CLASS].dhcp6_opt_addr],
                      DHCP6_VENDOR_CLASS_OPT_LEN +
                      GET16(&ds_ptr->dhcp6_opts[ds_ptr->dhcp6_client_opts
                      [DHCP6_OPT_VENDOR_CLASS].dhcp6_opt_addr],
                      DHCP6_OPT_LEN_OFFSET));

        /* Increment the length of the buffer. */
        nbytes += (DHCP6_VENDOR_CLASS_OPT_LEN +
            GET16(&ds_ptr->dhcp6_opts[ds_ptr->dhcp6_client_opts
                  [DHCP6_OPT_VENDOR_CLASS].dhcp6_opt_addr], DHCP6_OPT_LEN_OFFSET));
    }

    /* A Vendor Specific Info option is allowed in all packet types.  If the
     * user has included a Vendor Specific Info option, add it to the packet.
     */
    if (ds_ptr->dhcp6_client_opts[DHCP6_OPT_VENDOR_OPTS].dhcp6_opt_addr != 0xffffffff)
    {
        /* Copy the option into the packet. */
        NU_BLOCK_COPY(&dhcp_buffer[nbytes],
                      &ds_ptr->dhcp6_opts[ds_ptr->dhcp6_client_opts
                      [DHCP6_OPT_VENDOR_OPTS].dhcp6_opt_addr],
                      DHCP6_VENDOR_SPEC_OPT_LEN +
                      GET16(&ds_ptr->dhcp6_opts[ds_ptr->dhcp6_client_opts
                      [DHCP6_OPT_VENDOR_OPTS].dhcp6_opt_addr],
                      DHCP6_OPT_LEN_OFFSET));

        /* Increment the length of the buffer. */
        nbytes += (DHCP6_VENDOR_SPEC_OPT_LEN +
            GET16(&ds_ptr->dhcp6_opts[ds_ptr->dhcp6_client_opts
                  [DHCP6_OPT_VENDOR_OPTS].dhcp6_opt_addr],
                  DHCP6_OPT_LEN_OFFSET));
    }

    /* An Option Request option is allowed in all packet types.  If the
     * user has included an Option Request option, add it to the packet.
     */
    if (ds_ptr->dhcp6_client_opts[DHCP6_OPT_ORO].dhcp6_opt_addr != 0xffffffff)
    {
        /* Extract the total length of the options included in the Option
         * Request Option.
         */
        opt_len = GET16(&ds_ptr->dhcp6_opts[ds_ptr->dhcp6_client_opts
                        [DHCP6_OPT_ORO].dhcp6_opt_addr],
                        DHCP6_OPT_LEN_OFFSET);

        /* Traverse the list of options, adding all valid options to the
         * packet.
         */
        for (i = 0; i < opt_len; i += 2)
        {
            /* Get the option code from the packet. */
            opt_type = GET16(&ds_ptr->dhcp6_opts[ds_ptr->dhcp6_client_opts
                             [DHCP6_OPT_ORO].dhcp6_opt_addr],
                             DHCP6_ORO_CODE_OFFSET + i);

            /* If the server indicated to include this option, but the client
             * is already including it, remove it from the options that the
             * server has indicated to include.
             */
            if ( (opt_type < DHCP6_MAX_OPT_CODES) &&
                 (ds_ptr->dhcp6_system_opts[opt_type]) )
            {
                ds_ptr->dhcp6_system_opts[opt_type] = 0;
            }

            /* If this option is valid in this message type. */
            if (DHCP6_VALID_OPT_TYPE(opt_type, message_type) == NU_TRUE)
            {
                /* If this is the first valid option found, add the Option
                 * Request Option header to the packet.
                 */
                if (valid_opts == 0)
                {
                    /* The code is ORO. */
                    PUT16(&dhcp_buffer[nbytes], DHCP6_OPT_CODE_OFFSET,
                          DHCP6_OPT_ORO);

                    /* Increment the length of the data in the buffer.  The
                     * option length will be filled in once all options are
                     * added to the packet.
                     */
                    nbytes += DHCP6_OPT_REQUEST_OPT_LEN;
                }

                /* Increment the number of valid options that have been added
                 * to the Option Request Option.
                 */
                valid_opts ++;

                /* Copy the option into the packet. */
                PUT16(dhcp_buffer, nbytes, opt_type);

                /* Increment the length of the data in the buffer. */
                nbytes += 2;
            }
        }
    }

    /* Add any options the server returned in an Option Request Option
     * that the user is not already including.
     */
    for (i = 0; i < DHCP6_MAX_OPT_CODES; i++)
    {
        /* If the server indicated that the client should request this
         * option.
         */
        if ( (ds_ptr->dhcp6_system_opts[i]) &&
             (DHCP6_VALID_OPT_TYPE(i, message_type) == NU_TRUE) )
        {
            /* If the user did not specify any valid options for this
             * message, build the Options Request Option header.
             */
            if (!valid_opts)
            {
                /* Add the Option Request Option code to the packet. */
                PUT16(&dhcp_buffer[nbytes], DHCP6_OPT_CODE_OFFSET,
                      DHCP6_OPT_ORO);

                /* Increment the length of the data in the buffer.  The
                 * option length will be filled in once all options are
                 * added to the packet.
                 */
                nbytes += DHCP6_OPT_REQUEST_OPT_LEN;
            }

            /* Increment the number of options in the Option Request
             * Option.
             */
            valid_opts ++;

            /* Copy the option into the packet. */
            PUT16(dhcp_buffer, nbytes, i);

            /* Increment the length of the data in the buffer. */
            nbytes += 2;
        }
    }

    /* If an Option Request Option was built, add the total length of the
     * option to the Option Request Option.
     */
    if (valid_opts)
    {
        /* Length is 2 times the number of options.  Compute the offset of
         * the beginning of the Option Request Option by subtracting the
         * length of the options added to the packet and the length of the
         * length field in the Options Request Option.
         */
        PUT16(dhcp_buffer, nbytes - 2 - (2 * valid_opts), (2 * valid_opts));
    }

    return (nbytes);

} /* DHCP6_Add_User_Options */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Output
*
*   DESCRIPTION
*
*       This routine transmits a DHCPv6 client message.
*
*   INPUTS
*
*       *dhcp_buffer            Pointer to the buffer of data to transmit.
*       nbytes                  Length of the buffer to transmit.
*       *ds_ptr                 Pointer to DHCP structure associated with
*                               the message.
*       *tx_ptr                 Pointer to the transmission structure
*                               associated with this transaction.
*
*   OUTPUTS
*
*       NU_SUCCESS              The packet was successfully transmitted.
*       -1                      An error occurred.
*
**************************************************************************/
STATIC STATUS DHCP6_Output(UINT8 *dhcp_buffer, INT nbytes,
                           DHCP6_STRUCT *ds_ptr, DHCP6_TX_STRUCT *tx_ptr)
{
    STATUS              status = NU_SUCCESS;
    msghdr              msg;
    cmsghdr             *cmsg;
    in6_pktinfo         *pkt_info;
    INT32               bytes_sent;
    CHAR                anc_buf[NU_CMSG_SPACE(sizeof(in6_pktinfo))];

    /* Set up the msghdr structure. */
    msg.msg_iov = (CHAR*)dhcp_buffer;
    msg.msg_iovlen = (UINT16)nbytes;

    /* If the server has sent a server unicast option, and this is a request,
     * renew, release or decline message, unicast the message to the server.
     */
    if ( (ds_ptr->dhcp6_flags & DHCP6_SERVER_UNICAST) &&
         ((tx_ptr->dhcp6_type == DHCP6_REQUEST) ||
          (tx_ptr->dhcp6_type == DHCP6_RENEW) ||
          (tx_ptr->dhcp6_type == DHCP6_RELEASE) ||
          (tx_ptr->dhcp6_type == DHCP6_DECLINE)) )
    {
        msg.msg_name = &ds_ptr->dhcp6_server.dhcp6_iaddr;
    }

    /* Otherwise, multicast this message. */
    else
    {
        msg.msg_name = &DHCP6_Server_Addr;
    }

    msg.msg_namelen = sizeof(struct addr_struct);

    /* Set up the msghdr to point to the buffer of ancillary data */
    msg.msg_control = anc_buf;
    msg.msg_controllen = sizeof(anc_buf);

    /* Get a pointer to the first cmsghdr in the buffer */
    cmsg = NU_CMSG_FIRSTHDR(&msg);

    /* Set the level and type for specifying the link-local address and
     * interface to use.
     */
    cmsg->cmsg_level = IPPROTO_IPV6;
    cmsg->cmsg_type = IPV6_PKTINFO;

    /* Set the length of the ancillary data object. */
    cmsg->cmsg_len = CMSG_LEN(sizeof(in6_pktinfo));

    /* Get a pointer to the in6_pktinfo data structure. */
    pkt_info = (in6_pktinfo*)NU_CMSG_DATA(cmsg);

    /* Fill in the interface index of the interface to use. */
    pkt_info->ipi6_ifindex = ds_ptr->dhcp6_dev_index;

    /* Get the link-local address associated with the interface.  This call
     * will only fail if the interface has been deleted.
     */
    if (NU_Get_Link_Local_Addr(ds_ptr->dhcp6_dev_index,
                               pkt_info->ipi6_addr) == NU_SUCCESS)
    {
        /* Send the message. */
        bytes_sent = NU_Sendmsg(DHCP6_Client_Socket, &msg, 0);

        /* If the data was not successfully transmitted, log an error. */
        if (bytes_sent <= 0)
        {
            status = bytes_sent;

            NLOG_Error_Log("Unable to send DHCPv6 message", NERR_SEVERE,
                           __FILE__, __LINE__);
        }

        /* Retransmit the message according to RFC 3315. */
        if (DHCP6_Compute_Retransmission_Time(tx_ptr, ds_ptr) == NU_SUCCESS)
        {
            /* Get the NET semaphore since we are about to set a NET timer. */
            if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
            {
                /* Set a timer to retransmit the packet if a response is
                 * not received within ds_ptr->dhcp6_rt seconds.
                 */
                if (TQ_Timerset(DHCP6_Retrans_Event, (UNSIGNED)tx_ptr->dhcp6_id,
                                tx_ptr->dhcp6_rt, 0) != NU_SUCCESS)
                {
                    status = -1;

                    NLOG_Error_Log("Failed to set timer to retransmit DHCPv6 message",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }

                NU_Release_Semaphore(&TCP_Resource);
            }

            else
            {
                status = -1;

                NLOG_Error_Log("Failed to obtain TCP semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }

        else
        {
            status = -1;

            NLOG_Error_Log("DHCPv6 message transmitted max number of times",
                           NERR_INFORMATIONAL, __FILE__, __LINE__);
        }
    }

    else
    {
        status = -1;

        NLOG_Error_Log("Unable to get link-local address on DHCPv6 interface",
                       NERR_SEVERE, __FILE__, __LINE__);
    }

    return (status);

} /* DHCP6_Output */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Add_System_IA_NA_Option
*
*   DESCRIPTION
*
*       This routine adds an IA_NA option to the packet and adds an
*       IA_ADDR option for each non-link-local address that matches the
*       provided addr_flags in the list of addresses for the interface
*       associated with the ds_ptr.
*
*   INPUTS
*
*       *dhcp_buffer            Pointer to the buffer into which to build
*                               the options.
*       *ds_ptr                 Pointer to the DHCPv6 data structure for
*                               the packet being built.
*       addr_flags              Flags of the target IPv6 addresses to add
*                               to the packet.
*
*   OUTPUTS
*
*       The number of bytes added to the packet.
*
**************************************************************************/
STATIC UINT16 DHCP6_Add_System_IA_NA_Option(UINT8 *dhcp_buffer,
                                            DHCP6_STRUCT *ds_ptr,
                                            UINT32 addr_flags)
{
    UINT16          nbytes = 0;
    DEV6_IF_ADDRESS *addr_ptr = NU_NULL;
    DV_DEVICE_ENTRY *dev_ptr;
    STATUS          status;

    /* Increment the length of the buffer past the IA_NA option.  The IA_NA
     * option will be added after the number of IPv6 addresses are counted
     * and IA_ADDR options added for each address.
     */
    nbytes += DHCP6_IA_NA_OPT_LEN;

    /* Obtain the TCP semaphore. */
    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Get a pointer to the interface. */
        dev_ptr = DEV_Get_Dev_By_Index(ds_ptr->dhcp6_dev_index);

        /* If an interface was found. */
        if (dev_ptr)
        {
            /* Get a pointer to the address list associated with this
               interface. */
            if (dev_ptr->dev_flags & DV6_IPV6)
                addr_ptr = dev_ptr->dev6_addr_list.dv_head;
        }

        /* If there are addresses on the interface. */
        while (addr_ptr)
        {
            /* If this address matches the flag requirements. */
            if (addr_ptr->dev6_addr_flags & addr_flags)
            {
                /* Build an IA_ADDR option for this address. */
                nbytes +=
                    DHCP6_Build_IA_Addr_Option(&dhcp_buffer[nbytes],
                                               addr_ptr->dev6_ip_addr,
                                               addr_ptr->dev6_preferred_lifetime,
                                               addr_ptr->dev6_valid_lifetime);
            }

            /* Get a pointer to the next address in the list. */
            addr_ptr = addr_ptr->dev6_next;
        }

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
        }
    }

    /* If one or more IA_ADDR options were added to the packet. */
    if (nbytes > DHCP6_IA_NA_OPT_LEN)
    {
        /* Copy the user's IA_NA option into the packet. */
        NU_BLOCK_COPY(dhcp_buffer, &ds_ptr->dhcp6_opts
                      [ds_ptr->dhcp6_client_opts[DHCP6_OPT_IA_NA].dhcp6_opt_addr],
                      DHCP6_IA_NA_OPT_LEN);

        /* Set the length of the IA_NA option according to the number of
         * IA_ADDR options that were built.
         */
        PUT16(dhcp_buffer, DHCP6_OPT_LEN_OFFSET, nbytes - DHCP6_MSG_LEN);
    }

    /* Otherwise, no IA_ADDR options were added, so do not include an
     * IA_NA option in the packet.
     */
    else
    {
        nbytes = 0;
    }

    return (nbytes);

} /* DHCP6_Add_System_IA_NA_Option */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Find_Server
*
*   DESCRIPTION
*
*       This routine begins the process of server discovery by transmitting
*       an initial DHCPv6 Solicitation message.  Retransmissions take over
*       after the first transmission.
*
*   INPUTS
*
*       *ds_ptr                 Pointer to DHCPv6 structure for which server
*                               discovery is being performed.
*
*   OUTPUTS
*
*       NU_SUCCESS              The packet was successfully transmitted.
*       -1                      An error occurred.
*
**************************************************************************/
STATUS DHCP6_Find_Server(DHCP6_STRUCT *ds_ptr)
{
    STATUS          status;
    UINT8           flags = ds_ptr->dhcp6_flags;
    DHCP6_TX_STRUCT *tx_ptr;

    /* Reserve a transmission structure for this transaction. */
    tx_ptr = DHCP6_Obtain_TX_Struct();

    if (tx_ptr)
    {
        /* Tie the transmission structure to the DHCPv6 structure. */
        tx_ptr->dhcp6_ds_id = ds_ptr->dhcp6_id;

        /* Set up the retransmission parameters for the solicit. */
        tx_ptr->dhcp6_irt = DHCP6_SOL_TIMEOUT;
        tx_ptr->dhcp6_mrt = DHCP6_SOL_MAX_RT;
        tx_ptr->dhcp6_mrc = 0;
        tx_ptr->dhcp6_mrd = 0;

        /* Initialize the flags to zero. */
        ds_ptr->dhcp6_flags = 0;

        /* If this server discovery is being invoked because a rebind failed,
         * preserve the rebind failed flag.
         */
        if (flags & DHCP6_REBIND_EXPIRED)
        {
            ds_ptr->dhcp6_flags = DHCP6_REBIND_EXPIRED;
        }

        /* Transmit a server Solicitation to find a new server. */
        status = DHCP6_Solicit_Output(ds_ptr, tx_ptr);
    }

    else
    {
        NLOG_Error_Log("Failed to allocate DHCPv6 transmission structure",
                       NERR_SEVERE, __FILE__, __LINE__);

        status = -1;
    }

    return (status);

} /* DHCP6_Find_Server */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Solicit_Output
*
*   DESCRIPTION
*
*       This routine transmits a Solicit message.  The options to transmit
*       to the DHCPv6 server must be set up by the caller.
*
*   INPUTS
*
*       *ds_ptr                 Pointer to DHCPv6 structure associated with
*                               the message.
*       *tx_ptr                 Pointer to the transmission structure
*                               associated with this transaction.
*
*   OUTPUTS
*
*       NU_SUCCESS              The packet was successfully transmitted.
*       -1                      An error occurred.
*
**************************************************************************/
STATUS DHCP6_Solicit_Output(DHCP6_STRUCT *ds_ptr, DHCP6_TX_STRUCT *tx_ptr)
{
    STATUS  status;
    UINT8   dhcp_buffer[DHCP6_BUFFER_LEN];
    UINT16  nbytes = DHCP6_MSG_LEN, ia_na_opt_len, opt_offset, opt_code;
    UINT32  count;

    /* Set up the message type. */
    tx_ptr->dhcp6_type = DHCP6_SOLICIT;

    /* Build the common portion of the DHCPv6 client message. */
    DHCP6_Build_Packet(dhcp_buffer, tx_ptr);

    /* Add the allowable user-specified options to the packet. */
    nbytes += DHCP6_Add_User_Options(&dhcp_buffer[nbytes], ds_ptr,
                                     tx_ptr, tx_ptr->dhcp6_type);

    /* Ensure the user-specified options did not use up the entire
     * buffer.
     */
    if (nbytes < DHCP6_BUFFER_LEN)
    {
        /* RFC 3315 - section 18.1.4 - The client may choose to use a Solicit
         * message to locate a new DHCP server and send a Request for the
         * expired IA to the new server.
         */
        if (ds_ptr->dhcp6_flags & DHCP6_REBIND_EXPIRED)
        {
            nbytes += DHCP6_Add_System_IA_NA_Option(&dhcp_buffer[nbytes],
                                                    ds_ptr, ADDR6_DHCP_ADDR);
        }

        /* RFC 3315 - section 17.1.1 - The client uses IA_NA options to request
         * the assignment of non-temporary addresses.
         */
        else if (ds_ptr->dhcp6_client_opts[DHCP6_OPT_IA_NA].dhcp6_opt_addr != 0xffffffff)
        {
            /* Save off the number of options of this type in the packet. */
            count = ds_ptr->dhcp6_client_opts[DHCP6_OPT_IA_NA].dhcp6_opt_count;

            /* Set the byte counter to the address of this first option. */
            opt_offset =
                (UINT16)ds_ptr->dhcp6_client_opts[DHCP6_OPT_IA_NA].dhcp6_opt_addr;

            /* Copy each option of this type into the packet. */
            do
            {
                /* Determine the length of the IA_NA option, including the
                 * option header and any IA_ADDR options.
                 */
                ia_na_opt_len = (UINT16)(GET16(&ds_ptr->dhcp6_opts[opt_offset],
                    DHCP6_OPT_LEN_OFFSET) + DHCP6_MSG_LEN);

                /* If the option will fit in the buffer. */
                if (ia_na_opt_len < (DHCP6_BUFFER_LEN - nbytes))
                {
                    /* Copy the IA_NA option into the buffer. */
                    NU_BLOCK_COPY(&dhcp_buffer[nbytes],
                                  &ds_ptr->dhcp6_opts[opt_offset], ia_na_opt_len);

                    /* Increment the number of bytes of data in this packet. */
                    nbytes += ia_na_opt_len;
                }

                /* Decrement the count of IA_NA options. */
                count --;

                /* If there are more options of this type. */
                if (count)
                {
                    /* Find the next instance of this option. */
                    do
                    {
                        /* Get the length of this option. */
                        opt_offset += (GET16(&ds_ptr->dhcp6_opts[opt_offset],
                            DHCP6_OPT_LEN_OFFSET) + DHCP6_MSG_LEN);

                        /* Get the option code. */
                        opt_code = GET16(&ds_ptr->dhcp6_opts[opt_offset],
                            DHCP6_OPT_CODE_OFFSET);

                    } while (opt_code != DHCP6_OPT_IA_NA);
                }

            } while ( (count) && (nbytes < DHCP6_BUFFER_LEN) );
        }
    }

    /* RFC 3315 - section 17 - If the client will accept a Reply message
     * with committed address assignments and other resources in response
     * to the Solicit message, the client includes a Rapid Commit option
     * in the Solicit message.
     */
    if ( (nbytes < DHCP6_BUFFER_LEN) &&
         (ds_ptr->dhcp6_client_opts[DHCP6_OPT_RAPID_COMMIT].dhcp6_opt_addr != 0xffffffff) )
    {
        /* Copy the option into the packet. */
        NU_BLOCK_COPY(&dhcp_buffer[nbytes],
                      &ds_ptr->dhcp6_opts[ds_ptr->dhcp6_client_opts
                      [DHCP6_OPT_RAPID_COMMIT].dhcp6_opt_addr],
                      DHCP6_RAPID_COMMIT_OPT_LEN);

        /* Increment the buffer pointer. */
        nbytes += DHCP6_RAPID_COMMIT_OPT_LEN;

        /* Set the flag indicating that Rapid Commits are accepted. */
        ds_ptr->dhcp6_flags |= DHCP6_ALLOW_RAPID_COMMIT;
    }

    /* Send the packet. */
    status = DHCP6_Output(dhcp_buffer, nbytes, ds_ptr, tx_ptr);

    return (status);

} /* DHCP6_Solicit_Output */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Confirm_Addresses
*
*   DESCRIPTION
*
*       This routine begins the confirmation process for addresses
*       associated with an interface.  Whenever a client may have moved
*       to a new link, the prefixes from the addresses assigned to the
*       interfaces on that link may no longer be appropriate for the link
*       to which the client is attached.  Examples of times when a client
*       may have moved to a new link include:
*
*           o  The client reboots.
*           o  The client is physically connected to a wired connection.
*           o  The client returns from sleep mode.
*           o  The client using a wireless technology changes access points.
*
*   INPUTS
*
*       dev_index               The index of the interface for which to
*                               confirm the IPv6 addresses.
*
*   OUTPUTS
*
*       NU_SUCCESS              The packet was successfully transmitted.
*       -1                      An error occurred.
*
**************************************************************************/
STATUS DHCP6_Confirm_Addresses(UINT32 dev_index)
{
    STATUS          status;
    DHCP6_STRUCT    *ds_ptr;
    DHCP6_TX_STRUCT *tx_ptr;

    /* Obtain the DHCPv6 client semaphore. */
    status = NU_Obtain_Semaphore(&DHCP6_Cli_Resource, NU_NO_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Get a pointer to the DHCPv6 structure associated with this
         * interface.
         */
        ds_ptr = DHCP6_Find_Struct_By_Dev(dev_index);

        /* If a DHCPv6 structure was found, invoke the confirmation
         * process.
         */
        if (ds_ptr)
        {
            /* Reserve a transmission structure for this transaction. */
            tx_ptr = DHCP6_Obtain_TX_Struct();

            if (tx_ptr)
            {
                /* Tie the transmission structure to the DHCPv6
                 * structure.
                 */
                tx_ptr->dhcp6_ds_id = ds_ptr->dhcp6_id;

                /* Set up the retransmission parameters for the
                 * confirmation.
                 */
                tx_ptr->dhcp6_irt = DHCP6_CNF_TIMEOUT;
                tx_ptr->dhcp6_mrt = DHCP6_CNF_MAX_RT;
                tx_ptr->dhcp6_mrc = 0;
                tx_ptr->dhcp6_mrd = DHCP6_CNF_MAX_RD;

                /* Begin confirmation. */
                status = DHCP6_Confirm_Output(ds_ptr, tx_ptr);
            }

            /* Otherwise, log an error and return an error code. */
            else
            {
                status = -1;

                NLOG_Error_Log("Failed to obtain DHCPv6 transmission structure",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        /* Otherwise, log an error and return an error code. */
        else
        {
            status = -1;

            NLOG_Error_Log("No DHCPv6 structure for interface index",
                        NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* Release the DHCPv6 client semaphore. */
        NU_Release_Semaphore(&DHCP6_Cli_Resource);
    }

    /* Otherwise, log an error and return an error code. */
    else
    {
        NLOG_Error_Log("Failed to obtain DHCPv6 semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    return (status);

} /* DHCP6_Confirm_Addresses */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Confirm_Output
*
*   DESCRIPTION
*
*       This routine transmits a Confirm message.
*
*   INPUTS
*
*       *ds_ptr                 Pointer to DHCPv6 structure associated with
*                               the message.
*       *tx_ptr                 Pointer to the transmission structure
*                               associated with this transaction.
*
*   OUTPUTS
*
*       NU_SUCCESS              The packet was successfully transmitted.
*       -1                      An error occurred.
*
**************************************************************************/
STATUS DHCP6_Confirm_Output(DHCP6_STRUCT *ds_ptr, DHCP6_TX_STRUCT *tx_ptr)
{
    STATUS  status;
    UINT8   dhcp_buffer[DHCP6_BUFFER_LEN];
    UINT16  nbytes = DHCP6_MSG_LEN;

    /* Set up the message type. */
    tx_ptr->dhcp6_type = DHCP6_CONFIRM;

    /* Build the common portion of the DHCPv6 client message. */
    DHCP6_Build_Packet(dhcp_buffer, tx_ptr);

    /* Add the allowable user-specified options to the packet. */
    nbytes += DHCP6_Add_User_Options(&dhcp_buffer[nbytes], ds_ptr,
                                     tx_ptr, tx_ptr->dhcp6_type);

    if (nbytes < DHCP6_BUFFER_LEN)
    {
        /* RFC 3315 - section 18.1.2 - The client includes IA options for all
         * of the IAs assigned to the interface for which the Confirm message
         * is being sent.  The IA options include all of the addresses the
         * client currently has associated with those IAs.
         */
        nbytes += DHCP6_Add_System_IA_NA_Option(&dhcp_buffer[nbytes], ds_ptr,
                                                ADDR6_DHCP_ADDR);

        /* Send the packet. */
        status = DHCP6_Output(dhcp_buffer, nbytes, ds_ptr, tx_ptr);
    }

    /* If the IA_NA option cannot be added, there is no reason to try to
     * send the packet.
     */
    else
    {
        status = -1;
    }

    return (status);

} /* DHCP6_Confirm_Output */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Renew_Addresses
*
*   DESCRIPTION
*
*       This routine begins the renewal process to extend the preferred and
*       valid lifetimes of addresses associated with an interface.
*
*   INPUTS
*
*       dev_index               The index of the interface for which to
*                               renew the IPv6 addresses.
*
*   OUTPUTS
*
*       NU_SUCCESS              The packet was successfully transmitted.
*       -1                      An error occurred.
*
**************************************************************************/
STATUS DHCP6_Renew_Addresses(UINT32 dev_index)
{
    STATUS          status;
    DHCP6_STRUCT    *ds_ptr;
    DHCP6_TX_STRUCT *tx_ptr;

    /* Get a pointer to the DHCPv6 structure associated with this
     * interface.
     */
    ds_ptr = DHCP6_Find_Struct_By_Dev(dev_index);

    /* If a DHCPv6 structure was found, invoke the renewal process. */
    if (ds_ptr)
    {
        /* Reserve a transmission structure for this transaction. */
        tx_ptr = DHCP6_Obtain_TX_Struct();

        if (tx_ptr)
        {
            /* Tie the transmission structure to the DHCPv6 structure. */
            tx_ptr->dhcp6_ds_id = ds_ptr->dhcp6_id;

            /* Set up the retransmission parameters for the renewal. */
            tx_ptr->dhcp6_irt = DHCP6_REN_TIMEOUT;
            tx_ptr->dhcp6_mrt = DHCP6_REN_MAX_RT;
            tx_ptr->dhcp6_mrc = 0;

            /* MRD is the remaining time until T2, at which time the client
             * transmits a Rebind message.
             */
            tx_ptr->dhcp6_mrd =
                (ds_ptr->dhcp6_t2 - ds_ptr->dhcp6_t1) * SCK_Ticks_Per_Second;

            /* Beging the address renewal process. */
            status = DHCP6_Renew_Output(ds_ptr, tx_ptr);
        }

        /* Otherwise, log an error and return an error code. */
        else
        {
            status = -1;

            NLOG_Error_Log("Failed to obtain DHCPv6 transmission structure",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    /* Otherwise, log an error and return an error code. */
    else
    {
        status = -1;

        NLOG_Error_Log("No DHCPv6 structure for interface index",
                       NERR_SEVERE, __FILE__, __LINE__);
    }

    return (status);

} /* DHCP6_Renew_Addresses */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Renew_Output
*
*   DESCRIPTION
*
*       This routine transmits a Renew message.
*
*   INPUTS
*
*       *ds_ptr                 Pointer to DHCPv6 structure associated with
*                               the message.
*       *tx_ptr                 Pointer to the transmission structure
*                               associated with this transaction.
*
*   OUTPUTS
*
*       NU_SUCCESS              The packet was successfully transmitted.
*       -1                      An error occurred.
*
**************************************************************************/
STATUS DHCP6_Renew_Output(DHCP6_STRUCT *ds_ptr, DHCP6_TX_STRUCT *tx_ptr)
{
    STATUS  status;
    UINT8   dhcp_buffer[DHCP6_BUFFER_LEN];
    UINT16  nbytes = DHCP6_MSG_LEN, ia_na_bytes;

    /* Set up the message type. */
    tx_ptr->dhcp6_type = DHCP6_RENEW;

    /* Build the common portion of the DHCPv6 client message. */
    DHCP6_Build_Packet(dhcp_buffer, tx_ptr);

    /* RFC 3315 - section 18.1.3 - The client places the identifier of
     * the destination server in a Server Identifier option.
     */
    nbytes += DHCP6_Build_Server_ID_Option(&dhcp_buffer[nbytes], ds_ptr);

    if (nbytes < DHCP6_BUFFER_LEN)
    {
        /* Add the allowable user-specified options to the packet. */
        nbytes += DHCP6_Add_User_Options(&dhcp_buffer[nbytes], ds_ptr,
                                         tx_ptr, tx_ptr->dhcp6_type);

        if (nbytes < DHCP6_BUFFER_LEN)
        {
            /* RFC 3315 - section 18.1.3 - The client includes IA Address options in
             * the option for the addresses associated with the IA.  Since all
             * addresses associated with an IA share the same T1 and T2, renew all
             * addresses at the same time.
             */
            ia_na_bytes = DHCP6_Add_System_IA_NA_Option(&dhcp_buffer[nbytes], ds_ptr,
                                                        ADDR6_DHCP_ADDR);

            /* If there are still addresses associated with this IA on the
             * interface.
             */
            if (ia_na_bytes)
            {
                /* Increment the number of bytes in the packet by the number of
                 * bytes in the IA option.
                 */
                nbytes += ia_na_bytes;

                /* Send the packet. */
                status = DHCP6_Output(dhcp_buffer, nbytes, ds_ptr, tx_ptr);

                /* RFC 3315 - section 18.1.3 - The message exchange is terminated
                 * when time T2 is reached, at which time the client begins a Rebind
                 * message exchange.  If the call to output the message failed, it
                 * means T2 has been reached.
                 */
                if (status != NU_SUCCESS)
                {
                    /* Free the transmission structure entry. */
                    DHCP6_Free_TX_Struct(tx_ptr);

                    /* Attempt to rebind the addresses. */
                    status = DHCP6_Rebind_Addresses(ds_ptr);
                }
            }

            /* Otherwise, all IAs on this interface have been deleted.  Stop trying
             * to renew.
             */
            else
            {
                status = -1;

                /* Free the transmission structure entry. */
                DHCP6_Free_TX_Struct(tx_ptr);
            }
        }

        else
        {
            status = -1;

            /* Free the transmission structure entry. */
            DHCP6_Free_TX_Struct(tx_ptr);
        }
    }

    else
    {
        status = -1;

        /* Free the transmission structure entry. */
        DHCP6_Free_TX_Struct(tx_ptr);
    }

    return (status);

} /* DHCP6_Renew_Output */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Rebind_Addresses
*
*   DESCRIPTION
*
*       This routine begins the rebind process.  At time T2 for an IA
*       (which will only be reached if the server to which the Renew
*       message was sent at time T1 has not responded), the client
*       initiates a Rebind/Reply message exchange with any available
*       server.
*
*   INPUTS
*
*       *ds_ptr                 Pointer to the DHCPv6 structure associated
*                               with the rebind.
*
*   OUTPUTS
*
*       NU_SUCCESS              The packet was successfully transmitted.
*       -1                      An error occurred.
*
**************************************************************************/
STATUS DHCP6_Rebind_Addresses(DHCP6_STRUCT *ds_ptr)
{
    STATUS          status;
    DHCP6_TX_STRUCT *tx_ptr;
    UINT32          long_life = 0;
    DV_DEVICE_ENTRY *dev_ptr;
    DEV6_IF_ADDRESS *addr_ptr = NU_NULL;

    /* Reserve a transmission structure for this transaction. */
    tx_ptr = DHCP6_Obtain_TX_Struct();

    if (tx_ptr)
    {
        /* Tie the transmission structure to the DHCPv6 structure. */
        tx_ptr->dhcp6_ds_id = ds_ptr->dhcp6_id;

        /* Obtain the TCP semaphore. */
        status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Get a pointer to the interface. */
            dev_ptr = DEV_Get_Dev_By_Index(ds_ptr->dhcp6_dev_index);

            /* If an interface was found. */
            if (dev_ptr)
            {
                /* Get a pointer to the first address in the address list. */
                if (dev_ptr->dev_flags & DV6_IPV6)
                    addr_ptr = dev_ptr->dev6_addr_list.dv_head;

                /* Find the address with the longest valid lifetime. */
                while (addr_ptr)
                {
                    /* If this address was obtained via DHCPv6. */
                    if (addr_ptr->dev6_addr_flags & ADDR6_DHCP_ADDR)
                    {
                        /* If the valid lifetime is longer than the longest
                         * valid lifetime we have found, save off this
                         * lifetime.
                         */
                        if (addr_ptr->dev6_valid_lifetime > long_life)
                        {
                            long_life = addr_ptr->dev6_valid_lifetime;
                        }
                    }

                    /* Get a pointer to the next address. */
                    addr_ptr = addr_ptr->dev6_next;
                }
            }

            /* Release the semaphore. */
            if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                               __FILE__, __LINE__);
            }
        }

        /* Set up the retransmission parameters for the renewal. */
        tx_ptr->dhcp6_irt = DHCP6_REB_TIMEOUT;
        tx_ptr->dhcp6_mrt = DHCP6_REB_MAX_RT;
        tx_ptr->dhcp6_mrc = 0;
        tx_ptr->dhcp6_mrd = long_life * SCK_Ticks_Per_Second;

        /* Beging the address renewal process. */
        status = DHCP6_Rebind_Output(ds_ptr, tx_ptr);
    }

    /* Otherwise, log an error and return an error code. */
    else
    {
        status = -1;

        NLOG_Error_Log("No DHCPv6 transmission structure available",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    return (status);

} /* DHCP6_Rebind_Addresses */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Rebind_Output
*
*   DESCRIPTION
*
*       This routine transmits a Rebind message.
*
*   INPUTS
*
*       *ds_ptr                 Pointer to DHCPv6 structure associated with
*                               the message.
*       *tx_ptr                 Pointer to the transmission structure
*                               associated with the transaction.
*
*   OUTPUTS
*
*       NU_SUCCESS              The packet was successfully transmitted.
*       -1                      An error occurred.
*
**************************************************************************/
STATUS DHCP6_Rebind_Output(DHCP6_STRUCT *ds_ptr, DHCP6_TX_STRUCT *tx_ptr)
{
    STATUS          status;
    UINT8           dhcp_buffer[DHCP6_BUFFER_LEN];
    UINT16          nbytes = DHCP6_MSG_LEN;

    /* Set up the message type. */
    tx_ptr->dhcp6_type = DHCP6_REBIND;

    /* Build the common portion of the DHCPv6 client message. */
    DHCP6_Build_Packet(dhcp_buffer, tx_ptr);

    /* Add the allowable user-specified options to the packet. */
    nbytes += DHCP6_Add_User_Options(&dhcp_buffer[nbytes], ds_ptr,
                                     tx_ptr, tx_ptr->dhcp6_type);

    if (nbytes < DHCP6_BUFFER_LEN)
    {
        /* RFC 3315 - section 18.1.4 - The client includes an IA option with
         * all addresses currently assigned to the IA in its Rebind message.
         */
        nbytes += DHCP6_Add_System_IA_NA_Option(&dhcp_buffer[nbytes], ds_ptr,
                                                ADDR6_DHCP_ADDR);

        /* Send the packet. */
        status = DHCP6_Output(dhcp_buffer, nbytes, ds_ptr, tx_ptr);
    }

    else
    {
        status = -1;
    }

    /* RFC 3315 - section 18.1.4 - The message exchange is terminated when
     * the valid lifetimes of all the addresses assigned to the IA expire,
     * at which time the client may use a Solicit message to locate a new
     * DHCP server and send a Request for the expired IA to the new server.
     */
    if (status != NU_SUCCESS)
    {
        /* Free the transmission structure entry. */
        DHCP6_Free_TX_Struct(tx_ptr);

        /* Invoke the server discovery process. */
        DHCP6_Find_Server(ds_ptr);
    }

    return (status);

} /* DHCP6_Rebind_Output */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Obtain_Address
*
*   DESCRIPTION
*
*       This routine begins the process of obtaining an IPv6 address for
*       the interface.
*
*   INPUTS
*
*       *ds_ptr                 Pointer to DHCPv6 structure for which an
*                               address is being obtained.
*
*   OUTPUTS
*
*       NU_SUCCESS              The packet was successfully transmitted.
*       -1                      An error occurred.
*
**************************************************************************/
STATUS DHCP6_Obtain_Address(DHCP6_STRUCT *ds_ptr)
{
    STATUS          status;
    DHCP6_TX_STRUCT *tx_ptr;

    /* Reserve a transmission structure for this transaction. */
    tx_ptr = DHCP6_Obtain_TX_Struct();

    if (tx_ptr)
    {
        /* Tie the transmission structure to the DHCPv6 structure. */
        tx_ptr->dhcp6_ds_id = ds_ptr->dhcp6_id;

        /* Set up the retransmission parameters for the request. */
        tx_ptr->dhcp6_irt = DHCP6_REQ_TIMEOUT;
        tx_ptr->dhcp6_mrt = DHCP6_REQ_MAX_RT;
        tx_ptr->dhcp6_mrc = DHCP6_REQ_MAX_RC;
        tx_ptr->dhcp6_mrd = 0;

        /* Transmit the Request message. */
        status = DHCP6_Request_Output(ds_ptr, tx_ptr);
    }

    else
    {
        status = -1;

        NLOG_Error_Log("No DHCPv6 transmission structure available",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    return (status);

} /* DHCP6_Obtain_Address */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Request_Output
*
*   DESCRIPTION
*
*       This routine transmits a Request message.
*
*   INPUTS
*
*       *ds_ptr                 Pointer to DHCPv6 structure associated with
*                               the message.
*       *tx_ptr                 Pointer to the transmission structure
*                               associated with this transaction.
*
*   OUTPUTS
*
*       NU_SUCCESS              The packet was successfully transmitted.
*       -1                      An error occurred.
*
**************************************************************************/
STATUS DHCP6_Request_Output(DHCP6_STRUCT *ds_ptr, DHCP6_TX_STRUCT *tx_ptr)
{
    STATUS  status = NU_SUCCESS;
    UINT8   dhcp_buffer[DHCP6_BUFFER_LEN];
    UINT16  nbytes = DHCP6_MSG_LEN;
    UINT32  iaid;

    /* Set up the message type. */
    tx_ptr->dhcp6_type = DHCP6_REQUEST;

    /* Build the common portion of the DHCPv6 client message. */
    DHCP6_Build_Packet(dhcp_buffer, tx_ptr);

    /* RFC 3315 - section 18.1.1 - The client places the identifier of the
     * destination server in a Server Identifier option.
     */
    nbytes += DHCP6_Build_Server_ID_Option(&dhcp_buffer[nbytes], ds_ptr);

    /* Add the allowable user-specified options to the packet. */
    nbytes += DHCP6_Add_User_Options(&dhcp_buffer[nbytes], ds_ptr,
                                     tx_ptr, tx_ptr->dhcp6_type);

    /* RFC 3315 - section 18.1.1 - The client includes one or more IA
     * options in the Request message.
     */
    if (ds_ptr->dhcp6_client_opts[DHCP6_OPT_IA_NA].dhcp6_opt_addr != 0xffffffff)
    {
        if (nbytes < DHCP6_BUFFER_LEN)
        {
            /* Get the IAID for the interface. */
            NU_Get_DHCP_IAID(ds_ptr->dhcp6_dev_index, &iaid);

            /* Build an IA_NA option. */
            nbytes += DHCP6_Build_IA_NA_Option(&dhcp_buffer[nbytes], iaid, 0, 0,
                                               &ds_ptr->dhcp6_offered_addr,
                                               ds_ptr->dhcp6_offered_addr.
                                               dhcp6_ia_addr[0] != 0 ? 1 : 0);
        }

        else
        {
            status = -1;
        }
    }

    if (status == NU_SUCCESS)
    {
        /* Send the packet. */
        status = DHCP6_Output(dhcp_buffer, nbytes, ds_ptr, tx_ptr);
    }

    return (status);

} /* DHCP6_Request_Output */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Obtain_Config_Info
*
*   DESCRIPTION
*
*       This routine begins the process of obtaining IPv6 configuration
*       information from a DHCPv6 server.
*
*   INPUTS
*
*       *ds_ptr                 Pointer to DHCPv6 structure for which
*                               information is being obtained.
*
*   OUTPUTS
*
*       NU_SUCCESS              The packet was successfully transmitted.
*       -1                      An error occurred.
*
**************************************************************************/
STATUS DHCP6_Obtain_Config_Info(DHCP6_STRUCT *ds_ptr)
{
    STATUS          status;
    DHCP6_TX_STRUCT *tx_ptr;

    /* Reserve a transmission structure for this transaction. */
    tx_ptr = DHCP6_Obtain_TX_Struct();

    if (tx_ptr)
    {
        /* Tie the transmission structure to the DHCPv6 structure. */
        tx_ptr->dhcp6_ds_id = ds_ptr->dhcp6_id;

        /* Set up the retransmission parameters for the request. */
        tx_ptr->dhcp6_irt = DHCP6_INF_TIMEOUT;
        tx_ptr->dhcp6_mrt = DHCP6_INF_MAX_RT;
        tx_ptr->dhcp6_mrc = 0;
        tx_ptr->dhcp6_mrd = 0;

        /* Transmit the Info Request message. */
        status = DHCP6_Info_Request_Output(ds_ptr, tx_ptr);
    }

    else
    {
        status = -1;

        NLOG_Error_Log("No DHCPv6 transmission structure available",
                       NERR_RECOVERABLE, __FILE__, __LINE__);
    }

    return (status);

} /* DHCP6_Obtain_Config_Info */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Info_Request_Output
*
*   DESCRIPTION
*
*       This routine transmits an Information Request message.
*
*   INPUTS
*
*       *ds_ptr                 Pointer to DHCPv6 structure associated with
*                               the message.
*       *tx_ptr                 Pointer to transmission structure associated
*                               with this transaction.
*
*   OUTPUTS
*
*       NU_SUCCESS              The packet was successfully transmitted.
*       -1                      An error occurred.
*
**************************************************************************/
STATUS DHCP6_Info_Request_Output(DHCP6_STRUCT *ds_ptr,
                                 DHCP6_TX_STRUCT *tx_ptr)
{
    STATUS  status;
    UINT8   dhcp_buffer[DHCP6_BUFFER_LEN];
    UINT16  nbytes = DHCP6_MSG_LEN;

    /* Set up the message type. */
    tx_ptr->dhcp6_type = DHCP6_INFO_REQUEST;

    /* Build the common portion of the DHCPv6 client message. */
    DHCP6_Build_Packet(dhcp_buffer, tx_ptr);

    /* RFC 3315 - section 19.4.3 - When responding to a Reconfigure, the
     * client ... includes a Server Identifier option with the identifier
     * from the Reconfigure message to which the client is responding.
     */
    if (ds_ptr->dhcp6_flags & DHCP6_RECON_IN_PROGRESS)
    {
        nbytes +=
            DHCP6_Build_Server_ID_Option(&dhcp_buffer[nbytes], ds_ptr);
    }

    /* Add the allowable user-specified options to the packet. */
    nbytes += DHCP6_Add_User_Options(&dhcp_buffer[nbytes], ds_ptr,
                                     tx_ptr, tx_ptr->dhcp6_type);

    /* Send the packet. */
    status = DHCP6_Output(dhcp_buffer, nbytes, ds_ptr, tx_ptr);

    return (status);

} /* DHCP6_Info_Request_Output */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Release_Output
*
*   DESCRIPTION
*
*       This routine transmits a Release message.
*
*   INPUTS
*
*       *ds_ptr                 Pointer to DHCPv6 structure associated with
*                               the message.
*       *tx_ptr                 Pointer to transmission structure associated
*                               with this transaction.
*
*   OUTPUTS
*
*       NU_SUCCESS              The packet was successfully transmitted.
*       -1                      An error occurred.
*
**************************************************************************/
STATUS DHCP6_Release_Output(DHCP6_STRUCT *ds_ptr, DHCP6_TX_STRUCT *tx_ptr)
{
    STATUS                  status;
    UINT8                   dhcp_buffer[DHCP6_BUFFER_LEN];
    UINT16                  nbytes = DHCP6_MSG_LEN;
    DHCP6_IA_ADDR_STRUCT    ia_addr_ptr;

    /* Set up the message type. */
    tx_ptr->dhcp6_type = DHCP6_RELEASE;

    /* Build the common portion of the DHCPv6 client message. */
    DHCP6_Build_Packet(dhcp_buffer, tx_ptr);

    /* RFC 3315 - section 18.1.6 - The client places the identifier of the
     * server that allocated the address(es) in a Server Identifier option.
     */
    nbytes += DHCP6_Build_Server_ID_Option(&dhcp_buffer[nbytes], ds_ptr);

    /* Add the allowable user-specified options to the packet. */
    nbytes += DHCP6_Add_User_Options(&dhcp_buffer[nbytes], ds_ptr,
                                     tx_ptr, tx_ptr->dhcp6_type);

    if (nbytes < DHCP6_BUFFER_LEN)
    {
        /* Copy the address into the IA_ADDR structure. */
        NU_BLOCK_COPY(ia_addr_ptr.dhcp6_ia_addr, tx_ptr->dhcp6_release_addr,
                      IP6_ADDR_LEN);

        /* Set the preferred and valid lifetimes. */
        ia_addr_ptr.dhcp6_ia_pref_life = 0;
        ia_addr_ptr.dhcp6_ia_valid_life = 0;

        /* Build an IA_NA option for the address being released.  Extract the
         * IAID from the user's IA_NA option and set T1 and T2 to zero per RFC
         * 3315 section 18.1.2.
         */
        nbytes +=
            DHCP6_Build_IA_NA_Option(&dhcp_buffer[nbytes],
                                     GET16(&ds_ptr->dhcp6_opts
                                     [ds_ptr->dhcp6_client_opts[DHCP6_OPT_IA_NA].dhcp6_opt_addr],
                                     DHCP6_IA_NA_IAID_OFFSET),
                                     0, 0, &ia_addr_ptr, 1);

        /* Send the packet. */
        status = DHCP6_Output(dhcp_buffer, nbytes, ds_ptr, tx_ptr);
    }

    else
    {
        status = -1;
    }

    /* If this message has been transmitted the max number of times with no
     * response, give up.
     */
    if (status != NU_SUCCESS)
    {
        /* Free the transmission structure entry. */
        DHCP6_Free_TX_Struct(tx_ptr);

        /* Finalize deletion of the address. */
        DHCP6_Free_Address(ds_ptr);
    }

    return (status);

} /* DHCP6_Release_Output */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Release_Address
*
*   DESCRIPTION
*
*       This routine releases an address from an interface.
*
*   INPUTS
*
*       *addr_ptr               Pointer to the address to release.
*       dev_index               The index of the interface to which this
*                               address belongs.
*
*   OUTPUTS
*
*       None.
*
**************************************************************************/
VOID DHCP6_Release_Address(UINT8 *addr_ptr, UINT32 dev_index)
{
    DHCP6_STRUCT    *ds_ptr;
    DHCP6_TX_STRUCT *tx_ptr;
    DEV6_IF_ADDRESS *dv_addr = NU_NULL, *temp_addr = NU_NULL;
    DV_DEVICE_ENTRY *dv_ptr;

    /* Obtain the DHCPv6 semaphore. */
    if (NU_Obtain_Semaphore(&DHCP6_Cli_Resource,
                            NU_NO_SUSPEND) == NU_SUCCESS)
    {
        /* Get a pointer to the device structure. */
        dv_ptr = DEV_Get_Dev_By_Index(dev_index);

        if (dv_ptr != NU_NULL)
        {
            /* Get a pointer to the DHCPv6 structure associated with this address. */
            ds_ptr = DHCP6_Find_Struct_By_Dev(dev_index);

            /* If a structure was found. */
            if (ds_ptr)
            {
                /* Get a pointer to the head of the IPv6 address list. */
                if (dv_ptr->dev_flags & DV6_IPV6)
                    dv_addr = dv_ptr->dev6_addr_list.dv_head;

                /* If this is the only remaining address associated with
                 * the IAID, unset the IA_NA timer.
                 */
                while (dv_addr)
                {
                    /* If this address is being released, and the flag has been
                     * set to not send a release message.
                     */
                    if ( (dv_ptr->dev6_flags & DV6_NO_DHCP_RELEASE) &&
                         (dv_addr->dev6_addr_flags & ADDR6_DHCP_ADDR) &&
                         (dv_addr->dev6_addr_state & DV6_DETACHED) )
                    {
                        /* Clear the DHCP flag to indicate that this address is no longer
                         * being managed via the DHCP module.
                         */
                        dv_addr->dev6_addr_flags &= ~ADDR6_DHCP_ADDR;
                    }

                    /* If this is a DHCPv6 address, but not an address
                     * currently being released, save it.
                     */
                    if ( (dv_addr->dev6_addr_flags & ADDR6_DHCP_ADDR) &&
                         (!(dv_addr->dev6_addr_state & DV6_DETACHED)) )
                    {
                        temp_addr = dv_addr;
                    }

                    /* Get a pointer to the next entry in the list. */
                    dv_addr = dv_addr->dev6_next;
                }

                /* If no other DHCPv6 addresses were found on the interface,
                 * unset the IA_NA timer.
                 */
                if (!temp_addr)
                {
                    /* Unset the timer to renew the IA_NA. */
                    TQ_Timerunset(DHCP6_Renew_IA_NA_Event, TQ_CLEAR_EXACT,
                                  dev_index, DHCP6_RENEW);
                }

                /* If a release message should be sent. */
                if (!(dv_ptr->dev6_flags & DV6_NO_DHCP_RELEASE))
                {
                    /* Reserve a transmission structure for this transaction. */
                    tx_ptr = DHCP6_Obtain_TX_Struct();

                    if (tx_ptr)
                    {
                        /* Tie the transmission structure to the DHCPv6 structure. */
                        tx_ptr->dhcp6_ds_id = ds_ptr->dhcp6_id;

                        /* Set up the retransmission parameters for the request. */
                        tx_ptr->dhcp6_irt = DHCP6_REL_TIMEOUT;
                        tx_ptr->dhcp6_mrt = 0;
                        tx_ptr->dhcp6_mrc = DHCP6_REL_MAX_RC;
                        tx_ptr->dhcp6_mrd = 0;

                        /* Copy the address that is being released. */
                        NU_BLOCK_COPY(tx_ptr->dhcp6_release_addr, addr_ptr,
                                      IP6_ADDR_LEN);

                        /* Set an event to begin the release process. */
                        EQ_Put_Event(DHCP6_Release_Event, tx_ptr->dhcp6_id, 0);
                    }

                    else
                    {
                        NLOG_Error_Log("No DHCPv6 transmission structure available",
                                       NERR_RECOVERABLE, __FILE__, __LINE__);
                    }
                }
            }

            else
            {
                NLOG_Error_Log("No matching DHCPv6 structure found", NERR_RECOVERABLE,
                               __FILE__, __LINE__);
            }
        }

        else
        {
            NLOG_Error_Log("No matching network device found", NERR_RECOVERABLE,
                           __FILE__, __LINE__);
        }

        /* Release the DHCPv6 client semaphore. */
        NU_Release_Semaphore(&DHCP6_Cli_Resource);
    }

    /* Otherwise, log an error. */
    else
    {
        NLOG_Error_Log("Failed to obtain DHCPv6 semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

} /* DHCP6_Release_Address */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Decline_Output
*
*   DESCRIPTION
*
*       This routine transmits a Decline message.
*
*   INPUTS
*
*       *ds_ptr                 Pointer to DHCPv6 structure associated with
*                               the message.
*       *tx_ptr                 Pointer to transmission structure associated
*                               with this transaction.
*
*   OUTPUTS
*
*       NU_SUCCESS              The packet was successfully transmitted.
*       -1                      An error occurred.
*
**************************************************************************/
STATUS DHCP6_Decline_Output(DHCP6_STRUCT *ds_ptr, DHCP6_TX_STRUCT *tx_ptr)
{
    STATUS  status;
    UINT8   dhcp_buffer[DHCP6_BUFFER_LEN];
    UINT16  nbytes = DHCP6_MSG_LEN;

    /* Set up the message type. */
    tx_ptr->dhcp6_type = DHCP6_DECLINE;

    /* Build the common portion of the DHCPv6 client message. */
    DHCP6_Build_Packet(dhcp_buffer, tx_ptr);

    /* RFC 3315 - section 18.1.7 - The client places the identifier of the
     * server that allocated the address(es) in a Server Identifier option.
     */
    nbytes += DHCP6_Build_Server_ID_Option(&dhcp_buffer[nbytes], ds_ptr);

    /* Add the allowable user-specified options to the packet. */
    nbytes += DHCP6_Add_User_Options(&dhcp_buffer[nbytes], ds_ptr,
                                     tx_ptr, tx_ptr->dhcp6_type);

    if (nbytes < DHCP6_BUFFER_LEN)
    {
        /* RFC 3315 - section 18.1.7 - The client includes options containing
         * the IAs for the addresses it is declining in the "options" field.
         * The addresses to be declined MUST be included in the IAs.
         */
        nbytes += DHCP6_Add_System_IA_NA_Option(&dhcp_buffer[nbytes], ds_ptr,
                                                ADDR6_DHCP_DECLINE);

        /* Send the packet. */
        status = DHCP6_Output(dhcp_buffer, nbytes, ds_ptr, tx_ptr);
    }

    else
    {
        status = -1;
    }

    /* If this message has been transmitted the max number of times,
     * remove the address from the list and deallocate the memory for it.
     */
    if (status != NU_SUCCESS)
    {
        /* Free the transmission structure entry. */
        DHCP6_Free_TX_Struct(tx_ptr);

        DHCP6_Free_Address(ds_ptr);
    }

    return (status);

} /* DHCP6_Decline_Output */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Free_Address
*
*   DESCRIPTION
*
*       This routine frees the memory for addresses that have been
*       successfully released or declined.
*
*   INPUTS
*
*       *ds_ptr                 Pointer to DHCPv6 structure associated with
*                               the addresses.
*
*   OUTPUTS
*
*       None.
*
**************************************************************************/
VOID DHCP6_Free_Address(DHCP6_STRUCT *ds_ptr)
{
    STATUS          status;
    DV_DEVICE_ENTRY *dev_ptr;
    DEV6_IF_ADDRESS *addr_ptr = NU_NULL, *next_addr;

    /* Obtain the TCP semaphore. */
    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Get a pointer to the interface. */
        dev_ptr = DEV_Get_Dev_By_Index(ds_ptr->dhcp6_dev_index);

        /* If an interface was found. */
        if (dev_ptr)
        {
            /* Get a pointer to the address list associated with this
               interface. */
            if (dev_ptr->dev_flags & DV6_IPV6)
                addr_ptr = dev_ptr->dev6_addr_list.dv_head;
        }

        /* If there are addresses on the interface. */
        while (addr_ptr)
        {
            /* Get a pointer to the next address in the list. */
            next_addr = addr_ptr->dev6_next;

            /* If this address is to be deleted. */
            if ( (addr_ptr->dev6_addr_flags & ADDR6_DHCP_ADDR) &&
                 (addr_ptr->dev6_addr_state & DV6_DETACHED) )
            {
                /* Remove the address from the list of addresses for the
                 * interface
                 */
                DLL_Remove(&dev_ptr->dev6_addr_list, addr_ptr);

                /* Deallocate the memory being used by the address data
                 * structure
                 */
                if (NU_Deallocate_Memory((VOID*)addr_ptr) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to deallocate the memory for the IPv6 address",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }
            }

            /* Get a pointer to the next address in the list. */
            addr_ptr = next_addr;
        }

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
        }
    }

} /* DHCP6_Free_Address */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Decline_Address
*
*   DESCRIPTION
*
*       This routine declines an address or set of addresses on an
*       interface.
*
*   INPUTS
*
*       *addr_ptr               A pointer to the address structure of the
*                               address being declined.
*
*   OUTPUTS
*
*       None.
*
**************************************************************************/
VOID DHCP6_Decline_Address(DEV6_IF_ADDRESS *addr_ptr)
{
    DHCP6_TX_STRUCT *tx_ptr;
    DHCP6_STRUCT    *ds_ptr;

    /* Obtain the DHCPv6 client semaphore. */
    if (NU_Obtain_Semaphore(&DHCP6_Cli_Resource,
                            NU_NO_SUSPEND) == NU_SUCCESS)
    {
        /* Get a pointer to the DHCPv6 structure associated with this
         * device.
         */
        ds_ptr = DHCP6_Find_Struct_By_Dev(addr_ptr->dev6_device->dev_index);

        if (ds_ptr)
        {
            /* Reserve a transmission structure for this transaction. */
            tx_ptr = DHCP6_Obtain_TX_Struct();

            if (tx_ptr)
            {
                /* Tie the transmission structure to the DHCPv6 structure. */
                tx_ptr->dhcp6_ds_id = ds_ptr->dhcp6_id;

                /* Set up the retransmission parameters for the request. */
                tx_ptr->dhcp6_irt = DHCP6_DEC_TIMEOUT;
                tx_ptr->dhcp6_mrt = 0;
                tx_ptr->dhcp6_mrc = DHCP6_DEC_MAX_RC;
                tx_ptr->dhcp6_mrd = 0;

                /* Set the flag indicating that this address needs to be
                 * declined.
                 */
                addr_ptr->dev6_addr_flags |= ADDR6_DHCP_DECLINE;

                /* Set an event to begin the decline process. */
                EQ_Put_Event(DHCP6_Decline_Event, tx_ptr->dhcp6_id, 0);
            }

            else
            {
                NLOG_Error_Log("No DHCPv6 transmission structure available",
                               NERR_RECOVERABLE, __FILE__, __LINE__);
            }
        }

        /* Release the DHCPv6 client semaphore. */
        NU_Release_Semaphore(&DHCP6_Cli_Resource);
    }

    /* Otherwise, log an error. */
    else
    {
        NLOG_Error_Log("Failed to obtain DHCPv6 semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

} /* DHCP6_Decline_Address */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Compute_Retransmission_Time
*
*   DESCRIPTION
*
*       This routine computes the retransmission time for the next packet
*       as specified in RFC 3315, section 14.
*
*   INPUTS
*
*       *tx_ptr                 Pointer to the transmission structure
*                               associated with this transaction.
*       *ds_ptr                 Pointer to the DHCPv6 structure associated
*                               with the interface.
*
*   OUTPUTS
*
*       NU_SUCCESS              The retransmission time is stored in
*                               dhcp6_rt of the DHCPv6 structure.
*       -1                      The packet cannot be retransmitted again.
*
**************************************************************************/
STATIC STATUS DHCP6_Compute_Retransmission_Time(DHCP6_TX_STRUCT *tx_ptr,
                                                DHCP6_STRUCT *ds_ptr)
{
    UINT32  initial_rand_num, rand_num;
    STATUS  status = NU_SUCCESS;

    /* If this is the first transmission of this packet. */
    if (tx_ptr->dhcp6_rt == 0)
    {
        /* RFC 3315 - section 14 - The message exchange fails once MRD
         * seconds have elapsed since the client first transmitted the
         * message.  So, initialize the retransmission time to the time
         * at which the first packet was sent.
         */
        tx_ptr->dhcp6_init_retrans = NU_Retrieve_Clock();
    }

    /* Increment the number of times this packet has been transmitted. */
    tx_ptr->dhcp6_retrans_count ++;

    /* If this packet has not been retransmitted the maximum number of times,
     * or mrc is zero, set the timer to retransmit it again.  And, if this
     * packet has not be attempted to be retransmitted for the maximum amount
     * of time or mrd is zero, set the timer to transmit it again.
     */
    if ( ((tx_ptr->dhcp6_mrc == 0) ||
          (tx_ptr->dhcp6_retrans_count < tx_ptr->dhcp6_mrc)) &&
         ((tx_ptr->dhcp6_mrd == 0) ||
          (TQ_Check_Duetime(tx_ptr->dhcp6_mrd +
                            tx_ptr->dhcp6_init_retrans) > 0)) )
    {
        /* Generate a random number. */
        initial_rand_num = UTL_Rand();

        /* RFC 3315 - section 14 - RAND is a random number chosen with a
         * uniform distribution between -0.1 and +0.1, exclusive.
         */
        rand_num = (initial_rand_num % 9);

        /* Use the random number to index into the array that is loaded with
         * numbers that when divided into IRT will effectively make RAND a
         * value between 0 and +.1.  Efforts are taken later to randomly make
         * this a number between -0.1 and +0.1.
         */
        rand_num = DHCP6_Rand_Array[rand_num];

        /* If this is the first transmission of this packet, use the initial
         * transmission algorithm.
         */
        if (tx_ptr->dhcp6_rt == 0)
        {
            /* For the first message, transmission is based on IRT:
             * RT = IRT + (RAND * IRT).  In order to create a negative
             * randomization factor, if the initial random number generated
             * is even, use a positive random number.
             *
             * RFC 3315 - section 17.1.2 - The first RT of a Solicit that
             * is expecting an Advertise MUST be selected to be strictly
             * greater than IRT by choosing RAND to be strictly greater than
             * 0.
             */
            if ( (initial_rand_num % 2 == 0) ||
                 ((tx_ptr->dhcp6_type == DHCP6_SOLICIT) &&
                  (!(ds_ptr->dhcp6_flags & DHCP6_ALLOW_RAPID_COMMIT))) )
            {
                tx_ptr->dhcp6_rt =
                    tx_ptr->dhcp6_irt + (tx_ptr->dhcp6_irt / rand_num);
            }

            /* If the initial random number generated is odd, use a negative
             * random number.
             */
            else
            {
                tx_ptr->dhcp6_rt =
                    tx_ptr->dhcp6_irt - (tx_ptr->dhcp6_irt / rand_num);
            }
        }

        /* Retransmission time for each subsequent message transmission is
         * based on the previous value of RT:
         * RT = (2 * RTprev) + (RAND * RTprev)
         */
        else
        {
            /* Compute the next retransmission time value. In order to create
             * a negative randomization factor, if the initial random number
             * generated is even, use a positive random number.
             */
            if (initial_rand_num % 2 == 0)
            {
                tx_ptr->dhcp6_rt =
                    (2 * tx_ptr->dhcp6_rt) + (tx_ptr->dhcp6_rt / rand_num);
            }

            /* If the initial random number generated is odd, use a negative
             * random number.
             */
            else
            {
                tx_ptr->dhcp6_rt =
                    (2 * tx_ptr->dhcp6_rt) - (tx_ptr->dhcp6_rt / rand_num);
            }

            /* If retransmission time has reached the upper bound, reset it.
             * If max retransmission time has a value of 0, there is no
             * upper limit on the value of the retransmission time.
             */
            if ( (tx_ptr->dhcp6_mrt != 0) &&
                 (tx_ptr->dhcp6_rt > tx_ptr->dhcp6_mrt) )
            {
                /* In order to create a negative randomization factor, if the
                 * initial random number generated is even, use a positive
                 * random number.
                 */
                if (initial_rand_num % 2 == 0)
                {
                    tx_ptr->dhcp6_rt =
                        tx_ptr->dhcp6_mrt + (tx_ptr->dhcp6_mrt / rand_num);
                }

                /* If the initial random number generated is odd, use a
                 * negative random number.
                 */
                else
                {
                    tx_ptr->dhcp6_rt =
                        tx_ptr->dhcp6_mrt - (tx_ptr->dhcp6_mrt / rand_num);
                }
            }

            /* If the new RT will make this packet exceed the MRD, stop
             * transmitting the packet.
             */
            if ( (tx_ptr->dhcp6_mrd != 0) &&
                 (TQ_Check_Duetime(tx_ptr->dhcp6_mrd +
                                   tx_ptr->dhcp6_init_retrans) < tx_ptr->dhcp6_rt) )
            {
                status = -1;
            }
        }
    }

    /* Packet has been retransmitted maximum number of times. */
    else
    {
        status = -1;

        NLOG_Error_Log("DHCPv6 client packet retransmitted maximum number of times",
                       NERR_SEVERE, __FILE__, __LINE__);
    }

    return (status);

} /* DHCP6_Compute_Retransmission_Time */

/*************************************************************************
*
*   FUNCTION
*
*       DHCP6_Obtain_TX_Struct
*
*   DESCRIPTION
*
*       This routine obtains a DHCP transmission structure.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       A pointer to the DHCP6_TX_STRUCT or NU_NULL if there are no free
*       structures available.
*
*************************************************************************/
DHCP6_TX_STRUCT *DHCP6_Obtain_TX_Struct(VOID)
{
    DHCP6_TX_STRUCT *tx_ptr = NU_NULL;
    INT             i;

    /* Loop through the list of structures until an available one is
     * found.
     */
    for (i = 0; i < DHCP6_SIM_TX_COUNT; i++)
    {
        /* If this entry is not in use, reserve it. */
        if (DHCP6_Tx_List[i].dhcp6_use == 0)
        {
            /* Return a pointer to this entry. */
            tx_ptr = &DHCP6_Tx_List[i];

            /* Indicate that the entry is in use. */
            tx_ptr->dhcp6_use = 1;

            /* Set the ID for this structure. */
            tx_ptr->dhcp6_id = DHCP6_Tx_Id ++;

            /* Never let DHCP6_Tx_Id wrap to zero as this ID is unused. */
            if (DHCP6_Tx_Id == 0)
            {
                DHCP6_Tx_Id = 1;
            }

            /* Generate a random transaction ID for this structure. */
            DHCP6_Trans_ID_Value[0] += (UINT8)UTL_Rand();
            DHCP6_Trans_ID_Value[1] += (UINT8)UTL_Rand();
            DHCP6_Trans_ID_Value[2] += (UINT8)UTL_Rand();

            /* Copy the transaction ID into the DHCPv6 data structure. */
            memcpy(tx_ptr->dhcp6_xid, DHCP6_Trans_ID_Value, DHCP6_ID_LEN);

            break;
        }
    }

    return (tx_ptr);

} /* DHCP6_Obtain_TX_Struct */

#endif
