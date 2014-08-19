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
*       shell_internal.c
*
*   COMPONENT
*
*       Shell service
*
*   DESCRIPTION
*
*       This file contains the main functionality for the Shell service.
*
*   FUNCTIONS
*
*       shell_serial_init
*       shell_serial_deinit
*       shell_serial_puts
*       shell_serial_getch
*       shell_serial_special
*       shell_input_line
*       shell_find_command
*       shell_string_to_argv
*       shell_process
*       shell_command_help
*       shell_command_quit
*       shell_command_sleep
*       Shell_Banner
*       Shell_Remove_Shell
*       Shell_Thread_Entry
*       Shell_Register_Cmd
*       Shell_Remove_Cmd
*       Shell_Process_Cmd
*       Shell_Init_Struct
*       nu_os_svcs_shell_init
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       nu_services.h
*       nu_drivers.h
*       shell_defs.h
*       ctype.h
*       string.h
*
*************************************************************************/

/* Include files */
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "drivers/nu_drivers.h"
#include "shell_defs.h"
#include <ctype.h>
#include <string.h>

/* Global variables */
NU_SEMAPHORE        Shell_Mutex;
CS_NODE *           Shell_Created_Shell_List;
NU_SHELL *          Shell_Global_Cmds;

#if (CFG_NU_OS_SVCS_SHELL_SERIAL_ENABLED == 1)
/* Session pointer for serial shell */
NU_SHELL *          Shell_Serial_Session;
#endif

/* Local Function Prototypes */
static SHELL_CMD *  shell_find_command (NU_SHELL *, CHAR *);
static STATUS       shell_string_to_argv (CHAR *, VOID *, UINT, INT *, CHAR *** );
static STATUS       shell_process (NU_SHELL *);
static VOID         shell_input_line (NU_SHELL *);
static STATUS       shell_command_help (NU_SHELL *, INT, CHAR **);
static STATUS       shell_command_quit (NU_SHELL *, INT, CHAR **);
static STATUS       shell_command_sleep (NU_SHELL *, INT, CHAR **);

/* Local functions Definitions */

#if (CFG_NU_OS_SVCS_SHELL_SERIAL_ENABLED == 1)
/*************************************************************************
*
*   FUNCTION
*
*       shell_serial_init
*
*   DESCRIPTION
*
*       Function to init serial I/O for shell
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
static STATUS  shell_serial_init(NU_SHELL * p_shell)
{
    SERIAL_SESSION *    port;
    STATUS              status;


    /* Get standard I/O serial port */
    port = NU_SIO_Get_Port();

    /* Set status based on port */
    if (port == NU_NULL)
    {
        /* Return error */
        status = NU_NO_SERIAL_SHELL;
    }
    else
    {
        /* Set port handle into shell session I/O info */
        p_shell->session_io.io_session_info = (VOID *)port;

        /* Set session name */
        (VOID)NU_Shell_Set_Name(p_shell, "serial");

        /* Activate blocking serial reads. */
        status = NU_Serial_Set_Read_Mode(port,
                                         NU_SUSPEND,
                                         NU_SUSPEND);
    }

    /* Return status */
    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       shell_serial_deinit
*
*   DESCRIPTION
*
*       Function to deinit serial I/O for shell
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
static STATUS  shell_serial_deinit(NU_SHELL * p_shell)
{
    /* Set info to NULL */
    p_shell->session_io.io_session_info = NU_NULL;

    /* Return success */
    return (NU_SUCCESS);
}


/*************************************************************************
*
*   FUNCTION
*
*       shell_serial_puts
*
*   DESCRIPTION
*
*       Function to put a string to serial I/O
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
static VOID  shell_serial_puts(NU_SHELL * p_shell,
                               const CHAR * string_ptr)
{
    SERIAL_SESSION *    port;


    /* Point to the port structure */
    port = (SERIAL_SESSION *)p_shell->session_io.io_session_info;

    /* Output string */
    (VOID)NU_Serial_Puts(port, string_ptr);
}


/*************************************************************************
*
*   FUNCTION
*
*       shell_serial_getch
*
*   DESCRIPTION
*
*       Function to get a character from serial I/O
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
static CHAR  shell_serial_getch(NU_SHELL * p_shell)
{
    SERIAL_SESSION *    port;
    CHAR                ch;


    /* Point to the port structure */
    port = (SERIAL_SESSION *)p_shell->session_io.io_session_info;

    /* Get a character */
    ch = NU_Serial_Getchar(port);

    /* Return a character to the caller */
    return (ch);
}


