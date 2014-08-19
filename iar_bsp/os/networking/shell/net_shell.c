/*************************************************************************
*
*               Copyright 2013 Mentor Graphics Corporation
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
*       net_shell.c
*
*   COMPONENT
*
*       Networking
*
*   DESCRIPTION
*
*       This file contains functionality for adding networking commands
*       to a command shell
*
*   FUNCTIONS
*
*       command_ipconfig
*       nu_os_net_shell_init
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       nu_services.h
*       nu_networking.h
*       <string.h>
*       <stdio.h>
*
*************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "networking/nu_networking.h"
#include <string.h>
#include <stdio.h>


/*************************************************************************
*
*   FUNCTION
*
*       command_ipconfig
*
*   DESCRIPTION
*
*       Function to perform an 'ipconfig' command (IP configuration)
*
*   INPUTS
*
*       p_shell - Shell session handle
*       argc - number of arguments
*       argv - pointer to array of arguments
*
*   OUTPUTS
*
*       NU_SUCCESS
*
*************************************************************************/
static STATUS command_ipconfig(NU_SHELL *   p_shell,
                               INT          argc,
                               CHAR **      argv)
{
    struct if_nameindex *   if_index;
    struct if_nameindex *   if_index_orig;
    NU_IOCTL_OPTION         ioctl_opt;
    DV_DEVICE_ENTRY *       dev;
    ROUTE_NODE *            default_route;
    RTAB4_ROUTE_ENTRY *     default_gateway;
    CHAR                    gateway_ip_addr[4];
    STATUS                  status;
    CHAR                    buf[100];


    /* Determine if too many parameters passed-in */
    if (argc != 0)
    {
        /* Output error and format requirements */
        NU_Shell_Puts(p_shell, "\r\nERROR: Invalid Usage!\r\n");
        NU_Shell_Puts(p_shell, "Format: ipconfig\r\n");
    }
    else
    {
        /* Get pointer to list of interfaces */
        if_index_orig = if_index = NU_IF_NameIndex();

        /* Loop through interfaces */
        while ((if_index != NU_NULL) && (if_index->if_name != NU_NULL))
        {
            /* Output interface name */
            NU_Shell_Puts(p_shell,"\r\n");
            NU_Shell_Puts(p_shell, if_index->if_name);
            NU_Shell_Puts(p_shell,"\r\n");

            /* Get interface status */
            status = NU_Device_Up(if_index->if_name);

            /* Obtain the TCP semaphore to protect the stack global variables */
            (VOID)NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

            /* Get a pointer to the device */
            dev = DEV_Get_Dev_By_Name(if_index->if_name);

            /* Release the semaphore. */
            (VOID)NU_Release_Semaphore(&TCP_Resource);

            /* Put the interface hardware address in buffer */
            sprintf(buf, "    Hardware Address: %02x:%02x:%02x:%02x:%02x:%02x\r\n",dev->dev_mac_addr[0],
                                                                                   dev->dev_mac_addr[1],
                                                                                   dev->dev_mac_addr[2],
                                                                                   dev->dev_mac_addr[3],
                                                                                   dev->dev_mac_addr[4],
                                                                                   dev->dev_mac_addr[5]);

            /* Output interface information based on whether it is up or down */
            if (status == NU_TRUE)
            {
                /* Show interface up and hardware address */
                NU_Shell_Puts(p_shell,"    Status:           UP\r\n");
                NU_Shell_Puts(p_shell, buf);

                /* Put the interface name in the IOCTL option structure member */
                ioctl_opt.s_optval = (UINT8*)if_index->if_name;

                /* Call NU_Ioctl to get the IPv6 address */
                status = NU_Ioctl(SIOCGIFADDR_IN6, &ioctl_opt, sizeof(ioctl_opt));

                /* Check if we got the IPv6 address */
                if (status == NU_SUCCESS)
                {
                    /* Output the IPv6 address */
                    sprintf(buf, "    IPv6 Address:     %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\r\n",
                                                                          ioctl_opt.s_ret.s_ipaddr[0],
                                                                          ioctl_opt.s_ret.s_ipaddr[1],
                                                                          ioctl_opt.s_ret.s_ipaddr[2],
                                                                          ioctl_opt.s_ret.s_ipaddr[3],
                                                                          ioctl_opt.s_ret.s_ipaddr[4],
                                                                          ioctl_opt.s_ret.s_ipaddr[5],
                                                                          ioctl_opt.s_ret.s_ipaddr[6],
                                                                          ioctl_opt.s_ret.s_ipaddr[7],
                                                                          ioctl_opt.s_ret.s_ipaddr[8],
                                                                          ioctl_opt.s_ret.s_ipaddr[9],
                                                                          ioctl_opt.s_ret.s_ipaddr[10],
                                                                          ioctl_opt.s_ret.s_ipaddr[11],
                                                                          ioctl_opt.s_ret.s_ipaddr[12],
                                                                          ioctl_opt.s_ret.s_ipaddr[13],
                                                                          ioctl_opt.s_ret.s_ipaddr[14],
                                                                          ioctl_opt.s_ret.s_ipaddr[15]);
                    NU_Shell_Puts(p_shell, buf);
                }
                else
                {
                    /* Output that the IPv6 address is unavailable */
                    NU_Shell_Puts(p_shell, "    IPv6 Address:     Unavailable\r\n");
                }

                /* Call NU_Ioctl to get the IPv4 address. */
                status = NU_Ioctl(SIOCGIFADDR, &ioctl_opt, sizeof(ioctl_opt));

                /* Check if we got the IPv4 address */
                if (status == NU_SUCCESS)
                {
                    /* Output the IPv4 address */
                    sprintf(buf, "    IPv4 Address:     %d.%d.%d.%d\r\n", ioctl_opt.s_ret.s_ipaddr[0],
                                                                          ioctl_opt.s_ret.s_ipaddr[1],
                                                                          ioctl_opt.s_ret.s_ipaddr[2],
                                                                          ioctl_opt.s_ret.s_ipaddr[3]);
                    NU_Shell_Puts(p_shell, buf);
                }
                else
                {
                    /* Output that IPv4 address is unavailable */
                    NU_Shell_Puts(p_shell, "    IPv4 Address:     Unavailable\r\n");
                }

                /* Call NU_Ioctl to get the IP mask */
                status = NU_Ioctl(SIOCGIFNETMASK, &ioctl_opt, sizeof(ioctl_opt));

                /* Check if we got the IP mask */
                if (status == NU_SUCCESS)
                {
                    /* Output the net mask */
                    sprintf(buf, "    Subnet Mask:      %d.%d.%d.%d\r\n", ioctl_opt.s_ret.s_ipaddr[0],
                                                                          ioctl_opt.s_ret.s_ipaddr[1],
                                                                          ioctl_opt.s_ret.s_ipaddr[2],
                                                                          ioctl_opt.s_ret.s_ipaddr[3]);
                    NU_Shell_Puts(p_shell, buf);
                }
                else
                {
                    /* Output that the mask is unavailable */
                    NU_Shell_Puts(p_shell, "    Mask:             Unavailable\r\n");
                }

                /* Get the Default route */
                default_route = NU_Get_Default_Route(NU_FAMILY_IP);

                /* Check if we got the Default route */
                if (default_route != NU_NULL)
                {
                    /* Get a pointer to the Default Gateway address */
                    default_gateway = (RTAB4_ROUTE_ENTRY*)default_route->rt_route_entry_list.rt_entry_head;

                    /* Convert gateway to IP address format */
                    PUT32(&gateway_ip_addr[0], 0, default_gateway->rt_gateway_v4.sck_addr);

                    /* Output the Default Gateway address */
                    sprintf(buf, "    Default Gateway:  %d.%d.%d.%d\r\n", gateway_ip_addr[0],
                                                                          gateway_ip_addr[1],
                                                                          gateway_ip_addr[2],
                                                                          gateway_ip_addr[3]);
                    NU_Shell_Puts(p_shell, buf);
                }
                else
                {
                    /* Output that the Default Gateway address is unavailable */
                    NU_Shell_Puts(p_shell, "    Default Gateway:  Unavailable\r\n");
                }
            }
            else
            {
                /* Show the interface down and hardware address */
                NU_Shell_Puts(p_shell,"    Status:           DOWN\r\n");
                NU_Shell_Puts(p_shell, buf);
            }

            /* Go to next interface */
            if_index = (struct if_nameindex *)((CHAR *)if_index +
                        sizeof(struct if_nameindex) + DEV_NAME_LENGTH);

        }   /* while */

        /* Free name index memory */
        NU_IF_FreeNameIndex(if_index_orig);
    }

    /* Carriage return and line-feed before going back to command shell */
    NU_Shell_Puts(p_shell, "\r\n");

    /* Return success to caller */
    return (NU_SUCCESS);
}


