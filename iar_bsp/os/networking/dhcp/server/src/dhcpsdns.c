/**************************************************************************
*
*               Copyright Mentor Graphics Corporation 2003              
*                         All Rights Reserved.                          
*                                                                       
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   
* SUBJECT TO LICENSE TERMS.                                             
*
***************************************************************************

***************************************************************************
* FILE NAME                                                           
*                                                                      
*       dhcpsdns.c                                               
*
* DESCRIPTION                                                          
*                                                                      
*       This file contains each of the API functions of the DHCP server
*       that deal with the DNS server 
*
* DATA STRUCTURES
*
*       None
*                                                                      
* FUNCTIONS                                                            
*
*       DHCPS_Add_DNS_Server
*       DHCPS_Delete_DNS_Server      
*       DHCPS_Get_DNS_Servers
*                                                                      
* DEPENDENCIES
*
*       nucleus_gen_cfg.h
*       networking/nu_networking.h
*     os/networking/dhcp/server/inc/dhcps_ext.h
*     os/networking/dhcp/server/inc/dhcpsrv.h
*                                                                      
***************************************************************************/
/* Includes */
#include "nucleus_gen_cfg.h"
#include "networking/nu_networking.h"
#include "os/networking/dhcp/server/inc/dhcps_ext.h"
#include "os/networking/dhcp/server/inc/dhcpsrv.h"

#if (CFG_NU_OS_NET_DHCP_SERVER_INCLUDE_DNS_SERVER == NU_TRUE)
/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Add_DNS_Server                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     This function will add a DNS server IP address to the specified configuration
*     control block or specific binding.
*
* INPUTS                                                               
*                                                                      
*     DHCPS_CONFIG_CB *
*     struct id_struct *
*     struct id_struct *
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     STATUS
*                                                                      
**********************************************************************************/
STATUS DHCPS_Add_DNS_Server (DHCPS_CONFIG_CB *config_cb, const struct id_struct *client_ip_addr, 
                             struct id_struct *dns_ip_address)
{
    STATUS                  ret_status = NU_SUCCESS;

    UINT8                   *options_block,
                            i,
                            option_data_length;
    DHCPS_OPTIONS           *option_buffer;
    UINT32                  dns_serv_ip;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    dns_serv_ip = IP_ADDR(dns_ip_address->is_ip_addrs);

    /* If this is not a class A, B, or C address, or if it is 0 then it is not 
       a valid IP address. */
    if (   (!IP_CLASSA_ADDR(dns_serv_ip) 
         && !IP_CLASSB_ADDR(dns_serv_ip) 
         && !IP_CLASSC_ADDR(dns_serv_ip))
         || dns_serv_ip == 0)
    {
        ret_status = NU_INVALID_PARM;
    }

    /* First, we must determine if this DNS server entry is for a particular IP address (static IP).  
        This can be determined if a client IP address was passed in.  If so, then the DNS server 
        needs to be added to an IP option block. */
    if (client_ip_addr != NU_NULL)
    {
        /* Perform any memory block manipulation that may need to be done to make room for
            the server IP address that is being added. */
        options_block = DHCPS_Add_Option_To_Memory_Block(config_cb, client_ip_addr, DHCP_DNS, &option_buffer);    

        /* Test to ensure that the option block has been prepared to receive the new option. */
        if (options_block != NU_NULL)
        {
            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* The option can now be added to the options block. First, we must ensure that 
                the option has been previously added.  If not, we can just add the option tag and
                data length now. */
            if (*options_block == DHCP_END)
            {
                /* Store the server into the memory block in the format that it will be
                   provided to the client. */

                /* Option Tag */
                *options_block++ = DHCP_DNS;

                /* Data Length */
                *options_block++ = IP_ADDR_LEN;

                /* Server IP address */
                for (i = 0; i < IP_ADDR_LEN; i++)
                    *options_block++ = dns_ip_address->is_ip_addrs[i];

                /* Copy the end option tag into the buffer. */
                *options_block = DHCP_END;

                /* Increment the bytes written. */
                option_buffer->bytes_written += (IP_ADDR_LEN + 2);

            }    

            else
            {
                /* We must move to the end of the option and append our option entry to the 
                    end and adjust the data length.  */

                /* Increment to the option data length */
                options_block++;

                /* Save the current data length */
                option_data_length = *options_block;

                /* Adjust the data length */
                *options_block += IP_ADDR_LEN;

                /* Adjust the options block pointer to the end of the option data. */
                options_block += option_data_length + 1;

                /* Add the server IP address */
                for (i = 0; i < IP_ADDR_LEN; i++)
                    *options_block++ = dns_ip_address->is_ip_addrs[i];

                /* Add the end option tag. */
                *options_block = DHCP_END;

                /* Increment the bytes written. */
                option_buffer->bytes_written += IP_ADDR_LEN;
            }

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);
        }

        else
            /* An invalid parameter was passed into the function.  The DNS server can not be added. */
            ret_status = NU_INVALID_PARM;
    }       

    else
    {
        /* Protect the global data structures. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

        /* Add the router address to the array. */
        ret_status = DHCPS_Add_Entry_To_Array(&config_cb->dns_server[0], 
                                            dns_ip_address, DHCPS_MAX_DNS_SERVERS);

        /* Unprotect the global data structures. */
        NU_Release_Semaphore(&DHCPS_Semaphore);
    }
    /* Switch back to user mode */
    NU_USER_MODE();

    /* Return */
    return (ret_status);
} /* DHCPS_Add_DNS_Server */

