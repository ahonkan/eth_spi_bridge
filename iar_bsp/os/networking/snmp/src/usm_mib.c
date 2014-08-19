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
*       usm_mib.c                                                
*
*   COMPONENT
*
*       USM MIBs
*
*   DESCRIPTION
*
*       This file implements the USM MIB as defined in RFC2574.
*
*   DATA STRUCTURES
*
*       Temp_USM_Mib_Root
*       USM_MIB_Commit_Left
*
*   FUNCTIONS
*
*       usmStatsUnsupportedSecLevels
*       usmStatsNotInTimeWindows
*       usmStatsUnknownUserNames
*       usmStatsUnknownEngineIDs
*       usmStatsWrongDigests
*       usmStatsDecryptionErrors
*       usmUserSpinLock
*       Get_usmUserEntry
*       Create_usmUserEntry
*       Commit_usmUserEntryStatus
*       Commit_usmUserEntries
*       Undo_usmUserEntry
*       Set_usmUserEntry
*       usmUserEntry
*
*   DEPENDENCIES
*
*       target.h
*       externs.h
*       xtypes.h
*       snmp.h
*       agent.h
*       mib.h
*       usm.h
*       snmp_g.h
*       usm_mib.h
*
*************************************************************************/
#include "networking/target.h"
#include "networking/externs.h"
#include "networking/xtypes.h"
#include "networking/snmp.h"
#include "networking/agent.h"
#include "networking/mib.h"
#include "networking/usm.h"
#include "networking/snmp_g.h"
#include "networking/usm_mib.h"

#if (INCLUDE_MIB_USM == NU_TRUE)

USM_USERS_TABLE_ROOT            Temp_USM_Mib_Root;
STATIC UINT32                   USM_MIB_Commit_Left;

STATIC UINT16 Get_usmUserEntry(snmp_object_t *obj, UINT8 getflag);
STATIC UINT16 Create_usmUserEntry(const snmp_object_t *obj);
STATIC UINT16 Commit_usmUserEntryStatus(USM_USERS_STRUCT *usm_user_ptr,
                                        UINT8 row_status, UINT8 is_new);
STATIC VOID Commit_usmUserEntries(VOID);
STATIC UINT16 Undo_usmUserEntry(const snmp_object_t *obj);
STATIC UINT16 Set_usmUserEntry(const snmp_object_t *obj);

extern USM_MIB_STRUCT       Usm_Mib;
extern USM_AUTH_PROT_STRUCT Usm_Auth_Prot_Table[USM_MAX_AUTH_PROTOCOLS];
extern USM_PRIV_PROT_STRUCT Usm_Priv_Prot_Table[USM_MAX_PRIV_PROTOCOLS];
/************************************************************************
*
*   FUNCTION
*
*       usmStatsUnsupportedSecLevels
*
*   DESCRIPTION
*
*       This function processes the PDU action on the
*       usmStatsUnsupportedSecLevels.
*
*   INPUTS
*
*       *obj                A pointer to the object.
*       idlen               The length of the object ID.
*       *param              Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME     The object does not exist.
*       SNMP_NOERROR        The request was processed successfully.
*
*************************************************************************/
UINT16 usmStatsUnsupportedSecLevels(snmp_object_t *Obj, UINT16 IdLen,
                                    VOID *param)
{
    UINT16  status;

    UNUSED_PARAMETER(param);

    /* If the request is invalid. */
    if ((Obj->Request == SNMP_PDU_SET) ||
        (MibSimple(Obj, IdLen) == NU_FALSE))
    {
        /* Return error code. */
        status = SNMP_NOSUCHNAME;
    }

    /* If we have valid request. */
    else
    {
        /* Get the required value. */
        Obj->Syntax.LngUns =
                Usm_Mib.usm_stats_tab.usm_unsupported_sec_level;

        /* Return success code. */
        status = SNMP_NOERROR;
    }

    /* Return success or error code. */
    return (status);

} /* usmStatsUnsupportedSecLevels */

/************************************************************************
*
*   FUNCTION
*
*       usmStatsNotInTimeWindows
*
*   DESCRIPTION
*
*       This function processes the PDU action on the
*       usmStatsNotInTimeWindows.
*
*   INPUTS
*
*       *obj                A pointer to the object.
*       IdLen               The length of the object ID.
*       *param              Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME     The object does not exist.
*       SNMP_NOERROR        The request was processed successfully.
*
*************************************************************************/
UINT16 usmStatsNotInTimeWindows(snmp_object_t *Obj, UINT16 IdLen,
                                VOID *param)
{
    UINT16  status;

    UNUSED_PARAMETER(param);

    /* If we request is invalid. */
    if ((Obj->Request == SNMP_PDU_SET) ||
        (MibSimple(Obj, IdLen) == NU_FALSE))
    {
        /* Return error code. */
        status = SNMP_NOSUCHNAME;
    }

    /* If we have valid request. */
    else
    {
        /* Get the value of */
        Obj->Syntax.LngUns = Usm_Mib.usm_stats_tab.usm_not_in_time_win;

        /* Return success code. */
        status = SNMP_NOERROR;
    }

    /* Return success or error code. */
    return (status);

} /* usmStatsNotInTimeWindows */

/************************************************************************
*
*   FUNCTION
*
*       usmStatsUnknownUserNames
*
*   DESCRIPTION
*
*       This function processes the PDU action on the
*       usmStatsUnknownUserNames.
*
*   INPUTS
*
*       *obj                A pointer to the object.
*       IdLen               The length of the object ID.
*       *param              Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME     The object does not exist.
*       SNMP_NOERROR        The request was processed successfully.
*
*************************************************************************/
UINT16 usmStatsUnknownUserNames(snmp_object_t *Obj, UINT16 IdLen,
                                VOID *param)
{
    UINT16  status;

    /* Avoiding compilation warning. */
    UNUSED_PARAMETER(param);

    /* If the request is invalid. */
    if ((Obj->Request == SNMP_PDU_SET) ||
        (MibSimple(Obj, IdLen) == NU_FALSE))
    {
        /* Return error code. */
        status = SNMP_NOSUCHNAME;
    }

    /* If request is valid. */
    else
    {
        /* Get value of usmStatsUnknownUserNames. */
        Obj->Syntax.LngUns = Usm_Mib.usm_stats_tab.usm_unkown_user_name;

        /* Return success code. */
        status = SNMP_NOERROR;
    }

    /* Return success or error code. */
    return (status);

} /* usmStatsUnknownUserNames */

/************************************************************************
*
*   FUNCTION
*
*       usmStatsUnknownEngineIDs
*
*   DESCRIPTION
*
*       This function processes the PDU action on the
*       usmStatsUnknownEngineIDs.
*
*   INPUTS
*
*       *obj                A pointer to the object.
*       IdLen               The length of the object ID.
*       *param              Unused parameter.
*
*   OUTPUTS
*
*
*       SNMP_NOSUCHNAME     The object does not exist.
*       SNMP_NOERROR        The request was processed successfully.
*
*************************************************************************/
UINT16 usmStatsUnknownEngineIDs(snmp_object_t *Obj, UINT16 IdLen,
                                VOID *param)
{
    UINT16  status;

    /* Avoiding compilation warning. */
    UNUSED_PARAMETER(param);

    /* If request is invalid. */
    if ((Obj->Request == SNMP_PDU_SET) ||
        (MibSimple(Obj, IdLen) == NU_FALSE))
    {
        /* Return error code. */
        status = SNMP_NOSUCHNAME;
    }
    else
    {
        /* Get the value of usmStatsUnknownEngineIDs. */
        Obj->Syntax.LngUns = Usm_Mib.usm_stats_tab.usm_unknown_engine_id;

        /* Return success code. */
        status = SNMP_NOERROR;
    }

    /* Return success or error code. */
    return (status);

} /* usmStatsUnknownEngineIDs */

