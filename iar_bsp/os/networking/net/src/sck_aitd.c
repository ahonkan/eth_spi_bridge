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
*
*   FILENAME
*
*       sck_aitd.c
*
*   DESCRIPTION
*
*       This file contains the API function call NU_Attach_IP_To_Device
*       to assign an IPv4 address to a device.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Attach_IP_To_Device
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/**************************************************************************
*
*   FUNCTION
*
*       NU_Attach_IP_To_Device
*
*   DESCRIPTION
*
*       Given a device name, this routine will set the IP number into the
*       network interface table for the device.
*
*   INPUTS
*
*       *name                   A pointer to an ASCII string representing
*                               the device
*       *ip_addr                A pointer to the IP address to be
*                               associated with the device
*       *subnet                 A pointer to the subnet mask to be
*                               associated with the device
*
*   OUTPUTS
*
*       NU_SUCCESS              Device was updated
*       NU_INVALID_PARM         One of the pointers passed in is NULL or
*                               there is no device in the system with the
*                               specified name.
*
****************************************************************************/
STATUS NU_Attach_IP_To_Device(const CHAR *name, UINT8 *ip_addr,
                              const UINT8 *subnet)
{
    STATUS          status;

#if (INCLUDE_LL_CONFIG == NU_TRUE)
    DV_DEVICE_ENTRY *device;
#endif

    NU_SUPERV_USER_VARIABLES

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    if ( (name == NU_NULL) || (ip_addr == NU_NULL) || (subnet == NU_NULL) )
        return (NU_INVALID_PARM);

#endif

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

     /* Get the Nucleus NET semaphore. */
    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status != NU_SUCCESS)
    {
        /* Return to user mode */
        NU_USER_MODE();

        return (status);
    }

#if (INCLUDE_LL_CONFIG == NU_TRUE)

    /* If the IP address being added is a link-local address, kick off
     * the IPv4 link-local configuration state machine.
     */
    if (IP_LL_ADDR(ip_addr))
    {
        /* Get a pointer to the interface. */
        device = DEV_Get_Dev_By_Name(name);

        /* If an interface was found and a link-local address is not already
         * configured on the interface.
         */
        if ( (device) && (device->dev_addr.dev_link_local_addr == 0) )
        {
            /* Set the flag that indicates IPv4 link-local address
             * configuration is enabled on this interface.
             */
            device->dev_flags |= DV_CFG_IPV4_LL_ADDR;

            /* Initiate IPv4 link-local address configuration. */
            DEV_Configure_Link_Local_Addr(device, ip_addr);
        }
    }

    /* Otherwise, add the IP address to the interface now. */
    else
#endif

    if (DEV_Attach_IP_To_Device(name, ip_addr, subnet) == NU_SUCCESS)
    {
        /* Cache the IP address.  In case of link UP / link DOWN, the IP
         * address will be restored.
         */
        status = NU_Ifconfig_Set_Ipv4_Address(name, ip_addr, subnet);
    }

    else
    {
        status = NU_INVALID_PARM;
    }

    /* Release the TCP semaphore */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* NU_Attach_IP_To_Device */
