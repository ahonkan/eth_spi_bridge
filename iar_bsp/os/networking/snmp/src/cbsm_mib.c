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
*       cbsm_mib.c                                               
*
*   COMPONENT
*
*       CBSM MIBs
*
*   DESCRIPTION
*
*       This file implements the CBSM MIB as defined in RFC2576.
*
*   DATA STRUCTURES
*
*       Temp_Cbsm_Mib_Root
*       CBSM_MIB_Commit_Left
*
*   FUNCTIONS
*
*       Get_SNMP_CBSMEntry_Util
*       Get_SNMP_CBSMEntry
*       Get_snmpCommunityEntry
*       Create_snmpCommunityEntry
*       Commit_snmpCommunityEntryStatus
*       Commit_snmpCommunityEntries
*       Undo_snmpCommunityEntry
*       Set_snmpCommunityEntry
*       snmpCommunityEntry
*
*   DEPENDENCIES
*
*       agent.h
*       cbsm.h
*       snmp_g.h
*
*************************************************************************/
#include "networking/agent.h"
#include "networking/cbsm.h"
#include "networking/snmp_g.h"
#include "networking/mib.h"

#if (INCLUDE_MIB_CBSM == NU_TRUE)

/* Object Identifier sub-length. Table entry OID is of length 10
 * having snmpCommunityIndex as index this results in sub-length of 11.
 */
#define SNMP_CBSM_MIB_SUB_LEN           11
#define SNMP_CBSM_MIB_ATTR_OFFSET       10
#define SNMP_CBSM_MIB_MIN(a,b) (((a) < (b)) ? (a) : (b))

STATIC CBSM_COMMUNITY_TABLE     Temp_Cbsm_Mib_Root;
STATIC UINT32                   CBSM_MIB_Commit_Left;
extern CBSM_COMMUNITY_TABLE     Cbsm_Mib_Root;
extern SNMP_ENGINE_STRUCT       Snmp_Engine;

STATIC CBSM_COMMUNITY_STRUCT *Get_SNMP_CBSMEntry_Util(
         const UINT8 *community_index, const CBSM_COMMUNITY_TABLE * root);

STATIC CBSM_COMMUNITY_STRUCT *Get_SNMP_CBSMEntry(
                             const UINT8 *community_index, UINT8 getlfag);
STATIC UINT16 Get_snmpCommunityEntry(snmp_object_t *obj, UINT8 getflag);
STATIC UINT16 Create_snmpCommunityEntry(const snmp_object_t *obj);
STATIC UINT16 Commit_snmpCommunityEntryStatus(
                        CBSM_COMMUNITY_STRUCT *cbsm_community_ptr,
                        UINT8 row_status, UINT8 is_new);
STATIC VOID Commit_snmpCommunityEntries(VOID);
STATIC UINT16 Undo_snmpCommunityEntry(const snmp_object_t *obj);
STATIC UINT16 Set_snmpCommunityEntry(const snmp_object_t *obj);

/*************************************************************************
*
*   FUNCTION
*
*       Get_SNMP_CBSMEntry_Util
*
*   DESCRIPTION
*
*       This function is used to get the handle to the CBSM structure by
*       specifying the community index and list of CBSM structures.
*
*   INPUTS
*
*       *community_index        A pointer to the location where community
*                               index is stored.
*       *root                   The pointer to the root of list containing
*                               CBSM structures.
*
*   OUTPUTS
*
*       NU_NULL                 When fail to get handle to CBSM structure
*                               with community index passed in.
*       CBSM_COMMUNITY_STRUCT * When handle to CBSM structure found with
*                               community index passed in.
*
*************************************************************************/
STATIC CBSM_COMMUNITY_STRUCT *Get_SNMP_CBSMEntry_Util(
           const UINT8 *community_index, const CBSM_COMMUNITY_TABLE *root)
{
    /* Handle to the CBSM structure. */
    CBSM_COMMUNITY_STRUCT *cbsm_community_ptr = root->next;

    /* Loop to find the CBSM structure with community index passed in. */
    while (cbsm_community_ptr)
    {
        /* Comparing community index of current CBSM structure with value
         * passed in. If we reached at the exact match then break through
         * the loop.
         */
        if (strncmp((CHAR *)cbsm_community_ptr->cbsm_community_index,
                    (CHAR *)community_index, SNMP_SIZE_SMALLOBJECTID)== 0)
        {
            /* Break through the loop. */
            break;
        }

        /* Moving forward in the list. */
        cbsm_community_ptr = cbsm_community_ptr->next;
    }

    /* Return handle to the CBSM structure if found, otherwise return
     * NU_NULL.
     */
    return (cbsm_community_ptr);

} /* Get_SNMP_CBSMEntry_Util */

