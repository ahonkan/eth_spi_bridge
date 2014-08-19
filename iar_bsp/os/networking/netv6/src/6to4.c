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
*       6to4.c                                       
*                                                                                  
*   DESCRIPTION                                                              
*                
*       This file contains those functions necessary to use the 6to4 
*       virtual device to create a 6to4 tunnel.
*                                                                          
*   DATA STRUCTURES                                                          
*                                      
*       SIXTOFOUR_Prefix                                    
*                                                                          
*   FUNCTIONS                                                                
*           
*       SIXTOFOUR_Initialize
*       SIXTOFOUR_Output
*
*   DEPENDENCIES                                                             
*               
*       externs.h
*       nc6.h
*       nc6_eth.h
*       prefix6.h
*       6to4.h
*       in6.h
*       dad6.h
*                                                                          
*************************************************************************/

#include "networking/externs.h"
#include "networking/nc6.h"
#include "networking/nc6_eth.h"
#include "networking/prefix6.h"
#include "networking/6to4.h"
#include "networking/in6.h"
#include "networking/dad6.h"

#if (INCLUDE_IPV4 == NU_TRUE)

static  UINT8   SIXTOFOUR_Prefix[2] = {0x20, 0x02};

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       SIXTOFOUR_Initialize
*                                                                         
*   DESCRIPTION                                                           
*                 
*       This function initializes a 6to4 virtual device.          
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
*       NU_INVALID_PARM         The IPv4 device associated with the
*                               virtual device is not a valid device
*                               in the system.
*
*************************************************************************/
STATUS SIXTOFOUR_Initialize(DV_DEVICE_ENTRY *virtual_device)
{
    DV_DEVICE_ENTRY     *ipv4_device;
    UINT8               new_address[IP6_ADDR_LEN];
    STATUS              status;

    virtual_device->dev_type = DVT_6TO4;

    /* Set up the output routine for the virtual device */
    virtual_device->dev_output = SIXTOFOUR_Output;

    virtual_device->dev_flags |= (DV_UP | DV6_VIRTUAL_DEV | DV_MULTICAST);
    virtual_device->dev_flags2 |= DV6_UP;

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

    /* Get a pointer to the IPv4 device that the virtual device
     * will be using.
     */
    ipv4_device = DEV_Get_Dev_By_Name((CHAR*)virtual_device->dev_driver_options);

    if (ipv4_device == NU_NULL)
    {
        NLOG_Error_Log("The device associated with the virtual device is invalid", 
                       NERR_SEVERE, __FILE__, __LINE__);

        return (NU_INVALID_PARM);
    }

    /* RFC 2893 - set the MTU of the virtual device to the MTU of the link-layer
     * under the device (the real IPv4 device) minus the length of the 
     * IPv4 header.
     */
    virtual_device->dev6_link_mtu = 
        virtual_device->dev6_default_mtu = ipv4_device->dev_mtu - IP_HEADER_LEN;

    /* Allocate Memory for the Neighbor Cache */
    status = NU_Allocate_Memory(MEM_Cached,
                                (VOID**)&virtual_device->dev6_neighbor_cache,
                                (IP6_6TO4_NEIGHCACHE_ENTRIES * 
                                (sizeof(IP6_NEIGHBOR_CACHE_ENTRY))), 
                                NU_NO_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Zero out the Neighbor Cache */
        UTL_Zero(virtual_device->dev6_neighbor_cache, 
                 IP6_6TO4_NEIGHCACHE_ENTRIES * sizeof(IP6_NEIGHBOR_CACHE_ENTRY));

        virtual_device->dev6_nc_entries = IP6_6TO4_NEIGHCACHE_ENTRIES;
    }
    else
    {
        NLOG_Error_Log("Failed to Create the Neighbor Cache for the 6to4 device", 
                       NERR_SEVERE, __FILE__, __LINE__);

        return (status);
    }

    /* Initialize the Prefix List for the device */
    status = PREFIX6_Init_Prefix_List(&virtual_device->dev6_prefix_list);

    if (status == NU_SUCCESS)
    {
        /* Add the 6to4 Prefix to the list */
        if (PREFIX6_New_Prefix_Entry(virtual_device, SIXTOFOUR_Prefix, 
                                     16, 0xffffffffUL, 0xffffffffUL, 0) != NU_SUCCESS)
            NLOG_Error_Log("Failed to add the 6to4 Prefix to the Prefix List", 
                           NERR_SEVERE, __FILE__, __LINE__);
    }
    else
    {
        NLOG_Error_Log("Failed to Create the Prefix List for the 6to4 device", 
                       NERR_SEVERE, __FILE__, __LINE__);

        /* Deallocate the memory previously allocated for the Neighbor
         * Cache.
         */
        if (NU_Deallocate_Memory(virtual_device->dev6_neighbor_cache) != NU_SUCCESS)
            NLOG_Error_Log("Failed to Deallocate memory for the Neighbor Cache", 
                           NERR_SEVERE, __FILE__, __LINE__);

        virtual_device->dev6_nc_entries = 0;

        return (status);
    }

    UTL_Zero(new_address, IP6_ADDR_LEN);

    /* Create the 6to4 address from the IPv4 address associated
     * with the IPv4 device.
     */
    memcpy(new_address, SIXTOFOUR_Prefix, 2);

    /* Copy the IPv4 address into the 6to4 address */
    PUT32(new_address, 2, ipv4_device->dev_addr.dev_ip_addr);

    /* Copy the interface identifier of the IPv4 interface into the
     * 6to4 address.
     */
	if ( (ipv4_device->dev6_interface_id_length > 0) &&
		 (ipv4_device->dev6_interface_id_length <= 8) )
	{
	    memcpy(&new_address[IP6_ADDR_LEN - ipv4_device->dev6_interface_id_length], 
	           ipv4_device->dev6_interface_id, 
	           ipv4_device->dev6_interface_id_length);
	}

    /* Add the 6to4 address to the device with an infinite preferred 
     * and valid lifetime.
     */
    status = DEV6_Add_IP_To_Device(virtual_device, new_address, 48,
                                   0xffffffffUL, 0xffffffffUL, 0);

    if (status != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to Add the 6to4 IP address to the 6to4 device", 
                       NERR_SEVERE, __FILE__, __LINE__);

        /* Deallocate the memory previously allocated for the Neighbor
         * Cache.
         */
        if (NU_Deallocate_Memory(virtual_device->dev6_neighbor_cache) != NU_SUCCESS)
            NLOG_Error_Log("Failed to deallocate memory for the Neighbor Cache", 
                           NERR_SEVERE, __FILE__, __LINE__);

        virtual_device->dev6_nc_entries = 0;

        /* Delete the prefix added to the device */
        PREFIX6_Delete_Prefix(virtual_device, SIXTOFOUR_Prefix);

        status = NU_NO_MEMORY;
    }

    /* Add the prefix to the Prefix List for the device. */
    else if (PREFIX6_New_Prefix_Entry(virtual_device, new_address, 48, 
                                      0xffffffffUL, 0xffffffffUL, 
                                      0) != NU_SUCCESS)
        NLOG_Error_Log("Failed to add the Prefix to the Prefix List", 
                       NERR_SEVERE, __FILE__, __LINE__);

    /* Init the basic interface information. */
    /* Init the basic interface information. */
    MIB2_ifDescr_Set(virtual_device, "IPv6 6to4 Tunnel");
    MIB2_ifType_Set(virtual_device, 1);

    return (status);

} /* SIXTOFOUR_Initialize */