/************************************************************************
*
*   FUNCTION
*
*       usmStatsWrongDigests
*
*   DESCRIPTION
*
*       This function processes the PDU action on the
*       usmStatsWrongDigests.
*
*   INPUTS
*
*       *obj                A pointer to the object.
*       IdLen               The length of the object ID.
*       *param              Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME     The object does not exist.
*       SNMP_NOERROR        The request was processed successfully.
*
*************************************************************************/
UINT16 usmStatsWrongDigests(snmp_object_t *Obj, UINT16 IdLen, VOID *param)
{
    /* Status to return success or error code. */
    UINT16          status;

    /* Avoiding compilation warning. */
    UNUSED_PARAMETER(param);

    /* If request is invalid. */
    if ((Obj->Request == SNMP_PDU_SET) ||
        (MibSimple(Obj, IdLen) == NU_FALSE))
    {
        /* Return error code. */
        status = SNMP_NOSUCHNAME;
    }

    /* If request is valid. */
    else
    {
        /* Get the value of usmStatsWrongDigests. */
        Obj->Syntax.LngUns = Usm_Mib.usm_stats_tab.usm_wrong_digests;

        /* Return success code. */
        status = SNMP_NOERROR;
    }

    /* Return success or error code. */
    return (status);

} /* usmStatsWrongDigests */

/************************************************************************
*
*   FUNCTION
*
*       usmStatsDecryptionErrors
*
*   DESCRIPTION
*
*       This function processes the PDU action on the
*       usmStatsDecryptionErrors.
*
*   INPUTS
*
*       *obj                A pointer to the object.
*       IdLen               The length of the object ID.
*       *param              Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME     The object does not exist.
*       SNMP_NOERROR        The request was processed successfully.
*
*************************************************************************/
UINT16 usmStatsDecryptionErrors(snmp_object_t *Obj, UINT16 IdLen,
                                VOID *param)
{
    /* Status to return success or error code. */
    UINT16  status;

    /* Avoiding compilation warning. */
    UNUSED_PARAMETER(param);

    /* If we have invalid request. */
    if ((Obj->Request == SNMP_PDU_SET) ||
        (MibSimple(Obj, IdLen) == NU_FALSE))
    {
        /* Return error code. */
        status = SNMP_NOSUCHNAME;
    }

    /* If we have valid request. */
    else
    {
        /* Get the value of usmStatsDecryptionErrors. */
        Obj->Syntax.LngUns = Usm_Mib.usm_stats_tab.usm_decryption_err;

        /* Return success code. */
        status = SNMP_NOERROR;
    }

    /* Return success or error code. */
    return (status);

} /* usmStatsDecryptionErrors */

/************************************************************************
*
*   FUNCTION
*
*       usmUserSpinLock
*
*   DESCRIPTION
*
*       This function processes the PDU action on the usmUserSpinLock.
*
*   INPUTS
*
*       *obj                A pointer to the object.
*       IdLen               The length of the object ID.
*       *param              Unused parameter.
*
*   OUTPUTS
*
*       SNMP_NOSUCHNAME     The object does not exist.
*       SNMP_NOERROR        The request was processed successfully.
*
*************************************************************************/
UINT16 usmUserSpinLock(snmp_object_t *Obj, UINT16 IdLen, VOID *param)
{
    UINT16  status = SNMP_NOERROR;

    UNUSED_PARAMETER(param);

    if (MibSimple(Obj, IdLen) == NU_TRUE)
    {
        if (Obj->Request == SNMP_PDU_SET)
        {
            if(Usm_Mib.usm_user_spin_lock == Obj->Syntax.LngUns)
            {
                Usm_Mib.usm_user_spin_lock++;
            }
            else
            {
                status = SNMP_INCONSISTANTVALUE;
            }
        }
        else if ((Obj->Request != SNMP_PDU_UNDO) &&
                 (Obj->Request != SNMP_PDU_COMMIT))
        {
            Obj->Syntax.LngUns = Usm_Mib.usm_user_spin_lock;
        }
    }
    else
    {
        /* Return error code. */
        status = SNMP_NOSUCHNAME;
    }

    /* Return success or error code. */
    return (status);

} /* usmUserSpinLock */

