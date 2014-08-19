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
*       fd_extr.h
*                                                                       
* COMPONENT                                                             
*                                                                       
*       File Descriptor
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       External interface for file descriptor services.
*                                                                       
* DATA STRUCTURES                                                       
*                                                                       
*       None                                                           
*                                                                       
* FUNCTIONS                                                             
*                                                                       
*       None.
*                                                                       
*************************************************************************/
#include "storage/fd_defs.h"

#ifndef FD_EXTR_H
#define FD_EXTR_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */


/* Prototypes for file descriptor services */
STATUS  fd_init_fd_table(VOID);
INT     fd_allocate_fd(INT handle, MTE_S *mte);
INT     fd_handle_from_fd(INT fd);
VOID    fd_free_fd(INT fd);
STATUS  fd_free_fd_for_dh(INT dh);
MTE_S*  fd_mte_from_fd(INT fd);
STATUS  fd_get_fs_fd(INT vfs_fd,INT dh, INT *fd);



#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* FD_EXTR_H */
