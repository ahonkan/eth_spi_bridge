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
*       vacm_mib.c                                               
*
*   COMPONENT
*
*       VACM - MIB
*
*   DESCRIPTION
*
*       This file contains the implementation for the VACM-MIB.
*
*   DATA STRUCTURES
*
*       Vacm_Mib_Temp_Sec2Group_Table
*       VACM_Mib_Temp_Access_Root
*       Vacm_Mib_Temp_View_Family_Root
*       VACM_Mib_SEC2GROUP_Commit_Left
*       VACM_Mib_Access_Commit_Left
*       VACM_Mib_View_Tree_Commit_Left
*
*   FUNCTIONS
*
*       Get_vacmContextEntry
*       vacmContextEntry
*       Get_vacmSec2GroupEntry
*       Set_vacmSec2GroupEntry
*       Undo_vacmSec2GroupEntry
*       Create_vacmSec2GroupEntry
*       Commit_vacmSec2GroupEntryStatus
*       Commit_vacmSec2GroupEntries
*       vacmSec2GroupEntry
*       Get_vacmAccessEntry
*       Set_vacmAccessEntry
*       Undo_vacmAccessEntry
*       Create_vacmAccessEntry
*       Commit_vacmAccessEntryStatus
*       Commit_vacmAccessEntries
*       vacmAccessEntry
*       vacmViewSpinLock
*       Get_vacmViewTreeEntry
*       Set_vacmViewTreeEntry
*       Undo_vacmViewTreeEntry
*       Create_vacmViewTreeEntry
*       Commit_vacmViewTreeEntryStatus
*       Commit_vacmViewTreeEntries
*       vacmViewTreeEntry
*
*   DEPENDENCIES
*
*       target.h
*       externs.h
*       xtypes.h
*       snmp.h
*       agent.h
*       mib.h
*       vacm.h
*       vacm_mib.h
*       snmp_g.h
*
*************************************************************************/
#include "networking/target.h"
#include "networking/externs.h"
#include "networking/xtypes.h"
#include "networking/snmp.h"
#include "networking/agent.h"
#include "networking/mib.h"
#include "networking/vacm.h"
#include "networking/vacm_mib.h"
#include "networking/snmp_g.h"

#if (INCLUDE_MIB_VACM == NU_TRUE)

#define VACM_CONTEXT_NAME_SUB_LEN       11
#define VACM_CONTEXT_NAME_ATTR_OFFSET   10
#define VACM_SEC2GROUP_SUB_LEN          11
#define VACM_SEC2GROUP_ATTR_OFFSET      10
#define VACM_ACCESS_SUB_LEN             11
#define VACM_ACCESS_ATTR_OFFSET         10
#define VACM_VIEW_TREE_SUB_LEN          12
#define VACM_VIEW_TREE_ATTR_OFFSET      11

extern VACM_MIB_STRUCT      Vacm_Mib_Table;
VACM_SEC2GROUP_ROOT         Vacm_Mib_Temp_Sec2Group_Table;
VACM_ACCESS_ROOT            VACM_Mib_Temp_Access_Root;
VACM_VIEWTREE_ROOT          Vacm_Mib_Temp_View_Family_Root;
STATIC INT                  VACM_Mib_SEC2GROUP_Commit_Left;
STATIC INT                  VACM_Mib_Access_Commit_Left;
STATIC INT                  VACM_Mib_View_Tree_Commit_Left;

STATIC VOID Commit_vacmSec2GroupEntries(VOID);

/*************************************************************************
*
*   FUNCTION
*
*       Get_vacmContextEntry
*
*   DESCRIPTION
*
*       This function is used to process SNMP GET/GET-NEXT request on
*       vacmContextTable.
*
*   INPUTS
*
*       *obj                SNMP object containing request.
*       getflag             Flag to distinguish GET and GET-NEXT requests.
*
*   OUTPUTS
*
*       SNMP_NOERROR        Successful.
*       SNMP_NOSUCHINSTANCE Required instance of object is not found.
*       SNMP_NOSUCHNAME     SNMP Object not found.
*
*************************************************************************/
STATIC UINT16 Get_vacmContextEntry(snmp_object_t *obj, UINT8 getflag)
{
    /* Object identifier of table entry. */
    UINT32                  table_oid[] = {1, 3, 6, 1, 6, 3, 16, 1, 1, 1};

    /* Handle to the context name entry. */
    VACM_CONTEXT_STRUCT     *location_ptr = NU_NULL;

    /* Context name. */
    UINT8                   context_name[SNMP_SIZE_SMALLOBJECTID];

    /* Length of context name. */
    UINT32                  context_name_len;

    /* Variable to use in for-loop. */
    UINT32                  i;

    /* Status to return success or error code. */
    UINT16                  status = SNMP_NOERROR;

    /* Clear out context name. */
    UTL_Zero(context_name, sizeof(context_name));

    /* Get the length of context name from object identifier. */
    context_name_len = obj->Id[VACM_CONTEXT_NAME_SUB_LEN];

    /* Get the value of context name from object identifier. */
    for (i = 0;
         ((i < context_name_len) &&
          (i < SNMP_SIZE_SMALLOBJECTID) &&
          ((VACM_CONTEXT_NAME_SUB_LEN + 1 + i) < obj->IdLen));
         i++)
    {
        context_name[i] =
                    (UINT8)(obj->Id[VACM_CONTEXT_NAME_SUB_LEN + 1 + i]);
    }

    if (getflag)
    {
        /* If we don't have valid Object Identifier then return error
         * code.
         */
        if ((i != context_name_len) ||
            ((VACM_CONTEXT_NAME_SUB_LEN + 1 + i) != obj->IdLen) ||
            (context_name_len != strlen((CHAR *)context_name) ))
        {
            /* Return error code. */
            status = SNMP_NOSUCHINSTANCE;
        }
    }

    /* If no error till now then get handle to the context name entry. */
    if (status == SNMP_NOERROR)
    {
        /* If we are interested in first entry then set 'getflag' to
         * represent it.
         */
        if ((context_name_len == 0) && (!getflag) &&
            (obj->IdLen <= VACM_CONTEXT_NAME_SUB_LEN))
        {
            getflag = 2;
        }

        /* Get the handle to the context name entry. */
        location_ptr = VACM_Get_Context_Name_Util(context_name,
                                                  context_name_len,
                                                  getflag);

        /* If set the 'getflag' to represent that we need to get first
         * entry then revert back it to its original value.
         */
        if (getflag == 2)
            getflag = 0;
    }

    /* If we get the handle to the context entry then proceed. */
    if (location_ptr)
    {
        switch(obj->Id[VACM_CONTEXT_NAME_ATTR_OFFSET])
        {
        case 1:                         /* vacmContextName */

            /* Get the value of 'vacmContextName'. */
            strcpy((CHAR *)(obj->Syntax.BufChr),
                   location_ptr->context_name);

            /* Update the length of 'vacmContextName'. */
            obj->SyntaxLen = strlen(location_ptr->context_name);

            break;

        default:
            /* We have reached at end table. */
            status = SNMP_NOSUCHNAME;
        }

        /* If request was successful and we are handling GET-NEXT request
         * then update the object identifier.
         */
        if ((status == SNMP_NOERROR) && (!getflag))
        {
            /* Update table OID in object identifier. */
            NU_BLOCK_COPY(obj->Id, table_oid, sizeof(table_oid));

            /* Update context name length in object identifier. */
            obj->Id[VACM_CONTEXT_NAME_SUB_LEN] =
                                    strlen(location_ptr->context_name);

            /* Update context name in object identifier. */
            for (i = 0; i < obj->Id[VACM_CONTEXT_NAME_SUB_LEN]; i++)
            {
                obj->Id[VACM_CONTEXT_NAME_SUB_LEN + i + 1] =
                                  (UINT32)(location_ptr->context_name[i]);
            }

            /* Update length of object identifier. */
            obj->IdLen = VACM_CONTEXT_NAME_SUB_LEN + i + 1;
        }
    }

    /* If we did not get handle to the context name entry then return
     * error code.
     */
    else
    {
        /* Return error code. */
        status = SNMP_NOSUCHINSTANCE;
    }

    /* Return success or error code. */
    return (status);
} /* Get_vacmContextEntry */

