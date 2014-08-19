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
*       proc_shell.c
*
*   COMPONENT
*
*       process
*
*   DESCRIPTION
*
*       This file contains functionality for adding nlm commands
*       to a command shell
*
*   DATA STRUCTURES
*
*       PROC_Shell_Tryload
*       PROC_Shell_Tryload_Status
*       PROC_Shell_Tryload_Session
*
*   FUNCTIONS
*
*       process_command_load
*       command_load
*       command_tryload
*       command_start
*       command_stop
*       command_unload
*       command_proclist
*       command_kill
*       nu_os_kern_process_shell_init
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       nu_services.h
*       nu_storage.h
*       proc_extern.h
*       proc_core.h
*       <stdio.h>
*       <stdlib.h>
*
*************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "storage/nu_storage.h"
#include "kernel/proc_extern.h"
#include "../core/proc_core.h"
#include <stdio.h>
#include <stdlib.h>

/* Global variables */
BOOLEAN     PROC_Shell_Tryload;
STATUS      PROC_Shell_Tryload_Status;
NU_SHELL *  PROC_Shell_Tryload_Session;

/* Macros for various command parameters */
#define LOAD_HEAP_PARAM     "--heap="
#define LOAD_STACK_PARAM    "--stack="
#define LOAD_ADDR_PARAM     "--address="
#define LOAD_KM_PARAM       "--km="
#define TRUE_PARAM_VAL      "T"
#define FALSE_PARAM_VAL     "F"


