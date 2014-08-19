/*************************************************************************
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
*     dhcpsdb.c                                                
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
*     DHCPSDB_Process_Client_ID                                        
*     DHCPSDB_Process_Append                                        
*     DHCPSDB_Process_IP                                        
*     DHCPSDB_Process_Host_Long                                        
*     DHCPSDB_Process_Host_Short                                        
*     DHCPSDB_Process_String                                        
*     DHCPSDB_Process_Range                                        
*     DHCPSDB_Eat_Whitespace                                        
*     DHCPSDB_Get_String                                        
*     DHCPSDB_Calculate_Subnet_Number                                        
*     DHCPSDB_IP_Compare                                        
*     DHCPSDB_Adjust_Buffer                                        
*     DHCPSDB_Supported_Options_List                         
*     DHCPSDB_Isspace                                        
*                                                                      
* DEPENDENCIES                                                         
*
*     nucleus_gen_cfg.h
*     networking/nu_networking.h
*     os/networking/dhcp/server/inc/dhcps_ext.h
*     os/networking/dhcp/server/inc/dhcpsrv.h
*
************************************************************************/
#include "nucleus_gen_cfg.h"
#include "networking/nu_networking.h"
#include "os/networking/dhcp/server/inc/dhcps_ext.h"
#include "os/networking/dhcp/server/inc/dhcpsrv.h"

/*************************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPSDB_Process_Client_ID                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     This function will handle all of the neccessary calls to create a static
*     binding configuration.
*
* INPUTS                                                               
*
*     option_name:        Option name of the parameter being processed.
*     **symbol:           Ptr to ptr to the buffer that is being processed.
*     *config:            Ptr to the configuration structure in which the parameter
*                           value will be placed.
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     INT                                    
*                                                                      
**************************************************************************************/
INT DHCPSDB_Process_Client_ID(INT option_name, CHAR **symbol, DHCPS_CONFIG_CB *config)
{
    DHCPS_CONFIG_CB     *config_cb;
    DHCPS_CLIENT_ID     client_id;
    struct id_struct    static_ip;
    CHAR                config_entry_name[DHCPS_MAX_ENTRY_NAME_LENGTH];
    STATUS              status,
                        ret_status = NU_SUCCESS;

    /* Remove compiler warning */
    UNUSED_PARAMETER (option_name);

    /* Copy the client ID type into the ID structure. */
    client_id.idtype = **symbol;    


    /* Adjust the buffer  */
    DHCPSDB_Adjust_Buffer(symbol);

    /* If the id type is not a unique string of characters, we will read in the 6 byte address.  
        If it is not, then we will read in a string.  This is done due to the possibility 
        of a null character being in the ethernet hardware address */
    if (client_id.idtype == ETHERNET_DEVICE)
    {
        /* Copy the client's hardware address into the client ID struct. */
        memcpy(client_id.id, *symbol, DADDLEN);
        client_id.idlen = DADDLEN;
        *symbol += DADDLEN;
    }   

    else
    {
        /* Since the ID can be a string of variable length, just copy the string into the struct. */
        strcpy((CHAR *)client_id.id, *symbol);
        client_id.idlen = (UINT8)strlen((CHAR *)client_id.id);
        *symbol += client_id.idlen;
    }   

    /* Adjust the buffer pointer to just past the entry delimiter (:) */
    DHCPSDB_Adjust_Buffer(symbol);

    /* Get the entry name of the configuration that will contain the static binding. */
    status = DHCPSDB_Get_String(symbol, config_entry_name);

    if (status == NU_SUCCESS)
    {
        /* Protect the global data structures. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

        /* Search the configuration link list for a match of the entry name. */
        for (config_cb = DHCPS_Config_List.dhcp_config_head;
             config_cb;
             config_cb = config_cb->dhcps_config_next)
        {
            if (strcmp(config_entry_name, config_cb->config_entry_name) == 0)
            {
                /* We have found our match. */
                break;
            }
                     
        }         

        /* Unprotect the global data structures. */
        NU_Release_Semaphore(&DHCPS_Semaphore);

        /* This could possibly be a static binding. */ 
        if (config_cb != NU_NULL)
        {
            /* Adjust the buffer pointer to just past the entry delimiter (:) */
            DHCPSDB_Adjust_Buffer(symbol);

            /* Get the IP address of the static binding and place it in a id_struct. */
            memcpy(static_ip.is_ip_addrs, *symbol, IP_ADDR_LEN);

            /* We can now add the binding to the configuation control block. */
            status = DHCPS_Add_IP_Range(config, &client_id, &static_ip, NU_NULL);
        }    
    }    

    return(ret_status);
}

