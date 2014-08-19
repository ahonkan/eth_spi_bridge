/************************************************************************
*
*               Copyright Mentor Graphics Corporation 2003              
*                         All Rights Reserved.                          
*                                                                       
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   
* SUBJECT TO LICENSE TERMS.                                             
*
**************************************************************************

**************************************************************************
* FILE NAME                                                           
*                                                                      
*     dhcpslse.c                                               
*                                                                      
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     This file contains the API functions of the DHCP Server that deal
*     with the binding leases.
*
* DATA STRUCTURES
*
*     None
*                                                                      
* FUNCTIONS                                                            
*
*     DHCPS_Add_IP_Range
*     DHCPS_Delete_IP_Range
*     DHCPS_Get_IP_Range
*     DHCPS_Set_Renewal_Time
*     DHCPS_Get_Renewal_Time
*     DHCPS_Set_Rebind_Time
*     DHCPS_Get_Rebind_Time
*     DHCPS_Set_Default_Lease_Time
*     DHCPS_Get_Default_Lease_Time
*     DHCPS_Set_Offered_Wait_Time
*     DHCPS_Get_Offered_Wait_Time
*                                                                      
* DEPENDENCIES                                                         
*
*     networking/nu_networking.h
*     os/networking/dhcps/inc/dhcps_ext.h
*     os/networking/dhcps/inc/dhcpsrv.h
*                                                                      
**********************************************************************************/

/* Includes */
#include "networking/nu_networking.h"
#include "os/networking/dhcp/server/inc/dhcps_ext.h"
#include "os/networking/dhcp/server/inc/dhcpsrv.h"

