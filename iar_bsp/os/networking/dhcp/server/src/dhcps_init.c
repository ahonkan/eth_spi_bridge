/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2012
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
*     dhcps_init.c
*
* COMPONENT
*
*     Nucleus DHCP Server
*
* DESCRIPTION
*
*     This file holds the database processing functions of the DHCP server.
*     The database contains all of the binding and configuration information
*     the server will use to assign IP addresses and configuration information.
*
* DATA STRUCTURES
*
*     None
*
* FUNCTIONS
*
*     nu_os_net_dhcps_init
*     DHCPS_Server_Init_Task
*
* DEPENDENCIES
*
*     nucleus.h
*     nucleus_gen_cfg.h
*     networking/nu_networking.h
*
************************************************************************/
/* Includes */
#include "nucleus.h"
#include "nucleus_gen_cfg.h"
#include "networking/nu_networking.h"
#include "os/networking/dhcp/server/inc/dhcps_ext.h"

/* Task parameters */
#define DHCPS_NORMAL_TASK_PRIORITY            (20)
#define DHCPS_TASK_STACK_SIZE                 3000

/* Define Application data structures.  */
NU_MEMORY_POOL          *DHCPS_Mem_Pool;
NU_MEMORY_POOL          *DHCPServer_Memory_Pool_Ptr;
NU_TASK                 dhcpsrv_init_task;

DHCPS_CONFIG_CB         *global_config = NU_NULL, *device1_config = NU_NULL;

