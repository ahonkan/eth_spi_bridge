/*************************************************************************
*
*              Copyright 2010 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
*************************************************************************/

/*************************************************************************
*
*   FILE NAME
*
*       nu_fs.h
*
*   DESCRIPTION
*
*       This file defines an OS compatibility layer for Nucleus OS.
*
*   DATA STRUCTURES
*
*       None
*
*   DEPENDENCIES
*
*       nucleus.h
*       nu_net.h
*       pcdisk.h
*
************************************************************************/
#ifndef NUSSH_USERAUTH_H_
#define NUSSH_USERAUTH_H_
#include "nucleus.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * File API definitions and mappings to Nucleus equivalents.
 */
/* csgnu has its own definition in pwd.h */
#ifndef _PWD_H_
struct passwd {
	char	*pw_name;		/* user name */
	char	*pw_passwd;		/* encrypted password */
	uid_t	pw_uid;			/* user uid */
	gid_t	pw_gid;			/* user gid */
	char	*pw_comment;		/* comment */
	char	*pw_gecos;		/* Honeywell login info */
	char	*pw_dir;		/* home directory */
	char	*pw_shell;		/* default shell */
};
#endif
/*
 * File API definitions and mappings to Nucleus equivalents.
 */


/* Map file functions to the OS layer. */
#define getpwnam    nussh_getpwnam
#define crypt nussh_crypt

struct passwd *nussh_getpwnam(const char *name);
char *nussh_crypt(const char *key, const char *salt);

#ifdef __cplusplus
}
#endif

#endif /* NUSSH_USERAUTH_H_ */
