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
*        ip6_mib_if.c                                
*
*   COMPONENT
*
*        IPv6 Interface MIB
*
*   DESCRIPTION
*
*        This file contains the implementation of interface MIBs that will
*        be used by SNMP.
*
*   DATA STRUCTURES
*
*        None.
*
*   FUNCTIONS
*
*        IP6_IF_MIB_Get_Number
*        IP6_IF_MIB_Get_Next_Index
*        IP6_IF_MIB_Get_Index
*        IP6_IF_MIB_Get_Descr
*        IP6_IF_MIB_Set_Descr
*        IP6_IF_MIB_Get_Lower_Layer
*        IP6_IF_MIB_Get_Effective_MTU
*        IP6_IF_MIB_Get_Reasm_Max_Size
*        IP6_IF_MIB_Get_Identifier
*        IP6_IF_MIB_Set_Identifier
*        IP6_IF_MIB_Get_Identifier_Len
*        IP6_IF_MIB_Set_Identifier_Len
*        IP6_IF_MIB_Get_Status
*
*   DEPENDENCIES
*
*        nu_net.h
*        ip6_mib.h
*        snmp_api.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/ip6_mib.h"

#if (INCLUDE_SNMP == NU_TRUE)
#include "networking/snmp_api.h"
#endif

#if (INCLUDE_IP6_MIB_IF_GROUP == NU_TRUE)

/************************************************************************
*
*   FUNCTION
*
*        IP6_IF_MIB_Get_Number
*
*   DESCRIPTION
*
*        This function is used to get the count IPv6 enabled interfaces.
*
*   INPUTS
*
*        *number                Pointer to the memory location where count
*                               of IPv6 enabled is to be stored.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_ERROR          General Error.
*
************************************************************************/
UINT16 IP6_IF_MIB_Get_Number(UINT32 *number)
{
    /* Handle to the device interface. */
    DV_DEVICE_ENTRY         *dev;

    /* Status to return success or error code. */
    UINT16                  status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* Set IPv6 enabled interfaces to zero '0' to start counting. */
        (*number) = 0;

        /* Get the handle to first interface device to start counting the
         * number of IPv6 enabled interfaces.
         */
        dev = DEV_Table.dv_head;

        /* Loop through the interface device list to count the IPv6
         * enabled interfaces.
         */
        while (dev)
        {
            /* If we found an IPv6 enabled interface device then
             * increment the counter.
             */
            if ( (dev -> dev_flags) & DV6_IPV6)
            {
                /* Increment the counter. */
                (*number)++;
            }

            /* Moving forward in the list. */
            dev = dev -> dev_next;
        }

        /* Returning success code. */
        status = IP6_MIB_SUCCESS;

        /* Releasing the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release the semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* We failed to grab the semaphore. */
    else
    {
        NLOG_Error_Log("Failed to grab the semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        /* Return error code. */
        status = IP6_MIB_ERROR;
    }

    /* Return success or error code. */
    return (status);

} /* IP6_IF_MIB_Get_Number */

#endif /* (INCLUDE_IP6_MIB_IF_GROUP == NU_TRUE) */

#if ( (INCLUDE_IP6_MIB_IF_GROUP == NU_TRUE) || \
      (INCLUDE_IPV6_ICMP_MIB == NU_TRUE) )

