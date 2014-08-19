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
*       fstaticw.h
*
* COMPONENT
*
*       Safe
*
* DESCRIPTION
*
*       Safe's static wear header.
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

#ifndef _FSTATICW_H_
#define _FSTATICW_H_

#ifdef __cplusplus
extern "C" {
#endif

#define FS_STATIC_DISTANCE  1024	/* distance between statically used and free block */
#define FS_STATIC_PERIOD	1024	/* period, when to check static distance */

extern int fsm_staticwear(int drvnum);  /* for multitask */

#define fs_staticwear(drvnum) fsm_staticwear(drvnum)

#ifdef __cplusplus
}
#endif

/****************************************************************************
 *
 * End of fstaticw.h
 *
 ***************************************************************************/

#endif /* _FSTATICW_H_ */


