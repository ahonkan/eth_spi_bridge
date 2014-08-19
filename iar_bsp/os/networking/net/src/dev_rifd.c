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
*   FILE NAME
*
*       dev_rifd.c
*
*   DESCRIPTION
*
*       This file contains the implementation of NU_Remove_IP_From_Device
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Remove_IP_From_Device
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "services/nu_trace_os_mark.h"

/*************************************************************************
*
*   FUNCTION
*
*       NU_Remove_IP_From_Device
*
*   DESCRIPTION
*
*       This function removes the specified address from the device.
*       If the specified address is NULL, all addresses are removed
*       from the device and the device is marked as down.
*
*   INPUTS
*
*       *name                   A pointer to the name of the device.
*       *ip_addr                A pointer to the address to remove.
*       ip_family               The family of the IP address to remove.
*
*   OUTPUTS
*
*       NU_SUCCESS              Success
*       NU_INVALID_PARM         The device does not exist in the system.
*       NU_INVALID_ADDRESS      The IP address being removed was obtained
*                               via DHCP.  The function NU_Dhcp_Release
*                               must be called to delete an IP address
*                               obtained via DHCP.
*
*************************************************************************/
STATUS NU_Remove_IP_From_Device(const CHAR *name, const UINT8 *ip_addr,
                                INT16 ip_family)
{
    DV_DEVICE_ENTRY     *dev;
    STATUS              status;

#if (INCLUDE_IPV4 == NU_TRUE)
    DEV_IF_ADDR_ENTRY   *dev4_addr;
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
    DEV6_IF_ADDRESS *dev6_addr;
#endif

    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();

#if (INCLUDE_IPV6 == NU_FALSE)
    UNUSED_PARAMETER(ip_family);
#endif

    /* We must grab the NET semaphore */
    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status != NU_SUCCESS)
    {
        NU_USER_MODE();
        return (status);
    }

    /* Get the device using the IP Address */
    dev = DEV_Get_Dev_By_Name(name);

    if (dev == NU_NULL)
        status = NU_INVALID_PARM;

    else
    {
#if (INCLUDE_IPV6 == NU_TRUE)

        /* Remove the IPv6 address from the device */
        if (ip_family == NU_FAMILY_IP6)
        {
            /* Get a pointer to the address structure associated with
             * the address on the device.
             */
            dev6_addr = DEV6_Find_Target_Address(dev, ip_addr);

            /* If the address exists on the device, remove it from
             * the device.
             */
            if (dev6_addr)
            {
                /* Delete the IP address from the cache. */
                NU_Ifconfig_Delete_Ipv6_Address(dev->dev_net_if_name,
                                                dev6_addr->dev6_ip_addr,
                                                dev6_addr->dev6_prefix_length);

                /* Delete the address from the device */
                DEV6_Delete_IP_From_Device(dev6_addr);

                status = NU_SUCCESS;
            }

            /* Otherwise, the address does not exist on the device.
             * Return an error.
             */
            else
            {
                status = NU_INVALID_PARM;

                NLOG_Error_Log("IPv6 address does not exist on device",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }

#if (INCLUDE_IPV4 == NU_TRUE)
        else
#endif
#endif

#if (INCLUDE_IPV4 == NU_TRUE)
        {
            /* Get a pointer to the address structure associated with
             * the address on the device.
             */
            dev4_addr = DEV_Find_Target_Address(dev, IP_ADDR(ip_addr));

            /* If the address exists on the device, remove it from
             * the device.
             */
            if (dev4_addr)
            {
                /* Delete the IP address from the cache. */
                NU_Ifconfig_Delete_Ipv4_Address(dev->dev_net_if_name,
                                                dev4_addr->dev_entry_ip_addr,
                                                dev4_addr->dev_entry_netmask);

                status = DEV4_Delete_IP_From_Device(dev, dev4_addr);

#if (INCLUDE_LL_CONFIG == NU_TRUE)

                /* If the address being deleted is the link-local address,
                 * disable IPv4 link-local address configuration on the
                 * interface.
                 */
                if (IP_LL_ADDR(ip_addr))
                {
                    /* Remove the link-local address configuration flag. */
                    dev->dev_flags &= ~DV_CFG_IPV4_LL_ADDR;
                }
#endif
            }

            /* Otherwise, the address does not exist on the device.
             * Return an error.
             */
            else
            {
                status = NU_INVALID_PARM;

                NLOG_Error_Log("IPv6 address does not exist on device",
                               NERR_SEVERE, __FILE__, __LINE__);
            }
        }
#endif
    }

    DEV_Resume_All_Open_Sockets();

    /* Release the semaphore */
    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

    /* Trace log */
if (ip_family == NU_FAMILY_IP6)
{
    T_REMOVE_IP((char*)name, (UINT8*)ip_addr, ip_family, status, 16);
}
else
{
    T_REMOVE_IP((char*)name, (UINT8*)ip_addr, ip_family, status, 4);
}

    NU_USER_MODE();

    return (status);

} /* NU_Remove_IP_From_Device */