/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Add_IP_Range                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Adds a single or group of IP addresses to the specified configuration control
*     block.  If a static binding is desired, the client ID of the client that is
*     to receive the static binding must be declared.  Also, if only one IP address
*     is being added, the last passed in parameter must be set to NU_NULL.
*
* INPUTS                                                               
*                                                                      
*     DHCPS_CONFIG_CB *
*     DHCPS_CLIENT_ID *
*     struct id_struct *
*     struct id_struct *
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     STATUS
*                                                                      
**********************************************************************************/
STATUS DHCPS_Add_IP_Range (DHCPS_CONFIG_CB *config_cb, const DHCPS_CLIENT_ID *client_id, 
                            struct id_struct *ip_low, struct id_struct *ip_high)
{
    STATUS              ret_status = NU_INVALID_PARM;
    DHCPS_CONFIG_CB     *global_cb;
    DHCPS_BINDING       *temp_binding,
                        *search_binding;
    DHCPS_OFFERED       *offered;
    struct id_struct    subnet_low, 
                        subnet_high,
                        subnet_mask,
                        low_addr,
                        high_addr;
    UINT8               subnet_ok = NU_FALSE,
                        static_entry = NU_FALSE,
                        binding_ip_addr[IP_ADDR_LEN],
                        found;
    UINT16              i, 
                        class_b,
                        num_of_ip_addr;
    VOID                *dll_ptr;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Ensure that the control block pointer passed in is valid. */
    if (config_cb != NU_NULL)
    {
        /* Save off the low ip address to a local variable. */
        memcpy(low_addr.is_ip_addrs, ip_low->is_ip_addrs, IP_ADDR_LEN);

        /* If we are only adding one IP address, we need to test if this is a static entry. */
        if ((ip_high == NU_NULL) && client_id != NU_NULL)
        {
            /* This is a static entry.  Set flags to signify that this is a static entry. */
            static_entry = NU_TRUE;
            num_of_ip_addr = 1;

            /* Set the ip_high equal to the ip_low. */
            memcpy(high_addr.is_ip_addrs, ip_low->is_ip_addrs, IP_ADDR_LEN);
        }

        else if (ip_high == NU_NULL)
        {
            /* We are only adding a single IP address that is not a static entry. */
            num_of_ip_addr = 1;

            /* Set the ip_high equal to the ip_low. */
            memcpy(high_addr.is_ip_addrs, ip_low->is_ip_addrs, IP_ADDR_LEN);

        }    

        else
        {
            /* Save of the high ip address to a local variable */
            memcpy(high_addr.is_ip_addrs, ip_high->is_ip_addrs, IP_ADDR_LEN);

            /* First, we need to check to see if the subnet mask for the control block has
                been defined. */
            if (config_cb->subnet_mask_addr[0] != NU_NULL)
            {
                /* Copy the subnet mask into a id structure. */
                memcpy(subnet_mask.is_ip_addrs, config_cb->subnet_mask_addr, IP_ADDR_LEN);
            }

            /* We will need to use the global subnet mask for our subnet calculations. */
            else
            {
                /* Get a pointer to the global control block. */
                global_cb = DHCPS_Config_List.dhcp_config_head;

                /* Copy the subnet mask into a id structure. */
                memcpy(subnet_mask.is_ip_addrs, global_cb->subnet_mask_addr, IP_ADDR_LEN);
            }   

            /* Ensure that the specified range is valid.  This is done by verifying that the 
                IP addresses that define the range are on the same subnet. */
            ret_status = DHCPSDB_Calculate_Subnet_Number(&low_addr, &subnet_mask, &subnet_low);
            ret_status = DHCPSDB_Calculate_Subnet_Number(&high_addr, &subnet_mask, &subnet_high);

            /* Check to see if the two subnets are equal. */
            ret_status = DHCPSDB_IP_Compare(subnet_low.is_ip_addrs, subnet_high.is_ip_addrs);

            if (ret_status == NU_SUCCESS)
            {
                /* Set a variable that will let us konw that the range si all on the same 
                   subnet. */
                subnet_ok = NU_TRUE;
            } 

            /* Calculate the number of binding entries that will need to be added to the
                binding link list. */

            if (subnet_ok == NU_TRUE)
            {
                /* Determine if the number of addresses span across one Class C specification */
                if ((high_addr.is_ip_addrs[2] - low_addr.is_ip_addrs[2]) != 0)
                {
                    /*  Calculate the number of Class B addresses. */
                    class_b = high_addr.is_ip_addrs[2] - low_addr.is_ip_addrs[2];

                    /* The broadcast address needs to be omitted. */ 
                    class_b = (UINT16)(class_b * 254);

                    /* Get the total number of IP addresses to add. */
                    num_of_ip_addr = (class_b - low_addr.is_ip_addrs[3]) + (high_addr.is_ip_addrs[3] + 1);
                }    

                else
                {
                    /* Get the total number of IP addresses to add. */
                    num_of_ip_addr =  (high_addr.is_ip_addrs[3] - low_addr.is_ip_addrs[3]) + 1;
                }
            } 
            else
            {
                /* The stated range is invalid.  Do not add any IP addresses. */
                num_of_ip_addr = 0;
                ret_status = DHCPSERR_RANGE_SUBNET_INVALID;
            }    
        }

        /* Loop through the newly allocated memory block and store the IP range
            into the binding structures. */
        for (i = 0; i < num_of_ip_addr; i++)
        {
            found = NU_FALSE;

            /* Ensure that the IP address being added is not currently in the
                link list. If so, just skip it and continue. */
            for (search_binding = config_cb->ip_binding_list.dhcp_bind_head; 
                 search_binding;
                 search_binding = search_binding->dhcp_bind_next)            
            {
                /* Protect the global data structures. */
                NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                /* Save the binding IP address to a local variable. */
                memcpy(binding_ip_addr, search_binding->yiaddr.is_ip_addrs, IP_ADDR_LEN);

                /* Unprotect the global data structures. */
                NU_Release_Semaphore(&DHCPS_Semaphore);

                if (DHCPSDB_IP_Compare(binding_ip_addr, low_addr.is_ip_addrs)
                        == NU_SUCCESS)     
                {
                    /* If this is a static entry that is trying to be added, return a message
                        to the application stating that the entry was not added. */
                    if (static_entry)
                    {
                        found = NU_TRUE;
                        ret_status = DHCPSERR_BINDING_ALREADY_PRESENT;
                        break;
                    }    
                    /* This IP address is a duplicate entry.  Skip it and continue to the 
                        next IP address in the range. */
                    found = NU_TRUE;
                    break;    
                }
            }

            if (found != NU_TRUE)
            {
                /* Allocate a block of memory for the binding that will be added. */
                ret_status = NU_Allocate_Memory(DHCPServer_Memory, (VOID **)&temp_binding, 
                                    sizeof(DHCPS_BINDING), NU_SUSPEND);

                if (ret_status == NU_SUCCESS)
                {
                    /* Allocate a block of memory for the offered struct that will be added. */
                    ret_status = NU_Allocate_Memory(DHCPServer_Memory, (VOID **)&offered, 
                                        sizeof(DHCPS_OFFERED), NU_SUSPEND);

                    if (ret_status == NU_SUCCESS)
                    {
                        /* Initialize the offered sturcture. */
                        UTL_Zero(offered, sizeof(DHCPS_OFFERED));

                        /* Protect the global data structures. */
                        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                        /* Place the offered structure onto the link list. */
                        dll_ptr = DLL_Enqueue(&config_cb->ip_offer_list, offered);

                        /* Unprotect the global data structures. */
                        NU_Release_Semaphore(&DHCPS_Semaphore);

                        if (dll_ptr != NU_NULL)
                        {
                            /* Initialize the binding. */
                            UTL_Zero(temp_binding, sizeof(DHCPS_BINDING));

                            /* Copy the IP address into the binding structure. */
                            IP_ADDR_COPY(temp_binding->yiaddr.is_ip_addrs, low_addr.is_ip_addrs);

                            /* Place a pointer back to the configuration control block in the binding struct. */
                            temp_binding->config = config_cb;

                            /* If this is a static entry, (client hardware address has been passed in,
                                add the client hardware address to the binding and set the static
                                address flag. */
                            if ((static_entry == NU_TRUE) && (client_id != NU_NULL))
                            {
                                /* Copy the client's ID structure into the binding structure. */
                                memcpy(&temp_binding->client_id, client_id, sizeof(DHCPS_CLIENT_ID));

                                /* Set the static binding flag */
                                temp_binding->flags = temp_binding->flags | STATIC_ENTRY;

                                /* Protect the global data structures. */
                                NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                                /* Add this host to the front of the list. */
                                dll_ptr = DLL_Insert(&config_cb->ip_binding_list, temp_binding, 
                                                    config_cb->ip_binding_list.dhcp_bind_head);

                                /* Unprotect the global data structures. */
                                NU_Release_Semaphore(&DHCPS_Semaphore);
                                
                                if (dll_ptr == NU_NULL)
                                {
                                    /* An error occurred during inserting the binding into the link list. 
                                        deallocate the memory for the binding. */
                                    ret_status = NU_Deallocate_Memory(temp_binding);

                                    if (ret_status != NU_SUCCESS)
                                    {
                                        /* An error occurred while deallocating memory.  Log an error. */
                                        NERRS_Log_Error(NERR_FATAL, __FILE__, __LINE__);
                                    }

                                    /* Protect the global data structures. */
                                    NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                                    /* Deallocate the offered structure as well. */
                                    dll_ptr = DLL_Remove(&config_cb->ip_offer_list, offered);

                                    /* Unprotect the global data structures. */
                                    NU_Release_Semaphore(&DHCPS_Semaphore);

                                    if (dll_ptr != NU_NULL)
                                    {
                                        /* Deallocate the offered struct. */
                                        ret_status = NU_Deallocate_Memory(offered);

                                        if (ret_status != NU_SUCCESS)
                                        {
                                            /* An error occurred while deallocating memory.  Log an error. */
                                            NERRS_Log_Error(NERR_FATAL, __FILE__, __LINE__);
                                        }
                                    }    

                                    /* Set return status to return to the application. */
                                    ret_status = DHCPSERR_BINDING_NOT_ADDED;

                                    break;
                                }    
                            }

                            else
                            {
                                /* Protect the global data structures. */
                                NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                                /* Add this host to the list. */
                                dll_ptr = DLL_Enqueue(&config_cb->ip_binding_list, temp_binding);

                                /* Unprotect the global data structures. */
                                NU_Release_Semaphore(&DHCPS_Semaphore);

                                if (dll_ptr == NU_NULL)
                                {
                                    /* An error occurred during inserting the binding into the link list. 
                                        deallocate the memory for the binding. */
                                    ret_status = NU_Deallocate_Memory(temp_binding);

                                    if (ret_status != NU_SUCCESS)
                                    {
                                        /* An error occurred while deallocating memory.  Log an error. */
                                        NERRS_Log_Error(NERR_FATAL, __FILE__, __LINE__);
                                    }    

                                    /* Set return status to return to the application. */
                                    ret_status = DHCPSERR_BINDING_NOT_ADDED;

                                    break;
                                }    
                            }    
                        }

                        else
                        {
                            /* Since we were unable to add the offered structure to the link list,
                                we must deallocate both the offered structure and the binding 
                                structure. */
                            ret_status = NU_Deallocate_Memory(offered);

                            if (ret_status == NU_SUCCESS)
                            {
                                ret_status = NU_Deallocate_Memory(temp_binding);

                                if (ret_status != NU_SUCCESS)
                                {
                                    NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);    
                                } 
                            }

                            else
                            {
                                NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);    
                            }    

                            NERRS_Log_Error(NERR_RECOVERABLE, __FILE__, __LINE__);
                        }    
                    }
                    else
                    {
                        /* An error occurred during inserting the binding into the link list.
                            deallocate the memory for the binding. */
                        ret_status = NU_Deallocate_Memory(temp_binding);

                        if (ret_status != NU_SUCCESS)
                        {
                            /* An error occurred while deallocating memory.  Log an error. */
                            NERRS_Log_Error(NERR_FATAL, __FILE__, __LINE__);
                        }

                        /* Set return status to return to the application. */
                        ret_status = DHCPSERR_BINDING_NOT_ADDED;

                        /* Return the memory allocation error to the application. */
                        break;
                    }    
                }

                else
                {
                    /* Log an error message since we were not able to allocate memory. */
                    NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);

                    /* Return the memory allocation error to the application. */
                    break;
                }    
            }    
            
            if (ret_status == NU_SUCCESS)
            {
                /* Increment the ip_low address to the next address in the range. If the
                    increment rolls over to the next Class B address, ensure that the range
                    calls for this movement. Note:  there are 254 useable addresses in a 
                    Class C subnet. */
                if (low_addr.is_ip_addrs[3] == 254)
                {
                    /* The next higher octet needs to be incremented. */
                    low_addr.is_ip_addrs[2] = low_addr.is_ip_addrs[2] + 1;

                    /* Reset the lowest octet. */
                    low_addr.is_ip_addrs[3] = 1;
                }    

                else
                    /* Increment just the lowest octet. */
                    low_addr.is_ip_addrs[3] = low_addr.is_ip_addrs[3] + 1;
            }    
        }    
    }
    else
    {
        /* Return an error message that the control block pointer was incorrect. */
        ret_status = NU_INVALID_PARM;
    }
    /* Switch back to user mode */
    NU_USER_MODE();

    return(ret_status);
}                            