/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Delete_DNS_Server                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Removes the stated DNS server IP address from the specified configuration 
*     control block or the specific binding.
*
* INPUTS                                                               
*                                                                      
*     DHCPS_CONFIG_CB *
*     struct id_struct *
*     struct id_struct *
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     STATUS
*                                                                      
**********************************************************************************/
STATUS DHCPS_Delete_DNS_Server(DHCPS_CONFIG_CB *config_cb, const struct id_struct *client_ip_addr, 
                                  const struct id_struct *dns_ip_addr)
{
    STATUS                      ret_status;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* First, we must determine if the router entry is in a particular IP address option block.  
        This can be determined if a client IP address was passed in. */
    if (client_ip_addr != NU_NULL)
    {
        /* The DNS server must be removed from the option memory block and the remaining contents
            of the block adjusted to compensate for the removal of an entry. */
        ret_status = DHCPS_Remove_Option_From_Memory_Block(config_cb, client_ip_addr, DHCP_DNS, dns_ip_addr);
    }

    else
    {
        /* Protect the global data structures. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

        /* Remove the server IP address from the array. */
        ret_status = DHCPS_Remove_Entry_From_Array(&config_cb->dns_server[0], 
                                            dns_ip_addr, DHCPS_MAX_DNS_SERVERS);        

        /* Unprotect the global data structures. */
        NU_Release_Semaphore(&DHCPS_Semaphore);
    }
    /* Switch back to user mode */
    NU_USER_MODE();

    return ret_status;

} /* DHCPS_Delete_DNS_Server */

/****************************************************************************
* FUNCTION                                                                   
*                                                                            
*   DHCPS_Get_DNS_Servers                                                       
*                                                                            
* DESCRIPTION                                                                
*                                                                            
*    Returns a list of each of the DNS server IP addresses that are in the
*    specified configuration control block or specific binding. The returned
*    variable will be the total number of bytes that were written into the 
*    provided buffer. 
*
* INPUTS                                                                     
*                                                                            
*    DHCPS_CONFIG_CB *
*    struct id_struct *
*    UINT8 *
*    INT
*                                                                            
* OUTPUTS                                                                    
*                                                                            
*    INT
*                                                                            
******************************************************************************/
INT DHCPS_Get_DNS_Servers(const DHCPS_CONFIG_CB *config_cb, const struct id_struct *client_ip_addr, 
                            UINT8 *dns_buffer, INT buffer_size)
{
    UINT8                       *option_ptr,
                                option_data_len,
                                entries_written_to_buff = 0;
    INT                         i,
                                bytes_written;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* First, ensure that the passed in parameters are valid. */
    if ((config_cb == NU_NULL) || (dns_buffer == NU_NULL))
    {
        bytes_written = NU_INVALID_PARM;
    }

    /* Check to see if the server entry is destined for a static IP entry. */
    else if (client_ip_addr != NU_NULL)
    {
        /* Search through the static IP entries to find the DNS server entries. */
        option_ptr = DHCPS_Get_Option_From_Memory_Block(config_cb, client_ip_addr, DHCP_DNS);

        if (option_ptr == NU_NULL)
        {
            /* An error occurred during the search for the router for the particular IP address. */
            bytes_written = NU_INVALID_PARM;
        }    

        else
        {
            /* The DNS server options have been found for the desired IP address. */

            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* Increment the option pointer to point at the length of the server option. */
            option_data_len = *(option_ptr + 1);

            /* Ensure that the buffer is large enough to hold the option data. */
            if (buffer_size < option_data_len)
            {
                bytes_written = DHCPSERR_BUFFER_TOO_SMALL;
            }    

            else
            {
                /* Increment the option pointer to point at the data that will be copied. */
                option_ptr += 2;

                /* Copy the server options into the buffer that has been provided by the application. */
                memcpy(dns_buffer, option_ptr, option_data_len);

                /* Save the number of bytes that have been written into the buffer for returning to
                    the application. */
                bytes_written = option_data_len;
            }    
            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);
        }    
    }    

    else 
    {
        /* Protect the global data structures. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);
        
        /* The desired option might be inside the configuration control block.  Increment through the 
            server address array and copy then into provided buffer. */
        for (i = 0; i < DHCPS_MAX_DNS_SERVERS; i++)
        {
            if (buffer_size < IP_ADDR_LEN)
            {         
                bytes_written = DHCPSERR_BUFFER_TOO_SMALL;
                break;
            }

            else if (config_cb->dns_server[i].is_ip_addrs[0] == 0)
            {
                /* This is an empty entry.  Skip this one and go to the next entry. */
                continue;
            }    

            else
            {
                /* Copy the DNS server entry into the buffer. */
                memcpy(dns_buffer, config_cb->dns_server[i].is_ip_addrs, IP_ADDR_LEN);

                /* Increment a counting variable. */
                entries_written_to_buff++;

                /* Increment the buffer pointer */
                dns_buffer += IP_ADDR_LEN;
            }    
        }

        /* Unprotect the global data structures. */
        NU_Release_Semaphore(&DHCPS_Semaphore);

        /* Set the return variable to the number of bytes that were written into the buffer. */
        bytes_written = (entries_written_to_buff * IP_ADDR_LEN);      
    }
    /* Switch back to user mode */
    NU_USER_MODE();

    return (bytes_written); 

} /* DHCPS_Get_DNS_Servers */

#endif

