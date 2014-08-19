/*************************************************************************
*
*              Copyright 2012 Mentor Graphics Corporation
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
*       sck_dnssd_b.c
*
*   DESCRIPTION
*
*       This file contains the routines to browse instances of a service
*       on the network.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_DNS_SD_Browse
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
*       NU_DNS_SD_Browse
*
*   DESCRIPTION
*
*       This routine is used to invoke a thread that will browse for
*       instances of a service on the local network.  When a new instance
*       of the service is found, or a change is identified in an already
*       known service, a signal will be sent to the application thread
*       that invoked this routine.  Therefore, the caller must have
*       registered a signal handler via NU_Register_Signal_Handler()
*       and enabled the control signal MDNS_SIGNAL via NU_Control_Signals().
*
*   INPUTS
*
*       action                  NU_DNS_START to start the service and
*                               NU_DNS_STOP to stop the service.
*       type                    The name of the service for which to
*                               browse.
*       domain                  The domain in which to browse for the
*                               service.  Only the domain local. is
*                               currently supported.
*
*   OUTPUTS
*
*       NU_SUCCESS              The browse session has been started.  The
*                               calling thread will be signaled when a
*                               new record for the query is found.
*       NU_INVALID_PARM         An input parameter is invalid.
*       NU_NO_ACTION            The caller's command to stop the service
*                               had no effect, because the system is not
*                               currently browsing for the service.
*       An operating system specific error is returned otherwise.
*
*************************************************************************/
STATUS NU_DNS_SD_Browse(INT action, CHAR *type, CHAR *domain)
{
    STATUS              status;

#if (INCLUDE_MDNS == NU_TRUE)
    CHAR                *data;

    NU_SUPERV_USER_VARIABLES

    /* Validate the input. */
#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
    if ( (type == NU_NULL) || (domain == NU_NULL) ||
         (strcmp(domain, "local") != 0) )
        return (NU_INVALID_PARM);
#endif

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Allocate memory for the concatenated name. */
    status = NU_Allocate_Memory(MEM_Cached, (VOID **)&data,
                                strlen(type) + strlen(domain) + 2, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Concatenate the type and domain. */
        strcpy(data, type);
        strcat(data, ".");
        strcat(data, domain);

        /* Start browsing the network for the indicated service. */
        if (action == NU_DNS_START)
        {
            /* Invoke the mDNS query. */
            status = NU_Start_mDNS_Query(data, DNS_TYPE_PTR, NU_FAMILY_UNSPEC, NU_NULL);
        }

        /* Stop browsing the network for the indicated service. */
        else if (action == NU_DNS_STOP)
        {
            /* Stop the mDNS query. */
            status = NU_Stop_mDNS_Query(data, DNS_TYPE_PTR, NU_FAMILY_UNSPEC, NU_NULL);
        }

#if (INCLUDE_NET_API_ERR_CHECK == NU_TRUE)
        else
        {
            status = NU_INVALID_PARM;
        }
#endif

        /* Deallocate the memory for the data. */
        NU_Deallocate_Memory(data);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

#else

    status = NU_INVALID_PARM;

#endif

    return (status);

} /* NU_DNS_SD_Browse */
