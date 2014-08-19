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
*       dhcp6_parse_opts.c                                       
*
*   DESCRIPTION
*
*       This file contains functions necessary for parsing incoming DHCPv6 
*       options.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       DHCP6_Parse_User_Options
*       DHCP6_Parse_Server_Options
*       DHCP6_Parse_Client_ID_Option
*       DHCP6_Parse_Server_ID_Option
*       DHCP6_Parse_Reconfigure_Msg_Option
*       DHCP6_Parse_Reconfigure_Accept_Option
*       DHCP6_Parse_Status_Code_Option
*       DHCP6_Parse_Preference_Option
*       DHCP6_Parse_DNS_Server_Option
*       DHCP6_Parse_Option_Request_Option
*       DHCP6_Parse_Server_Unicast_Option
*       DHCP6_Parse_IA_NA_Option
*       DHCP6_Parse_IA_Addr_Option
*       DHCP6_Parse_User_Class_Option
*
*   DEPENDENCIES
*
*       target.h                                                        
*       externs.h
*       externs6.h
*       dhcp6.h
*       prefix6.h
*
**************************************************************************/

#include "networking/target.h"
#include "networking/externs.h"
#include "networking/externs6.h"
#include "networking/dhcp6.h"
#include "networking/prefix6.h"

/* Array pre-loaded with status codes to determine whether a given option
 * is permitted in the respective incoming DHCPv6 server packet.  1 indicates
 * that the option is valid for the packet type.  0 indicates that the option
 * is not valid for the packet type.  The packet types are ordered in the 
 * array as follows; Advertise, Reply, Reconfigure.
 */
UINT8   DHCP6_Valid_Server_Opts[DHCP6_MAX_OPT_CODES][3] = {
        {0, 0, 0},  /* There is no option that maps to 0 */
        {1, 1, 1},  /* DHCP6_OPT_CLIENTID */
        {1, 1, 1},  /* DHCP6_OPT_SERVERID */     
        {1, 1, 0},  /* DHCP6_OPT_IA_NA */  
        {1, 1, 0},  /* DHCP6_OPT_IA_TA */       
        {1, 1, 0},  /* DHCP6_OPT_IAADDR */      
        {0, 0, 1},  /* DHCP6_OPT_ORO */         
        {1, 1, 0},  /* DHCP6_OPT_PREFERENCE */          
        {0, 0, 0},  /* DHCP6_OPT_ELAPSED_TIME */
        {0, 0, 0},  /* DHCP6_OPT_RELAY_MSG */   
        {0, 0, 0},  /* There is no option that maps to 10 */
        {1, 1, 1},  /* DHCP6_OPT_AUTH */                
        {0, 1, 0},  /* DHCP6_OPT_UNICAST */             
        {0, 1, 0},  /* DHCP6_OPT_STATUS_CODE */ 
        {0, 1, 0},  /* DHCP6_OPT_RAPID_COMMIT */
        {1, 1, 0},  /* DHCP6_OPT_USER_CLASS */  
        {1, 1, 0},  /* DHCP6_OPT_VENDOR_CLASS */
        {1, 1, 0},  /* DHCP6_OPT_VENDOR_OPTS */ 
        {0, 0, 0},  /* DHCP6_OPT_INTERFACE_ID */
        {0, 0, 1},  /* DHCP6_OPT_RECONF_MSG */    
        {1, 1, 0},  /* DHCP6_OPT_RECONF_ACCEPT */
        {1, 1, 1},  /* DHCP6_OPT_SIP_SERVER_D */  
        {1, 1, 1},  /* DHCP6_OPT_SIP_SERVER_A */
        {1, 1, 0},  /* DHCP6_OPT_DNS_SERVERS */ 
        {1, 1, 0}   /* DHCP6_OPT_DOMAIN_LIST */ 
    }; 

