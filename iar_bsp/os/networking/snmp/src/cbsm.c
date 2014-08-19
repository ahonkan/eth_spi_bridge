/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 1998 - 2007
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
*       cbsm.c                                                   
*
*   DESCRIPTION
*
*       This file contains functions common to both CBSMv1 and CBSMv2.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       CBSM_Init
*       CBSM_Config
*       CBSM_Verify
*       CBSM_Get_Community_Name
*       CBSM_Add_Community
*       CBSM_Compare_Index
*       CBSM_Save_Community
*
*   DEPENDENCIES
*
*       nucleus.h
*       snmp.h
*       agent.h
*       snmp_dis.h
*       snmp_no.h
*       cbsm.h
*
*************************************************************************/

#include "nucleus.h"
#include "networking/snmp.h"
#include "networking/agent.h"
#include "networking/snmp_dis.h"
#include "networking/snmp_no.h"
#include "networking/cbsm.h"

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)
#include "networking/snmp_file.h"
#endif

#if ((INCLUDE_SNMPv1 == NU_TRUE) || (INCLUDE_SNMPv2 == NU_TRUE))


extern SNMP_ENGINE_STRUCT       Snmp_Engine;
extern SNMP_TARGET_MIB          Snmp_Target_Mib;
extern agent_stat_t             agentStat;
extern  snmp_cfig_t             snmp_cfg;
extern NU_MEMORY_POOL System_Memory;

CBSM_COMMUNITY_TABLE               Cbsm_Mib_Root;
STATIC UINT8                       Cbsm_Status = SNMP_MODULE_NOTSTARTED;
extern SNMP_CBSM_COMMUNITY_STRUCT  CBSM_Community_Table[];

/************************************************************************
*
*   FUNCTION
*
*       CBSM_Init
*
*   DESCRIPTION
*
*       This function initializes the Community-based Security Model.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       NU_SUCCESS              Initialization successful
*       SNMP_ERROR              Initialization unsuccessful
*
*************************************************************************/
STATUS CBSM_Init(VOID)
{

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)

    /* Structure to which data will be read from file. */
    CBSM_COMMUNITY_STRUCT community;

    SNMP_READ_FILE      table_information =
        /* File name, Structure pointer, Sizeof structure, Insert
         * function.
         */
        {CBSM_FILE, NU_NULL,
         (SNMP_ADD_MIB_ENTRY)CBSM_Add_Community,
         sizeof(CBSM_COMMUNITY_STRUCT)};
#endif

    STATUS                status;

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)

    /* Assign a location that will be used to read data from file. */
    table_information.snmp_read_pointer = &community;

    /* Read the table from file. */
    status = SNMP_Read_File(&table_information);

    /* If we successfully read data from file. */
    if(status == NU_SUCCESS)
    {
        /* CBSM successfully initialized. */
        Cbsm_Status = SNMP_MODULE_INITIALIZED;
    }

    else
#endif
    {
        status = SNMP_ERROR;
        Cbsm_Status = SNMP_MODULE_NOTINITIALIZED;
    }


    return (status);

} /* CBSM_Init */

