/***************************************************************************
*
*            Copyright Mentor Graphics Corporation 2001-2011
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
****************************************************************************/
/****************************************************************************
*                                                                            
*   FILENAME                                                                          
*                                                                                    
*       nat_prtmap.c                                                
*                                                                                    
*   DESCRIPTION                                                                
*
*       This file contains the function necessary to add, delete and read 
*       entries from the Portmap Table so select private nodes are 
*       accessible to the public network.
*                                                           
*   DATA STRUCTURES                                                            
*
*       None.                             
*                                               
*   FUNCTIONS                                                                  
*                                             
*       NAT_Portmapper                              
*                                             
*   DEPENDENCIES                                                               
*
*       target.h
*       externs.h
*       nat_defs.h            
*       nat_extr.h
*                                                                
******************************************************************************/

#include "networking/target.h"
#include "networking/externs.h"
#include "networking/nat_defs.h"
#include "networking/nat_extr.h"

extern NU_MEMORY_POOL          *NAT_Memory;
extern DV_DEVICE_ENTRY         *NAT_External_Device;
extern NAT_PORTMAP_TABLE       NAT_Portmap_Table;

/****************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       NAT_Portmapper
*                                                                       
*   DESCRIPTION                                                           
*            
*       This function adds, deletes and reads entries from the Portmap 
*       Table as specified by the user.                                                           
*                                                                       
*   INPUTS                                                                
*                                
*       function                NAT_PORTMAP_ADD to add an entry to the 
*                               Portmap Table.  NAT_PORTMAP_DELETE to remove 
*                               an entry from the Portmap Table.
*                               NAT_PORTMAP_READ to retrieve a list of 
*                               entries.
*       *buffer                 A pointer to a NAT_PORTMAP_SERVICE data 
*                               structure for an ADD or DELETE, or a 
*                               pointer to the memory buffer into which
*                               to place the entries for a READ.
*       buffer_length           Length of the buffer
*                                                                       
*   OUTPUTS                                                               
*    
*       For an ADD or DELETE:                  
*
*           NU_SUCCESS          The operation was successful.
*           NU_INVALID_PARM     One of the parameters passed in is invalid.
*           NU_NO_MEMORY        There is not enough memory to add an entry.
*           NAT_ENTRY_EXISTS    The entry already exists.
*
*       For a READ:
*
*           entries_returned    The number of entries placed in the buffer.
*           NU_INVALID_PARM     One of the parameters passed in is invalid.
*           NU_NO_MEMORY        There is not enough memory to add an entry.
*                                                                       
****************************************************************************/
STATUS NAT_Portmapper(UINT8 function, VOID *buffer, UINT32 buffer_length)
{
    NAT_PORTMAP_ENTRY   *current_entry, *previous_entry, *new_entry, *next_entry;
    UINT32              new_service_ip_addr = 0;
    STATUS              status = NU_SUCCESS;    
    RTAB_ROUTE          ro;
    NAT_PORTMAP_SERVICE *new_portmap_service = NU_NULL;
    NAT_PORTMAP_SERVICE dummy_entry;
    UINT16              entries_returned = 0, entries_possible;
    UINT8               *data_ptr;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Check the input parameters */
    if (buffer == NU_NULL)
    {
        /* Switch back to user mode. */
        NU_USER_MODE();

        return (NU_INVALID_PARM);
    }
    
    UTL_Zero(&ro, sizeof(RTAB_ROUTE));

    if ( (function == NAT_PORTMAP_ADD) || (function == NAT_PORTMAP_DELETE) )
    {
        new_portmap_service = (NAT_PORTMAP_SERVICE*)buffer;

        if (new_portmap_service->nat_internal_source_ip == NU_NULL)
        {
            /* Switch back to user mode. */
            NU_USER_MODE();

            return (NU_INVALID_PARM);
        }
        
        /* Setup the route structure. */
        ro.rt_ip_dest.sck_family     = SK_FAM_IP;
        ro.rt_ip_dest.sck_len        = sizeof (SCK_SOCKADDR_IP);
        ro.rt_ip_dest.sck_addr       = IP_ADDR(new_portmap_service->nat_internal_source_ip);
                
        /* Find a route to the internal node to get a pointer to the device. */
        NAT_Find_Route(&ro);      
        
        if (ro.rt_route == NU_NULL)
        {
            /* Switch back to user mode. */
            NU_USER_MODE();

            return (NAT_NO_ROUTE);
        }

        new_service_ip_addr = IP_ADDR(new_portmap_service->nat_internal_source_ip);   
    }

    else if (buffer_length == 0)
    {
        /* Switch back to user mode. */
        NU_USER_MODE();

        return (NU_INVALID_PARM);
    }

    /* Get the first entry in the Portmap Table */
    current_entry = NAT_Portmap_Table.nat_head;
    
    /* Save off the current node */
    previous_entry = current_entry;

    switch (function)
    {
    /* Add an entry to the Portmap Table */
    case NAT_PORTMAP_ADD:

        switch (new_portmap_service->nat_protocol)
        {
        case IPPROTO_TCP:
        case IPPROTO_UDP:

            /* Check that the entry does not already exist */
            while (current_entry != NU_NULL)
            {
                /* TCP and UDP entries are added in order of internal port 
                 * number.  If the current node's port number is greater than
                 * the new node's port number, we can stop searching through
                 * the list, because the node does not already exist.
                 */
                if (current_entry->nat_external_source_port > new_portmap_service->
                    nat_internal_source_port)
                    break;

                /* Check if the entry matches the new node */
                else if ( (current_entry->nat_internal_source_port == 
                           new_portmap_service->nat_internal_source_port) &&
                          (current_entry->nat_protocol == 
                          new_portmap_service->nat_protocol) &&
                          (current_entry->nat_internal_source_ip == new_service_ip_addr) )
                {
                    status = NAT_ENTRY_EXISTS;
                    break;
                }
    
                /* Save off the previous node and get the next node */
                else
                {
                    previous_entry = current_entry;
                    current_entry = current_entry->nat_next;
                }
            }
    
            /* If the entry does not already exist, allocate memory for the new
             * entry and add it to the list.
             */
            if (status == NU_SUCCESS)
            {
                if ((status = NU_Allocate_Memory(NAT_Memory, (VOID**)&new_entry, 
                     sizeof(NAT_PORTMAP_ENTRY), NU_NO_SUSPEND)) == NU_SUCCESS)
                {
                    UTL_Zero(new_entry, sizeof(NAT_PORTMAP_ENTRY));

                    new_entry->nat_internal_device = ro.rt_route->rt_device;
                    new_entry->nat_internal_source_ip = new_service_ip_addr;

                    new_entry->nat_internal_source_port = new_portmap_service->
                        nat_internal_source_port;

                    new_entry->nat_external_source_port = new_portmap_service->
                        nat_internal_source_port;

                    new_entry->nat_next = NU_NULL;
                    new_entry->nat_protocol = new_portmap_service->nat_protocol;
                    new_entry->nat_used_flag = 1;

                    /* If this is the first node in the list, point the head
                     * of the list at this node.
                     */
                    if ( (NAT_Portmap_Table.nat_head == NU_NULL) ||
                         (current_entry == NAT_Portmap_Table.nat_head) )
                        NAT_Portmap_Table.nat_head = new_entry;
                    else
                        previous_entry->nat_next = new_entry;
    
                    /* Set the new node's next pointer */
                    new_entry->nat_next = current_entry;
    
                    /* Check if there is another entry in the table that matches
                     * this entry but with a different IP address.
                     */
                    next_entry = NAT_Find_Portmap_Entry(new_entry->nat_protocol, 
                                                        NAT_EXTERNAL_DEVICE, NU_NULL, 0,
                                                        NAT_External_Device->dev_addr.dev_ip_addr,
                                                        new_entry->nat_external_source_port);
    
                    /* If another match was found, set the used flag to 0 */
                    if (next_entry != new_entry)
                        new_entry->nat_used_flag = 0;
                }
            }
            break;

        default:

            status = NU_INVALID_PARM;
        }

        break;

    /* Delete an entry from the Portmap Table */
    case NAT_PORTMAP_DELETE:

        switch (new_portmap_service->nat_protocol)
        {
        case IPPROTO_TCP:
        case IPPROTO_UDP:

            /* Find the entry in the list */
            while (current_entry != NU_NULL)
            {          
                /* TCP and UDP entries are added in order of internal port 
                 * number.  If the current node's port number is greater than
                 * the new node's port number, we can stop searching through
                 * the list, because the node does not exist.
                 */
                if (current_entry->nat_external_source_port > 
                    new_portmap_service->nat_internal_source_port)
                    break;

                /* Check if the current node is the target */
                else if ( (current_entry->nat_internal_source_port == 
                           new_portmap_service->nat_internal_source_port) &&
                          (current_entry->nat_protocol == 
                           new_portmap_service->nat_protocol) &&
                          (current_entry->nat_internal_source_ip == 
                           new_service_ip_addr) )
                {
                    /* The node to delete is the Root Node. */
                    if (previous_entry == current_entry)
                    {
                        /* There are no other nodes in the list */
                        if (current_entry->nat_next == NU_NULL)
                            NAT_Portmap_Table.nat_head = NU_NULL;
                        else
                            NAT_Portmap_Table.nat_head = current_entry->nat_next;
                    }
    
                    /* There are other nodes */
                    else
                        previous_entry->nat_next = current_entry->nat_next;
    
                    /* Deallocate the memory for the node */
                    NU_Deallocate_Memory(current_entry);
    
                    break;
                }
    
                /* Otherwise, get the next node */
                else
                {
                    /* Save off the current node */
                    previous_entry = current_entry;
    
                    current_entry = current_entry->nat_next;
                }
            }
            break;

        default:

            status = NU_INVALID_PARM;
        }

        break;

        case NAT_PORTMAP_READ:

            /* Calculate the number of entries that will fit in the buffer */
            entries_possible = 
                (UINT16)(buffer_length / sizeof(NAT_PORTMAP_SERVICE));

            /* Zero out the padding parameter of the NAT_PORTMAP_SERVICE data
             * structure.
             */
            UTL_Zero(dummy_entry.nat_portmap_service_padding, 3);

            /* Point to the buffer */
            data_ptr = (UINT8*)buffer;

            /* If we get to the end of the list or reach the maximum number of
             * entries that will fit into the buffer, quit.
             */
            while ( (current_entry) && (entries_possible) )
            {
                /* Fill in the NAT_PORTMAP_SERVICE data structure */
                dummy_entry.nat_external_source_port = 
                    current_entry->nat_external_source_port;

				/* Initialize to remove KW warning. */
				memset(dummy_entry.nat_internal_source_ip, 0, IP_ADDR_LEN);
				
                PUT32(dummy_entry.nat_internal_source_ip, 0, 
                      current_entry->nat_internal_source_ip);

                dummy_entry.nat_internal_source_port = 
                    current_entry->nat_internal_source_port;

                dummy_entry.nat_protocol = current_entry->nat_protocol;

                /* Put the data structure in the buffer */
                memcpy(data_ptr, &dummy_entry, sizeof(NAT_PORTMAP_SERVICE));

                /* Increment the buffer for the next entry */
                data_ptr += sizeof(NAT_PORTMAP_SERVICE);

                /* Get the next entry in the list */
                current_entry = current_entry->nat_next;

                /* Decrement the number of entries that will fit in the buffer */
                entries_possible --;

                /* Increment the number of entries placed in the buffer */
                entries_returned ++;
            }

            /* Return the number of entries in the buffer */
            status = entries_returned;

            break;

    /* The specified action is not valid */
    default:    

        status = NU_INVALID_PARM;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* NAT_Portmapper */