/*************************************************************************
*
*   FUNCTION
*
*       Get_usmUserEntry
*
*   DESCRIPTION
*
*       This function is used to handle SNMP Get request on usmUserEntry.
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
*                               non-existing USM user structure.
*       SNMP_NOSUCHNAME         When object identifier refers to a
*                               non-existing or non-accessible attribute.
*       SNMP_NOERROR            When successful.
*
*************************************************************************/
STATIC UINT16 Get_usmUserEntry(snmp_object_t *obj, UINT8 getflag)
{
    /* Object identifier of table entry. */
    UINT32              table_oid[] = {1, 3, 6, 1, 6, 3, 15, 1, 2, 2, 1};

    /* OID of 'snmpAuthProtocols'. */
    UINT32              snmp_auth_protocols[] = {1, 3, 6, 1, 6, 3, 10, 1,
                                                 1, 0};

    /* Length of OID of 'snmpAuthProtocols'. */
    UINT32              snmp_auth_protocol_len = 10;

    /* OID of 'snmpPrivProtocols'. */
    UINT32              snmp_priv_protocols[] = {1, 3, 6, 1, 6, 3, 10, 1,
                                                 2, 0};

    /* Length of OID of 'snmpPrivProtocols'. */
    UINT32              snmp_priv_protocol_len = 10;

    /* Handle to usmUserEntry. */
    USM_USERS_STRUCT    *usm_user_ptr = NU_NULL;

    /* Engine ID. */
    UINT8               engine_id[SNMP_SIZE_SMALLOBJECTID];

    /* Engine ID length. */
    UINT32              engine_id_len;

    /* User name. */
    UINT8               user_name[SNMP_SIZE_SMALLOBJECTID];

    /* User name length. */
    UINT32              user_name_len = 0;

    /* Status to return success or error code. */
    UINT16      status = SNMP_NOERROR;

    /* Variable to use in for-loop. */
    UINT32      i, temp;

    /* Clear out the the engine ID. */
    UTL_Zero(engine_id, sizeof(engine_id));

    /* Clear out the user name. */
    UTL_Zero(user_name, sizeof(user_name));

    engine_id_len = obj->Id[SNMP_USM_MIB_SUB_LEN];

    if (engine_id_len > SNMP_SIZE_SMALLOBJECTID)
        status = SNMP_GENERROR;
    else
    {
        for (i = 0; i < engine_id_len; i++)
        {
            engine_id[i] = (UINT8)(obj->Id[i + 1 + SNMP_USM_MIB_SUB_LEN]);
        }

        user_name_len = obj->Id[engine_id_len + 1 + SNMP_USM_MIB_SUB_LEN];

        if (user_name_len > SNMP_SIZE_SMALLOBJECTID)
            status = SNMP_GENERROR;
        else
        {
            temp = engine_id_len + 2 + SNMP_USM_MIB_SUB_LEN;

            for (i = 0; i < user_name_len; i++)
            {
                user_name[i] = (UINT8)(obj->Id[temp + i]);
            }

            if ( (getflag) &&
                 (obj->IdLen != (SNMP_USM_MIB_SUB_LEN + 2 +
                                 user_name_len + engine_id_len)) )
            {
                status = SNMP_NOSUCHINSTANCE;
            }
        }
    }

    if (status == SNMP_NOERROR)
    {
        usm_user_ptr = Get_USM_User_Entry(engine_id, engine_id_len,
                                          user_name, user_name_len,
                                          getflag);

        if (!usm_user_ptr)
            status = SNMP_NOSUCHINSTANCE;
    }

    if (usm_user_ptr)
    {
        switch(obj->Id[SNMP_USM_MIB_ATTR_OFFSET])
        {
        case 3:                             /* usmUserSecurityName */

            /* Get Syntax length. */
            obj->SyntaxLen =
                    strlen((CHAR *)usm_user_ptr->usm_security_name);

            /* Get the value of 'usmUserSecurityName'. */
            NU_BLOCK_COPY(obj->Syntax.BufChr,
                        usm_user_ptr->usm_security_name, obj->SyntaxLen);

            break;

        case 4:                             /* usmUserCloneFrom */

            /* Set value to Null OID. */
            UTL_Zero(obj->Syntax.BufInt,
                    (sizeof(UINT32) * SNMP_SIZE_BUFINT));

            obj->SyntaxLen = 2;

            break;

        case 5:                             /* usmUserAuthProtocol */

            /* Set the value to OID of 'snmpAuthProtocols'. */
            NU_BLOCK_COPY(obj->Syntax.BufInt, snmp_auth_protocols,
                    (sizeof(UINT32)*snmp_auth_protocol_len));

            /* Identify the actual authentication protocol. */
            obj->Syntax.BufInt[snmp_auth_protocol_len - 1] =
                                         usm_user_ptr->usm_auth_index + 1;

            /* Set the syntax length. */
            obj->SyntaxLen = snmp_auth_protocol_len;

            break;

        case 6:                             /* usmUserAuthKeyChange */
        case 7:                             /* usmUserOwnAuthKeyChange */
        case 9:                             /* usmUserPrivKeyChange */
        case 10:                            /* usmUserOwnPrivKeyChange */

            /* Copying the required value in the buffer. */
            obj->SyntaxLen = 0;
            strcpy((CHAR*)obj->Syntax.BufChr ,(CHAR*)"");

            break;

        case 8:                             /* usmUserPrivProtocol */

            /* Set the value to OID of 'snmpPrivProtocols'. */
            NU_BLOCK_COPY(obj->Syntax.BufInt, snmp_priv_protocols,
                   (sizeof(UINT32) * snmp_priv_protocol_len));

            /* Specify the required privacy protocol. */
            obj->Syntax.BufInt[snmp_priv_protocol_len - 1] =
                                         usm_user_ptr->usm_priv_index + 1;

            /* Set the length of OID stored in value. */
            obj->SyntaxLen = snmp_priv_protocol_len;

            break;

        case 11:                            /* usmUserPublic */

            /* Copying the required value in the buffer. */
            obj->SyntaxLen = (UINT32)strlen((CHAR*)usm_user_ptr->
                                                        usm_user_public);

            strcpy((CHAR*)obj->Syntax.BufChr,
                   (CHAR*)usm_user_ptr->usm_user_public);

            break;

        case 12:                            /* usmUserStorageType */

            /* Getting the value of 'usmUserStorageType'. */
            obj->Syntax.LngUns = (UINT32)(usm_user_ptr->usm_storage_type);

            break;

        case 13:                            /* usmUserStatus */

            /* Getting the value of 'usmUserStatus'. */
            obj->Syntax.LngUns = usm_user_ptr->usm_status;

            break;

        default:    /* We have reached at end of the table. */

            /* Return error code. */
            status = SNMP_NOSUCHNAME;
        }

        /* Update OID if we are handling GET-NEXT request. */
        if ((status == SNMP_NOERROR) && (!getflag))
        {
            /* Update the table OID. */
            NU_BLOCK_COPY(obj->Id, table_oid, sizeof(table_oid));

            /* Updating the length of engine ID. */
            obj->Id[SNMP_USM_MIB_SUB_LEN] = (UINT32)(usm_user_ptr->
                                                usm_user_engine_id_len);

            /* Update the value of engine ID. */
            for (i = 0; i < (UINT32)(usm_user_ptr->
                                            usm_user_engine_id_len); i++)
            {
				if (i < SNMP_SIZE_SMALLOBJECTID)
				{
	                obj->Id[SNMP_USM_MIB_SUB_LEN + i + 1] =
	                            (UINT32)(usm_user_ptr->usm_user_engine_id[i]);
				}
				
				else
					break;
            }

            /* Get the consumed OID length. */
            temp = SNMP_USM_MIB_SUB_LEN + i + 1;

			if (temp < SNMP_SIZE_OBJECTID)
			{
	            /* Update the length of user name. */
	            obj->Id[temp] = strlen((CHAR *)(usm_user_ptr->usm_user_name));
	
	            /* Update the value of user name. */
	            for (i = 0; i < obj->Id[temp]; i++)
	            {
	                obj->Id[temp + i + 1] =
	                                (UINT32)(usm_user_ptr->usm_user_name[i]);
	            }
	
	            /* Update the OID length. */
	            obj->IdLen = temp + i + 1;
			}
			
			else
				status = SNMP_GENERROR;
        }
    }

    /* Return success or error code. */
    return (status);

} /* Get_usmUserEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Create_usmUserEntry
*
*   DESCRIPTION
*
*       This function is used to handle SNMP Create request on
*       usmUserEntry.
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
STATIC UINT16 Create_usmUserEntry(const snmp_object_t *obj)
{
    /* Handle to usmUserEntry. */
    USM_USERS_STRUCT    usm_user_ptr;

    /* Engine ID. */
    UINT8       engine_id[SNMP_SIZE_SMALLOBJECTID];

    /* Engine ID length. */
    UINT32      engine_id_len;

    /* User name. */
    UINT8       user_name[SNMP_SIZE_SMALLOBJECTID];

    /* User name length. */
    UINT32      user_name_len;

    /* Status to return success or error code. */
    UINT16      status = SNMP_NOERROR;

    /* Variable to use in for-loop. */
    UINT32      i, temp;

    /* Clear out the the engine ID. */
    UTL_Zero(engine_id, sizeof(engine_id));

    /* Clear out the user name. */
    UTL_Zero(user_name, sizeof(user_name));

    engine_id_len = obj->Id[SNMP_USM_MIB_SUB_LEN];

    if (engine_id_len > SNMP_SIZE_SMALLOBJECTID)
        status = SNMP_GENERROR;
    else
    {
        for (i = 0; i < engine_id_len; i++)
        {
            engine_id[i] = (UINT8)(obj->Id[i + 1 + SNMP_USM_MIB_SUB_LEN]);
        }

        user_name_len = obj->Id[engine_id_len + 1 + SNMP_USM_MIB_SUB_LEN];

        if (user_name_len > SNMP_SIZE_SMALLOBJECTID)
            status = SNMP_GENERROR;
        else
        {
            temp = engine_id_len + 2 + SNMP_USM_MIB_SUB_LEN;

            for (i = 0; i < user_name_len; i++)
            {
                user_name[i] = (UINT8)(obj->Id[temp + i]);
            }

            if (obj->IdLen != (2 + user_name_len + engine_id_len +
                               SNMP_USM_MIB_SUB_LEN))
            {
                status = SNMP_NOSUCHINSTANCE;
            }
        }
    }

    if (status == SNMP_NOERROR)
    {
        /* Clear out the USM user structure. */
        UTL_Zero(&usm_user_ptr, sizeof(USM_USERS_STRUCT));

        /* Set value of engine ID. */
        NU_BLOCK_COPY(usm_user_ptr.usm_user_engine_id, engine_id,
               engine_id_len);

        /* Set the value of engine ID length. */
        usm_user_ptr.usm_user_engine_id_len = (UINT8)engine_id_len;

        /* Set the value of user name. */
        strcpy((CHAR*)usm_user_ptr.usm_user_name, (CHAR*)user_name);

        /* Set the value of security name. */
        strcpy((CHAR*)usm_user_ptr.usm_security_name, (CHAR*)user_name);

        /* Set the value of storage type. */
        usm_user_ptr.usm_storage_type = SNMP_STORAGE_DEFAULT;

        /* Set the value of row status. */
        usm_user_ptr.usm_status = SNMP_ROW_CREATEANDWAIT;

        /* Set the value of authentication protocol as
         * 'usmNoAuthProtocol'.
         */
        usm_user_ptr.usm_auth_index = 0;

        /* Set the value of privacy protocol as 'usmNoPrivProtocol'.
         */
        usm_user_ptr.usm_priv_index = 0;

        if (USM_Add_User_Util(&usm_user_ptr, &Temp_USM_Mib_Root) !=
                                                            NU_SUCCESS)
        {
            NLOG_Error_Log("SNMP: Failed to add USM user.", NERR_SEVERE,
                            __FILE__, __LINE__);

            status = SNMP_GENERROR;
        }
    }

    else if ((obj->Id[SNMP_USM_MIB_ATTR_OFFSET] == 13) &&
             ((obj->Syntax.LngUns == SNMP_ROW_CREATEANDGO) ||
              (obj->Syntax.LngUns == SNMP_ROW_CREATEANDWAIT)))
    {
        status = SNMP_NOCREATION;
    }

    /* Return success or error code. */
    return (status);

}/* Create_usmUserEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Commit_usmUserEntryStatus
*
*   DESCRIPTION
*
*       This function is used to commit status of USM user structure.
*
*   INPUTS
*
*       *usm_user_ptr           Pointer to USM user structure.
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
STATIC UINT16 Commit_usmUserEntryStatus(USM_USERS_STRUCT *usm_user_ptr,
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
                /* Activating the entry. */
                usm_user_ptr->usm_status = SNMP_ROW_ACTIVE;
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
                usm_user_ptr->usm_status = SNMP_ROW_NOTINSERVICE;
            }

            else
            {
                /* Inconsistent value. */
                status = SNMP_INCONSISTANTVALUE;
            }

            break;

        case SNMP_ROW_DESTROY:

            /* Setting row status to SNMP_ROW_DESTROY. */
            usm_user_ptr->usm_status = SNMP_ROW_DESTROY;

            break;

        default:
            if ((usm_user_ptr->usm_row_flag == 0) ||
                (usm_user_ptr->usm_row_flag == SNMP_ROW_DESTROY))
            {
                status = SNMP_INCONSISTANTVALUE;
            }
            else if (usm_user_ptr->usm_row_flag == SNMP_ROW_CREATEANDGO)
            {
                usm_user_ptr->usm_status = SNMP_ROW_ACTIVE;
            }
            else if (usm_user_ptr->usm_row_flag == SNMP_ROW_CREATEANDWAIT)
            {
                usm_user_ptr->usm_status = SNMP_ROW_NOTINSERVICE;
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
               (usm_user_ptr->usm_status != SNMP_ROW_ACTIVE))
            {
                /* Activating the entry. */
                usm_user_ptr->usm_status = SNMP_ROW_ACTIVE;
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
            if(usm_user_ptr->usm_status != SNMP_ROW_ACTIVE)
            {
                usm_user_ptr->usm_status = SNMP_ROW_NOTINSERVICE;
            }

            else
            {
                /* Setting status to error code of in-consistent
                   value. */
                status = SNMP_INCONSISTANTVALUE;
            }

            break;

        default:

            if ((usm_user_ptr->usm_row_flag == 0) ||
                (usm_user_ptr->usm_row_flag == SNMP_ROW_DESTROY))
            {
                usm_user_ptr->usm_status = SNMP_ROW_NOTINSERVICE;
            }
            else if (usm_user_ptr->usm_row_flag ==
                                                SNMP_ROW_CREATEANDGO)
            {
                usm_user_ptr->usm_status = SNMP_ROW_ACTIVE;
            }
            else if (usm_user_ptr->usm_row_flag ==
                                            SNMP_ROW_CREATEANDWAIT)
            {
                usm_user_ptr->usm_status = SNMP_ROW_NOTINSERVICE;
            }

        }
    }

    /* Return success or error code. */
    return (status);

} /* Commit_usmUserEntryStatus */

