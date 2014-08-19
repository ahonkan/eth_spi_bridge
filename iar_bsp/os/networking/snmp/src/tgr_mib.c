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
*       tgr_mib.c                                                
*
*   COMPONENT
*
*       Target - MIB
*
*   DESCRIPTION
*
*       This file implements the functions used by Target MIBs.
*
*   DATA STRUCTURES
*
*       Snmp_Target_Mib
*       Temp_Target_Addr_Mib_Root
*       Temp_Target_Param_Mib_Root
*       Target_Addr_MIB_Commit_Left
*       Target_Params_MIB_Commit_Left
*
*   FUNCTIONS
*
*       Set_snmpTargetAddrEntry
*       Set_snmpTargetParamsEntry
*       Create_snmpTargetAddrEntry
*       Create_snmpTargetParamsEntry
*       Undo_snmpTargetAddrEntry
*       Undo_snmpTargetParamsEntry
*       Commit_snmpTargetAddrEntryStatus
*       Commit_snmpTargetParamsEntryStatus
*       TargetMIB_Compare_name
*       TargetMIB_Compare_params
*       Save_Target_Addr
*       Save_Target_Params
*       Commit_snmpTargetAddrEntries
*       Commit_snmpTargetParamsEntries
*       snmpTargetSpinLock
*       Get_snmpTargetAddrEntry
*       snmpTargetAddrEntry
*       Get_snmpTargetParamsEntry
*       snmpTargetParamsEntry
*       snmpUnavailableContexts
*       snmpUnknownContexts
*
*   DEPENDENCIES
*
*       target.h
*       nu_net.h
*       tgr_api.h
*       mib.h
*       snmp_no.h
*       snmp_g.h
*
*************************************************************************/
#include "networking/target.h"
#include "networking/nu_net.h"
#include "networking/tgr_api.h"
#include "networking/mib.h"
#include "networking/snmp_no.h"
#include "networking/snmp_g.h"

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)
#include "networking/snmp_file.h"
#endif

#if ((INCLUDE_SNMPv1 == NU_TRUE) || (INCLUDE_SNMPv2 == NU_TRUE))
SNMP_TARGET_MIB             Snmp_Target_Mib;
SNMP_TARGET_ADDRESS_TABLE   Temp_Target_Addr_Mib_Root;
SNMP_TARGET_PARAMS_TABLE    Temp_Target_Param_Mib_Root;
STATIC UINT32               Target_Addr_MIB_Commit_Left;
STATIC UINT32               Target_Params_MIB_Commit_Left;


#if (INCLUDE_MIB_TARGET == NU_TRUE)

STATIC SNMP_TARGET_ADDRESS_TABLE * Get_Target_Addr_Entry_Util(
    const UINT8 *target_name, SNMP_TARGET_ADDRESS_TABLE *root);
STATIC SNMP_TARGET_PARAMS_TABLE * Get_Target_Params_Entry_Util(
    const UINT8 *target_params, SNMP_TARGET_PARAMS_TABLE *root);
STATIC SNMP_TARGET_ADDRESS_TABLE *Get_Target_Addr_Entry(
    const UINT8 *target_name, UINT8 getflag);
STATIC SNMP_TARGET_PARAMS_TABLE *Get_Target_Param_Entry(
    const UINT8 *target_param, UINT8 getflag);
STATIC UINT16 Create_snmpTargetAddrEntry(const snmp_object_t *obj);
STATIC UINT16 Create_snmpTargetParamsEntry(const snmp_object_t *obj);
STATIC UINT16 Undo_snmpTargetAddrEntry(const snmp_object_t *obj);
STATIC UINT16 Undo_snmpTargetParamsEntry(const snmp_object_t *obj);
STATIC UINT16 Commit_snmpTargetAddrEntryStatus(
    SNMP_TARGET_ADDRESS_TABLE *table_ptr, UINT8 row_status, UINT8 is_new);
STATIC UINT16 Commit_snmpTargetParamsEntryStatus(
    SNMP_TARGET_PARAMS_TABLE *table_ptr, UINT8 row_status, UINT8 is_new);
STATIC VOID Commit_snmpTargetAddrEntries(VOID);
STATIC VOID Commit_snmpTargetParamsEntries(VOID);
STATIC UINT16 Get_snmpTargetAddrEntry(snmp_object_t *obj, UINT8 getflag);
STATIC UINT16 Get_snmpTargetParamsEntry(snmp_object_t *obj, UINT8 getflag);

/*************************************************************************
*
*   FUNCTION
*
*       Get_Target_Addr_Entry_Util
*
*   DESCRIPTION
*
*       This function is used to get the handle to the Target address table
*       structure by specifying the target name and list of Table structures.
*
*   INPUTS
*
*       *target_name            A pointer to the target name string.
*       *root                   The pointer to the root of list containing
*                               Table structures.
*
*   OUTPUTS
*
*       NU_NULL                 When fail to get handle to Table structure
*                               with target name passed in.
*   SNMP_TARGET_ADDRESS_TABLE * When handle to Target structure found with
*                               target name passed in.
*
*************************************************************************/
STATIC SNMP_TARGET_ADDRESS_TABLE * Get_Target_Addr_Entry_Util(
    const UINT8 *target_name, SNMP_TARGET_ADDRESS_TABLE *root)
{
    /* Handle to the Target Address Table structure. */
    SNMP_TARGET_ADDRESS_TABLE *target_ptr = root;

    INT                         cmp = 0;

    /* Loop to find the Target address table with target name passed in. */
    while (target_ptr)
    {
        /* Check whether the target names of the current entry
        * matches, the target name passed or if the current
        * entry has a bigger target name. If it does, break
        * out of the loop.
        */
        if((cmp = strcmp(target_ptr->snmp_target_addr_name,
            (CHAR *)target_name)) >= 0)
        {
            break;
        }

        /* Moving forward in the list. */
        target_ptr = target_ptr->next;
    }

    /* If the name comparison had indicated that the current entry has
    * a greater target name we need to assign null to the returned
    * value. This is because we broke out of the loop when we had
    * reached a target which had a greater target name than what we
    * were looking. Since we have a list in ascending order, there is
    * no hope of finding an entry which matches.
    */
    if(cmp > 0) /* We didn't find the entry so assign null. */
    {
        target_ptr = NU_NULL;
    }

    /* Return handle to the Target address table. It will be a valid entry
    * if found, otherwise it will contain NU_NULL.
    */
    return (target_ptr);

} /* Get_Target_Addr_Entry_Util */

/*************************************************************************
*
*   FUNCTION
*
*       Get_Target_Param_Entry_Util
*
*   DESCRIPTION
*
*       This function is used to get the handle to the Target params table
*       structure by specifying the target name and list of Table structures.
*
*   INPUTS
*
*       *target_name            A pointer to the target params name string.
*       *root                   The pointer to the root of list containing
*                               Table structures.
*
*   OUTPUTS
*
*       NU_NULL                 When fail to get handle to Table structure
*                               with target name passed in.
*   SNMP_TARGET_PARAMS_TABLE *  When handle to Target structure found with
*                               target name passed in.
*
*************************************************************************/
STATIC SNMP_TARGET_PARAMS_TABLE * Get_Target_Params_Entry_Util(
    const UINT8 *target_params, SNMP_TARGET_PARAMS_TABLE *root)
{
    /* Handle to the Target params table structure. */
    SNMP_TARGET_PARAMS_TABLE *target_ptr = root;

    /* Loop to find the Target params table with target name passed in. */
    while (target_ptr)
    {
        /* Check whether the params names of the current entry
        * matches, the params name passed.
        */
        if (strcmp(target_ptr->snmp_target_params_name,(CHAR *)target_params) == 0)
        {
            break;
        }

        /* Moving forward in the list. */
        target_ptr = target_ptr->next;
    }

    /* Return handle to the Target params table entry. It will contain
    * a valid value if found or NU_NULL otherwise.
    */
    return (target_ptr);

} /* Get_Target_Params_Entry_Util */



/***********************************************************************
*
*   Function
*
*       Get_Target_Addr_Entry
*
*   DESCRIPTION
*
*       Find an entry in the target address table which matches the passed
*       target name. It attempts to find the entry in the permanent list
*       and then tries the temporary list if failed.
*
*   INPUTS
*
*       *target_name            Name of the target.
*       getflag                 Flag to distinguish GET and GET-NEXT
*                               request.
*
*   OUTPUTS
*
*       Pointer to the target address entry if one is found, NU_NULL
*       otherwise.
*
*************************************************************************/
STATIC SNMP_TARGET_ADDRESS_TABLE *Get_Target_Addr_Entry(
    const UINT8 *target_name, UINT8 getflag)
{
    /* Handle to the target address entry. */
    SNMP_TARGET_ADDRESS_TABLE   *target;

    /* If we are handling GET request. */
    if (getflag)
    {
        /* Try to get the Target address handle from permanent list. */
        target = Get_Target_Addr_Entry_Util(target_name,
            Snmp_Target_Mib.target_addr_table.next);

        /* If it was not found in the permanent list. */
        if (!target)
        {
            /* Try to get Target address handle from temporary list. */
            target = Get_Target_Addr_Entry_Util(target_name,
                &Temp_Target_Addr_Mib_Root);
        }
    }

    /* If we are handling GET-NEXT request. */
    else
    {
        /* Start traversing the permanent list. */
        target =  Snmp_Target_Mib.target_addr_table.next;

        /* Loop till we find the next target address table structure based
        * on target name passed in.
        */
        while (target)
        {
            /* If we reached at an entry that is lexicographically greater
            * than the target name passed then break out of the loop.
            */
            if (strcmp(target->snmp_target_addr_name,
                (CHAR *)target_name) > 0)
            {
                break;
            }

            /* Moving forward in the list. */
            target = target->next;
        }
    }

    /* Return handle to the target address table entry. It will either
    * contain a valid value or NU_NULL.
    */
    return (target);

} /* Get_Target_Addr_Entry */

/***********************************************************************
*
*   Function
*
*       Get_Target_Param_Entry
*
*   DESCRIPTION
*
*       Find an entry in the target params table which matches the passed
*       target name.
*
*   INPUTS
*
*       *target_param           Name of the target.
*       getflag                 Flag to distinguish GET and GET-NEXT
*                               request.
*
*   OUTPUTS
*
*       Pointer to the target address entry if one is found, NU_NULL
*       otherwise.
*
*************************************************************************/
STATIC SNMP_TARGET_PARAMS_TABLE *Get_Target_Param_Entry(
    const UINT8 *target_param, UINT8 getflag)
{
    /* Handle to the target params table entry. */
    SNMP_TARGET_PARAMS_TABLE        *target;

    /* If we are handling GET request. */
    if (getflag)
    {
        /* Try to get the Target params handle from the permanent list. */
        target = Get_Target_Params_Entry_Util(target_param, Snmp_Target_Mib.
            target_params_table.next);

        /* If it was not found in the permanent list. */
        if (!target)
        {
            /* Try to get Target params handle from temporary list. */
            target = Get_Target_Params_Entry_Util(target_param,
                &Temp_Target_Param_Mib_Root);
        }
    }

    /* If we are handling GET-NEXT request. */
    else
    {
        /* Start traversing the permanent list. */
        target =  Snmp_Target_Mib.target_params_table.next;

        /* Loop till we find the next target params table structure based
        * on target params name passed in.
        */
        while (target)
        {
            /* If we reached at an entry that is lexicographically greater
            * than the target name passed then break out of the loop.
            */
            if (strcmp(target->snmp_target_params_name,
                (CHAR *)target_param) > 0)
            {
                break;
            }

            /* Moving forward in the list. */
            target = target->next;
        }
    }

    /* Return handle to the target params entry. It will be a valid value
    * if found or NU_NULL otherwise.
    */
    return (target);

} /* Get_Target_Param_Entry */

