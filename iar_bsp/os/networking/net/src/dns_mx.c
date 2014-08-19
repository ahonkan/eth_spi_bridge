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
*       dns_mx.c
*
*   DESCRIPTION
*
*       This file contains routines for resolving the MX server name of
*       a host name.
*
*   DATA STRUCTURES
*
*       None.
*
*   FUNCTIONS
*
*       NU_Get_Host_MX
*       DNS_Resolve_MX
*       DNS_Extract_Data_MX
*
*   DEPENDENCIES
*
*       nu_net.h
*       net_extr.h
*       dns4.h
*
************************************************************************/

#include "networking/nu_net.h"
#include "networking/net_extr.h"

#if (INCLUDE_IPV4 == NU_TRUE)
#include "networking/dns4.h"
#endif

extern DNS_SERVER_LIST  DNS_Servers;

extern INT  DNS_Unpack_Domain_Name(CHAR *, INT, CHAR *, CHAR *);

/* Function Prototypes. */
STATIC STATUS DNS_Resolve_MX(CHAR *, DNS_MX_RR *, UINT16, UINT16 *);

STATIC STATUS DNS_Extract_Data_MX(DNS_PKT_HEADER *, DNS_MX_RR *, UINT16,
                                  UINT16 *);

/****************************************************************************
*
*   FUNCTION
*
*       NU_Get_Host_MX
*
*   DESCRIPTION
*
*       This routine resolves the MX host name associated with a domain
*       name.  The user can then resolve those host names into IP
*       addresses using NU_Get_IP_Node_By_Name.
*
*   INPUTS
*
*       *name                   Pointer to the host name to resolve into
*                               one or more MX records.
*       *mx_records             Pointer to the memory in which to store
*                               the records.
*       *record_count           Upon calling the routine, this is the
*                               total number of DNS_MX_RR's that will
*                               fit in the user buffer.  Upon return, this
*                               value is filled in with the number of
*                               records filled into the buffer.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_NO_MEMORY            No memory.
*       NU_DNS_ERROR            DNS could not resolve.
*       NU_NO_DNS_SERVER        No DNS server exists.
*
******************************************************************************/
STATUS NU_Get_Host_MX(CHAR *name, DNS_MX_RR *mx_records, UINT16 *record_count)
{
    STATUS      status;

    NU_SUPERV_USER_VARIABLES

    NU_SUPERVISOR_MODE();

    /* Obtain the semaphore. */
    status = NU_Obtain_Semaphore(&DNS_Resource, NU_SUSPEND);

    if (status == NU_SUCCESS)
    {
        /* Query for MX records. */
        status = DNS_Resolve_MX(name, mx_records, DNS_TYPE_MX, record_count);

        NU_Release_Semaphore(&DNS_Resource);
    }

    /* Switch back to user mode. */
    NU_USER_MODE();

    return (status);

} /* NU_Get_Host_MX */

