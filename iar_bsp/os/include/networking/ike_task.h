/*************************************************************************
*
*               Copyright 2005 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       ike_task.h
*
* COMPONENT
*
*       IKE - Task
*
* DESCRIPTION
*
*       This file contains function prototypes needed to
*       implement the IKE task.
*
* DATA STRUCTURES
*
*       None
*
* DEPENDENCIES
*
*       None
*
*************************************************************************/
#ifndef IKE_TASK_H
#define IKE_TASK_H

#ifdef  __cplusplus
extern  "C" {                               /* C declarations in C++ */
#endif /* __cplusplus */

/**** Function prototypes. ****/

VOID IKE_Task_Entry(UNSIGNED argc, VOID *argv);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* IKE_TASK_H */
