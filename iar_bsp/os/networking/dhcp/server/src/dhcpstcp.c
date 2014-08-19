/*************************************************************************
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
*     dhcpstcp.c                                              
*                                                                      
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     This file contains the API functions of the DHCP Server that deal
*     with the TCP layer options.
*
* DATA STRUCTURES
*
*     None
*
* FUNCTIONS                                                            
*
*     DHCPS_Set_TCP_TTL
*     DHCPS_Get_TCP_TTL
*     DHCPS_Set_TCP_Keepalive_Interval
*     DHCPS_Get_TCP_Keepalive_Interval
*     DHCPS_Enable_TCP_Keepalive_Garbage
*     DHCPS_Disable_TCP_Keepalive_Garbage
*
* DEPENDENCIES                                                         
*
*     networking/nu_networking.h
*     os/networking/dhcps/inc/dhcps_ext.h
*     os/networking/dhcps/inc/dhcpsrv.h
*                                                                      
***************************************************************************/

/* Includes */
#include "networking/nu_networking.h"
#include "os/networking/dhcp/server/inc/dhcps_ext.h"
#include "os/networking/dhcp/server/inc/dhcpsrv.h"

/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Set_TCP_TTL                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Set the TCP time to live value for the specified configuration control block
*      or specific binding.
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
STATUS DHCPS_Set_TCP_TTL (DHCPS_CONFIG_CB *config_cb, const struct id_struct *client_ip_addr, 
                                    UINT8 tcp_ttl)
{
    STATUS                  ret_status = NU_SUCCESS;
    UINT8                   *options_block;
    DHCPS_OPTIONS           *option_buffer;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Ensure that the configuration control block pointer is valid. */
    if (config_cb == NU_NULL)
    {
        ret_status = NU_INVALID_PARM;
    }    

    else
    {
        /* First, we must determine if this TCP TTL entry is for a particular IP address (static IP).  
            This can be determined if a client IP address was passed in.  If so, then the TCP TTL 
            needs to be added to an IP option block. */
        if (client_ip_addr != NU_NULL)
        {
            /* Perform any memory block manipulation that may need to be done to make room for
                the TCP TTL entry that is being added. */
            options_block = DHCPS_Add_Option_To_Memory_Block(config_cb, client_ip_addr, DEFAULT_TCP_TTL, 
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
                    *options_block++ = DEFAULT_TCP_TTL;

                    /* Data Length */
                    *options_block++ = sizeof(UINT8);
                    
                    /* Default TCP TTL */
                    memcpy(options_block, &tcp_ttl, sizeof(UINT8));

                    /* Increment the options block ptr */
                    options_block++;

                    /* Copy the end option tag into the buffer. */
                    *options_block = DHCP_END;

                    /* Increment the bytes written. */
                    option_buffer->bytes_written += (sizeof(UINT8) + 2);

                }    

                else
                {
                    /* We must overwrite the option value that is currently in place.  */

                    /* Increment to the location of the option data. */
                    options_block += 2;

                    /* Default TCP TTL */
                    memcpy(options_block, &tcp_ttl, sizeof(UINT8));

                    /* Increment the options block ptr */

                    /* Add the end option tag. */
                    *options_block = DHCP_END;

                    /* Increment the bytes written. */
                    option_buffer->bytes_written += (sizeof(UINT8));
                }

                /* Unprotect the global data structures. */
                NU_Release_Semaphore(&DHCPS_Semaphore);
            }

            else
                /* An invalid parameter was passed into the function.  The TCP TTL entry can not be added. */
                ret_status = NU_INVALID_PARM;
        }       

        else
        {
            /* This entry is meant for the configuration control block. */

            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* Copy the default TCP TTL into the control block. */
            config_cb->default_tcp_ttl = tcp_ttl;

            /* Set the return status as successful */
            ret_status = NU_SUCCESS;

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);
        }
    }
    /* Switch back to user mode */
    NU_USER_MODE();

    return (ret_status);
} /* DHCPS_Set_TCP_TTL */


