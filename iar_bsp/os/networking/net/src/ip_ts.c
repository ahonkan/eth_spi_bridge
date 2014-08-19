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
*        ip_ts.c
*
* DESCRIPTION
*
*        This file contains functions responsible for the interfacing SNMP
*        queries related to IP Tunnel.
*
* DATA STRUCTURES
*
*        Temp_Tunnel_Config_List
*
* FUNCTIONS
*
*        IP_Get_Next_Tunnel_Config_Entry
*        IP_Get_Tunnel_Config_IfIndex
*        IP_Get_Tunnel_Config_Status
*        IP_Create_Tunnel_Config_Entry
*        IP_Commit_Tunnel_Config_Entries
*        IP_Undo_Tunnel_Config_Status
*        IP_Commit_Tunnel_Config_Status
*
* DEPENDENCIES
*
*        nu_net.h
*        ip_tun.h
*        snmp.h
*
*************************************************************************/

#include "networking/nu_net.h"

#if ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))
#include "networking/snmp.h"

static IP_TUNNEL_CONFIG_ROOT               Temp_Tunnel_Config_List;

#if (INCLUDE_STATIC_BUILD == NU_TRUE)

extern IP_TUNNEL_CONFIG         NET_IP_Tun_Config_Memory[];
extern UINT8                    NET_IP_Tun_Memory_Used[];

#endif /* (INCLUDE_STATIC_BUILD) */

/* The following structure is used for configuration the list of created
   Tunnels. */
extern IP_TUNNEL_CONFIG_ROOT        IP_Tunnel_Configuration;

extern NU_PROTECT                   IP_Tun_Protect;

/*************************************************************************
*
* FUNCTION
*
*       IP_Get_Next_Tunnel_Config_Entry
*
* DESCRIPTION
*
*       This is used to get the specification of the next tunnel
*       configuration by updating the the values passed in.
*
* INPUTS
*
*       *local_addr             Local address of the tunnel.
*       *remote_addr            Remote address of the tunnel.
*       *encaps_method          Encapsulation method to be used.
*       *config_id              Configuration ID.
*
* OUTPUTS
*
*       NU_TRUE                 If next tunnel configuration exist.
*       NU_FALSE                If next tunnel configuration does not
*                               exist.
*
*************************************************************************/
UINT16 IP_Get_Next_Tunnel_Config_Entry(UINT8  *local_addr,
                                       UINT8  *remote_addr,
                                       UINT8  *encaps_metod,
                                       UINT32 *config_id)
{
    /* Handle to the tunnel configuration. */
    IP_TUNNEL_CONFIG        *tunnel_config;

    /* Status for returning success or error code. */
    UINT16                  success;

    /* Variable to hold the comparison result. */
    INT                     cmp_result;

    /* Increment configuration ID to get next tunnel configuration. */
    (*config_id)++;

    /* Protecting global variables against multiple accesses. */
    NU_Protect(&IP_Tun_Protect);

    /* Getting handle to the next tunnel configuration. */
    tunnel_config =
        IP_Tunnel_Config_Get_Location(&IP_Tunnel_Configuration,
                                      local_addr, remote_addr,
                                      (*encaps_metod), (*config_id),
                                      &cmp_result);

    /* If we have got the handle to the next tunnel configuration. */
    if (tunnel_config)
    {
        /* Copying local address value. */
        NU_BLOCK_COPY(local_addr, tunnel_config->ip_local_address,
                      IP_ADDR_LEN);

        /* Copying remote address value. */
        NU_BLOCK_COPY(remote_addr, tunnel_config->ip_remote_address,
                      IP_ADDR_LEN);

        /* Copying encapsulation method value. */
        (*encaps_metod) = tunnel_config->ip_encaps_method;

        /* Copying config ID. */
        (*config_id) = tunnel_config->ip_config_id;

        /* Returning success code. */
        success = NU_TRUE;
    }

    /* If we did not get the next tunnel configuration then return
       error code. */
    else
        success = NU_FALSE;

    /* We are out of critical section now. */
    NU_Unprotect();

    /* Returning success or failure code. */
    return (success);

} /* IP_Get_Next_Tunnel_Config_Entry */

