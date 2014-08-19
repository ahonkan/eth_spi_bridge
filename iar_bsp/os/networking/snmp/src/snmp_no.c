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
*       snmp_no.c                                                
*
*   DESCRIPTION
*
*       This file contains functions used for the Notification Originator
*       component.
*
*   DATA STRUCTURES
*
*       Snmp_Notification_List
*       Snmp_Notification_Status
*       Snmp_Notify_Table
*       Snmp_Profile_Table
*       Snmp_Filter_Table
*
*   FUNCTIONS
*
*       Notification_Init
*       SNMP_Notification_Task_Entry
*       SNMP_Prepare_Notification
*       SNMP_Compare_Tag
*       SNMP_Find_Target_Params
*       SNMP_Get_Notification_Ptr
*       SNMP_Retrieve_Notification
*       SNMP_Notification_Ready
*       SNMP_Prepare_Object_List
*       SNMP_Notification_Config
*       SNMP_Add_To_Notify_Tbl
*       SNMP_Add_To_Profile_Tbl
*       SNMP_Add_To_Filter_Tbl
*       SNMP_Add_Target
*       SNMP_Add_Params
*       SNMP_Subtree_Compare
*       SNMP_Notification_Filtering
*
*   DEPENDENCIES
*
*       nu_net.h
*       snmp_cfg.h
*       snmp.h
*       agent.h
*       snmp_no.h
*       no_api.h
*       snmp_g.h
*       snmp_utl.h
*       mib.h
*       1213sys.h
*       cbsm.h
*
************************************************************************/

#include "networking/nu_net.h"

#include "networking/snmp_cfg.h"
#include "networking/snmp.h"
#include "networking/agent.h"
#include "networking/snmp_no.h"
#include "networking/no_api.h"
#include "networking/snmp_g.h"
#include "networking/snmp_utl.h"
#include "networking/mib.h"
#include "networking/1213sys.h"
#include "networking/tgr_mib.h"

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)
#include "networking/snmp_file.h"
#endif

#if ((INCLUDE_SNMPv1 == NU_TRUE) || (INCLUDE_SNMPv2 == NU_TRUE))
#include "networking/cbsm.h"
#endif

extern NU_TASK                                  Snmp_Notification_Task;
extern SNMP_TARGET_MIB                          Snmp_Target_Mib;
extern SNMP_TARGET_ADDR_TABLE                   Snmp_Cfg_Tgr_Addr_Tbl[];
extern SNMP_TARGET_PARAMS_TABLE_CONFIG          Snmp_Cfg_Tgr_Params_Tbl[];
extern SNMP_NOTIFY_TABLE_CONFIG                 Snmp_Cfg_Notify_Tbl[];
extern SNMP_NOTIFY_FILTER_PROFILE_TABLE_CONFIG  Snmp_Cfg_Fltr_Prof_Tbl[];
extern SNMP_NOTIFY_FILTER_TABLE_CONFIG          Snmp_Cfg_Fltr_Tbl[];
extern NU_MEMORY_POOL                           System_Memory;
extern NU_EVENT_GROUP                           Snmp_Events;

SNMP_NOTIFY_REQ_LIST                            Snmp_Notification_List = {NU_NULL, NU_NULL};

STATIC UINT8        Snmp_Notification_Status = SNMP_MODULE_NOTSTARTED;

SNMP_NOTIFY_TABLE_ROOT                          Snmp_Notify_Table;
SNMP_PROFILE_TABLE_ROOT                         Snmp_Profile_Table;
SNMP_FILTER_TABLE_ROOT                          Snmp_Filter_Table;

/************************************************************************
*
*   FUNCTION
*
*       Notification_Init
*
*   DESCRIPTION
*
*       This function initializes the Notification Tables.
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
STATUS Notification_Init(VOID)
{
    STATUS                status;

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)

    /* Structures to which data will be read from files. */
    SNMP_TARGET_ADDRESS_TABLE target_addr;
    SNMP_TARGET_PARAMS_TABLE target_params;
    SNMP_READ_FILE      addr_table_information;
    SNMP_READ_FILE      params_table_information;

    /* File name, Structure pointer, Sizeof structure, Insert
     * function.
     */
    addr_table_information.snmp_file_name       = TARGET_ADDR_FILE;
    addr_table_information.snmp_read_pointer    =    NU_NULL;
    addr_table_information.snmp_insert_func     =
        (SNMP_ADD_MIB_ENTRY)SNMP_Add_Target;
    addr_table_information.snmp_sizeof_struct   =
        sizeof(SNMP_TARGET_ADDRESS_TABLE);

    params_table_information.snmp_file_name     = TARGET_PARAMS_FILE;
    params_table_information.snmp_read_pointer  = NU_NULL;
    params_table_information.snmp_insert_func   =
        (SNMP_ADD_MIB_ENTRY)SNMP_Add_Params;
    params_table_information.snmp_sizeof_struct =
        sizeof(SNMP_TARGET_PARAMS_TABLE);

    /* Assign a location that will be used to read data from file. */
    addr_table_information.snmp_read_pointer = &target_addr;
    params_table_information.snmp_read_pointer = &target_params;

    /* Read the address table from file. */
    status = SNMP_Read_File(&addr_table_information);

    /* If we successfully read data from file. */
    if(status == NU_SUCCESS)
    {
        status = SNMP_Read_File(&params_table_information);

        if(status == NU_SUCCESS)
        {
            /* Notification Module successfully initialized. */
            Snmp_Notification_Status = SNMP_MODULE_INITIALIZED;
        }
    }

#else
    /* If file storage is not enabled then we cannot initialize from it. */
    status = SNMP_ERROR;
