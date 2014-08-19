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
*       dhcp6_input.c                                       
*
*   DESCRIPTION
*
*       This file contains functions necessary for processing incoming
*       DHCPv6 packets on the client.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       DHCP6_Interpret
*       DHCP6_Process_Options
*       DHCP6_Advertise_Input
*       DHCP6_Reply_Input
*       DHCP6_Reconfigure_Input
*       DHCP6_Find_Struct_By_XID
*
*   DEPENDENCIES
*
*       target.h                                                        
*       externs.h
*       externs6.h
*       dhcp6.h
*
**************************************************************************/

#include "networking/target.h"
#include "networking/externs.h"
#include "networking/externs6.h"
#include "networking/dhcp6.h"

#if (INCLUDE_DHCP6 == NU_TRUE)

STATIC STATUS DHCP6_Process_Options(UINT8 *, DHCP6_STRUCT *, 
                                    DHCP6_TX_STRUCT *, DHCP6_OPT_STRUCT *);

/*************************************************************************
*
*   FUNCTION
*
*       DHCP6_Interpret
*
*   DESCRIPTION
*
*       This routine processes an incoming DHCPv6 packet.
*
*       0                   1                   2                   3
*       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*      |    msg-type   |               transaction-id                  |
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*      |                                                               |
*      .                            options                            .
*      .                           (variable)                          .
*      |                                                               |
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*
*   INPUTS
*
*       *buffer             A pointer to the buffer containing the
*                           DHCPv6 packet.
*       buf_len             The length of the DHCPv6 packet.
*       *dest_addr          The destination address to which this packet
*                           was sent.
*       if_index            The index of the interface that received this 
*                           packet.
*
*   OUTPUTS
*
*       NU_SUCCESS          The packet was successfully processed.
*       -1                  An error was encountered.
*
*************************************************************************/
STATUS DHCP6_Interpret(UINT8 *buffer, UINT16 buf_len, UINT8 *dest_addr,
                       UINT32 if_index)
{
    STATUS          status;
    DHCP6_STRUCT    *ds_ptr;
    DHCP6_TX_STRUCT *tx_ptr = NU_NULL;
    UINT8           packet_type;

    /* Extract the packet type from the incoming packet. */
    packet_type = GET8(buffer, DHCP6_MSG_TYPE_OFFSET);

    /* If the packet type is not Reconfigure, find the DHCP structure
     * associated with the transaction-id in the packet.
     */
    if (packet_type != DHCP6_RECONFIGURE)
    {
        /* Get a pointer to the DHCP transmission structure associated with 
         * the transaction ID in the packet.
         */
        tx_ptr = DHCP6_Find_TX_Struct_By_XID(&buffer[DHCP6_TRANS_ID_OFFSET]);

        if (tx_ptr) 
        {
            /* Get a pointer to the DHCPv6 structure associated with the
             * transmission structure.
             */
            ds_ptr = DHCP6_Find_Struct_By_ID(tx_ptr->dhcp6_ds_id);
        }

        else
        {
            ds_ptr = NU_NULL;

            NLOG_Error_Log("No matching transmission structure for incoming DHCPv6 packet", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);
        }
    }

    else
    {
        /* Get a pointer to the DHCPv6 structure associated with the 
         * interface on which this packet was received.
         */
        ds_ptr = DHCP6_Find_Struct_By_Dev(if_index);
    }

    /* If a matching structure was found or this is a Reconfigure message. */
    if (ds_ptr)
    {
        /* Process the DHCPv6 packet type. */
        switch (packet_type)
        {
        case DHCP6_ADVERTISE:

            /* Process the incoming Adertise message. */
            status = DHCP6_Advertise_Input(buffer, buf_len, ds_ptr, tx_ptr);

            break;

        case DHCP6_REPLY:

            /* Process the incoming Reply message. */
            status = DHCP6_Reply_Input(buffer, buf_len, ds_ptr, tx_ptr);

            break;

        case DHCP6_RECONFIGURE:

            /* RFC 3315 - section 15.11 - Discard the Reconfigure message
             * if it was not unicast to the client.
             */
            if (!(IPV6_IS_ADDR_MULTICAST(dest_addr)))
            {
                /* Process the incoming Reconfigure message. */
                status = DHCP6_Reconfigure_Input(buffer, buf_len, ds_ptr);
            }

            /* Log an error and return an error status. */
            else
            {
                status = -1;

                NLOG_Error_Log("DHCPv6 Reconfigure packet not sent to unicast address", 
                               NERR_INFORMATIONAL, __FILE__, __LINE__);
            }

            break;

        default:

            /* The packet type must be ignored by the DHCP Client. */
            status = -1;

            NLOG_Error_Log("Invalid DHCPv6 packet type received on client", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);

            break;
        }
    }

    /* Log an error and return an error code. */
    else
    {
        status = -1;

        NLOG_Error_Log("No matching transaction ID for incoming DHCPv6 packet", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);
    }

    return (status);

} /* DHCP6_Interpret */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Process_Options
*
*   DESCRIPTION
*
*       This routine processes the options appended to an incoming DHCPv6
*       server message that were not already processed by the calling
*       routine.
*
*   INPUTS
*
*       *buffer                 Pointer to the buffer in which the 
*                               options are contained.
*       *ds_ptr                 Pointer to DHCP structure associated with
*                               the packet.
*       *tx_ptr                 Pointer to the transmission structure
*                               associated with this transaction.
*       *parse_opts             Pointer to the options contained in the
*                               DHCPv6 server packet.
*
*   OUTPUTS
*
*       NU_SUCCESS              The options were successfully processed.
*       DHCP6_RETRANS_MSG       The message that prompted this message
*                               should be retransmitted.
*       DHCP6_FIND_SERVER       The client should stop transmitting the
*                               message that prompted this message and
*                               begin Server Discovery to find a new
*                               DHCPv6 server.
*       DHCP6_SEND_REQUEST      The client should stop transmitting the
*                               message that prompted this message and
*                               transmit a Request message.
*       -1                      An option contains an unacceptable value.
*
**************************************************************************/
STATIC STATUS DHCP6_Process_Options(UINT8 *buffer, DHCP6_STRUCT *ds_ptr, 
                                    DHCP6_TX_STRUCT *tx_ptr,
                                    DHCP6_OPT_STRUCT *parse_opts)
{
    STATUS  status = NU_SUCCESS;
    INT     i;
    UINT32  count, nbytes;
    UINT16  opt_code;

    /* Process each option that has not already been processed. */
    for (i = 1; i < DHCP6_MAX_OPT_CODES; i++)
    {
        /* If this option needs to be processed, process it now. */
        if (parse_opts[i].dhcp6_opt_addr != 0xffffffff)
        {
            /* Save off the number of options of this type in the packet. */
            count = parse_opts[i].dhcp6_opt_count;

            /* Set the byte counter to the address of this first option. */
            nbytes = parse_opts[i].dhcp6_opt_addr;

            /* Process each option of this type in the packet. */
            do
            {
                /* If this option is supported, parse it. */
                if (parse_opts[i].dhcp6_parse_opt != NU_NULL)
                {
                    status = parse_opts[i].dhcp6_parse_opt(&buffer[nbytes], 
                                                           ds_ptr, tx_ptr);
                }

                /* If the option contains an error, stop processing and 
                 * return an error. 
                 */
                if (status != NU_SUCCESS)
                {
                    NLOG_Error_Log("Error in incoming DHCPv6 option", 
                                   NERR_INFORMATIONAL, __FILE__, __LINE__); 

                    break;
                }

                count --;

                /* If there are more options of this type. */
                if (count)
                {
                    /* Find the next instance of this option. */
                    do
                    {
                        /* Get the length of this option. */
                        nbytes += (GET16(&buffer[nbytes], DHCP6_OPT_LEN_OFFSET) +   
                            DHCP6_MSG_LEN);

                        /* Get the option code. */
                        opt_code = GET16(&buffer[nbytes], DHCP6_OPT_CODE_OFFSET);

                    } while (opt_code != i);
                }

            } while (count);
        }
    }

    return (status);

} /* DHCP6_Process_Options */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Advertise_Input
*
*   DESCRIPTION
*
*       This routine processes an incoming DHCPv6 Advertise message.
*       A server sends an Advertise message to indicate that it is 
*       available for DHCP service, in response to a Solicit message 
*       received from a client.
*
*   INPUTS
*
*       *buffer                 Pointer to the buffer in which the 
*                               message is contained.
*       buf_len                 The length of the DHCPv6 message.
*       *ds_ptr                 Pointer to DHCP structure associated with
*                               the packet.
*       *tx_ptr                 Pointer to the transmission structure
*                               associated with the transaction.
*
*   OUTPUTS
*
*       NU_SUCCESS              The packet was successfully processed.
*       -1                      The packet contains an error.
*
**************************************************************************/
STATUS DHCP6_Advertise_Input(UINT8 *buffer, UINT16 buf_len, 
                             DHCP6_STRUCT *ds_ptr, DHCP6_TX_STRUCT *tx_ptr)
{
    STATUS              status = NU_SUCCESS;
    DHCP6_OPT_STRUCT    parse_opts[DHCP6_MAX_OPT_CODES];

    /* Strip off the DHCPv6 message header. */
    buffer += DHCP6_MSG_LEN;

    /* Initialize the options parsing structure according to the template
     * structure.
     */
    NU_BLOCK_COPY(parse_opts, DHCP6_Options, 
                  sizeof(DHCP6_OPT_STRUCT) * DHCP6_MAX_OPT_CODES);

    /* Determine what options are present in the packet. */
    if (DHCP6_Parse_Server_Options(buffer, buf_len - DHCP6_MSG_LEN, 
                                   parse_opts, DHCP6_ADVERTISE) != NU_SUCCESS)
    {
        status = -1;

        NLOG_Error_Log("DHCPv6 Advertise message contains invalid options", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);
    }

    /* RFC 3315 - section 15.3 - Discard the message if it does not include 
     * a Server Identifier option.
     */
    else if (parse_opts[DHCP6_OPT_SERVERID].dhcp6_opt_addr != 0xffffffff)
    {
        /* RFC 3315 - section 15.3 - Discard the message if it does not 
         * include a Client Identifier option.
         */
        if (parse_opts[DHCP6_OPT_CLIENTID].dhcp6_opt_addr != 0xffffffff)
        {
            /* RFC 3315 - section 15.3 - Discard the message if the contents 
             * of the Client Identifier option do not match the client's 
             * DUID.
             */
            if (DHCP6_Parse_Client_ID_Option(&buffer[parse_opts
                                             [DHCP6_OPT_CLIENTID].dhcp6_opt_addr], 
                                             ds_ptr, tx_ptr) == NU_SUCCESS)
            {
                /* Indicate that this option has already been parsed. */
                parse_opts[DHCP6_OPT_CLIENTID].dhcp6_opt_addr = 0xffffffff;

                /* If the packet contains a Status Code option. */
                if (parse_opts[DHCP6_OPT_STATUS_CODE].dhcp6_opt_addr != 0xffffffff)
                {
                    /* Parse the Status Code option. */
                    DHCP6_Parse_Status_Code_Option(&buffer[parse_opts
                                                   [DHCP6_OPT_STATUS_CODE].
                                                   dhcp6_opt_addr], ds_ptr, 
                                                   tx_ptr);

                    /* RFC 3315 - section 17.1.3 - The client MUST ignore 
                     * any Advertise message that includes a Status Code 
                     * option containing the value NoAddrsAvail.
                     */
                    if (ds_ptr->dhcp6_status_code == DHCP6_STATUS_NO_ADDRS_AVAIL)
                    {
                        status = -1;

                        NLOG_Error_Log("DHCPv6 Advertise message contains invalid status", 
                                       NERR_INFORMATIONAL, __FILE__, __LINE__);
                    }
                }
            }

            /* Log an error and return an error code. */
            else
            {
                status = -1;

                NLOG_Error_Log("DHCPv6 Client ID option does not match client", 
                               NERR_INFORMATIONAL, __FILE__, __LINE__);
            }
        }

        /* Log an error and return an error code. */
        else
        {
            status = -1;

            NLOG_Error_Log("DHCPv6 Advertise message must contain Client ID option", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);
        }
    }

    /* Log an error and return an error code. */
    else
    {
        status = -1;

        NLOG_Error_Log("DHCPv6 Advertise message must contain Server ID option", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);
    }

    /* If the message passed validation. */
    if (status == NU_SUCCESS)
    {   
        /* If the packet does not contain a Preference option, a preference 
         * of zero is to be assumed for this server.  If another server has
         * already responded, ignore this server.
         */
        if ( (parse_opts[DHCP6_OPT_PREFERENCE].dhcp6_opt_addr == 0xffffffff) &&
             (ds_ptr->dhcp6_server.dhcp6_resp_time != 0) )
        {
            status = -1;

            NLOG_Error_Log("Ignoring DHCP server due to preference option", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);
        }

        /* If this server is going to be saved. */
        if (status == NU_SUCCESS)
        {
            /* Initialize the server status to NU_SUCCESS. */
            ds_ptr->dhcp6_status_code = NU_SUCCESS;

            /* Parse the remaining options to ensure all values are valid. */
            if (DHCP6_Process_Options(buffer, ds_ptr, tx_ptr,
                                      parse_opts) != NU_SUCCESS)
            {
                status = -1;

                NLOG_Error_Log("DHCPv6 Advertise message contains invalid option", 
                               NERR_INFORMATIONAL, __FILE__, __LINE__);
            }

            /* Store the server's DUID. */
            else
            {
                /* Store the response time. */
                ds_ptr->dhcp6_server.dhcp6_resp_time = NU_Retrieve_Clock();

                /* RFC 3315 - section 17.1.2 - If the client receives an 
                 * Advertise message that includes a Preference option with
                 * a preference value of 255, the client immediately begins 
                 * a client-initiated message exchange ... The client 
                 * terminates the retransmission process as soon as it 
                 * receives any Advertise message, and the client acts on 
                 * the received Advertise message without waiting for any 
                 * additional Advertise messages.    
                 */
                if ( (ds_ptr->dhcp6_server.dhcp6_pref == DHCP6_MAX_SERVER_PREF) || 
                     (tx_ptr->dhcp6_retrans_count != 1) )
                {
                    /* Free the transmission structure. */
                    DHCP6_Free_TX_Struct(tx_ptr);

                    /* Obtain an IPv6 address. */
                    DHCP6_Obtain_Address(ds_ptr);
                }
            }
        }
    }

    return (status);

} /* DHCP6_Advertise_Input */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Reply_Input
*
*   DESCRIPTION
*
*       This routine processes an incoming DHCPv6 Reply message.
*
*   INPUTS
*
*       *buffer                 Pointer to the buffer in which the 
*                               message is contained.
*       buf_len                 The length of the DHCPv6 message.
*       *ds_ptr                 Pointer to DHCP structure associated with
*                               the packet.
*       *tx_ptr                 Pointer to the transmission structure
*                               associated with this transaction.
*
*   OUTPUTS
*
*       NU_SUCCESS              The packet was successfully processed.
*       -1                      The packet contains an error.
*
**************************************************************************/
STATUS DHCP6_Reply_Input(UINT8 *buffer, UINT16 buf_len, DHCP6_STRUCT *ds_ptr,
                         DHCP6_TX_STRUCT *tx_ptr)
{
    STATUS              status = NU_SUCCESS;
    DHCP6_OPT_STRUCT    parse_opts[DHCP6_MAX_OPT_CODES];

    /* Strip off the DHCPv6 message header. */
    buffer += DHCP6_MSG_LEN;

    /* Initialize the options parsing structure according to the template
     * structure.
     */
    NU_BLOCK_COPY(parse_opts, DHCP6_Options, 
                  sizeof(DHCP6_OPT_STRUCT) * DHCP6_MAX_OPT_CODES);

    /* Determine what options are present in the packet. */
    if (DHCP6_Parse_Server_Options(buffer, buf_len - DHCP6_MSG_LEN, 
                                   parse_opts, DHCP6_REPLY) != NU_SUCCESS)
    {
        status = -1;

        NLOG_Error_Log("DHCPv6 Reply message contains invalid options", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);
    }

    /* RFC 3315 - section 15.10 - Discard the message if it does not include 
     * a Server Identifier option.
     */
    else if (parse_opts[DHCP6_OPT_SERVERID].dhcp6_opt_addr != 0xffffffff)
    {
        /* RFC 3315 - section 15.10 - If the client included a Client 
         * Identifier option in the original message, the Reply message 
         * MUST include a Client Identifier option.  All DHCPv6 client
         * messages include a Client Identifier option in Nucleus IPv6.
         */
        if (parse_opts[DHCP6_OPT_CLIENTID].dhcp6_opt_addr != 0xffffffff)
        {
            /* RFC 3315 - section 15.10 - Discard the message if the 
             * contents of the Client Identifier option do not match the 
             * client's DUID.
             */
            if (DHCP6_Parse_Client_ID_Option(&buffer[parse_opts
                                             [DHCP6_OPT_CLIENTID].
                                             dhcp6_opt_addr], 
                                             ds_ptr, tx_ptr) != NU_SUCCESS)
            {
                status = -1;

                NLOG_Error_Log("DHCPv6 Client ID option does not match client", 
                               NERR_INFORMATIONAL, __FILE__, __LINE__);
            }

            /* Indicate that this option has already been parsed. */
            parse_opts[DHCP6_OPT_CLIENTID].dhcp6_opt_addr = 0xffffffff;
        }

        /* Log an error and return an error code. */
        else
        {
            status = -1;

            NLOG_Error_Log("DHCPv6 Reply message must contain Client ID option", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);
        }
    }

    /* Log an error and return an error code. */
    else
    {
        status = -1;

        NLOG_Error_Log("DHCPv6 Reply message must contain Server ID option", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);
    }

    /* If the message passed validation. */
    if (status == NU_SUCCESS)
    {
        /* RFC 3315 - section 17.1.4 - If the client includes a Rapid Commit 
         * option in the Solicit message, it will expect a Reply message 
         * that includes a Rapid Commit option in response.  The client 
         * discards any Reply messages it receives that do not include a 
         * Rapid Commit option.
         */
        if ( (tx_ptr->dhcp6_type == DHCP6_SOLICIT) && 
             (ds_ptr->dhcp6_flags & DHCP6_ALLOW_RAPID_COMMIT) )
        {
            /* If there is no Rapid Commit option. */
            if (parse_opts[DHCP6_OPT_RAPID_COMMIT].dhcp6_opt_addr == 0xffffffff)
            {
                status = -1;

                NLOG_Error_Log("DHCPv6 Reply message must contain Rapid Commit option", 
                               NERR_INFORMATIONAL, __FILE__, __LINE__);
            }
        }

        /* If all options were parsed successfully and DHCPv6 configuration
         * is complete.
         */
        if (status == NU_SUCCESS)
        {
            /* RFC 3315 - section 18.1.8 - Upon the receipt of a valid Reply 
             * message in response to a Solicit, Request, Confirm, Renew, 
             * Rebind or Information-request message, the client extracts 
             * the configuration information contained in the Reply...
             * When the client receives a valid Reply in response to a
             * Release or Decline message, the client considers the event
             * completed regardless of the Status Code option(s) returned
             * by the server.
             */
            if ( (tx_ptr->dhcp6_type == DHCP6_SOLICIT) ||
                 (tx_ptr->dhcp6_type == DHCP6_REQUEST) ||
                 (tx_ptr->dhcp6_type == DHCP6_CONFIRM) ||
                 (tx_ptr->dhcp6_type == DHCP6_RENEW) ||
                 (tx_ptr->dhcp6_type == DHCP6_REBIND) || 
                 (tx_ptr->dhcp6_type == DHCP6_INFO_REQUEST))
            {   
                /* Initialize the status code to success, so if this message
                 * does not contain a status code, an old value will not be
                 * used in determining what to do on failure. 
                 */
                ds_ptr->dhcp6_status = NU_SUCCESS;

                /* Parse the remaining options to ensure all values are 
                 * valid. 
                 */
                if (DHCP6_Process_Options(buffer, ds_ptr, tx_ptr,
                                          parse_opts) != NU_SUCCESS)
                {
                    /* If the server returned a status indicating that the
                     * client should transmit a Request packet. 
                     */
                    if (ds_ptr->dhcp6_status == DHCP6_SEND_REQUEST)
                    {
                        /* Free the transmission structure. */
                        DHCP6_Free_TX_Struct(tx_ptr);

                        /* Begin the request process. */
                        DHCP6_Obtain_Address(ds_ptr);
                    }

                    /* If the server returned a status indicating that the
                     * client should begin server discovery or an error
                     * occurred that does not require the server to
                     * retransmit the last message sent.
                     */
                    else if (ds_ptr->dhcp6_status != DHCP6_RETRANS_MSG)
                    {
                        /* Free the transmission structure. */
                        DHCP6_Free_TX_Struct(tx_ptr);

                        /* RFC 3315 - section 18.1.1 - If the message 
                         * exchange fails, the client takes an action based 
                         * on the client's local policy.  In this case, 
                         * initiate the server discovery process to find a 
                         * new server.
                         */
                        DHCP6_Find_Server(ds_ptr);
                    }

                    status = -1;

                    NLOG_Error_Log("DHCPv6 Reply message contains invalid option", 
                                   NERR_INFORMATIONAL, __FILE__, __LINE__);
                }

                /* If this is not a Renew message, or the address requesting
                 * renewal were included in the packet.  Otherwise, continue 
                 * to retransmit the Renew until the requested addresses are 
                 * renewed.
                 */
                else if ( ((tx_ptr->dhcp6_type != DHCP6_RENEW) &&
                           (tx_ptr->dhcp6_type != DHCP6_REBIND)) ||
                          (parse_opts[DHCP6_OPT_IAADDR].dhcp6_opt_addr != 0xffffffff) )
                {
                    /* Clear the reconfigure in progress flag. */
                    ds_ptr->dhcp6_flags &= ~DHCP6_RECON_IN_PROGRESS;

                    /* If this is a response to a Release or Decline message. */
                    if ( (tx_ptr->dhcp6_type == DHCP6_RELEASE) || 
                         (tx_ptr->dhcp6_type == DHCP6_DECLINE) )
                    {
                        /* Finalize deletion of the address. */
                        DHCP6_Free_Address(ds_ptr);
                    }

                    /* Free the transmission structure. */
                    DHCP6_Free_TX_Struct(tx_ptr);

                    /* If there is a task suspended awaiting completion. */
                    if (ds_ptr->dhcp6_task)
                    {
                        /* Resume the task. */
                        NU_Resume_Task(ds_ptr->dhcp6_task);
                    }
                }
            }

            /* Any reply is acceptable to stop retransmission of the 
             * message. 
             */
            else
            {
                /* Clear the reconfigure in progress flag. */
                ds_ptr->dhcp6_flags &= ~DHCP6_RECON_IN_PROGRESS;

                /* If the Reply is in response to a Release or Decline,
                 * the address is partially deleted from the interface.
                 * Finish deletion of the address now.
                 */
                if ( (tx_ptr->dhcp6_type == DHCP6_RELEASE) ||
                     (tx_ptr->dhcp6_type == DHCP6_DECLINE) )
                {
                    /* Finalize deletion of the address. */
                    DHCP6_Free_Address(ds_ptr);
                }

                /* Free the transmission structure. */
                DHCP6_Free_TX_Struct(tx_ptr);
            }
        }
    }

    return (status);

} /* DHCP6_Reply_Input */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Reconfigure_Input
*
*   DESCRIPTION
*
*       This routine processes an incoming DHCPv6 Reconfigure message.  
*       A server sends a Reconfigure message to cause a client to initiate
*       immediately a Renew/Reply or Information-request/Reply message
*       exchange with the server.
*
*   INPUTS
*
*       *buffer                 Pointer to the buffer in which the 
*                               message is contained.
*       buf_len                 The length of the DHCPv6 message.
*       *ds_ptr                 Pointer to DHCP structure associated with
*                               the packet.
*
*   OUTPUTS
*
*       NU_SUCCESS              The packet was successfully processed.
*       -1                      The packet contains an error.
*
**************************************************************************/
STATUS DHCP6_Reconfigure_Input(UINT8 *buffer, UINT16 buf_len, 
                               DHCP6_STRUCT *ds_ptr)
{
    STATUS              status = NU_SUCCESS;
    DHCP6_OPT_STRUCT    parse_opts[DHCP6_MAX_OPT_CODES];

    /* RFC 3315 - section 19.4.1 - Once the client has received a Reconfigure, 
     * the client proceeds with the message exchange (retransmitting the Renew 
     * or Information-request message if necessary); the client ignores any 
     * additional Reconfigure messages until the exchange is complete.
     */
    if ( (ds_ptr->dhcp6_flags & DHCP6_RECON_IN_PROGRESS) ||
         (!(ds_ptr->dhcp6_flags & DHCP6_RECON_ACCEPT)) )
    {
        return (NU_SUCCESS);
    }

    /* Strip off the DHCPv6 message header. */
    buffer += DHCP6_MSG_LEN;

    /* Initialize the options parsing structure according to the template
     * structure.
     */
    NU_BLOCK_COPY(parse_opts, DHCP6_Options, 
                  sizeof(DHCP6_OPT_STRUCT) * DHCP6_MAX_OPT_CODES);

    /* Determine what options are present in the packet. */
    if (DHCP6_Parse_Server_Options(buffer, buf_len - DHCP6_MSG_LEN, 
                                   parse_opts, DHCP6_RECONFIGURE) != NU_SUCCESS)
    {
        status = -1;

        NLOG_Error_Log("DHCPv6 Reconfigure message contains invalid options", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);
    }

    /* RFC 3315 - section 15.11 - Discard the message if it does not include 
     * a Server Identifier option.
     */
    else if (parse_opts[DHCP6_OPT_SERVERID].dhcp6_opt_addr != 0xffffffff)
    {
        /* RFC 3315 - section 15.11 - The Reconfigure message MUST include 
         * a Client Identifier option.
         */
        if (parse_opts[DHCP6_OPT_CLIENTID].dhcp6_opt_addr != 0xffffffff)
        {
            /* RFC 3315 - section 15.11 - Discard the message if the 
             * contents of the Client Identifier option do not match the 
             * client's DUID.
             */
            if (DHCP6_Parse_Client_ID_Option(&buffer[parse_opts
                                             [DHCP6_OPT_CLIENTID].dhcp6_opt_addr], 
                                             ds_ptr, NU_NULL) == NU_SUCCESS)
            {
                /* Indicate that this option has already been parsed. */
                parse_opts[DHCP6_OPT_CLIENTID].dhcp6_opt_addr = 0xffffffff;

                /* RFC 3315 - section 15.11 - Discard the message if it
                 * does not contain a Reconfigure Message option.
                 */
                if (parse_opts[DHCP6_OPT_RECONF_MSG].dhcp6_opt_addr != 0xffffffff)
                {
                    /* RFC 3315 - section 15.11 - Discard the message if
                     * the Reconfigure Message option is invalid.
                     */ 
                    if (DHCP6_Parse_Reconfigure_Msg_Option(&buffer[parse_opts
                        [DHCP6_OPT_RECONF_MSG].dhcp6_opt_addr], ds_ptr, 
                        NU_NULL) == NU_SUCCESS)
                    {
                        /* Indicate that this option has already been parsed. */
                        parse_opts[DHCP6_OPT_RECONF_MSG].dhcp6_opt_addr = 0xffffffff;

                        /* If the Reconfigure Message option type is
                         * Information Request.
                         */
                        if (ds_ptr->dhcp6_msg_type == DHCP6_INFO_REQUEST)
                        {
                            /* RFC 3315 - section 15.11 - Discard the message
                             * if it includes any IA options and the 
                             * Reconfigure Message option is Information
                             * Request.
                             */
                            if (parse_opts[DHCP6_OPT_IAADDR].dhcp6_opt_addr != 0xffffffff)
                            {
                                status = -1;

                                NLOG_Error_Log("DHCPv6 Reconfigure message must not contain IA option", 
                                               NERR_INFORMATIONAL, __FILE__, __LINE__);
                            }
                        }

                        if (status == NU_SUCCESS)
                        {
                            /* RFC 3315 - section 15.11 - Discard the message
                             * if it does not include DHCP Authentication.
                             */
                            if (parse_opts[DHCP6_OPT_AUTH].dhcp6_opt_addr != 0xffffffff)
                            {
                                /* Indicate that this option has already been parsed. */
                                parse_opts[DHCP6_OPT_AUTH].dhcp6_opt_addr = 0xffffffff;
                            }

                            /* Log an error and return an error code. */
                            else
                            {
                                status = -1;

                                NLOG_Error_Log("DHCPv6 Reconfigure message must contain authentication option", 
                                               NERR_INFORMATIONAL, __FILE__, __LINE__);
                            }
                        }
                    }

                    /* Log an error and return an error code. */
                    else
                    {
                        status = -1;

                        NLOG_Error_Log("DHCPv6 Reconfigure option invalid", 
                                        NERR_INFORMATIONAL, __FILE__, __LINE__);
                    }
                }

                /* Log an error and return an error code. */
                else
                {
                    status = -1;

                    NLOG_Error_Log("DHCPv6 Reconfigure message must contain Reconfigure option", 
                                   NERR_INFORMATIONAL, __FILE__, __LINE__);
                }
            }
    
            /* Log an error and return an error code. */
            else
            {
                status = -1;

                NLOG_Error_Log("DHCPv6 Client ID option does not match client", 
                               NERR_INFORMATIONAL, __FILE__, __LINE__);
            }
        }

        /* Log an error and return an error code. */
        else
        {
            status = -1;

            NLOG_Error_Log("DHCPv6 Reconfigure message must contain Client ID option", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);
        }
    }

    /* Log an error and return an error code. */
    else
    {
        status = -1;

        NLOG_Error_Log("DHCPv6 Reconfigure message must contain Server ID option", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);
    }

    /* If this is a valid Reconfigure message. */
    if (status == NU_SUCCESS)
    {
        /* Set the flag indicating that a Reconfigure is in progress. */
        ds_ptr->dhcp6_flags |= DHCP6_RECON_IN_PROGRESS;

        /* RFC 3315 - section 19.1.1 - The server MUST include a Reconfigure 
         * Message option to select whether the client responds with a Renew
         * message or an Information-Request message.
         */
        if (ds_ptr->dhcp6_msg_type == DHCP6_RENEW)
        {
            DHCP6_Renew_Addresses(ds_ptr->dhcp6_dev_index);
        }

        /* Otherwise, initiate an Information-Request / Reply exchange with
         * the server.
         */
        else    
        {
            DHCP6_Obtain_Config_Info(ds_ptr);
        }
    }

    return (status);

} /* DHCP6_Reconfigure_Input */

/*************************************************************************
*
*   FUNCTION
*
*       DHCP6_Find_TX_Struct_By_XID
*
*   DESCRIPTION
*
*       This routine finds a transmission structure based on the transaction 
*       ID of an incoming packet.
*
*   INPUTS
*
*       *target_id          A pointer to the transaction ID of the structure
*                           to find.
*
*   OUTPUTS
*
*       A pointer to the DHCP6_TX_STRUCT or NU_NULL if no matching structure
*       was found.
*
*************************************************************************/
DHCP6_TX_STRUCT *DHCP6_Find_TX_Struct_By_XID(UINT8 *target_id)
{
    DHCP6_TX_STRUCT    *tx_ptr = NU_NULL;
    INT                 i;

    /* Loop through the list of structures until a match is found. */
    for (i = 0; i < DHCP6_SIM_TX_COUNT; i++)
    {
        /* If this is the target entry, return it. */
        if (memcmp(DHCP6_Tx_List[i].dhcp6_xid, target_id, 3) == 0)
        {
            tx_ptr = &DHCP6_Tx_List[i];

            break;
        }
    }

    return (tx_ptr);

} /* DHCP6_Find_Struct_By_XID */

#endif
