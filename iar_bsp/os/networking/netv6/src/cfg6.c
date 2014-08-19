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
*       cfg6.c                                       
*                                                                                  
*   DESCRIPTION                                                              
*                
*       This file contains those functions necessary to use the 
*       configured virtual device.
*                                                                          
*   DATA STRUCTURES                                                          
*                                      
*       None
*                                                                          
*   FUNCTIONS                                                                
*           
*       CFG6_Configure_Tunnel
*       CFG6_Initialize_Tunnel
*       CFG6_Output
*
*   DEPENDENCIES                                                             
*               
*       externs.h
*       prefix6.h
*       cfg6.h
*       nc6.h
*       nc6_eth.h
*                                                                          
*************************************************************************/

#include "networking/externs.h"
#include "networking/prefix6.h"
#include "networking/cfg6.h"
#include "networking/nc6.h"
#include "networking/nc6_eth.h"

#if (INCLUDE_IPV4 == NU_TRUE)

extern UINT8    IP6_Link_Local_Prefix[];

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       CFG6_Configure_Tunnel
*                                                                         
*   DESCRIPTION                                                           
*                 
*       This function sets up the tunnel source and endpoint of the
*       configured tunnel for the virtual device.
*                                                                        
*   INPUTS                                                                
*                                                        
*       *dev_name               The name of the virtual device.
*       *source_addr            A pointer to the source address of the 
*                               configured tunnel.
*       *tunnel_endpoint        A pointer to the endpoint of the 
*                               configured tunnel.
*                                                                         
*   OUTPUTS                                                               
*                                         
*       NU_SUCCESS              The tunnel was successfully configured.
*       NU_INVALID_PARM         One of the parameters passed in is 
*                               NULL.
*       NU_NO_MEMORY            Insufficient memory.
*
*************************************************************************/
STATUS CFG6_Configure_Tunnel(const UINT8 *dev_name, const UINT8 *source_addr, 
                             const UINT8 *tunnel_endpoint)
{
    DV_DEVICE_ENTRY *virtual_device;
    STATUS          status;
    UINT8           link_local_addr[IP6_ADDR_LEN];
    NU_SUPERV_USER_VARIABLES

    /* Validate the incoming parameters */
    if ( (dev_name == NU_NULL) || (source_addr == NU_NULL) || 
         (tunnel_endpoint == NU_NULL) )
         return (NU_INVALID_PARM);

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Grab the semaphore to protect the device table */
    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status != NU_SUCCESS)
    {
        /* Switch back to user mode. */
        NU_USER_MODE();
        return (status);
    }

    /* Get a pointer to the device specified by the name */
    virtual_device = DEV_Get_Dev_By_Name((CHAR*)dev_name);

    /* If a device was found and the device is an IPv6 configured virtual
     * device.
     */
    if ( (virtual_device) && (virtual_device->dev_type == DVT_CFG6) )
    {
        /* Create the interface identifier from the IPv4 source address */
        memcpy(virtual_device->dev6_interface_id, source_addr, IP_ADDR_LEN);
        virtual_device->dev6_interface_id_length = IP_ADDR_LEN;

        /* Create the link-local address from the source address */
        UTL_Zero(link_local_addr, IP6_ADDR_LEN);
        memcpy(link_local_addr, IP6_Link_Local_Prefix, LINK_LOCAL_PREFIX_LENGTH);        
        memcpy(&link_local_addr[12], source_addr, IP_ADDR_LEN);

        /* Add the link-local address to the virtual device */
        if (DEV6_Add_IP_To_Device(virtual_device, link_local_addr, 0, 
                                  0xffffffffUL, 0xffffffffUL, 0) < 0)
        {
            NLOG_Error_Log("Could not add the link-local address to the Configured Tunnel device", 
                           NERR_SEVERE, __FILE__, __LINE__);

            status = NU_NO_MEMORY;
        }

        else
        {
            virtual_device->dev_addr.dev_ip_addr = IP_ADDR(source_addr);
            virtual_device->dev_addr.dev_dst_ip_addr = IP_ADDR(tunnel_endpoint);

            /* The virtual device is not flagged as UP until the tunnel 
             * source and endpoint addresses have been specified.
             */
            virtual_device->dev_flags |= DV_UP;
            virtual_device->dev_flags2 |= DV6_UP;

            status = NU_SUCCESS;
        }
    }
    else
    {
        status = NU_INVALID_PARM;

        NLOG_Error_Log("No matching device entry for name", 
                       NERR_INFORMATIONAL, __FILE__, __LINE__);
    }

    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE, 
                       __FILE__, __LINE__);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* CFG6_Configure_Tunnel */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       CFG6_Initialize_Tunnel
