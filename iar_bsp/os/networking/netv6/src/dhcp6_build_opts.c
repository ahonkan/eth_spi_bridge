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
*       dhcp6_build_opts.c                                       
*
*   DESCRIPTION
*
*       This file contains functions necessary for building DHCPv6 
*       options.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       DHCP6_Build_Client_ID_Option
*       DHCP6_Build_Server_ID_Option
*       DHCP6_Build_IA_NA_Option
*       DHCP6_Build_IA_Addr_Option
*       DHCP6_Build_Option_Request_Option
*       DHCP6_Build_Rapid_Commit_Option
*       DHCP6_Build_Elapsed_Time_Option
*       DHCP6_Build_Status_Code_Option
*       DHCP6_Build_User_Class_Option
*       DHCP6_Build_Vendor_Class_Option
*       DHCP6_Build_Vendor_Specific_Info_Option
*       DHCP6_Build_Reconfigure_Accept_Option
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

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Build_Client_ID_Option
*
*   DESCRIPTION
*
*       This routine builds a Client ID option in the provided buffer.
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
*                               option is being built.
*
*   OUTPUTS
*
*       The number of bytes added to the packet.
*
**************************************************************************/
UINT16 DHCP6_Build_Client_ID_Option(UINT8 *buffer)
{
    UINT16  duid_len;

    /* The code is CLIENT ID. */
    PUT16(buffer, DHCP6_OPT_CODE_OFFSET, DHCP6_OPT_CLIENTID);

    /* Add the proper code to the packet. */
    PUT16(buffer, DHCP6_DUID_CODE_OFFSET, DHCP_Duid.duid_type);

    /* Build the DUID Assigned by Vendor Based on Enterprise Number. 
     *
     * 0                   1                   2                   3
     * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |               2               |       enterprise-number       |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |   enterprise-number (contd)   |                               |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               |
     * .                           identifier                          .
     * .                       (variable length)                       .
     * .                                                               .
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ 
     */
    if (DHCP_Duid.duid_type == DHCP_DUID_EN)
    {
        /* Add the Enterprise Number to the packet. */
        PUT32(buffer, DHCP6_DUID_EN_ENT_NO_OFFSET, DHCP_Duid.duid_ent_no); 

        /* Add the Identifier to the packet. */
        NU_BLOCK_COPY(&buffer[DHCP6_DUID_EN_ID_OFFSET], DHCP_Duid.duid_id, 
                      DHCP_Duid.duid_id_no_len);

        /* Compute the length of the DUID. */
        duid_len = 6 + DHCP_Duid.duid_id_no_len;
    }

    /* Build the DUID Based on Link-layer Address.
     * 
     * 0                   1                   2                   3
     * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |               3               |    hardware type (16 bits)    |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * .                                                               .
     * .             link-layer address (variable length)              .
     * .                                                               .
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     */
    else
    {
        /* Add the Hardware Type to the packet. */
        PUT16(buffer, DHCP6_DUID_LL_HW_TYPE_OFFSET, DHCP_Duid.duid_hw_type); 

        /* Add the Link-Layer address to the packet. */
        NU_BLOCK_COPY(&buffer[DHCP6_DUID_LL_ADDR_OFFSET],
                      DHCP_Duid.duid_ll_addr, DADDLEN);

        /* Compute the length of the DUID. */
        duid_len = 4 + DADDLEN;
    }

    /* Insert the length into the packet. */
    PUT16(buffer, DHCP6_OPT_LEN_OFFSET, duid_len);

    return ((UINT16)(duid_len + DHCP6_MSG_LEN));

} /* DHCP6_Build_Client_ID_Option */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Build_Server_ID_Option
*
*   DESCRIPTION
*
*       This routine builds a Server ID option in the provided buffer.
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
*                               option is being built.
*       *ds_ptr                 Pointer to DHCP Structure that contains 
*                               data pertinent to the option.
*
*   OUTPUTS
*
*       The number of bytes added to the packet.
*
**************************************************************************/
UINT16 DHCP6_Build_Server_ID_Option(UINT8 *buffer, DHCP6_STRUCT *ds_ptr)
{
    UINT16  len = DHCP6_SERVER_ID_OPT_LEN;

    /* The code is Server ID. */
    PUT16(buffer, DHCP6_OPT_CODE_OFFSET, DHCP6_OPT_SERVERID);

    /* The length is the length of the DUID. */
    PUT16(buffer, DHCP6_OPT_LEN_OFFSET, 
          ds_ptr->dhcp6_server.dhcp6_duid.duid_len);

    /* Add the proper code to the packet. */
    PUT16(buffer, DHCP6_DUID_CODE_OFFSET, 
          ds_ptr->dhcp6_server.dhcp6_duid.duid_type);

    /* If this DUID is a DUID Based on Link-layer Address Plus Time. */
    if (ds_ptr->dhcp6_server.dhcp6_duid.duid_type == DHCP_DUID_LLT)
    {
        /* Add the hardware type to the packet. */
        PUT16(buffer, DHCP6_DUID_LLT_HW_TYPE_OFFSET, 
              ds_ptr->dhcp6_server.dhcp6_duid.duid_hw_type); 

        /* Add the time to the packet. */
        PUT32(buffer, DHCP6_DUID_LLT_TIME_OFFSET,
              ds_ptr->dhcp6_server.dhcp6_duid.duid_time);

        /* Add the Link-Layer Address to the packet. */
        NU_BLOCK_COPY(&buffer[DHCP6_DUID_LLT_ADDR_OFFSET],
                      ds_ptr->dhcp6_server.dhcp6_duid.duid_ll_addr, 
                      DADDLEN);

        len += (DHCP6_DUID_LLT_LEN + DADDLEN);
    }

    /* If this DUID is a DUID Assigned by Vendor Based on Enterprise 
     * Number. 
     */
    else if (ds_ptr->dhcp6_server.dhcp6_duid.duid_type == DHCP_DUID_EN)
    {
        /* Add the Enterprise Number to the packet. */
        PUT32(buffer, DHCP6_DUID_EN_ENT_NO_OFFSET, 
              ds_ptr->dhcp6_server.dhcp6_duid.duid_ent_no); 

        /* Add the Identifier to the packet. */
        NU_BLOCK_COPY(&buffer[DHCP6_DUID_EN_ID_OFFSET],
                      ds_ptr->dhcp6_server.dhcp6_duid.duid_id, 
                      DADDLEN);

        len += (DHCP6_DUID_EN_LEN + DADDLEN);
    }

    /* Otherwise, this DUID is a DUID Based on Link-layer Address. */
    else
    {
        /* Add the Hardware Type to the packet. */
        PUT16(buffer, DHCP6_DUID_LL_HW_TYPE_OFFSET, 
              ds_ptr->dhcp6_server.dhcp6_duid.duid_hw_type); 

        /* Add the Identifier to the packet. */
        NU_BLOCK_COPY(&buffer[DHCP6_DUID_LL_ADDR_OFFSET],
                      ds_ptr->dhcp6_server.dhcp6_duid.duid_ll_addr, 
                      DADDLEN);

        len += (DHCP6_DUID_LL_LEN + DADDLEN);
    }

    return (len);

} /* DHCP6_Build_Server_ID_Option */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Build_IA_NA_Option
*
*   DESCRIPTION
*
*       This routine builds an Identity Association for Non-Temporary
*       Addresses option in the provided buffer.
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
*                               option is being built.
*       identity_assoc          The Identity Association to add to this 
*                               packet.
*       time1                   The time at which the client contacts the
*                               server from which the addresses in the IA_NA
*                               were obtained to extend the lifetimes of the
*                               addresses assigned to the IA_NA; T1 is a
*                               time duration relative to the current time
*                               expressed in units of seconds.  The client 
*                               sets T1 and T2 to 0 if it has no preference 
*                               for those values; otherwise, this value 
*                               indicates the client's preference.
*       time2                   The time at which the client contacts any
*                               available server to extend the lifetimes of
*                               the addresses assigned to the IA_NA; T2 is a
*                               time duration relative to the current time
*                               expressed in units of seconds.  The client 
*                               sets T1 and T2 to 0 if it has no preference 
*                               for those values; otherwise, this value 
*                               indicates the client's preference.
*       *ia_addr_ptr            Pointer to the structure containing the
*                               IPv6 addresses to add as IA_ADDR options.
*       ia_count                The number of addresses in ia_addr_ptr.
*
*   OUTPUTS
*
*       The number of bytes added to the packet.
*
**************************************************************************/
UINT16 DHCP6_Build_IA_NA_Option(UINT8 *buffer, UINT32 identity_assoc, 
                                UINT32 time1, UINT32 time2, 
                                DHCP6_IA_ADDR_STRUCT *ia_addr_ptr,
                                INT ia_count)
{
    INT     i;
    UINT16  nbytes;

    /* The code is IA_NA. */
    PUT16(buffer, DHCP6_OPT_CODE_OFFSET, DHCP6_OPT_IA_NA);

    /* The length is 12 plus the length of the options. */
    PUT16(buffer, DHCP6_OPT_LEN_OFFSET, (UINT16)(DHCP6_IA_NA_PAYLOAD_LEN + 
          DHCP6_IA_ADDR_OPT_LEN * ia_count));

    /* Add the IAID to the packet. */
    PUT32(buffer, DHCP6_IA_NA_IAID_OFFSET, identity_assoc);

    /* Add T1 to the packet. */
    PUT32(buffer, DHCP6_IA_NA_T1_OFFSET, time1);

    /* Add T2 time to the packet. */
    PUT32(buffer, DHCP6_IA_NA_T2_OFFSET, time2);

    /* Initialize nbytes to the total number of bytes added to the packet
     * so far.
     */
    nbytes = DHCP6_IA_NA_OPT_LEN;

    /* Add each IPv6 address to the packet in the form of an IA_ADDR 
     * option.
     */
    for (i = 0; i < ia_count; i++)
    {
        /* Build the IA_ADDR option for this IPv6 address. */
        nbytes += DHCP6_Build_IA_Addr_Option(&buffer[nbytes], 
                                             ia_addr_ptr->dhcp6_ia_addr,
                                             ia_addr_ptr->dhcp6_ia_pref_life, 
                                             ia_addr_ptr->dhcp6_ia_valid_life);
    }

    /* Return the total number of bytes added to the packet. */
    return (nbytes);

} /* DHCP6_Build_IA_NA_Option */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Build_IA_Addr_Option
*
*   DESCRIPTION
*
*       This routine builds an IA Address option in the provided buffer.
*       The IA Address option must be encapsulated in the Options field 
*       of an IA_NA option.
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
*       *buffer                 Pointer to the buffer in which the IA_ADDR 
*                               option is being built.
*       *addr_ptr               Pointer to the IP address to add to the
*                               packet.
*       pref_life               The preferred lifetime for the IPv6 address 
*                               in the option, expressed in units of seconds.
*       valid_life              The valid lifetime for the IPv6 address in 
*                               the option, expressed in units of seconds.
*
*   OUTPUTS
*
*       The number of bytes added to the packet.
*
**************************************************************************/
UINT16 DHCP6_Build_IA_Addr_Option(UINT8 *buffer, UINT8 *addr_ptr, 
                                  UINT32 pref_life, UINT32 valid_life)
{
    /* The code is IA ADDR. */
    PUT16(buffer, DHCP6_OPT_CODE_OFFSET, DHCP6_OPT_IAADDR);

    /* The length is 24 plus the length of the IAaddr-options field. */
    PUT16(buffer, DHCP6_OPT_LEN_OFFSET, 
          DHCP6_IA_ADDR_OPT_LEN - DHCP6_MSG_LEN);

    /* Add the IP address to the packet. */
    NU_BLOCK_COPY(&buffer[DHCP6_IAADR_ADDR_OFFSET], addr_ptr, IP6_ADDR_LEN);

    /* Add the preferred lifetime to the packet. */
    PUT32(buffer, DHCP6_IAADR_PREF_LIFE_OFFSET, pref_life);

    /* Add the valid lifetime to the packet. */
    PUT32(buffer, DHCP6_IAADR_VALID_LIFE_OFFSET, valid_life);

    return ((UINT16)DHCP6_IA_ADDR_OPT_LEN);

} /* DHCP6_Build_IA_Addr_Option */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Build_Option_Request_Option
*
*   DESCRIPTION
*
*       This routine builds an Option Request option in the provided buffer.
*       The Option Request option is used to identify a list of options in a
*       message between a client and a server.
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
*                               option is being built.
*       opt_count               The number of options being included in the
*                               Option Request Option.
*       *opts_ptr               A pointer to the options to add to the 
*                               buffer.
*
*   OUTPUTS
*
*       The number of bytes added to the packet.
*
**************************************************************************/
UINT16 DHCP6_Build_Option_Request_Option(UINT8 *buffer, UINT8 opt_count, 
                                         UINT16 *opts_ptr)
{
    UINT8   i;

    /* The code is ORO. */
    PUT16(buffer, DHCP6_OPT_CODE_OFFSET, DHCP6_OPT_ORO);

    /* The length is 2 times the number of requested options. */
    PUT16(buffer, DHCP6_OPT_LEN_OFFSET, 2 * opt_count);

    /* Add each option to the packet. */
    for (i = 0; i < opt_count; i++)
    {
        /* Add the option to the packet.  Each option is 2 bytes, so 
         * increase the offset into the buffer accordingly. 
         */
        PUT16(buffer, DHCP6_ORO_CODE_OFFSET + (i * 2), opts_ptr[i]);
    }

    /* Return the total number of bytes added to the packet. */
    return (DHCP6_OPT_REQUEST_OPT_LEN + (2 * opt_count));

} /* DHCP6_Build_Option_Request_Option */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Build_Rapid_Commit_Option
*
*   DESCRIPTION
*
*       This routine builds a Rapid Commit option in the provided buffer.
*       A client MAY include this option in a Solicit message if the client
*       is prepared to perform the Solicit-Reply message exchange.
*
*     0                   1                   2                   3
*     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*    |      OPTION_RAPID_COMMIT      |               0               |
*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*
*   INPUTS
*
*       *buffer                 Pointer to the buffer in which the 
*                               option is being built.
*
*   OUTPUTS
*
*       The number of bytes added to the packet.
*
**************************************************************************/
UINT16 DHCP6_Build_Rapid_Commit_Option(UINT8 *buffer)
{
    /* The code is RAPID COMMIT. */
    PUT16(buffer, DHCP6_OPT_CODE_OFFSET, DHCP6_OPT_RAPID_COMMIT);

    /* The length is 0. */
    PUT16(buffer, DHCP6_OPT_LEN_OFFSET, 0);

    return ((UINT16)DHCP6_RAPID_COMMIT_OPT_LEN);

} /* DHCP6_Build_Rapid_Commit_Option */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Build_Elapsed_Time_Option
*
*   DESCRIPTION
*
*       This routine builds an Elapsed Time option in the provided buffer.
*       A client MUST include an Elapsed Time option in messages to indicate
*       how long the client has been trying to complete a DHCP message
*       exchange.  Elapsed time is expressed in hundredths of seconds.
*       The DHCPv6 module will insert the proper time value in the packet
*       upon transmission.
*
*       0                   1                   2                   3
*       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*      |      OPTION_ELAPSED_TIME      |           option-len          |
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*      |          elapsed-time         |
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*
*   INPUTS
*
*       *buffer                 Pointer to the buffer in which the 
*                               option is being built.
*
*   OUTPUTS
*
*       The number of bytes added to the packet.
*
**************************************************************************/
UINT16 DHCP6_Build_Elapsed_Time_Option(UINT8 *buffer, DHCP6_TX_STRUCT *tx_ptr)
{
    UINT32  elapsed_time;

    /* The code is ELAPSED TIME. */
    PUT16(buffer, DHCP6_OPT_CODE_OFFSET, DHCP6_OPT_ELAPSED_TIME);

    /* The length is 2. */
    PUT16(buffer, DHCP6_OPT_LEN_OFFSET, 
          DHCP6_ELAPSED_TIME_OPT_LEN - DHCP6_MSG_LEN);

    /* RFC 3315 - section 22.9 - The elapsed-time field is set to 0 in 
     * the first message in the message exchange.
     */
    if (tx_ptr->dhcp6_init_time == 0)
    {
        elapsed_time = 0;

        /* Initialize the init_time. */
        tx_ptr->dhcp6_init_time = NU_Retrieve_Clock();
    }

    /* Determine how long has passed since the first packet was 
     * transmitted to the server for this transaction.
     */
    else
    {
        elapsed_time = NU_Retrieve_Clock() - tx_ptr->dhcp6_init_time;

        /* If the value exceeds the 2-byte field it has to fit in,
         * reset it.
         */
        if (elapsed_time > 0xffff)
            elapsed_time = 0xffff;
    }

    /* Add the elapsed time to the packet. */
    PUT16(buffer, DHCP6_ELAPSE_TIME_OFFSET, (UINT16)elapsed_time);

    return ((UINT16)DHCP6_ELAPSED_TIME_OPT_LEN);

} /* DHCP6_Build_Elapsed_Time_Option */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Build_Status_Code_Option
*
*   DESCRIPTION
*
*       This routine builds a Status Code option in the provided buffer.
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
*       *buffer                 Pointer to the buffer in which the 
*                               option is being built.
*       *status_msg             A pointer to the message to add to the 
*                               packet.
*       status_code             The status code to add to the packet.
*
*   OUTPUTS
*
*       The number of bytes added to the packet.
*
**************************************************************************/
UINT16 DHCP6_Build_Status_Code_Option(UINT8 *buffer, CHAR *status_msg,
                                      UINT16 status_code)
{
    /* The code is STATUS CODE. */
    PUT16(buffer, DHCP6_OPT_CODE_OFFSET, DHCP6_OPT_STATUS_CODE);

    /* The length is 2 plus the length of the status message. */
    PUT16(buffer, DHCP6_OPT_LEN_OFFSET, 
          (UINT16)((DHCP6_STATUS_CODE_OPT_LEN - DHCP6_MSG_LEN) + strlen(status_msg)));

    /* Add the status code to the packet. */
    PUT16(buffer, DHCP6_STATUS_CODE_OFFSET, status_code);

    /* Add the status message to the packet. */
    NU_BLOCK_COPY(&buffer[DHCP6_STATUS_CODE_MSG_OFFSET], status_msg, 
                  strlen(status_msg));

    return ((UINT16)(DHCP6_STATUS_CODE_OPT_LEN + strlen(status_msg)));

} /* DHCP6_Build_Status_Code_Option */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Build_User_Class_Option
*
*   DESCRIPTION
*
*       This routine builds a User Class option in the provided buffer.
*
*       0                   1                   2                   3
*       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*      |       OPTION_USER_CLASS       |          option-len           |
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*      .                                                               .
*      .                          user-class-data                      .
*      .                                                               .
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*
*       Where user-class-data is represented as:
*
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-...-+-+-+-+-+-+-+
*      |        user-class-len         |          opaque-data          |
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-...-+-+-+-+-+-+-+    
*
*   INPUTS
*
*       *buffer                 Pointer to the buffer in which the 
*                               option is being built.
*       *user_class_ptr         Pointer to user-class-data information to
*                               add to the packet.  Each user-class-data
*                               member is null-termined to indicate the end 
*                               of that entry.
*       opt_count               The number of entries of user-class-data in 
*                               the user_class_ptr memory.
*
*   OUTPUTS
*
*       The number of bytes added to the packet.
*
**************************************************************************/
UINT16 DHCP6_Build_User_Class_Option(UINT8 *buffer, CHAR *user_class_ptr,
                                     INT opt_count)
{
    INT     i, opt_len = 0, buffer_idx = 0;
    UINT16  user_data_len;

    /* The code is USER CLASS. */
    PUT16(buffer, DHCP6_OPT_CODE_OFFSET, DHCP6_OPT_USER_CLASS);

    /* Add each user-class-data entry to the packet. */
    for (i = 0; i < opt_count; i ++)
    {
        /* Get the length of the user-class-data in this option. */
        user_data_len = (UINT16)(strlen(&user_class_ptr[buffer_idx]));

        /* Add the length of the opaque data in this user-class-data field. */
        PUT16(buffer, opt_len + DHCP6_USER_CLASS_LEN_OFFSET, 
              user_data_len);

        /* Add the opaque data in the user-class-data field to the packet. */
        strcpy((CHAR*)&buffer[opt_len + DHCP6_USER_CLASS_DATA_OFFSET], 
               &user_class_ptr[buffer_idx]);

        /* Increment the length of the data added to the packet, including
         * the two bytes of data for the user-class-data length field. 
         */
        opt_len += (user_data_len + 2);

        /* Increment the index into the user buffer, including the 
         * null-terminator of the user-class-data string. 
         */
        buffer_idx += (1 + user_data_len);
    }

    /* Add the length of all user-class-data entries to the packet. */
    PUT16(buffer, DHCP6_OPT_LEN_OFFSET, (UINT16)opt_len);

    /* Return the total number of bytes added to the buffer. */
    return ((UINT16)(opt_len + DHCP6_MSG_LEN));

} /* DHCP6_Build_User_Class_Option */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Build_Vendor_Class_Option
*
*   DESCRIPTION
*
*       This routine builds a Vendor Class option in the provided buffer.
*
*       0                   1                   2                   3
*       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*      |      OPTION_VENDOR_CLASS      |           option-len          |
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*      |                       enterprise-number                       |
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*      .                                                               .
*      .                       vendor-class-data                       .
*      .                             . . .                             .
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*
*       Where vendor-class-data is represented as:
*
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-...-+-+-+-+-+-+-+
*      |       vendor-class-len        |          opaque-data          |
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-...-+-+-+-+-+-+-+
*
*   INPUTS
*
*       *buffer                 Pointer to the buffer in which the 
*                               option is being built.
*       ent_number              The enterprise-number.
*       *vendor_class_ptr       A pointer to the vendor-class-data to
*                               add to the packet.  Each vendor-class-data
*                               member is null-termined to indicate the end 
*                               of that entry.
*       opt_count               The number of entries of vendor-class-data 
*                               in the vendor_class_ptr memory.
*
*   OUTPUTS
*
*       The number of bytes added to the packet.
*
**************************************************************************/
UINT16 DHCP6_Build_Vendor_Class_Option(UINT8 *buffer, UINT32 ent_number,
                                       CHAR *vendor_class_ptr, INT opt_count)
{
    INT     i, opt_len = 0, buffer_idx = 0;
    UINT16  vendor_data_len;

    /* The code is VENDOR CLASS. */
    PUT16(buffer, DHCP6_OPT_CODE_OFFSET, DHCP6_OPT_VENDOR_CLASS);

    /* Insert the enterprise number. */
    PUT32(buffer, DHCP6_VENDOR_CLASS_ENT_OFFSET, ent_number);

    /* Add each vendor-class-data entry to the packet. */
    for (i = 0; i < opt_count; i ++)
    {
        /* Determine the length of the vendor-class-data. */
        vendor_data_len = (UINT16)(strlen(&vendor_class_ptr[buffer_idx]));

        /* Add the length of the opaque data in this vendor-class-data 
         * field. 
         */
        PUT16(buffer, opt_len + DHCP6_VENDOR_CLASS_LEN_OFFSET, 
              vendor_data_len);

        /* Add the opaque data in the vendor-class-data field to the packet. */
        strcpy((CHAR*)&buffer[opt_len + DHCP6_VENDOR_CLASS_DATA_OFFSET], 
               &vendor_class_ptr[buffer_idx]);

        /* Increment the length of the data added to the packet, including
         * the two bytes of data for the vendor-class-data length field. 
         */
        opt_len += (vendor_data_len + 2);

        /* Increment the index into the user buffer, including the 
         * null-terminator. 
         */
        buffer_idx += (1 + vendor_data_len);
    }

    /* Add the length of all user-data-class entries plus 4-bytes for the 
     * enterprise number to the packet.
     */
    PUT16(buffer, DHCP6_OPT_LEN_OFFSET, (UINT16)(opt_len + 4));

    /* Return the total number of bytes added to the buffer. */
    return ((UINT16)(opt_len + DHCP6_VENDOR_CLASS_OPT_LEN + 4));

} /* DHCP6_Build_Vendor_Class_Option */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Build_Vendor_Specific_Info_Option
*
*   DESCRIPTION
*
*       This routine builds a Vendor Specific Information option in the 
*       provided buffer.  Multiple instances of the Vendor-specific 
*       Information option may appear in a DHCP message.  For each
*       subsequent set of option-data the user wishes to include, the user
*       must make a separate call to this routine to add another Vendor
*       Specific Information Option to the packet.
*
*       0                   1                   2                   3
*       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*      |      OPTION_VENDOR_OPTS       |           option-len          |
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*      |                       enterprise-number                       |
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*      .                                                               .
*      .                          option-data                          .
*      .                                                               .
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*
*       Where option-data is represented as:
*
*       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*      |          opt-code             |             option-len        |
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*      .                                                               .
*      .                          option-data                          .
*      .                                                               .
*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*
*   INPUTS
*
*       *buffer                 Pointer to the buffer in which the 
*                               option is being built.
*       ent_number              The enterprise-number.
*       opt_code                The option code to add to the option-data
*                               opaque field.
*       *option_data            A pointer to the null-terminated option-data 
*                               to add to the option-data opaque field.
*
*   OUTPUTS
*
*       The number of bytes added to the packet.
*
**************************************************************************/
UINT16 DHCP6_Build_Vendor_Specific_Info_Option(UINT8 *buffer, 
                                               UINT32 ent_number, 
                                               UINT16 opt_code,
                                               CHAR *option_data)
{
    UINT16  data_len;

    /* Determine the length of the opaque data being added to the packet. */
    data_len = (UINT16)(strlen(option_data));

    /* The code is VENDOR OPTS. */
    PUT16(buffer, DHCP6_OPT_CODE_OFFSET, DHCP6_OPT_VENDOR_OPTS);

    /* The total data length of the option is 4 + the length of the 
     * option-data field. 
     */
    PUT16(buffer, DHCP6_OPT_LEN_OFFSET, (DHCP6_VENDOR_SPEC_OPQ_LEN + 
          data_len + DHCP6_VENDOR_SPEC_OPT_LEN));

    /* Add the enterprise number to the packet. */
    PUT32(buffer, DHCP6_VENDOR_SPEC_ENT_OFFSET,  ent_number);

    /* Add the code of the Vendor Specific field to the packet. */
    PUT16(buffer, DHCP6_VENDOR_SPEC_CODE_OFFSET, opt_code);

    /* Add the length of the Vendor Specific data to the packet. */
    PUT16(buffer, DHCP6_VENDOR_SPEC_LEN_OFFSET, data_len);

    /* Add the Vendor Specific data to the packet. */
    strcpy((CHAR*)&buffer[DHCP6_VENDOR_SPEC_DATA_OFFSET], option_data);

    /* Return the total number of bytes of data added to the packet. */
    return ((UINT16)(DHCP6_VENDOR_SPEC_OPT_LEN + DHCP6_VENDOR_SPEC_OPQ_LEN + 
            data_len + DHCP6_MSG_LEN));

} /* DHCP6_Build_Vendor_Specific_Info_Option */

/**************************************************************************
*
*   FUNCTION
*
*       DHCP6_Build_Reconfigure_Accept_Option
*
*   DESCRIPTION
*
*       This routine builds a Reconfigure Accept option in the provided 
*       buffer.
*
*     0                   1                   2                   3
*     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*    |     OPTION_RECONF_ACCEPT      |               0               |
*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*
*   INPUTS
*
*       *buffer                 Pointer to the buffer in which the 
*                               option is being built.
*
*   OUTPUTS
*
*       The number of bytes added to the packet.
*
**************************************************************************/
UINT16 DHCP6_Build_Reconfigure_Accept_Option(UINT8 *buffer)
{
    /* The code is RECONF ACCEPT. */
    PUT16(buffer, DHCP6_OPT_CODE_OFFSET, DHCP6_OPT_RECONF_ACCEPT);

    /* The length is 0. */
    PUT16(buffer, DHCP6_OPT_LEN_OFFSET, DHCP6_RECON_ACC_LEN_VALUE);

    return ((UINT16)DHCP6_RCNFGR_ACC_OPT_LEN);

} /* DHCP6_Build_Reconfigure_Accept_Option */

#endif
