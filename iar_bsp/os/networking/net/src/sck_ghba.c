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
*       sck_ghba.c
*
* DESCRIPTION
*
*       This file contains the implementation of NU_Get_Host_By_Addr.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_Get_Host_By_Addr
*
* DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

extern NU_HOSTENT        *Saved_hp;

/*************************************************************************
*
*   FUNCTION
*
*       NU_Get_Host_By_Addr
*
*   DESCRIPTION
*
*       Given a host IP Address this function searches for the host in
*       the "hosts file".  A structure allocated by the calling
*       application is filled in with the host information.
*
*   INPUTS
*
*       *addr                   Pointer to the IP address of the host to
*                               find.
*       len                     The length of the address.  If family
*                               is NU_FAMILY_IP, len is 4.  If family
*                               is NU_FAMILY_IP6, len is 16.
*       type                    The type of address.  NU_FAMILY_IP or
*                               NU_FAMILY_IP6.
*       *host_entry             A pointer to a structure that will be
*                               filled in with the host information.
*
*   OUTPUTS
*
*       NU_SUCCESS              Success
*       NU_NO_MEMORY            No memory so host was not added
*       NU_DNS_ERROR            DNS could not resolve
*       NU_NO_DNS_SERVER        No DNS server exists
*       NU_INVALID_PARM         A passed in parameter is invalid.
*       NU_INVALID_LABEL        One of the parameters was a NULL pointer.
*
*************************************************************************/
STATUS NU_Get_Host_By_Addr(CHAR *addr, INT len, INT type, NU_HOSTENT *host_entry)
{
    STATUS  status;

    NU_SUPERV_USER_VARIABLES

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    if ( (!addr) || (!host_entry) || (
#if (INCLUDE_IPV4 == NU_TRUE)
         ((type == NU_FAMILY_IP) && (len != IP_ADDR_LEN))
#if (INCLUDE_IPV6 == NU_TRUE)
         ||
#endif
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
        ((type == NU_FAMILY_IP6) && (len != IP6_ADDR_LEN))
#endif
          ) )
    	return (NU_INVALID_PARM);

#endif

	/* Switch to supervisor mode. */
	NU_SUPERVISOR_MODE();

	/* Saved_hp is used to make NU_Get_IP_Node_By_Addr backward
	 * compatible with NU_Get_Host_By_Addr.  If the memory has not
	 * been freed, free it now, because it is going to be used.
	 */
	if (Saved_hp)
	{
		status = NU_Free_Host_Entry(Saved_hp);

		if (status == NU_SUCCESS)
			Saved_hp = NU_NULL;
	}

	/* If the memory was able to be deallocated */
	if (!Saved_hp)
		Saved_hp = NU_Get_IP_Node_By_Addr(addr, len, type, &status);

	/* If a matching record was found, fill in the user's host_entry */
	if (Saved_hp)
	{
		host_entry->h_name = Saved_hp->h_name;
		host_entry->h_alias = (CHAR**)NU_NULL;
		host_entry->h_addrtype = Saved_hp->h_addrtype;
		host_entry->h_length = Saved_hp->h_length;
		host_entry->h_addr_list = Saved_hp->h_addr_list;
	}

	/* Switch back to user mode. */
	NU_USER_MODE();

    return (status);

} /* NU_Get_Host_By_Addr */


