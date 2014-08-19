/*************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
* FILE NAME
*
*       ip_tun.c
*
* DESCRIPTION
*
*        This file contains functions responsible for the maintenance of
*        IP Tunnels.
*
* DATA STRUCTURES
*
*       IP_Config_ID_Gen
*       IP_Tunnel_Interfaces
*       IP_Tunnel_Configuration
*       NET_IP_Tun_Config_Memory
*       NET_IP_Tun_If_Memory
*       NET_IP_Tun_Memory_Used
*
* FUNCTIONS
*
*       IP_Tunnel_Config_Get_Location
*       IP_Make_Tunnel
*       IP_Config_Tunnel
*       IP_Destroy_Tunnel
*       IP_Create_Tunnel
*       IP_Add_Tunnel
*       IP_Add_Tunnel_Config
*
* DEPENDENCIES
*
*       nu_net.h
*       snmp.h
*
*************************************************************************/

#include "networking/nu_net.h"

#if ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))

#include "networking/snmp.h"

#endif /* ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))
        */

NU_PROTECT                      IP_Tun_Protect;

/* The following structure is used to generate automated config id. */
UINT32                          IP_Config_ID_Gen;

/* The following structure maintains the list of created Tunnels. */
IP_TUNNEL_INTERFACE_ROOT        IP_Tunnel_Interfaces;

#if ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))

/* The following structure is used for configuring the list of created
   Tunnels. */
IP_TUNNEL_CONFIG_ROOT           IP_Tunnel_Configuration;

#if (INCLUDE_STATIC_BUILD == NU_TRUE)

IP_TUNNEL_CONFIG                NET_IP_Tun_Config_Memory[NET_MAX_TUNNEL];

#endif /* (INCLUDE_STATIC_BUILD == NU_TRUE) */

#endif /* ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))
        */

/* The following structure maintains the list of available Tunnel
   encapsulation methods. */
extern IP_TUNNEL_PROTOCOL_ROOT  IP_Tunnel_Protocols;

#if (INCLUDE_STATIC_BUILD == NU_TRUE)

IP_TUNNEL_INTERFACE             NET_IP_Tun_If_Memory[NET_MAX_TUNNEL];
UINT8                           NET_IP_Tun_Memory_Used[NET_MAX_TUNNEL];

#endif /* (INCLUDE_STATIC_BUILD) */

STATIC STATUS IP_Make_Tunnel(IP_CFG_TUNNEL *ip_cfg_tunnel,
                            UINT32 config_id);

