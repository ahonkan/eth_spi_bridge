/*************************************************************************
*
*              Copyright 1993 Mentor Graphics Corporation
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
*       sck_gdn.c
*
*   DESCRIPTION
*
*       This file contains the implementation of SCK_Get_Domain_Name.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       SCK_Get_Domain_Name
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

extern UINT8 SCK_Domain_Name[];
extern NU_PROTECT   SCK_Protect;

/*************************************************************************
*
*   FUNCTION
*
*       SCK_Get_Domain_Name
*
*   DESCRIPTION
*
*       This function returns the name of the local domain.
*
*   INPUTS
*
*       *name                   Location to copy the host name
*       name_length             Size of this location
*
*   OUTPUTS
*
*       NU_SUCCESS              Success
*       NU_INVALID_PARM         One of the parameters is invalid
*
*************************************************************************/
INT SCK_Get_Domain_Name(CHAR *name, INT name_length)
{
    INT     x;
    INT     ret_status;

    NU_SUPERV_USER_VARIABLES

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    /* Validate parameters. */
    if ( (name == NU_NULL) || (name_length <= 0) )
    	return (NU_INVALID_PARM);

#endif

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

	/* Protect the SCK_Domain_Name structure */
	NU_Protect(&SCK_Protect);

	/* Loop, copying the name until the end of the name is reached
	   or until the end of the user supplied buffer is reached. */
	for (x = 0; ( (x < name_length) && (SCK_Domain_Name[x] != 0) ); x++)
		name[x] = (CHAR)(SCK_Domain_Name[x]);

	/* If there is room null terminate the name. */
	if (x < name_length)
	{
		name[x] = 0;
		ret_status = NU_SUCCESS;
	}

	/* If the null terminator was not reached then the entire
	   name was not copied to the user buffer. Report the
	   error. */
	else if (SCK_Domain_Name[x] != 0)
		ret_status = NU_INVALID_PARM;

	else
		ret_status = NU_SUCCESS;

	/* Disable protection */
	NU_Unprotect();

    /* Return to user mode */
    NU_USER_MODE();

    return (ret_status);

} /* SCK_Get_Domain_Name */
