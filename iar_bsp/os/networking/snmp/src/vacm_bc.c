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
*       vacm_bc.c                                                
*
*   DESCRIPTION
*
*       This file contains functions from VACM that have been left for
*       backward compatibility.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       VACM_Add_Group
*       VACM_Add_AccessEntry
*       VACM_Add_MibView
*
*   DEPENDENCIES
*
*       nu_net.h
*       snmp_cfg.h
*       xtypes.h
*       vacm.h
*
*************************************************************************/
#include "networking/nu_net.h"
#include "networking/snmp_cfg.h"
#include "networking/xtypes.h"
#include "networking/vacm.h"

/************************************************************************
*
*   FUNCTION
*
*       VACM_Add_Group
*
*   DESCRIPTION
*
*       Adds a group entry to the group table.
*
*   INPUTS
*
*       snmp_sm                 Security model.
*       *security_name          Security name.
*       *group_name             Group name.
*       storage_type            Storage type.
*       row_status              Row status.
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation successful.
*       SNMP_ERROR              Operation failed.
*
*************************************************************************/
STATUS VACM_Add_Group (UINT32 snmp_sm, UINT8 *security_name,
                       UINT8 *group_name, UINT8 storage_type,
                       UINT8 row_status)
{
    STATUS                  status;
    VACM_SEC2GROUP_STRUCT   node;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

#if (INCLUDE_MIB_VACM == NU_FALSE)

    UNUSED_PARAMETER(storage_type);
    UNUSED_PARAMETER(row_status);

#endif

    /* Make sure that security name and group name do not exceed the
     * maximum length.
     */
    if((strlen((CHAR *)security_name) >= SNMP_SIZE_SMALLOBJECTID) ||
       (strlen((CHAR *)group_name) >= SNMP_SIZE_SMALLOBJECTID))
    {
        status = SNMP_ERROR;
    }
    else
    {
        /* Clear the structure. */
        UTL_Zero(&node, sizeof(VACM_SEC2GROUP_STRUCT));

        /* Fill in the structure. */
        node.vacm_security_model = snmp_sm;
        strcpy((CHAR *)node.vacm_security_name, (CHAR *)security_name);
        strcpy((CHAR *)node.vacm_group_name, (CHAR *)group_name);

#if (INCLUDE_MIB_VACM == NU_TRUE)
        node.vacm_storage_type   = storage_type;
        node.vacm_status         = row_status;

        if((node.vacm_status == SNMP_ROW_CREATEANDGO) ||
           (node.vacm_status == SNMP_ROW_CREATEANDWAIT))
        {
            /* The row is being created. Set the row flag accordingly. */
            node.vacm_row_flag = SNMP_PDU_CREATE;
        }
#endif

        /* Calling the function to insert node. */
        status = VACM_InsertGroup(&node);
    }

    /* return to user mode */
    NU_USER_MODE();
    return (status);

} /* VACM_Add_Group */

/************************************************************************
*
*   FUNCTION
*
*       VACM_Add_AccessEntry
*
*   DESCRIPTION
*
*       Adds an access entry to the list of access entries.
*
*   INPUTS
*
*       *group_name             Group name.
*       *context_prefix         Context prefix.
*       snmp_sm                 Security model.
*       security_level          Security level.
*       contextMatch            Context match.
*       *read_view              Read view name.
*       *write_view             Write view name.
*       *notify_view            Notify view name.
*       storage_type            Storage type.
*       row_status              Row status.
*
*   OUTPUTS
*
*       NU_SUCCESS              Operation successful.
*       SNMP_ERROR              Operation failed.
*
************************************************************************/
STATUS VACM_Add_AccessEntry(UINT8 *group_name, UINT8 *context_prefix,
                            UINT32 snmp_sm, UINT8 security_level,
                            UINT8 contextMatch, UINT8 *read_view,
                            UINT8 *write_view, UINT8 *notify_view,
                            UINT8 storage_type, UINT8 row_status)
{

    STATUS                  status;
    VACM_ACCESS_STRUCT      node;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

#if (INCLUDE_MIB_VACM == NU_FALSE)

    UNUSED_PARAMETER(storage_type);
    UNUSED_PARAMETER(row_status);

#endif

    /* Verify that none of the lengths are greater than their respective
     * buffers.
     */
    if( (strlen((CHAR *)group_name) >= SNMP_SIZE_SMALLOBJECTID)     ||
        (strlen((CHAR *)context_prefix) >= SNMP_SIZE_SMALLOBJECTID) ||
        (strlen((CHAR *)read_view) >= SNMP_SIZE_SMALLOBJECTID)      ||
        (strlen((CHAR *)write_view) >= SNMP_SIZE_SMALLOBJECTID)     ||
        (strlen((CHAR *)notify_view) >= SNMP_SIZE_SMALLOBJECTID)
    )
    {
        status = SNMP_ERROR;
    }
    else
    {
        /* Clear the node. */
        UTL_Zero(&node, sizeof(VACM_ACCESS_STRUCT));

        /* Fill the node. */
        strcpy((CHAR *)node.vacm_group_name,(CHAR *)group_name);
        strcpy((CHAR *)node.vacm_context_prefix,(CHAR *)context_prefix);
        node.vacm_security_model = snmp_sm;
        node.vacm_security_level = security_level;
        node.vacm_context_match  = contextMatch;
        strcpy((CHAR *)node.vacm_read_view,(CHAR *)read_view);
        strcpy((CHAR *)node.vacm_write_view,(CHAR *)write_view);
        strcpy((CHAR *)node.vacm_notify_view,(CHAR *)notify_view);

#if (INCLUDE_MIB_VACM == NU_TRUE)
        node.vacm_storage_type   = storage_type;
        node.vacm_status         = row_status;

        if((node.vacm_status == SNMP_ROW_CREATEANDGO) ||
           (node.vacm_status == SNMP_ROW_CREATEANDWAIT))
        {
            /* The row is being created. Set the row flag accordingly. */
            node.vacm_row_flag = SNMP_PDU_CREATE;
        }
#endif

        /* Calling the function to insert node. */
        status = VACM_InsertAccessEntry(&node);
    }

    /* return to user mode */
    NU_USER_MODE();
    return (status);

} /* VACM_Add_AccessEntry */