/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Delete_IP_Range                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Remove the declared IP range from the specified configuration control block. 
*
* INPUTS                                                               
*                                                                      
*     None.                                      
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     None.                                    
*                                                                      
**********************************************************************************/
STATUS DHCPS_Delete_IP_Range (DHCPS_CONFIG_CB *config_cb, struct id_struct *ip_low, 
                              const struct id_struct *ip_high)
{
    STATUS              ret_status;
    DHCPS_BINDING       *search_binding;
    DHCPS_OFFERED       *offered;

    DHCPS_CONFIG_CB     *global_cb;
    struct id_struct    subnet_low, 
                        subnet_high,
                        subnet_mask,
                        low_addr; 
    UINT8               found,
                        binding_ip_addr[IP_ADDR_LEN];
    UINT16              i, j,
                        num_of_ip_addr,
                        class_b;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Ensure that the control block pointer passed in is valid. */
    if (config_cb != NU_NULL)
    {
        /* Protect the global data structures. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

        /* First, we need to check to see if the subnet mask for the control block has
            been defined. */
        if (config_cb->subnet_mask_addr[0] != NU_NULL)  
        {
            /* Copy the subnet mask into a id structure. */
            memcpy(subnet_mask.is_ip_addrs, config_cb->subnet_mask_addr, IP_ADDR_LEN);
        }

        /* We will need to use the global subnet mask for our subnet calculations. */
        else
        {
            /* Get a pointer to the global control block. */
            global_cb = DHCPS_Config_List.dhcp_config_head;

            /* Copy the subnet mask into a id structure. */
            memcpy(subnet_mask.is_ip_addrs, global_cb->subnet_mask_addr, IP_ADDR_LEN);
        }   

        /* Unprotect the global data structures. */
        NU_Release_Semaphore(&DHCPS_Semaphore);

        /* Ensure that the specified range is valid.  This is done by verifying that the 
            IP addresses that define the range are on the same subnet. */
        ret_status  = DHCPSDB_Calculate_Subnet_Number(ip_low, &subnet_mask, &subnet_low);
        ret_status  = DHCPSDB_Calculate_Subnet_Number(ip_high, &subnet_mask, &subnet_high);

        /* Check to see if the two subnets are equal. */
        ret_status = DHCPSDB_IP_Compare(subnet_low.is_ip_addrs, subnet_high.is_ip_addrs);

        if (ret_status == NU_SUCCESS)
        {
            /* Calculate the number of binding entries that will need to be removed to the
                binding link list. */

            /* Determine if the number of addresses span across two or more Class C specifications */
            if ((ip_high->is_ip_addrs[2] - ip_low->is_ip_addrs[2]) != 0)
            {
                /*  Calculate the number of Class B addresses. */
                class_b = ip_high->is_ip_addrs[2] - ip_low->is_ip_addrs[2];

                /* The broadcast address needs to be omitted. */ 
                class_b = (UINT16)(class_b * 254);

                /* Get the total number of IP addresses to removed. */
                num_of_ip_addr = (class_b - ip_low->is_ip_addrs[3]) + (ip_high->is_ip_addrs[3] + 1);
            }    

            else
            {
                /* Get the total number of IP addresses to removed. */
                num_of_ip_addr =  1 + (ip_high->is_ip_addrs[3] - ip_low->is_ip_addrs[3]);
            }

            /* Save the low IP address to a local variable. */
            memcpy(low_addr.is_ip_addrs, ip_low->is_ip_addrs, IP_ADDR_LEN);

            /* Reset the incrementing variable. */
            j = 0;

            /* Walk the link list to find the specified IP address. */
            for (search_binding = config_cb->ip_binding_list.dhcp_bind_head; 
                 search_binding;
                 search_binding = search_binding->dhcp_bind_next)                                     
            {
                /* Protect the global data structures. */
                NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                /* Save the binding IP address to a local variable. */
                memcpy(binding_ip_addr, search_binding->yiaddr.is_ip_addrs, IP_ADDR_LEN);

                /* Unprotect the global data structures. */
                NU_Release_Semaphore(&DHCPS_Semaphore);

                if (DHCPSDB_IP_Compare(binding_ip_addr, low_addr.is_ip_addrs)
                        == NU_SUCCESS)     
                {
                    /* This IP address is a duplicate entry.  Skip it and continue to the 
                        next IP address in the range. */
                    j++;
                
                    /* Increment the ip_low address to the next address in the range. If the
                       increment rolls over to the next Class B address, ensure that the range
                        calls for this movement. Note:  there are 254 useable addresses in each
                        Class C subnet segment. */
                    if (low_addr.is_ip_addrs[3] == 254)
                    {
                        /* The next higher octet needs to be incremented. */
                        low_addr.is_ip_addrs[2] = low_addr.is_ip_addrs[2] + 1;

                        /* Reset the lowest octet. */
                        low_addr.is_ip_addrs[3] = 1;
                    }    

                    else
                    {
                        /* Look to see if we have looped through the range. */
                        if (low_addr.is_ip_addrs[3] == ip_high->is_ip_addrs[3])
                        {
                            /* We are done.  Break out. */
                            break;
                        }    

                        else
                        {
                            /* Increment just the lowest octet. */
                            low_addr.is_ip_addrs[3] = low_addr.is_ip_addrs[3] + 1;
                        }    
                    }    
                }    
            }

            /* If the entire range is valid, then remove the specified IP addresses. */
            if (num_of_ip_addr == j)
            {
                /* Reset the local variable to the low IP address. */
                memcpy(low_addr.is_ip_addrs, ip_low->is_ip_addrs, IP_ADDR_LEN);

                /* Loop through the IP range and remove each IP from the link list and
                    deallocate the binding structure. */
                for (i = 0; i < num_of_ip_addr; i++)
                {
                    found = NU_FALSE;

                    /* Walk the link list to find the specified IP address. */
                    for (search_binding = config_cb->ip_binding_list.dhcp_bind_head; 
                         search_binding;
                         search_binding = search_binding->dhcp_bind_next)                                         
                    {
                        /* Protect the global data structures. */
                        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                        /* Save the binding IP address to a local variable. */
                        memcpy(binding_ip_addr, search_binding->yiaddr.is_ip_addrs, IP_ADDR_LEN);     

                        /* Unprotect the global data structures. */
                        NU_Release_Semaphore(&DHCPS_Semaphore);

                        if (DHCPSDB_IP_Compare(binding_ip_addr, low_addr.is_ip_addrs)
                                == NU_SUCCESS)     
                        {
                            /* We have found an IP address in the range.  Break. */
                            found = NU_TRUE;
                            break;    
                        }
                    }

                    if (found == NU_TRUE)
                    {
                        /* The address has been found.  It may now be removed from the link list and 
                            the binding structure deallocated. */

                        /* Protect the global data structures. */
                        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                        /* Remove the binding from the list. */
                        DLL_Remove(&config_cb->ip_binding_list, search_binding);

                        /* Unprotect the global data structures. */
                        NU_Release_Semaphore(&DHCPS_Semaphore);

                        /* Deallocate the memory for the binding structure. */
                        ret_status = NU_Deallocate_Memory (search_binding);

                        if (ret_status != NU_SUCCESS)
                        {
                            /* An error occurred while deallocating the binding.  Return the
                                error status to the application. */
                            break;
                        }    

                        /* Protect the global data structures. */
                        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                        /* Now, we must remove an offered struct from the link list that corresponded
                            to the removed binding. */
                        for (offered = config_cb->ip_offer_list.dhcp_offer_head;
                             offered;
                             offered = offered->dhcp_offer_next)
                        {
                            /* We only want to remove an empty entry from the list.  It does not
                                matter which one, as long as it is empty. */
                            if (offered->dp_chaddr == NU_NULL)
                            {
                                /* This is what we were looking for.  Break. */
                                break;
                            }    
                        }           

                        /* Unprotect the global data structures. */
                        NU_Release_Semaphore(&DHCPS_Semaphore);

                        if (offered != NU_NULL)
                        {
                            /* Protect the global data structures. */
                            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                            /* Remove the binding from the list. */
                            DLL_Remove(&config_cb->ip_offer_list, offered);

                            /* Unprotect the global data structures. */
                            NU_Release_Semaphore(&DHCPS_Semaphore);
                        }    
                        else
                        {
                            /* Unable to find an empty structure.  Log an error. */
                            NERRS_Log_Error(NERR_SEVERE, __FILE__, __LINE__);
                        }    

                        /* Increment the ip_low address to the next address in the range. If the
                           increment rolls over to the next Class B address, ensure that the range
                            calls for this movement. Note:  there are 254 useable addresses in each
                            Class C subnet segment. */
                        if (low_addr.is_ip_addrs[3] == 254)
                        {
                            /* The next higher octet needs to be incremented. */
                            low_addr.is_ip_addrs[2] = low_addr.is_ip_addrs[2] + 1;

                            /* Reset the lowest octet. */
                            low_addr.is_ip_addrs[3] = 1;
                        }    

                        else
                            /* Increment just the lowest octet. */
                            low_addr.is_ip_addrs[3] = low_addr.is_ip_addrs[3] + 1;
                    }
                    else
                        ret_status = DHCPSERR_IP_RANGE_INVALID;

                } /* for (i = 0; i < num_of_ip_addr; i++) */    
            }
            else
                ret_status = DHCPSERR_IP_RANGE_INVALID;
        }
        else
            ret_status = DHCPSERR_RANGE_SUBNET_INVALID;
    }
    else
    {
        /* Return an error message that the control block pointer was incorrect. */
        ret_status = NU_INVALID_PARM;
    }
    /* Switch back to user mode */
    NU_USER_MODE();

    return(ret_status);
}

