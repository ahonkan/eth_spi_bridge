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
*     dhcpsdom.c                                               
*                                                                      
* COMPONENT                                                            
*                                                                      
*     Nucleus DHCP Server                                   
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     This file contains the API functions of the DHCP Server that deal
*     with the domain name option.
*
* DATA STRUCTURES
*
*     None
*                                                                      
* FUNCTIONS                                                            
*
*     DHCPS_Add_Domain_Name                                        
*     DHCPS_Delete_Domain_Name                                        
*     DHCPS_Get_Domain_Name                                        
*                                                                      
* DEPENDENCIES
*
*     networking/nu_networking.h
*     os/networking/dhcp/server/inc/dhcps_ext.h
*     os/networking/dhcp/server/inc/dhcpsrv.h
*                                                                      
**************************************************************************/
/* Includes */
#include "nucleus_gen_cfg.h"
#include "networking/nu_networking.h"
#include "os/networking/dhcp/server/inc/dhcps_ext.h"
#include "os/networking/dhcp/server/inc/dhcpsrv.h"

#if (CFG_NU_OS_NET_DHCP_SERVER_INCLUDE_DOMAIN_NAME == NU_TRUE)
/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Add_Domain_Name                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Add the stated domain name to the specified configuration control block or
*     the specific binding.
*
* INPUTS                                                               
*                                                                      
*     DHCPS_CONFIG_CB *
*     struct id_struct *
*     CHAR *
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     STATUS
*                                                                      
**********************************************************************************/
STATUS DHCPS_Add_Domain_Name (DHCPS_CONFIG_CB *config_cb, const struct id_struct *client_ip_addr, 
                              const CHAR *domain_name)
{
    STATUS                  ret_status = NU_SUCCESS;

    UINT8                   *options_block,
                            option_data_length;
    DHCPS_OPTIONS           *option_buffer;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Ensure that the parameters past in are valid. */
    if ((config_cb != NU_NULL) && (domain_name != NU_NULL))
    {
        /* First, we must determine if this domain name entry is for a particular IP address (static IP).  
            This can be determined if a client IP address was passed in.  If so, then the domain name 
            needs to be added to an IP option block. */
        if (client_ip_addr != NU_NULL)
        {
            /* Perform any memory block manipulation that may need to be done to make room for
                the domain name that is being added. */
            options_block = DHCPS_Add_Option_To_Memory_Block(config_cb, client_ip_addr, DNS_DOMAIN_NAME, 
                                                                &option_buffer);    

            /* Test to ensure that the option block has been prepared to receive the new option. */
            if (options_block != NU_NULL)
            {
                /* Protect the global data structures. */
                NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

                /* The domain name can now be added to the options block. First, we must ensure that 
                    the option has not been previously added.  If not, we can just add the option tag and
                    data length now. */
                if (*options_block == DHCP_END)
                {
                    /* Store the domain name into the memory block in the format that it will be
                       provided to the client. */

                    /* Option Tag */
                    *options_block++ = DNS_DOMAIN_NAME;

                    /* Data Length */
                    *options_block++ = (UINT8)strlen(domain_name);
                    option_data_length = (UINT8)strlen(domain_name);

                    /* Domain name */
                    memcpy(options_block, domain_name, option_data_length);

                    /* Increment the options block ptr */
                    options_block += option_data_length;

                    /* Copy the end option tag into the buffer. */
                    *options_block = DHCP_END;

                    /* Increment the bytes written. */
                    option_buffer->bytes_written += (option_data_length + 2);

                }    

                else
                {
                    /* There is already a domain name option in place for this binding. */
                    ret_status = DHCPSERR_OPTION_ALREADY_PRESENT;
                }

                /* Unprotect the global data structures. */
                NU_Release_Semaphore(&DHCPS_Semaphore);
            }

            else
                /* An invalid parameter was passed into the function.  The domain name can not be added. */
                ret_status = NU_INVALID_PARM;
        }       

        else
        {
            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* Add the length of the domain name to the domain name structure. */
            config_cb->dns_domain_name_length = (UINT8)strlen(domain_name);

            /* Copy the domain name into the domain name structure of the control block. */
            memcpy(config_cb->dns_domain_name, domain_name, config_cb->dns_domain_name_length);

            /* Set the return status as successful */
            ret_status = NU_SUCCESS;

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);
        }
    }

    else
    {
        /* An invalid parameter was passed into the function.  The domain name can not be added. */
        ret_status = NU_INVALID_PARM;
    }
    /* Switch back to user mode */
    NU_USER_MODE();

    return (ret_status);
} /* DHCPS_Add_Domain_Name */