/*****************************************************************************
*
*   FUNCTION
*
*       DNS_Resolve_MX
*
*   DESCRIPTION
*
*       This routine handles domain based name lookup for MX queries.
*
*   INPUTS
*
*       *name                   Pointer to a host name.
*       *mx_records             Pointer to the memory in which to store
*                               the records.
*       type                    Type of query.
*       *record_count           Upon calling the routine, this is the
*                               total number of DNS_MX_RR's that will
*                               fit in the user buffer.  Upon return, this
*                               value is filled in with the number of
*                               records filled into the buffer.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful operation.
*       NU_NO_MEMORY            No memory.
*       NU_DNS_ERROR            DNS could not resolve.
*       NU_NO_DNS_SERVER        No DNS server exists.
*
******************************************************************************/
STATIC STATUS DNS_Resolve_MX(CHAR *name, DNS_MX_RR *mx_records, UINT16 type,
                             UINT16 *record_count)
{
    STATUS              stat;
    CHAR                *buffer;
    INT                 q_size, bytes_received;
    DNS_SERVER          *cur_server;
    struct addr_struct  dns_server;

    /* Is there at least one DNS Server. */
    if (memcmp(DNS_Servers.dnss_head->dnss_ip,
               "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", MAX_ADDRESS_SIZE) == 0)
    {
        return (NU_NO_DNS_SERVER);
    }

    /* Build the DNS query. */
    switch (type)
    {
#if (INCLUDE_IPV4 == NU_TRUE)
        case DNS_TYPE_MX :
#endif
            q_size = DNS_Build_Header((VOID **)&buffer, 1, DNS_RD);

            if (q_size)
            {
                q_size = sizeof(DNS_PKT_HEADER);

                q_size += DNS_Build_Query(name, 0, &buffer[sizeof(DNS_PKT_HEADER)],
                                          type, NU_FAMILY_IP, DNS_CLASS_IN,
                                          DNS_MAX_MESSAGE_SIZE - q_size);
            }

            if (q_size < 0)
                return (q_size);

            break;

#if (INCLUDE_IPV4 == NU_TRUE)
        case DNS_TYPE_CNAME :
#endif
            q_size = DNS_Build_Header((VOID **)&buffer, 1, DNS_RD);

            if (q_size)
            {
                q_size = sizeof(DNS_PKT_HEADER);

                q_size += DNS_Build_Query(name, 0, &buffer[sizeof(DNS_PKT_HEADER)],
                                          type, NU_FAMILY_IP, DNS_CLASS_IN,
                                          DNS_MAX_MESSAGE_SIZE - q_size);
            }

            if(q_size <0)
                return (q_size);

        default:

            return (NU_INVALID_PARM);
    }

    /* Get a pointer to the first server in the list */
    cur_server = DNS_Servers.dnss_head;

    /* While there are servers in the list to query */
    while (cur_server)
    {
        /* If the current server's IP address is NULL, it is the last
         * server in the list.  Break out of the loop.
         */
        if (memcmp(cur_server->dnss_ip, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
                   MAX_ADDRESS_SIZE) == 0)
            break;

        /* Set up the destination address structure. */
        dns_server.name = "DNSmx";
        dns_server.family = cur_server->family;
        dns_server.port = DNS_PORT;
        memcpy(dns_server.id.is_ip_addrs, cur_server->dnss_ip, MAX_ADDRESS_SIZE);

        /* Query the DNS server.  Upon returning the buffer will contain the
         * response to our query.
         */
        bytes_received = DNS_Query(buffer, q_size, &dns_server);

        if (bytes_received > 0)
            break;

        cur_server = cur_server->dnss_next;
    }

    /* If we stepped through every entry in the list or if we stepped through
       every used entry then return failure. */
    if ( (cur_server == NU_NULL) ||
         (memcmp(cur_server->dnss_ip, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
                 MAX_ADDRESS_SIZE) == 0) )
    {
        /* Return the buffer to the memory pool. */
        if (NU_Deallocate_Memory(buffer) != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to deallocate memory for buffer", NERR_SEVERE,
                           __FILE__, __LINE__);
        }

        /* Return error */
        return (NU_DNS_ERROR);
    }

        /* Now process the response. */
    switch (type)
    {
        case DNS_TYPE_MX :

            stat = DNS_Extract_Data_MX((DNS_PKT_HEADER *)buffer,
                                       mx_records, type, record_count);

            break;

        default:

            return (NU_INVALID_PARM);
    }

    /* Deallocate the memory buffer. */
    if (NU_Deallocate_Memory(buffer) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to deallocate memory for buffer", NERR_SEVERE,
                       __FILE__, __LINE__);
    }

    /* If an error occurred. */
    if (stat != NU_SUCCESS)
    {
        *record_count = 0;
    }

    return (stat);

} /* DNS_Resolve_MX */

