/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 2003              
*                         All Rights Reserved.                          
*                                                                       
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   
* SUBJECT TO LICENSE TERMS.                                             
*
**************************************************************************/ 
/*************************************************************************
*                                                                            
* FILE NAME                                                                         
*                                                                                    
*   dhcps_cfg.h
*
* COMPONENT
*
*   Nucleus DHCP Server
*                                                                                    
* DESCRIPTION                                                                
*                                                                            
*   User configurable macros that are used by the Nucleus DHCP Server
*
* DATA STRUCTURES
*
*   None
*
* DEPENDENCIES
*
*   None
*                                                                            
**************************************************************************/
#ifndef _DHCPS_CFG_H_
#define _DHCPS_CFG_H_

/* Maximum number of characters to be used for an entry name. */
#define DHCPS_MAX_ENTRY_NAME_LENGTH     8           

/* The interval between savings of the binding structures to a file.  
    This saving of the binding structures needs to take place at regular intervals  
    in case of a server restart. */
#define DHCPS_LEASE_TIMER_SLEEP  (60 * SCK_Ticks_Per_Second)  

/* If an offered binding is declined, the IP address associated with that binding is in use by 
    another client.  Therefore, this binding will be marked as IN_USE for this period of time so 
    that it will not be immediately offered to another client. */
#define DHCPS_DEFAULT_DECLINED_LEASE_LENGTH     360  

/* This macro is the value for the block of memory that will be read from a file at initialization.  
    If memory limitations are a concern, this macro can be set to a lower value.  However, this may 
    lengthen the initialization process, due to the need for multiple file reads. */
#define DHCPS_FILE_READ_SIZE                    512   

/* This macro is the maximum length of the file names for the configuration and binding files.  
    This includes the file extension as well.  */
#define DHCPS_MAX_FILE_NAME_SIZE                30    

/* This macro is the maximum length for a character string.  If a character string is longer than 
    this value, it will be truncated to the MAX_STRING_LEN. */  
#define DHCPS_MAX_STRING_LEN                    128  

/* This macro is the maximum length of the domain name. */
#define DHCPS_MAX_DOMAIN_NAME_LENGTH            16

/* This macro is the maximum length for a client ID that the server will encounter.  The client ID 
    is a unique combination of characters that the server can use to distinguish between different 
    clients. */ 
#define DHCPS_MAX_CLIENT_ID_LENGTH              16    

/* This macro is the maximum number of DNS servers that can be added to an individual configuration 
    control block. */ 
#define DHCPS_MAX_DNS_SERVERS                   3

/* This macro is the maximum number of routers that can be added to an individual configuration 
    control block. */
#define DHCPS_MAX_ROUTERS                       3

/* This macro is the size of the memory block that will be used to store option values that are specific 
    to a certain binding. */
#define DHCPS_OPTIONS_MEMORY_BLOCK_SIZE         200

#endif /* _DHCPS_CFG_H_ */
