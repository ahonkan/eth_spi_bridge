/*************************************************************************
*
*              Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/**************************************************************************
*
* FILE NAME
*
*      netboot_query.c
*
* DESCRIPTION
*
*      This file contains functions to query the configuration
*      options from the registry.
*
*      The Nucleus Registry is a read-only data structure that
*      contains the value of settings compiled from the Net
*      project's metadata.  Since the registry is read-only, a
*      second, read/write list of the configuration settings is
*      built and maintained which contains the configuration
*      settings for each network interface that has been
*      registered by the device manager.
*
*      Functions are provided here to:
*          1) Get configuration setting from the registry by
*             registry path, and
*          2) Get and Set configuration settings from/to the
*             interface configuration list by interface name.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       add_to_query_key
*
*    Interface Configuration List Management.
*       Ifconfig_Init
*       Ifconfig_Deinit
*       Ifconfig_Remove_All_Interfaces
*       Ifconfig_Find_Interface
*       NU_Ifconfig_Update_Interface
*       NU_Ifconfig_Delete_Interface
*       NU_Ifconfig_Set_Interface_Defaults
*
*    Read-only Registry queries
*       NU_Registry_Get_IPv4_Enabled
*       NU_Registry_Get_DHCP4_Enabled
*       NU_Registry_Get_IPv6_Enabled
*       NU_Registry_Get_DHCP6_Enabled
*       NU_Registry_Get_Ipv4_Address
*       NU_Registry_Get_Ipv4_Netmask
*       NU_Registry_Get_Ipv6_Address
*       NU_Registry_Get_Ipv6_Prefix_Length
*
*    Interface configuration list queries.
*       NU_Ifconfig_Get_IPv4_Enabled
*       NU_Ifconfig_Get_DHCP4_Enabled
*       NU_Ifconfig_Set_DHCP4_Enabled
*       NU_Ifconfig_Get_IPv6_Enabled
*       NU_Ifconfig_Get_DHCP6_Enabled
*       NU_Ifconfig_Set_DHCP6_Enabled
*       NU_Ifconfig_Get_Ipv4_Address
*       NU_Ifconfig_Set_Ipv4_Address
*       NU_Ifconfig_Delete_Ipv4_Address
*       NU_Ifconfig_Get_Ipv6_Address
*       NU_Ifconfig_Set_Ipv6_Address
*       NU_Ifconfig_Delete_Ipv6_Address
*
* DEPENDENCIES
*
*       nucleus.h
*       nu_net.h
*       netboot_query.h
*       reg_api.h
*
*************************************************************************/

#include "nucleus.h"
#include "networking/nu_net.h"
#include "networking/netboot_query.h"
#include "services/reg_api.h"

/***************/
/* Global data */
/***************/

/* Semaphore to protect internal data structures. */
NU_SEMAPHORE Ifconfig_Semaphore;

/* List of interface configuration structures. */
IFCONFIG_NODE_LIST Ifconfig_Node_List;

/**************************/
/* Static local functions */
/**************************/

/*************************************************************************
*
*   FUNCTION
*
*       add_to_query_key
*
*   DESCRIPTION
*
*       Concatenate a string to the query string if the resulting
*       string will fit in the given size
*
*   INPUTS
*
*       query_key_buf  The query buffer
*       size           The size of the query buffer (including space
*                      for the required NULL.)
*       keypart        string to add to the query.
*
*   RETURNS
*
*       A pointer to the query buffer.
*
*************************************************************************/
static CHAR *add_to_query_key(CHAR *query_key_buf, UINT16 size, CHAR *keypart)
{
    /* Add the key part to the query key if it will fit. */
    if ( (query_key_buf) && (keypart) &&
         (strlen(query_key_buf) + strlen(keypart) < size) )
    {
        strcat(query_key_buf, keypart);
    }

    return (query_key_buf);

} /* add_to_query_key */

/*********************/
/* Utility functions */
/*********************/