/************************************************************************
*
*   FUNCTION
*
*       CBSM_Config
*
*   DESCRIPTION
*
*       This function configures the Community-based Security Model.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       NU_SUCCESS              Initialization successful
*       SNMP_ERROR              Initialization unsuccessful
*
*************************************************************************/
STATUS CBSM_Config(VOID)
{
    STATUS                  status = NU_SUCCESS;
    INT                     i;
    CBSM_COMMUNITY_STRUCT   community;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* Wait for the CBSM to be initialized. */
    while(Cbsm_Status == SNMP_MODULE_NOTSTARTED)
        /* Sleep for a second. */
        NU_Sleep(NU_PLUS_Ticks_Per_Second);

    /* Check whether the initialization has succeeded. */
    if (Cbsm_Status == SNMP_MODULE_NOTINITIALIZED)
    {
        for (i = 0; i < CBSM_MAX_COMMUNITIES; i++)
        {
            /* Clearing out the community structure. */
            UTL_Zero(&community, sizeof(CBSM_COMMUNITY_STRUCT));

            /* Update community index. */
            strcpy((CHAR *)community.cbsm_community_index,
                   CBSM_Community_Table[i].community_name);

            /* Update community index length. */
            community.cbsm_community_index_len =
                (UINT8)(strlen(CBSM_Community_Table[i].community_name));

            /* Updating community name. */
            strcpy((CHAR *)community.cbsm_community_name,
                   CBSM_Community_Table[i].community_name);

            /* Updating the context name. */
            strcpy((CHAR *)community.cbsm_context_name,
                   CBSM_Community_Table[i].context_name);

            /* Updating engine ID. */
            NU_BLOCK_COPY(community.cbsm_engine_id,
                          Snmp_Engine.snmp_engine_id,
                          (unsigned int)Snmp_Engine.snmp_engine_id_len);

            /* Updating Engine ID length. */
            community.cbsm_engine_id_len =
                                    (UINT8)Snmp_Engine.snmp_engine_id_len;

            /* Updating security name. */
            strcpy((CHAR*)community.cbsm_security_name,
                    CBSM_Community_Table[i].security_name);

#if (INCLUDE_MIB_CBSM == NU_TRUE)
            /* Setting row status to 'active'. */
            community.cbsm_status = SNMP_ROW_ACTIVE;

            /* Setting storage to non-volatile. */
            community.cbsm_storage_type = SNMP_STORAGE_DEFAULT;
#endif
            /* Updating transport tag. */
            strcpy((CHAR *)community.cbsm_transport_tag,
                   CBSM_Community_Table[i].transport_tag);

            /* Updating transport tag length. */
            community.cbsm_transport_tag_len =
                            strlen((CHAR *)community.cbsm_transport_tag);

            /* Adding CBSM community. */
            status = CBSM_Add_Community(&community);

            /* If CBSM community is not added successfully then break
             * through the loop and return error code.
             */
            if (status != NU_SUCCESS)
            {
                NLOG_Error_Log("SNMP: Failed to add CBSM community",
                                NERR_SEVERE, __FILE__, __LINE__);

                break;
            }
        }

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)

        if (status == NU_SUCCESS)
        {
            /* Create the file. */
            status = SNMP_Create_File(CBSM_FILE);

            if (status == NU_SUCCESS)
            {
                /* Save the configurations to file. */
                status = CBSM_Save_Community(Cbsm_Mib_Root.next);
            }
        }
#endif /* (SNMP_ENABLE_FILE_STORAGE == NU_TRUE) */
        if (status == NU_SUCCESS)
        {
            Cbsm_Status = SNMP_MODULE_INITIALIZED;
        }
    }   /* if (cbsm_Status == SNMP_MODULE_NOTINITIALIZED) */

    /* return to user mode */
    NU_USER_MODE();
    return (status);

} /* CBSM_Config */