/************************************************************************
*
*   FUNCTION
*
*        IP6_IF_MIB_Get_Next_Index
*
*   DESCRIPTION
*
*        This function is used to get next IPv6 enabled interface index.
*
*   INPUTS
*
*        *if_index              The pointer to memory where interface
*                               index is stored whose next interface index
*                               is required.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   When we failed to find the next.
*        IP6_MIB_ERROR          General Error.
*
************************************************************************/
UINT16 IP6_IF_MIB_Get_Next_Index(UINT32 *if_index)
{
    /* Handle to the interface device. */
    DV_DEVICE_ENTRY         *dev;

    /* Status to return success or error code. */
    UINT16                  status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* Initialize device handle to first interface device. */
        dev = DEV_Table.dv_head;

        /* Loop to find next IPV6 enabled device. */
        while ( (dev) && 
                (((dev -> dev_index + 1) <= (*if_index)) || 
                  (!(dev->dev_flags & DV6_IPV6))) )
        {
            dev = dev -> dev_next;
        }

        /* If we got the handle to the next IPV6 enabled device. */
        if (dev)
        {
            /* Update interface index value. */
            (*if_index) = (dev -> dev_index + 1);

            /* Return success code. */
            status = IP6_MIB_SUCCESS;
        }

        /* If we did not get the handle to the next IPV6 enabled device
         * then return error code. 
         */
        else
            status = IP6_MIB_NOSUCHOBJECT;

        /* Releasing semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* If we failed to grab the semaphore. */
    else
    {
        NLOG_Error_Log("Failed to grab the semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        /* Return error code. */
        status = IP6_MIB_ERROR;
    }

    /* Return success or failure code. */
    return (status);

} /* IP6_IF_MIB_Get_Next_Index */

#endif

#if (INCLUDE_IP6_MIB_IF_GROUP == NU_TRUE)

/************************************************************************
*
*   FUNCTION
*
*        IP6_IF_MIB_Get_Index
*
*   DESCRIPTION
*
*        This function is used to validate interface index of IPv6
*        interface.
*
*   INPUTS
*
*        if_index               Interface index
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When interface index exists.
*        IP6_MIB_ERROR          General Error.
*        IP6_MIB_NOSUCHOBJECT   When interface does not exist.
*
************************************************************************/
UINT16 IP6_IF_MIB_Get_Index(UINT32 if_index)
{
    /* Handle to the interface device. */
    DV_DEVICE_ENTRY *dev;

    /* Status to return success or error code. */
    UINT16          status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        if (if_index > 0)
        {
            /* Get a pointer to the device */
            dev = DEV_Get_Dev_By_Index(if_index - 1);
            
            /* If the device exists, and the interface MIB statistics
             * gathering is enabled, return success.
             */
            if ( (dev != NU_NULL) && (dev->ip6_interface_mib != NU_NULL) )
            {
                /* Return success code. */
                status = IP6_MIB_SUCCESS;
            }

            else
            {
                /* Return error code. */
                status = IP6_MIB_NOSUCHOBJECT;
            }
        }

        /* If either index was not valid or we did not find a IPV6
         * enabled device with interface index passed in. 
         */
        else
        {
            /* Return error code. */
            status = IP6_MIB_NOSUCHOBJECT;
        }

        /* Releasing semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* If we failed to grab the semaphore. */
    else
    {
        NLOG_Error_Log("Failed to grab the semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        /* Return error code. */
        status = IP6_MIB_ERROR;
    }

    /* Return success or error code. */
    return (status);

} /* IP6_IF_MIB_Get_Index */

/************************************************************************
*
*   FUNCTION
*
*        IP6_IF_MIB_Get_Descr
*
*   DESCRIPTION
*
*        This function is used to get interface description of IPv6
*        interface.
*
*   INPUTS
*
*        if_index               Interface index.
*        *if_descr              Interface description.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   There does not exist an interface with
*                               the index passed in.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_IF_MIB_Get_Descr(UINT32 if_index, CHAR *if_descr)
{
    /* Handle to interface device. */
    DV_DEVICE_ENTRY         *dev;

    /* Status to return success or error code. */
    UINT16                  status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        if (if_index > 0)
        {
            /* Get a pointer to the device */
            dev = DEV_Get_Dev_By_Index(if_index - 1);
            
            /* If the device exists, and the interface MIB statistics
             * gathering is enabled, return success.
             */
            if ( (dev != NU_NULL) && (dev->ip6_interface_mib != NU_NULL) )
            {
                /* Get the value of ipv6IfDescr. */
                strcpy(if_descr, dev->ip6_interface_mib->ip6_if_desc);

                /* Return success code. */
                status = IP6_MIB_SUCCESS;
            }

            else
            {
                /* Return error code. */
                status = IP6_MIB_NOSUCHOBJECT;
            }
        }

        /* If we don't get IPv6 enabled interface with the index passed
         * in then return error code.
         */
        else
            status = IP6_MIB_NOSUCHOBJECT;

        /* Releasing semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* If we failed to obtain semaphore. */
    else
    {
        NLOG_Error_Log("Failed to grab the semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        /* Returning error code. */
        status = IP6_MIB_ERROR;
    }

    /* Return success or error code. */
    return (status);

} /* IP6_IF_MIB_Get_Descr */

/************************************************************************
*
*   FUNCTION
*
*        IP6_IF_MIB_Set_Descr
*
*   DESCRIPTION
*
*        This function is used to set interface description of IPv6
*        enabled interface.
*
*   INPUTS
*
*        if_index               Interface index.
*        *if_descr              Interface description.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   There does not exist an interface with
*                               the index passed in.
*        IP6_MIB_ERROR          General error.
*        IP6_MIB_WRONGLENGTH    When length of interface description is
*                               not valid.
*
************************************************************************/
UINT16 IP6_IF_MIB_Set_Descr(UINT32 if_index, CHAR *if_descr)
{
    /* Handle to interface device. */
    DV_DEVICE_ENTRY         *dev;

    /* Variable to hold the length of description. */
    INT                     len;

    /* Status to return success or error code. */
    UINT16                  status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        if (if_index > 0)
        {
            /* Get a pointer to the device */
            dev = DEV_Get_Dev_By_Index(if_index - 1);
            
            /* If the device exists, and the interface MIB statistics
             * gathering is enabled, return success.
             */
            if ( (dev != NU_NULL) && (dev->ip6_interface_mib != NU_NULL) )
            {
                /* Get the length of ifDescr. */
                len = (INT)strlen(if_descr);

                /* If we have a valid length. */
                if (len < IP6_IF_DESC_SIZE)
                {
                    /* Copy ifDescr. */
                    NU_BLOCK_COPY(dev->ip6_interface_mib->ip6_if_desc,
                                  if_descr, len + 1);

                    /* Return success code. */
                    status = IP6_MIB_SUCCESS;
                }

                /* If we don't have valid length then return error code. */
                else
                    status = IP6_MIB_WRONGLENGTH;
            }

            else
            {
                /* Return error code. */
                status = IP6_MIB_NOSUCHOBJECT;
            }
        }

        /* If we don't get IPv6 enabled interface with the index passed
         * in then return error code.
         */
        else
            status = IP6_MIB_NOSUCHOBJECT;

        /* Releasing semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* If we failed to obtain semaphore. */
    else
    {
        NLOG_Error_Log("Failed to grab the semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        /* Return error code. */
        status = IP6_MIB_ERROR;
    }

    /* Return success or error code. */
    return (status);

} /* IP6_IF_MIB_Set_Descr */

#if (INCLUDE_IF_STACK == NU_TRUE)

/************************************************************************
*
*   FUNCTION
*
*        IP6_IF_MIB_Get_Lower_Layer
*
*   DESCRIPTION
*
*        This function is used to get object identifier of lower layer.
*
*   INPUTS
*
*        if_index               Interface index.
*        *object_oid            Object identifier.
*        *oid_len               OID length.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_ERROR          General error.
*        IP6_MIB_NOSUCHOBJECT   There does not exist an IPv6 enabled
*                               interface with interface index passed in.
*
************************************************************************/
UINT16 IP6_IF_MIB_Get_Lower_Layer(UINT32 if_index, UINT32 *object_oid,
                                  UINT32 *oid_len)
{
    /* Handle to the interface stack entry. */
    MIB2_IF_STACK_STRUCT    *if_stack_entry;

#if (INCLUDE_IPV4 == NU_TRUE)

    /* Variable to hold IP address of interface. */
    UINT32                  if_ip_addr;

    /* OID of ipAdEntAddr. */
    UINT32                  ip_ad_ent_addr_oid[] = 
                                {1, 3, 6, 1, 2, 1, 4, 20, 1, 1};
#endif

    /* OID of ifIndex. */
    UINT32                  if_index_oid[] = 
                                {1, 3, 6, 1, 2, 1, 2, 2,  1, 1};

    /* Status to return success or error code. */
    UINT16                  status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        if (if_index > 0)
        {
            if_stack_entry = MIB2_If_Stack_Get_HI_Entry(if_index, NU_TRUE);

            /* If we have valid interface index and there exists an interface
             * stack entry with higher interface having same interface
             * index as passed in and device associated with interface index
             * passed in is IPv6 enabled.
             */
            if ( (if_stack_entry) && 
                 (if_stack_entry->mib2_higher_layer->ip6_interface_mib) )
            {
                /* If interface is running over data link layer (no lower
                 * interface) or running over another that is IPv6 enabled
                 * device.
                 */
                if ( (if_stack_entry->mib2_lower_layer == NU_NULL) ||
                     (if_stack_entry->mib2_lower_layer->dev_flags & DV6_IPV6) )
                {
                    /* Copying the OID of ifIndex. */
                    NU_BLOCK_COPY(object_oid, if_index_oid,
                                  sizeof(if_index_oid));

                    /* If we have lower interface. */
                    if (if_stack_entry->mib2_lower_layer)
                    {
                        /* Append interface index of lower interface to OID.
                         */
                        object_oid[IP6_IF_MIB_LOW_LYR_SUB_OID_LEN] =
                            (if_stack_entry->mib2_lower_layer->dev_index + 1);
                    }

                    /* If we don't have lower interface. */
                    else
                    {
                        /* Append interface index of itself because this layer
                         * itself is implementing data link layer.
                         */
                        object_oid[IP6_IF_MIB_LOW_LYR_SUB_OID_LEN] = if_index;
                    }

                    /* Updating the OID length. */
                    (*oid_len) = IP6_IF_MIB_LOW_LYR_SUB_OID_LEN + 1;

                    /* Returning success code. */
                    status = IP6_MIB_SUCCESS;
                }

#if (INCLUDE_IPV4 == NU_TRUE)

                /* If interface is running over IPv4 interface and we have
                 * at least one IP address associated with the device.
                 */
                else if (if_stack_entry->mib2_lower_layer->dev_addr.
                         dev_addr_list.dv_head)
                {
                    /* Copying OID of ipAdEntAddr in object_oid. */
                    NU_BLOCK_COPY(object_oid, ip_ad_ent_addr_oid,
                                  sizeof(ip_ad_ent_addr_oid));

                    /* Getting interface device address in local variable. */
                    if_ip_addr = 
                        if_stack_entry->mib2_lower_layer->dev_addr.
                        dev_addr_list.dv_head->dev_entry_ip_addr;

                    /* Appending IP address of interface to OID. */

                    /* Append most significant byte in the OID. */
                    object_oid[IP6_IF_MIB_LOW_LYR_SUB_OID_LEN] =
                        (if_ip_addr >> 24);

                    /* Append second significant byte in the OID. */
                    object_oid[IP6_IF_MIB_LOW_LYR_SUB_OID_LEN + 1] =
                        ((if_ip_addr & 0xFF0000L) >> 16);

                    /* Append third significant byte in the OID. */
                    object_oid[IP6_IF_MIB_LOW_LYR_SUB_OID_LEN + 2] =
                        ((if_ip_addr & 0xFF00) >> 8);

                    /* Append least significant byte in the OID. */
                    object_oid[IP6_IF_MIB_LOW_LYR_SUB_OID_LEN + 3] =
                        (if_ip_addr & 0xFF);

                    /* Updating OID length. */
                    (*oid_len) = IP6_IF_MIB_LOW_LYR_SUB_OID_LEN + 4;

                    /* Returning success code. */
                    status = IP6_MIB_SUCCESS;
                }
#endif
                /* If interface device has not any IP address. */
                else
                    status = IP6_MIB_ERROR;
            }

            /* We did not find IPv6 enabled interface with interface index
             * passed in. 
             */
            else
                status = IP6_MIB_NOSUCHOBJECT;
        }

        /* We did not find IPv6 enabled interface with interface index
         * passed in. 
         */
        else
            status = IP6_MIB_NOSUCHOBJECT;

        /* Releasing semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* If we failed to grab the semaphore. */
    else
    {
        NLOG_Error_Log("Failed to obtain the semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        /* Return error code. */
        status = IP6_MIB_ERROR;
    }

    /* Return success or error code. */
    return (status);

} /* IP6_IF_MIB_Get_Lower_Layer */

#endif /* (INCLUDE_IF_STACK == NU_TRUE) */

/************************************************************************
*
*   FUNCTION
*
*        IP6_IF_MIB_Get_Effective_MTU
*
*   DESCRIPTION
*
*        This function is used to get the value of effective MTU of IPv6
*        enabled interface.
*
*   INPUTS
*
*        if_index               Interface index.
*        *effective_mtu         Pointer to memory location where effective
*                               MTU is to be stored.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   There does not exist an IPv6 enabled
*                               interface with interface index passed in.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_IF_MIB_Get_Effective_MTU(UINT32 if_index, UINT32 *effective_mtu)
{
    /* Handle to interface device. */
    DV_DEVICE_ENTRY     *dev;

    /* Status to return success or error code. */
    UINT16              status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* If we have valid interface index and we have IPv6 enabled
         * device with the interface index passed in.
         */
        if (if_index != 0)
        {
            dev = DEV_Get_Dev_By_Index(if_index - 1);

            if ( (dev) && (dev->ip6_interface_mib) )
            {
                /* Getting effective MTU. */
                (*effective_mtu) = dev->dev6_link_mtu;

                /* Returning success code. */
                status = IP6_MIB_SUCCESS;
            }

            /* If we did not get an IPv6 enabled interface with interface
             * index passed in.
             */
            else
                status = IP6_MIB_NOSUCHOBJECT;
        }

        /* If we did not get an IPv6 enabled interface with interface
         * index passed in.
         */
        else
            status = IP6_MIB_NOSUCHOBJECT;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* If we failed to grab the semaphore. */
    else
    {
        NLOG_Error_Log("Failed to grab the semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        /* Return error code. */
        status = IP6_MIB_ERROR;
    }

    /* Return status. */
    return (status);

} /* IP6_IF_MIB_Get_Effective_MTU */

/************************************************************************
*
*   FUNCTION
*
*        IP6_IF_MIB_Get_Reasm_Max_Size
*
*   DESCRIPTION
*
*        This function is used to get the value of Reasm Max Size.
*
*   INPUTS
*
*        if_index               Interface index.
*        *reasm_max_size        The pointer to memory location where value
*                               of reasm max size is to be stored.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   There does not exist an IPv6 enabled
*                               interface with interface index passed in.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_IF_MIB_Get_Reasm_Max_Size(UINT32 if_index,
                                     UINT32 *reasm_max_size)
{
    /* Handle to interface device. */
    DV_DEVICE_ENTRY     *dev;

    /* Status to return success or error code. */
    UINT16              status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* If we have valid interface index and we have IPV6 enabled
         * device with the interface index passed in.
         */
        if (if_index != 0)
        {
            dev = DEV_Get_Dev_By_Index(if_index - 1);

            if ( (dev) && (dev->ip6_interface_mib) )
            {
                /* Getting effective reasm_max_size. */
                (*reasm_max_size) = dev->dev_reasm_max_size;

                /* Returning success code. */
                status = IP6_MIB_SUCCESS;
            }

            /* If we did not get an IPv6 enabled interface with interface
             * index passed in.
             */
            else
                status = IP6_MIB_NOSUCHOBJECT;
        }

        /* If we did not find an IPv6 enabled interface with interface
         * index passed in then return error code.
         */
        else
            status = IP6_MIB_NOSUCHOBJECT;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* If we failed to grab the semaphore. */
    else
    {
        NLOG_Error_Log("Failed to grab the semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        /* Return error code. */
        status = IP6_MIB_ERROR;
    }

    /* Return status. */
    return (status);

} /* IP6_IF_MIB_Get_Reasm_Max_Size */

/************************************************************************
*
*   FUNCTION
*
*       IP6_IF_MIB_Get_Identifier
*
*   DESCRIPTION
*
*       This function is used to get interface identifier of IPv6 
*       enabled interface.
*
*   INPUTS
*
*       if_index                Interface index.
*       *if_identifier          Pointer to the memory where interface
*                               identifier is to be stored.
*
*   OUTPUTS
*
*       IP6_MIB_SUCCESS         When successful.
*       IP6_MIB_NOSUCHOBJECT    There does not exist an IPv6 enabled
*                               interface with index passed in.
*       IP6_MIB_ERROR           General error.
*
************************************************************************/
UINT16 IP6_IF_MIB_Get_Identifier(UINT32 if_index, UINT8 *if_identifier)
{
    /* Handle to interface device. */
    DV_DEVICE_ENTRY     *dev;

    /* Status to return success or error code. */
    UINT16              status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* If we have valid interface index and we have IPV6 enabled
         * device with the interface index passed in.
         */
        if (if_index != 0)
        {
            dev = DEV_Get_Dev_By_Index(if_index - 1);

            if ( (dev) && (dev->ip6_interface_mib) )
            {
                /* Copying interface identifier. */
                NU_BLOCK_COPY(if_identifier, dev->dev6_interface_id, 8);

                /* Returning success code. */
                status = IP6_MIB_SUCCESS;
            }

            /* If we did not get an IPv6 enabled interface with interface
             * index passed in.
             */
            else
                status = IP6_MIB_NOSUCHOBJECT;
        }

        /* If we did not get an IPv6 enabled interface with index passed
         * in.
         */
        else
            status = IP6_MIB_NOSUCHOBJECT;

        /* Releasing the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* If we failed to grab the semaphore. */
    else
    {
        NLOG_Error_Log("Failed to grab the semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        /* Return error code. */
        status = IP6_MIB_ERROR;
    }

    /* Return status. */
    return (status);

} /* IP6_IF_MIB_Get_Identifier */

/************************************************************************
* FUNCTION
*
*        IP6_IF_MIB_Set_Identifier
*
* DESCRIPTION
*
*        This function is used to set the value to interface identifier.
*
* INPUTS
*
*        if_index               Interface index.
*        *if_identifier         Interface identifier to set,
*
* OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   There does not exist an IPv6 enabled
*                               interface with index passed in.
*        IP6_MIB_WRONGLENGTH    ID length passed in is invalid.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_IF_MIB_Set_Identifier(UINT32 if_index, UINT8 *if_identifier)
{
    /* Handle to interface device. */
    DV_DEVICE_ENTRY     *dev;

    /* Status to return success or error code. */
    UINT16              status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* If we have valid interface index and we have IPV6 enabled
         * device with the interface index passed in.
         */
        if (if_index != 0)
        {
            dev = DEV_Get_Dev_By_Index(if_index - 1);

            if ( (dev) && (dev->ip6_interface_mib) )
            {
                /* Copying interface identifier. */
                NU_BLOCK_COPY(dev->dev6_interface_id, if_identifier, 8);

                /* Returning success code. */
                status = IP6_MIB_SUCCESS;
            }

            /* If we did not get an IPv6 enabled interface with interface
             * index passed in.
             */
            else
                status = IP6_MIB_NOSUCHOBJECT;
        }

        /* If we did get an IPv6 enabled interface with interface
         * index passed in.
         */
        else
            status = IP6_MIB_NOSUCHOBJECT;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release the semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* If we failed to grab the semaphore. */
    else
    {
        NLOG_Error_Log("Failed to grab the semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        /* Return error code. */
        status = IP6_MIB_ERROR;
    }

    /* Return status. */
    return (status);

} /* IP6_IF_MIB_Set_Identifier */

/************************************************************************
*
*   FUNCTION
*
*        IP6_IF_MIB_Get_Identifier_Len
*
*   DESCRIPTION
*
*        This function is used to get interface identifier length of 
*        IPv6 enabled interface.
*
*   INPUTS
*
*        if_index               Interface index.
*        *id_len                Pointer to memory location where 
*                               interface identifier length is to be 
*                               stored.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   There does exist an IPv6 enabled 
*                               interface with index passed in.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_IF_MIB_Get_Identifier_Len(UINT32 if_index, UINT32 *id_len)
{
    /* Handle to interface device. */
    DV_DEVICE_ENTRY     *dev;

    /* Status to return success or error code. */
    UINT16              status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* If we have valid interface index and we have IPV6 enabled
         * device with the interface index passed in.
         */
        if (if_index != 0)
        {
            dev = DEV_Get_Dev_By_Index(if_index - 1);

            if ( (dev) && (dev->ip6_interface_mib) )
            {
                /* Updating interface identifier length. */
                (*id_len) =
                        (((UINT32)(dev->dev6_interface_id_length)) << 3);

                /* Returning success code. */
                status = IP6_MIB_SUCCESS;
            }

            /* If we did not get an IPv6 enabled interface with interface
             * index passed in.
             */
            else
                status = IP6_MIB_NOSUCHOBJECT;
        }

        /* If we did not find an IPv6 enabled interface with index passed
         * in then return error code.
         */
        else
            status = IP6_MIB_NOSUCHOBJECT;

        /* Releasing the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* If we failed to grab the semaphore. */
    else
    {
        NLOG_Error_Log("Failed to grab the semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        /* Return error code. */
        status = IP6_MIB_ERROR;
    }

    /* Return success or error code. */
    return (status);

} /* IP6_IF_MIB_Get_Identifier_Len */

/************************************************************************
*
*   FUNCTION
*
*        IP6_IF_MIB_Set_Identifier_Len
*
*   DESCRIPTION
*
*        This function is used to set value to interface identifier
*        length.
*
*   INPUTS
*
*        if_index               Interface index.
*        id_len                 Value of interface identifier to set.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   There does not exist an IPv6 enabled
*                               interface with index passed in.
*        IP6_MIB_WRONGVALUE     Identifier length is invalid.
*        IP6_MIB_ERROR          General error.
*
************************************************************************/
UINT16 IP6_IF_MIB_Set_Identifier_Len(UINT32 if_index, UINT32 id_len)
{
    /* Handle to interface device. */
    DV_DEVICE_ENTRY     *dev;

    /* Status to return success or error code. */
    UINT16              status;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
    {
        /* If we have valid interface identifier length. */
        if ( (id_len <= 64) && ((id_len & 0x7) == 0) )
        {
            /* If we have valid interface index and we have IPV6 enabled
             * device with the interface index passed in.
             */
            if (if_index != 0)
            {
                dev = DEV_Get_Dev_By_Index(if_index - 1);

                if ( (dev) && (dev->ip6_interface_mib) )
                {
                    /* Updating interface identifier length. */
                    dev->dev6_interface_id_length = (UINT8)(id_len >> 3);

                    /* Returning success code. */
                    status = IP6_MIB_SUCCESS;
                }

                /* If we did not get an IPv6 enabled interface with interface
                 * index passed in.
                 */
                else
                    status = IP6_MIB_NOSUCHOBJECT;
            }

            /* If we failed to get an IPv6 enabled interface with index
             * passed in then return error code.
             */
            else
                status = IP6_MIB_NOSUCHOBJECT;
        }

        /* Identifier length was invalid. */
        else
            status = IP6_MIB_WRONGVALUE;

        /* Releasing the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release the semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* If we failed to grab the semaphore. */
    else
    {
        NLOG_Error_Log("Failed to grab the semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        /* Return error code. */
        status = IP6_MIB_ERROR;
    }

    /* Return success or error code. */
    return (status);

} /* IP6_IF_MIB_Set_Identifier_Len */

/************************************************************************
*
*   FUNCTION
*
*        IP6_IF_MIB_Get_Status
*
*   DESCRIPTION
*
*        This function is used to get the value of statistical counters of
*        IPv6 enabled interface.
*
*   INPUTS
*
*        if_index               Interface index
*        opt                    Variable to represent which counter's is
*                               required.
*        *counter               Pointer to the memory location where value
*                               of counter is to be copied.
*
*   OUTPUTS
*
*        IP6_MIB_SUCCESS        When successful.
*        IP6_MIB_NOSUCHOBJECT   There does not exist an IPv6 enabled
*                               interface with interface index passed in.
*        IP6_MIB_ERROR          General error.
*        IP6_MIB_NOSUCHNAME     Value of 'opt' is invalid.
*
************************************************************************/
UINT16 IP6_IF_MIB_Get_Status(UINT32 if_index, INT opt, UINT32 *counter)
{
    /* Handle to the device interface. */
    DV_DEVICE_ENTRY     *dev;

    /* Status to return success or error code. */
    UINT16              status;

    /* Return error code if we have invalid interface index. */
    if (if_index == 0)
        status = IP6_MIB_NOSUCHOBJECT;

    /* If we have valid opt value then proceed otherwise return error
       code. */
    else if ( (opt > 0) && (opt <= 20) )
    {
        /* Grab the semaphore. */
        if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) == NU_SUCCESS)
        {
            /* Get the handle to the interface device. */
            dev = DEV_Get_Dev_By_Index((if_index - 1));

            /* If we get the handle to the interface device. */
            if ( (dev) && (dev->ip6_interface_mib) )
            {
                /* Get the value of counter. */
                (*counter) = dev->ip6_interface_mib->ip6_stats[(opt - 1)];

                /* Return success code. */
                status = IP6_MIB_SUCCESS;
            }

            /* If we did not get the handle to the device then return
             * error code.
             */
            else
                status = IP6_MIB_NOSUCHOBJECT;

            /* Release the semaphore. */
            if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                               __FILE__, __LINE__);
        }

        /* If we failed to grab the semaphore. */
        else
        {
            NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);

            /* Return error code. */
            status = IP6_MIB_ERROR;
        }
    }

    /* If we have invalid opt value then return error code. */
    else
        status = IP6_MIB_NOSUCHNAME;

    /* Return success or error code. */
    return (status);

} /* IP6_IF_MIB_Get_Status */

#endif /* (INCLUDE_IP6_MIB_IF_GROUP == NU_TRUE) */