/************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPSDB_Process_Append                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Copy the values of one configuration into another. Can be used to
*     place global values into a configuration.
*
* INPUTS                                                               
*
*     option_name:        Option name of the parameter being processed.
*     **symbol:           Ptr to ptr to the buffer that is being processed.
*     *config:            Ptr to the configuration structure in which the 
*                               parameter value will be placed.
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     INT                                    
*                                                                      
************************************************************************/
STATIC INT DHCPSDB_Process_Append(INT option_name, CHAR **symbol, DHCPS_CONFIG_CB *config1)
{
    DHCPS_CONFIG_CB  *config2 = NU_NULL;
    CHAR             tempstr[DHCPS_MAX_STRING_LEN],
                     config_name[DHCPS_MAX_ENTRY_NAME_LENGTH];

    INT              ret_status = NU_SUCCESS;

    /* Remove compiler warning */
    UNUSED_PARAMETER (option_name);

    /* Copy the configuration control block name that we are to append into a char string. */
    ret_status = DHCPSDB_Get_String(symbol, tempstr);

    /* Save the entry name of the configuration that we are about to append. */
    strcpy(config_name, config1->config_entry_name);

    /* Protect the global data structures. */
    NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);
    
    /* Search through all of the array of configuration structures to find a match */
    for (config2 = DHCPS_Config_List.dhcp_config_head;
         config2;
         config2 = config2->dhcps_config_next)
    {
        if (strcmp(tempstr, config2->config_entry_name) == 0)
        {
            /* Copy the entire control block. */
            memcpy(config1, config2, sizeof(DHCPS_CONFIG_CB));

            /* Place the name of the control block back into the structure. */
            strcpy(config1->config_entry_name, config_name);

            break;
        }
    }
    /* Unprotect the global data structures. */
    NU_Release_Semaphore(&DHCPS_Semaphore);

    if (!config2)
    {
        ret_status = DHCPSERR_CONFIGURATION_NOT_FOUND;
    }
    return(ret_status);
}


/************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPSDB_Process_IP                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Read in any IP address and place it into the appropriate config
*     structure entry.
*
* INPUTS                                                               
*
*     option_name:        Option name of the parameter being processed.
*     **symbol:           Ptr to ptr to the buffer that is being processed.
*     *config:            Ptr to the configuration structure in which the parameter
*                              value will be placed.
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     INT                                    
*                                                                      
************************************************************************/
INT DHCPSDB_Process_IP (INT option_name, CHAR **symbol, DHCPS_CONFIG_CB *config)
{
    STATUS  ret_status = NU_SUCCESS;

#if ((CFG_NU_OS_NET_DHCP_SERVER_INCLUDE_ROUTER == NU_TRUE) ||                \
                (CFG_NU_OS_NET_DHCP_SERVER_INCLUDE_DNS_SERVER == NU_TRUE))

    CHAR    test_char;
    struct  id_struct option_ip_address;

#endif

    switch (option_name) 
    {
        case SIADDR:
            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* Move the Server address from the buffer into the 
                configuration structure */
            memcpy(config->server_ip_addr, *symbol, IP_ADDR_LEN);

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);

            *symbol += IP_ADDR_LEN;
            break;


        case SUBNET_MASK:
            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* Move the subnet mask from the buffer into the 
                configuration structure */
            memcpy(config->subnet_mask_addr, *symbol, IP_ADDR_LEN);

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);

            *symbol += IP_ADDR_LEN;
            break;

        case SUBNET_ADDRESS:
            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* Move the subnet address from the buffer into the 
                configuration structure */
            memcpy(config->subnet_addr, *symbol, IP_ADDR_LEN);

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);

            *symbol += IP_ADDR_LEN;
            break;            

        case BRDCAST_ADDR:
            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* Move the broadcast address from the buffer into the 
                configuration structure */
            memcpy(config->broadcast_addr, *symbol, IP_ADDR_LEN);

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);

            *symbol += IP_ADDR_LEN;
            break;

        case ROUTER:
