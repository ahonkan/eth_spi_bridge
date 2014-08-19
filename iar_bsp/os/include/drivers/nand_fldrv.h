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
*       nand_fldrv.h
*
* COMPONENT
*
*       Nucleus Safe NAND Driver
*
* DESCRIPTION
*
*       NAND generic flash header.
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

#ifndef _NFLSHDRV_H_
#define _NFLSHDRV_H_

#include "storage/fsf.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FS_NAND_RESERVEDBLOCK 12 /* defines reserved block in nandflash  */

extern int fs_mount_nandflashdrive(FS_VOLUMEDESC *vd,FS_PHYGETID phyfunc);
extern long fs_getmem_nandflashdrive(FS_PHYGETID phyfunc);

#ifdef __cplusplus
}
#endif

/****************************************************************************
 *
 * end of nflshdrv.h
 *
 ***************************************************************************/

#endif	/* _NFLSHDRV_H_ */