/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Get_IP_Range                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                        
*     This function will copy each of the IP addresses from the binding structures
*     into the buffer provided by the application.  The total number
*     of bytes written into the buffer will be returned.
*
* INPUTS                                                               
*                                                                      
*     DHCPS_CONFIG_CB *       
*     UINT8 *
*     INT.                                      
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     INT                                    
*                                                                      
**********************************************************************************/
INT DHCPS_Get_IP_Range(const DHCPS_CONFIG_CB *config_cb, UINT8 *binding_buffer, INT buffer_size)
{
    STATUS                  ret_status = NU_SUCCESS,
                            i = 0;
    DHCPS_CONFIG_CB         *current_config;
    DHCPS_BINDING           *current_binding;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Ensure that the passed-in parameter is valid */
    if (binding_buffer == NU_NULL)
    {
        ret_status = NU_INVALID_PARM;
    }

    else
    {
        /* Find the configuration control block on the link list. */
        for (current_config = DHCPS_Config_List.dhcp_config_head;
             current_config;
             current_config = current_config->dhcps_config_next)
        {
            if (current_config == config_cb)
            {
                /* The config_cb pointer is valid.  Break. */
                break;
            }    
        }
        if (current_config != NU_NULL)
        {
            /* Loop through each of the bindings of the configuration control block 
                and copy the IP addresses into the provided buffer. */
            for (current_binding = config_cb->ip_binding_list.dhcp_bind_head;
                 current_binding;
                 current_binding = current_binding->dhcp_bind_next)
            {

                if (buffer_size < IP_ADDR_LEN)
                {
                    ret_status = DHCPSERR_BUFFER_TOO_SMALL;
                    break;
                }    

                else
                {
                    /* Protect the global data structures. */
                    NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                    memcpy(binding_buffer, current_binding->yiaddr.is_ip_addrs, IP_ADDR_LEN);

                    /* Unprotect the global data structures. */
                    NU_Release_Semaphore(&DHCPS_Semaphore);

                    binding_buffer += IP_ADDR_LEN;
                    i += IP_ADDR_LEN;
                    buffer_size -= IP_ADDR_LEN;
                }    
            }    
        }            

        if (ret_status == NU_SUCCESS)      
            ret_status = i;
    }    
    /* Switch back to user mode */
    NU_USER_MODE();

    return(ret_status);
}   /* DHCPS_Get_IP_Range */