/************************************************************************
*
*   FUNCTION
*
*       CBSM_Verify
*
*   DESCRIPTION
*
*       This function verifies the incoming message.
*
*   INPUTS
*
*       mms                     maximum message size
*       *security_param         security parameters
*       **whole_msg             whole message
*       *msg_len                length of complete message
*       *security_engine_id     engine id
*       *security_engine_id_len length of engine id
*       *security_name          security name
*       *max_size_response_pdu  size of response PDU
*       *error_indication       filled in case of error indication
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_ERROR
*
*************************************************************************/
STATUS CBSM_Verify(UINT32 mms, UINT8 *security_param, UINT8 **whole_msg,
                   UINT32 *msg_len, UINT8 *security_engine_id,
                   UINT32 *security_engine_id_len, UINT8 *security_name,
                   UINT32 *max_size_response_pdu,
                   SNMP_ERROR_STRUCT *error_indication)
{
    STATUS                  status = SNMP_ERROR;
    CBSM_COMMUNITY_STRUCT   *node;
    SNMP_TARGET_ADDRESS_TABLE  *t_node;
    UINT32                  i;
    SNMP_NOTIFY_REQ_STRUCT  *snmp_notification;
    static UINT8            community_name[SNMP_SIZE_BUFCHR];
    UINT32                  t_domain;
    INT16                   source_family;
    static UINT8            source_address[SNMP_MAX_IP_ADDRS];
    static UINT8            temp_address[SNMP_MAX_IP_ADDRS];
    static UINT32           auth_trap_oid[] = SNMP_AUTH_FAILURE_TRAP;

    UNUSED_PARAMETER(whole_msg);
    UNUSED_PARAMETER(msg_len);
    UNUSED_PARAMETER(error_indication);

    /* Sleep for a second while CBSM is not initialized. */
    while (Cbsm_Status != SNMP_MODULE_INITIALIZED)
        NU_Sleep(NU_PLUS_Ticks_Per_Second);

    /* Decode the community name from security parameters. */
    for (i = 0; security_param[i] != ':'; i++)
        community_name[i] = security_param[i];

    /* Null terminate the community name. */
    community_name[i] = NU_NULL;

    /* Get the transport domain. */
    t_domain = security_param[i + 1];

    source_family = (INT16)(((INT16)(security_param[i+3]) & 0xff) +
                  ((INT16)(((UINT32)(security_param[i+4]) & 0xff) << 8)));

    /* Get the source address. */
    NU_BLOCK_COPY(source_address, &security_param[i + 5],
                  SNMP_MAX_IP_ADDRS);

    /* Search the community table. */
    node = Cbsm_Mib_Root.next;

    while (node != NU_NULL)
    {
        /* If the community name is equal and the row is active. */
        if (
#if (INCLUDE_MIB_CBSM == NU_TRUE)
             (node->cbsm_status == SNMP_ROW_ACTIVE) &&
#endif
             (strcmp((CHAR*)node->cbsm_community_name,
                     (CHAR*)community_name) == 0) )
        {
            /* If snmpCommunityTransportTag is an empty string then
               the message is authenticated. */
            if (node->cbsm_transport_tag_len == 0)
            {
                status = NU_SUCCESS;
                break;
            }
            else
            {
                /* Otherwise, check whether the source address matches
                 * the address of the entry in the target address table.
                 */
                t_node = Snmp_Target_Mib.target_addr_table.next;

                while (t_node != NU_NULL)
                {
                    if (SNMP_Compare_Tag(node->cbsm_transport_tag,
                                        node->cbsm_transport_tag_len,
                                        t_node->snmp_target_addr_tag_list,
                                        t_node->tag_list_len))
                    {
                        /* We have found an address with the given tag.
                           Check whether this is the source address we are
                           looking for. */

                        /* First check if this row is active. */
#if (INCLUDE_MIB_TARGET == NU_TRUE)
                        if (t_node->snmp_target_addr_row_status ==
                                                          SNMP_ROW_ACTIVE)
#endif
                        {
                            /* We need to do an exact match if the table
                             * extension is not enabled.
                             */
                            if ( (t_node->snmp_ext_enabled == NU_FALSE) &&
                                 (t_node->snmp_target_addr_tfamily ==
                                                        source_family) &&
                                 (((t_node->snmp_target_addr_tfamily ==
                                                        NU_FAMILY_IP) &&
                                   (memcmp(t_node->
                                            snmp_target_addr_tAddress,
                                       source_address, IP_ADDR_LEN) == 0))
#if (INCLUDE_IPV6 == NU_TRUE)
                                   || ((t_node->snmp_target_addr_tfamily
                                                    == NU_FAMILY_IP6) &&
                                   (memcmp(t_node->
                                      snmp_target_addr_tAddress,
                                      source_address, IP6_ADDR_LEN) == 0))
#endif
                                  ) && (t_node->snmp_target_addr_tDomain
                                                            == t_domain) )
                            {
                                status = NU_SUCCESS;
                                break;
                            }
                            else if ( (t_node->snmp_ext_enabled ==
                                                               NU_TRUE) &&
                                      (t_node->snmp_target_addr_tDomain ==
                                                              t_domain) &&
                                      (t_node->snmp_target_addr_tfamily ==
                                                           source_family) )
                            {
                                /* We need to use the mask. This loop
                                   does a logical and for both the
                                   addresses with the mask. The resulting
                                   addresses are compared and we are done.
                                 */

                                /* First set the address from the table in
                                   to temp_address. We do not want to ruin
                                   the tables value. */
                                NU_BLOCK_COPY(temp_address,
                                        t_node->snmp_target_addr_tAddress,
                                        SNMP_MAX_IP_ADDRS);

                                for (i = 0; i < SNMP_MAX_IP_ADDRS; i++)
                                {
                                    temp_address[i] =
                                        (UINT8)(temp_address[i] &
                                                t_node->
                                               snmp_target_addr_tmask[i]);

                                    source_address[i] =
                                        (UINT8)(source_address[i] &
                                                t_node->
                                               snmp_target_addr_tmask[i]);
                                }

                                if ( ( (source_family == NU_FAMILY_IP) &&
                                       (memcmp(source_address,
                                               temp_address,
                                               IP_ADDR_LEN) == 0) )
#if (INCLUDE_IPV6 == NU_TRUE)
                                     || ( (source_family == NU_FAMILY_IP6)
                                          && (memcmp(source_address,
                                                     temp_address,
                                                     IP6_ADDR_LEN) == 0) )
#endif /* (INCLUDE_IPV6 == NU_TRUE) */
                                   )
                                {
                                    status = NU_SUCCESS;
                                    mms = t_node->snmp_target_addr_mms;
                                    break;
                                }
                            }
                        }
                    }

                    t_node = t_node->next;
                }
            }
        }

        /* Break out of the loop if authentication has been successful. */
        if (status == NU_SUCCESS)
            break;

        node = node->next;
    }

    /* If the authentication was successful...*/
    if ((status == NU_SUCCESS) && (node))
    {
        /* Copy the local engine id. */
        NU_BLOCK_COPY(security_engine_id, node->cbsm_engine_id,
                      (unsigned int)node->cbsm_engine_id_len);

        *security_engine_id_len = node->cbsm_engine_id_len;

        /* 35 is the maximum possible header size for v1 and v2. */
        *max_size_response_pdu = mms - (30 + 5 +
                                 strlen((CHAR*)community_name));

        /* Use the security parameters to pass the context engine id and
         * context name.
         */
        PUT32(security_param, 0, (unsigned long)node->cbsm_engine_id_len);

        NU_BLOCK_COPY(&security_param[4], node->cbsm_engine_id,
               node->cbsm_engine_id_len);

#if (INCLUDE_MIB_CBSM == NU_TRUE)
        /* Copy the context name. */
        strcpy((CHAR*)(&security_param[4 + node->cbsm_engine_id_len]),
               (CHAR*)node->cbsm_context_name);
#else
        /* append a null character */
        security_param[4 + node->cbsm_engine_id_len] = 0;
#endif

        /* Copy the security name. */
        strcpy((CHAR*)security_name, (CHAR*)node->cbsm_security_name);
    }

    else
    {
        /* Authentication was not successful! */
        agentStat.InBadCommunityNames++;
        agentStat.InBadCommunityUses++;

        /* Send the authentication failure trap. */
        if (snmp_cfg.authentrap_enable == NU_TRUE )
        {
            /* Get an empty location to place the notification. */
            if(SNMP_Get_Notification_Ptr(&snmp_notification) ==
                                                               NU_SUCCESS)
            {
                /* Copy the Authentication Failure OID. */
                NU_BLOCK_COPY(snmp_notification->OID.notification_oid,
                              auth_trap_oid,
                              sizeof(UINT32) * SNMP_TRAP_OID_LEN);

                snmp_notification->OID.oid_len = SNMP_TRAP_OID_LEN;

                /* The notification is ready to be sent. */
                if (SNMP_Notification_Ready(snmp_notification) != NU_SUCCESS)
                {
                    NLOG_Error_Log("SNMP: Failed to send notification",
                                    NERR_SEVERE, __FILE__, __LINE__);
                }
            }
        }
    }

    return (status);

} /* CBSM_Verify */