/*************************************************************************
*
* FUNCTION
*
*       Ifconfig_Init
*
* DESCRIPTION
*
*       This function initializes the interface configuration module.
*       It initializes all the internal data structures.
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       NU_SUCCESS              If the processing is successful.
*
*************************************************************************/
STATUS Ifconfig_Init(VOID)
{
    STATUS status;

    /* Create the semaphore to protect interface structures. */
    status = NU_Create_Semaphore(&Ifconfig_Semaphore, "IF_CFG",
                                 1, NU_FIFO);

    if (status == NU_SUCCESS)
    {
        /* Obtain semaphore. */
        status = NU_Obtain_Semaphore(&Ifconfig_Semaphore, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Initialize the interface configuration list head */
            Ifconfig_Node_List.if_flink = NU_NULL;
            Ifconfig_Node_List.if_blink = NU_NULL;

            /* Release semaphore. */
            if (NU_Release_Semaphore(&Ifconfig_Semaphore) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

    return (status);

} /* Ifconfig_Init */

/*************************************************************************
*
* FUNCTION
*
*       Ifconfig_Deinit
*
* DESCRIPTION
*
*       This function de-initializes the interface configuration module.
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       NU_SUCCESS              If the processing is successful.
*       NU_INVALID_SEMAPHORE    Indicates the semaphore pointer is
*                               invalid.
*       NU_INVALID_SUSPEND      Indicates that suspend attempted from a
*                               non-task thread.
*
*************************************************************************/
STATUS Ifconfig_Deinit(VOID)
{
    STATUS status;

    /* Obtain semaphore. */
    status = NU_Obtain_Semaphore(&Ifconfig_Semaphore, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Remove all the interfaces. */
        if (Ifconfig_Remove_All_Interfaces()!= NU_SUCCESS)
        {
            NLOG_Error_Log("Failure in Ifconfig_Remove_All_Interfaces()",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Release semaphore. */
        if (NU_Release_Semaphore(&Ifconfig_Semaphore) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release semaphore",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Delete the semaphore now. */
        if (NU_Delete_Semaphore(&Ifconfig_Semaphore) != NU_SUCCESS)
        {
            NLOG_Error_Log("Unable to delete semaphore",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    return (status);

} /* Ifconfig_Deinit */

/*************************************************************************
*
* FUNCTION
*
*       Ifconfig_Remove_All_Interfaces
*
* DESCRIPTION
*
*       This utility function removes all Interfaces from the
*       list and deallocates their resources. It is called
*       when the component is being de-initialized.
*
*       This is an internal utility function. The  semaphore
*       should be obtained when this is called.
*
* INPUTS
*
*       None
*
* OUTPUTS
*
*       NU_SUCCESS              If the processing is successful.
*
*************************************************************************/
STATUS Ifconfig_Remove_All_Interfaces(VOID)
{
    IFCONFIG_NODE       *ifconfig_node_ptr;
    DEV_IF_ADDR_ENTRY   *addr_ptr;

    /* Get the first interface in the list. */
    ifconfig_node_ptr = DLL_Dequeue(&Ifconfig_Node_List);

    /* Deallocate memory for each interface structure. */
    while (ifconfig_node_ptr)
    {
        /* Get the first address structure in the list. */
        addr_ptr = DLL_Dequeue(&ifconfig_node_ptr->ifconfig_addr_list);

        /* Deallocate memory for each address structure. */
        while (addr_ptr)
        {
            if (NU_Deallocate_Memory(addr_ptr) != NU_SUCCESS)
            {
                NLOG_Error_Log("Unable to deallocate memory",
                               NERR_SEVERE, __FILE__, __LINE__);
            }

            /* Get the next address structure in the list. */
            addr_ptr = DLL_Dequeue(&ifconfig_node_ptr->ifconfig_addr_list);
        }

        /* And finally, deallocate memory of this interface item. */
        if (NU_Deallocate_Memory(ifconfig_node_ptr) != NU_SUCCESS)
        {
            NLOG_Error_Log("Unable to deallocate memory",
                           NERR_SEVERE, __FILE__, __LINE__);
        }

        /* Get the next interface in the list. */
        ifconfig_node_ptr = DLL_Dequeue(&Ifconfig_Node_List);
    }

    Ifconfig_Node_List.if_flink = NU_NULL;
    Ifconfig_Node_List.if_blink = NU_NULL;

    return (NU_SUCCESS);

} /* Ifconfig_Remove_All_Interfaces */

/****************************/
/* API functions start here */
/****************************/

/*************************************************************************
*
* FUNCTION
*
*       Ifconfig_Find_Interface
*
* DESCRIPTION
*
*       Finds an interface-configuration-node by its name and returns a
*       pointer to it.
*
* INPUTS
*
*       *name                   Name of the Interface.
*       **ifconfig_node_ptr     A double pointer to return a pointer to
*                               the found interface-configuration-node.
*
* RETURNS
*
*       NU_SUCCESS              If the processing is successful.
*
*************************************************************************/
STATUS Ifconfig_Find_Interface(const CHAR *name,
                               IFCONFIG_NODE **ifconfig_node_ptr)
{
    STATUS          status = -1;
    IFCONFIG_NODE   *int_ptr;

    /* Initially set return pointer to NU_NULL. */
    *ifconfig_node_ptr = NU_NULL;

    int_ptr = Ifconfig_Node_List.if_flink;

    /* Loop for all interface-configuration-nodes in the list. */
    while (int_ptr != NU_NULL)
    {
        /* If a match is found. */
        if (strcmp(int_ptr->if_name, name) == 0)
        {
            status = NU_SUCCESS;
            break;
        }

        int_ptr = int_ptr->if_flink;
    }

    /* Set the return pointer value (or NU_NULL). */
    *ifconfig_node_ptr = int_ptr;

    return (status);

} /* Ifconfig_Find_Interface */

/*************************************************************************
*
* FUNCTION
*
*       NU_Ifconfig_Validate_Interface
*
* DESCRIPTION
*
*       This function checks if the interface is valid in the system.
*
* INPUTS
*
*       *name                   Name of the Interface.
*
* RETURNS
*
*       NU_TRUE					The interface is valid.
*       NU_FALSE				The interface does not exist.
*
*************************************************************************/
BOOLEAN NU_Ifconfig_Validate_Interface(CHAR *name)
{
    BOOLEAN			ret_val = NU_FALSE;
    STATUS			status;
    IFCONFIG_NODE   *int_ptr;

    /* Obtain semaphore. */
    status = NU_Obtain_Semaphore(&Ifconfig_Semaphore, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
    	/* If the interface can be found, return NU_TRUE. */
    	if (Ifconfig_Find_Interface(name, &int_ptr) == NU_SUCCESS)
    		ret_val = NU_TRUE;

        /* Release semaphore. */
        if (NU_Release_Semaphore(&Ifconfig_Semaphore) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release semaphore",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    return (ret_val);

} /* NU_Ifconfig_Validate_Interface */

/*************************************************************************
*
* FUNCTION
*
*       NU_Ifconfig_Update_Interface
*
* DESCRIPTION
*
*       If an interface-configuration-node by the specified name already
*       exists in the list, then update it with the contents of the given
*       interface-configuration-node.

*       Otherwise, append a new interface-configuration-node for the
*       named interface to the end of the interface-configuration-node
*       list.
*
* INPUTS
*
*       *name                   Name of the Interface.
*       *ifconfig_node_ptr      points to the configuration information
*                               for the named interface.
*
* RETURNS
*
*       NU_SUCCESS              If the processing is successful.
*
*************************************************************************/
STATUS NU_Ifconfig_Update_Interface(CHAR *name,
                                    IFCONFIG_NODE *ifconfig_node_ptr)
{
    STATUS          status =-1;
    IFCONFIG_NODE   *int_ptr;

    /* Obtain semaphore. */
    status = NU_Obtain_Semaphore(&Ifconfig_Semaphore, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        int_ptr = Ifconfig_Node_List.if_flink;

        /* Loop for all interface-configuration-nodes in the list. */
        while (int_ptr != NU_NULL)
        {
            /* If a match is found. */
            if (strcmp(int_ptr->if_name, name) == 0)
            {
                break;
            }

            int_ptr = int_ptr->if_flink;
        }

        /********************************************************/
        /* If a match was not found add a new node to the list. */
        /********************************************************/
        if (int_ptr == NU_NULL)
        {
            /* Allocate memory for the new interface-configuration-node. */
            status = NU_Allocate_Memory(MEM_Cached, (VOID**)&int_ptr,
                                        sizeof(IFCONFIG_NODE) +
                                        strlen(name) + 1, NU_NO_SUSPEND);

            if (status == NU_SUCCESS)
            {
                /* Zero out the allocated memory. */
                memset(int_ptr, 0, sizeof(IFCONFIG_NODE));

                /* Add the new interface-configuration-node to the
                 * interface-configuration-nodes list.
                 */
                DLL_Enqueue(&Ifconfig_Node_List, int_ptr);
            }

            else
            {
                int_ptr = NU_NULL;
            }
        }

        /* Either we found an existing node or we added a new node.
         * Fill in the contents of the interface-configuration-node.
         */
        if (int_ptr != NU_NULL)
        {
            UINT8   *dst;
            UINT8   *src;
            UINT32  len;

            /* Here we want to copy the data from the input node to the
             * node we found or created.  In either case we don't want
             * to overwrite the 2 list pointers which are at the begining
             * of the structure.
             */

            dst = (UINT8 *)int_ptr           +(2*sizeof(UINT *));
            src = (UINT8 *)ifconfig_node_ptr +(2*sizeof(UINT *));
            len = sizeof(IFCONFIG_NODE)      -(2*sizeof(UINT *));

            /* Update the node on the list, but don't overwrite the
             * pointers at the beginning.
             */
            memcpy(dst, src, len);

            status = NU_SUCCESS;
        }

        /* Release semaphore. */
        if (NU_Release_Semaphore(&Ifconfig_Semaphore) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release semaphore",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }

    return (status);

} /* NU_Ifconfig_Update_Interface */

/*************************************************************************
*
* FUNCTION
*
*       NU_Ifconfig_Delete_Interface
*
* DESCRIPTION
*
*       Finds an Interface by its name and deletes it.  This both removes
*       it from the list and frees its memory.
*
* INPUTS
*
*       *name                   Name of the Interface.
*
* OUTPUTS
*
*       NU_SUCCESS              If the processing is successful.
*
*************************************************************************/
STATUS NU_Ifconfig_Delete_Interface(const CHAR *name)
{
    STATUS              status = -1;
    IFCONFIG_NODE       *ifconfig_node_ptr;
    DEV_IF_ADDR_ENTRY   *addr_ptr;

    /* Obtain semaphore. */
    status = NU_Obtain_Semaphore(&Ifconfig_Semaphore, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        ifconfig_node_ptr = Ifconfig_Node_List.if_flink;

        /* Search the list for the target interface. */
        while (ifconfig_node_ptr)
        {
            /* If a match is found. */
            if (strcmp(ifconfig_node_ptr->if_name, name) == 0)
            {
                /* Remove this interface from the list. */
                DLL_Remove(&Ifconfig_Node_List, ifconfig_node_ptr);

                /* Get the first address structure in the list. */
                addr_ptr = DLL_Dequeue(&ifconfig_node_ptr->ifconfig_addr_list);

                /* Deallocate memory for each address structure. */
                while (addr_ptr)
                {
                    if (NU_Deallocate_Memory(addr_ptr) != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Unable to deallocate memory",
                                       NERR_SEVERE, __FILE__, __LINE__);
                    }

                    /* Get the next address structure in the list. */
                    addr_ptr = DLL_Dequeue(&ifconfig_node_ptr->ifconfig_addr_list);
                }

                /* Deallocate the memory for this interface. */
                if (NU_Deallocate_Memory(ifconfig_node_ptr) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to deallocate memory for interface",
                                   NERR_SEVERE, __FILE__, __LINE__);
                }

                status = NU_SUCCESS;
                break;
            }

            ifconfig_node_ptr = ifconfig_node_ptr->if_flink;
        }

        /* Release semaphore. */
        if (NU_Release_Semaphore(&Ifconfig_Semaphore) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release semaphore",
               NERR_SEVERE, __FILE__, __LINE__);
        }

    }

    return (status);

} /* NU_Ifconfig_Delete_Interface */

/*************************************************************************
*
*   FUNCTION
*
*       NU_Ifconfig_Set_Interface_Defaults
*
*   DESCRIPTION
*
*       Given a path to configuration settings, add a node to the
*       local list of interface configuration nodes.
*
*       If an interface-configuration-node for the named interface
*       already exists in the list, then it will be updated.  Otherwise,
*       a new node will be created and appended to the the list.
*
*       The configuration information will be set to the default values
*       from the registry.
*
*   INPUTS
*
*       if_name        Name of the interface.
*       path           Registry path for the interface.
*
*   RETURNS
*
*       STATUS
*
*************************************************************************/
STATUS NU_Ifconfig_Set_Interface_Defaults(CHAR *if_name, CHAR *path)
{
    STATUS              status = -1;
    UINT8               bytes[MAX_ADDRESS_SIZE] = {0};
    IFCONFIG_NODE       ifconfig;
    DEV_IF_ADDR_ENTRY   *new_entry;
    UINT8               value;

    /* Zero out the allocated memory. */
    memset(&ifconfig, 0, sizeof(IFCONFIG_NODE));

    if ( (if_name) && (path) )
    {
        /* Interface Name */
        strncpy(ifconfig.if_name, if_name, DEV_NAME_LENGTH);
        ifconfig.if_name[DEV_NAME_LENGTH-1] = 0;

        /* IPv4 enabled */
        ifconfig.IPv4_Enabled = NU_Registry_Get_IPv4_Enabled(path);

        /* DHCPv4 enabled */
        ifconfig.DHCP4_Enabled = NU_Registry_Get_DHCP4_Enabled(path);

        /* IPv6 enabled */
        ifconfig.IPv6_Enabled = NU_Registry_Get_IPv6_Enabled(path);

        /* DHCPv6 enabled */
        ifconfig.DHCP6_Enabled = NU_Registry_Get_DHCP6_Enabled(path);

        /* Set the head and tail of the address list to NULL. */
        ifconfig.ifconfig_addr_list.dv_head = NU_NULL;
        ifconfig.ifconfig_addr_list.dv_tail = NU_NULL;

        /* If IPv4 is enabled on the interface. */
        if (ifconfig.IPv4_Enabled)
        {
            /* Get the IPv4 address from the registry. */
            status = NU_Registry_Get_Ipv4_Address(path, bytes);

            if (status == NU_SUCCESS)
            {
                /* Allocate memory for the address list entry */
                status = NU_Allocate_Memory(MEM_Cached,
                                            (VOID**)&new_entry,
                                            sizeof(DEV_IF_ADDR_ENTRY), NU_NO_SUSPEND);

                if (status == NU_SUCCESS)
                {
                    /* Zero out the memory */
                    UTL_Zero(new_entry, sizeof(DEV_IF_ADDR_ENTRY));

                    /* Save the IPv4 address. */
                    new_entry->dev_entry_ip_addr = IP_ADDR(bytes);

                    /* Get the IPv4 subnet mask from the registry. */
                    status = NU_Registry_Get_Ipv4_Netmask(path, bytes);

                    if (status == NU_SUCCESS)
                    {
                        /* Save the IPv4 subnet mask. */
                        new_entry->dev_entry_netmask = IP_ADDR(bytes);

                        /* Add the entry to the head of the list. */
                        DLL_Enqueue(&ifconfig.ifconfig_addr_list, new_entry);
                    }

                    /* Deallocate the address structure. */
                    else
                    {
                        NU_Deallocate_Memory(new_entry);
                    }
                }
            }
        }

        /* If IPv6 is enabled on the interface. */
        if (ifconfig.IPv6_Enabled)
        {
            /* IPv6 address */
            status = NU_Registry_Get_Ipv6_Address(path, bytes);

            if (status == NU_SUCCESS)
            {
                memcpy(ifconfig.Ipv6_Address, bytes, MAX_ADDRESS_SIZE);
            }

            /* IPv6 prefix length */
            status = NU_Registry_Get_Ipv6_Prefix_Length(path, &value);

            if (status == NU_SUCCESS)
            {
                if ( (value >= 1) && (value <= 128) )
                {
                    ifconfig.Ipv6_Prefix_Length = value;
                }
            }
        }

        /* Add this interface configuration node to the list. */
        status = NU_Ifconfig_Update_Interface(if_name, &ifconfig);
    }

    return (status);

} /* NU_Ifconfig_Set_Interface_Defaults */

/**********************************************************/
/* Query the Read-only registry for the requested values. */
/**********************************************************/

/*************************************************************************
*
*   FUNCTION
*
*       NU_Registry_Get_IPv4_Enabled
*
*   DESCRIPTION
*
*       Return NU_TRUE if IPv4 is enabled.
*
*   INPUTS
*
*       path           Registry path for the interface.
*
*   RETURNS
*
*       Boolean        TRUE or FALSE
*
*************************************************************************/
UINT32 NU_Registry_Get_IPv4_Enabled(CHAR *path)
{
    UINT32     rcode = NU_FALSE;
    UINT8      value = 0;
    STATUS     reg_status;
    CHAR       query_key_buf[256];     /* registry query string buffer. */

    if (path)
    {
        /* Start with a zero length string */
        query_key_buf[0] = 0;
        add_to_query_key(query_key_buf, sizeof(query_key_buf), path);
        add_to_query_key(query_key_buf, sizeof(query_key_buf),"/");
        add_to_query_key(query_key_buf, sizeof(query_key_buf),"eth_enable_ipv4");

        reg_status = REG_Get_Boolean(query_key_buf, &value);

        if (reg_status == NU_SUCCESS)
        {
            rcode = value;
        }
    }
    return (rcode);

} /* NU_Registry_Get_IPv4_Enabled */

/*************************************************************************
*
*   FUNCTION
*
*       NU_Registry_Get_DHCP4_Enabled
*
*   DESCRIPTION
*
*       Return NU_TRUE if DHCP4 is enabled.
*
*   INPUTS
*
*       path           Registry path for the interface.
*
*   RETURNS
*
*       Boolean        TRUE or FALSE
*
*************************************************************************/
UINT32 NU_Registry_Get_DHCP4_Enabled(CHAR *path)
{
    UINT32     rcode = NU_FALSE;
    UINT8      value = 0;
    STATUS     reg_status;
    CHAR       query_key_buf[256];     /* registry query string buffer. */

    if (path)
    {
        /* Start with a zero length string */
        query_key_buf[0] = 0;
        add_to_query_key(query_key_buf, sizeof(query_key_buf),path);
        add_to_query_key(query_key_buf, sizeof(query_key_buf),"/");
        add_to_query_key(query_key_buf, sizeof(query_key_buf),"eth_enable_ipv4_dhcp");

        reg_status = REG_Get_Boolean(query_key_buf, &value);

        if (reg_status == NU_SUCCESS)
        {
            rcode = value;
        }
    }
    return (rcode);

} /* NU_Registry_Get_DHCP4_Enabled */

/*************************************************************************
*
*   FUNCTION
*
*       NU_Registry_Get_IPv6_Enabled
*
*   DESCRIPTION
*
*       Return NU_TRUE if Include IPv6 is enabled.
*
*   INPUTS
*
*       path           Registry path for the interface.
*
*   RETURNS
*
*       Boolean        TRUE or FALSE
*
*************************************************************************/
UINT32 NU_Registry_Get_IPv6_Enabled(CHAR *path)
{
    UINT32     rcode = NU_FALSE;
    UINT8      value = 0;
    STATUS     reg_status;
    CHAR       query_key_buf[256];     /* registry query string buffer. */

    if (path)
    {
        /* Start with a zero length string */
        query_key_buf[0] = 0;
        add_to_query_key(query_key_buf, sizeof(query_key_buf),path);
        add_to_query_key(query_key_buf, sizeof(query_key_buf),"/");
        add_to_query_key(query_key_buf, sizeof(query_key_buf),"eth_enable_ipv6");

        reg_status = REG_Get_Boolean(query_key_buf, &value);

        if (reg_status == NU_SUCCESS)
        {
            rcode = value;
        }
    }
    return (rcode);

} /* NU_Registry_Get_IPv6_Enabled */

/*************************************************************************
*
*   FUNCTION
*
*       NU_Registry_Get_DHCP6_Enabled
*
*   DESCRIPTION
*
*       Return NU_TRUE if DHCP6 is enabled.
*
*   INPUTS
*
*       path           Registry path for the interface.
*
*   RETURNS
*
*       Boolean        TRUE or FALSE
*
*************************************************************************/
UINT32 NU_Registry_Get_DHCP6_Enabled(CHAR *path)
{
    UINT32     rcode = NU_FALSE;
    UINT8      value = 0;
    STATUS     reg_status;
    CHAR       query_key_buf[256];     /* registry query string buffer. */

    if (path)
    {
        /* Start with a zero length string */
        query_key_buf[0] = 0;
        add_to_query_key(query_key_buf, sizeof(query_key_buf),path);
        add_to_query_key(query_key_buf, sizeof(query_key_buf),"/");
        add_to_query_key(query_key_buf, sizeof(query_key_buf),"eth_enable_ipv6_dhcp");

        reg_status = REG_Get_Boolean(query_key_buf, &value);

        if (reg_status == NU_SUCCESS)
        {
            rcode = value;
        }
    }
    return (rcode);

} /* NU_Registry_Get_DHCP6_Enabled */

/*************************************************************************
*
*   FUNCTION
*
*       NU_Registry_Get_Ipv4_Address
*
*   DESCRIPTION
*
*       Get the IPv4 address from the registry.
*
*   INPUTS
*
*       path           Registry path for the interface.
*       ipv4_addr      Pointer to IPv4 address buffer.
*
*   RETURNS
*
*       STATUS
*
*************************************************************************/
STATUS NU_Registry_Get_Ipv4_Address(CHAR *path, UINT8 *ipv4_addr)
{
    STATUS     status = -1;
    STATUS     reg_status;
    CHAR       buffer[IPV4_ASCII_BUFSIZE] = {0};
    UINT8      bytes[IP_ADDR_LEN] = {0};
    CHAR       query_key_buf[256];     /* registry query string buffer. */

    if (path && ipv4_addr)
    {
        /* Start with a zero length string */
        query_key_buf[0] = 0;
        add_to_query_key(query_key_buf, sizeof(query_key_buf),path);
        add_to_query_key(query_key_buf, sizeof(query_key_buf),"/");
        add_to_query_key(query_key_buf, sizeof(query_key_buf),"eth_ipv4_address");

        reg_status = REG_Get_String(query_key_buf, buffer, IPV4_ASCII_BUFSIZE);

        if (reg_status == NU_SUCCESS)
        {
            status = NU_Inet_PTON(SK_FAM_IP, buffer, (VOID *)bytes);

            /* Ensure the address was converted successfully and the address
             * is not all zeros.
             */
            if ( (status == NU_SUCCESS) && (IP_ADDR(bytes) != 0) )
            {
                memcpy(ipv4_addr, bytes, IP_ADDR_LEN);
            }

            else
            {
                status = -1;
            }
        }
    }
    return (status);

} /* NU_Registry_Get_Ipv4_Address */

/*************************************************************************
*
*   FUNCTION
*
*       NU_Registry_Get_Ipv4_Netmask
*
*   DESCRIPTION
*
*       Get the IPv4 netmask from the registry.
*
*   INPUTS
*
*       path           Registry path for the interface.
*       ipv4_netmask   Pointer to IPv4 netmask buffer.
*
*   RETURNS
*
*       STATUS
*
*************************************************************************/
STATUS NU_Registry_Get_Ipv4_Netmask(CHAR *path, UINT8 *ipv4_netmask)
{
    STATUS     status = -1;
    STATUS     reg_status;
    CHAR       buffer[IPV4_ASCII_BUFSIZE] = {0};
    UINT8      bytes[IP_ADDR_LEN] = {0};
    CHAR       query_key_buf[256];     /* registry query string buffer. */

    if ( (path) && (ipv4_netmask) )
    {
        /* Start with a zero length string */
        query_key_buf[0] = 0;
        add_to_query_key(query_key_buf, sizeof(query_key_buf),path);
        add_to_query_key(query_key_buf, sizeof(query_key_buf),"/");
        add_to_query_key(query_key_buf, sizeof(query_key_buf),"eth_ipv4_netmask");

        reg_status = REG_Get_String(query_key_buf, buffer, IPV4_ASCII_BUFSIZE);

        if (reg_status == NU_SUCCESS)
        {
            status = NU_Inet_PTON(SK_FAM_IP, buffer, (VOID *)bytes);

            if (status == NU_SUCCESS)
            {
                memcpy(ipv4_netmask, bytes, IP_ADDR_LEN);
            }
        }
    }
    return (status);

} /* NU_Registry_Get_Ipv4_Netmask */

/*************************************************************************
*
*   FUNCTION
*
*       NU_Registry_Get_Ipv6_Address
*
*   DESCRIPTION
*
*       Get the IPv6 address from the registry.
*
*   INPUTS
*
*       path           Registry path for the interface.
*       ipv6_addr      Pointer to IPv6 address buffer.
*
*   RETURNS
*
*       STATUS
*
*************************************************************************/
STATUS NU_Registry_Get_Ipv6_Address(CHAR *path, UINT8 *local_ipv6)
{
    STATUS     status = -1;
    STATUS     reg_status;
    CHAR       buffer[IPV6_ASCII_BUFSIZE] = {0};
    UINT8      bytes[MAX_ADDRESS_SIZE] = {0};
    CHAR       query_key_buf[256];     /* registry query string buffer. */

    if ( (path) && (local_ipv6) )
    {
        /* Start with a zero length string */
        query_key_buf[0] = 0;
        add_to_query_key(query_key_buf, sizeof(query_key_buf),path);
        add_to_query_key(query_key_buf, sizeof(query_key_buf),"/");
        add_to_query_key(query_key_buf, sizeof(query_key_buf),"eth_ipv6_address");

        reg_status = REG_Get_String(query_key_buf, buffer, IPV6_ASCII_BUFSIZE);

        if (reg_status == NU_SUCCESS)
        {
            status = NU_Inet_PTON(SK_FAM_IP6, buffer, (VOID *)bytes);

            if (status == NU_SUCCESS)
            {
                memcpy(local_ipv6, bytes, MAX_ADDRESS_SIZE);
            }
        }
    }

    return (status);

} /* NU_Registry_Get_Ipv6_Address */

/*************************************************************************
*
*   FUNCTION
*
*       NU_Registry_Get_Ipv6_Prefix_Length
*
*   DESCRIPTION
*
*       Get the IPv6 prefix length from the registry.
*
*   INPUTS
*
*       path           Registry path for the interface.
*       prefix_len     Pointer to the prefix length variable.
*
*   RETURNS
*
*       STATUS
*
*************************************************************************/
STATUS NU_Registry_Get_Ipv6_Prefix_Length(CHAR *path, UINT8 *prefix_len)
{
    STATUS     status = -1;
    STATUS     reg_status;
    CHAR       query_key_buf[256];     /* registry query string buffer. */

    if ( (path) && (prefix_len) )
    {
        /* Start with a zero length string */
        query_key_buf[0] = 0;
        add_to_query_key(query_key_buf, sizeof(query_key_buf), path);
        add_to_query_key(query_key_buf, sizeof(query_key_buf), "/");
        add_to_query_key(query_key_buf, sizeof(query_key_buf), "eth_ipv6_prefix_length");

        reg_status = REG_Get_UINT8(query_key_buf, prefix_len);

        if (reg_status == NU_SUCCESS)
        {
            if ((*prefix_len >= 1) && (*prefix_len <= 128))
            {
                status = NU_SUCCESS;
            }
        }
    }

    return (status);

} /* NU_Registry_Get_Ipv6_Prefix_Length */

/***********************************************************/
/* Get and Set values in the Interface Configuration List. */
/***********************************************************/

/*************************************************************************
*
*   FUNCTION
*
*       NU_Ifconfig_Get_IPv4_Enabled
*
*   DESCRIPTION
*
*       Return NU_TRUE if IPv4 is enabled for the named interface.
*
*   INPUTS
*
*       if_name        Name of the interface.
*
*   RETURNS
*
*       Boolean        TRUE or FALSE
*
*************************************************************************/
UINT32 NU_Ifconfig_Get_IPv4_Enabled(CHAR *if_name)
{
    STATUS         status = -1;
    UINT32         rcode  = 0;
    IFCONFIG_NODE  *ifconfig_node_ptr;

    if (if_name)
    {
        /* Obtain semaphore. */
        status = NU_Obtain_Semaphore(&Ifconfig_Semaphore, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            status = Ifconfig_Find_Interface(if_name, &ifconfig_node_ptr);

            if ( (status == NU_SUCCESS) && (ifconfig_node_ptr != NU_NULL) )
            {
                rcode = ifconfig_node_ptr->IPv4_Enabled;
            }

            /* Release semaphore. */
            if (NU_Release_Semaphore(&Ifconfig_Semaphore) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

    return (rcode);

} /* NU_Ifconfig_Get_IPv4_Enabled */

/*************************************************************************
*
*   FUNCTION
*
*       NU_Ifconfig_Get_DHCP4_Enabled
*
*   DESCRIPTION
*
*       Return NU_TRUE if DHCP4 is enabled for the named interface.
*
*   INPUTS
*
*       if_name        Name of the interface.
*
*   RETURNS
*
*       Boolean        TRUE or FALSE
*
*************************************************************************/
UINT32 NU_Ifconfig_Get_DHCP4_Enabled(CHAR *if_name)
{
    STATUS         status = -1;
    UINT32         rcode  = 0;
    IFCONFIG_NODE  *ifconfig_node_ptr;

    if (if_name)
    {
        /* Obtain semaphore. */
        status = NU_Obtain_Semaphore(&Ifconfig_Semaphore, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            status = Ifconfig_Find_Interface(if_name, &ifconfig_node_ptr);

            if ( (status == NU_SUCCESS) && (ifconfig_node_ptr != NU_NULL) )
            {
                rcode = ifconfig_node_ptr->DHCP4_Enabled;
            }

            /* Release semaphore. */
            if (NU_Release_Semaphore(&Ifconfig_Semaphore) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

    return (rcode);

} /* NU_Ifconfig_Get_DHCP4_Enabled */

/*************************************************************************
*
*   FUNCTION
*
*       NU_Ifconfig_Set_DHCP4_Enabled
*
*   DESCRIPTION
*
*       Set the DHCP4 enabled flag for the named interface in the
*       ifconfig list.
*
*   INPUTS
*
*       if_name        Name of the interface.
*       value          Boolean value to set
*
*   RETURNS
*
*       STATUS
*
*************************************************************************/
STATUS NU_Ifconfig_Set_DHCP4_Enabled(const CHAR *if_name, UINT8 value)
{
    STATUS         status = -1;
    IFCONFIG_NODE  *ifconfig_node_ptr;

    if (if_name)
    {
        /* Obtain semaphore. */
        status = NU_Obtain_Semaphore(&Ifconfig_Semaphore, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            status = Ifconfig_Find_Interface(if_name, &ifconfig_node_ptr);

            if ( (status == NU_SUCCESS) && (ifconfig_node_ptr != NU_NULL) )
            {
                ifconfig_node_ptr->DHCP4_Enabled = value;
                status = NU_SUCCESS;
            }

            /* Release semaphore. */
            if (NU_Release_Semaphore(&Ifconfig_Semaphore) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

    return (status);

} /* NU_Ifconfig_Set_DHCP4_Enabled */

/*************************************************************************
*
*   FUNCTION
*
*       NU_Ifconfig_Get_IPv6_Enabled
*
*   DESCRIPTION
*
*       Return NU_TRUE if IPv6 is enabled for the named interface.
*
*   INPUTS
*
*       if_name        Name of the interface.
*
*   RETURNS
*
*       Boolean        TRUE or FALSE
*
*************************************************************************/
UINT32 NU_Ifconfig_Get_IPv6_Enabled(CHAR *if_name)
{
    STATUS         status = -1;
    UINT32         rcode  = 0;
    IFCONFIG_NODE  *ifconfig_node_ptr;

    if (if_name)
    {
        /* Obtain semaphore. */
        status = NU_Obtain_Semaphore(&Ifconfig_Semaphore, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            status = Ifconfig_Find_Interface(if_name, &ifconfig_node_ptr);

            if ( (status == NU_SUCCESS) && (ifconfig_node_ptr != NU_NULL) )
            {
                rcode = ifconfig_node_ptr->IPv6_Enabled;
            }

            /* Release semaphore. */
            if (NU_Release_Semaphore(&Ifconfig_Semaphore) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

    return (rcode);

} /* NU_Ifconfig_Get_IPv6_Enabled */

/*************************************************************************
*
*   FUNCTION
*
*       NU_Ifconfig_Get_DHCP6_Enabled
*
*   DESCRIPTION
*
*       Return NU_TRUE if DHCP6 is enabled for the named interface.
*
*   INPUTS
*
*       if_name        Name of the interface.
*
*   RETURNS
*
*       Boolean        TRUE or FALSE
*
*************************************************************************/
UINT32 NU_Ifconfig_Get_DHCP6_Enabled(CHAR *if_name)
{
    STATUS         status = -1;
    UINT32         rcode  = 0;
    IFCONFIG_NODE  *ifconfig_node_ptr;

    if (if_name)
    {
        /* Obtain semaphore. */
        status = NU_Obtain_Semaphore(&Ifconfig_Semaphore, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            status = Ifconfig_Find_Interface(if_name, &ifconfig_node_ptr);

            if ( (status == NU_SUCCESS) && (ifconfig_node_ptr != NU_NULL) )
            {
                rcode = ifconfig_node_ptr->DHCP6_Enabled;
            }

            /* Release semaphore. */
            if (NU_Release_Semaphore(&Ifconfig_Semaphore) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

    return (rcode);

} /* NU_Ifconfig_Get_DHCP6_Enabled */

/*************************************************************************
*
*   FUNCTION
*
*       NU_Ifconfig_Set_DHCP6_Enabled
*
*   DESCRIPTION
*
*       Set the DHCP6 enabled flag for the named interface in the
*       ifconfig list.
*
*   INPUTS
*
*       if_name                 Name of the interface.
*       value                   The new value to set.
*
*   RETURNS
*
*       STATUS
*
*************************************************************************/
STATUS NU_Ifconfig_Set_DHCP6_Enabled(CHAR *if_name, UINT8 value)
{
    STATUS         status = -1;
    IFCONFIG_NODE  *ifconfig_node_ptr;

    if (if_name)
    {
        /* Obtain semaphore. */
        status = NU_Obtain_Semaphore(&Ifconfig_Semaphore, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            status = Ifconfig_Find_Interface(if_name, &ifconfig_node_ptr);

            if ( (status == NU_SUCCESS) && (ifconfig_node_ptr != NU_NULL) )
            {
                ifconfig_node_ptr->DHCP6_Enabled = value;
                status = NU_SUCCESS;
            }

            /* Release semaphore. */
            if (NU_Release_Semaphore(&Ifconfig_Semaphore) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

    return (status);

} /* NU_Ifconfig_Set_DHCP6_Enabled */

/*************************************************************************
*
*   FUNCTION
*
*       NU_Ifconfig_Get_Ipv4_Address
*
*   DESCRIPTION
*
*       Get the IPv4 address and corresponding subnet mask for the named
*       interface from the interface configuration list.  To get the first
*       address/subnet combination, pass in all zero's for ipv4_addr and
*       subnet_mask.  For subsequent addresses, pass in the previously
*       retrieved address/subnet mask.
*
*   INPUTS
*
*       if_name                 Name of the interface.
*       ipv4_addr               Pointer to IPv4 address buffer.
*       subnet_mask             Pointer to the subnet mask buffer.
*
*   RETURNS
*
*       NU_SUCCESS              The address was found.
*       -1                      Either the interface does not exist or
*                               there is no address configured for the
*                               interface.
*
*************************************************************************/
STATUS NU_Ifconfig_Get_Ipv4_Address(CHAR *if_name, UINT8 *ipv4_addr,
                                    UINT8 *subnet_mask)
{
    STATUS              status = -1;
    IFCONFIG_NODE       *ifconfig_node_ptr;
    DEV_IF_ADDR_ENTRY   *addr_ptr;
    UINT32              ipv4_addr_uint32, subnet_mask_uint32;

    /* Validate the input parameters. */
    if ( (if_name) && (ipv4_addr) && (subnet_mask) )
    {
        /* Obtain semaphore. */
        status = NU_Obtain_Semaphore(&Ifconfig_Semaphore, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Get a pointer to the interface configuration entry for this
             * interface.
             */
            status = Ifconfig_Find_Interface(if_name, &ifconfig_node_ptr);

            /* If the matching interface was found, and there is an address configured
             * on it.
             */
            if ( (status == NU_SUCCESS) && (ifconfig_node_ptr != NU_NULL) &&
                 (ifconfig_node_ptr->ifconfig_addr_list.dv_head) )
            {
                /* Convert the values to 32-bit integers. */
                ipv4_addr_uint32 = IP_ADDR(ipv4_addr);
                subnet_mask_uint32 = IP_ADDR(subnet_mask);

                /* If the first address is desired. */
                if (ipv4_addr_uint32 == 0)
                {
                    /* Copy the first IPv4 address. */
                    PUT32(ipv4_addr, 0, ifconfig_node_ptr->ifconfig_addr_list.
                          dv_head->dev_entry_ip_addr);

                    /* Copy the corresponding subnet mask. */
                    PUT32(subnet_mask, 0, ifconfig_node_ptr->ifconfig_addr_list.
                          dv_head->dev_entry_netmask);
                }

                else
                {
                    /* Get the first address structure in the list. */
                    addr_ptr = ifconfig_node_ptr->ifconfig_addr_list.dv_head;

                    /* Search for the entry after the entry that was passed in. */
                    while (addr_ptr)
                    {
                        /* If this is the last address returned. */
                        if ( (addr_ptr->dev_entry_ip_addr == ipv4_addr_uint32) &&
                             (addr_ptr->dev_entry_netmask == subnet_mask_uint32) )
                        {
                            break;
                        }

                        /* Get the next address structure in the list. */
                        addr_ptr = addr_ptr->dev_entry_next;
                    }

                    /* If the last address returned was found, and there is a next
                     * address.
                     */
                    if ( (addr_ptr) && (addr_ptr->dev_entry_next) )
                    {
                        /* Copy the IPv4 address. */
                        PUT32(ipv4_addr, 0, addr_ptr->dev_entry_next->dev_entry_ip_addr);

                        /* Copy the corresponding subnet mask. */
                        PUT32(subnet_mask, 0, addr_ptr->dev_entry_next->dev_entry_netmask);
                    }

                    /* There are no more addresses. */
                    else
                    {
                        status = -1;
                    }
                }
            }

            /* No interface or no IP address. */
            else
            {
                status = -1;
            }

            /* Release semaphore. */
            if (NU_Release_Semaphore(&Ifconfig_Semaphore) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

    return (status);

} /* NU_Ifconfig_Get_Ipv4_Address */

/*************************************************************************
*
*   FUNCTION
*
*       NU_Ifconfig_Set_Ipv4_Address
*
*   DESCRIPTION
*
*       Set the IPv4 address for the named interface in the interface
*       configuration list.
*
*   INPUTS
*
*       if_name                 Name of the interface.
*       ipv4_addr               Pointer to IPv4 address buffer.
*       subnet_mask             The subnet mask of the new address.
*
*   RETURNS
*
*       Boolean                 TRUE or FALSE
*
*************************************************************************/
STATUS NU_Ifconfig_Set_Ipv4_Address(const CHAR *if_name, UINT8 *ipv4_addr,
                                    const UINT8 *subnet_mask)
{
    STATUS              status = -1;
    IFCONFIG_NODE       *ifconfig_node_ptr;
    DEV_IF_ADDR_ENTRY   *new_entry, *addr_ptr;
    UINT32              ipv4_addr_uint32, ipv4_mask_uint32;

    if ( (if_name) && (ipv4_addr) && (subnet_mask) )
    {
        /* Convert the addresses to 32-bit values for comparison below. */
        ipv4_addr_uint32 = IP_ADDR(ipv4_addr);
        ipv4_mask_uint32 = IP_ADDR(subnet_mask);

        /* Obtain semaphore. */
        status = NU_Obtain_Semaphore(&Ifconfig_Semaphore, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Get a pointer to the configuration structure for this interface. */
            status = Ifconfig_Find_Interface(if_name, &ifconfig_node_ptr);

            if ( (status == NU_SUCCESS) && (ifconfig_node_ptr != NU_NULL) )
            {
                /* Ensure this address has not already been set in the configuration. */
                for (addr_ptr = ifconfig_node_ptr->ifconfig_addr_list.dv_head;
                     addr_ptr != NU_NULL;
                     addr_ptr = addr_ptr->dev_entry_next)
                {
                    /* If the address and subnet mask match. */
                    if ( (addr_ptr->dev_entry_ip_addr == ipv4_addr_uint32) &&
                         (addr_ptr->dev_entry_netmask == ipv4_mask_uint32) )
                        break;
                }

                /* If the address is not already stored in the configuration. */
                if (addr_ptr == NU_NULL)
                {
                    /* Allocate memory for the address list entry */
                    status = NU_Allocate_Memory(MEM_Cached,
                                                (VOID**)&new_entry,
                                                sizeof(DEV_IF_ADDR_ENTRY), NU_NO_SUSPEND);

                    if (status == NU_SUCCESS)
                    {
                        /* Zero out the memory */
                        UTL_Zero(new_entry, sizeof(DEV_IF_ADDR_ENTRY));

                        /* Save the IPv4 address. */
                        new_entry->dev_entry_ip_addr = IP_ADDR(ipv4_addr);

                        if (status == NU_SUCCESS)
                        {
                            /* Save the IPv4 subnet mask. */
                            new_entry->dev_entry_netmask = IP_ADDR(subnet_mask);

                            if (status == NU_SUCCESS)
                            {
                                /* Add the entry to the head of the list. */
                                DLL_Enqueue(&ifconfig_node_ptr->ifconfig_addr_list, new_entry);
                            }
                        }
                    }
                }

                else
                {
                    /* Set status to success since the address already exists. */
                    status = NU_SUCCESS;
                }
            }

            /* Release semaphore. */
            if (NU_Release_Semaphore(&Ifconfig_Semaphore) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

    return (status);

} /* NU_Ifconfig_Set_Ipv4_Address */

/*************************************************************************
*
*   FUNCTION
*
*       NU_Ifconfig_Delete_Ipv4_Address
*
*   DESCRIPTION
*
*       Delete the IPv4 address from the named interface in the interface
*       configuration list.
*
*   INPUTS
*
*       if_name                 Name of the interface.
*       ipv4_addr               IPv4 address to delete.
*       subnet_mask             Corresponding subnet mask of
*                               address to delete.
*
*   RETURNS
*
*       NU_SUCCESS
*
*************************************************************************/
STATUS NU_Ifconfig_Delete_Ipv4_Address(CHAR *if_name, UINT32 ipv4_addr,
                                       UINT32 subnet_mask)
{
    STATUS              status;
    IFCONFIG_NODE       *ifconfig_node_ptr;
    DEV_IF_ADDR_ENTRY   *addr_ptr;

    /* Validate the input parameters. */
    if (if_name)
    {
        /* Obtain semaphore. */
        status = NU_Obtain_Semaphore(&Ifconfig_Semaphore, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Get a pointer to the configuration structure for this interface. */
            status = Ifconfig_Find_Interface(if_name, &ifconfig_node_ptr);

            if ( (status == NU_SUCCESS) && (ifconfig_node_ptr != NU_NULL) )
            {
                /* Find the address. */
                for (addr_ptr = ifconfig_node_ptr->ifconfig_addr_list.dv_head;
                     addr_ptr != NU_NULL;
                     addr_ptr = addr_ptr->dev_entry_next)
                {
                    /* If the address and subnet mask match. */
                    if ( (addr_ptr->dev_entry_ip_addr == ipv4_addr) &&
                         (addr_ptr->dev_entry_netmask == subnet_mask) )
                    {
                        /* Remove the address from the list. */
                        DLL_Remove(&ifconfig_node_ptr->ifconfig_addr_list, addr_ptr);

                        /* Deallocate the memory for the address structure. */
                        if (NU_Deallocate_Memory(addr_ptr) != NU_SUCCESS)
                        {
                            NLOG_Error_Log("Unable to deallocate memory",
                                           NERR_SEVERE, __FILE__, __LINE__);
                        }

                        break;
                    }
                }
            }

            /* Release semaphore. */
            if (NU_Release_Semaphore(&Ifconfig_Semaphore) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

    /* Return success whether the address was found or not - if it
     * wasn't found, there was nothing to delete.
     */
    return (NU_SUCCESS);

} /* NU_Ifconfig_Delete_Ipv4_Address */

/*************************************************************************
*
*   FUNCTION
*
*       NU_Ifconfig_Get_Ipv6_Address
*
*   DESCRIPTION
*
*       Get the IPv6 address for the named interface from the interface
*       configuration list.
*
*   INPUTS
*
*       if_name                 Name of the interface.
*       ipv6_addr               Pointer to IPv6 address buffer.
*       prefix_len              Pointer to the IPv6 address prefix length.
*
*   RETURNS
*
*       STATUS
*
*************************************************************************/
STATUS NU_Ifconfig_Get_Ipv6_Address(CHAR *if_name, UINT8 *local_ipv6,
                                    UINT8 *prefix_len)
{
    STATUS          status = -1;
    IFCONFIG_NODE   *ifconfig_node_ptr;
    UINT8           Unspecified_IPv6[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    if ( (if_name) && (local_ipv6) && (prefix_len) )
    {
        /* Obtain semaphore. */
        status = NU_Obtain_Semaphore(&Ifconfig_Semaphore, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            status = Ifconfig_Find_Interface(if_name, &ifconfig_node_ptr);

            if ( (status == NU_SUCCESS) && (ifconfig_node_ptr != NU_NULL) )
            {
                /* Ensure an actual IPv6 address is configured. */
                if (memcmp(ifconfig_node_ptr->Ipv6_Address, Unspecified_IPv6,
                           sizeof(Unspecified_IPv6)) != 0)
                {
                    /* Copy the IPv6 address. */
                    memcpy(local_ipv6, ifconfig_node_ptr->Ipv6_Address, MAX_ADDRESS_SIZE);

                    /* Return the prefix length of this address. */
                    *prefix_len = ifconfig_node_ptr->Ipv6_Prefix_Length;
                }

                else
                {
                    status = -1;
                }
            }

            /* Release semaphore. */
            if (NU_Release_Semaphore(&Ifconfig_Semaphore) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

    return (status);

} /* NU_Ifconfig_Get_Ipv6_Address */

/*************************************************************************
*
*   FUNCTION
*
*       NU_Ifconfig_Set_Ipv6_Address
*
*   DESCRIPTION
*
*       Set the IPv6 address for the named interface in the interface
*       configuration list.
*
*   INPUTS
*
*       if_name                 Name of the interface.
*       ipv6_addr               Pointer to IPv6 address buffer.
*       prefix_len              The prefix length of the address.
*
*   RETURNS
*
*       STATUS
*
*************************************************************************/
STATUS NU_Ifconfig_Set_Ipv6_Address(CHAR *if_name, UINT8 *local_ipv6,
                                    UINT8 prefix_len)
{
    STATUS         status = -1;
    IFCONFIG_NODE  *ifconfig_node_ptr;

    if ( (if_name) && (local_ipv6) )
    {
        /* Obtain semaphore. */
        status = NU_Obtain_Semaphore(&Ifconfig_Semaphore, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            status = Ifconfig_Find_Interface(if_name, &ifconfig_node_ptr);

            if ( (status == NU_SUCCESS) && (ifconfig_node_ptr != NU_NULL) )
            {
                memcpy(ifconfig_node_ptr->Ipv6_Address, local_ipv6, MAX_ADDRESS_SIZE);

                /* Set the prefix length. */
                ifconfig_node_ptr->Ipv6_Prefix_Length = prefix_len;
            }

            /* Release semaphore. */
            if (NU_Release_Semaphore(&Ifconfig_Semaphore) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

    return (status);

} /* NU_Ifconfig_Set_Ipv6_Address */

/*************************************************************************
*
*   FUNCTION
*
*       NU_Ifconfig_Delete_Ipv6_Address
*
*   DESCRIPTION
*
*       Deletes the IPv6 address for the named interface from the interface
*       configuration list.
*
*   INPUTS
*
*       if_name                 Name of the interface.
*       ipv6_addr               Pointer to IPv6 address buffer.
*       prefix_len              The prefix length of the address.
*
*   RETURNS
*
*       STATUS
*
*************************************************************************/
STATUS NU_Ifconfig_Delete_Ipv6_Address(CHAR *if_name, UINT8 *local_ipv6,
                                       UINT8 prefix_len)
{
    STATUS         status = -1;
    IFCONFIG_NODE  *ifconfig_node_ptr;

    if ( (if_name) && (local_ipv6) )
    {
        /* Obtain semaphore. */
        status = NU_Obtain_Semaphore(&Ifconfig_Semaphore, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            status = Ifconfig_Find_Interface(if_name, &ifconfig_node_ptr);

            /* If the stored address matches the address to delete. */
            if ( (status == NU_SUCCESS) && (ifconfig_node_ptr != NU_NULL) &&
                 (memcmp(ifconfig_node_ptr->Ipv6_Address, local_ipv6, MAX_ADDRESS_SIZE) == 0) &&
                 (ifconfig_node_ptr->Ipv6_Prefix_Length == prefix_len) )
            {
                memset(ifconfig_node_ptr->Ipv6_Address, 0, MAX_ADDRESS_SIZE);

                /* Set the prefix length. */
                ifconfig_node_ptr->Ipv6_Prefix_Length = 0;
            }

            /* Release semaphore. */
            if (NU_Release_Semaphore(&Ifconfig_Semaphore) != NU_SUCCESS)
            {
                NLOG_Error_Log("Failed to release semaphore",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
    }

    return (status);

} /* NU_Ifconfig_Delete_Ipv6_Address */
