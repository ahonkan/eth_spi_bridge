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
*       sck_gvprio.c
*
*   DESCRIPTION
*
*       This file contains the routine to get the VLAN priority of an
*       interface.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Ioctl_SIOCGETVLANPRIO
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
*       NU_Ioctl_SIOCGETVLANPRIO
*
*   DESCRIPTION
*
*       This function gets the VLAN priority of an interface.
*
*   INPUTS
*
*       *option                 Pointer that holds the interface name
*                               of the interface for which to retrieve
*                               the priority in the s_optval parameter
*                               of the structure.  Upon successful
*                               completion, the routine fills in the
*                               s_ret.vlan_prio member of the structure
*                               with the VLAN priority of the interface.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful completion of the service.
*       NU_INVALID_PARM         One of the input parameters is invalid.
*
*************************************************************************/
STATUS NU_Ioctl_SIOCGETVLANPRIO(SCK_IOCTL_OPTION *option)
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

	/* Obtain the TCP semaphore. */
	status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

	if (status == NU_SUCCESS)
	{
		/* Get the VLAN priority associated with the interface. */
		status = Ioctl_SIOCGETVLANPRIO(option);

		if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
		{
			NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
						   __FILE__, __LINE__);
		}
	}

	/* Switch back to user mode. */
	NU_USER_MODE();

    return (status);

} /* NU_Ioctl_SIOCGETVLANPRIO */
