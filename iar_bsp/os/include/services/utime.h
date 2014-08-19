/*************************************************************************
*
*                  Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
*************************************************************************/
/*************************************************************************
*
*   FILE NAME
*
*       utime.h                
*
*   COMPONENT
*
*		PX - POSIX
*
*   DESCRIPTION
*
*		This file contains the definitions for the access and modification
*		time structure.
*
*   DATA STRUCTURES
*
*		time_t
*		utimbuf
*
*   DEPENDENCIES
*
*		config.h
*		compiler.h
*		types.h
*
*************************************************************************/

#ifndef NU_PSX_UTIME_H
#define NU_PSX_UTIME_H

#include "services/config.h"
#include "services/compiler.h"
#include "services/sys/types.h"

/* For Metaware Metrowerks and KMC GNU Tools.  */
#ifndef _UTIME_H
#define _UTIME_H

/* For ADS Tools.  */
#ifndef __utime_h
#define __utime_h

/* For Hitachi Tools and TI Tools.  */
#ifndef _UTIME
#define _UTIME

/* For Paradigm Tools and Microtec Tools.  */
#ifndef __UTIME_H
#define __UTIME_H

/* For Microsoft Visual C.  */
#ifndef _INC_UTIME
#define _INC_UTIME

#ifndef __UTIME_H_
#define __UTIME_H_

/* For MinGNU or other GNU toolsets  */
#ifndef _UTIME_H_
#define _UTIME_H_

/* For DIAB tools */
#ifndef __Iutime
#define __Iutime

/* For Sourcery CodeBench CSLibc and Newlib C library */
#ifndef _SYS_UTIME_H
#define _SYS_UTIME_H

/* Time buffer structure */
struct utimbuf
{

	time_t	actime;							/* Access time. */
	time_t	modtime;						/* Modification time. */
};

#ifdef __cplusplus
extern "C" {
#endif /*  __cplusplus  */

/* Set file access and modification times. */
int utime(const char *path, const struct utimbuf *times);

#ifdef __cplusplus
}
#endif  /*  __cplusplus  */

#endif /* _SYS_UTIME_H */
#endif /* _UTIME_H_ */
#endif /* __Iutime */
#endif /* _UTIME_H */
#endif /* __UTIME_H_ */
#endif /* _INC_UTIME */
#endif /* __UTIME_H */
#endif /* _UTIME */
#endif /* __utime_h */

#endif /* NU_PSX_UTIME_H */
