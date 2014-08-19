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
*       nussh_api.c
*
*   COMPONENT
*
*       SSH Server
*
*   DESCRIPTION
*
*       This file contains definitions of all the publicly exposed
*       APIs for SSH Server.
*
*   FUNCTIONS
*
*       NUSSH_Start
*       NUSSH_Stop
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_kernel.h
*       nu_services.h
*
*************************************************************************/
#include "nussh_includes.h"
#include "session.h"

/* Task parameters */
#define SSHS_NORMAL_TASK_PRIORITY            (20)
#define SSHS_TASK_STACK_SIZE                 5000

/* SSH Server Errors */
#define NUSSH_ALREADY_RUNNING -300
#define NUSSH_NOT_RUNNING     -301

/* Define Application data structures.  */
NU_MEMORY_POOL          *SSHS_Mem_Pool;
NU_TASK                 *sshsrv_task;

extern int sshs_main(int argc, char ** argv);

/* GLOBALS */
extern STATUS  sshsrv_running;

/* SSH Server command line. */
static CHAR * sshsrv_argv[] = {"dropbear", "-F",
#ifdef DEBUG_TRACE
                        "-v"
#endif /*DEBUG_TRACE*/
};

/*************************************************************************
*
*   FUNCTION
*
*       NUSSH_Start
*
*   DESCRIPTION
*
*       Function to initiate SSH Server. If SSH Server is already running
*       returns an error.
*
*   INPUTS
*
*       VOID
*
*   OUTPUTS
*
*       NU_SUCCESS
*       NUSSH_ALREADY_RUNNING
*
*************************************************************************/
STATUS NUSSH_Start(VOID)
{

    STATUS         status;
    INT            sshsrv_argc;

    if(sshsrv_running){
        return NUSSH_ALREADY_RUNNING;
    }

    status = NU_System_Memory_Get(&SSHS_Mem_Pool, NU_NULL);

    if (status == NU_SUCCESS)
    {
        sshsrv_argc = sizeof(sshsrv_argv)/sizeof(CHAR*);
        status = NU_Create_Auto_Clean_Task(&sshsrv_task, "SSHSRVINIT",
                                (VOID (*)(UNSIGNED, VOID *))sshs_main,
                                sshsrv_argc, &sshsrv_argv, SSHS_Mem_Pool, SSHS_TASK_STACK_SIZE,
                                SSHS_NORMAL_TASK_PRIORITY, 0, NU_PREEMPT, NU_START);

    }

    return (status);
}

/*************************************************************************
*
*   FUNCTION
*
*       NUSSH_Stop
*
*   DESCRIPTION
*
*       This function stops a running SSH Server instance.
*
*   INPUTS
*       VOID
*
*   OUTPUTS
*       NU_SUCCESS
*       NUSSH_NOT_RUNNING
*
*************************************************************************/
STATUS NUSSH_Stop(VOID)
{
    STATUS ret_stat = NU_SUCCESS;

    if(sshsrv_running){

        /* if flag is set, someone has already triggered ssh shutdown. */
        if(exitflag != 1)
        {
            /* Set the exitflag for SSH Server task to trigger cleanup. */
            exitflag = 1;

            /* Resume the SSH task to act upon exitflag */
            NU_Resume_Task(sshsrv_task);
        }

    }
    else
        ret_stat = NUSSH_NOT_RUNNING;

    return ret_stat;
}