/*************************************************************************
*
* FUNCTION
*
*       IP_Get_Tunnel_Config_IfIndex
*
* DESCRIPTION
*
*       This functions gets the value of ifIndex of interface device
*       associated with tunnel configuration.
*
* INPUTS
*
*       *local_addr             Local address of the tunnel.
*       *remote_addr            Remote address of the tunnel.
*       encaps_method           Encapsulation method to be used.
*       config_id               Configuration ID.
*       *if_index               Pointer to the memory location where
*                               interface index is to be copied.
*
* OUTPUTS
*
*       NU_TRUE                 If request was successful.
*       NU_FALSE                If request was failed.
*
*************************************************************************/
UINT16 IP_Get_Tunnel_Config_IfIndex(const UINT8 *local_addr,
                                    const UINT8 *remote_addr,
                                    UINT8 encaps_method, UINT32 config_id,
                                    UINT32 *if_index)
{
    /* Handle to the tunnel configuration. */
    IP_TUNNEL_CONFIG    *tunnel_config;

    /* Variable to hold the comparison result. */
    INT                 cmp_result;

    /* Success or failure code to return. */
    UINT16              success;

    /* Protecting global variables against multiple accesses. */
    NU_Protect(&IP_Tun_Protect);

    /* Getting handle to the tunnel configuration from permanent list. */
    tunnel_config = IP_Get_Tunnel_Config(local_addr, remote_addr,
                                         encaps_method, config_id);

    /* If we have not got the handle to the tunnel configuration from
       permanent list then search the handle to the tunnel
       configuration from temporary list. */
    if (tunnel_config == NU_NULL)
    {
        /* Getting handle to the tunnel configuration from temporary
           list. */
        tunnel_config =
            IP_Tunnel_Config_Get_Location(&Temp_Tunnel_Config_List,
                                          local_addr, remote_addr,
                                          encaps_method, config_id,
                                          &cmp_result);

        /* If we did not get exact match then set tunnel config handle
           to NU_NULL. */
        if (cmp_result != 0)
        {
            tunnel_config = NU_NULL;
        }
    }

    /* If we have got handle to the tunnel configuration. */
    if (tunnel_config)
    {
        /* Getting the value of interface index. */
        (*if_index) = tunnel_config->ip_if_index;

        /* Returning success code. */
        success = NU_TRUE;
    }

    /* If we did not get the handle to the tunnel configuration then
       return failure code. */
    else
    {
        /* Returning failure code. */
        success = NU_FALSE;
    }

    /* We are out of critical section now. */
    NU_Unprotect();

    /* Returning success or failure code. */
    return (success);

} /* IP_Get_Tunnel_Config_IfIndex */

/*************************************************************************
*
* FUNCTION
*
*       IP_Get_Tunnel_Config_Status
*
* DESCRIPTION
*
*       This is used to get the value of row status of tunnel
*       configuration specified by the indexes passed in.
*
* INPUTS
*
*       *local_addr             Local address of the tunnel.
*       *remote_addr            Remote address of the tunnel.
*       encaps_method           Encapsulation method to be used.
*       config_id               Configuration ID.
*       *row_status             Pointer to the memory location where
*                               row status is to be copied.
*
* OUTPUTS
*
*       NU_TRUE                 If request was successful.
*       NU_FALSE                If request was failed.
*
*************************************************************************/
UINT16 IP_Get_Tunnel_Config_Status(const UINT8 *local_addr,
                                   const UINT8 *remote_addr,
                                   UINT8 encaps_method, UINT32 config_id,
                                   UINT8 *row_status)
{
    /* Handle to the tunnel configuration. */
    IP_TUNNEL_CONFIG    *tunnel_config;

    /* Variable to hold the comparison result. */
    INT                 cmp_result;

    /* Success or failure code to return. */
    UINT16              success;

    /* Protecting global variables against multiple accesses. */
    NU_Protect(&IP_Tun_Protect);

    /* Getting handle to the tunnel configuration from permanent list. */
    tunnel_config = IP_Get_Tunnel_Config(local_addr, remote_addr,
                                         encaps_method, config_id);

    /* If we have not got the handle to the tunnel configuration from
       permanent list then search the handle to the tunnel
       configuration from temporary list. */
    if (tunnel_config == NU_NULL)
    {
        /* Getting handle to the tunnel configuration from temporary
           list. */
        tunnel_config =
            IP_Tunnel_Config_Get_Location(&Temp_Tunnel_Config_List,
                                          local_addr, remote_addr,
                                          encaps_method, config_id,
                                          &cmp_result);

        /* If we did not get exact match then set tunnel config handle
           to NU_NULL. */
        if (cmp_result != 0)
        {
            tunnel_config = NU_NULL;
        }
    }

    /* If we have got handle to the tunnel configuration. */
    if (tunnel_config != NU_NULL)
    {
        /* Getting the value of interface index. */
        (*row_status) = tunnel_config->ip_row_status;

        /* Returning success code. */
        success = NU_TRUE;
    }

    /* If we did not get the handle to the tunnel configuration then
       return failure code. */
    else
    {
        /* Returning failure code. */
        success = NU_FALSE;
    }

    /* We are out of critical section now. */
    NU_Unprotect();

    /* Returning success or failure code. */
    return (success);

} /* IP_Get_Tunnel_Config_Status */

