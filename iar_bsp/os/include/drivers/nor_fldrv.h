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
*       nor_fldrv.h
*
* COMPONENT
*
*       Nucleus Safe NOR Driver
*
* DESCRIPTION
*
*       NOR generic flash driver.
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

#ifndef _FLASHDRV_H_
#define _FLASHDRV_H_

#include "storage/fsf.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int fs_mount_flashdrive(FS_VOLUMEDESC *vd,FS_PHYGETID phyfunc);
extern long fs_getmem_flashdrive(FS_PHYGETID phyfunc);

#ifdef __cplusplus
}
#endif

/****************************************************************************
 *
 * end of flashdrv.h
 *
 ***************************************************************************/

#endif	/* _FLASHDRV_H_ */
