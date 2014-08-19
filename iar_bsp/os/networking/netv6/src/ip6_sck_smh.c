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
*       ip6_sck_smh.c                                
*                                                                                  
*   DESCRIPTION                                                              
*                
*       This file contains the routine for setting the multicast 
*       hop limit.
*                                                                          
*   DATA STRUCTURES                                                          
*                      
*       None                
*                                                                          
*   FUNCTIONS                                                                
*           
*       IP6_Set_IPV6_MULTICAST_HOPS
*                                                                          
*   DEPENDENCIES                                                             
*              
*       nu_net.h
*                                                                          
*************************************************************************/

#include "networking/nu_net.h"

/***********************************************************************
*                                                                       
*   FUNCTION                                                              
*                                                                       
*       IP6_Set_IPV6_MULTICAST_HOPS                                                       
*                                                                       
*   DESCRIPTION                                                           
*                                                                         
*       This function sets the multicast interface for a socket.
*                                                                         
*   INPUTS                                                                
*                                                                         
*       socketd                 Specifies a socket descriptor
*       hop_limit               Hop limit to use for multicast packets
*                               sent via this socket.
*
*   OUTPUTS                                                               
*                                                                         
*       NU_SUCCESS              Successful operation.
*       NU_MEM_ALLOC            Insufficient memory.
*       NU_INVAL                hop_limit is invalid.
*                                                                       
*************************************************************************/
STATUS IP6_Set_IPV6_MULTICAST_HOPS(INT socketd, INT hop_limit)
{
    struct  sock_struct *sck_ptr = SCK_Sockets[socketd];
    MULTI_SCK_OPTIONS   *moptions;
    STATUS              status = NU_SUCCESS;

    /* Is there a multicast option buffer attached to the socket. */
    if (sck_ptr->s_moptions_v6 == NU_NULL)
    {
        /* Allocate a multicast option buffer. */
        if (NU_Allocate_Memory(MEM_Cached, (VOID**)&sck_ptr->s_moptions_v6,
                               sizeof(MULTI_SCK_OPTIONS), 
                               (UNSIGNED)NU_NO_SUSPEND) != NU_SUCCESS)
            return (NU_MEM_ALLOC);

        /* Initialize the option buffer to the default values. */
        moptions = sck_ptr->s_moptions_v6;
        UTL_Zero(moptions, sizeof(MULTI_SCK_OPTIONS));
        moptions->multio_device = NU_NULL;
        moptions->multio_hop_lmt = IP6_DEFAULT_MULTICAST_TTL;
        moptions->multio_loop = IP6_DEFAULT_MULTICAST_LOOP;
        moptions->multio_num_mem_v6 = 0;
    }
    else
        moptions = sck_ptr->s_moptions_v6;

    /* If a valid value was passed in, use the value.  If -1 was passed in,
     * use the kernel default.
     */
    if ( (hop_limit >= 0) && (hop_limit <= 255) )
        moptions->multio_hop_lmt = (UINT8)hop_limit;

    /* If an invalid value was passed in, return an error */
    else if ( (hop_limit < -1) || (hop_limit >= 256) )
        status = NU_INVAL;

    /* If all the options have default values, then there is no need 
     * to keep the structure. 
     */
    if ( (moptions->multio_device == NU_NULL) && 
         (moptions->multio_hop_lmt == IP6_DEFAULT_MULTICAST_TTL) &&
         (moptions->multio_loop == IP6_DEFAULT_MULTICAST_LOOP) &&
         (moptions->multio_num_mem_v6 == 0) )
    {
        if (NU_Deallocate_Memory(moptions) != NU_SUCCESS)
            NLOG_Error_Log("Failed to deallocate memory for multicast options", 
                           NERR_SEVERE, __FILE__, __LINE__);

        sck_ptr->s_moptions_v6 = NU_NULL;
    }

    return (status);

} /* IP6_Set_IPV6_MULTICAST_HOPS */
