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
*       shell_public.c
*
*   COMPONENT
*
*       Shell service
*
*   DESCRIPTION
*
*       This file contains the public interfaces for the Shell service.
*
*   FUNCTIONS
*
*       shell_dummy_puts
*       shell_register_global_cmds
*       NU_Create_Shell
*       NU_Delete_Shell
*       NU_Register_Command
*       NU_Unregister_Command
*       NU_Execute_Command
*       NU_Shell_Puts
*       NU_Shell_Abort
*       NU_Get_Shell_Serial_Session_ID
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       nu_services.h
*       shell_defs.h
*       stdlib.h
*       ctype.h
*       string.h
*
*************************************************************************/

/* Include files */
#include "nucleus.h"
#include "kernel/nu_kernel.h"
#include "services/nu_services.h"
#include "shell_defs.h"
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/* Local Function Prototypes */
static VOID         shell_dummy_puts(NU_SHELL *, const CHAR * string_ptr);
static VOID         shell_register_global_cmds(NU_SHELL * p_shell);

/* Local functions */

/*************************************************************************
*
*   FUNCTION
*
*       shell_dummy_puts
*
*   DESCRIPTION
*
*       Function to put a string no where
*
*   INPUTS
*
*       p_shell - Shell session handle
*
*       string_ptr - Pointer to string to be thrown away
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
static VOID  shell_dummy_puts(NU_SHELL * p_shell,
                              const CHAR * string_ptr)
{
    /* Avoid compiler warnings */
    NU_UNUSED_PARAM(p_shell);
    NU_UNUSED_PARAM(string_ptr);
}


/*************************************************************************
*
*   FUNCTION
*
*       shell_register_global_cmds
*
*   DESCRIPTION
*
*       Function that will register all global commands with the
*       passed-in shell session
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
static VOID  shell_register_global_cmds(NU_SHELL * p_shell)
{
    STATUS          status;
    SHELL_CMD *     p_cmd;
    INT             list_count;


    /* Obtain mutex for the global commands */
    status = NU_Obtain_Semaphore(&(Shell_Global_Cmds -> cmd_sem), NU_SUSPEND);

    /* Ensure mutex obtained */
    if (status == NU_SUCCESS)
    {
        /* Get size of global command list */
        list_count = Shell_Global_Cmds -> cmd_count;

        /* Get first command in the list. */
        p_cmd = (SHELL_CMD *)Shell_Global_Cmds -> cmd_list -> cs_next;

        /* Add all globally registered commands to this session */
        while (list_count > 0)
        {
            /* Add this command */
            (VOID)Shell_Register_Cmd (p_shell, p_cmd->str, p_cmd->fcn);

            /* Move to the next command */
            p_cmd = (SHELL_CMD *)(p_cmd -> node.cs_next);

            /* Decrement number of remaining commands to register */
            list_count--;
        }

        /* Release the global command mutex */
        (VOID)NU_Release_Semaphore(&(Shell_Global_Cmds -> cmd_sem));
    }
}

/* Public functions */

