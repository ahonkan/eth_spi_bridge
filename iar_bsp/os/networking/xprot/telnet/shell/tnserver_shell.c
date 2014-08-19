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
*       tnserver_shell.c
*
*   COMPONENT
*
*       Telnet
*
*   DESCRIPTION
*
*       This file contains functionality for utilizing the shell component
*       for a telnet server session
*
*   FUNCTIONS
*
*       TBD
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       nu_services.h
*
*************************************************************************/
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "networking/nu_networking.h"
#include "networking/nvt.h"
#include "networking/negotiat.h"
#include "tnserver_shell.h"
#include <string.h>


/* Local variables */
static CHAR tns_nego_table[] = {
/*  the command index,  the option  */
    DONT_WONT,          TO_BINARY,
    DONT_WONT,          TO_ECHO,      /* this side support ECHO */
    DO_WILL,            TO_SGA,       /* this side suppress Go Ahead */
    DO,                 TO_TERMTYPE,  /* require other side to tell terminal type */
    DO,                 TO_NAWS,      /* require other side to tell window size */
    END_OF_NEGO_TABLE                 /* NOTE: nego_table should always end with NULL,
                                         or NEG_Size_Of_Nego_Table() will no work right. */
};

static NU_SHELL *   telnet_shell_session;
static NU_TASK *    telnet_shell_task;
static INT          telnet_shell_socket;

/* Local functions Definitions */

/*************************************************************************
*
*   FUNCTION
*
*       tns_shell_thread_entry
*
*   DESCRIPTION
*
*       Task entry function.  This task waits for a telnet connection
*       and when one becomes available, creates a shell session,
*       puts the socket number into the shell structure, and self-suspends.
*       This task will be resumed when the shell session is shut-down.
*
*   INPUTS
*
*       p_shell - Shell session handle
*
*   OUTPUTS
*
*       NU_SUCCESS
*
*************************************************************************/
static VOID tns_shell_thread_entry(UNSIGNED       argc,
                                   VOID *         argv)
{
    STATUS              status = NU_SUCCESS;
    CHAR                server_ip[4] = {0, 0, 0, 0};
    INT                 socket;


    /* Wait until the NET stack is initialized and a networking driver
       is up and running. */
    status = NETBOOT_Wait_For_Network_Up(NU_SUSPEND);

    /* Ensure a networking interface is up */
    if (status == NU_SUCCESS)
    {
        /* Open telnet socket for any ip address. */
        socket = (INT16)(NU_Telnet_Socket(server_ip, "telnet_svr"));
        telnet_shell_socket = socket;

        /* Loop while a valid socket */
        while (telnet_shell_socket >= 0)
        {
            /* Accept the incoming connection and save socket */
            telnet_shell_socket = NU_Telnet_Server_Accept(socket);

            /* Ensure socket valid */
            if (telnet_shell_socket >= 0)
            {
                /* Initialize the data structure that contains all the
                   parameters of the Telnet server session. */
                status = NU_Telnet_Server_Init_Parameters(telnet_shell_socket);

                /* Ensure the last operation succeeded */
                if (status == NU_SUCCESS)
                {
                    /* Telnet server options negotiation */
                    status = NU_Telnet_Do_Negotiation(telnet_shell_socket, tns_nego_table);

                    /* Ensure negotiation is successful */
                    if (status == NU_SUCCESS)
                    {
                        /* Create a telnet shell session */
                        status = NU_Create_Telnet_Shell(&telnet_shell_session);

                        /* Ensure shell session successfully created */
                        if (status == NU_SUCCESS)
                        {
                            /* Self-suspend */
                            (VOID)NU_Suspend_Task(NU_Current_Task_Pointer());

                            /* Clear telnet shell session pointer */
                            telnet_shell_session = NU_NULL;
                        }
                    }
                }

                /* Free telnet parameters */
                NU_Telnet_Free_Parameters(telnet_shell_socket);

                /* Ensure socket is closed to free socket resources */
                (VOID)NU_Close_Socket(telnet_shell_socket);
            }
        }   /* while */
    }
}