/*************************************************************************
*
* FUNCTION
*
*        IP_Create_Tunnel_Config_Entry
*
* DESCRIPTION
*
*        This function is used to create tunnel configuration entry.
*
* INPUTS
*
*       *local_addr             Local address of the tunnel.
*       *remote_addr            Remote address of the tunnel.
*       encaps_method           Encapsulation method to be used.
*       config_id               Configuration ID.
*
* OUTPUTS
*
*       NU_TRUE                 If processing succeeded.
*       NU_FALSE                If processing failed.
*
*************************************************************************/
UINT16 IP_Create_Tunnel_Config_Entry(const UINT8 *local_addr,
                                     const UINT8 *remote_addr,
                                     UINT8 encaps_method, UINT32 config_id)
{
    /* Handle to the tunnel configuration. */
    IP_TUNNEL_CONFIG    *tunnel_config;

    /* Handle to tunnel configuration that is to be created. */
    IP_TUNNEL_CONFIG    *ip_tun_config;

#if (INCLUDE_STATIC_BUILD == NU_TRUE)

    /* Variable used in for loop. */
    UINT32              loop;

#endif /* (INCLUDE_STATIC_BUILD == NU_TRUE) */

    /* Variable to hold comparison result. */
    INT                 cmp_result;

    /* Success or failure code to return. */
    UINT16              success = NU_FALSE;

    if (IP_Is_Registered_Prot(encaps_method) == NU_TRUE)
    {
#if (INCLUDE_STATIC_BUILD == NU_TRUE)

        /* Initializing tunnel configuration to NU_NULL. */
        ip_tun_config = NU_NULL;

        /* Loop for searching the free memory for allocation. */
        for (loop = 0; loop < NET_MAX_TUNNEL; loop++)
        {
            /* If we found the free memory. */
            if (NET_IP_Tun_Memory_Used[loop] == NU_FALSE)
            {
                /* Mark the memory as allocated. */
                NET_IP_Tun_Memory_Used[loop] = NU_TRUE;

                /* Getting the handle to the memory allocated. */
                ip_tun_config = &NET_IP_Tun_Config_Memory[loop];

                /* Breaking through the loop. */
                break;
            }
        }

        /* If we did not get the free memory then return error
           code. */
        if (loop != NET_MAX_TUNNEL)

#else /* (INCLUDE_STATIC_BUILD == NU_TRUE) */

        if (NU_Allocate_Memory(MEM_Cached, (VOID **)&ip_tun_config,
                               sizeof(IP_TUNNEL_CONFIG),
                               NU_NO_SUSPEND) == NU_SUCCESS)

#endif /* (INCLUDE_STATIC_BUILD == NU_TRUE) */
        {
            /* Clearing the tunnel configuration. */
            UTL_Zero(ip_tun_config, sizeof(IP_TUNNEL_CONFIG));

            /* Copying local address. */
            NU_BLOCK_COPY(ip_tun_config->ip_local_address,
                          local_addr, IP_ADDR_LEN);

            /* Copying remote address. */
            NU_BLOCK_COPY(ip_tun_config->ip_remote_address,
                          remote_addr, IP_ADDR_LEN);

            /* Copying the value of encapsulation method. */
            ip_tun_config->ip_encaps_method = encaps_method;

            /* Copying the value of tunnel configuration ID. */
            ip_tun_config->ip_config_id = config_id;

            /* Protecting global variables against multiple accesses. */
            NU_Protect(&IP_Tun_Protect);

            /* Getting handle to the tunnel configuration from permanent
               list. */
            tunnel_config =
                IP_Get_Tunnel_Config(local_addr, remote_addr,
                                     encaps_method, config_id);

            /* If we have not got the handle to the tunnel configuration
               from permanent list then search the handle to the tunnel
               configuration from temporary list. */
            if (tunnel_config == NU_NULL)
            {
                /* Getting handle to the tunnel configuration from
                   temporary list. */
                tunnel_config =
                    IP_Tunnel_Config_Get_Location(&Temp_Tunnel_Config_List,
                                                  local_addr, remote_addr,
                                                  encaps_method, config_id,
                                                  &cmp_result);

                /* If we did find a exact match then create tunnel
                   configuration, otherwise return error code. */
                if ( (tunnel_config == NU_NULL) || (cmp_result != 0) )
                {
                    /* Add tunnel configuration to its proper position. */
                    if (tunnel_config)
                    {
                        DLL_Insert(&Temp_Tunnel_Config_List,
                                   ip_tun_config, tunnel_config);
                    }

                    else
                    {
                        DLL_Enqueue(&Temp_Tunnel_Config_List, ip_tun_config);
                    }

                    /* Returning success code. */
                    success = NU_TRUE;
                }
            }

            /* We are out of critical section now. */
            NU_Unprotect();

            /* If processing failed then deallocate memory. */
            if (success == NU_FALSE)
            {
#if (INCLUDE_STATIC_BUILD == NU_TRUE)

                /* Deallocating memory. */
                NET_IP_Tun_Memory_Used[ip_tun_config -
                                    NET_IP_Tun_Config_Memory] = NU_FALSE;

#else /* (INCLUDE_STATIC_BUILD == NU_TRUE) */

                /* Deallocating memory. */
                if (NU_Deallocate_Memory(ip_tun_config) != NU_SUCCESS)
                {
                    NLOG_Error_Log("Memory deallocation failed",
                                   NERR_RECOVERABLE, __FILE__, __LINE__);
                }

#endif /* (INCLUDE_STATIC_BUILD == NU_TRUE) */
            }
        }

        /* If memory allocation failed. */
        else
        {
            NLOG_Error_Log("Failed to allocate memory",
                            NERR_RECOVERABLE, __FILE__, __LINE__);
        }
    }

    /* Returning success or failure code. */
    return (success);

} /* IP_Create_Tunnel_Config_Entry */

