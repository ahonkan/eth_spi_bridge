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
*       sck_io.c
*
*   DESCRIPTION
*
*       This file contains the implementation of NU_Ioctl.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Ioctl
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/externs6.h"
#endif

/*************************************************************************
*
*   FUNCTION
*
*       NU_Ioctl
*
*   DESCRIPTION
*
*       Performs special functions on an interface or other object.
*
*   INPUTS
*
*       optname                 Specifies an option as follows:
*
*               SIOCGIFADDR         Get the IP address associated with an
*                                   interface.
*               SIOCGIFDSTADDR      Get the IP address of the foreign side
*                                   of a PPP link. Only valid for PPP
*                                   links.
*               SIOCSPHYSADDR       Set the MAC address associated with
*                                   an interface.
*               SIOCSIFADDR         Set the IP address associated with an
*                                   interface.
*               SIOCGIFNETMASK      Get the subnet mask associated with
*                                   an IP address associated with an
*                                   interface.
*               SIOCSARP            Add or modify an entry in the ARP
*                                   cache.
*               SIOCDARP            Delete an entry from the ARP cache.
*               SIOCGARP            Get an entry from the ARP cache.
*               SIOCIFREQ           Issue the IOCTL command directly to
*                                   the device.
*               SIOCGIFADDR_IN6     Get the IPv6 address associated with
*                                   an interface.
*               SIOCGIFDSTADDR_IN6  Get the IPv6 address of the foreign
*                                   side of a PPP link.  Only valid for
*                                   PPP links.
*               SIOCLIFGETND        Get the MAC address associated with an
*                                   IPv6 address.
*               FIONREAD            Returns the number of bytes of data
*                                   pending on the socket to be read.
*               SIOCSETVLAN         Set the VLAN ID.
*               SIOCGETVLAN         Get the VLAN ID.
*               SIOCSETVLANPRIO     Set the VLAN priority.
*               SIOCGETVLANPRIO     Get the VLAN priority.
*               SIOCGHWCAP          Queries the communication controller
*                                   for the set of available hardware
*                                   offloading capabilities.
*               SIOCSHWOPTS         Set selected HW OFFLOADING options in
*                                   the controller.
*               SIOCICMPLIMIT       Set the rate-limiting interval for
*                                   outgoing ICMP error messages.
*
*       *option                 Return pointer for option status.
*       optlen                  Specifies the size in bytes of the
*                               location pointed to by option
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful completion of the service.
*       NU_INVAL                The value specified in the optlen
*                               parameter was not large enough to store
*                               the option status.
*       NU_INVALID_PARM         The device specified by the option
*                               parameter is invalid.
*       NU_INVALID_OPTION       The value specified in the optname
*                               parameter is invalid.
*       NU_INVALID_SOCKET       The socket passed in is invalid.
*
*************************************************************************/
STATUS NU_Ioctl(INT optname, SCK_IOCTL_OPTION *option, INT optlen)
{
    STATUS          status;

#if (INCLUDE_IPV4 == NU_TRUE)
    DV_DEVICE_ENTRY *dev_ptr;
#endif

    NU_SUPERV_USER_VARIABLES

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    /* If the user passed a NULL parameter, set status to error */
    if (option == NU_NULL)
        return (NU_INVALID_PARM);

#endif

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

    if (status != NU_SUCCESS)
    {
        /* Return to user mode */
        NU_USER_MODE();

        return (status);
    }

    switch (optname)
    {
#if (INCLUDE_IPV4 == NU_TRUE)

        /* Get the interface address */
        case SIOCGIFADDR :

            if (optlen >= (INT)sizeof(SCK_IOCTL_OPTION))
                status = Ioctl_SIOCGIFADDR(option);

            else
                status = NU_INVAL;

            break;

        /* Get the subnet mask associated with the IP address
         * for an interface.
         */
        case SIOCGIFNETMASK:

            if (optlen >= (INT)sizeof(SCK_IOCTL_OPTION))
                status = Ioctl_SIOCGIFNETMASK(option);

            else
                status = NU_INVAL;

            break;

        /* Get the point-to-point address */
        case SIOCGIFDSTADDR :

            if (optlen >= (INT)sizeof(SCK_IOCTL_OPTION))
                status = Ioctl_SIOCGIFDSTADDR(option);

            else
                status = NU_INVAL;

            break;

        /* Set the physical address by IP*/
        case SIOCSPHYSADDR:

            status = Ioctl_SIOCSPHYSADDR(option);
            break;

        /* Set the physical address by using device name */
        case SIOCSIFHWADDR:

            status = Ioctl_SIOCSIFHWADDR(option);
            break;

        /* Gets the physical address by using device name */
        case SIOCGIFHWADDR:

            status = Ioctl_SIOCGIFHWADDR(option);
            break;

        /* Set the interface address */
        case SIOCSIFADDR:

            status = Ioctl_SIOCSIFADDR(option);
            break;

#if (INCLUDE_ARP == NU_TRUE)

        /* Create or Modify an ARP entry */
        case SIOCSARP:

            status = Ioctl_SIOCSARP(option);
            break;

        /* Get an ARP entry */
        case SIOCGARP:

            status = Ioctl_SIOCGARP(option);
            break;

        /* Delete an ARP entry */
        case SIOCDARP:

            status = Ioctl_SIOCDARP(option);
            break;
#endif
#endif

        /* Issue the IOCTL command directly to the device */
        case SIOCIFREQ:

            status = Ioctl_SIOCIFREQ(option);
            break;

#if (INCLUDE_IPV6 == NU_TRUE)

        case SIOCGIFADDR_IN6:

            if (optlen < (INT)sizeof(SCK_IOCTL_OPTION))
            {
                status = NU_INVAL;
                break;
            }

            status = IP6_Ioctl_SIOCGIFADDR_IN6((CHAR*)option->s_optval,
                                               option->s_ret.s_ipaddr,
                                               &(option->s_optval_octet));

            break;

        case SIOCGIFDSTADDR_IN6:

            if (optlen < (INT)sizeof(SCK_IOCTL_OPTION))
            {
                status = NU_INVAL;
                break;
            }

            status = IP6_Ioctl_SIOCGIFDSTADDR_IN6((CHAR*)option->s_optval,
                                                  option->s_ret.s_ipaddr);

            break;

        case SIOCLIFGETND:

            if (optlen < (INT)sizeof(SCK_IOCTL_OPTION))
            {
                status = NU_INVAL;
                break;
            }

            status = IP6_Ioctl_SIOCLIFGETND((CHAR*)option->s_optval,
                                            option->s_ret.s_ipaddr,
                                            option->s_ret.mac_address);

            break;

#endif

#if ( (INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_VLAN == NU_TRUE) )

        case SIOCSETVLAN:

            status = Ioctl_SIOCSETVLAN(option);
            break;

        case SIOCGETVLAN:

            status = Ioctl_SIOCGETVLAN(option);
            break;

#endif

#if (INCLUDE_VLAN == NU_TRUE)

        case SIOCGETVLANPRIO:

            status = Ioctl_SIOCGETVLANPRIO(option);
            break;

        case SIOCSETVLANPRIO:

            status = Ioctl_SIOCSETVLANPRIO(option);
            break;

#endif

        case FIONREAD:

            status = Ioctl_FIONREAD(option);
            break;

#if (HARDWARE_OFFLOAD == NU_TRUE)

        case SIOCGHWCAP:

            /* Query the communication controller for set of hw
             * offloading features
             */
            if (optlen < (INT)sizeof(SCK_IOCTL_OPTION))
            {
                status = NU_INVAL;
                break;
            }

            status = Ioctl_SIOCGHWCAP(option);
            break;

        case SIOCSHWOPTS:

            /* Set or Clear device's HW Offloading features */
            if (optlen < (INT)sizeof(SCK_IOCTL_OPTION))
            {
                status = NU_INVAL;
                break;
            }

            status = Ioctl_SIOCSHWOPTS(option);
            break;
#endif

        case SIOCICMPLIMIT:

            /* Check the incoming data structure. */
            if (optlen < (INT)sizeof(SCK_IOCTL_OPTION))
            {
                status = NU_INVAL;
                break;
            }

            /* Set the interval */
            status = Ioctl_SIOCICMPLIMIT((CHAR*)option->s_optval,
                                         option->sck_max_msgs,
                                         option->sck_interval);
            break;

        default :

#if (INCLUDE_IPV4 == NU_TRUE)

            /* Find a matching device in the system. */
            dev_ptr = DEV_Get_Dev_By_Addr(option->s_ret.s_ipaddr);

            if (dev_ptr)
            {
                /* Issue the IOCTL call directly to the hardware. */
                status = (dev_ptr->dev_ioctl)(dev_ptr, optname,
                                              &option->s_ret.s_dvreq);
            }

            else
            {
                status = NU_INVALID_OPTION;
            }

#else
            status = NU_INVALID_OPTION;
#endif

            break;

    } /* end switch */

    if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
        NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* NU_Ioctl */