/************************************************************************
*
*   FUNCTION
*
*       CBSM_Get_Community_Name
*
*   DESCRIPTION
*
*       This function takes a security name, traverses the community table
*       and copies the community name matching that security upon success.
*       The calling function is responsible for allocating enough memory
*       for the out parameter community name.
*
*   INPUTS
*
*       *security_name              security name
*       *community_name             community name out parameter. Upon
*                                   success, this field will contain the
*                                   community string matching the security.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       SNMP_ERROR
*
*************************************************************************/
STATUS CBSM_Get_Community_Name(UINT8 *security_name, UINT8 *community_name)
{
    STATUS                  status = SNMP_ERROR;
    CBSM_COMMUNITY_STRUCT   *node;

    /* Search the community table. */
    node = Cbsm_Mib_Root.next;

    while(node != NU_NULL)
    {
        /* If the security name is equal and the row is active. */
        if(
#if (INCLUDE_MIB_CBSM == NU_TRUE)
            (node->cbsm_status == SNMP_ROW_ACTIVE) &&
#endif
           (strcmp((CHAR*)node->cbsm_security_name,
                   (CHAR*) security_name) == 0))
        {
            strcpy((CHAR *)community_name,
                   (CHAR *)node->cbsm_community_name);

            status = NU_SUCCESS;
            break;
        }

        node = node->next;
    }

    return (status);

} /* CBSM_Get_Community_Name */