/****************************************************************************
* FUNCTION                                                                   
*                                                                            
*    DHCPS_Get_TCP_TTL                                                       
*                                                                            
* DESCRIPTION                                                                
*                                                                            
*    Return the value of the TCP time to live from the specified configuraiton
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
INT DHCPS_Get_TCP_TTL (const DHCPS_CONFIG_CB *config_cb, const struct id_struct *client_ip_addr, 
                               UINT8 *tcp_ttl_buffer, INT buffer_size)
{
    UINT8                       *option_ptr,
                                option_data_len;
    INT                         bytes_written;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* First, ensure that the buffer that has been passed in is large enough to accept at least one
        entry. */
    if ((buffer_size < sizeof(UINT8)) || (tcp_ttl_buffer == NU_NULL))
    {
        bytes_written = NU_INVALID_PARM;
    }

    /* Check to see if the TCP TTL entry is destined for a static IP entry. */
    else if (client_ip_addr != NU_NULL)
    {
        /* Search through the static IP entries to find the default TCP TTL entry. */
        option_ptr = DHCPS_Get_Option_From_Memory_Block(config_cb, client_ip_addr, DEFAULT_TCP_TTL);

        if (option_ptr == NU_NULL)
        {
            /* An error occurred during the search for the TCP TTL for the particular IP address. */
            bytes_written = NU_INVALID_PARM;
        }    

        else
        {
            /* The option has been found for the desired IP address. */

            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* Increment the option pointer to point at the length of the option. */
            option_data_len = *(option_ptr + 1);

            /* Ensure tha that the buffer size is large enough to hold the option data. */
            if (buffer_size < option_data_len)
            {
                bytes_written = DHCPSERR_BUFFER_TOO_SMALL;
            }    

            else
            {
                /* Increment the option pointer to point at the data that will be copied. */
                option_ptr += 2;         

                /* Copy the option into the buffer that has been provided by the application. */
                memcpy(tcp_ttl_buffer, option_ptr, option_data_len);

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
        /* Ensure tha that the buffer size is large enough to hold the option data. */
        if (buffer_size < sizeof(UINT8))
        {
            bytes_written = DHCPSERR_BUFFER_TOO_SMALL;
        }    

        else
        {        
            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* Copy the TCP TTL option entry into the buffer provided by the application. */
            memcpy(tcp_ttl_buffer, &config_cb->default_tcp_ttl, sizeof(UINT8));

            /* Set the return variable to the number of bytes that were written into the buffer. */
            bytes_written = sizeof(UINT8);      

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);
        }    
    }
    /* Switch back to user mode */
    NU_USER_MODE();

    return (bytes_written); 

} /* DHCPS_Get_TCP_TTL */


/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Set_TCP_Keepalive_Interval                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Set the TCP keepalive interval for the specified configuration control block
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
STATUS DHCPS_Set_TCP_Keepalive_Interval (DHCPS_CONFIG_CB *config_cb, const struct id_struct *client_ip_addr, 
                                            UINT32 tcp_keepalive)
{
    STATUS                  ret_status = NU_SUCCESS;
    UINT8                   *options_block;
    DHCPS_OPTIONS           *option_buffer;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Ensure that the configuration control block pointer is valid. */
    if (config_cb == NU_NULL)
    {
        ret_status = NU_INVALID_PARM;
    }    

    else
    {
        /* First, we must determine if this TCP keepalive entry is for a particular IP address (static IP).  
            This can be determined if a client IP address was passed in.  If so, then the TCP keepalive interval 
            needs to be added to an IP option block. */
        if (client_ip_addr != NU_NULL)
        {
            /* Perform any memory block manipulation that may need to be done to make room for
                the keepalive entry that is being added. */
            options_block = DHCPS_Add_Option_To_Memory_Block(config_cb, client_ip_addr, KEEPALIVE_INTERVAL,
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
                    *options_block++ = KEEPALIVE_INTERVAL;

                    /* Data Length */
                    *options_block++ = sizeof(UINT32);
                    
                    /* TCP keepalive interval */
                    memcpy(options_block, &tcp_keepalive, sizeof(UINT32));

                    /* Increment the options block ptr */
                    options_block += sizeof(UINT32);

                    /* Copy the end option tag into the buffer. */
                    *options_block = DHCP_END;

                    /* Increment the bytes written. */
                    option_buffer->bytes_written += (sizeof(UINT32) + 2);

                }    

                else
                {
                    /* We must overwrite the option value that is currently in place.  */

                    /* Increment to the location of the option data. */
                    options_block += 2;

                    /* TCP keepalive interval */
                    memcpy(options_block, &tcp_keepalive, sizeof(UINT32));

                    /* Increment the options block ptr */
                    options_block += sizeof(UINT32);

                    /* Add the end option tag. */
                    *options_block = DHCP_END;

                    /* Increment the bytes written. */
                    option_buffer->bytes_written += (sizeof(UINT32));
                }

                /* Unprotect the global data structures. */
                NU_Release_Semaphore(&DHCPS_Semaphore);
            }

            else
                /* An invalid parameter was passed into the function.  The keepalive interval can not be added. */
                ret_status = NU_INVALID_PARM;
        }       

        else
        {
            /* This entry is meant for the configuration control block. */

            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* Copy the keepalive interval into the control block. */
            config_cb->tcp_keepalive_interval = tcp_keepalive;

            /* Set the return status as successful */
            ret_status = NU_SUCCESS;

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);
        }
    }
    /* Switch back to user mode */
    NU_USER_MODE();

    return (ret_status);
} /* DHCPS_Set_TCP_Keepalive_Interval */