/*************************************************************************
*
*   FUNCTION
*
*       Get_SNMP_CBSMEntry
*
*   DESCRIPTION
*
*       This function is used to get the handle to the CBSM structure by
*       specifying the community index and list of CBSM structures.
*
*   INPUTS
*
*       *community_index        A pointer to the location where community
*                               index is stored.
*       getflag                 The flag to distinguish between GET and
*                               GET-NEXT request.
*
*   OUTPUTS
*
*       NU_NULL                 When fail to get handle to CBSM structure
*                               with community index passed in.
*       CBSM_COMMUNITY_STRUCT * When handle to CBSM structure found with
*                               community index passed in.
*
*************************************************************************/
STATIC CBSM_COMMUNITY_STRUCT *Get_SNMP_CBSMEntry(
                              const UINT8 *community_index, UINT8 getlfag)
{
    /* Handle to the CBSM structure. */
    CBSM_COMMUNITY_STRUCT *cbsm_community_ptr;

    /* If we are handling GET request. */
    if (getlfag)
    {
        /* Try to get the CBSM structure handle from permanent list. */
        cbsm_community_ptr = Get_SNMP_CBSMEntry_Util(community_index,
                                                     &Cbsm_Mib_Root);

        /* If CBSM structure handle was not found in permanent list. */
        if (!cbsm_community_ptr)
        {
            /* Try to get the CBSM structure handle from temporary list.
             */
            cbsm_community_ptr = Get_SNMP_CBSMEntry_Util(community_index,
                                                     &Temp_Cbsm_Mib_Root);
        }
    }

    /* If we are handling GET-NEXT request. */
    else
    {
        /* Start traversing the permanent list. */
        cbsm_community_ptr = Cbsm_Mib_Root.next;

        /* Loop to find the next CBSM structure based on community index
         * passed in.
         */
        while (cbsm_community_ptr)
        {
            /* If we have reached at a CBSM structure with greater
             * community index then break through the loop.
             */
            if (memcmp(cbsm_community_ptr->cbsm_community_index,
                        community_index, SNMP_SIZE_SMALLOBJECTID) > 0)
            {
                /* Break through the loop. */
                break;
            }

            /* Moving forward in the list. */
            cbsm_community_ptr = cbsm_community_ptr->next;
        }
    }

    /* Return the handle to the CBSM structure, if found. Otherwise return
     * NU_NULL.
     */
    return (cbsm_community_ptr);

} /* Get_SNMP_CBSMEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Get_snmpCommunityEntry
*
*   DESCRIPTION
*
*       This function is used to handle SNMP Get request on
*       snmpCommunityEntry.
*
*   INPUTS
*
*       *obj                    SNMP object containing request.
*       getflag                 The flag to distinguish between GET and
*                               GET-NEXT requests.
*
*   OUTPUTS
*
*       SNMP_NOSUCHINSTANCE     When object identifier refers to a
*                               non-existing CBSM structure.
*       SNMP_NOSUCHNAME         When object identifier refers to a
*                               non-existing or non-accessible attribute.
*       SNMP_NOERROR            When successful.
*
*************************************************************************/
STATIC UINT16 Get_snmpCommunityEntry(snmp_object_t *obj, UINT8 getflag)
{
    /* Object identifier of table entry. */
    UINT32      table_oid[] = {1, 3, 6, 1, 6, 3, 18, 1, 1, 1};

    /* Handle to snmpCommunityEntry. */
    CBSM_COMMUNITY_STRUCT *cbsm_community_ptr = NU_NULL;

    /* Community Index. */
    UINT8       community_index[SNMP_SIZE_SMALLOBJECTID];

    /* Status to return success or error code. */
    UINT16      status = SNMP_NOERROR;

    /* Variable to use in for-loop. */
    UINT32      i;

    /* Clear out the community index. */
    UTL_Zero(community_index, sizeof(community_index));

    /* Get the value of community index from the OID. */
    for (i = 0;
         (i < SNMP_SIZE_SMALLOBJECTID) &&
         ((i + SNMP_CBSM_MIB_SUB_LEN) < obj->IdLen);
         i++)
     {
        community_index[i] = (UINT8)(obj->Id[SNMP_CBSM_MIB_SUB_LEN + i]);
     }


    /* If we are having GET request. */
    if (getflag)
    {
        /* If we don't have valid community index then return error code.
         */
        if ( ((i + SNMP_CBSM_MIB_SUB_LEN) != obj->IdLen) ||
             (i == 0) || (community_index[i - 1] == 0) )
        {
            /* Returning error code. */
            status = SNMP_NOSUCHINSTANCE;
        }
    }

    /* If we are handling GET-NEXT request make community index, null
     * terminating.
     */
    else if (i < SNMP_SIZE_SMALLOBJECTID)
    {
        community_index[i] = NU_NULL;
    }

    /* If no error till now then get the handle to the CBSM structure. */
    if (status == SNMP_NOERROR)
    {
        /* Getting handle to the CBSM structure. */
        cbsm_community_ptr = Get_SNMP_CBSMEntry(community_index, getflag);
    }

    /* If we got the handle to the CBSM structure. */
    if (cbsm_community_ptr)
    {
        switch(obj->Id[SNMP_CBSM_MIB_ATTR_OFFSET])
        {
        case 2:                         /* snmpCommunityName */

            /* Get Syntax length. */
            obj->SyntaxLen = strlen((CHAR *)(cbsm_community_ptr->
                                                    cbsm_community_name));

            /* Get the value of 'snmpCommunityName'. */
            NU_BLOCK_COPY(obj->Syntax.BufChr,
                          cbsm_community_ptr->cbsm_community_name,
                          obj->SyntaxLen);

            break;

        case 3:                         /* snmpCommunitySecurityName */

            /* Get the syntax length. */
            obj->SyntaxLen = strlen((CHAR *)(cbsm_community_ptr->
                                                    cbsm_security_name));

            /* Get the value of 'snmpCommunitySecurityName'. */
            NU_BLOCK_COPY(obj->Syntax.BufChr,
                          cbsm_community_ptr->cbsm_security_name,
                          obj->SyntaxLen);

            break;

        case 4:                         /* snmpCommunityContextEngineID */

            /* Get the syntax length. */
            obj->SyntaxLen = cbsm_community_ptr->cbsm_engine_id_len;

            /* Get the value of 'snmpCommunityContextEngineID'. */
            NU_BLOCK_COPY(obj->Syntax.BufChr,
                          cbsm_community_ptr->cbsm_engine_id,
                          obj->SyntaxLen);

            break;

        case 5:                         /* snmpCommunityContextName */

            /* Get the syntax length. */
            obj->SyntaxLen = strlen((CHAR *)(cbsm_community_ptr->
                                                cbsm_context_name));

            /* Get the value of 'snmpCommunityContextName'. */
            NU_BLOCK_COPY(obj->Syntax.BufChr,
                          cbsm_community_ptr->cbsm_context_name,
                          obj->SyntaxLen);

            break;

        case 6:                         /* snmpCommunityTransportTag */

            /* Get the syntax length. */
            obj->SyntaxLen = cbsm_community_ptr->cbsm_transport_tag_len;

            /* Get the value of 'snmpCommunityTransportTag'. */
            NU_BLOCK_COPY(obj->Syntax.BufChr,
                          cbsm_community_ptr->cbsm_transport_tag,
                          obj->SyntaxLen);

            break;

        case 7:                         /* snmpCommunityStorageType */

            /* Get the value of 'snmpCommunityStorageType'. */
            obj->Syntax.LngUns =
                        (UINT32)(cbsm_community_ptr->cbsm_storage_type);

            break;

        case 8:                         /* snmpCommunityStatus */

            /* Get the value of 'snmpCommunityStatus'. */
            obj->Syntax.LngUns =
                        ((UINT32)(cbsm_community_ptr->cbsm_status));

            break;

        default:                        /* We have reached at end of the
                                         * table.
                                         */

            /* Return error code. */
            status = SNMP_NOSUCHNAME;
        }

        /* Update OID if we are handling GET-NEXT request. */
        if ((status == SNMP_NOERROR) && (!getflag))
        {
            /* Update the table OID. */
            NU_BLOCK_COPY(obj->Id, table_oid, sizeof(table_oid));

            /* Update index field of OID. */
            for (i = 0 ; (i < SNMP_SIZE_SMALLOBJECTID); i++)
            {
                obj->Id[SNMP_CBSM_MIB_SUB_LEN + i] =
                    (UINT32)(cbsm_community_ptr->cbsm_community_index[i]);

                if (cbsm_community_ptr->cbsm_community_index[i] == 0)
                    break;
            }

            /* Update OID length. */
            obj->IdLen = (SNMP_CBSM_MIB_SUB_LEN + i);
        }
    }

    /* If we did not get the handle to the CBSM structure then return
     * error code.
     */
    else
    {
        /* Returning error code. */
        status = SNMP_NOSUCHINSTANCE;
    }

    /* Return success or error code. */
    return (status);

} /* Get_snmpCommunityEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Create_snmpCommunityEntry
*
*   DESCRIPTION
*
*       This function is used to handle SNMP Get request on
*       snmpCommunityEntry.
*
*   INPUTS
*
*       *obj                    SNMP object containing request.
*
*   OUTPUTS
*
*       SNMP_NOSUCHINSTANCE     When we have invalid OID.
*       SNMP_GENERROR           When object already exist or memory
*                               allocation failed.
*       SNMP_NOERROR            When successful.
*
*************************************************************************/
STATIC UINT16 Create_snmpCommunityEntry(const snmp_object_t *obj)
{
    /* Handle to snmpCommunityEntry. */
    CBSM_COMMUNITY_STRUCT *cbsm_community_ptr = NU_NULL;

    /* Handle to find the proper place in the list. */
    CBSM_COMMUNITY_STRUCT *locator;

    /* Community Index. */
    UINT8       community_index[SNMP_SIZE_SMALLOBJECTID];

    /* Status to return success or error code. */
    UINT16      status;

    /* Variable to hold the comparison result. */
    INT         cmp_result;

    /* Variable to use in for-loop. */
    UINT32      i;

    /* Clear out the value of community index. */
    UTL_Zero(community_index, sizeof(community_index));

    /* Get the value of community index from the object identifier. */
    for (i = 0;
         (i < SNMP_SIZE_SMALLOBJECTID) &&
         ((i + SNMP_CBSM_MIB_SUB_LEN) < obj->IdLen);
         i++)
     {
        community_index[i] = (UINT8)(obj->Id[SNMP_CBSM_MIB_SUB_LEN + i]);
     }


    /* If we have valid value for community index. */
    if ( ((i + SNMP_CBSM_MIB_SUB_LEN) == obj->IdLen) &&
         (i > 0) && (community_index[i - 1] != 0) )
    {
        /* Get the handle of CBSM structure if exists. */
        cbsm_community_ptr = Get_SNMP_CBSMEntry(community_index, NU_TRUE);

        /* If we don't currently have CBSM structure. */
        if (!cbsm_community_ptr)
        {
            /* Allocate the memory for new CBSM structure. */
            if (NU_Allocate_Memory(&System_Memory,
                                  (VOID **)&cbsm_community_ptr,
                                  sizeof(CBSM_COMMUNITY_STRUCT),
                                  NU_NO_SUSPEND) == NU_SUCCESS)
            {
                cbsm_community_ptr =
                                    TLS_Normalize_Ptr(cbsm_community_ptr);

                /* Clear out the newly allocated CBSM structure. */
                UTL_Zero(cbsm_community_ptr,
                         sizeof(CBSM_COMMUNITY_STRUCT)),

                /* Set the length of community index. */
                cbsm_community_ptr->cbsm_community_index_len = ((UINT8)i);

                /* Set the value of community index. */
                NU_BLOCK_COPY(cbsm_community_ptr->cbsm_community_index,
                              community_index, i);

                /* Set the value to community engine ID. */
                NU_BLOCK_COPY(cbsm_community_ptr->cbsm_engine_id,
                              Snmp_Engine.snmp_engine_id,
                              Snmp_Engine.snmp_engine_id_len);

                /* Set value to community engine ID length. */
                cbsm_community_ptr->cbsm_engine_id_len =
                                (UINT8)(Snmp_Engine.snmp_engine_id_len);

                /* Set the storage type to non-volatile. */
                cbsm_community_ptr->cbsm_storage_type =
                                            SNMP_STORAGE_DEFAULT;

                /* Set the status to 'createAndWait'. */
                cbsm_community_ptr->cbsm_status = SNMP_ROW_CREATEANDWAIT;

                /* Get the starting handle to the list to find the proper
                 * position of new CBSM structure.
                 */
                locator = Temp_Cbsm_Mib_Root.next;

                /* Loop to find the proper position. */
                while (locator)
                {
                    /* Compare the community indexes. */
                    cmp_result =
                        memcmp(locator->cbsm_community_index,
                          cbsm_community_ptr->cbsm_community_index,
                          SNMP_CBSM_MIB_MIN(
                           locator->cbsm_community_index_len,
                           cbsm_community_ptr->cbsm_community_index_len));

                    /* If we have reached at the proper position then
                     * break through the loop.
                     */
                    if (cmp_result > 0)
                    {
                        /* Breaking through the loop. */
                        break;
                    }

                    /* Moving forward in the list. */
                    locator = locator->next;
                }

                /* Add CBSM structure to proper position. */
                if (locator)
                {
                    DLL_Insert(&Temp_Cbsm_Mib_Root, cbsm_community_ptr,
                               locator);
                }
                else
                {
                    DLL_Enqueue(&Temp_Cbsm_Mib_Root, cbsm_community_ptr);
                }

                /* Return success code. */
                status = SNMP_NOERROR;
            }

            /* If memory allocation failed then return error code. */
            else
            {
                NLOG_Error_Log("SNMP: Failed to allocate memory",
                                NERR_SEVERE, __FILE__, __LINE__);

                status = SNMP_GENERROR;
            }
        }

        /* If CBSM structure already exists the return error code. */
        else
        {
            status = SNMP_GENERROR;
        }
    }

    /* If we have invalid OID then return error code. */
    else
    {
        status = SNMP_NOSUCHINSTANCE;
    }

    /* Return success or error code. */
    return (status);

} /* Create_snmpCommunityEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Commit_snmpCommunityEntryStatus
*
*   DESCRIPTION
*
*       This function is used to commit status of CBSM structure.
*
*   INPUTS
*
*       *cbsm_community_ptr     Pointer to CBSM structure.
*       row_status              Row status.
*       is_new                  Flag to distinguish between new and old
*                               entries.
*
*   OUTPUTS
*
*       SNMP_INCONSISTANTVALUE  When value of status is inconsistent with
*                               the current state.
*       SNMP_NOERROR            When successful.
*
*************************************************************************/
STATIC UINT16 Commit_snmpCommunityEntryStatus(
                        CBSM_COMMUNITY_STRUCT *cbsm_community_ptr,
                        UINT8 row_status, UINT8 is_new)
{
    /* Status for returning success or error code. */
    UINT16          status = SNMP_NOERROR;

    /* Temporary row status. */
    UINT8           temp_status;

    /* Buffer to check. */
    UINT8           check_buffer[SNMP_SIZE_SMALLOBJECTID];

    /* Clear out the check buffer. */
    UTL_Zero(check_buffer, sizeof(check_buffer));

    /* If community name or security name is not set then set temporary
     * status is 'notReady'. Otherwise set it to 'notInService'. */
    if ( (memcmp(check_buffer, cbsm_community_ptr->cbsm_community_name,
          SNMP_SIZE_SMALLOBJECTID) == 0) ||
         (memcmp(check_buffer, cbsm_community_ptr->cbsm_security_name,
            SNMP_SIZE_SMALLOBJECTID) == 0) )
    {
        temp_status = SNMP_ROW_NOTREADY;
    }
    else
    {
        temp_status = SNMP_ROW_NOTINSERVICE;
    }

    if((row_status != 0) &&
       (row_status != SNMP_ROW_CREATEANDGO) &&
       (row_status != SNMP_ROW_CREATEANDWAIT))
    {
        switch(row_status)
        {
        case SNMP_ROW_ACTIVE:

            /* New entries and not ready entries can't be activated.
               However, new entry can be activated by setting its
               status to CREATEANDGO. */
            if((temp_status == SNMP_ROW_NOTINSERVICE) &&
               (!is_new))
            {
                /* Activating the WEP KEY MAPPING. */
                cbsm_community_ptr->cbsm_status = SNMP_ROW_ACTIVE;
            }

            else
            {
                /* Setting status to error code of in-consistent
                   value. */
                status = SNMP_INCONSISTANTVALUE;
            }

            break;

        case SNMP_ROW_NOTINSERVICE:

            /* Row status can't be set to NOTINSERVICE for new and
               not ready entries. However row status can be set to
               NOTINSERVICE by setting row status to CREATEANDWAIT for
               new entries. */
            if((temp_status == SNMP_ROW_NOTINSERVICE) && (!is_new))
            {
                /* Setting the row status to 'NOTINSERVICE'. */
                cbsm_community_ptr->cbsm_status = SNMP_ROW_NOTINSERVICE;
            }

            else
            {
                /* Inconsistent value. */
                status = SNMP_INCONSISTANTVALUE;
            }

            break;

        case SNMP_ROW_DESTROY:

            /* Setting row status to SNMP_ROW_DESTROY. */
            cbsm_community_ptr->cbsm_status = SNMP_ROW_DESTROY;

            break;

        default:

            status = SNMP_INCONSISTANTVALUE;
        }
    }

    else
    {
        switch(row_status)
        {
            case SNMP_ROW_CREATEANDGO:

                /* Entry should be ready and either new or non-active. */
                if((temp_status == SNMP_ROW_NOTINSERVICE) &&
                  ((is_new) ||
                   (cbsm_community_ptr->cbsm_status != SNMP_ROW_ACTIVE)))
                {
                    /* Activating WEP KEY MAPPING. */
                    cbsm_community_ptr->cbsm_status = SNMP_ROW_ACTIVE;
                }

                else
                {
                    /* Setting status to error code of in-consistent
                       value. */
                    status = SNMP_INCONSISTANTVALUE;
                }

                break;

            case SNMP_ROW_CREATEANDWAIT:

                /* Entry should not be 'active'. */
                if(cbsm_community_ptr->cbsm_status != SNMP_ROW_ACTIVE)
                {
                    cbsm_community_ptr->cbsm_status = (UINT8)temp_status;
                }

                else
                {
                    /* Setting status to error code of in-consistent
                       value. */
                    status = SNMP_INCONSISTANTVALUE;
                }

                break;

            default:

                if ((cbsm_community_ptr->cbsm_row_flag == 0) ||
                    (temp_status == SNMP_ROW_NOTREADY) ||
                    (cbsm_community_ptr->cbsm_row_flag ==
                                                      SNMP_ROW_DESTROY))
                {
                    cbsm_community_ptr->cbsm_status = temp_status;
                }
                else if (cbsm_community_ptr->cbsm_row_flag ==
                                                    SNMP_ROW_CREATEANDGO)
                {
                    if (temp_status == SNMP_ROW_NOTINSERVICE)
                        cbsm_community_ptr->cbsm_status = SNMP_ROW_ACTIVE;
                    else
                        status = SNMP_INCONSISTANTVALUE;
                }
                else if (cbsm_community_ptr->cbsm_row_flag ==
                                                  SNMP_ROW_CREATEANDWAIT)
                {
                    cbsm_community_ptr->cbsm_status = temp_status;
                }
        }
    }

    /* Return success or error code. */
    return (status);

} /* Commit_snmpCommunityEntryStatus */