/*************************************************************************
*
*   FUNCTION
*
*       nu_os_net_shell_init
*
*   DESCRIPTION
*
*       This function is called by the Nucleus OS run-level system to
*       initialize or terminate the net shell component
*
*   INPUTS
*
*       path - Path of the Nucleus OS registry entry for the Nucleus
*              Agent.
*
*       init_cmd - Run-level commmand.
*
*   OUTPUTS
*
*       NU_SUCCESS - Indicates successful operation.
*
*       <other> - Indicates (other) internal error occured.
*
*************************************************************************/
STATUS nu_os_net_shell_init (CHAR *   path, INT cmd)
{
    STATUS  status=NU_SUCCESS;


    /* Determine how to proceed based on the control command. */
    switch (cmd)
    {
        case RUNLEVEL_STOP :
        {
            /* ERROR: Shell service does not support shutdown. */

            /* ERROR RECOVERY: Report success and do nothing. */

            break;
        }

        case RUNLEVEL_START :
        {
            /* Register 'ipconfig' command with all active shell sessions */
            status = NU_Register_Command(NU_NULL, "ipconfig", command_ipconfig);
            
            break;
        }

        case RUNLEVEL_HIBERNATE :
        case RUNLEVEL_RESUME :
        {
            /* Nothing to do for hibernate operations. */

            break;
        }

        default :
        {
            /* ERROR: Unknown control command value. */

            /* ERROR RECOVERY: Report success and do nothing. */

            break;
        }
    }

    return (status);
}
