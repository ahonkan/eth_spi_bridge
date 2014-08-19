/*************************************************************************
*
*               Copyright 2010 Mentor Graphics Corporation
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
*       tftp_shell.c
*
*   COMPONENT
*
*       TFTP
*
*   DESCRIPTION
*
*       This file contains functionality for adding TFTP commands
*       to a command shell
*
*   FUNCTIONS
*
*       command_tftps_dir
*       nu_os_net_prot_tftp_shell_init
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
#include "os/services/init/inc/runlevel.h"
#include <string.h>
#include <stdio.h>

/* Local functions Definitions */

/*************************************************************************
*
*   FUNCTION
*
*       command_tftps
*
*   DESCRIPTION
*
*       Function to perform a 'tftps' command (various TFTP Server functions)
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
static STATUS command_tftps(NU_SHELL *   p_shell,
                            INT          argc,
                            CHAR **      argv)
{
    STATUS      status;
    CHAR        tftps_dir[] = "A:\\";
    CHAR        temp_buf[32];


    /* Determine if wrong number of parameters passed-in */
    if (argc != 1)
    {
        /* Output error and format requirements */
        NU_Shell_Puts(p_shell, "\r\nERROR: Invalid Usage!\r\n");
        NU_Shell_Puts(p_shell, "Format: tftps <operation>\r\n");
        NU_Shell_Puts(p_shell, "Where: operation = dir, start, or stop");
    }
    else
    {
        /* Check for 'dir' */
        if (strcmp(argv[0],"dir") == 0)
        {
            /* Replace drive with configured drive */
            tftps_dir[0] = 'A' + CFG_NU_OS_NET_PROT_TFTP_SERVER_DEFAULT_DRIVE;

            /* Print default drive */
            NU_Shell_Puts(p_shell, "TFTP Server Directory = ");
            NU_Shell_Puts(p_shell, tftps_dir);
        }
        else if (strcmp(argv[0],"stop") == 0)
        {
            /* Determine if server is already stopped */
            if (TFTP_Is_Server_Running() == NU_FALSE)
            {
                /* Print message that server is already stopped */
                NU_Shell_Puts(p_shell, "ERROR: TFTP Server Already Shut Down!");
            }
            else
            {
                /* Print message that server is being stopped */
                NU_Shell_Puts(p_shell, "Shutting down TFTP Server...\r\n");

                /* Stopping TFTP Server */
                status = NU_RunLevel_Component_Control("/nu/os/net/prot/tftp/server", RUNLEVEL_STOP);

                /* See if server stopped */
                if (status == NU_SUCCESS)
                {
                    /* Print message that server is stopped */
                    NU_Shell_Puts(p_shell, "TFTP Server Shut Down Complete!");
                }
                else
                {
                    /* Get error number */
                    sprintf(temp_buf," (error = %d)",status);

                    /* Print message that server didn't stop */
                    NU_Shell_Puts(p_shell, "ERROR: TFTP Server Shut Down Error");
                    NU_Shell_Puts(p_shell, temp_buf);
                }
            }
        }
        else if (strcmp(argv[0],"start") == 0)
        {
            /* Determine if server is already running */
            if (TFTP_Is_Server_Running() == NU_TRUE)
            {
                /* Print message that server is already running */
                NU_Shell_Puts(p_shell, "ERROR: TFTP Server Already Running!");
            }
            else
            {
                /* Print message that server is being started */
                NU_Shell_Puts(p_shell, "Starting TFTP Server...\r\n");

                /* Starting TFTP Server */
                status = NU_RunLevel_Component_Control("/nu/os/net/prot/tftp/server", RUNLEVEL_START);

                /* See if server stopped */
                if (status == NU_SUCCESS)
                {
                    /* Print message that server is stopped */
                    NU_Shell_Puts(p_shell, "TFTP Server Running!");
                }
                else
                {
                    /* Get error number */
                    sprintf(temp_buf," (error = %d)",status);

                    /* Print message that server didn't start */
                    NU_Shell_Puts(p_shell, "ERROR: TFTP Server Start Error");
                    NU_Shell_Puts(p_shell, temp_buf);
                }
            }
        }
        else
        {
            /* Output error and format requirements */
            NU_Shell_Puts(p_shell, "\r\nERROR: Invalid Usage!\r\n");
            NU_Shell_Puts(p_shell, "Format: tftps <operation>\r\n");
            NU_Shell_Puts(p_shell, "Where: operation = dir, start, or stop");
        }
    }

    /* Carriage return and 2 x line-feed before going back to command shell */
    NU_Shell_Puts(p_shell, "\r\n\n");

    /* Return success to caller */
    return (NU_SUCCESS);
}


/*************************************************************************
*
*   FUNCTION
*
*       nu_os_net_prot_tftp_shell_init
*
*   DESCRIPTION
*
*       This function is called by the Nucleus OS run-level system to
*       initialize or terminate the TFTP shell component
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
STATUS nu_os_net_prot_tftp_shell_init (CHAR *   path, INT cmd)
{
    STATUS  status = NU_SUCCESS;


    /* Determine how to proceed based on the control command. */
    switch (cmd)
    {
        case RUNLEVEL_STOP :
        {
            /* Unregister 'tftps' command with all active shell sessions */
            status = NU_Unregister_Command(NU_NULL, "tftps");

            break;
        }

        case RUNLEVEL_START :
        {
            /* Register 'tftps' command with all active shell sessions */
            status = NU_Register_Command(NU_NULL, "tftps", command_tftps);

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
