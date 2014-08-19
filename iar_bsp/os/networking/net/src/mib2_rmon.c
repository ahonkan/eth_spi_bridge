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
*        mib2_rmon.c
*
* COMPONENT
*
*        MIB II - Remote Monitoring.
*
* DESCRIPTION
*
*        This file contain the function that are responsible for
*        maintaining statistics of remote monitoring.
*
* DATA STRUCTURES
*
*        None
*
* FUNCTIONS
*
*        MIB2_Get_1757Octets
*        MIB2_Add_1757Octets
*        MIB2_Get_1757Pkts
*        MIB2_Add_1757Pkts
*        MIB2_Get_1757BroadcastPkts
*        MIB2_Add_1757BroadcastPkts
*        MIB2_Get_1757MulticastPkts
*        MIB2_Add_1757MulticastPkts
*        MIB2_Get_1757CRCAlignErrors
*        MIB2_Add_1757CRCAlignErrors
*        MIB2_Get_1757UndersizePkts
*        MIB2_Add_1757UndersizePkts
*        MIB2_Get_1757OversizePkts
*        MIB2_Add_1757OversizePkts
*        MIB2_Get_1757Fragments
*        MIB2_Add_1757Fragments
*        MIB2_Get_1757Jabbers
*        MIB2_Add_1757Jabbers
*        MIB2_Get_1757Collisions
*        MIB2_Add_1757Collisions
*        MIB2_Get_1757Pkts64Octets
*        MIB2_Add_1757Pkts64Octets
*        MIB2_Get_1757Pkts65to127Octets
*        MIB2_Add_1757Pkts65to127Octets
*        MIB2_Get_1757Pkts128to255Octets
*        MIB2_Add_1757Pkts128to255Octets
*        MIB2_Get_1757Pkts256to511Octets
*        MIB2_Add_1757Pkts256to511Octets
*        MIB2_Get_1757Pkts512to1023Octets
*        MIB2_Add_1757Pkts512to1023Octets
*        MIB2_Get_1757Pkts1024to1518Octets
*        MIB2_Add_1757Pkts1024to1518Octets
*        MIB2_Get_1757LostPkts
*        MIB2_Add_1757LostPkts
*        MIB2_Get_1757DiscardedPkts
*        MIB2_Add_1757DiscardedPkts
*        MIB2_Get_1757OutPkts
*        MIB2_Add_1757OutPkts
*        MIB2_Get_1757OutOctets
*        MIB2_Add_1757OutOctets
*        MIB2_Get_1757OutMulticastPkts
*        MIB2_Add_1757OutMulticastPkts
*        MIB2_Get_1757OutBroadcastPkts
*        MIB2_Add_1757OutBroadcastPkts
*        MIB2_Get_1757OutErrors
*        MIB2_Add_1757OutErrors
*
* DEPENDENCIES
*
*        nu_net.h
*
*************************************************************************/

#include "networking/nu_net.h"

#if (MIB2_IF_INCLUDE == NU_TRUE)

/*-----------------------------------------------------------------------
 * RMON Group
 *----------------------------------------------------------------------*/

#if(INCLUDE_SNMP == NU_TRUE)
#if(INCLUDE_MIB_RMON1 == NU_TRUE)