/*************************************************************************
*
*   FUNCTION
*
*       NU_Create_Shell
*
*   DESCRIPTION
*
*       This function is used to create a Shell session
*
*   INPUTS
*
*       p_shell_return - pointer to where Shell session handle will be returned
*
*       io_init - pointer to I/O init routine
*
*       io_deinit - pointer to I/O deinit routine
*
*       io_puts - pointer to I/O put string routine
*
*       io_getch - pointer to I/O get char routine
*
*       io_special - pointer to I/O specific handler for "special" characters
*
*       io_echo_on - NU_TRUE means that characters received will be echoed
*
*   OUTPUTS
*
*       NU_SUCCESS - Session created
*
*************************************************************************/
STATUS NU_Create_Shell (NU_SHELL ** p_shell_return,
                        STATUS      (*io_init)(NU_SHELL *),
                        STATUS      (*io_deinit)(NU_SHELL *),
                        VOID        (*io_puts)(NU_SHELL *, const CHAR *),
                        CHAR        (*io_getch)(NU_SHELL *),
                        VOID        (*io_special)(NU_SHELL *, CHAR),
                        INT         io_echo_on)
{
    STATUS           status;
    NU_MEMORY_POOL * sys_mem;
    NU_SHELL *       p_shell;
    NU_SUPERV_USER_VARIABLES


    NU_SUPERVISOR_MODE();

    /* Get Nucleus OS (cached) memory resources. */
    status = NU_System_Memory_Get(&sys_mem, NU_NULL); 

    /* Ensure memory pool address obtained */
    if (status == NU_SUCCESS)
    {
        /* Init shell structure */
        status = Shell_Init_Struct(&p_shell, sys_mem);
    }

    if (status == NU_SUCCESS)
    {
        /* Set I/O pointers */
        p_shell->session_io.io_init = io_init;
        p_shell->session_io.io_deinit = io_deinit;
        p_shell->session_io.io_puts = io_puts;
        p_shell->session_io.io_getch = io_getch;
        p_shell->session_io.io_special = io_special;
        p_shell->session_io.io_echo_on = io_echo_on;
    }

    if (status == NU_SUCCESS)
    {
        /* Create shell interface task. */
        status = NU_Create_Auto_Clean_Task(&(p_shell->shell_task),
                                           "SHELL",
                                           Shell_Thread_Entry,
                                           0,
                                           (VOID *)p_shell,
                                           sys_mem,
                                           SHELL_STACK_SIZE,
                                           SHELL_TASK_PRIORITY,
                                           SHELL_TASK_SLICE,
                                           NU_PREEMPT,
                                           NU_START);
    }

    /* Add shell session to list and set return pointer if successful creating the session */
    if (status == NU_SUCCESS)
    {
        /* Obtain mutex while traversing or manipulating created shell list */
        status = NU_Obtain_Semaphore(&Shell_Mutex, NU_SUSPEND);

        /* Ensure successful */
        if (status == NU_SUCCESS)
        {
            /* Add to list of created shell sessions */
            NU_Place_On_List(&Shell_Created_Shell_List, &(p_shell -> created_list));

            /* Ensure all globally registered commands applied to this shell */
            shell_register_global_cmds(p_shell);

            /* Release mutex */
            (VOID)NU_Release_Semaphore(&Shell_Mutex);
        }

        /* Return pointer to shell structure */
        *p_shell_return = (VOID *)p_shell;
    }
    else
    {
        *p_shell_return = NU_NULL;
    }

    NU_USER_MODE();

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       NU_Delete_Shell
*
*   DESCRIPTION
*
*       This function is used to delete a Shell session
*
*   INPUTS
*
*       p_shell - Shell session handle
*
*   OUTPUTS
*
*       NU_SUCCESS - Session created
*
*************************************************************************/
STATUS NU_Delete_Shell (NU_SHELL * p_shell)
{
    STATUS          status;
    NU_SUPERV_USER_VARIABLES


    NU_SUPERVISOR_MODE();

    /* Terminate the shell session task */
    status = NU_Terminate_Task(p_shell->shell_task);

    if (status == NU_SUCCESS)
    {
        /* Delete the shell session task */
        status = NU_Delete_Task(p_shell->shell_task);
    }

    if (status == NU_SUCCESS)
    {
        /* Remove rest of shell */
        status = Shell_Remove_Shell(p_shell);
    }

    NU_USER_MODE();

    /* Return status to caller */
    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       NU_Register_Command
*
*   DESCRIPTION
*
*       This function is used to register external commands with
*       SHELL.
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
*       NU_SUCCESS - Command was registered
*
*       NU_DUPLICATE_CMD - Duplicate command exists
*
*************************************************************************/
STATUS NU_Register_Command (NU_SHELL *  p_shell,
                            CHAR *      cmd,
                            STATUS      (*cmd_fcn) (NU_SHELL *, INT, CHAR **))
{
    STATUS              status;
    CS_NODE *           tmp_list_ptr;
    NU_SUPERV_USER_VARIABLES


    NU_SUPERVISOR_MODE();

    /* Check if p_shell is NU_NULL and register command with all created sessions if true */
    if (p_shell == NU_NULL)
    {
        /* Obtain mutex while traversing or manipulating created shell list */
        status = NU_Obtain_Semaphore(&Shell_Mutex, NU_SUSPEND);

        /* Ensure successful */
        if (status == NU_SUCCESS)
        {
            /* Get pointer to current head of created shell sessions list */
            tmp_list_ptr = Shell_Created_Shell_List;

            /* Loop through all created shell sessions in the list */
            /* NOTE: Keep looping if a duplicate command is detected to ensure all
                     shell sessions have a chance to register this command */
            while (tmp_list_ptr)
            {
                /* Get pointer to the current shell session */
                p_shell = (NU_SHELL *)tmp_list_ptr;

                /* Register command for this shell session */
                status = Shell_Register_Cmd(p_shell, cmd, cmd_fcn);

                /* Move to next shell session on the linked list */
                tmp_list_ptr = tmp_list_ptr -> cs_next;

                /* Check to see if back to head of list */
                if (tmp_list_ptr == Shell_Created_Shell_List)
                {
                    /* Break from loop */
                    break;
                }
            }   /* while */

            /* Register as a global command */
            (VOID)Shell_Register_Cmd(Shell_Global_Cmds, cmd, cmd_fcn);

            /* Release mutex */
            (VOID)NU_Release_Semaphore(&Shell_Mutex);

            /* Return success */
            status = NU_SUCCESS;
        }
    }
    else
    {
        /* Register the command with the specified session */
        status = Shell_Register_Cmd(p_shell, cmd, cmd_fcn);
    }

    NU_USER_MODE();

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       NU_Unregister_Command
*
*   DESCRIPTION
*
*      Remove command from system
*
*   INPUTS
*
*       p_shell - Shell session handle
*
*       cmd - String pointer of command to remove
*
*   OUTPUTS
*
*      NU_SUCCESS - Command was successfully removed
*
*      NU_UNAVAILABLE - Command was not found
*
*************************************************************************/
STATUS NU_Unregister_Command(NU_SHELL *  p_shell,
                             CHAR *      cmd)
{
    STATUS              status;
    CS_NODE *           tmp_list_ptr;
    NU_SUPERV_USER_VARIABLES


    NU_SUPERVISOR_MODE();

    /* Check to see if unregistering from all active sessions */
    if (p_shell == NU_NULL)
    {
        /* Obtain mutex while traversing or manipulating created shell list */
        status = NU_Obtain_Semaphore(&Shell_Mutex, NU_SUSPEND);

        /* Ensure mutex obtained */
        if (status == NU_SUCCESS)
        {
            /* Get pointer to current head of created shell sessions list */
            tmp_list_ptr = Shell_Created_Shell_List;

            /* Loop through all created shell sessions in the list */
            while (tmp_list_ptr)
            {
                /* Get pointer to the current shell session */
                p_shell = (NU_SHELL *)tmp_list_ptr;

                /* Unregister command for this shell session */
                (VOID)Shell_Remove_Cmd(p_shell, cmd);

                /* Move to next shell session on the linked list */
                tmp_list_ptr = tmp_list_ptr -> cs_next;

                /* Check to see if back to head of list */
                if (tmp_list_ptr == Shell_Created_Shell_List)
                {
                    /* Break from loop */
                    break;
                }
            }   /* while */

            /* Unregister as a global command */
            (VOID)Shell_Remove_Cmd(Shell_Global_Cmds, cmd);

            /* Release mutex */
            (VOID)NU_Release_Semaphore(&Shell_Mutex);

            /* Return success - even if some failures occurred, no way to convey this... */
            status = NU_SUCCESS;
        }
    }
    else
    {
        /* Delete this command */
        status = Shell_Remove_Cmd (p_shell, cmd);
    }

    NU_USER_MODE();

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       NU_Execute_Command
*
*   DESCRIPTION
*
*       Executes the specified command within the context of the
*       specified shell
*
*   INPUTS
*
*       p_shell - Shell session handle
*
*       cmd - Pointer to string with command to execute
*
*       echo - determines whether command and results of command
*              echoed to shell session terminal
*
*   OUTPUTS
*
*      NU_SUCCESS - Command issued to terminal
*
*      NU_INVALID_OPERATION - Invalid shell session
*
*************************************************************************/
STATUS NU_Execute_Command (NU_SHELL * p_shell, CHAR * cmd, BOOLEAN echo)
{
    STATUS      status = NU_INVALID_OPERATION;
    CHAR        cmd_str_lc[CFG_NU_OS_SVCS_SHELL_COLUMNS];
    VOID        (*io_puts)(NU_SHELL *, const CHAR *);
    INT         i;
    NU_SUPERV_USER_VARIABLES


    NU_SUPERVISOR_MODE();

    /* Ensure shell session not null */
    if (p_shell != NU_NULL)
    {
        /* Convert command to lower case to prevent case sensitivity. */
        SHELL_STRING_TO_LOWER_CASE(cmd, cmd_str_lc);

        /* Ensure command is not "quit" command (not allowed) */
        if (strncmp("quit", cmd_str_lc, sizeof("quit")) != 0)
        {
            /* Obtain session mutex */
            status = NU_Obtain_Semaphore(&p_shell->cmd_sem, NU_SUSPEND);

            /* Ensure mutex obtained */
            if (status == NU_SUCCESS)
            {
                /* Suspend task associated with this shell session to stop I/O processing */
                status = NU_Suspend_Task(p_shell->shell_task);

                /* Ensure shell session task suspended */
                if (status == NU_SUCCESS)
                {
                    /* Check if echo disabled */
                    if (echo == NU_FALSE)
                    {
                        /* Save I/O put string pointer in local variable */
                        io_puts = p_shell->session_io.io_puts;

                        /* Change shell session I/O put string pointer to dummy function */
                        p_shell->session_io.io_puts = shell_dummy_puts;
                    }
                    else
                    {
                        /* Remove any characters present on current input line */
                        for (i=0; i < strlen(p_shell->input_line[p_shell->input_line_active]);i++)
                        {
                            /* Delete character */
                            p_shell->session_io.io_puts(p_shell, "\b \b");
                        }

                        /* Echo command being executed to terminal */
                        p_shell->session_io.io_puts(p_shell, cmd_str_lc);
                    }

                    /* Release mutex */
                    (VOID)NU_Release_Semaphore(&(p_shell->cmd_sem));

                    /* Call internal function to process the command */
                    (VOID)Shell_Process_Cmd(p_shell, cmd_str_lc);

                    /* Check if echo disabled */
                    if (echo == NU_FALSE)
                    {
                        /* Restore I/O put string pointer for this shell session */
                        p_shell->session_io.io_puts = io_puts;
                    }
                    else
                    {
                        /* Print command prompt for after I/O task restarted */
                        p_shell->session_io.io_puts(p_shell, SHELL_PROMPT_STR);

                        /* Print any characters present on active command line */
                        p_shell->session_io.io_puts(p_shell,
                                                    p_shell->input_line[p_shell->input_line_active]);
                    }

                    /* Resume shell session task */
                    status = NU_Resume_Task(p_shell->shell_task);
                }
            }
        }
    }

    NU_USER_MODE();

    /* Return status to caller */
    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       NU_Shell_Puts
*
*   DESCRIPTION
*
*      Writes string to puts associated with the passed-in shell
*
*   INPUTS
*
*       p_shell - Shell session handle
*
*       str_ptr - Pointer to string
*
*   OUTPUTS
*
*      NU_SUCCESS - String sent
*
*      NU_INVALID_OPERATION - Indicates the API inactive
*
*************************************************************************/
STATUS NU_Shell_Puts(NU_SHELL *  p_shell,
                     CHAR *      str_ptr)
{
    STATUS      status = NU_SUCCESS;
    NU_SUPERV_USER_VARIABLES


    NU_SUPERVISOR_MODE();

    /* Ensure valid operation */
    if ((p_shell == NU_NULL) || (p_shell -> session_io.io_puts == NU_NULL))
    {
        /* ERROR: Invalid operation */
        status = NU_INVALID_OPERATION;
    }

    if (status == NU_SUCCESS)
    {
        /* Transmit data using this shell session's puts function */
        p_shell -> session_io.io_puts(p_shell,str_ptr);
    }

    NU_USER_MODE();

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       NU_Shell_Abort
*
*   DESCRIPTION
*
*      Sets flag in shell session that will cause shell session to abort /
*      quit upon processing of next command
*
*   INPUTS
*
*       p_shell - Shell session handle
*
*   OUTPUTS
*
*      NU_SUCCESS
*
*************************************************************************/
STATUS NU_Shell_Abort(NU_SHELL *  p_shell)
{
    NU_SUPERV_USER_VARIABLES


    NU_SUPERVISOR_MODE();

    /* Set flag in control block */
    p_shell->shell_abort = NU_TRUE;

    NU_USER_MODE();

    return (NU_SUCCESS);
}


/*************************************************************************
*
*   FUNCTION
*
*       NU_Shell_Set_Name
*
*   DESCRIPTION
*
*      Sets a string name for the specified shell that can be used to
*      more readily identify the shell session interface
*
*   INPUTS
*
*       p_shell         - Shell session handle
*       name            - Pointer to shell session name to be set
*
*   OUTPUTS
*
*      NU_SUCCESS           Pointers valid
*      NU_INVALID_POINTER   One of passed-in pointers NULL
*
*************************************************************************/
STATUS NU_Shell_Set_Name(NU_SHELL *  p_shell, CHAR * name)
{
    STATUS      status;
    NU_SUPERV_USER_VARIABLES


    NU_SUPERVISOR_MODE();

    /* Ensure pointers are valid */
    if ((p_shell != NU_NULL) && (name != NU_NULL))
    {
        /* Copy name into shell session control block */
        strncpy((VOID *)p_shell->name, (VOID *)name, CFG_NU_OS_SVCS_SHELL_NAME_LEN - 1);

        /* Ensure null terminated string set in control block */
        p_shell->name[CFG_NU_OS_SVCS_SHELL_NAME_LEN - 1] = 0;

        /* Set return value */
        status = NU_SUCCESS;
    }
    else
    {
        /* Set error value */
        status = NU_INVALID_POINTER;
    }


    NU_USER_MODE();

    return (status);
}


/*************************************************************************
*
*   FUNCTION
*
*       NU_Shell_Get_Name
*
*   DESCRIPTION
*
*      Gets the string name for the specified shell that can be used to
*      more readily identify the shell session interface
*
*   INPUTS
*
*       p_shell         - Shell session handle
*       name            - Pointer to return shell session name
*       max_name_len    - Maximum length of name returned to caller
*
*   OUTPUTS
*
*      NU_SUCCESS           Pointers valid
*      NU_INVALID_POINTER   One of passed-in pointers NULL
*
*************************************************************************/
STATUS NU_Shell_Get_Name(NU_SHELL *  p_shell, CHAR * name, INT max_name_len)
{
    STATUS      status;
    NU_SUPERV_USER_VARIABLES


    NU_SUPERVISOR_MODE();

    /* Ensure pointers are valid */
    if ((p_shell != NU_NULL) && (name != NU_NULL))
    {
        /* Copy name from shell session control block */
        strncpy((VOID *)name, (VOID *)p_shell->name, max_name_len - 1);

        /* Ensure name is null terminated */
        name[max_name_len - 1] = 0;

        /* Set return value */
        status = NU_SUCCESS;
    }
    else
    {
        /* Set error value */
        status = NU_INVALID_POINTER;
    }

    NU_USER_MODE();

    return (status);
}

#if (CFG_NU_OS_SVCS_SHELL_SERIAL_ENABLED == 1)
/*************************************************************************
*
*   FUNCTION
*
*       NU_Get_Shell_Serial_Session_ID
*
*   DESCRIPTION
*
*       This function returns the serial Shell session handle
*
*   INPUTS
*
*       p_shell_return - pointer to where Shell session handle is saved
*
*   OUTPUTS
*
*       NU_SUCCESS - Session handle returned
*
*************************************************************************/
STATUS NU_Get_Shell_Serial_Session_ID (NU_SHELL ** p_shell_return)
{
    NU_SUPERV_USER_VARIABLES


    NU_SUPERVISOR_MODE();

    /* Set return pointer */
    *p_shell_return = Shell_Serial_Session;

    NU_USER_MODE();

    /* Return success */
    return (NU_SUCCESS);
}
#else
/*************************************************************************
*
*   FUNCTION
*
*       NU_Get_Shell_Serial_Session_ID
*
*   DESCRIPTION
*
*       This function returns an error as the serial shell is disabled
*
*   INPUTS
*
*       p_shell_return - pointer to where Shell session handle is saved
*
*   OUTPUTS
*
*       NU_NO_SERIAL_SHELL  - No serial shell
*
*************************************************************************/
STATUS NU_Get_Shell_Serial_Session_ID (NU_SHELL ** p_shell_return)
{
    NU_SUPERV_USER_VARIABLES


    NU_SUPERVISOR_MODE();

    /* Set return pointer */
    *p_shell_return = NU_NULL;

    NU_USER_MODE();

    /* Return error */
    return (NU_NO_SERIAL_SHELL);
}
#endif  /* #if (CFG_NU_OS_SVCS_SHELL_SERIAL_ENABLED == 1) */