/*************************************************************************
*
*   FUNCTION
*
*       command_load
*
*   DESCRIPTION
*
*       Function to perform a 'load' command
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
static STATUS command_load(NU_SHELL *   p_shell,
                           INT          argc,
                           CHAR **      argv)
{
    STATUS              status = NU_SUCCESS;
    INT                 id;
    CHAR                buff[PROC_NAME_LENGTH];
    PROC_CB *           process;
    NU_LOAD_EXTENSION   load_ext;
    VOID *              load_addr = (VOID *)NU_LOAD_DYNAMIC;


    /* Determine if wrong number of parameters passed-in */
    if (argc == 0)
    {
        /* ERROR: Incorrect number of parameters passed in */
        status = NU_INVALID_OPTIONS;
    }
    else
    {
        /* Clear extensions structure - values of 0 in structure cause defaults to be used... */
        memset((VOID *)&load_ext, 0, sizeof(NU_LOAD_EXTENSION));

        /* Loop through all arguments besides argv[0] which is the filename
           NOTE: Multiple instances of a command-line option will result
                 in last instance being used */
        for (id = 1 ; (id < argc) && (status == NU_SUCCESS) ; id++)
        {
            /* Determine if this parameter contains heap setting */
            if (strncmp(argv[id],LOAD_HEAP_PARAM, strlen(LOAD_HEAP_PARAM)) == 0)
            {
                /* Convert heap size portion of parameter from string to integer */
                load_ext.heap_size = strtol((const CHAR *)&argv[id][strlen(LOAD_HEAP_PARAM)], NU_NULL, 0);
            }
            else if (strncmp(argv[id],LOAD_STACK_PARAM, strlen(LOAD_STACK_PARAM)) == 0)
            {
                /* Convert stack size portion of parameter string to integer */
                load_ext.stack_size = strtol((const CHAR *)&argv[id][strlen(LOAD_STACK_PARAM)], NU_NULL, 0);
            }
            else if (strncmp(argv[id], LOAD_ADDR_PARAM, strlen(LOAD_ADDR_PARAM)) == 0)
            {
                /* Convert load address portion of parameter from string to integer */
                load_addr = (VOID *)strtol((const CHAR *)&argv[id][strlen(LOAD_ADDR_PARAM)], NU_NULL, 0);

                /* Check for unsupported values */
                if (load_addr != NU_LOAD_DYNAMIC)
                {
                    /* ERROR: Feature not supported */
                    status = NU_UNAVAILABLE;
                }
            }
            else if (strncmp(argv[id], LOAD_KM_PARAM, strlen(LOAD_KM_PARAM)) == 0)
            {
                /* Convert kernel-mode portion of parameter from string to boolean */
                if (strncmp((const CHAR *)&argv[id][strlen(LOAD_KM_PARAM)], TRUE_PARAM_VAL, strlen(TRUE_PARAM_VAL)) == 0)
                {
                    load_ext.kernel_mode = NU_TRUE;
                }
                else if (strncmp((const CHAR *)&argv[id][strlen(LOAD_KM_PARAM)], FALSE_PARAM_VAL, strlen(FALSE_PARAM_VAL)) == 0)
                {
                    load_ext.kernel_mode = NU_FALSE;
                }
                else
                {
                    /* ERROR: Invalid kernel-mode parameter value */
                    status = NU_INVALID_OPTIONS;
                }

            }
            else
            {
                /* ERROR: Invalid parameter */
                status = NU_INVALID_OPTIONS;
            }
        }

        /* Ensure status still success */
        if (status == NU_SUCCESS)
        {
            /* Load the image */
            status = NU_Load(argv[0], &id, load_addr, (VOID *)&load_ext, NU_SUSPEND);
        }

        /* Ensure load succeeded */
        if (status == NU_SUCCESS)
        {
            /* Obtain the process control block */
            process = PROC_Get_Pointer(id);
            if (process == NU_NULL)
            {
                /* ERROR: Loaded process invalid */
                status = NU_INVALID_PROCESS;
            }
        }
    }

    /* Execute based on return value */
    switch (status)
    {
        case    NU_SUCCESS:

            /* Print ID */
            sprintf(&buff[0], "PID: %d\r\n", (UINT)id);
            NU_Shell_Puts(p_shell, buff);

            /* Print load address */
            sprintf(&buff[0], "Load Address: 0x%08x\r\n", (UINT)process -> load_addr);
            NU_Shell_Puts(p_shell, buff);

            /* Print entry address */
            sprintf(&buff[0], "Entry Point: 0x%08x\r\n", (UINT)process -> entry_addr);
            NU_Shell_Puts(p_shell, buff);

            /* Print kernel-mode status */
            if (process -> kernel_mode == NU_TRUE)
            {
                NU_Shell_Puts(p_shell, "Kernel-Mode: TRUE\r\n");
            }
            else
            {
                NU_Shell_Puts(p_shell, "Kernel-Mode: FALSE\r\n");
            }

            break;

        case    NUF_NOFILE:

            /* Print file not found error */
            NU_Shell_Puts(p_shell, "ERROR: File not found.\r\n");

            break;

        case    NU_INVALID_ENTRY:

        	/* Print duplicate symbols found error */
        	NU_Shell_Puts(p_shell, "ERROR: Duplicate symbols found.\r\n");

        	break;

        case    NU_INVALID_OPTIONS:

            /* Print invalid usage error and usage information */
            NU_Shell_Puts(p_shell, "\r\nERROR: Invalid Usage!\r\n");
            NU_Shell_Puts(p_shell, "Format: load|tryload <name> --address=<load address> --heap=<heap size> --stack=<stack size> --km=<T|F>\r\n");

            break;

        case    NU_UNAVAILABLE:

            /* Print feature not supported error */
            NU_Shell_Puts(p_shell, "ERROR: Feature not supported.\r\n");

            break;

        case    NU_NOT_PRESENT:

            /* Print unresolved symbols found error */
            NU_Shell_Puts(p_shell, "ERROR: Unresolved symbols found.\r\n");

            break;
            
        default:

            /* Print general error */
            sprintf(&buff[0], "ERROR: Image load error (error = %d)\r\n", (UINT)status);
            NU_Shell_Puts(p_shell, buff);

            break;
    }

    /* Carriage return and line-feed before going back to command shell */
    NU_Shell_Puts(p_shell, "\r\n");

    /* Return success to caller */
    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       command_tryload
