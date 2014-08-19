/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 1998 - 2007
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
*       mac.c                                                    
*
*   DESCRIPTION
*
*       This file contains functions specific to the maintenance of
*       the global data structures related to the devices on the
*       system.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       MacInit
*       MacCollRegister
*       MacCollRemove
*       MacIfaceCount
*
*   DEPENDENCIES
*
*       target.h
*       externs.h
*       xtypes.h
*       snmp.h
*       agent.h
*       link.h
*       mac.h
*
*************************************************************************/

#include "networking/target.h"
#include "networking/externs.h"
#include "networking/xtypes.h"
#include "networking/snmp.h"
#include "networking/agent.h"
#include "networking/link.h"
#include "networking/mac.h"

#if (INCLUDE_MIB_RMON1 == NU_TRUE)
mac_coll_t        *macCollList = 0;
#endif

mac_perf_t        macPerf;

/************************************************************************
*
*   FUNCTION
*
*       MacInit
*
*   DESCRIPTION
*
*       This function initializes the fields of the global macPerf.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       NU_TRUE
*
*************************************************************************/
BOOLEAN MacInit(VOID)
{
    macPerf.on = NU_FALSE;
    macPerf.pkts = 0UL;
    macPerf.octets = 0UL;
    macPerf.timeTotal = 0UL;
    macPerf.timeMin = 0xffffffffUL;
    macPerf.timeMax = 0UL;

    return (NU_TRUE);

} /* MacInit */

#if (INCLUDE_MIB_RMON1 == NU_TRUE)
#if (RFC1757_FILT_INCLUDE == NU_TRUE)

/************************************************************************
*
*   FUNCTION
*
*       MacCollRegister
*
*   DESCRIPTION
*
*       This function adds an entry to the global list macCollList.
*
*   INPUTS
*
*       *coll                   A pointer to the entry to add.
*
*   OUTPUTS
*
*       NU_TRUE
*
*************************************************************************/
BOOL MacCollRegister(mac_coll_t *coll)
{
    coll->next = macCollList;
    macCollList = coll;

    return (NU_TRUE);

} /* MacCollRegister */

#endif

#if ( (RFC1757_FILT_INCLUDE == NU_TRUE) || \
      (RFC1757_HOST_INCLUDE == NU_TRUE) || \
      (RFC1757_MATX_INCLUDE == NU_TRUE) )

/************************************************************************
*
*   FUNCTION
*
*       MacCollRemove
*
*   DESCRIPTION
*
*       This function removes the specified entry from the global list
*       macCollList.
*
*   INPUTS
*
*       *coll                   The entry to remove.
*
*   OUTPUTS
*
*       NU_TRUE
*
*************************************************************************/
BOOL MacCollRemove(mac_coll_t *coll)
{
    mac_coll_t **p;

    /* Get a pointer to the head of the MAC coll list */
    p = &macCollList;

    /* Traverse the list until we find the target MAC coll list or until
     * we reach the end of the list.
     */
    while (*p != 0)
    {
        if (*p == coll)
            *p = (*p)->next;
        else
            p = &(*p)->next;
    }

    return (NU_TRUE);

} /* MacCollRemove */

#endif /* ( (RFC1757_FILT_INCLUDE == NU_TRUE) || \
            (RFC1757_HOST_INCLUDE == NU_TRUE) || \
            (RFC1757_MATX_INCLUDE == NU_TRUE) ) */

/************************************************************************
*
*   FUNCTION
*
*       MacIfaceCount
*
*   DESCRIPTION
*
*       This function returns the number of interfaces on the system.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       count                   The number of interfaces on the system.
*
*************************************************************************/
UINT16 MacIfaceCount(VOID)
{
    return (MIB2_totalInterface_Get);

} /* MacIfaceCount */

#endif  /* (INCLUDE_MIB_RMON1 == NU_TRUE) */