*                                                                         
*   DESCRIPTION                                                           
*                 
*       This function initializes the device specific parameters of
*       the configured tunnel.
*                                                                        
*   INPUTS                                                                
*                                                        
*       *virtual_device         A pointer to the virtual device to 
*                               initialize.
*                                                                         
*   OUTPUTS                                                               
*                                         
*       NU_SUCCESS              The virtual device was initialized.
*       NU_NO_MEMORY            Insufficient memory.
*
*************************************************************************/
STATUS CFG6_Initialize_Tunnel(DV_DEVICE_ENTRY *virtual_device)
{
    STATUS      status;

    virtual_device->dev_type = DVT_CFG6;

    /* Set up the output routine for the virtual device */
    virtual_device->dev_output = CFG6_Output;

    virtual_device->dev_flags |= (DV6_VIRTUAL_DEV | DV_MULTICAST);

    virtual_device->dev6_link_mtu = 
        virtual_device->dev6_default_mtu = ETHERNET_MTU - IP_HEADER_LEN;

    /* Set the length of the link-layer header to 0, because when using a
     * virtual device, an IPv6 packet is encapsulated in an IPv4 packet, and
     * the IPv4 send routine will take care of building the header for the
     * link-layer.
     */
    virtual_device->dev_hdrlen = 0;

    /* Set up the function pointers to manipulate the Neighbor Cache */
    virtual_device->dev6_add_neighcache_entry = NC6ETH_Add_NeighCache_Entry;
    virtual_device->dev6_del_neighcache_entry = NC6ETH_Delete_NeighCache_Entry;
    virtual_device->dev6_fnd_neighcache_entry = NC6ETH_Find_NeighCache_Entry;

    /* Allocate Memory for the Neighbor Cache */
    status = NU_Allocate_Memory(MEM_Cached, 
                                (VOID**)&virtual_device->dev6_neighbor_cache,
                                (IP6_CFG6_NEIGHCACHE_ENTRIES * 
                                (sizeof(IP6_NEIGHBOR_CACHE_ENTRY))), 
                                NU_NO_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Zero out the Neighbor Cache */
        UTL_Zero(virtual_device->dev6_neighbor_cache, 
                 IP6_CFG6_NEIGHCACHE_ENTRIES * sizeof(IP6_NEIGHBOR_CACHE_ENTRY));

        virtual_device->dev6_nc_entries = IP6_CFG6_NEIGHCACHE_ENTRIES;

        /* Initialize the Prefix List for the device */
        status = PREFIX6_Init_Prefix_List(&virtual_device->dev6_prefix_list);

        if (status != NU_SUCCESS)
        {
            /* Deallocate the memory previously allocated for the Neighbor
             * Cache.
             */
            if (NU_Deallocate_Memory(virtual_device->dev6_neighbor_cache) != NU_SUCCESS)
                NLOG_Error_Log("Failed to deallocate memory for the Neighbor Cache", 
                               NERR_SEVERE, __FILE__, __LINE__);

            virtual_device->dev6_nc_entries = 0;
        }

        /* Init the basic interface information. */
        else
        {
            MIB2_ifDescr_Set(virtual_device, "IPv6 Configured Tunnel");
            MIB2_ifType_Set(virtual_device, 1);
        }
    }

    else
        NLOG_Error_Log("Failed to Create the Neighbor Cache for the 6to4 device", 
                       NERR_SEVERE, __FILE__, __LINE__); 

    return (status);

} /* CFG6_Initialize_Tunnel */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       CFG6_Output
*                                                                         
*   DESCRIPTION                                                           
*                 
*       This is the output routine for data being transmitted out a
*       configured virtual device.
*                                                                        
*   INPUTS                                                                
*                                                        
*       *buf_ptr                A pointer to the buffer to transmit.
*       *device                 A pointer to the device out which to 
*                               transmit the buffer.
*       *dest                   A pointer to the destination.
*       *ro                     A pointer to the route to use.
*                                                                         
*   OUTPUTS                                                               
*                                         
*       NU_SUCCESS              The memory was allocated.
*       NU_HOST_UNREACHABLE     The device is not UP and RUNNING.
*       NU_MSGSIZE              The size of the message is wrong.
*
*************************************************************************/
STATUS CFG6_Output(NET_BUFFER *buf_ptr, DV_DEVICE_ENTRY *device, 
                   VOID *dest, VOID *ro)
{
    STATUS  status;

    UNUSED_PARAMETER(ro);
    UNUSED_PARAMETER(dest);

    /* Verify that the device is up. */
    if ( (device->dev_flags & (DV_UP | DV_RUNNING)) != (DV_UP | DV_RUNNING) )
    {
        NLOG_Error_Log("The interface is not UP and RUNNING", 
                       NERR_SEVERE, __FILE__, __LINE__);

        return (NU_HOST_UNREACHABLE);
    }

    /* Remove the flag indicating this is an IPv6 packet. */
    buf_ptr->mem_flags &= ~NET_IP6;
   
    /* Send the packet via IPv4 */
    status = IP_Send(buf_ptr, NU_NULL, 
                     device->dev_addr.dev_dst_ip_addr, 
                     device->dev_addr.dev_ip_addr, 0, 0, 
                     IPPROTO_IPV6, IP_TYPE_OF_SERVICE, NU_NULL);

    return (status);

} /* CFG6_Output */

#endif