/*************************************************************************
*
*   FUNCTION
*
*       Commit_usmUserEntries
*
*   DESCRIPTION
*
*       This function is used to commit all the newly created USM user
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
STATIC VOID Commit_usmUserEntries(VOID)
{
    /* Handle to the USM user structure. */
    USM_USERS_STRUCT    *usm_user_ptr = Temp_USM_Mib_Root.next;

    /* Loop till there exists an entry in temporary list. */
    while(usm_user_ptr)
    {
        /* Add USM user structure into permanent list. */
        if (USM_Add_User(usm_user_ptr) != NU_SUCCESS)
        {
            NLOG_Error_Log("SNMP: Failed to Add USM user entry",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* Remove a USM user structure from the temporary list. */
        DLL_Dequeue(&Temp_USM_Mib_Root);

        /* Deallocate memory of USM user structure as USM_Add_User
         * creates its own copy.
         */
        if (NU_Deallocate_Memory(usm_user_ptr) != NU_SUCCESS)
        {
            NLOG_Error_Log("SNMP: Failed to deallocate memory",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* Getting next handle to the USM user structure from the
         * temporary list.
         */
        usm_user_ptr = Temp_USM_Mib_Root.next;
    }

    /* Get the starting handle of USM user entry. */
    usm_user_ptr = Usm_Mib.usm_user_table.next;

    /* Loop to clear out all the row flags. */
    while (usm_user_ptr)
    {
        /* Clear out the row flag. */
        usm_user_ptr->usm_row_flag = 0;

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)
        /* Update data in file. */
        USM_Save_User(usm_user_ptr);
#endif
        
        /* Moving forward in the list. */
        usm_user_ptr = usm_user_ptr->next;
    }

} /* Commit_usmUserEntries */

/*************************************************************************
*
*   FUNCTION
*
*       Undo_usmUserEntry
*
*   DESCRIPTION
*
*       This function is used handle SNMP UNDO request on USM user
*       structures.
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
STATIC UINT16 Undo_usmUserEntry(const snmp_object_t *obj)
{
    /* Handle to usmUserEntry. */
    USM_USERS_STRUCT        *usm_user_ptr = NU_NULL;

    /* Handle to Clone from usmUserEntry. */
    USM_USERS_STRUCT        *cf_usm_user_ptr;

    /* Engine ID. */
    UINT8                   engine_id[SNMP_SIZE_SMALLOBJECTID];

    /* Clone From Engine ID. */
    UINT8                   cf_engine_id[SNMP_SIZE_SMALLOBJECTID];

    /* Engine ID length. */
    UINT32                  engine_id_len;

    /* Clone From Engine ID Length. */
    UINT32                  cf_engine_id_len;

    /* User name. */
    UINT8                   user_name[SNMP_SIZE_SMALLOBJECTID];

    /* Clone From User Name. */
    UINT8                   cf_user_name[SNMP_SIZE_SMALLOBJECTID];

    /* User name length. */
    UINT32                  user_name_len = 0;

    /* Clone From User name length. */
    UINT32                  cf_user_name_len = 0;

    /* Status to return success or error code. */
    UINT16                  status = SNMP_NOERROR;

    /* Variable to use in for-loop. */
    UINT32                  i, temp;

    /* Clear out the the engine ID. */
    UTL_Zero(engine_id, sizeof(engine_id));

    /* Clear out the user name. */
    UTL_Zero(user_name, sizeof(user_name));

    engine_id_len = obj->Id[SNMP_USM_MIB_SUB_LEN];

    if (engine_id_len > SNMP_SIZE_SMALLOBJECTID)
        status = SNMP_GENERROR;
    else
    {
        for (i = 0; i < engine_id_len; i++)
        {
            engine_id[i] = (UINT8)(obj->Id[i + 1 + SNMP_USM_MIB_SUB_LEN]);
        }

        user_name_len = obj->Id[engine_id_len + 1 + SNMP_USM_MIB_SUB_LEN];

        if (user_name_len > SNMP_SIZE_SMALLOBJECTID)
            status = SNMP_GENERROR;
        else
        {
            temp = engine_id_len + 2 + SNMP_USM_MIB_SUB_LEN;

            for (i = 0; i < user_name_len; i++)
            {
                user_name[i] = (UINT8)(obj->Id[temp + i]);
            }

            if (obj->IdLen != (2 + user_name_len + engine_id_len +
                               SNMP_USM_MIB_SUB_LEN))
            {
                status = SNMP_NOSUCHINSTANCE;
            }
        }
    }

    if (status == SNMP_NOERROR)
    {
        usm_user_ptr = Get_USM_User_Entry_Util(engine_id, engine_id_len,
                           user_name, user_name_len,  &Temp_USM_Mib_Root);

        if (usm_user_ptr)
        {
            DLL_Remove(&Temp_USM_Mib_Root, usm_user_ptr);

            if (NU_Deallocate_Memory(usm_user_ptr) != NU_SUCCESS)
            {
                NLOG_Error_Log("SNMP: Failed to deallocate memory",
                                NERR_SEVERE, __FILE__, __LINE__);
            }

            usm_user_ptr = NU_NULL;
        }
        else
        {
            usm_user_ptr = Get_USM_User_Entry_Util(engine_id,
                                  engine_id_len, user_name,
                                  user_name_len, &Usm_Mib.usm_user_table);
        }
    }

    if (usm_user_ptr)
    {
        /* Clear out the row flag. */
        usm_user_ptr->usm_row_flag = 0;

        switch(obj->Id[SNMP_USM_MIB_ATTR_OFFSET])
        {
        case 4:                             /* usmUserCloneFrom */

            /* Clear out the the clone from engine ID. */
            UTL_Zero(cf_engine_id, sizeof(cf_engine_id));

            /* Clear out the clone from user name. */
            UTL_Zero(cf_user_name, sizeof(cf_user_name));

            /* Get the value of clone from engine ID length. */
            cf_engine_id_len = obj->Syntax.BufInt[SNMP_USM_MIB_SUB_LEN];

            /* Validating the value of clone from engine ID length. */
            if (cf_engine_id_len > SNMP_SIZE_SMALLOBJECTID)
            {
                /* Return error code if clone from engine ID length is
                 * invalid.
                 */
                status = SNMP_WRONGVALUE;
            }

            /* If we have valid clone from engine ID length. */
            else
            {
                /* Loop to get the value of clone from engine ID. */
                for (i = 0; i < cf_engine_id_len; i++)
                {
                    /* Getting value of clone from engine ID. */
                    cf_engine_id[i] =
                        (UINT8)(obj->Syntax.
                                    BufInt[i + 1 + SNMP_USM_MIB_SUB_LEN]);
                }

                /* Getting length of clone from user name. */
                cf_user_name_len =
                    obj->Syntax.BufInt
                            [engine_id_len + 1 + SNMP_USM_MIB_SUB_LEN];

                /* Validating the clone from user name length. */
                if (cf_user_name_len > SNMP_SIZE_SMALLOBJECTID)
                {
                    /* Return error code if we don't have valid clone from
                     * user name length.
                     */
                    status = SNMP_WRONGVALUE;
                }

                /* If we have the valid clone from user name length. */
                else
                {
                    /* Calculating the starting point of clone user name
                     * in OID in syntax of object.
                     */
                    temp = cf_engine_id_len + 2 + SNMP_USM_MIB_SUB_LEN;

                    /* Loop to get the clone from user name. */
                    for (i = 0; i < cf_user_name_len; i++)
                    {
                        /* Getting value of clone from user name. */
                        cf_user_name[i] =
                                    (UINT8)(obj->Syntax.BufInt[temp + i]);
                    }

                    /* If we don't consume all OID then return error code.
                     */
                    if (obj->SyntaxLen !=
                               (2 + cf_user_name_len + cf_engine_id_len))
                    {
                        /* Returning error code. */
                        status = SNMP_WRONGVALUE;
                    }
                }
            }

            /* If no error till now then proceed. */
            if (status == SNMP_NOERROR)
            {
                /* Get the handle to the USM clone from entry. */
                cf_usm_user_ptr = Get_USM_User_Entry(cf_engine_id,
                                        cf_engine_id_len, cf_user_name,
                                        cf_user_name_len, NU_TRUE);

                /* If we got the handle to the USM clone from entry. */
                if (cf_usm_user_ptr)
                {
                    /* Copy 'usmUserAuthProtocol'. */
                    usm_user_ptr->usm_auth_index =
                                        cf_usm_user_ptr->usm_auth_index;

                    /* Copy 'usmUserPrivProtocol'. */
                    usm_user_ptr->usm_priv_index =
                                        cf_usm_user_ptr->usm_priv_index;

                    /* Copy 'usmUserAuthKey'. */
                    NU_BLOCK_COPY(usm_user_ptr->usm_auth_key,
                           cf_usm_user_ptr->usm_auth_key,
                           USM_KEYCHANGE_MAX_SIZE);

                    /* Copy 'usmUserOwnAuthKey'. */
                    NU_BLOCK_COPY(usm_user_ptr->usm_own_auth_key,
                           cf_usm_user_ptr->usm_own_auth_key,
                           USM_KEYCHANGE_MAX_SIZE);

                    /* Copy 'usmUserPrivKey'. */
                    NU_BLOCK_COPY(usm_user_ptr->usm_priv_key,
                           cf_usm_user_ptr->usm_priv_key,
                           USM_KEYCHANGE_MAX_SIZE);

                    /* Copy 'usmUserOwnPrivKey'. */
                    NU_BLOCK_COPY(usm_user_ptr->usm_own_priv_key,
                           cf_usm_user_ptr->usm_own_priv_key,
                           USM_KEYCHANGE_MAX_SIZE);
                }
            }

            break;

        case 5:                             /* usmUserAuthProtocol */

            usm_user_ptr->usm_auth_index =
                               obj->Syntax.BufInt[obj->SyntaxLen - 1] - 1;

            break;

        case 6:                             /* usmUserAuthKeyChange */
        case 9:                             /* usmUserPrivKeyChange */
            break;

        case 8:                             /* usmUserPrivProtocol */

            usm_user_ptr->usm_priv_index =
                             obj->Syntax.BufInt[obj->SyntaxLen - 1] - 1;

            break;

        case 11:                            /* usmUserPublic */

            if (strlen((CHAR *)obj->Syntax.BufChr) <
                                               SNMP_SIZE_SMALLOBJECTID)
            {
                strcpy((CHAR *)usm_user_ptr->usm_user_public,
                       (CHAR *)obj->Syntax.BufChr);
            }

            break;

        case 12:                            /* usmUserStorageType */

            usm_user_ptr->usm_storage_type = (UINT8)(obj->Syntax.LngUns);


            break;

        case 13:                            /* usmUserStatus */

            usm_user_ptr->usm_status = (UINT8)(obj->Syntax.LngUns);

            break;
        }
    }

    status = SNMP_NOERROR;

    /* Return success code. */
    return (status);

} /* Undo_usmUserEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Set_usmUserEntry
*
*   DESCRIPTION
*
*       This function is used to handle SNMP SET request on USM user
*       structures.
*
*   INPUTS
*
*       *obj                    The SNMP object containing request.
*
*   OUTPUTS
*
*       SNMP_NOSUCHINSTANCE     When there does not exist an USM user
*                               structure specified by OID.
*       SNMP_NOSUCHNAME         When required attribute does not exists
*                               or inaccessible.
*       SNMP_WRONGLENGTH        When syntax length is not valid.
*       SNMP_WRONGVALUE         When value is not valid to set.
*       SNMP_NOERROR            When successful.
*
*************************************************************************/
STATIC UINT16 Set_usmUserEntry(const snmp_object_t *obj)
{
    /* Handle to usmUserEntry. */
    USM_USERS_STRUCT        *usm_user_ptr = NU_NULL;

    /* Handle to clone from usmUserEntry. */
    USM_USERS_STRUCT        *cf_usm_user_ptr;

    /* Engine ID. */
    UINT8                   engine_id[SNMP_SIZE_SMALLOBJECTID];

    /* Clone From Engine ID. */
    UINT8                   cf_engine_id[SNMP_SIZE_SMALLOBJECTID];

    /* Engine ID length. */
    UINT32                  engine_id_len;

    /* Clone From Engine ID length. */
    UINT32                  cf_engine_id_len;

    /* User name. */
    UINT8                   user_name[SNMP_SIZE_SMALLOBJECTID];

    /* Clone From user name. */
    UINT8                   cf_user_name[SNMP_SIZE_SMALLOBJECTID];

    /* User name length. */
    UINT32                  user_name_len = 0;

    /* Clone From user name length. */
    UINT32                  cf_user_name_len = 0;

    /* Status to return success or error code. */
    UINT16                  status = SNMP_NOERROR;

    /* Variable to use in for-loop. */
    UINT32                  i, temp;

    /* AuthProtocol OID. */
    UINT32                 auth_prot_oid[] = {1, 3, 6, 1, 6, 3, 10, 1, 1};

    /* PrivProtocol OID. */
    UINT32                 priv_prot_oid[] = {1, 3, 6, 1, 6, 3, 10, 1, 2};

    /* Flag to represent that entry to process is new entry. */
    UINT8                   is_new = NU_FALSE;

    /* Clear out the the engine ID. */
    UTL_Zero(engine_id, sizeof(engine_id));

    /* Clear out the user name. */
    UTL_Zero(user_name, sizeof(user_name));

    engine_id_len = obj->Id[SNMP_USM_MIB_SUB_LEN];

    if (engine_id_len > SNMP_SIZE_SMALLOBJECTID)
        status = SNMP_GENERROR;
    else
    {
        for (i = 0; i < engine_id_len; i++)
        {
            engine_id[i] = (UINT8)(obj->Id[i + 1 + SNMP_USM_MIB_SUB_LEN]);
        }

        user_name_len = obj->Id[engine_id_len + 1 + SNMP_USM_MIB_SUB_LEN];

        if (user_name_len > SNMP_SIZE_SMALLOBJECTID)
            status = SNMP_GENERROR;
        else
        {
            temp = engine_id_len + 2 + SNMP_USM_MIB_SUB_LEN;

            for (i = 0; i < user_name_len; i++)
            {
                user_name[i] = (UINT8)(obj->Id[temp + i]);
            }

            if (obj->IdLen != (2 + user_name_len + engine_id_len +
                               SNMP_USM_MIB_SUB_LEN))
            {
                status = SNMP_NOSUCHINSTANCE;
            }
        }
    }

    if (status == SNMP_NOERROR)
    {
        usm_user_ptr = Get_USM_User_Entry_Util(engine_id, engine_id_len,
                                               user_name, user_name_len,
                                               &Usm_Mib.usm_user_table);

        if ( (usm_user_ptr) &&
             (usm_user_ptr->usm_storage_type == SNMP_STORAGE_READONLY) )
        {
             status = SNMP_READONLY;
        }
        else if (!usm_user_ptr)
        {
            usm_user_ptr = Get_USM_User_Entry_Util(engine_id,
                                       engine_id_len, user_name,
                                       user_name_len, &Temp_USM_Mib_Root);

            if (!usm_user_ptr)
                status = SNMP_NOSUCHINSTANCE;
            else
                is_new = NU_TRUE;
        }
    }

    if (usm_user_ptr)
    {
        switch(obj->Id[SNMP_USM_MIB_ATTR_OFFSET])
        {
        case 4:                             /* usmUserCloneFrom */

            /* Clear out the the clone from engine ID. */
            UTL_Zero(cf_engine_id, sizeof(cf_engine_id));

            /* Clear out the clone from user name. */
            UTL_Zero(cf_user_name, sizeof(cf_user_name));

            /* Get the value of clone from engine ID length. */
            cf_engine_id_len = obj->Syntax.BufInt[SNMP_USM_MIB_SUB_LEN];

            /* Validating the value of clone from engine ID length. */
            if (cf_engine_id_len > SNMP_SIZE_SMALLOBJECTID)
            {
                /* Return error code if clone from engine ID length is
                 * invalid.
                 */
                status = SNMP_WRONGVALUE;
            }

            /* If we have valid clone from engine ID length. */
            else
            {
                /* Loop to get the value of clone from engine ID. */
                for (i = 0; i < cf_engine_id_len; i++)
                {
                    /* Getting value of clone from engine ID. */
                    cf_engine_id[i] =
                        (UINT8)(obj->Syntax.
                                    BufInt[i + 1 + SNMP_USM_MIB_SUB_LEN]);
                }

                /* Getting length of clone from user name. */
                cf_user_name_len =
                    obj->Syntax.BufInt
                            [engine_id_len + 1 + SNMP_USM_MIB_SUB_LEN];

                /* Validating the clone from user name length. */
                if (cf_user_name_len > SNMP_SIZE_SMALLOBJECTID)
                {
                    /* Return error code if we don't have valid clone from
                     * user name length.
                     */
                    status = SNMP_WRONGVALUE;
                }

                /* If we have the valid clone from user name length. */
                else
                {
                    /* Calculating the starting point of clone user name
                     * in OID in syntax of object.
                     */
                    temp = cf_engine_id_len + 2 + SNMP_USM_MIB_SUB_LEN;

                    /* Loop to get the clone from user name. */
                    for (i = 0; i < cf_user_name_len; i++)
                    {
                        /* Getting value of clone from user name. */
                        cf_user_name[i] =
                                    (UINT8)(obj->Syntax.BufInt[temp + i]);
                    }

                    /* If we don't consume all OID then return error code.
                     */
                    if (obj->SyntaxLen !=
                               (2 + cf_user_name_len + cf_engine_id_len +
                                SNMP_USM_MIB_SUB_LEN))
                    {
                        /* Returning error code. */
                        status = SNMP_WRONGVALUE;
                    }
                }
            }

            /* If no error till now then proceed. */
            if (status == SNMP_NOERROR)
            {
                /* Get the handle to the USM clone from entry. */
                cf_usm_user_ptr = Get_USM_User_Entry(cf_engine_id,
                                    cf_engine_id_len, cf_user_name,
                                    cf_user_name_len, NU_TRUE);

                /* If we got the handle to the USM clone from entry. */
                if (cf_usm_user_ptr)
                {
                    /* Copy 'usmUserAuthProtocol'. */
                    usm_user_ptr->usm_auth_index =
                                        cf_usm_user_ptr->usm_auth_index;

                    /* Copy 'usmUserPrivProtocol'. */
                    usm_user_ptr->usm_priv_index =
                                        cf_usm_user_ptr->usm_priv_index;

                    /* Copy 'usmUserAuthKey'. */
                    NU_BLOCK_COPY(usm_user_ptr->usm_auth_key,
                           cf_usm_user_ptr->usm_auth_key,
                           USM_KEYCHANGE_MAX_SIZE);

                    /* Copy 'usmUserOwnAuthKey'. */
                    NU_BLOCK_COPY(usm_user_ptr->usm_own_auth_key,
                           cf_usm_user_ptr->usm_own_auth_key,
                           USM_KEYCHANGE_MAX_SIZE);

                    /* Copy 'usmUserPrivKey'. */
                    NU_BLOCK_COPY(usm_user_ptr->usm_priv_key,
                           cf_usm_user_ptr->usm_priv_key,
                           USM_KEYCHANGE_MAX_SIZE);

                    /* Copy 'usmUserOwnPrivKey'. */
                    NU_BLOCK_COPY(usm_user_ptr->usm_own_priv_key,
                           cf_usm_user_ptr->usm_own_priv_key,
                           USM_KEYCHANGE_MAX_SIZE);
                }

                /* If we did not get the handle to the USM clone from
                 * entry.
                 */
                else
                {
                    status = SNMP_INCONSISTANTNAME;
                }
            }
            else
            {
                status = SNMP_INCONSISTANTNAME;
            }

            break;

        case 5:                             /* usmUserAuthProtocol */

            /* If the entry is new. */
            if (is_new)
            {
                /* If an initial set operation (i.e. at row creation time)
                 * tries to set a value for an unknown or unsupported
                 * protocol, then a 'wrongValue' error must be returned.
                 */
                status = SNMP_WRONGVALUE;

                if ((obj->SyntaxLen == (SNMP_USM_AUTH_PROT_OID_LEN + 1))
                    && (memcmp(auth_prot_oid, obj->Syntax.BufInt,
                               sizeof(auth_prot_oid)) == 0))
                {
                    for(i = 0; i < USM_MAX_AUTH_PROTOCOLS; i++)
                    {
                        if(Usm_Auth_Prot_Table[i].usm_index ==
                                   obj->Syntax.BufInt[obj->SyntaxLen - 1]-1 )
                        {
                            status = SNMP_NOERROR;
                            break;
                        }
                    }
                }
            }

            /* If USM entry was already present. */
            else
            {
                /* Once instantiated, the value of such an instance of
                 * this object can only be changed via a set operation to
                 * the value of the usmNoAuthProtocol.
                 *
                 * If a set operation tries to change the value of an
                 * existing instance of this object to any value other
                 * than usmNoAuthProtocol, then an 'inconsistentValue'
                 * error must be returned.
                 */
                if ((obj->SyntaxLen != (SNMP_USM_AUTH_PROT_OID_LEN + 1))
                    || (memcmp(auth_prot_oid, obj->Syntax.BufInt,
                               sizeof(auth_prot_oid)) != 0)
                    || (obj->Syntax.BufInt[obj->SyntaxLen - 1] != 1) )
                    status = SNMP_INCONSISTANTVALUE;

                else
                    status = SNMP_NOERROR;
            }

            /* If validation goes successful then update the value. */
            if (status == SNMP_NOERROR)
            {
                if ((!is_new) && (usm_user_ptr->usm_priv_index == USM_DES) && 
                    (obj->Syntax.BufInt[obj->SyntaxLen - 1] - 1 == USM_NOAUTH))
                {
                    status = SNMP_INCONSISTANTVALUE;
                }
                else
                {
                    usm_user_ptr->usm_auth_index =
                        obj->Syntax.BufInt[obj->SyntaxLen - 1] - 1;
                }
            }

            break;

        case 6:                             /* usmUserAuthKeyChange */

            /* Value will be set in commit phase. */
            status = SNMP_NOERROR;

            break;

        case 7:                            /* usmUserOwnAuthKeyChange */

            if (obj->SyntaxLen > SNMP_SIZE_SMALLOBJECTID) 
            {
                status = SNMP_WRONGLENGTH;
            }
            else if ((!is_new) && (obj->SyntaxLen ==0)) 
            {
                status = SNMP_WRONGLENGTH;
            }
            else
            {
                status = SNMP_NOERROR;
            }

            break;

        case 8:                             /* usmUserPrivProtocol */

            /* If the USM entry is under creation. */
            if (is_new)
            {
                /* If an initial set operation (i.e. at row creation time)
                 * tries to set a value for an unknown or unsupported
                 * protocol, then a 'wrongValue' error must be returned.
                 */
                status = SNMP_WRONGVALUE;

                if ( (obj->SyntaxLen == (SNMP_USM_PRIV_PROT_OID_LEN + 1))
                     && (memcmp(priv_prot_oid, obj->Syntax.BufInt,
                            sizeof(priv_prot_oid)) == 0) )
                {
                    for(i = 0; i < USM_MAX_PRIV_PROTOCOLS; i++)
                    {
                        if(Usm_Priv_Prot_Table[i].usm_index ==
                                    obj->Syntax.BufInt[obj->SyntaxLen -1]-1)
                        {
                            status = SNMP_NOERROR;
                        }
                    }
                }
            }

            /* If USM entry already exists. */
            else
            {
                /*  Once instantiated, the value of such an instance of
                 * this object can only be changed via a set operation to
                 * the value of the usmNoPrivProtocol.
                 *
                 * If a set operation tries to change the value of an
                 * existing instance of this object to any value other
                 * than usmNoPrivProtocol, then an 'inconsistentValue'
                 * error must be returned.
                 */
                if ( (obj->SyntaxLen != (SNMP_USM_PRIV_PROT_OID_LEN + 1))
                     || (obj->Syntax.BufInt[obj->SyntaxLen - 1] != 1) ||
                     (memcmp(priv_prot_oid, obj->Syntax.BufInt,
                            sizeof(priv_prot_oid)) != 0) )
                {
                    status = SNMP_INCONSISTANTVALUE;
                }
                else
                {
                    status = SNMP_NOERROR;
                }
            }

            /* Perform actual set operation if error till now. */
            if (status == SNMP_NOERROR)
            {
                usm_user_ptr->usm_priv_index =
                    obj->Syntax.BufInt[obj->SyntaxLen - 1] - 1;
            }

            break;

        case 9:                             /* usmUserPrivKeyChange */
            /* Value will be set during commit phase. */
            if (obj->SyntaxLen > SNMP_SIZE_SMALLOBJECTID) 
            {
                status = SNMP_WRONGLENGTH;
            }
            else
            {
                status = SNMP_NOERROR;
            }

            break;

        case 10:                            /* usmUserOwnPrivKeyChange */

            if (obj->SyntaxLen > SNMP_SIZE_SMALLOBJECTID) 
            {
                status = SNMP_WRONGLENGTH;
            }
            else if ((!is_new) && (obj->SyntaxLen ==0)) 
            {
                status = SNMP_WRONGLENGTH;
            }
            else
            {
                status = SNMP_NOERROR;
            }

            break;

        case 11:                            /* usmUserPublic */

            if (strlen((CHAR *)obj->Syntax.BufChr) >=
                                                SNMP_SIZE_SMALLOBJECTID)
                status = SNMP_WRONGLENGTH;
            else
            {
                strcpy((CHAR *)usm_user_ptr->usm_user_public,
                       (CHAR *)obj->Syntax.BufChr);
            }

            break;

        case 12:                            /* usmUserStorageType */
            if (obj->Syntax.LngUns < 1 || obj->Syntax.LngUns > 5)
            {
                status = SNMP_WRONGVALUE;
            }

            /* A permanent or read only entry cannot be changed. */
            else if (
                (usm_user_ptr->usm_storage_type == SNMP_STORAGE_PERMANENT)
                ||
                (usm_user_ptr->usm_storage_type == SNMP_STORAGE_READONLY)) 
            {
                status = SNMP_WRONGVALUE;
            }

            /* User who employs authentication or privacy must allow its
            * secret(s) to be updated and thus cannot be 'readOnly'.
            */
            else if ((obj->Syntax.LngUns == SNMP_STORAGE_READONLY) &&
                ((usm_user_ptr->usm_auth_index != 0) ||
                (usm_user_ptr->usm_priv_index != 0)))
            {
                status = SNMP_INCONSISTANTVALUE;
            }

            else
            {
                usm_user_ptr->usm_storage_type =
                            (UINT8)(obj->Syntax.LngUns);
            }

            break;

        case 13:                            /* usmUserStatus */

            /* Validate the row status value. */
            if ((obj->Syntax.LngUns < 1) || (obj->Syntax.LngUns > 6))
            {
                status = SNMP_WRONGVALUE;
            }
            else if ((usm_user_ptr->usm_row_flag == 0) ||
                     (usm_user_ptr->usm_row_flag ==
                        ((UINT8)obj->Syntax.LngUns)))
            {
                usm_user_ptr->usm_row_flag = (UINT8)(obj->Syntax.LngUns);
            }
            else
            {
                status = SNMP_INCONSISTANTVALUE;
            }

            break;

        default:                            /* We have reached at end of
                                             * table.
                                             */
            status = SNMP_NOSUCHNAME;
        }
    }

    /* Return success or error code. */
    return (status);

} /* Set_usmUserEntry */

/*************************************************************************
*
*   FUNCTION
*
*       usmUserEntry
*
*   DESCRIPTION
*
*       This function is used to handle SNMP requests on USM user
*       structures.
*
*   INPUTS
*
*       *obj                    The SNMP object containing request.
*       idlen                   Unused.
*       *param                  Unused.
*
*   OUTPUTS
*
*       SNMP_NOSUCHINSTANCE     When there does not exist an USM user
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
UINT16 usmUserEntry(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    /* Handle to the usmUserEntry. */
    USM_USERS_STRUCT        *usm_user_ptr = NU_NULL;

    /* Temporary handle to the usmUserEntry. */
    USM_USERS_STRUCT        *temp_usm_user_ptr = NU_NULL;

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

    UINT32                  current_state;

    /* Handle to authentication protocol. */
    USM_AUTH_PROT_STRUCT    *auth_prot;

    /* Engine ID. */
    UINT8               engine_id[SNMP_SIZE_SMALLOBJECTID];

    /* Engine ID length. */
    UINT32              engine_id_len;

    /* User name. */
    UINT8               user_name[SNMP_SIZE_SMALLOBJECTID];

    /* User name length. */
    UINT32              user_name_len = 0;

    /* Status to return success or error code. */
    UINT16              status = SNMP_NOERROR;

    /* Variable to use in for-loop. */
    UINT32              i, temp;

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
        status = Get_usmUserEntry(obj, getflag);

        break;

    case SNMP_PDU_BULK:                     /* Get-bulk request. */

        /* Processing of bulk operation. */
        status = SNMP_Get_Bulk(obj, Get_usmUserEntry);

        break;

    case SNMP_PDU_SET:                      /* Set request. */

        USM_MIB_Commit_Left++;

        /* Process the SET operation. */
        status = Set_usmUserEntry(obj);

        if (status != SNMP_NOSUCHINSTANCE)
            break;

    case SNMP_PDU_CREATE:

        /* Processing of create operation. */
        status = Create_usmUserEntry(obj);

        /* If the entry was successfully created, set the value. */
        if(status == SNMP_NOERROR)
            status = Set_usmUserEntry(obj);

        break;

    case SNMP_PDU_UNDO:                     /* Undo request. */

        USM_MIB_Commit_Left = 0;

        /* Processing of UNDO operations. */
        status = Undo_usmUserEntry(obj);

        break;

    case SNMP_PDU_COMMIT:                   /* Commit request. */

        USM_MIB_Commit_Left--;

        /* Clear out the the engine ID. */
        UTL_Zero(engine_id, sizeof(engine_id));

        /* Clear out the user name. */
        UTL_Zero(user_name, sizeof(user_name));

        engine_id_len = obj->Id[SNMP_USM_MIB_SUB_LEN];

        if (engine_id_len > SNMP_SIZE_SMALLOBJECTID)
            status = SNMP_GENERROR;
        else
        {
            for (i = 0; i < engine_id_len; i++)
            {
                engine_id[i] =
                        (UINT8)(obj->Id[i + 1 + SNMP_USM_MIB_SUB_LEN]);
            }

            user_name_len = obj->Id[engine_id_len + 1 +
                                    SNMP_USM_MIB_SUB_LEN];

            if (user_name_len > SNMP_SIZE_SMALLOBJECTID)
                status = SNMP_GENERROR;
            else
            {
                temp = engine_id_len + 2 + SNMP_USM_MIB_SUB_LEN;

                for (i = 0; i < user_name_len; i++)
                {
                    user_name[i] = (UINT8)(obj->Id[temp + i]);
                }

                if (obj->IdLen != (2 + user_name_len + engine_id_len +
                                     SNMP_USM_MIB_SUB_LEN))
                {
                    status = SNMP_NOSUCHINSTANCE;
                }
            }
        }

        if (status == SNMP_NOERROR)
        {
            usm_user_ptr = Get_USM_User_Entry_Util(engine_id,
                                       engine_id_len, user_name,
                                       user_name_len, &Temp_USM_Mib_Root);

            temp_usm_user_ptr = usm_user_ptr;

            if (!usm_user_ptr)
            {
                usm_user_ptr = Get_USM_User_Entry_Util(engine_id,
                                  engine_id_len, user_name,
                                  user_name_len, &Usm_Mib.usm_user_table);

            }
        }

        /* If we got the handle either from permanent or temporary list.
         */
        if (usm_user_ptr)
        {
            /* Check whether it was a row set operation. */
            if (obj->Id[SNMP_USM_MIB_ATTR_OFFSET] == 13)
            {
                if (temp_usm_user_ptr)
                    current_state = 0;
                else if (usm_user_ptr->usm_status != SNMP_ROW_ACTIVE)
                    current_state = 2;
                else
                    current_state = 3;

                if (status_trans[obj->Syntax.LngUns - 1][current_state]
                                                          != SNMP_NOERROR)
                {
                    status =
                      status_trans[obj->Syntax.LngUns - 1][current_state];
                }
                else
                {
                    status = Commit_usmUserEntryStatus(
                        usm_user_ptr, (UINT8)(obj->Syntax.LngUns),
                        (UINT8)(temp_usm_user_ptr != NU_NULL) );
                }
            }

            /* If it was not row set operation. */
            else
            {
                status = Commit_usmUserEntryStatus(usm_user_ptr, 0,
                                                   NU_FALSE);
            }

            /* usmUserPrivKeyChange */
            if (obj->Id[SNMP_USM_MIB_ATTR_OFFSET] == 9)
            {
                /* Has the key been initialized? */
                if(strlen((CHAR *)usm_user_ptr->usm_priv_key) == 0)
                {
                    status = SNMP_INCONSISTANTNAME;
                }
                else
                {
                    /* Get the Authentication Protocol. */
                    auth_prot = USM_Lookup_Auth_Prot(usm_user_ptr->
                                                        usm_auth_index);

                    if(auth_prot)
                    {
                        /*applying key change algorithm*/
                        if(USM_Key_Change(usm_user_ptr->usm_auth_index,
                                          usm_user_ptr->usm_priv_key,
                                          obj->Syntax.BufChr,
                                          obj->SyntaxLen) != NU_SUCCESS)
                        {
                            status = SNMP_WRONGVALUE;
                        }
                        else
                        {
                            /* Updating the own_priv_key in the user
                             * table. Please note that the correct length
                             * of privacy key will be present in obj.
                             * For e.g. for DES it must be 16. The Key
                             * change function adjusts key lengths itself
                             * if authentication protocol's standard
                             * key size differs from that of encryption
                             * protocol.
                             */
                            NU_BLOCK_COPY(usm_user_ptr->usm_own_priv_key,
                                   usm_user_ptr->usm_priv_key,USM_DES_PRIV_KEY_SIZE);

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)
                            /* Calling the module to update the structure
                             * in file.
                             */
                            if (USM_Save_User(usm_user_ptr) != NU_SUCCESS)
                            {
                                NLOG_Error_Log("SNMP: USM_Save_User "
                                    "failed", NERR_SEVERE, __FILE__,
                                    __LINE__);
                            }
#endif

                            strcpy((CHAR*)obj->Syntax.BufChr, (CHAR*)"");
                            obj->SyntaxLen = 0;
                        }
                    }
                }
            }

            /* usmUserAuthKeyChange */
            else if (obj->Id[SNMP_USM_MIB_ATTR_OFFSET] == 6)
            {
                /* Has the key been initialized? */
                if(strlen((CHAR *)usm_user_ptr->usm_auth_key) == 0)
                {
                    status = SNMP_INCONSISTANTNAME;
                }
                else
                {
                    /* Get the Authentication Protocol. */
                    auth_prot =
                       USM_Lookup_Auth_Prot(usm_user_ptr->usm_auth_index);

                    if(auth_prot)
                    {
                        /*applying key change algorithm*/
                        if(USM_Key_Change(usm_user_ptr->usm_auth_index,
                                          usm_user_ptr->usm_auth_key,
                                          obj->Syntax.BufChr,
                                          obj->SyntaxLen) != NU_SUCCESS)
                        {
                            status = SNMP_WRONGVALUE;
                        }
                        else
                        {
                            /* Updating the own_auth_key in the user
                             * table.
                             */
                            NU_BLOCK_COPY(usm_user_ptr->usm_own_auth_key,
                                   usm_user_ptr->usm_auth_key,
                                   auth_prot->key_length);

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)
                            /* Calling the module to update the structure
                             * in file.
                             */
                            if (USM_Save_User(usm_user_ptr) != NU_SUCCESS)
                            {
                                NLOG_Error_Log("SNMP: USM_Save_User "
                                        "failed", NERR_SEVERE,
                                        __FILE__, __LINE__);
                            }
#endif

                            strcpy((CHAR*)obj->Syntax.BufChr,(CHAR*)"");
                            obj->SyntaxLen = 0;
                        }
                    }
                }
            }

            /* If the row status is set to 'DESTROY'. */
            if (usm_user_ptr->usm_status == SNMP_ROW_DESTROY)
            {
                /* If handle was from temporary list then remove it from
                 * temporary list.
                 */
                if (temp_usm_user_ptr)
                    DLL_Remove(&Temp_USM_Mib_Root, usm_user_ptr);

                /* If handle was from permanent list then remove it from
                 * permanent list.
                 */
                else
                    DLL_Remove(&Usm_Mib.usm_user_table, usm_user_ptr);

                /* Deallocate memory attained by this USM user structure.
                 */
                if (NU_Deallocate_Memory(usm_user_ptr) != NU_SUCCESS)
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
        if ((status == SNMP_NOERROR) && (USM_MIB_Commit_Left == 0))
        {
            Commit_usmUserEntries();
        }

        break;

    default:                                /* Invalid request. */
        status = SNMP_GENERROR;
    }

    /* Return success or error code. */
    return (status);

} /* usmUserEntry */

#endif /* (INCLUDE_MIB_USM == NU_TRUE) */


