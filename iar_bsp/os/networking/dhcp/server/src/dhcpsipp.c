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
*     dhcpsipp.c                                               
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     This file contains the API functions of the DHCP Server that deal
*     with the IP layer options.
*                                                                      
* FUNCTIONS                                                            
*
*     DHCPS_Set_IP_TTL
*     DHCPS_Get_IP_TTL 
*     DHCPS_Enable_IP_Forwarding
*     DHCPS_Disable_IP_Forwarding
*                                                                      
* DEPENDENCIES                                                         
*
*     networking/nu_networking.h
*     os/networking/dhcps/inc/dhcps_ext.h
*     os/networking/dhcps/inc/dhcpsrv.h
*
**************************************************************************/
/* Includes */
#include "networking/nu_networking.h"
#include "os/networking/dhcp/server/inc/dhcps_ext.h"
#include "os/networking/dhcp/server/inc/dhcpsrv.h"

/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Set_IP_TTL                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Set the IP time to live value for the specified configuration control block
*     or specific binding.
*
* INPUTS                                                               
*                                                                      
*     DHCPS_CONFIG_CB *
*     struct id_struct *
*     UINT8
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     STATUS                                    
*                                                                      
**********************************************************************************/
STATUS DHCPS_Set_IP_TTL (DHCPS_CONFIG_CB *config_cb, const struct id_struct *client_ip_addr, 
                                UINT8 ip_ttl)
{
    STATUS                  ret_status = NU_SUCCESS;
    UINT8                   *options_block;
    DHCPS_OPTIONS           *option_buffer;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */    
    NU_SUPERVISOR_MODE();

    /* First, we must determine if this IP TTL entry is for a particular IP address (static IP).  
        This can be determined if a client IP address was passed in.  If so, then the IP TTL 
        needs to be added to an IP option block. */
    if (client_ip_addr != NU_NULL)
    {
        /* Perform any memory block manipulation that may need to be done to make room for
            the default IP TTL entry that is being added. */
        options_block = DHCPS_Add_Option_To_Memory_Block(config_cb, client_ip_addr, DEFAULT_IP_TTL, 
                                                            &option_buffer);    

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
                /* Store the option into the memory block in the format that it will be
                   provided to the client. */

                /* Option Tag */
                *options_block++ = DEFAULT_IP_TTL;

                /* Data Length */
                *options_block++ = sizeof(UINT8);

                /* Default IP TTL */
                memcpy(options_block, &ip_ttl, sizeof(UINT8));

                /* Increment the options block ptr */
                options_block++;

                /* Copy the end option tag into the buffer. */
                *options_block = DHCP_END;

                /* Increment the bytes written. */
                option_buffer->bytes_written += (sizeof(UINT8) + 2);

            }    

            else
            {
                /* We must overwrite the data already present. */

                /* Increment to the location where the data will be written. */
                options_block += 2;

                /* Default IP TTL */
                memcpy(options_block, &ip_ttl, sizeof(UINT8));

                /* Increment the options block ptr */
                options_block++;

                /* Add the end option tag. */
                *options_block = DHCP_END;
            }

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);
        }

        else
            /* An invalid parameter was passed into the function.  The entry can not be added. */
            ret_status = NU_INVALID_PARM;
    }       

    else
    {
        /* This entry is meant for the configuration control block. */

        /* Protect the global data structures. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

        /* Copy the lease renewal time into the control block. */
        config_cb->default_ip_ttl = ip_ttl;

        /* Set the return status as successful */
        ret_status = NU_SUCCESS;

        /* Unprotect the global data structures. */
        NU_Release_Semaphore(&DHCPS_Semaphore);
    }
    /* Switch back to user mode */
    NU_USER_MODE();

    return (ret_status);
} /* DHCPS_Set_Default_IP_TTL */