/*************************************************************************
*
* FUNCTION
*
*        IP_Commit_Tunnel_Config_Entries
*
* DESCRIPTION
*
*        This function is used to commit all Group Address entries.
*
* INPUTS
*
*       None.
*
* OUTPUTS
*       None.
*
*************************************************************************/
VOID IP_Commit_Tunnel_Config_Entries(VOID)
{
    /* Dummy handle to the tunnel configuration. */
    IP_TUNNEL_CONFIG    *temp_ptr;

    /* Variable to hold interface index. */
    UINT32              if_index;

    /* Variable to hold local address. */
    UINT8               ip_local_address[IP_ADDR_LEN];

    /* Variable to hold remote address. */
    UINT8               ip_remote_address[IP_ADDR_LEN];

    /* Variable to hold encapsulation method. */
    UINT8               ip_encaps_method;

    /* Variable to hold configuration ID. */
    UINT32              ip_config_id;

    /* Protecting global variables against multiple accesses. */
    NU_Protect(&IP_Tun_Protect);

    /* Getting handle to the tunnel configuration. */
    temp_ptr = Temp_Tunnel_Config_List.ip_flink;

    while (temp_ptr != NU_NULL)
    {
        /* Copying local address. */
        NU_BLOCK_COPY(ip_local_address, temp_ptr->ip_local_address,
                      IP_ADDR_LEN);

        /* Copying remote address. */
        NU_BLOCK_COPY(ip_remote_address, temp_ptr->ip_remote_address,
                      IP_ADDR_LEN);

        /* Copying encapsulation method. */
        ip_encaps_method = temp_ptr->ip_encaps_method;

        /* Copying configuration ID. */
        ip_config_id = temp_ptr->ip_config_id;

        /* Removing node from configuration list. */
        DLL_Remove(&Temp_Tunnel_Config_List, temp_ptr);

        /* We are out of critical section now. */
        NU_Unprotect();

#if (INCLUDE_STATIC_BUILD == NU_TRUE)

        /* Deallocating memory. */
        NET_IP_Tun_Memory_Used[temp_ptr - NET_IP_Tun_Config_Memory] =
            NU_FALSE;

#else /* (INCLUDE_STATIC_BUILD == NU_TRUE) */

        /* If memory deallocation failed add error log. */
        if (NU_Deallocate_Memory(temp_ptr) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate memory",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

#endif /* (INCLUDE_STATIC_BUILD == NU_TRUE) */

        /* If we failed to create tunnel. */
        if (IP_Create_Tunnel(ip_local_address, ip_remote_address,
                             ip_encaps_method, ip_config_id,
                             &if_index) != NU_SUCCESS)
        {
            /* Adding error log. */
            NLOG_Error_Log("Failed to create IP Tunnel",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

        /* Protecting global variables against multiple accesses. */
        NU_Protect(&IP_Tun_Protect);

        /* Getting next handle to the tunnel configuration. */
        temp_ptr = Temp_Tunnel_Config_List.ip_flink;
    }

    /* We are out of critical section now. */
    NU_Unprotect();

} /* IP_Commit_Tunnel_Config_Entries */

/*************************************************************************
*
* FUNCTION
*
*       IP_Undo_Tunnel_Config_Status
*
* DESCRIPTION
*
*       This function updates the row status of the tunnel configuration
*       entry specified by indexes passed in.
*
* INPUTS
*
*       *local_addr             Local address of the tunnel.
*       *remote_addr            Remote address of the tunnel.
*       encaps_method           Encapsulation method to be used.
*       config_id               Configuration ID.
*       row_status              The value of row status.
*
* OUTPUTS
*
*       NU_TRUE                 If processing succeeded.
*       NU_FALSE                If processing failed.
*
*************************************************************************/
UINT16 IP_Undo_Tunnel_Config_Status(const UINT8 *local_addr,
                                    const UINT8 *remote_addr,
                                    UINT8 encaps_method, UINT32 config_id,
                                    UINT8 row_status)
{
    /* Handle to the tunnel configuration. */
    IP_TUNNEL_CONFIG    *tunnel_config;

    /* Variable to hold the comparison result. */
    INT                 cmp_result;

    /* Protecting global variables against multiple accesses. */
    NU_Protect(&IP_Tun_Protect);

    /* Getting handle to the tunnel configuration from the temporary
       list. */
    tunnel_config =
        IP_Tunnel_Config_Get_Location(&Temp_Tunnel_Config_List,
                                      local_addr, remote_addr,
                                      encaps_method, config_id,
                                      &cmp_result);

    /* If we got the handle to the tunnel configuration from temporary
       list then remove the tunnel configuration. */
    if ( (tunnel_config) && (cmp_result == 0) )
    {
        /* Removing tunnel configuration. */
        DLL_Remove(&Temp_Tunnel_Config_List, tunnel_config);

        /* We are out of critical section now. */
        NU_Unprotect();

#if (INCLUDE_STATIC_BUILD == NU_TRUE)

        /* Deallocating memory attained by remove tunnel configuration. */
        NET_IP_Tun_Memory_Used[tunnel_config - NET_IP_Tun_Config_Memory] =
            NU_FALSE;

#else /* (INCLUDE_STATIC_BUILD == NU_TRUE) */

        /* Deallocating memory attained by remove tunnel configuration. */
        if (NU_Deallocate_Memory(tunnel_config) != NU_SUCCESS)
        {
            /* If memory deallocation failed then add error log. */
            NLOG_Error_Log("Failed to deallocate memory",
                           NERR_RECOVERABLE, __FILE__, __LINE__);
        }

#endif /* (INCLUDE_STATIC_BUILD == NU_TRUE) */
    }

    /* If we did not get the tunnel configuration in temporary list then
       search it in permanent list. */
    else
    {
        /* Getting tunnel configuration from permanent list. */
        tunnel_config = IP_Get_Tunnel_Config(local_addr, remote_addr,
                                             encaps_method, config_id);

        /* If we have got the tunnel configuration from the permanent list
           then process the undo operation as set operation. */
        if (tunnel_config != NU_NULL)
        {
            /* Setting the value of row status. */
            tunnel_config->ip_row_status = row_status;

            /* We are out of critical section now. */
            NU_Unprotect();

            IP_Commit_Tunnel_Config_Status(local_addr, remote_addr,
                                           encaps_method, config_id, 0);
        }

        else
            /* We are out of critical section now. */
            NU_Unprotect();
    }

    return (NU_TRUE);

} /* IP_Undo_Tunnel_Config_Status */

/*************************************************************************
* FUNCTION
*
*        IP_Commit_Tunnel_Config_Status
*
* DESCRIPTION
*
*        This function is used to commit status of tunnel configuration
*        entries.
*
* INPUTS
*
*        *local_addr            Local address of the tunnel.
*        *remote_addr           Remote address of the tunnel.
*        encaps_method          Encapsulation method to be used.
*        config_id              Configuration ID.
*        row_status             The value of row status.
*
* OUTPUTS
*        SNMP_INCONSISTANTVALUE If commit fails.
*        SNMP_NOERROR           If the processing succeeded.
*
*************************************************************************/
UINT16 IP_Commit_Tunnel_Config_Status(const UINT8 *local_addr,
                                      const UINT8 *remote_addr,
                                      UINT8 encaps_method,
                                      UINT32 config_id, UINT8 row_status)
{
    /* Handle to the tunnel configuration. */
    IP_TUNNEL_CONFIG            *tunnel_config;

    /* Handle to the device. */
    DV_DEVICE_ENTRY             *dev;

    /* Device name. */
    CHAR                        dev_name[DEV_NAME_LENGTH];

    /* Boolean variable when NU_TRUE represent that we have obtained the
       semaphore. */
    UINT32                      semaphore_flag;

    /* Variable to hold comparison result. */
    INT                         cmp_result;

    /* Status for returning success or error code. */
    UINT16                      status = SNMP_NOERROR;

    /* Flag when true represent that current entry is new entry. */
    UINT8                       is_new;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* We have obtained the semaphore. */
        semaphore_flag = NU_TRUE;

        /* Protecting global variables against multiple accesses. */
        NU_Protect(&IP_Tun_Protect);

        /* Getting handle to the tunnel configuration from temporary list. */
        tunnel_config =
            IP_Tunnel_Config_Get_Location(&Temp_Tunnel_Config_List,
                                          local_addr, remote_addr,
                                          encaps_method, config_id,
                                          &cmp_result);

        /* If we have found the tunnel configuration from temporary list
         * then it is a new entry.
         */
        if ( (tunnel_config) && (cmp_result == 0) )
        {
            is_new = NU_TRUE;
        }

        else
        {
            /* Getting handle to the tunnel configuration from permanent
               list. */
            tunnel_config = IP_Get_Tunnel_Config(local_addr, remote_addr,
                                                 encaps_method, config_id);

            /* Since we got the handle from the permanent list, therefore
               this is an old entry. */
            is_new = NU_FALSE;
        }

        /* If we have got the handle to tunnel interface either from
           temporary list or permanent list. */
        if (tunnel_config)
        {
            switch (row_status)
            {
            case SNMP_ROW_ACTIVE:       /* Active */

                /* If we have entry from temporary list, we can't
                 * activate by setting its status to 'active'. However
                 * we can do it by setting its status to 'create and go'.
                 */
                if (is_new)
                    status = SNMP_INCONSISTANTVALUE;

                /* If we have entry from permanent list then set the row
                   status to 'active'. */
                else
                {
                    /* Getting handle to the device. */
                    dev = DEV_Get_Dev_By_Index(tunnel_config->ip_if_index);

                    /* If we got the handle to the interface device. */
                    if (dev)
                    {
                        /* Setting row status to active. */
                        tunnel_config->ip_row_status = SNMP_ROW_ACTIVE;

                        /* Activating corresponding device. */
                        dev->dev_flags |= DV_UP;
                    }

                    /* If we did not get the handle to the interface
                     * device then return error code.
                     */
                    else
                    {
                        /* Returning error code. */
                        status = SNMP_ERROR;
                    }
                }

                break;

            case SNMP_ROW_NOTINSERVICE:     /* Not In Service */
            case SNMP_ROW_NOTREADY:

                /* The agent need not support setting this object to
                 * createAndWait or notInService since there are no other
                 * writable objects in this table, and writable objects
                 * in rows of corresponding tables such as the
                 * tunnelIfTable may be modified while this row is active.
                 */
                status = SNMP_INCONSISTANTVALUE;

                break;

            case SNMP_ROW_DESTROY:  /* Destroy */

                /* Setting status to destroy. */
                tunnel_config->ip_row_status = SNMP_ROW_DESTROY;

                break;

            case SNMP_ROW_CREATEANDGO:  /* Create and Go */

                /* 'Create and Go' is valid only for new entry and
                 * 'non-active' permanent entries.
                 */
                if (is_new)
                {
                    /* Setting status to 'active' if entry is new. */
                    tunnel_config->ip_row_status = SNMP_ROW_ACTIVE;
                }

                /* If we have non-active permanent entry. */
                else if (tunnel_config->ip_row_status != SNMP_ROW_ACTIVE)
                {
                    /* Getting handle to the interface device. */
                    dev = DEV_Get_Dev_By_Index(tunnel_config->ip_if_index);

                    /* If we have got the handle to the interface device. */
                    if (dev)
                    {
                        /* Activating the device. */
                        dev->dev_flags |= DV_UP;

                        /* Setting status to 'active'. */
                        tunnel_config->ip_row_status = SNMP_ROW_ACTIVE;
                    }

                    /* If we did get the handle to the interface device
                     * then return error code.
                     */
                    else
                    {
                        /* Returning error code. */
                        status = SNMP_ERROR;
                    }
                }

                /* If we have permanent 'active' entry then return error
                   code. */
                else
                {
                    /* Returning error code. */
                    status = SNMP_INCONSISTANTVALUE;
                }

                break;

            case SNMP_ROW_CREATEANDWAIT:    /* Create and Wait */

                /* We always has tunnel configuration in ready state.
                 * So 'Create and Wait' option always is invalid for
                 * tunnel configuration.
                 */
                status = SNMP_INCONSISTANTVALUE;
                break;

            default:

                /* Setting status to 'Not in Service'. */
                tunnel_config->ip_row_status = SNMP_ROW_NOTINSERVICE;
            }

            /* If tunnel configuration is need to be destroyed. */
            if (tunnel_config->ip_row_status == SNMP_ROW_DESTROY)
            {
                /* If entry is from temporary list. */
                if (is_new)
                {
                    /* Removing tunnel configuration from temporary list. */
                    DLL_Remove(&Temp_Tunnel_Config_List, tunnel_config);

                    NU_Unprotect();

#if (INCLUDE_STATIC_BUILD == NU_TRUE)

                    /* Deallocating the removed configuration. */
                    NET_IP_Tun_Memory_Used[tunnel_config -
                        NET_IP_Tun_Config_Memory] = NU_FALSE;

#else /* (INCLUDE_STATIC_BUILD == NU_TRUE) */

                    /* Deallocating the removed configuration. */
                    if (NU_Deallocate_Memory(tunnel_config) != NU_SUCCESS)
                    {
                        NLOG_Error_Log("Failed to deallocate memory",
                                        NERR_RECOVERABLE, __FILE__, __LINE__);
                    }

#endif /* (INCLUDE_STATIC_BUILD == NU_TRUE) */

                }

                else
                {
                    /* Getting handle to the interface device. */
                    dev = DEV_Get_Dev_By_Index((tunnel_config->ip_if_index));

                    NU_Unprotect();

                    /* Destroy tunnel interface and tunnel configuration. */
                    if (dev != NU_NULL)
                    {
                        /* Getting device name in local variable. */
                        strcpy(dev_name, dev->dev_net_if_name);

                        /* Release the semaphore. */
                        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                            NLOG_Error_Log("Failed to release semaphore",
                                           NERR_SEVERE, __FILE__, __LINE__);

                        /* We have released the semaphore. */
                        semaphore_flag = NU_FALSE;

                        /* Removing the interface device. */
                        NU_Remove_Device(dev_name, 0);
                    }
                }
            }
            else
                NU_Unprotect();
        }
        else
            NU_Unprotect();

        /* If we currently have the semaphore then release it. */
        if (semaphore_flag == NU_TRUE)
        {
            /* Release the semaphore. */
            if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                               __FILE__, __LINE__);
        }
    }

    else
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* Returning status. */
    return (status);

} /* IP_Commit_Tunnel_Config_Status */

#endif /* ((INCLUDE_SNMP == NU_TRUE) && (INCLUDE_IP_TUN_MIB == NU_TRUE))
        */

