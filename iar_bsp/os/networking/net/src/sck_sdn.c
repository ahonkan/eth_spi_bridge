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
*       sck_sdn.c
*
*   DESCRIPTION
*
*       This file contains the implementation of SCK_Set_Domain_Name.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       SCK_Set_Domain_Name
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
*       SCK_Set_Domain_Name
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
INT SCK_Set_Domain_Name(const CHAR *name, INT name_length)
{
    NU_SUPERV_USER_VARIABLES

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    /* Validate parameters. */
    if ( (name == NU_NULL) || (name_length <= 0) ||
         (name_length >= NET_MAX_DOMAIN_NAME_LENGTH) )
    	return (NU_INVALID_PARM);
#endif

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

	/* Protect the SCK_Domain_Name structure */
	NU_Protect(&SCK_Protect);

	/* Zero out the domain name */
	UTL_Zero(SCK_Domain_Name, NET_MAX_DOMAIN_NAME_LENGTH);

	/* Copy the new domain name into the global */
	NU_BLOCK_COPY(SCK_Domain_Name, name, (unsigned int)name_length);

	/* Disable protection */
	NU_Unprotect();

    /* Return to user mode */
    NU_USER_MODE();

    return (NU_SUCCESS);

} /* SCK_Set_Domain_Name */
