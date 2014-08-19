/****************************************************************************
*
*            Copyright Mentor Graphics Corporation 2001-2011
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
****************************************************************************/
/****************************************************************************
*                                                                            
*   FILENAME                                                                          
*                                                                                    
*       nat_mgmt.c                                                  
*                                                                                    
*   DESCRIPTION                                                                
*
*       This file contains the functions necessary to retrieve statistics
*       about the Nucleus NAT environment.
*                                                           
*   DATA STRUCTURES                                                            
*
*       None.                             
*                                               
*   FUNCTIONS                                                                  
*                
*       NATMGMT_Calculate_Used_TCP_Ports 
*       NATMGMT_Calculate_Used_UDP_Ports 
*       NATMGMT_Calculate_Used_ICMP_Ports  
*       NATMGMT_Calculate_Total_Portmap_Entries                         
*                                             
*   DEPENDENCIES                                                               
*
*       target.h
*       externs.h
*       nat_defs.h            
*       nat_mgmt.h
*                                                                
******************************************************************************/

#include "networking/target.h"
#include "networking/externs.h"
#include "networking/nat_defs.h"
#include "networking/nat_mgmt.h"

extern NAT_TRANSLATION_TABLE   NAT_Translation_Table;
extern NAT_PORTMAP_TABLE       NAT_Portmap_Table;

/***********************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       NATMGMT_Calculate_Used_TCP_Ports
*                                                                       
*   DESCRIPTION                                                           
*                 
*       This function returns the total number of TCP entries currently
*       being used by NAT.
*                                                                       
*   INPUTS                                                                
*          
*       None
*                                                                       
*   OUTPUTS                                                               
*                 
*       The total number of TCP entries in use.
*
*************************************************************************/
UINT16 NATMGMT_Calculate_Used_TCP_Ports(VOID)
{ 
    UINT16  tcp_ports = 0;
    UINT16  i;

    /* Traverse the array of TCP entries.  Increment the number of TCP
     * ports open for each entry with a timeout value greater than 0.
     */
    for (i = 0; i < NAT_MAX_TCP_CONNS; i++)
    {
        if (NAT_Translation_Table.nat_tcp_table->nat_tcp_entry[i].nat_timeout != 0)
            tcp_ports++;
    }

    return (tcp_ports);

} /*  NATMGMT_Calculate_Used_TCP_Ports */

/***********************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       NATMGMT_Calculate_Used_UDP_Ports
*                                                                       
*   DESCRIPTION                                                           
*                 
*       This function returns the total number of UDP entries currently
*       being used by NAT.
*                                                                       
*   INPUTS                                                                
*          
*       None
*                                                                       
*   OUTPUTS                                                               
*                 
*       The total number of UDP entries in use.
*
*************************************************************************/
UINT16 NATMGMT_Calculate_Used_UDP_Ports(VOID)
{
    UINT16  udp_ports = 0;
    UINT16  i;

    /* Traverse the array of UDP entries.  Increment the number of UDP
     * ports open for each entry with a timeout value greater than 0.
     */
    for (i = 0; i < NAT_MAX_UDP_CONNS; i++)
    {
        if (NAT_Translation_Table.nat_udp_table->nat_udp_entry[i].nat_timeout != 0)
            udp_ports++;
    }

    return (udp_ports);

} /* NATMGMT_Calculate_Used_UDP_Ports */

/***********************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       NATMGMT_Calculate_Used_ICMP_Ports
*                                                                       
*   DESCRIPTION                                                           
*                 
*       This function returns the total number of ICMP entries currently
*       being used by NAT.
*                                                                       
*   INPUTS                                                                
*          
*       None
*                                                                       
*   OUTPUTS                                                               
*                 
*       The total number of ICMP entries in use.
*
*************************************************************************/
UINT16 NATMGMT_Calculate_Used_ICMP_Ports(VOID)
{
    UINT16  icmp_ports = 0;
    UINT16  i;

    /* Traverse the array of ICMP entries.  Increment the number of ICMP
     * ports open for each entry with a timeout value greater than 0.
     */
    for (i = 0; i < NAT_MAX_ICMP_CONNS; i++)
    {
        if (NAT_Translation_Table.nat_icmp_table->nat_icmp_entry[i].nat_timeout != 0)
            icmp_ports++;
    }

    return (icmp_ports);

} /* NATMGMT_Calculate_Used_ICMP_Ports */

/***********************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       NATMGMT_Calculate_Total_Portmap_Entries
*                                                                       
*   DESCRIPTION                                                           
*                 
*       This function returns the total number of portmap entries in
*       the NAT Portmap Table.
*                                                                       
*   INPUTS                                                                
*          
*       None
*                                                                       
*   OUTPUTS                                                               
*                 
*       The total number of portmap entries.
*
*************************************************************************/
UINT16 NATMGMT_Calculate_Total_Portmap_Entries(VOID)
{
    NAT_PORTMAP_ENTRY   *current_entry;
    UINT16              total_entries = 0;

    current_entry = NAT_Portmap_Table.nat_head;

    while (current_entry)
    {
        current_entry = current_entry->nat_next;
        total_entries ++;
    }

    return (total_entries); 
    
} /* NATMGMT_Calculate_Total_Portmap_Entries */
