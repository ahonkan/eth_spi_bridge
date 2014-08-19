/************************************************************************
*
*                  Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
*************************************************************************/
/*************************************************************************
*
*   FILE NAME
*
*       psx_extr.h
*
*   COMPONENT
*
*       POSIX - External Interface
*
*   DESCRIPTION
*
*       The following contains the Nucleus POSIX external interface.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       nucleus.h
*       psx_defs.h
*       pthread.h
*       select.h
*
************************************************************************/

#ifndef PSX_EXTR
#define PSX_EXTR

#include "os/include/nucleus.h"
#include "services/pthread.h"

#ifdef CFG_NU_OS_SVCS_POSIX_NET_ENABLE

#include "services/sys/select.h"

#endif /* CFG_NU_OS_SVCS_POSIX_NET_ENABLE */

#ifdef __cplusplus
extern "C" {
#endif

/* POSIX Core */

/* Nucleus PLUS support */
INT     NU_Posix_Register_Plus_Task(pthread_t * thread);
INT     NU_Posix_Register_Plus_Task2(pthread_t * thread, NU_TASK * task_ptr);
VOID    NU_Posix_Exit_Plus_Task(VOID);
VOID    NU_Posix_Exit_Plus_Task2(NU_TASK * task_ptr);
STATUS  nu_os_svcs_posix_core_init_early(VOID);

/* Legacy API support */
#define NU_POSIX_Reg_Thread     NU_Posix_Register_Plus_Task
#define NU_POSIX_Thread_End     NU_Posix_Exit_Plus_Task

#ifdef CFG_NU_OS_SVCS_POSIX_FS_ENABLE

/* POSIX File System */

#endif /* CFG_NU_OS_SVCS_POSIX_FS_ENABLE */

#ifdef CFG_NU_OS_SVCS_POSIX_NET_ENABLE

/* POSIX Networking */

#endif /* CFG_NU_OS_SVCS_POSIX_NET_ENABLE */

#ifdef CFG_NU_OS_SVCS_POSIX_RTL_ENABLE

/* POSIX RTL */

#endif /* CFG_NU_OS_SVCS_POSIX_RTL_ENABLE */

#ifdef CFG_NU_OS_SVCS_POSIX_AIO_ENABLE

/* POSIX Asynchronous IO */

#endif /* CFG_NU_OS_SVCS_POSIX_AIO_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* PSX_EXTR */