/*************************************************************************
*
*   FUNCTION                                                              
*                                                                         
*       SIXTOFOUR_Output
*                                                                         
*   DESCRIPTION                                                           
*                 
*       This is the output routine for data being transmitted out a
*       6to4 virtual device.
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
*       NU_NO_MEMORY            Insufficient memory.
*       NU_HOST_UNREACHABLE     The device is not UP and RUNNING.
*       NU_INVALID_ADDRESS      There is not an address of appropriate
*                               scope on the IPv6 virtual device.
*
*************************************************************************/
STATUS SIXTOFOUR_Output(NET_BUFFER *buf_ptr, DV_DEVICE_ENTRY *device, 
                        VOID *dest, VOID *ro)
{
    UINT8   *source_ip;
    STATUS  status;

    UNUSED_PARAMETER(ro);

    /* Verify that the device is up. */
    if ( (device->dev_flags & (DV_UP | DV_RUNNING)) != (DV_UP | DV_RUNNING) )
    {
        NLOG_Error_Log("The interface is not UP and RUNNING", 
                       NERR_SEVERE, __FILE__, __LINE__);

        return (NU_HOST_UNREACHABLE);
    }

    /* Get a pointer to the 6to4 address on the interface */
    source_ip = in6_ifawithifp(device, ((SCK6_SOCKADDR_IP*)(dest))->sck_addr);

    if (source_ip != NU_NULL)
    {
        /* Remove the flag indicating this is an IPv6 packet. */
        buf_ptr->mem_flags &= ~NET_IP6;
   
        /* Send the packet via IPv4 */
        status = IP_Send(buf_ptr, NU_NULL, 
                         IP_ADDR(&((SCK6_SOCKADDR_IP*)(dest))->sck_addr[2]), 
                         IP_ADDR(&source_ip[2]), 0, 0, IPPROTO_IPV6, 
                         IP_TYPE_OF_SERVICE, NU_NULL);
    }

    /* Otherwise, there is no 6to4 address on this interface */
    else
    {
        status = NU_INVALID_ADDRESS;

        NLOG_Error_Log("No 6to4 address associated with the 6to4 device", 
                        NERR_RECOVERABLE, __FILE__, __LINE__);

    }

    return (status);

} /* SIXTOFOUR_Output */

#endif
