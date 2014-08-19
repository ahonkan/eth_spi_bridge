/*************************************************************************
*
*              Copyright 2012 Mentor Graphics Corporation
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
*       sck6_gnd.c
*
*   DESCRIPTION
*
*       This file contains the routine that retrieves the MAC address
*       associated with a foreign IPv6 Address as stored in the Neighbor
*       Cache.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Ioctl_SIOCLIFGETND
*
*   DEPENDENCIES
*
*       nu_net.h
*       externs6.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/externs6.h"

/*************************************************************************
*                                                                         
*   FUNCTION                                                              
*                                                                         
*       NU_Ioctl_SIOCLIFGETND
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function retrieves the MAC address associated with a foreign
*       IPv6 address as stored in the Neighbor Cache.
*                                                                         
*   INPUTS                                                                
*                                                                         
*       *option                 Return pointer for option status. 
*                                                                         
*   OUTPUTS                                                               
*                                                                         
*       NU_SUCCESS              Successful completion of the service.
*       NU_INVALID_PARM         One of the input parameters is invalid.
*                                                                         
*************************************************************************/
STATUS NU_Ioctl_SIOCLIFGETND(SCK_IOCTL_OPTION *option)
{
    STATUS  status; 

    NU_SUPERV_USER_VARIABLES

    /* If the user passed a NULL parameter, set status to error */
    if (option != NU_NULL)
    {
        /* Switch to supervisor mode. */
        NU_SUPERVISOR_MODE();

        status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);
        
        if (status == NU_SUCCESS)
        {
            /* Retrieve the MAC address. */
            status = IP6_Ioctl_SIOCLIFGETND((CHAR*)option->s_optval,
                                             option->s_ret.s_ipaddr,
                                             option->s_ret.mac_address);

            if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
                NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE, 
                               __FILE__, __LINE__);
        }

        /* Switch back to user mode. */
        NU_USER_MODE();
    }

    else
        status = NU_INVALID_PARM;

    return (status);

} /* NU_Ioctl_SIOCLIFGETND */