/*************************************************************************
*
*   FUNCTION
*
*       tns_shell_init
*
*   DESCRIPTION
*
*       Function to init telnet I/O for shell
*
*   INPUTS
*
*       p_shell - Shell session handle
*
*   OUTPUTS
*
*       NU_SUCCESS
*
*************************************************************************/
static STATUS  tns_shell_init(NU_SHELL * p_shell)
{
    /* Set telnet shell session to currently used telnet socket */
    p_shell->session_io.io_session_info = (VOID *)telnet_shell_socket;

    /* Set session name */
    (VOID)NU_Shell_Set_Name(p_shell, "telnet");

    /* Return NU_SUCCESS */
    return (NU_SUCCESS);
}


/*************************************************************************
*
*   FUNCTION
*
*       tns_shell_deinit
*
*   DESCRIPTION
*
*       Function to deinit telnet I/O for shell
*
*   INPUTS
*
*       p_shell - Shell session handle
*
*   OUTPUTS
*
*       NU_SUCCESS
*
*************************************************************************/
static STATUS  tns_shell_deinit(NU_SHELL * p_shell)
{
    /* Resume the telnet shell task to finish clean-up and wait
       for a new client */
    (VOID)NU_Resume_Task(telnet_shell_task);

    /* Return success */
    return (NU_SUCCESS);
}


/*************************************************************************
*
*   FUNCTION
*
*       tns_shell_puts
*
*   DESCRIPTION
*
*       Function to put a string to telnet I/O
*
*   INPUTS
*
*       p_shell - Shell session handle
*
*       string_ptr - Pointer to string to be written
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
static VOID  tns_shell_puts(NU_SHELL * p_shell,
                            const CHAR * string_ptr)
{
    INT16 total = strlen(string_ptr);
    INT16 sent;


    /* Keep sending until entire contents sent */
    while (total)
    {
        /* Send string over socket. */
        sent = NU_Send((INT)p_shell->session_io.io_session_info,
                       (CHAR *)string_ptr, (UINT16)total, 0);

        /* See if an error occurred during the send */
        if (sent < 0)
        {
            /* Break from loop */
            break;
        }

        /* Adjust total and string pointer by amount actually sent */
        total -= sent;
        string_ptr += sent;
    }
}


/*************************************************************************
*
*   FUNCTION
*
*       tns_shell_getch
*
*   DESCRIPTION
*
*       Function to get a character from telnet I/O
*
*   INPUTS
*
*       p_shell - Shell session handle
*
*   OUTPUTS
*
*       CHAR
*
*************************************************************************/
static CHAR  tns_shell_getch(NU_SHELL * p_shell)
{
    STATUS  status;
    CHAR    ch = 0;


    /* Suspend waiting for a character */
    status = NU_Telnet_Get_Filtered_Char((INT)p_shell->session_io.io_session_info,
                                         NU_SUSPEND, &ch);

    /* Check status of call */
    if (status != NU_SUCCESS)
    {
        /* Conection with client must be lost - signal abort on shell */
        (VOID)NU_Shell_Abort(p_shell);
    }

    /* Return a character to the caller */
    return (ch);
}


/*************************************************************************
*
*   FUNCTION
*
*       tns_shell_special
*
*   DESCRIPTION
*
*       Function to handle special characters from telnet I/O
*
*   INPUTS
*
*       p_shell - Shell session handle
*
*       ch - Character to be processed
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
static VOID tns_shell_special(NU_SHELL * p_shell, CHAR ch)
{
    /* Execute on character */
    switch (ch)
    {
        /* Escape character */
        case 0x1b:

            /* Consume the next 2 characters */
            (VOID)p_shell->session_io.io_getch(p_shell);
            (VOID)p_shell->session_io.io_getch(p_shell);

            break;

        /* Some terminals use DEL (127) for backspace */
        case 0x7F:

            /* Ensure length is not 0 */
            if (p_shell->input_line_len > 0)
            {
                /* Reduce the length of the string by one */
                p_shell->input_line_len--;

                /* Null terminate the last character to erase it */
                p_shell -> input_line[p_shell -> input_line_active][p_shell->input_line_len] = 0;

                /* Send a backspace + space + backspace sequence to delete character */
                p_shell->session_io.io_puts(p_shell,"\b \b");
            }

            break;

        default:

            /* Do nothing */

            break;
    }
}