#if (CFG_NU_OS_NET_DHCP_SERVER_INCLUDE_ROUTER == NU_TRUE)
            /* Ensure that there is actually a router address to add. */
            memcpy(&test_char, *symbol, 1);

            while (test_char != COLON)
            {
                /* Protect the global data structures. */
                NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                /* Move the router address from the buffer into the 
                    option ip structure */
                memcpy(option_ip_address.is_ip_addrs, *symbol, IP_ADDR_LEN);

                /* Unprotect the global data structures. */
                NU_Release_Semaphore(&DHCPS_Semaphore);

                /* Call the function to add the router to the link list. */
                ret_status = DHCPS_Add_Router(config, NU_NULL, &option_ip_address);
            
                /* If an error occurred during the adding of the router, exit. */          
                if (ret_status != NU_SUCCESS)
                    break;

                *symbol += IP_ADDR_LEN;

                /* Determine if there are more router addresses available */
                memcpy(&test_char, *symbol, 1);
            }
#endif          
            break;

        case DNS_SERVER_ADDR:
#if (CFG_NU_OS_NET_DHCP_SERVER_INCLUDE_DNS_SERVER == NU_TRUE)
            /* Ensure that there is an address to add. */
            memcpy(&test_char, *symbol, 1);

            while (test_char != COLON)
            {
                /* Protect the global data structures. */
                NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                /* Move the dns server address from the buffer into the 
                    option ip structure */
                memcpy(option_ip_address.is_ip_addrs, *symbol, IP_ADDR_LEN);

                /* Unprotect the global data structures. */
                NU_Release_Semaphore(&DHCPS_Semaphore);

                /* Call the function to add the DNS server to the link list. */
                ret_status = DHCPS_Add_DNS_Server(config, NU_NULL, &option_ip_address);

                /* If an error occurred during the adding of the server, exit. */          
                if (ret_status != NU_SUCCESS)
                    break;

                *symbol += IP_ADDR_LEN;

                /* Determine if there are more router addresses available */
                memcpy(&test_char, *symbol, 1);
            }
#endif          
            break;

        default:
            break;  
    }
    return(ret_status);
}


/************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPSDB_Process_Host_Long                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Read in a long size variable from a configuration file and 
*     place it into the appropriate config structure entry.
*
* INPUTS                                                               
*
*     option_name:        Option name of the parameter being processed.
*     **symbol:           Ptr to ptr to the buffer that is being processed.
*     *config:            Ptr to the configuration structure in which the parameter
*                              value will be placed.
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     INT                                    
*                                                                      
************************************************************************/
INT DHCPSDB_Process_Host_Long(INT option_name, CHAR **symbol,DHCPS_CONFIG_CB *config)
{

    switch (option_name) 
    {
        case MAX_LEASE:
            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            memcpy(&config->max_lease_length, *symbol, sizeof(UINT32));

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);

            *symbol += sizeof(UINT32);
            break;

        case DEFAULT_LEASE:
            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            memcpy(&config->default_lease_length,*symbol, sizeof(UINT32));

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);

            *symbol += sizeof(UINT32);
            break;

        case DHCPS_RENEWAL_T1:
            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            memcpy(&config->dhcp_renew_t1, *symbol, sizeof(UINT32));

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);

            *symbol += sizeof(UINT32);
            break;

        case DHCPS_REBIND_T2:
            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            memcpy(&config->dhcp_rebind_t2, *symbol, sizeof(UINT32));

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);

            *symbol += sizeof(UINT32);
            break;  

        case DHCP_OFFERED_WAIT:
            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            memcpy(&config->offered_wait_time , *symbol, sizeof(UINT32));

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);

            *symbol += sizeof(UINT32);
            break; 

        case ARP_CACHE_TIMEOUT:
            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            memcpy(&config->arp_cache_to, *symbol, sizeof(UINT32));

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);

            *symbol += sizeof(UINT32);
            break;

        default:
            break;

    }

    return(NU_SUCCESS);
}

