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
*       ip6_sck_sc.c                                 
*                                                                                  
*   DESCRIPTION                                                              
*                
*       This file contains the routine for enabling or disabling the
*       socket option for the IP layer to compute and insert the
*       checksum into a RAW packet and also verify the checksum on 
*       receipt.
*                                                                          
*   DATA STRUCTURES                                                          
*                      
*       None                
*                                                                          
*   FUNCTIONS                                                                
*           
*       IP6_Set_IPV6_CHECKSUM
*                                                                          
*   DEPENDENCIES                                                             
*              
*       nu_net.h
*                                                                          
*************************************************************************/

#include "networking/nu_net.h"
#include "networking/ipraw.h"

/***********************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       IP6_Set_IPV6_CHECKSUM                                                       
*                                                                       
*   DESCRIPTION                                                           
*                                                                         
*       This function sets the socket option for enabling or disabling
*       the socket option for the IP layer to compute and insert the 
*       checksum into a RAW packet and also verify the checksum on 
*       receipt.
*                                                                         
*   INPUTS                                                                
*                                                                         
*       socketd                 Specifies a socket descriptor
*       optval                  The integer offset into the user data of
*                               where the checksum is located.
*
*   OUTPUTS                                                               
*                                                                         
*       NU_SUCCESS              Successful operation.
*       NU_INVALID_SOCKET       The socket is not a raw socket.
*       NU_INVAL                The offset is a positive odd value.
*                                                                       
*************************************************************************/
STATUS IP6_Set_IPV6_CHECKSUM(INT socketd, INT optval)
{
    struct  sock_struct *sck_ptr = SCK_Sockets[socketd];
    STATUS  status = NU_SUCCESS;

    /* An attempt to set this socket option for a non-raw socket will
     * fail.
     */
    if (IS_RAW_PROTOCOL(sck_ptr->s_protocol))
    {
        /* Enable the option on the socket */
        if (optval)
        {   
            /* A positive odd value is an invalid offset */
            if (!(optval & 1))
            {
                /* Set the flag */
                sck_ptr->s_options |= SO_IPV6_CHECKSUM;

                /* Save the offset into the packet where the checksum is
                 * located.
                 */
                IPR_Ports[sck_ptr->s_port_index]->ip_chk_off = optval;
            }

            else
                status = NU_INVAL;
        }

        /* Disable the option on the socket */
        else  
            sck_ptr->s_options &= ~SO_IPV6_CHECKSUM;
    }

    else
        status = NU_INVALID_SOCKET;

    return (status);

} /* IP6_Set_IPV6_CHECKSUM */
