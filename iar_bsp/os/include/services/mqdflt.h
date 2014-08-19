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
*       mqdflt.h               
*
* COMPONENT
*
*		MQ - Message Queue
*
* DESCRIPTION
*
*		This file contains the internal routine used by the Message Queue
*		component.
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
#ifndef __MQDFLT_H_
#define __MQDFLT_H_

#ifdef __cplusplus
extern "C" {
#endif

#if (_POSIX_MESSAGE_PASSING	!=	-1)
void _posix_remove_message_queue(PPROC_THREAD_RES *psx_res,POSIX_MCB* mcb);
#endif

VOID MQ_Mqueue_Maps_Error_Values(STATUS errval);

#ifdef __cplusplus
}
#endif

#endif  /*  __MQDFLT_H_  */




