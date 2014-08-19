/*************************************************************************
*
*              Copyright 2002 Mentor Graphics Corporation              
*                         All Rights Reserved.                          
*                                                                       
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   
* SUBJECT TO LICENSE TERMS.                                             
*
*************************************************************************/

/*************************************************************************
*
*   FILE NAME
*
*       sck6_cpt.c
*
*   DESCRIPTION
*
*       This file contains the routine that configures the Policy Table
*       for IPv6 source and destination address selection as defined in
*       RFC 3484.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Configure_Policy_Table
*       IP6_Get_Policy_Entry
*
*   DEPENDENCIES
*
*       nu_net.h
*       nu_net6.h
*       in6.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/nu_net6.h"
#include "networking/in6.h"

extern IP6_POLICY_TABLE_STRUCT  IP6_Policy_Table;

STATIC IP6_POLICY_TABLE_ENTRY *IP6_Get_Policy_Entry(IP6_POLICY_ENTRY *);

/*************************************************************************
*                                                                         
*   FUNCTION                                                              
*                                                                         
*       NU_Configure_Policy_Table                                                     
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function configures the policies in the Policy Table
*       per RFC 3484.
*                                                                         
*   INPUTS                                                                
*                           
*       *plcy_ptr               A pointer to the policy.  The prefix and
*                               prefix length fields must always be 
*                               specified.  The precedence and label 
*                               fields are required only when adding or 
*                               updating a policy.  Note that the prefix
*                               and prefix length cannot be changed once
*                               the policy is added since these two fields
*                               are used to identify the policy.  To change
*                               either of these fields, the user must first
*                               delete the policy, then add a new policy
*                               with the desired values.
*       action                  The action to perform on the policy.
*                               Valid values are; IP6_ADD_POLICY, 
*                               IP6_DELETE_POLICY, IP6_UPDATE_POLICY.                                              
*                                                                         
*   OUTPUTS                                                               
*                                                                         
*       NU_SUCCESS              Successful completion of the service.
*       NU_INVALID_PARM         One of the input parameters is invalid,
*                               a policy already exists for the prefix /
*                               prefix length combination the user is 
*                               trying to add, or the target policy to 
*                               update does not exist in the Policy Table.
*       NU_NO_MEMORY            There is insufficient memory in the system
*                               to add the new policy.
*                                                                         
*************************************************************************/
STATUS NU_Configure_Policy_Table(IP6_POLICY_ENTRY *plcy_ptr, UINT32 action)
{
    STATUS                  status; 
    IP6_POLICY_TABLE_ENTRY  *target_plcy;

    NU_SUPERV_USER_VARIABLES

    /* Validate the input parameters. */
    if ( (plcy_ptr != NU_NULL) && (action <= IP6_UPDATE_POLICY) )
    {
        /* Switch to supervisor mode. */
        NU_SUPERVISOR_MODE();

        /* Obtain the semaphore. */
        status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);
        
        /* If the semaphore was successfully obtained. */
        if (status == NU_SUCCESS)
        {
            /* Get a pointer to the target policy. */
            target_plcy = IP6_Get_Policy_Entry(plcy_ptr); 

            /* Delete a policy. */
            if (action == IP6_DELETE_POLICY)
            {        
                /* If the policy was found. */
                if (target_plcy)
                {
                    /* Remove the policy from the table. */
                    DLL_Remove(&IP6_Policy_Table, target_plcy);

                    /* Deallocate the memory for the policy. */
                    NU_Deallocate_Memory(target_plcy);
                }
    
                /* The policy does not exist in the table. */
                else
                {
                    status = NU_INVALID_PARM;
                }
            }

            /* Add a new policy. */
            else if (action == IP6_ADD_POLICY)
            {
                /* If a policy does not already exist for this prefix. */
                if (!target_plcy)
                {
                    status = IP6_Add_Policy(plcy_ptr);
                }
    
                /* A policy already exists. */
                else
                {
                    status = NU_INVALID_PARM;
                }
            }

            /* Update an existing policy. */
            else
            {   
                /* If the policy was found. */
                if (target_plcy)
                {
                    /* Update the Precedence. */
                    target_plcy->policy.precedence = plcy_ptr->precedence;

                    /* Update the label. */
                    target_plcy->policy.label = plcy_ptr->label;
                }
    
                /* The policy does not exist in the table. */
                else
                {
                    status = NU_INVALID_PARM;
                }
            }

            if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE, 
                               __FILE__, __LINE__);
        }

        /* Switch back to user mode. */
        NU_USER_MODE();
    }

    /* One of the input parameters is invalid. */
    else
    {
        status = NU_INVALID_PARM;
    }

    return (status);

} /* NU_Configure_Policy_Table */

/*************************************************************************
*                                                                         
*   FUNCTION                                                              
*                                                                         
*       IP6_Get_Policy_Entry                                                     
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function returns a pointer to the Policy Table entry
*       associated with the specified prefix / prefix length combination.  
*       The Policy Table is sorted by descending prefix length, so once 
*       a match is found, there is no need to check successive entries.
*                                                                         
*   INPUTS                                                                
*                           
*       *plcy_ptr               A pointer to the policy to find.                                             
*                                                                         
*   OUTPUTS                                                               
*                                                                         
*       A pointer to the requested policy or NU_NULL if the policy does
*       not exist in the table.
*                                                                         
*************************************************************************/
STATIC IP6_POLICY_TABLE_ENTRY *IP6_Get_Policy_Entry(IP6_POLICY_ENTRY *plcy_ptr)
{
    IP6_POLICY_TABLE_ENTRY  *current_plcy;

    /* Get a pointer to the first policy in the table. */
    current_plcy = IP6_Policy_Table.head; 
        
    /* Traverse the list of policies until the target policy is found. */
    while (current_plcy)
    {   
        /* If the prefix length and prefix matches, this is the target
         * policy. 
         */
        if ( (plcy_ptr->prefix_len == current_plcy->policy.prefix_len) &&
             (in6_matchlen(plcy_ptr->prefix, current_plcy->policy.prefix) >= 
              plcy_ptr->prefix_len) )
        {
            break;
        }

        /* Get a pointer to the next policy. */
        current_plcy = current_plcy->next;
    }

    return (current_plcy);

} /* IP6_Get_Policy_Entry */