/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Set_Renewal_Time                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Set the renewal time of the IP address leases in the specified configuration 
*     control block or specified binding.
*
* INPUTS                                                               
*                                                                      
*     DHCPS_CONFIG_CB *
*     struct id_struct *
*     UINT32
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     STATUS
*                                                                      
**********************************************************************************/
STATUS DHCPS_Set_Renewal_Time (DHCPS_CONFIG_CB *config_cb, const struct id_struct *client_ip_addr, 
                                UINT32 renew_time)
{
    STATUS                  ret_status = NU_SUCCESS;
    UINT8                   *options_block;
    DHCPS_OPTIONS           *option_buffer;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Ensure that the configuration control block pointer is valid. */
    if (config_cb == NU_NULL)
    {
        ret_status = NU_INVALID_PARM;
    }    

    else
    {
        /* First, we must determine if this lease renewal entry is for a particular IP address (static IP).  
            This can be determined if a client IP address was passed in.  If so, then the renewal 
            needs to be added to an IP option block. */
        if (client_ip_addr != NU_NULL)
        {
            /* Perform any memory block manipulation that may need to be done to make room for
                the lease renewal entry that is being added. */
            options_block = DHCPS_Add_Option_To_Memory_Block(config_cb, client_ip_addr, DHCPS_RENEWAL_T1, 
                                                                &option_buffer);    
            /* Test to ensure that the option block has been prepared to receive the new option. */
            if (options_block != NU_NULL)
            {
                /* Protect the global data structures. */
                NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                /* The option can now be added to the options block. First, we must ensure that 
                    the option has been previously added.  If not, we can just add the option tag and
                    data length now. */
                if (*options_block == DHCP_END)
                {
                    /* Store the option into the memory block in the format that it will be
                       provided to the client. */

                    /* Option Tag */
                    *options_block++ = DHCPS_RENEWAL_T1;

                    /* Data Length */
                    *options_block++ = sizeof(UINT32);

                    /* Lease renewal time */
                    memcpy(options_block, &renew_time, sizeof(UINT32));

                    /* Increment the options block ptr */
                    options_block += sizeof(UINT32);

                    /* Copy the end option tag into the buffer. */
                    *options_block = DHCP_END;

                    /* Increment the bytes written. */
                    option_buffer->bytes_written += (sizeof(UINT32) + 2);

                }    

                else
                {
                    /* We must overwrite the option value that is currently in place.  */

                    /* Increment to the location of the option data. */
                    options_block += 2;

                    /* Lease renewal time */
                    memcpy(options_block, &renew_time, sizeof(UINT32));

                    /* Increment the options block ptr */
                    options_block += sizeof(UINT32);

                    /* Add the end option tag. */
                    *options_block = DHCP_END;

                    /* Increment the bytes written. */
                    option_buffer->bytes_written += (sizeof(UINT32));
                }

                /* Unprotect the global data structures. */
                NU_Release_Semaphore(&DHCPS_Semaphore);
            }

            else
                /* An invalid parameter was passed into the function.  The rebinding entry can not be added. */
                ret_status = NU_INVALID_PARM;
        }       

        else
        {
            /* This entry is meant for the configuration control block. */

            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* Copy the lease renewal time into the control block. */
            config_cb->dhcp_renew_t1 = renew_time;

            /* Set the return status as successful */
            ret_status = NU_SUCCESS;

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);
        }
    }
    /* Switch back to user mode */
    NU_USER_MODE();

    return (ret_status);
} /* DHCPS_Set_Renewal_Time */


/****************************************************************************
* FUNCTION                                                                   
*                                                                            
*    DHCPS_Get_Renewal_Time                                                       
*                                                                            
* DESCRIPTION                                                                
*                                                                            
*    Return the renewal value for IP address lease from the specified configuration 
*    control block or specific binding.  The returned value is the total number 
*    of bytes that were written into the buffer.
*
* INPUTS                                                                     
*                                                                            
*    DHCPS_CONFIG_CB *
*    struct id_struct *
*    UINT8 *
*    INT
*                                                                            
* OUTPUTS                                                                    
*                                                                            
*    INT
*                                                                            
******************************************************************************/
INT DHCPS_Get_Renewal_Time (const DHCPS_CONFIG_CB *config_cb, const struct id_struct *client_ip_addr, 
                            UINT8 *renew_buffer, INT buffer_size)
{
    UINT8                       *option_ptr,
                                option_data_len;
    INT                         bytes_written;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* First, ensure that the buffer that has been passed in is large enough to accept at least one
        entry. */
    if ((buffer_size < sizeof(UINT32)) || (renew_buffer == NU_NULL))
    {
        bytes_written = NU_INVALID_PARM;
    }

    /* Check to see if the renewal entry is destined for a static IP entry. */
    else if (client_ip_addr != NU_NULL)
    {
        /* Search through the static IP entries to find the lease renewal time entry. */
        option_ptr = DHCPS_Get_Option_From_Memory_Block(config_cb, client_ip_addr, DHCPS_RENEWAL_T1);

        if (option_ptr == NU_NULL)
        {
            /* An error occurred during the search for the renewal time for the particular IP address. */
            bytes_written = NU_INVALID_PARM;
        }    

        else
        {
            /* The lease renewal time option has been found for the desired IP address. */

            /* Increment the option pointer to point at the length of the renewal option. */
            option_data_len = *(option_ptr + 1);

            /* Ensure that the buffer is large enough to hold the option data. */
            if (buffer_size < option_data_len)
            {
                bytes_written = DHCPSERR_BUFFER_TOO_SMALL;
            }    

            else
            {
                /* Increment the option pointer to point at the data that will be copied. */
                option_ptr += 2;

                /* Protect the global data structures. */
                NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                /* Copy the renewal time option into the buffer that has been provided by the application. */
                memcpy(renew_buffer, option_ptr, option_data_len);

                /* Unprotect the global data structures. */
                NU_Release_Semaphore(&DHCPS_Semaphore);
                
                /* Save the number of bytes that have been written into the buffer for returning to
                    the application. */
                bytes_written = option_data_len;
            }    
        }    
    }    

    else 
    {
        /* Ensure that the buffer is large enough to hold the option data. */
        if (buffer_size < sizeof(UINT32))
        {
            bytes_written = DHCPSERR_BUFFER_TOO_SMALL;
        }    

        else        
        {
            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* Copy the renewal time option entry into the buffer provided by the application. */
            memcpy(renew_buffer, &config_cb->dhcp_renew_t1, sizeof(UINT32));

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);

            /* Set the return variable to the number of bytes that were written into the buffer. */
            bytes_written = sizeof(UINT32);  
        }    
    }
    /* Switch back to user mode */
    NU_USER_MODE();

    return (bytes_written); 

} /* DHCPS_Get_Renewal_Time */