/****************************************************************************
*
*   FUNCTION
*
*       DNS_Extract_Data_MX
*
*   DESCRIPTION
*
*       This function takes a DNS response for a MX or CNAME query and
*       extracts the required information.
*
*   INPUTS
*
*       *pkt                    A pointer to the DNS response.
*       *data                   A pointer to the buffer into which to
*                               put the address or name.
*       type                    The type of query made to get this answer.
*       record_count            Upon calling the routine, this is the
*                               total number of DNS_MX_RR's that will
*                               fit in the user buffer.  Upon return, this
*                               value is filled in with the number of
*                               records filled into the buffer.
*
*   OUTPUTS
*
*       NU_SUCCESS              Indicates success.
*       NU_DNS_ERROR            The server returned an error.
*       NU_MEM_ALLOC            No memory to allocate.
*
******************************************************************************/
STATIC STATUS DNS_Extract_Data_MX(DNS_PKT_HEADER *pkt, DNS_MX_RR *data,
                                  UINT16 type, UINT16 *record_count)
{
    DNS_MX_RR       *rr_ptr;
    INT             record_size = 0;
    INT             name_size, n_answers, i = 0;
    CHAR            *p_ptr;
    STATUS          answer_received = NU_DNS_ERROR;

    /* Get the number of answers in this message. */
    n_answers = GET16(pkt, DNS_ANCOUNT_OFFSET);

    /* Was an error returned? */
    if (DNS_RCODE_MASK & GET16(pkt, DNS_FLAGS_OFFSET))
    {
        return (NU_DNS_ERROR);
    }

    /* If there is at least one answer and this is a response, process it. */
    if ( (n_answers > 0) && (GET16(pkt, DNS_FLAGS_OFFSET) & DNS_QR) )
    {
        /* Point to where the question starts. */
        p_ptr = (CHAR *)(pkt + 1);

        /* Extract the name. */
        name_size = DNS_Unpack_Domain_Name(data[i].dns_name, DNS_MAX_NAME_SIZE, p_ptr,
                                           (CHAR *)pkt);

        /* Move the pointer past the name QTYPE and QCLASS to point at the
         * answer section of the response.
         */
        p_ptr += (name_size + DNS_MX_FIXED_HEADER_LEN);

        /* Process each answer. */
        while ( ((n_answers--) > 0) && (i < *record_count) )
        {
            /* Extract the name from the answer. */
            name_size = DNS_Unpack_Domain_Name(data[i].dns_name,
                                               DNS_MAX_NAME_SIZE, p_ptr,
                                               (CHAR *)pkt);

            /* Point to the resource record. */
            rr_ptr = (DNS_MX_RR *)p_ptr;

            /* Verify the type and class. */
            if ( (GET16(p_ptr, DNS_MX_TYPE_OFFSET) == type) &&
                 (GET16(p_ptr, DNS_MX_CLASS_OFFSET) == DNS_CLASS_IN) )
            {
                switch (type)
                {
                    case DNS_TYPE_MX :

                        /* Get the TTL. */
                        data[i].dns_ttl = GET32(rr_ptr, DNS_MX_TTL_OFFSET);

                        /* Get the preference. */
                        data[i].dns_preference =
                            GET16(rr_ptr, DNS_MX_PREF_OFFSET);

                        /* Get the MX server name. */
                        DNS_Unpack_Domain_Name(data[i].dns_mx,                                                                             DNS_MAX_NAME_SIZE,
                                               &p_ptr[DNS_MX_SRVR_OFFSET],
                                               (CHAR *)pkt);

                        /* Copy the length of this answer. */
                        data[i].dns_rdlength = GET16(rr_ptr, DNS_MX_LENGTH_OFFSET);

                        /* Calculate the total size of this record. */
                        record_size = (data[i].dns_rdlength +
                            DNS_MX_FIXED_RR_HDR_LEN + name_size);

                        /* Move to the next record space. */
                        i++;

                        break;

                    default :

                        return (NU_INVALID_PARM);
                }

                /* Indicate an answer was found. */
                answer_received = NU_SUCCESS;
            }

            /* Point to the next answer, if any. */
            p_ptr += record_size;
        }

        /* If at least one answer was successfully returned. */
        if (i > 0)
        {
            *record_count = i;
        }

        else
        {
            *record_count = 0;
        }
    }

    return (answer_received);

} /* DNS_Extract_Data_MX */
