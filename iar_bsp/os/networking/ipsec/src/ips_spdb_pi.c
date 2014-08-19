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
*       ips_spdb_pi.c
*
* COMPONENT
*
*       DATABASE - Security Policy
*
* DESCRIPTION
*
*       This file contains the implementations of the function
*       IPSEC_Priority_Insert, which inserts a policy according to its
*       specified priority.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       IPSEC_Priority_Insert
*
* DEPENDENCIES
*
*       nu_net.h
*       ips_externs.h
*
*************************************************************************/
/* Including the required header files. */
#include "networking/nu_net.h"
#include "networking/ips_externs.h"
#include "networking/ips_api.h"

#if ( IPSEC_ENABLE_PRIORITY == NU_TRUE )
/************************************************************************
*
* FUNCTION
*
*       IPSEC_Priority_Insert
*
* DESCRIPTION
*
*       This function inserts a new policy according to its priority.
*
* INPUTS
*
*       *policy_list            List of policies.
*       *new_policy             The new policy to be inserted.
*
* OUTPUTS
*
*       None.
*
************************************************************************/
VOID IPSEC_Priority_Insert ( IPSEC_SPDB *policy_list,
                             IPSEC_POLICY *new_policy )
{
    IPSEC_POLICY        *position_ptr;

    /* If we need to insert this policy at the head. This happens, when
     * the list is empty or we have a new policy with the highest priority.
     */
    if ( ( policy_list->ipsec_head == NU_NULL ) ||
         ( new_policy->ipsec_priority <
           policy_list->ipsec_head->ipsec_priority ) )
    {
        /* Insert at the root. */
        new_policy->ipsec_flink = policy_list->ipsec_head;
        policy_list->ipsec_head = new_policy;
    }

    else
    {
        position_ptr = policy_list->ipsec_head;

        /* Find the position where we need to insert this new policy. */
        while ( ( position_ptr->ipsec_flink != NU_NULL ) &&
                ( new_policy->ipsec_priority >=
                  position_ptr->ipsec_flink->ipsec_priority ) )
        {
            position_ptr = position_ptr->ipsec_flink;
        }

        /* Insert the policy. */
        new_policy->ipsec_flink = position_ptr->ipsec_flink;
        position_ptr->ipsec_flink = new_policy;
    }

} /* IPSEC_Priority_Insert */

#endif /* ( IPSEC_ENABLE_PRIORITY == NU_TRUE ) */


