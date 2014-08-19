/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 1998 - 2007
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
*   FILENAME                                               
*
*       no_mib.c                                                 
*
*   COMPONENT
*
*       Notification MIBs
*
*   DESCRIPTION
*
*       This file contains implementation of the Notification MIBs.
*
*   DATA STRUCTURES
*
*       Temp_Profile_Mib_Root
*       Profile_Table_MIB_Commit_Left
*
*   FUNCTIONS
*
*       NO_MIB_Get_Notify
*       Get_snmpNotifyEntry
*       snmpNotifyEntry
*       NO_MIB_Get_Filter
*       NO_MIB_Get_Filter_Util
*       Set_snmpNotifyFiltProfEntry
*       Create_snmpNotifyFiltProfEntry
*       Get_snmpNotifyFiltProfEntry
*       SNMP_MIB_No_Get_Filter_Util
*       Undo_snmpNotifyFiltProfEntry
*       Commit_snmpNotifyFiltProfEntryStatus
*       Commit_snmpNotifyFiltProfEntries
*       snmpNotifyFiltProfEntry
*       Get_snmpNotifyFilterEntry
*       snmpNotifyFilterEntry
*
*   DEPENDENCIES
*
*       snmp.h
*       snmp_utl.h
*       snmp_no.h
*       no_mib.h
*       snmp_g.h
*       mib.h
*
*************************************************************************/
#include "networking/snmp.h"
#include "networking/snmp_utl.h"
#include "networking/snmp_no.h"
#include "networking/no_mib.h"
#include "networking/snmp_g.h"
#include "networking/mib.h"

#if (INCLUDE_MIB_NO == NU_TRUE)

extern SNMP_NOTIFY_TABLE_ROOT           Snmp_Notify_Table;
extern SNMP_PROFILE_TABLE_ROOT          Snmp_Profile_Table;
extern SNMP_FILTER_TABLE_ROOT           Snmp_Filter_Table;
SNMP_NOTIFY_FILTER_PROFILE_TABLE        Temp_Profile_Mib_Root;
STATIC UINT32                           Profile_Table_MIB_Commit_Left;

/*************************************************************************
*
*   FUNCTION
*
*       NO_MIB_Get_Notify
*
*   DESCRIPTION
*
*       This function is used to get handle for the notification table
*       entry by specifying notify name.
*
*   INPUTS
*
*       *snmp_notify_name       Pointer to the memory where notify name
*                               is stored.
*       getflag                 Flag to distinguish GET or GET-NEXT
*                               requests.
*
*   OUTPUTS
*
*       SNMP_NOTIFY_TABLE *     When there exists the required
*                               notification table handle.
*       NU_NULL                 When there does not exist the required
*                               notification handle.
*
*************************************************************************/
SNMP_NOTIFY_TABLE *NO_MIB_Get_Notify(const UINT8 *snmp_notify_name,
                                     UINT8 getflag)
{
    /* Handle to the notification table entry. */
    SNMP_NOTIFY_TABLE *locator = Snmp_Notify_Table.flink;

    /* If we are handling GET request. */
    if (getflag)
    {
        /* Loop to find the exact match. */
        while (locator)
        {
            /* If we reach at the required position then break through the
             * loop and return the value.
             */
            if (strcmp((CHAR *)locator->snmp_notify_name,
                       (CHAR *)snmp_notify_name) == 0)
            {
                /* Breaking through the loop. */
                break;
            }

            /* Moving forward in the list. */
            locator = locator->flink;
        }
    }

    /* If we are handling GET-NEXT request. */
    else
    {
        /* Loop till we reach an entry with greater index. */
        while (locator)
        {
            /* If we reached at the entry with better index then break
             * through the loop.
             */
            if (strcmp((CHAR *)locator->snmp_notify_name,
                       (CHAR *)snmp_notify_name) > 0)
            {
                /* Breaking through the loop. */
                break;
            }

            /* Moving forward in the list. */
            locator = locator->flink;
        }
    }

    /* Return the handle to the notification table entry if found.
     * Otherwise return NU_NULL. */
    return (locator);

} /* NO_MIB_Get_Notify */

/*************************************************************************
*
*   FUNCTION
*
*       Get_snmpNotifyEntry
*
*   DESCRIPTION
*
*       This function is used to handle SNMP GET and GET-NEXT requests on
*       snmpNotifyTable.
*
*   INPUTS
*
*       *obj                    SNMP object containing request.
*       getflag                 Flag to distinguish GET and GET-NEXT
*                               requests.
*
*   OUTPUTS
*
*       SNMP_NOERROR            Successful
*       SNMP_NOSUCHINSTANCE     Instance specify by indexes does not
*                               exists.
*       SNMP_NOSUCHNAME         The attribute specified in the object
*                               identifier is not supported.
*
*************************************************************************/
STATIC UINT16 Get_snmpNotifyEntry(snmp_object_t *obj, UINT8 getflag)
{
    /* Object identifier of table entry. */
    UINT32              table_oid[] = {1, 3, 6, 1, 6, 3, 13, 1, 1, 1};

    /* Handle to snmpNotifyEntry. */
    SNMP_NOTIFY_TABLE   *snmp_notify_entry = NU_NULL;

    /* Notify Name (Index). */
    UINT8               snmp_notify_name[MAX_NOTIFY_NAME_SZE];

    /* Status to return success or error code. */
    UINT16              status = SNMP_NOERROR;

    /* Variable to use in for-loop. */
    UINT32              i;

    /* Clear out the notify name. */
    UTL_Zero(snmp_notify_name, sizeof(snmp_notify_name));

    /* Get the value of notify name from the OID. */
    for (i = 0;
         (i < MAX_NOTIFY_NAME_SZE) &&
         ((i + SNMP_NO_MIB_NOTIFY_SUB_LEN) < obj->IdLen);
         i++)
     {
        snmp_notify_name[i] =
                        (UINT8)(obj->Id[SNMP_NO_MIB_NOTIFY_SUB_LEN + i]);
     }

    /* If we are having GET request. */
    if (getflag)
    {
        /* If we don't have valid notify name then return error code. */
        if ( ((i + SNMP_NO_MIB_NOTIFY_SUB_LEN) != obj->IdLen) ||
             (i == 0) || (snmp_notify_name[i - 1] == 0) )
        {
            /* Returning error code. */
            status = SNMP_NOSUCHINSTANCE;
        }
    }

    /* If we are handling GET-NEXT request make notify name, null
     * terminating.
     */
    else if (i < MAX_NOTIFY_NAME_SZE)
    {
        snmp_notify_name[i] = NU_NULL;
    }

    /* If no error till now then get the handle to the snmpNotifyEntry. */
    if (status == SNMP_NOERROR)
    {
        /* Getting handle to the snmpNotifyEntry structure. */
        snmp_notify_entry = NO_MIB_Get_Notify(snmp_notify_name, getflag);
    }

    /* If we got the handle to the snmpNotifyEntry structure. */
    if (snmp_notify_entry)
    {
        switch(obj->Id[SNMP_NO_MIB_NOTIFY_ATTR_OFFSET])
        {
        case 2:                         /* snmpNotifyTag */

            /* Get the value of 'snmpNotifyTag'. */
            NU_BLOCK_COPY(obj->Syntax.BufChr,
                          snmp_notify_entry->snmp_notify_tag,
                          snmp_notify_entry->tag_len);

            /* Get the length of snmpNotifyTag. */
            obj->SyntaxLen = snmp_notify_entry->tag_len;

            break;

        case 3:                         /* snmpNotifyType */

            /* Get the value of 'snmpNotifyType'. */
            obj->Syntax.LngUns =
                            (UINT32)snmp_notify_entry->snmp_notify_type;

            break;

        case 4:                         /* snmpNotifyStorageType */

            /* Get the value of 'snmpNotifyStorageType'. */
            obj->Syntax.LngUns = snmp_notify_entry->
                                                snmp_notify_storage_type;

            break;

        case 5:                         /* snmpNotifyRowStatus */

            /* Get the value of 'snmpNotifyRowStatus'. */
            obj->Syntax.LngUns = snmp_notify_entry->
                                            snmp_notify_row_status;

            break;

        default:                        /* We have reached at end of the
                                         * table.
                                         */

            /* Return error code. */
            status = SNMP_NOSUCHNAME;
        }

        /* Update OID if we are handling GET-NEXT request and request was
         * successful.
         */
        if ((status == SNMP_NOERROR) && (!getflag))
        {
            /* Update the table OID. */
            NU_BLOCK_COPY(obj->Id, table_oid, sizeof(table_oid));

            /* Update notify name in the index field of OID. */
            for (i = 0 ; (i < MAX_NOTIFY_NAME_SZE); i++)
            {
                obj->Id[SNMP_NO_MIB_NOTIFY_SUB_LEN + i] =
                    (UINT32)(snmp_notify_entry->snmp_notify_name[i]);

                /* If notify name is terminated then break the loop. */
                if (snmp_notify_entry->snmp_notify_name[i] == 0)
                    break;
            }

            /* Update OID length. */
            obj->IdLen = (SNMP_NO_MIB_NOTIFY_SUB_LEN + i);
        }
    }

    /* If we did not get the handle to the snmpNotifyEntry structure then
     * return error code.
     */
    else
    {
        /* Returning error code. */
        status = SNMP_NOSUCHINSTANCE;
    }

    /* Return success or error code. */
    return (status);

} /* Get_snmpNotifyEntry */

