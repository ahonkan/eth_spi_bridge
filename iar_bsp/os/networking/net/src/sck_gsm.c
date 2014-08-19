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
*       sck_gsm.c
*
*   DESCRIPTION
*
*       This file contains the routine to get the subnet mask associated
*       with an IP address on a given interface.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Ioctl_SIOCGIFNETMASK
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
*       NU_Ioctl_SIOCGIFNETMASK
*
*   DESCRIPTION
*
*       This function retrieves the subnet mask associated with an IP
*       address of a specific interface.  If no IP address is specified,
*       the routine returns the subnet mask of the first IP address
*       in the list of addresses for the interface.
*
*   INPUTS
*
*       *option                 s_optval member holds the name of the
*                               interface, and s_ret.s_ipaddr holds the
*                               IP address on the interface for which to
*                               retrieve the subnet mask.  If no IP address
*                               is specified, the routine returns the
*                               subnet mask of the first IP address in
*                               the list of addresses for the interface.
*                               On return, s_ret.s_ipaddr holds the subnet
*                               mask.
*       optlen                  Specifies the size in bytes of the
*                               location pointed to by option
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful completion of the service.
*       NU_INVALID_PARM         One of the input parameters is invalid.
*
*************************************************************************/
STATUS NU_Ioctl_SIOCGIFNETMASK(SCK_IOCTL_OPTION *option, INT optlen)
{
    STATUS  status;

    NU_SUPERV_USER_VARIABLES

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    /* If the user passed a NULL parameter, set status to error */
    if ( (option == NU_NULL) || (option->s_optval == NU_NULL) ||
         (optlen < (INT)sizeof(SCK_IOCTL_OPTION)) )
    	return (NU_INVALID_PARM);

#endif

	/* Switch to supervisor mode. */
	NU_SUPERVISOR_MODE();

	status = NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND);

	if (status == NU_SUCCESS)
	{
		/* Get the subnet mask. */
		status = Ioctl_SIOCGIFNETMASK(option);

		if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
			NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
						   __FILE__, __LINE__);
	}

	/* Switch back to user mode. */
	NU_USER_MODE();

    return (status);

} /* NU_Ioctl_SIOCGIFNETMASK */
