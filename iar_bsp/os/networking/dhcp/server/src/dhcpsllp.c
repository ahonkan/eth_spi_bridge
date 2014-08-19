/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2002              
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
*     dhcpsllp.c                                               
*                                                                      
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     This file contains the API functions of the DHCP Server that deal
*     with the link layer options.
*
* DATA STRUCTURES
*
*     None
*                                                                      
* FUNCTIONS                                                            
*
*     DHCPS_Set_ARP_Cache_Timeout
*     DHCPS_Get_ARP_Cache_Timeout
*     DHCPS_Enable_Ethernet_IEEE_Encapsulation
*     DHCPS_Disable_Ethernet_IEEE_Encapsulation
*     DHCPS_Enable_Trailer_Encapsulation
*     DHCPS_Disable_Trailer_Encapsulation
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
*     DHCPS_Set_ARP_Cache_Timeout                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Set the new ARP cache timeout value into the specified configuration control
*     block block or specific binding.
*
* INPUTS                                                               
*                                                                      
*     DHCPS_CONFIG_CB *
*     struct id_struct *
*     UINT32
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     STATUS                                    
*                                                                      
**********************************************************************************/
STATUS DHCPS_Set_ARP_Cache_Timeout (DHCPS_CONFIG_CB *config_cb, const struct id_struct *client_ip_addr, 
                                    UINT32 arp_cache_timeout)
{
    STATUS                  ret_status = NU_SUCCESS;
    UINT8                   *options_block;
    DHCPS_OPTIONS           *option_buffer;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Ensure that the passed in control block pointer is valid.  If not, return an error. */
    if (config_cb == NU_NULL)
    {
        ret_status = NU_INVALID_PARM;
    }    

    else
    {
        /* We must determine if this ARP cache timeout entry is for a particular IP address 
            (static IP). This can be determined if a client IP address was passed in.  If so, then 
            the renewal needs to be added to an IP option block. */
        if (client_ip_addr != NU_NULL)
        {
            /* Perform any memory block manipulation that may need to be done to make room for
                the ARP cache timeout entry that is being added. */
            options_block = DHCPS_Add_Option_To_Memory_Block(config_cb, client_ip_addr, ARP_CACHE_TIMEOUT, 
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
                    *options_block++ = ARP_CACHE_TIMEOUT;

                    /* Data Length */
                    *options_block++ = sizeof(UINT32);

                    /* ARP cache timeout */
                    memcpy(options_block, &arp_cache_timeout, sizeof(UINT32));

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

                    /* ARP cache timeout */
                    memcpy(options_block, &arp_cache_timeout, sizeof(UINT32));

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
                /* An invalid parameter was passed into the function.  The timeout value can not be 
                    added. */
                ret_status = NU_INVALID_PARM;
        }       

        else
        {
            /* This entry is meant for the configuration control block. */

            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* Copy the ARP cache timeout into the control block. */
            config_cb->arp_cache_to = arp_cache_timeout;

            /* Set the return status as successful */
            ret_status = NU_SUCCESS;

            /* Unprotect the global data structures. */
            NU_Release_Semaphore(&DHCPS_Semaphore);
        }
    }
    /* Switch back to user mode */
    NU_USER_MODE();

    return (ret_status);
} /* DHCPS_Set_ARP_Cache_Timeout */


/****************************************************************************
* FUNCTION                                                                   
*                                                                            
*    DHCPS_Get_ARP_Cache_Timeout                                                       
*                                                                            
* DESCRIPTION                                                                
*                                                                            
*    Return the value for the ARP cache timeout from the specified configuration
*    control block or specific binding in the provided buffer.  The return 
*    value is the total number of bytes that were written into the buffer. 
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
INT DHCPS_Get_ARP_Cache_Timeout (const DHCPS_CONFIG_CB *config_cb, const struct id_struct *client_ip_addr, 
                                 UINT8 *arp_cache_buffer, INT buffer_size)
{
    UINT8                       *option_ptr,
                                option_data_len;
    INT                         bytes_written;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* First, ensure that the passed in pointers are valid. */
    if ((config_cb == NU_NULL) || (arp_cache_buffer == NU_NULL))
    {
        bytes_written = NU_INVALID_PARM;
    }

    /* Check to see if the ARP cache entry is destined for a static IP entry. */
    else if (client_ip_addr != NU_NULL)
    {
        /* Search through the static IP entries to find the lease renewal time entry. */
        option_ptr = DHCPS_Get_Option_From_Memory_Block(config_cb, client_ip_addr, ARP_CACHE_TIMEOUT);

        if (option_ptr == NU_NULL)
        {
            /* An error occurred during the search for the ARP cache timeout for the particular 
                IP address. */
            bytes_written = NU_INVALID_PARM;
        }    

        else
        {
            /* The ARP cache timeout option has been found for the desired IP address. */

            /* Protect the global data structures. */
            NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

            /* Increment the option pointer to point at the length of the ARP cache option. */
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

                /* Copy the ARP cache option into the buffer that has been provided by the application. */
                memcpy(arp_cache_buffer, option_ptr, option_data_len);

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
        if (buffer_size < sizeof(UINT32))
        {
            bytes_written = DHCPSERR_BUFFER_TOO_SMALL;
        }    

        else        
        {
            /* Copy the ARP cache option entry into the buffer provided by the application. */
            memcpy(arp_cache_buffer, &config_cb->arp_cache_to, sizeof(UINT32));

            /* Set the return variable to the number of bytes that were written into the buffer. */
            bytes_written = sizeof(UINT32);  
        } 
        /* Unprotect the global data structures. */
        NU_Release_Semaphore(&DHCPS_Semaphore);
    }
    /* Switch back to user mode */
    NU_USER_MODE();

    return (bytes_written); 

} /* DHCPS_Get_ARP_Cache_Timeout */

/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Enable_Ethernet_IEEE_Encapsulation                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Set the bit in the specified configuration control block's status flags  to 
*     instruct requesting clients to enable ethernet encapsulation.
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
STATUS DHCPS_Enable_Ethernet_IEEE_Encapsulation (DHCPS_CONFIG_CB *config_cb)
{
    STATUS                  ret_status = NU_SUCCESS;
    UINT16                  flag_stat;
                            
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Ensure that we have been passed a valid control block pointer. */
    if (config_cb != NU_NULL)
    {
        /* Two forms of ethernet encapsulation can be provided to requesting clients.  
            If the ethernet encapsulation bit in the flag variable is set to 0, the
            client will use Ethernet version 2 encapsulation.  If the bit is set to
            1, then IEEE 802.3 encapsulation will be specified.*/

        /* Protect the global data structures. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

        /* Get the current status of the flags variable. */
        flag_stat = config_cb->flags;

        /* Set the IEEE Ethernet encapsulation bit of the flags variable. */
        flag_stat = flag_stat | ETHERNET_IEEE_802_ENCAPSULATION;

        /* Save the new flag settings to the configuration control block. */
        config_cb->flags = flag_stat;

        /* The IEEE Ethernet encapsulation has been successfully added.. */  
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
}   /* DHCPS_Enable_Ethernet_IEEE_Encapsulation */


/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Disable_Ethernet_IEEE_Encapsulation                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Clear the bit in the specified configuration control block's status flag so that
*     requesting clients will not be instructed to use ethernet encapsulation.
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
STATUS DHCPS_Disable_Ethernet_IEEE_Encapsulation (DHCPS_CONFIG_CB *config_cb)
{
    STATUS                  ret_status = NU_SUCCESS;
    UINT16                  flag_stat;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Ensure that we have been passed a valid control block pointer. */
    if (config_cb != NU_NULL)
    {
        /* By disabling IEEE Ethernet encapsulation, Ethernet version 2 encapsulation
            is specified. */

        /* Protect the global data structures. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

        /* Get the current settings of the control block flags. */
        flag_stat = config_cb->flags;

        /* Clear the IEEE Ethernet encapsulation bit */
        flag_stat = flag_stat & ~ETHERNET_IEEE_802_ENCAPSULATION;

        /* Save the new flag settings to the configuration control block. */
        config_cb->flags = flag_stat;

        /* The IEEE Ethernet encapsulation has been disabled. */  
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
}   /* DHCPS_Disable_Ethernet_IEEE_Encapsulation */


/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Enable_Trailer_Encapsulation                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Set the bit in the specified configuration control block's status flags  to 
*     instruct requesting clients to enable trailer encapsulation.
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
STATUS DHCPS_Enable_Trailer_Encapsulation (DHCPS_CONFIG_CB *config_cb)
{
    STATUS                  ret_status = NU_SUCCESS;
    UINT16                  flag_stat;
                            
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Ensure that we have been passed a valid control block pointer. */
    if (config_cb != NU_NULL)
    {
        /* Trailer encapsulation can be enabled for the requesting clients. */

        /* Protect the global data structures. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

        /* Get the current status of the flags variable. */
        flag_stat = config_cb->flags;

        /* Set the trailer encapsulation bit of the flags variable. */
        flag_stat = flag_stat | TRAILER_ENCAPSULATION;

        /* Save the new flag settings to the configuration control block. */
        config_cb->flags = flag_stat;

        /* The trailer encapsulation has been successfully added.. */  
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
}   /* DHCPS_Enable_Trailer_Encapsulation */


/**********************************************************************************
* FUNCTION                                                             
*                                                                      
*     DHCPS_Disable_Trailer_Encapsulation                                        
*                                                                      
* DESCRIPTION                                                          
*                                                                      
*     Clear the bit in the specified configuration control block's status flag so that
*     requesting clients will not be instructed to use trailer encapsulation.
*
* INPUTS                                                               
*                                                                      
*     None.                                      
*                                                                      
* OUTPUTS                                                              
*                                                                      
*     None.                                    
*                                                                      
**********************************************************************************/
STATUS DHCPS_Disable_Trailer_Encapsulation (DHCPS_CONFIG_CB *config_cb)
{
    STATUS                  ret_status = NU_SUCCESS;
    UINT16                  flag_stat;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode */
    NU_SUPERVISOR_MODE();

    /* Ensure that we have been passed a valid control block pointer. */
    if (config_cb != NU_NULL)
    {
        /* Trailer encapsulation can be disabled to instruct requesting clients not
            to use trailer encapsulation. */

        /* Protect the global data structures. */
        NU_Obtain_Semaphore(&DHCPS_Semaphore, NU_SUSPEND);

        /* Get the current settings of the control block flags. */
        flag_stat = config_cb->flags;

        /* Clear the trailer encapsulation bit */
        flag_stat = flag_stat & ~TRAILER_ENCAPSULATION;

        /* Save the new flag settings to the configuration control block. */
        config_cb->flags = flag_stat;

        /* The trailer encapsulation has been disabled. */  
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
}   /* DHCPS_Disable_Trailer_Encapsulation */
