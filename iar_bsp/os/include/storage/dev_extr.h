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
*       dev_extr.h
*                                                                       
* COMPONENT                                                             
*                                                                       
*       Device
*                                                                       
* DESCRIPTION                                                           
*                                                                       
*       External interface for device services.
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
#include "storage/dev_defs.h"

#ifndef DEV_EXTR_H
#define DEV_EXTR_H

#ifdef          __cplusplus
extern  "C" {                               /* C declarations in C++     */
#endif /* _cplusplus */

/* Prototypes for device management */
STATUS fs_dev_devname_to_fdev(CHAR *devname, FDEV_S **fdev);
STATUS fs_dev_set_fdev_flags(CHAR *devname, UINT32 flags);
STATUS fs_dev_get_fdev_flags(CHAR *devname, UINT32 *flags);

STATUS fs_dev_open_proc(UINT16 dh, CHAR *devname, VOID *args);
STATUS fs_dev_io_proc(UINT16 dh, UINT32 sector, VOID *buffer, UINT16 count, INT reading);
STATUS fs_dev_ioctl_proc(UINT16 dh, UINT16 command, VOID *buffer, INT ioctl_data_len);
STATUS fs_dev_dskchk_proc(UINT16 dh);
STATUS fs_dev_close_proc(UINT16 dh);
#ifdef          __cplusplus
}
#endif /* _cplusplus */

#endif /* DEV_EXTR_H */