/************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPSDB_Process_Host_Short                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Read in a short-size variable from the configuration file
*     and place it into the appropriate config structure entry.
*
* INPUTS                                                               
*
*     option_name:        Option name of the parameter being processed.
*     **symbol:           Ptr to ptr to the buffer that is being processed.
*     *config:            Ptr to the configuration structure in which the 
*                               parameter value will be placed.
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     INT                                    
*                                                                      
************************************************************************/
INT DHCPSDB_Process_Host_Short (INT option_name, CHAR **symbol, DHCPS_CONFIG_CB *config)
{

    switch (option_name) 
    {

        case DHCP_IP_TIME_TO_LIVE :
            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            memcpy(&config->default_ip_ttl, *symbol, 1);

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);

            *symbol += 1;
            break;

        case DHCP_TCP_DEFAULT_TTL :
            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            memcpy(&config->default_tcp_ttl, *symbol, 1);

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);

            *symbol += 1;
            break;

        case STRUCTURE_STATUS_FLAGS :
            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            memcpy(&config->flags, *symbol, 2);

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);

            *symbol += 2;
            break;


        default:
            break;
    }
    return(NU_SUCCESS);
}

/************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPSDB_Process_String                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Read in a character string from a config file and place
*     it in the appropriate config structure entry.
*
* INPUTS                                                               
*
*     option_name:        Option name of the parameter being processed.
*     **symbol:           Ptr to ptr to the buffer that is being processed.
*     *config:            Ptr to the configuration structure in which the 
*                               parameter value will be placed.
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     INT                                    
*                                                                      
************************************************************************/
INT DHCPSDB_Process_String (INT option_name, CHAR **symbol, DHCPS_CONFIG_CB *config)
{
    CHAR    str_ptr[DHCPS_MAX_STRING_LEN];
    STATUS  status = -1;

    switch (option_name) 
    {
        case DNS_DOMAIN_NAME:
#if (CFG_NU_OS_NET_DHCP_SERVER_INCLUDE_DOMAIN_NAME == NU_TRUE)
            status = DHCPSDB_Get_String(symbol, str_ptr);

            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            config->dns_domain_name_length = (UINT8)(strlen(str_ptr));

            if((UINT8)(strlen(str_ptr)) < DHCPS_MAX_DOMAIN_NAME_LENGTH)
            {
                strcpy(config->dns_domain_name, str_ptr);
            }
            else
            {
                /* Domain name length is greater than maximum */
                status = -1;
            }

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);
#endif
            break;

        case NETWORK_INTERFACE_NAME:
            status = DHCPSDB_Get_String(symbol, str_ptr);

            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            if((UINT8)(strlen(str_ptr)) < DEV_NAME_LENGTH)
            {
                strcpy(config->device_name, str_ptr);
            }
            else
            {
                /* Device name length is greater than maximum */
                status = -1;
            }

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);
            break;
            
        default:
            break;
    }
    return(status);
}


/************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPSDB_Process_Range                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Read in a low IP address and and high IP address from a config file
*     and will configure binding entries for every IP address that falls
*     within those two IP addresses.
*
* INPUTS                                                               
*
*     option_name:        Option name of the parameter being processed.
*     **symbol:           Ptr to ptr to the buffer that is being processed.
*     *config:            Ptr to the configuration structure in which the 
*                               parameter value will be placed.
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     INT                                    
*                                                                      
************************************************************************/
INT DHCPSDB_Process_Range(INT option_name, CHAR **symbol, DHCPS_CONFIG_CB *config)
{
    struct id_struct            low, 
                                high; 
    CHAR                        test_char;
    UINT8                       len = IP_ADDR_LEN;
    INT                         ret_status = NU_SUCCESS;

    /* Remove compiler warning */
    UNUSED_PARAMETER (option_name);

    /* Move the low ip into the structure */
    memcpy(low.is_ip_addrs, *symbol, 4);

    *symbol += len;

    /* Move past any white space between the low and high addresses */
    DHCPSDB_Eat_Whitespace(symbol);

    /* Is there only one address? */
    memcpy(&test_char, *symbol, 1);

    if (test_char != COLON)      /* Equal to a colon */
    {
        /* Copy the upper IP address in the range into a local struct. */
        memcpy(high.is_ip_addrs, *symbol, 4);

        *symbol += len;
    }   

    /* Only one address */
    else
        high = low;

    /* Call the function to add the IP range to the binding link list. */
    ret_status = DHCPS_Add_IP_Range(config, NU_NULL, &low, &high);

    return(ret_status);
}   

