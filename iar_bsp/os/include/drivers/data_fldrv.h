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
*       data_fldrv.h
*
* COMPONENT
*
*       Nucleus Safe Dataflash Driver
*
* DESCRIPTION
*
*       DataFlash generic handler header.
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

#ifndef _DFDRV_H_
#define _DFDRV_H_

#include "storage/fsf.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int fs_mount_dataflashdrive(FS_VOLUMEDESC *vd,FS_PHYGETID phyfunc);
extern long fs_getmem_dataflashdrive(FS_PHYGETID phyfunc);

#ifdef __cplusplus
}
#endif

/****************************************************************************
 *
 * end of dfdrv.h
 *
 ***************************************************************************/

#endif	/* _DFDRV_H_ */