#if ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))
/*************************************************************************
*
* FUNCTION
*
*       IP_Tunnel_Config_Get_Location
*
* DESCRIPTION
*
*       This function return the first tunnel configuration that have
*       either equal or greater indexes passed in.
*
* INPUTS
*
*       *config_root            Pointer to tunnel configuration list
*                               header.
*       *local_addr             Local address of the tunnel.
*       *remote_addr            Remote address of the tunnel.
*       encaps_method           Encapsulation method to be used.
*       config_id               Configuration ID.
*       *cmp_result             Variable to hold comparison result.
*
* OUTPUTS
*
*       Pointer to first tunnel configuration with equal or greater
*       indexes passed if one was found otherwise NU_NULL.
*
*************************************************************************/
IP_TUNNEL_CONFIG *IP_Tunnel_Config_Get_Location(
        const IP_TUNNEL_CONFIG_ROOT *config_root, const UINT8 *local_addr,
        const UINT8 *remote_addr, UINT8 encaps_method, UINT32 config_id,
        INT *cmp_result)
{
    IP_TUNNEL_CONFIG            *ip_tunnel_config;

    for (ip_tunnel_config = config_root->ip_flink;
         ip_tunnel_config;
         ip_tunnel_config = ip_tunnel_config->ip_flink)
    {
        /* Comparing local address. */
        (*cmp_result) = memcmp(ip_tunnel_config->ip_local_address,
                               local_addr, IP_ADDR_LEN);

        /* If we are currently at node with lesser local
           address then move to next node because we do not
           need to make further comparisons. */
        if ((*cmp_result) < 0)
            continue;

        /* If we have reached a node with greater local
           address then break through the loop because we have
           reached at proper location where new node can be
           inserted. */
        if ((*cmp_result) > 0)
            break;

        /* Comparing remote address because we are at a node that
           has same local address as passed in. */
        (*cmp_result) = memcmp(ip_tunnel_config->ip_remote_address,
                               remote_addr, IP_ADDR_LEN);

        /* If we are at node with lesser remote address then
           skip remaining comparisons and move to the next
           node. */
        if ((*cmp_result) < 0)
            continue;

        /* If we have reached a node with same local address
           and greater remote address then break through the
           loop because we have reached at proper place. */
        if ((*cmp_result) > 0)
            break;

        /* If we are at node that have similar local and
           remote addresses but lesser encapsulation method
           value then skip remaining comparison and move to
           the next node. */
        if (ip_tunnel_config->ip_encaps_method < encaps_method)
            continue;

        /* If we are at node that have similar local and
           remote addresses and greater encapsulation method
           value then break through the loop because we have
           reached at a proper place. */
        if (ip_tunnel_config->ip_encaps_method > encaps_method)
        {
            /* Setting cmp_result to a value that is greater
               than 0 because when its value is greater than 0
               then exactly similar node is not present. */
            (*cmp_result) = 1;

            break;
        }

        /* If we are at node that have similar local addr,
           remote addr and greater encapsulation method value
           and greater config ID then break through the loop
           because we have reached at a proper place. */
        if (ip_tunnel_config->ip_config_id > config_id)
        {
            /* Setting cmp_result to a value that is greater
               than 0 because when its value is greater than 0
               then exactly similar node is not present. */
            (*cmp_result) = 1;

            break;
        }

        /* If we have reached at a node that have exact match
           of local addr, remote addr, encapsulation method
           and config ID, then break through the loop and at
           this point cmp_result will obviously have the value
           of 0. */
        if (ip_tunnel_config->ip_config_id == config_id)
        {
            break;
        }
    }

    /* Return handle to tunnel configuration's proper location. */
    return (ip_tunnel_config);

} /* IP_Tunnel_Config_Get_Location */

#endif /* ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))
        */