/*************************************************************************
*
*   FUNCTION
*
*       Commit_snmpCommunityEntries
*
*   DESCRIPTION
*
*       This function is used to commit all the newly created CBSM
*       structures.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
STATIC VOID Commit_snmpCommunityEntries(VOID)
{
    /* Handle to the CBSM structure. */
    CBSM_COMMUNITY_STRUCT *cbsm_community_ptr = Temp_Cbsm_Mib_Root.next;

    /* Loop till there exists an entry in temporary list. */
    while(cbsm_community_ptr)
    {
        /* Add community structure into permanent list. */
        if (CBSM_Add_Community(cbsm_community_ptr) != NU_SUCCESS)
        {
            NLOG_Error_Log("SNMP: Failed to Add CBSM community",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* Remove a CBSM structure from the temporary list. */
        DLL_Dequeue(&Temp_Cbsm_Mib_Root);

        /* Deallocate memory of CBSM structure as CBSM_Add_Community
         * creates its own copy.
         */
        if (NU_Deallocate_Memory(cbsm_community_ptr) != NU_SUCCESS)
        {
            NLOG_Error_Log("SNMP: Failed to deallocate memory",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* Getting next handle to the CBSM structure from the temporary
         * list.
         */
        cbsm_community_ptr = Temp_Cbsm_Mib_Root.next;
    }

    cbsm_community_ptr = Cbsm_Mib_Root.next;

    while (cbsm_community_ptr)
    {
        /* Clear out the row flag. */
        cbsm_community_ptr->cbsm_row_flag = 0;

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)
        /* Update data in file. */
        CBSM_Save_Community(cbsm_community_ptr);
#endif

        /* Moving forward in the list. */
        cbsm_community_ptr = cbsm_community_ptr->next;
    }

} /* Commit_snmpCommunityEntries */

/*************************************************************************
*
*   FUNCTION
*
*       Undo_snmpCommunityEntry
*
*   DESCRIPTION
*
*       This function is used handle SNMP UNDO request on CBSM structures.
*
*   INPUTS
*
*       *obj                    The SNMP object containing request.
*
*   OUTPUTS
*
*       SNMP_NOERROR            Always successful.
*
*************************************************************************/
STATIC UINT16 Undo_snmpCommunityEntry(const snmp_object_t *obj)
{
    /* Handle to snmpCommunityEntry. */
    CBSM_COMMUNITY_STRUCT   *cbsm_community_ptr = NU_NULL;

    /* Community Index. */
    UINT8                   community_index[SNMP_SIZE_SMALLOBJECTID];

    /* Status to return success or error code. */
    UINT16                  status = SNMP_NOERROR;

    /* Variable to use in for-loop. */
    UINT32                  i;

    /* Clear out the value of community index. */
    UTL_Zero(community_index, sizeof(community_index));

    /* Get the value of community index from OID. */
    for (i = 0;
         (i < SNMP_SIZE_SMALLOBJECTID) &&
         ((i + SNMP_CBSM_MIB_SUB_LEN) < obj->IdLen);
         i++)
    {
        community_index[i] = (UINT8)(obj->Id[SNMP_CBSM_MIB_SUB_LEN + i]);
    }

    /* If community index is valid. */
    if ( ((i + SNMP_CBSM_MIB_SUB_LEN) == obj->IdLen) &&
         (i > 0) && (community_index[i - 1] != 0) )
    {
        /* Get the handle of CBSM structure from temporary list. */
        cbsm_community_ptr = Get_SNMP_CBSMEntry_Util(community_index,
                                                    &Temp_Cbsm_Mib_Root);

        /* If we got the handle to CBSM structure from the temporary
         * list.
         */
        if (cbsm_community_ptr)
        {
            /* Remove the handle from the temporary list. */
            DLL_Remove(&Temp_Cbsm_Mib_Root, cbsm_community_ptr);

            /* Deallocate the memory attained by CBSM structure. */
            if (NU_Deallocate_Memory(cbsm_community_ptr) != NU_SUCCESS)
            {
                NLOG_Error_Log("SNMP: Failed to deallocate memory",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            /* Set the handle to NU_NULL. */
            cbsm_community_ptr = NU_NULL;
        }
        else
        {
            /* Get the handle from permanent list. */
            cbsm_community_ptr = Get_SNMP_CBSMEntry_Util(community_index,
                                                        &Cbsm_Mib_Root);
        }
    }

    /* If we have the handle to the CBSM structure. */
    if (cbsm_community_ptr)
    {
        /* Clearing row flag. */
        cbsm_community_ptr->cbsm_row_flag = 0;

        switch(obj->Id[SNMP_CBSM_MIB_ATTR_OFFSET])
        {
        case 2:                         /* snmpCommunityName */

            /* Set the value of 'snmpCommunityName'. */
            NU_BLOCK_COPY(cbsm_community_ptr->cbsm_community_name,
                          obj->Syntax.BufChr, obj->SyntaxLen);

            /* Set the length of 'snmpCommunityName'. */
            cbsm_community_ptr->cbsm_community_index_len =
                                                    (UINT8)obj->SyntaxLen;

            break;

        case 3:                         /* snmpCommunitySecurityName */

            if (strlen((CHAR *)(obj->Syntax.BufChr)) <
                                                 SNMP_SIZE_SMALLOBJECTID)
            {
                /* Set the value of security name. */
                strcpy((CHAR *)(cbsm_community_ptr->cbsm_security_name),
                       (CHAR *)(obj->Syntax.BufChr));
            }

            break;

        case 4:                         /* snmpCommunityContextEngineID */

            /* Set the length of engine ID. */
            cbsm_community_ptr->cbsm_engine_id_len =
                                                (UINT8)(obj->SyntaxLen);

            /* Set the value of engine ID. */
            NU_BLOCK_COPY(cbsm_community_ptr->cbsm_engine_id,
                          obj->Syntax.BufChr, obj->SyntaxLen);


            break;
        case 5:                         /* snmpCommunityContextName */

            if (strlen((CHAR *)(obj->Syntax.BufChr)) <
                                                  SNMP_SIZE_SMALLOBJECTID)
            {
                /* Set the value context name. */
                strcpy((CHAR *)cbsm_community_ptr->cbsm_context_name,
                       (CHAR *)obj->Syntax.BufChr);
            }

            break;

        case 6:                         /* snmpCommunityTransportTag */

            /* Set the value of transport Tag length. */
            cbsm_community_ptr->cbsm_transport_tag_len = obj->SyntaxLen;

            /* Set the value of transport Tag length. */
            NU_BLOCK_COPY(cbsm_community_ptr->cbsm_transport_tag,
                          obj->Syntax.BufChr, obj->SyntaxLen);

            break;

        case 7:                         /* snmpCommunityStorageType */

            /* Set the value of storage type. */
            cbsm_community_ptr->cbsm_storage_type =
                                            (UINT8)(obj->Syntax.LngUns);

            break;

        case 8:                         /* snmpCommunityStatus */

            /* Set the value of row status. */
            cbsm_community_ptr->cbsm_status = (UINT8)obj->Syntax.LngUns;

            break;
        }
    }

    /* Return success or error code. */
    return (status);

} /* Undo_snmpCommunityEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Set_snmpCommunityEntry
*
*   DESCRIPTION
*
*       This function is used to handle SNMP SET request on CBSM
*       structures.
*
*   INPUTS
*
*       *obj                    The SNMP object containing request.
*
*   OUTPUTS
*
*       SNMP_NOSUCHINSTANCE     When there does not exist an CBSM
*                               structure specified by OID.
*       SNMP_NOSUCHNAME         When required attribute does not exists
*                               or inaccessible.
*       SNMP_WRONGLENGTH        When syntax length is not valid.
*       SNMP_WRONGVALUE         When value is not valid to set.
*       SNMP_NOERROR            When successful.
*
*************************************************************************/
STATIC UINT16 Set_snmpCommunityEntry(const snmp_object_t *obj)
{
    /* Handle to snmpCommunityEntry. */
    CBSM_COMMUNITY_STRUCT   *cbsm_community_ptr = NU_NULL;

    /* Community Index. */
    UINT8                   community_index[SNMP_SIZE_SMALLOBJECTID];

    /* Status to return success or error code. */
    UINT16                  status = SNMP_NOERROR;

    /* The variable to use in for-loop. */
    UINT32                  i;

    /* Clear out the value of community index. */
    UTL_Zero(community_index, sizeof(community_index));

    /* Getting the value of community index. */
    for (i = 0;
         (i < SNMP_SIZE_SMALLOBJECTID) &&
         ((i + SNMP_CBSM_MIB_SUB_LEN) < obj->IdLen);
         i++)
    {
        community_index[i] = (UINT8)(obj->Id[SNMP_CBSM_MIB_SUB_LEN + i]);
    }

    /* If we have valid community index. */
    if ( ((i + SNMP_CBSM_MIB_SUB_LEN) == obj->IdLen) &&
         (i > 0) && (community_index[i - 1] != 0) )
    {
        /* Get the handle to the CBSM structure. */
        cbsm_community_ptr = Get_SNMP_CBSMEntry(community_index, NU_TRUE);
    }

    /* If we got the handle to the CBSM structure. */
    if (cbsm_community_ptr)
    {
        switch(obj->Id[SNMP_CBSM_MIB_ATTR_OFFSET])
        {
        case 2:                         /* snmpCommunityName */

            /* If length is not valid for 'snmpCommunityName' then return
             * error code.
             */
            if ((obj->SyntaxLen >= SNMP_SIZE_SMALLOBJECTID) ||
                (obj->SyntaxLen == 0))
            {
                status = SNMP_WRONGLENGTH;
            }

            /* If the value of 'snmpCommunityName' is not valid then
             * return error code.
             */
            else if (strlen((CHAR *)obj->Syntax.BufChr) != obj->SyntaxLen)
            {
                status = SNMP_WRONGVALUE;
            }

            /* If value to set is valid. */
            else
            {
                /* Set the value to 'snmpCommunityName'. */
                strncpy((CHAR *)cbsm_community_ptr->cbsm_community_name,
                    (CHAR *)obj->Syntax.BufChr, SNMP_SIZE_SMALLOBJECTID);
            }

            break;

        case 3:                         /* snmpCommunitySecurityName */

            /* If length is not valid for 'snmpCommunitySecurityName' then
             * return error code.
             */
            if ((obj->SyntaxLen == 0) ||
                (obj->SyntaxLen >= SNMP_SIZE_SMALLOBJECTID))
            {
                status = SNMP_WRONGLENGTH;
            }

            /* If the value of 'snmpCommunitySecurityName' is not valid
             * then return error code.
             */
            else if (strlen((CHAR *)obj->Syntax.BufChr) != obj->SyntaxLen)
            {
                status = SNMP_WRONGVALUE;
            }

            /* If value to set is valid. */
            else
            {
                /* Set the value to 'snmpCommunitySecurityName'. */
                strncpy((CHAR *)cbsm_community_ptr->cbsm_security_name,
                       (CHAR *)obj->Syntax.BufChr, SNMP_SIZE_SMALLOBJECTID);
            }

            break;

        case 4:                         /* snmpCommunityContextEngineID */

            /* If length is not valid for 'snmpCommunityContextEngineID'
             * then return error code.
             */
            if ((obj->SyntaxLen > SNMP_SIZE_SMALLOBJECTID) ||
                (obj->SyntaxLen <= 4))
            {
                status = SNMP_WRONGLENGTH;
            }

            /* If value to set is valid. */
            else
            {
                /* Set the length of engine ID. */
                cbsm_community_ptr->cbsm_engine_id_len =
                                                (UINT8)(obj->SyntaxLen);

                /* Set the value of 'snmpCommunityContextEngineID'. */
                NU_BLOCK_COPY(cbsm_community_ptr->cbsm_engine_id,
                              obj->Syntax.BufChr, obj->SyntaxLen);
            }

            break;

        case 5:                         /* snmpCommunityContextName */

            /* If length is not valid for 'snmpCommunityContextName' then
             * return error code.
             */
            if (obj->SyntaxLen >= SNMP_SIZE_SMALLOBJECTID)
            {
                status = SNMP_WRONGLENGTH;
            }

            /* If the value of 'snmpCommunityContextName' is not valid
             * then return error code.
             */
            else if (strlen((CHAR *)obj->Syntax.BufChr) != obj->SyntaxLen)
            {
                status = SNMP_WRONGVALUE;
            }

            /* If value to set is valid. */
            else
            {
                /* Set the value of 'snmpCommunityContextName'. */
                strncpy((CHAR *)cbsm_community_ptr->cbsm_context_name,
                       (CHAR *)obj->Syntax.BufChr, SNMP_SIZE_SMALLOBJECTID);
            }

            break;

        case 6:                         /* snmpCommunityTransportTag */

            /* If length is not valid for 'snmpCommunityTransportTag' then
             * return error code.
             */
            if (obj->SyntaxLen >= SNMP_SIZE_BUFCHR)
            {
                status = SNMP_WRONGLENGTH;
            }

            /* If the value of 'snmpCommunityTransportTag' is not valid
             * then return error code.
             */
            else if ( (strlen((CHAR *)obj->Syntax.BufChr) != obj->SyntaxLen) ||
					  (obj->SyntaxLen > SNMP_SIZE_SMALLOBJECTID) )
            {
                status = SNMP_WRONGVALUE;
            }

            /* If value to set is valid. */
            else
            {
                /* Set length of 'snmpCommunityTransportTag'. */
                cbsm_community_ptr->cbsm_transport_tag_len =
                                                        obj->SyntaxLen;

                /* Set value of 'snmpCommunityTransportTag'. */
                NU_BLOCK_COPY(cbsm_community_ptr->cbsm_transport_tag,
                              obj->Syntax.BufChr, obj->SyntaxLen);
            }

            break;

        case 7:                         /* snmpCommunityStorageType */

            /* If the value of 'snmpCommunityStorageType' is not in valid
             * range then return error code.
             */
            if ((obj->Syntax.LngUns < 1) || (obj->Syntax.LngUns > 5))
            {
                status = SNMP_WRONGVALUE;
            }

            /* If the value is in valid range. */
            else
            {
                /* Set the value of 'snmpCommunityStorageType'. */
                cbsm_community_ptr->cbsm_storage_type =
                                        (UINT8)(obj->Syntax.LngUns);
            }

            break;

        case 8:                         /* snmpCommunityStatus */

            /* If the value is not in valid range then return error code.
             */
            if ((obj->Syntax.LngUns < 1) || (obj->Syntax.LngUns > 6))
            {
                status = SNMP_WRONGVALUE;
            }
            else if ((cbsm_community_ptr->cbsm_row_flag == 0) ||
                     (cbsm_community_ptr->cbsm_row_flag ==
                            (UINT8)(obj->Syntax.LngUns)))
            {
                cbsm_community_ptr->cbsm_row_flag =
                                              (UINT8)(obj->Syntax.LngUns);
            }
            else
            {
                status = SNMP_INCONSISTANTVALUE;
            }


            break;

        default:                        /* We have reached at end of the
                                         * table.
                                         */

            /* Return error code. */
            status = SNMP_NOSUCHNAME;
        }
    }

    /* If we did not get the handle to the CBSM structure then return
     * error code.
     */
    else
    {
        status = SNMP_NOSUCHINSTANCE;
    }

    /* Return success or error code. */
    return (status);

} /* Set_snmpCommunityEntry */

/*************************************************************************
*
*   FUNCTION
*
*       snmpCommunityEntry
*
*   DESCRIPTION
*
*       This function is used to handle SNMP requests on CBSM structures.
*
*   INPUTS
*
*       *obj                    The SNMP object containing request.
*       idlen                   Unused.
*       *param                  Unused.
*
*   OUTPUTS
*
*       SNMP_NOSUCHINSTANCE     When there does not exist an CBSM
*                               structure specified by OID.
*       SNMP_NOSUCHNAME         When required attribute does not exists
*                               or inaccessible.
*       SNMP_WRONGLENGTH        When syntax length is not valid.
*       SNMP_WRONGVALUE         When value is not valid to set.
*       SNMP_INCONSISTANTVALUE  When value of status is inconsistent with
*                               the current state.
*       SNMP_NOERROR            When successful.
*
*************************************************************************/
UINT16 snmpCommunityEntry(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    /* Handle to the snmpCommunityEntry. */
    CBSM_COMMUNITY_STRUCT   *cbsm_community_ptr;

    /* Temporary handle to the snmpCommunityEntry. */
    CBSM_COMMUNITY_STRUCT   *temp_cbsm_community_ptr;

    /* State transitions. */
    UINT16                  status_trans[6][4] =
    {{SNMP_INCONSISTANTVALUE, SNMP_INCONSISTANTVALUE, SNMP_NOERROR,
      SNMP_NOERROR},
     {SNMP_INCONSISTANTVALUE, SNMP_INCONSISTANTVALUE, SNMP_NOERROR,
      SNMP_NOERROR},
     {SNMP_INCONSISTANTVALUE, SNMP_INCONSISTANTVALUE,
      SNMP_INCONSISTANTVALUE, SNMP_INCONSISTANTVALUE},
     {SNMP_NOERROR, SNMP_INCONSISTANTVALUE, SNMP_INCONSISTANTVALUE,
      SNMP_INCONSISTANTVALUE},
     {SNMP_NOERROR, SNMP_INCONSISTANTVALUE, SNMP_INCONSISTANTVALUE,
      SNMP_INCONSISTANTVALUE},
     {SNMP_NOERROR, SNMP_NOERROR, SNMP_NOERROR, SNMP_NOERROR}
    };

    /* Variable to use in for-loop. */
    UINT32                  i, current_state;

    /* Community Index. */
    UINT8                   community_index[SNMP_SIZE_SMALLOBJECTID];

    /* Status to return success or error code. */
    UINT16          status = SNMP_NOERROR;

    /* Flag to distinguish between get and get-next requests. */
    UINT8           getflag = 0;

    /* Avoid compilation warnings. */
    UNUSED_PARAMETER(param);
    UNUSED_PARAMETER(idlen);

    switch(obj->Request)
    {
    case SNMP_PDU_GET:                      /* Get request. */

        /* Set getflag so that it can represent get-request. */
        getflag++;

        /* Fall through the next case for fulfilling the get-request. */

    case SNMP_PDU_NEXT:                     /* Get next request. */

        /* Processing GET / GET-NEXT operations. */
        status = Get_snmpCommunityEntry(obj, getflag);

        break;

    case SNMP_PDU_BULK:                     /* Get-bulk request. */

        /* Processing of bulk operation. */
        status = SNMP_Get_Bulk(obj, Get_snmpCommunityEntry);

        break;

    case SNMP_PDU_SET:                      /* Set request. */

        CBSM_MIB_Commit_Left++;

        /* Process the SET operation. */
        status = Set_snmpCommunityEntry(obj);

        if (status != SNMP_NOSUCHINSTANCE)
            break;

    case SNMP_PDU_CREATE:

        /* Processing of create operation. */
        status = Create_snmpCommunityEntry(obj);

        /* If the entry was successfully created, set the value. */
        if(status == SNMP_NOERROR)
            status = Set_snmpCommunityEntry(obj);

        break;

    case SNMP_PDU_UNDO:                     /* Undo request. */

        CBSM_MIB_Commit_Left = 0;

        /* Processing of UNDO operations. */
        status = Undo_snmpCommunityEntry(obj);

        break;

    case SNMP_PDU_COMMIT:                   /* Commit request. */

        CBSM_MIB_Commit_Left--;

        /* Clear out the community index. */
        UTL_Zero(community_index, sizeof(community_index));

        /* Get the value of community index. */
        for (i = 0;
             (i < SNMP_SIZE_SMALLOBJECTID) &&
             ((i + SNMP_CBSM_MIB_SUB_LEN) < obj->IdLen);
             i++)
        {
            community_index[i] =
                              (UINT8)(obj->Id[SNMP_CBSM_MIB_SUB_LEN + i]);
        }

        /* Get the handle to the CBSM structure from temporary list. */
        cbsm_community_ptr = Get_SNMP_CBSMEntry_Util(community_index,
                                                    &Temp_Cbsm_Mib_Root);

        /* Save the handle in temporary handled. */
        temp_cbsm_community_ptr = cbsm_community_ptr;

        /* If we did not get handle to the CBSM structure from the
         * temporary list then the CBSM structure from permanent list.
         */
        if (!cbsm_community_ptr)
        {
            /* Get the handle of CBSM structure from permanent list. */
            cbsm_community_ptr = Get_SNMP_CBSMEntry_Util(community_index,
                                                        &Cbsm_Mib_Root);
        }

        /* If we got the handle either from permanent or temporary list.
         */
        if (cbsm_community_ptr)
        {
            /* Check whether it was a row set operation. */
            if (obj->Id[SNMP_CBSM_MIB_ATTR_OFFSET] == 8)
            {
                /* Update the value of current state.
                 *  0) New entry.
                 *  1) Not ready.
                 *  2) Not In service.
                 *  3) Active
                 */

                /* If we have new entry the set current state to 0. */
                if (temp_cbsm_community_ptr)
                {
                    current_state = 0;
                }

                /* If we have new not ready entry then set current state
                 * to 1.
                 */
                else if ((strlen((CHAR *)cbsm_community_ptr->
                                            cbsm_community_name) == 0) ||
                         (strlen((CHAR *)cbsm_community_ptr->
                                                cbsm_security_name) == 0))
                {
                    current_state = 1;
                }

                /* If we have not in service entry then set current state
                 * to 2.
                 */
                else if (cbsm_community_ptr->cbsm_status !=
                                                        SNMP_ROW_ACTIVE)
                {
                    current_state = 2;
                }

                /* If we have active entry then set current state to 3. */
                else
                {
                    current_state = 3;
                }

                /* If required row status transitions is not allowed then
                 * return error code.
                 */
                if (status_trans[obj->Syntax.LngUns - 1][current_state] !=
                                                            SNMP_NOERROR)
                {
                    status =
                      status_trans[obj->Syntax.LngUns - 1][current_state];
                }

                /* If required state transition is allowed then commit the
                 * row status value.
                 */
                else
                {
                    /* Commit the row status value. */
                    status = Commit_snmpCommunityEntryStatus(
                        cbsm_community_ptr, (UINT8)(obj->Syntax.LngUns),
                        (UINT8)(temp_cbsm_community_ptr != NU_NULL) );
                }
            }

            /* If it was not row set operation. */
            else
            {
                status = Commit_snmpCommunityEntryStatus(
                                        cbsm_community_ptr, 0, NU_FALSE);
            }

            /* If the row status is set to 'DESTROY'. */
            if (cbsm_community_ptr->cbsm_status == SNMP_ROW_DESTROY)
            {
                /* If handle was from temporary list then remove it from
                 * temporary list.
                 */
                if (temp_cbsm_community_ptr)
                    DLL_Remove(&Temp_Cbsm_Mib_Root, cbsm_community_ptr);

                /* If handle was from permanent list then remove it from
                 * permanent list.
                 */
                else
                    DLL_Remove(&Cbsm_Mib_Root, cbsm_community_ptr);

                /* Deallocate memory attained by this CBSM structure. */
                if (NU_Deallocate_Memory(cbsm_community_ptr) !=
                                                              NU_SUCCESS)
                {
                    NLOG_Error_Log("SNMP: Failed to deallocate memory",
                                    NERR_RECOVERABLE, __FILE__, __LINE__);
                }
            }
        }

        /* If commit operation went okay, and this was the last commit
         * operation then shift all the entries from temporary list to
         * permanent list.
         */
        if ((status == SNMP_NOERROR) && (CBSM_MIB_Commit_Left == 0))
        {
            Commit_snmpCommunityEntries();
        }

        break;

    default:                                /* Invalid request. */
        status = SNMP_GENERROR;
    }

    /* Return success or error code. */
    return (status);

} /* snmpCommunityEntry */

#endif /* (INCLUDE_MIB_CBSM == NU_TRUE)*/