/*************************************************************************
*
*   FUNCTION
*
*       shell_serial_special
*
*   DESCRIPTION
*
*       Function to handle special characters from serial I/O
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
static VOID shell_serial_special(NU_SHELL * p_shell, CHAR ch)
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

#ifdef CFG_NU_BSP_REALVIEW_EB_CT926EJS_ENABLE

        /* QEMU uses DEL for backspace */
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
#endif

        default:

            /* Do nothing */

            break;
    }
}
#endif  /* #if (CFG_NU_OS_SVCS_SHELL_SERIAL_ENABLED == 1) */


/*************************************************************************
*
*   FUNCTION
*
*       shell_input_line
*
*   DESCRIPTION
*
*       This function will retrieve a line of input.  It handles console
*       control characters, such as backspace, as well as echoing entered
*       characters back to the console.
*
*   INPUTS
*
*       p_shell - Shell session handle
*
*   OUTPUTS
*
*       None.
*
*************************************************************************/
static VOID shell_input_line(NU_SHELL *       p_shell)
{
    BOOLEAN     done = NU_FALSE;
    CHAR        ch;


    /* Zeroize the input buffer */
    memset((VOID *)p_shell -> input_line[p_shell -> input_line_active],
            0, CFG_NU_OS_SVCS_SHELL_COLUMNS + 1);

    /* Process until user done entering data */
    while (done == NU_FALSE)
    {
        /* Get a character from the user. */
        ch = p_shell->session_io.io_getch(p_shell);

        /* Check for abort assertion */
        if (p_shell->shell_abort == NU_FALSE)
        {
            /* Obtain session mutex while processing character */
            NU_Obtain_Semaphore(&(p_shell->cmd_sem), NU_SUSPEND);

            /* Check character */
            switch (ch)
            {
                /* Backspace */
                case '\b':

                    /* Ensure length is not 0 */
                    if (p_shell->input_line_len > 0)
                    {
                        /* Reduce the length of the string by one */
                        p_shell->input_line_len--;

                        /* Null terminate the last character to erase it */
                        p_shell -> input_line[p_shell -> input_line_active][p_shell->input_line_len] = 0;

                        /* See if echo is on */
                        if (p_shell->session_io.io_echo_on == NU_TRUE)
                        {
                            /* Echo backspace */
                            p_shell->session_io.io_puts(p_shell, "\b");
                        }

                        /* Send a space + backspace sequence to delete character */
                        p_shell->session_io.io_puts(p_shell," \b");
                    }
                    else if (p_shell->session_io.io_echo_on == NU_FALSE)
                    {
                        /* Put last character of prompt to prevent backspace in terminal */
                        p_shell->session_io.io_puts(p_shell,">");
                    }

                    break;

                /* Carriage-return */
                case '\r':


                    /* See if echo is on */
                    if (p_shell->session_io.io_echo_on == NU_TRUE)
                    {
                        /* Echo carriage return / line feed */
                        p_shell->session_io.io_puts(p_shell, "\r\n");
                    }

                    /* Set flag showing line input done */
                    done = NU_TRUE;

                    /* Reset command length for next command processing */
                    p_shell->input_line_len = 0;

                    break;

                /* Line feed */
                case '\n':

                    /* Do nothing */
                    break;

                /* All other characters */
                default:

                    /* Ensure data doesn't exceed full terminal width */
                    if (p_shell->input_line_len < CFG_NU_OS_SVCS_SHELL_COLUMNS)
                    {
                        /* See if a "standard" printable ASCII character received */
                        if ((ch >= 32) && (ch <= 126))
                        {
                            /* Add character to string */
                            p_shell -> input_line[p_shell -> input_line_active][p_shell->input_line_len] = ch;

                            /* See if echo is on */
                            if (p_shell->session_io.io_echo_on == NU_TRUE)
                            {
                                /* Echo back the input */
                                p_shell->session_io.io_puts(p_shell,
                                                            &p_shell -> input_line[p_shell -> input_line_active][p_shell->input_line_len]);
                            }

                            /* Move to next character in string */
                            p_shell->input_line_len++;
                        }
                        else
                        {
                            /* See if a "special" character handler is installed */
                            if (p_shell->session_io.io_special)
                            {
                                /* Call special character handler */
                                p_shell->session_io.io_special(p_shell, ch);
                            }
                        }
                    }

                    break;
            }   /* Switch statement */

            /* Done processing character - release mutex */
            (VOID)NU_Release_Semaphore(&(p_shell->cmd_sem));
        }
        else
        {
            /* Break from while loop */
            done = NU_TRUE;
        }
    }   /* while loop */
}


