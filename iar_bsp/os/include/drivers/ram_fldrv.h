/*************************************************************************/
/*                                                                       */
/*               Copyright 2007 Mentor Graphics Corporation              */
/*                         All Rights Reserved.                          */
/*                                                                       */
/* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  */
/* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS   */
/* SUBJECT TO LICENSE TERMS.                                             */
/*                                                                       */
/*************************************************************************/
/************************************************************************
* FILE NAME
*
*       ram_fldrv.h
*
* COMPONENT
*
*       Nucleus Safe Ramdisk Driver
*
* DESCRIPTION
*
*       RAM disk header file. 
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

#ifndef _RAM_FLDRV_H_
#define _RAM_FLDRV_H_

#include "storage/fsf.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SAFE_RAMDISK_SIZE (512*1024)

extern int  fs_mount_ramdrive(FS_VOLUMEDESC *vd,FS_PHYGETID phyfunc);
extern long fs_getmem_ramdrive(FS_PHYGETID phyfunc);
VOID        nu_os_drvr_safe_sra_init (const CHAR * key, INT startstop);

#ifdef __cplusplus
}
#endif

/****************************************************************************
 *
 * end of ram_fldrv.h
 *
 ***************************************************************************/

#endif	/* _RAM_FLDRV_H_ */
