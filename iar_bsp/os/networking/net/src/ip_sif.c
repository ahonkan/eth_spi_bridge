/*************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
* FILE NAME
*
*       ip_sif.c
*
* DESCRIPTION
*
*       This file contains the implementation of NU_Set_IP_Forwarding.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_Set_IP_Forwarding
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/************************************************************************
*
*   FUNCTION
*
*       NU_Set_IP_Forwarding
*
*   DESCRIPTION
*
*       This function turns IP forwarding on or off.
*
*   INPUTS
*
*       forwarding              The new state of IP_Forwarding.
*
*   OUTPUTS
*
*       NU_SUCCESS              The action was completed successfully.
*       NU_INVAL                Either forwarding is not a valid value
*                               or IP Forwarding is not enabled in
*                               net_cfg.h.
*
*************************************************************************/
STATUS NU_Set_IP_Forwarding(UINT8 forwarding)
{
    STATUS          status;

#if (INCLUDE_IP_FORWARDING == NU_TRUE)

#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV6_ROUTER_SUPPORT == NU_TRUE) )
    DV_DEVICE_ENTRY *dev_ptr;
#endif

    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();

    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status != NU_SUCCESS)
    {
        NU_USER_MODE();
        return (status);
    }

    if ( (forwarding == NU_TRUE) || (forwarding == NU_FALSE) )
    {
#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_IPV6_ROUTER_SUPPORT == NU_TRUE) )

        dev_ptr = DEV_Table.dv_head;

        while (dev_ptr)
        {
            /* RFC 4861 - section 6.2.5 - If system management disables a
             * router's IP forwarding capability, subsequent Router
             * Advertisements MUST set the Router Lifetime field to zero.
             * If IP forwarding is being enabled, set the Router Lifetime
             * to the default.
             */
            if (dev_ptr->dev_flags & DV6_ISROUTER)
            {
                /* Set the Router Lifetime field to zero so hosts
                 * will not use this node to forward packets.
                 */
                if (forwarding < IP_Forwarding)
                    dev_ptr->dev6_AdvDefaultLifetime = 0;

                /* Set the Router Lifetime field to a valid value
                 * so hosts will use this node to forward packets.
                 */
                else if (forwarding > IP_Forwarding)
                    dev_ptr->dev6_AdvDefaultLifetime =
                        IP6_DEFAULT_ADVERT_DEFAULT_LIFETIME;
            }

            /* Get a pointer to the next interface */
            dev_ptr = dev_ptr->dev_next;
        }
#endif

        IP_Forwarding = forwarding;
    }
    else
        status = NU_INVAL;

    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

    NU_USER_MODE();

#else
    UNUSED_PARAMETER(forwarding);

    status = NU_INVAL;
#endif

    return (status);

} /* NU_Set_IP_Forwarding */
