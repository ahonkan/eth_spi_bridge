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
*       sck6_cii.c
*                                                                                 
*   DESCRIPTION                                                           
*                                                                         
*       This file contains the function to configure IPv6 features for
*       an interface.
*                                                                         
*   DATA STRUCTURES                                                       
*                                                                         
*       None
*                                                                         
*   FUNCTIONS                                                             
*                                                                         
*       NU_Configure_IPv6_Interface                                                      
*
*   DEPENDENCIES                                                          
*                                                                         
*       nu_net.h
*                                                                         
*************************************************************************/

#include "networking/nu_net.h"

/*************************************************************************
*
*   FUNCTION                                                              
*
*       NU_Configure_IPv6_Interface
*
*   DESCRIPTION                                                           
*
*       This function configures the IPv6 features of an interface.
*
*   INPUTS                    
*                           
*       dev_index               The interface index of the interface to
*                               configure.
*       opt_name                The parameter to configure.  Current
*                               available options include:
*                                - IP6_AUTO_ADDR_CONFIG - configures the
*                                  ability to create global and site-local
*                                  addresses from advertised prefixes.  1
*                                  enables the feature.  0 disables it.
*       opt_val                 The value of the parameter.
*
*   OUTPUTS
*
*       NU_SUCCESS              The IP address was successfully added.
*       NU_INVALID_PARM         One of the input parameters is invalid.
*
*************************************************************************/
STATUS NU_Configure_IPv6_Interface(UINT32 dev_index, INT opt_name, 
                                   VOID *opt_val)
{
    DV_DEVICE_ENTRY *device;
    STATUS          status;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Obtain the semaphore */
    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status != NU_SUCCESS)
    {
        NU_USER_MODE();
        return (status);
    }

    /* Get a pointer to the interface. */
    device = DEV_Get_Dev_By_Index(dev_index);

    /* If an IPv6 enabled device was found */
    if ( (device) && (device->dev_flags & DV6_IPV6) )
    {
        switch (opt_name)
        {
            case IP6_AUTO_ADDR_CONFIG:

                /* Disable global and site-local address configuration
                 * on the interface.
                 */
                if (*(INT*)opt_val == 0)
                {
                    /* Add the flag to disable address configuration. */
                    device->dev6_flags |= DV6_DISABLE_ADDR_CONFIG;
                }

                /* Enable global and site-local address configuration
                 * on the interface.
                 */
                else
                {
                    /* Remove the flag that disables address configuration. */
                    device->dev6_flags &= ~DV6_DISABLE_ADDR_CONFIG;
                }

                break;

            default:

                status = NU_INVALID_PARM;
                break;
        }
    }

    else
    {
        status = NU_INVALID_PARM;

        NLOG_Error_Log("No matching interface entry for name", NERR_INFORMATIONAL, 
                       __FILE__, __LINE__);
    }

    /* Release the semaphore */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE, 
                       __FILE__, __LINE__);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* NU_Configure_IPv6_Interface */
