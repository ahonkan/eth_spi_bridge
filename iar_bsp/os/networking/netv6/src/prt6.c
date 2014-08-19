/*************************************************************************
*
*              Copyright 2002 Mentor Graphics Corporation              
*                         All Rights Reserved.                          
*                                                                       
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   
* SUBJECT TO LICENSE TERMS.                                             
*
*************************************************************************/

/*************************************************************************
*                                                                       
*   FILE NAME                                                               
*                                                                               
*       prt6.c                                       
*                                                                               
*   DESCRIPTION                                                           
*                                                                       
*       This file contains the function to find a port for an IPv6
*       session.
*                                                                       
*   DATA STRUCTURES                                                       
*                                                                       
*       None
*                                                                       
*   FUNCTIONS                                                             
*           
*       PRT6_Find_Matching_Port
*                                                                       
*   DEPENDENCIES                                                          
*                                                                       
*       externs.h
*       tcp6.h
*       udp6.h
*       externs6.h
*                                                                       
*************************************************************************/

#include "networking/externs.h"
#include "networking/tcp6.h"
#include "networking/udp6.h"
#include "networking/externs6.h"

/*************************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       PRT6_Find_Matching_Port                                                     
*                                                                       
*   DESCRIPTION                                                           
*                                                                       
*       This function returns the port associated with the parameters
*       provided.
*                                                                       
*   INPUTS                                                                
*                                                                       
*       protocol                The protocol of the port to find.
*       *source_ip              A pointer to the Source Address of the 
*                               target entry.
*       *dest_ip                A pointer to the Destination Address of 
*                               the target entry.    
*       source_port             The source port of the target entry.
*       dest_port               The destination port of the target entry.
*                                                                       
*   OUTPUTS                                                               
*                                                                       
*       The index of the corresponding port or -1 if no port found.
*                                                                       
*************************************************************************/
INT32 PRT6_Find_Matching_Port(INT protocol, const UINT8 *source_ip, 
                              const UINT8 *dest_ip, UINT16 source_port, 
                              UINT16 dest_port)
{
    INT32   status;

    switch (protocol)
    {
    case NU_PROTO_UDP:

        status = UDP6_Find_Matching_UDP_Port(source_ip, dest_ip, source_port, 
                                             dest_port);
        break;

    case NU_PROTO_TCP:

        status = TCP6_Find_Matching_TCP_Port(source_ip, dest_ip, source_port, 
                                             dest_port);
        break;

    default:

        status = -1;
        break;
    }

    return (status);

} /* PRT6_Find_Matching_Port */