/************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPSDB_Eat_Whitespace                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Increment the passed in pointer past any blank spaces.
*
* INPUTS                                                               
*                                                                      
*     **s:           Ptr to ptr to a buffer location that is being read.                                      
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     None.                                    
*                                                                      
************************************************************************/
VOID DHCPSDB_Eat_Whitespace(CHAR **s)
{
    CHAR *t;

    t = *s;

    /* Move the pointer past any whitespace characters. */
    while (*t && DHCPSDB_Isspace(*t))
    {
        t++;
    }

    *s = t;
}


/************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPSDB_Get_String                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Read in a character string, null terminate it, and return a ptr
*     to the beginning of the string.
*
* INPUTS                                                               
*                                                                      
*     **src:           Ptr to ptr to buffer that contains the char string.                                      
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     CHAR *                                    
*                                                                      
************************************************************************/
STATUS DHCPSDB_Get_String(CHAR **src, CHAR *str)
{
    INT     n, 
            len, 
            quoteflag;
    CHAR    tmp[DHCPS_MAX_STRING_LEN], 
            *tmpp;

    tmpp = tmp;
    quoteflag = NU_FALSE;
    n = 0;
    len = DHCPS_MAX_STRING_LEN - 1;

    /* Loop until we reach the max string length or encounter a NULL */
    while ((n < len) && (**src))        
    {
        /* If this is the end of the entry, break. */
        if (!quoteflag && (**src == ':')) 
        {
            break;
        }

        if (**src == '"') 
        {
            (*src)++;
            quoteflag = !quoteflag;
            continue;
        }

        if (**src == '\\') 
        {
            (*src)++;

            if (! **src) 
            {
                break;
            }
        }
        *tmpp++ = *(*src)++;
        n++;
    }
  
    *tmpp = '\0';

    strcpy(str, tmp);
    return(NU_SUCCESS);
}

/************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPSDB_Calculate_Subnet_Number                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Uses an IP address and a subnet mask to determine the network subnet
*     address.
*
* INPUTS                                                               
*                                                                      
*     *addr:        Ptr to the address structure that contains the IP address used
*                      to calculate the subnet.
*     *mask:        Ptr to the subnet mask used to calculate the subnet.
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     struct id_sturct: Subnet address.                                    
*                                                                      
************************************************************************/
STATUS DHCPSDB_Calculate_Subnet_Number (const struct id_struct *addr, 
                                        const struct id_struct *mask, 
                                        struct id_struct *subnet)
{
    INT i;
    UINT8 length;

    length = sizeof(addr->is_ip_addrs);

    /* Calculate the subnet address by AND'ing the IP address with the 
        subnet mask */
    for (i = 0; i < length; i++)
        subnet->is_ip_addrs[i] = addr->is_ip_addrs[i] & mask->is_ip_addrs[i];

    return NU_SUCCESS;
}


/************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPSDB_IP_Compare                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Compare two IP addresses to see if they are identical.
*
* INPUTS                                                               
*                                                                      
*     *ip1:     Ptr to the first IP address to be compared.
*     *ip2:     Ptr to the second IP address to be compared.
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     STATUS                                    
*                                                                      
************************************************************************/
STATUS DHCPSDB_IP_Compare(const UINT8 *ip1, const UINT8 *ip2)
{
    STATUS  ret_status = -1;

    if (ip1[0] == ip2[0])
    {
        if(ip1[1] == ip2[1])
        {
            if(ip1[2] == ip2[2])
            {
                if(ip1[3] == ip2[3])
                {
                    ret_status = NU_SUCCESS;
                }    
            }    
        }    
    }    
    return (ret_status);
}

/************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPSDB_Adjust_Buffer                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Adjust the buffer pointer until an entry delimiter is encountered.
*
* INPUTS                                                               
*                                                                      
*     **buffer:     Ptr to ptr to the buffer that needs to be adjusted.                                      
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     None.                                    
*                                                                      
************************************************************************/
VOID DHCPSDB_Adjust_Buffer(CHAR **buffer)
{
    register char *t;

    t = *buffer;

    /* Increment the pointer until the colon delimiter is found */
    while (*t && (*t != ':')) 
    {
        t++;
    }
    if (*t) 
    {
        t++;
    }
    *buffer = t;
}