*
*   DESCRIPTION
*
*       Function to perform a 'tryload' command
*
*       NOTE:  This command uses global variables and is NOT
*              re-entrant - this is meant as a debugging tool only.
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
static STATUS command_tryload(NU_SHELL *   p_shell,
                              INT          argc,
                              CHAR **      argv)
{
    STATUS              status = NU_SUCCESS;

    /* Set up global variables used by this command */
    PROC_Shell_Tryload = NU_TRUE;
    PROC_Shell_Tryload_Status = NU_SUCCESS;
    PROC_Shell_Tryload_Session = p_shell;

    /* Call the load command function */
    status = command_load(p_shell, argc, argv);

    /* Reset necessary global used by this command */
    PROC_Shell_Tryload = NU_FALSE;

    /* Return success to caller */
    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       command_start
*
*   DESCRIPTION
*
*       Function to perform a 'start' command
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
static STATUS command_start(NU_SHELL *   p_shell,
                            INT          argc,
                            CHAR **      argv)
{
    STATUS  status;
    INT     id;
    CHAR *  name;
    CHAR    buff[PROC_NAME_LENGTH];


    /* Determine if wrong number of parameters passed-in */
    if (argc != 1)
    {
        /* Output error and format requirements */
        NU_Shell_Puts(p_shell, "\r\nERROR: Invalid Usage!\r\n");
        NU_Shell_Puts(p_shell, "Format: start <pid>\r\n");
    }
    else
    {
        /* Convert parameter to a number */
        id = strtol(argv[0], NU_NULL, 10);

        /* Start the process */
        status = NU_Start(id, NU_NULL, NU_SUSPEND);

        /* Execute based on status */
        switch (status)
        {
            case    NU_SUCCESS:

                /* Get name of process */
                (VOID)PROC_Get_Name(id, &name);

                /* Show process started */
                sprintf(&buff[0], "%s started\r\n", name);
                NU_Shell_Puts(p_shell, buff);

                break;

            default:

                /* Print general error */
                sprintf(&buff[0], "ERROR: Process start error (error = %d)\r\n", (UINT)status);
                NU_Shell_Puts(p_shell, buff);

                break;
        }
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
*       command_stop
*
*   DESCRIPTION
*
*       Function to perform a 'stop' command
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
static STATUS command_stop(NU_SHELL *   p_shell,
                           INT          argc,
                           CHAR **      argv)
{
    STATUS  status;
    INT     id;
    CHAR    buff[PROC_NAME_LENGTH];
    CHAR *  name;


    /* Determine if wrong number of parameters passed-in */
    if (argc != 1)
    {
        /* Output error and format requirements */
        NU_Shell_Puts(p_shell, "\r\nERROR: Invalid Usage!\r\n");
        NU_Shell_Puts(p_shell, "Format: stop <pid>\r\n");
    }
    else
    {
        /* Convert parameter to a number */
        id = strtol(argv[0], NU_NULL, 10);

        /* Stop the process */
        status = NU_Stop(id, EXIT_STOP, NU_SUSPEND);

        /* Execute based on status */
        switch (status)
        {
            case    NU_SUCCESS:

                /* Get name of process */
                (VOID)PROC_Get_Name(id, &name);

                /* Show process stopped */
                sprintf(&buff[0], "%s stopped\r\n", name);

                NU_Shell_Puts(p_shell, buff);

                break;

            default:

                /* Print general error */
                sprintf(&buff[0], "ERROR: Process stop error (error = %d)\r\n", (UINT)status);
                NU_Shell_Puts(p_shell, buff);

                break;
        }
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
*       command_unload
*
*   DESCRIPTION
*
*       Function to perform a 'unload' command
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
static STATUS command_unload(NU_SHELL *   p_shell,
                             INT          argc,
                             CHAR **      argv)
{
    STATUS  status;
    INT     id;
    CHAR    buff[PROC_NAME_LENGTH];
    CHAR *  name;


    /* Determine if wrong number of parameters passed-in */
    if (argc != 1)
    {
        /* Output error and format requirements */
        NU_Shell_Puts(p_shell, "\r\nERROR: Invalid Usage!\r\n");
        NU_Shell_Puts(p_shell, "Format: unload <pid>\r\n");
    }
    else
    {
        /* Convert parameter to a number */
        id = strtol(argv[0], NU_NULL, 10);

        /* Get name of process - this must be done before the process is unloaded */
        (VOID)PROC_Get_Name(id, &name);

        /* Unload the image */
        status = NU_Unload(id, NU_SUSPEND);

        /* Execute based on status */
        switch (status)
        {
            case    NU_SUCCESS:

                /* Show process unloaded */
                sprintf(&buff[0], "%s unloaded\r\n", name);
                NU_Shell_Puts(p_shell, buff);

                break;

            default:

                /* Print general error */
                sprintf(&buff[0], "ERROR: Process unload error (error = %d)\r\n", (UINT)status);
                NU_Shell_Puts(p_shell, buff);

                break;
        }
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
*       command_proclist
*
*   DESCRIPTION
*
*       Function to perform the 'proclist' command
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
static STATUS command_proclist(NU_SHELL *   p_shell,
                               INT          argc,
                               CHAR **      argv)
{
    CHAR                buff[256];
    UNSIGNED            num_procs, count;
    NU_PROCESS_INFO *   info_array;
    static const CHAR * state_string[] = {"Created","Loading","Starting","Deiniting","Stopping","Unloading","Killing","Stopped","Started","Failed"};
    CHAR *              format_string;
    STATUS              status = NU_SUCCESS;


    /* Determine if wrong number of parameters passed-in */
    if (argc > 1)
    {
        /* Output error and format requirements */
        NU_Shell_Puts(p_shell, "\r\nERROR: Invalid Usage!\r\n");
        NU_Shell_Puts(p_shell, "Format: proclist [--delimited]\r\n");
    }
    else
    {
        /* Check if 1 parameter */
        if (argc == 1)
        {
            /* Ensure parameter is valid */
            if (strncmp(argv[0], "--delimited", 11) == 0)
            {
                /* Output delimited process list header */
                NU_Shell_Puts(p_shell, "<PID(integer)><Name(string)><State(string)><Load Address(integer)><Entry Point(integer)><Exit Code(integer)><Kernel-Mode(string)>\r\n");

                /* Set format string */
                format_string = "<%d><%s><%s><0x%08x><0x%08x><%d><%s>\r\n";
            }
            else
            {
                /* Output error and format requirements */
                NU_Shell_Puts(p_shell, "\r\nERROR: Invalid Usage!\r\n");
                NU_Shell_Puts(p_shell, "Format: proclist [--delimited]\r\n");

                /* Set format string to NULL */
                format_string = NU_NULL;
            }
        }
        else
        {
            /* Output human readable process list header */
            NU_Shell_Puts(p_shell, "PID  Name                                     State      Load Address Entry Point Exit Code   Kernel-Mode\r\n");
            NU_Shell_Puts(p_shell, "===  ====                                     =====      ============ =========== =========   ===========\r\n");

            /* Set format string */
            format_string = "%-4d %-40s %-10s 0x%08x   0x%08x  %-11d %s\r\n";
        }

        /* Check if a valid format string set */
        if (format_string != NU_NULL)
        {
            /* Get number of processes in system */
            num_procs = NU_Established_Processes();

            /* Alloc memory for the process info */
            status = PROC_Alloc((VOID **)&info_array, num_procs * sizeof(NU_PROCESS_INFO), 4);

            /* Ensure allocation successful */
            if (status == NU_SUCCESS)
            {
                /* Get the processes information */
                status = NU_Processes_Information(&num_procs, info_array);

                /* Ensure process information retrieved */
                if (status == NU_SUCCESS)
                {
                    /* Loop through and print out process info */
                    for (count = 0; count < num_procs; count++)
                    {
                        /* Create the process data string for this process */
                        sprintf(&buff[0], format_string, (UINT)info_array[count].pid,
                                                          info_array[count].name,
                                                          state_string[info_array[count].state],
                                                          (UINT)info_array[count].load_addr,
                                                          (UINT)info_array[count].entry_addr,
                                                          info_array[count].exit_code,
                                                          (info_array[count].kernel_mode?"TRUE":"FALSE"));

                        /* Print process information to the command shell */
                        NU_Shell_Puts(p_shell, buff);
                    }
                }

                /* Free memory */
                (VOID)PROC_Free((VOID *)info_array);
            }
        }
    }

    /* Carriage return and line-feed before going back to command shell */
    NU_Shell_Puts(p_shell, "\r\n");

    /* Return success to caller */
    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       command_kill
*
*   DESCRIPTION
*
*       Function to perform a 'kill' command
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
static STATUS command_kill(NU_SHELL *   p_shell,
                           INT          argc,
                           CHAR **      argv)
{
    STATUS  status;
    INT     id;
    CHAR    buff[PROC_NAME_LENGTH];
    CHAR *  name;


    /* Determine if wrong number of parameters passed-in */
    if (argc != 1)
    {
        /* Output error and format requirements */
        NU_Shell_Puts(p_shell, "\r\nERROR: Invalid Usage!\r\n");
        NU_Shell_Puts(p_shell, "Format: kill <pid>\r\n");
    }
    else
    {
        /* Convert parameter to a number */
        id = strtol(argv[0], NU_NULL, 10);

        /* Get name of process - must be done before it is killed / unloaded */
        (VOID)PROC_Get_Name(id, &name);

        /* Kill the process */
        status = NU_Kill(id, NU_SUSPEND);

        /* Execute based on status */
        switch (status)
        {
            case    NU_SUCCESS:

                /* Show process killed */
                sprintf(&buff[0], "%s killed\r\n", name);
                NU_Shell_Puts(p_shell, buff);

                break;

            default:

                /* Print general error */
                sprintf(&buff[0], "ERROR: Process kill error (error = %d)\r\n", (UINT)status);
                NU_Shell_Puts(p_shell, buff);

                break;
        }
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
*       nu_os_kern_process_shell_init
*
*   DESCRIPTION
*
*       This function is called by the Nucleus OS run-level system to
*       initialize or terminate the process shell component
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
STATUS nu_os_kern_process_shell_init (CHAR *   path, INT cmd)
{
    STATUS  status;


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
            /* Register commands with all active shell sessions */
            status = NU_Register_Command(NU_NULL, "load",   command_load);

            /* If successful, continue registering commands... */
            if (status == NU_SUCCESS)
            {
                status = NU_Register_Command(NU_NULL, "start",    command_start);
            }

            /* If successful, continue registering commands... */
            if (status == NU_SUCCESS)
            {
                status = NU_Register_Command(NU_NULL, "stop",   command_stop);
            }

            /* If successful, continue registering commands... */
            if (status == NU_SUCCESS)
            {
                status = NU_Register_Command(NU_NULL, "unload", command_unload);
            }

            /* If successful, continue registering commands... */
            if (status == NU_SUCCESS)
            {
                status = NU_Register_Command(NU_NULL, "proclist", command_proclist);
            }

            /* If successful, continue registering commands... */
            if (status == NU_SUCCESS)
            {
                status = NU_Register_Command(NU_NULL, "kill", command_kill);
            }

            /* If successful, continue registering commands... */
            if (status == NU_SUCCESS)
            {
                status = NU_Register_Command(NU_NULL, "tryload", command_tryload);
            }

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

