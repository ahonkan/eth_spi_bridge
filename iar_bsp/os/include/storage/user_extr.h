/*************************************************************************/
/*                                                                       */
/*               Copyright 1993 Mentor Graphics Corporation              */
/*                         All Rights Reserved.                          */
/*                                                                       */
/* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  */
/* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   */
/* SUBJECT TO LICENSE TERMS.                                             */
/*                                                                       */
/*************************************************************************/
/*************************************************************************
* FILE NAME
*                                                                       
*       user_extr.h
*                                                                       
* COMPONENT                                                             
*                                                                       
*       User
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       Contains external interface for user services.
*                                                                       
* DATA STRUCTURES                                                       
*                                                                       
*       None.
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*       None.
*                                                                       
*************************************************************************/
#include "storage/user_defs.h"

#ifndef USER_EXTR_H
#define USER_EXTR_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */


INT                 NUFP_Current_Task_ID(VOID);
VOID                fsu_set_user_error(UINT32 file_errno);
UINT32              fsu_get_user_error(VOID);
PFILE_SYSTEM_USER   fs_current_user_structure(VOID);
VOID                pc_free_all_users(UINT16 dh);
STATUS              fsu_init_users (VOID);
UINT32              fsu_get_user_index(VOID);
UINT32              fsu_get_default_user_index(VOID);
INT                 fsu_deallocate_task_id(NU_TASK *task_ptr);
INT                 fsu_allocate_task_id(NU_TASK *task_ptr);
VOID                pc_memfill(VOID *vto, INT size, UINT8 c);



#ifdef          __cplusplus
}

#endif /* _cplusplus */


#endif /* USER_EXTR_H */
