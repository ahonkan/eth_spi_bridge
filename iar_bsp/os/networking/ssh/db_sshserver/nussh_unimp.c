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
*       nussh_unimp.c
*
*   DESCRIPTION
*
*       This file posix routines that have not been implemented.
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

int unimplemented(
#ifdef DEBUG_TRACE
		CHAR *file, INT line
#endif /*DEBUG_TRACE*/
		)
{
	#ifdef DEBUG_TRACE
		TRACE(("Unimplemented routine call, Line:%d File:%s", line, file))
	#endif

	return 0;
}
