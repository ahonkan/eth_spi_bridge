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
*       sck6_cr.c                                    
*
*   DESCRIPTION
*
*       This file contains the implementation of 
*       NU_Configure_Router.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Configure_Router
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/*************************************************************************
*                                                                         
*   FUNCTION                                                              
*                                                                         
*       NU_Configure_Router
*                                                                         
*   DESCRIPTION                                                           
*                                                                         
*       This function configures router-specific parameters for an 
*       IPv6-enabled interface.
*                                                                         
*   INPUTS                                                                
*                                                 
*       dev_index               The interface index of the interface to 
*                               configure.
*       flags                   Flags to set or remove from the interface:
*
*                               DV6_ISROUTER - This interface should 
*                                              transmit Router Advertisement 
*                                              messages and process Router 
*                                              Solicitation messages.
*
*                               DV6_ADV_MGD  - Set the Managed Address 
*                                              Configuration flag in 
*                                              outgoing Router Advertisement 
*                                              messages.
*
*                               DV6_ADV_OTH_CFG - Set the Other Stateful 
*                                                 Configuration flag in 
*                                                 outgoing Router 
*                                                 Advertisement messages.
*
*       flag_opts               Instructions on what to do with the
*                               flags specified:
*
*                               DV6_REM_FLGS    Remove the indicated flags 
*                                               from the interface.  
*                               DV6_ADD_FLGS    Add the indicated flags 
*                                               to the interface.  
*                               DV6_IGN_FLGS    Leave  the flags on the 
*                                               interface unmodified.
*
*       *rtr_opts               A pointer to the parameters to configure:
*
*       rtr_MaxRtrAdvInterval   The maximum time allowed between sending 
*                               unsolicited multicast Router Advertisements 
*                               from the interface, in seconds.  MUST be no 
*                               less than 4 seconds and no greater than 
*                               1800 seconds.  A value of -1 causes the 
*                               default value to be used.  A value of -2
*                               leaves the current value unmodified.
*
*       rtr_MinRtrAdvInterval   The minimum time allowed between sending 
*                               unsolicited multicast Router Advertisements 
*                               from the interface, in seconds.  MUST be no 
*                               less than 3 seconds and no greater than 
*                               .75 * maxRtrInt.  A value of -1 causes the 
*                               default value to be used.  A value of -2
*                               leaves the current value unmodified.
*
*       rtr_AdvLinkMTU          The value to be placed in MTU options sent 
*                               by the router.  A value of zero indicates 
*                               that no MTU options are sent.  A value of
*                               -1 causes the default value to be used.  
*                               A value of -2 leaves the current value 
*                               unmodified.
*
*       rtr_AdvReachableTime    The value to be placed in the Reachable 
*                               Time field in the Router Advertisement 
*                               messages sent by the router.  The value 
*                               zero means unspecified (by this router).  
*                               MUST be no greater than 3,600,000 
*                               milliseconds (1 hour).  A value of -1 causes 
*                               the default value to be used.  A value of -2
*                               leaves the current value unmodified.
*
*       rtr_AdvCurHopLimit      The value to be placed in the Retrans 
*                               Timer field in the Router Advertisement 
*                               messages sent by the router.  The value 
*                               zero means unspecified (by this router).  
*                               A value of -1 causes the default value to 
*                               be used.  A value of -2 leaves the current 
*                               value unmodified.
*
*       rtr_AdvRetransTimer     The default value to be placed in the Cur 
*                               Hop Limit field in the Router Advertisement 
*                               messages sent by the router.  The value 
*                               zero means unspecified (by this router).  
*                               A value of -1 causes the default value to 
*                               be used.  A value of -2 leaves the current 
*                               value unmodified.
*
*       rtr_AdvDefaultLifetime  The value to be placed in the Router 
*                               Lifetime field of Router Advertisements 
*                               sent from the interface, in seconds.  A 
*                               value of zero indicates that the router 
*                               is not to be used as a default router.  
*                               MUST be either zero or between maxRtrInt 
*                               and 9000 seconds.  A value of -1 causes 
*                               the default value to be used.  A value of -2
*                               leaves the current value unmodified.
*
*       *rtr_AdvPrefixList      A pointer to a list of prefixes to add
*                               to the Prefix List for the device.  The
*                               final prefix in the list is signified by
*                               a prefix of all zero's.
*                                                 
*   OUTPUTS                                                               
*                                                                         
*       NU_SUCCESS              The router was successfully configured.
*       NU_INVALID_PARM         The device is invalid, rtr_opts is 
*                               NULL or the prefix length of a specified
*                               prefix is invalid.
*       NU_NO_MEMORY            Insufficient memory.
*                                                                         
*************************************************************************/
STATUS NU_Configure_Router(UINT32 dev_index, UINT32 flags,
                           INT flag_opts, const DEV6_RTR_OPTS *rtr_opts)
{
    STATUS          status;
    DV_DEVICE_ENTRY *dev_ptr;

    NU_SUPERV_USER_VARIABLES

    /* Validate the rtr_opts pointer */
    if (rtr_opts == NU_NULL)
        return (NU_INVALID_PARM);

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

     /* Get the Nucleus NET semaphore. */
    status =  NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status != NU_SUCCESS)
    {
        /* Return to user mode */
        NU_USER_MODE();

        return (status);
    }

    /* Get a pointer to the device */
    dev_ptr = DEV_Get_Dev_By_Index(dev_index);

    /* Validate the interface */
    if (dev_ptr != NU_NULL)
    {
        /* Configure the router parameters */
        DEV6_Configure_Router(dev_ptr, flags, flag_opts, rtr_opts);

        /* Add the Prefix List entries */
        if (rtr_opts->rtr_AdvPrefixList)
        {
            status = 
                DEV6_Configure_Prefix_List(dev_ptr, rtr_opts->rtr_AdvPrefixList);
        }
    }

    else
        status = NU_INVALID_PARM;

    /* Release the TCP semaphore */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE, 
                       __FILE__, __LINE__);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* NU_Configure_Router */