extern MIB2_INTERFACE_STRUCT           *MIB2_Interface_Get(UINT32);

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_1757Octets
*
* DESCRIPTION
*
*       This function gets the interface's octets count.
*
* INPUTS
*
*      index                    The interface index of the device.
*
* OUTPUTS
*
*       UINT32                  If a device is found with the specified
*                               index, this function returns the count.
*                               Otherwise, it returns 0.
*
*************************************************************************/
UINT32 MIB2_Get_1757Octets(UINT32 index)
{
    MIB2_INTERFACE_STRUCT   *iface;
    UINT32                  octets;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        octets = 0;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If the interface is found, get the octets count. */
        if (iface != NU_NULL)
            octets = iface->eth->Octets;
        else
            octets = 0;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* return the octets */
    return (octets);

} /* MIB2_Get_1757Octets */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Add_1757Octets
*
* DESCRIPTION
*
*       This function adds/sets the interface's octets count
*
* INPUTS
*
*      index                    The interface index of the device.
*      octets                   The octets to be stored.
*      reset                    NU_TRUE if the value is to be set.
*                               NU_FALSE if the value is to be added.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Add_1757Octets(UINT32 index, UINT32 octets, INT16 reset)
{
    MIB2_INTERFACE_STRUCT   *iface;
    INT16                   status;

    /* Find the target interface. */
    iface = MIB2_Interface_Get(index);

    /* If the interface is found, update the octets. */
    if (iface != NU_NULL)
    {
        if (reset == NU_TRUE)
            iface->eth->Octets = octets;
        else
            iface->eth->Octets += octets;

        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Returning status */
    return (status);

} /* MIB2_Add_1757Octets */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_1757Pkts
*
* DESCRIPTION
*
*       This function gets the interface's packets count.
*
* INPUTS
*
*      index                    The interface index of the device.
*
* OUTPUTS
*
*       UINT32                  If a device is found with the specified
*                               index, this function returns the count.
*                               Otherwise, it returns 0.
*
*************************************************************************/
UINT32 MIB2_Get_1757Pkts(UINT32 index)
{
    MIB2_INTERFACE_STRUCT   *iface;
    UINT32                  packets;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        packets = 0;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If the interface is found, get the packets count. */
        if (iface != NU_NULL)
        {
            packets = iface->eth->Pkts;
        }

        else
            packets = 0;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* return the no of packets */
    return (packets);

} /* MIB2_Get_1757Pkts */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Add_1757Pkts
*
* DESCRIPTION
*
*       This function adds/sets the interface's packets count
*
* INPUTS
*
*      index                    The interface index of the device.
*      packets                  The packets count to be stored.
*      reset                    NU_TRUE if the value is to be set.
*                               NU_FALSE if the value is to be added.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Add_1757Pkts(UINT32 index, UINT32 packets, INT16 reset)
{
    MIB2_INTERFACE_STRUCT   *iface;
    INT16                   status;

    /* Find the target interface. */
    iface = MIB2_Interface_Get(index);

    /* If the interface is found, set the packets count. */
    if (iface != NU_NULL)
    {
        if (reset == NU_TRUE)
            iface->eth->Pkts = packets;
        else
            iface->eth->Pkts += packets;

        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Returning status */
    return (status);

} /* MIB2_Add_1757Pkts */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_1757BroadcastPkts
*
* DESCRIPTION
*
*       This function gets the interface's broadcast packets count.
*
* INPUTS
*
*      index                    The interface index of the device.
*
* OUTPUTS
*
*       UINT32                  If a device is found with the specified
*                               index, this function returns the count.
*                               Otherwise, it returns 0.
*
*************************************************************************/
UINT32 MIB2_Get_1757BroadcastPkts(UINT32 index)
{
    MIB2_INTERFACE_STRUCT   *iface;
    UINT32                  packets;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        packets = 0;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If the interface is found, get the broadcast packets count. */
        if (iface != NU_NULL)
            packets = iface->eth->BroadcastPkts;
        else
            packets = 0;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Returning status */
    return (packets);

} /* MIB2_Get_1757BroadcastPkts */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Add_1757BroadcastPkts
*
* DESCRIPTION
*
*       This function adds/sets the interface's broadcast packets count
*
* INPUTS
*
*      index                    The interface index of the device.
*      packets                  The broadcast packets count to be stored.
*      reset                    NU_TRUE if the value is to be set.
*                               NU_FALSE if the value is to be added.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Add_1757BroadcastPkts(UINT32 index, UINT32 packets,
                                 INT16 reset)
{
    MIB2_INTERFACE_STRUCT   *iface;
    INT16                   status;

    /* Find the target interface. */
    iface = MIB2_Interface_Get(index);

    /* If the interface is found, set the packets count. */
    if (iface != NU_NULL)
    {
        if (reset == NU_TRUE)
            iface->eth->BroadcastPkts = packets;
        else
            iface->eth->BroadcastPkts += packets;

        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Returning status */
    return (status);

} /* MIB2_Add_1757BroadcastPkts */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_1757MulticastPkts
*
* DESCRIPTION
*
*       This function gets the interface's multicast packets count.
*
* INPUTS
*
*      index                    The interface index of the device.
*
* OUTPUTS
*
*       UINT32                  If a device is found with the specified
*                               index, this function returns the count.
*                               Otherwise, it returns 0.
*
*************************************************************************/
UINT32 MIB2_Get_1757MulticastPkts(UINT32 index)
{
    MIB2_INTERFACE_STRUCT   *iface;
    UINT32                  packets;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        packets = 0;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If the interface is found, get the multicast packets count. */
        if (iface != NU_NULL)
            packets = iface->eth->MulticastPkts;
        else
            packets = 0;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* return the packets */
    return (packets);

} /* MIB2_Get_1757MulticastPkts */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Add_1757MulticastPkts
*
* DESCRIPTION
*
*       This function adds/sets the interface's multicast packets count
*
* INPUTS
*
*      index                    The interface index of the device.
*      packets                  The multicast packets count to be stored.
*      reset                    NU_TRUE if the value is to be set.
*                               NU_FALSE if the value is to be added.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Add_1757MulticastPkts(UINT32 index, UINT32 packets,
                                 INT16 reset)
{
    MIB2_INTERFACE_STRUCT   *iface;
    INT16                   status;

    /* Find the target interface. */
    iface = MIB2_Interface_Get(index);

    /* If the interface is found, set the multicast packets count. */
    if (iface != NU_NULL)
    {
        if (reset == NU_TRUE)
            iface->eth->MulticastPkts = packets;
        else
            iface->eth->MulticastPkts += packets;

        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Returning status */
    return (status);

} /* MIB2_Add_1757MulticastPkts */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_1757CRCAlignErrors
*
* DESCRIPTION
*
*       This function gets the interface's CRC Align errors count.
*
* INPUTS
*
*      index                    The interface index of the device.
*
* OUTPUTS
*
*       UINT32                  If a device is found with the specified
*                               index, this function returns the count.
*                               Otherwise, it returns 0.
*
*************************************************************************/
UINT32 MIB2_Get_1757CRCAlignErrors(UINT32 index)
{
    MIB2_INTERFACE_STRUCT   *iface;
    UINT32                  errors;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        errors = 0;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If the interface is found, get the errors count. */
        if (iface != NU_NULL)
            errors = iface->eth->CRCAlignErrors;
        else
            errors = 0;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* return the errors */
    return (errors);

} /* MIB2_Get_1757CRCAlignErrors */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Add_1757CRCAlignErrors
*
* DESCRIPTION
*
*       This function adds/sets the interface's CRC Align errors count
*
* INPUTS
*
*      index                    The interface index of the device.
*      errors                   The errors count to be stored.
*      reset                    NU_TRUE if the value is to be set.
*                               NU_FALSE if the value is to be added.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Add_1757CRCAlignErrors(UINT32 index, UINT32 errors,
                                  INT16 reset)
{
    MIB2_INTERFACE_STRUCT   *iface;
    INT16                   status;

    /* Find the target interface. */
    iface = MIB2_Interface_Get(index);

    /* If the interface is found, set the errors count. */
    if (iface != NU_NULL)
    {
        if (reset == NU_TRUE)
            iface->eth->CRCAlignErrors = errors;
        else
            iface->eth->CRCAlignErrors += errors;

        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Returning status */
    return (status);

} /* MIB2_Add_1757CRCAlignErrors */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_1757UndersizePkts
*
* DESCRIPTION
*
*       This function gets the interface's undersize packets count.
*
* INPUTS
*
*      index                    The interface index of the device.
*
* OUTPUTS
*
*       UINT32                  If a device is found with the specified
*                               index, this function returns the count.
*                               Otherwise, it returns 0.
*
*************************************************************************/
UINT32 MIB2_Get_1757UndersizePkts(UINT32 index)
{
    MIB2_INTERFACE_STRUCT   *iface;
    UINT32                  packets;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        packets = 0;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If the interface is found, get the undersize packets count. */
        if (iface != NU_NULL)
            packets = iface->eth->UndersizePkts;
        else
            packets = 0;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* return the packets */
    return (packets);

} /* MIB2_Get_1757UndersizePkts */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Add_1757UndersizePkts
*
* DESCRIPTION
*
*       This function adds/sets the interface's undersize packets count
*
* INPUTS
*
*      index                    The interface index of the device.
*      packets                  The count to be stored.
*      reset                    NU_TRUE if the value is to be set.
*                               NU_FALSE if the value is to be added.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Add_1757UndersizePkts(UINT32 index, UINT32 packets,
                                 INT16 reset)
{
    MIB2_INTERFACE_STRUCT   *iface;
    INT16                   status;

    /* Find the target interface. */
    iface = MIB2_Interface_Get(index);

    /* If the interface is found, set the packets count. */
    if (iface != NU_NULL)
    {
        if (reset == NU_TRUE)
            iface->eth->UndersizePkts = packets;
        else
            iface->eth->UndersizePkts += packets;

        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Returning status */
    return (status);

} /* MIB2_Add_1757UndersizePkts */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_1757OversizePkts
*
* DESCRIPTION
*
*       This function gets the interface's oversize packets count.
*
* INPUTS
*
*      index                    The interface index of the device.
*
* OUTPUTS
*
*       UINT32                  If a device is found with the specified
*                               index, this function returns the count.
*                               Otherwise, it returns 0.
*
*************************************************************************/
UINT32 MIB2_Get_1757OversizePkts(UINT32 index)
{
    MIB2_INTERFACE_STRUCT   *iface;
    UINT32                  packets;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        packets = 0;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If the interface is found, get the oversize packets count. */
        if (iface != NU_NULL)
            packets = iface->eth->OversizePkts;
        else
            packets = 0;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* return the packets */
    return (packets);

} /* MIB2_Get_1757OversizePkts */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Add_1757OversizePkts
*
* DESCRIPTION
*
*       This function adds/sets the interface's oversize packets count
*
* INPUTS
*
*      index                    The interface index of the device.
*      packets                  The count to be stored.
*      reset                    NU_TRUE if the value is to be set.
*                               NU_FALSE if the value is to be added.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Add_1757OversizePkts(UINT32 index, UINT32 packets, INT16 reset)
{
    MIB2_INTERFACE_STRUCT   *iface;
    INT16                   status;

    /* Find the target interface. */
    iface = MIB2_Interface_Get(index);

    /* If the interface is found, set the packets count. */
    if (iface != NU_NULL)
    {
        if (reset == NU_TRUE)
            iface->eth->OversizePkts = packets;
        else
            iface->eth->OversizePkts += packets;

        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Returning status */
    return (status);

} /* MIB2_Add_1757OversizePkts */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_1757Fragments
*
* DESCRIPTION
*
*       This function gets the interface's fragments count.
*
* INPUTS
*
*      index                    The interface index of the device.
*
* OUTPUTS
*
*       UINT32                  If a device is found with the specified
*                               index, this function returns the count.
*                               Otherwise, it returns 0.
*
*************************************************************************/
UINT32 MIB2_Get_1757Fragments(UINT32 index)
{
    MIB2_INTERFACE_STRUCT   *iface;
    UINT32                  fragments;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        fragments = 0;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If the interface is found, get the fragments count. */
        if (iface != NU_NULL)
            fragments = iface->eth->Fragments;
        else
            fragments = 0;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* return the fragments */
    return (fragments);

} /* MIB2_Get_1757Fragments */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Add_1757Fragments
*
* DESCRIPTION
*
*       This function adds/sets the interface's fragments count
*
* INPUTS
*
*      index                    The interface index of the device.
*      fragments                The count to be stored.
*      reset                    NU_TRUE if the value is to be set.
*                               NU_FALSE if the value is to be added.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Add_1757Fragments(UINT32 index, UINT32 fragments, INT16 reset)
{
    MIB2_INTERFACE_STRUCT   *iface;
    INT16                   status;

    /* Find the target interface. */
    iface = MIB2_Interface_Get(index);

    /* If the interface is found, set the fragments count. */
    if (iface != NU_NULL)
    {
        if (reset == NU_TRUE)
            iface->eth->Fragments = fragments;
        else
            iface->eth->Fragments += fragments;

        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Returning status */
    return (status);

} /* MIB2_Add_1757Fragments */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_1757Jabbers
*
* DESCRIPTION
*
*       This function gets the interface's jabbers.
*
* INPUTS
*
*      index                    The interface index of the device.
*
* OUTPUTS
*
*       UINT32                  If a device is found with the specified
*                               index, this function returns the count.
*                               Otherwise, it returns 0.
*
*************************************************************************/
UINT32 MIB2_Get_1757Jabbers(UINT32 index)
{
    MIB2_INTERFACE_STRUCT   *iface;
    UINT32                  jabbers;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        jabbers = 0;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If the interface is found, get the jabbers count. */
        if (iface != NU_NULL)
            jabbers = iface->eth->Jabbers;
        else
            jabbers = 0;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* return the count */
    return (jabbers);

} /* MIB2_Get_1757Jabbers */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Add_1757Jabbers
*
* DESCRIPTION
*
*       This function adds/sets the interface's jabbers count
*
* INPUTS
*
*      index                    The interface index of the device.
*      jabbers                  The count to be stored.
*      reset                    NU_TRUE if the value is to be set.
*                               NU_FALSE if the value is to be added.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Add_1757Jabbers(UINT32 index, UINT32 jabbers, INT16 reset)
{
    MIB2_INTERFACE_STRUCT   *iface;
    INT16                   status;

    /* Find the target interface. */
    iface = MIB2_Interface_Get(index);

    /* If the interface is found, set the jabbers count. */
    if (iface != NU_NULL)
    {
        if (reset == NU_TRUE)
            iface->eth->Jabbers = jabbers;
        else
            iface->eth->Jabbers += jabbers;

        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Returning status */
    return (status);

} /* MIB2_Add_1757Jabbers */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_1757Collisions
*
* DESCRIPTION
*
*       This function gets the interface's collisions.
*
* INPUTS
*
*      index                    The interface index of the device.
*
* OUTPUTS
*
*       UINT32                  If a device is found with the specified
*                               index, this function returns the count.
*                               Otherwise, it returns 0.
*
*************************************************************************/
UINT32 MIB2_Get_1757Collisions(UINT32 index)
{
    MIB2_INTERFACE_STRUCT   *iface;
    UINT32                  collisions;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        collisions = 0;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If the interface is found, get the collisions count. */
        if (iface != NU_NULL)
            collisions = iface->eth->Collisions;
        else
            collisions = 0;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* return the collisions */
    return (collisions);

} /* MIB2_Get_1757Collisions */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Add_1757Collisions
*
* DESCRIPTION
*
*       This function adds/sets the interface's collisions count
*
* INPUTS
*
*      index                    The interface index of the device.
*      collisions               The count to be stored.
*      reset                    NU_TRUE if the value is to be set.
*                               NU_FALSE if the value is to be added.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Add_1757Collisions(UINT32 index, UINT32 collisions,
                              INT16 reset)
{
    MIB2_INTERFACE_STRUCT   *iface;
    INT16                   status;

    /* Find the target interface. */
    iface = MIB2_Interface_Get(index);

    /* If the interface is found, set the collisions count. */
    if (iface != NU_NULL)
    {
        if (reset == NU_TRUE)
            iface->eth->Collisions = collisions;
        else
            iface->eth->Collisions += collisions;

        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Returning status */
    return (status);

} /* MIB2_Add_1757Collisions */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_1757Pkts64Octets
*
* DESCRIPTION
*
*       This function gets the interface's packets count.
*
* INPUTS
*
*      index                    The interface index of the device.
*
* OUTPUTS
*
*       UINT32                  If a device is found with the specified
*                               index, this function returns the count.
*                               Otherwise, it returns 0.
*
*************************************************************************/
UINT32 MIB2_Get_1757Pkts64Octets(UINT32 index)
{
    MIB2_INTERFACE_STRUCT   *iface;
    UINT32                  packets;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        packets = 0;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If the interface is found, get the packets count. */
        if (iface != NU_NULL)
            packets = iface->eth->Pkts64Octets;
        else
            packets = 0;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* return the packets */
    return (packets);

} /* MIB2_Get_1757Pkts64Octets */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Add_1757Pkts64Octets
*
* DESCRIPTION
*
*       This function sets the interface's packets count
*
* INPUTS
*
*      index                    The interface index of the device.
*      packets                  The count to be stored.
*      reset                    NU_TRUE if the value is to be set.
*                               NU_FALSE if the value is to be added.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Add_1757Pkts64Octets(UINT32 index, UINT32 packets, INT16 reset)
{
    MIB2_INTERFACE_STRUCT   *iface;
    INT16                   status;

    /* Find the target interface. */
    iface = MIB2_Interface_Get(index);

    /* If the interface is found, set the packets count. */
    if (iface != NU_NULL)
    {
        if (reset == NU_TRUE)
            iface->eth->Pkts64Octets = packets;
        else
            iface->eth->Pkts64Octets += packets;

        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Returning status */
    return (status);

} /* MIB2_Add_1757Pkts64Octets */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_1757Pkts65to127Octets
*
* DESCRIPTION
*
*       This function gets the interface's packets count.
*
* INPUTS
*
*      index                    The interface index of the device.
*
* OUTPUTS
*
*       UINT32                  If a device is found with the specified
*                               index, this function returns the count.
*                               Otherwise, it returns 0.
*
*************************************************************************/
UINT32 MIB2_Get_1757Pkts65to127Octets(UINT32 index)
{
    MIB2_INTERFACE_STRUCT   *iface;
    UINT32                  packets;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        packets = 0;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If the interface is found, get the packets count. */
        if (iface != NU_NULL)
            packets = iface->eth->Pkts65to127Octets;
        else
            packets = 0;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* return the packets */
    return (packets);

} /* MIB2_Get_1757Pkts65to127Octets */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Add_1757Pkts65to127Octets
*
* DESCRIPTION
*
*       This function adds/sets the interface's packets count
*
* INPUTS
*
*      index                    The interface index of the device.
*      packets                  The count to be stored.
*      reset                    NU_TRUE if the value is to be set.
*                               NU_FALSE if the value is to be added.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Add_1757Pkts65to127Octets(UINT32 index, UINT32 packets,
                                     INT16 reset)
{
    MIB2_INTERFACE_STRUCT   *iface;
    INT16                   status;

    /* Find the target interface. */
    iface = MIB2_Interface_Get(index);

    /* If the interface is found, set the packets count. */
    if (iface != NU_NULL)
    {
        if (reset == NU_TRUE)
            iface->eth->Pkts65to127Octets = packets;
        else
            iface->eth->Pkts65to127Octets += packets;

        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Returning status */
    return (status);

} /* MIB2_Add_1757Pkts65to127Octets */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_1757Pkts128to255Octets
*
* DESCRIPTION
*
*       This function gets the interface's packets count.
*
* INPUTS
*
*      index                    The interface index of the device.
*
* OUTPUTS
*
*       UINT32                  If a device is found with the specified
*                               index, this function returns the count.
*                               Otherwise, it returns 0.
*
*************************************************************************/
UINT32 MIB2_Get_1757Pkts128to255Octets(UINT32 index)
{
    MIB2_INTERFACE_STRUCT   *iface;
    UINT32                  packets;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        packets = 0;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If the interface is found, get the packets count. */
        if (iface != NU_NULL)
            packets = iface->eth->Pkts128to255Octets;
        else
            packets = 0;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* return the packets */
    return (packets);

} /* MIB2_Get_1757Pkts128to255Octets */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Add_1757Pkts128to255Octets
*
* DESCRIPTION
*
*       This function adds/sets the interface's packets count
*
* INPUTS
*
*      index                    The interface index of the device.
*      packets                  The count to be stored.
*      reset                    NU_TRUE if the value is to be set.
*                       `       NU_FALSE if the value is to be added.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Add_1757Pkts128to255Octets(UINT32 index, UINT32 packets,
                                      INT16 reset)
{
    MIB2_INTERFACE_STRUCT   *iface;
    INT16                   status;

    /* Find the target interface. */
    iface = MIB2_Interface_Get(index);

    /* If the interface is found, set the packets count. */
    if (iface != NU_NULL)
    {
        if (reset == NU_TRUE)
            iface->eth->Pkts128to255Octets = packets;
        else
            iface->eth->Pkts128to255Octets += packets;

        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Returning status */
    return (status);

} /* MIB2_Add_1757Pkts128to255Octets */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_1757Pkts256to511Octets
*
* DESCRIPTION
*
*       This function gets the interface's packets count.
*
* INPUTS
*
*      index                    The interface index of the device.
*
* OUTPUTS
*
*       UINT32                  If a device is found with the specified
*                               index, this function returns the count.
*                               Otherwise, it returns 0.
*
*************************************************************************/
UINT32 MIB2_Get_1757Pkts256to511Octets(UINT32 index)
{
    MIB2_INTERFACE_STRUCT   *iface;
    UINT32                  packets;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        packets = 0;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If the interface is found, get the packets count. */
        if (iface != NU_NULL)
            packets = iface->eth->Pkts256to511Octets;
        else
            packets = 0;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* return the packets */
    return (packets);

} /* MIB2_Get_1757Pkts256to511Octets */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Add_1757Pkts256to511Octets
*
* DESCRIPTION
*
*       This function adds/sets the interface's packets count
*
* INPUTS
*
*      index                    The interface index of the device.
*      packets                  The count to be stored.
*      reset                    NU_TRUE if the value is to be set.
*                               NU_FALSE if the value is to be added.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Add_1757Pkts256to511Octets(UINT32 index, UINT32 packets,
                                      INT16 reset)
{
    MIB2_INTERFACE_STRUCT   *iface;
    INT16                   status;

    /* Find the target interface. */
    iface = MIB2_Interface_Get(index);

    /* If the interface is found, set the packets count. */
    if (iface != NU_NULL)
    {
        if (reset == NU_TRUE)
            iface->eth->Pkts256to511Octets = packets;
        else
            iface->eth->Pkts256to511Octets += packets;

        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Returning status */
    return (status);

} /* MIB2_Add_1757Pkts256to511Octets */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_1757Pkts512to1023Octets
*
* DESCRIPTION
*
*       This function gets the interface's packets count.
*
* INPUTS
*
*      index                    The interface index of the device.
*
* OUTPUTS
*
*       UINT32                  If a device is found with the specified
*                               index, this function returns the count.
*                               Otherwise, it returns 0.
*
*************************************************************************/
UINT32 MIB2_Get_1757Pkts512to1023Octets(UINT32 index)
{
    MIB2_INTERFACE_STRUCT   *iface;
    UINT32                  packets;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        packets = 0;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If the interface is found, get the packets count. */
        if (iface != NU_NULL)
            packets = iface->eth->Pkts512to1023Octets;
        else
            packets = 0;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* return the packets */
    return (packets);

} /* MIB2_Get_1757Pkts512to1023Octets */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Add_1757Pkts512to1023Octets
*
* DESCRIPTION
*
*       This function adds/sets the interface's packets count
*
* INPUTS
*
*      index                    The interface index of the device.
*      packets                  The count to be stored.
*      reset                    NU_TRUE if the value is to be set.
*                               NU_FALSE if the value is to be added.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Add_1757Pkts512to1023Octets(UINT32 index, UINT32 packets,
                                       INT16 reset)
{
    MIB2_INTERFACE_STRUCT   *iface;
    INT16                   status;

    /* Find the target interface. */
    iface = MIB2_Interface_Get(index);

    /* If the interface is found, set the packets count. */
    if (iface != NU_NULL)
    {
        if (reset == NU_TRUE)
            iface->eth->Pkts512to1023Octets = packets;
        else
            iface->eth->Pkts512to1023Octets += packets;

        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Returning status */
    return (status);

} /* MIB2_Add_1757Pkts512to1023Octets */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_1757Pkts1024to1518Octets
*
* DESCRIPTION
*
*       This function gets the interface's packets count.
*
* INPUTS
*
*      index                    The interface index of the device.
*
* OUTPUTS
*
*       UINT32                  If a device is found with the specified
*                               index, this function returns the count.
*                               Otherwise, it returns 0.
*
*************************************************************************/
UINT32 MIB2_Get_1757Pkts1024to1518Octets(UINT32 index)
{
    MIB2_INTERFACE_STRUCT   *iface;
    UINT32                  packets;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        packets = 0;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If the interface is found, get the packets count. */
        if (iface != NU_NULL)
            packets = iface->eth->Pkts1024to1518Octets;
        else
            packets = 0;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* return the packets */
    return (packets);

} /* MIB2_Get_1757Pkts1024to1518Octets */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Add_1757Pkts1024to1518Octets
*
* DESCRIPTION
*
*       This function adds/sets the interface's packets count
*
* INPUTS
*
*      index                    The interface index of the device.
*      packets                  The count to be stored.
*      reset                    NU_TRUE if the value is to be set.
*                               NU_FALSE if the value is to be added.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Add_1757Pkts1024to1518Octets(UINT32 index, UINT32 packets,
                                        INT16 reset)
{
    MIB2_INTERFACE_STRUCT   *iface;
    INT16                   status;

    /* Find the target interface. */
    iface = MIB2_Interface_Get(index);

    /* If the interface is found, set the packets count. */
    if (iface != NU_NULL)
    {
        if (reset == NU_TRUE)
            iface->eth->Pkts1024to1518Octets = packets;
        else
            iface->eth->Pkts1024to1518Octets += packets;

        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Returning status */
    return (status);

} /* MIB2_Add_1757Pkts1024to1518Octets */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_1757LostPkts
*
* DESCRIPTION
*
*       This function gets the interface's lost packets count.
*
* INPUTS
*
*      index                    The interface index of the device.
*
* OUTPUTS
*
*       UINT32                  If a device is found with the specified
*                               index, this function returns the count.
*                               Otherwise, it returns 0.
*
*************************************************************************/
UINT32 MIB2_Get_1757LostPkts(UINT32 index)
{
    MIB2_INTERFACE_STRUCT   *iface;
    UINT32                  packets;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        packets = 0;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If the interface is found, get the lost packets count. */
        if (iface != NU_NULL)
            packets = iface->eth->LostPkts;
        else
            packets = 0;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* return the packets */
    return (packets);

} /* MIB2_Get_1757LostPkts */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Add_1757LostPkts
*
* DESCRIPTION
*
*       This function adds/sets the interface's lost packets count
*
* INPUTS
*
*      index                    The interface index of the device.
*      packets                  The count to be stored.
*      reset                    NU_TRUE if the value is to be set.
*                               NU_FALSE if the value is to be added.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Add_1757LostPkts(UINT32 index, UINT32 packets, INT16 reset)
{
    MIB2_INTERFACE_STRUCT   *iface;
    INT16                   status;

    /* Find the target interface. */
    iface = MIB2_Interface_Get(index);

    /* If the interface is found, set the packets count. */
    if (iface != NU_NULL)
    {
        if (reset == NU_TRUE)
            iface->eth->LostPkts = packets;
        else
            iface->eth->LostPkts += packets;

        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Returning status */
    return (status);

} /* MIB2_Add_1757LostPkts */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_1757DiscardedPkts
*
* DESCRIPTION
*
*       This function gets the interface's discarded packets count.
*
* INPUTS
*
*      index                    The interface index of the device.
*
* OUTPUTS
*
*       UINT32                  If a device is found with the specified
*                               index, this function returns the count.
*                               Otherwise, it returns 0.
*
*************************************************************************/
UINT32 MIB2_Get_1757DiscardedPkts(UINT32 index)
{
    MIB2_INTERFACE_STRUCT   *iface;
    UINT32                  packets;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        packets = 0;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If the interface is found, get the discarded packets count. */
        if (iface != NU_NULL)
            packets = iface->eth->DiscardedPkts;
        else
            packets = 0;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* return the packets */
    return (packets);

} /* MIB2_Get_1757DiscardedPkts */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Add_1757DiscardedPkts
*
* DESCRIPTION
*
*       This function adds/sets the interface's discarded packets count
*
* INPUTS
*
*      index                    The interface index of the device.
*      packets                  The count to be stored.
*      reset                    NU_TRUE if the value is to be set.
*                               NU_FALSE if the value is to be added.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Add_1757DiscardedPkts(UINT32 index, UINT32 packets,
                                 INT16 reset)
{
    MIB2_INTERFACE_STRUCT   *iface;
    INT16                   status;

    /* Find the target interface. */
    iface = MIB2_Interface_Get(index);

    /* If the interface is found, set the packets count. */
    if (iface != NU_NULL)
    {
        if (reset == NU_TRUE)
            iface->eth->DiscardedPkts = packets;
        else
            iface->eth->DiscardedPkts += packets;

        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Returning status */
    return (status);

} /* MIB2_Add_1757DiscardedPkts */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_1757OutPkts
*
* DESCRIPTION
*
*       This function gets the interface's outgoing packets count.
*
* INPUTS
*
*      index                    The interface index of the device.
*
* OUTPUTS
*
*       UINT32                  If a device is found with the specified
*                               index, this function returns the count.
*                               Otherwise, it returns 0.
*
*************************************************************************/
UINT32 MIB2_Get_1757OutPkts(UINT32 index)
{
    MIB2_INTERFACE_STRUCT   *iface;
    UINT32                  packets;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        packets = 0;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If the interface is found, get the outgoing packets count. */
        if (iface != NU_NULL)
            packets = iface->eth->OutPkts;
        else
            packets = 0;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* return the packets */
    return (packets);

} /* MIB2_Get_1757OutPkts */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Add_1757OutPkts
*
* DESCRIPTION
*
*       This function adds/sets the interface's discarded packets count
*
* INPUTS
*
*      index                    The interface index of the device.
*      packets                  The count to be stored.
*      reset                    NU_TRUE if the value is to be set.
*                               NU_FALSE if the value is to be added.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Add_1757OutPkts(UINT32 index, UINT32 packets, INT16 reset)
{
    MIB2_INTERFACE_STRUCT   *iface;
    INT16                   status;

    /* Find the target interface. */
    iface = MIB2_Interface_Get(index);

    /* If the interface is found, set the packets count. */
    if (iface != NU_NULL)
    {
        if (reset == NU_TRUE)
            iface->eth->OutPkts = packets;
        else
            iface->eth->OutPkts += packets;

        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Returning status */
    return (status);

} /* MIB2_Add_1757OutPkts */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_1757OutOctets
*
* DESCRIPTION
*
*       This function gets the interface's outgoing octets count.
*
* INPUTS
*
*      index                    The interface index of the device.
*
* OUTPUTS
*
*       UINT32                  If a device is found with the specified
*                               index, this function returns the count.
*                               Otherwise, it returns 0.
*
*************************************************************************/
UINT32 MIB2_Get_1757OutOctets(UINT32 index)
{
    MIB2_INTERFACE_STRUCT   *iface;
    UINT32                  octets;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        octets = 0;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If the interface is found, get the outgoing octets count. */
        if (iface != NU_NULL)
            octets = iface->eth->OutOctets;
        else
            octets = 0;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* return the octets. */
    return (octets);

} /* MIB2_Get_1757OutOctets */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Add_1757OutOctets
*
* DESCRIPTION
*
*       This function adds/sets the interface's octets count
*
* INPUTS
*
*      index                    The interface index of the device.
*      octets                   The count to be stored.
*      reset                    NU_TRUE if the value is to be set.
*                               NU_FALSE if the value is to be added.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Add_1757OutOctets (UINT32 index, UINT32 octets, INT16 reset)
{
    MIB2_INTERFACE_STRUCT   *iface;
    INT16                   status;

    /* Find the target interface. */
    iface = MIB2_Interface_Get(index);

    /* If the interface is found, set the octet count. */
    if (iface != NU_NULL)
    {
        if(reset == NU_TRUE)
            iface->eth->OutOctets = octets;
        else
            iface->eth->OutOctets += octets;

        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Returning status */
    return (status);

} /* MIB2_Add_1757OutOctets */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_1757OutMulticastPkts
*
* DESCRIPTION
*
*       This function gets the interface's outgoing multicast packets
*       count.
*
* INPUTS
*
*      index                    The interface index of the device.
*
* OUTPUTS
*
*       UINT32                  If a device is found with the specified
*                               index, this function returns the count.
*                               Otherwise, it returns 0.
*
*************************************************************************/
UINT32 MIB2_Get_1757OutMulticastPkts(UINT32 index)
{
    MIB2_INTERFACE_STRUCT   *iface;
    UINT32                  packets;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        packets = 0;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If the interface is found, get the outgoing packets count. */
        if (iface != NU_NULL)
            packets = iface->eth->OutMulticastPkts;
        else
            packets = 0;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* return the packets. */
    return (packets);

} /* MIB2_Get_1757OutMulticastPkts */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Add_1757OutMulticastPkts
*
* DESCRIPTION
*
*       This function adds/sets the interface's packets count
*
* INPUTS
*
*      index                    The interface index of the device.
*      packets                  The count to be stored.
*      reset                    NU_TRUE if the value is to be set.
*                               NU_FALSE if the value is to be added.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Add_1757OutMulticastPkts(UINT32 index, UINT32 packets,
                                    INT16 reset)
{
    MIB2_INTERFACE_STRUCT   *iface;
    INT16                   status;

    /* Find the target interface. */
    iface = MIB2_Interface_Get(index);

    /* If the interface is found, set the packets count. */
    if (iface != NU_NULL)
    {
        if (reset == NU_TRUE)
            iface->eth->OutMulticastPkts = packets;
        else
            iface->eth->OutMulticastPkts += packets;

        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Returning status */
    return (status);

} /* MIB2_Add_1757OutMulticastPkts */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_1757OutBroadcastPkts
*
* DESCRIPTION
*
*       This function gets the interface's outgoing broadcast packets
*       count.
*
* INPUTS
*
*      index                    The interface index of the device.
*
* OUTPUTS
*
*       UINT32                  If a device is found with the specified
*                               index, this function returns the count.
*                               Otherwise, it returns 0.
*
*************************************************************************/
UINT32 MIB2_Get_1757OutBroadcastPkts(UINT32 index)
{
    MIB2_INTERFACE_STRUCT   *iface;
    UINT32                  packets;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        packets = 0;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If the interface is found, get the outgoing packets count. */
        if (iface != NU_NULL)
            packets = iface->eth->OutBroadcastPkts;
        else
            packets = 0;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* Return the packets. */
    return (packets);

} /* MIB2_Get_1757OutBroadcastPkts */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Add_1757OutBroadcastPkts
*
* DESCRIPTION
*
*       This function adds/sets the interface's packets count
*
* INPUTS
*
*      index                    The interface index of the device.
*      packets                  The count to be stored.
*      reset                    NU_TRUE if the value is to be set.
*                               NU_FALSE if the value is to be added.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Add_1757OutBroadcastPkts(UINT32 index, UINT32 packets,
                                    INT16 reset)
{
    MIB2_INTERFACE_STRUCT   *iface;
    INT16                   status;

    /* Find the target interface. */
    iface = MIB2_Interface_Get(index);

    /* If the interface is found, set the packets count. */
    if (iface != NU_NULL)
    {
        if (reset == NU_TRUE)
            iface->eth->OutBroadcastPkts = packets;
        else
            iface->eth->OutBroadcastPkts += packets;

        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Returning status */
    return (status);

} /* MIB2_Add_1757OutBroadcastPkts */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Get_1757OutErrors
*
* DESCRIPTION
*
*       This function gets the interface's outgoing errors count.
*
* INPUTS
*
*      index                    The interface index of the device.
*
* OUTPUTS
*
*       UINT32                  If a device is found with the specified
*                               index, this function returns the count.
*                               Otherwise, it returns 0.
*
*************************************************************************/
UINT32 MIB2_Get_1757OutErrors(UINT32 index)
{
    MIB2_INTERFACE_STRUCT   *iface;
    UINT32                  errors;

    /* Grab the semaphore. */
    if (NU_Obtain_Semaphore(&TCP_Resource, NU_SUSPEND) != NU_SUCCESS)
    {
        NLOG_Error_Log("Failed to obtain semaphore", NERR_SEVERE,
                       __FILE__, __LINE__);

        errors = 0;
    }
    else
    {
        /* Find the target interface. */
        iface = MIB2_Interface_Get(index);

        /* If the interface is found, get the outgoing errors count. */
        if (iface != NU_NULL)
            errors = iface->eth->OutErrors;
        else
            errors = 0;

        /* Release the semaphore. */
        if (NU_Release_Semaphore(&TCP_Resource) != NU_SUCCESS)
            NLOG_Error_Log("Failed to release semaphore", NERR_SEVERE,
                           __FILE__, __LINE__);
    }

    /* return the packets. */
    return (errors);

} /* MIB2_Get_1757OutErrors */

/*************************************************************************
*
* FUNCTION
*
*       MIB2_Add_1757OutErrors
*
* DESCRIPTION
*
*       This function adds/sets the interface's outgoing errors count
*
* INPUTS
*
*      index                    The interface index of the device.
*      errors                   The count to be stored.
*      reset                    NU_TRUE if the value is to be set.
*                               NU_FALSE if the value is to be added.
*
* OUTPUTS
*
*       NU_SUCCESS              When request was successful.
*       MIB2_UNSUCCESSFUL       When request was unsuccessful.
*
*************************************************************************/
INT16 MIB2_Add_1757OutErrors(UINT32 index, UINT32 errors, INT16 reset)
{
    MIB2_INTERFACE_STRUCT   *iface;
    INT16                   status;

    /* Find the target interface. */
    iface = MIB2_Interface_Get(index);

    /* If the interface is found, set the packets count. */
    if (iface != NU_NULL)
    {
        if (reset == NU_TRUE)
            iface->eth->OutErrors = errors;
        else
            iface->eth->OutErrors += errors;

        status = NU_SUCCESS;
    }

    /* Otherwise, return an error */
    else
        status = MIB2_UNSUCCESSFUL;

    /* Returning status */
    return (status);

} /* MIB2_Add_1757OutErrors */

#endif /* (INCLUDE_MIB_RMON1 == NU_TRUE) */
#endif /* (INCLUDE_SNMP == NU_TRUE) */

#endif /* (INCLUDE_MIB2_RFC1213 == NU_TRUE) */

