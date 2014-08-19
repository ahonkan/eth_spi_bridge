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
*   FILENAME
*
*       gre.c
*
*   DESCRIPTION
*
*       This file contains the implementation of the Generic Routing
*       Encapsulation (GRE) protocol.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       GRE_Interpret
*
*   DEPENDENCIES
*
*       nu_net.h
*       pptp_api.h
*
*************************************************************************/

#include "networking/nu_net.h"

#if (INCLUDE_IPV4 == NU_TRUE)
#if (INCLUDE_GRE == NU_TRUE)

#include "pptp/inc/pptp_api.h"

/***********************************************************************
*
*   FUNCTION
*
*       GRE_Interpret
*
*   DESCRIPTION
*
*       Process received GRE datagrams.
*
*   INPUTS
*
*       *buf_ptr                A pointer to the NET buffer.
*       ip_source               The source IP address of the incoming
*                               packet.
*
*   OUTPUTS
*
*       NU_SUCCESS              Success
*       -1                      Failure
*
*************************************************************************/
STATUS GRE_Interpret(NET_BUFFER *buf_ptr, UINT32 ip_source)
{
    INT         version;
    UINT16      protocol;
    STATUS      status;

    /* Get the version of GRE packet. Nucleus NET currently only supports
     * GRE version 1 packets.
     */
    version = (GET8(buf_ptr->data_ptr, GRE_VERSION_BYTE_OFFSET) &
               GRE_VERSION_OFFSET);

    /* Get the GRE protocol field. */
    protocol = GET16(buf_ptr->data_ptr, GRE_PROTOCOL_OFFSET);

    /* If this is a supported version / protocol combination, process the
     * packet accordingly.
     */
    if ( (version == GRE_VERSION_ONE) &&
         (protocol == GRE_VERSION1_PROTOCOL) )
    {
        /* PPTP will process GRE version 1 packets. */
        status = PPTP_Data_Interpret(buf_ptr, ip_source);

        if (status != NU_SUCCESS)
        {
            NLOG_Error_Log("Failed to successfully interpret PPTP-GRE packet.",
                           NERR_SEVERE, __FILE__, __LINE__);
        }
    }
    else
    {
        NLOG_Error_Log("Cannot process incoming GRE packet - unsupported protocol or version.",
                       NERR_INFORMATIONAL, __FILE__, __LINE__);

        status = -1;

        /* Drop the packet by placing it back on the buffer_freelist. */
        MEM_Buffer_Chain_Free(&MEM_Buffer_List, &MEM_Buffer_Freelist);
    }

    return (status);

} /* GRE_Interpret */
#endif
#endif