/*************************************************************************
*
*   FUNCTION
*
*       vacmContextEntry
*
*   DESCRIPTION
*
*       This function is used to handle SNMP request on vacmContextEntry.
*
*   INPUTS
*
*       *obj                SNMP object containing request.
*       idlen               Not used.
*       *param              Not used.
*
*   OUTPUTS
*
*       SNMP_NOERROR        Successful.
*       SNMP_NOSUCHINSTANCE Required instance of object is not found.
*       SNMP_NOSUCHNAME     SNMP Object not found.
*       SNMP_GENERROR       Invalid request.
*
*************************************************************************/
UINT16 vacmContextEntry(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    /* Status to return success or error code. */
    UINT16              status;

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
        status = Get_vacmContextEntry(obj, getflag);

        break;

    case SNMP_PDU_BULK:                     /* Get-bulk request. */

        /* Processing of bulk operation. */
        status = SNMP_Get_Bulk(obj, Get_vacmContextEntry);

        break;

    case SNMP_PDU_SET:                      /* Set request. */
    case SNMP_PDU_UNDO:                     /* Undo request. */
    case SNMP_PDU_COMMIT:                   /* Commit request. */
    default:                                /* Invalid request. */
        status = SNMP_GENERROR;
    }

    /* Return success or error code. */
    return (status);

} /* vacmContextEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Get_vacmSec2GroupEntry
*
*   DESCRIPTION
*
*       This function is used to process SNMP GET/GET-NEXT request on
*       vacmSecurityToGroupTable.
*
*   INPUTS
*
*       *obj                SNMP object containing request.
*       getflag             Flag to distinguish GET and GET-NEXT requests.
*
*   OUTPUTS
*
*       SNMP_NOERROR        Successful.
*       SNMP_NOSUCHINSTANCE Required instance of object is not found.
*       SNMP_NOSUCHNAME     SNMP Object not found.
*
*************************************************************************/
STATIC UINT16 Get_vacmSec2GroupEntry(snmp_object_t *obj,
                                           UINT8 getflag)
{
    /* Object identifier of table entry. */
    UINT32                  table_oid[] = {1, 3, 6, 1, 6, 3, 16, 1, 2, 1};

    /* VACM security model. */
    UINT32                  vacm_sec_model;

    /* VACM security name. */
    UINT8                   vacm_sec_name[SNMP_SIZE_SMALLOBJECTID];

    /* VACM security name length. */
    UINT32                  vacm_sec_name_len;

    /* Handle to the vacmSec2GroupEntry. */
    VACM_SEC2GROUP_STRUCT   *vacm_sec2group_entry = NU_NULL;

    /* Variable to use in for-loop. */
    UINT32                  i;

    /* Status to return success or error code. */
    UINT16                  status = SNMP_NOERROR;

    /* Get the value of security model from object identifier. */
    vacm_sec_model = obj->Id[VACM_SEC2GROUP_SUB_LEN];

    /* Get the value of security name length from the object identifier.
     */
    vacm_sec_name_len = obj->Id[VACM_SEC2GROUP_SUB_LEN + 1];

    /* Clear out security name. */
    UTL_Zero(vacm_sec_name, sizeof(vacm_sec_name));

    /* Loop to get the value of security name. */
    for (i = 0;
         ((i < vacm_sec_name_len) &&
          (i < (SNMP_SIZE_SMALLOBJECTID - 1)) &&
          ((VACM_SEC2GROUP_SUB_LEN + 2 + i) < obj->IdLen));
         i++)
    {
        vacm_sec_name[i] =
                        (UINT8)(obj->Id[VACM_SEC2GROUP_SUB_LEN + 2 + i]);
    }

    /* If we are handling GET request. */
    if (getflag)
    {
        /* If we have invalid OID then return error code. */
        if ((i != vacm_sec_name_len) ||
            ((VACM_SEC2GROUP_SUB_LEN + 2 + i) != obj->IdLen) ||
            (strlen((CHAR *)vacm_sec_name) != vacm_sec_name_len))
        {
            status = SNMP_NOSUCHINSTANCE;
        }
    }

    /* If no error till now then get the handle to the
     * vacmSec2GroupEntry.
     */
    if (status == SNMP_NOERROR)
    {
        /* Get handle to vacmSec2GroupEntry. */
        vacm_sec2group_entry = VACM_Get_Sec2Group_Entry(vacm_sec_model,
                                                        vacm_sec_name_len,
                                                        vacm_sec_name,
                                                        getflag);
    }

    /* If we got handle to vacmSec2GroupEntry then proceed.
     * Otherwise return error code.
     */
    if (vacm_sec2group_entry)
    {
        switch(obj->Id[VACM_SEC2GROUP_ATTR_OFFSET])
        {
        case 3:                     /* vacmGroupName */

            /* Get value of 'vacmGroupName'. */
            strcpy((CHAR *)(obj->Syntax.BufChr),
                   (CHAR *)(vacm_sec2group_entry->vacm_group_name));

            /* Get length of 'vacmGroupName'. */
            obj->SyntaxLen = strlen((CHAR *)(vacm_sec2group_entry->
                                                        vacm_group_name));

            break;

        case 4:                     /* vacmSecurityToGroupStorageType */

            /* Get the value of 'vacmSecurityToGroupStorageType'. */
            obj->Syntax.LngUns = vacm_sec2group_entry->vacm_storage_type;

            break;
        case 5:                     /* vacmSecurityToGroupStatus */

            /* Get the value of 'vacmSecurityToGroupStatus'. */
            obj->Syntax.LngUns = vacm_sec2group_entry->vacm_status;

            break;

        default:
            /* We have reached at end of the table. */
            status = SNMP_NOSUCHNAME;
        }

        if ((status == SNMP_NOERROR) && (!getflag))
        {
            /* Update table OID in object identifier. */
            NU_BLOCK_COPY(obj->Id, table_oid, sizeof(table_oid));

            /* Update security model in object identifier. */
            obj->Id[VACM_SEC2GROUP_SUB_LEN] =
                                vacm_sec2group_entry->vacm_security_model;

            /* Update security name length in object identifier. */
            obj->Id[VACM_SEC2GROUP_SUB_LEN + 1] =
                strlen((CHAR *)vacm_sec2group_entry->vacm_security_name);

            /* Update security name in object identifier. */
            for (i = 0; i < obj->Id[VACM_SEC2GROUP_SUB_LEN + 1]; i++)
            {
                obj->Id[VACM_SEC2GROUP_SUB_LEN + 2 + i] =
                            vacm_sec2group_entry->vacm_security_name[i];
            }

            /* Update object identifier length. */
            obj->IdLen = (VACM_SEC2GROUP_SUB_LEN + 2 + i);
        }
    }

    /* If we did not get handle to vacmSec2GroupEntry then return
     * error code.
     */
    else
    {
        /* Returning error code. */
        status = SNMP_NOSUCHINSTANCE;
    }

    /* Returning success or error code. */
    return (status);

} /* Get_vacmSec2GroupEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Set_vacmSec2GroupEntry
*
*   DESCRIPTION
*
*       This function is used to process SNMP SET request on
*       vacmSecurityToGroupTable.
*
*   INPUTS
*
*       *obj                SNMP object containing request.
*
*   OUTPUTS
*
*       SNMP_NOERROR            Successful.
*       SNMP_NOSUCHINSTANCE     Required instance of object is not found.
*       SNMP_NOSUCHNAME         SNMP Object not found.
*       SNMP_INCONSISTANTVALUE  Value to set is inconsistent.
*       SNMP_READONLY           Instance has storage type of read-only.
*       SNMP_WRONGVALUE         Wrong value.
*       SNMP_WRONGLENGTH        Wrong length.
*
*************************************************************************/
STATIC UINT16 Set_vacmSec2GroupEntry(snmp_object_t *obj)
{
    /* VACM security model. */
    UINT32                  vacm_sec_model;

    /* VACM security name length. */
    UINT32                  vacm_sec_name_len;

    /* Handle to the vacmSec2GroupEntry. */
    VACM_SEC2GROUP_STRUCT   *vacm_sec2group_entry = NU_NULL;

    /* VACM security name. */
    UINT8                   vacm_sec_name[SNMP_SIZE_SMALLOBJECTID];

    /* Status to return success or error code. */
    UINT16                  status = SNMP_NOERROR;

    /* Variable to use in for-loop. */
    UINT32                  i;

    /* Get the value of security model from object identifier. */
    vacm_sec_model = obj->Id[VACM_SEC2GROUP_SUB_LEN];

    /* Get the value of security name length from the object identifier.
     */
    vacm_sec_name_len = obj->Id[VACM_SEC2GROUP_SUB_LEN + 1];

    /* Clear out security name. */
    UTL_Zero(vacm_sec_name, sizeof(vacm_sec_name));

    /* Loop to get the value of security name. */
    for (i = 0;
         ((i < vacm_sec_name_len) &&
          (i < (SNMP_SIZE_SMALLOBJECTID - 1)) &&
          ((VACM_SEC2GROUP_SUB_LEN + 2 + i) < obj->IdLen));
         i++)
    {
        vacm_sec_name[i] =
                        (UINT8)(obj->Id[VACM_SEC2GROUP_SUB_LEN + 2 + i]);
    }

    if ((vacm_sec_model == 0) || (vacm_sec_name_len == 0) ||
        (vacm_sec_name_len > SNMP_SIZE_SMALLOBJECTID))
    {
        status = SNMP_NOCREATION;
    }

    /* If we have invalid OID then return error code. */
    else if ((i != vacm_sec_name_len) ||
         ((VACM_SEC2GROUP_SUB_LEN + 2 + i) != obj->IdLen) ||
         (strlen((CHAR *)vacm_sec_name) != vacm_sec_name_len) )
    {
        status = SNMP_NOSUCHINSTANCE;
    }

    if (status == SNMP_NOERROR)
    {
        vacm_sec2group_entry = VACM_Get_Sec2Group_Entry_Util(
                        vacm_sec_model, vacm_sec_name_len, vacm_sec_name,
                        &(Vacm_Mib_Table.vacm_security_to_group_tab));

        if ((vacm_sec2group_entry) &&
            (vacm_sec2group_entry->vacm_storage_type ==
                                                SNMP_STORAGE_READONLY))
        {
            status = SNMP_READONLY;
        }

        else if (!vacm_sec2group_entry)
        {
            vacm_sec2group_entry = VACM_Get_Sec2Group_Entry_Util(
                        vacm_sec_model, vacm_sec_name_len, vacm_sec_name,
                        &(Vacm_Mib_Temp_Sec2Group_Table));
        }
    }

    if ((vacm_sec2group_entry) && (status == SNMP_NOERROR))
    {
        switch(obj->Id[VACM_SEC2GROUP_ATTR_OFFSET])
        {
        case 3:                         /* vacmGroupName */

            /* If we have invalid value then return error code. */
            if (strlen((CHAR *)obj->Syntax.BufChr) != obj->SyntaxLen)
            {
                /* Return error code. */
                status = SNMP_WRONGVALUE;
            }

            /* If we have invalid value then return error code. */
            else if (strlen((CHAR *)obj->Syntax.BufChr) == 0)
            {
                /* Return error code. */
                status = SNMP_WRONGLENGTH;
            }

            /* If we have invalid length then return error code. */
            else if (obj->SyntaxLen >= SNMP_SIZE_SMALLOBJECTID)
            {
                /* Return error code. */
                status = SNMP_WRONGLENGTH;
            }
            else
            {
                /* Set the value of 'vacmGroupName'. */
                strcpy((CHAR *)vacm_sec2group_entry->vacm_group_name,
                       (CHAR *)obj->Syntax.BufChr);
            }

            break;

        case 4:                     /* vacmSecurityToGroupStorageType */

            /* If we don't have the valid value to set then return error
             * code.
             */
            if ((obj->Syntax.LngUns < 1) || (obj->Syntax.LngUns > 5) ||
                ((((vacm_sec2group_entry->vacm_storage_type ==
                                                SNMP_STORAGE_PERMANENT) ||
                   (obj->Syntax.LngUns == SNMP_STORAGE_PERMANENT) ||
                   (obj->Syntax.LngUns == SNMP_STORAGE_READONLY) )) &&
                 (vacm_sec2group_entry->vacm_row_flag !=
                                                SNMP_ROW_CREATEANDGO) &&
                 (vacm_sec2group_entry->vacm_row_flag !=
                                                SNMP_ROW_CREATEANDWAIT)))
            {
                /* Returning error code. */
                status = SNMP_WRONGVALUE;
            }
            else
            {
                /* Setting value of 'vacmSecurityToGroupStorageType'. */
                vacm_sec2group_entry->vacm_storage_type =
                                            ((UINT8)(obj->Syntax.LngUns));
            }

            break;

        case 5:                         /* vacmSecurityToGroupStatus */

            /* Validate the row status value. */
            if ((obj->Syntax.LngUns < 1) || (obj->Syntax.LngUns > 6))
            {
                status = SNMP_WRONGVALUE;
            }

            else if ((vacm_sec2group_entry->vacm_row_flag == 0) ||
                     (vacm_sec2group_entry->vacm_row_flag ==
                        ((UINT8)obj->Syntax.LngUns)))
            {
                vacm_sec2group_entry->vacm_row_flag =
                                            (UINT8)(obj->Syntax.LngUns);
            }
            else
            {
                status = SNMP_INCONSISTANTVALUE;
            }

            break;

        default:

            /* We have reached at end of the table. */
            status = SNMP_NOSUCHNAME;
        }
    }
    else
    {
        status = SNMP_NOSUCHINSTANCE;
    }

    /* Return success or error code. */
    return (status);

} /* Set_vacmSec2GroupEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Undo_vacmSec2GroupEntry
*
*   DESCRIPTION
*
*       This function is used to process SNMP UNDO request on
*       vacmSecurityToGroupTable.
*
*   INPUTS
*
*       *obj                SNMP object containing request.
*
*   OUTPUTS
*
*       SNMP_NOERROR            Successful.
*
*************************************************************************/
STATIC UINT16 Undo_vacmSec2GroupEntry(const snmp_object_t *obj)
{
    /* VACM security model. */
    UINT32                  vacm_sec_model;

    /* VACM security name length. */
    UINT32                  vacm_sec_name_len;

    /* Handle to the vacmSec2GroupEntry. */
    VACM_SEC2GROUP_STRUCT   *vacm_sec2group_entry = NU_NULL;

    /* VACM security name. */
    UINT8                   vacm_sec_name[SNMP_SIZE_SMALLOBJECTID];

    /* Status to return success or error code. */
    UINT16                  status = SNMP_NOERROR;

    /* Variable to use in for-loop. */
    UINT32                  i;

    /* Get the value of security model from object identifier. */
    vacm_sec_model = obj->Id[VACM_SEC2GROUP_SUB_LEN];

    /* Get the value of security name length from the object identifier.
     */
    vacm_sec_name_len = obj->Id[VACM_SEC2GROUP_SUB_LEN + 1];

    /* Clear out security name. */
    UTL_Zero(vacm_sec_name, sizeof(vacm_sec_name));

    /* Loop to get the value of security name. */
    for (i = 0;
         ((i < vacm_sec_name_len) &&
          (i < (SNMP_SIZE_SMALLOBJECTID - 1)) &&
          ((VACM_SEC2GROUP_SUB_LEN + 2 + i) < obj->IdLen));
         i++)
    {
        vacm_sec_name[i] =
                        (UINT8)(obj->Id[VACM_SEC2GROUP_SUB_LEN + 2 + i]);
    }


    /* If we have invalid OID then return error code. */
    if ( (i != vacm_sec_name_len) ||
         ((VACM_SEC2GROUP_SUB_LEN + 2 + i) != obj->IdLen) ||
         (strlen((CHAR *)vacm_sec_name) != vacm_sec_name_len) )
    {
        status = SNMP_NOSUCHINSTANCE;
    }

    /* If no error till now then proceed. */
    if (status == SNMP_NOERROR)
    {
        /* Get the handle to the entry from permanent list. */
        vacm_sec2group_entry = VACM_Get_Sec2Group_Entry_Util(
                        vacm_sec_model, vacm_sec_name_len, vacm_sec_name,
                        &(Vacm_Mib_Table.vacm_security_to_group_tab));

        /* If we did not get handle to the entry in permanent list. */
        if (!vacm_sec2group_entry)
        {
            /* Get handle to the entry from temporary list. */
            vacm_sec2group_entry = VACM_Get_Sec2Group_Entry_Util(
                        vacm_sec_model, vacm_sec_name_len, vacm_sec_name,
                        &(Vacm_Mib_Temp_Sec2Group_Table));

            /* If we got handle to the entry from temporary list. */
            if (vacm_sec2group_entry)
            {
                /* Remove the entry from the list. */
                DLL_Remove(&Vacm_Mib_Temp_Sec2Group_Table,
                           vacm_sec2group_entry);

                /* Deallocate the entry. */
                if (NU_Deallocate_Memory(vacm_sec2group_entry) !=
                                                            NU_SUCCESS)
                {
                    NLOG_Error_Log("SNMP: Failed to deallocate memory",
                                    NERR_SEVERE, __FILE__, __LINE__);
                }

                /* Nullify the entry handle. */
                vacm_sec2group_entry = NU_NULL;
            }
        }
    }

    /* If we have handle to the entry the proceed. */
    if ((vacm_sec2group_entry) && (status == SNMP_NOERROR))
    {
        switch(obj->Id[VACM_SEC2GROUP_ATTR_OFFSET])
        {
        case 3:                         /* vacmGroupName */

            if (strlen((CHAR *)(obj->Syntax.BufChr)) <
                                                  SNMP_SIZE_SMALLOBJECTID)
            {
                /* Set the value of 'vacmGroupName'. */
                strcpy((CHAR *)(vacm_sec2group_entry->vacm_group_name),
                       (CHAR *)(obj->Syntax.BufChr));
            }

            break;

        case 4:                     /* vacmSecurityToGroupStorageType */

            /* Set value of 'vacmSecurityToGroupStorageType'. */
            vacm_sec2group_entry->vacm_storage_type =
                                            (UINT8)(obj->Syntax.LngUns);

            break;

        case 5:                         /* vacmSecurityToGroupStatus */

            /* Set the value of 'vacmSecurityToGroupStatus'. */
            vacm_sec2group_entry->vacm_status =
                                            (UINT8)(obj->Syntax.LngUns);

            break;
        }

        /* Clear out row flag. */
        vacm_sec2group_entry->vacm_row_flag = 0;
    }

    /* Return success or error code. */
    return (SNMP_NOERROR);

} /* Undo_vacmSec2GroupEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Create_vacmSec2GroupEntry
*
*   DESCRIPTION
*
*       This function is used to process SNMP CREATE request on
*       vacmSecurityToGroupTable.
*
*   INPUTS
*
*       *obj                SNMP object containing request.
*
*   OUTPUTS
*
*       SNMP_NOERROR            Successful.
*       SNMP_NOCREATION         Creation fail.
*       SNMP_NOSUCHINSTANCE     Creation fail due to wrong OID.
*       SNMP_GENERROR           General error.
*
*************************************************************************/
STATIC UINT16 Create_vacmSec2GroupEntry(const snmp_object_t *obj)
{
    /* VACM security model. */
    UINT32                  vacm_sec_model;

    /* VACM security name length. */
    UINT32                  vacm_sec_name_len;

    /* vacmSec2GroupEntry. */
    VACM_SEC2GROUP_STRUCT   vacm_sec2group_entry;

    /* VACM security name. */
    UINT8                   vacm_sec_name[SNMP_SIZE_SMALLOBJECTID];

    /* Variable to use in for-loop. */
    UINT32                  i;

    /* Status to return success or error code. */
    UINT16                  status = SNMP_NOERROR;

    /* Clear out vacmSec2GroupEntry. */
    UTL_Zero(&vacm_sec2group_entry, sizeof(vacm_sec2group_entry));

    /* Get the value of security model from object identifier. */
    vacm_sec_model = obj->Id[VACM_SEC2GROUP_SUB_LEN];

    /* Get the value of security name length from the object identifier.
     */
    vacm_sec_name_len = obj->Id[VACM_SEC2GROUP_SUB_LEN + 1];

    /* Clear out security name. */
    UTL_Zero(vacm_sec_name, sizeof(vacm_sec_name));

    /* Loop to get the value of security name. */
    for (i = 0;
         ((i < vacm_sec_name_len) &&
          (i < (SNMP_SIZE_SMALLOBJECTID - 1)) &&
          ((VACM_SEC2GROUP_SUB_LEN + 2 + i) < obj->IdLen));
         i++)
    {
        vacm_sec_name[i] =
                        (UINT8)(obj->Id[VACM_SEC2GROUP_SUB_LEN + 2 + i]);
    }

    if ((vacm_sec_model == 0) || (vacm_sec_name_len == 0) ||
        (vacm_sec_name_len > SNMP_SIZE_SMALLOBJECTID))
    {
        status = SNMP_NOCREATION;
    }

    /* If we have invalid OID then return error code. */
    else if ( (i != vacm_sec_name_len) ||
         ((VACM_SEC2GROUP_SUB_LEN + 2 + i) != obj->IdLen) ||
         (strlen((CHAR *)vacm_sec_name) != vacm_sec_name_len) )
    {
        status = SNMP_NOSUCHINSTANCE;

        if ((obj->Id[VACM_SEC2GROUP_ATTR_OFFSET] == 5) &&
            ((obj->Syntax.LngUns == SNMP_ROW_CREATEANDGO) ||
             (obj->Syntax.LngUns == SNMP_ROW_CREATEANDWAIT)))
        {
            status = SNMP_NOCREATION;
        }
    }

    if (status == SNMP_NOERROR)
    {
        /* Clear out the structure. */
        UTL_Zero(&vacm_sec2group_entry, sizeof(VACM_SEC2GROUP_STRUCT));

        /* Setting value of security model. */
        vacm_sec2group_entry.vacm_security_model = vacm_sec_model;

        /* Setting value of security name. */
        NU_BLOCK_COPY(vacm_sec2group_entry.vacm_security_name,
                      vacm_sec_name, sizeof(vacm_sec_name));

        /* Setting value to storage type. */
        vacm_sec2group_entry.vacm_storage_type = SNMP_STORAGE_DEFAULT;

        /* Setting value to row status. */
        vacm_sec2group_entry.vacm_status = SNMP_ROW_CREATEANDWAIT;

        /* Create the entry. */
        if (VACM_InsertGroup_Util(&vacm_sec2group_entry,
                            &Vacm_Mib_Temp_Sec2Group_Table) != NU_SUCCESS)
        {
            NLOG_Error_Log("SNMP: VACM_InsertGroup failed", NERR_SEVERE,
                            __FILE__, __LINE__);

            status = SNMP_GENERROR;
        }
    }

    /* Return success or error code. */
    return (status);

} /* Create_vacmSec2GroupEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Commit_vacmSec2GroupEntryStatus
*
*   DESCRIPTION
*
*       This function is used to process commit row status request on
*       vacmSecurityToGroupTable.
*
*   INPUTS
*
*       *vacm_sec2group_entry   Handle to the security to group entry.
*       row_status              New value to row status.
*       is_new                  Flag to distinguish new and old entries.
*
*   OUTPUTS
*
*       SNMP_NOERROR            Successful.
*       SNMP_INCONSISTANTVALUE  Inconsistent value.
*
*************************************************************************/
STATIC UINT16 Commit_vacmSec2GroupEntryStatus(
            VACM_SEC2GROUP_STRUCT *vacm_sec2group_entry, UINT8 row_status,
            UINT8 is_new)
{
    /* Status for returning success or error code. */
    UINT16          status = SNMP_NOERROR;

    /* Temporary row status. */
    UINT8           temp_status;

    /* Buffer to check. */
    UINT8           check_buffer[SNMP_SIZE_SMALLOBJECTID];

    /* Clear out the check buffer. */
    UTL_Zero(check_buffer, sizeof(check_buffer));

    /* If we have not all the data ready then mark the row as 'not ready'.
     */
    if (strlen((CHAR *)(vacm_sec2group_entry->vacm_group_name)) == 0)
    {
        temp_status = SNMP_ROW_NOTREADY;
    }

    /* If row is ready then mark it as 'not in service'. */
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
                vacm_sec2group_entry->vacm_status = SNMP_ROW_ACTIVE;
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
                vacm_sec2group_entry->vacm_status = SNMP_ROW_NOTINSERVICE;
            }

            else
            {
                /* Inconsistent value. */
                status = SNMP_INCONSISTANTVALUE;
            }

            break;

        case SNMP_ROW_NOTREADY:

            /* Setting the row status to 'NOTINSERVICE'. */
            vacm_sec2group_entry->vacm_status = SNMP_ROW_NOTINSERVICE;

            break;

        case SNMP_ROW_DESTROY:

            /* Setting row status to SNMP_ROW_DESTROY. */
            vacm_sec2group_entry->vacm_status = SNMP_ROW_DESTROY;

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
               (vacm_sec2group_entry->vacm_status !=
                                                    SNMP_ROW_ACTIVE)))
            {
                /* Activating the entry. */
                vacm_sec2group_entry->vacm_status = SNMP_ROW_ACTIVE;
            }

            else
            {
                /* Setting status to error code of in-consistent
                 * value.
                 */
                status = SNMP_INCONSISTANTVALUE;
            }

            break;

        case SNMP_ROW_CREATEANDWAIT:

            /* Entry should not be 'active'. */
            if(vacm_sec2group_entry->vacm_status != SNMP_ROW_ACTIVE)
            {
                vacm_sec2group_entry->vacm_status =
                                                (UINT8)temp_status;
            }

            else
            {
                /* Setting status to error code of in-consistent
                   value. */
                status = SNMP_INCONSISTANTVALUE;
            }

            break;

        default:

            if ((vacm_sec2group_entry->vacm_row_flag == 0) ||
                (temp_status == SNMP_ROW_NOTREADY) ||
                (vacm_sec2group_entry->vacm_row_flag ==
                                                  SNMP_ROW_DESTROY))
            {
                vacm_sec2group_entry->vacm_status = temp_status;
            }

            else if (vacm_sec2group_entry->vacm_row_flag ==
                                                SNMP_ROW_CREATEANDGO)
            {
                if (temp_status == SNMP_ROW_NOTINSERVICE)
                {
                    vacm_sec2group_entry->vacm_status =
                                                      SNMP_ROW_ACTIVE;
                }
                else
                {
                    status = SNMP_INCONSISTANTVALUE;
                }
            }
            else if (vacm_sec2group_entry->vacm_row_flag ==
                                              SNMP_ROW_CREATEANDWAIT)
            {
                vacm_sec2group_entry->vacm_status = temp_status;
            }
        }
    }

    /* Return success or error code. */
    return (status);

} /* Commit_vacmSec2GroupEntryStatus */

