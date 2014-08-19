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
*       udefs_s.h
*
* COMPONENT
*
*       Safe
*
* DESCRIPTION
*
*       User port definitions.
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

#ifndef _UDEFS_S_H_
#define _UDEFS_S_H_

#include "storage/file_cfg.h"
#include "storage/fsl_defs.h"
#include "storage/user_defs.h"

/****************************************************************************
 *
 * Open bracket for C++ Compatibility
 *
 ***************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

 
#ifndef USE_VFS
#define USE_VFS 1
#endif


/****************************************************************************
 *
 * volumes definitions
 *
 ***************************************************************************/

/* FS_MAXVOLUME should be set to n + 1 so format has an available temporary drive index */
#if(USE_VFS)
#define FS_MAXVOLUME    CFG_NU_OS_STOR_FILE_VFS_MAX_MOUNT_POINTS  /* maximum number of volumes */
#define FS_MAXTASK      VFS_NUM_USERS_WDU  /* maximum number of task + 1 for default user */
#define FS_CURRDRIVE    NO_DRIVE   /* setting the current drive at startup (-1 means no default current drive)*/
#else
#define FS_MAXVOLUME    10 /* maximum number of volumes */
#define FS_MAXTASK      10 /* maximum number of task */
#define FS_CURRDRIVE    0

#endif


#define FS_MAXPATH      EMAXPATH   /* This value can be changed to define the max path for the Safe file system,
                                      by default it is set to the VFS's max path. NOTE: VFS EMAXPATH, should always
                                      be greater than or equal to this value.*/


#define FS_MUTEX_TYPE NU_SEMAPHORE /* mutex type for locking mechanism */

/****************************************************************************
 *
 * Set FS_CASE_SENSITIVE to 1 if you want a case sensitive file system
 *
 ***************************************************************************/

#define FS_EFFS_CASE_SENSITIVE 0

/****************************************************************************
 *
 * Set SAFE_16BIT_CHAR to 1 if a char is 16bit length (e.g. on DSPs)
 *
 ***************************************************************************/

#define SAFE_16BIT_CHAR 0

/****************************************************************************
 *
 * Set CRCONFILES to 1 if files need crc protection at open and calculation
 * at close. Default state is 0.
 *
 ***************************************************************************/

#define	CRCONFILES 0

/****************************************************************************
 *
 * set USE_TASK_SEPARATED_CWD to 1 (default) if every task need separated
 * current working folder.
 *
 ***************************************************************************/

#define	USE_TASK_SEPARATED_CWD 1

/****************************************************************************
 *  if Unicode is used then comment in SAFE_UNICODE define
 *
 ***************************************************************************/

/* #define SAFE_UNICODE */ /* enable this line for unicode compatibility */

typedef unsigned short wchar; /* typedef for wide char */

/* define W_CHAR type according to SAFE_UNICODE */

#ifdef SAFE_UNICODE
#define W_CHAR wchar
#else
#define W_CHAR char
#endif

/* select system directory separator char */
/* the system will use it as default, but */
/* it also recognizes both incoming parameter */

#if 0
#define FS_SEPARATORCHAR '/'
#else
#define FS_SEPARATORCHAR '\\'
#endif

/* this define is needed for compatibility of CAPI */

#define F_SEPARATOR FS_SEPARATORCHAR

/****************************************************************************
 *
 * Last error usage
 *
 ***************************************************************************/

#if 1
	#if(USE_VFS)
		/* simple assignment */
		#define F_SETLASTERROR(ec) (*(fm->lasterror)=(ec))
		#define F_SETLASTERROR_NORET(ec) (*(fm->lasterror)=(ec))
	#else
		#define F_SETLASTERROR(ec) (fm->lasterror=(ec))
		#define F_SETLASTERROR_NORET(ec) (fm->lasterror=(ec))
	#endif
#elif 0
/* function calls used for it */
#define F_SETLASTERROR(ec) fg_setlasterror(fm,ec)
#define F_SETLASTERROR_NORET(ec) fg_setlasterror_noret(fm,ec)
#elif 0
/* no last error is used (save code space) */
#define F_SETLASTERROR(ec) (ec)
#define F_SETLASTERROR_NORET(ec) 
#endif

/****************************************************************************
 *
 * Set FSF_MOST_FREE_ALLOC to 1 if most free block allocation is used <default>
 * in flash drivers for allocating a new sector.
 *
 ***************************************************************************/

#define FSF_MOST_FREE_ALLOC 1

#if(USE_VFS)
/* This value serves as a constant. This value isn't configurable. */
#define SAFE_RD_SEC_PER_CL 1
#endif

/****************************************************************************
 *
 * Close bracket for C++
 *
 ***************************************************************************/

#ifdef __cplusplus
}
#endif

/****************************************************************************
 *
 * end of udefs_s.h
 *
 ***************************************************************************/

#endif /* _UDEFS_S_H_ */

