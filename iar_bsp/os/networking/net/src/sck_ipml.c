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
* FILE NAME
*
*       sck_ipml.c
*
* DESCRIPTION
*
*       This file contains the API for joining and leaving a multicast
*         group
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_IP_Multicast_Listen
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

/*************************************************************************
*
*   FUNCTION
*
*       NU_IP_Multicast_Listen
*
*   DESCRIPTION
*
*       This function is used to join or leave a multicast group.  This
*       function is the API function that has been outlined in RFC 3376,
*       IGMPv3.
*
*   INPUTS
*
*       socketd                 Specifies a socket descriptor
*       *interface_addr         Ptr to the IP addr of the device to use
*       *multi_addr             Ptr to the address of the multicast group
*       filter_mode             Filter mode for the socket
*                                   (INCLUDE or EXCLUDE)
*       source_list             Ptr to the list of source addresses that
*                                   should be INCLUDED or EXCLUDED
*                                   depending on the filter mode
*       num_source_addr         Number of source addresses in the source_list
*
*   OUTPUTS
*
*       NU_SUCCESS                  Indicates the socket option was set
*                                       successfully.
*       NU_INVALID_SOCKET           The passed-in socket descriptor is
*                                       invalid.
*       NU_INVALID_PARM             One of the passed-in parameters is
*                                       invalid.
*
*************************************************************************/
STATUS NU_IP_Multicast_Listen(INT socketd, UINT8 *interface_addr,
                              UINT8 *multi_addr, UINT16 filter_mode,
                              const UINT8 *source_list, UINT16 num_source_addr)
{
    STATUS      ret_status;

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    /* check validity of socket descriptor */
    if ( (socketd < 0) || (socketd >= NSOCKETS) )
        return (NU_INVALID_SOCKET);

    /* Check the validity of the passed in parms */
    if ( (interface_addr == NU_NULL) || (multi_addr == NU_NULL) ||
         (num_source_addr > MAX_MULTICAST_SRC_ADDR) ||
         ((num_source_addr != 0) && (source_list == NU_NULL)) )
    	return (NU_INVALID_PARM);
#endif

	/* Obtain the semaphore and validate the socket */
	ret_status = SCK_Protect_Socket_Block(socketd);

	if (ret_status == NU_SUCCESS)
	{
		/* Call the function to process multicast request */
		ret_status = IP_Process_Multicast_Listen(socketd,
												 interface_addr,
												 multi_addr,
												 filter_mode,
												 source_list,
												 num_source_addr);

		/* Release the semaphore */
		SCK_Release_Socket();
	}

    return (ret_status);

} /* NU_IP_Multicast_Listen */