#if (INCLUDE_DHCP6 == NU_TRUE)

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Parse_User_Options
*
*   DESCRIPTION
*
*       This routine determines what options are present in a DHCPv6 
*       user-specified buffer.
*
*   INPUTS
*
*       *buffer                 Pointer to the buffer in which the 
*                               message is contained.
*       buf_len                 The length of all options contained in
*                               the buffer.
*       *parse_opts             Pointer to the array to populate with
*                               the addresses of the respective options.
*
*   OUTPUTS
*
*       NU_SUCCESS              The options were successfully parsed.
*       -1                      An option was included in an invalid
*                               packet type.
*
**************************************************************************/
VOID DHCP6_Parse_User_Options(UINT8 *buffer, UINT16 buf_len, 
                              DHCP6_CLIENT_OPTS *parse_opts)
{
    UINT16  opt_code, opt_len;
    UINT32  i;

    /* Initialize all options to 0xffffffff. */
    for (i = 0; i < DHCP6_MAX_OPT_CODES; i++)
    {
        parse_opts[i].dhcp6_opt_addr = 0xffffffff;
        parse_opts[i].dhcp6_opt_count = 0;
    }

    /* Parse through each option in the packet, storing off the memory
     * location of each recognized option.
     */
    for (i = 0; i < buf_len; i += (opt_len + DHCP6_MSG_LEN))
    {
        /* Get the option code. */
        opt_code = GET16(&buffer[i], DHCP6_OPT_CODE_OFFSET);

        /* Get the length of the option. */
        opt_len = GET16(&buffer[i], DHCP6_OPT_LEN_OFFSET);

        /* Store the option code if it is recognized by the client. */
        if (opt_code <= DHCP6_MAX_OPT_CODES)
        {
            /* If this is the first instance of this option, save the
             * address location.
             */
            if (parse_opts[opt_code].dhcp6_opt_count == 0)
            {
                parse_opts[opt_code].dhcp6_opt_addr = i;
            }

            /* Increment the count of this option. */
            parse_opts[opt_code].dhcp6_opt_count ++;
        }

        /* Unrecognized options should be ignored. */
        else
        {
            NLOG_Error_Log("Unrecognized option encountered while parsing", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);
        }
    }

} /* DHCP6_Parse_User_Options */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Parse_Server_Options
*
*   DESCRIPTION
*
*       This routine determines what options are present in a DHCPv6 
*       server buffer of options and determines whether the option is
*       valid for the packet type.
*
*   INPUTS
*
*       *buffer                 Pointer to the buffer in which the 
*                               message is contained.
*       buf_len                 The length of all options contained in
*                               the buffer.
*       *parse_opts             Pointer to the array to populate with
*                               the addresses of the respective options.
*       msg_type                The type of message these options are 
*                               being extracted from.
*
*   OUTPUTS
*
*       NU_SUCCESS              The options were successfully parsed.
*       -1                      An option was included in an invalid
*                               packet type.
*
**************************************************************************/
STATUS DHCP6_Parse_Server_Options(UINT8 *buffer, UINT16 buf_len, 
                                  DHCP6_OPT_STRUCT *parse_opts, UINT8 msg_type)
{
    UINT16  opt_code, opt_len;
    UINT32  i, j;
    STATUS  status = NU_SUCCESS;

    /* Determine the value of j to use to index into the 2-dimensional
     * array that has been loaded with status values for each option/
     * packet type combination.
     */
    if (msg_type == DHCP6_ADVERTISE)
        j = 0;
    else if (msg_type == DHCP6_REPLY)
        j = 1;
    else
        j = 2;

    /* Parse through each option in the packet, storing off the memory
     * offset of each recognized option and checking that the option
     * is valid for the packet type.
     */
    for (i = 0; i < buf_len; i += (opt_len + DHCP6_MSG_LEN))
    {
        /* Get the option code. */
        opt_code = GET16(&buffer[i], DHCP6_OPT_CODE_OFFSET);

        /* Get the length of the option. */
        opt_len = GET16(&buffer[i], DHCP6_OPT_LEN_OFFSET);

        /* Store the option code if it is recognized by the client and
         * valid for this packet type. 
         */
        if (opt_code < DHCP6_MAX_OPT_CODES)
        {
            /* Check that the option is legal for this packet. */
            if (DHCP6_Valid_Server_Opts[opt_code][j] == 1)
            {
                /* RFC 3315 - section 21.4.2 - Any DHCP message that includes 
                 * more than one authentication option MUST be discarded.
                 */
                if ( (opt_code == DHCP6_OPT_AUTH) && 
                     (parse_opts[opt_code].dhcp6_opt_addr != 0xffffffff) )
                {
                    status = -1;

                    NLOG_Error_Log("DHCPv6 server packet includes multiple AUTH options", 
                                   NERR_INFORMATIONAL, __FILE__, __LINE__);

                    break;
                }

                /* Save a pointer to the first reference to this option. */
                if (parse_opts[opt_code].dhcp6_opt_count == 0)
                {
                    /* Add DHCP6_MSG_LEN bytes to account for the message
                     * header.
                     */
                    parse_opts[opt_code].dhcp6_opt_addr = i;
                }

                /* Increment the count of this option. */
                parse_opts[opt_code].dhcp6_opt_count ++;
            }

            /* If the option is not legal, return an error. */
            else
            {   
                status = -1;

                NLOG_Error_Log("Illegal option in DHCPv6 server packet", 
                               NERR_INFORMATIONAL, __FILE__, __LINE__);

                break;
            }
        }

        /* Unrecognized options should be ignored. */
        else
        {
            NLOG_Error_Log("Unrecognized option encountered will parsing", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);
        }
    }

    return (status);

} /* DHCP6_Parse_Server_Options */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Parse_Client_ID_Option
*
*   DESCRIPTION
*
*       This routine parses an incoming Client ID option and ensures it
*       is destined for the receiving node.
*
*       0                   1                   2                   3
*       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*      |        OPTION_CLIENTID        |          option-len           |
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*      .                                                               .
*      .                              DUID                             .
*      .                        (variable length)                      .
*      .                                                               .
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*
*   INPUTS
*
*       *buffer                 Pointer to the buffer in which the 
*                               option is contained.
*       *ds_ptr                 Pointer to DHCP Structure that contains 
*                               data pertinent to the option.
*       *tx_ptr                 Unused.
*
*   OUTPUTS
*
*       NU_SUCCESS              The option was successfully validated.
*       -1                      The option contains an error.
*
**************************************************************************/
STATUS DHCP6_Parse_Client_ID_Option(UINT8 *buffer, DHCP6_STRUCT *ds_ptr,
                                    DHCP6_TX_STRUCT *tx_ptr)
{
    UINT16  opt_len, opt_code;
    STATUS  status = NU_SUCCESS;
    UINT16  duid_len = 0;

    UNUSED_PARAMETER(tx_ptr);
    UNUSED_PARAMETER(ds_ptr);

    /* Get the length of the DUID. */
    opt_len = GET16(buffer, DHCP6_OPT_LEN_OFFSET);

    /* Calculate the length of the client's DUID. */
    if (DHCP_Duid.duid_type == DHCP_DUID_EN)
    {
        /* Compute the length of the DUID. */
        duid_len = 6 + DHCP_Duid.duid_id_no_len;
    }

    else if (DHCP_Duid.duid_type == DHCP_DUID_LL)
    {
        /* Compute the length of the DUID. */
        duid_len = 4 + DADDLEN;
    }

    /* Ensure the length matches the client's DUID length. */
    if (opt_len == duid_len)
    {
        /* Extract the code from the packet. */
        opt_code = GET16(buffer, DHCP6_DUID_CODE_OFFSET);

        /* If this DUID is a DUID Assigned by Vendor Based on Enterprise 
         * Number. 
         */
        if ( (DHCP_Duid.duid_type == DHCP_DUID_EN) && 
             (opt_code == DHCP_DUID_EN) )
        {
            /* Ensure the Enterprise Number matches the client's enterprise 
             * number.
             */
            if ((GET32(buffer, DHCP6_DUID_EN_ENT_NO_OFFSET)) == 
                DHCP_Duid.duid_ent_no)
            {
                /* Ensure the Identifier in the packet matches the client's
                 * identifier.
                 */
                if (memcmp(&buffer[DHCP6_DUID_EN_ID_OFFSET], DHCP_Duid.duid_id, 
                           DHCP_Duid.duid_id_no_len) != 0)
                {
                    status = -1;

                    NLOG_Error_Log("Identifier invalid in DUID for incoming DHCPv6 packet", 
                                   NERR_INFORMATIONAL, __FILE__, __LINE__);
                }
            }

            /* Log an error and return an error code. */
            else
            {
                status = -1;

                NLOG_Error_Log("Enterprise number in DUID invalid for incoming DHCPv6 packet", 
                               NERR_INFORMATIONAL, __FILE__, __LINE__);
            }
        }

        /* Otherwise, this DUID is a DUID Based on Link-layer Address. */
        else if ( (DHCP_Duid.duid_type == DHCP_DUID_LL) &&
                  (opt_code == DHCP_DUID_LL) )
        {
            /* Ensure the hardware type matches the client's hardware type. */
            if ((GET16(buffer, DHCP6_DUID_LL_HW_TYPE_OFFSET)) ==
                DHCP_Duid.duid_hw_type)
            {
                /* Ensure the Link-Layer address matches the client's 
                 * Link-Layer address.
                 */
                if (memcmp(&buffer[DHCP6_DUID_LL_ADDR_OFFSET],
                           DHCP_Duid.duid_ll_addr, DADDLEN) != 0)
                {
                    status = -1;

                    NLOG_Error_Log("Link-Layer address invalid in DUID for incoming DHCPv6 packet", 
                                   NERR_INFORMATIONAL, __FILE__, __LINE__);
                }
            }

            /* Log an error and return an error code. */
            else
            {
                status = -1;

                NLOG_Error_Log("Hardware type in DUID invalid for incoming DHCPv6 packet", 
                               NERR_INFORMATIONAL, __FILE__, __LINE__);
            }
        }

        /* The DUID type is one that is not support by the client.  Log an 
         * error and return an error code.
         */
        else
        {
            status = -1;

            NLOG_Error_Log("Invalid DUID type for incoming DHCPv6 packet", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);
        }
    }
    
    /* The DUID length is invalid.  Log an error and return an error code. */
    else
    {
        status = -1;

        NLOG_Error_Log("Invalid DUID length for incoming DHCPv6 packet", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);
    }

    return (status);

} /* DHCP6_Parse_Client_ID_Option */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Parse_Server_ID_Option
*
*   DESCRIPTION
*
*       This routine parses an incoming Server ID option and stores the
*       contents in the DHCPv6 structure for the client.
*
*       0                   1                   2                   3
*       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*      |        OPTION_SERVERID        |          option-len           |
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*      .                                                               .
*      .                              DUID                             .
*      .                        (variable length)                      .
*      .                                                               .
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*
*   INPUTS
*
*       *buffer                 Pointer to the buffer in which the 
*                               option is contained.
*       *ds_ptr                 Pointer to DHCP Structure into which to
*                               store the data from the option.
*       *tx_ptr                 Unused.
*
*   OUTPUTS
*
*       NU_SUCCESS              The option was successfully parsed.
*       -1                      The option contains an error.
*
**************************************************************************/
STATUS DHCP6_Parse_Server_ID_Option(UINT8 *buffer, DHCP6_STRUCT *ds_ptr,
                                    DHCP6_TX_STRUCT *tx_ptr)
{
    STATUS  status = NU_SUCCESS;

    UNUSED_PARAMETER(tx_ptr);

    /* Store the length of the DUID. */
    ds_ptr->dhcp6_server.dhcp6_duid.duid_len = 
        GET16(buffer, DHCP6_OPT_LEN_OFFSET);

    /* Store the type of DUID. */
    ds_ptr->dhcp6_server.dhcp6_duid.duid_type = 
        GET16(buffer, DHCP6_DUID_CODE_OFFSET);

    /* If this is a DUID Based on Link-layer Address Plus Time. */
    if (ds_ptr->dhcp6_server.dhcp6_duid.duid_type == DHCP_DUID_LLT)
    {
        /* Extract the hardware type. */
        ds_ptr->dhcp6_server.dhcp6_duid.duid_hw_type = 
            GET16(buffer, DHCP6_DUID_LLT_HW_TYPE_OFFSET); 

        /* Extract the time. */
        ds_ptr->dhcp6_server.dhcp6_duid.duid_time = 
            GET32(buffer, DHCP6_DUID_LLT_TIME_OFFSET); 

        /* Extract the Link-Layer address. */
        NU_BLOCK_COPY(ds_ptr->dhcp6_server.dhcp6_duid.duid_ll_addr,
                      &buffer[DHCP6_DUID_LLT_ADDR_OFFSET], DADDLEN);
    }

    /* If this is a DUID Assigned by Vendor Based on Enterprise Number. */
    else if (ds_ptr->dhcp6_server.dhcp6_duid.duid_type == DHCP_DUID_EN)
    {
        /* Extract the Enterprise Number. */
        ds_ptr->dhcp6_server.dhcp6_duid.duid_ent_no = 
            GET32(buffer, DHCP6_DUID_EN_ENT_NO_OFFSET); 

        /* Extract the Indentifier. */
        NU_BLOCK_COPY(ds_ptr->dhcp6_server.dhcp6_duid.duid_id,
                      &buffer[DHCP6_DUID_EN_ID_OFFSET], DADDLEN);
    }

    /* If this is a DUID Based on Link-layer Address. */
    else if (ds_ptr->dhcp6_server.dhcp6_duid.duid_type == DHCP_DUID_LL)
    {
        /* Extract the hardware type. */
        ds_ptr->dhcp6_server.dhcp6_duid.duid_hw_type = 
            GET16(buffer, DHCP6_DUID_LL_HW_TYPE_OFFSET); 

        /* Extract the Indentifier. */
        NU_BLOCK_COPY(ds_ptr->dhcp6_server.dhcp6_duid.duid_ll_addr,
                      &buffer[DHCP6_DUID_LL_ADDR_OFFSET], DADDLEN);
    }

    else
    {
        status = -1;

        NLOG_Error_Log("Invalid DUID type for incoming DHCPv6 packet", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);
    }
  
    return (status);

} /* DHCP6_Parse_Server_ID_Option */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Parse_Reconfigure_Msg_Option
*
*   DESCRIPTION
*
*       This routine parses an incoming Reconfigure Message option.
*
*     0                   1                   2                   3
*     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*    |      OPTION_RECONF_MSG        |         option-len            |
*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*    |    msg-type   |
*    +-+-+-+-+-+-+-+-+
*
*   INPUTS
*
*       *buffer                 Pointer to the buffer that holds the option
*                               being parsed.
*       *ds_ptr                 Pointer to the DHCPv6 structure associated
*                               with the packet.
*       *tx_ptr                 Unused.
*
*   OUTPUTS
*
*       NU_SUCCESS              The option was parsed successfully.
*       -1                      The option contains an invalid message
*                               type.
*
**************************************************************************/
STATUS DHCP6_Parse_Reconfigure_Msg_Option(UINT8 *buffer, DHCP6_STRUCT *ds_ptr,
                                          DHCP6_TX_STRUCT *tx_ptr)
{
    STATUS  status = NU_SUCCESS;

    UNUSED_PARAMETER(tx_ptr);

    /* Ensure the length is valid. */
    if (GET16(buffer, DHCP6_OPT_LEN_OFFSET) == DHCP6_RECON_LEN_VALUE)
    {
        /* Extract the message type and return it to the caller. */
        ds_ptr->dhcp6_msg_type = GET8(buffer, DHCP6_RECON_TYPE_OFFSET);
    }

    else
    {
        status = -1;

        NLOG_Error_Log("DHCPv6 Reconfigure message option length invalid", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);
    }

    return (status);

} /* DHCP6_Parse_Reconfigure_Msg_Option */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Parse_Reconfigure_Accept_Option
*
*   DESCRIPTION
*
*       This routine parses an incoming Reconfigure Accept option.
*
*     0                   1                   2                   3
*     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*    |     OPTION_RECONF_ACCEPT      |               0               |
*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*
*   INPUTS
*
*       *buffer                 Pointer to the buffer that holds the option
*                               being parsed.
*       *ds_ptr                 Pointer to the DHCPv6 structure associated
*                               with the packet.
*       *tx_ptr                 Unused.
*
*   OUTPUTS
*
*       NU_SUCCESS              The option was parsed successfully.
*       -1                      The option contains an invalid length or
*                               the client does not want to accept 
*                               reconfigure messages.
*
**************************************************************************/
STATUS DHCP6_Parse_Reconfigure_Accept_Option(UINT8 *buffer, 
                                             DHCP6_STRUCT *ds_ptr,
                                             DHCP6_TX_STRUCT *tx_ptr)
{
    STATUS  status = NU_SUCCESS;

    UNUSED_PARAMETER(tx_ptr);

    /* Ensure the length is valid. */
    if (GET16(buffer, DHCP6_OPT_LEN_OFFSET) == DHCP6_RECON_ACC_LEN_VALUE)
    {
        /* If the client does not want to accept Reconfigure messages,
         * reject this server.
         */
        if (ds_ptr->dhcp6_client_opts[DHCP6_OPT_RECONF_ACCEPT].dhcp6_opt_addr == 
            0xffffffff)
        {
            status = -1;

            NLOG_Error_Log("DHCPv6 server requires Reconfigure Accept", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);
        }
    }

    else
    {
        status = -1;

        NLOG_Error_Log("DHCPv6 Reconfigure accept option length invalid", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);
    }

    return (status);

} /* DHCP6_Parse_Reconfigure_Accept_Option */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Parse_Status_Code_Option
*
*   DESCRIPTION
*
*       This routine parses an incoming Status Code option.
*
*     0                   1                   2                   3
*     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*    |       OPTION_STATUS_CODE      |         option-len            |
*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*    |          status-code          |                               |
*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               |
*    .                                                               .
*    .                        status-message                         .
*    .                                                               .
*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*
*   INPUTS
*
*       *buffer                 Pointer to the buffer that contains the
*                               option.
*       *ds_ptr                 Pointer to the DHCPv6 structure associated
*                               with the packet.
*       *tx_ptr                 Pointer to the transmission structure
*                               associated with this transaction.
*
*   OUTPUTS
*
*       NU_SUCCESS              The status code option contains a success
*                               code.
*       DHCP6_RETRANS_MSG       Retransmit the message that caused this
*                               reply.
*       DHCP6_FIND_SERVER       Invoke server discovery and cease 
*                               transmission of the message that caused this
*                               reply.
*       DHCP6_SEND_REQUEST      Transmit a Request message to this server.
*
**************************************************************************/
STATUS DHCP6_Parse_Status_Code_Option(UINT8 *buffer, DHCP6_STRUCT *ds_ptr,
                                      DHCP6_TX_STRUCT *tx_ptr)
{
    STATUS  status = NU_SUCCESS;

    /* Extract the status code from the packet. */
    ds_ptr->dhcp6_status_code = GET16(buffer, DHCP6_STATUS_CODE_OFFSET);

    /* RFC 3315 - section 18.1.8 - If the client receives a Reply message 
     * with a Status Code containing UnspecFail, if the client retransmits 
     * the original message to the same server, the client MUST limit the 
     * rate at which it retransmits the message.
     */
    if (ds_ptr->dhcp6_status_code == DHCP6_STATUS_UNSPEC_FAIL)
    {
        /* This routine will return an error, and the message being 
         * transmitted will get retransmitted by the event handler since
         * there is a timer running to retransmit the message. 
         */
        status = ds_ptr->dhcp6_status = DHCP6_RETRANS_MSG;
    }

    /* RFC 3315 - section 18.1.8 - When the client receives a Reply message 
     * with a Status Code option with the value UseMulticast, the client 
     * records the receipt of the message and sends subsequent messages to 
     * the server through the interface on which the message was received 
     * using multicast.  The client resends the original message using 
     * multicast.
     */
    else if (ds_ptr->dhcp6_status_code == DHCP6_STATUS_USE_MULTICAST)
    {
        /* Clear the flag indicating to unicast packets to the server. */
        ds_ptr->dhcp6_flags &= ~DHCP6_SERVER_UNICAST;
        
        /* This routine will return an error, and the message being 
         * transmitted will get retransmitted by the event handler since
         * there is a timer running to retransmit the message. 
         */
        status = ds_ptr->dhcp6_status = DHCP6_RETRANS_MSG;
    }

    /* RFC 3315 - section 18.1.8 - When the client receives a NotOnLink 
     * status from the server in response to a Confirm message, the client 
     * performs DHCP server solicitation and client-initiated configuration.  
     * If the client receives any Reply messages that do not indicate a 
     * NotOnLink status, the client can use the addresses in the IA and 
     * ignore any messages that indicate a NotOnLink status.  When the 
     * client receives a NotOnLink status from the server in response to a 
     * Request, the client can ... restart the DHCP server discovery process.
     */
    else if ( (ds_ptr->dhcp6_status_code == DHCP6_STATUS_NOT_ONLINK) &&
              ((tx_ptr->dhcp6_type == DHCP6_CONFIRM) ||
               (tx_ptr->dhcp6_type == DHCP6_REQUEST)) )
    {
        /* Return an error. */
        status = ds_ptr->dhcp6_status = DHCP6_FIND_SERVER;
    }

    /* RFC 3315 - section 18.1.3 - If the status code is NoAddrsAvail, the 
     * client has received no usable addresses in the IA and may choose to 
     * try obtaining addresses for the IA from another server.  The client 
     * uses addresses and other information from any IAs that do not contain 
     * a Status Code option with the NoAddrsAvail code.
     */
    else if (ds_ptr->dhcp6_status_code == DHCP6_STATUS_NO_ADDRS_AVAIL)
    {
        /* Return an error.  The caller will continue to process all IA's
         * to see if there is one with a valid status code.
         */
        status = ds_ptr->dhcp6_status = DHCP6_FIND_SERVER;
    }

    /* RFC 3315 - section 18.1.3 - Send a Request message if the IA contains 
     * a Status Code option with the NoBinding status (and do not send any 
     * additional Renew/Rebind messages).
     */
    else if (ds_ptr->dhcp6_status_code == DHCP6_STATUS_NO_BINDING)
    {
        /* If the reply is in response to a renew or rebind message. */
        if ( (tx_ptr->dhcp6_type == DHCP6_RENEW) ||
             (tx_ptr->dhcp6_type == DHCP6_REBIND) )
        {
            /* Send a request message to the server. */
            status = ds_ptr->dhcp6_status = DHCP6_SEND_REQUEST;
        }

        /* Otherwise, retransmit the message. */
        else
        {
            status = ds_ptr->dhcp6_status = DHCP6_RETRANS_MSG;
        }
    }

    return (status);

} /* DHCP6_Parse_Status_Code_Option */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Parse_Preference_Option
*
*   DESCRIPTION
*
*       This routine parses an incoming Preference option.
*
*       0                   1                   2                   3
*       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*      |       OPTION_PREFERENCE       |          option-len           |
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*      |  pref-value   |
*      +-+-+-+-+-+-+-+-+
*
*   INPUTS
*
*       *buffer                 Pointer to the buffer that contains the
*                               option.
*       *ds_ptr                 Pointer to the DHCPv6 structure associated
*                               with the packet.
*       *tx_ptr                 Unused.
*
*   OUTPUTS
*
*       NU_SUCCESS              The option was successfully parsed.
*       -1                      The length is invalid or a higher preference
*                               server has already been found.
*
**************************************************************************/
STATUS DHCP6_Parse_Preference_Option(UINT8 *buffer, DHCP6_STRUCT *ds_ptr,
                                     DHCP6_TX_STRUCT *tx_ptr)
{
    STATUS  status = NU_SUCCESS;
    UINT8   pref_value;

    UNUSED_PARAMETER(tx_ptr);

    /* Ensure the length is correct. */
    if (GET16(buffer, DHCP6_OPT_LEN_OFFSET) == DHCP6_PREF_LEN_VALUE)
    {
        /* Extract the preference value from the packet. */
        pref_value = GET8(buffer, DHCP6_PREF_VALUE_OFFSET);

        /* If a higher preference server has not already responded, store 
         * this server's information.
         */
        if ( (!ds_ptr->dhcp6_server.dhcp6_resp_time) ||
             (ds_ptr->dhcp6_server.dhcp6_pref < pref_value) )
        {
            /* Store the preference value. */
            ds_ptr->dhcp6_server.dhcp6_pref = pref_value;
        }
        
        /* Otherwise, do not store this server's preference value. */
        else
        {
            status = -1;

            NLOG_Error_Log("Higher preference DHCPv6 server already responded", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);
        }
    }

    else
    {
        status = -1;

        NLOG_Error_Log("DHCPv6 Preference option length invalid", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);
    }

    return (status);

} /* DHCP6_Parse_Preference_Option */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Parse_DNS_Server_Option
*
*   DESCRIPTION
*
*       This routine parses an incoming DNS Recursive Name Server option
*       and adds the respective server(s) to the DNS list of servers to use
*       for DNS resolution.
*
*      0                   1                   2                   3
*      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
*     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*     |      OPTION_DNS_SERVERS       |         option-len            |
*     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*     |                                                               |
*     |            DNS-recursive-name-server (IPv6 address)           |
*     |                                                               |
*     |                                                               |
*     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*     |                                                               |
*     |            DNS-recursive-name-server (IPv6 address)           |
*     |                                                               |
*     |                                                               |
*     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*     |                              ...                              |
*     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*
*   INPUTS
*
*       *buffer                 Pointer to the buffer that contains the
*                               option.
*       *ds_ptr                 Unused.
*       *tx_ptr                 Unused.
*
*   OUTPUTS
*
*       NU_SUCCESS
*
**************************************************************************/
STATUS DHCP6_Parse_DNS_Server_Option(UINT8 *buffer, DHCP6_STRUCT *ds_ptr,
                                     DHCP6_TX_STRUCT *tx_ptr)
{
    UINT16  opt_len = DHCP6_DNS_SERVER_LEN;
    INT     i;

    UNUSED_PARAMETER(ds_ptr);
    UNUSED_PARAMETER(tx_ptr);
    
    /* Add the length of the IPv6 addresses contained in the option. */
    opt_len += GET16(buffer, DHCP6_OPT_LEN_OFFSET);

    /* Extract each address from the packet and add it to the list of
     * servers on the node.
     */
    for (i = DHCP6_DNS_SERVER_LEN; i < opt_len; i += IP6_ADDR_LEN)
    {
        /* Add the server to the list of DNS servers for this node. */
        if (NU_Add_DNS_Server2(&buffer[i], DNS_ADD_TO_FRONT, 
                               NU_FAMILY_IP6) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to add DHCPv6 server to list", 
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    return (NU_SUCCESS);

} /* DHCP6_Parse_DNS_Server_Option */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Parse_Option_Request_Option
*
*   DESCRIPTION
*
*       This routine parses an Option Request option sent from a DHCPv6
*       server in a reconfigure message.
*
*       0                   1                   2                   3
*       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*      |           OPTION_ORO          |           option-len          |
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*      |    requested-option-code-1    |    requested-option-code-2    |
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*      |                              ...                              |
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*
*   INPUTS
*
*       *buffer                 Pointer to the buffer in which the 
*                               option is contained.
*       *ds_ptr                 Pointer to DHCP structure associated with
*                               this packet.
*       *tx_ptr                 Unused.
*
*   OUTPUTS
*
*       NU_SUCCESS
*
**************************************************************************/
STATUS DHCP6_Parse_Option_Request_Option(UINT8 *buffer, 
                                         DHCP6_STRUCT *ds_ptr, 
                                         DHCP6_TX_STRUCT *tx_ptr)
{
    UINT16  i, opt_count;

    UNUSED_PARAMETER(tx_ptr);

    /* Clear out any system options that were specified in the last 
     * message received from the server.
     */
    memset(ds_ptr->dhcp6_system_opts, 0, DHCP6_MAX_OPT_CODES);

    /* The length is 2 times the number of requested options. */
    opt_count = (UINT16)(GET16(buffer, DHCP6_OPT_LEN_OFFSET) >> 1);

    /* Extract each option from the packet. */
    for (i = 0; (i < opt_count) && (i < DHCP6_MAX_OPT_CODES); i++)
    {
        /* Each option is 2 bytes, so increase the offset into the buffer 
         * accordingly. 
         */
        GET16(buffer, DHCP6_ORO_CODE_OFFSET + (i * 2));

        /* Indicate that the client should include this option in the 
         * next request to the server.
         */
        ds_ptr->dhcp6_system_opts[i] = 1;
    }

    return (NU_SUCCESS);

} /* DHCP6_Parse_Option_Request_Option */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Parse_Server_Unicast_Option
*
*   DESCRIPTION
*
*       This routine parses an incoming Server Unicast option.  When a 
*       client receives this option, where permissible and appropriate, 
*       the client sends messages directly to the server using the IPv6 
*       address specified in the server-address field of the option.
*
*     0                   1                   2                   3
*     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*    |          OPTION_UNICAST       |        option-len             |
*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*    |                                                               |
*    |                       server-address                          |
*    |                                                               |
*    |                                                               |
*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*
*   INPUTS
*
*       *buffer                 Pointer to the buffer that contains the
*                               option.
*       *ds_ptr                 Pointer to the DHCPv6 structure associated
*                               with the incoming packet.
*       *tx_ptr                 Unused.
*
*   OUTPUTS
*
*       NU_SUCCESS              Option successfully parsed.
*       -1                      The option contains an error.
*
**************************************************************************/
STATUS DHCP6_Parse_Server_Unicast_Option(UINT8 *buffer, DHCP6_STRUCT *ds_ptr,
                                         DHCP6_TX_STRUCT *tx_ptr)
{   
    STATUS  status = NU_SUCCESS;

    UNUSED_PARAMETER(tx_ptr);

    /* Ensure the option length is valid. */
    if (GET16(buffer, DHCP6_OPT_LEN_OFFSET) == IP6_ADDR_LEN)
    {
        /* Set up the address structure parameters. */
        ds_ptr->dhcp6_server.dhcp6_iaddr.family = NU_FAMILY_IP6;
        ds_ptr->dhcp6_server.dhcp6_iaddr.port = DHCP6_SERVER_PORT;
        ds_ptr->dhcp6_server.dhcp6_iaddr.name = "DHCP6Rtr";

        /* Copy the address into the server address structure. */
        NU_BLOCK_COPY(ds_ptr->dhcp6_server.dhcp6_iaddr.id.is_ip_addrs, 
                      &buffer[DHCP6_UNICAST_ADDR_OFFSET], IP6_ADDR_LEN);

        /* Set the flag indicating that the client can unicast Request, 
         * Renew, Release and Decline messages to the server.
         */
        ds_ptr->dhcp6_flags |= DHCP6_SERVER_UNICAST;
    }

    else
    {
        status = -1;

        NLOG_Error_Log("Server Unicast option has invalid length", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);
    }

    return (status);

} /* DHCP6_Parse_Server_Unicast_Option */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Parse_IA_NA_Option
*
*   DESCRIPTION
*
*       This routine parses an Identity Association for Non-Temporary
*       Addresses option received from a DHCPv6 server.
*
*       0                   1                   2                   3
*       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*      |          OPTION_IA_NA         |          option-len           |
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*      |                        IAID (4 octets)                        |
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*      |                              T1                               |
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*      |                              T2                               |
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*      |                                                               |
*      .                         IA_NA-options                         .
*      .                                                               .
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*
*   INPUTS
*
*       *buffer                 Pointer to the buffer in which the 
*                               option is contained.
*       *ds_ptr                 Pointer to DHCP structure that contains 
*                               data pertinent to the option.
*       *tx_ptr                 Pointer to the transmission structure
*                               associated with this transaction.
*
*   OUTPUTS
*
*       NU_SUCCESS              The option was successfully parsed.
*       -1                      The option contains an error.
*
**************************************************************************/
STATUS DHCP6_Parse_IA_NA_Option(UINT8 *buffer, DHCP6_STRUCT *ds_ptr,
                                DHCP6_TX_STRUCT *tx_ptr)
{
    UINT32  iaid;
    STATUS  status = NU_SUCCESS, ia_status = -1;
    UINT16  opt_len, type, len, nbytes = DHCP6_MSG_LEN;

    /* Get the option length from the packet. */
    opt_len = GET16(buffer, DHCP6_OPT_LEN_OFFSET);

    /* Ensure the length is valid. */
    if (opt_len >= DHCP6_IA_NA_PAYLOAD_LEN)
    {
        /* Get the IA ID associated with this interface. */
        NU_Get_DHCP_IAID(ds_ptr->dhcp6_dev_index, &iaid);

        /* Check that the IA matches the IA of this DHCPv6 structure. */
        if (GET32(buffer, DHCP6_IA_NA_IAID_OFFSET) == iaid)
        {
            /* RFC 3315 - section 22.4 - In a message sent by a server to 
             * a client, the client MUST use the values in the T1 and T2 
             * fields for the T1 and T2 parameters, unless those values 
             * in those fields are 0.  The values in the T1 and T2 fields 
             * are the number of seconds until T1 and T2.
             */
            ds_ptr->dhcp6_t1 = GET32(buffer, DHCP6_IA_NA_T1_OFFSET);
            ds_ptr->dhcp6_t2 = GET32(buffer, DHCP6_IA_NA_T2_OFFSET);

            /* RFC 3315 - section 22.4 - If a client receives an IA_NA with 
             * T1 greater than T2, and both T1 and T2 are greater than 0, 
             * the client discards the IA_NA option and processes the 
             * remainder of the message as though the server had not 
             * included the invalid IA_NA option.
             */
            if ( (ds_ptr->dhcp6_t1 > ds_ptr->dhcp6_t2) &&
                 (ds_ptr->dhcp6_t1 > 0) && (ds_ptr->dhcp6_t2 > 0) )
            {
                NLOG_Error_Log("Ignoring IA_NA option with invalid T1 and T2", 
                               NERR_INFORMATIONAL, __FILE__, __LINE__);
            }

            /* Process the IA_NA option. */
            else
            {
                /* RFC 3315 - section 22.4 - If the time at which the 
                 * addresses in an IA_NA are to be renewed is to be left to 
                 * the discretion of the client, the server sets T1 and T2 
                 * to 0.
                 */
                if (ds_ptr->dhcp6_t1 == 0)
                {
                    /* Set the value to the default. */
                    ds_ptr->dhcp6_t1 = DHCP6_T1_DEFAULT;
                }

                if (ds_ptr->dhcp6_t2 == 0)
                {
                    /* Set the value to the default. */
                    ds_ptr->dhcp6_t2 = DHCP6_T2_DEFAULT;
                }

                /* Decrement the length of the fixed payload of the 
                 * option. 
                 */
                opt_len -= DHCP6_IA_NA_PAYLOAD_LEN;

                /* Increment the number of bytes that have been 
                 * processed. 
                 */
                nbytes += DHCP6_IA_NA_PAYLOAD_LEN;

                /* If option length is not zero, there are options associated
                 * with this IA_NA option.
                 */
                if (opt_len)
                {
                    /* Process each option in the packet. */
                    do
                    {
                        /* Get the option type. */
                        type = GET16(&buffer[nbytes], DHCP6_OPT_CODE_OFFSET);

                        /* If this option is recognized. */
                        if (type < DHCP6_MAX_OPT_CODES)
                        {
                            /* Process the option. */
                            status = DHCP6_Options[type].
                                dhcp6_parse_opt(&buffer[nbytes], ds_ptr, 
                                                tx_ptr);
                        }

                        /* If this was an IA_ADDR option and it was 
                         * processed successfully, set the ia_status to 
                         * success.  Otherwise, continue to process the 
                         * options looking for a successful IA_ADDR option.
                         */
                        if (type == DHCP6_OPT_IAADDR)
                        {
                            if (status == NU_SUCCESS)
                            {
                                ia_status = NU_SUCCESS;
                            }
                        }

                        /* Otherwise, an error was encountered.  Stop
                         * processing this option.
                         */
                        else if (status != NU_SUCCESS)
                        {
                            break;
                        }

                        /* Get the option length. */
                        len = GET16(&buffer[nbytes], DHCP6_OPT_LEN_OFFSET);

                        /* Increment the index, taking into account the
                         * length of the option header.
                         */
                        nbytes += (len + DHCP6_MSG_LEN);

                        /* Decrement the remaining bytes, taking into account
                         * the length of the option header.
                         */
                        opt_len -= (len + DHCP6_MSG_LEN);

                    } while (opt_len);

                    /* If no valid IA's were found in the packet, return
                     * an error.
                     */
                    if (ia_status != NU_SUCCESS)
                    {
                        status = -1;
                    }

                    /* Set the T1 timer if the value is not infinity. */
                    else if ( (status == NU_SUCCESS) &&
                              (ds_ptr->dhcp6_t1 != 0xffffffff) && 
                              ((tx_ptr->dhcp6_type == DHCP6_REQUEST) ||
                               ((tx_ptr->dhcp6_type == DHCP6_SOLICIT) && 
                               (ds_ptr->dhcp6_flags & DHCP6_ALLOW_RAPID_COMMIT))) )
                    {
                        /* Get the semaphore. */
                        status = NU_Obtain_Semaphore(&TCP_Resource, 
                                                     NU_NO_SUSPEND);

                        if (status == NU_SUCCESS)
                        {
                            /* Set the timer. */
                            TQ_Timerset(DHCP6_Renew_IA_NA_Event, 
                                        ds_ptr->dhcp6_dev_index, 
                                        ds_ptr->dhcp6_t1 * SCK_Ticks_Per_Second, 
                                        DHCP6_RENEW);

                            /* Release the semaphore. */
                            NU_Release_Semaphore(&TCP_Resource);
                        }

                        else
                        {
                            NLOG_Error_Log("Failed to obtain TCP semaphore",  
                                           NERR_SEVERE, __FILE__, __LINE__);
                        }
                    }
                }
            }
        }

        /* Log an error and return an error code. */
        else
        {
            status = -1;

            NLOG_Error_Log("IA_NA option has invalid IA ID", 
                           NERR_INFORMATIONAL, __FILE__, __LINE__);
        }
    }

    /* Log an error and return an error code. */
    else
    {
        status = -1;

        NLOG_Error_Log("IA_NA option has invalid length", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);
    }

    return (status);

} /* DHCP6_Parse_IA_NA_Option */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Parse_IA_Addr_Option
*
*   DESCRIPTION
*
*       This routine parses an incoming IA Address option.
*
*       0                   1                   2                   3
*       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*      |          OPTION_IAADDR        |          option-len           |
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*      |                                                               |
*      |                         IPv6 address                          |
*      |                                                               |
*      |                                                               |
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*      |                      preferred-lifetime                       |
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*      |                        valid-lifetime                         |
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*      .                                                               .
*      .                        IAaddr-options                         .
*      .                                                               .
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*
*   INPUTS
*
*       *buffer                 Pointer to the buffer that contains the
*                               option.
*       *ds_ptr                 Pointer to the DHCPv6 structure associated
*                               with the incoming packet.
*       *tx_ptr                 Unused.
*
*   OUTPUTS
*
*       NU_SUCCESS              Option parsed successfully.
*       -1                      There is an error in the option.
*
**************************************************************************/
STATUS DHCP6_Parse_IA_Addr_Option(UINT8 *buffer, DHCP6_STRUCT *ds_ptr,
                                  DHCP6_TX_STRUCT *tx_ptr)
{
    STATUS              status = NU_SUCCESS;
    DV_DEVICE_ENTRY     *dv_ptr;
    UINT32              pref_life, valid_life;
    DEV6_IF_ADDRESS     *addr_ptr = NU_NULL;
    UINT16              opt_code, len, opt_len, cli_opt_len;
    UINT32              nbytes;
    INT                 i;
    IP6_PREFIX_ENTRY    *prfx_entry;

    UNUSED_PARAMETER(tx_ptr);

    /* Extract the length of the option. */
    opt_len = GET16(buffer, DHCP6_OPT_LEN_OFFSET);

    /* Ensure the length is valid.  option-len is 24 plus the length of 
     * any options. 
     */
    if (opt_len < (DHCP6_IA_ADDR_OPT_LEN - DHCP6_MSG_LEN))
    {
        status = -1;

        NLOG_Error_Log("IA Addr option has invalid length", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);
    }

    /* Process the option. */
    else
    {
        /* Decrement the length of the fixed payload of the option. */
        opt_len -= (DHCP6_IA_ADDR_OPT_LEN - DHCP6_MSG_LEN);

        /* If option length is not zero, there are options associated
         * with this IA_ADDR option.  Before processing the IA_ADDR 
         * option, check the status of the option.
         */
        if (opt_len)
        {
            /* Increment past the IA_ADDR option. */
            nbytes = DHCP6_IA_ADDR_OPT_LEN;

            /* Process each option in the packet. */
            do
            {
                /* Get the option type from the packet. */
                opt_code = GET16(&buffer[nbytes], DHCP6_OPT_CODE_OFFSET);

                /* If this option is supported. */
                if (opt_code < DHCP6_MAX_OPT_CODES)
                {
                    /* Process the option. */
                    status = DHCP6_Options[opt_code].
                        dhcp6_parse_opt(&buffer[nbytes], ds_ptr, tx_ptr);

                    /* Stop processing this option if an error is found. */
                    if (status != NU_SUCCESS)
                        break;
                }

                /* Get the option length, adding the 4 bytes of the option 
                 * header. 
                 */
                len = (UINT16)(GET16(&buffer[nbytes], DHCP6_OPT_LEN_OFFSET) + 
                    DHCP6_MSG_LEN);

                /* Increment the index. */
                nbytes += len;

                /* Decrement the remaining bytes. */
                opt_len -= len;

            } while (opt_len);
        }

        if (status == NU_SUCCESS)
        {
            /* Extract the preferred and valid lifetimes. */
            pref_life = GET32(buffer, DHCP6_IAADR_PREF_LIFE_OFFSET);
            valid_life = GET32(buffer, DHCP6_IAADR_VALID_LIFE_OFFSET);

            /* A client discards any addresses for which the preferred
             * lifetime is greater than the valid lifetime.
             */
            if (pref_life > valid_life)
            {
                status = -1;
            }

            /* If the status code associated with the option was success. */
            else
            {
                /* Get the NET semaphore. */
                status = NU_Obtain_Semaphore(&TCP_Resource, NU_NO_SUSPEND);

                if (status == NU_SUCCESS)
                {
                    /* Get a pointer to the interface. */
                    dv_ptr = DEV_Get_Dev_By_Index(ds_ptr->dhcp6_dev_index);

                    /* Determine if this address already exists on the 
                     * interface. 
                     */
                    if (dv_ptr)
                    {
                        /* Get a pointer to this address structure on the 
                         * interface. 
                         */
                        addr_ptr = 
                            DEV6_Find_Target_Address(dv_ptr, 
                                                     &buffer
                                                     [DHCP6_IAADR_ADDR_OFFSET]);

	                    /* RFC 3315 - section 18.1.3 - The server may remove addresses 
	                     * from the IA by setting the preferred and valid lifetimes of 
	                     * those addresses to zero.
	                     */
	                    if ( (pref_life == 0) && (valid_life == 0) )
	                    {
	                        /* If we are attempting to Release or Decline this address, 
	                         * free the memory for it. 
	                         */
	                        if ( (tx_ptr->dhcp6_type == DHCP6_RELEASE) ||
	                             (tx_ptr->dhcp6_type == DHCP6_DECLINE) )
	                        {
	                            /* Release the semaphore. */
	                            NU_Release_Semaphore(&TCP_Resource);
	
	                            DHCP6_Free_Address(ds_ptr);
	                        }
	
	                        /* Otherwise, remove the address from the interface. */
	                        else if (addr_ptr)
	                        {
	                            /* Clear the DHCP flag so a Release is not sent. */
	                            addr_ptr->dev6_addr_flags &= ~ADDR6_DHCP_ADDR;
	
	                            /* Delete the address. */
	                            DEV6_Delete_IP_From_Device(addr_ptr);
	
	                            /* Release the semaphore. */
	                            NU_Release_Semaphore(&TCP_Resource);
	                        }
	
	                        else
	                        {
	                            /* Release the semaphore. */
	                            NU_Release_Semaphore(&TCP_Resource);
	                        }
	
	                        /* If the user specified this address. */
	                        if (ds_ptr->dhcp6_client_opts[DHCP6_OPT_IAADDR].dhcp6_opt_addr 
	                            != 0xffffffff)
	                        {
	                            /* Find the IA in the user buffer and remove it. */
	                            for (i = 0; i < ds_ptr->dhcp6_opts_len; i += cli_opt_len)
	                            {
	                                /* If this is an IA_ADDR option. */
	                                if (GET16(&ds_ptr->dhcp6_opts[i], 
	                                          DHCP6_OPT_CODE_OFFSET) == DHCP6_OPT_IAADDR)
	                                {
	                                    /* If this address matches the target address. */
	                                    if (memcmp(&ds_ptr->dhcp6_opts[i + 
	                                               DHCP6_IAADR_ADDR_OFFSET], 
	                                               &buffer[DHCP6_IAADR_ADDR_OFFSET], 
	                                               IP6_ADDR_LEN) == 0)
	                                    {
	                                        /* Delete the IA_ADDR option. */
	                                        memmove(&ds_ptr->dhcp6_opts[i], 
	                                                &ds_ptr->dhcp6_opts[i + 
	                                                DHCP6_IA_ADDR_OPT_LEN], 
	                                                ds_ptr->dhcp6_opts_len - i - 
	                                                DHCP6_IA_ADDR_OPT_LEN);
	
	                                        /* Decrement the length of the options 
	                                         * buffer. 
	                                         */
	                                        ds_ptr->dhcp6_opts_len -= 
	                                            DHCP6_IA_ADDR_OPT_LEN;
	
	                                        break;
	                                    }
	                                }
	
	                                /* Get the length of this option. */
	                                cli_opt_len = GET16(&ds_ptr->dhcp6_opts[i], 
	                                                    DHCP6_OPT_LEN_OFFSET);
	                            }
	                        }
	                    }
	
	                    else
	                    {   
	                        /* RFC 3315 - section 18.1.8 - Add any new addresses 
	                         * in the IA option to the IA as recorded by the 
	                         * client.
	                         */
	                        if (!addr_ptr)
	                        {
	                            /* If this is a Reply message, assign the address. */
	                            if ( (tx_ptr->dhcp6_type == DHCP6_REQUEST) ||
	                                 (tx_ptr->dhcp6_type == DHCP6_RENEW) ||
	                                 ((tx_ptr->dhcp6_type == DHCP6_SOLICIT) && 
	                                 (ds_ptr->dhcp6_flags & DHCP6_ALLOW_RAPID_COMMIT)) )
	                            {
	                                /* Search for a prefix list entry added via a
	                                 * Router Advertisement to determine the prefix
	                                 * length of the new address.
	                                 */
	                                prfx_entry = PREFIX6_Find_On_Link_Prefix(dv_ptr);
	
	                                /* RFC 3315 - section 18.1.8 - The client SHOULD 
	                                 * perform duplicate address detection on each of 
	                                 * the addresses in any IAs it receives in the 
	                                 * Reply message before using that address for 
	                                 * traffic.  If any of the addresses are found to 
	                                 * be in use on the link, the client sends a 
	                                 * Decline message to the server.  This is handled 
	                                 * by DEV6_Add_IP_To_Device and the stack.
	                                 */
	                                status = 
	                                    DEV6_Add_IP_To_Device(dv_ptr, 
	                                                          &buffer
	                                                          [DHCP6_IAADR_ADDR_OFFSET], 
	                                                          prfx_entry != NU_NULL ? 
	                                                          prfx_entry->ip6_prfx_lst_prfx_length : 64, 
	                                                          pref_life, valid_life, 
	                                                          ADDR6_DHCP_ADDR);
	                            }
	
	                            /* Otherwise, fill in the offered address parameters. */
	                            else
	                            {
	                                NU_BLOCK_COPY(ds_ptr->dhcp6_offered_addr.dhcp6_ia_addr,
	                                              &buffer[DHCP6_IAADR_ADDR_OFFSET], 
	                                              IP6_ADDR_LEN);
	
	                                ds_ptr->dhcp6_offered_addr.dhcp6_ia_pref_life = 
	                                    pref_life;
	
	                                ds_ptr->dhcp6_offered_addr.dhcp6_ia_valid_life = 
	                                    valid_life;
	                            }
	                        }
	
	                        /* RFC 3315 - section 18.1.8 - Update lifetimes for any 
	                         * addresses in the IA option that the client already has 
	                         * recorded in the IA.
	                         */
	                        else
	                        {
	                            /* Update the Valid Lifetime. */
	                            DEV6_Update_Address_Entry(dv_ptr, addr_ptr, 
	                                                      valid_life, 0);
	
	                            /* Update the Preferred Lifetime. */
	                            DEV6_Update_Address_Entry(dv_ptr, addr_ptr, 0, 
	                                                      pref_life);
	                        }

	                        /* Release the semaphore. */
	                        NU_Release_Semaphore(&TCP_Resource);
						}
                    }
					
					else
					{
                        /* Release the semaphore. */
                        NU_Release_Semaphore(&TCP_Resource);						
					}
                }
            }
        }
    }

    return (status);

} /* DHCP6_Parse_IA_Addr_Option */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Parse_User_Class_Option
*
*   DESCRIPTION
*
*       This routine parses a User Class option sent by a DHCPv6 server.
*       In response to a message containing a User Class option, a server
*       includes a User Class option containing those classes that were
*       successfully interpreted by the server, so that the client can be
*       informed of the classes interpreted by the server.
*
*       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*      |       OPTION_USER_CLASS       |          option-len           |
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*      .                                                               .
*      .                          user-class-data                      .
*      .                                                               .
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*
*   INPUTS
*
*       *buffer                 Pointer to the buffer that contains the
*                               option.
*       *ds_ptr                 Pointer to the DHCPv6 structure associated
*                               with the incoming packet.
*       *tx_ptr                 Unused.
*
*   OUTPUTS
*
*       NU_SUCCESS              Option parsed successfully.
*       -1                      There is an error in the option.
*
**************************************************************************/
STATUS DHCP6_Parse_User_Class_Option(UINT8 *buffer, DHCP6_STRUCT *ds_ptr,
                                     DHCP6_TX_STRUCT *tx_ptr)
{
    UINT16  current_opt_len, total_len, len_count;
    STATUS  status;

    UNUSED_PARAMETER(tx_ptr);

    /* Extract the total length from the buffer. */
    total_len = GET16(buffer, DHCP6_OPT_LEN_OFFSET);

    /* If memory has already been allocated from a previous DHCPv6 server
     * message, deallocate it now.
     */
    if (ds_ptr->dhcp6_user_classes)
    {
        /* Deallocate the memory. */
        NU_Deallocate_Memory(ds_ptr->dhcp6_user_classes);

        /* Set the count back to zero. */
        ds_ptr->dhcp6_user_class_count = 0;
    }

    /* Allocate memory in the ds_ptr structure for the user-class entries.
     * Since opt_len includes memory for each option length field, this will
     * be enough memory for null-terminating each user-class-data field in
     * the buffer that will be returned to the user.
     */
    status = NU_Allocate_Memory(MEM_Cached, 
                                (VOID **)&ds_ptr->dhcp6_user_classes,
                                total_len, (UNSIGNED)NU_SUSPEND);  

    /* If memory was successfully allocated. */
    if (status == NU_SUCCESS)
    {
        /* Copy each option into the buffer that will be returned to the 
         * user. 
         */
        for (len_count = 0; 
             len_count < total_len; 
             len_count += current_opt_len, ds_ptr->dhcp6_user_class_count ++)
        {
            /* Extract the length of this user-class-data option. */
            current_opt_len = GET16(buffer, len_count + DHCP6_OPT_LEN_OFFSET);

            /* Extract the opaque data from the packet. */
            NU_BLOCK_COPY(&ds_ptr->dhcp6_user_classes[len_count], 
                          &buffer[len_count + DHCP6_USER_CLASS_DATA_OFFSET], 
                          current_opt_len);

            /* Null-terminate the opaque data in the buffer. */
            ds_ptr->dhcp6_user_classes[len_count + current_opt_len] = '\0';

            /* Increment the byte count to include the null-terminator. */
            len_count ++;

            /* Decrement one byte from the total_length for the 
             * user-class-data length field and the null-terminator.
             */
            total_len -= 1;
        }
    }

    return (status);

} /* DHCP6_Parse_User_Class_Option */

#endif
