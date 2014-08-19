/*              Copyright 2013 Mentor Graphics Corporation
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
*       nu_fs.c
*
*   DESCRIPTION
*
*       This file defines functions of an OS compatibility layer
*       for the Nucleus User authentication.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/
#include "nussh_includes.h"

/* Replicating getpwnam call of *NIX. Does not set errno in process.
 * assumption: the return structure is a static memory and is assumed to
 * get free before next call to this function. */
struct passwd *nussh_getpwnam(const char *name)
{
	UM_USER_ELEMENT *nu_user;
	static struct passwd global_pw;

	if(name == NU_NULL)
		return 0;

	nu_user = UM_Scan_User(name);
	if(nu_user && (nu_user->um_pv & UM_SSH)) {
		global_pw.pw_uid = nu_user->um_id;
		global_pw.pw_gid = nu_user->um_id;
		global_pw.pw_name = nu_user->um_name;
		global_pw.pw_dir = "";
		global_pw.pw_shell = "";
		global_pw.pw_passwd = nu_user->um_pw;

		return &global_pw;
	}
	else
		return 0;
}

/* Our UM implementaion in NET does not keep passwd encrypted, so this
 * is just a stub that returns the same passwd as passed in. If UM gets
 * modified to keep encrypted passwd then this will FAIL. */
char *nussh_crypt(const char *key, const char *salt)
{
	static char output[21];

	if(strlen(key) >= sizeof(output))
	{
		TRACE(("The password received length is greater then buffer size."));
		return 0;
	}
	else
		return strcpy(output, key);
}
