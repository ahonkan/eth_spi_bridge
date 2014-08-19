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
*       sck6_gda.c                                   
*
*   DESCRIPTION
*
*       This file contains the routine that gets the IPV6 address  
*       of the foreign side of the link
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Ioctl_SIOCGIFDSTADDR_IN6
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/externs6.h"

/*************************************************************************
*                                                                         
*   FUNCTION                                                              
*                                                                         
*       NU_Ioctl_SIOCGIFDSTADDR_IN6                                                     
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function gets the IPV6 address of the foreign side of the link
*                                                                         
*   INPUTS                                                                
*                                                                         
*       *option                 Return pointer for option status. 
*       optlen                  Specifies the size in bytes of the 
*                               location pointed to by option
*                                                                         
*   OUTPUTS                                                               
*                                                                         
*       NU_SUCCESS              Successful completion of the service.
*       NU_INVALID_PARM         One of the input parameters is invalid.
*                                                                         
*************************************************************************/
STATUS NU_Ioctl_SIOCGIFDSTADDR_IN6(SCK_IOCTL_OPTION *option, INT optlen)
{
    STATUS  status; 

    NU_SUPERV_USER_VARIABLES

    /* If the user passed a NULL parameter, set status to error */
    if ( (option != NU_NULL) && (optlen >= IP6_ADDR_LEN) )
    {
        /* Switch to supervisor mode. */
        NU_SUPERVISOR_MODE();

        status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);
        
        if (status == NU_SUCCESS)
        {
            status = IP6_Ioctl_SIOCGIFDSTADDR_IN6((CHAR*)option->s_optval, 
                                                  option->s_ret.s_ipaddr);

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

} /* NU_Ioctl_SIOCGIFDSTADDR_IN6 */