/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Set_Rebind_Time                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Set the rebinding time of the IP address leases in the specified configuration 
*     control block or specified binding.
*
* INPUTS                                                               
*                                                                      
*     None.                                      
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     None.                                    
*                                                                      
**********************************************************************************/
STATUS DHCPS_Set_Rebind_Time (DHCPS_CONFIG_CB *config_cb, const struct id_struct *client_ip_addr, 
                                UINT32 rebind_time)
{
    STATUS                  ret_status = NU_SUCCESS;
    UINT8                   *options_block;
    DHCPS_OPTIONS           *option_buffer;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();
   
    /* Ensure that the configuration control block pointer is valid. */
    if (config_cb == NU_NULL)
    {
        ret_status = NU_INVALID_PARM;
    }    

    else
    {
        /* First, we must determine if this lease rebinding entry is for a particular IP address (static IP).  
            This can be determined if a client IP address was passed in.  If so, then the rebinding 
            needs to be added to an IP option block. */
        if (client_ip_addr != NU_NULL)
        {
            /* Perform any memory block manipulation that may need to be done to make room for
                the lease rebinding entry that is being added. */
            options_block = DHCPS_Add_Option_To_Memory_Block(config_cb, client_ip_addr, DHCP_REBINDING_T2, 
                                                                &option_buffer);    

            /* Test to ensure that the option block has been prepared to receive the new option. */
            if (options_block != NU_NULL)
            {
                /* Protect the global data structures. */
                NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                /* The option can now be added to the options block. First, we must ensure that 
                    the option has been previously added.  If not, we can just add the option tag and
                    data length now. */
                if (*options_block == DHCP_END)
                {
                    /* Store the option into the memory block in the format that it will be
                       provided to the client. */

                    /* Option Tag */
                    *options_block++ = DHCP_REBINDING_T2;

                    /* Data Length */
                    *options_block++ = sizeof(UINT32);

                     /* Lease rebind time */
                    memcpy(options_block, &rebind_time, sizeof(UINT32));

                    /* Increment the options block ptr */
                    options_block += sizeof(UINT32);

                    /* Copy the end option tag into the buffer. */
                    *options_block = DHCP_END;

                    /* Increment the bytes written. */
                    option_buffer->bytes_written += (sizeof(UINT32) + 2);

                }    

                else
                {
                    /* We must overwrite the option value that is currently in place.  */

                    /* Increment to the location of the option data. */
                    options_block += 2;

                    /* Lease rebind time */
                    memcpy(options_block, &rebind_time, sizeof(UINT32));

                    /* Increment the options block ptr */
                    options_block += sizeof(UINT32);

                    /* Add the end option tag. */
                    *options_block = DHCP_END;

                    /* Increment the bytes written. */
                    option_buffer->bytes_written += (sizeof(UINT32));
                }

                /* Unprotect the global data structures. */
                NU_Release_Semaphore(&DHCPS_Semaphore);
            }

            else
                /* An invalid parameter was passed into the function.  The rebinding entry can not be added. */
                ret_status = NU_INVALID_PARM;
        }       

        else
        {
            /* This entry is meant for the configuration control block. */

            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* Copy the lease rebinding time into the control block. */
            config_cb->dhcp_rebind_t2 = rebind_time;

            /* Set the return status as successful */
            ret_status = NU_SUCCESS;

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);
        }
    }
    /* Switch back to user mode */
    NU_USER_MODE();

    return (ret_status);
} /* DHCPS_Set_Rebind_Time */


/****************************************************************************
* FUNCTION                                                                   
*                                                                            
*    DHCPS_Get_Rebind_Time                                                       
*                                                                            
* DESCRIPTION                                                                
*                                                                            
*    Return the renewal value for IP address lease from the specified configuration 
*    control block or specific binding.  The returned value is the total number 
*    of bytes that were written into the buffer.
*
* INPUTS                                                                     
*                                                                            
*    DHCPS_CONFIG_CB *
*    struct id_struct *
*    UINT8 *
*    INT
*                                                                            
* OUTPUTS                                                                    
*                                                                            
*    INT
*                                                                            
******************************************************************************/
INT DHCPS_Get_Rebind_Time (const DHCPS_CONFIG_CB *config_cb, const struct id_struct *client_ip_addr, 
                           UINT8 *rebind_buffer, INT buffer_size)
{
    UINT8                       *option_ptr,
                                option_data_len;
    INT                         bytes_written;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* First, ensure that the buffer that has been passed in is large enough to accept at least one
        entry. */
    if ((buffer_size < sizeof(UINT32)) || (rebind_buffer == NU_NULL))
    {
        bytes_written = NU_INVALID_PARM;
    }

    /* Check to see if the rebinding entry is destined for a static IP entry. */
    else if (client_ip_addr != NU_NULL)
    {
        /* Search through the static IP entries to find the lease rebinding time entry. */
        option_ptr = DHCPS_Get_Option_From_Memory_Block(config_cb, client_ip_addr, DHCP_REBINDING_T2);

        if (option_ptr == NU_NULL)
        {
            /* An error occurred during the search for the rebinding time for the particular IP address. */
            bytes_written = NU_INVALID_PARM;
        }    

        else
        {
            /* The lease rebinding time option has been found for the desired IP address. */

            /* Increment the option pointer to point at the length of the rebinding option. */
            option_data_len = *(option_ptr + 1);

            /* Ensure that the buffer is large enough to hold the option data. */
            if (buffer_size < option_data_len)
            {
                bytes_written = DHCPSERR_BUFFER_TOO_SMALL;
            }    

            else
            {
                /* Increment the option pointer to point at the data that will be copied. */
                option_ptr += 2;           

                /* Protect the global data structures. */
                NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                /* Copy the rebinding time option into the buffer that has been provided by the application. */
                memcpy(rebind_buffer, option_ptr, option_data_len);

                /* Unprotect the global data structures. */
                NU_Release_Semaphore(&DHCPS_Semaphore);

                /* Save the number of bytes that have been written into the buffer for returning to
                    the application. */
                bytes_written = option_data_len;
            }    
        }    
    }    

    else 
    {
        /* Ensure that the buffer is large enough to hold the option data. */
        if (buffer_size < sizeof(UINT32))
        {
            bytes_written = DHCPSERR_BUFFER_TOO_SMALL;
        }    

        else
        {
            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* Copy the rebinding time option entry into the buffer provided by the application. */
            memcpy(rebind_buffer, &config_cb->dhcp_rebind_t2, sizeof(UINT32));

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);

            /* Set the return variable to the number of bytes that were written into the buffer. */
            bytes_written = sizeof(UINT32);      
        }    
    }
    /* Switch back to user mode */
    NU_USER_MODE();

    return (bytes_written); 

} /* DHCPS_Get_Rebind_Time */