/****************************************************************************
* FUNCTION                                                                   
*                                                                            
*    DHCPS_Get_IP_TTL                                                       
*                                                                            
* DESCRIPTION                                                                
*                                                                            
*    Return the value of the IP time to live from the specified configuraiton
*    control block or specific binding in the provided buffer.  The returned 
*    value is the number of bytes that were written into the buffer.
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
INT DHCPS_Get_IP_TTL (const DHCPS_CONFIG_CB *config_cb, const struct id_struct *client_ip_addr, 
                              UINT8 *ip_ttl_buffer, INT buffer_size)
{
    UINT8                       *option_ptr,
                                option_data_len;
    INT                         bytes_written;
                                
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* First, ensure that the buffer that has been passed in is large enough to accept at least one
        entry. */
    if ((buffer_size < sizeof(UINT8)) || (ip_ttl_buffer == NU_NULL))
    {
        bytes_written = NU_INVALID_PARM;
    }

    /* Check to see if the IP TTL entry is destined for a static IP entry. */
    else if (client_ip_addr != NU_NULL)
    {
        /* Search through the static IP entries to find the default IP TTL entry. */
        option_ptr = DHCPS_Get_Option_From_Memory_Block(config_cb, client_ip_addr, DEFAULT_IP_TTL);

        if (option_ptr == NU_NULL)
        {
            /* An error occurred during the search for the IP TTL for the particular IP address. */
            bytes_written = NU_INVALID_PARM;
        }    

        else
        {
            /* The default IP TTL option has been found for the desired IP address. */

            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);
            
            /* Increment the option pointer to point at the length of the option. */
            option_data_len = *(option_ptr + 1);

            /* Ensure that the buffer is large enough for the option data. */
            if (buffer_size < option_data_len)
            {
                bytes_written = DHCPSERR_BUFFER_TOO_SMALL;
            }    

            else
            {
                /* Increment the option pointer to point at the data that will be copied. */
                option_ptr += 2;

                /* Copy the option into the buffer that has been provided by the application. */
                memcpy(ip_ttl_buffer, option_ptr, option_data_len);

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

        /* Ensure that the buffer is large enough for the option data. */
        if (buffer_size < sizeof(UINT8))
        {
            bytes_written = DHCPSERR_BUFFER_TOO_SMALL;
        }    

        else
        {   
            /* Copy the option entry into the buffer provided by the application. */
            memcpy(ip_ttl_buffer, &config_cb->default_ip_ttl, sizeof(UINT8));

            /* Set the return variable to the number of bytes that were written into the buffer. */
            bytes_written = sizeof(UINT8);  
        }
        /* Unprotect the global data structures. */
        NU_Release_Semaphore(&DHCPS_Semaphore);
    }
    /* Switch back to user mode */
    NU_USER_MODE();

    return (bytes_written); 

} /* DHCPS_Get_IP_TTL */


/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Enable_IP_Forwarding                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Set the bit in the specified configuration control block status flags to
*     alert the requesting clients to enable IP forwarding.
*
* INPUTS                                                               
*                                                                      
*     DHCPS_CONFIG_CB *
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     STATUS
*                                                                      
**********************************************************************************/
STATUS DHCPS_Enable_IP_Forwarding (DHCPS_CONFIG_CB *config_cb)
{
    STATUS                  ret_status = NU_SUCCESS;
    UINT16                  flag_stat;
                            
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Ensure that we have been passed a valid control block pointer. */
    if (config_cb != NU_NULL)
    {
        /* Instructs the requesting client to forward IP datagrams between its interfaces. */

        /* Protect the global data structures. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

        /* Get the current status of the flags variable. */
        flag_stat = config_cb->flags;

        /* Set the IP forwarding bit of the flags variable. */
        flag_stat = flag_stat | IP_FORWARDING_ENABLED;

        /* Save the new flag settings to the configuration control block. */
        config_cb->flags = flag_stat;

        /* IP forwarding has been successfully added.. */  
        ret_status = NU_SUCCESS;

        /* Unprotect the global data structures. */
        NU_Release_Semaphore(&DHCPS_Semaphore);
    }

    else
    {
        /* An invalid control block pointer was passed in. */
        ret_status = NU_INVALID_PARM;
    }    
    /* Switch back to user mode */
    NU_USER_MODE();

    return(ret_status);
}   /* DHCPS_Enable_IP_Forwarding */


/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Disable_IP_Forwarding                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Reset the bit of the specified configuration control block so that requesting
*     clients will not be informed to use IP forwarding.
*
* INPUTS                                                               
*                                                                      
*     DHCPS_CONFIG_CB *
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     STATUS
*                                                                      
**********************************************************************************/
STATUS DHCPS_Disable_IP_Forwarding (DHCPS_CONFIG_CB *config_cb)
{
    STATUS                  ret_status = NU_SUCCESS;
    UINT16                  flag_stat;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Ensure that we have been passed a valid control block pointer. */
    if (config_cb != NU_NULL)
    {
        /* If this option is disabled, the requesting client will not forward IP datagrams
            between its interfaces. */

        /* Protect the global data structures. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

        /* Get the current settings of the control block flags. */
        flag_stat = config_cb->flags;

        /* Clear the IP forwarding bit */
        flag_stat = flag_stat & ~IP_FORWARDING_ENABLED;

        /* Save the new flag settings to the configuration control block. */
        config_cb->flags = flag_stat;

        /* IP forwarding has been disabled. */  
        ret_status = NU_SUCCESS;

        /* Unprotect the global data structures. */
        NU_Release_Semaphore(&DHCPS_Semaphore);
    }

    else
    {
        /* An invalid control block pointer was passed in. */
        ret_status = NU_INVALID_PARM;
    }    
    /* Switch back to user mode */
    NU_USER_MODE();

    return(ret_status);
}   /* DHCPS_Disable_IP_Forwarding */

