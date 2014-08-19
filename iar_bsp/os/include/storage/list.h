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
 *       list.h
 *
 * COMPONENT
 *
 *      LIST
 *  
 * DESCRIPTION
 *
 *      Structure definitions for listing
 *
 * DATA STRUCTURES
 *
 *      FS_LIST_S
 *      DEV_LIST_S
 *      MNT_LIST_S
 *
 * FUNCTIONS
 *
 *      None.
 *
 *************************************************************************/

/******************************  Definitions  ****************************/

#ifndef LIST_H
#define LIST_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */


/* All list_entry structures MUST start with pointers for
 * next and previous entries.
 */
typedef struct file_system_list_entry
{
    struct file_system_list_entry   *next;
    struct file_system_list_entry   *previous;
    const CHAR                      *fs_name;
}FS_LIST_S;

typedef struct device_list_entry
{
    struct device_list_entry        *next;
    struct device_list_entry        *previous;
    const CHAR                      *dev_name;
}DEV_LIST_S;

typedef struct mount_list_entry
{
    struct mount_list_entry *next;
    struct mount_list_entry *previous;
    const CHAR              *fs_name;
    const CHAR              *dev_name;
    const CHAR              *mnt_name;
    const VOID              *config;
}MNT_LIST_S;

#ifdef          __cplusplus
}
#endif /* _cplusplus */
#endif /* LIST_H */

