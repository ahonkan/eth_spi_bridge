/*************************************************************************
*
*              Copyright 2013 Mentor Graphics Corporation
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
*       sshnu_shell.h
*
*   DESCRIPTION
*
*       This file defines an shell layer for dropbear SSH Server.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_net.h
*       pcdisk.h
*
************************************************************************/
#ifndef NUSSH_SHELL_H_
#define NUSSH_SHELL_H_

#ifdef __cplusplus
extern "C" {
#endif


typedef struct __nu_pty {
	CHAR pty_name[12];
	INT pty_sock;
	INT tty_sock;
	INT pty_flags;
	NU_SHELL *shell;
	struct Channel *chan;
}nu_pty;
extern nu_pty nupty_table[];

/* Maxinum number of nu-ptys*/
#define MAX_NU_PTY 1

/* These flags define the status of nu_pty */
#define NUPTY_FREE  0x00000000
#define NUPTY_PTY   0x00000001 /* pty socket exist */
#define NUPTY_TTY   0x00000002 /* tty socket exist */
#define NUPTY_SHELL 0x00000004 /* associated with nu shell. */

/* Convert posix signal to Nucleus signal.
 * @x x: POSIX signal. */
#define SIG_PSX_TO_NU(x) (0x01 << x)

/* Returns the index in nupty_table for the passed in nu_pty
 * @x nu_pty
 */
#define NUPTY_TBL_INDEX(x) (x - nupty_table)/sizeof(nu_pty)

#define tcgetattr    nussh_tcgetattr

/*
 * API definitions.
 */

STATUS  ssh_shell_init(NU_SHELL * p_shell);
STATUS  ssh_shell_deinit(NU_SHELL * p_shell);
VOID  ssh_shell_puts(NU_SHELL * p_shell, const CHAR * string_ptr);
CHAR  ssh_shell_getch(NU_SHELL * p_shell);
VOID ssh_shell_special(NU_SHELL * p_shell, CHAR ch);
nu_pty* Get_NUPTY_by_Name(const char *tty);
int nussh_tcgetattr(int fildes, struct termios *termios_p);

#ifdef __cplusplus
}
#endif

#endif /* NUSSH_SHELL_H_ */