/************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPSDB_Supported_Options_List                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     List of all supported DHCP options.  Each option has an option tag
*     and handling function that will be placed into the OPTIONMAP 
*     structure.
*
* INPUTS                                                               
*                                                                      
*     *opcode:          Ptr to the option code that needs to be found.
*     *optionmap_ptr:   Ptr to the option map structure that will hold the
*                        information of the handling function.
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     STATUS                                    
*                                                                      
************************************************************************/
STATUS DHCPSDB_Supported_Options_List(const CHAR *opcode, struct OPTIONMAP *optionmap_ptr)  
{
    STATUS  ret_status = -1;

    /* Append */
    if (strcmp(opcode, "apnd") == 0)
    {
        optionmap_ptr->opcode = "apnd";
        optionmap_ptr->option_name = APPEND_ENTRY;
        optionmap_ptr->func = DHCPSDB_Process_Append;
        ret_status = NU_SUCCESS;     
    }

    /* Subnet mask */
    if (strcmp(opcode, "snmk") == 0)
    {
        optionmap_ptr->opcode = "snmk";
        optionmap_ptr->option_name = SUBNET_MASK;
        optionmap_ptr->func = DHCPSDB_Process_IP;
        ret_status = NU_SUCCESS;     
    }

    /* Host name */
    if (strcmp(opcode, "hstn") == 0)
    {
        optionmap_ptr->opcode = "hstn";
        optionmap_ptr->option_name = HOST_NAME;
        optionmap_ptr->func = DHCPSDB_Process_String;
        ret_status = NU_SUCCESS;     
    }

    /* DNS domain name */
    if (strcmp(opcode, "dnsd") == 0)
    {
        optionmap_ptr->opcode = "dnsd";
        optionmap_ptr->option_name = DNS_DOMAIN_NAME;
        optionmap_ptr->func = DHCPSDB_Process_String;
        ret_status = NU_SUCCESS;     
    }

    /* DNS server address */
    if (strcmp(opcode, "dnsv") == 0)
    {
        optionmap_ptr->opcode = "dnsv";
        optionmap_ptr->option_name = DNS_SERVER_ADDR;
        optionmap_ptr->func = DHCPSDB_Process_IP;
        ret_status = NU_SUCCESS;     
    }   

    /* Broadcast address */
    if (strcmp(opcode, "brda") == 0)
    {
        optionmap_ptr->opcode = "brda";
        optionmap_ptr->option_name = BRDCAST_ADDR;
        optionmap_ptr->func = DHCPSDB_Process_IP;
        ret_status = NU_SUCCESS;     
    }

    /* Router address */
    if (strcmp(opcode, "rout") == 0)
    {
        optionmap_ptr->opcode = "rout";
        optionmap_ptr->option_name = ROUTER;
        optionmap_ptr->func = DHCPSDB_Process_IP;
        ret_status = NU_SUCCESS;     
    }   


    /* Renew-t1 time */
    if (strcmp(opcode, "dht1") == 0)
    {
        optionmap_ptr->opcode = "dht1";
        optionmap_ptr->option_name = DHCPS_RENEWAL_T1;
        optionmap_ptr->func = DHCPSDB_Process_Host_Long;
        ret_status = NU_SUCCESS;     
    }

    /* Rebind-t2 time */
    if (strcmp(opcode, "dht2") == 0)
    {
        optionmap_ptr->opcode = "dht2";
        optionmap_ptr->option_name = DHCPS_REBIND_T2;
        optionmap_ptr->func = DHCPSDB_Process_Host_Long;
        ret_status = NU_SUCCESS;     
    }

    /* Offered wait time */
    if (strcmp(opcode, "dofw") == 0)
    {
        optionmap_ptr->opcode = "dofw";
        optionmap_ptr->option_name = DHCP_OFFERED_WAIT;
        optionmap_ptr->func = DHCPSDB_Process_Host_Long;
        ret_status = NU_SUCCESS;     
    }

    /* Configuration Control Block Entry Name */
    if (strcmp(opcode, "cfgn") == 0)
    {
        optionmap_ptr->opcode = "cfgn";
        optionmap_ptr->option_name = CONFIG_CONTROL_BLOCK_NAME;
        optionmap_ptr->func = DHCPSDB_Process_String;
        ret_status = NU_SUCCESS;     
    }

    /* IP address */
    if (strcmp(opcode, "ipad") == 0)
    {
        optionmap_ptr->opcode = "ipad";
        optionmap_ptr->option_name = IP_ADDRESS;
        optionmap_ptr->func = DHCPSDB_Process_IP;
        ret_status = NU_SUCCESS;     
    }

    /* Max lease length */
    if (strcmp(opcode, "maxl") == 0)
    {
        optionmap_ptr->opcode = "maxl";
        optionmap_ptr->option_name = MAX_LEASE;
        optionmap_ptr->func = DHCPSDB_Process_Host_Long;
        ret_status = NU_SUCCESS;     
    }

    /* Client ID (used in old API for static binding) */
    if (strcmp(opcode, "clid") == 0)
    {
        optionmap_ptr->opcode = "clid";
        optionmap_ptr->option_name = CLIENT_ID;
        optionmap_ptr->func = DHCPSDB_Process_Client_ID;
        ret_status = NU_SUCCESS;     
    }

    /* Default lease length */
    if (strcmp(opcode, "dfll") == 0)
    {
        optionmap_ptr->opcode = "dfll";
        optionmap_ptr->option_name = DEFAULT_LEASE;
        optionmap_ptr->func = DHCPSDB_Process_Host_Long;
        ret_status = NU_SUCCESS;     
    }

    /* IP range */
    if (strcmp(opcode, "rang") == 0)
    {
        optionmap_ptr->opcode = "rang";
        optionmap_ptr->option_name = IP_ADDRESS_RANGE;
        optionmap_ptr->func = DHCPSDB_Process_Range;
        ret_status = NU_SUCCESS;     
    }

    /* IP time-to-live */
    if (strcmp(opcode, "ittl") == 0)
    {
        optionmap_ptr->opcode = "ittl";
        optionmap_ptr->option_name = DHCP_IP_TIME_TO_LIVE;
        optionmap_ptr->func = DHCPSDB_Process_Host_Short;
        ret_status = NU_SUCCESS;     
    }

    /* TCP time-to-live */
    if (strcmp(opcode, "tttl") == 0)
    {
        optionmap_ptr->opcode = "tttl";
        optionmap_ptr->option_name = DHCP_TCP_DEFAULT_TTL;
        optionmap_ptr->func = DHCPSDB_Process_Host_Short;
        ret_status = NU_SUCCESS;
    }    

    /* Structure Status Flags */
    if (strcmp(opcode, "flgs") == 0)
    {
        optionmap_ptr->opcode = "flgs";
        optionmap_ptr->option_name = STRUCTURE_STATUS_FLAGS;
        optionmap_ptr->func = DHCPSDB_Process_Host_Short;
        ret_status = NU_SUCCESS;     
    }

    /* DHCP Server IP address */
    if (strcmp(opcode, "svip") == 0)
    {
        optionmap_ptr->opcode = "svip";
        optionmap_ptr->option_name = SIADDR;
        optionmap_ptr->func = DHCPSDB_Process_IP;
        ret_status = NU_SUCCESS;     
    }   

    /* Subnet Address */
    if (strcmp(opcode, "suba") == 0)
    {
        optionmap_ptr->opcode = "suba";
        optionmap_ptr->option_name = SUBNET_ADDRESS;
        optionmap_ptr->func = DHCPSDB_Process_IP;
        ret_status = NU_SUCCESS;     
    }   

    /* Network Interface Device Name */
    if (strcmp(opcode, "ifnm") == 0)
    {
        optionmap_ptr->opcode = "ifnm";
        optionmap_ptr->option_name = NETWORK_INTERFACE_NAME;
        optionmap_ptr->func = DHCPSDB_Process_String;
        ret_status = NU_SUCCESS;     
    }

    /* ARP Cache Timeout */
    if (strcmp(opcode, "arpt") == 0)
    {
        optionmap_ptr->opcode = "arpt";
        optionmap_ptr->option_name = ARP_CACHE_TIMEOUT;
        optionmap_ptr->func = DHCPSDB_Process_Host_Long;
        ret_status = NU_SUCCESS;     
    }

    return(ret_status);
}

/************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPSDB_Isspace                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Function will determine if the character that was passed in is a space
*     that should be ignored.
*
* INPUTS                                                               
*                                                                      
*     c:    Character to be analyzed.
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     INT:  return 1 if the character is a space.  Else, return 0                                    
*                                                                      
************************************************************************/
INT DHCPSDB_Isspace(register INT c)
{
    return c == 0x20 || (c >= 0x09 && c <= 0x0D); 
}

