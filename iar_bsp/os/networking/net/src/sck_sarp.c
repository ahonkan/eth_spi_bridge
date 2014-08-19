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
*       sck_sarp.c
*
*   DESCRIPTION
*
*       This file contains the routine to create or modify
*       an entry in the arp cache
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Ioctl_SIOCSARP
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_ARP == NU_TRUE)

/*************************************************************************
*
*   FUNCTION
*
*       NU_Ioctl_SIOCSARP
*
*   DESCRIPTION
*
*       This function creates or modifies an entry in the arp cache
*
*   INPUTS
*
*       *option                 Return pointer for option status.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful completion of the service.
*       NU_INVALID_PARM         One of the input parameters is invalid.
*       NU_NO_MEMORY            A matching ARP entry could not be found,
*                               and a new entry could not be created.
*
*************************************************************************/
STATUS NU_Ioctl_SIOCSARP(const SCK_IOCTL_OPTION *option)
{
    STATUS  status;

    NU_SUPERV_USER_VARIABLES

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    /* If the user passed a NULL parameter, set status to error */
    if (option == NU_NULL)
    	return (NU_INVALID_PARM);

#endif

	/* Switch to supervisor mode. */
	NU_SUPERVISOR_MODE();

	status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

	if (status == NU_SUCCESS)
	{
		status = Ioctl_SIOCSARP(option);

		if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
			NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
						   __FILE__, __LINE__);
	}

	/* Switch back to user mode. */
	NU_USER_MODE();

    return (status);

} /* NU_Ioctl_SIOCSARP */

#endif