/************************************************************************
*
*   FUNCTION
*
*       CBSM_Add_Community
*
*   DESCRIPTION
*
*       This function adds a new community to the table.
*
*   INPUTS
*
*       *node                   Information about the new community to
*                               be added
*
*   OUTPUTS
*
*       NU_SUCCESS           Community successfully added.
*       SNMP_BAD_PARAMETER   Invalid argument
*       SNMP_WARNING         The community already exists.
*       SNMP_NO_MEMORY       MEmory allocation failed.
*
*************************************************************************/
STATUS CBSM_Add_Community(CBSM_COMMUNITY_STRUCT *node)
{
    CBSM_COMMUNITY_STRUCT       *temp = NU_NULL;
    CBSM_COMMUNITY_STRUCT       *new_node;
    INT32                       cmp;
    STATUS                      status = NU_SUCCESS;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */
    if ( (node != NU_NULL) &&
#if (INCLUDE_MIB_CBSM == NU_TRUE)
         (node->cbsm_status >= SNMP_ROW_ACTIVE) &&
         (node->cbsm_status <= SNMP_ROW_DESTROY) &&
         ( ( (node->cbsm_status != SNMP_ROW_ACTIVE) &&
             (node->cbsm_status != SNMP_ROW_NOTINSERVICE) ) ||
           ( (node->cbsm_storage_type >= SNMP_STORAGE_OTHER) &&
             (node->cbsm_storage_type <= SNMP_STORAGE_READONLY) &&
#else       
         ( (
#endif
           (((strlen((CHAR *)(node->cbsm_security_name)) != 0) &&
             ((strlen((CHAR *)(node->cbsm_community_index))) ==
                                        node->cbsm_community_index_len) &&
              (node->cbsm_community_index_len != 0) )) ) ) )
    {
        /* Get a pointer to the root of the list. */
        temp = Cbsm_Mib_Root.next;

        /* Find the location where the root is to be inserted. */
        while(temp != NU_NULL)
        {
            /* If the element has a target name which is
               lexicographically greater then we need to insert the new
               node before this. */
            if((cmp = (memcmp(temp->cbsm_community_index,
                              node->cbsm_community_index,
                              ((temp->cbsm_community_index_len >
                                node->cbsm_community_index_len) ?
                                  node->cbsm_community_index_len :
                                  temp->cbsm_community_index_len)))) >= 0)
            {
                /* If both the community indexes were equal i.e. their
                 * elements matched and their lengths were equal, then the
                 * node is already present and we do not need to insert it.
                 */
                if(cmp == 0 && node->cbsm_community_index_len ==
                    temp->cbsm_community_index_len)
                {
                    status = SNMP_WARNING;
                    break;
                }

                /* Otherwise, if the new community index is lexicographically
                 * lesser than the current one or it matched above but
                 * it's length was shorter, then it should be inserted
                 * before the current node.
                 */
                else if(cmp > 0 || (node->cbsm_community_index_len <
                    temp->cbsm_community_index_len) )
                    break;
            }

            temp = temp->next;
        }
    }

    else
    {
        status = SNMP_BAD_PARAMETER;
    }

    /* Insert the node at the appropriate location. */
    if(status == NU_SUCCESS)
    {
        /* Allocate memory for the new node. */
        if ((NU_Allocate_Memory(&System_Memory,
                                         (VOID**)&(new_node),
                                         sizeof(CBSM_COMMUNITY_STRUCT),
                                         NU_NO_SUSPEND)) == NU_SUCCESS)
        {
            new_node = TLS_Normalize_Ptr(new_node);

            /* Copy value to the new node. */
            NU_BLOCK_COPY(new_node, node, sizeof(CBSM_COMMUNITY_STRUCT));

            /* If temp is NU_NULL that means we need to enqueue at
               the end of the list. */
            if(temp == NU_NULL)
            {
                DLL_Enqueue(&Cbsm_Mib_Root, new_node);
            }
            else
            {
                /* Otherwise, insert before temp. */
                DLL_Insert(&Cbsm_Mib_Root, new_node, temp);
            }
        }
        else
        {
            NLOG_Error_Log("SNMP: Failed to allocate memory.",
                            NERR_SEVERE, __FILE__, __LINE__);
            status = SNMP_NO_MEMORY;
        }
    }

    NU_USER_MODE();    /* return to user mode */
    return (status);

} /* CBSM_Add_Community */

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)
/************************************************************************
*
*   FUNCTION
*
*       CBSM_Compare_Index
*
*   DESCRIPTION
*
*       This function is used to compare the indices of the CBSM
*       table entries. This function is being used by the file
*       component.
*
*   INPUTS
*
*       *left_side                      Pointer to a CBSM entry.
*       *right_side                     Pointer to a CBSM entry.
*
*   OUTPUTS
*
*       0                               Both the entries are equal.
*       -1                              The entries are not equal.
*
*************************************************************************/
INT32 CBSM_Compare_Index(VOID *left_side, VOID *right_side)
{
    CBSM_COMMUNITY_STRUCT   *left_ptr = left_side;
    CBSM_COMMUNITY_STRUCT   *right_ptr = right_side;
    INT32                   cmp = -1;

    /* Compare the community index length and community index. This forms
     * the index for the CBSM table.
     */
    if ((left_ptr->cbsm_community_index_len ==
         right_ptr->cbsm_community_index_len) &&
        (memcmp(left_ptr->cbsm_community_index,
                right_ptr->cbsm_community_index,
                left_ptr->cbsm_community_index_len) == 0) )
    {
        cmp = 0;
    }

    return (cmp);

} /* CBSM_Compare_Index */

/************************************************************************
*
*   FUNCTION
*
*       CBSM_Save_Community
*
*   DESCRIPTION
*
*       This function saves the new state of community to the file.
*
*   INPUTS
*
*       *community              The community whose changed state is to
*                               be saved
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation successful.
*
*************************************************************************/
STATUS CBSM_Save_Community(CBSM_COMMUNITY_STRUCT *community)
{
    /* This variable will be used by the file component to temporarily
     * store information read from file.
     */
    CBSM_COMMUNITY_STRUCT   read_community;
    STATUS                  status;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* Call the function that performs the actual logic. */
    status = SNMP_Save(
        Cbsm_Mib_Root.next,
        community,
        &read_community,
        SNMP_MEMBER_OFFSET(CBSM_COMMUNITY_STRUCT, cbsm_storage_type),
        SNMP_MEMBER_OFFSET(CBSM_COMMUNITY_STRUCT, cbsm_status),
        sizeof(CBSM_COMMUNITY_STRUCT),
        CBSM_FILE,
        CBSM_Compare_Index,
        INCLUDE_MIB_CBSM);

    NU_USER_MODE();    /* return to user mode */
    return (status);

} /* CBSM_Save_Community */

#endif /* (SNMP_ENABLE_FILE_STORAGE == NU_TRUE) */
#endif /* ((INCLUDE_SNMPv1 == NU_TRUE) || (INCLUDE_SNMPv2 == NU_TRUE)) */



