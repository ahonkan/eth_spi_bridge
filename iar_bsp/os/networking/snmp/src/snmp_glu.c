/*************************************************************************
*
*               Copyright Mentor Graphics Corporation 1998 - 2007
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
*   FILENAME                                               
*
*       snmp_glu.c                                               
*
*   DESCRIPTION
*
*       This file contains those functions within SNMP that are used
*       specifically for a particular MIB but are required for SNMP.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       snmp_pkttype
*       set_1213outQLen
*       set_1213outErrors
*       set_1213outDiscards
*       set_1213outNUcastPkts
*       set_1213outUcastPkts
*       set_1213outOctets
*       set_1213inUcastPkts
*       set_1213inNUcastPkts
*       set_1213inUnknownProtos
*       set_1213inErrors
*       set_1213inDiscards
*       set_1213inOctets
*       set_1213ifType
*       set_1213ifMtu
*       set_1213ifSpeed
*       set_1213ifPhysAddress
*       set_1213ifAdminStatus
*       set_1213ifOperStatus
*       set_1213ifLastChange
*       set_1213ifIndex
*       set_1213ifDescr
*
*   DEPENDENCIES
*
*       externs.h
*       xtypes.h
*       link.h
*       snmp_sys.h
*       mac.h
*       agent.h
*
*************************************************************************/

#include "networking/externs.h"
#include "networking/xtypes.h"
#include "networking/link.h"
#include "networking/snmp_sys.h"
#include "networking/mac.h"
#include "networking/agent.h"

