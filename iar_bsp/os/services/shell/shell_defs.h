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
*       shell_defs.h
*
*   COMPONENT
*
*       Shell Service
*
*   DESCRIPTION
*
*       This file contains the C definitions for the component.
*
*   DATA STRUCTURES
*
*       CNS_CB_STRUCT
*
*   FUNCTIONS
*
*       nu_os_svcs_shell_init
*
*   DEPENDENCIES
*
*       None
*
*************************************************************************/

#ifndef SHELL_DEFS_H
#define SHELL_DEFS_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Prompt - The prompt string.  The default value is "SHELL:\\>" */
#define SHELL_PROMPT_STR                    CFG_NU_OS_SVCS_SHELL_PROMPT

/* Shell Task - The following parameters configure the shell task.  NOTE
   that registered commands run on this task, as well as the shell
   console interface. */
#define SHELL_STACK_SIZE                    ((NU_MIN_STACK_SIZE * 2) + (CFG_NU_OS_SVCS_SHELL_COLUMNS * 4) + CFG_NU_OS_SVCS_SHELL_ADDITIONAL_STACK_SIZE)
#define SHELL_TASK_PRIORITY                 4
#define SHELL_TASK_SLICE                    10

/* ASCII Manipulation */
#define SHELL_ASCII_LOWER_CASE_OFFSET       32

/* Input Line Other - Switch to the "other" input line (there are only
   two input lines total). */
#define SHELL_INPUT_LINE_OTHER(v)           (((v) + 1) % 2)

/* Shell Command Function */
typedef STATUS (*SHELL_CMD_FCN) (NU_SHELL *, INT, CHAR **);

/* Shell Command */
typedef struct SHELL_CMD_STRUCT
{
    CS_NODE         node;       /* Linked list node */
    CHAR *          str;        /* Command string */
    SHELL_CMD_FCN   fcn;        /* Command call-back function */

} SHELL_CMD;

/* Macro to convert string lower case */
#define SHELL_STRING_TO_LOWER_CASE(src, dst)                                    \
        {                                                                       \
            INT     i;                                                          \
                                                                                \
            /* Loop through entire string (including the NULL terminator). */   \
            for (i=0;i<=strlen(src);i++)                                        \
            {                                                                   \
                /* Convert characters from upper to lower case. */              \
                dst[i] = tolower((INT)src[i]);                                  \
            }                                                                   \
        }

/* Internal function prototypes */
extern ESAL_TS_WEAK_REF(VOID Shell_Banner(NU_SHELL * p_shell));
extern STATUS       Shell_Remove_Shell(NU_SHELL * p_shell);
extern VOID         Shell_Thread_Entry(UNSIGNED argc, VOID * argv);
extern STATUS       Shell_Register_Cmd(NU_SHELL *  p_shell,
                                       CHAR *      cmd,
                                       STATUS      (*cmd_fcn) (NU_SHELL *, INT, CHAR **));
extern STATUS       Shell_Remove_Cmd(NU_SHELL * p_shell, CHAR * cmd);
extern STATUS       Shell_Process_Cmd(NU_SHELL * p_shell, CHAR * p_input_line);
extern STATUS       Shell_Init_Struct(NU_SHELL ** p_shell, NU_MEMORY_POOL * mem_pool);

/* Internal variable prototypes */
extern NU_SHELL *   Shell_Serial_Session;
extern NU_SEMAPHORE Shell_Mutex;
extern CS_NODE *    Shell_Created_Shell_List;
extern NU_SHELL *   Shell_Global_Cmds;

#ifdef __cplusplus
}
#endif

#endif /* SHELL_DEFS_H */