/****************************************************************************
* FUNCTION                                                                   
*                                                                            
*    DHCPS_Get_TCP_Keepalive_Interval                                                       
*                                                                            
* DESCRIPTION                                                                
*                                                                            
*    Return the value of the TCP keepalive interval from the specified configuraiton
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
INT DHCPS_Get_TCP_Keepalive_Interval (const DHCPS_CONFIG_CB *config_cb, const struct id_struct *client_ip_addr, 
                                        UINT8 *tcp_keepalive_buffer, INT buffer_size)
{
    UINT8                       *option_ptr,
                                option_data_len;
    INT                         bytes_written;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* First, ensure that the passed in pointers are valid. */
    if ((config_cb == NU_NULL) || (tcp_keepalive_buffer == NU_NULL))
    {
        bytes_written = NU_INVALID_PARM;
    }

    /* Check to see if the keepalive interval entry is destined for a static IP entry. */
    else if (client_ip_addr != NU_NULL)
    {
        /* Search through the static IP entries to find the keepalive interval entry. */
        option_ptr = DHCPS_Get_Option_From_Memory_Block(config_cb, client_ip_addr, KEEPALIVE_INTERVAL);

        if (option_ptr == NU_NULL)
        {
            /* An error occurred during the search for the keepalive interval for the particular 
                IP address. */
            bytes_written = NU_INVALID_PARM;
        }    

        else
        {
            /* The TCP keepalive interval option has been found for the desired IP address. */

            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* Increment the option pointer to point at the length of the option. */
            option_data_len = *(option_ptr + 1);

            /* Ensure that the buffer size is large enough to hold the option data. */
            if (buffer_size < option_data_len)
            {
                bytes_written = DHCPSERR_BUFFER_TOO_SMALL;
            }    

            else
            {
                /* Increment the option pointer to point at the data that will be copied. */
                option_ptr += 2;

                /* Copy the option into the buffer that has been provided by the application. */
                memcpy(tcp_keepalive_buffer, option_ptr, option_data_len);

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
        /* Ensure that the buffer size is large enough to hold the option data. */
        if (buffer_size < sizeof(UINT32))
        {
            bytes_written = DHCPSERR_BUFFER_TOO_SMALL;
        }    

        else
        {        
            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* Copy the option entry into the buffer provided by the application. */
            memcpy(tcp_keepalive_buffer, &config_cb->tcp_keepalive_interval, sizeof(UINT32));

            /* Set the return variable to the number of bytes that were written into the buffer. */
            bytes_written = sizeof(UINT32);  

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);
        }    
    }
    /* Switch back to user mode */
    NU_USER_MODE();

    return (bytes_written); 

} /* DHCPS_Get_TCP_Keepalive_Interval */



/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Enable_TCP_Keepalive_Garbage                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Set the bit in the specified configuration control block status flags to
*     alert the requesting clients to use TCP keepalive garbage.
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
STATUS DHCPS_Enable_TCP_Keepalive_Garbage (DHCPS_CONFIG_CB *config_cb)
{
    STATUS                  ret_status = NU_SUCCESS;
    UINT16                  flag_stat;
                            
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Ensure that we have been passed a valid control block pointer. */
    if (config_cb != NU_NULL)
    {
        /* Instructs the client to send a garbage byte in keepalive segments. */

        /* Protect the global data structures. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

        /* Get the current status of the flags variable. */
        flag_stat = config_cb->flags;

        /* Set the TCP keepalive garbage bit of the flags variable. */
        flag_stat = flag_stat | TCP_KEEPALIVE_GARBAGE;

        /* Save the new flag settings to the configuration control block. */
        config_cb->flags = flag_stat;

        /* TCP keepalive garbage has been successfully added.. */  
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
}   /* DHCPS_Enable_TCP_Keepalive_Garbage */


/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Disable_TCP_Keepalive_Garbage                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Reset the bit of the specified configuration control block so that requesting
*     clients will not be informed to use TCP keepalive garbage.
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
STATUS DHCPS_Disable_TCP_Keepalive_Garbage (DHCPS_CONFIG_CB *config_cb)
{
    STATUS                  ret_status = NU_SUCCESS;
    UINT16                  flag_stat;
   
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Ensure that we have been passed a valid control block pointer. */
    if (config_cb != NU_NULL)
    {
        /* If this option is disabled, the requesting client will be instructed not to
            send a garbage byte in its keepalive sgements. */

        /* Protect the global data structures. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

        /* Get the current settings of the control block flags. */
        flag_stat = config_cb->flags;

        /* Clear the TCP keepalive garbage bit */
        flag_stat = flag_stat & ~TCP_KEEPALIVE_GARBAGE;

        /* Save the new flag settings to the configuration control block. */
        config_cb->flags = flag_stat;

        /* The TCP keepalive garbage has been disabled. */  
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
}   /* DHCPS_Disable_TCP_Keepalive_Garbage */
