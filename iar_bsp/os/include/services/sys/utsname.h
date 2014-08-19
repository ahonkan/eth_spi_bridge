/************************************************************************
*
*                  Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
*************************************************************************

*************************************************************************
* FILE NAME               
*
*       utsname.h                 
*
* COMPONENT
*
*		PX - POSIX.
*
* DESCRIPTION
*
*		Contains a data type.
*
* DATA STRUCTURES
*
*		utsname							Used for uname data structure.
*
* DEPENDENCIES
*
*		None.
*
************************************************************************/
#ifndef SYS_UTSNAME_H
#define SYS_UTSNAME_H

/* For DIAB tools */
#ifndef __Iutsname
#define __Iutsname

#define UTSNAMELEN 25

#ifdef __cplusplus
extern "C" {
#endif

    struct utsname
    {
        char sysname[UTSNAMELEN];
        char nodename[UTSNAMELEN];
        char release[UTSNAMELEN];
        char version[UTSNAMELEN];
        char machine[UTSNAMELEN];
    };

int uname (struct utsname *);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __Iutsname 	*/
#endif /* #ifndef SYS_UTSNAME_H */