/* Define prototypes for function references.  */
VOID DHCPS_Server_Init_Task(UNSIGNED argc, VOID *argv);
/******************************************************************************
*
* FUNCTION
*
*      nu_os_net_dhcp_server_init
*
* DESCRIPTION
*
*      Initializes the tasks, queues and events used by the demonstration
*      application.
*
* INPUTS
*
*      key - registry key for component specific settings (Unused).
*
*      startstop - value to specify whether to start (NU_START)
*                  or stop (NU_STOP) a given component.
*
* OUTPUTS
*
*      NU_SUCCESS - Indicates successful operation.
*
*
******************************************************************************/
STATUS nu_os_net_dhcp_server_init (const CHAR * key, INT startstop)
{

    VOID           *pointer;
    STATUS         status;

    /* Remove compiler warnings */
    UNUSED_PARAMETER(key);
    UNUSED_PARAMETER(startstop);

    status = NU_System_Memory_Get(&DHCPS_Mem_Pool, NU_NULL);

    if (status == NU_SUCCESS)
    {
    /* Set the DHCP server memory pool pointer to point to the system_pool */
    status = NU_System_Memory_Get(&DHCPServer_Memory_Pool_Ptr, NU_NULL);

        if (status == NU_SUCCESS)
        {
            /* Allocate stack space for and create each task in the system.  */

            /* Allocate stack space for and create the DHCPServ_Demo Task. */
            status = NU_Allocate_Memory(DHCPS_Mem_Pool, &pointer, DHCPS_TASK_STACK_SIZE, NU_NO_SUSPEND);

            if (status == NU_SUCCESS)
            {
                /* Create the DHCPS_Server_Init_Task task. */
                status = NU_Create_Task(&dhcpsrv_init_task, "DHCPSRVINIT",  DHCPS_Server_Init_Task, 0, NU_NULL, pointer,
                                        DHCPS_TASK_STACK_SIZE, DHCPS_NORMAL_TASK_PRIORITY, 0, NU_PREEMPT, NU_START);

                if (status != NU_SUCCESS)
                {
                    NU_Deallocate_Memory(DHCPS_Mem_Pool);
                }
            }
        }
    }

    return status;

}
/******************************************************************************************
*
* FUNCTION
*
*      DHCPS_Server_Init_Task
*
* DESCRIPTION
*
*      Initialize and setup the DHCP server with desired parameters.
*
* INPUTS
*
*      argc - Not used.
*
*      argv - Not used.
*
* OUTPUTS
*
*      None
*
******************************************************************************************/
VOID DHCPS_Server_Init_Task (UNSIGNED argc, VOID *argv)
{
    STATUS              status;

#if (CFG_NU_OS_NET_DHCP_SERVER_INCLUDE_DNS_SERVER == NU_TRUE)

    struct id_struct    dns_server_1,
                        dns_server_2;

#endif

#if (CFG_NU_OS_NET_DHCP_SERVER_INCLUDE_ROUTER == NU_TRUE)

    struct id_struct    router;

#endif

    /* Configuration Parameters */
    CHAR    DHCPSrv_Subnet_Mask[IP_ADDR_LEN];
    CHAR    DHCPSrv_Broadcast_Addr[IP_ADDR_LEN];
    CHAR    DHCPSrv_Subnet_Addr[IP_ADDR_LEN];
    UINT32  DHCPSrv_Lease_Time = CFG_NU_OS_NET_DHCP_SERVER_LEASE_TIME;
    UINT32  DHCPSrv_Renewal_T1_Time = CFG_NU_OS_NET_DHCP_SERVER_RENEWAL_T1_TIME;
    UINT32  DHCPSrv_Rebind_T2_Time = CFG_NU_OS_NET_DHCP_SERVER_REBIND_T2_TIME;
    UINT32  DHCPSrv_Offered_Wait_Time = CFG_NU_OS_NET_DHCP_SERVER_OFFERED_WAIT_TIME;

    #if (CFG_NU_OS_NET_DHCP_SERVER_INCLUDE_ROUTER == NU_TRUE)
    CHAR    DHCPSrv_Router1_Address[IP_ADDR_LEN];
    #endif

    #if (CFG_NU_OS_NET_DHCP_SERVER_INCLUDE_DNS_SERVER == NU_TRUE)
    CHAR    DHCPSrv_Domain_Server1_Addr[IP_ADDR_LEN];
    CHAR    DHCPSrv_Domain_Server2_Addr[IP_ADDR_LEN];
    #endif

    #if (CFG_NU_OS_NET_DHCP_SERVER_INCLUDE_DOMAIN_NAME == NU_TRUE)
    CHAR    *DHCPSrv_Domain_Name = CFG_NU_OS_NET_DHCP_SERVER_DOMAIN_NAME;
    #endif

    CHAR    DHCPSrv_IP_Range_Low[IP_ADDR_LEN];
    CHAR    DHCPSrv_IP_Range_High[IP_ADDR_LEN];

    struct id_struct    subnet_mask,
                        subnet_addr,
                        broadcast_addr,
                        ip_range_low,
                        ip_range_high;

    UINT8               binding_buff[40],
                        *buff_ptr;
    INT                 buff_size = 40;

    /* Remove compiler warnings */
    UNUSED_PARAMETER(argc);
    UNUSED_PARAMETER(argv);

    /* Get configuration addresses for the DHCP server */
    status = NU_Inet_PTON(2, CFG_NU_OS_NET_DHCP_SERVER_SUBNET_MASK, DHCPSrv_Subnet_Mask);

#if (CFG_NU_OS_NET_DHCP_SERVER_INCLUDE_ROUTER == NU_TRUE)

    if (status == NU_SUCCESS)
    {
        status = NU_Inet_PTON(2, CFG_NU_OS_NET_DHCP_SERVER_ROUTER1_ADDRESS, DHCPSrv_Router1_Address);
    }

#endif

#if (CFG_NU_OS_NET_DHCP_SERVER_INCLUDE_DNS_SERVER == NU_TRUE)

    if (status == NU_SUCCESS)
    {
        status = NU_Inet_PTON(2, CFG_NU_OS_NET_DHCP_SERVER_DOMAIN_SERVER1_ADDR, DHCPSrv_Domain_Server1_Addr);
    }

    if (status == NU_SUCCESS)
    {
        status = NU_Inet_PTON(2, CFG_NU_OS_NET_DHCP_SERVER_DOMAIN_SERVER2_ADDR, DHCPSrv_Domain_Server2_Addr);
    }

#endif

    if (status == NU_SUCCESS)
    {
        status = NU_Inet_PTON(2, CFG_NU_OS_NET_DHCP_SERVER_BROADCAST_ADDR, DHCPSrv_Broadcast_Addr);
    }

    if (status == NU_SUCCESS)
    {
        status = NU_Inet_PTON(2, CFG_NU_OS_NET_DHCP_SERVER_SUBNET_ADDR, DHCPSrv_Subnet_Addr);
    }

    if (status == NU_SUCCESS)
    {
        status = NU_Inet_PTON(2, CFG_NU_OS_NET_DHCP_SERVER_IP_RANGE_LOW, DHCPSrv_IP_Range_Low);
    }

    if (status == NU_SUCCESS)
    {
        status = NU_Inet_PTON(2, CFG_NU_OS_NET_DHCP_SERVER_IP_RANGE_HIGH, DHCPSrv_IP_Range_High);
    }

    if(status == NU_SUCCESS)
    {
        /* Now the DHCP Server initialization function can be called */
        status = DHCPS_Init(DHCPServer_Memory_Pool_Ptr,CFG_NU_OS_NET_DHCP_SERVER_DIRECTORY_PATH,
                    CFG_NU_OS_NET_DHCP_SERVER_CONFIGURATION_FILE_NAME, CFG_NU_OS_NET_DHCP_SERVER_BINDING_FILE_NAME,
                    CFG_NU_OS_NET_DHCP_SERVER_BINDING_FILE_BACKUP, CFG_NU_OS_NET_DHCP_SERVER_OPTIONS_BLOCK_FILE_NAME);
    }

    if (status == NU_SUCCESS)
    {
        /* Create a global configuration global control block. The call will return failure if DHCPS_Init
           found already saved configuration files */
        if (DHCPS_Create_Config_Entry(&global_config, "GLOBAL", 0) == NU_SUCCESS)
        {
            /* We can now add the configuration parameters to the global control block. */

            /* Copy the subnet mask into a structure. */
            memcpy(&subnet_mask, DHCPSrv_Subnet_Mask, IP_ADDR_LEN);

            /* Add the Subnet mask to the global control block. */
            status = DHCPS_Set_Subnet_Mask(global_config, NU_NULL, &subnet_mask);

            if (status == NU_SUCCESS)
            {
                /* Copy the broadcast address into a structure. */
                memcpy(&broadcast_addr, DHCPSrv_Broadcast_Addr, IP_ADDR_LEN);

                /* Add the broadcast address to the global control block. */
                status = DHCPS_Add_Broadcast_Address(global_config, NU_NULL, &broadcast_addr);
            }

#if (CFG_NU_OS_NET_DHCP_SERVER_INCLUDE_DNS_SERVER == NU_TRUE)

            if (status == NU_SUCCESS)
            {
                /* Copy a DNS server IP address into a structure. */
                memcpy(&dns_server_1, DHCPSrv_Domain_Server1_Addr, IP_ADDR_LEN);

                /* Add the DNS server to the global control block. */
                status = DHCPS_Add_DNS_Server(global_config, NU_NULL, &dns_server_1);

                if (status == NU_SUCCESS)
                {
                    /* Add another DNS server to the global control block. */
                    memcpy(&dns_server_2, DHCPSrv_Domain_Server2_Addr, IP_ADDR_LEN);

                    status = DHCPS_Add_DNS_Server(global_config, NU_NULL, &dns_server_2);
                }
            }
#endif

#if (CFG_NU_OS_NET_DHCP_SERVER_INCLUDE_ROUTER == NU_TRUE)

            if (status == NU_SUCCESS)
            {
                /* Copy a router IP address into a structure. */
                memcpy(&router, DHCPSrv_Router1_Address, IP_ADDR_LEN);

                /* Add the router to the control block. */
                status = DHCPS_Add_Router(global_config, NU_NULL, &router);
            }

#endif

#if (CFG_NU_OS_NET_DHCP_SERVER_INCLUDE_DOMAIN_NAME == NU_TRUE)

            if (status == NU_SUCCESS)
            {
                /* Add the domain name to the global control block. */
                status = DHCPS_Add_Domain_Name(global_config, NU_NULL, DHCPSrv_Domain_Name);
            }
#endif

            if (status == NU_SUCCESS)
            {
                /* Add the lease time to the global control block. */
                status = DHCPS_Set_Default_Lease_Time(global_config, NU_NULL, DHCPSrv_Lease_Time);
            }

            if (status == NU_SUCCESS)
            {
                /* Add the renewal time to the global control block. */
                status = DHCPS_Set_Renewal_Time(global_config, NU_NULL, DHCPSrv_Renewal_T1_Time);
            }

            if (status == NU_SUCCESS)
            {
                /* Add the rebinding time to the global control block. */
                status = DHCPS_Set_Rebind_Time(global_config, NU_NULL, DHCPSrv_Rebind_T2_Time);
            }

            if (status == NU_SUCCESS)
            {
                /* Add the offered wait time to the global control block. */
                status = DHCPS_Set_Offered_Wait_Time(global_config, NU_NULL, DHCPSrv_Offered_Wait_Time);
            }

            if (status == NU_SUCCESS)
            {
                /* Add a new configuration control block for the IP range that we are about to add. */
                if (DHCPS_Create_Config_Entry(&device1_config, "SUBNET1", CFG_NU_OS_NET_DHCP_SERVER_INTERFACE_NAME) == NU_SUCCESS)
                {
                    /* Copy the IP range hi and low into id_structs. */
                    memcpy(&ip_range_low, DHCPSrv_IP_Range_Low, IP_ADDR_LEN);
                    memcpy(&ip_range_high, DHCPSrv_IP_Range_High, IP_ADDR_LEN);

                    /* Add IP range to the control block. */
                    status = DHCPS_Add_IP_Range(device1_config, NU_NULL, &ip_range_low, &ip_range_high);

                    if (status == NU_SUCCESS)
                    {
                        /* Copy the subnet address into an id_struct. */
                        memcpy(&subnet_addr, DHCPSrv_Subnet_Addr, IP_ADDR_LEN);

                        /* Add the subnet address to the control block. */
                        status = DHCPS_Set_Subnet_Address(device1_config, NU_NULL, &subnet_addr);
                    }

                    if (status == NU_SUCCESS)
                    {
                        /* Enable the device control block. */
                        status = DHCPS_Enable_Configuration(device1_config);
                    }

                    if (status == NU_SUCCESS)
                    {
                        /* Set a pointer to the head of the buffer. */
                        buff_ptr = &binding_buff[0];

                        /* Get the IP addresses of all of the bindings. */
                        DHCPS_Get_IP_Range(device1_config, buff_ptr, buff_size);
                    }
                }
            }

            if (status == NU_SUCCESS)
            {
                /* Enable the global configuration control block. */
                status = DHCPS_Enable_Configuration(global_config);
            }
        }

        if (status == NU_SUCCESS)
        {
            /* Save off all of the configuration control blocks to a file. */
            status = DHCPS_Save_Config_Data_To_File();
        }
    }

    /* Set flag to indicate that the DHCP server is ready */
    DHCPS_Initialized = NU_TRUE;
}
