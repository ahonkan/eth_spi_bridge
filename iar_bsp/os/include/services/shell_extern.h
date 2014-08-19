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
*       shell_extern.h
*                                                                      
*   COMPONENT                                                            
*                                                                      
*       Shell - External interface
*                                                                      
*   DESCRIPTION                                                          
*                                                                      
*       This file contains the C definitions for the component.                            
*                                                                      
*   DATA STRUCTURES                                                      
*                       
*       None
*               
*   FUNCTIONS
*
*       None
*
*   DEPENDENCIES
*                
*       nucleus.h
*                                                                      
*************************************************************************/

#ifndef SHELL_EXTERN_H
#define SHELL_EXTERN_H

#include "nucleus.h"
#include "kernel/nu_kernel.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* Nucleus Shell Error Codes */
#define NU_DUPLICATE_CMD                    -4251   /* Duplicate command */
#define NU_NO_SERIAL_SHELL                  -4252   /* No serial shell enabled */
#define NU_KILL_SHELL                       -4253   /* Ends processing of shell */

/* Type definition for an NU_SHELL session */
typedef struct NU_SHELL_STRUCT NU_SHELL;

/* I/O structure */
typedef struct SHELL_IO_STRUCT
{
    VOID *              io_session_info;
    STATUS              (*io_init)(NU_SHELL *);
    STATUS              (*io_deinit)(NU_SHELL *);
    VOID                (*io_puts)(NU_SHELL *, const CHAR *);
    CHAR                (*io_getch)(NU_SHELL *);
    VOID                (*io_special)(NU_SHELL *, CHAR);
    INT                 io_echo_on;

} SHELL_IO;

/* Shell Control Block */
struct NU_SHELL_STRUCT
{
    CS_NODE                 created_list;                       /* List of created shell sessions */
    struct SHELL_IO_STRUCT  session_io;                         /* Session I/O information */
    CHAR                    input_line[2][CFG_NU_OS_SVCS_SHELL_COLUMNS + 1];    /* Input lines (current and last) */
    CHAR                    name[CFG_NU_OS_SVCS_SHELL_NAME_LEN];/* Session name */
    UINT                    input_line_len;                     /* Length of current input line */
    UNSIGNED                input_line_active;                  /* Active input line index */
    CS_NODE *               cmd_list;                           /* List of registered commands */
    NU_SEMAPHORE            cmd_sem;                            /* Controls access to command list */
    UNSIGNED                cmd_count;                          /* Count of added commands */
    NU_TASK *               shell_task;                         /* Task that manages I/O and shell */
    BOOLEAN                 shell_abort;                        /* Flag used to indicate that shell session needs to abort / quit */
};

/* Nucleus Shell External Interface */
extern STATUS NU_Register_Command (NU_SHELL * shell,
                                   CHAR * cmd,
                                   STATUS (*cmd_fcn)(NU_SHELL *, INT, CHAR **));
extern STATUS NU_Unregister_Command  (NU_SHELL * shell, CHAR * cmd);
extern STATUS NU_Execute_Command (NU_SHELL * p_shell, CHAR * cmd, BOOLEAN echo);
extern STATUS NU_Create_Shell (NU_SHELL ** p_shell_return,
                               STATUS      (*io_init)(NU_SHELL *),
                               STATUS      (*io_deinit)(NU_SHELL *),
                               VOID        (*io_puts)(NU_SHELL *, const CHAR *),
                               CHAR        (*io_getch)(NU_SHELL *),
                               VOID        (*io_special)(NU_SHELL *, CHAR),
                               INT         io_echo_on);
extern STATUS NU_Delete_Shell (NU_SHELL * p_shell);
extern STATUS NU_Shell_Abort(NU_SHELL *  p_shell);
extern STATUS NU_Get_Shell_Serial_Session_ID (NU_SHELL ** p_shell_return);
extern STATUS NU_Shell_Puts(NU_SHELL * p_shell, CHAR * str_ptr);
extern STATUS NU_Shell_Set_Name(NU_SHELL *  p_shell, CHAR * name);
extern STATUS NU_Shell_Get_Name(NU_SHELL *  p_shell, CHAR * name, INT max_name_len);

#ifdef __cplusplus
}
#endif

#endif /* SHELL_EXTERN_H */