/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Delete_Domain_Name                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Remove the stated domain name from the specified configuration control block 
*     or specific binding.
*
* INPUTS                                                               
*                                                                      
*     DHCPS_CONFIG_CB *
*     struct id_struct *
*     CHAR *
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     STATUS
*                                                                      
**********************************************************************************/
STATUS DHCPS_Delete_Domain_Name (DHCPS_CONFIG_CB *config_cb, const struct id_struct *client_ip_addr, 
                                   const CHAR *domain_name)
{
    STATUS  ret_status = NU_SUCCESS;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* First, we must determine if the domain name entry is in a particular IP address option block.  
        This can be determined if a client IP address was passed in. */
    if (client_ip_addr != NU_NULL)
    {
        /* The domain name must be removed from the option memory block and the remaining contents
            of the block adjusted to compensate for the removal of an entry. */
        ret_status = DHCPS_Remove_Option_From_Memory_Block(config_cb, client_ip_addr, DNS_DOMAIN_NAME, domain_name);
    }

    else
    {
        /* Ensure that we have a match between the control block domain name and the domain name that
            was passed in from the application. */
        ret_status = strcmp(config_cb->dns_domain_name, domain_name);

        if (ret_status == NU_SUCCESS)
        {
            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* Since the domain name entry to be removed is not a IP specific option, the option might
                be a configuration-wide entry.  Therefore, we will clear the domain name structure. */
            UTL_Zero(config_cb->dns_domain_name, DHCPS_MAX_DOMAIN_NAME_LENGTH);
            config_cb->dns_domain_name_length = 0;

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);
        }
        else
        {
            ret_status = DHCPSERR_PARAMETER_NOT_FOUND;
        }    
    }
    /* Switch back to user mode */
    NU_USER_MODE();

    return (ret_status);

} /* DHCPS_Delete_Domain_Name */

/****************************************************************************
* FUNCTION                                                                   
*                                                                            
*   DHCPS_Get_Domain_Name                                                       
*                                                                            
* DESCRIPTION                                                                
*                                                                            
*    Return the domain name of the specified configuration control block or
*    specific binding.  The return variable is the total number of bytes
*    that were written into the provided buffer.
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
INT DHCPS_Get_Domain_Name (const DHCPS_CONFIG_CB *config_cb, const struct id_struct *client_ip_addr, 
                           UINT8 *domain_buffer, INT buffer_size)
{
    UINT8                       *option_ptr,
                                option_data_len;

    INT                         bytes_written;
                             
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* First, ensure that the passed in parameters are valid. */
    if ((config_cb == NU_NULL) || (domain_buffer == NU_NULL))
    {
        bytes_written = NU_INVALID_PARM;
    }

    /* Check to see if the server entry is destined for a static IP entry. */
    else if (client_ip_addr != NU_NULL)
    {
        /* Search through the static IP entries to find the domain name entry. */
        option_ptr = DHCPS_Get_Option_From_Memory_Block(config_cb, client_ip_addr, DNS_DOMAIN_NAME);

        if (option_ptr == NU_NULL)
        {
            /* An error occurred during the search for the domain name for the particular IP address. */
            bytes_written = NU_INVALID_PARM;
        }    

        else
        {
            /* The domain name option has been found for the desired IP address. */

            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* Increment the option pointer to point at the length of the domain name option. */
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

                /* Copy the domain name option into the buffer that has been provided by the application. */
                memcpy(domain_buffer, option_ptr, option_data_len);

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

        /* Copy the domain name length. */
        option_data_len = config_cb->dns_domain_name_length;

        /* Ensure that the buffer is large enough to hold the option data. */
        if (buffer_size < option_data_len)
        {
            bytes_written = DHCPSERR_BUFFER_TOO_SMALL;
        }    

        else
        {
            /* Copy the domain name option entry into the buffer provided by the application. */
            memcpy(domain_buffer, config_cb->dns_domain_name, option_data_len);

            /* Set the return variable to the number of bytes that were written into the buffer. */
            bytes_written = option_data_len;      
        }
        /* Unprotect the global data structures. */
        NU_Release_Semaphore(&DHCPS_Semaphore);
    }
    /* Switch back to user mode */
    NU_USER_MODE();

    return (bytes_written); 

} /* DHCPS_Get_Domain_Name */
 
#endif
