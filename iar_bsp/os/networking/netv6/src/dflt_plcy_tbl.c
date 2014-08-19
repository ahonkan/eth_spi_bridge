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
*   FILENAME
*
*       dflt_plcy_tbl.c
*                                                                                  
*   DESCRIPTION                                                              
*                
*       This file contains the Default Policy Table entries for IPv6.  
*       The user can add, delete or modify values from this table at 
*       compile-time to configure the default Policy Table.  This table 
*       can also be modified at run-time via the API routine 
*       NU_Configure_Policy_Table().
*                                                                          
*   DATA STRUCTURES                                                          
*                                      
*       None.                                    
*                                                                          
*   FUNCTIONS                                                                
*           
*       IP6_Find_Policy_For_Address
*       IP6_Add_Policy
*
*   DEPENDENCIES                                                             
*               
*       nu_net.h
*       nu_net6.h
*       in6.h
*                                                                          
*************************************************************************/

#include "networking/nu_net.h"
#include "networking/nu_net6.h"
#include "networking/in6.h"

/* This is the Default Policy Table per RFC 3484 used for default address
 * selection for IPv6 when selecting source and destination addresses.  
 * This default table gives preference to IPv6 addresses over IPv4 addresses.
 * The user can change the system to prefer IPv4 addresses by giving the
 * prefix ::ffff:0:0/96 a higher precedence value.  The user may add,
 * remove or modify any entries in this table at compile-time to invoke
 * desired behaviour in address selection.
 */
IP6_POLICY_ENTRY IP6_Default_Policy_Entries[] =
{   
    {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}, 128, 50, 0},
    {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0, 40, 1},
    {{0x20, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 16, 30, 2},
    {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 96, 20, 3},
    {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 0, 0, 0, 0}, 96, 10, 4},
    {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0, 0, 0},
};

extern IP6_POLICY_TABLE_STRUCT     IP6_Policy_Table;

/****************************************************************************
*                                                                          
*   FUNCTION                                                                 
*                                                                          
*       IP6_Find_Policy_For_Address                                                        
*                                                                          
*   DESCRIPTION                                                              
*                                                                          
*       This routine returns the policy associated with the input address
*       that has the longest matching prefix with the address.                                
*                                                                          
*   INPUTS                                                                   
*                                                                          
*       *addr_ptr               A pointer to the address for which to find
*                               a matching policy.
*                                                                          
*   OUTPUTS                                                                  
*                                                                          
*       A pointer to the matching policy or NU_NULL if no policy was found.
*                                                                          
****************************************************************************/
IP6_POLICY_TABLE_ENTRY* IP6_Find_Policy_For_Address(UINT8 *addr_ptr)
{
    IP6_POLICY_TABLE_ENTRY  *policy_ptr;

    /* Get a pointer to the first policy in the table. */
    policy_ptr = IP6_Policy_Table.head;

    /* Find the longest matching policy. */
    while (policy_ptr)
    {
        /* If the policy covers this address, return this policy. */
        if (in6_matchlen(addr_ptr, policy_ptr->policy.prefix) >= 
            policy_ptr->policy.prefix_len)
        {
            break;
        }

        /* Get a pointer to the next policy. */
        policy_ptr = policy_ptr->next;
    }

    return (policy_ptr);

} /* IP6_Find_Policy_For_Address */

/****************************************************************************
*                                                                          
*   FUNCTION                                                                 
*                                                                          
*       IP6_Add_Policy                                                        
*                                                                          
*   DESCRIPTION                                                              
*                                                                          
*       This routine adds a new policy to the Policy Table.                                
*                                                                          
*   INPUTS                                                                   
*                                                                          
*       *new_policy             A pointer to the new policy to add to the
*                               Policy Table.
*                                                                          
*   OUTPUTS                                                                  
*                                                                          
*       NU_SUCCESS              The entry was successfully added.
*       NU_NO_MEMORY            An error occurred when allocating memory
*                               for the new entry.
*                                                                          
****************************************************************************/
STATUS IP6_Add_Policy(IP6_POLICY_ENTRY *new_policy)
{
    STATUS                  status;
    IP6_POLICY_TABLE_ENTRY  *plcy_ptr, *current_plcy;

    /* Allocate memory for the new policy. */
    status = NU_Allocate_Memory(MEM_Cached, (VOID **)(&plcy_ptr),
                                sizeof(IP6_POLICY_TABLE_ENTRY), 
                                NU_NO_SUSPEND);

    /* Allocate memory for the new entry and add it to the table in
     * decreasing order of prefix length.  Keeping the table sorted
     * will allow for faster longest-matching look ups at run-time.
     */
    if (status == NU_SUCCESS)
    {
        /* Zero out the new entry. */
        memset(plcy_ptr, 0, sizeof(IP6_POLICY_TABLE_ENTRY));

        /* Copy the prefix. */
        memcpy(plcy_ptr->policy.prefix, new_policy->prefix, IP6_ADDR_LEN);

        /* Copy the prefix length, precedence and label. */
        plcy_ptr->policy.prefix_len = new_policy->prefix_len;
        plcy_ptr->policy.precedence = new_policy->precedence;
        plcy_ptr->policy.label = new_policy->label;

        /* If there are entries in the Policy Table, find the proper
         * position to insert the new policy.
         */
        if (IP6_Policy_Table.head)
        {
            /* Get a pointer to the first policy. */
            current_plcy = IP6_Policy_Table.head;

            /* Check each policy. */
            while (current_plcy)
            {
                /* If the new policy has a shorter prefix length than the
                 * current policy, continue to traverse the list.
                 */
                if (current_plcy->policy.prefix_len > 
                    plcy_ptr->policy.prefix_len)
                {
                    current_plcy = current_plcy->next;
                }

                /* Otherwise, insert the policy here. */
                else
                {
                    break;
                }
            }
        
            /* Insert the policy. */
            DLL_Insert(&IP6_Policy_Table, plcy_ptr, current_plcy);
        }

        /* If this is the first entry, add it to the table. */
        else
        {
            /* Add the policy to the table. */
            DLL_Enqueue(&IP6_Policy_Table, plcy_ptr);
        }
    }

    else
    {
        status = NU_NO_MEMORY;
    }

    return (status);

} /* IP6_Add_Policy */
