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
*
*   FILENAME
*
*       dns.c
*
*   DESCRIPTION
*
*       This file contains the Domain Name System (DNS) component.  Given a
*       name this component will discover the matching IP address.
*
*   DATA STRUCTURES
*
*       NONE
*
*   FUNCTIONS
*
*       DNS_Initialize
*       DNS_Create_Host_Entry
*
*   DEPENDENCIES
*
*       nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

/* This is a list of DNS servers or resolvers. */
extern DNS_SERVER_LIST      DNS_Servers;

/* This is the list of hosts.  If a host can not be found in this list,
   DNS will be used to retrieve the information.  The new host will be
   added to the list. */
extern DNS_HOST_LIST        DNS_Hosts;
UNSIGNED                    DNS_Id;

#ifdef VIRTUAL_NET_INCLUDED
extern STATUS  VDRV_DNS_Init(VOID);
#endif

extern struct host hostTable[];


#if (INCLUDE_STATIC_BUILD == NU_TRUE)

/* Declare Memory for DNS hosts, initial  */
DNS_HOST    DNS_Host_Memory[NET_MAX_INIT_HOSTS];

#if (INCLUDE_DNS == NU_TRUE)
/* Declare Memory for DNS Servers */
DNS_SERVER  DNS_Server_Memory[DNS_MAX_DNS_SERVERS];
#endif

/* Declare Memory for new DNS hosts */
CHAR        DNS_New_Host_Memory[NET_HOSTS_MEMORY * NET_MAX_NEW_HOSTS];

/* Declare memory flags array for the array above */
UINT8       DNS_New_Host_Memory_Flags[NET_MAX_NEW_HOSTS] = {0};

#endif      /* INCLUDE_STATIC_BUILD */

/****************************************************************************
*
*   FUNCTION
*
*       DNS_Initialize
*
*   DESCRIPTION
*
*       This function initializes the DNS component.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       NU_SUCCESS              Indicates successful operation.
*       NU_MEM_ALLOC            Not enough dynamic memory available.
*
******************************************************************************/
STATUS DNS_Initialize(VOID)
{
    struct host         *hst;
    STATUS              status;

#if (INCLUDE_DNS == NU_TRUE)
    INT                 i;
    DNS_SERVER          *dns_server;
#endif

#if (INCLUDE_DNS == NU_TRUE)

    /* Create a semaphore to protect DNS globals. */
    status = NU_Create_Semaphore(&DNS_Resource, "DNS", (UNSIGNED)1, NU_FIFO);

    if (status == NU_SUCCESS)
    {
        /* Obtain the semaphore. */
        status = NU_Obtain_Semaphore(&DNS_Resource, NU_NO_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* Create a list of DNS servers. Initially there will be
               DNS_MAX_DNS_SERVERS entries in the list. All entries will have a null
               ip address initially. That is the list will be empty until an
               application adds a DNS_Server. */

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
            /* Allocate a block of memory to build the DNS server list with. */
            status = NU_Allocate_Memory (MEM_Cached, (VOID **)&dns_server,
                                         sizeof (DNS_SERVER) * DNS_MAX_DNS_SERVERS,
                                         NU_NO_SUSPEND);

            if (status == NU_SUCCESS)
            {
#else
                /* Assign memory to the dns server */
                dns_server = DNS_Server_Memory;

#endif  /* INCLUDE_STATIC_BUILD */

                DNS_Servers.dnss_head = NU_NULL;
                DNS_Servers.dnss_tail = NU_NULL;

                /* Generate a random initial ID. */
                DNS_Id = UTL_Rand();

                /* Zero is not a valid ID. */
                if (DNS_Id == 0)
                    DNS_Id ++;

                for (i = 0; i < DNS_MAX_DNS_SERVERS; i++, dns_server++)
                {
                    /* Clear the IP address. */
                    UTL_Zero(dns_server, sizeof(DNS_SERVER));

                    /* Add this host to the list. */
                    DLL_Enqueue(&DNS_Servers, dns_server);
                }
            }
        }
    }
#else

    status = NU_SUCCESS;

#endif /* INCLUDE_DNS == NU_TRUE */

    if (status == NU_SUCCESS)
    {
        DNS_Hosts.dns_head = NU_NULL;
        DNS_Hosts.dns_tail = NU_NULL;

        /* Add each host in the Host Table in hosts.c to the Host Table. */
        for (hst = hostTable; hst->name[0]; hst++)
        {
            /* Create a new Host Entry for this Host. */
            if (DNS_Add_Host(hst->name, (CHAR*)hst->address, 0, hst->h_family,
#if (INCLUDE_IPV4 == NU_TRUE)
#if (INCLUDE_IPV6 == NU_TRUE)
                             hst->h_family == NU_FAMILY_IP ?
#endif
                             DNS_TYPE_A
#if (INCLUDE_IPV6 == NU_TRUE)
                             :
#endif
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
                             DNS_TYPE_AAAA
#endif
                             , 1, DNS_PERMANENT_ENTRY, NU_NULL) == NU_NULL)
            {
                NLOG_Error_Log("Error adding host to DNS table", NERR_SEVERE,
                               __FILE__, __LINE__);
            }
            else
            {
                /* Add a new PTR record */
                if (DNS_Add_Host(hst->name, (CHAR*)hst->address, 0, hst->h_family,
                                 DNS_TYPE_PTR,
                                 1, DNS_LOCAL_RECORD | DNS_PERMANENT_ENTRY, NU_NULL) == NU_NULL)
                {
                    NLOG_Error_Log("Error adding PTR Record to DNS table", NERR_SEVERE,
                                   __FILE__, __LINE__);
                }
            }
        }

#ifdef VIRTUAL_NET_INCLUDED
        status = VDRV_DNS_Init();
#endif

#if (INCLUDE_DNS == NU_TRUE)
        NU_Release_Semaphore(&DNS_Resource);
#endif
    }

    return (status);

} /* DNS_Initialize */

