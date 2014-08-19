/************************************************************************
*
*                  Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
*************************************************************************

*************************************************************************
* FILE NAME
*
*       posixinit.h
*
* COMPONENT
*
*       PX - POSIX
*
* DESCRIPTION
*
*       Contains the various POSIX Internal initialization routines.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       "nucleus.h"                         Contains various Nucleus PLUS
*                                           related definitions.
*
************************************************************************/
#ifndef __POSIXINIT_H_
#define __POSIXINIT_H_

#include "services/config.h"

/* Function Declarations.  */

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize the the POSIX layer for NET */
INT posix_net_init(pid_t pid);

/* POSIX Release Information */
char  *POSIX_Release_Information(void);

/* Thread related initializations.  */
int thread_init(pid_t pid);

/* Timer related initializations.  */
int timer_init(pid_t pid);

/* Signal related initializations.  */
int sig_init(pid_t pid);

/* Semaphore related initializations.  */
int semaphore_init(pid_t pid);

/* Message Queue related initializations.  */
int mq_init(pid_t pid);

/* Condition Variable related definitions */
int cond_init(pid_t pid);

/* Initialize the the POSIX layer for file */
int posix_file_init(int driveno);

/* Initialize the devices for the POSIX Layer */
int devices_init(void);

/* Initialize the POSIX layer for handling interprocessor communication.*/
int pipc_init(void);

/* Initialize the descriptor table to be used by POSIX. */
int desc_init(pid_t pid);

/* Initialize the environment variable */
int initenv(pid_t pid);

/* Initialize the  POSIX layer.  */
int posix_init(void);

#ifdef __cplusplus
}
#endif

#endif  /*  __POSIXINIT_H_  */
