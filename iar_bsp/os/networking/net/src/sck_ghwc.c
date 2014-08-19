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
*       sck_ghwc.c
*
*   DESCRIPTION
*
*       This file contains the routine to retrieve the hardware
*       offloading capabilities of the specific interface.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Ioctl_SIOCGHWCAP
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/*************************************************************************
*
*   FUNCTION
*
*       NU_Ioctl_SIOCGHWCAP
*
*   DESCRIPTION
*
*       This function retrieves the hardware offloading capabilities of
*       an interface.
*
*   INPUTS
*
*       *option                 Return pointer for option status.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful completion of the service.
*       NU_INVALID_PARM         The specified interface does not exist
*                               in the system.
*
*************************************************************************/
STATUS NU_Ioctl_SIOCGHWCAP(SCK_IOCTL_OPTION *option)
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
		/* Determine which hardware offloading options are supported
		 * on the interface.
		 */
		status = Ioctl_SIOCGHWCAP(option);

		if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
			NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
						   __FILE__, __LINE__);
	}

	/* Switch back to user mode. */
	NU_USER_MODE();

    return (status);

} /* NU_Ioctl_SIOCGHWCAP */
