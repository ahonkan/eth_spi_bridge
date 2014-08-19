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
*       ptdflt.h               
*
* COMPONENT
*
*		TC - Thread Control.
*
* DESCRIPTION
*
*		Contains the internal POSIX thread related routines.
*
* DATA STRUCTURES
*
*		None
*
* DEPENDENCIES
*
*		None
*
************************************************************************/
#ifndef __PTDFLT_H_
#define __PTDFLT_H_


#define DETACHED            0x00000001

/* Function Declarations.  */
#ifdef __cplusplus
extern "C" {
#endif

void ptHandleDetach(unsigned long argc, void* argv);
int  _posix_thread_destructor(pid_t pid,pthread_t thread,VOID**   value_ptr);
void POSIX_Thread_Cancel_Run(pid_t pid,PPROC_THREAD_RES *psx_res,pthread_t  thread);
void task_delete( UNSIGNED argc, VOID *argv);

#ifdef __cplusplus
}
#endif

#endif  /*  __PTDFLT_H_  */




