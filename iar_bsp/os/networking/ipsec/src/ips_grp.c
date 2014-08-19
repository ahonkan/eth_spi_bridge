/*************************************************************************
*
*               Copyright 2005 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       ips_grp.c
*
* COMPONENT
*
*       DATABASE - Groups
*
* DESCRIPTION
*
*       This file contains all the implementations details
*       regarding the IPsec groups.
*
* DATA STRUCTURES
*
*       IPS_Group_DB
*
* FUNCTIONS
*
*       IPSEC_Group_Init
*       IPSEC_Get_Group_Entry_Real
*       IPSEC_Cmp_Groups
*
* DEPENDENCIES
*
*       nu_net.h
*       ips_api.h
*
*************************************************************************/
/* Including the header files required. */
#include "networking/nu_net.h"
#include "networking/ips_api.h"

/* Declaring the global policy group database list. */
IPSEC_GROUP_DB              IPS_Group_DB;

/************************************************************************
*
* FUNCTION
*
*       IPSEC_Group_Init
*
* DESCRIPTION
*
*       This function initializes the group component of
*       Nucleus IPsec.
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       NU_SUCCESS              Initialization successfully done.
*
************************************************************************/
VOID IPSEC_Group_Init(VOID)
{
    /* Initializing the group list database of the IPsec. */
    IPS_Group_DB.head = NU_NULL;
    IPS_Group_DB.tail = NU_NULL;

    /* Return the success parameter. */
    return;
} /* IPSEC_Group_Init. */

/************************************************************************
*
* FUNCTION
*
*       IPSEC_Get_Group_Entry_Real
*
* DESCRIPTION
*
*       This is an internal function. It returns a pointer to
*       the group corresponding to the passed group name.
*
* INPUTS
*
*       *group_db               Pointer to a Group DB.
*       *group_name             Name of the group.
*       **ret_group             Pointer to that group.
*
* OUTPUTS
*
*       NU_SUCCESS              In case of success.
*       IPSEC_NOT_FOUND         Group not found.
*       IPSEC_INVALID_PARAMS    Parameter is invalid.
*
************************************************************************/
STATUS IPSEC_Get_Group_Entry_Real(IPSEC_GROUP_DB *group_db,
                                  CHAR *group_name,
                                  IPSEC_POLICY_GROUP **ret_group)
{
    STATUS              status = IPSEC_NOT_FOUND;
    INT                 result;
    IPSEC_POLICY_GROUP  *group_list;

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if((group_db == NU_NULL) || (group_name == NU_NULL) ||
                                (ret_group  == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }

#endif

    /* Getting pointer to the group list. */
    group_list = group_db->head;

    /* Loop until desired group is found. */
    while(group_list != NU_NULL)
    {
        /* Compare the group names and break on the match. */
        result = strcmp(group_name, group_list->ipsec_group_name);

        /* If they are equal. */
        if(result == 0)
        {
            /* Mark the status as success. */
            status = NU_SUCCESS;
            break;
        }
        else
        {
            /* If the given group name is less then the current group
               name in the list then no need to traverse further. */
            if(result < 0)
            {
                /* Just break the loop. */
                break;
            }
            else
            {
                /* Point to the next group in the list. */
                group_list = group_list->ipsec_flink;
            }
        }
    }

    /* Now return the group entry. */
    *ret_group = group_list;

    /* Return the status value. */
    return (status);

} /* IPSEC_Get_Group_Entry_Real. */

/************************************************************************
*
* FUNCTION
*
*       IPSEC_Cmp_Groups
*
* DESCRIPTION
*
*       This is an internal utility function used for comparison
*       for adding sorted items to the Groups database. This
*       is a callback function and would never be called directly.
*       It is called by SLL_Insert_Sorted.
*
* INPUTS
*
*       *a                      Pointer to the first node of type
*                               IKE_POLICY_GROUP node.
*       *b                      Pointer to the second node of type
*                               IKE_POLICY_GROUP node.
*
* OUTPUTS
*
*       -1                      If a < b.
*       1                       If a > b.
*       0                       If a == b.
*       IPSEC_INVALID_PARAMS    Invalid parameter(s).
*
************************************************************************/
INT IPSEC_Cmp_Groups(VOID *a, VOID *b)
{

#if (IPSEC_DEBUG == NU_TRUE)

    /* Validate input parameters. */
    if((a == NU_NULL) || (b == NU_NULL))
    {
        return (IPSEC_INVALID_PARAMS);
    }

#endif

    /* Now compare the two names and return the result. */
    return (strcmp(((IPSEC_POLICY_GROUP *)a)->ipsec_group_name,
                  ((IPSEC_POLICY_GROUP *)b)->ipsec_group_name));

} /* IPSEC_Cmp_Groups. */