/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Set_Default_Lease_Time                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Set the lease time of the IP address leases in the specified configuration 
*     control block or specified binding.
*
* INPUTS                                                               
*                                                                      
*     None.                                      
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     None.                                    
*                                                                      
**********************************************************************************/
STATUS DHCPS_Set_Default_Lease_Time (DHCPS_CONFIG_CB *config_cb, const struct id_struct *client_ip_addr, 
                                UINT32 default_lease_time)
{
    STATUS                  ret_status = NU_SUCCESS;
    UINT8                   *options_block;
    DHCPS_OPTIONS           *option_buffer;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();
   
    /* Ensure that the configuration control block pointer is valid. */
    if (config_cb == NU_NULL)
    {
        ret_status = NU_INVALID_PARM;
    }    

    else
    {
        /* First, we must determine if this default lease entry is for a particular IP address (static IP).  
            This can be determined if a client IP address was passed in.  If so, then the default lease 
            needs to be added to an IP option block. */
        if (client_ip_addr != NU_NULL)
        {
            /* Perform any memory block manipulation that may need to be done to make room for
                the default lease entry that is being added. */
            options_block = DHCPS_Add_Option_To_Memory_Block(config_cb, client_ip_addr, DEFAULT_LEASE, 
                                                                &option_buffer);    

            /* Test to ensure that the option block has been prepared to receive the new option. */
            if (options_block != NU_NULL)
            {
                /* Protect the global data structures. */
                NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                /* The option can now be added to the options block. First, we must ensure that 
                    the option has been previously added.  If not, we can just add the option tag and
                    data length now. */
                if (*options_block == DHCP_END)
                {
                    /* Store the option into the memory block in the format that it will be
                       provided to the client. */

                    /* Option Tag */
                    *options_block++ = DEFAULT_LEASE;

                    /* Data Length */
                    *options_block++ = sizeof(UINT32);

                    /* Default lease time */
                    memcpy(options_block, &default_lease_time, sizeof(UINT32));

                    /* Increment the options block ptr */
                    options_block += sizeof(UINT32);

                    /* Copy the end option tag into the buffer. */
                    *options_block = DHCP_END;

                    /* Increment the bytes written. */
                    option_buffer->bytes_written += (sizeof(UINT32) + 2);

                }    

                else
                {
                    /* We must overwrite the option value that is currently in place.  */

                    /* Increment to the location of the option data. */
                    options_block += 2;

                    /* Default lease time */
                    memcpy(options_block, &default_lease_time, sizeof(UINT32));

                    /* Increment the options block ptr */
                    options_block += sizeof(UINT32);

                    /* Add the end option tag. */
                    *options_block = DHCP_END;

                    /* Increment the bytes written. */
                    option_buffer->bytes_written += (sizeof(UINT32));
                }

                /* Unprotect the global data structures. */
                NU_Release_Semaphore(&DHCPS_Semaphore);
            }

            else
                /* An invalid parameter was passed into the function.  The default lease time can not be added. */
                ret_status = NU_INVALID_PARM;
        }       

        else
        {
            /* This entry is meant for the configuration control block. */

            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* Copy the default lease time into the control block. */
            config_cb->default_lease_length = default_lease_time;

            /* Set the return status as successful */
            ret_status = NU_SUCCESS;

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);
        }
    }
    /* Switch back to user mode */
    NU_USER_MODE();

    return (ret_status);
} /* DHCPS_Set_Default_Lease_Time */


/****************************************************************************
* FUNCTION                                                                   
*                                                                            
*    DHCPS_Get_Default_Lease_Time                                                       
*                                                                            
* DESCRIPTION                                                                
*                                                                            
*    Return the lease time value for IP address lease from the specified configuration 
*    control block or specific binding.  The returned value is the total number 
*    of bytes that were written into the buffer.
*
* INPUTS                                                                     
*                                                                            
*    DHCPS_CONFIG_CB *
*    struct id_struct *
*    UINT8 *
*    INT
*                                                                            
* OUTPUTS                                                                    
*                                                                            
*    INT
*                                                                            
******************************************************************************/
INT DHCPS_Get_Default_Lease_Time (const DHCPS_CONFIG_CB *config_cb, const struct id_struct *client_ip_addr, 
                                    UINT8 *default_lease_buffer, INT buffer_size)
{
    UINT8                       *option_ptr,
                                option_data_len;
    INT                         bytes_written;

    NU_SUPERV_USER_VARIABLES

    /* Swtich to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* First, ensure that the buffer that has been passed in is large enough to accept at least one
        entry. */
    if ((buffer_size < sizeof(UINT32)) || (default_lease_buffer == NU_NULL))
    {
        bytes_written = NU_INVALID_PARM;
    }

    /* Check to see if the default lease entry is destined for a static IP entry. */
    else if (client_ip_addr != NU_NULL)
    {
        /* Search through the static IP entries to find the default lease time entry. */
        option_ptr = DHCPS_Get_Option_From_Memory_Block(config_cb, client_ip_addr, DEFAULT_LEASE);

        if (option_ptr == NU_NULL)
        {
            /* An error occurred during the search for the default lease time for the particular IP address. */
            bytes_written = NU_INVALID_PARM;
        }    

        else
        {
            /* The default lease time option has been found for the desired IP address. */

            /* Increment the option pointer to point at the length of the default lease option. */
            option_data_len = *(option_ptr + 1);

            /* Ensure that the buffer size is large enough to hold the option data. */
            if (buffer_size < option_data_len)
            {
                bytes_written = DHCPSERR_BUFFER_TOO_SMALL;
            }    

            else
            {
                /* Increment the option pointer to point at the data that will be copied. */
                option_ptr += 2;           

                /* Protect the global data structures. */
                NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                /* Copy the default lease time option into the buffer that has been provided by the application. */
                memcpy(default_lease_buffer, option_ptr, option_data_len);

                /* Unprotect the global data structures. */
                NU_Release_Semaphore(&DHCPS_Semaphore);

                /* Save the number of bytes that have been written into the buffer for returning to
                    the application. */
                bytes_written = option_data_len;
            }    
        }    
    }    

    else
    {
        /* Ensure that the buffer size is large enough to hold the option data. */
        if (buffer_size < sizeof(UINT32))
        {
            bytes_written = DHCPSERR_BUFFER_TOO_SMALL;
        }    

        else
        {
            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* Copy the default lease time option entry into the buffer provided by the application. */
            memcpy(default_lease_buffer, &config_cb->default_lease_length, sizeof(UINT32));

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);

            /* Set the return variable to the number of bytes that were written into the buffer. */
            bytes_written = sizeof(UINT32);  
        }    
    }
    /* Switch back to user mode */
    NU_USER_MODE();

    return (bytes_written); 

} /* DHCPS_Get_Default_Lease_Time */


