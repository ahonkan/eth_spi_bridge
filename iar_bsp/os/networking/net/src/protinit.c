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
*       protinit.c
*
*   COMPONENT
*
*       PROT
*
*   DESCRIPTION
*
*       Initialize the various Nucleus Net protocols.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       PROT_Protocol_Init
*
*   DEPENDENCIES
*
*       nu_net.h
*       ipraw.h
*       resolve6.h
*       tcp6.h
*       udp6.h
*       dad6.h
*
*************************************************************************/

#include "networking/nu_net.h"
#include "networking/ipraw.h"
#include "networking/um_defs.h"

#if (INCLUDE_SNMP == NU_TRUE)
#include SNMP_GLUE
#endif

#if (INCLUDE_IPV6 == NU_TRUE)
#include "networking/resolve6.h"
#include "networking/tcp6.h"
#include "networking/udp6.h"
#include "networking/dad6.h"
#endif

/* Import the global function pointer that lets NET pass
   packets to SPAN. */
extern UINT32 (*span_process_packet) (UINT8 *, UINT32, UINT32);

/* Import PPPoE input function pointer for initialization. */
extern VOID   (*ppe_process_packet)(UINT16);

extern NU_PROTECT   SCK_Protect;

/************************************************************************
*
*   FUNCTION
*
*       PROT_Protocol_Init
*
*   DESCRIPTION
*
*       Calls all the other packet initialization keep this order as some
*       packet inits require lower layers already be initialized.
*
*   INPUTS
*
*       None.
*
*   OUTPUTS
*
*       NU_SUCCESS              Successful
*       NU_MEM_ALLOC            Memory allocation failed
*
*************************************************************************/
STATUS PROT_Protocol_Init(VOID)
{
#if (INCLUDE_SOCKETS == NU_TRUE)
    INT         i;

    for (i = 0; i < NSOCKETS; i++)
        SCK_Sockets[i] = NU_NULL;
#endif

    /* Zero the SPAN function pointer. When, and if, SPAN is
       initialized it will setup this pointer to the correct
       function. */
    span_process_packet = NU_NULL;

    /* Zero the PPPoE input function pointer. This will be initialized
       within the PPPoE library if it is included. */
    ppe_process_packet = NU_NULL;

    /* Zero out the Sockets protection data structure */
    UTL_Zero(&SCK_Protect, sizeof(NU_PROTECT));

#if ( (INCLUDE_DHCP == NU_TRUE) || (INCLUDE_DHCP6 == NU_TRUE) )

    /* Initialize all fields to zero. */
    UTL_Zero(&DHCP_Duid, sizeof(DHCP_DUID_STRUCT));

#if (INCLUDE_DHCP_DUID_EN == NU_TRUE)

    /* Set the type of DUID being used on the client. */
    DHCP_Duid.duid_type = DHCP_DUID_EN;

    /* Set the enterprise number of the DUID. */
    DHCP_Duid.duid_ent_no = DHCP_DUID_PRIV_ENT_NO;

    /* Set the length of the ID number. */
    DHCP_Duid.duid_id_no_len = strlen(DHCP_DUID_ID_NO);

    /* Set the identifier of the DUID. */
    NU_BLOCK_COPY(DHCP_Duid.duid_id, DHCP_DUID_ID_NO,
                  DHCP_Duid.duid_id_no_len);

#endif

#if (INCLUDE_DHCP_DUID_LL == NU_TRUE)

    /* Set the type of DUID being used on the client. */
    DHCP_Duid.duid_type = DHCP_DUID_LL;

    /* Set the hardware type of the DUID. */
    DHCP_Duid.duid_hw_type = DHCP_DUID_HW_TYPE;

#endif
#endif

    /* Initialize the MIB-II data structures. */
#if (INCLUDE_MIB2_RFC1213 == NU_TRUE)

    MIB2_Init();

#endif

    EQ_Init();

#if ( (INCLUDE_IPV4 == NU_TRUE) && (INCLUDE_ARP == NU_TRUE) )

    ARP_Init();                             /* ARP packets          */

#endif

#if (INCLUDE_IPV4 == NU_TRUE)

    ICMP_Init();                            /* ping packets         */

#endif

    IP_Initialize();                        /* ip packets           */

#if (INCLUDE_IPV4 == NU_TRUE)

    RTAB4_Init();

#endif

#if (INCLUDE_IP_MULTICASTING == NU_TRUE)

    Multi_Init();

#endif

#if (INCLUDE_IPV6 == NU_TRUE)

    Resolve6_Init();

    IP6_Initialize();

    ICMP6_Init();

    RTAB6_Init();


#if (INCLUDE_DAD6 == NU_TRUE)

    nd6_dad_initialize();

#endif
#endif

#if (INCLUDE_TCP == NU_TRUE)
    TCP_Init();                             /* tcp packets          */
#endif

#if (INCLUDE_UDP == NU_TRUE)
    UDP_Init();                             /* udp packets          */
#endif

#if (INCLUDE_IP_RAW == NU_TRUE)
    IPRaw_Init();                           /* raw ip packets       */
#endif

#if ( (INCLUDE_IPV6 == NU_TRUE) && (INCLUDE_SNMP == NU_TRUE) && \
      (INCLUDE_IPV6_MIB == NU_TRUE) )

    IP6_MIB_Initialize();

#endif

#if ( (INCLUDE_NET_ERROR_LOGGING == NU_TRUE) || (INCLUDE_TCP_INFO_LOGGING == NU_TRUE) || \
      (INCLUDE_IP_INFO_LOGGING == NU_TRUE)   || (INCLUDE_ARP_INFO_LOGGING == NU_TRUE) || \
      (INCLUDE_ICMP_INFO_LOGGING == NU_TRUE) || (INCLUDE_UDP_INFO_LOGGING == NU_TRUE) )
    NLOG_Init();
#endif

#if (INCLUDE_STATIC_BUILD == NU_TRUE)
    RTAB4_Memory_Init();                    /* Static memory initialization for RTAB */
#endif

#if (INCLUDE_PMTU_DISCOVERY == NU_TRUE)
    /* Initialize the Path MTU Module */
    PMTU_Init();
#endif

    /* Initialize the User-Management Module */
    UM_Initialize();

#if (INCLUDE_IP_TUNNEL == NU_TRUE)
    /* Initialize the IP Tunnel. */
    IP_Tunnel_Init();
#endif

    return (DNS_Initialize());

} /* PROT_Protocol_Init */