/****************************************************************************
*
*   FUNCTION
*
*       DNS_Create_Host_Entry
*
*   DESCRIPTION
*
*       This function allocates memory for and fills in a NU_HOSTENT data
*       structure to be returned to the application.
*
*   INPUTS
*
*       *name                   A pointer to the name to put in the entry.
*       family                  The family type of the record.
*       *ip_addr_buff           A pointer to the IP addresses for the record.
*       *error_num              A pointer to the status of the creation.
*
*   OUTPUTS
*
*       NU_HOSTENT*             A pointer to the new host entry data
*                               structure.
*       NU_NULL                 Memory could not be allocated.
*
******************************************************************************/
NU_HOSTENT *DNS_Create_Host_Entry(CHAR* name, INT16 family, const CHAR* ip_addr_buff,
                                  STATUS *error_num)
{
    NU_HOSTENT      *host_entry = NU_NULL;
    NU_MEMORY_POOL  *pool;
    INT             i;

    pool = MEM_Cached;

#if (INCLUDE_STATIC_BUILD == NU_FALSE)
    /* Allocate memory for the new entry.  Be sure to allocate enough memory
     * for the NULL entry at the end of the address list.
     */
    *error_num = NU_Allocate_Memory(pool, (VOID**)&host_entry,
                                    sizeof(NU_HOSTENT) +
                                       (sizeof(CHAR*) * (DNS_MAX_IP_ADDRS + 1)) +
                                       ((DNS_MAX_IP_ADDRS + 1) * MAX_ADDRESS_SIZE),
                                    NU_NO_SUSPEND);
#else

    /* Find an unused memory */
    for(i=0; (DNS_New_Host_Memory_Flags[i] != NU_FALSE) && (i != NET_MAX_NEW_HOSTS); i++)
        ;
    if(i != NET_MAX_NEW_HOSTS)
    {
        /* Assign the unused memory*/
        host_entry = (NU_HOSTENT *)&DNS_New_Host_Memory[NET_HOSTS_MEMORY * i];
        /* Turn on the memory flag */
        DNS_New_Host_Memory_Flags[i] = NU_TRUE;
        *error_num = NU_SUCCESS;
    }
    else
        *error_num = NU_NO_MEMORY;          /* if free memory could not be found */

#endif  /* INCLUDE_STATIC_BUILD */

    if (*error_num == NU_SUCCESS)
    {
        host_entry->h_name = name;
        host_entry->h_alias = (CHAR **)NU_NULL;
        host_entry->h_addrtype = family;

#if (INCLUDE_IPV6 == NU_TRUE)
        if (family == NU_FAMILY_IP6)
            host_entry->h_length = IP6_ADDR_LEN;
#endif

#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV4 == NU_TRUE) )
        else
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
            host_entry->h_length = IP_ADDR_LEN;
#endif

        host_entry->h_addr_list = (CHAR**)(&host_entry[1]);

        /* Setup a list of pointers such that each pointer would point to a unique
         * IP address resolved by DNS.
         */
        for (i = 0; i < DNS_MAX_IP_ADDRS; i++)
            *(host_entry->h_addr_list + i) = (CHAR*)(host_entry->h_addr_list +
                                                     (DNS_MAX_IP_ADDRS + 1)  +
                                                     (i * (MAX_ADDRESS_SIZE/4)));

        /* Zero out the NULL entry. */
        *(host_entry->h_addr_list + i) = NU_NULL;

        /* Copy the addresses that were resolved via DNS query into the
         * address list.  If no addresses were resolved, the memory is still
         * copied, because the memory needs to be zeroed out.
         */
        memcpy(*host_entry->h_addr_list, ip_addr_buff,
               DNS_MAX_IP_ADDRS * MAX_ADDRESS_SIZE);

        /* Zero out the NULL entry. */
        memset(&((*host_entry->h_addr_list)[DNS_MAX_IP_ADDRS * MAX_ADDRESS_SIZE]), 0, MAX_ADDRESS_SIZE);
    }
    else
        *error_num = NU_NO_MEMORY;

    return (host_entry);

} /* DNS_Create_Host_Entry */



