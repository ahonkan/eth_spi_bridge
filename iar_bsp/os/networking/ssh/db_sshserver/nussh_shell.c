/*              Copyright 2013 Mentor Graphics Corporation
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
*       sshnu_shell.c
*
*   COMPONENT
*
*       SSH Server
*
*   DESCRIPTION
*
*       This file contains functionality for utilizing the shell component
*       for a ssh server session
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
#include "nussh_includes.h"
#include "chansession.h"

/* Local functions Definitions */
static VOID ssh_shell_signal_handler(UNSIGNED signals);

/*************************************************************************
*
*   FUNCTION
*
*       ssh_shell_init
*
*   DESCRIPTION
*
*       Function to init SSH I/O for shell
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
STATUS  ssh_shell_init(NU_SHELL * p_shell)
{
    nu_pty *nupty;
    /* !!! MGC: This need to be fixed. We are relying now on this because
     * we know that shell structure when created is all set to zero. Only
     * the parent task will set it to other value. */
    while(!p_shell->session_io.io_session_info)
        NU_Sleep(NU_PLUS_Ticks_Per_Second);

    nupty = p_shell->session_io.io_session_info;

    (VOID)NU_Shell_Set_Name(p_shell, "sshshell");

    /* Register signal handler. */
    NU_Register_Signal_Handler(ssh_shell_signal_handler);
    NU_Control_Signals(SIG_PSX_TO_NU(SIGTERM));

    /* Mark pty to have a shell task. */
    nupty->pty_flags |= NUPTY_SHELL;

    /* Return NU_SUCCESS */
    return (NU_SUCCESS);
}

/*************************************************************************
*
*   FUNCTION
*
*       ssh_shell_deinit
*
*   DESCRIPTION
*
*       Function to deinit ssh I/O for shell
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
STATUS  ssh_shell_deinit(NU_SHELL * p_shell)
{
    pid_t pid;
    struct exitinfo *exit = NULL;
    nu_pty *nupty = (nu_pty*)p_shell->session_io.io_session_info;

    /* Close tty end. */
    NU_Close_Socket(nupty->tty_sock);

    /* Signal ssh to close pty end. */
    if(nupty->pty_flags & NUPTY_PTY)
    {
        struct ChanSess *chansess = (struct ChanSess*)nupty->chan->typedata;

        pid = (INT)NU_Current_Task_Pointer();

        exit = &chansess->exit;
        exit->exitpid = pid;
        exit->exitstatus = -100;
        exit->exitsignal = SIGHUP;
    }

    /* Mark closing of tty. */
    nupty->pty_flags &= ~NUPTY_TTY;

    /* Mark closing of shell. */
    nupty->pty_flags &= ~NUPTY_SHELL;

    /* Return success */
    return (NU_SUCCESS);
}

/*************************************************************************
*
*   FUNCTION
*
*       ssh_shell_puts
*
*   DESCRIPTION
*
*       Function to put a string to ssh I/O
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
VOID  ssh_shell_puts(NU_SHELL * p_shell,
                            const CHAR * string_ptr)
{
    nu_pty *nupty = (nu_pty*)p_shell->session_io.io_session_info;

    /* Check status of call */
    if (NU_Send(nupty->tty_sock, (CHAR *)string_ptr, strlen(string_ptr), 0) < 0)
    {
        /* Conection with nupty has failed - signal abort on shell */
        (VOID)NU_Shell_Abort(p_shell);
    }
}

/*************************************************************************
*
*   FUNCTION
*
*       ssh_shell_getch
*
*   DESCRIPTION
*
*       Function to get a character from ssh I/O
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
CHAR  ssh_shell_getch(NU_SHELL * p_shell)
{
    CHAR rcv;
    nu_pty *nupty = (nu_pty*)p_shell->session_io.io_session_info;

    if (NU_Recv(nupty->tty_sock, &rcv, 1, 0) < 0)
    {
        /* Conection with nupty is lost - signal abort on shell */
        (VOID)NU_Shell_Abort(p_shell);
    }
    return rcv;

}

/*************************************************************************
*
*   FUNCTION
*
*       ssh_shell_special
*
*   DESCRIPTION
*
*       Function to handle special characters from ssh I/O
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
VOID ssh_shell_special(NU_SHELL * p_shell, CHAR ch)
{
    /* TODO: To be implemented. */
}

static VOID ssh_shell_signal_handler(UNSIGNED signals)
{
    nu_pty *nupty = 0;
    int i = 0;

    for(i=0; i < MAX_NU_PTY; i++)
    {
        if(nupty_table[i].shell->shell_task == NU_Current_Task_Pointer())
        {
            nupty = &nupty_table[i];
            break;
        }
    }

    if(!nupty)
        return;

    if(signals & SIG_PSX_TO_NU(SIGTERM))
        NU_Shell_Abort(nupty->shell);

    /* TODO: Other signal handlers. */

}

/* Returns the pointer to nupty structure for associated tty and Zero
 * if invalid tty specified.
 * @x tty: name of tty
 */
nu_pty* Get_NUPTY_by_Name(const char *tty)
{
    char             index_str[3];
    unsigned int     pty_index;

    /* Pull out the nupty in use from ptytable*/
    memcpy(index_str, &tty[6], 3);
    pty_index = atoi(index_str);

    return (pty_index < MAX_NU_PTY) ? &nupty_table[pty_index] : NU_NULL;
}

/* Returns -1 to indicate that we do not support setting terminal modes.*/
int nussh_tcgetattr(int fildes, struct termios *termios_p)
{
	return -1;
}
