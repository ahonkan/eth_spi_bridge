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
*       sck_shn.c
*
*   DESCRIPTION
*
*       This file contains the implementation of SCK_Set_Host_Name.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       SCK_Set_Host_Name
*       NU_Set_Device_Hostname
*
*   DEPENDENCIES
*
*       nu_net.h
*
************************************************************************/

#include "networking/nu_net.h"

extern UINT8            SCK_Host_Name[];
extern NU_PROTECT       SCK_Protect;

#if (INCLUDE_MDNS == NU_TRUE)
extern NU_SEMAPHORE     MDNS_Resource;
#endif

/*************************************************************************
*
*   FUNCTION
*
*       SCK_Set_Host_Name
*
*   DESCRIPTION
*
*       This function sets the name of the local host.
*
*   INPUTS
*
*       *name                   New host name
*       name_length             Length of the host name
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful completion of the service.
*       NU_INVALID_PARM         There is a problem with one of the
*                               parameters.  It is likely that name is a
*                               null pointer or the name_length is either
*                               0 or too small to hold  the complete
*                               hostname.
*
*************************************************************************/
INT SCK_Set_Host_Name(const CHAR *name, INT name_length)
{
    NU_SUPERV_USER_VARIABLES

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)

    /* Validate parameters. */
    if ( (name == NU_NULL) || (name_length <= 0) ||
         (name_length >= MAX_HOST_NAME_LENGTH) )
        return (NU_INVALID_PARM);
#endif

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Protect the SCK_Host_Name structure */
    NU_Protect(&SCK_Protect);

    /* Zero out the host name */
    UTL_Zero(SCK_Host_Name, MAX_HOST_NAME_LENGTH);

    /* Copy the new host name into the global */
    strcpy((CHAR*)SCK_Host_Name, name);

    /* Disable protection */
    NU_Unprotect();

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (NU_SUCCESS);

} /* SCK_Set_Host_Name */

/*************************************************************************
*
*   FUNCTION
*
*       NU_Set_Device_Hostname
*
*   DESCRIPTION
*
*       This function changes the host name of the interface associated
*       with the respective interface index.  The host name was configured
*       initially when the interface was initialized using the metadata
*       option "hostname" and the interface index. The interface's host
*       name is used only when mDNS is enabled to claim ownership of the
*       A and AAAA records for the respective IP address(es) assigned to
*       the interface and to answer A, AAAA and PTR queries for the host
*       name. If the interface has already been configured with an IP
*       address(es) before this routine is invoked, the respective local
*       DNS record(s) will be updated with the new host name, and probing and
*       announcing will be initiated.
*
*   INPUTS
*
*       *name                   New host name or NU_NULL to clear the host
*                               name.
*       dev_index               The index of the interface being configured.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful completion of the service.
*       NU_INVALID_PARM         There is no interface with a matching
*                               index.
*
*       Otherwise, an operating-system specific error is returned.
*
*************************************************************************/
STATUS NU_Set_Device_Hostname(CHAR *name, UINT32 dev_index)
{
    STATUS      status;
#if (INCLUDE_MDNS == NU_TRUE)
    CHAR        *name_ptr = NU_NULL;
#endif

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

#if (INCLUDE_MDNS == NU_TRUE)

    /* Get the mDNS semaphore. */
    status = NU_Obtain_Semaphore(&MDNS_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Get the DNS semaphore. */
        status = NU_Obtain_Semaphore(&DNS_Resource, NU_SUSPEND);

        if (status == NU_SUCCESS)
        {
            /* If the application is setting the name. */
            if (name)
            {
                /* Allocate memory for the new name. */
                status = NU_Allocate_Memory(MEM_Cached, (VOID**)&name_ptr,
                                            (UNSIGNED)strlen(name) + 1,
                                            (UNSIGNED)NU_SUSPEND);

                if (status == NU_SUCCESS)
                {
                    /* Copy the new name into the memory allocated for it. */
                    strcpy(name_ptr, name);
                }
            }

            if (status == NU_SUCCESS)
            {
                /* Change the host name, and update the DNS and mDNS modules. */
                status = MDNS_Set_Device_Hostname(name_ptr, dev_index);
            }

            NU_Release_Semaphore(&DNS_Resource);
        }

        NU_Release_Semaphore(&MDNS_Resource);
    }

#else

    /* mDNS must be included to configure a device's host name. */
    status = NU_INVALID_PARM;

#endif

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* NU_Set_Device_Hostname */