/*************************************************************************
*
*   FUNCTION
*
*       Commit_vacmSec2GroupEntries
*
*   DESCRIPTION
*
*       This function is used to process commit vacmSecurityToGroupTable
*       entries for moving all the temporary entries to permanent entries.
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
STATIC VOID Commit_vacmSec2GroupEntries(VOID)
{
    /* Handle to the security to group entry. */
    VACM_SEC2GROUP_STRUCT   *vacm_sec2group_entry;

    /* Start traversing temporary list. */
    vacm_sec2group_entry = Vacm_Mib_Temp_Sec2Group_Table.next;

    /* Loop till temporary list is not empty. */
    while (vacm_sec2group_entry)
    {
        /* Make the entry permanent. */
        if (VACM_InsertGroup(vacm_sec2group_entry) != NU_SUCCESS)
        {
            NLOG_Error_Log("SNMP: Failed to insert SEC2GROUP Entry",
                            NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Remove entry fro the temporary list. */
        DLL_Dequeue(&Vacm_Mib_Temp_Sec2Group_Table);

        /* Deallocate the memory attained by the entry as VACM_InsertGroup
         * makes its own copy.
         */
        if (NU_Deallocate_Memory(vacm_sec2group_entry) != NU_SUCCESS)
        {
            NLOG_Error_Log("SNMP: Memory deallocation failed",
                            NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Get next temporary entry. */
        vacm_sec2group_entry = Vacm_Mib_Temp_Sec2Group_Table.next;
    }

    /* Start traversing permanent list. */
    vacm_sec2group_entry = Vacm_Mib_Table.vacm_security_to_group_tab.next;

    /* Traverse all the entries in permanent list. */
    while (vacm_sec2group_entry)
    {
        /* Clear out row flag. */
        vacm_sec2group_entry->vacm_row_flag = 0;

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)
        /* Update data in file. */
        VACM_Save_Group(vacm_sec2group_entry);
#endif

        /* Moving forward in the list. */
        vacm_sec2group_entry = vacm_sec2group_entry->next;
    }

} /* Commit_vacmSec2GroupEntries */

/*************************************************************************
*
*   FUNCTION
*
*       vacmSec2GroupEntry
*
*   DESCRIPTION
*
*       This function is used to process SNMP request on
*       vacmSecurityToGroupTable.
*
*   INPUTS
*
*       *obj                SNMP object containing request.
*
*   OUTPUTS
*
*       SNMP_NOERROR            Successful.
*       SNMP_NOSUCHINSTANCE     Required instance of object is not found.
*       SNMP_NOSUCHNAME         SNMP Object not found.
*       SNMP_INCONSISTANTVALUE  Value to set is inconsistent.
*       SNMP_READONLY           Instance has storage type of read-only.
*       SNMP_WRONGVALUE         Wrong value.
*       SNMP_WRONGLENGTH        Wrong length.
*       SNMP_NOCREATION         Creation fail.
*       SNMP_GENERROR           General error.
*
*************************************************************************/
UINT16 vacmSec2GroupEntry(snmp_object_t *obj, UINT16 idlen,
                                VOID *param)
{
    /* Handle to the security to group entry. */
    VACM_SEC2GROUP_STRUCT   *vacm_sec2group_entry;

    /* Temporary handle to the security to group entry. */
    VACM_SEC2GROUP_STRUCT   *temp_vacm_sec2group_entry;

    /* Transitions from one row status value to another. */
    UINT16                  status_trans[6][4] =
    {
     {SNMP_INCONSISTANTVALUE, SNMP_INCONSISTANTVALUE, SNMP_NOERROR,
      SNMP_NOERROR},
     {SNMP_INCONSISTANTVALUE, SNMP_INCONSISTANTVALUE, SNMP_NOERROR,
      SNMP_NOERROR},
     {SNMP_WRONGVALUE, SNMP_WRONGVALUE,
      SNMP_WRONGVALUE, SNMP_WRONGVALUE},
     {SNMP_NOERROR, SNMP_INCONSISTANTVALUE, SNMP_INCONSISTANTVALUE,
      SNMP_INCONSISTANTVALUE},
     {SNMP_NOERROR, SNMP_INCONSISTANTVALUE, SNMP_INCONSISTANTVALUE,
      SNMP_INCONSISTANTVALUE},
     {SNMP_NOERROR, SNMP_NOERROR, SNMP_NOERROR, SNMP_NOERROR}
    };

    /* Current state. */
    UINT32                  current_state;

    /* VACM security model. */
    UINT32                  vacm_sec_model;

    /* VACM security name. */
    UINT8                   vacm_sec_name[SNMP_SIZE_SMALLOBJECTID];

    /* VACM security name length. */
    UINT32                  vacm_sec_name_len;

    /* Status to return success or error code. */
    UINT16              status = SNMP_NOERROR;

    /* Variable to use in for-loop. */
    UINT32              i;

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
        status = Get_vacmSec2GroupEntry(obj, getflag);

        break;

    case SNMP_PDU_BULK:                     /* Get-bulk request. */

        /* Processing of bulk operation. */
        status = SNMP_Get_Bulk(obj, Get_vacmSec2GroupEntry);

        break;

    case SNMP_PDU_SET:                      /* Set request. */

        VACM_Mib_SEC2GROUP_Commit_Left++;

        /* Process the SET operation. */
        status = Set_vacmSec2GroupEntry(obj);

        if (status != SNMP_NOSUCHINSTANCE)
            break;

    case SNMP_PDU_CREATE:                   /* Create request. */

        /* Processing of create operation. */
        status = Create_vacmSec2GroupEntry(obj);

        /* If the entry was successfully created, set the value. */
        if(status == SNMP_NOERROR)
            status = Set_vacmSec2GroupEntry(obj);

        break;

    case SNMP_PDU_UNDO:                     /* Undo request. */

        VACM_Mib_SEC2GROUP_Commit_Left = 0;

        /* Processing of UNDO operations. */
        status = Undo_vacmSec2GroupEntry(obj);

        break;

    case SNMP_PDU_COMMIT:                   /* Commit request. */

        VACM_Mib_SEC2GROUP_Commit_Left--;

        /* Get the value of security model from object identifier. */
        vacm_sec_model = obj->Id[VACM_SEC2GROUP_SUB_LEN];

        /* Get the value of security name length from the object
         * identifier.
         */
        vacm_sec_name_len = obj->Id[VACM_SEC2GROUP_SUB_LEN + 1];

        /* Clear out security name. */
        UTL_Zero(vacm_sec_name, sizeof(vacm_sec_name));

        /* Loop to get the value of security name. */
        for (i = 0;
             ((i < vacm_sec_name_len) &&
              (i < (SNMP_SIZE_SMALLOBJECTID - 1)) &&
              ((VACM_SEC2GROUP_SUB_LEN + 2 + i) < obj->IdLen));
             i++)
        {
            vacm_sec_name[i] =
                        (UINT8)(obj->Id[VACM_SEC2GROUP_SUB_LEN + 2 + i]);
        }

        /* Get the handle to the entry from temporary list. */
        vacm_sec2group_entry = VACM_Get_Sec2Group_Entry_Util(
                                        vacm_sec_model, vacm_sec_name_len,
                                        vacm_sec_name,
                                        &Vacm_Mib_Temp_Sec2Group_Table);

        /* Copy the handle in temporary variable. */
        temp_vacm_sec2group_entry = vacm_sec2group_entry;

        /* If we did not got the handle to the entry. */
        if (!vacm_sec2group_entry)
        {
            /* Get handle from the permanent list. */
            vacm_sec2group_entry = VACM_Get_Sec2Group_Entry_Util(
                         vacm_sec_model, vacm_sec_name_len, vacm_sec_name,
                         &(Vacm_Mib_Table.vacm_security_to_group_tab));
        }

        /* If we got the handle either from permanent or temporary list.
         */
        if (vacm_sec2group_entry)
        {
            /* Check whether it was a row set operation. */
            if (obj->Id[VACM_SEC2GROUP_ATTR_OFFSET] == 5)
            {
                /* Update the value of current state. Following is the
                 * list meaning of different values.
                 *
                 * 0. New Entry
                 * 1. Not Ready
                 * 2. Not In Service
                 * 3. Active
                 */

                /* If entry is new entry then set current state to 0. */
                if (temp_vacm_sec2group_entry)
                {
                    current_state = 0;
                }

                /* If entry is not ready then set current state to 1. */
                else if (strlen((CHAR *)vacm_sec2group_entry->
                                                    vacm_group_name) == 0)
                {
                    current_state = 1;
                }

                /* If entry is ready but not active, or in other words is
                 * 'not in service' then set current state to 2. */
                else if (vacm_sec2group_entry->vacm_status !=
                                                        SNMP_ROW_ACTIVE)
                {
                    current_state = 2;
                }

                /* If entry is active then set current state to 3. */
                else
                {
                    current_state = 3;
                }

                /* If required state transition is not allowed then return
                 * error code.
                 */
                if (status_trans[obj->Syntax.LngUns - 1][current_state]
                                                         != SNMP_NOERROR)
                {
                    /* Return error code. */
                    status =
                      status_trans[obj->Syntax.LngUns - 1][current_state];
                }

                /* If required state transition is allowed. */
                else
                {
                    /* Commit row status. */
                    status = Commit_vacmSec2GroupEntryStatus(
                        vacm_sec2group_entry, (UINT8)(obj->Syntax.LngUns),
                        (UINT8)(temp_vacm_sec2group_entry != NU_NULL));
                }
            }

            /* If it was not row set operation. */
            else
            {
                /* Commit row status. */
                status = Commit_vacmSec2GroupEntryStatus(
                    vacm_sec2group_entry,
                    vacm_sec2group_entry->vacm_status, NU_FALSE);
            }

            /* If the row status is set to 'DESTROY'. */
            if (vacm_sec2group_entry->vacm_status == SNMP_ROW_DESTROY)
            {
                /* If handle was from temporary list then remove it from
                 * temporary list.
                 */
                if (temp_vacm_sec2group_entry)
                {
                    DLL_Remove(&Vacm_Mib_Temp_Sec2Group_Table,
                                vacm_sec2group_entry);
                }

                /* If handle was from permanent list then remove it from
                 * permanent list.
                 */
                else
                {
                    DLL_Remove(&Vacm_Mib_Table.vacm_security_to_group_tab,
                               vacm_sec2group_entry);
                }

                /* Deallocate memory attained by this entry. */
                if (NU_Deallocate_Memory(vacm_sec2group_entry) !=
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
        if ((status == SNMP_NOERROR) &&
            (VACM_Mib_SEC2GROUP_Commit_Left == 0))
        {
            Commit_vacmSec2GroupEntries();
        }

        break;

    default:                                /* Invalid request. */

        /* Return error code. */
        status = SNMP_GENERROR;
    }

    /* Return success or error code. */
    return (status);

} /* vacmSec2GroupEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Get_vacmAccessEntry
*
*   DESCRIPTION
*
*       This function is used to process SNMP GET/GET-NEXT request on
*       vacmAccessTable.
*
*   INPUTS
*
*       *obj                SNMP object containing request.
*       getflag             Flag to distinguish GET and GET-NEXT requests.
*
*   OUTPUTS
*
*       SNMP_NOERROR        Successful.
*       SNMP_NOSUCHINSTANCE Required instance of object is not found.
*       SNMP_NOSUCHNAME     SNMP Object not found.
*
*************************************************************************/
STATIC UINT16 Get_vacmAccessEntry(snmp_object_t *obj, UINT8 getflag)
{
    /* Table entry OID. */
    UINT32              table_oid[] = {1, 3, 6, 1, 6, 3, 16, 1, 4, 1};

    /* Handle to VACM Access entry. */
    VACM_ACCESS_STRUCT  *vacm_access_entry = NU_NULL;

    /* Group name length. */
    UINT32              vacm_group_name_len;

    /* Group name. */
    UINT8               vacm_group_name[SNMP_SIZE_SMALLOBJECTID];

    /* Length of context prefix. */
    UINT32              vacm_context_prefix_len;

    /* Context Prefix. */
    UINT8               vacm_context_prefix[SNMP_SIZE_SMALLOBJECTID];

    /* Security model. */
    UINT32              vacm_sec_model;

    /* Security level. */
    UINT32              vacm_sec_level;

    /* Variable to use in for-loop. */
    UINT32              i, temp;

    /* Status to return success or error code. */
    UINT16              status = SNMP_NOERROR;

    /* Get the value of Group name length from object identifier. */
    vacm_group_name_len = obj->Id[VACM_ACCESS_SUB_LEN];

    /* Clear out the value of group name. */
    UTL_Zero(vacm_group_name, sizeof(vacm_group_name));

    /* Get the value of group name. */
    for (i = 0;
         ( (i < vacm_group_name_len) &&
           (i < (SNMP_SIZE_SMALLOBJECTID - 1)) );
         i++)
    {
        vacm_group_name[i] =
                            (UINT8)(obj->Id[VACM_ACCESS_SUB_LEN + 1 + i]);
    }

    /* Get the value of context prefix length. */
    vacm_context_prefix_len = obj->Id[VACM_ACCESS_SUB_LEN + 1 + i];

    /* Hold current available position in temporary variable. */
    temp = VACM_ACCESS_SUB_LEN + 2 + i;

    /* Clear out context prefix. */
    UTL_Zero(vacm_context_prefix, sizeof(vacm_context_prefix));

    /* Get the value context prefix. */
    for (i = 0;
         ((i < vacm_context_prefix_len) &&
          (i < (SNMP_SIZE_SMALLOBJECTID - 1)));
         i++)
    {
        vacm_context_prefix[i] = (UINT8)(obj->Id[temp + i]);
    }

    /* Get the value of security model. */
    vacm_sec_model = obj->Id[temp + i];

    /* Get the value of security level. */
    vacm_sec_level = obj->Id[temp + i + 1];

    /* If we are handling GET request. */
    if (getflag)
    {
        /* If we have invalid OID then return error code. */
        if ( ((temp + i + 2) != obj->IdLen) ||
             (strlen((CHAR *)(vacm_group_name)) != vacm_group_name_len) ||
             (strlen((CHAR *)(vacm_context_prefix)) !=
                                                vacm_context_prefix_len) )
        {
            /* Return error code. */
            status = SNMP_NOSUCHINSTANCE;
        }
    }

    /* If no error till now the proceed. */
    if (status == SNMP_NOERROR)
    {
        /* Get the handle to the VACM Access entry. */
        vacm_access_entry = VACM_MIB_Get_AccessEntry(vacm_group_name_len,
                                vacm_group_name, vacm_context_prefix_len,
                                vacm_context_prefix, vacm_sec_model,
                                vacm_sec_level, getflag);
    }

    /* If we got the handle to the VACM Access entry. */
    if (vacm_access_entry)
    {
        switch(obj->Id[VACM_ACCESS_ATTR_OFFSET])
        {
        case 4:                         /* vacmAccessContextMatch */

            /* Get the value of 'vacmAccessContextMatch'. */
            obj->Syntax.LngUns =
                        (UINT32)(vacm_access_entry->vacm_context_match);

            break;

        case 5:                         /* vacmAccessReadViewName */

            /* Get the value of 'vacmAccessReadViewName'. */
            strcpy((CHAR *)(obj->Syntax.BufChr),
                   (CHAR *)(vacm_access_entry->vacm_read_view));

            /* Update the length of syntax. */
            obj->SyntaxLen = strlen((CHAR *)(obj->Syntax.BufChr));

            break;

        case 6:                         /* vacmAccessWriteViewName */

            /* Get the value of 'vacmAccessWriteViewName'. */
            strcpy((CHAR *)(obj->Syntax.BufChr),
                   (CHAR *)(vacm_access_entry->vacm_write_view));

            /* Update the length of syntax. */
            obj->SyntaxLen = strlen((CHAR *)(obj->Syntax.BufChr));

            break;

        case 7:                         /* vacmAccessNotifyViewName */

            /* Get the value of 'vacmAccessNotifyViewName'. */
            strcpy((CHAR *)(obj->Syntax.BufChr),
                   (CHAR *)(vacm_access_entry->vacm_notify_view));

            /* Update the value of length of syntax. */
            obj->SyntaxLen = strlen((CHAR *)(obj->Syntax.BufChr));

            break;

        case 8:                         /* vacmAccessStorageType */

            /* Get the value of 'vacmAccessStorageType'. */
            obj->Syntax.LngUns = vacm_access_entry->vacm_storage_type;

            break;

        case 9:                         /* vacmAccessStatus */

            /* Get the value of 'vacmAccessStatus'. */
            obj->Syntax.LngUns = vacm_access_entry->vacm_status;

            break;

        default:
            /* We have reached at end of the table. */
            status = SNMP_NOSUCHNAME;
        }

        if ((status == SNMP_NOERROR) && (!getflag))
        {
            /* Update table OID in object identifier. */
            NU_BLOCK_COPY(obj->Id, table_oid, sizeof(table_oid));

            /* Update the value of Group name length in object identifier.
             */
            obj->Id[VACM_ACCESS_SUB_LEN] =
                      strlen((CHAR *)vacm_access_entry->vacm_group_name);

            /* Update the value of group name in object identifier. */
            for (i = 0;
                 ( (i < obj->Id[VACM_ACCESS_SUB_LEN]) &&
                   (i < (SNMP_SIZE_SMALLOBJECTID - 1)) );
                 i++)
            {
                obj->Id[VACM_ACCESS_SUB_LEN + 1 + i] =
                                    vacm_access_entry->vacm_group_name[i];
            }

            /* Update the value of context prefix length in identifier. */
            obj->Id[VACM_ACCESS_SUB_LEN + 1 + i] =
                 strlen((CHAR *)(vacm_access_entry->vacm_context_prefix));

            /* Hold current available position in temporary variable. */
            temp = VACM_ACCESS_SUB_LEN + 2 + i;

            /* Update the value context prefix in object identifier. */
            for (i = 0;
                 ((i < obj->Id[temp -1]) &&
                  (i < (SNMP_SIZE_SMALLOBJECTID - 1)));
                 i++)
            {
                obj->Id[temp + i] =
                                vacm_access_entry->vacm_context_prefix[i];
            }

            /* Update the value of security model in object identifier. */
            obj->Id[temp + i] = vacm_access_entry->vacm_security_model;

            /* Update the value of security level in object identifier. */
            obj->Id[temp + i + 1] =
                                vacm_access_entry->vacm_security_level;

            /* Update object identifier length. */
            obj->IdLen = temp + i + 2;
        }
    }

    /* If we did not get the handle to the VACM Access entry then return
     * error code.
     */
    else
    {
        /* Return error code. */
        status = SNMP_NOSUCHINSTANCE;
    }

    /* Return success or error code. */
    return (status);

} /* Get_vacmAccessEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Set_vacmAccessEntry
*
*   DESCRIPTION
*
*       This function is used to process SNMP SET request on
*       vacmAccessTable.
*
*   INPUTS
*
*       *obj                SNMP object containing request.
*
*   OUTPUTS
*
*       SNMP_NOERROR            Successful.
*       SNMP_NOSUCHINSTANCE     Required instance of object is not found.
*       SNMP_NOSUCHNAME         SNMP Object not found.
*       SNMP_INCONSISTANTVALUE  Value to set is inconsistent.
*       SNMP_READONLY           Instance has storage type of read-only.
*       SNMP_WRONGVALUE         Wrong value.
*       SNMP_WRONGLENGTH        Wrong length.
*
*************************************************************************/
STATIC UINT16 Set_vacmAccessEntry(const snmp_object_t *obj)
{
    /* Handle to VACM Access entry. */
    VACM_ACCESS_STRUCT      *vacm_access_entry = NU_NULL;

    /* Group name length. */
    UINT32              vacm_group_name_len;

    /* Group name. */
    UINT8               vacm_group_name[SNMP_SIZE_SMALLOBJECTID];

    /* Length of context prefix. */
    UINT32              vacm_context_prefix_len;

    /* Context Prefix. */
    UINT8               vacm_context_prefix[SNMP_SIZE_SMALLOBJECTID];

    /* Security model. */
    UINT32              vacm_sec_model;

    /* Security level. */
    UINT32              vacm_sec_level;

    /* Variable to use in for-loop. */
    UINT32              i, temp;

    /* Status to return success or error code. */
    UINT16              status = SNMP_NOERROR;

    /* Get the value of Group name length from object identifier. */
    vacm_group_name_len = obj->Id[VACM_ACCESS_SUB_LEN];

    /* Clear out the value of group name. */
    UTL_Zero(vacm_group_name, sizeof(vacm_group_name));

    /* Get the value of group name. */
    for (i = 0;
         ( (i < vacm_group_name_len) &&
           (i < (SNMP_SIZE_SMALLOBJECTID - 1)) );
         i++)
    {
        vacm_group_name[i] =
                            (UINT8)(obj->Id[VACM_ACCESS_SUB_LEN + 1 + i]);
    }

    /* Get the value of context prefix length. */
    vacm_context_prefix_len = obj->Id[VACM_ACCESS_SUB_LEN + 1 + i];

    /* Hold current available position in temporary variable. */
    temp = VACM_ACCESS_SUB_LEN + 2 + i;

    /* Clear out context prefix. */
    UTL_Zero(vacm_context_prefix, sizeof(vacm_context_prefix));

    /* Get the value context prefix. */
    for (i = 0;
         ((i < vacm_context_prefix_len) &&
          (i < (SNMP_SIZE_SMALLOBJECTID - 1)));
         i++)
    {
        vacm_context_prefix[i] = (UINT8)(obj->Id[temp + i]);
    }

    /* Get the value of security model. */
    vacm_sec_model = obj->Id[temp + i];

    /* Get the value of security level. */
    vacm_sec_level = obj->Id[temp + i + 1];

    if (vacm_context_prefix_len > SNMP_SIZE_SMALLOBJECTID)
    {
        status = SNMP_NOCREATION;
    }

    /* If we have invalid OID then return error code. */
    else if ( ((temp + i + 2) != obj->IdLen) ||
         (strlen((CHAR *)(vacm_group_name)) != vacm_group_name_len) ||
         (strlen((CHAR *)(vacm_context_prefix)) !=
                                                vacm_context_prefix_len) )
    {
        /* Return error code. */
        status = SNMP_NOSUCHINSTANCE;
    }

    /* If no error till now then proceed. */
    if (status == SNMP_NOERROR)
    {
        /* Get the handle to the access entry fro the permanent list. */
        vacm_access_entry = VACM_MIB_Get_AccessEntry_Util(
            vacm_group_name_len, vacm_group_name, vacm_context_prefix_len,
            vacm_context_prefix, vacm_sec_model, vacm_sec_level,
            &(Vacm_Mib_Table.vacm_access_tab));

        /* If we got the handle to the access entry from permanent list
         * and storage type of the entry is read only then return error
         * code.
         */
        if ( (vacm_access_entry) &&
             (vacm_access_entry->vacm_storage_type ==
                                                 SNMP_STORAGE_READONLY) )
        {
            /* Return error code. */
            status = SNMP_READONLY;

            /* Nullified the handle so that we could proceed to set the
             * value.
             */
            vacm_access_entry = NU_NULL;
        }

        /* If we did not get the handle to the access entry from permanent
         * list.
         */
        else if (!vacm_access_entry)
        {
            /* Get the handle to the access entry from temporary list. */
            vacm_access_entry = VACM_MIB_Get_AccessEntry_Util(
                                    vacm_group_name_len, vacm_group_name,
                                    vacm_context_prefix_len,
                                    vacm_context_prefix, vacm_sec_model,
                                    vacm_sec_level,
                                    &(VACM_Mib_Temp_Access_Root));

            /* If we did not get handle even from permanent list then
             * return error code.
             */
            if (!vacm_access_entry)
            {
                /* Return error code. */
                status = SNMP_NOSUCHINSTANCE;
            }
        }
    }

    /* If we have the handle to the access entry. */
    if (vacm_access_entry)
    {
        switch(obj->Id[VACM_ACCESS_ATTR_OFFSET])
        {
        case 4:                         /* vacmAccessContextMatch */

            /* If we don't have valid value then return error code. */
            if ((obj->Syntax.LngUns != 1) && (obj->Syntax.LngUns != 2))
            {
                /* Return error code. */
                status = SNMP_WRONGVALUE;
            }

            /* If we have valid value to set. */
            else
            {
                /* Set the value of 'vacmAccessContextMatch'. */
                vacm_access_entry->vacm_context_match =
                                                (UINT8)obj->Syntax.LngUns;
            }

            break;

        case 5:                         /* vacmAccessReadViewName */

            /* If length of value is invalid then return error code. */
            if (obj->SyntaxLen >= SNMP_SIZE_SMALLOBJECTID)
            {
                /* Return error code. */
                status = SNMP_WRONGLENGTH;
            }

            /* If the value is invalid then return error code. */
            else if (strlen((CHAR *)(obj->Syntax.BufChr)) !=
                                                           obj->SyntaxLen)
            {
                /* Return error code. */
                status = SNMP_WRONGVALUE;
            }

            /* If we have valid value then set the value of
             * 'vacmAccessReadViewName'.
             */
            else
            {
                /* Set the value of 'vacmAccessReadViewName'. */
                strncpy((CHAR *)(vacm_access_entry->vacm_read_view),
                       (CHAR *)(obj->Syntax.BufChr), SNMP_SIZE_SMALLOBJECTID);
            }

            break;

        case 6:                         /* vacmAccessWriteViewName */

            /* If we have invalid length of value then return error code.
             */
            if (obj->SyntaxLen >= SNMP_SIZE_SMALLOBJECTID)
            {
                /* Return error code. */
                status = SNMP_WRONGLENGTH;
            }

            /* If we have invalid value then return error code. */
            else if (strlen((CHAR *)(obj->Syntax.BufChr)) !=
                                                           obj->SyntaxLen)
            {
                /* Return error code. */
                status = SNMP_WRONGVALUE;
            }

            /* If we have valid value to set then set the value of
             * 'vacmAccessWriteViewName'.
             */
            else
            {
                /* Set the value of 'vacmAccessWriteViewName'. */
                strncpy((CHAR *)(vacm_access_entry->vacm_write_view),
                        (CHAR *)(obj->Syntax.BufChr), SNMP_SIZE_SMALLOBJECTID);
            }

            break;

        case 7:                         /* vacmAccessNotifyViewName */

            /* If we have invalid value length then return error code. */
            if (obj->SyntaxLen >= SNMP_SIZE_SMALLOBJECTID)
            {
                /* Return error code. */
                status = SNMP_WRONGLENGTH;
            }

            /* If we have invalid value to set then return error code. */
            else if (strlen((CHAR *)(obj->Syntax.BufChr)) !=
                                                           obj->SyntaxLen)
            {
                /* Return error code. */
                status = SNMP_WRONGVALUE;
            }

            /* If we have valid value to set then set the value of
             * 'vacmAccessNotifyViewName'.
             */
            else
            {
                /* Set the value of 'vacmAccessNotifyViewName'. */
                strncpy((CHAR *)(vacm_access_entry->vacm_notify_view),
                       (CHAR *)(obj->Syntax.BufChr), SNMP_SIZE_SMALLOBJECTID);
            }

            break;

        case 8:                         /* vacmAccessStorageType */

            /* If we have invalid value to set then return error code. */
            if ((obj->Syntax.LngUns < 1) || (obj->Syntax.LngUns > 5))
            {
                /* Return error code. */
                status = SNMP_WRONGVALUE;
            }

            /* If we have valid to set then update
             * 'vacmAccessStorageType'. */
            else
            {
                /* Set the value of 'vacmAccessStorageType'. */
                vacm_access_entry->vacm_storage_type =
                                            (UINT8)(obj->Syntax.LngUns);
            }

            break;

        case 9:                         /* vacmAccessStatus */

            /* Validate the row status value. */
            if ((obj->Syntax.LngUns < 1) || (obj->Syntax.LngUns > 6))
            {
                status = SNMP_WRONGVALUE;
            }

            /* If we have valid value for 'vacmAccessStatus' then update
             * row flag.
             */
            else if ((vacm_access_entry->vacm_row_flag == 0) ||
                     (vacm_access_entry->vacm_row_flag ==
                        ((UINT8)obj->Syntax.LngUns)))
            {
                /* Update row flag value. We will update row status value
                 * during commit phase.
                 */
                vacm_access_entry->vacm_row_flag =
                                            (UINT8)(obj->Syntax.LngUns);
            }

            /* If we have invalid value to set return error code. */
            else
            {
                /* Return error code. */
                status = SNMP_INCONSISTANTVALUE;
            }

            break;

        default:
            /* We have reached at end of the table. */

            /* Return error code. */
            status = SNMP_NOSUCHNAME;
        }
    }

    /* Return success or error code. */
    return (status);

} /* Set_vacmAccessEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Undo_vacmAccessEntry
*
*   DESCRIPTION
*
*       This function is used to process SNMP UNDO request on
*       vacmAccessTable.
*
*   INPUTS
*
*       *obj                SNMP object containing request.
*
*   OUTPUTS
*
*       SNMP_NOERROR            Successful.
*
*************************************************************************/
STATIC UINT16 Undo_vacmAccessEntry(const snmp_object_t *obj)
{
    /* Handle to VACM Access entry. */
    VACM_ACCESS_STRUCT  *vacm_access_entry = NU_NULL;

    /* Group name length. */
    UINT32              vacm_group_name_len;

    /* Group name. */
    UINT8               vacm_group_name[SNMP_SIZE_SMALLOBJECTID];

    /* Length of context prefix. */
    UINT32              vacm_context_prefix_len;

    /* Context Prefix. */
    UINT8               vacm_context_prefix[SNMP_SIZE_SMALLOBJECTID];

    /* Security model. */
    UINT32              vacm_sec_model;

    /* Security level. */
    UINT32              vacm_sec_level;

    /* Variable to use in for-loop. */
    UINT32              i, temp;

    /* Status to return success or error code. */
    UINT16              status = SNMP_NOERROR;

    /* Get the value of Group name length from object identifier. */
    vacm_group_name_len = obj->Id[VACM_ACCESS_SUB_LEN];

    /* Clear out the value of group name. */
    UTL_Zero(vacm_group_name, sizeof(vacm_group_name));

    /* Get the value of group name. */
    for (i = 0;
         ( (i < vacm_group_name_len) &&
           (i < (SNMP_SIZE_SMALLOBJECTID - 1)) );
         i++)
    {
        vacm_group_name[i] =
                            (UINT8)(obj->Id[VACM_ACCESS_SUB_LEN + 1 + i]);
    }

    /* Get the value of context prefix length. */
    vacm_context_prefix_len = obj->Id[VACM_ACCESS_SUB_LEN + 1 + i];

    /* Hold current available position in temporary variable. */
    temp = VACM_ACCESS_SUB_LEN + 2 + i;

    /* Clear out context prefix. */
    UTL_Zero(vacm_context_prefix, sizeof(vacm_context_prefix));

    /* Get the value context prefix. */
    for (i = 0;
         ((i < vacm_context_prefix_len) &&
          (i < (SNMP_SIZE_SMALLOBJECTID - 1)));
         i++)
    {
        vacm_context_prefix[i] = (UINT8)(obj->Id[temp + i]);
    }

    /* Get the value of security model. */
    vacm_sec_model = obj->Id[temp + i];

    /* Get the value of security level. */
    vacm_sec_level = obj->Id[temp + i + 1];


    /* If we have invalid OID then return error code. */
    if ( ((temp + i + 2) != obj->IdLen) ||
         (strlen((CHAR *)(vacm_group_name)) != vacm_group_name_len) ||
         (strlen((CHAR *)(vacm_context_prefix)) !=
                                                vacm_context_prefix_len) )
    {
        /* Return error code. */
        status = SNMP_NOSUCHINSTANCE;
    }

    /* If no error till now then proceed. */
    if (status == SNMP_NOERROR)
    {
        /* Get the handle to the access entry from permanent list. */
        vacm_access_entry = VACM_MIB_Get_AccessEntry_Util(
            vacm_group_name_len, vacm_group_name, vacm_context_prefix_len,
            vacm_context_prefix, vacm_sec_model, vacm_sec_level,
            &(Vacm_Mib_Table.vacm_access_tab));

        /* If the got the handle to the access entry and storage type of
         * the entry is read-only then return error code.
         */
        if ( (vacm_access_entry) &&
             (vacm_access_entry->vacm_storage_type ==
                                                 SNMP_STORAGE_READONLY) )
        {
            /* Nullify the handle so that we could not proceed with
             * setting the value.
             */
            vacm_access_entry = NU_NULL;
        }

        /* If we did not get the handle to the access entry from permanent
         * list then get the handle to the access entry from temporary
         * list.
         */
        else if (!vacm_access_entry)
        {
            /* Get the handle to the access entry from temporary list. */
            vacm_access_entry = VACM_MIB_Get_AccessEntry_Util(
                                    vacm_group_name_len, vacm_group_name,
                                    vacm_context_prefix_len,
                                    vacm_context_prefix, vacm_sec_model,
                                    vacm_sec_level,
                                    &(VACM_Mib_Temp_Access_Root));

            /* If we got the handle to the access entry from temporary
             * list then remove and deallocate the entry.
             */
            if (vacm_access_entry)
            {
                /* Remove access entry from the temporary list. */
                DLL_Remove(&(VACM_Mib_Temp_Access_Root),
                                                       vacm_access_entry);

                /* Deallocate memory attained by access entry. */
                if (NU_Deallocate_Memory(vacm_access_entry) != NU_SUCCESS)
                {
                    NLOG_Error_Log("SNMP: Failed to deallocate memory",
                                    NERR_SEVERE, __FILE__, __LINE__);
                }
            }
        }
    }

    /* If we have the handle to the access entry. */
    if (vacm_access_entry)
    {
        switch(obj->Id[VACM_ACCESS_ATTR_OFFSET])
        {
        case 4:                         /* vacmAccessContextMatch */

            /* Set the value of 'vacmAccessContextMatch'. */
            vacm_access_entry->vacm_context_match =
                                            (UINT8)(obj->Syntax.LngUns);

            break;

        case 5:                         /* vacmAccessReadViewName */

            if (strlen((CHAR *)(obj->Syntax.BufChr)) <
                                                  SNMP_SIZE_SMALLOBJECTID)
            {
                /* set the value of 'vacmAccessReadViewName'. */
                strcpy((CHAR *)(vacm_access_entry->vacm_read_view),
                       (CHAR *)obj->Syntax.BufChr);
            }

            break;

        case 6:                         /* vacmAccessWriteViewName */

            if (strlen((CHAR *)(obj->Syntax.BufChr)) <
                                                SNMP_SIZE_SMALLOBJECTID)
            {
                /* Set the value of 'vacmAccessWriteViewName'. */
                strcpy((CHAR *)(vacm_access_entry->vacm_write_view),
                       (CHAR *)(obj->Syntax.BufChr));
            }

            break;

        case 7:                         /* vacmAccessNotifyViewName */

            if (strlen((CHAR *)(obj->Syntax.BufChr)) <
                                                SNMP_SIZE_SMALLOBJECTID)
            {
                /* Set the value of 'vacmAccessNotifyViewName'. */
                strcpy((CHAR *)(vacm_access_entry->vacm_notify_view),
                       (CHAR *)(obj->Syntax.BufChr));
            }

            break;

        case 8:                         /* vacmAccessStorageType */

            /* Set the value of storage type. */
            vacm_access_entry->vacm_storage_type =
                                              (UINT8)(obj->Syntax.LngUns);

            break;

        case 9:                         /* vacmAccessStatus */

            /* Set the value of row status. */
            vacm_access_entry->vacm_status = (UINT8)(obj->Syntax.LngUns);
        }

        /* Clear out the row flag. */
        vacm_access_entry->vacm_row_flag = 0;
    }

    /* Return success. */
    return (SNMP_NOERROR);

} /* Undo_vacmAccessEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Create_vacmAccessEntry
*
*   DESCRIPTION
*
*       This function is used to process SNMP CREATE request on
*       vacmAccessTable.
*
*   INPUTS
*
*       *obj                SNMP object containing request.
*
*   OUTPUTS
*
*       SNMP_NOERROR            Successful.
*       SNMP_NOCREATION         Creation fail.
*       SNMP_NOSUCHINSTANCE     Creation fail due to wrong OID.
*       SNMP_GENERROR           General error.
*
*************************************************************************/
STATIC UINT16 Create_vacmAccessEntry(const snmp_object_t *obj)
{
    /* VACM Access entry. */
    VACM_ACCESS_STRUCT  vacm_access_entry;

    /* Group name length. */
    UINT32              vacm_group_name_len;

    /* Group name. */
    UINT8               vacm_group_name[SNMP_SIZE_SMALLOBJECTID];

    /* Length of context prefix. */
    UINT32              vacm_context_prefix_len;

    /* Context Prefix. */
    UINT8               vacm_context_prefix[SNMP_SIZE_SMALLOBJECTID];

    /* Security model. */
    UINT32              vacm_sec_model;

    /* Security level. */
    UINT32              vacm_sec_level;

    /* Variable to use in for-loop. */
    UINT32              i, temp;

    /* Status to return success or error code. */
    UINT16              status = SNMP_NOERROR;

    /* Get the value of Group name length from object identifier. */
    vacm_group_name_len = obj->Id[VACM_ACCESS_SUB_LEN];

    /* Clear out the value of group name. */
    UTL_Zero(vacm_group_name, sizeof(vacm_group_name));

    /* Get the value of group name. */
    for (i = 0;
         ( (i < vacm_group_name_len) &&
           (i < (SNMP_SIZE_SMALLOBJECTID - 1)) );
         i++)
    {
        vacm_group_name[i] =
                            (UINT8)(obj->Id[VACM_ACCESS_SUB_LEN + 1 + i]);
    }

    /* Get the value of context prefix length. */
    vacm_context_prefix_len = obj->Id[VACM_ACCESS_SUB_LEN + 1 + i];

    /* Hold current available position in temporary variable. */
    temp = VACM_ACCESS_SUB_LEN + 2 + i;

    /* Clear out context prefix. */
    UTL_Zero(vacm_context_prefix, sizeof(vacm_context_prefix));

    /* Get the value context prefix. */
    for (i = 0;
         ((i < vacm_context_prefix_len) &&
          (i < (SNMP_SIZE_SMALLOBJECTID - 1)));
         i++)
    {
        vacm_context_prefix[i] = (UINT8)(obj->Id[temp + i]);
    }

    /* Get the value of security model. */
    vacm_sec_model = obj->Id[temp + i];

    /* Get the value of security level. */
    vacm_sec_level = obj->Id[temp + i + 1];


    /* If we have invalid OID then return error code. */
    if ( ((temp + i + 2) != obj->IdLen) ||
         (strlen((CHAR *)(vacm_group_name)) != vacm_group_name_len) ||
         (strlen((CHAR *)(vacm_context_prefix)) !=
                                                vacm_context_prefix_len) )
    {
        /* Return error code. */
        status = SNMP_NOSUCHINSTANCE;
    }

    if (status == SNMP_NOERROR)
    {
        /* Clear out VACM Access entry. */
        UTL_Zero(&vacm_access_entry, sizeof(vacm_access_entry));

        /* Set the value of context prefix. */
        NU_BLOCK_COPY(vacm_access_entry.vacm_context_prefix,
                      vacm_context_prefix, sizeof(vacm_context_prefix));

        /* Set the value of group name. */
        NU_BLOCK_COPY(vacm_access_entry.vacm_group_name, vacm_group_name,
                      sizeof(vacm_group_name));

        /* Set the value of security model. */
        vacm_access_entry.vacm_security_model = vacm_sec_model;

        /* Set the value of security level. */
        vacm_access_entry.vacm_security_level = (UINT8)(vacm_sec_level);

        /* Set the value of context match to 'exact(1)'. */
        vacm_access_entry.vacm_context_match = 1;

        /* Set the value of storage type. */
        vacm_access_entry.vacm_storage_type = SNMP_STORAGE_DEFAULT;

        /* Insert the entry in the temporary list. */
        if (VACM_InsertAccessEntry_Util(&vacm_access_entry,
                                &VACM_Mib_Temp_Access_Root) != NU_SUCCESS)
        {
            NLOG_Error_Log("SNMP: VACM_InsertAccessEntry_Util failed",
                            NERR_SEVERE, __FILE__, __LINE__);

            /* Return error code. */
            status = SNMP_GENERROR;
        }
    }

    /* If there was sum error in the processing and we have set request
     * for row status of 'create and go' or 'create and wait' then return
     * error code of 'no creation'.
     */
    if ( (status != SNMP_NOERROR) &&
         (obj->Id[VACM_ACCESS_ATTR_OFFSET] == 9) &&
         ( (obj->Syntax.LngUns == SNMP_ROW_CREATEANDGO) ||
           (obj->Syntax.LngUns == SNMP_ROW_CREATEANDWAIT) ) )
    {
        /* Return error code of 'no creation'. */
        status = SNMP_NOCREATION;
    }

    /* Return success or error code. */
    return (status);

} /* Create_vacmAccessEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Commit_vacmAccessEntryStatus
*
*   DESCRIPTION
*
*       This function is used to process commit row status request on
*       vacmAccessTable.
*
*   INPUTS
*
*       *vacm_access_entry      Handle to the access entry.
*       row_status              New value to row status.
*       is_new                  Flag to distinguish new and old entries.
*
*   OUTPUTS
*
*       SNMP_NOERROR            Successful.
*       SNMP_INCONSISTANTVALUE  Inconsistent value.
*
*************************************************************************/
STATIC UINT16 Commit_vacmAccessEntryStatus(
                                    VACM_ACCESS_STRUCT *vacm_access_entry,
                                    UINT8 row_status, UINT8 is_new)
{
    /* Status for returning success or error code. */
    UINT16          status = SNMP_NOERROR;

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
            if(!is_new)
            {
                /* Activating the WEP KEY MAPPING. */
                vacm_access_entry->vacm_status = SNMP_ROW_ACTIVE;
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
            if(!is_new)
            {
                /* Setting the row status to 'NOTINSERVICE'. */
                vacm_access_entry->vacm_status = SNMP_ROW_NOTINSERVICE;
            }

            else
            {
                /* Inconsistent value. */
                status = SNMP_INCONSISTANTVALUE;
            }

            break;

        case SNMP_ROW_DESTROY:

            /* Setting row status to SNMP_ROW_DESTROY. */
            vacm_access_entry->vacm_status = SNMP_ROW_DESTROY;

            break;

        default:
            if ((vacm_access_entry->vacm_row_flag == 0) ||
                (vacm_access_entry->vacm_row_flag == SNMP_ROW_DESTROY))
            {
                status = SNMP_INCONSISTANTVALUE;
            }
            else if (vacm_access_entry->vacm_row_flag ==
                                                    SNMP_ROW_CREATEANDGO)
            {
                vacm_access_entry->vacm_status = SNMP_ROW_ACTIVE;
            }
            else if (vacm_access_entry->vacm_row_flag ==
                                                   SNMP_ROW_CREATEANDWAIT)
            {
                vacm_access_entry->vacm_status = SNMP_ROW_NOTINSERVICE;
            }

        }
    }

    else
    {
        switch(row_status)
        {
        case SNMP_ROW_CREATEANDGO:

            /* Entry should be ready and either new or non-active. */
            if((is_new) ||
               (vacm_access_entry->vacm_status != SNMP_ROW_ACTIVE))
            {
                /* Activating the entry. */
                vacm_access_entry->vacm_status = SNMP_ROW_ACTIVE;
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
            if(vacm_access_entry->vacm_status != SNMP_ROW_ACTIVE)
            {
                vacm_access_entry->vacm_status = SNMP_ROW_NOTINSERVICE;
            }

            else
            {
                /* Setting status to error code of in-consistent
                   value. */
                status = SNMP_INCONSISTANTVALUE;
            }

            break;

        default:
            if ((vacm_access_entry->vacm_row_flag == 0) ||
                (vacm_access_entry->vacm_row_flag == SNMP_ROW_DESTROY))
            {
                vacm_access_entry->vacm_status = SNMP_ROW_NOTINSERVICE;
            }
            else if (vacm_access_entry->vacm_row_flag ==
                                                SNMP_ROW_CREATEANDGO)
            {
                vacm_access_entry->vacm_status = SNMP_ROW_ACTIVE;
            }
            else if (vacm_access_entry->vacm_row_flag ==
                                            SNMP_ROW_CREATEANDWAIT)
            {
                vacm_access_entry->vacm_status = SNMP_ROW_NOTINSERVICE;
            }
        }
    }

    /* Return success or error code. */
    return (status);

} /* Commit_vacmAccessEntryStatus */

/*************************************************************************
*
*   FUNCTION
*
*       Commit_vacmAccessEntries
*
*   DESCRIPTION
*
*       This function is used to process commit vacmAccessTable entries
*       for moving all the temporary entries to permanent entries.
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
STATIC VOID Commit_vacmAccessEntries(VOID)
{
    /* Handle to the access entry. */
    VACM_ACCESS_STRUCT      *vacm_access_entry;

    /* Start traversing temporary list of access entries. */
    vacm_access_entry = VACM_Mib_Temp_Access_Root.next;

    /* Loop to traverse access entries from permanent list. */
    while (vacm_access_entry)
    {
        /* Make access entry permanent. */
        if (VACM_InsertAccessEntry(vacm_access_entry) != NU_SUCCESS)
        {
            NLOG_Error_Log("SNMP: VACM_InsertAccessEntry failed",
                            NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Remove access from temporary list. */
        DLL_Dequeue(&VACM_Mib_Temp_Access_Root);

        /* Deallocate memory attained by access entry. */
        if (NU_Deallocate_Memory(vacm_access_entry) != NU_SUCCESS)
        {
            NLOG_Error_Log("SNMP: Memory allocation failed", NERR_SEVERE,
                            __FILE__, __LINE__);
        }

        /* Get handle to the next entry. */
        vacm_access_entry = VACM_Mib_Temp_Access_Root.next;
    }

    /* Start traversing permanent list. */
    vacm_access_entry = Vacm_Mib_Table.vacm_access_tab.next;

    /* Loop to traverse permanent access entry list. */
    while (vacm_access_entry)
    {
        /* Clear out the row flag. */
        vacm_access_entry->vacm_row_flag = 0;

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)
        /* Update data in file. */
        VACM_Save_Access(vacm_access_entry);
#endif

        /* Moving forward in the list. */
        vacm_access_entry = vacm_access_entry->next;
    }

} /* Commit_vacmAccessEntries */

/*************************************************************************
*
*   FUNCTION
*
*       vacmAccessEntry
*
*   DESCRIPTION
*
*       This function is used to process SNMP requests on vacmAccessTable.
*
*   INPUTS
*
*       *obj                    SNMP object containing request.
*
*   OUTPUTS
*
*       SNMP_NOERROR            Successful.
*       SNMP_NOSUCHINSTANCE     Required instance of object is not found.
*       SNMP_NOSUCHNAME         SNMP Object not found.
*       SNMP_INCONSISTANTVALUE  Value to set is inconsistent.
*       SNMP_READONLY           Instance has storage type of read-only.
*       SNMP_WRONGVALUE         Wrong value.
*       SNMP_WRONGLENGTH        Wrong length.
*       SNMP_NOCREATION         Creation fail.
*       SNMP_GENERROR           General error.
*
*************************************************************************/
UINT16 vacmAccessEntry(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    /* Handle to the usmUserEntry. */
    VACM_ACCESS_STRUCT      *vacm_access_entry = NU_NULL;

    /* Temporary handle to the usmUserEntry. */
    VACM_ACCESS_STRUCT      *temp_vacm_access_entry = NU_NULL;

    /* Transitions from one row status value to another. */
    UINT16                  status_trans[6][4] =
    {
     {SNMP_INCONSISTANTVALUE, SNMP_INCONSISTANTVALUE, SNMP_NOERROR,
      SNMP_NOERROR},
     {SNMP_INCONSISTANTVALUE, SNMP_INCONSISTANTVALUE, SNMP_NOERROR,
      SNMP_NOERROR},
     {SNMP_WRONGVALUE, SNMP_INCONSISTANTVALUE,
      SNMP_WRONGVALUE, SNMP_WRONGVALUE},
     {SNMP_NOERROR, SNMP_INCONSISTANTVALUE, SNMP_INCONSISTANTVALUE,
      SNMP_INCONSISTANTVALUE},
     {SNMP_NOERROR, SNMP_INCONSISTANTVALUE, SNMP_INCONSISTANTVALUE,
      SNMP_INCONSISTANTVALUE},
     {SNMP_NOERROR, SNMP_NOERROR, SNMP_NOERROR, SNMP_NOERROR}
    };

    /* Group name length. */
    UINT32              vacm_group_name_len;

    /* Group name. */
    UINT8               vacm_group_name[SNMP_SIZE_SMALLOBJECTID];

    /* Length of context prefix. */
    UINT32              vacm_context_prefix_len;

    /* Context Prefix. */
    UINT8               vacm_context_prefix[SNMP_SIZE_SMALLOBJECTID];

    /* Security model. */
    UINT32              vacm_sec_model;

    /* Security level. */
    UINT32              vacm_sec_level;

    /* Variable to use in for-loop. */
    UINT32              i, temp, current_state;

    /* Status to return success or error code. */
    UINT16              status = SNMP_NOERROR;

    /* Flag to distinguish between get and get-next requests. */
    UINT8               getflag = 0;

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
        status = Get_vacmAccessEntry(obj, getflag);

        break;

    case SNMP_PDU_BULK:                     /* Get-bulk request. */

        /* Processing of bulk operation. */
        status = SNMP_Get_Bulk(obj, Get_vacmAccessEntry);

        break;

    case SNMP_PDU_SET:                      /* Set request. */

        VACM_Mib_Access_Commit_Left++;

        /* Process the SET operation. */
        status = Set_vacmAccessEntry(obj);

        if (status != SNMP_NOSUCHINSTANCE)
            break;

    case SNMP_PDU_CREATE:

        /* Processing of create operation. */
        status = Create_vacmAccessEntry(obj);

        /* If the entry was successfully created, set the value. */
        if(status == SNMP_NOERROR)
            status = Set_vacmAccessEntry(obj);

        break;

    case SNMP_PDU_UNDO:                     /* Undo request. */

        VACM_Mib_Access_Commit_Left = 0;

        /* Processing of UNDO operations. */
        status = Undo_vacmAccessEntry(obj);

        break;

    case SNMP_PDU_COMMIT:                   /* Commit request. */

        VACM_Mib_Access_Commit_Left--;

        /* Get the value of Group name length from object identifier. */
        vacm_group_name_len = obj->Id[VACM_ACCESS_SUB_LEN];

        /* Clear out the value of group name. */
        UTL_Zero(vacm_group_name, sizeof(vacm_group_name));

        /* Get the value of group name. */
        for (i = 0;
             ( (i < vacm_group_name_len) &&
               (i < (SNMP_SIZE_SMALLOBJECTID - 1)) );
             i++)
        {
            vacm_group_name[i] =
                            (UINT8)(obj->Id[VACM_ACCESS_SUB_LEN + 1 + i]);
        }

        /* Get the value of context prefix length. */
        vacm_context_prefix_len = obj->Id[VACM_ACCESS_SUB_LEN + 1 + i];

        /* Hold current available position in temporary variable. */
        temp = VACM_ACCESS_SUB_LEN + 2 + i;

        /* Clear out context prefix. */
        UTL_Zero(vacm_context_prefix, sizeof(vacm_context_prefix));

        /* Get the value context prefix. */
        for (i = 0;
             ((i < vacm_context_prefix_len) &&
              (i < (SNMP_SIZE_SMALLOBJECTID - 1)));
             i++)
        {
            vacm_context_prefix[i] = (UINT8)(obj->Id[temp + i]);
        }

        /* Get the value of security model. */
        vacm_sec_model = obj->Id[temp + i];

        /* Get the value of security level. */
        vacm_sec_level = obj->Id[temp + i + 1];


        /* If we have invalid OID then return error code. */
        if ( ((temp + i + 2) != obj->IdLen) ||
             (strlen((CHAR *)(vacm_group_name)) != vacm_group_name_len) ||
             (strlen((CHAR *)(vacm_context_prefix)) !=
                                                vacm_context_prefix_len) )
        {
            /* Return error code. */
            status = SNMP_NOSUCHINSTANCE;
        }

        if (status == SNMP_NOERROR)
        {
            /* Get the handle to the access entry from temporary list. */
            vacm_access_entry = VACM_MIB_Get_AccessEntry_Util(
                            vacm_group_name_len, vacm_group_name,
                            vacm_context_prefix_len, vacm_context_prefix,
                            vacm_sec_model, vacm_sec_level,
                            &VACM_Mib_Temp_Access_Root);

            /* Make a copy of the handle in local variable. */
            temp_vacm_access_entry = vacm_access_entry;

            /* If we did not get the handle to the access entry then get
             * it from permanent entry.
             */
            if (!vacm_access_entry)
            {
                /* Get the handle to the access entry from permanent list.
                 */
                vacm_access_entry = VACM_MIB_Get_AccessEntry_Util(
                            vacm_group_name_len, vacm_group_name,
                            vacm_context_prefix_len, vacm_context_prefix,
                            vacm_sec_model, vacm_sec_level,
                            &(Vacm_Mib_Table.vacm_access_tab));
            }
        }

        /* If we got the handle either from permanent or temporary list.
         */
        if (vacm_access_entry)
        {
            /* Check whether it was a row set operation. */
            if (obj->Id[VACM_ACCESS_ATTR_OFFSET] == 9)
            {
                /* Update the value of current state.
                 *  0) New entry.
                 *  1) Not Ready.
                 *  2) Not in service.
                 *  3) Active
                 */

                /* If access entry is new then set current state to 0. */
                if (temp_vacm_access_entry)
                {
                    current_state = 0;
                }

                /* If access entry is not in service then set current
                 * state to 2.
                 */
                else if (vacm_access_entry->vacm_status
                                                       != SNMP_ROW_ACTIVE)
                {
                    current_state = 2;
                }

                /* If access entry is active the set current state to 3.
                 */
                else
                    current_state = 3;

                /* If required state transition is not allowed then return
                 * error code.
                 */
                if (status_trans[obj->Syntax.LngUns - 1][current_state] !=
                                                            SNMP_NOERROR)
                {
                    /* Return error code. */
                    status =
                      status_trans[obj->Syntax.LngUns - 1][current_state];
                }

                /* If required transition is allowed then commit the value
                 * of row status.
                 */
                else
                {
                    /* Commit the value of row status. */
                    status = Commit_vacmAccessEntryStatus(
                           vacm_access_entry, (UINT8)(obj->Syntax.LngUns),
                           (UINT8)(temp_vacm_access_entry != NU_NULL) );
                }
            }

            /* If it was not row set operation. */
            else
            {
                /* Commit row status value. */
                status = Commit_vacmAccessEntryStatus(vacm_access_entry,
                                                      vacm_access_entry->vacm_status, NU_FALSE);
            }

            /* If the row status is set to 'DESTROY'. */
            if (vacm_access_entry->vacm_status == SNMP_ROW_DESTROY)
            {
                /* If handle was from temporary list then remove it from
                 * temporary list.
                 */
                if (temp_vacm_access_entry)
                {
                    /* Removing access entry from temporary list. */
                    DLL_Remove(&VACM_Mib_Temp_Access_Root,
                               vacm_access_entry);
                }

                /* If handle was from permanent list then remove it from
                 * permanent list.
                 */
                else
                {
                    /* Removing access entry from permanent list. */
                    DLL_Remove(&Vacm_Mib_Table.vacm_access_tab,
                               vacm_access_entry);
                }

                /* Deallocate memory attained by this access entry
                 * structure.
                 */
                if (NU_Deallocate_Memory(vacm_access_entry) != NU_SUCCESS)
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
        if ( (status == SNMP_NOERROR) &&
             (VACM_Mib_Access_Commit_Left == 0) )
        {
            Commit_vacmAccessEntries();
        }

        break;

    default:                                /* Invalid request. */
        status = SNMP_GENERROR;
    }

    /* Return success or error code. */
    return (status);

} /* vacmAccessEntry */

/*************************************************************************
*
*   FUNCTION
*
*       vacmViewSpinLock
*
*   DESCRIPTION
*
*       This function is used to handle SNMP request on
*       'vacmViewSpinLock'.
*
*   INPUTS
*
*       *obj                    SNMP object containing request.
*       idLen                   Length of SNMP request.
*       *param                  Additional parameters (Not used).
*
*   OUTPUTS
*
*       SNMP_NOERROR            Successful.
*       SNMP_NOSUCHNAME         Invalid request.
*       SNMP_INCONSISTANTVALUE  Wrong value in SET request.
*
*************************************************************************/
UINT16 vacmViewSpinLock(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    /* Status to return success roe error code. */
    UINT16              status;

    UNUSED_PARAMETER(param);

    /* If we have valid request. */
    if (MibSimple(obj, idlen) == NU_TRUE)
    {
        /* Set status to success code. */
        status = SNMP_NOERROR;

        /* If we have SNMP set request to handle. */
        if (obj->Request == SNMP_PDU_SET)
        {
            /* If we have valid value to set. */
            if(Vacm_Mib_Table.vacm_view_spin_lock == obj->Syntax.LngUns)
            {
                /* Increment the value of 'vacmViewSpinLock'. */
                Vacm_Mib_Table.vacm_view_spin_lock++;
            }

            /* If we don't have valid value to set then return error code.
             */
            else
            {
                /* Return error code. */
                status = SNMP_INCONSISTANTVALUE;
            }
        }

        /* If we have GET / GET-NEXT or GET-BULK request then proceed. */
        else if ((obj->Request != SNMP_PDU_UNDO) &&
                 (obj->Request != SNMP_PDU_COMMIT))
        {
            obj->Syntax.LngUns = Vacm_Mib_Table.vacm_view_spin_lock;
        }
    }

    /* If we don't have valid request then return error code. */
    else
    {
        /* Return error code. */
        status = SNMP_NOSUCHNAME;
    }

    /* Return success or error code. */
    return (status);

} /* vacmViewSpinLock */

/*************************************************************************
*
*   FUNCTION
*
*       Get_vacmViewTreeEntry
*
*   DESCRIPTION
*
*       This function is used to process SNMP GET/GET-NEXT request on
*       Get_vacmViewTreeFamilyTable.
*
*   INPUTS
*
*       *obj                SNMP object containing request.
*       getflag             Flag to distinguish GET and GET-NEXT requests.
*
*   OUTPUTS
*
*       SNMP_NOERROR        Successful.
*       SNMP_NOSUCHINSTANCE Required instance of object is not found.
*       SNMP_NOSUCHNAME     SNMP Object not found.
*
*************************************************************************/
STATIC UINT16 Get_vacmViewTreeEntry(snmp_object_t *obj,
                                          UINT8 getflag)
{
    /* Table entry object identifier. */
    UINT32                  table_oid[] = {1, 3, 6, 1, 6, 3, 16, 1, 5, 2,
                                           1};

    /* Handle to view tree entry. */
    VACM_VIEWTREE_STRUCT    *vacm_view_tree_entry = NU_NULL;

    /* View name length. */
    UINT32                  vacm_view_name_len;

    /* View name. */
    UINT8                   vacm_view_name[SNMP_SIZE_SMALLOBJECTID];

    /* Subtree length. */
    UINT32                  vacm_subtree_len;

    /* Subtree. */
    UINT32                  vacm_subtree[VACM_SUBTREE_LEN];

    /* Variable to use in for-loop. */
    UINT32                  i, temp;

    /* Status to return success or error code. */
    UINT16                  status = SNMP_NOERROR;

    /* Get the value of view name from object identifier. */
    vacm_view_name_len = obj->Id[VACM_VIEW_TREE_SUB_LEN];

    /* Clear out view name. */
    UTL_Zero(vacm_view_name, sizeof(vacm_view_name));

    /* Get the value of view name. */
    for (i = 0;
         ( (i < vacm_view_name_len) &&
           (i < (SNMP_SIZE_SMALLOBJECTID - 1)) );
         i++)
    {
        vacm_view_name[i] =
                        (UINT8)(obj->Id[VACM_VIEW_TREE_SUB_LEN + 1 + i]);
    }

    /* Get the value of Sub-tree length. */
    vacm_subtree_len = obj->Id[VACM_VIEW_TREE_SUB_LEN + 1 + i];

    /* Get current available position in the local variable. */
    temp = VACM_VIEW_TREE_SUB_LEN + 2 + i;

    /* Clear out subtree value. */
    UTL_Zero(vacm_subtree, sizeof(vacm_subtree));

    /* Get the value of subtree. */
    for (i =  0; ((i < vacm_subtree_len) && (i < VACM_SUBTREE_LEN)); i++)
    {
        vacm_subtree[i] = obj->Id[temp + i];
    }

    /* If we are handling GET request. */
    if (getflag)
    {
        /* If we don't have valid object identifier then return error
         * code.
         */
        if ( ((temp + i) != obj->IdLen) ||
             (i != vacm_subtree_len) ||
             (strlen((CHAR *)(vacm_view_name)) != vacm_view_name_len) )

        {
            /* Return error code. */
            status = SNMP_NOSUCHINSTANCE;
        }
    }

    /* If no error till now then proceed. */
    if (status == SNMP_NOERROR)
    {
        /* Get the handle to the view family tree entry. */
        vacm_view_tree_entry = VACM_GetViewTreeFamily_Entry(
                                vacm_view_name_len, vacm_view_name,
                                vacm_subtree_len, vacm_subtree, getflag);
    }

    /* If we got the handle to the view family tree entry. */
    if (vacm_view_tree_entry)
    {
        switch(obj->Id[VACM_VIEW_TREE_ATTR_OFFSET])
        {
        case 3:                         /* vacmViewTreeFamilyMask */

            /* Getting the value 'vacmViewTreeFamilyMask'. */
            NU_BLOCK_COPY(obj->Syntax.BufChr,
                          vacm_view_tree_entry->vacm_family_mask,
                          VACM_MASK_SIZE);

            /* Get the length of 'vacmViewTreeFamilyMask'. */
            obj->SyntaxLen = vacm_view_tree_entry->vacm_mask_length;

            break;

        case 4:                         /* vacmViewTreeFamilyType */

            /* Get the value of 'vacmViewTreeFamilyType'. */
            obj->Syntax.LngUns = vacm_view_tree_entry->vacm_family_type;

            break;

        case 5:                        /* vacmViewTreeFamilyStorageType */

            /* Get the value of 'vacmViewTreeFamilyStorageType'. */
            obj->Syntax.LngUns = vacm_view_tree_entry->vacm_storage_type;

            break;

        case 6:                         /* vacmViewTreeFamilyStatus */

            /* Get the value of 'vacmViewTreeFamilyStatus'. */
            obj->Syntax.LngUns = vacm_view_tree_entry->vacm_status;

            break;

        default:
            /* We have reached at end of table. */

            /* Return error code. */
            status = SNMP_NOSUCHNAME;
        }

        /* If there is no error till now and we are processing GET-NEXT
         * request then update the object identifier.
         */
        if ((status == SNMP_NOERROR) && (!getflag))
        {
            /* Update table OID in object identifier. */
            NU_BLOCK_COPY(obj->Id, table_oid, sizeof(table_oid));

            /* Update the value of view name length in object identifier.
             */
            obj->Id[VACM_VIEW_TREE_SUB_LEN] =
                    strlen((CHAR *)vacm_view_tree_entry->vacm_view_name);

            /* Update the value of view name in object identifier. */
            for (i = 0;
                 ( (i < obj->Id[VACM_VIEW_TREE_SUB_LEN]) &&
                   (i < (SNMP_SIZE_SMALLOBJECTID - 1)) );
                 i++)
            {
                obj->Id[VACM_VIEW_TREE_SUB_LEN + 1 + i] =
                           (UINT8)vacm_view_tree_entry->vacm_view_name[i];
            }

            /* Update the value of subtree length in object identifier. */
            obj->Id[VACM_VIEW_TREE_SUB_LEN + 1 + i] =
                                vacm_view_tree_entry->vacm_subtree_len;

            temp = VACM_VIEW_TREE_SUB_LEN + 2 + i;

            /* Update the value of subtree in object identifier. */
            for (i =  0;
                 ((i < vacm_view_tree_entry->vacm_subtree_len) &&
                 (i < VACM_SUBTREE_LEN));
                 i++)
            {
                obj->Id[temp + i] = vacm_view_tree_entry->vacm_subtree[i];
            }

            /* Update the object identifier length. */
            obj->IdLen = temp + i;
        }
    }

    /* If we did not GET handle to view tree family entry then return
     * error code.
     */
    else
    {
        /* Return error code. */
        status = SNMP_NOSUCHINSTANCE;
    }

   /* Return success or error code. */
   return (status);

} /* Get_vacmViewTreeEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Set_vacmViewTreeEntry
*
*   DESCRIPTION
*
*       This function is used to process SNMP SET request on
*       Set_vacmViewTreeFamilyTable.
*
*   INPUTS
*
*       *obj                SNMP object containing request.
*
*   OUTPUTS
*
*       SNMP_NOERROR            Successful.
*       SNMP_NOSUCHINSTANCE     Required instance of object is not found.
*       SNMP_NOSUCHNAME         SNMP Object not found.
*       SNMP_INCONSISTANTVALUE  Value to set is inconsistent.
*       SNMP_READONLY           Instance has storage type of read-only.
*       SNMP_WRONGVALUE         Wrong value.
*       SNMP_WRONGLENGTH        Wrong length.
*
*************************************************************************/
STATIC UINT16 Set_vacmViewTreeEntry(const snmp_object_t *obj)
{
    /* Handle to view tree entry. */
    VACM_VIEWTREE_STRUCT    *vacm_view_tree_entry = NU_NULL;

    /* View name length. */
    UINT32                  vacm_view_name_len;

    /* View name. */
    UINT8                   vacm_view_name[SNMP_SIZE_SMALLOBJECTID];

    /* Subtree length. */
    UINT32                  vacm_subtree_len;

    /* Subtree. */
    UINT32                  vacm_subtree[VACM_SUBTREE_LEN];

    /* Variable to use in for-loop. */
    UINT32                  i, temp;

    /* Status to return success or error code. */
    UINT16                  status = SNMP_NOERROR;

    /* Get the value of view name from object identifier. */
    vacm_view_name_len = obj->Id[VACM_VIEW_TREE_SUB_LEN];

    /* Clear out view name. */
    UTL_Zero(vacm_view_name, sizeof(vacm_view_name));

    /* Get the value of view name. */
    for (i = 0;
         ( (i < vacm_view_name_len) &&
           (i < (SNMP_SIZE_SMALLOBJECTID - 1)) );
         i++)
    {
        vacm_view_name[i] =
                        (UINT8)(obj->Id[VACM_VIEW_TREE_SUB_LEN + 1 + i]);
    }

    /* Get the value of subtree value. */
    vacm_subtree_len = obj->Id[VACM_VIEW_TREE_SUB_LEN + 1 + i];

    /* Get the current available position. */
    temp = VACM_VIEW_TREE_SUB_LEN + 2 + i;

    /* Clear out the value of subtree. */
    UTL_Zero(vacm_subtree, sizeof(vacm_subtree));

    /* Get the value of subtree from object identifier. */
    for (i =  0; ((i < vacm_subtree_len) && (i < VACM_SUBTREE_LEN)); i++)
    {
        vacm_subtree[i] = obj->Id[temp + i];
    }

    if ((vacm_view_name_len == 0) ||
        (vacm_view_name_len > SNMP_SIZE_SMALLOBJECTID))
    {
        status = SNMP_NOCREATION;
    }

    /* If we don't have a valid object identifier then return error code.
     */
    else if ( ((temp + i) != obj->IdLen) ||
         (i != vacm_subtree_len) ||
         (strlen((CHAR *)vacm_view_name) != vacm_view_name_len) )

    {
        /* Return error code. */
        status = SNMP_NOSUCHINSTANCE;
    }

    /* If no error till now then get the handle to the view family tree
     * entry.
     */
    if (status == SNMP_NOERROR)
    {
        /* Get the handle to the view family tree from permanent list. */
        vacm_view_tree_entry = VACM_GetViewTree_Entry_Util(
               vacm_view_name_len, vacm_view_name, vacm_subtree_len,
               vacm_subtree, &(Vacm_Mib_Table.vacm_view_tree_family_tab));

        /* If we got the handle to the view tree family entry from the
         * permanent list and storage of the entry is read-only then
         * return error code.
         */
        if ((vacm_view_tree_entry) &&
            (vacm_view_tree_entry->vacm_storage_type ==
                                                   SNMP_STORAGE_READONLY))
        {
            /* Return error code. */
            status = SNMP_READONLY;

            /* Nullify the handle to the view tree family entry, so that
             * we can't alter any value in it.
             */
            vacm_view_tree_entry = NU_NULL;
        }

        /* If we did not get the handle to the view tree family entry from
         * the permanent list then get the handle to view tree family
         * entry from the temporary list.
         */
        else if (!vacm_view_tree_entry)
        {
            /* Get the handle to the view family tree from permanent list.
             */
            vacm_view_tree_entry = VACM_GetViewTree_Entry_Util(
                    vacm_view_name_len, vacm_view_name, vacm_subtree_len,
                    vacm_subtree, &(Vacm_Mib_Temp_View_Family_Root));

            /* If we did not get the handle to the view tree family entry
             * from temporary list then return error code.
             */
            if (!vacm_view_tree_entry)
            {
                /* Return error code. */
                status = SNMP_NOSUCHINSTANCE;
            }
        }
    }

    /* If we have handle to the view tree family entry. */
    if (vacm_view_tree_entry)
    {
        switch(obj->Id[VACM_VIEW_TREE_ATTR_OFFSET])
        {
        case 3:                         /* vacmViewTreeFamilyMask */

            /* If we don't have valid value to set then return error code.
             */
            if (obj->SyntaxLen > VACM_MASK_SIZE)
            {
                /* Return error code. */
                status = SNMP_WRONGLENGTH;
            }
            else
            {
                /* Set the value of 'vacmViewTreeFamilyMask'. */
                NU_BLOCK_COPY(vacm_view_tree_entry->vacm_family_mask,
                              obj->Syntax.BufChr, obj->SyntaxLen);

                /* Update the length of 'vacmViewTreeFamilyMask'. */
                vacm_view_tree_entry->vacm_mask_length = obj->SyntaxLen;
            }

            break;

        case 4:                         /* vacmViewTreeFamilyType */

            /* If we don't have valid value to set then return error code.
             */
            if ( (obj->Syntax.LngUns != VACM_FAMILY_INCLUDED) &&
                 (obj->Syntax.LngUns != VACM_FAMILY_EXCLUDED) )
            {
                /* Return error code. */
                status = SNMP_WRONGVALUE;
            }

            /* If we have valid value to set then set the value of
             * 'vacmViewTreeFamilyType'.
             */
            else
            {
                /* Set the value of 'vacmViewTreeFamilyType'. */
                vacm_view_tree_entry->vacm_family_type =
                                            (UINT8)(obj->Syntax.LngUns);
            }

            break;

        case 5:                        /* vacmViewTreeFamilyStorageType */

            /* If we don't have valid value to set then return error code.
             */
            if ((obj->Syntax.LngUns < 1) || (obj->Syntax.LngUns > 5))
            {
                /* Return error code. */
                status = SNMP_WRONGVALUE;
            }

            /* If we have valid to set then set the value to
             * 'vacmViewTreeFamilyStorageType'.
             */
            else
            {
                /* Set the value to 'vacmViewTreeFamilyStorageType'. */
                vacm_view_tree_entry->vacm_storage_type =
                                            (UINT8)(obj->Syntax.LngUns);
            }

            break;

        case 6:                         /* vacmViewTreeFamilyStatus */

            /* If we don't have valid value to set then return error
             * code.
             */
            if ((obj->Syntax.LngUns < 1) || (obj->Syntax.LngUns > 6))
            {
                /* Return error code. */
                status = SNMP_WRONGVALUE;
            }

            /* If value of row status was not updated in the same request
             * or new value is same as requested previously in the same
             * request then set the value to row flag. Row status will be
             * updated during commit operation.
             */
            else if ((vacm_view_tree_entry->vacm_row_flag == 0) ||
                     (vacm_view_tree_entry->vacm_row_flag ==
                        ((UINT8)obj->Syntax.LngUns)))
            {
                /* Set value to row flag. */
                vacm_view_tree_entry->vacm_row_flag =
                                            (UINT8)(obj->Syntax.LngUns);
            }

            /* If value is inconsistent then return error code. */
            else
            {
                status = SNMP_INCONSISTANTVALUE;
            }

            break;

        default:
            /* We have reached at end of the table. */

            /* Return error code. */
            status = SNMP_NOSUCHNAME;
        }
    }

    /* Return success or error code. */
    return (status);

} /* Set_vacmViewTreeEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Undo_vacmViewTreeEntry
*
*   DESCRIPTION
*
*       This function is used to process SNMP UNDO request on
*       vacmViewTreeFamilyTable.
*
*   INPUTS
*
*       *obj                SNMP object containing request.
*
*   OUTPUTS
*
*       SNMP_NOERROR            Successful.
*
*************************************************************************/
STATIC UINT16 Undo_vacmViewTreeEntry(const snmp_object_t *obj)
{
    /* Handle to view tree entry. */
    VACM_VIEWTREE_STRUCT    *vacm_view_tree_entry = NU_NULL;

    /* View name length. */
    UINT32                  vacm_view_name_len;

    /* View name. */
    UINT8                   vacm_view_name[SNMP_SIZE_SMALLOBJECTID];

    /* Subtree length. */
    UINT32                  vacm_subtree_len;

    /* Subtree. */
    UINT32                  vacm_subtree[VACM_SUBTREE_LEN];

    /* Variable to use in for-loop. */
    UINT32                  i, temp;

    /* Status to return success or error code. */
    UINT16                  status = SNMP_NOERROR;

    /* Get the value of view name from object identifier. */
    vacm_view_name_len = obj->Id[VACM_VIEW_TREE_SUB_LEN];

    /* Clear out view name. */
    UTL_Zero(vacm_view_name, sizeof(vacm_view_name));

    /* Get the value of view name. */
    for (i = 0;
         ( (i < vacm_view_name_len) &&
           (i < (SNMP_SIZE_SMALLOBJECTID - 1)) );
         i++)
    {
        vacm_view_name[i] =
                        (UINT8)(obj->Id[VACM_VIEW_TREE_SUB_LEN + 1 + i]);
    }

    /* Get the value of subtree length. */
    vacm_subtree_len = obj->Id[VACM_VIEW_TREE_SUB_LEN + 1 + i];

    /* Get the current available position. */
    temp = VACM_VIEW_TREE_SUB_LEN + 2 + i;

    /* Clear out subtree value. */
    UTL_Zero(vacm_subtree, sizeof(vacm_subtree));

    /* Get the value of subtree. */
    for (i =  0; ((i < vacm_subtree_len) && (i < VACM_SUBTREE_LEN)); i++)
    {
        vacm_subtree[i] = obj->Id[temp + i];
    }

    /* If we have invalid object identifier then return error code. */
    if ( ((temp + i) != obj->IdLen) ||
         (i != vacm_subtree_len) ||
         (strlen((CHAR *)vacm_view_name) != vacm_view_name_len) )
    {
        /* Return error code. */
        status = SNMP_NOSUCHINSTANCE;
    }

    /* If no error till now then proceed. */
    if (status == SNMP_NOERROR)
    {
        /* Get the handle to view tree family entry from permanent list.
         */
        vacm_view_tree_entry = VACM_GetViewTree_Entry_Util(
            vacm_view_name_len, vacm_view_name, vacm_subtree_len,
            vacm_subtree, &(Vacm_Mib_Table.vacm_view_tree_family_tab));

        /* If we did not get the handle to view tree family entry from
         * permanent list then get the handle to view tree entry from
         * temporary list.
         */
        if (!vacm_view_tree_entry)
        {
            /* Get the handle to the view tree family entry from temporary
             * list.
             */
            vacm_view_tree_entry = VACM_GetViewTree_Entry_Util(
                    vacm_view_name_len, vacm_view_name, vacm_subtree_len,
                    vacm_subtree, &(Vacm_Mib_Temp_View_Family_Root));

            /* If we got the handle to the view tree family entry from
             * temporary list then remove and deallocate entry.
             */
            if (vacm_view_tree_entry)
            {
                /* Remove view tree family entry from temporary list. */
                DLL_Remove(&Vacm_Mib_Temp_View_Family_Root,
                           vacm_view_tree_entry);

                /* Deallocate memory attained by view tree entry. */
                if (NU_Deallocate_Memory(vacm_view_tree_entry) !=
                                                               NU_SUCCESS)
                {
                    NLOG_Error_Log("SNMP: Failed to deallocate memory",
                                    NERR_SEVERE, __FILE__, __LINE__);
                }

                /* Nullify the handle to the view tree family entry, so
                 * that we can't use it.
                 */
                vacm_view_tree_entry = NU_NULL;
            }
        }
    }

    /* If we have handle to the view family tree entry. */
    if (vacm_view_tree_entry)
    {
        switch(obj->Id[VACM_VIEW_TREE_ATTR_OFFSET])
        {
        case 3:                         /* vacmViewTreeFamilyMask */

            /* Set the value of 'vacmViewTreeFamilyMask'. */
            NU_BLOCK_COPY(vacm_view_tree_entry->vacm_family_mask,
                          obj->Syntax.BufChr, obj->SyntaxLen);

            /* Update the length of 'vacmViewTreeFamilyMask'. */
            vacm_view_tree_entry->vacm_mask_length = obj->SyntaxLen;

            break;

        case 4:                         /* vacmViewTreeFamilyType */

            /* Set the value to 'vacmViewTreeFamilyType'. */
            vacm_view_tree_entry->vacm_family_type =
                                            (UINT8)(obj->Syntax.LngUns);

            break;

        case 5:                        /* vacmViewTreeFamilyStorageType */

            /* Set the value to 'vacmViewTreeFamilyStorageType'. */
            vacm_view_tree_entry->vacm_storage_type =
                                            (UINT8)(obj->Syntax.LngUns);

            break;

        case 6:                         /* vacmViewTreeFamilyStatus */

            /* Set the value to 'vacmViewTreeFamilyStatus'. */
            vacm_view_tree_entry->vacm_status =
                                            (UINT8)(obj->Syntax.LngUns);

            break;
        }

        /* Clear out row flag. */
        vacm_view_tree_entry->vacm_row_flag = 0;
    }

    /* Return success or error code. */
    return (SNMP_NOERROR);

} /* Undo_vacmViewTreeEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Create_vacmViewTreeEntry
*
*   DESCRIPTION
*
*       This function is used to process SNMP CREATE request on
*       vacmViewTreeFamilyTable.
*
*   INPUTS
*
*       *obj                SNMP object containing request.
*
*   OUTPUTS
*
*       SNMP_NOERROR            Successful.
*       SNMP_NOCREATION         Creation fail.
*       SNMP_NOSUCHINSTANCE     Creation fail due to wrong OID.
*       SNMP_GENERROR           General error.
*
*************************************************************************/
STATIC UINT16 Create_vacmViewTreeEntry(snmp_object_t *obj)
{
    /* Handle to view tree entry. */
    VACM_VIEWTREE_STRUCT    vacm_view_tree_entry;

    /* View name length. */
    UINT32                  vacm_view_name_len;

    /* View name. */
    UINT8                   vacm_view_name[SNMP_SIZE_SMALLOBJECTID];

    /* Subtree length. */
    UINT32                  vacm_subtree_len;

    /* Subtree. */
    UINT32                  vacm_subtree[VACM_SUBTREE_LEN];

    /* Variable to use in for-loop. */
    UINT32                  i, temp;

    /* Status to return success or error code. */
    UINT16                  status = SNMP_NOERROR;

    /* Get the value of view name from object identifier. */
    vacm_view_name_len = obj->Id[VACM_VIEW_TREE_SUB_LEN];

    /* Clear out view name. */
    UTL_Zero(vacm_view_name, sizeof(vacm_view_name));

    /* Get the value of view name. */
    for (i = 0;
         ( (i < vacm_view_name_len) &&
           (i < (SNMP_SIZE_SMALLOBJECTID - 1)) );
         i++)
    {
        vacm_view_name[i] =
                        (UINT8)(obj->Id[VACM_VIEW_TREE_SUB_LEN + 1 + i]);
    }

    /* Get the value of subtree length. */
    vacm_subtree_len = obj->Id[VACM_VIEW_TREE_SUB_LEN + 1 + i];

    /* Get the current available position. */
    temp = VACM_VIEW_TREE_SUB_LEN + 2 + i;

    /* Clear out the value of subtree. */
    UTL_Zero(vacm_subtree, sizeof(vacm_subtree));

    /* Get the value of subtree. */
    for (i =  0; ((i < vacm_subtree_len) && (i < VACM_SUBTREE_LEN)); i++)
    {
        vacm_subtree[i] = obj->Id[temp + i];
    }

    if ((vacm_view_name_len == 0) ||
        (vacm_view_name_len > SNMP_SIZE_SMALLOBJECTID))
    {
        status = SNMP_NOCREATION;
    }

    /* If we don't have the valid object identifier then return error
     * code.
     */
    else if ( ((temp + i) != obj->IdLen) ||
         (i != vacm_subtree_len) ||
         (strlen((CHAR *)vacm_view_name) != vacm_view_name_len) )

    {
        /* Return error code. */
        status = SNMP_NOSUCHINSTANCE;

        /* If we have row set operation top value of 'create and go' or
         * 'create and wait' then return error code of 'no creation'.
         */
        if ((obj->Id[VACM_VIEW_TREE_ATTR_OFFSET] == 6) &&
            ((obj->Syntax.LngUns == SNMP_ROW_CREATEANDGO) ||
             (obj->Syntax.LngUns == SNMP_ROW_CREATEANDWAIT)))
        {
            /* Return error code of no creation. */
            status = SNMP_NOCREATION;
        }
    }

    /* If no error till now then proceed. */
    if (status == SNMP_NOERROR)
    {
        /* Clear out the view tree family entry structure. */
        UTL_Zero(&vacm_view_tree_entry, sizeof(vacm_view_tree_entry));

        /* Set the value of view name. */
        strcpy((CHAR *)(vacm_view_tree_entry.vacm_view_name),
               (CHAR *)vacm_view_name);

        /* Set the value of subtree. */
        NU_BLOCK_COPY(vacm_view_tree_entry.vacm_subtree, vacm_subtree,
                      vacm_subtree_len * sizeof(UINT32));

        /* Set the value of subtree length. */
        vacm_view_tree_entry.vacm_subtree_len = vacm_subtree_len;

        /* Set the value of mask length. */
        vacm_view_tree_entry.vacm_mask_length = 0;

        /* Set the value of family type. */
        vacm_view_tree_entry.vacm_family_type = VACM_FAMILY_INCLUDED;

        /* Set storage type. */
        vacm_view_tree_entry.vacm_storage_type = SNMP_STORAGE_DEFAULT;

        /* Add view tree family entry in temporary list. */
        if (VACM_InsertMibView_Util(&(vacm_view_tree_entry),
                        &(Vacm_Mib_Temp_View_Family_Root)) != NU_SUCCESS)
        {
            NLOG_Error_Log("SNMP: VACM_InsertMibView_Util failed.",
                            NERR_SEVERE, __FILE__, __LINE__);

            /* Return error code if VACM_InsertMibView_Util failed. */
            status = SNMP_ERROR;
        }
    }

    /* Return success or error code. */
    return (status);

} /* Create_vacmViewTreeEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Commit_vacmViewTreeEntryStatus
*
*   DESCRIPTION
*
*       This function is used to process commit row status request on
*       vacmViewTreeFamilyTable.
*
*   INPUTS
*
*       *vacm_view_entry        Handle to view tree family entry.
*       row_status              New value to row status.
*       is_new                  Flag to distinguish new and old entries.
*
*   OUTPUTS
*
*       SNMP_NOERROR            Successful.
*       SNMP_INCONSISTANTVALUE  Inconsistent value.
*
*************************************************************************/
STATIC UINT16 Commit_vacmViewTreeEntryStatus(
                  VACM_VIEWTREE_STRUCT *vacm_view_entry, UINT8 row_status,
                  UINT8 is_new)
{
    /* Status for returning success or error code. */
    UINT16          status = SNMP_NOERROR;

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
            if(!is_new)
            {
                /* Activating the entry. */
                vacm_view_entry->vacm_status = SNMP_ROW_ACTIVE;
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
            if(!is_new)
            {
                /* Setting the row status to 'NOTINSERVICE'. */
                vacm_view_entry->vacm_status = SNMP_ROW_NOTINSERVICE;
            }

            else
            {
                /* Inconsistent value. */
                status = SNMP_INCONSISTANTVALUE;
            }

            break;

        case SNMP_ROW_NOTREADY:

            /* Setting the row status to 'NOTINSERVICE'. */
            vacm_view_entry->vacm_status = SNMP_ROW_NOTINSERVICE;

            break;

        case SNMP_ROW_DESTROY:

            /* Setting row status to SNMP_ROW_DESTROY. */
            vacm_view_entry->vacm_status = SNMP_ROW_DESTROY;

            break;

        default:
            if ((vacm_view_entry->vacm_row_flag == 0) ||
                (vacm_view_entry->vacm_row_flag == SNMP_ROW_DESTROY))
            {
                status = SNMP_INCONSISTANTVALUE;
            }
            else if (vacm_view_entry->vacm_row_flag ==
                                            SNMP_ROW_CREATEANDGO)
            {
                vacm_view_entry->vacm_status = SNMP_ROW_ACTIVE;
            }
            else if (vacm_view_entry->vacm_row_flag ==
                                            SNMP_ROW_CREATEANDWAIT)
            {
                vacm_view_entry->vacm_status = SNMP_ROW_NOTINSERVICE;
            }

        }
    }

    else
    {
        switch(row_status)
        {
        case SNMP_ROW_CREATEANDGO:

            /* Entry should be ready and either new or non-active. */
            if((is_new) ||
               (vacm_view_entry->vacm_status != SNMP_ROW_ACTIVE))
            {
                /* Activating the entry. */
                vacm_view_entry->vacm_status = SNMP_ROW_ACTIVE;
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
            if(vacm_view_entry->vacm_status != SNMP_ROW_ACTIVE)
            {
                vacm_view_entry->vacm_status = SNMP_ROW_NOTINSERVICE;
            }

            else
            {
                /* Setting status to error code of in-consistent
                   value. */
                status = SNMP_INCONSISTANTVALUE;
            }

            break;

        default:

            if ((vacm_view_entry->vacm_row_flag == 0) ||
                (vacm_view_entry->vacm_row_flag == SNMP_ROW_DESTROY))
            {
                vacm_view_entry->vacm_status = SNMP_ROW_NOTINSERVICE;
            }
            else if (vacm_view_entry->vacm_row_flag ==
                                                SNMP_ROW_CREATEANDGO)
            {
                vacm_view_entry->vacm_status = SNMP_ROW_ACTIVE;
            }
            else if (vacm_view_entry->vacm_row_flag ==
                                            SNMP_ROW_CREATEANDWAIT)
            {
                vacm_view_entry->vacm_status = SNMP_ROW_NOTINSERVICE;
            }

        }
    }

    /* Return success or error code. */
    return (status);

} /* Commit_vacmViewTreeEntryStatus */

/*************************************************************************
*
*   FUNCTION
*
*       Commit_vacmViewTreeEntries
*
*   DESCRIPTION
*
*       This function is used to process commit vacmViewTreeFamilyTable
*       entries for moving all the temporary entries to permanent entries.
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
STATIC VOID Commit_vacmViewTreeEntries(VOID)
{
    /* Handle to the view tree family entry. */
    VACM_VIEWTREE_STRUCT    *vacm_view_entry;

    /* Start traversing temporary list. */
    vacm_view_entry = Vacm_Mib_Temp_View_Family_Root.next;

    /* Loop to traverse temporary list. */
    while (vacm_view_entry)
    {
        /* Add entry to the permanent list. */
        if (VACM_InsertMibView(vacm_view_entry) != NU_SUCCESS)
        {
            NLOG_Error_Log("SNMP: VACM_InsertMibView Failed",
                            NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Remove entry from the temporary list. */
        DLL_Remove(&Vacm_Mib_Temp_View_Family_Root, vacm_view_entry);

        /* Deallocate memory attained by view tree family entry. */
        if (NU_Deallocate_Memory(vacm_view_entry) != NU_SUCCESS)
        {
            NLOG_Error_Log("SNMP: Memory deallocation failed.",
                            NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Moving forward in the temporary list. */
        vacm_view_entry = Vacm_Mib_Temp_View_Family_Root.next;
    }

    /* Start traversing permanent list. */
    vacm_view_entry = Vacm_Mib_Table.vacm_view_tree_family_tab.next;

    /* Loop to clear row flag. */
    while (vacm_view_entry)
    {
        /* Clear out row flag. */
        vacm_view_entry->vacm_row_flag = 0;

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)
        VACM_Save_View(vacm_view_entry);
#endif

        /* Moving forward in the list. */
        vacm_view_entry = vacm_view_entry->next;
    }

} /* Commit_vacmViewTreeEntries */

/*************************************************************************
*
*   FUNCTION
*
*       vacmViewTreeEntry
*
*   DESCRIPTION
*
*       This function is used to process SNMP request on
*       vacmViewTreeFamilyTable.
*
*   INPUTS
*
*       *obj                SNMP object containing request.
*
*   OUTPUTS
*
*       SNMP_NOERROR            Successful.
*       SNMP_NOSUCHINSTANCE     Required instance of object is not found.
*       SNMP_NOSUCHNAME         SNMP Object not found.
*       SNMP_INCONSISTANTVALUE  Value to set is inconsistent.
*       SNMP_READONLY           Instance has storage type of read-only.
*       SNMP_WRONGVALUE         Wrong value.
*       SNMP_WRONGLENGTH        Wrong length.
*       SNMP_NOCREATION         Creation fail.
*       SNMP_GENERROR           General error.
*
*************************************************************************/
UINT16 vacmViewTreeEntry(snmp_object_t *obj, UINT16 idlen,
                               VOID *param)
{
    /* Handle to the vacmViewTreeEntry. */
    VACM_VIEWTREE_STRUCT    *vacm_view_entry = NU_NULL;

    /* Temporary handle to the vacmViewTreeEntry. */
    VACM_VIEWTREE_STRUCT    *temp_vacm_view_entry = NU_NULL;

    /* Transitions from one row status value to another. */
    UINT16                  status_trans[6][4] =
    {
     {SNMP_INCONSISTANTVALUE, SNMP_INCONSISTANTVALUE, SNMP_NOERROR,
      SNMP_NOERROR},
     {SNMP_INCONSISTANTVALUE, SNMP_INCONSISTANTVALUE, SNMP_NOERROR,
      SNMP_NOERROR},
     {SNMP_WRONGVALUE, SNMP_INCONSISTANTVALUE,
      SNMP_WRONGVALUE, SNMP_WRONGVALUE},
     {SNMP_NOERROR, SNMP_INCONSISTANTVALUE, SNMP_INCONSISTANTVALUE,
      SNMP_INCONSISTANTVALUE},
     {SNMP_NOERROR, SNMP_INCONSISTANTVALUE, SNMP_INCONSISTANTVALUE,
      SNMP_INCONSISTANTVALUE},
     {SNMP_NOERROR, SNMP_NOERROR, SNMP_NOERROR, SNMP_NOERROR}
    };

    /* View name length. */
    UINT32                  vacm_view_name_len;

    /* View name. */
    UINT8                   vacm_view_name[SNMP_SIZE_SMALLOBJECTID];

    /* Subtree length. */
    UINT32                  vacm_subtree_len;

    /* Subtree. */
    UINT32                  vacm_subtree[VACM_SUBTREE_LEN];

    /* Status to return success or error code. */
    UINT16              status = SNMP_NOERROR;

    /* Variable to use in for-loop. */
    UINT32              i, temp, current_state;

    /* Flag to distinguish between get and get-next requests. */
    UINT8               getflag = 0;

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
        status = Get_vacmViewTreeEntry(obj, getflag);

        break;

    case SNMP_PDU_BULK:                     /* Get-bulk request. */

        /* Processing of bulk operation. */
        status = SNMP_Get_Bulk(obj, Get_vacmViewTreeEntry);

        break;

    case SNMP_PDU_SET:                      /* Set request. */

        VACM_Mib_View_Tree_Commit_Left++;

        /* Process the SET operation. */
        status = Set_vacmViewTreeEntry(obj);

        if (status != SNMP_NOSUCHINSTANCE)
            break;

    case SNMP_PDU_CREATE:

        /* Processing of create operation. */
        status = Create_vacmViewTreeEntry(obj);

        /* If the entry was successfully created, set the value. */
        if(status == SNMP_NOERROR)
            status = Set_vacmViewTreeEntry(obj);

        break;

    case SNMP_PDU_UNDO:                     /* Undo request. */

        VACM_Mib_View_Tree_Commit_Left = 0;

        /* Processing of UNDO operations. */
        status = Undo_vacmViewTreeEntry(obj);

        break;

    case SNMP_PDU_COMMIT:                   /* Commit request. */

        VACM_Mib_View_Tree_Commit_Left--;

        /* Get the value of view name from object identifier. */
        vacm_view_name_len = obj->Id[VACM_VIEW_TREE_SUB_LEN];

        /* Clear out view name. */
        UTL_Zero(vacm_view_name, sizeof(vacm_view_name));

        /* Get the value of view name. */
        for (i = 0;
             ( (i < vacm_view_name_len) &&
               (i < (SNMP_SIZE_SMALLOBJECTID - 1)) );
             i++)
        {
            vacm_view_name[i] =
                        (UINT8)(obj->Id[VACM_VIEW_TREE_SUB_LEN + 1 + i]);
        }

        /* Get the value of subtree length. */
        vacm_subtree_len = obj->Id[VACM_VIEW_TREE_SUB_LEN + 1 + i];

        /* Get the current available position. */
        temp = VACM_VIEW_TREE_SUB_LEN + 2 + i;

        /* Clear out the value of subtree. */
        UTL_Zero(vacm_subtree, sizeof(vacm_subtree));

        /* Get the value of subtree. */
        for (i =  0;
             ((i < vacm_subtree_len) && (i < VACM_SUBTREE_LEN));
             i++)
        {
            vacm_subtree[i] = obj->Id[temp + i];
        }

        /* If we have invalid object identifier then return error code. */
        if ( ((temp + i) != obj->IdLen) ||
             (i != vacm_subtree_len) ||
             (strlen((CHAR *)vacm_view_name) != vacm_view_name_len) )
        {
            /* Return error code. */
            status = SNMP_NOSUCHINSTANCE;
        }

        /* If no error is encountered till now then proceed. */
        if (status == SNMP_NOERROR)
        {
            /* Get the handle to the view tree family entry from temporary
             * list.
             */
            vacm_view_entry = VACM_GetViewTree_Entry_Util(
                                       vacm_view_name_len, vacm_view_name,
                                       vacm_subtree_len, vacm_subtree,
                                       &Vacm_Mib_Temp_View_Family_Root);

            /* Have a copy of handle in another local variable. */
            temp_vacm_view_entry = vacm_view_entry;

            /* If we did get the handle to the view tree family entry
             * then get the handle to the view tree entry from permanent
             * list.
             */
            if (!vacm_view_entry)
            {
                /* Get the handle to the view tree family entry from the
                 * list.
                 */
                vacm_view_entry = VACM_GetViewTree_Entry_Util(
                            vacm_view_name_len, vacm_view_name,
                            vacm_subtree_len, vacm_subtree,
                            &(Vacm_Mib_Table.vacm_view_tree_family_tab));
            }
        }

        /* If we got the handle either from permanent or temporary list.
         */
        if (vacm_view_entry)
        {
            /* Check whether it was a row set operation. */
            if (obj->Id[VACM_VIEW_TREE_ATTR_OFFSET] == 6)
            {
                /* Set the value of current state.
                 *  0) New entry.
                 *  1) Not ready.
                 *  2) Not in service.
                 *  3) Active.
                 */

                /* If the handle is from temporary list then set value of
                 * current state to 0.
                 */
                if (temp_vacm_view_entry)
                {
                    current_state = 0;
                }

                /* If entry is from permanent list and row status is not
                 * active then set the value of current state to 1.
                 */
                else if (vacm_view_entry->vacm_status != SNMP_ROW_ACTIVE)
                {
                    current_state = 2;
                }

                /* If the entry is from permanent list and is active then
                 * set the current state to 3.
                 */
                else
                {
                    current_state = 3;
                }

                /* If required state transition is not allowed then return
                 * error code.
                 */
                if (status_trans[obj->Syntax.LngUns - 1][current_state] !=
                                                             SNMP_NOERROR)
                {
                    /* Return error code. */
                    status =
                      status_trans[obj->Syntax.LngUns - 1][current_state];
                }

                /* If the request is valid then commit the row status
                 * value.
                 */
                else
                {
                    status = Commit_vacmViewTreeEntryStatus(
                            vacm_view_entry, (UINT8)(obj->Syntax.LngUns),
                            (UINT8)(temp_vacm_view_entry != NU_NULL) );
                }
            }

            /* If it was not row set operation. */
            else
            {
                /* Commit row status. */
                status = Commit_vacmViewTreeEntryStatus(
                    vacm_view_entry, vacm_view_entry->vacm_status, NU_FALSE);
            }

            /* If the row status is set to 'DESTROY'. */
            if (vacm_view_entry->vacm_status == SNMP_ROW_DESTROY)
            {
                /* If handle was from temporary list then remove it from
                 * temporary list.
                 */
                if (temp_vacm_view_entry)
                {
                    DLL_Remove(&Vacm_Mib_Temp_View_Family_Root,
                               vacm_view_entry);
                }

                /* If handle was from permanent list then remove it from
                 * permanent list.
                 */
                else
                {
                    DLL_Remove(&Vacm_Mib_Table.vacm_view_tree_family_tab,
                               vacm_view_entry);
                }

                /* Deallocate memory attained by this entry structure.
                 */
                if (NU_Deallocate_Memory(vacm_view_entry) != NU_SUCCESS)
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
        if ( (status == SNMP_NOERROR) &&
             (VACM_Mib_View_Tree_Commit_Left == 0) )
        {
            Commit_vacmViewTreeEntries();
        }

        break;

    default:                                /* Invalid request. */
        status = SNMP_GENERROR;
    }

    /* Return success or error code. */
    return (status);

} /* vacmViewTreeEntry */

#endif /* (INCLUDE_MIB_VACM == NU_TRUE) */
