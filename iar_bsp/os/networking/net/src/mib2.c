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
*        mib2.c
*
* COMPONENT
*
*        MIB II Statistics.
*
* DESCRIPTION
*
*        This file contains MIB2 Statistics routines Initialization.
*
* DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       MIB2_Init
*
*   DEPENDENCIES
*
*       nu_net.h
*       sys.h
*
*************************************************************************/
#include "networking/nu_net.h"

#if (INCLUDE_MIB2_RFC1213 == NU_TRUE)

#if (INCLUDE_SNMP == NU_TRUE)
#include "networking/sys.h"
#endif /* (INCLUDE_SNMP == NU_TRUE) */

#if (INCLUDE_SNMP == NU_TRUE)
extern UINT32               MIB2_If_Tbl_Last_Change;
#endif /* (INCLUDE_SNMP == NU_TRUE) */

/*************************************************************************
*
* FUNCTION
*
*        MIB2_Init
*
* DESCRIPTION
*
*        This function initializes the data structures used by MIB-II.
*
* INPUTS
*
*        None
*
* OUTPUTS
*
*        NU_SUCCESS
*
*************************************************************************/
STATUS MIB2_Init(VOID)
{
#if (MIB2_ICMP_INCLUDE == NU_TRUE)

    /* Initialize the ICMP group. */
    UTL_Zero(&Mib2_Icmp_Data, sizeof(MIB2_ICMP_STRUCT));

#endif /* (MIB2_ICMP_INCLUDE == NU_TRUE) */

#if (MIB2_IP_INCLUDE == NU_TRUE)

    /* Initialize the IP group. */
    UTL_Zero(&Mib2_Ip_Data, sizeof(MIB2_IP_STRUCT));

#endif /* (MIB2_IP_INCLUDE == NU_TRUE) */

#if ( (INCLUDE_TCP == NU_TRUE) && (MIB2_TCP_INCLUDE == NU_TRUE) )

    /* Initialize the TCP group. */
    UTL_Zero(&Mib2_Tcp_Data, sizeof(MIB2_TCP_STRUCT));

    MIB2_tcpRtoAlgorithm_Set(MIB2_VAN_JACOBSON);

#endif /* ( (INCLUDE_TCP == NU_TRUE) && (MIB2_TCP_INCLUDE == NU_TRUE) )*/

#if ( (INCLUDE_UDP == NU_TRUE) && (MIB2_UDP_INCLUDE == NU_TRUE) )

    /* Initialize the UDP group. */
    UTL_Zero(&Mib2_Udp_Data, sizeof(MIB2_UDP_STRUCT));

#endif /* ( (INCLUDE_UDP == NU_TRUE) && (MIB2_UDP_INCLUDE == NU_TRUE) )*/

    /* Updating ifTableLastChange, a variable representing last change
       in ifTable.  */
    MIB2_UPDATE_IF_TAB_LAST_CHNG;

    /* Return status. */
    return (NU_SUCCESS);

} /* MIB2_Init */

#endif  /* (INCLUDE_MIB2_RFC1213 == NU_TRUE) */