/*************************************************************************
*
* FUNCTION
*
*       IP_Make_Tunnel
*
* DESCRIPTION
*
*       This function configures an IP Tunnel. All the lists which need to
*       be updated will be updated during this configuration.
*
* INPUTS
*
*       *ip_cfg_tunnel          Configuration of tunnel to be configure.
*       config_id               Configuration ID.
*
* OUTPUTS
*
*       NU_SUCCESS              If the request was successful,
*       NU_INVALID_PARM         If parameters are invalid or protocol is
*                               not present.
*       NU_NO_MEMORY            If memory allocation failed.
*       NU_NOT_CONNECTED        If tunnel could not be connected to remote
*                               endpoint.
*       NU_TIMEOUT              If the operation time out.
*
*************************************************************************/
STATIC STATUS IP_Make_Tunnel(IP_CFG_TUNNEL *ip_cfg_tunnel,
                             UINT32 config_id)
{
    /* Device pointer for getting device by name as specified in passed
       configuration. */
    DV_DEVICE_ENTRY                 *dev;

    /* Variable to hold the device type. */
    UINT8                           device_type = 0;

    /* Variable to hold ifIndex. */
    UINT32                          if_index = 0;

    /* Protocol pointer for searching the protocol from tunnel protocol
       list. */
    IP_TUNNEL_PROTOCOL              *protocol;

    /* Tunnel interface's handle for new tunnel. */
    IP_TUNNEL_INTERFACE             *tunnel = NU_NULL;

#if ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))

    /* Configuration pointer to hold the new configuration node. */
    IP_TUNNEL_CONFIG        *config = NU_NULL;

#endif /* ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))
        */

    /* Variable to hold the progress least significant bit when set
       represent that memory has allocated. Second least significant bit
       represent that tunnel configuration has added to configuration
       list. And finally third least significant bit represent that tunnel
       interface has added to tunnel interface list. */
    UINT8                   progress_flags = 0;

#if (INCLUDE_STATIC_BUILD == NU_TRUE)

    /* Variable for using in for loop while search the free memory. */
    UINT16                  loop;

#endif /* (INCLUDE_STATIC_BUILD == NU_TRUE) */

    /* Status for returning success or error code. */
    STATUS                  status;

#if ((INCLUDE_SNMP == NU_FALSE) || (INCLUDE_IP_TUN_MIB == NU_FALSE))

    /* Avoiding compilation warnings. */
    UNUSED_PARAMETER(config_id);

#endif /* ((INCLUDE_SNMP == NU_FALSE) || (INCLUDE_IP_TUN_MIB == NU_FALSE)
        */

    /* Grab the semaphore. */
    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    /* If we successfully grab the semaphore then proceed, otherwise return
       error code. */
    if (status == NU_SUCCESS)
    {
        /* Getting handle to the device interface. */
        dev = DEV_Get_Dev_By_Name(ip_cfg_tunnel->tun_dev_name);

        if (dev)
        {
            /* Getting device type in local variable. */
            device_type = dev->dev_type;

            /* Getting device index in local variable. */
            if_index = dev->dev_index;
        }

        /* If we did not get the handle to the device interface then
           return error code. */
        else
        {
            /* Returning error code. */
            status = NU_INVALID_PARM;
        }

        /* Releasing semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
        }
    }

    /* If we failed to obtain semaphore the log the error. */
    else
    {
        NLOG_Error_Log("Failed to obtain the semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    if (status == NU_SUCCESS)
    {
#if (INCLUDE_STATIC_BUILD == NU_TRUE)

        /* Loop for searching the free memory for allocation. */
        for (loop = 0; loop < NET_MAX_TUNNEL; loop++)
        {
            /* If we found the free memory. */
            if (NET_IP_Tun_Memory_Used[loop] == NU_FALSE)
            {
                /* Mark the memory as allocated. */
                NET_IP_Tun_Memory_Used[loop] = NU_TRUE;

#if ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))

                /* Getting the handle to the memory allocated. */
                config = &NET_IP_Tun_Config_Memory[loop];

#endif /* ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))
        */

                /* Getting handle to tunnel interface memory. */
                tunnel = &NET_IP_Tun_If_Memory[loop];

                /* Breaking through the loop. */
                break;
            }
        }

        /* If we did not get the free memory then return error
           code. */
        if (loop == NET_MAX_TUNNEL)
        {
            /* Returning error code. */
            status = NU_NO_MEMORY;
        }

#else /* (INCLUDE_STATIC_BUILD == NU_TRUE) */

#if ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))

        /* Allocate memory for tunnel interface and tunnel
           configuration node. */
        status = NU_Allocate_Memory(MEM_Cached, (VOID**)&tunnel,
                                    (sizeof(IP_TUNNEL_CONFIG) +
                                     sizeof(IP_TUNNEL_INTERFACE)),
                                    NU_NO_SUSPEND);

#else /* ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE) */

        /* Allocate memory for tunnel interface. */
        status = NU_Allocate_Memory(MEM_Cached, (VOID **)&tunnel,
                                    sizeof(IP_TUNNEL_INTERFACE),
                                    NU_NO_SUSPEND);

#endif /* ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE)
        */

#endif /* (INCLUDE_STATIC_BUILD == NU_TRUE) */

        if (status == NU_SUCCESS)
        {
            /* We successfully allocated memory. */
            progress_flags |= 1;

            /* Initializing tunnel interface. */

            /* Clearing the tunnel interface. */
            UTL_Zero(tunnel, sizeof(IP_TUNNEL_INTERFACE));

            /* Setting ifIndex of tunnel interface. */
            tunnel->ip_if_index = if_index;

            /* Setting security to none. */
            tunnel->ip_security = 1;

            /* Setting local IP address of tunnel. */
            NU_BLOCK_COPY(tunnel->ip_local_address,
                          ip_cfg_tunnel->tun_local_addr, IP_ADDR_LEN);

            /* Setting remote IP address of tunnel. */
            NU_BLOCK_COPY(tunnel->ip_remote_address,
                          ip_cfg_tunnel->tun_remote_addr, IP_ADDR_LEN);

#if ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
            /* Getting handle to allocated memory. */
            config = ((IP_TUNNEL_CONFIG *)(tunnel + 1));

#endif /* (INCLUDE_STATIC_BUILD == NU_FALSE) */

            /* Initializing the tunnel configuration. */

            /* Clearing the tunnel configuration. */
            UTL_Zero(config, sizeof(IP_TUNNEL_CONFIG));

            /* Setting ifIndex of tunnel configuration. */
            config->ip_if_index = if_index;

            /* Setting local IP address of tunnel configuration. */
            NU_BLOCK_COPY(config->ip_local_address,
                          ip_cfg_tunnel->tun_local_addr, IP_ADDR_LEN);

            /* Setting remote IP address of tunnel configuration. */
            NU_BLOCK_COPY(config->ip_remote_address,
                          ip_cfg_tunnel->tun_remote_addr, IP_ADDR_LEN);



            /* Setting ip_row_status. */
            IP_TUN_CONFIG_ACTIVATE(config);

            /* Setting available ID. */
            config->ip_config_id = config_id;
#endif /* ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))
        */

            /* Protecting IP Tunnel's global structures against multiple
             * accesses.
             */
            NU_Protect(&IP_Tun_Protect);

            /* Searching in the protocol list for the entry which has
               Interface type equivalent to type for this device. */
            for (protocol = IP_Tunnel_Protocols.ip_flink;
                 ( (protocol) && (protocol->ip_if_type != device_type) );
                 protocol = protocol->ip_flink)
                 ;

            /* If we found the IP Tunnel Protocol. */
            if (protocol)
            {

#if ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))
                /* Setting encapsulation method. */
                config->ip_encaps_method = protocol->ip_encaps_method;

                /* Add an entry to the tunnel configuration list. */
                status = IP_Add_Tunnel_Config(config);

                /* If we successfully added the tunnel configuration,
                   so proceed. */
                if (status == NU_SUCCESS)
                {
                    /* We have successfully added tunnel configuration
                       in tunnel configuration list. */
                    progress_flags |= 2;

#endif /* ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))
        */

                    /* Setting encapsulation method. */
                    tunnel->ip_encaps_method =
                            protocol->ip_encaps_method;

                    /* Adding an entry to the list of tunnels. */
                    status = DLLI_Add_Node(&IP_Tunnel_Interfaces,
                                           tunnel, DLLI_INDEX_32);

                    /* If tunnel added to tunnel list successfully
                       then call the protocol's configuration
                       function if we had pointer to that
                       function. */
                    if (status == NU_SUCCESS)
                    {
                        /* We have successfully added tunnel
                           interface. */
                        progress_flags |= 4;

                        if (protocol->ip_config_tunnel != NU_NULL)
                        {
                            /* Temporarily unprotect because upcoming
                             * function may not make nested call to
                             * NU_Protect.
                             */
                            NU_Unprotect();

                            /* Calling the protocol's configuration
                             * function.
                             */
                            status =
                                protocol->ip_config_tunnel(ip_cfg_tunnel);

                            /* Again protecting IP Tunnel global variable
                               lists. */
                            NU_Protect(&IP_Tun_Protect);
                        }

                        /* If tunnel added successfully then
                           increment the tunnel count for
                           protocol. */
                        if (status == NU_SUCCESS)
                        {
                            /* Increment the tunnel count of
                               protocol if configured
                               successfully. */
                            protocol->ip_tunnel_no++;
                        }
                    }

#if ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))
                }
#endif /* ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))
        */

                /* If processing was failed. */
                if (status != NU_SUCCESS)
                {
#if ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))

                    /* If tunnel configuration was successfully added to
                       configuration. */
                    if (progress_flags & 2)
                    {
                        /* Removing tunnel configuration. */
                       DLL_Remove(&IP_Tunnel_Configuration, config);
                    }

#endif /* ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))
        */

                    /* If tunnel interface was added successfully. */
                    if (progress_flags & 4)
                    {
                        /* Removing tunnel configuration. */
                        DLL_Remove(&IP_Tunnel_Interfaces, tunnel);
                    }

                }
            }

            /* If we did not get the handle to the protocol then return
             * error code.
             */
            else
            {
                /* Returning error code. */
                status = NU_INVALID_PARM;
            }

            NU_Unprotect();

            /* If we failed to configure tunnel then release the allocated
               memory. */
            if (status != NU_SUCCESS)
            {
#if (INCLUDE_STATIC_BUILD == NU_TRUE)

                /* Deallocating memory. */
                NET_IP_Tun_Memory_Used[(UINT8)(tunnel - NET_IP_Tun_If_Memory)] =
                    NU_FALSE;
#else /* (INCLUDE_STATIC_BUILD == NU_TRUE) */

                if (NU_Deallocate_Memory(tunnel) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Failed to deallocate memory",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }
#endif /* (INCLUDE_STATIC_BUILD == NU_TRUE) */
            }
        }

        /* If we failed to allocate memory. */
        else
        {
            NLOG_Error_Log("Failed to allocate memory",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    /* Return status. */
    return (status);

} /* IP_Make_Tunnel */

/*************************************************************************
*
* FUNCTION
*
*       IP_Config_Tunnel
*
* DESCRIPTION
*
*       This function configures an IP Tunnel. All the lists which need to
*       be updated will be updated during this configuration.
*
* INPUTS
*
*       *ip_cfg_tunnel          Configuration of tunnel to be configure.
*
* OUTPUTS
*
*       NU_SUCCESS              If the request was successful,
*       NU_INVALID_PARM         If parameters are invalid or protocol is
*                               not present.
*       NU_NO_MEMORY            If memory allocation failed.
*       NU_NOT_CONNECTED        If tunnel could not be connected to remote
*                               endpoint.
*       NU_TIMEOUT              If the operation time out.
*
*************************************************************************/
STATUS IP_Config_Tunnel(IP_CFG_TUNNEL *ip_cfg_tunnel)
{
    /* Status for returning success or error code. */
    STATUS                  status;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* If parameter are valid then get the handle to the device. */
    if ( (ip_cfg_tunnel != NU_NULL) &&
         (ip_cfg_tunnel->tun_dev_name != NU_NULL) &&
         (ip_cfg_tunnel->tun_local_addr != NU_NULL) &&
         (ip_cfg_tunnel->tun_remote_addr != NU_NULL) )
    {
        status = IP_Make_Tunnel(ip_cfg_tunnel, (++IP_Config_ID_Gen));
    }
    else
    {
        status = NU_INVALID_PARM;
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    /* Return status. */
    return (status);

} /* IP_Config_Tunnel */

/*************************************************************************
*
* FUNCTION
*
*       IP_Destroy_Tunnel
*
* DESCRIPTION
*
*       This function destroys an IP Tunnel and update All the lists which
*       need to be updated will be updated.
*
* INPUTS
*
*       tun_ifindex             IfIndex of the tunnel to be removed.
*
* OUTPUTS
*
*       NU_SUCCESS              If the request was successful.
*       NU_INVALID_PARM         If parameter are invalid or device is not
*                               present.
*       NU_NOT_FOUND            If tunnel or tunnel configuration is not
*                               present.
*
*************************************************************************/
STATUS IP_Destroy_Tunnel(DV_DEVICE_ENTRY *device)
{
    /* Handle to the tunnel interface. */
    IP_TUNNEL_INTERFACE     *tunnel = NU_NULL;

#if ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))

    /* Pointer to tunnel configuration that is to point at node that is to
       be deleted and it also to use for searching node. */
    IP_TUNNEL_CONFIG        *config;

#endif /* ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))
        */

    /* Handle to encapsulation method. */
    IP_TUNNEL_PROTOCOL      *protocol;

    /* Status for returning success or error code. */
    STATUS                  status;

    /* Search Protocol against the tunnel to be deleted. */
    for (protocol = IP_Tunnel_Protocols.ip_flink;
         ( (protocol) && (protocol->ip_if_type != device->dev_type) );
         protocol = protocol->ip_flink)
        ;

    /* If protocol found then decrement its tunnel count. */
    if (protocol)
    {
        protocol->ip_tunnel_no--;

        /* Protecting IP Tunnel's global variable. */
        NU_Protect(&IP_Tun_Protect);

        /* Removing entry from tunnel interface list if it
           exist. */
        tunnel = DLLI_Remove_Node(&IP_Tunnel_Interfaces,
                                  &(device->dev_index),
                                  DLLI_INDEX_32);

        /* Tunnel interface removed successfully then remove
           tunnel configuration. */
        if (tunnel)
        {

#if ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))

#if (INCLUDE_STATIC_BUILD == NU_TRUE)

            config =
                &NET_IP_Tun_Config_Memory[tunnel - NET_IP_Tun_If_Memory];

#else /* (INCLUDE_STATIC_BUILD == NU_TRUE) */

            /* Getting handle to tunnel configuration. */
            config = (IP_TUNNEL_CONFIG *)(tunnel + 1);

#endif /* (INCLUDE_STATIC_BUILD == NU_TRUE) */

            /* Removing entry from tunnel configuration list
               if it exists. */
            DLL_Remove(&IP_Tunnel_Configuration, config);

#endif /* ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))
        */

            /* Returning success code. */
            status = NU_SUCCESS;
        }

        /* Tunnel Interface was not removed successfully then
           return error code. */
        else
        {
            status = NU_NOT_FOUND;

            NLOG_Error_Log("Failed to remove tunnel interface",
                            NERR_SEVERE, __FILE__, __LINE__);
        }

        /* We have passed by critical section. */
        NU_Unprotect();
    }

    /* If we did not get the handle to tunnel protocol. */
    else
    {
        /* Returning error code. */
        status = NU_NOT_FOUND;

        NLOG_Error_Log("Failed to find tunnel protocol",
                       NERR_SEVERE, __FILE__, __LINE__);
    }


    if (status == NU_SUCCESS)
    {
#if (INCLUDE_STATIC_BUILD == NU_TRUE)

        /* Deallocating tunnel interface. */
        NET_IP_Tun_Memory_Used[(UINT8)(tunnel - NET_IP_Tun_If_Memory)] =
            NU_FALSE;

#else /* (INCLUDE_STATIC_BUILD == NU_TRUE) */

        /* Deallocating tunnel interface. */
        if (NU_Deallocate_Memory(tunnel) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate memory",
                    NERR_RECOVERABLE, __FILE__, __LINE__);
        }

#endif /* (INCLUDE_STATIC_BUILD == NU_TRUE) */

    }

    /* Return status. */
    return (status);

} /* IP_Destroy_Tunnel */

/*************************************************************************
*
* FUNCTION
*
*       IP_Create_Tunnel
*
* DESCRIPTION
*
*       This function creates an IP Tunnel. All the lists which need to be
*       updated will be updated during this creation.
*
* INPUTS
*
*       *local_addr             Local address for Tunnel.
*       *remote_addr            Remote address for Tunnel.
*       encaps_method           Encapsulation protocol to be used.
*       config_id               Configuration ID. (0 if this would be
*                               assigned automatically.
*       *if_index               The interface index assigned to the
*                               tunnel.
*
* OUTPUTS
*
*       NU_SUCCESS              If the request was successful.
*       NU_NO_MEMORY            If memory allocation fail.
*       IP_TUN_ERR              If device initialization fail.
*       NU_INVALID_PARM         If parameter are invalid or tunnel already
*                               exists.
*
*************************************************************************/
STATUS IP_Create_Tunnel(UINT8 *local_addr, UINT8 *remote_addr,
                        UINT8 encaps_method, UINT32 config_id,
                        UINT32 *if_index)
{
    /* Handle for tunnel protocol. */
    IP_TUNNEL_PROTOCOL      *protocol;

    /* String for device name that is to be created. */
    CHAR                    device_name[20] = {"IPMIB_"};

    /* Tunnel CFG for parameter to protocol initializer. */
    IP_CFG_TUNNEL           cfg_tunnel;

    /* Status for returning success or error code. */
    STATUS                  status;

    /* Device structure. */
    NU_DEVICE               l_device;

    /* Searching the protocol list for provided encapsulation method. */
    protocol = DLLI_Search_Node(&IP_Tunnel_Protocols, &encaps_method,
                                DLLI_INDEX_8);

    /* If protocol exists then create tunnel otherwise return error code. */
    if (protocol)
    {
        /* Initialize device. */

        /* Initializing device name. */
        NU_ITOA(((INT)(config_id)), &(device_name[6]), 10);

        /* Copying device structure. */
        NU_BLOCK_COPY(&l_device, &(protocol->ip_device),
                      sizeof(NU_DEVICE));

        /* Initializing device name. */
        l_device.dv_name = device_name;

        /* Initializing device. */
        status = NU_Init_Devices((&l_device), 1);

        if (status == NU_SUCCESS)
        {
            /* Setting device name. */
            cfg_tunnel.tun_dev_name = device_name;

            /* Setting local address. */
            cfg_tunnel.tun_local_addr = local_addr;

            /* Setting remote address. */
            cfg_tunnel.tun_remote_addr = remote_addr;

            /* Setting timeout value. */
            cfg_tunnel.tun_timeout = IP_CFG_TIMEOUT;

            /* Call routine that create tunnel. */
            status = IP_Make_Tunnel((&cfg_tunnel), config_id);

            /* If tunnel configure successfully then get the interface
               device index, otherwise remove the interface device. */
            if (status == NU_SUCCESS)
            {
                (*if_index) = (UINT32)(NU_IF_NameToIndex(device_name));
            }

            /* If tunnel is not configured successfully then remove
               the device. */
            else
            {
               /* Removing device. */
                if (NU_Remove_Device(device_name, 0) != NU_SUCCESS)
                    NLOG_Error_Log("Failed to remove the device", NERR_SEVERE,
                                   __FILE__, __LINE__);
            }
        }
    }

    /* If we did not get the handle to the protocol. */
    else
    {
        status = NU_INVALID_PARM;
    }

    /* Return success or failure code. */
    return (status);

} /* IP_Create_Tunnel */

/*************************************************************************
*
* FUNCTION
*
*       IP_Add_Tunnel
*
* DESCRIPTION
*
*       This function adds a Tunnel to the list of Tunnel interfaces. This
*       list is in ascending order of interface index.
*
* INPUTS
*
*       *tunnel                 The new tunnel interface node to be added.
*
* OUTPUTS
*
*        NU_SUCCESS             The request was successful.
*        NU_INVALID_PARM        Parameter(s) was/were invalid or node
*                               with same index already exists.
*        NU_NO_MEMORY           Memory allocation was failed.
*        OS_INVALID_POOL        The dynamic memory pool was invalid.
*        NU_INVALID_POINTER     Indicates the return pointer from
*                               NU_Allocate_Memory was NULL.
*
*************************************************************************/
STATUS IP_Add_Tunnel(IP_TUNNEL_INTERFACE *tunnel)
{
    /* Status for returning success or error code. */
    STATUS      status;

    /* Protecting against multiple accesses. */
    NU_Protect(&IP_Tun_Protect);

    /* Call the function that is responsible for adding node in sorted
       list. */
    status = DLLI_Add_Node(&IP_Tunnel_Interfaces, tunnel, DLLI_INDEX_32);

    /* We are out of critical section. */
    NU_Unprotect();

    /* Return status. */
    return (status);

} /* IP_Add_Tunnel */

#if ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))

/*************************************************************************
*
* FUNCTION
*
*       IP_Add_Tunnel_Config
*
* DESCRIPTION
*
*       This function adds a Tunnel to the Tunnel Configuration's list.
*       This list is in ascending order of (local address, remote address,
*       encapsulation method and configuration id).
*
* INPUTS
*
*       *node                   The new node to be added.
*
* OUTPUTS
*
*       NU_SUCCESS              If the request was successful.
*       NU_NO_MEMORY            If memory allocation fail.
*       NU_INVALID_PARM         If parameter are invalid.
*       IP_TUN_EXIST           If tunnel already exists.
*
*************************************************************************/
STATUS IP_Add_Tunnel_Config(IP_TUNNEL_CONFIG *node)
{
    /* Status for returning success or error code. */
    STATUS              status = NU_SUCCESS;

    /* Handle to dummy configuration for searching the proper place. */
    IP_TUNNEL_CONFIG    *ip_tunnel_config;

    /* Variable to hold comparison result. */
    INT                 cmp_result;

    /* Find a location in the tunnel configuration interface list,
       which is sorted in ascending order of local address, remote
       address, encapsulation method and configuration id */
    ip_tunnel_config =
        IP_Tunnel_Config_Get_Location(&(IP_Tunnel_Configuration),
                                      node->ip_local_address,
                                      node->ip_remote_address,
                                      node->ip_encaps_method,
                                      node->ip_config_id, (&cmp_result) );

    /* If node is to be added at end of list. */
    if (ip_tunnel_config == NU_NULL)
    {
        /* Adding at end of list. */
        DLL_Enqueue(&IP_Tunnel_Configuration, node);
    }

    /* If node with exact match found then parameter are invalid. */
    else if (cmp_result == 0)
    {
        /* Setting status to error code. */
        status = NU_INVALID_PARM;
    }

    else
    {
        /* Inserting node at proper place. */
        DLL_Insert(&IP_Tunnel_Configuration, node, ip_tunnel_config);
    }

    /* return status */
    return (status);

} /* IP_Add_Tunnel_Config */

#endif /* ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))
        */