/*************************************************************************
*
*   FUNCTION
*
*       shell_find_command
*
*   DESCRIPTION
*
*       Used to match command strings with command structures.
*
*       NOTE: This function accesses the registered commands list.  Any
*       caller of this function should have exclusive access to the
*       registered commands list (e.g. through use of a mutex).
*
*   INPUTS
*
*       p_shell - Shell session handle
*
*       cmd_str - Command name string
*
*   OUTPUTS
*
*       Pointer to shell command structure if a matching command string is
*       found or NU_NULL otherwise.
*
*************************************************************************/
static SHELL_CMD * shell_find_command(NU_SHELL *     p_shell,
                                    CHAR *       cmd_str)
{
    SHELL_CMD *     p_cmd;
    CHAR            cmd_str_lc[CFG_NU_OS_SVCS_SHELL_COLUMNS];
    BOOLEAN         is_found = NU_FALSE;


    /* Ensure the list is not empty. */
    if (p_shell -> cmd_count > 0)
    {
        /* Convert command to lower case to prevent case sensitivity. */
        SHELL_STRING_TO_LOWER_CASE(cmd_str, cmd_str_lc);

        /* Get first command in the list. */
        p_cmd = (SHELL_CMD *)p_shell -> cmd_list -> cs_next;

        /* Check each command in the list for a match. */
        while ((p_cmd != NU_NULL) &&
               (is_found == NU_FALSE))
        {
            /* Check current command string. */
            if (strcmp(p_cmd -> str, cmd_str_lc) == 0)
            {
                is_found = NU_TRUE;
            }
            else
            {
                /* Move to next command in the list (if one exists). */
                if (p_shell -> cmd_list -> cs_next == p_cmd -> node.cs_next)
                {
                    p_cmd = NU_NULL;
                }
                else
                {
                    p_cmd = (SHELL_CMD *) p_cmd -> node.cs_next;
                }
            }
        }
    }
    else
    {
        /* No commands in the list. */
        p_cmd = NU_NULL;
    }

    return (p_cmd);
}


/*************************************************************************
*
*   FUNCTION
*
*       shell_string_to_argv
*
*   DESCRIPTION
*
*       Convert an input string to an array of null terminated string
*       pointers
*
*   INPUTS
*
*       argv_str - Source string to be converted to vector.
*
*                  NOTE: This operation is destructive to the string and
*                  will not contain the same value after this operation.
*
*       p_argv_mem - Pointer of memory used to build argv vector.
*
*       p_argv_mem_size - Size (in bytes) of memory used to build argv
*                         vector.
*
*       p_argc - Return parameter that will be updated to contain the
*                number of strings in the vector if the operation is
*                successful.  If the operation fails the value is
*                undetermined.
*
*       p_argv - Return parameter that will be updated to contain the
*                string vector if the operation is successful.  If the
*                operation fails the value is undetermined.
*
*   OUTPUTS
*
*       NU_SUCCESS - Indicates successful operation.
*
*************************************************************************/
static STATUS shell_string_to_argv(CHAR *    argv_str,
                                   VOID *    p_argv_mem,
                                   UINT      argv_mem_size,
                                   INT *     p_argc,
                                   CHAR ***  p_argv)
{
    UINT        argc;
    CHAR **     argv;
    CHAR *      p_ch;


    /* Setup initial argument values. */
    argc = 0;
    argv = NU_NULL;

    /* Ensure there are arguments to be processed. */
    if (argv_str != NU_NULL)
    {
        /* Process the argument string (there is at least one element). */
        argc = 0;
        argv = (CHAR **)p_argv_mem;
        p_ch = argv_str;
        while(*p_ch != NU_NULL)
        {
            /* Add argument (string) pointer to the vector. */
            argv[argc] = p_ch;

            /* Move past the vector entry argument string (in the argument
               string). */
            while((*p_ch != ' ') &&
                  (*p_ch != ',') &&
                  (*p_ch != NU_NULL))
            {
                p_ch++;
            }

            /* Count the argument just processed. */
            argc++;

            /* Check for the end of the argument string. */
            if (*p_ch != NU_NULL)
            {
                /* Terminate the vector entry argument string and move to
                   the next. */
                *p_ch = NU_NULL;
                p_ch++;
            }
        }
    }

    /* Update return parameters */
    *p_argc = argc;
    *p_argv = argv;

    return (NU_SUCCESS);
}