/*************************************************************************
*
*   FUNCTION
*
*       NU_Create_Telnet_Shell
*
*   DESCRIPTION
*
*       Function to create a single telnet shell session.  For multiple
*       clients, this function will need to be called once for each client.
*
*   INPUTS
*
*       shell_session - pointer to location to store shell session pointer
*
*   OUTPUTS
*
*       NU_SUCCESS if successful
*
*************************************************************************/
STATUS  NU_Create_Telnet_Shell(NU_SHELL ** shell_session)
{
    STATUS  status;


    /* Check if pointer passed in is valid */
    if (shell_session == NU_NULL)
    {
        /* Return invalid pointer error */
        status = NU_INVALID_POINTER;
    }
    else
    {
        /* Create Shell session using telnet I/O */
        status = NU_Create_Shell(shell_session,
                                 tns_shell_init,
                                 tns_shell_deinit,
                                 tns_shell_puts,
                                 tns_shell_getch,
                                 tns_shell_special,
                                 NU_TRUE);
    }

    /* Return status */
    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       nu_os_svcs_shell_init
*
*   DESCRIPTION
*
*       This function is called by the Nucleus OS run-level system to
*       initialize or terminate the telnet shell component
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
STATUS nu_os_net_prot_telnet_shell_init (CHAR *   path, INT cmd)
{
    STATUS              status;
    NU_MEMORY_POOL *    sys_mem;


    /* Set initial function status. */
    status = NU_SUCCESS;

    /* Determine how to proceed based on the control command. */
    switch (cmd)
    {
        case RUNLEVEL_STOP :
        {
            /* Determine if there is a telnet shell session running right now */
            if (telnet_shell_session)
            {
                /* Delete the shell */
                (VOID)NU_Delete_Shell(telnet_shell_session);
            }

            /* Check if telnet shell task is not NULL */
            if (telnet_shell_task)
            {
                /* Terminate the telnet shell task */
                status = NU_Terminate_Task(telnet_shell_task);

                if (status == NU_SUCCESS)
                {
                    /* Delete the telnet shell task */
                    status = NU_Delete_Task(telnet_shell_task);

                    /* Set task pointer to null */
                    telnet_shell_task = NU_NULL;
                }
            }

            /* Determine if telnet socket is connected */
            if (NU_Is_Connected(telnet_shell_socket))
            {
                /* Close the socket */
                NU_Close_Socket(telnet_shell_socket);
            }

            /* Free telnet parameters */
            NU_Telnet_Free_Parameters(telnet_shell_socket);

            /* Set telnet shell socket to 0 */
            telnet_shell_socket = 0;

            break;
        }

        case RUNLEVEL_START :
        {
            /* Get Nucleus OS (cached) memory resources. */
            status = NU_System_Memory_Get(&sys_mem, NU_NULL);

            /* Ensure memory pool available */
            if ((status == NU_SUCCESS) && (telnet_shell_task == NU_NULL))
            {
                /* Create shell interface task. */
                status = NU_Create_Auto_Clean_Task(&telnet_shell_task,
                                                   "TNS_TSK",
                                                   tns_shell_thread_entry,
                                                   0,
                                                   NU_NULL,
                                                   sys_mem,
                                                   TNS_TASK_STACK_SIZE,
                                                   TNS_TASK_PRIORITY,
                                                   0,
                                                   NU_PREEMPT,
                                                   NU_START);
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


/*************************************************************************
*
*   FUNCTION
*
*       NU_Get_Telnet_Shell_Session
*
*   DESCRIPTION
*
*       Returns the pointer to the automatically created telnet shell
*       session (created during run-level initialization).
*
*   INPUTS
*
*       shell_session - pointer to location to store shell session pointer
*
*   OUTPUTS
*
*       NU_SUCCESS if successful
*
*************************************************************************/
STATUS  NU_Get_Telnet_Shell_Session(NU_SHELL ** shell_session)
{
    STATUS  status;


    /* Check if pointer passed in is valid */
    if (shell_session == NU_NULL)
    {
        /* Return invalid pointer error */
        status = NU_INVALID_POINTER;
    }
    /* See if telnet shell session has been created */
    else if (telnet_shell_session == NU_NULL)
    {
        /* Return unavailable error */
        status = NU_UNAVAILABLE;
    }
    else
    {
        /* Return pointer */
        *shell_session = telnet_shell_session;
    }

    /* Return status */
    return (status);
}