#endif

    if (status != NU_SUCCESS)
    {
        Snmp_Notification_Status = SNMP_MODULE_NOTINITIALIZED;
    }

    return (status);

} /* Notification_Init */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Notification_Task_Entry
*
*   DESCRIPTION
*
*       This function is the entry point for Notification Originator
*       task. This task is suspended, it resumes when asked to send
*       send notifications.
*
*   INPUTS
*
*       argc                    Unused parameter.
*       argv                    Unused parameter.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
VOID SNMP_Notification_Task_Entry(UNSIGNED argc, VOID *argv)
{
    UINT32                  sysUpTimeOID[] = {1, 3, 6, 1, 2, 1, 1, 3, 0};
    UINT32                  sysUpTimeOIDLen = 9;
    UINT32                  snmpTrapOID[] = {1, 3, 6, 1, 6, 3, 1, 1, 4, 1,
                                             0};
    UINT32                  snmpTrapOIDLen = 11;

    /* Pointer to the notification session. */
    SNMP_SESSION_STRUCT     *snmp_notification_session;

    /* Buffer used to send notifications. */
    UINT8                   *snmp_notification_buffer;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    UNUSED_PARAMETER(argc);
    UNUSED_PARAMETER(argv);

    /* Initialize the notification buffer, which will be used to send
     * notifications.
     */
    if(NU_Allocate_Memory(&System_Memory,
                          (VOID **)&snmp_notification_buffer,
                          (SNMP_BUFSIZE + SNMP_CIPHER_PAD_SIZE),
                          NU_NO_SUSPEND) != NU_SUCCESS)
    {
        /* This is a critical error and we cannot continue. Log an error.
         */
         NLOG_Error_Log("Unable to allocate memory for notification "
                        "buffer", NERR_SEVERE, __FILE__, __LINE__);

        /* Suspend Task */
        NU_Suspend_Task(&Snmp_Notification_Task);
    }

    snmp_notification_buffer =
                TLS_Normalize_Ptr(snmp_notification_buffer);

    /* Allocate memory for the session structure, when ever a notification
     * is being sent, it needs a session, this structure will be used.
     */
    if(NU_Allocate_Memory(&System_Memory,
                          (VOID **)&snmp_notification_session,
                          sizeof(SNMP_SESSION_STRUCT),
                          NU_NO_SUSPEND) != NU_SUCCESS)
    {
         /* This is a critical error and we cannot continue. Log an error.
          */
         NLOG_Error_Log("Unable to allocate memory for notification"
                        " session", NERR_SEVERE, __FILE__, __LINE__);

        /* Suspend Task */
        NU_Suspend_Task(&Snmp_Notification_Task);
    }

    snmp_notification_session =
                            TLS_Normalize_Ptr(snmp_notification_session);

    /* Clear the session structure. */
    UTL_Zero(snmp_notification_session, sizeof(SNMP_SESSION_STRUCT));

    /* There are two objects that will always be part of an SNMP
     * notification: sysUpTime and snmpTrapOID. We set these
     * two OIDs here to prevent from setting them for each
     * request.
     */

    /* sysUpTime. */
    NU_BLOCK_COPY(snmp_notification_session->snmp_object_list[0].Id,
                    sysUpTimeOID, sysUpTimeOIDLen * sizeof(UINT32));

    snmp_notification_session->snmp_object_list[0].IdLen =
                                                    sysUpTimeOIDLen;

    /* snmpTrapOID. */
    NU_BLOCK_COPY(snmp_notification_session->snmp_object_list[1].Id,
                    snmpTrapOID, snmpTrapOIDLen * sizeof(UINT32));

    snmp_notification_session->snmp_object_list[1].IdLen = snmpTrapOIDLen;

    /* Type for snmpTrapOID. This is required because we do not get the
     * value for this object. Type for the other objects will be
     * automatically updated, when their value is retrieved.
     */
    snmp_notification_session->snmp_object_list[1].Type = SNMP_OBJECTID;

    /* Will wait for the notifications to be added to the queue */
    for (;;)
    {
        /* Get a notification request. We will suspend if there are no
         * pending requests. When a request is later received, the task
         * will be resumed.
         */
        if (SNMP_Retrieve_Notification(snmp_notification_session)
                                                            != NU_SUCCESS)
            break;

        /* This function is called for preparing notifications */
        SNMP_Prepare_Notification(snmp_notification_session,
                                  snmp_notification_buffer);
    }

    /* Deallocate the response buffer. */
    if (NU_Deallocate_Memory(snmp_notification_buffer) != NU_SUCCESS)
    {
        NLOG_Error_Log("SNMP: Failed to deallocate memory", NERR_SEVERE,
                        __FILE__, __LINE__);
    }

    /* Deallocate the session structure. */
    if (NU_Deallocate_Memory(snmp_notification_session) != NU_SUCCESS)
    {
        NLOG_Error_Log("SNMP: Failed to deallocate memory",
                        NERR_SEVERE, __FILE__, __LINE__);
    }

    /* Suspend Task */
    NU_Suspend_Task(&Snmp_Notification_Task);

    NU_USER_MODE(); /* we only need this statement to avoid compile
                     * warning.
                     */
} /* SNMP_Notification_Task_Entry */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Prepare_Notification
*
*   DESCRIPTION
*
*       This function prepares a notification and then sends it. It
*       determines the parameters for the notification and the targets
*       to which the notification will be sent. SNMP message is then
*       created and sent to the required targets.
*
*   INPUTS
*
*       *notification       Pointer to a structure which defines the
*                           notification to be sent.
*
*   OUTPUTS
*
*       NU_SUCCESS          Operation was successful (errors are logged).
*
*************************************************************************/
STATUS SNMP_Prepare_Notification(SNMP_SESSION_STRUCT *notification,
                                 UINT8 *snmp_notification_buffer)
{
    /* Pointer used to traverse the notify table. */
    SNMP_NOTIFY_TABLE           *notify_entry = Snmp_Notify_Table.flink;

    /* Pointer used to traverse the target address table. */
    SNMP_TARGET_ADDRESS_TABLE   *target_entry;

    /* Pointer to an entry used to determine the parameters for the
     * notification PDU.
     */
    SNMP_TARGET_PARAMS_TABLE    *params_entry;

    /* Pointers to entries used to determine the notification filter
     * criteria.
     */
    SNMP_NOTIFY_FILTER_PROFILE_TABLE *params_filter_entry;

    SNMP_MESSAGE_STRUCT         snmp_notification_message;

    /* ID counter used to identify each notification. */
    static UINT32               request_id = 0;

    /* Socket identifier for sending notifications. */
    INT                     socket;

    /* Status of the request. */
    STATUS                      status;

    /* The port and transport domain for all notifications are constant
     * for now. Therefore, we set these outside the loop, to prevent
     * doing this for each iteration. If a port number is specified in the
     * target address table entry, we will change it later.
     */
    snmp_notification_message.snmp_transport_address.port =
                        SNMP_TRAP_PORT;

    /* Transport domain is UDP. */
    snmp_notification_message.snmp_transport_domain = SNMP_UDP;

    /* Go through all the entries in the notify table, and send a trap
     * to all the targets that are associated with each entry in the
     * notify table.
     */
    while (notify_entry != NU_NULL)
    {
        /* Go through all the entries in the target address table,
         * and determine targets to send notification to.
         */
        target_entry = Snmp_Target_Mib.target_addr_table.next;

        while(target_entry != NU_NULL)
        {
            /* Using the tag value in the notify entry and the tag
             * list in the target entry determine whether a notification
             * should be sent to this target. If the tag list contains
             * the tag value, this function will return true.
             */
            if(SNMP_Compare_Tag(notify_entry->snmp_notify_tag,
                                notify_entry->tag_len,
                                target_entry->snmp_target_addr_tag_list,
                                target_entry->tag_list_len)
                                == NU_TRUE)
            {
                /* Set the status to success for this target until
                 * we are proved otherwise.
                 */
                status = NU_SUCCESS;

                /* We need to send a notification to this target.  Now
                 * determine the parameters to use for sending the
                 * notification
                 */
                params_entry = SNMP_Find_Target_Params(
                            (CHAR *)target_entry->snmp_target_addr_params,
                            Snmp_Target_Mib.target_params_table.next);

                /* If we found a params entry. */
                if(params_entry != NU_NULL)
                {
                    /* Fill in the notification. */

                    /* Message model. */
                    notification->snmp_mp =
                        params_entry->snmp_target_params_mp_model;

                    /* Security model. */
                    notification->snmp_sm =
                        params_entry->snmp_target_params_security_model;

                    /* Security name. */
                    strcpy((CHAR *)notification->snmp_security_name,
                          params_entry->snmp_target_params_security_name);

                    /* Security level. */
                    notification->snmp_security_level =
                        (UINT8)params_entry->
                                snmp_target_params_security_level;

                    /* Request ID for this notification. */
                    notification->snmp_pdu.Request.Id = ++request_id;

                    /* Notification context engine id and it's length */
                    SNMP_Get_Engine_ID(notification->snmp_context_engine_id,
                        &(notification->snmp_context_engine_id_len));

                    /* Reset snmp_buffer in snmp_response */
                    snmp_notification_message.snmp_buffer =
                                                snmp_notification_buffer;

                    /* Copy the IP address and family for the target. */
                    snmp_notification_message.snmp_transport_address.
                        family = target_entry->snmp_target_addr_tfamily;

                    NU_BLOCK_COPY(&(snmp_notification_message.
                                    snmp_transport_address.id),
                                  target_entry->snmp_target_addr_tAddress,
                                  SNMP_MAX_IP_ADDRS);

                    /* If a valid port number is specified in the target
                     * address table for traps, then we change the default
                     * trap port to this port number.
                     */
                    if(target_entry->snmp_target_addr_portnumber > 0)
                    {
                        snmp_notification_message.snmp_transport_address.
                            port = target_entry->snmp_target_addr_portnumber;
                    }

                    /* Now find a notification filter that is associated
                     * with this params entry.
                     */

                    /* Use params name to find the filter name. */
                    params_filter_entry = SNMP_Find_Profile_Entry(
                          (UINT8 *)params_entry->snmp_target_params_name);

                    /* Apply all filters which correspond to the
                     * filter name found.
                     */
                    if(params_filter_entry != NU_NULL)
                    {
                        status = SNMP_Notification_Filtering(notification,
                                        params_filter_entry->
                                         snmp_notify_filter_profile_name);
                    }

                    /* If the notification passed the filter. */
                    if(status == NU_SUCCESS)
                    {
                        /* Prepare the object list. */
                        status = SNMP_Prepare_Object_List(
                                                    notification);
                    }

                    if(status == NU_SUCCESS)
                    {
                        /* Create a socket to send notifications. */
                        socket = NU_Socket(NU_FAMILY_IP, NU_TYPE_DGRAM,
                                           0);

                        if (socket >= 0)
                        {
                            /* now send notification */
                            SNMP_Send_Notification(
                                &snmp_notification_message,
                                notification,
                                socket);

                            NU_Close_Socket(socket);
                        }
                        else
                        {
                            /* Log an error. */
                            NLOG_Error_Log(
                               "Socket error, unable to send notification"
                               " to one of the targets",
                               NERR_INFORMATIONAL, __FILE__, __LINE__);
                        }
                    }
                    else
                    {
                        /* Log an error. */
                        NLOG_Error_Log(
                           "Unable to send notification because of access"
                           " restrictions",
                            NERR_INFORMATIONAL, __FILE__, __LINE__);
                    }

                }

                /* Otherwise, we did not find a params entry. */
                else
                {
                    /* Log an error. */
                    NLOG_Error_Log("SNMP parameters not found for target",
                                  NERR_INFORMATIONAL, __FILE__, __LINE__);
                }

            }

            /* Move to the next entry in the target address table. */
            target_entry = target_entry->next;

        } /* while(target_entry != NU_NULL) */

        /* Move to the next entry in the Notify table. */
        notify_entry = notify_entry->flink;

    } /* while(notify_entry != NU_NULL)*/

    /* We always return success. */
    return (NU_SUCCESS);

} /* SNMP_Prepare_Notification */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Compare_Tag
*
*   DESCRIPTION
*
*       This function finds an occurrence of tag in tagList.
*
*   INPUTS
*
*       *tag                Tag we are looking for in the list.
*       tag_len             Length of the tag.
*       *tag_list           Tag list in which we will look for the tag.
*       tag_list_len        Length of the tag list.
*
*   OUTPUTS
*
*       NU_TRUE             Tag exists in the tag list.
*       NU_FALSE            Tag does not exist in the tag list.
*
*************************************************************************/
UINT8 SNMP_Compare_Tag(UINT8 *tag, UINT32 tag_len, const UINT8 *tag_list,
                       UINT32 tag_list_len)
{
    /* Temporary variables used in for loop. */
    UINT32              i;

    /* Variable which indicates whether the current tag in tag_list is a
     * candidate.
     */
    UINT8               candidate = NU_TRUE;

    /* Variable that indicates that the tag was found. */
    UINT8               found = NU_FALSE;

    /* Variable that will be used to traverse the tag. */
    UINT8               *temp_tag = tag;

    for(i = 0; i < tag_list_len; i++)
    {
        /* If we encountered a delimiter for the current tag in list. */
        if((*tag_list == 0x20) || (*tag_list == 0x09) ||
           (*tag_list == 0x0D) || (*tag_list == 0x0A))
        {
            /* If this was a candidate in consideration and the tag has
             * also exhausted its length then we have a match.
             */
            if((candidate == NU_TRUE) && (temp_tag == tag + tag_len))
            {
                found = NU_TRUE;
                break;
            }
            else
            {
                /* This may mean the start of a new tag and a new
                 * candidate.
                 */
                candidate = NU_TRUE;
                temp_tag = tag;
            }
        }

        /* Otherwise, if the current tag is a candidate, then check with
         * the tag in the list.
         */
        else if((candidate == NU_TRUE) &&
                (*temp_tag != *tag_list))
        {
            candidate = NU_FALSE;
        }

        /* Otherwise, if the current tag is a candidate and the character
         * compares with the one in tag, move to the next character.
         */
        else if(candidate == NU_TRUE)
        {
            temp_tag++;
        }

        /* Go to the next character in the tag list. */
        tag_list++;
    }

    /* This condition checks if we broke out of the loop because the
     * tag list length was exhausted. If this is the case and the
     * current tag was a candidate and the tag had also been
     * exhausted then we have found a match.
     */
    if((i == tag_list_len) &&
       (candidate == NU_TRUE) &&
       (temp_tag == tag + tag_len))
    {
        found = NU_TRUE;
    }

    return (found);

} /* SNMP_Compare_Tag */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Find_Target_Params
*
*   DESCRIPTION
*
*       Finds an entry in the parameters table which has parameters name
*       and length as specified by the passed arguments.
*
*   INPUTS
*
*       *params_name            Name of the parameters entry.
*       *params_tbl             Pointer to the list of parameters.
*
*   OUTPUTS
*
*       A valid pointer if an entry was found, NU_NULL otherwise.
*
*************************************************************************/
SNMP_TARGET_PARAMS_TABLE *SNMP_Find_Target_Params(
                                  const CHAR *params_name,
                                  SNMP_TARGET_PARAMS_TABLE *params_tbl)
{
    /* Go through the parameters list till we find what we are looking
     * for.
     */
    while(params_tbl != NU_NULL)
    {
        /* If the names match, break out of the loop and return this
         * entry.
         */
        if(strcmp(params_name, params_tbl->snmp_target_params_name) == 0)
            break;

        /* Otherwise, go to the next entry in list. */
        params_tbl = params_tbl->next;
    }

    return (params_tbl);
} /* SNMP_Find_Target_Params */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Get_Notification_Ptr
*
*   DESCRIPTION
*
*       Applications, which want Notification Originator to generate
*       notifications, use this function to get a pointer to
*       SNMP_NOTIFY_REQ_STRUCT structures.
*
*   INPUTS
*
*       **notification      A pointer to the notification request.
*
*   OUTPUTS
*
*       NU_SUCCESS          Pointer was successfully obtained.
*       SNMP_NO_MEMORY      Unable to obtain a pointer for the
*                           notification request.
*
*************************************************************************/
STATUS SNMP_Get_Notification_Ptr(SNMP_NOTIFY_REQ_STRUCT **notification)
{
    STATUS          status = NU_SUCCESS;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    if(NU_Allocate_Memory(&System_Memory, (VOID **)notification,
            sizeof(SNMP_NOTIFY_REQ_STRUCT), NU_NO_SUSPEND) != NU_SUCCESS)
    {
        /* Unable to allocate memory for request. */
        status = SNMP_NO_MEMORY;

        /* Log an error. */
         NLOG_Error_Log("Unable to allocate memory for notification "
                        "request", NERR_SEVERE, __FILE__, __LINE__);
    }
    else
    {
        (*notification) = TLS_Normalize_Ptr(*notification);

        /* Clear memory that was just allocated. */
        UTL_Zero(*notification, sizeof(SNMP_NOTIFY_REQ_STRUCT));
    }

    NU_USER_MODE();    /* switch back to user mode */
    return (status);
} /* SNMP_Get_Notification_Ptr */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Retrieve_Notification
*
*   DESCRIPTION
*
*       This function retrieves a notification request from the
*       notifications queue and fills the passed in function with the
*       appropriate values.
*
*   INPUTS
*
*       *notification           Information about the notification to be
*                               sent.
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation was successful.
*
*************************************************************************/
STATUS SNMP_Retrieve_Notification (SNMP_SESSION_STRUCT *notification)
{
    STATUS                  status = SNMP_ERROR;
    UNSIGNED                retrieved_events;
    SNMP_NOTIFY_REQ_STRUCT  *notification_req;

    /* While there are still requests in the notification request list.
     */
    while(status != NU_SUCCESS)
    {
        /* If there are pending requests in the list. */
        if (Snmp_Notification_List.snmp_flink != NU_NULL)
        {
            /* Dequeue the first notification request. */
            notification_req = DLL_Dequeue(&Snmp_Notification_List);

            /* Get the notification OID. */
            notification->snmp_object_list[1].SyntaxLen =
                            notification_req->OID.oid_len;

            NU_BLOCK_COPY(notification->snmp_object_list[1].Syntax.BufInt,
                            notification_req->OID.notification_oid,
                            sizeof(UINT32) *
                             notification->snmp_object_list[1].SyntaxLen);

            /* Get the objects that will be sent in the notification. */
            notification->snmp_object_list_len = 2 +
                notification_req->snmp_object_list_len;

            NU_BLOCK_COPY(&notification->snmp_object_list[2],
                notification_req->snmp_object_list,
                sizeof(snmp_object_t) *
                                    notification->snmp_object_list_len);

            /* Deallocate the notification structure request. */
            if (NU_Deallocate_Memory(notification_req) != NU_SUCCESS)
            {
                NLOG_Error_Log("SNMP: Failed to deallocate memory",
                                NERR_SEVERE, __FILE__, __LINE__);
            }

            /* We found an entry. */
            status = NU_SUCCESS;
        }
        else
        {   /* Wait for an event to indicate that a request has arrived.
             */
            if (NU_Retrieve_Events(&Snmp_Events, SNMP_NOTIFICATION_ARRIVED,
                            NU_OR_CONSUME, &retrieved_events, NU_SUSPEND)
                                                            != NU_SUCCESS)
            {
                NLOG_Error_Log("SNMP: Failed to retrieve event",
                                NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

    return (status);

} /* SNMP_Retrieve_Notification */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Notification_Ready
*
*   DESCRIPTION
*
*       This function tells the notification originator that the
*       notification is ready.
*
*   INPUTS
*
*       *notification       A pointer to the notification which is to be
*                           sent.
*
*   OUTPUTS
*
*       NU_SUCCESS             Operation was successful.
*       SNMP_ERROR             The event for notification arrival
*                              could not be set.
*       SNMP_BAD_PARAMETER Null pointer passed in. 
*
*************************************************************************/
STATUS SNMP_Notification_Ready(SNMP_NOTIFY_REQ_STRUCT *notification)
{
    STATUS          status = NU_SUCCESS;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    if (notification != NU_NULL)
    {
        /* Enqueue this request at the end of the list. */
        DLL_Enqueue(&Snmp_Notification_List, notification);

        /* Set the event only if the notification originator is up
         * and running.
         */
        if(Snmp_Notification_Status == SNMP_MODULE_INITIALIZED)
        {
            if(NU_Set_Events(&Snmp_Events, SNMP_NOTIFICATION_ARRIVED,
                                   NU_OR)!=NU_SUCCESS)
            {
                status = SNMP_ERROR;
            }
        }
  
    }

    else
    {
        status = SNMP_BAD_PARAMETER;
    }

    /* switch back to user mode */
    NU_USER_MODE();
    return (status);

} /* SNMP_Notification_Ready */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Prepare_Object_List
*
*   DESCRIPTION
*
*       This function will check access of each of the objects in the list
*       using VACM. The values are retrieved through the MIB database.
*
*   INPUTS
*
*       *notification           Pointer to the notification session.
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation was successful.
*       SNMP_ERROR              Error in retrieving object values.
*
*************************************************************************/
STATUS SNMP_Prepare_Object_List(SNMP_SESSION_STRUCT *notification)
{
    /* Status of the request. */
    STATUS                  status = NU_SUCCESS;
    UINT32                  i;
    INT16                   last_index;

#if ((INCLUDE_SNMPv1 == NU_TRUE) || (INCLUDE_SNMPv2 == NU_TRUE))
    UINT8                   community_name[SNMP_SIZE_SMALLOBJECTID] = {0};

    /* If we are using CBSM, then we will have to find community name. */
    if((notification->snmp_sm == 1) || (notification->snmp_sm == 2))
    {
        /* Determine the community name using security name. */
        if(CBSM_Get_Community_Name(notification->snmp_security_name,
            community_name) != NU_SUCCESS)
        {
            return (SNMP_ERROR);
        }
    }

#endif

#if (INCLUDE_SNMPv1 == NU_TRUE)

    /* If this is an SNMPv1 Notification. */
    if(notification->snmp_mp == SNMP_VERSION_V1)
    {
        for (i = 2; i < notification->snmp_object_list_len; i++)
        {
            /* This is a v1 Trap request. */
            notification->snmp_object_list[i].Request = SNMP_PDU_TRAP_V1;

            /* Make a request for this object. */
            status = Request(&notification->snmp_object_list[i],
                             (UINT16 *)&last_index, notification->snmp_sm,
                             notification->snmp_security_level,
                             notification->snmp_security_name,
                             notification->snmp_context_name);

            /* Make sure the request was successful. */
            if(status != NU_SUCCESS)
                break;
        }
    }

#endif

#if ((INCLUDE_SNMPv1 == NU_TRUE) && \
     ((INCLUDE_SNMPv2 == NU_TRUE) || (INCLUDE_SNMPv3 == NU_TRUE)))
    /* Otherwise, if this is an SNMPv2 or SNMPv3 Notification. */
    else
#endif

#if ((INCLUDE_SNMPv2 == NU_TRUE) || (INCLUDE_SNMPv3 == NU_TRUE))
    if((notification->snmp_mp == SNMP_VERSION_V2) ||
       (notification->snmp_mp == SNMP_VERSION_V3))
    {
        for (i = 0; i < notification->snmp_object_list_len; i++)
        {
            /* This is a v2 Trap request. */
            notification->snmp_object_list[i].Request = SNMP_PDU_TRAP_V2;

            /* If this is the notification OID, just check that the
             * user has access for this OID.
             */
            if(i == 1)
            {
                status = VACM_CheckInstance_Access(
                          notification->snmp_sm,
                          notification->snmp_security_level,
                          notification->snmp_security_name,
                          notification->snmp_context_name,
                          VACM_NOTIFY_VIEW,
                          notification->snmp_object_list[i].Syntax.BufInt,
                          notification->snmp_object_list[i].SyntaxLen);
            }
            else
            {
                /* Make a request for this object. */
                status = MIB_Request_V2(
                            &notification->snmp_object_list[i],
                            (UINT16 *)&last_index, notification->snmp_sm,
                            notification->snmp_security_level,
                            notification->snmp_security_name,
                            notification->snmp_context_name);

                /* Make sure that the object was found. */
                if((notification->snmp_object_list[i].Type ==
                                                SNMP_NOSUCHINSTANCE) ||
                   (notification->snmp_object_list[i].Type ==
                                                SNMP_NOSUCHOBJECT))
                {
                    status = SNMP_ERROR;
                }
            }

            /* Make sure the request was successful. */
            if(status != NU_SUCCESS)
                break;

        }
    }
#endif

#if ((INCLUDE_SNMPv1 == NU_TRUE) || (INCLUDE_SNMPv2 == NU_TRUE))

    /* Since we are done with all security name checks, so now we copy
     * the previously found community name into the security name field.
     * The reason is, because the generated notification should contain
     * community name instead of security name. This is done in case of
     * using CBSM only.
     */
    if((notification->snmp_sm == 1) || (notification->snmp_sm == 2))
    {
        strcpy((CHAR *)notification->snmp_security_name, (CHAR *)community_name);
    }

#endif

    return (status);
} /* SNMP_Prepare_Object_List */

/************************************************************************
*
*   FUNCTION
*
*       SNMP_Notification_Config
*
*   DESCRIPTION
*
*       This function configures the Notification Module.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       NU_SUCCESS      The configuration was successful.
*
*************************************************************************/
STATUS SNMP_Notification_Config(VOID)
{
    STATUS                              status = NU_SUCCESS;
    SNMP_TARGET_ADDRESS_TABLE           tgr_node;
    SNMP_TARGET_PARAMS_TABLE            param_node;
    SNMP_NOTIFY_TABLE                   notify_node;
    SNMP_NOTIFY_FILTER_PROFILE_TABLE    profile_node;
    SNMP_NOTIFY_FILTER_TABLE            filter_node;
    UINT8                               i;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* Wait for Notification Module to be initialized. */
    while(Snmp_Notification_Status == SNMP_MODULE_NOTSTARTED)
        /* Sleep for a second. */
        NU_Sleep(NU_PLUS_Ticks_Per_Second);

    /* Check whether the initialization has succeeded. */
    if (Snmp_Notification_Status == SNMP_MODULE_NOTINITIALIZED)
    {
        /* Initialize the target address table.
         * Add the entries according to the data structure defined in
         * snmp_cfg.c:
         */

        /* Clear the memory. */
        UTL_Zero(&tgr_node, sizeof(SNMP_TARGET_ADDRESS_TABLE));

        for(i = 0; i < TGR_ADDR_TBL_SIZE; i++)
        {
            /* Host name. */
            strcpy(tgr_node.snmp_target_addr_name,
                Snmp_Cfg_Tgr_Addr_Tbl[i].snmp_target_addr_name);

            /* IP Family. */
            tgr_node.snmp_target_addr_tfamily =
                (INT16)(Snmp_Cfg_Tgr_Addr_Tbl[i].snmp_target_addr_tfamily);

            /* IP Address. */
            NU_BLOCK_COPY(tgr_node.snmp_target_addr_tAddress,
                Snmp_Cfg_Tgr_Addr_Tbl[i].snmp_target_addr_tAddress,
                SNMP_MAX_IP_ADDRS);

            /* Port Number. */
            tgr_node.snmp_target_addr_portnumber =
                Snmp_Cfg_Tgr_Addr_Tbl[i].snmp_target_addr_port_num;

            /* Parameters. */
            tgr_node.params_len =
                strlen(Snmp_Cfg_Tgr_Addr_Tbl[i].snmp_target_addr_params);

            NU_BLOCK_COPY(tgr_node.snmp_target_addr_params,
                Snmp_Cfg_Tgr_Addr_Tbl[i].snmp_target_addr_params,
                tgr_node.params_len);
#if (INCLUDE_MIB_TARGET == NU_TRUE)

            /* Storage Type. */
            tgr_node.snmp_target_addr_storage_type = SNMP_STORAGE_DEFAULT;

            /* Row Status. */
            tgr_node.snmp_target_addr_row_status = SNMP_ROW_ACTIVE;
#endif
            /* Domain. */
            tgr_node.snmp_target_addr_tDomain =
                Snmp_Cfg_Tgr_Addr_Tbl[i].snmp_target_addr_tDomain;

            /* Tag list. */
            tgr_node.tag_list_len = Snmp_Cfg_Tgr_Addr_Tbl[i].tag_list_len;
            NU_BLOCK_COPY(tgr_node.snmp_target_addr_tag_list,
                Snmp_Cfg_Tgr_Addr_Tbl[i].snmp_target_addr_tag_list,
                tgr_node.tag_list_len);

            /* Add the entry. */
            SNMP_Add_Target(&tgr_node);
        }

        /* Initialize the target params table. */
        /* Clear the memory. */
        UTL_Zero(&param_node, sizeof(SNMP_TARGET_PARAMS_TABLE));

#if (INCLUDE_MIB_TARGET == NU_TRUE)
        /* Storage Type. */
        param_node.snmp_target_params_storage_type = SNMP_STORAGE_READONLY;

        /* Row Status. */
        param_node.snmp_target_params_row_status = SNMP_ROW_ACTIVE;

#endif

        for(i = 0; i < TGR_PARAMS_TBL_SIZE; i++)
        {
            /* Target Params Name. */
            strcpy(param_node.snmp_target_params_name,
                (CHAR *)Snmp_Cfg_Tgr_Params_Tbl[i].snmp_params_name);

            /* MP Model*/
            param_node.snmp_target_params_mp_model =
                Snmp_Cfg_Tgr_Params_Tbl[i].snmp_mp_model;

            /* Security Model */
            param_node.snmp_target_params_security_model =
                Snmp_Cfg_Tgr_Params_Tbl[i].snmp_security_model;

            /* Security Name */
            strcpy(param_node.snmp_target_params_security_name,
                (CHAR *)Snmp_Cfg_Tgr_Params_Tbl[i].snmp_security_name);

            /* Security Level */
            param_node.snmp_target_params_security_level =
                Snmp_Cfg_Tgr_Params_Tbl[i].snmp_security_level;

            /* Add entry. */
            SNMP_Add_Params(&param_node);

        } /* end of for loop */

/* If Target MIB is included in build along with file saving */
#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)
#if (INCLUDE_MIB_TARGET == NU_TRUE)
        if (status == NU_SUCCESS)
        {
            /* Create the file. */
            status = SNMP_Create_File(TARGET_ADDR_FILE);

            if (status == NU_SUCCESS)
            {
                /* Save the configurations to file. */
                status = Save_Target_Addr(Snmp_Target_Mib.target_addr_table.next);

                if (status == NU_SUCCESS)
                {
                    status = SNMP_Create_File(TARGET_PARAMS_FILE);

                    if (status == NU_SUCCESS)
                    {
                        status = Save_Target_Params(
                            Snmp_Target_Mib.target_params_table.next);
                    }
                }
            }
        }
#endif /* INCLUDE_MIB_TARGET == NU_TRUE */
#endif /* (SNMP_ENABLE_FILE_STORAGE == NU_TRUE) */

        if (status == NU_SUCCESS)
        {
            Snmp_Notification_Status = SNMP_MODULE_INITIALIZED;
        }

    } /* If (Snmp_Notification_Status == SNMP_MODULE_NOTINITIALIZED) */

    /* Initialize the Notify table. */
    /* Clear the memory. */
    UTL_Zero(&notify_node, sizeof(SNMP_NOTIFY_TABLE));

    /* Type. */
    notify_node.snmp_notify_type = TRAP;

#if (INCLUDE_MIB_NO == NU_TRUE)
    /* Storage Type. */
    notify_node.snmp_notify_storage_type = SNMP_STORAGE_READONLY;

    /* Row Status. */
    notify_node.snmp_notify_row_status = SNMP_ROW_ACTIVE;

#endif

    for(i = 0; i < NOTIFY_TBL_SIZE; i++)
    {
        /* Notify Name. */
        strcpy(notify_node.snmp_notify_name,
               (CHAR *)Snmp_Cfg_Notify_Tbl[i].snmp_table_name);

        /* Tag Length */
        notify_node.tag_len =
                    Snmp_Cfg_Notify_Tbl[i].snmp_target_tag_length;

        /* Target Tag Name */
        NU_BLOCK_COPY(notify_node.snmp_notify_tag,
                      Snmp_Cfg_Notify_Tbl[i].snmp_target_tag_name,
                      notify_node.tag_len);

        /* Add entry. */
        SNMP_Add_To_Notify_Tbl(&notify_node);

    } /* end of for loop */

    /* Initialize the Filter Profile Table. */
    /* Clear the memory. */
    UTL_Zero(&profile_node, sizeof(SNMP_NOTIFY_FILTER_PROFILE_TABLE));

#if (INCLUDE_MIB_NO == NU_TRUE)
    /* Storage Type. */
    profile_node.snmp_notify_filter_profile_storType =
                                                    SNMP_STORAGE_READONLY;

    /* Row Status. */
    profile_node.snmp_notify_filter_profile_row_status = SNMP_ROW_ACTIVE;

#endif

    for(i = 0; i < FLTR_PROF_TBL_SIZE; i++)
    {
        /* Params Name. */
        strcpy(profile_node.snmp_target_params_name,
               (CHAR *)Snmp_Cfg_Fltr_Prof_Tbl[i].snmp_params_name);

        /* Filter Profile Name. */
        strcpy(profile_node.snmp_notify_filter_profile_name,
               (CHAR *)Snmp_Cfg_Fltr_Prof_Tbl[i].snmp_filter_name);

        /* Add entry. */
        SNMP_Add_To_Profile_Tbl(&profile_node);

    } /* end of for loop */

    /* Initialize the Filter Table. */
    /* Clear the memory. */
    UTL_Zero(&filter_node, sizeof(SNMP_NOTIFY_FILTER_TABLE));

#if (INCLUDE_MIB_NO == NU_TRUE)
    /* Storage Type. */
    filter_node.snmp_notify_filter_storage_type = SNMP_STORAGE_READONLY;

    /* Row Status. */
    filter_node.snmp_notify_filter_row_status = SNMP_ROW_ACTIVE;

#endif

    for(i = 0; i < FLTR_TBL_SIZE; i++)
    {
        /* Filter Name. */
        strcpy(filter_node.snmp_notify_filter_profile_name,
               (CHAR *)Snmp_Cfg_Fltr_Tbl[i].snmp_filter_name);

        /* Subtree Length */
        filter_node.subtree_len = Snmp_Cfg_Fltr_Tbl[i].snmp_subtree_len;

        /* Subtree. */
        NU_BLOCK_COPY(filter_node.snmp_notify_filter_subtree,
                      Snmp_Cfg_Fltr_Tbl[i].snmp_notify_filter_subtree,
                      filter_node.subtree_len * sizeof(UINT32));

        /* Mask Length */
        filter_node.mask_len =
                            (UINT32) (Snmp_Cfg_Fltr_Tbl[i].snmp_mask_len);

        /* Filter Mask. */
        NU_BLOCK_COPY(filter_node.snmp_notify_filter_mask,
                      Snmp_Cfg_Fltr_Tbl[i].snmp_notify_filter_mask,
                      Snmp_Cfg_Fltr_Tbl[i].snmp_mask_len);

        /* Filter Type. */
        filter_node.snmp_notify_filter_type =
                            (INT32)Snmp_Cfg_Fltr_Tbl[i].snmp_filter_type;

        /* Add the entry to the list. */
        SNMP_Add_To_Filter_Tbl(&filter_node);

    } /* end of for loop */

    /* return to user mode */
    NU_USER_MODE();
    return (status);

} /* SNMP_Notification_Config */

/***********************************************************************
*
*   Function
*
*       SNMP_Add_To_Notify_Tbl
*
*   DESCRIPTION
*
*       Inserts a node to the Notify Table.
*
*   INPUTS
*
*       *node               Node which will be added
*
*   OUTPUTS
*
*       NU_SUCCESS             Successful operation.
*       SNMP_WARNING           Node already exists.
*       SNMP_NO_MEMORY         Memory allocation failed
*       SNMP_BAD_PARAMETER Invalid argument
*
*************************************************************************/
STATUS SNMP_Add_To_Notify_Tbl(SNMP_NOTIFY_TABLE *node)
{
    SNMP_NOTIFY_TABLE           *temp = NU_NULL;
    SNMP_NOTIFY_TABLE           *new_node;
    INT32                       cmp;
    STATUS                      status = NU_SUCCESS;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    if ( (node != NU_NULL) && (
#if (INCLUDE_MIB_NO == NU_TRUE)
          ( (node->snmp_notify_row_status != SNMP_ROW_ACTIVE) &&
            (node->snmp_notify_row_status != SNMP_ROW_NOTINSERVICE) ) ||
#endif
         ((strlen(node->snmp_notify_name) != 0) &&
          (node->snmp_notify_type >= 1) &&
          (node->snmp_notify_type <= 2)) )
#if (INCLUDE_MIB_NO == NU_TRUE)
         && (node->snmp_notify_row_status >= SNMP_ROW_ACTIVE) &&
         (node->snmp_notify_row_status <= SNMP_ROW_DESTROY) &&
         (node->snmp_notify_storage_type >= SNMP_STORAGE_OTHER) &&
         (node->snmp_notify_storage_type <= SNMP_STORAGE_READONLY)
#endif
       )
    {
        /* Get a pointer to the root of the list. */
        temp = Snmp_Notify_Table.flink;

        /* Find the location where the root is to be inserted. */
        while(temp != NU_NULL)
        {
            /* If the element has a notify name which is
               lexicographically greater then we need to insert the new
               node before this. */
            if((cmp = (strcmp(temp->snmp_notify_name,
                              node->snmp_notify_name))) >= 0)
            {
                /* If both the strings were equal, then the node is
                 * already present and we do not need to insert it. */
                if(cmp == 0)
                {
                    status = SNMP_WARNING;
                }

                break;
            }

            temp = temp->flink;
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
        if ((status = NU_Allocate_Memory(&System_Memory,
                                         (VOID **)&new_node,
                                         sizeof(SNMP_NOTIFY_TABLE),
                                         NU_NO_SUSPEND)) == NU_SUCCESS)
        {
            new_node = TLS_Normalize_Ptr(new_node);

            /* Copy value to the new node. */
            NU_BLOCK_COPY(new_node, node, sizeof(SNMP_NOTIFY_TABLE));

            /* If temp is NU_NULL that means we need to enqueue at
               the end of the list. */
            if(temp == NU_NULL)
            {
                DLL_Enqueue(&Snmp_Notify_Table, new_node);
            }
            else
            {
                /* Otherwise, insert before temp. */
                DLL_Insert(&Snmp_Notify_Table, new_node, temp);
            }
        }
        else
        {
            /* Log error. */
            NLOG_Error_Log("Unable to allocate memory for a new entry",
                         NERR_SEVERE, __FILE__, __LINE__);
            status = SNMP_NO_MEMORY;
        }
    }

    /* return to user mode */
    NU_USER_MODE();
    return (status);

} /* SNMP_Add_To_Notify_Tbl */

/***********************************************************************
*
*   Function
*
*       SNMP_Add_To_Profile_Tbl
*
*   DESCRIPTION
*
*       Inserts a node to the Profile Table.
*
*   INPUTS
*
*       *node               Node which will be added
*
*   OUTPUTS
*
*       NU_SUCCESS             Successful operation.
*       SNMP_WARNING           Node already exists.
*       SNMP_NO_MEMORY         Memory allocation failed
*       SNMP_BAD_PARAMETER Invalid argument
*
*************************************************************************/
STATUS SNMP_Add_To_Profile_Tbl(SNMP_NOTIFY_FILTER_PROFILE_TABLE *node)
{
    SNMP_NOTIFY_FILTER_PROFILE_TABLE            *temp = NU_NULL;
    SNMP_NOTIFY_FILTER_PROFILE_TABLE            *new_node;
    INT32                                       cmp;
    STATUS                                      status = NU_SUCCESS;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    if ( (node != NU_NULL) &&
#if (INCLUDE_MIB_NO == NU_TRUE)
         (node->snmp_notify_filter_profile_row_status >=
                                                      SNMP_ROW_ACTIVE) &&
         (node->snmp_notify_filter_profile_row_status <=
                                                      SNMP_ROW_DESTROY) &&
         ( ( (node->snmp_notify_filter_profile_row_status !=
                                                       SNMP_ROW_ACTIVE) &&
             (node->snmp_notify_filter_profile_row_status !=
                                               SNMP_ROW_NOTINSERVICE) ) ||
           ( (node->snmp_notify_filter_profile_storType >=
                                                    SNMP_STORAGE_OTHER) &&
             (node->snmp_notify_filter_profile_storType <=
                                                 SNMP_STORAGE_READONLY) &&
#else       
         ( (
#endif
           ((strlen(node->snmp_notify_filter_profile_name) != 0)) ) ) )
    {
        /* Get a pointer to the root of the list. */
        temp = Snmp_Profile_Table.flink;

        /* Find the location where the root is to be inserted. */
        while(temp != NU_NULL)
        {
            /* If the params name is lexicographically greater then,
             * we need to insert the new before this node.
             */
            if((cmp = (strcmp(temp->snmp_target_params_name,
                              node->snmp_target_params_name))) >= 0)
            {
                /* If both the strings were equal, then the node is already
                   present and we do not need to insert it. */
                if(cmp == 0)
                {
                    status = SNMP_WARNING;
                }

                break;
            }

            temp = temp->flink;
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
        if ((status = NU_Allocate_Memory(&System_Memory,
                                (VOID **)&new_node,
                                sizeof(SNMP_NOTIFY_FILTER_PROFILE_TABLE),
                                NU_NO_SUSPEND)) == NU_SUCCESS)
        {
            new_node = TLS_Normalize_Ptr(new_node);

            /* Copy value to the new node. */
            NU_BLOCK_COPY(new_node, node,
                          sizeof(SNMP_NOTIFY_FILTER_PROFILE_TABLE));

            /* If temp is NU_NULL that means we need to enqueue at
               the end of the list. */
            if(temp == NU_NULL)
            {
                DLL_Enqueue(&Snmp_Profile_Table, new_node);
            }
            else
            {
                /* Otherwise, insert before temp. */
                DLL_Insert(&Snmp_Profile_Table, new_node, temp);
            }
        }
        else
        {
            /* Log error. */
            NLOG_Error_Log("Unable to allocate memory for a new entry",
                         NERR_SEVERE, __FILE__, __LINE__);
            status = SNMP_NO_MEMORY;
        }
    }

    /* return to user mode */
    NU_USER_MODE();
    return (status);

} /* SNMP_Add_To_Profile_Tbl */

/***********************************************************************
*
*   Function
*
*       SNMP_Add_To_Filter_Tbl
*
*   DESCRIPTION
*
*       Inserts a node to the Filter Table.
*
*   INPUTS
*
*       *node          Node which will be added
*
*   OUTPUTS
*
*       NU_SUCCESS             Node successfully added
*       SNMP_NO_MEMORY         Memory allocation failed
*       SNMP_WARNING           Node already exists
*       SNMP_BAD_PARAMETER Invalid argument
*
*************************************************************************/
STATUS SNMP_Add_To_Filter_Tbl(SNMP_NOTIFY_FILTER_TABLE *node)
{
    SNMP_NOTIFY_FILTER_TABLE    *temp = NU_NULL;
    SNMP_NOTIFY_FILTER_TABLE    *new_node;
    INT32                       cmp;
    STATUS                      status = NU_SUCCESS;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */
    
    if ( (node != NU_NULL) &&
#if (INCLUDE_MIB_NO == NU_TRUE)
         (node->snmp_notify_filter_row_status >= SNMP_ROW_ACTIVE) &&
         (node->snmp_notify_filter_row_status <= SNMP_ROW_DESTROY) &&
         ( ( (node->snmp_notify_filter_row_status != SNMP_ROW_ACTIVE) &&
             (node->snmp_notify_filter_row_status !=
                                               SNMP_ROW_NOTINSERVICE) ) ||
           ( (node->snmp_notify_filter_storage_type >=
                                                    SNMP_STORAGE_OTHER) &&
             (node->snmp_notify_filter_storage_type <=
                                                 SNMP_STORAGE_READONLY) &&
#else       
         ( (
#endif
           (( (node->mask_len <= MAX_FILTER_MASK_SZE) && 
              (node->snmp_notify_filter_type >= INCLUDED) &&
              (node->snmp_notify_filter_type <= EXCLUDED) &&
              (node->subtree_len <= SNMP_SIZE_OBJECTID) )) ) ) )
    {
        /* Get a pointer to the root of the list. */
        temp = Snmp_Filter_Table.flink;

        /* Find the location where the root is to be inserted. */
        while(temp != NU_NULL)
        {
            /* If the element has a profile name which is
               lexicographically greater or is equal, then, this may
               be where we will be inserting the new node. */
            if((cmp = (UTL_Admin_String_Cmp(
                          temp->snmp_notify_filter_profile_name,
                          node->snmp_notify_filter_profile_name))) >= 0)
            {
                /* If both the strings were equal, then we will
                 * insert the node before this if the subtree is greater.
                 */
                if(cmp == 0)
                {
                    cmp = MibCmpObjId(temp->snmp_notify_filter_subtree,
                                      temp->subtree_len,
                                      node->snmp_notify_filter_subtree,
                                      node->subtree_len);

                    /* If the comparison showed both subtrees are equal.
                     * Then we will not insert the node because it already
                     * exists.
                     */
                    if(cmp == 0)
                    {
                        status = SNMP_WARNING;
                        break;
                    }

                    /* Otherwise, if the first subtree is greater,
                     * we will insert the node here.
                     */
                    else if(cmp > 0)
                    {
                        break;
                    }
                }
                else
                {
                    /* We will insert the node here, because the next
                     * profile name is greater. */
                    break;
                }

            }

            temp = temp->flink;
        } /* end of while */
    }
    
    else
    {
        status = SNMP_BAD_PARAMETER;
    }

    /* Insert the node at the appropriate location. */
    if(status == NU_SUCCESS)
    {
        /* Allocate memory for the new node. */
        if ((status = NU_Allocate_Memory(&System_Memory,
                                         (VOID **)&new_node,
                                         sizeof(SNMP_NOTIFY_FILTER_TABLE),
                                         NU_NO_SUSPEND)) == NU_SUCCESS)
        {
            new_node = TLS_Normalize_Ptr(new_node);

            /* Copy value to the new node. */
            NU_BLOCK_COPY(new_node, node,
                          sizeof(SNMP_NOTIFY_FILTER_TABLE));

            /* Extend the mask with all ones. */
            VACM_Filling_FamilyMask(new_node->snmp_notify_filter_mask,
                                    new_node->mask_len);

            /* If temp is NU_NULL that means we need to enqueue at
               the end of the list. */
            if(temp == NU_NULL)
            {
                DLL_Enqueue(&Snmp_Filter_Table, new_node);
            }
            else
            {
                /* Otherwise, insert before temp. */
                DLL_Insert(&Snmp_Filter_Table, new_node, temp);
            }
        }
        else
        {
            /* Log error. */
            NLOG_Error_Log("Unable to allocate memory for a new entry",
                         NERR_SEVERE, __FILE__, __LINE__);
            status = SNMP_NO_MEMORY;
        }
    }

    NU_USER_MODE();    /* return to user mode */
    return (status);

} /* SNMP_Add_To_Filter_Tbl */

/***********************************************************************
*
*   Function
*
*       SNMP_Add_Target
*
*   DESCRIPTION
*
*       Adds a node in target address table.
*
*   INPUTS
*
*    *node                  Node which will be added
*
*   OUTPUTS
*
*       NU_SUCCESS             Node successfully added
*       SNMP_WARNING           Node already exists.
*       SNMP_NO_MEMORY         Memeory allocation failed
*       SNMP_BAD_PARAMETER Invalid argument
*
*************************************************************************/
STATUS SNMP_Add_Target(const SNMP_TARGET_ADDRESS_TABLE *node)
{
    SNMP_TARGET_ADDRESS_TABLE      *temp = NU_NULL;
    SNMP_TARGET_ADDRESS_TABLE      *new_node;
    INT32                          cmp;
    STATUS                         status = NU_SUCCESS;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    if ( (node != NU_NULL) &&
#if (INCLUDE_MIB_TARGET == NU_TRUE)
         (node->snmp_target_addr_row_status >= SNMP_ROW_ACTIVE) &&
         (node->snmp_target_addr_row_status <= SNMP_ROW_DESTROY) &&
         ( ( (node->snmp_target_addr_row_status != SNMP_ROW_ACTIVE) &&
             (node->snmp_target_addr_row_status !=
                                               SNMP_ROW_NOTINSERVICE) ) ||
           ( (node->snmp_target_addr_storage_type >=
                                                    SNMP_STORAGE_OTHER) &&
             (node->snmp_target_addr_storage_type <=
                                                 SNMP_STORAGE_READONLY) &&
#else       
         ( (
#endif
           ( (strlen(node->snmp_target_addr_name) != 0) &&
             (node->snmp_target_addr_retry_count >= 0) &&
             (node->snmp_target_addr_retry_count <= 255) &&
             (node->snmp_target_addr_tDomain != 0) &&
             (strlen((CHAR *)(node->snmp_target_addr_params)) != 0) &&
             (strlen((CHAR *)(node->snmp_target_addr_tAddress)) != 0) ) ) ))
    {

       /* Get a pointer to the root of the list. */
        temp = Snmp_Target_Mib.target_addr_table.next;

        /* Find the location where the root is to be inserted. */
        while(temp != NU_NULL)
        {
            /* If the element has a target name which is
               lexicographically greater then we need to insert the new
               node before this. */
            if((cmp = (strcmp(temp->snmp_target_addr_name,
                              node->snmp_target_addr_name))) >= 0)
            {
                /* If both the strings were equal, then the node is
                 * already present and we do not need to insert it.
                 */
                if(cmp == 0)
                {
                    status = SNMP_WARNING;
                }

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
        if ((status = NU_Allocate_Memory(&System_Memory,
                                        (VOID **)&new_node,
                                        sizeof(SNMP_TARGET_ADDRESS_TABLE),
                                        NU_NO_SUSPEND)) == NU_SUCCESS)
        {
            new_node = TLS_Normalize_Ptr(new_node);

            /* Copy value to the new node. */
            NU_BLOCK_COPY(new_node, node,
                          sizeof(SNMP_TARGET_ADDRESS_TABLE));

            /* Make sure that the params name is null terminated. */
            if(new_node->params_len < MAX_TARG_NAME_SZE)
            {
                new_node->snmp_target_addr_params[new_node->params_len]
                                                                   = '\0';
            }
            else
            {
                new_node->snmp_target_addr_params[MAX_TARG_NAME_SZE - 1]
                                                                   = '\0';
            }

            /* If temp is NU_NULL that means we need to enqueue at
                the end of the list. */
            if(temp == NU_NULL)
            {
                DLL_Enqueue(&Snmp_Target_Mib.target_addr_table, new_node);
            }
            else
            {
                /* Otherwise, insert before temp. */
                DLL_Insert(&Snmp_Target_Mib.target_addr_table, new_node,
                           temp);
            }

        }
        else
        {
            /* Log error. */
            NLOG_Error_Log("Unable to allocate memory for a new entry",
                         NERR_SEVERE, __FILE__, __LINE__);
            status = SNMP_NO_MEMORY;
        }
    }

    /* return to user mode */
    NU_USER_MODE();
    return (status);

} /* SNMP_Add_Target */

/***********************************************************************
*
*   Function
*
*       SNMP_Add_Params
*
*   DESCRIPTION
*
*       This function adds a new node in params table.
*
*   INPUT
*
*       *node               Node to be added.
*
*   OUTPUT
*
*       NU_SUCCESS             Node successfully added.
*       SNMP_WARNING           Node already exists.
*       SNMP_BAD_PARAMETER     Invalid node 
*       SNMP_NO_MEMORY         Memory allocation failed
*
*************************************************************************/
STATUS SNMP_Add_Params(const SNMP_TARGET_PARAMS_TABLE *node)
{
    SNMP_TARGET_PARAMS_TABLE    *temp = NU_NULL;
    SNMP_TARGET_PARAMS_TABLE    *new_node;
    INT32                       cmp;
    STATUS                      status = NU_SUCCESS;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    if ( (node != NU_NULL) &&
#if (INCLUDE_MIB_TARGET == NU_TRUE)
         (node->snmp_target_params_row_status >= SNMP_ROW_ACTIVE) &&
         (node->snmp_target_params_row_status <= SNMP_ROW_DESTROY) &&
         ( ( (node->snmp_target_params_row_status != SNMP_ROW_ACTIVE) &&
             (node->snmp_target_params_row_status !=
                                               SNMP_ROW_NOTINSERVICE) ) ||
           ( (node->snmp_target_params_storage_type >=
                                                    SNMP_STORAGE_OTHER) &&
             (node->snmp_target_params_storage_type <=
                                                 SNMP_STORAGE_READONLY) &&
#else       
         ( (
#endif
           (( (strlen(node->snmp_target_params_name) != 0) &&
              (node->snmp_target_params_security_model >= SNMP_CBSM_V1) &&
              (node->snmp_target_params_security_model <= 0x7FFFFFFF))))))
    {
        /* Get a pointer to the root of the list. */
        temp = Snmp_Target_Mib.target_params_table.next;

        /* Find the location where the root is to be inserted. */
        while(temp != NU_NULL)
        {
            /* If the element has a params name which is
               lexicographically greater then we need to insert the new
               node before this. */
            if((cmp = (strcmp(temp->snmp_target_params_name,
                              node->snmp_target_params_name))) >= 0)
            {
                /* If both the strings were equal, then the node is
                 * already present and we do not need to insert it.
                 */
                if(cmp == 0)
                {
                    status = SNMP_WARNING;
                }

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
        if ((status = NU_Allocate_Memory(&System_Memory,
                                         (VOID **)&new_node,
                                         sizeof(SNMP_TARGET_PARAMS_TABLE),
                                         NU_NO_SUSPEND)) == NU_SUCCESS)
        {
            new_node = TLS_Normalize_Ptr(new_node);

            /* Copy value to the new node. */
            NU_BLOCK_COPY(new_node, node,
                          sizeof(SNMP_TARGET_PARAMS_TABLE));

            /* If temp is NU_NULL that means we need to enqueue at
               the end of the list. */
            if(temp == NU_NULL)
            {
                DLL_Enqueue(&Snmp_Target_Mib.target_params_table,
                            new_node);
            }
            else
            {
                /* Otherwise, insert before temp. */
                DLL_Insert(&Snmp_Target_Mib.target_params_table, new_node,
                           temp);
            }
        }
        else
        {
            /* Log error. */
            NLOG_Error_Log("Unable to allocate memory for a new entry",
                         NERR_SEVERE, __FILE__, __LINE__);
            status = SNMP_NO_MEMORY;
        }
    }

    /* return to user mode */
    NU_USER_MODE();
    return (status);

} /* SNMP_Add_Params */

/***********************************************************************
*
*   Function
*
*       SNMP_Subtree_Compare
*
*   DESCRIPTION
*
*       This function compares the passed subtrees with the passed view
*       and determines whether the sub-tree is part of the view.
*
*   INPUT
*
*       *subtree            Sub tree which is to be compared with the
*                           view.
*       subtree_len         Length of the sub-tree.
*       *view_tree          The sub-tree for the view.
*       view_tree_len       Length of the sub-tree for the view.
*       *view_mask          The mask of the view.
*
*   OUTPUT
*
*       NU_SUCCESS          Sub-tree is part of the view.
*       SNMP_ERROR          Sub-tree is not part of the view.
*
*************************************************************************/
STATUS SNMP_Subtree_Compare(const UINT32 *subtree, UINT32 subtree_len,
                            const UINT32 *view_tree, UINT32 view_tree_len,
                            const UINT8 *view_mask)
{
    UINT32               i = 0;
    UINT8                bit_position = 0x80;
    STATUS               status = SNMP_ERROR;

    /* If the subtree length is less than the view length, then this view
     * will not match the passed subtree.
     */
    if(subtree_len >= view_tree_len)
    {
        /* Go through the subtree till we find a sub-identifier which does
        not match or we reach the end of one of the OIDs.*/
        for(i = 0; (i < view_tree_len); i++)
        {
            /* If we are going to the next byte of the mask. */
            if(bit_position == 0)
                bit_position = 0x80;

            /* If the mask bit is 1 for the OID sub-identifier being
             * compared, then we need to compare the sub-identifiers
             * otherwise, we do not need an exact match and the
             * sub-identifiers can be ignored.
             */
            if(view_mask[(i >> 3)] & bit_position)
            {
                /* Compare the OIDs. */
                if(view_tree[i] != subtree[i])
                    break;
            }

            /* Go to the next bit position. */
            bit_position = (UINT8)(bit_position >> 1);
        }
    }

    /* Is the subtree part of the view? If i is equal to sub-tree length
     * of the view, that means we did not break out of the loop because
     * of mis-matched entries. Therefore, the subtree is part of the view.
     */
    if(i == view_tree_len)
    {
        /* Yes it is. */
        status = NU_SUCCESS;
    }

    return (status);

} /* SNMP_Subtree_Compare */

/***********************************************************************
*
*   Function
*
*       SNMP_Notification_Filtering
*
*   DESCRIPTION
*
*       This function checks if access is allowed for a notification to
*       be generated.
*
*   INPUT
*
*       *notification       Pointer to notification session.
*       *filter_name        Name of filter to be applied.
*
*   OUTPUT
*
*       NU_SUCCESS          Access is allowed.
*       SNMP_ERROR          Access is not allowed.
*
*************************************************************************/
STATUS SNMP_Notification_Filtering(SNMP_SESSION_STRUCT *notification,
                                   const CHAR *filter_name)
{
    SNMP_NOTIFY_FILTER_TABLE    *temp = Snmp_Filter_Table.flink;
    STATUS                      status  = SNMP_ERROR;
    UINT32                      subtree_len = 0;
    UINT32                      i;
    INT32                       cmp;
    UINT8                       found = NU_FALSE;

    /* First check for the inclusion of the trap OID. */
    while(temp != NU_NULL)
    {
        /* If the filter names match. */
        cmp = strcmp(temp->snmp_notify_filter_profile_name, filter_name);

        if(cmp == 0)
        {
            /* We found at least one entry. */
            found = NU_TRUE;

            /* Is the notification OID part of the current view? */
            if(SNMP_Subtree_Compare(
                        notification->snmp_object_list[1].Syntax.BufInt,
                        notification->snmp_object_list[1].SyntaxLen,
                        temp->snmp_notify_filter_subtree,
                        temp->subtree_len,
                        temp->snmp_notify_filter_mask) == NU_SUCCESS)
            {
                /* Since this is the lexicographically greater view found
                 * till now, it will determine the rights assigned to the
                 * view. Just make sure that it has more identifiers than
                 * the last candidate.
                 */
                if(temp->subtree_len >= subtree_len)
                {
                    /* Update the length of the subtree used. */
                    subtree_len = temp->subtree_len;

                    if(temp->snmp_notify_filter_type == INCLUDED)
                    {
                        status = NU_SUCCESS;
                    }
                    else
                    {
                        status = SNMP_ERROR;
                    }
                }
            }
        }
        else if(cmp > 0)
        {
            /* No point in continuing any further, because the names
             * are going to get lexicographically greater.
             */
            break;
        }

        temp = temp->flink;
    }

    /* Now check that none of the OIDs have been specifically excluded. */
    if(status == NU_SUCCESS)
    {
        /* If this is SNMPv1 notification, then the first two objects
         * are not for us.
         */
        if(notification->snmp_mp == SNMP_VERSION_V1)
            i = 2;
        else
            i = 0;

        for(; i < notification->snmp_object_list_len; i++)
        {
            /* If i is 1, this is the notification OID, skip it. */
            if(i == 1)
            {
                continue;
            }

            /* Search from the start of the filter table. */
            temp = Snmp_Filter_Table.flink;
            subtree_len = 0;

            while(temp != NU_NULL)
            {
                /* If the filter names match. */
                cmp = strcmp(temp->snmp_notify_filter_profile_name,
                             filter_name);

                if(cmp == 0)
                {
                    /* We found at least one entry. */
                    found = NU_TRUE;

                    /* Is the object's OID part of the current view? */
                    if(SNMP_Subtree_Compare(
                            notification->snmp_object_list[i].Id,
                            notification->snmp_object_list[i].IdLen,
                            temp->snmp_notify_filter_subtree,
                            temp->subtree_len,
                            temp->snmp_notify_filter_mask) == NU_SUCCESS)
                    {
                        /* Since this is the lexicographically greater
                         * view found till now, it will determine the
                         * rights assigned to the view. Just make sure
                         * that it has more identifiers than the last
                         * candidate.
                         */
                        if(temp->subtree_len >= subtree_len)
                        {
                            /* Update the length of the subtree used. */
                            subtree_len = temp->subtree_len;

                            if(temp->snmp_notify_filter_type == EXCLUDED)
                            {
                                status = SNMP_ERROR;
                            }
                            else
                            {
                                status = NU_SUCCESS;
                            }
                        }
                    }
                }
                else if(cmp > 0)
                {
                    /* No point in continuing any further, because the
                     * names are going to get lexicographically greater.
                     */
                    break;
                }

                temp = temp->flink;
            }

            /* If the current object was excluded. Then we will not be
             * sending the notification so break out of the loop.
             */
            if(status == SNMP_ERROR)
            {
                break;
            }

        }
    }

    /* If we did not find any entry, then we should return success.
     * because filtering is not required in this case.
     */
    if(found == NU_FALSE)
        status = NU_SUCCESS;

    return (status);

} /* SNMP_Notification_Filtering */



