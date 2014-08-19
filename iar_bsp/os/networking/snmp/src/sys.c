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
*       sys.c                                                    
*
*   DESCRIPTION
*
*       This file contains all functions to retrieve and update the
*       sys parameters of the agent.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       SysTime
*       get_system_contact
*       get_system_description
*       get_system_location
*       get_system_objectid
*       get_system_services
*       sc_init
*
*   DEPENDENCIES
*
*       target.h
*       snmp_prt.h
*       xtypes.h
*       snmp.h
*       agent.h
*
*************************************************************************/

#include "networking/target.h"
#include "networking/snmp_prt.h"
#include "networking/xtypes.h"
#include "networking/snmp.h"
#include "networking/agent.h"

extern  rfc1213_vars_t  rfc1213_vars;

/************************************************************************
*
*   FUNCTION
*
*       SysTime
*
*   DESCRIPTION
*
*       This function returns the time in clock ticks that the system
*       has been up.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       UINT32      The time in clock ticks that the system has been up.
*
*************************************************************************/
UINT32 SysTime (VOID)
{
/* Current ticks minus startup ticks 
    if ((UINT32)rfc1213_vars.rfc1213_sys.sysUpTime <= NU_Retrieve_Clock())
        return (NU_Retrieve_Clock() - rfc1213_vars.rfc1213_sys.sysUpTime);
    else
        return (NU_Retrieve_Clock());
*/
	/*Current PLUS time in 100th of a second*/
	UINT32 current_time = NU_Retrieve_Clock()*(100/NU_PLUS_Ticks_Per_Second);
    
	/* Current ticks minus startup ticks */
    if ((UINT32)rfc1213_vars.rfc1213_sys.sysUpTime < current_time)
        return (current_time - rfc1213_vars.rfc1213_sys.sysUpTime);
    else
        return ((0xFFFFFFFFUL - (UINT32)rfc1213_vars.rfc1213_sys.sysUpTime) + current_time);

} /* SysTime */

/************************************************************************
*
*   FUNCTION
*
*       get_system_contact
*
*   DESCRIPTION
*
*       This function copies the system contact information into the
*       given buffer.  This function will copy MAX_SYS_STRINGS bytes of
*       data regardless of the length of the system contact information.
*
*   INPUTS
*
*       *buf        A pointer to the buffer into which to copy the
*                   system contact information.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID get_system_contact (UINT8 *buf)
{
    NU_BLOCK_COPY(buf, rfc1213_vars.rfc1213_sys.sysContact,
                  MAX_1213_STRSIZE);

} /* get_system_contact */

/************************************************************************
*
*   FUNCTION
*
*       get_system_description
*
*   DESCRIPTION
*
*       This function copies the system description into the given
*       buffer.  This function will copy MAX_SYS_STRINGS bytes of data
*       into the buffer regardless of the length of the system
*       description.
*
*   INPUTS
*
*       *buf        A pointer to the buffer into which to copy the
*                   system description.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID get_system_description (UINT8 *buf)
{
    NU_BLOCK_COPY(buf, rfc1213_vars.rfc1213_sys.sysDescr,
                  MAX_1213_STRSIZE);

} /* get_system_description */

/************************************************************************
*
*   FUNCTION
*
*       get_system_location
*
*   DESCRIPTION
*
*       This function copies the system location into the give buffer.
*       This function will copy MAX_SYS_STRINGS bytes of data into the
*       buffer regardless of the length of the system location.
*
*   INPUTS
*
*       *buf        A pointer to the buffer into which to copy the
*                   system location.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID get_system_location (UINT8 *buf)
{
    NU_BLOCK_COPY(buf, rfc1213_vars.rfc1213_sys.sysLocation,
                  MAX_1213_STRSIZE);

} /* get_system_location */

/************************************************************************
*
*   FUNCTION
*
*       get_system_objectid
*
*   DESCRIPTION
*
*       This function copies the system identifier into the given buffer.
*       This function will copy MAX_SYS_STRINGS bytes of data into the
*       buffer regardless of the length of the system identifier.
*
*   INPUTS
*
*       *buf        A pointer to the buffer into which to copy the
*                   system identifier.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID get_system_objectid (UINT8 *buf)
{
    NU_BLOCK_COPY(buf, rfc1213_vars.rfc1213_sys.sysObjectID,
                  MAX_1213_OIDSIZE);

} /* get_system_objectid */

/************************************************************************
*
*   FUNCTION
*
*       get_system_services
*
*   DESCRIPTION
*
*       This function copies the system services into the give buffer.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       rfc1213_vars.rfc1213_sys.sysServices
*
*************************************************************************/
INT32 get_system_services (VOID)
{
    return (rfc1213_vars.rfc1213_sys.sysServices);

} /* get_system_services */

/************************************************************************
*
*   FUNCTION
*
*       sc_init
*
*   DESCRIPTION
*
*       This function initializes the rfc1213_vars.rfc1213_sys parameters
*       to the default macros specified in snmp_prt.h.
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID sc_init (VOID)
{
    strcpy((CHAR *)(rfc1213_vars.rfc1213_sys.sysContact),
            MIBII_SYSCONTACT);

    strcpy((CHAR *)(rfc1213_vars.rfc1213_sys.sysDescr),
            MIBII_SYSDESCRIPTION);

    strcpy((CHAR *)(rfc1213_vars.rfc1213_sys.sysLocation),
            MIBII_SYSLOCATION);

    strcpy((CHAR *)rfc1213_vars.rfc1213_sys.sysObjectID, MIBII_OBJECTID);

    rfc1213_vars.rfc1213_sys.sysServices = MIBII_SERVICES;

} /* sc_init */