/*************************************************************************
*
*   FUNCTION
*
*       Set_snmpTargetAddrEntry
*
*   DESCRIPTION
*
*       This function is used to handle SNMP SET request on
*       snmpTargetAddrTable.
*
*   INPUTS
*
*       *obj                    The SNMP object containing request.
*
*   OUTPUTS
*
*       SNMP_NOSUCHINSTANCE     When there does not exist a TargetAddr
*                               structure specified by OID.
*       SNMP_NOSUCHNAME         When required attribute does not exists
*                               or inaccessible.
*       SNMP_WRONGLENGTH        When syntax length is not valid.
*       SNMP_WRONGVALUE         When value is not valid to set.
*       SNMP_NOERROR            When successful.
*
*************************************************************************/
STATIC UINT16 Set_snmpTargetAddrEntry(const snmp_object_t *obj)
{
    /* Handle to the target address entry. */
    SNMP_TARGET_ADDRESS_TABLE   *target_entry = NU_NULL;

    /* Variable to hold target address name. */
    UINT8                       target_name[MAX_TARG_NAME_SZE];

    /* Variable to use in for-loop. */
    UINT32                      i;

    /* Status to return success or error code. */
    UINT16                      status = SNMP_NOERROR;

    /* Clear out target name. */
    UTL_Zero(target_name, sizeof(target_name));

    /* Get the value of target address name. */
    for (i = 0;
        (i < (MAX_TARG_NAME_SZE - 1)) &&
        (SNMP_TARGET_ADDR_SUB_LEN + i < obj->IdLen);
    i++)
    {
        target_name[i] = (UINT8)(obj->Id[SNMP_TARGET_ADDR_SUB_LEN + i]);
    }

    /* If we don't have valid object identifier then return error
     * code.
     */
    if ((obj->IdLen != (SNMP_TARGET_ADDR_SUB_LEN + i)) ||
        (strlen((CHAR *)target_name) != i))
    {
        /* Return error code. */
        status = SNMP_NOSUCHINSTANCE;
    }

    /* If no error till now then proceed. */
    if (status == SNMP_NOERROR)
    {
        /* Get the handle to the target address table entry. This is a set
         * request so it should refer to current entry, thus second
         * parameter should be NU_TRUE.
         */
        target_entry = Get_Target_Addr_Entry(target_name, NU_TRUE);
    }

    /* If we got the handle to the target address entry then proceed. */
    if (target_entry)
    {
        switch (obj->Id[SNMP_TARGET_ADDR_ATTR_OFFSET])
        {
        case 2:                         /* snmpTargetAddrTDomain */

            /* If length is not valid for 'snmpTargetAddrTDomain' then
             * return error code.
             */
            if ((obj->SyntaxLen >= SNMP_SIZE_SMALLOBJECTID) ||
                (obj->SyntaxLen < 7))
            {
                status = SNMP_WRONGLENGTH;
            }

            /* If value to set is valid. */
            else
            {
                /* Set the value to 'snmpTargetAddrTDomain'. */
                target_entry->snmp_target_addr_tDomain =
                    (UINT32)obj->Syntax.BufInt[6];
            }

            break;

        case 3:                         /* snmpTargetAddrTAddress */

            /* The format of this field is dependent upon value of
             * snmpTargetAddrTDomain. If it is the default domain-SNMP_UDP
             * i.e. UDP over IPv4 / IPv6 (RFC 1906 section-2), then this
             * field would contain IP address followed by optional port
             * number to which traps should be sent if specified.
             */
            if ((obj->SyntaxLen == IP_ADDR_LEN + SNMP_PORT_NUM_LEN) ||
                (obj->SyntaxLen == IP_ADDR_LEN) )
            {
                /* Set the value of 'snmpTargetAddrTAddress'. */
                NU_BLOCK_COPY(target_entry->snmp_target_addr_tAddress,
                    obj->Syntax.BufChr,IP_ADDR_LEN);

                /* Update the IP Address Family. */
                target_entry->snmp_target_addr_tfamily = NU_FAMILY_IP;

                if(obj->SyntaxLen == IP_ADDR_LEN + SNMP_PORT_NUM_LEN)
                {
                    /* Update the port number if specified. */
                    target_entry->snmp_target_addr_portnumber =
                        GET16(obj->Syntax.BufChr, IP_ADDR_LEN);
                }

                else
                {
                    /* Mark port number as zero. This means port was
                     * not specified and notification module will use
                     * the default trap port.
                     */
                    target_entry->snmp_target_addr_portnumber = 0;
                }
            }

#if (INCLUDE_IPV6 == NU_TRUE)
            /* If address is IPv6 with / without port number. */
            else if ((obj->SyntaxLen == IP6_ADDR_LEN + SNMP_PORT_NUM_LEN)
                || (obj->SyntaxLen == IP6_ADDR_LEN) )
            {
                /* Set the value of snmpTargetAddrTAddress. */
                NU_BLOCK_COPY(target_entry->snmp_target_addr_tAddress,
                    obj->Syntax.BufChr, IP6_ADDR_LEN);

                /* Update the IP Address Family. */
                target_entry->snmp_target_addr_tfamily = NU_FAMILY_IP6;

                if (obj->SyntaxLen == IP6_ADDR_LEN + SNMP_PORT_NUM_LEN)
                {
                    /* Update the port number if mentioned. */
                    target_entry->snmp_target_addr_portnumber =
                        GET16(obj->Syntax.BufChr, IP6_ADDR_LEN);
                }

                else
                {
                    /* Mark port number as zero. This means port was
                     * not specified and notification module will use
                     * the default trap port.
                     */
                    target_entry->snmp_target_addr_portnumber = 0;
                }

            }
#endif /* (INCLUDE_IPV6 == NU_TRUE) */

            else /* The address field has invalid length. */
            {
                status = SNMP_WRONGLENGTH;
            }

            break;

        case 4:                         /* snmpTargetAddrTimeout */

            /* Set the value of 'snmpTargetAddrTimeout'. */
            target_entry->snmp_target_addr_time_out = obj->Syntax.LngUns;

            break;

        case 5:                         /* snmpTargetAddrRetryCount */

            /* Set the value of 'snmpTargetAddrRetryCount'. */
            target_entry->snmp_target_addr_retry_count = obj->Syntax.LngUns;

            break;

        case 6:                         /* snmpTargetAddrTagList */

            /* If length is not valid for 'snmpTargetAddrTagList' then
             * return error code.
             */
            if (obj->SyntaxLen >= SNMP_SIZE_SMALLOBJECTID)
            {
                status = SNMP_WRONGLENGTH;
            }

            /* If the value of 'snmpTargetAddrTagList' is not valid then
             * return error code.
             */
            else if (strlen((CHAR *)obj->Syntax.BufChr) != obj->SyntaxLen)
            {
                status = SNMP_WRONGVALUE;
            }

            /* If value to set is valid or empty string. */
            else
            {
                /* Set the value of 'snmpTargetAddrTagList'. */
                NU_BLOCK_COPY(target_entry->snmp_target_addr_tag_list,
                    obj->Syntax.BufChr, obj->SyntaxLen);

                target_entry->snmp_target_addr_tag_list[obj->SyntaxLen]
                = NU_NULL;

                /* Update length of 'snmpTargetAddrTagList'. */
                target_entry->tag_list_len = obj->SyntaxLen;
            }

            break;

        case 7:                         /* snmpTargetAddrParams */

            /* If length is not valid for 'snmpTargetAddrParams' then
             * return error code.
             */
            if (obj->SyntaxLen >= SNMP_SIZE_SMALLOBJECTID)
            {
                status = SNMP_WRONGLENGTH;
            }

            /* If the value of 'snmpTargetAddrParams' is not valid
             * then return error code.
             */
            else if (strlen((CHAR *)obj->Syntax.BufChr) != obj->SyntaxLen)
            {
                status = SNMP_WRONGVALUE;
            }

            /* If value to set is valid. */
            else
            {
                NU_BLOCK_COPY(target_entry->snmp_target_addr_params,
                    obj->Syntax.BufChr, obj->SyntaxLen);

                target_entry->snmp_target_addr_params[obj->SyntaxLen] =
                    NU_NULL;

                /* Update length of 'snmpTargetAddrParams'. */
                target_entry->params_len = obj->SyntaxLen;
            }

            break;

        case 8:                         /* snmpTargetAddrStorageType */

            /* If the value of 'snmpTargetAddrStorageType' is not in valid
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
                target_entry->snmp_target_addr_storage_type =
                    (UINT8)(obj->Syntax.LngUns);
            }

            break;

        case 9:                         /* snmpTargetAddrRowStatus */

            /* If the value is not in valid range then return error code.
             */
            if ((obj->Syntax.LngUns < 1) || (obj->Syntax.LngUns > 6))
            {
                status = SNMP_WRONGVALUE;
            }

            else if ((target_entry->snmp_target_addr_row_flag == 0) ||
                (target_entry->snmp_target_addr_row_flag ==
                (UINT8)(obj->Syntax.LngUns)))
            {
                target_entry->snmp_target_addr_row_flag =
                    (UINT8)(obj->Syntax.LngUns);
            }

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

    /* If we did not get the handle to the target address entry then
     * return error code.
     */
    else
    {
        /* Return error code. */
        status = SNMP_NOSUCHINSTANCE;
    }

    /* Return success or error code. */
    return (status);

} /* Set_snmpTargetAddrEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Set_snmpTargetParamsEntry
*
*   DESCRIPTION
*
*       This function is used to handle SNMP SET request on
*       snmpTargetParamsTable.
*
*   INPUTS
*
*       *obj                    The SNMP object containing request.
*
*   OUTPUTS
*
*       SNMP_NOSUCHINSTANCE     When there does not exist a TargetParams
*                               structure specified by OID.
*       SNMP_NOSUCHNAME         When required attribute does not exists
*                               or inaccessible.
*       SNMP_WRONGLENGTH        When syntax length is not valid.
*       SNMP_WRONGVALUE         When value is not valid to set.
*       SNMP_NOERROR            When successful.
*
*************************************************************************/
STATIC UINT16 Set_snmpTargetParamsEntry(const snmp_object_t *obj)
{
    /* Handle to the target params entry. */
    SNMP_TARGET_PARAMS_TABLE   *target_entry = NU_NULL;

    /* Variable to hold target params name. */
    UINT8                       target_param[MAX_TARG_NAME_SZE];

    /* Variable to use in for-loop. */
    UINT32                      i;

    /* Status to return success or error code. */
    UINT16                      status = SNMP_NOERROR;

    /* Clear out target param name. */
    UTL_Zero(target_param, sizeof(target_param));

    /* Get the value of target params name. */
    for (i = 0;
        ((i < MAX_TARG_NAME_SZE) &&
        ((SNMP_TARGET_PARAM_SUB_LEN + i) < obj->IdLen));
    i++)
    {
        target_param[i] = (UINT8)(obj->Id[SNMP_TARGET_PARAM_SUB_LEN + i]);
    }

    /* If we don't have valid object identifier then return error
     * code.
     */
    if ((obj->IdLen != (SNMP_TARGET_PARAM_SUB_LEN + i)) ||
        (strlen((CHAR *)target_param) != i))
    {
        /* Return error code. */
        status = SNMP_NOSUCHINSTANCE;
    }

    /* If no error till now then proceed. */
    if (status == SNMP_NOERROR)
    {
        /* Get the handle to the target params table entry. This is a set
         * request so it should refer to current entry, thus second
         * parameter should be NU_TRUE.
         */
        target_entry = Get_Target_Param_Entry(target_param, NU_TRUE);
    }

    /* If we got the handle to the target params entry then proceed. */
    if (target_entry)
    {
        switch (obj->Id[SNMP_TARGET_PARAM_ATTR_OFFSET])
        {
        case 2:                         /* snmpTargetParamsMPModel */

            /* If the value of 'snmpTargetParamsMPModel'is not in valid
             * range then return error code. Valid values are any of the
             * three versions e.g. SNMP_VERSION_V1.
             */
            if ((obj->Syntax.LngUns > 3) || ((INT32)obj->Syntax.LngUns < 0)
                || obj->Syntax.LngUns == 2)
            {
                status = SNMP_WRONGVALUE;
            }

            /* If value to set is valid. */
            else
            {
                /* Set the value to 'snmpTargetParamsMPModel'. */
                target_entry->snmp_target_params_mp_model =
                    obj->Syntax.LngUns;
            }

            break;

        case 3:                         /* snmpTargetParamsSecurityModel */

            /* If the value of 'snmpTargetParamsSecurityModel'is not in
             * valid range then return error code.
             */
            if ((obj->Syntax.LngUns > 3) || (obj->Syntax.LngUns < 1))
            {
                status = SNMP_WRONGVALUE;
            }

            else
            {
                /* Set the value of 'snmpTargetParamsSecurityModel'. */
                target_entry->snmp_target_params_security_model =
                    obj->Syntax.LngUns;
            }

            break;

        case 4:                         /* snmpTargetParamsSecurityName */

            /* If length is not valid for 'snmpTargetParamsSecurityName'
             * then return error code.
             */
            if ((obj->SyntaxLen >= SNMP_SIZE_SMALLOBJECTID) ||
                (obj->SyntaxLen == 0))
            {
                status = SNMP_WRONGLENGTH;
            }

            /* If the value of 'snmpTargetParamsSecurityName' is not valid
             * then return error code.
             */
            else if (strlen((CHAR *)obj->Syntax.BufChr) != obj->SyntaxLen)
            {
                status = SNMP_WRONGVALUE;
            }

            /* If value to set is valid. */
            else
            {
                /* Set the value of 'snmpTargetParamsSecurityName'. */
                NU_BLOCK_COPY(target_entry->snmp_target_params_security_name,
                    obj->Syntax.BufChr, obj->SyntaxLen);

                target_entry->snmp_target_params_security_name[obj->
                    SyntaxLen] = NU_NULL;
            }

            break;

        case 5:                         /* snmpTargetParamsSecurityLevel */

            /* If the value of 'snmpTargetParamsSecurityLevel'is not in
             * valid range then return error code.
             */
            if ((obj->Syntax.LngUns > 3) || (obj->Syntax.LngUns < 1))
            {
                status = SNMP_WRONGVALUE;
            }

            else
            {
                /* Set the value of 'snmpTargetParamsSecurityLevel'. */
                target_entry->snmp_target_params_security_level =
                    obj->Syntax.LngUns;
            }

            break;

        case 6:                         /* snmpTargetParamsStorageType */

            /* If the value of 'snmpTargetParamsStorageType' is not in valid
             * range then return error code.
             */
            if ((obj->Syntax.LngUns < 1) || (obj->Syntax.LngUns > 5))
            {
                status = SNMP_WRONGVALUE;
            }

            /* If value to set is valid. */
            else
            {
                /* Set the value of 'snmpTargetParamsStorageType'. */
                target_entry->snmp_target_params_storage_type =
                    (UINT8)(obj->Syntax.LngUns);
            }

            break;

        case 7:                         /* snmpTargetParamsRowStatus */

            /* If the value is not in valid range then return error code. */
            if ((obj->Syntax.LngUns < 1) || (obj->Syntax.LngUns > 6))
            {
                status = SNMP_WRONGVALUE;
            }

            else if ((target_entry->snmp_target_params_row_flag == 0) ||
                (target_entry->snmp_target_params_row_flag ==
                (UINT8)(obj->Syntax.LngUns)))
            {
                target_entry->snmp_target_params_row_flag =
                    (UINT8)(obj->Syntax.LngUns);
            }

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

    /* If we did not get the handle to the target params entry then
     * return error code.
     */
    else
    {
        /* Return error code. */
        status = SNMP_NOSUCHINSTANCE;
    }

    /* Return success or error code. */
    return (status);

} /* Set_snmpTargetParamsEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Create_snmpTargetAddrEntry
*
*   DESCRIPTION
*
*       This function is used to handle SNMP create request on Target
*       address table entry.
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
STATIC UINT16 Create_snmpTargetAddrEntry(const snmp_object_t *obj)
{
    /* Handle to snmpTargetAddrEntry. */
    SNMP_TARGET_ADDRESS_TABLE *target_ptr = NU_NULL;

    /* Handle to find the proper place in the list. */
    SNMP_TARGET_ADDRESS_TABLE *locator;

    /* Variable to hold target address name. */
    UINT8                       target_name[MAX_TARG_NAME_SZE];

    /* Status to return success or error code. */
    UINT16      status = SNMP_NOERROR;

    /* Variable to hold the comparison result. */
    INT         cmp_result;

    /* Variable to use in for-loop. */
    UINT32      i;

    /* Clear out the value of target name. */
    UTL_Zero(target_name, sizeof(target_name));

    /* Get the value of target address name. */
    for (i = 0;
        (i < (MAX_TARG_NAME_SZE - 1)) &&
        (SNMP_TARGET_ADDR_SUB_LEN + i < obj->IdLen);
    i++)
    {
        target_name[i] = (UINT8)(obj->Id[SNMP_TARGET_ADDR_SUB_LEN + i]);
    }

    /* If we don't have valid object identifier then return error code. */
    if ((obj->IdLen != (SNMP_TARGET_ADDR_SUB_LEN + i)) ||
        (strlen((CHAR *)target_name) != i))
    {
        /* Return error code. */
        status = SNMP_NOSUCHINSTANCE;
    }

    /* If no error till now then proceed. */
    if (status == SNMP_NOERROR)
    {
        /* Get the handle to the target address table entry. This is a
         * create request so it should refer to current entry, thus second
         * parameter should be NU_TRUE.
         */
        target_ptr = Get_Target_Addr_Entry(target_name, NU_TRUE);
    }

    /* If we don't currently have the Target address table structure. */
    if (!target_ptr)
    {
        /* Allocate memory for the new table structure. */
        if (NU_Allocate_Memory(&System_Memory,(VOID **)&target_ptr,
                sizeof(SNMP_TARGET_ADDRESS_TABLE), NU_NO_SUSPEND) ==
                NU_SUCCESS)
        {
            target_ptr = TLS_Normalize_Ptr(target_ptr);

            /* Clear out the newly allocated table structure. */
            UTL_Zero(target_ptr, sizeof(SNMP_TARGET_ADDRESS_TABLE));

            /* Set the target address name. */
            NU_BLOCK_COPY(target_ptr->snmp_target_addr_name, target_name,
                strlen((CHAR*)target_name));

            /* Set the default domain. */
            target_ptr->snmp_target_addr_tDomain = SNMP_UDP;

            /* Set the default target address family which is IPv6, if it
             * is enabled in net and IPv4 otherwise. This is required
             * because the manager won't be able to interpret the length of
             * address and show it as 'not accessible' when the row is not
             * ready and value of this field was not entered upon row
             * creation (create and wait). However, this would be changed
             * automatically upon change of target (ipv4 or ipv6) address.
             */
#if(INCLUDE_IPV6 == NU_TRUE)
            target_ptr->snmp_target_addr_tfamily = NU_FAMILY_IP6;
#else
            target_ptr->snmp_target_addr_tfamily = NU_FAMILY_IP;
#endif

            /* Set the storage type to non-volatile. */
            target_ptr->snmp_target_addr_storage_type = SNMP_STORAGE_DEFAULT;

            /* Set the status to 'createAndWait'. */
            target_ptr->snmp_target_addr_row_status = SNMP_ROW_CREATEANDWAIT;

            /* Get the starting handle to the list to find the proper
             * position of new Target address table structure.
             */
            locator = Temp_Target_Addr_Mib_Root.next;

            /* Loop to find the proper position. */
            while (locator)
            {
                /* Compare the target names */
                if((cmp_result = strcmp(locator->snmp_target_addr_name,
                    target_ptr->snmp_target_addr_name)) >= 0)
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
                locator = locator->next;
            }

            /* Add target address table structure to proper position provided
             * it is a new entry.
             */
            if (status == SNMP_NOERROR)
            {
                if (locator)
                {
                    DLL_Insert(&Temp_Target_Addr_Mib_Root, target_ptr,
                        locator);
                }

                else
                {
                    DLL_Enqueue(&Temp_Target_Addr_Mib_Root, target_ptr);
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

    /* If Target address table structure already exists then return error. */
    else
    {
        status = SNMP_GENERROR;
    }

    /* Return success or error code. */
    return (status);

} /* Create_snmpTargetAddrEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Create_snmpTargetParamsEntry
*
*   DESCRIPTION
*
*       This function is used to handle SNMP create request on Target
*       params table entry.
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
STATIC UINT16 Create_snmpTargetParamsEntry(const snmp_object_t *obj)
{
    /* Handle to snmpTargetParamsEntry. */
    SNMP_TARGET_PARAMS_TABLE *target_ptr = NU_NULL;

    /* Handle to find the proper place in the list. */
    SNMP_TARGET_PARAMS_TABLE *locator;

    /* Variable to hold target params name. */
    UINT8                       target_params[MAX_TARG_NAME_SZE];

    /* Status to return success or error code. */
    UINT16      status = SNMP_NOERROR;

    /* Variable to hold the comparison result. */
    INT         cmp_result;

    /* Variable to use in for-loop. */
    UINT32      i;

    /* Clear out the value of target params name. */
    UTL_Zero(target_params, sizeof(target_params));

    /* Get the value of target params name. */
    for (i = 0;
        (i < (MAX_TARG_NAME_SZE - 1)) &&
        (SNMP_TARGET_PARAM_SUB_LEN + i < obj->IdLen);
    i++)
    {
        target_params[i] = (UINT8)(obj->Id[SNMP_TARGET_PARAM_SUB_LEN + i]);
    }

    /* If we don't have valid object identifier then return error code. */
    if ((obj->IdLen != (SNMP_TARGET_PARAM_SUB_LEN + i)) ||
        (strlen((CHAR *)target_params) != i))
    {
        /* Return error code. */
        status = SNMP_NOSUCHINSTANCE;
    }

    /* If no error till now then proceed. */
    if (status == SNMP_NOERROR)
    {
        /* Get the handle to the target params table entry. This is a
         * create request so it should refer to current entry, thus second
         * parameter should be NU_TRUE.
         */
        target_ptr = Get_Target_Param_Entry(target_params, NU_TRUE);
    }

    /* If we don't currently have the Target params table structure. */
    if (!target_ptr)
    {
        /* Allocate memory for the new table structure. */
        if (NU_Allocate_Memory(&System_Memory,(VOID **)&target_ptr,
            sizeof(SNMP_TARGET_PARAMS_TABLE), NU_NO_SUSPEND) ==
            NU_SUCCESS)
        {
            target_ptr = TLS_Normalize_Ptr(target_ptr);

            /* Clear out the newly allocated table structure. */
            UTL_Zero(target_ptr, sizeof(SNMP_TARGET_PARAMS_TABLE));

            /* Set the target params name. */
            NU_BLOCK_COPY(target_ptr->snmp_target_params_name, target_params,
                strlen((CHAR*)target_params));

            /* Set the storage type to non-volatile. */
            target_ptr->snmp_target_params_storage_type =
                SNMP_STORAGE_DEFAULT;

            /* Set the status to 'createAndWait'. */
            target_ptr->snmp_target_params_row_status =
                SNMP_ROW_CREATEANDWAIT;

            /* Get the starting handle to the list to find the proper
             * position of new Target params table structure.
             */
            locator = Temp_Target_Param_Mib_Root.next;

            /* Loop to find the proper position. */
            while (locator)
            {
                /* Compare the target names */
                if((cmp_result = strcmp(locator->snmp_target_params_name,
                    target_ptr->snmp_target_params_name)) >= 0)
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
                locator = locator->next;
            }

            /* Add target params table structure to proper position provided
             * it is a valid new entry.
             */
            if (status == SNMP_NOERROR)
            {
                if (locator)
                {
                    DLL_Insert(&Temp_Target_Param_Mib_Root, target_ptr,
                        locator);
                }

                else
                {
                    DLL_Enqueue(&Temp_Target_Param_Mib_Root, target_ptr);
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

    /* If Target params table structure already exists then return error. */
    else
    {
        status = SNMP_GENERROR;
    }

    /* Return success or error code. */
    return (status);

} /* Create_snmpTargetParamsEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Undo_snmpTargetAddrEntry
*
*   DESCRIPTION
*
*       This function is used handle SNMP UNDO request on target address
*       table structures.
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
STATIC UINT16 Undo_snmpTargetAddrEntry(const snmp_object_t *obj)
{
    /* Handle to the Target address table structure. */
    SNMP_TARGET_ADDRESS_TABLE *target_ptr = NU_NULL;

    /* Variable to hold target address name. */
    UINT8                       target_name[MAX_TARG_NAME_SZE];

    /* Status to return success or error code. */
    UINT16                  status = SNMP_NOERROR;

    /* Variable to use in for-loop. */
    UINT32                  i;

    /* Clear out target name. */
    UTL_Zero(target_name, sizeof(target_name));

    /* Get the value of target address name. */
    for (i = 0;
        (i < (MAX_TARG_NAME_SZE - 1)) &&
        (SNMP_TARGET_ADDR_SUB_LEN + i < obj->IdLen);
    i++)
    {
        target_name[i] = (UINT8)(obj->Id[SNMP_TARGET_ADDR_SUB_LEN + i]);
    }

    /* If we don't have valid object identifier then return error code. */
    if ((obj->IdLen != (SNMP_TARGET_ADDR_SUB_LEN + i)) ||
        (strlen((CHAR *)target_name) != i))
    {
        /* Return error code. */
        status = SNMP_NOSUCHINSTANCE;
    }

    /* If no error till now then proceed. */
    if (status == SNMP_NOERROR)
    {
        /* Get the target address table entry from temporary list. */
        target_ptr = Get_Target_Addr_Entry_Util(target_name,
            &Temp_Target_Addr_Mib_Root);

        /* If we got the handle to target structure from above list. */
        if (target_ptr)
        {
            /* Remove the handle from the temporary list. */
            DLL_Remove(&Temp_Target_Addr_Mib_Root, target_ptr);

            /* Deallocate the memory attained by the structure. */
            if (NU_Deallocate_Memory(target_ptr) != NU_SUCCESS)
            {
                NLOG_Error_Log("SNMP: Failed to deallocate memory",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            /* Set the handle to NU_NULL. */
            target_ptr = NU_NULL;
        }

        else
        {
            /* Get the handle from permanent list. */
            target_ptr = Get_Target_Addr_Entry_Util(target_name,
                Snmp_Target_Mib.target_addr_table.next);
        }

        /* If we have the handle to the Target Address Table structure. */
        if (target_ptr)
        {
            /* Clearing the row flag. */
            target_ptr->snmp_target_addr_row_flag = 0;

            switch (obj->Id[SNMP_TARGET_ADDR_ATTR_OFFSET])
            {
            case 2:                         /* snmpTargetAddrTDomain */

                /* If length is not valid for 'snmpTargetAddrTDomain' then
                 * return error code.
                 */
                if ((obj->SyntaxLen >= SNMP_SIZE_SMALLOBJECTID) ||
                    (obj->SyntaxLen < 7))
                {
                    status = SNMP_WRONGLENGTH;
                }

                /* If value to set is valid. */
                else
                {
                    /* Set the value to 'snmpTargetAddrTDomain'. */
                    target_ptr->snmp_target_addr_tDomain =
                        (UINT32)obj->Syntax.BufInt[6];
                }

                break;

            case 3:                         /* snmpTargetAddrTAddress */

                /* The format of this field is dependent upon value of
                 * snmpTargetAddrTDomain. If it is the default domain
                 * SNMP_UDP i.e. UDP over IPv4 / IPv6 (RFC 1906 section-2),
                 * then this field would contain IP address followed by
                 * optional port number to which traps should be sent if
                 * specified.
                 */
                if ((obj->SyntaxLen == IP_ADDR_LEN + SNMP_PORT_NUM_LEN) ||
                    (obj->SyntaxLen == IP_ADDR_LEN ))
                {
                    /* Set the value of 'snmpTargetAddrTAddress'. */
                    NU_BLOCK_COPY(target_ptr->snmp_target_addr_tAddress,
                        obj->Syntax.BufChr,IP_ADDR_LEN);

                    /* Update the IP Address Family. */
                    target_ptr->snmp_target_addr_tfamily = NU_FAMILY_IP;

                    if (obj->SyntaxLen == IP_ADDR_LEN + SNMP_PORT_NUM_LEN)
                    {
                        /* Update the port number. */
                        target_ptr->snmp_target_addr_portnumber =
                            GET16(obj->Syntax.BufChr,IP_ADDR_LEN);
                    }

                    else
                    {
                        /* Mark port number as zero. This means port was
                         * not specified and notification module will use
                         * the default trap port.
                         */
                        target_ptr->snmp_target_addr_portnumber = 0;
                    }
                }

#if (INCLUDE_IPV6 == NU_TRUE)
                /* If address is IPv6. */
                else if ((obj->SyntaxLen == IP6_ADDR_LEN + SNMP_PORT_NUM_LEN)
                    || (obj->SyntaxLen == IP6_ADDR_LEN))
                {
                    /* Set the value of snmpTargetAddrTAddress. */
                    NU_BLOCK_COPY(target_ptr->snmp_target_addr_tAddress,
                        obj->Syntax.BufChr, IP6_ADDR_LEN);

                    /* Update the IP Address Family. */
                    target_ptr->snmp_target_addr_tfamily = NU_FAMILY_IP6;

                    if((obj->SyntaxLen == IP6_ADDR_LEN + SNMP_PORT_NUM_LEN))
                    {
                        /* Update the port number.if specified */
                        target_ptr->snmp_target_addr_portnumber =
                            GET16(obj->Syntax.BufChr,IP6_ADDR_LEN);
                    }

                    else
                    {
                        /* Mark port number as zero. This means port was
                         * not specified and notification module will use
                         * the default trap port.
                         */
                        target_ptr->snmp_target_addr_portnumber = 0;
                    }
                }
#endif /* (INCLUDE_IPV6 == NU_TRUE) */

                else /* The address field has invalid length. */
                {
                    status = SNMP_WRONGLENGTH;
                }

                break;

            case 4:                         /* snmpTargetAddrTimeout */

                /* Set the value of 'snmpTargetAddrTimeout'. */
                target_ptr->snmp_target_addr_time_out = obj->Syntax.LngUns;

                break;

            case 5:                         /* snmpTargetAddrRetryCount */

                /* Set the value of 'snmpTargetAddrRetryCount'. */
                target_ptr->snmp_target_addr_retry_count = obj->Syntax.LngUns;

                break;

            case 6:                         /* snmpTargetAddrTagList */

                /* If length is not valid for 'snmpTargetAddrTagList' then
                 * return error code.
                 */
                if ((obj->SyntaxLen >= SNMP_SIZE_SMALLOBJECTID) ||
                    (obj->SyntaxLen == 0))
                {
                    status = SNMP_WRONGLENGTH;
                }

                /* If the value of 'snmpTargetAddrTagList' is not valid then
                 * return error code.
                 */
                else if (strlen((CHAR *)obj->Syntax.BufChr) != obj->SyntaxLen)
                {
                    status = SNMP_WRONGVALUE;
                }

                /* If value to set is valid. */
                else
                {
                    /* Set the value of 'snmpTargetAddrTagList'. */
                    NU_BLOCK_COPY(target_ptr->snmp_target_addr_tag_list,
                        obj->Syntax.BufChr, obj->SyntaxLen);

                    target_ptr->snmp_target_addr_tag_list[obj->SyntaxLen] =
                        NU_NULL;

                    /* Update length of 'snmpTargetAddrTagList'. */
                    target_ptr->tag_list_len = obj->SyntaxLen;
                }

                break;

            case 7:                         /* snmpTargetAddrParams */

                /* If length is not valid for 'snmpTargetAddrParams' then
                 * return error code.
                 */
                if (obj->SyntaxLen >= SNMP_SIZE_SMALLOBJECTID)
                {
                    status = SNMP_WRONGLENGTH;
                }

                /* If the value of 'snmpTargetAddrParams' is not valid
                 * then return error code.
                 */
                else if (strlen((CHAR *)obj->Syntax.BufChr) != obj->SyntaxLen)
                {
                    status = SNMP_WRONGVALUE;
                }

                /* If value to set is valid. */
                else
                {
                    NU_BLOCK_COPY(target_ptr->snmp_target_addr_params,
                        obj->Syntax.BufChr, obj->SyntaxLen);

                    target_ptr->snmp_target_addr_params[obj->SyntaxLen] = 
                        NU_NULL;

                    /* Update length of 'snmpTargetAddrParams'. */
                    target_ptr->params_len = obj->SyntaxLen;
                }

                break;

            case 8:                         /* snmpTargetAddrStorageType */

                /* If the value of 'snmpTargetAddrStorageType' is not with
                 * in the valid range then return error code.
                 */
                if ((obj->Syntax.LngUns < 1) || (obj->Syntax.LngUns > 5))
                {
                    status = SNMP_WRONGVALUE;
                }

                /* If the value is in valid range. */
                else
                {
                    /* Set the value of 'snmpCommunityStorageType'. */
                    target_ptr->snmp_target_addr_storage_type =
                        (UINT8)(obj->Syntax.LngUns);
                }

                break;

            case 9:                         /* snmpTargetAddrRowStatus */

                /* If the value is not in valid range then return error code. */
                if ((obj->Syntax.LngUns < 1) || (obj->Syntax.LngUns > 6))
                {
                    status = SNMP_WRONGVALUE;
                }

                else if ((target_ptr->snmp_target_addr_row_status == 0) ||
                       (target_ptr->snmp_target_addr_row_status ==
                       (UINT8)(obj->Syntax.LngUns)))
                {
                    target_ptr->snmp_target_addr_row_status =
                        (UINT8)(obj->Syntax.LngUns);
                }

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
    }

    /* Return success or error code. */
    return status;

} /* Undo_snmpTargetAddrEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Undo_snmpTargetParamsEntry
*
*   DESCRIPTION
*
*       This function is used handle SNMP UNDO request on target params
*       table structures.
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
STATIC UINT16 Undo_snmpTargetParamsEntry(const snmp_object_t *obj)
{
    /* Handle to the Target params table structure. */
    SNMP_TARGET_PARAMS_TABLE *target_ptr = NU_NULL;

    /* Variable to hold target params name. */
    UINT8                       target_params[MAX_TARG_NAME_SZE];

    /* Status to return success or error code. */
    UINT16                  status = SNMP_NOERROR;

    /* Variable to use in for-loop. */
    UINT32                  i;

    /* Clear out target params name. */
    UTL_Zero(target_params, sizeof(target_params));

    /* Get the value of target params name. */
    for (i = 0;
        (i < (MAX_TARG_NAME_SZE - 1)) &&
        (SNMP_TARGET_PARAM_SUB_LEN + i < obj->IdLen);
    i++)
    {
        target_params[i] = (UINT8)(obj->Id[SNMP_TARGET_PARAM_SUB_LEN + i]);
    }

    /* If we don't have valid object identifier then return error code. */
    if ((obj->IdLen != (SNMP_TARGET_PARAM_SUB_LEN + i)) ||
        (strlen((CHAR *)target_params) != i))
    {
        /* Return error code. */
        status = SNMP_NOSUCHINSTANCE;
    }

    /* If no error till now then proceed. */
    if (status == SNMP_NOERROR)
    {
        /* Get the target params table entry from temporary list. */
        target_ptr = Get_Target_Params_Entry_Util(target_params,
            &Temp_Target_Param_Mib_Root);

        /* If we got the handle to target structure from above list. */
        if (target_ptr)
        {
            /* Remove the handle from the temporary list. */
            DLL_Remove(&Temp_Target_Param_Mib_Root, target_ptr);

            /* Deallocate the memory attained by the structure. */
            if (NU_Deallocate_Memory(target_ptr) != NU_SUCCESS)
            {
                NLOG_Error_Log("SNMP: Failed to deallocate memory",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
            }

            /* Set the handle to NU_NULL. */
            target_ptr = NU_NULL;
        }

        else
        {
            /* Get the handle from permanent list. */
            target_ptr = Get_Target_Params_Entry_Util(target_params,
                Snmp_Target_Mib.target_params_table.next);
        }

        /* If we have the handle to the Target Params Table structure. */
        if (target_ptr)
        {
            /* Clearing the row flag. */
            target_ptr->snmp_target_params_row_flag = 0;

            switch (obj->Id[SNMP_TARGET_PARAM_ATTR_OFFSET])
            {
            case 2:                         /* snmpTargetParamsMPModel */

                /* If the value of 'snmpTargetParamsMPModel'is not in valid
                 * range then return error code.
                 */
                if ((obj->Syntax.LngUns > 3) || (obj->Syntax.LngUns < 1))
                {
                    status = SNMP_WRONGVALUE;
                }

                /* If value to set is valid. */
                else
                {
                    /* Set the value to 'snmpTargetParamsMPModel'. */
                    target_ptr->snmp_target_params_mp_model =
                        obj->Syntax.LngUns;
                }

                break;

            case 3:                      /* snmpTargetParamsSecurityModel */

                /* If the value of 'snmpTargetParamsSecurityModel'is not in
                 * valid range then return error code.
                 */
                if ((obj->Syntax.LngUns > 3) || (obj->Syntax.LngUns < 1))
                {
                    status = SNMP_WRONGVALUE;
                }

                else
                {
                    /* Set the value of 'snmpTargetParamsSecurityModel'. */
                    target_ptr->snmp_target_params_security_model =
                        obj->Syntax.LngUns;
                }

                break;

            case 4:                       /* snmpTargetParamsSecurityName */

                /* If length is not valid for 'snmpTargetParamsSecurityName'
                 * then return error code.
                 */
                if ((obj->SyntaxLen >= SNMP_SIZE_SMALLOBJECTID) ||
                    (obj->SyntaxLen == 0))
                {
                    status = SNMP_WRONGLENGTH;
                }

                /* If the value of 'snmpTargetParamsSecurityName' is not
                 * valid then return error code.
                 */
                else if (strlen((CHAR *)obj->Syntax.BufChr) != obj->SyntaxLen)
                {
                    status = SNMP_WRONGVALUE;
                }

                /* If value to set is valid. */
                else
                {
                    /* Set the value of 'snmpTargetParamsSecurityName'. */
                    NU_BLOCK_COPY(target_ptr->
                        snmp_target_params_security_name,
                        obj->Syntax.BufChr, obj->SyntaxLen);

                    target_ptr->snmp_target_params_security_name[obj->
                        SyntaxLen] = NU_NULL;
                }

                break;

            case 5:                      /* snmpTargetParamsSecurityLevel */

                /* If the value of 'snmpTargetParamsSecurityLevel'is not in
                 * valid range then return error code.
                 */
                if ((obj->Syntax.LngUns > 3) || (obj->Syntax.LngUns < 1))
                {
                    status = SNMP_WRONGVALUE;
                }

                else
                {
                    /* Set the value of 'snmpTargetParamsSecurityLevel'. */
                    target_ptr->snmp_target_params_security_level =
                        obj->Syntax.LngUns;
                }

                break;

            case 6:                         /* snmpTargetParamsStorageType */

                /* If the value of 'snmpTargetParamsStorageType' is not in
                 * valid range then return error code.
                 */
                if ((obj->Syntax.LngUns < 1) || (obj->Syntax.LngUns > 5))
                {
                    status = SNMP_WRONGVALUE;
                }

                /* If value to set is valid. */
                else
                {
                    /* Set the value of 'snmpTargetParamsStorageType'. */
                    target_ptr->snmp_target_params_storage_type =
                       (UINT8)(obj->Syntax.LngUns);
                }

                break;

            case 7:                         /* snmpTargetParamsRowStatus */

                /* If the value is not in valid range then return error code. */
                if ((obj->Syntax.LngUns < 1) || (obj->Syntax.LngUns > 6))
                {
                    status = SNMP_WRONGVALUE;
                }

                else if ((target_ptr->snmp_target_params_row_status == 0) ||
                    (target_ptr->snmp_target_params_row_status ==
                    (UINT8)(obj->Syntax.LngUns)))
                {
                    target_ptr->snmp_target_params_row_status =
                        (UINT8)(obj->Syntax.LngUns);
                }

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
    }

    /* Return success or error code. */
    return status;

} /* Undo_snmpTargetParamsEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Commit_snmpTargetAddrEntryStatus
*
*   DESCRIPTION
*
*       This function is used to commit status of Target table structure.
*
*   INPUTS
*
*       *table_ptr              Pointer to Target address table structure.
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
STATIC UINT16 Commit_snmpTargetAddrEntryStatus(
    SNMP_TARGET_ADDRESS_TABLE *table_ptr,
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

    /* If target address name, domain, target ip address or params is not
     * set then set temporary status to 'notReady'.Otherwise set it to
     * 'notInService'.This is explained in RFC 3413 Section 4.1.2.
     * Third memcmp call below uses SNMP_MAX_IP_ADDRS because it's value is
     * always less than SNMP_SIZE_SMALLOBJECTID and call will always
     * fail if latter is used.
     */
    if ( memcmp(check_buffer, table_ptr->snmp_target_addr_name,
        SNMP_SIZE_SMALLOBJECTID) == 0 ||
        table_ptr->snmp_target_addr_tDomain == 0 ||
        memcmp(check_buffer, table_ptr->snmp_target_addr_tAddress,
        SNMP_MAX_IP_ADDRS) == 0 ||
        memcmp(check_buffer,table_ptr->snmp_target_addr_params,
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
                table_ptr->snmp_target_addr_row_status = SNMP_ROW_ACTIVE;
            }

            else
            {
                /* Setting status to error code of in-consistent value. */
                status = SNMP_INCONSISTANTVALUE;
            }

            break;

        case SNMP_ROW_NOTINSERVICE:

            /* Row status can't be set to NOTINSERVICE for new and not ready
             * entries. However row status can be set to NOTINSERVICE by
             * setting row status to CREATEANDWAIT for new entries.
             */
            if((temp_status == SNMP_ROW_NOTINSERVICE) && (!is_new))
            {
                /* Setting the row status to 'NOTINSERVICE'. */
                table_ptr->snmp_target_addr_row_status = SNMP_ROW_NOTINSERVICE;
            }

            else
            {
                /* Inconsistent value. */
                status = SNMP_INCONSISTANTVALUE;
            }

            break;

        case SNMP_ROW_DESTROY:

            /* Setting row status to SNMP_ROW_DESTROY. */
            table_ptr->snmp_target_addr_row_status = SNMP_ROW_DESTROY;

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
                (table_ptr->snmp_target_addr_row_status != SNMP_ROW_ACTIVE)))
            {
                /* Activating WEP KEY MAPPING. */
                table_ptr->snmp_target_addr_row_status = SNMP_ROW_ACTIVE;
            }

            else
            {
                /* Setting status to error code of in-consistent value. */
                status = SNMP_INCONSISTANTVALUE;
            }

            break;

        case SNMP_ROW_CREATEANDWAIT:

            /* Entry should not be 'active'. */
            if(table_ptr->snmp_target_addr_row_status != SNMP_ROW_ACTIVE)
            {
                table_ptr->snmp_target_addr_row_status = (UINT8)temp_status;
            }

            else
            {
                /* Setting status to error code of in-consistent value. */
                status = SNMP_INCONSISTANTVALUE;
            }

            break;

        default:

            if ((table_ptr->snmp_target_addr_row_flag == 0) ||
                (temp_status == SNMP_ROW_NOTREADY) ||
                (table_ptr->snmp_target_addr_row_flag ==
                SNMP_ROW_DESTROY))
            {
                table_ptr->snmp_target_addr_row_status = temp_status;
            }
            else if (table_ptr->snmp_target_addr_row_flag ==
                SNMP_ROW_CREATEANDGO)
            {
                if (temp_status == SNMP_ROW_NOTINSERVICE)
                    table_ptr->snmp_target_addr_row_status = SNMP_ROW_ACTIVE;
                else
                    status = SNMP_INCONSISTANTVALUE;
            }
            else if (table_ptr->snmp_target_addr_row_flag ==
                SNMP_ROW_CREATEANDWAIT)
            {
                table_ptr->snmp_target_addr_row_status = temp_status;
            }
        }
    }

    /* Return success or error code. */
    return (status);

} /* Commit_snmpTargetAddrEntryStatus */

/*************************************************************************
*
*   FUNCTION
*
*       Commit_snmpTargetParamsEntryStatus
*
*   DESCRIPTION
*
*       This function is used to commit status of Target Params table.
*
*   INPUTS
*
*       *table_ptr              Pointer to Target params table structure.
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
STATIC UINT16 Commit_snmpTargetParamsEntryStatus(
    SNMP_TARGET_PARAMS_TABLE *table_ptr,
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

    /* If target params name, MP model, security model, security name and
     * security level are not set then make temporary status 'notReady',
     * otherwise set it to 'notInService' as required by RFC3413
     * section-4.1.2.
     */
    if ( memcmp(check_buffer, table_ptr->snmp_target_params_name,
        SNMP_SIZE_SMALLOBJECTID) == 0 ||
        ( (table_ptr->snmp_target_params_mp_model != SNMP_VERSION_V1) &&
          (table_ptr->snmp_target_params_mp_model != SNMP_VERSION_V2) &&
          (table_ptr->snmp_target_params_mp_model != SNMP_VERSION_V3) )
        ||
        (table_ptr->snmp_target_params_security_model == 0) ||
         memcmp(check_buffer, table_ptr->snmp_target_params_security_name,
        SNMP_SIZE_SMALLOBJECTID) == 0 ||
        (table_ptr->snmp_target_params_security_level == 0))
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
                table_ptr->snmp_target_params_row_status = SNMP_ROW_ACTIVE;
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
                table_ptr->snmp_target_params_row_status = SNMP_ROW_NOTINSERVICE;
            }

            else
            {
                /* Inconsistent value. */
                status = SNMP_INCONSISTANTVALUE;
            }

            break;

        case SNMP_ROW_DESTROY:

            /* Setting row status to SNMP_ROW_DESTROY. */
            table_ptr->snmp_target_params_row_status = SNMP_ROW_DESTROY;

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
                (table_ptr->snmp_target_params_row_status != SNMP_ROW_ACTIVE)))
            {
                /* Activating WEP KEY MAPPING. */
                table_ptr->snmp_target_params_row_status = SNMP_ROW_ACTIVE;
            }

            else
            {
                /* Setting status to error code of in-consistent value. */
                status = SNMP_INCONSISTANTVALUE;
            }

            break;

        case SNMP_ROW_CREATEANDWAIT:

            /* Entry should not be 'active'. */
            if(table_ptr->snmp_target_params_row_status != SNMP_ROW_ACTIVE)
            {
                table_ptr->snmp_target_params_row_status = temp_status;
            }

            else
            {
                /* Setting status to error code of in-consistent value. */
                status = SNMP_INCONSISTANTVALUE;
            }

            break;

        default:

            if ((table_ptr->snmp_target_params_row_flag == 0) ||
                (temp_status == SNMP_ROW_NOTREADY) ||
                (table_ptr->snmp_target_params_row_flag ==
                SNMP_ROW_DESTROY))
            {
                table_ptr->snmp_target_params_row_status = temp_status;
            }
            else if (table_ptr->snmp_target_params_row_flag ==
                SNMP_ROW_CREATEANDGO)
            {
                if (temp_status == SNMP_ROW_NOTINSERVICE)
                    table_ptr->snmp_target_params_row_status = SNMP_ROW_ACTIVE;
                else
                    status = SNMP_INCONSISTANTVALUE;
            }
            else if (table_ptr->snmp_target_params_row_flag ==
                SNMP_ROW_CREATEANDWAIT)
            {
                table_ptr->snmp_target_params_row_status = temp_status;
            }
        }
    }

    /* Return success or error code. */
    return (status);

} /* Commit_snmpTargetParamsEntryStatus */

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)
/************************************************************************
*
*   FUNCTION
*
*       TargetMIB_Compare_name
*
*   DESCRIPTION
*
*       This function is used to compare the target names of the Target
*       address table entries. This function is being used by the file
*       component.
*
*   INPUTS
*
*       *left_side                      Pointer to a Target entry.
*       *right_side                     Pointer to a Target entry.
*
*   OUTPUTS
*
*       0                               Both the entries are equal.
*       -1                              The entries are not equal.
*
*************************************************************************/
INT32 TargetMIB_Compare_name(VOID *left_side, VOID *right_side)
{
    SNMP_TARGET_ADDRESS_TABLE   *left_ptr = left_side;
    SNMP_TARGET_ADDRESS_TABLE   *right_ptr = right_side;
    INT32                   cmp = -1;

    /* Compare the target names. This determines the position of the entry
    * in target tables.
    */
    if ( strcmp(left_ptr->snmp_target_addr_name,
        right_ptr->snmp_target_addr_name) == 0 )
    {
        cmp = 0;
    }

    return (cmp);

} /* TargetMIB_Compare_name */

/************************************************************************
*
*   FUNCTION
*
*       TargetMIB_Compare_params
*
*   DESCRIPTION
*
*       This function is used to compare the target param names of the Target
*       params table entries. This function is being used by the file
*       component.
*
*   INPUTS
*
*       *left_side                      Pointer to a Target entry.
*       *right_side                     Pointer to a Target entry.
*
*   OUTPUTS
*
*       0                               Both the entries are equal.
*       -1                              The entries are not equal.
*
*************************************************************************/
INT32 TargetMIB_Compare_params(VOID *left_side, VOID *right_side)
{
    SNMP_TARGET_PARAMS_TABLE   *left_ptr = left_side;
    SNMP_TARGET_PARAMS_TABLE   *right_ptr = right_side;
    INT32                   cmp = -1;

    /* Compare the target param names. This determines the position of the
     * entry in target tables.
     */
    if ( strcmp(left_ptr->snmp_target_params_name,
        right_ptr->snmp_target_params_name) == 0 )
    {
        cmp = 0;
    }

    return (cmp);

} /* TargetMIB_Compare_params */

/************************************************************************
*
*   FUNCTION
*
*       Save_Target_Addr
*
*   DESCRIPTION
*
*       This function saves the new state of Target Address Table to the file.
*
*   INPUTS
*
*       *target_addr           The target addr entry whose changed state is to
*                               be saved
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation successful.
*
*************************************************************************/
STATUS Save_Target_Addr(SNMP_TARGET_ADDRESS_TABLE *target_addr)
{
    /* This variable will be used by the file component to temporarily
    * store information read from file.
    */
    SNMP_TARGET_ADDRESS_TABLE   read_target_addr;
    STATUS                      status;

    NU_SUPERV_USER_VARIABLES
        NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* Call the function that performs the actual logic. */
    status = SNMP_Save(
        Snmp_Target_Mib.target_addr_table.next,
        target_addr,
        &read_target_addr, SNMP_MEMBER_OFFSET(SNMP_TARGET_ADDRESS_TABLE,
        snmp_target_addr_storage_type),
        SNMP_MEMBER_OFFSET(SNMP_TARGET_ADDRESS_TABLE, snmp_target_addr_row_status),
        sizeof(SNMP_TARGET_ADDRESS_TABLE),
        TARGET_ADDR_FILE,
        TargetMIB_Compare_name,
        INCLUDE_MIB_TARGET);

    NU_USER_MODE();    /* return to user mode */
    return (status);

} /* Save_Target_Addr */

/************************************************************************
*
*   FUNCTION
*
*       Save_Target_Params
*
*   DESCRIPTION
*
*       This function saves the new state of Target Params Table to the file.
*
*   INPUTS
*
*       *target_params           The target param entry whose changed state
*                                is to be saved.
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation successful.
*
*************************************************************************/
STATUS Save_Target_Params(SNMP_TARGET_PARAMS_TABLE *target_param)
{
    /* This variable will be used by the file component to temporarily
    * store information read from file.
    */
    SNMP_TARGET_PARAMS_TABLE   read_target_param;
    STATUS                      status;

    NU_SUPERV_USER_VARIABLES
        NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

    /* Call the function that performs the actual logic. */
    status = SNMP_Save(
        Snmp_Target_Mib.target_params_table.next,
        target_param,
        &read_target_param, SNMP_MEMBER_OFFSET(SNMP_TARGET_PARAMS_TABLE,
        snmp_target_params_storage_type),
        SNMP_MEMBER_OFFSET(SNMP_TARGET_PARAMS_TABLE, snmp_target_params_row_status),
        sizeof(SNMP_TARGET_PARAMS_TABLE),
        TARGET_PARAMS_FILE,
        TargetMIB_Compare_params,
        INCLUDE_MIB_TARGET);

    NU_USER_MODE();    /* return to user mode */
    return (status);

} /* Save_Target_Params */

#endif /* (SNMP_ENABLE_FILE_STORAGE == NU_TRUE) */

/*************************************************************************
*
*   FUNCTION
*
*       Commit_snmpTargetAddrEntries
*
*   DESCRIPTION
*
*       This function is used to commit all the newly created Target address
*       table structures.
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
STATIC VOID Commit_snmpTargetAddrEntries(VOID)
{
    /* Handle to the target address table structure. */
    SNMP_TARGET_ADDRESS_TABLE *target_ptr = Temp_Target_Addr_Mib_Root.next;

    /* Loop till there exists an entry in temporary list. */
    while(target_ptr)
    {
        /* Add target address table structure into permanent list. */
        if (SNMP_Add_Target(target_ptr) != NU_SUCCESS)
        {
            NLOG_Error_Log("SNMP: Failed to Add Target",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* Remove a target address table structure from the temporary list. */
        DLL_Dequeue(&Temp_Target_Addr_Mib_Root);

        /* Deallocate memory of above structure as SNMP_Add_Target
         * creates its own copy.
         */
        if (NU_Deallocate_Memory(target_ptr) != NU_SUCCESS)
        {
            NLOG_Error_Log("SNMP: Failed to deallocate memory",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* Getting next handle to the table structure from the temporary
         * list.
         */
        target_ptr = Temp_Target_Addr_Mib_Root.next;
    }

    target_ptr = Snmp_Target_Mib.target_addr_table.next;

    while(target_ptr)
    {
        /* Clear out the row flag. */
        target_ptr->snmp_target_addr_row_flag = 0;

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)
        /* Update data in file */
        Save_Target_Addr(target_ptr);
#endif

        /* Moving forward in the list. */
        target_ptr = target_ptr->next;
    }

} /* Commit_snmpTargetAddrEntries */

/*************************************************************************
*
*   FUNCTION
*
*       Commit_snmpTargetParamsEntries
*
*   DESCRIPTION
*
*       This function is used to commit all the newly created Target params
*       table structures.
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
STATIC VOID Commit_snmpTargetParamsEntries(VOID)
{
    /* Handle to the target params table structure. */
    SNMP_TARGET_PARAMS_TABLE *target_ptr = Temp_Target_Param_Mib_Root.next;

    /* Loop till there exists an entry in temporary list. */
    while(target_ptr)
    {
        /* Add target params table structure into permanent list. */
        if (SNMP_Add_Params(target_ptr) != NU_SUCCESS)
        {
            NLOG_Error_Log("SNMP: Failed to Add Target Params",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* Remove a target params table structure from the temporary list. */
        DLL_Dequeue(&Temp_Target_Param_Mib_Root);

        /* Deallocate memory of above structure as SNMP_Add_Params
         * creates its own copy.
         */
        if (NU_Deallocate_Memory(target_ptr) != NU_SUCCESS)
        {
            NLOG_Error_Log("SNMP: Failed to deallocate memory",
                NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* Getting next handle to the table structure from the temporary
         * list.
         */
        target_ptr = Temp_Target_Param_Mib_Root.next;
    }

    target_ptr = Snmp_Target_Mib.target_params_table.next;

    while(target_ptr)
    {
        /* Clear out the row flag. */
        target_ptr->snmp_target_params_row_flag = 0;

#if (SNMP_ENABLE_FILE_STORAGE == NU_TRUE)
        /* Update data in file */
        Save_Target_Params(target_ptr);
#endif

        /* Moving forward in the list. */
        target_ptr = target_ptr->next;
    }

} /* Commit_snmpTargetParamsEntries */

/*************************************************************************
*
*   FUNCTION
*
*       snmpTargetSpinLock
*
*   DESCRIPTION
*
*       This function is used to handle SNMP request on
*       'snmpTargetSpinLock'.
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
UINT16 snmpTargetSpinLock(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    /* Status to return success or error code. */
    UINT16              status;

    /* Avoid compilation warnings. */
    UNUSED_PARAMETER(param);

    /* If we have valid request. */
    if (MibSimple(obj, idlen) == NU_TRUE)
    {
        /* Set return status to success code. */
        status = SNMP_NOERROR;

        /* If we have SNMP SET request. */
        if (obj->Request == SNMP_PDU_SET)
        {
            /* If we have valid value to set. */
            if(Snmp_Target_Mib.snmp_target_spin_lock ==
                                                       obj->Syntax.LngUns)
            {
                /* Increment the spin lock. */
                Snmp_Target_Mib.snmp_target_spin_lock++;
            }

            /* If we don't have valid value to set then return error code.
             */
            else
            {
                /* Return error code. */
                status = SNMP_INCONSISTANTVALUE;
            }
        }

        /* If we have GET, GET-NEXT or GET-BULK request then proceed. */
        else if ((obj->Request != SNMP_PDU_UNDO) &&
                 (obj->Request != SNMP_PDU_COMMIT))
        {
            obj->Syntax.LngUns = Snmp_Target_Mib.snmp_target_spin_lock;
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

} /* snmpTargetSpinLock */

/*************************************************************************
*
*   FUNCTION
*
*       Get_snmpTargetAddrEntry
*
*   DESCRIPTION
*
*       This function is used to process SNMP GET/GET-NEXT request on
*       snmpTargetAddrTable.
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
STATIC UINT16 Get_snmpTargetAddrEntry(snmp_object_t *obj, UINT8 getflag)
{
    /* Handle to the target address entry. */
    SNMP_TARGET_ADDRESS_TABLE   *target_entry = NU_NULL;

    /* Variable to hold target address name. */
    UINT8                       target_name[MAX_TARG_NAME_SZE];

    /* SNMP Target Domains' OID. */
    UINT32                      snmp_domains_oid[] = {1, 3, 6, 1, 6, 1};

    /* Table entry object identifier. */
    UINT32                      table_oid[] = {1, 3, 6, 1, 6, 3, 12, 1, 2,
                                               1};

    /* Variable to use in for-loop. */
    UINT32                      i;

    /* Status to return success or error code. */
    UINT16                      status = SNMP_NOERROR;

    /* Clear out target name. */
    UTL_Zero(target_name, sizeof(target_name));

    /* Get the value of target address name. */
    for (i = 0;
         (i < (MAX_TARG_NAME_SZE - 1)) &&
         (SNMP_TARGET_ADDR_SUB_LEN + i < obj->IdLen);
         i++)
    {
        target_name[i] = (UINT8)(obj->Id[SNMP_TARGET_ADDR_SUB_LEN + i]);
    }

    /* If we are handling GET request. */
    if (getflag)
    {
        /* If we don't have valid object identifier then return error
         * code.
         */
        if ((obj->IdLen != (SNMP_TARGET_ADDR_SUB_LEN + i)) ||
            (strlen((CHAR *)target_name) != i))
        {
            /* Return error code. */
            status = SNMP_NOSUCHINSTANCE;
        }
    }

   /* If we are handling GET-NEXT request make community index, null
    * terminating.
    */
    else if (i < SNMP_SIZE_SMALLOBJECTID)
    {
        target_name[i] = NU_NULL;
    }

    /* If no error till now then proceed. */
    if (status == SNMP_NOERROR)
    {
        /* Get the handle to the target address table entry. */
        target_entry = Get_Target_Addr_Entry(target_name, getflag);
    }

    /* If we got the handle to the target address entry then proceed. */
    if (target_entry)
    {
        switch(obj->Id[SNMP_TARGET_ADDR_ATTR_OFFSET])
        {
        case 2:                         /* snmpTargetAddrTDomain */

            /* Copy SNMP Domain OID. */
            NU_BLOCK_COPY(obj->Syntax.BufInt, snmp_domains_oid,
                sizeof(snmp_domains_oid));

            /* UDP over IPv4 which is the default domain. */
            if (target_entry->snmp_target_addr_tDomain == SNMP_UDP)
            {
                obj->Syntax.BufInt[6] = SNMP_UDP;
            }

            /* UDP over IPv6 domain. */
            else
            {
                /* The OID for target domain UDP over IPv6 is not defined.
                 * However, it's most likely that it would be
                 * 1.3.6.1.6.1.6, when defined.
                 */
                obj->Syntax.BufInt[6] = 6;
            }

            obj->SyntaxLen = 7;
            break;

        case 3:                         /* snmpTargetAddrTAddress */

            /* The format of this field is dependent upon value of
             * snmpTargetAddrTDomain. If it is the default domain-SNMP_UDP
             * i.e. UDP over IPv4 / IPv6 (RFC 1906 section-2), then this
             * field would contain IP address followed by optional port
             * number to which traps should be sent if specified.
             */
            if (target_entry->snmp_target_addr_tfamily == NU_FAMILY_IP)
            {
                /* Get the value of 'snmpTargetAddrTAddress'. */
                NU_BLOCK_COPY(obj->Syntax.BufChr,
                              target_entry->snmp_target_addr_tAddress,
                              IP_ADDR_LEN);

                /* Get the value of 'snmpTargetAddrPortNumber'. */
                PUT16(obj->Syntax.BufChr,IP_ADDR_LEN,
                    target_entry->snmp_target_addr_portnumber);

                /* Update the length of IP Address with port number. */
                obj->SyntaxLen = IP_ADDR_LEN + SNMP_PORT_NUM_LEN;
            }

#if (INCLUDE_IPV6 == NU_TRUE)

            /* If address is of IPv6. */
            else if (target_entry->snmp_target_addr_tfamily ==
                                                            NU_FAMILY_IP6)
            {
                /* Get the value of snmpTargetAddrTAddress. */
                NU_BLOCK_COPY(obj->Syntax.BufChr,
                              target_entry->snmp_target_addr_tAddress,
                              IP6_ADDR_LEN);

                /* Get the value of 'snmpTargetAddrPortNumber'*/
                PUT16(obj->Syntax.BufChr,IP6_ADDR_LEN,
                    target_entry->snmp_target_addr_portnumber);

                /* Update the length of IP address. */
                obj->SyntaxLen = IP6_ADDR_LEN + SNMP_PORT_NUM_LEN;
            }

#endif /* (INCLUDE_IPV6 == NU_TRUE) */

            /* If address is from other than IPv6 and IPv4 address. */
            else
            {
                /* Return error code. */
                status = SNMP_GENERROR;
            }

            break;

        case 4:                         /* snmpTargetAddrTimeout */

            /* Get the value of 'snmpTargetAddrTimeout'. */
            obj->Syntax.LngUns = target_entry->snmp_target_addr_time_out;

            break;

        case 5:                         /* snmpTargetAddrRetryCount */

            /* Get the value of 'snmpTargetAddrRetryCount'. */
            obj->Syntax.LngUns =
                       (UINT32)target_entry->snmp_target_addr_retry_count;

            break;

        case 6:                         /* snmpTargetAddrTagList */

            /* Get the value of 'snmpTargetAddrTagList'. */
            NU_BLOCK_COPY(obj->Syntax.BufChr,
                         target_entry->snmp_target_addr_tag_list,
                         target_entry->tag_list_len);

            /* Update length of 'snmpTargetAddrTagList'. */
            obj->SyntaxLen = target_entry->tag_list_len;

            break;

        case 7:                         /* snmpTargetAddrParams */

            /* Get the length of 'snmpTargetAddrParams'. */
            obj->SyntaxLen =
                  strlen((CHAR *)(target_entry->snmp_target_addr_params));

            NU_BLOCK_COPY(obj->Syntax.BufChr,
                          target_entry->snmp_target_addr_params,
                          obj->SyntaxLen);

            break;

        case 8:                     /* snmpTargetAddrStorageType */

            /* Get the value of 'snmpTargetAddrStorageType'. */
            obj->Syntax.LngUns =
                            target_entry->snmp_target_addr_storage_type;

            break;

        case 9:                     /* snmpTargetAddrRowStatus */

            /* Get the value of 'snmpTargetAddrRowStatus'. */
            obj->Syntax.LngUns =
                                target_entry->snmp_target_addr_row_status;

            break;

        default:
            /* We have reached at end of the table. */

            /* Return error code. */
            status = SNMP_NOSUCHOBJECT;
        }

        /* If request was successful and we are handling GET-NEXT request
         * then update the object identifier.
         */
        if ((status == SNMP_NOERROR) && (!getflag))
        {
            /* Update OID for target address table. */
            NU_BLOCK_COPY(obj->Id, table_oid, sizeof(table_oid));

            /* Update target name in the object identifier. */
            for (i = 0; target_entry->snmp_target_addr_name[i]; i++)
            {
                obj->Id[SNMP_TARGET_ADDR_SUB_LEN + i] =
                         (UINT32)(target_entry->snmp_target_addr_name[i]);
            }

            /* Update the length of object identifier. */
            obj->IdLen = (SNMP_TARGET_ADDR_SUB_LEN + i);
        }
    }

    /* If we did not get the handle to the target address entry then
     * return error code.
     */
    else
    {
        /* Return error code. */
        status = SNMP_NOSUCHINSTANCE;
    }

    /* Return success or error code. */
    return (status);

} /* Get_snmpTargetAddrEntry */

/*************************************************************************
*
*   FUNCTION
*
*       snmpTargetAddrEntry
*
*   DESCRIPTION
*
*       This function is used to process SNMP request on
*       snmpTargetAddrTable.
*
*   INPUTS
*
*       *obj                    SNMP object containing request.
*       idlen                   Unused.
*       *param                  Unused.
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
UINT16 snmpTargetAddrEntry(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    /* Status to return success or error code. */
    UINT16              status = SNMP_NOERROR;

    /* Flag to distinguish GET and GET-NEXT request. */
    UINT8               getflag = 0;

    /* Handle to the target address entry. */
    SNMP_TARGET_ADDRESS_TABLE   *target_entry;

    /* Handle to the temporary target address entry. */
    SNMP_TARGET_ADDRESS_TABLE   *temp_target_entry;

    /* Variable to hold target address name. */
    UINT8                       target_name[MAX_TARG_NAME_SZE];

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
    UNUSED_PARAMETER(idlen);
    UNUSED_PARAMETER(param);

    switch(obj->Request)
    {
    case SNMP_PDU_GET:                      /* Get Request. */

        /* Set getflag to represent GET request. */
        getflag++;

        /* Fall through next case to process request. */

    case SNMP_PDU_NEXT:

        /* Process GET or GET-NEXT request. */
        status = Get_snmpTargetAddrEntry(obj, getflag);

        break;

    case SNMP_PDU_BULK:                     /* Get-BULK request. */

        /* Process GET-BULK request. */
        status = SNMP_Get_Bulk(obj, Get_snmpTargetAddrEntry);

        break;

    case SNMP_PDU_SET:                   /* Set request. */

        Target_Addr_MIB_Commit_Left++;

        /* Process the SET operation. */
        status = Set_snmpTargetAddrEntry(obj);

        if (status != SNMP_NOSUCHINSTANCE)
            break;

    case SNMP_PDU_CREATE:                   /* Create request. */

        /* Processing of create operation. */
        status = Create_snmpTargetAddrEntry(obj);

        /* If the entry was successfully created, set the value. */
        if(status == SNMP_NOERROR)
            status = Set_snmpTargetAddrEntry(obj);

        break;

    case SNMP_PDU_UNDO:                     /* Undo request. */

        Target_Addr_MIB_Commit_Left = 0;

        /* Processing of UNDO operations. */
        status = Undo_snmpTargetAddrEntry(obj);

        break;

    case SNMP_PDU_COMMIT:                   /* Commit request. */

        Target_Addr_MIB_Commit_Left--;

        /* Clear out target name. */
        UTL_Zero(target_name, sizeof(target_name));

        /* Get the value of target address name. */
        for (i = 0;
            (i < (MAX_TARG_NAME_SZE - 1)) &&
            (SNMP_TARGET_ADDR_SUB_LEN + i < obj->IdLen);
        i++)
        {
            target_name[i] = (UINT8)(obj->Id[SNMP_TARGET_ADDR_SUB_LEN + i]);
        }

        /* If we don't have valid object identifier then return error
         * code.
         */
        if ((obj->IdLen != (SNMP_TARGET_ADDR_SUB_LEN + i)) ||
            (strlen((CHAR *)target_name) != i))
        {
            /* Return error code. */
            status = SNMP_NOSUCHINSTANCE;
        }

        /* If no error till now then proceed. */
        if (status == NU_SUCCESS)
        {
            /* Get the handle to the target address table entry from
             * temporary list.
             */
            target_entry = Get_Target_Addr_Entry_Util(target_name,
                                                &Temp_Target_Addr_Mib_Root);

            /* Save the handle in temporary one. */
            temp_target_entry = target_entry;

            /* If we did not get handle to the Target Address Table from
             * the temporary list then get it from the permanent list.
             */
            if (!target_entry)
            {
                /* Get the handle from the permanent list. */
                target_entry = Get_Target_Addr_Entry_Util(target_name,
                                   Snmp_Target_Mib.target_addr_table.next);
            }

            /* If we got the handle either from permanent or temporary list.
             */
            if (target_entry)
            {
                /* Check whether it was a row set operation. */
                if (obj->Id[SNMP_TARGET_ADDR_ATTR_OFFSET] == 9)
                {
                    /* Update the value of current state.
                     *  0) New entry.
                     *  1) Not ready.
                     *  2) Not In service.
                     *  3) Active
                     */

                    /* If we have new entry the set current state to 0. */
                    if (temp_target_entry)
                    {
                        current_state = 0;
                    }

                    /* If we have new not ready entry then set current state
                     * to 1.
                     */
                    else if ((strlen((CHAR *)target_entry->
                        snmp_target_addr_name) == 0))
                    {
                        current_state = 1;
                    }

                    /* If we have not in service entry then set current state
                     * to 2.
                     */
                    else if (target_entry->snmp_target_addr_row_status !=
                        SNMP_ROW_ACTIVE)
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
                        status = Commit_snmpTargetAddrEntryStatus(
                            target_entry, (UINT8)(obj->Syntax.LngUns),
                            (UINT8)(temp_target_entry != NU_NULL) );
                    }
                }

                else
                {
                    status = Commit_snmpTargetAddrEntryStatus(
                        target_entry, 0, NU_FALSE);
                }

                /* If the row status is set to 'DESTROY'. */
                if (target_entry->snmp_target_addr_row_status ==
                    SNMP_ROW_DESTROY)
                {
                    /* If handle was from temporary list then remove it
                     * from temporary list.
                     */
                    if (temp_target_entry)
                        DLL_Remove(&Temp_Target_Addr_Mib_Root, target_entry);

                    /* If handle was from permanent list then remove it from
                     * permanent list.
                     */
                    else
                        DLL_Remove(&Snmp_Target_Mib.target_addr_table,
                            target_entry);

                    /* Deallocate memory attained by this target address
                     * table structure.
                     */
                    if (NU_Deallocate_Memory(target_entry) !=
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
            if ((status == SNMP_NOERROR) && (Target_Addr_MIB_Commit_Left == 0))
            {
                Commit_snmpTargetAddrEntries();
            }
        }

        break;

    default:

        /* Return error code. */
        status = SNMP_GENERROR;
    }

    /* Return success or error code. */
    return (status);

} /* snmpTargetAddrEntry */

/*************************************************************************
*
*   FUNCTION
*
*       Get_snmpTargetParamsEntry
*
*   DESCRIPTION
*
*       This function is used to process SNMP GET/GET-NEXT request on
*       snmpTargetParamsTable.
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
STATIC UINT16 Get_snmpTargetParamsEntry(snmp_object_t *obj, UINT8 getflag)
{
    /* Object identifier of the table entry. */
    UINT32                      table_oid[] = {1, 3, 6, 1, 6, 3, 12, 1, 3,
                                               1};

    /* Handle to the target param table entry. */
    SNMP_TARGET_PARAMS_TABLE    *target_param_entry = NU_NULL;

    /* Variable to use in for-loop. */
    UINT32                      i;

    /* status to return success or error code. */
    UINT16                      status = SNMP_NOERROR;

    /* Target param name. */
    UINT8                       target_param[MAX_TARG_NAME_SZE];

    /* Clear out target param name. */
    UTL_Zero(target_param, sizeof(target_param));

    for (i = 0;
         ((i < MAX_TARG_NAME_SZE) &&
         ((SNMP_TARGET_PARAM_SUB_LEN + i) < obj->IdLen));
         i++)
    {
        target_param[i] = (UINT8)(obj->Id[SNMP_TARGET_PARAM_SUB_LEN + i]);
    }

    /* If we are handling GET request. */
    if (getflag)
    {
        /* If we don't have valid object identifier then return error
         * code.
         */
        if ((i != strlen((CHAR *)target_param)) ||
            (obj->IdLen != (SNMP_TARGET_PARAM_SUB_LEN + i)))
        {
            /* Return error code. */
            status = SNMP_NOSUCHINSTANCE;
        }
    }

    /* If no error till now then proceed. */
    if (status == SNMP_NOERROR)
    {
        /* Get the handle to the target param table entry. */
        target_param_entry = Get_Target_Param_Entry(target_param,
                                                    getflag);

        /* If we did not get the handle to the target param table entry
         * then return error code.
         */
        if (!target_param_entry)
        {
            /* Return error code. */
            status = SNMP_NOSUCHINSTANCE;
        }
    }

    /* If we got the handle to the target param table entry then proceed. */
    if (target_param_entry)
    {
        switch(obj->Id[SNMP_TARGET_PARAM_ATTR_OFFSET])
        {
        case 2:                     /* snmpTargetParamsMPModel */

            /* Get the value of 'snmpTargetParamsMPModel'. */
            obj->Syntax.LngUns =
                        target_param_entry->snmp_target_params_mp_model;

            break;

        case 3:                     /* snmpTargetParamsSecurityModel */

            /* Get the value of 'snmpTargetParamsSecurityModel'. */
            obj->Syntax.LngUns =
                    target_param_entry->snmp_target_params_security_model;

            break;

        case 4:                     /* snmpTargetParamsSecurityName */

            /* Get the length of 'snmpTargetParamsSecurityName'. */
            obj->SyntaxLen = strlen(target_param_entry->
                                        snmp_target_params_security_name);

            /* Get the value of 'snmpTargetParamsSecurityName'. */
            NU_BLOCK_COPY(obj->Syntax.BufChr,
                     target_param_entry->snmp_target_params_security_name,
                     obj->SyntaxLen);

            break;

        case 5:                     /* snmpTargetParamsSecurityLevel */

            /* Get the value of 'snmpTargetParamsSecurityLevel'. */
            obj->Syntax.LngUns = (UINT32)target_param_entry->
                                        snmp_target_params_security_level;

            break;

        case 6:                     /* snmpTargetParamsStorageType */

            /* Get the value of 'snmpTargetParamsStorageType'. */
            obj->Syntax.LngUns = (UINT32)(target_param_entry->
                                          snmp_target_params_storage_type);

            break;

        case 7:                     /* snmpTargetParamsRowStatus */

            /* Get the value of 'snmpTargetParamsRowStatus'. */
            obj->Syntax.LngUns = (UINT32)(target_param_entry->
                                           snmp_target_params_row_status);

            break;

        default:
            /* We have reached at end of the table. */

            /* Return error code. */
            status = SNMP_NOSUCHNAME;
        }

        /* If we have successfully process the request and we are
         * handling get-next request then update the object identifier.
         */
        if ((status == SNMP_NOERROR) && (!getflag))
        {
            /* Update the table entry in object identifier. */
            NU_BLOCK_COPY(obj->Id, table_oid, sizeof(table_oid));

            /* Update target param name in object identifier. */
            for (i = 0; target_param_entry->snmp_target_params_name[i];
                 i++)
            {
                obj->Id[SNMP_TARGET_PARAM_SUB_LEN + i] =
                 (UINT32)(target_param_entry->snmp_target_params_name[i]);
            }

            /* Update object identifier length. */
            obj->IdLen = (SNMP_TARGET_PARAM_SUB_LEN + i);
        }
    }

    /* Return success or error code. */
    return (status);

} /* Get_snmpTargetParamsEntry */

/*************************************************************************
*
*   FUNCTION
*
*       snmpTargetParamsEntry
*
*   DESCRIPTION
*
*       This function is used to process SNMP request on
*       snmpTargetParamsTable.
*
*   INPUTS
*
*       *obj                    SNMP object containing request.
*       idlen                   Unused.
*       *param                  Unused.
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
UINT16 snmpTargetParamsEntry(snmp_object_t *obj, UINT16 idlen,
                             VOID *param)
{
    /* Status to return success or error code. */
    UINT16              status = SNMP_NOERROR;

    /* Flag to distinguish GET and GET-NEXT request. */
    UINT8               getflag = 0;

    /* Handle to the target params entry. */
    SNMP_TARGET_PARAMS_TABLE   *target_entry;

    /* Handle to the temporary target params entry. */
    SNMP_TARGET_PARAMS_TABLE   *temp_target_entry;

    /* Variable to hold target params name. */
    UINT8                       target_params[MAX_TARG_NAME_SZE];

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
    UNUSED_PARAMETER(idlen);
    UNUSED_PARAMETER(param);

    switch(obj->Request)
    {
    case SNMP_PDU_GET:                  /* GET request. */
        /* Set flag to represent GET request. */
        getflag++;

        /* Fall through next case to fulfill GET request. */

    case SNMP_PDU_NEXT:                 /* GET-NEXT request. */

        /* Process GET or GET-NEXT request. */
        status = Get_snmpTargetParamsEntry(obj, getflag);

        break;

    case SNMP_PDU_BULK:                 /* GET-BULK request. */

        /* Process GET-BULK request. */
        status = SNMP_Get_Bulk(obj, Get_snmpTargetParamsEntry);

        break;

    case SNMP_PDU_SET:                  /* SET request. */

        Target_Params_MIB_Commit_Left++;

        /* Process the SET operation. */
        status = Set_snmpTargetParamsEntry(obj);

        if (status != SNMP_NOSUCHINSTANCE)
            break;

    case SNMP_PDU_CREATE:               /* CREATE request. */

        /* Processing of create operation. */
        status = Create_snmpTargetParamsEntry(obj);

        /* If the entry was successfully created, set the value. */
        if(status == SNMP_NOERROR)
            status = Set_snmpTargetParamsEntry(obj);

        break;

    case SNMP_PDU_UNDO:                 /* UNDO request. */

        Target_Params_MIB_Commit_Left = 0;

        /* Processing of UNDO operations. */
        status = Undo_snmpTargetParamsEntry(obj);

        break;

    case SNMP_PDU_COMMIT:               /* COMMIT request. */

        Target_Params_MIB_Commit_Left--;

        /* Clear out target params name. */
        UTL_Zero(target_params, sizeof(target_params));

        /* Get the value of target params name. */
        for (i = 0;
            (i < (MAX_TARG_NAME_SZE - 1)) &&
            (SNMP_TARGET_PARAM_SUB_LEN + i < obj->IdLen);
        i++)
        {
            target_params[i] = (UINT8)(obj->Id[SNMP_TARGET_PARAM_SUB_LEN + i]);
        }

        /* If we don't have valid object identifier then return error code. */
        if ((obj->IdLen != (SNMP_TARGET_PARAM_SUB_LEN + i)) ||
            (strlen((CHAR *)target_params) != i))
        {
            /* Return error code. */
            status = SNMP_NOSUCHINSTANCE;
        }

        /* If no error till now then proceed. */
        if (status == NU_SUCCESS)
        {
            /* Get the handle to the target params table entry from 
             * temporary list.
             */
            target_entry = Get_Target_Params_Entry_Util(target_params,
                                             &Temp_Target_Param_Mib_Root);

            /* Save the handle in temporary one. */
            temp_target_entry = target_entry;

            /* If we did not get handle to the Target params table from
             * the temporary list then get it from the permanent list.
             */
            if (!target_entry)
            {
                /* Get the handle from the permanent list. */
                target_entry = Get_Target_Params_Entry_Util(target_params,
                                  Snmp_Target_Mib.target_params_table.next);
            }

            /* If we got the handle either from permanent or temporary list.
             */
            if (target_entry)
            {
                /* Check whether it was a row set operation. */
                if (obj->Id[SNMP_TARGET_PARAM_ATTR_OFFSET] == 7)
                {
                    /* Update the value of current state.
                     *  0) New entry.
                     *  1) Not ready.
                     *  2) Not In service.
                     *  3) Active
                     */

                    /* If we have new entry the set current state to 0. */
                    if (temp_target_entry)
                    {
                        current_state = 0;
                    }

                    /* If we have new not ready entry then set current
                     * state to 1.
                     */
                    else if ((strlen((CHAR *)target_entry->
                        snmp_target_params_name) == 0))
                    {
                        current_state = 1;
                    }

                    /* If we have not in service entry then set current
                     * state to 2.
                     */
                    else if (target_entry->snmp_target_params_row_status !=
                        SNMP_ROW_ACTIVE)
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

                    /* If required state transition is allowed then commit
                     * the row status value.
                     */
                    else
                    {
                        /* Commit the row status value. */
                        status = Commit_snmpTargetParamsEntryStatus(
                            target_entry, (UINT8)(obj->Syntax.LngUns),
                            (UINT8)(temp_target_entry != NU_NULL) );
                    }
                }

                else
                {
                    status = Commit_snmpTargetParamsEntryStatus(
                        target_entry, 0, NU_FALSE);
                }

                /* If the row status is set to 'DESTROY'. */
                if (target_entry->snmp_target_params_row_status == 
                    SNMP_ROW_DESTROY)
                {
                    /* If handle was from temporary list then remove it
                     * from temporary list.
                     */
                    if (temp_target_entry)
                        DLL_Remove(&Temp_Target_Param_Mib_Root, target_entry);

                    /* If handle was from permanent list then remove it from
                     * permanent list.
                     */
                    else
                        DLL_Remove(&Snmp_Target_Mib.target_params_table,
                            target_entry);

                    /* Deallocate memory attained by this target params
                     * table structure.
                     */
                    if (NU_Deallocate_Memory(target_entry) !=
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
                (Target_Params_MIB_Commit_Left == 0))
            {
                Commit_snmpTargetParamsEntries();
            }
        }

        break;

    default:

        /* Return error code. */
        status = SNMP_GENERROR;
    }

    /* Return success or error code. */
    return (status);

} /* snmpTargetParamsEntry */

/*************************************************************************
*
*   FUNCTION
*
*       snmpUnavailableContexts
*
*   DESCRIPTION
*
*       This function is used to handle SNMP request on
*       'snmpUnavailableContexts'.
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
*       SNMP_READONLY           Read only.
*
*************************************************************************/
UINT16 snmpUnavailableContexts(snmp_object_t *obj, UINT16 idlen,
                               VOID *param)
{
    /* Status to return success or error code. */
    UINT16              status;

    /* Avoid compilation warnings. */
    UNUSED_PARAMETER(param);

    /* If we have valid request. */
    if (MibSimple(obj, idlen) == NU_TRUE)
    {
        /* If we have GET, GET-NEXT or GET-BULK request then proceed. */
        if ((obj->Request == SNMP_PDU_GET) ||
                 (obj->Request == SNMP_PDU_NEXT) ||
                 (obj->Request == SNMP_PDU_BULK))
        {
            /* Get the value of 'snmpUnavailableContexts'. */
            obj->Syntax.LngUns =
                        Snmp_Target_Mib.snmp_unavailable_contexts;

            /* Return success code. */
            status = SNMP_NOERROR;
        }
        else
        {
            /* Return error code. */
            status = SNMP_READONLY;
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

} /* snmpUnavailableContexts */

/*************************************************************************
*
*   FUNCTION
*
*       snmpUnknownContexts
*
*   DESCRIPTION
*
*       This function is used to handle SNMP request on
*       'snmpUnknownContexts'.
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
*       SNMP_READONLY           Read only.
*
*************************************************************************/
UINT16 snmpUnknownContexts(snmp_object_t *obj, UINT16 idlen, VOID *param)
{
    /* Status to return success or error code. */
    UINT16              status;

    /* Avoid compilation warnings. */
    UNUSED_PARAMETER(param);

    /* If we have valid request. */
    if (MibSimple(obj, idlen) == NU_TRUE)
    {
        /* If we have GET, GET-NEXT or GET-BULK request then proceed. */
        if ((obj->Request == SNMP_PDU_GET) ||
                 (obj->Request == SNMP_PDU_NEXT) ||
                 (obj->Request == SNMP_PDU_BULK))
        {
            /* Get the value of 'snmpUnknownContexts'. */
            obj->Syntax.LngUns = Snmp_Target_Mib.snmp_unknown_contexts;

            /* Return success code. */
            status = SNMP_NOERROR;
        }
        else
        {
            /* Return error code. */
            status = SNMP_READONLY;
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

} /* snmpUnknownContexts */

#endif /* (INCLUDE_MIB_TARGET == NU_TRUE) */
#endif /* ((INCLUDE_SNMPv1 == NU_TRUE) || (INCLUDE_SNMPv2 == NU_TRUE)) */