/*************************************************************************
*
*   FUNCTION
*
*       shell_process
*
*   DESCRIPTION
*
*       This is the shell function for managing command processing
*
*   INPUTS
*
*       p_shell - Shell session handle
*
*   OUTPUTS
*
*       NU_SUCCESS - Indicates successful operation.
*
*       <other> - Indicates (other) internal error.
*
*************************************************************************/
static STATUS shell_process(NU_SHELL *  p_shell)
{
    STATUS      status;
    CHAR *      p_input_line;


    /* Check for abort request */
    if (p_shell->shell_abort == NU_FALSE)
    {
        /* Check for the repeat command character in (active) input line. */
        if (p_shell -> input_line[p_shell -> input_line_active][0] == '.')
        {
            /* Repeat the last command (using inactive input line). */
            p_input_line = &p_shell -> input_line[SHELL_INPUT_LINE_OTHER(p_shell -> input_line_active)][0];
        }
        else
        {
            /* Process current command (using active input line). */
            p_input_line = &p_shell -> input_line[p_shell -> input_line_active][0];

            /* Switch active input line. */
            p_shell -> input_line_active = SHELL_INPUT_LINE_OTHER(p_shell -> input_line_active);
        }

        /* Process command */
        status = Shell_Process_Cmd(p_shell, p_input_line);
    }
    else
    {
        /* Remove the shell */
        (VOID)Shell_Remove_Shell(p_shell);

        /* Return kill shell status */
        status = NU_KILL_SHELL;
    }

    /* Process command and return result to caller */
    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       shell_command_help
*
*   DESCRIPTION
*
*       This is the built-in command: help
*
*   INPUTS
*
*       p_shell - Shell session handle
*
*       argc - Argument count.  NOT USED.
*
*       argv - Argument vector.  NOT USED.
*
*   OUTPUTS
*
*       NU_SUCCESS - Indicates successful operation.
*
*       <other> - Indicates (other) error occurred.
*
*************************************************************************/
static STATUS shell_command_help(NU_SHELL *   p_shell,
                                 INT          argc,
                                 CHAR **      argv)
{
    STATUS              status;
    INT                 i;
    SHELL_CMD *         p_cmd = NU_NULL;


    /* Print title */
    NU_Shell_Puts(p_shell,"\r\n");
    NU_Shell_Puts(p_shell,"Registered Commands:\r\n");
    NU_Shell_Puts(p_shell,"\r\n");

    /* Obtain access to the registered command list. */
    status = NU_Obtain_Semaphore(&(p_shell -> cmd_sem), NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Proceed based on the number of registered commands. */
        if (p_shell -> cmd_count == 0)
        {
            /* No registered commands */
            NU_Shell_Puts(p_shell,"NONE\r\n");
        }
        else
        {
            /* Print all registered commands */
            p_cmd = (SHELL_CMD *)p_shell -> cmd_list;
            for (i = 0; i < (INT)(p_shell -> cmd_count ) && p_cmd != NU_NULL; i++)
            {
                /* Check if we've filled the screen with info */
                if ( ((i + 1) % CFG_NU_OS_SVCS_SHELL_ROWS) == 0) /* i + 1 used to avoid 0%CFG_NU_OS_SVCS_SHELL_ROWS=0 */
                {
                    /* Pause before we continue on to the next page. */

                    /* Print message to the user.  */
                    NU_Shell_Puts(p_shell,"<**** Hit any key to continue ****>");

                    /* Wait for a character from user (NOT USED) */
                    (VOID)p_shell->session_io.io_getch(p_shell);

                    /* Print a new line after the key is hit.  */
                    NU_Shell_Puts(p_shell,"\r\n");
                }

                /* Output the command string */
                NU_Shell_Puts(p_shell, p_cmd -> str);
                NU_Shell_Puts(p_shell, "\r\n");

                /* Move to the next command in the list */
                p_cmd = (SHELL_CMD *)p_cmd -> node.cs_next;
            }
        }

        NU_Shell_Puts(p_shell, "\r\n");

        /* Release access to the registered commands list. */
        (VOID)NU_Release_Semaphore(&(p_shell -> cmd_sem));
    }

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       shell_command_quit
*
*   DESCRIPTION
*
*       This is the built-in command: quit
*
*   INPUTS
*
*       p_shell - Shell session handle
*
*       argc - Argument count.  NOT USED.
*
*       argv - Argument vector.  NOT USED.
*
*   OUTPUTS
*
*       NU_KILL_SHELL
*
*************************************************************************/
static STATUS shell_command_quit(NU_SHELL *   p_shell,
                                 INT          argc,
                                 CHAR **      argv)
{
    /* Output message showing quitting shell */
    NU_Shell_Puts(p_shell, "\r\nQuitting Shell - Bye!\r\n");

    /* Remove the shell */
    (VOID)Shell_Remove_Shell(p_shell);

    /* Return error that causes shell task to exit and clean-up */
    return (NU_KILL_SHELL);
}


/*************************************************************************
*
*   FUNCTION
*
*       shell_command_sleep
*
*   DESCRIPTION
*
*       This is the built-in command: sleep
*
*   INPUTS
*
*       p_shell - Shell session handle
*
*       argc - Argument count
*
*       argv - Argument vector
*
*   OUTPUTS
*
*       NU_SUCCESS
*
*************************************************************************/
static STATUS shell_command_sleep(NU_SHELL *   p_shell,
                                  INT          argc,
                                  CHAR **      argv)
{
    UINT32  time;


    /* Ensure only 1 arg */
    if (argc != 1)
    {
        /* Output error and format requirements */
        NU_Shell_Puts(p_shell, "\r\nInvalid Usage!\r\n");
        NU_Shell_Puts(p_shell, "Format: sleep <milliseconds>");
    }
    else
    {
        /* Convert parameter to a number */
        time = strtol(argv[0], NU_NULL, 10);

        /* Ensure time is not 0 */
        if (time > 0)
        {
            /* Adjust sleep time based on OS tick rate */
            time = (time * NU_PLUS_TICKS_PER_SEC) / 1000;

            /* Check if math resulted in a time of 0 */
            if (time == 0)
            {
                /* Ensure at least delay of 1 tick */
                time = 1;
            }

            /* Output status */
            NU_Shell_Puts(p_shell, "\r\nSleeping...");

            /* Sleep for calculated time */
            NU_Sleep(time);

            /* Print carriage return and line-feed */
            NU_Shell_Puts(p_shell, " Done!");
        }
        else
        {
            /* Output error and format requirements */
            NU_Shell_Puts(p_shell, "\r\nInvalid Usage!\r\n");
            NU_Shell_Puts(p_shell, "Format: sleep <milliseconds>");
        }
    }

    /* Carriage return and 2 x line-feed before going back to command shell */
    NU_Shell_Puts(p_shell, "\r\n\n");

    /* Return success to caller */
    return (NU_SUCCESS);
}

/* Global functions */

/*************************************************************************
*
*   FUNCTION
*
*       Shell_Banner
*
*   DESCRIPTION
*
*       Displays the initial Shell banner information.
*
*   INPUTS
*
*       p_shell - Shell session handle
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
ESAL_TS_WEAK_DEF(VOID Shell_Banner(NU_SHELL * p_shell))
{
    NU_Shell_Puts(p_shell, "\r\n\n*** Nucleus Shell ***\r\n\r\n");
}


/*************************************************************************
*
*   FUNCTION
*
*       Shell_Remove_Shell
*
*   DESCRIPTION
*
*       Removes the specified shell (deletes semaphore,
*       deallocates memory, etc)
*
*   INPUTS
*
*       p_shell - Shell session handle
*
*   OUTPUTS
*
*       NU_SUCCESS if successful at removing shell
*
*************************************************************************/
STATUS      Shell_Remove_Shell(NU_SHELL * p_shell)
{
    STATUS          status;
    SHELL_CMD *     p_cmd;


    /* Deinit the I/O */
    status = p_shell->session_io.io_deinit(p_shell);

    if (status == NU_SUCCESS)
    {
        /* Delete all commands registered for this session */
        while (p_shell->cmd_count)
        {
            /* Get first command in the list. */
            p_cmd = (SHELL_CMD *)p_shell -> cmd_list -> cs_next;

            /* Delete this command */
            status = Shell_Remove_Cmd (p_shell, p_cmd->str);
        }
    }

    if (status == NU_SUCCESS)
    {
        /* Delete the mutex */
        status = NU_Delete_Semaphore(&(p_shell -> cmd_sem));
    }

    if (status == NU_SUCCESS)
    {
        /* Obtain mutex while traversing or manipulating created shell list */
        status = NU_Obtain_Semaphore(&Shell_Mutex, NU_SUSPEND);

        /* Ensure mutex obtained */
        if (status == NU_SUCCESS)
        {
            /* Remove shell session from list */
            NU_Remove_From_List(&Shell_Created_Shell_List, &(p_shell -> created_list));

            /* Release mutex */
            (VOID)NU_Release_Semaphore(&Shell_Mutex);
        }
    }

    if (status == NU_SUCCESS)
    {
        /* Deallocate the memory for this shell session */
        status = NU_Deallocate_Memory(p_shell);
    }

    /* Return status to caller */
    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       Shell_Thread_Entry
*
*   DESCRIPTION
*
*       This is the main function of the Nucleus SHELL task.  It is
*       mainly responsible for dispatching commands to the various
*       responsible functions.
*
*   INPUTS
*
*       argc - Argument count.  NOT USED.
*
*       argv - Argument vector.  Will contain a pointer to the Shell
*              session handle.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID        Shell_Thread_Entry(UNSIGNED argc, VOID * argv)
{
    NU_SHELL *      p_shell = (NU_SHELL *)argv;
    STATUS          status;
    NU_SUPERV_USER_VARIABLES


    NU_SUPERVISOR_MODE();

    /* Call I/O init function */
    status = p_shell->session_io.io_init(p_shell);

    /* Ensure I/O initialized successfully */
    if (status == NU_SUCCESS)
    {
        /* Print the initial banner before entering the processing loop. */
        Shell_Banner(p_shell);

        /* Loop processing commands until a kill shell status returned */
        while (status != NU_KILL_SHELL)
        {
            /* Prompt the user for a selection. */
            p_shell->session_io.io_puts(p_shell, SHELL_PROMPT_STR);

            /* Obtain an input line from the user. */
            shell_input_line(p_shell);

            /* Process current input line. */
            status = shell_process(p_shell);
        }
    }

    NU_USER_MODE();
}


/*************************************************************************
*
*   FUNCTION
*
*       Shell_Register_Cmd
*
*   DESCRIPTION
*
*       Registers a command to the specified shell session
*
*   INPUTS
*
*       p_shell - Shell session handle
*
*       cmd - Command name string
*
*       cmd_fcn - Command function pointer
*
*   OUTPUTS
*
*       NU_SUCCESS if successful at removing shell
*
*************************************************************************/
STATUS   Shell_Register_Cmd(NU_SHELL *  p_shell,
                            CHAR *      cmd,
                            STATUS      (*cmd_fcn) (NU_SHELL *, INT, CHAR **))
{
    STATUS                  status;
    NU_MEMORY_POOL *        p_memory_pool;
    SHELL_CMD *             p_cmd;
    UNSIGNED                cmd_mem_size;


    /* Obtain command list access. */
    status = NU_Obtain_Semaphore(&(p_shell -> cmd_sem), NU_SUSPEND);

    /* Ensure mutex owned  */
    if (status == NU_SUCCESS)
    {
        /* Check if a duplicate command exists */
        p_cmd = shell_find_command(p_shell,
                                 cmd);
        if (p_cmd != NU_NULL)
        {
            /* ERROR: Found duplicate command already registered. */
            status = NU_DUPLICATE_CMD;
        }

        if (status == NU_SUCCESS)
        {
            /* Get Nucleus OS (cached) memory resources. */
            status = NU_System_Memory_Get(&p_memory_pool, NU_NULL);
        }

        if (status == NU_SUCCESS)
        {
            /* Allocate enough memory for the command structure and the
               command string plus a null terminator for the string */
            cmd_mem_size = (sizeof(SHELL_CMD)
                            + (sizeof(CHAR) * (strlen(cmd) + 1)));

            status = NU_Allocate_Memory(p_memory_pool,
                                        (VOID **)&p_cmd,
                                        cmd_mem_size,
                                        NU_NO_SUSPEND);
        }

        if (status == NU_SUCCESS)
        {
            /* Zero initialize the command memory. */
            memset(p_cmd, 0x00, cmd_mem_size);

            /* Place the string pointer past the structure. */
            p_cmd -> str = (CHAR *)p_cmd + sizeof(SHELL_CMD);

            /* Copy the input string, cmd, and make it lower case. */
            SHELL_STRING_TO_LOWER_CASE(cmd, p_cmd -> str);

            /* Null terminate the string. */
            p_cmd -> str[strlen(cmd)] = 0;

            /* Set command function. */
            p_cmd -> fcn  = cmd_fcn;

            /* Add the command to the list. */
            NU_Place_On_List(&p_shell -> cmd_list, (CS_NODE*) p_cmd);

            /* Update command count. */
            p_shell -> cmd_count++;
        }

        /* Release the command list access (always if access obtained). */
        (VOID)NU_Release_Semaphore(&(p_shell -> cmd_sem));
    }

    /* Return status to caller */
    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       Shell_Remove_Cmd
*
*   DESCRIPTION
*
*       Removes the specified command from the shell session's command
*       list
*
*   INPUTS
*
*       p_shell - Shell session handle
*
*       cmd - command string to remove
*
*   OUTPUTS
*
*       NU_SUCCESS if successful at removing command
*
*************************************************************************/
STATUS Shell_Remove_Cmd(NU_SHELL * p_shell, CHAR * cmd)
{
    STATUS          status;
    SHELL_CMD *     p_cmd;


    /* Obtain command list access. */
    status = NU_Obtain_Semaphore(&(p_shell -> cmd_sem), NU_SUSPEND);

    /* Ensure mutex owned */
    if (status == NU_SUCCESS)
    {
        /* Find the matching command structure. */
        p_cmd = shell_find_command(p_shell, cmd);

        /* Determine if command found */
        if (p_cmd == NU_NULL)
        {
            status = NU_UNAVAILABLE;
        }
        else
        {
            /* Remove it from the list of commands. */
            NU_Remove_From_List(&p_shell -> cmd_list, (CS_NODE*) p_cmd);

            /* Decrement the number of command's associated with this shell */
            p_shell -> cmd_count--;

            /* Return the memory. */
            status = NU_Deallocate_Memory(p_cmd);
        }

        /* Release the command list access (always if access obtained). */
        (VOID)NU_Release_Semaphore(&(p_shell -> cmd_sem));
    }

    /* Return status to caller */
    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       Shell_Process_Cmd
*
*   DESCRIPTION
*
*       This processes a single command passed in by the caller.
*       It is mainly responsible for dispatching commands to the various
*       responsible functions.
*
*   INPUTS
*
*       p_shell - Shell session handle
*
*       p_input_line - command line to process
*
*   OUTPUTS
*
*       NU_SUCCESS - Indicates successful operation.
*
*       <other> - Indicates (other) internal error.
*
*************************************************************************/
STATUS      Shell_Process_Cmd(NU_SHELL * p_shell, CHAR * p_input_line)
{
    STATUS          status = NU_SUCCESS;
    SHELL_CMD *     p_cmd;
    SHELL_CMD_FCN   cmd_fcn;
    CHAR            cmd_argv_str[CFG_NU_OS_SVCS_SHELL_COLUMNS + 1];
    UINT8           cmd_argv_mem[sizeof(CHAR *) * ((CFG_NU_OS_SVCS_SHELL_COLUMNS + 1) / 2)];
    INT             cmd_argc;
    CHAR **         cmd_argv;


    /* Copy the input line into an argument string to become part of the
       argument vector. */
    (VOID)strcpy(&cmd_argv_str[0], p_input_line);

    /* Build the argv vector from the string.  The first argument
       in the resulting vector will be the command string itself. */

    /* NOTE: This process is destructive to the argument string! */

    (VOID)shell_string_to_argv(&cmd_argv_str[0],
                               (VOID *)&cmd_argv_mem[0],
                               sizeof(cmd_argv_mem),
                               &cmd_argc,
                               &cmd_argv);

    /* Determine if there is a command to process. */
    if (cmd_argc != 0)
    {
        /* Obtain access to the registered command list. */
        status = NU_Obtain_Semaphore(&(p_shell -> cmd_sem), NU_SUSPEND);

        /* Ensure mutex is obtained or already owned by this thread */
        if ((status == NU_SUCCESS) || (status == NU_SEMAPHORE_ALREADY_OWNED))
        {
            /* See if command is in the registered command list. */
            p_cmd = shell_find_command(p_shell, cmd_argv[0]);

            if (p_cmd != NU_NULL)
            {
                /* Make a copy of the command function to in case it is
                   removed right before the call. */
                cmd_fcn = p_cmd -> fcn;

                /* Release access to the registered commands list. */

                /* NOTE: Releasing access to the command list allows
                   commands to register or unregister other commands and
                   themselves. */
                (VOID)NU_Release_Semaphore(&(p_shell -> cmd_sem));

                /* Call the command passing the appropriate command
                   arguments. */
                if ((cmd_argc - 1) == 0)
                {
                    status = cmd_fcn(p_shell, 0, NU_NULL);
                }
                else
                {
                    status = cmd_fcn(p_shell, (cmd_argc - 1), &cmd_argv[1]);
                }
            }
            else
            {
                /* ERROR: Invalid command entered. */
                p_shell->session_io.io_puts(p_shell, "\r\nERROR: Invalid Command\r\n\r\n");

                /* Release access to the registered commands list. */
                (VOID)NU_Release_Semaphore(&(p_shell -> cmd_sem));
            }
        }
    }

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       Shell_Init_Struct
*
*   DESCRIPTION
*
*       Allocates and initializes shell session structure
*
*   INPUTS
*
*       p_shell - Pointer to shell session handle
*
*   OUTPUTS
*
*       NU_SUCCESS if structure successfully allocated / initialized
*
*************************************************************************/
STATUS   Shell_Init_Struct(NU_SHELL ** p_shell, NU_MEMORY_POOL * mem_pool)
{
    STATUS           status;


    /* Allocate memory for shell session */
    status = NU_Allocate_Memory(mem_pool, (VOID **)p_shell,
                                sizeof(NU_SHELL), NU_NO_SUSPEND);

    /* Ensure previous operation successful */
    if (status == NU_SUCCESS)
    {
        /* Zero-initialize the service control block. */
        memset((VOID *)*p_shell, 0x00, sizeof(NU_SHELL));

        /* Create Priority Inheritance semaphore (Mutex) for management of
           dynamic commands. */
        status = NU_Create_Semaphore(&((*p_shell) -> cmd_sem),
                                     "SHELLCMD",
                                     1,
                                     NU_PRIORITY_INHERIT);
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
*       initialize or terminate the Shell service.
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
STATUS nu_os_svcs_shell_init (CHAR *   path, INT cmd)
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
            /* ERROR: Shell service does not support shutdown. */

            /* ERROR RECOVERY: Report success and do nothing. */

            break;
        }

        case RUNLEVEL_START :
        {
            /* Get Nucleus OS (cached) memory resources. */
            status = NU_System_Memory_Get(&sys_mem, NU_NULL);

            /* Ensure previous operation successful */
            if (status == NU_SUCCESS)
            {
                /* Create container for "global" commands */
                status = Shell_Init_Struct(&Shell_Global_Cmds,sys_mem);
            }

            if (status == NU_SUCCESS)
            {
                /* Register the built-in "help" command. */
                status = Shell_Register_Cmd(Shell_Global_Cmds, "help", shell_command_help);
            }

            if (status == NU_SUCCESS)
            {
                /* Register the built-in "quit" command. */
                status = Shell_Register_Cmd(Shell_Global_Cmds, "quit", shell_command_quit);
            }

            if (status == NU_SUCCESS)
            {
                /* Register the built-in "sleep" command. */
                status = Shell_Register_Cmd(Shell_Global_Cmds, "sleep",shell_command_sleep);
            }

            /* Ensure previous operation successful */
            if (status == NU_SUCCESS)
            {
                /* Create Priority Inheritance semaphore (Mutex) for management of
                   shell sessions, etc */
                status = NU_Create_Semaphore(&Shell_Mutex,
                                             "SHELL",
                                             1,
                                             NU_PRIORITY_INHERIT);
            }

#if (CFG_NU_OS_SVCS_SHELL_SERIAL_ENABLED == 1)
            /* Ensure previous operation successful */
            if (status == NU_SUCCESS)
            {
                /* Create Shell session using serial I/O */
                status = NU_Create_Shell(&Shell_Serial_Session,
                                         shell_serial_init,
                                         shell_serial_deinit,
                                         shell_serial_puts,
                                         shell_serial_getch,
                                         shell_serial_special,
                                         NU_TRUE);
            }
#endif  /* (CFG_NU_OS_SVCS_SHELL_SERIAL_ENABLED == 1) */

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
