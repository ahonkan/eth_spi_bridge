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
*       port_s.h
*
* COMPONENT
*
*       Safe
*
* DESCRIPTION
*
*       Safe's port specific functions.
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

#ifndef _PORT_S_H_
#define _PORT_S_H_

#ifdef __cplusplus
extern "C" {
#endif

extern void fs_getcurrenttimedate(unsigned short *ptime, unsigned short *pdate);

extern int fs_mutex_get (FS_MUTEX_TYPE *);
extern int fs_mutex_put (FS_MUTEX_TYPE *);
extern int fs_mutex_create (FS_MUTEX_TYPE *);
extern int fs_mutex_delete (FS_MUTEX_TYPE *);

extern long fn_gettaskID(void);

#ifdef __cplusplus
}
#endif

/****************************************************************************
 *
 * end of port_s.h
 *
 ***************************************************************************/

#endif /* _PORT_S_H_ */