/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Set_Offered_Wait_Time                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Set the offered wait time for an offered IP address in the specified configuration 
*     control block or specified binding.
*
* INPUTS                                                               
*                                                                      
*     DHCPS_CONFIG_CB *
*     struct id_struct *
*     UINT32
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     None.                                    
*                                                                      
**********************************************************************************/
STATUS DHCPS_Set_Offered_Wait_Time (DHCPS_CONFIG_CB *config_cb, const struct id_struct *client_ip_addr, 
                                    UINT32 offered_wait_time)
{
    STATUS                  ret_status = NU_SUCCESS;
    UINT8                   *options_block;
    DHCPS_OPTIONS           *option_buffer;
   
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Ensure that the configuration control block pointer is valid. */
    if (config_cb == NU_NULL)
    {
        ret_status = NU_INVALID_PARM;
    }    

    else
    {
        /* First, we must determine if this offered wait entry is for a particular IP address (static IP).  
            This can be determined if a client IP address was passed in.  If so, then the offered wait time 
            needs to be added to an IP option block. */
        if (client_ip_addr != NU_NULL)
        {
            /* Perform any memory block manipulation that may need to be done to make room for
                the offered wait time entry that is being added. */
            options_block = DHCPS_Add_Option_To_Memory_Block(config_cb, client_ip_addr, DHCP_OFFERED_WAIT, 
                                                                &option_buffer);    

            /* Test to ensure that the option block has been prepared to receive the new option. */
            if (options_block != NU_NULL)
            {
                /* Protect the global data structures. */
                NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                /* The option can now be added to the options block. First, we must ensure that 
                    the option has been previously added.  If not, we can just add the option tag and
                    data length now. */
                if (*options_block == DHCP_END)
                {
                    /* Store the option into the memory block in the format that it will be
                       provided to the client. */

                    /* Option Tag */
                    *options_block++ = DHCP_OFFERED_WAIT;

                    /* Data Length */
                    *options_block++ = sizeof(UINT32);

                    /* Offered wait time */
                    memcpy(options_block, &offered_wait_time, sizeof(UINT32));

                    /* Increment the options block ptr */
                    options_block += sizeof(UINT32);

                    /* Copy the end option tag into the buffer. */
                    *options_block = DHCP_END;

                    /* Increment the bytes written. */
                    option_buffer->bytes_written += (sizeof(UINT32) + 2);

                }    

                else
                {
                    /* We must overwrite the option value that is currently in place.  */

                    /* Increment to the location of the option data. */
                    options_block += 2;

                    /* Offered wait time */
                    memcpy(options_block, &offered_wait_time, sizeof(UINT32));

                    /* Increment the options block ptr */
                    options_block += sizeof(UINT32);

                    /* Add the end option tag. */
                    *options_block = DHCP_END;

                    /* Increment the bytes written. */
                    option_buffer->bytes_written += (sizeof(UINT32));
                }

                /* Unprotect the global data structures. */
                NU_Release_Semaphore(&DHCPS_Semaphore);
            }

            else
                /* An invalid parameter was passed into the function.  The offered wait time can not 
                    be added. */
                ret_status = NU_INVALID_PARM;
        }       

        else
        {
            /* This entry is meant for the configuration control block. */

            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* Copy the offered wait time into the control block. */
            config_cb->offered_wait_time = offered_wait_time;

            /* Set the return status as successful */
            ret_status = NU_SUCCESS;

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);
        }
    }
    /* Switch back to user mode */
    NU_USER_MODE();

    return (ret_status);
} /* DHCPS_Set_Offered_Wait_Time */


/****************************************************************************
* FUNCTION                                                                   
*                                                                            
*    DHCPS_Get_Offered_Wait_Time                                                       
*                                                                            
* DESCRIPTION                                                                
*                                                                            
*    Return the offered wait value for and offered IP address from the specified 
*    configuration control block or specific binding.  The returned value is 
*    the total number of bytes that were written into the buffer.
*
* INPUTS                                                                     
*                                                                            
*    DHCPS_CONFIG_CB *
*    struct id_struct *
*    UINT8 *
*    INT
*                                                                            
* OUTPUTS                                                                    
*                                                                            
*    INT
*                                                                            
******************************************************************************/
INT DHCPS_Get_Offered_Wait_Time (const DHCPS_CONFIG_CB *config_cb, const struct id_struct *client_ip_addr, 
                                    UINT8 *offered_wait_buffer, INT buffer_size)
{
    UINT8                       *option_ptr,
                                option_data_len;
    INT                         bytes_written;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* First, ensure that the buffer that has been passed in is large enough to accept at least one
        entry. */
    if ((buffer_size < sizeof(UINT32)) || (offered_wait_buffer == NU_NULL))
    {
        bytes_written = NU_INVALID_PARM;
    }

    /* Check to see if the offered wait time entry is destined for a static IP entry. */
    else if (client_ip_addr != NU_NULL)
    {
        /* Search through the static IP entries to find the offered wait time entry. */
        option_ptr = DHCPS_Get_Option_From_Memory_Block(config_cb, client_ip_addr, DHCP_OFFERED_WAIT);

        if (option_ptr == NU_NULL)
        {
            /* An error occurred during the search for the offered wait time for the particular 
                IP address. */
            bytes_written = NU_INVALID_PARM;
        }    

        else
        {
            /* The offered wait time option has been found for the desired IP address. */

            /* Increment the option pointer to point at the length of the offered wait 
                time option. */
            option_data_len = *(option_ptr + 1);

            /* Ensure that the buffer size is large enough to hold the option data. */
            if (buffer_size < option_data_len)
            {
                bytes_written = DHCPSERR_BUFFER_TOO_SMALL;
            }    

            else
            {
                /* Increment the option pointer to point at the data that will be copied. */
                option_ptr += 2;

                /* Protect the global data structures. */
                NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                /* Copy the offered wait time option into the buffer that has been provided by 
                    the application. */
                memcpy(offered_wait_buffer, option_ptr, option_data_len);

                /* Unprotect the global data structures. */
                NU_Release_Semaphore(&DHCPS_Semaphore);

                /* Save the number of bytes that have been written into the buffer for returning to
                    the application. */
                bytes_written = option_data_len;
            }    
        }    
    }    

    else
    {
        /* Ensure that the buffer size is large enough to hold the option data. */
        if (buffer_size < sizeof(UINT32))
        {
            bytes_written = DHCPSERR_BUFFER_TOO_SMALL;
        }    

        else
        {        
            /* Copy the offered wait time option entry into the buffer provided by the application. */
            memcpy(offered_wait_buffer, &config_cb->offered_wait_time, sizeof(UINT32));

            /* Set the return variable to the number of bytes that were written into the buffer. */
            bytes_written = sizeof(UINT32);  
        }    
    }
    /* Switch back to user mode */
    NU_USER_MODE();
    
    return (bytes_written); 

} /* DHCPS_Get_Offered_Wait_Time */