/*************************************************************************
*
*   FUNCTION
*
*       snmpNotifyEntry
*
*   DESCRIPTION
*
*       This function is used to handle SNMP request on snmpNotifyTable.
*
*   INPUTS
*
*       *obj                SNMP object containing request.
*       idlen               Not used.
*       *param              Not used.
*
*   OUTPUTS
*
*       SNMP_NOERROR            Successful
*       SNMP_NOSUCHINSTANCE     Instance specify by indexes does not
*                               exists.
*       SNMP_NOSUCHNAME         The attribute specified in the object
*                               identifier is not supported.
*       SNMP_GENERROR           Invalid request.
*
*************************************************************************/
UINT16 snmpNotifyEntry(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    /* Status to return success or error code. */
    UINT16          status;

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
        status = Get_snmpNotifyEntry(obj, getflag);

        break;

    case SNMP_PDU_BULK:                     /* Get-bulk request. */

        /* Processing of bulk operation. */
        status = SNMP_Get_Bulk(obj, Get_snmpNotifyEntry);

        break;

    case SNMP_PDU_SET:                      /* Set request. */
    case SNMP_PDU_UNDO:                     /* Undo request. */
    case SNMP_PDU_COMMIT:                   /* Commit request. */
    default:                                /* Invalid request. */
        status = SNMP_GENERROR;
    }

    /* Return success or error code. */
    return (status);

} /* snmpNotifyEntry */

/*************************************************************************
*
*   FUNCTION
*
*       NO_MIB_Get_Filter_Util
*
*   DESCRIPTION
*
*       This function is used to get handle for the notification filter
*       profile table entry by specifying target param name and the handle
*       to the root table.
*
*   INPUTS
*
*       *target_param_name                 Pointer to the memory where
*                                          target param name is stored.
*       *root                              Pointer to the source table
*
*   OUTPUTS
*
*       SNMP_NOTIFY_FILTER_PROFILE_TABLE *  When there exists the required
*                                           notification filter profile
*                                           table entry handle.
*       NU_NULL                             When there does not exist the
*                                           required notification filter
*                                           profile table entry handle.
*
*************************************************************************/
STATIC SNMP_NOTIFY_FILTER_PROFILE_TABLE* NO_MIB_Get_Filter_Util(
    const UINT8 *target_param_name, SNMP_NOTIFY_FILTER_PROFILE_TABLE* root)
{
    /* Handle to the filter profile table structure. */
    SNMP_NOTIFY_FILTER_PROFILE_TABLE *profile_ptr = root;

    /* Loop to find the profile table with target params name passed in. */
    while (profile_ptr)
    {
        /* Check whether the target params name of the current entry
         * matches, the name passed.
         */
        if(strcmp(profile_ptr->snmp_target_params_name,(CHAR *)target_param_name) == 0)
        {
            break;
        }

        /* Moving forward in the list. */
        profile_ptr = profile_ptr->flink;
    }

    /* Return handle to the Notification filter profile table. If found, it
     * will be a valid value or NU_NULL otherwise.
     */
    return (profile_ptr);

} /* NO_MIB_Get_Filter_Util */

/*************************************************************************
*
*   FUNCTION
*
*       NO_MIB_Get_Filter
*
*   DESCRIPTION
*
*       This function is used to get handle for the notification filter
*       profile table entry by specifying target param name only. It looks
*       in the permanent list and if not found there then searches the
*       temporary list.
*
*   INPUTS
*
*       *target_param_name      Pointer to the memory where target param
*                               name is stored.
*       getflag                 Flag to distinguish GET or GET-NEXT
*                               requests.
*
*   OUTPUTS
*
*       SNMP_NOTIFY_FILTER_PROFILE_TABLE *  When there exists the required
*                                           notification filter profile
*                                           table entry handle.
*       NU_NULL                             When there does not exist the
*                                           required notification filter
*                                           profile table entry handle.
*
*************************************************************************/
STATIC SNMP_NOTIFY_FILTER_PROFILE_TABLE* NO_MIB_Get_Filter(
                            const UINT8 *target_param_name, UINT8 getflag)
{
    /* Handle to the snmpNotifyFiltProfEntry. */
    SNMP_NOTIFY_FILTER_PROFILE_TABLE    *location_ptr;

    /* If we are handling GET request. */
    if (getflag)
    {
        /* Try to get the notification filter profile table handle from
         * permanent list.
         */
        location_ptr = NO_MIB_Get_Filter_Util(target_param_name,
                            Snmp_Profile_Table.flink);

        /* If it was not found in the permanent list. */
        if (!location_ptr)
        {
            /* Try to get profile table handle from temporary list. */
            location_ptr = NO_MIB_Get_Filter_Util(target_param_name,
                               &Temp_Profile_Mib_Root);
        }
    }

    /* If we are handling GET-NEXT request. */
    else
    {
        /* Start traversing the permanent list. */
        location_ptr = Snmp_Profile_Table.flink;

        /* Loop to find the snmpNotifyFiltProfEntry with greater
         * index as passed in.
         */
        while (location_ptr)
        {
            /* If we reached at entry with greater index then break
             * through the loop and return current handle.
             */
            if (strcmp((CHAR *)location_ptr->snmp_target_params_name,
                       (CHAR *)target_param_name) > 0)
            {
                /* Break through the loop. */
                break;
            }

            /* Moving forward in the list. */
            location_ptr = location_ptr->flink;
        }
    }

    /* Return handle to the snmpNotifyFiltProfEntry. It will be valid value
     * if found or NU_NULL otherwise.
     */
    return (location_ptr);

} /* NO_MIB_Get_Filter */