/************************************************************************
*
*   FUNCTION
*
*       VACM_Add_MibView
*
*   DESCRIPTION
*
*       Adds an MIB view to the MIB view list.
*
*   INPUTS
*
*       *view_name          View name.
*       *subtree            Subtree.
*       subtree_len         Length of subtree.
*       familyType          Family type.
*       *family_mask        Family mask.
*       mask_len            Mask length.
*       storage_type        Storage Type.
*       row_status          Row status.
*   OUTPUTS
*
*       NU_SUCCESS          Operation successful.
*       SNMP_ERROR          Operation failed.
*
*************************************************************************/
STATUS VACM_Add_MibView (UINT8 *view_name, UINT32 *subtree,
                         UINT32 subtree_len, UINT8 familyType,
                         UINT8 *family_mask, UINT32 mask_len,
                         UINT8 storage_type, UINT8 row_status)
{

    STATUS                  status;
    VACM_VIEWTREE_STRUCT    node;

    NU_SUPERV_USER_VARIABLES
    NU_SUPERVISOR_MODE();    /* switch to supervisor mode */

#if (INCLUDE_MIB_VACM == NU_FALSE)

    UNUSED_PARAMETER(storage_type);
    UNUSED_PARAMETER(row_status);

#endif

    /* Verify that the lengths are within range allowed. */
    if( (strlen((CHAR *)view_name) >= SNMP_SIZE_SMALLOBJECTID) ||
        (mask_len > VACM_MASK_SIZE))
    {
        status = SNMP_ERROR;
    }
    else
    {
        /* Clear the structure. */
        UTL_Zero(&node, (sizeof(VACM_VIEWTREE_STRUCT)));

        /* Fill in the structure. */
        strcpy((CHAR *)node.vacm_view_name,(CHAR *)view_name);
        NU_BLOCK_COPY(node.vacm_subtree, subtree,
                      sizeof(UINT32)* subtree_len);
        node.vacm_subtree_len  = (UINT16)subtree_len;
        node.vacm_family_type  = familyType;

        if(family_mask != NU_NULL)
        {
            NU_BLOCK_COPY(node.vacm_family_mask, family_mask, mask_len);
        }

        node.vacm_mask_length  = mask_len;

#if (INCLUDE_MIB_VACM == NU_TRUE)
        node.vacm_storage_type   = storage_type;
        node.vacm_status         = row_status;

        if((node.vacm_status == SNMP_ROW_CREATEANDGO) ||
           (node.vacm_status == SNMP_ROW_CREATEANDWAIT))
        {
            /* The row is being created. Set the row flag accordingly. */
            node.vacm_row_flag = SNMP_PDU_CREATE;
        }
#endif
        /* Calling the function to fill the family mask with ONES. */
        VACM_Filling_FamilyMask(node.vacm_family_mask, mask_len);

        /* Calling the function to insert node. */
        status = VACM_InsertMibView(&node);
    }

    /* return to user mode */
    NU_USER_MODE();
    return (status);

} /* VACM_Add_MibView */