static UINT8 bcaststr[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

/************************************************************************
*
*   FUNCTION
*
*       snmp_pkttype
*
*   DESCRIPTION
*
*       This function determines whether a packet is broadcast,
*       multicast or unicast.
*
*   INPUTS
*
*       unit    Unused - the device index.
*       *pkt    A pointer to the packet.
*
*   OUTPUTS
*
*       BCAST
*       MCAST
*       UNICAST
*
*************************************************************************/
UINT32 snmp_pkttype (UINT32 dev_index, const UINT8 *pkt)
{
    UINT32  packet_type;

    UNUSED_PARAMETER(dev_index);

    if (((UINT8 *)pkt)[0] & 0x01)
    {
        if (((UINT8 *)pkt)[0] == 0xFF)
        {
            if (memcmp(pkt, bcaststr, 6 ) == 0)
                packet_type = BCAST;
            else
                packet_type = UNICAST;
        }
        else
            packet_type = MCAST;
    }
    else
        packet_type = UNICAST;

    return (packet_type);

} /* snmp_pkttype */

/************************************************************************
*
*   FUNCTION
*
*       set_1213ifIndex
*
*   DESCRIPTION
*
*       This function sets the index of the device associated with unit.
*
*   INPUTS
*
*       unit        The index of the device.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID set_1213ifIndex (UINT32 unit)
{
    UNUSED_PARAMETER(unit);

} /* set_1213ifIndex */

/************************************************************************
*
*   FUNCTION
*
*       set_1213ifDescr
*
*   DESCRIPTION
*
*       This function sets the description of the device.
*
*   INPUTS
*
*       unit        The index of the device.
*       descr       A pointer to a buffer containing the description.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID set_1213ifDescr (UINT32 unit, CHAR *descr)
{
    MIB2_Set_IfDescr(unit, (UINT8 *)descr);

} /* set_1213ifDescr */

/************************************************************************
*
*   FUNCTION
*
*       set_1213ifType
*
*   DESCRIPTION
*
*       This function sets the type of the device.
*
*   INPUTS
*
*       unit        The index of the device.
*       value       The type of the device.
*
*   OUTPUTS
*
*       None
*
************************************************************************/
VOID set_1213ifType (UINT32 unit, UINT32 value)
{
    MIB2_Set_IfType(unit, (INT32)value);

} /* set_1213ifType */

/************************************************************************
*
*   FUNCTION
*
*       set_1213ifMtu
*
*   DESCRIPTION
*
*       This function sets the MTU of the device.
*
*   INPUTS
*
*       unit        The index of the device.
*       value       The MTU of the device.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID set_1213ifMtu (UINT32 unit, UINT32 value)
{
    MIB2_Set_IfMtu(unit, value);

} /* set_1213ifMtu */

/************************************************************************
*
*   FUNCTION
*
*       set_1213ifSpeed
*
*   DESCRIPTION
*
*       This function sets the speed of the device.
*
*   INPUTS
*
*       unit        The index of the device.
*       value       The speed of the device.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID set_1213ifSpeed (UINT32 unit, UINT32 value)
{
    MIB2_Set_IfSpeed(unit, value);

} /* set_1213ifSpeed */

/************************************************************************
*
*   FUNCTION
*
*       set_1213ifPhysAddress
*
*   DESCRIPTION
*
*       This function sets the MAC address of the device.
*
*   INPUTS
*
*       unit        The index of the device.
*       value       The MAC address of the device.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID set_1213ifPhysAddress (UINT32 unit, UINT8 *value)
{
    MIB2_Set_IfAddr(unit, value);
} /* set_1213ifPhysAddress */

/************************************************************************
*
*   FUNCTION
*
*       set_1213ifAdminStatus
*
*   DESCRIPTION
*
*       This function sets the value of AdminStatus of the device.
*
*   INPUTS
*
*       unit        The index of the device.
*       value       The value of AdminStatus.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID set_1213ifAdminStatus (UINT32 unit, UINT32 value)
{
    MIB2_Set_IfStatusAdmin(unit, (INT32)value);

} /* set_1213ifAdminStatus */

/************************************************************************
*
*   FUNCTION
*
*       set_1213OperStatus
*
*   DESCRIPTION
*
*       This function sets the value of OperStatus for the device.
*
*   INPUTS
*
*       unit        The index of the device.
*       value       The value of OperStatus.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID set_1213ifOperStatus (UINT32 unit, UINT32 value)
{
    UNUSED_PARAMETER(unit);
    UNUSED_PARAMETER(value);

} /* set_1213ifOperStatus */

/************************************************************************
*
*   FUNCTION
*
*       set_1213ifLastChange
*
*   DESCRIPTION
*
*       This function sets the LastChange of the device.
*
*   INPUTS
*
*       unit        The index of the device.
*       value       The LastChange of the device.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID set_1213ifLastChange (UINT32 unit, UINT32 value)
{
    MIB2_Set_IfLastChange(unit, value);

} /* set_1213ifLastChange */

/************************************************************************
*
*   FUNCTION
*
*       set_1213outQLen
*
*   DESCRIPTION
*
*       This function updates the number of packets sent out a device
*       based on the index of the device.
*
*   INPUTS
*
*       unit        The index of the device.
*       value       The updated number of packets sent out the device.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID set_1213outQLen (UINT32 unit, UINT32 value)
{
    UNUSED_PARAMETER(unit);
    UNUSED_PARAMETER(value);

} /* set_1213outQLen */

/************************************************************************
*
*   FUNCTION
*
*       set_1213outErrors
*
*   DESCRIPTION
*
*       This function updates the number of errors that have occurred
*       on a device based on the index of the device.
*
*   INPUTS
*
*       unit        The index of the device.
*       value       The updated number of errors on that device.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID set_1213outErrors (UINT32 unit, UINT32 value)
{
    MIB2_Add_IfOutErrors(unit, value, NU_TRUE);
} /* set_1213outErrors */

/************************************************************************
*
*   FUNCTION
*
*       set_1213outDiscards
*
*   DESCRIPTION
*
*       This function updates the number of packets discarded for a
*       device based on the index of the device.
*
*   INPUTS
*
*       unit        The index of the device.
*       value       The updated number of discarded packets for the
*                   device.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID set_1213outDiscards (UINT32 unit, UINT32 value)
{
    MIB2_Add_IfOutDiscards(unit, value, NU_TRUE);

} /* set_1213outDiscards */

/************************************************************************
*
*   FUNCTION
*
*       set_1213outNUcastPkts
*
*   DESCRIPTION
*
*       This function updates the number of non-unicast packets sent
*       out a device based on the index of the device.
*
*   INPUTS
*
*       unit        The index of the device.
*       value       The updated number of packets sent out the device.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID set_1213outNUcastPkts (UINT32 unit, UINT32 value)
{
    MIB2_Add_IfOutNUcastPkts(unit, value, NU_TRUE);

} /* set_1213outNUcastPkts */

/************************************************************************
*
*   FUNCTION
*
*       set_1213outUcastPkts
*
*   DESCRIPTION
*
*       This function updates the number of unicast packets sent out a
*       device based on the index of the device.
*
*   INPUTS
*
*       unit        The index of the device.
*       value       The updated number of packets sent out the device.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID set_1213outUcastPkts (UINT32 unit, UINT32 value)
{
    MIB2_Add_IfOutUcastPkts(unit, value, NU_TRUE);

} /* set_1213outUcastPkts */

/************************************************************************
*
*   FUNCTION
*
*       set_1213outOctets
*
*   DESCRIPTION
*
*       This function updates the number of octets sent out a device
*       based on the index of the device.
*
*   INPUTS
*
*       unit        The index of the device.
*       value       The updated number of octets sent out the device.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID set_1213outOctets (UINT32 unit, UINT32 value)
{
    MIB2_Add_IfOutOctets(unit, value, NU_TRUE);

} /* set_1213outOctets */

/************************************************************************
*
*   FUNCTION
*
*       set_1213inUcastPkts
*
*   DESCRIPTION
*
*       This function updates the number of unicast packets received
*       on a device based on the index of the device.
*
*   INPUTS
*
*       unit        The index of the device.
*       value       The updated number of packets received on the device.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID set_1213inUcastPkts (UINT32 unit, UINT32 value)
{
    MIB2_Add_IfInUcastPkts(unit, value, NU_TRUE);

} /* set_1213inUcastPkts */

/************************************************************************
*
*   FUNCTION
*
*       set_1213inNUcastPkts
*
*   DESCRIPTION
*
*       This function updates the number of non-unicast packets received
*       on a device based on the index of the device.
*
*   INPUTS
*
*       unit        The index of the device.
*       value       The updated number of packets received on the device.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID set_1213inNUcastPkts (UINT32 unit, UINT32 value)
{
    MIB2_Add_IfInNUcastPkts(unit, value, NU_TRUE);

} /* set_1213inNUcastPkts */

/************************************************************************
*
*   FUNCTION
*
*       set_1213inUnknownProtos
*
*   DESCRIPTION
*
*       This function updates the number of packets of unknown protocol
*       received on a device based on the index of the device.
*
*   INPUTS
*
*       unit        The index of the device.
*       value       The updated number of packets received on the device.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID set_1213inUnknownProtos (UINT32 unit, UINT32 value)
{
    MIB2_Add_IfInUnknownProtos(unit, value, NU_TRUE);

} /* set_1213inUnknownProtos */

/************************************************************************
*
*   FUNCTION
*
*       set_1213inErrors
*
*   DESCRIPTION
*
*       This function updates the number of incoming errors on a device
*       based on the index of the device.
*
*   INPUTS
*
*       unit        The index of the device.
*       value       The updated number of errors on the device.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID set_1213inErrors (UINT32 unit, UINT32 value)
{
    MIB2_Add_IfInErrors(unit, value, NU_TRUE);
} /* set_1213inErrors */

/************************************************************************
*
*   FUNCTION
*
*       set_1213inDiscards
*
*   DESCRIPTION
*
*       This function updates the number of packets discarded on a device
*       based on the index of the device.
*
*   INPUTS
*
*       unit        The index of the device.
*       value       The updated number of packets discarded on the device.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID set_1213inDiscards (UINT32 unit, UINT32 value)
{
    MIB2_Add_IfInDiscards(unit, value, NU_TRUE);

} /* set_1213inDiscards */

/************************************************************************
*
*   FUNCTION
*
*       set_1213inOctets
*
*   DESCRIPTION
*
*       This function updates the number of octets received on a device
*       based on the index of the device.
*
*   INPUTS
*
*       unit        The index of the device.
*       value       The updated number of octets received on the device.
*
*   OUTPUTS
*
*       None
*
*************************************************************************/
VOID set_1213inOctets (UINT32 unit, UINT32 value)
{
    MIB2_Add_IfInOctets(unit, value, NU_TRUE);

} /* set_1213inOctets */