/*************************************************************************
*
*   FUNCTION
*
*       Set_snmpNotifyFiltProfEntry
*
*   DESCRIPTION
*
*       This function is used to handle SNMP SET request on Notify filter
*       profile table.
*
*   INPUTS
*
*       *obj                    The SNMP object containing request.
*
*   OUTPUTS
*
*       SNMP_NOSUCHINSTANCE     When there does not exist a Profile
*                               structure specified by OID.
*       SNMP_NOSUCHNAME         When required attribute does not exists
*                               or inaccessible.
*       SNMP_WRONGLENGTH        When syntax length is not valid.
*       SNMP_WRONGVALUE         When value is not valid to set.
*       SNMP_NOERROR            When successful.
*
*************************************************************************/
STATIC UINT16 Set_snmpNotifyFiltProfEntry(const snmp_object_t *obj)
{
    /* Handle to the profile entry. */
    SNMP_NOTIFY_FILTER_PROFILE_TABLE   *profile_entry = NU_NULL;

    /* Variable to hold target params name. */
    UINT8                       target_params[MAX_FILTER_PROF_NAME_SZE];

    /* Variable to use in for-loop. */
    UINT32                      i;

    /* Status to return success or error code. */
    UINT16                      status = SNMP_NOERROR;

    /* Clear out target params name. */
    UTL_Zero(target_params, sizeof(target_params));

    /* Get the value of target params name. */
    for (i = 0;
        (i < (MAX_FILTER_PROF_NAME_SZE - 1)) &&
        (SNMP_NO_MIB_FILTER_PRO_SUB_LEN + i < obj->IdLen);
    i++)
    {
        target_params[i] = (UINT8)(obj->Id[SNMP_NO_MIB_FILTER_PRO_SUB_LEN + i]);
    }

    /* If we don't have valid object identifier then return error code. */
    if ((obj->IdLen != (SNMP_NO_MIB_FILTER_PRO_SUB_LEN + i)) ||
        (strlen((CHAR *)target_params) != i))
    {
        /* Return error code. */
        status = SNMP_NOSUCHINSTANCE;
    }

    /* If no error till now then proceed. */
    if (status == SNMP_NOERROR)
    {
        /* Get the handle to the notify filter profile entry. This is a set
         * request so it should refer to current entry, thus second
         * parameter should be NU_TRUE.
         */
        profile_entry = NO_MIB_Get_Filter(target_params, NU_TRUE);
    }

    /* If we got the handle to the filter profile entry then proceed. */
    if (profile_entry)
    {
        switch(obj->Id[SNMP_NO_MIB_FILTER_PRO_ATTR_OFFSET])
        {
        case 1:                     /* snmpNotifyFilterProfileName */

            /* If length is not valid for 'snmpNotifyFilterProfileName'
             * then return error code.
             */
            if ((obj->SyntaxLen >= SNMP_SIZE_SMALLOBJECTID) ||
                (obj->SyntaxLen == 0))
            {
                status = SNMP_WRONGLENGTH;
            }

            /* If the value of 'snmpNotifyFilterProfileName' is not valid
             * then return error code.
             */
            else if (strlen((CHAR *)obj->Syntax.BufChr) != obj->SyntaxLen)
            {
                status = SNMP_WRONGVALUE;
            }

            /* If value to set is valid. */
            else
            {
                /* Set the value of 'snmpNotifyFilterProfileName'. */
                strncpy(profile_entry->snmp_notify_filter_profile_name,
                    (CHAR*)(obj->Syntax.BufChr), MAX_FILTER_PROF_NAME_SZE);
            }

            break;

        case 2:                     /* snmpNotifyFilterProfileStoreType */

            /* If the value of 'snmpNotifyFilterProfileStoreType' is not in
             * valid range then return error code.
             */
            if ((obj->Syntax.LngUns < 1) || (obj->Syntax.LngUns > 5))
            {
                status = SNMP_WRONGVALUE;
            }

            /* If value to set is valid. */
            else
            {
                /* Set the value of 'snmpNotifyFilterProfileStoreType'. */
                profile_entry->snmp_notify_filter_profile_storType =
                    (INT32)obj->Syntax.LngUns;
            }

            break;

        case 3:                     /* snmpNotifyFilterProfileRowStatus */

            /* If the value is not in valid range then return error code. */
            if ((obj->Syntax.LngUns < 1) || (obj->Syntax.LngUns > 6))
            {
                status = SNMP_WRONGVALUE;
            }

            else if ((profile_entry->snmp_notify_filter_profile_row_flag == 0)
                || (profile_entry->snmp_notify_filter_profile_row_flag ==
                (UINT8)(obj->Syntax.LngUns)))
            {
                profile_entry->snmp_notify_filter_profile_row_flag =
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

    /* If we did not get the handle to the profile entry then
     * return error code.
     */
    else
    {
        /* Return error code. */
        status = SNMP_NOSUCHINSTANCE;
    }

    /* Return success or error code. */
    return (status);

} /* Set_snmpNotifyFiltProfEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Create_snmpNotifyFiltProfEntry
*
*   DESCRIPTION
*
*       This function is used to handle SNMP create request on Notify filter
*       profile table.
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
STATIC UINT16 Create_snmpNotifyFiltProfEntry(const snmp_object_t *obj)
{
    /* Handle to snmpNotifyFiltProfEntry. */
    SNMP_NOTIFY_FILTER_PROFILE_TABLE *profile_ptr = NU_NULL;

    /* Handle to find the proper place in the list. */
    SNMP_NOTIFY_FILTER_PROFILE_TABLE *locator;

    /* Variable to hold target params name. */
    UINT8                       target_params[MAX_FILTER_PROF_NAME_SZE];

    /* Status to return success or error code. */
    UINT16      status = SNMP_NOERROR;

    /* Variable to hold the comparison result. */
    INT         cmp_result;

    /* Variable to use in for-loop. */
    UINT32      i;

    /* Clear out the value of target name. */
    UTL_Zero(target_params, sizeof(target_params));

    /* Get the value of target params name. */
    for (i = 0;
        (i < (MAX_FILTER_PROF_NAME_SZE - 1)) &&
        (SNMP_NO_MIB_FILTER_PRO_SUB_LEN + i < obj->IdLen);
    i++)
    {
        target_params[i] = (UINT8)(obj->Id[SNMP_NO_MIB_FILTER_PRO_SUB_LEN + i]);
    }

    /* If we don't have valid object identifier then return error code. */
    if ((obj->IdLen != (SNMP_NO_MIB_FILTER_PRO_SUB_LEN + i)) ||
        (strlen((CHAR *)target_params) != i))
    {
        /* Return error code. */
        status = SNMP_NOSUCHINSTANCE;
    }

    /* If no error till now then proceed. */
    if (status == SNMP_NOERROR)
    {
        /* Get the handle to the profile table entry. This is a create
         * request so it should refer to current entry, thus second
         * parameter should be NU_TRUE.
         */
        profile_ptr = NO_MIB_Get_Filter(target_params, NU_TRUE);
    }

    /* If we don't currently have the profile table structure. */
    if (!profile_ptr)
    {
        /* Allocate memory for the new table structure. */
        if (NU_Allocate_Memory(&System_Memory,(VOID **)&profile_ptr,
            sizeof(SNMP_NOTIFY_FILTER_PROFILE_TABLE), NU_NO_SUSPEND) ==
            NU_SUCCESS)
        {
            profile_ptr = TLS_Normalize_Ptr(profile_ptr);

            /* Clear out the newly allocated table structure. */
            UTL_Zero(profile_ptr, sizeof(SNMP_NOTIFY_FILTER_PROFILE_TABLE));

            /* Set the target params name. */
            NU_BLOCK_COPY(profile_ptr->snmp_target_params_name, target_params,
                strlen((CHAR*)target_params));

            /* Set the storage type to non-volatile. */
            profile_ptr->snmp_notify_filter_profile_storType =
                SNMP_STORAGE_DEFAULT;

            /* Set the status to 'createAndWait'. */
            profile_ptr->snmp_notify_filter_profile_row_status =
                SNMP_ROW_CREATEANDWAIT;

            /* Get the starting handle to the list to find the proper
             * position of new profile table structure.
             */
            locator = Temp_Profile_Mib_Root.flink;

            /* Loop to find the proper position. */
            while (locator)
            {
                /* Compare the target names */
                if((cmp_result = strcmp(locator->snmp_target_params_name,
                    profile_ptr->snmp_target_params_name)) >= 0)
                {
                    /* If both the strings were equal, then the node is
                     * already present and we do not need to insert it.
                     */
                    if(cmp_result == 0)
                    {
                        status = SNMP_ERROR;
                    }

                    /* Break through the loop. */
                    break;
                }

                /* Moving forward in the list. */
                locator = locator->flink;
            }

            /* Add profile table structure to proper position provided it
             * is a new valid entry.
             */
            if (status == SNMP_NOERROR)
            {
                if (locator)
                {
                    DLL_Insert(&Temp_Profile_Mib_Root, profile_ptr,
                        locator);
                }

                else
                {
                    DLL_Enqueue(&Temp_Profile_Mib_Root, profile_ptr);
                }
            }
        }

        /* If memory allocation failed then return error code. */
        else
        {
            NLOG_Error_Log("SNMP: Failed to allocate memory",
                NERR_SEVERE, __FILE__, __LINE__);

            status = SNMP_GENERROR;
        }
    }

    /* If profile table structure already exists then return error. */
    else
    {
        status = SNMP_GENERROR;
    }

    /* Return success or error code. */
    return (status);

} /* Create_snmpNotifyFiltProfEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Get_snmpNotifyFiltProfEntry
*
*   DESCRIPTION
*
*       This function is used to handle SNMP GET and GET-NEXT requests on
*       snmpNotifyFiltProfEntry.
*
*   INPUTS
*
*       *obj                    SNMP object containing request.
*       getflag                 Flag to distinguish GET and GET-NEXT
*                               requests.
*
*   OUTPUTS
*
*       SNMP_NOERROR            Successful
*       SNMP_NOSUCHINSTANCE     Instance specify by indexes does not
*                               exists.
*       SNMP_NOSUCHNAME         The attribute specified in the object
*                               identifier is not supported.
*
*************************************************************************/
STATIC UINT16 Get_snmpNotifyFiltProfEntry(snmp_object_t *obj,
                                               UINT8 getflag)
{
    /* Object identifier of table entry. */
    UINT32              table_oid[] = {1, 3, 6, 1, 6, 3, 13, 1, 2, 1};

    /* Handle to snmpNotifyFiltProfEntry. */
    SNMP_NOTIFY_FILTER_PROFILE_TABLE    *snmp_filter_entry = NU_NULL;

    /* Target Params Name (Index). */
    UINT8               snmp_target_param_name[MAX_FILTER_PROF_NAME_SZE];

    /* Status to return success or error code. */
    UINT16              status = SNMP_NOERROR;

    /* Variable to use in for-loop. */
    UINT32              i;

    /* Clear out the target param name. */
    UTL_Zero(snmp_target_param_name, sizeof(snmp_target_param_name));

    /* Get the value of target param name from the OID. */
    for (i = 0;
         (i < MAX_FILTER_PROF_NAME_SZE) &&
         ((i + SNMP_NO_MIB_FILTER_PRO_SUB_LEN) < obj->IdLen);
         i++)
    {
        snmp_target_param_name[i] =
                    (UINT8)(obj->Id[SNMP_NO_MIB_FILTER_PRO_SUB_LEN + i]);
    }

    /* If we are having GET request. */
    if (getflag)
    {
        /* If we don't have valid target param name then return error
         * code.
         */
        if ( ((i + SNMP_NO_MIB_FILTER_PRO_SUB_LEN) != obj->IdLen) ||
             (i == 0) || (snmp_target_param_name[i - 1] == 0) )
        {
            /* Returning error code. */
            status = SNMP_NOSUCHINSTANCE;
        }
    }

    /* If we are handling GET-NEXT request make target param name, null
     * terminating.
     */
    else if (i < MAX_FILTER_PROF_NAME_SZE)
    {
        snmp_target_param_name[i] = NU_NULL;
    }

    /* If no error till now then get the handle to the
     * snmpNotifyFiltProfEntry.
     */
    if (status == SNMP_NOERROR)
    {
        /* Getting handle to the snmpNotifyFiltProfEntry. */
        snmp_filter_entry = NO_MIB_Get_Filter(snmp_target_param_name,
                                              getflag);
    }

    /* If we got the handle to the snmpNotifyFiltProfEntry. */
    if (snmp_filter_entry)
    {
        switch(obj->Id[SNMP_NO_MIB_FILTER_PRO_ATTR_OFFSET])
        {
        case 1:                     /* snmpNotifyFilterProfileName */

            /* Get the value of 'snmpNotifyFilterProfileName'. */
            strcpy((CHAR*)obj->Syntax.BufChr,
                   (CHAR*)(snmp_filter_entry->
                                    snmp_notify_filter_profile_name));

            /* Get the length of 'snmpNotifyFilterProfileName'. */
            obj->SyntaxLen = strlen((CHAR *)obj->Syntax.BufChr);

            break;

        case 2:                     /* snmpNotifyFilterProfileStorType */

            /* Get the value of 'snmpNotifyFilterProfileStorType'. */
            obj->Syntax.LngUns = (UINT32)(snmp_filter_entry->
                                    snmp_notify_filter_profile_storType);

            break;

        case 3:                     /* snmpNotifyFilterProfileRowStatus */

            /* Get the value of 'snmpNotifyFilterProfileRowStatus'. */
            obj->Syntax.LngUns = (UINT32)(snmp_filter_entry->
                                   snmp_notify_filter_profile_row_status);

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
            for (i = 0 ; (i < MAX_FILTER_PROF_NAME_SZE); i++)
            {
                obj->Id[SNMP_NO_MIB_NOTIFY_SUB_LEN + i] =
                  (UINT32)(snmp_filter_entry->snmp_target_params_name[i]);

                if (snmp_filter_entry->snmp_target_params_name[i] == 0)
                    break;
            }

            /* Update OID length. */
            obj->IdLen = (SNMP_NO_MIB_FILTER_PRO_SUB_LEN + i);
        }
    }

    /* If we did not get the handle to the snmpNotifyFiltProfEntry
     * then return error code.
     */
    else
    {
        /* Returning error code. */
        status = SNMP_NOSUCHINSTANCE;
    }

    /* Return success or error code. */
    return (status);

} /* Get_snmpNotifyFiltProfEntry */

/*************************************************************************
*
*   FUNCTION
*
*       SNMP_MIB_No_Get_Filter_Util
*
*   DESCRIPTION
*
*       This function is used to get handle for the notification filter
*       table entry by specifying the profile name and subtree.
*
*   INPUTS
*
*       *profile_name           Pointer to the memory where profile name
*                               is stored.
*       profile_name_len        Length of profile name.
*       *subtree                Pointer to the memory location where
*                               subtree is stored.
*       subtree_len             Length of subtree.
*       getflag                 Flag to distinguish GET or GET-NEXT
*                               requests.
*
*   OUTPUTS
*
*       SNMP_NOTIFY_FILTER_TABLE *  When there exists the required
*                                   notification filter table entry
*                                   handle.
*       NU_NULL                     When there does not exist the required
*                                   notification filter table entry
*                                   handle.
*
*************************************************************************/
SNMP_NOTIFY_FILTER_TABLE *SNMP_MIB_No_Get_Filter_Util(UINT8 *profile_name,
                     INT profile_name_len, const UINT32 *subtree,
                     UINT32 subtree_len, UINT8 getflag)
{
    /* Handle to the filter table entry. */
    SNMP_NOTIFY_FILTER_TABLE    *location_ptr = Snmp_Filter_Table.flink;
    INT                         cmp_val;

    /* If we are handling GET-Request. */
    if (getflag)
    {
        /* Loop to find filter table entry with same indexes as passed in.
         */
        while (location_ptr)
        {
            /* If we have reached at entry with exact matching indexes as
             * passed in then break through the loop.
             */
            if ((UTL_Admin_String_Cmp((CHAR *)(location_ptr->
                        snmp_notify_filter_profile_name),
                        (CHAR *)profile_name) == 0) &&
                (MibCmpObjId(location_ptr->snmp_notify_filter_subtree,
                            location_ptr->subtree_len, subtree,
                            subtree_len) == 0))
            {
                /* Breaking through the loop. */
                break;
            }

            /* Moving forward in the list. */
            location_ptr = location_ptr->flink;
        }
    }

    /* If we are handling GET-NEXT request. */
    else
    {
        /* Loop to find filter table entry with greater indexes as passed
         * in.
         */
        while (location_ptr)
        {
            /* Comparing profile name of current entry with the profile
             * name passed in.
             */
            cmp_val = strlen((CHAR *)location_ptr->
                      snmp_notify_filter_profile_name) - profile_name_len;

            if (cmp_val == 0)
            {
                cmp_val = strcmp((CHAR *)(location_ptr->
                            snmp_notify_filter_profile_name),
                        (CHAR *)(profile_name));
            }

            /* If we have reached at an entry with greater indexes with
             * respect to what is passed in then break through the loop.
             */
            if ( (cmp_val > 0) ||
                 ((cmp_val == 0) &&
                  (MibCmpObjId(location_ptr->snmp_notify_filter_subtree,
                               location_ptr->subtree_len, subtree,
                               subtree_len) > 0)) )
            {
                /* Break through the loop. */
                break;
            }

            /* Moving forward in the list. */
            location_ptr = location_ptr->flink;
        }
    }

    /* Return handle to the filter table entry if found. Otherwise return
     * NU_NULL.
     */
    return (location_ptr);

} /* SNMP_MIB_No_Get_Filter_Util */

/*************************************************************************
*
*   FUNCTION
*
*       Undo_snmpNotifyFiltProfEntry
*
*   DESCRIPTION
*
*       This function is used handle SNMP UNDO request on notify filter
*       profile table structures.
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
STATIC UINT16 Undo_snmpNotifyFiltProfEntry(const snmp_object_t *obj)
{
    /* Handle to the notify filter profile table structure. */
    SNMP_NOTIFY_FILTER_PROFILE_TABLE *profile_ptr = NU_NULL;

    /* Variable to hold target params name. */
    UINT8                       target_params[MAX_TARG_NAME_SZE];

    /* Status to return success or error code. */
    UINT16                  status = SNMP_NOERROR;

    /* Variable to use in for-loop. */
    UINT32                  i;

    /* Clear out target name. */
    UTL_Zero(target_params, sizeof(target_params));

    /* Get the value of target address name. */
    for (i = 0;
        (i < (MAX_FILTER_PROF_NAME_SZE - 1)) &&
        (SNMP_NO_MIB_FILTER_PRO_SUB_LEN + i < obj->IdLen);
    i++)
    {
        target_params[i] = (UINT8)(obj->Id[SNMP_NO_MIB_FILTER_PRO_SUB_LEN + i]);
    }

    /* If we don't have valid object identifier then return error code. */
    if ((obj->IdLen != (SNMP_NO_MIB_FILTER_PRO_SUB_LEN + i)) ||
        (strlen((CHAR *)target_params) != i))
    {
        /* Return error code. */
        status = SNMP_NOSUCHINSTANCE;
    }

    /* If no error till now then proceed. */
    if (status == SNMP_NOERROR)
    {
        /* Get the target address table entry from temporary list. */
        profile_ptr = NO_MIB_Get_Filter_Util(target_params,
            &Temp_Profile_Mib_Root);

        /* If we got the handle to profile structure from above list. */
        if (profile_ptr)
        {
            /* Remove the handle from the temporary list. */
            DLL_Remove(&Temp_Profile_Mib_Root, profile_ptr);

            /* Deallocate the memory attained by the structure. */
            if (NU_Deallocate_Memory(profile_ptr) != NU_SUCCESS)
            {
                NLOG_Error_Log("SNMP: Failed to deallocate memory",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            /* Set the handle to NU_NULL. */
            profile_ptr = NU_NULL;
        }

        else
        {
            /* Get the handle from permanent list. */
            profile_ptr = NO_MIB_Get_Filter_Util(target_params,
                Snmp_Profile_Table.flink);
        }

        /* If we have the handle to the Profile Table structure. */
        if (profile_ptr)
        {
            /* Clearing the row flag. */
            profile_ptr->snmp_notify_filter_profile_row_flag = 0;

            switch(obj->Id[SNMP_NO_MIB_FILTER_PRO_ATTR_OFFSET])
            {
            case 1:                     /* snmpNotifyFilterProfileName */

                /* If length is not valid for 'snmpNotifyFilterProfileName'
                 * then return error code.
                 */
                if ((obj->SyntaxLen >= SNMP_SIZE_SMALLOBJECTID) ||
                    (obj->SyntaxLen == 0))
                {
                    status = SNMP_WRONGLENGTH;
                }

                /* If the value of 'snmpNotifyFilterProfileName' is not valid
                 * then return error code.
                 */
                else if (strlen((CHAR *)obj->Syntax.BufChr) != obj->SyntaxLen)
                {
                    status = SNMP_WRONGVALUE;
                }

                /* If value to set is valid. */
                else
                {
                    /* Set the value of 'snmpNotifyFilterProfileName'. */
                    strncpy(profile_ptr->snmp_notify_filter_profile_name,
                        (CHAR*)(obj->Syntax.BufChr), MAX_FILTER_PROF_NAME_SZE);
                }

                break;

            case 2:                     /* snmpNotifyFilterProfileStoreType */

                /* If the value of 'snmpNotifyFilterProfileStoreType' is not in
                 * valid range then return error code.
                 */
                if ((obj->Syntax.LngUns < 1) || (obj->Syntax.LngUns > 5))
                {
                    status = SNMP_WRONGVALUE;
                }

                /* If value to set is valid. */
                else
                {
                    /* Set the value of 'snmpNotifyFilterProfileStoreType'. */
                    profile_ptr->snmp_notify_filter_profile_storType =
                        (INT32)obj->Syntax.LngUns;
                }

                break;

            case 3:                     /* snmpNotifyFilterProfileRowStatus */

                /* If the value is not in valid range then return error code. */
                if ((obj->Syntax.LngUns < 1) || (obj->Syntax.LngUns > 6))
                {
                    status = SNMP_WRONGVALUE;
                }

                else if ((profile_ptr->snmp_notify_filter_profile_row_status == 0)
                    || (profile_ptr->snmp_notify_filter_profile_row_status ==
                    (INT32)(obj->Syntax.LngUns)))
                {
                    profile_ptr->snmp_notify_filter_profile_row_status =
                        (INT32)(obj->Syntax.LngUns);
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
    }

    /* Return success or error code. */
    return status;

} /* Undo_snmpNotifyFiltProfEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Commit_snmpNotifyFiltProfEntryStatus
*
*   DESCRIPTION
*
*       This function is used to commit status of Notify filter profile
*       table structure.
*
*   INPUTS
*
*       *table_ptr              Pointer to Profile table structure.
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
STATIC UINT16 Commit_snmpNotifyFiltProfEntryStatus(
    SNMP_NOTIFY_FILTER_PROFILE_TABLE *table_ptr,
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

    /* If target params name or notification filter profile name is not set
     * then set temporary status to 'notReady'. Otherwise set it to
     * 'notInService'.
     */
    if ( memcmp(check_buffer, table_ptr->snmp_target_params_name,
        SNMP_SIZE_SMALLOBJECTID) == 0 ||
        memcmp(check_buffer, table_ptr->snmp_notify_filter_profile_name,
        SNMP_SIZE_SMALLOBJECTID) == 0 )
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
             * However, new entry can be activated by setting its
             * status to CREATEANDGO.
             */
            if((temp_status == SNMP_ROW_NOTINSERVICE) &&
                (!is_new))
            {
                /* Activating the WEP KEY MAPPING. */
                table_ptr->snmp_notify_filter_profile_row_status =
                    SNMP_ROW_ACTIVE;
            }

            else
            {
                /* Setting status to error code of in-consistent value. */
                status = SNMP_INCONSISTANTVALUE;
            }

            break;

        case SNMP_ROW_NOTINSERVICE:

            /* Row status can't be set to NOTINSERVICE for new and
             * not ready entries. However row status can be set to
             * NOTINSERVICE by setting row status to CREATEANDWAIT for
             * new entries.
             */
            if((temp_status == SNMP_ROW_NOTINSERVICE) && (!is_new))
            {
                /* Setting the row status to 'NOTINSERVICE'. */
                table_ptr->snmp_notify_filter_profile_row_status =
                    SNMP_ROW_NOTINSERVICE;
            }

            else
            {
                /* Inconsistent value. */
                status = SNMP_INCONSISTANTVALUE;
            }

            break;

        case SNMP_ROW_DESTROY:

            /* Setting row status to SNMP_ROW_DESTROY. */
            table_ptr->snmp_notify_filter_profile_row_status = SNMP_ROW_DESTROY;

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
            if((temp_status == SNMP_ROW_NOTINSERVICE) && ((is_new) ||
                (table_ptr->snmp_notify_filter_profile_row_status !=
                SNMP_ROW_ACTIVE)))
            {
                /* Activating WEP KEY MAPPING. */
                table_ptr->snmp_notify_filter_profile_row_status =
                    SNMP_ROW_ACTIVE;
            }

            else
            {
                /* Setting status to error code of in-consistent value. */
                status = SNMP_INCONSISTANTVALUE;
            }

            break;

        case SNMP_ROW_CREATEANDWAIT:

            /* Entry should not be 'active'. */
            if(table_ptr->snmp_notify_filter_profile_row_status !=
                SNMP_ROW_ACTIVE)
            {
                table_ptr->snmp_notify_filter_profile_row_status =
                    (INT32)temp_status;
            }

            else
            {
                /* Setting status to error code of in-consistent value. */
                status = SNMP_INCONSISTANTVALUE;
            }

            break;

        default:

            if ((table_ptr->snmp_notify_filter_profile_row_flag == 0) ||
                (temp_status == SNMP_ROW_NOTREADY) ||
                (table_ptr->snmp_notify_filter_profile_row_flag ==
                SNMP_ROW_DESTROY))
            {
                table_ptr->snmp_notify_filter_profile_row_status =
                    (INT32)temp_status;
            }

            else if (table_ptr->snmp_notify_filter_profile_row_flag ==
                SNMP_ROW_CREATEANDGO)
            {
                if (temp_status == SNMP_ROW_NOTINSERVICE)
                    table_ptr->snmp_notify_filter_profile_row_status =
                    SNMP_ROW_ACTIVE;
                else
                    status = SNMP_INCONSISTANTVALUE;
            }

            else if (table_ptr->snmp_notify_filter_profile_row_flag ==
                SNMP_ROW_CREATEANDWAIT)
            {
                table_ptr->snmp_notify_filter_profile_row_status =
                    (INT32)temp_status;
            }
        }
    }

    /* Return success or error code. */
    return (status);

} /* Commit_snmpNotifyFiltProfEntryStatus */

/*************************************************************************
*
*   FUNCTION
*
*       Commit_snmpNotifyFiltProfEntries
*
*   DESCRIPTION
*
*       This function is used to commit all the newly created Notification
*       filter profile table structures.
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
STATIC VOID Commit_snmpNotifyFiltProfEntries(VOID)
{
    /* Handle to the profile table structure. */
    SNMP_NOTIFY_FILTER_PROFILE_TABLE *profile_ptr = Temp_Profile_Mib_Root.flink;

    /* Loop till there exists an entry in temporary list. */
    while(profile_ptr)
    {
        /* Add profile table structure into permanent list. */
        if (SNMP_Add_To_Profile_Tbl(profile_ptr) != NU_SUCCESS)
        {
            NLOG_Error_Log("SNMP: Failed to Add profile table",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* Remove a profile table structure from the temporary list. */
        DLL_Dequeue(&Temp_Profile_Mib_Root);

        /* Deallocate memory of above structure as SNMP_Add_To_Profile_Tbl
         * creates its own copy.
         */
        if (NU_Deallocate_Memory(profile_ptr) != NU_SUCCESS)
        {
            NLOG_Error_Log("SNMP: Failed to deallocate memory",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* Getting next handle to the table structure from the temporary
         * list.
         */
        profile_ptr = Temp_Profile_Mib_Root.flink;
    }

    profile_ptr = Snmp_Profile_Table.flink;

    while(profile_ptr)
    {
        /* Clear out the row flag. */
        profile_ptr->snmp_notify_filter_profile_row_flag = 0;

        /* Moving forward in the list. */
        profile_ptr = profile_ptr->flink;
    }

} /* Commit_snmpNotifyFiltProfEntries */

/*************************************************************************
*
*   FUNCTION
*
*       snmpNotifyFiltProfEntry
*
*   DESCRIPTION
*
*       This function is used to handle SNMP request on
*       snmpNotifyFiltProfEntry.
*
*   INPUTS
*
*       *obj                SNMP object containing request.
*       idlen               Not used.
*       *param              Not used.
*
*   OUTPUTS
*
*       SNMP_NOERROR            Successful
*       SNMP_NOSUCHINSTANCE     Instance specify by indexes does not
*                               exists.
*       SNMP_NOSUCHNAME         The attribute specified in the object
*                               identifier is not supported.
*       SNMP_GENERROR           Invalid request.
*
*************************************************************************/
UINT16 snmpNotifyFiltProfEntry(snmp_object_t *obj, UINT16 idlen,
                               VOID *param)
{
    /* Status to return success or error code. */
    UINT16          status = SNMP_NOERROR;

    /* Flag to distinguish between get and get-next requests. */
    UINT8           getflag = 0;

    /* Handle to the notification filter profile entry. */
    SNMP_NOTIFY_FILTER_PROFILE_TABLE   *profile_entry;

    /* Handle to the temporary notification filter profile entry. */
    SNMP_NOTIFY_FILTER_PROFILE_TABLE   *temp_profile_entry;

    /* Variable to hold target params name. */
    UINT8                              target_params[MAX_FILTER_PROF_NAME_SZE];

    /* Variable to use in for-loop. */
    UINT32                      i, current_state;

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
        status = Get_snmpNotifyFiltProfEntry(obj, getflag);

        break;

    case SNMP_PDU_BULK:                     /* Get-bulk request. */

        /* Processing of bulk operation. */
        status = SNMP_Get_Bulk(obj, Get_snmpNotifyFiltProfEntry);

        break;

    case SNMP_PDU_SET:                      /* Set request. */

        Profile_Table_MIB_Commit_Left++;

        /* Process the SET operation. */
        status = Set_snmpNotifyFiltProfEntry(obj);

        if (status != SNMP_NOSUCHINSTANCE)
            break;

    case SNMP_PDU_CREATE:                   /* Create request. */

        /* Processing of create operation. */
        status = Create_snmpNotifyFiltProfEntry(obj);

        /* If the entry was successfully created, set the value. */
        if (status == SNMP_NOERROR)
            status = Set_snmpNotifyFiltProfEntry(obj);

        break;

    case SNMP_PDU_UNDO:                     /* Undo request. */

        Profile_Table_MIB_Commit_Left = 0;

        /* Processing of UNDO operations. */
        status = Undo_snmpNotifyFiltProfEntry(obj);

        break;

    case SNMP_PDU_COMMIT:                   /* Commit request. */

        Profile_Table_MIB_Commit_Left--;

        /* Clear out target params name. */
        UTL_Zero(target_params, sizeof(target_params));

        /* Get the value of target params name. */
        for (i = 0;
            (i < (MAX_FILTER_PROF_NAME_SZE - 1)) &&
            (SNMP_NO_MIB_FILTER_PRO_SUB_LEN + i < obj->IdLen);
        i++)
        {
            target_params[i] = (UINT8)(obj->Id[SNMP_NO_MIB_FILTER_PRO_SUB_LEN + i]);
        }

        /* If we don't have valid object identifier then return error
         * code.
         */
        if ((obj->IdLen != (SNMP_NO_MIB_FILTER_PRO_SUB_LEN + i)) ||
            (strlen((CHAR *)target_params) != i))
        {
            /* Return error code. */
            status = SNMP_NOSUCHINSTANCE;
        }

        /* If no error till now then proceed. */
        if (status == NU_SUCCESS)
        {
            /* Get the handle to the notification filter profile table entry
             * from temporary list.
             */
            profile_entry = NO_MIB_Get_Filter_Util(target_params,
                &Temp_Profile_Mib_Root);

            /* Save the handle in temporary one. */
            temp_profile_entry = profile_entry;

            /* If we did not get handle to the Profile table from
             * the temporary list then get it from the permanent list.
             */
            if (!profile_entry)
            {
                /* Get the handle from the permanent list. */
                profile_entry = NO_MIB_Get_Filter_Util(target_params,
                    Snmp_Profile_Table.flink);
            }

            /* If we got the handle either from permanent or temporary list.
             */
            if (profile_entry)
            {
                /* Check whether it was a row set operation. */
                if (obj->Id[SNMP_NO_MIB_FILTER_PRO_ATTR_OFFSET] == 3)
                {
                    /* Update the value of current state.
                     *  0) New entry.
                     *  1) Not ready.
                     *  2) Not In service.
                     *  3) Active
                     */

                    /* If we have new entry the set current state to 0. */
                    if (temp_profile_entry)
                    {
                        current_state = 0;
                    }

                    /* If we have new not ready entry then set current state
                     * to 1.
                     */
                    else if ((strlen((CHAR *)profile_entry->
                         snmp_notify_filter_profile_name) == 0))
                    {
                        current_state = 1;
                    }

                    /* If we have not in service entry then set current state
                     * to 2.
                     */
                    else if (profile_entry->
                       snmp_notify_filter_profile_row_status != SNMP_ROW_ACTIVE)
                    {
                        current_state = 2;
                    }

                    /* If we have active entry then set current state to 3. */
                    else
                    {
                        current_state = 3;
                    }

                    /* If required state transition is not allowed then
                     * return error code.
                     */
                    if (status_trans[obj->Syntax.LngUns - 1][current_state]
                    != SNMP_NOERROR)
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
                        status = Commit_snmpNotifyFiltProfEntryStatus(
                            profile_entry, (UINT8)(obj->Syntax.LngUns),
                            (UINT8)(temp_profile_entry != NU_NULL) );
                    }
                }

                else
                {
                    status = Commit_snmpNotifyFiltProfEntryStatus(
                        profile_entry, 0, NU_FALSE);
                }

                /* If the row status is set to 'DESTROY'. */
                if (profile_entry->snmp_notify_filter_profile_row_status ==
                    SNMP_ROW_DESTROY)
                {
                    /* If handle was from temporary list then remove it
                     * from temporary list.
                     */
                    if (temp_profile_entry)
                        DLL_Remove(&Temp_Profile_Mib_Root, profile_entry);

                    /* If handle was from permanent list then remove it from
                     * permanent list.
                     */
                    else
                        DLL_Remove(&Snmp_Profile_Table.flink, profile_entry);

                    /* Deallocate memory attained by this profile table
                     * structure.
                     */
                    if (NU_Deallocate_Memory(profile_entry) !=
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
            if ((status == SNMP_NOERROR) && (Profile_Table_MIB_Commit_Left == 0))
            {
                Commit_snmpNotifyFiltProfEntries();
            }
        }

        break;

    default:                                /* Invalid request. */
        status = SNMP_GENERROR;
    }

    /* Return success or error code. */
    return (status);

} /* snmpNotifyFiltProfEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Get_snmpNotifyFilterEntry
*
*   DESCRIPTION
*
*       This function is used to handle SNMP GET and GET-NEXT requests on
*       snmpNotifyFilterEntry.
*
*   INPUTS
*
*       *obj                    SNMP object containing request.
*       getflag                 Flag to distinguish GET and GET-NEXT
*                               requests.
*
*   OUTPUTS
*
*       SNMP_NOERROR            Successful
*       SNMP_NOSUCHINSTANCE     Instance specify by indexes does not
*                               exists.
*       SNMP_NOSUCHNAME         The attribute specified in the object
*                               identifier is not supported.
*
*************************************************************************/
STATIC UINT16 Get_snmpNotifyFilterEntry(snmp_object_t *obj, UINT8 getflag)
{
    /* Object identifier of table entry. */
    UINT32              table_oid[] = {1, 3, 6, 1, 6, 3, 13, 1, 3, 1};

    /* Handle to snmpNotifyFilterEntry. */
    SNMP_NOTIFY_FILTER_TABLE    *snmp_filter_entry = NU_NULL;

    /* Filter Profile Name (Index). */
    UINT8               filter_prof_name[MAX_FILTER_PROF_NAME_SZE];

    /* Filter Profile Name length. */
    UINT32              filter_prof_name_len;

    /* Filter subtree. */
    UINT32              filter_subtree[SNMP_SIZE_OBJECTID];

    /* Filter subtree length. */
    UINT32              filter_subtree_len = 0;

    /* Status to return success or error code. */
    UINT16              status = SNMP_NOERROR;

    /* Variable to use in for-loop. */
    UINT32              i;

    /* Temporary variable. */
    UINT32              temp_index;

    /* Clear out the filter profile name. */
    UTL_Zero(filter_prof_name, sizeof(filter_prof_name));

    /* Clear out the filter profile subtree. */
    UTL_Zero(filter_subtree, sizeof(filter_subtree));

    /* Get filter profile name length. */
    filter_prof_name_len = obj->Id[SNMP_NO_MIB_FILTER_SUB_LEN];

    /* If filter profile name is out of range then return error code. */
    if (filter_prof_name_len >= MAX_FILTER_PROF_NAME_SZE)
    {
        /* Returning error code. */
        status = SNMP_NOSUCHINSTANCE;
    }
    else
    {
        /* Get the value of filter profile name from the OID. */
        for (i = 0;
             (i < filter_prof_name_len) &&
             ((i + SNMP_NO_MIB_FILTER_SUB_LEN + 1) < obj->IdLen);
             i++)
        {
            filter_prof_name[i] =
                        (UINT8)(obj->Id[SNMP_NO_MIB_FILTER_SUB_LEN
                                            + 1 + i]);
        }

        /* If the OID is not valid then return error code and we are
         * handling GET request then return error code.
         */
        if ((i != filter_prof_name_len) && (getflag))
        {
            /* Returning error code. */
            status = SNMP_NOSUCHINSTANCE;
        }
        else
        {
            /* Get starting index for subtree value in OID. */
            temp_index = SNMP_NO_MIB_FILTER_SUB_LEN + 1 + i;

            /* Get the value of subtree from OID. */
            for (filter_subtree_len = 0;
                 (filter_subtree_len <
                                (SNMP_NO_MIB_FILTER_SUB_LEN -1)) &&
                 ((temp_index + filter_subtree_len) < obj->IdLen);
                 filter_subtree_len++)
            {
                filter_subtree[filter_subtree_len] =
                        obj->Id[temp_index + filter_subtree_len];
            }
        }
    }

    /* If we are having GET request. */
    if (getflag)
    {
        /* If we don't have valid target param name then return error
         * code.
         */
        if (((filter_prof_name_len + 1 + SNMP_NO_MIB_FILTER_SUB_LEN +
                filter_subtree_len) != obj->IdLen) &&
            (strlen((CHAR *)filter_prof_name) != filter_prof_name_len))
        {
            /* Returning error code. */
            status = SNMP_NOSUCHINSTANCE;
        }
    }

    /* If no error till now then get the handle to the
     * snmpNotifyFilterEntry.
     */
    if (status == SNMP_NOERROR)
    {
        /* Getting handle to the snmpNotifyFilterEntry. */
        snmp_filter_entry = SNMP_MIB_No_Get_Filter_Util(filter_prof_name,
                                               (INT)filter_prof_name_len,
                                               filter_subtree,
                                               filter_subtree_len,
                                               getflag);
    }

    /* If we got the handle to the snmpNotifyFilterEntry. */
    if (snmp_filter_entry)
    {
        switch(obj->Id[SNMP_NO_MIB_FILTER_ATTR_OFFSET])
        {
        case 2:                         /* snmpNotifyFilterMask */

            /* Get the value of 'snmpNotifyFilterMask'. */
            NU_BLOCK_COPY(obj->Syntax.BufChr,
                          snmp_filter_entry->snmp_notify_filter_mask,
                          snmp_filter_entry->mask_len);

            obj->SyntaxLen = snmp_filter_entry->mask_len;

            break;

        case 3:                         /* snmpNotifyFilterType */

            /* Get the value of 'snmpNotifyFilterType'. */
            obj->Syntax.LngUns = (UINT32)(snmp_filter_entry->
                                                snmp_notify_filter_type);

            break;

        case 4:                         /* snmpNotifyFilterStorageType */

            /* Get the value of 'snmpNotifyFilterStorageType'. */
            obj->Syntax.LngUns =
                    snmp_filter_entry->snmp_notify_filter_storage_type;

            break;

        case 5:                         /* snmpNotifyFilterRowStatus */

            /* Get the value of 'snmpNotifyFilterRowStatus'. */
            obj->Syntax.LngUns =
                        snmp_filter_entry->snmp_notify_filter_row_status;

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

            /* Update the filter profile name length in the Object OID. */
            obj->Id[SNMP_NO_MIB_FILTER_SUB_LEN] =
               strlen(snmp_filter_entry->snmp_notify_filter_profile_name);

            /* Update filter profile name in object OID. */
            for (i = 0; i < obj->Id[SNMP_NO_MIB_FILTER_SUB_LEN]; i++)
            {
                obj->Id[SNMP_NO_MIB_FILTER_SUB_LEN + 1 + i] = (UINT32)(
                   snmp_filter_entry->snmp_notify_filter_profile_name[i]);
            }

            /* Get starting index for subtree. */
            temp_index = SNMP_NO_MIB_FILTER_SUB_LEN + 1 + i;

            /* Update value of subtree in object OID. */
            for (i = 0; i < snmp_filter_entry->subtree_len; i++)
            {
                obj->Id[temp_index + i] = snmp_filter_entry->
                                        snmp_notify_filter_subtree[i];
            }

            /* Update OID length. */
            obj->IdLen = temp_index + i;
        }
    }

    /* If we did not get the handle to the snmpNotifyFilterEntry
     * then return error code.
     */
    else
    {
        /* Returning error code. */
        status = SNMP_NOSUCHINSTANCE;
    }

    /* Return success or error code. */
    return (status);

} /* Get_snmpNotifyFilterEntry */

/*************************************************************************
*
*   FUNCTION
*
*       snmpNotifyFilterEntry
*
*   DESCRIPTION
*
*       This function is used to handle SNMP request on
*       snmpNotifyFilterEntry.
*
*   INPUTS
*
*       *obj                SNMP object containing request.
*       idlen               Not used.
*       *param              Not used.
*
*   OUTPUTS
*
*       SNMP_NOERROR            Successful
*       SNMP_NOSUCHINSTANCE     Instance specify by indexes does not
*                               exists.
*       SNMP_NOSUCHNAME         The attribute specified in the object
*                               identifier is not supported.
*       SNMP_GENERROR           Invalid request.
*
*************************************************************************/
UINT16 snmpNotifyFilterEntry(snmp_object_t *obj, UINT16 idlen,
                             VOID *param)
{
    /* Status to return success or error code. */
    UINT16          status;

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
        status = Get_snmpNotifyFilterEntry(obj, getflag);

        break;

    case SNMP_PDU_BULK:                     /* Get-bulk request. */

        /* Processing of bulk operation. */
        status = SNMP_Get_Bulk(obj, Get_snmpNotifyFilterEntry);

        break;

    case SNMP_PDU_SET:                      /* Set request. */
    case SNMP_PDU_UNDO:                     /* Undo request. */
    case SNMP_PDU_COMMIT:                   /* Commit request. */
    default:                                /* Invalid request. */
        status = SNMP_GENERROR;
    }

    /* Return success or error code. */
    return (status);

} /* snmpNotifyFiltProfEntry */

#endif /* (INCLUDE_MIB_NO == NU_TRUE) */
