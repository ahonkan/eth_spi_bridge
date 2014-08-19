/*************************************************************************
*
*               Copyright 1997 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS
* THE PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS
* SUBJECT TO LICENSE TERMS.
*
**************************************************************************
**************************************************************************
*
*   FILE NAME
*
*       pmn.c
*
*   COMPONENT
*
*       PMN - PPP MIB access to NCP
*
*   DESCRIPTION
*
*       Network Control MIB access functions for RFC 1473.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       PML_GetIpOperStatus
*       PMN_GetIpLocalToRemoteCompressionProtocol
*       PMN_GetIpRemoteToLocalCompressionProtocol
*       PMN_GetIpRemoteMaxSlotId
*       PMN_GetIpLocalMaxSlotId
*       PMN_GetIpConfigAdminStatus
*       PMN_GetIpConfigCompression
*       PMN_SetIpConfigAdminStatus
*       PMN_SetIpConfigCompression
*
*   DEPENDENCIES
*
*       nu_ppp.h
*
*************************************************************************/
#include "drivers/nu_ppp.h"


/*************************************************************************
* FUNCTION
*
*        PMN_GetIpOperStatus
*
* DESCRIPTION
*
*        Get the operational status of the IPCP layer. If IPCP is
*        up, then 1 is returned. Otherwise, 2 is returned.
*
* INPUTS
*
*        *dev_ptr
*        *value
*
* OUTPUTS
*
*        STATUS
*
*************************************************************************/
STATUS PMN_GetIpOperStatus(DV_DEVICE_ENTRY *dev_ptr, INT32 *value)
{
    LINK_LAYER       *link_layer;

    /* Validate the device pointer and make sure it is a PPP device. */
    if (dev_ptr && dev_ptr->dev_type == DVT_PPP)
    {
        /* Access the PPP structure variable. */
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

        if (link_layer != NU_NULL)
        {
#if (INCLUDE_IPV4 == NU_TRUE)
            if (link_layer->ncp.state == OPENED)
                *value = 1;
            else
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
            if (link_layer->ncp6.state == OPENED)
                *value = 1;
            else
#endif
                *value = 2;

            return NU_SUCCESS;
        }
    }

    /* Something was wrong. Perhaps this isn't a PPP device or
       a link was not established. */
    return PM_ERROR;

} /* PMN_GetIpOperStatus */



/*************************************************************************
* FUNCTION
*
*        PMN_GetIpLocalToRemoteCompressionProtocol
*
* DESCRIPTION
*
*        Determine whether IP compression is used for packets transmitted
*        to the peer. Note: Since Nucleus PPP does not currently
*        support IP compression, only 1 (no compression) is returned.
*
* INPUTS
*
*        *dev_ptr
*        *value
*
* OUTPUTS
*
*        NU_SUCCESS
*        PM_ERROR
*
*************************************************************************/
STATUS PMN_GetIpLocalToRemoteCompressionProtocol(DV_DEVICE_ENTRY *dev_ptr,
                                                 INT32 *value)
{
    LINK_LAYER       *link_layer;
    STATUS           status = PM_ERROR;

    /* Validate the device pointer and make sure it is a PPP device. */
    if (dev_ptr && dev_ptr->dev_type == DVT_PPP)
    {
        /* Access the PPP structure variable. */
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

        if (link_layer != NU_NULL)
        {
#if (INCLUDE_IPV4 == NU_TRUE)
           if(link_layer->ncp.state == OPENED)
           {
               /* IP compression is not currently supported. */
               *value = (INT32)1;
                status = NU_SUCCESS;
           }
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
           if(link_layer->ncp6.state == OPENED)
           {
               /* IP compression is not currently supported. */
               *value = (INT32)1;
               status = NU_SUCCESS;
           }
#endif
        }
    }

    return status;

} /* PMN_GetIpLocalToRemoteCompressionProtocol */


/*************************************************************************
* FUNCTION
*
*        PMN_GetIpRemoteToLocalCompressionProtocol
*
* DESCRIPTION
*
*        Determine whether IP compression is used for packets received
*        from the peer. Note: Since Nucleus PPP does not currently
*        support IP compression, only 1 (no compression) is returned.
*
* INPUTS
*
*        *dev_ptr
*        *value
*
* OUTPUTS
*
*        NU_SUCCESS
*        PM_ERROR
*
*************************************************************************/
STATUS PMN_GetIpRemoteToLocalCompressionProtocol(DV_DEVICE_ENTRY *dev_ptr,
                                                 INT32 *value)
{
    LINK_LAYER       *link_layer;
    STATUS            status = PM_ERROR;

    /* Validate the device pointer and make sure it is a PPP device. */
    if (dev_ptr && dev_ptr->dev_type == DVT_PPP)
    {
        /* Access the PPP structure variable. */
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

        if (link_layer != NU_NULL)
        {
#if (INCLUDE_IPV4 == NU_TRUE)
           if(link_layer->ncp.state == OPENED)
           {
                /* IP compression is not currently supported. */
                *value = (INT32)1;
                status = NU_SUCCESS;
           }
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
           if(link_layer->ncp6.state == OPENED)
           {
               /* IP compression is not currently supported. */
                *value = (INT32)1;
                status = NU_SUCCESS;
           }
#endif
        }
    }
    /* Something was wrong. Perhaps this isn't a PPP device or
       a link was not established. */
    return status;

} /* PMN_GetIpRemoteToLocalCompressionProtocol */



/*************************************************************************
* FUNCTION
*
*        PMN_GetIpRemoteMaxSlotId
*
* DESCRIPTION
*
*        Retrieve the Max Slot Id used for packets transmitted
*        to the peer. Note: Since Nucleus PPP does not currently
*        support IP compression, a max of 0 is returned.
*
* INPUTS
*
*        *dev_ptr
*        *value
*
* OUTPUTS
*
*        NU_SUCCESS
*        PM_ERROR
*
*************************************************************************/
STATUS PMN_GetIpRemoteMaxSlotId(DV_DEVICE_ENTRY *dev_ptr, INT32 *value)
{
    LINK_LAYER       *link_layer;
    STATUS            status = PM_ERROR;

    /* Validate the device pointer and make sure it is a PPP device. */
    if (dev_ptr && dev_ptr->dev_type == DVT_PPP)
    {
        /* Access the PPP structure variable. */
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

        if (link_layer != NU_NULL)
        {
#if (INCLUDE_IPV4 == NU_TRUE)
           if(link_layer->ncp.state == OPENED)
           {
                /* IP compression is not currently supported. */
                *value = (INT32)0;
                status = NU_SUCCESS;
           }
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
           if(link_layer->ncp6.state == OPENED)
           {
                /* IP compression is not currently supported. */
                *value = (INT32)0;
                status = NU_SUCCESS;
           }
#endif
        }
    }

    return status;

} /* PMN_GetIpRemoteMaxSlotId */



/*************************************************************************
* FUNCTION
*
*        PMN_GetIpLocalMaxSlotId
*
* DESCRIPTION
*
*        Retrieve the Max Slot Id used for packets received
*        from the peer. Note: Since Nucleus PPP does not currently
*        support IP compression, a max of 0 is returned.
*
* INPUTS
*
*        *dev_ptr
*        *value
*
* OUTPUTS
*
*        NU_SUCCESS
*        PM_ERROR
*
*************************************************************************/
STATUS PMN_GetIpLocalMaxSlotId(DV_DEVICE_ENTRY *dev_ptr, INT32 *value)
{
    LINK_LAYER       *link_layer;
    STATUS            status = PM_ERROR;

    /* Validate the device pointer and make sure it is a PPP device. */
    if (dev_ptr && dev_ptr->dev_type == DVT_PPP)
    {
        /* Access the PPP structure variable. */
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

        if (link_layer != NU_NULL)
        {
#if (INCLUDE_IPV4 == NU_TRUE)
           if(link_layer->ncp.state == OPENED)
           {
                /* IP compression is not currently supported. */
                *value = (INT32)0;
                status = NU_SUCCESS;
           }
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
           if(link_layer->ncp6.state == OPENED)
           {
                /* IP compression is not currently supported. */
                *value = (INT32)0;
                status = NU_SUCCESS;
           }
#endif
        }
    }

    return status;

} /* PMN_GetIpLocalMaxSlotId */



/*************************************************************************
* FUNCTION
*
*        PMN_GetIpConfigAdminStatus
*
* DESCRIPTION
*
*        Retrieve the administrative setting for the desired operational
*        status of the IPCP layer. If IPCP is up, then 1 is returned.
*        Otherwise, 2 is returned.
*
* INPUTS
*
*        *dev_ptr
*        *value
*
* OUTPUTS
*
*        NU_SUCCESS
*        PM_ERROR
*
*************************************************************************/
STATUS PMN_GetIpConfigAdminStatus(DV_DEVICE_ENTRY *dev_ptr, INT32 *value)
{
    LINK_LAYER       *link_layer;

    /* Validate the device pointer and make sure it is a PPP device. */
    if (dev_ptr && dev_ptr->dev_type == DVT_PPP)
    {
        /* Access the PPP structure variable. */
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

        if (link_layer != NU_NULL)
        {
#if (INCLUDE_IPV4 == NU_TRUE)
           if(link_layer->ncp.state == OPENED)
           {
                *value = (INT32)1;
           }
           else
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
           if(link_layer->ncp6.state == OPENED)
           {
                /* IP compression is not currently supported. */
                *value = (INT32)1;
           }
           else
#endif
               *value = 2;

            return NU_SUCCESS;
        }
    }

    /* Something was wrong. Perhaps this isn't a PPP device or
       a link was not established. */
    return PM_ERROR;

} /* PMN_GetIpConfigAdminStatus */



/*************************************************************************
* FUNCTION
*
*        PMN_GetIpConfigCompression
*
* DESCRIPTION
*
*        Retrieve the administrative setting for the use of IP
*        compression for IPCP. Note: Since Nucleus PPP does not currently
*        support IP compression, only 1 (no compression) is returned.
*
* INPUTS
*
*        *dev_ptr
*        *value
*
* OUTPUTS
*
*        NU_SUCCESS
*        PM_ERROR
*
*************************************************************************/
STATUS PMN_GetIpConfigCompression(DV_DEVICE_ENTRY* dev_ptr, INT32 *value)
{
    LINK_LAYER       *link_layer;
    STATUS           status = PM_ERROR;

    /* Validate the device pointer and make sure it is a PPP device. */
    if (dev_ptr && dev_ptr->dev_type == DVT_PPP)
    {
        /* Access the PPP structure variable. */
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

        if (link_layer != NU_NULL)
        {
#if (INCLUDE_IPV4 == NU_TRUE)
           if(link_layer->ncp.state == OPENED)
           {
                /* IP compression is not currently supported. */
                *value = (INT32)1;
                status = NU_SUCCESS;
           }
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
           if(link_layer->ncp6.state == OPENED)
           {
                /* IP compression is not currently supported. */
                *value = (INT32)1;
                status = NU_SUCCESS;
           }
#endif
        }
    }

    return status;

} /* PMN_GetIpConfigCompression */



/*************************************************************************
* FUNCTION
*
*        PMN_SetIpConfigAdminStatus
*
* DESCRIPTION
*
*        Set the administrative setting for the desired operational
*        status of the IPCP layer. If IPCP is set to 1 (open), then a
*        NCP_OPEN_REQUEST event is set, which will open the IPCP layer
*        if it is not already up. Likewise, if it is set to 2 (close),
*        then a NCP_CLOSE_REQUEST event is set, which will close an
*        IPCP link.
*
* INPUTS
*
*        *dev_ptr
*        value
*
* OUTPUTS
*
*        NU_SUCCESS
*        PM_ERROR
*
*************************************************************************/
STATUS PMN_SetIpConfigAdminStatus(DV_DEVICE_ENTRY *dev_ptr, INT32 value)
{
    LINK_LAYER       *link_layer;
    STATUS           status = PM_ERROR;

    /* Check parameters. */
    if (value != 1 && value != 2)
        return PM_ERROR;

    /* Validate the device pointer and make sure it is a PPP device. */
    if (dev_ptr && dev_ptr->dev_type == DVT_PPP)
    {
        /* Access the PPP structure variable. */
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

        if (link_layer != NU_NULL)
        {
#if (INCLUDE_IPV4 == NU_TRUE)
            /* If the device is an IPv4 device. */
            if(!(link_layer->ncp.options.ipcp.flags & PPP_FLAG_NO_IPV4))
            {
                /* If NCP is open, it is ok to close it (state 2). Likewise, if NCP
                   is closed, it is ok to open it (state 1). Otherwise, an event
                   shouldn't be sent. */
                if (link_layer->ncp.state == OPENED && value == 2)
                    EQ_Put_Event(IPCP_Event, (UNSIGNED)dev_ptr, NCP_CLOSE_REQUEST);

                else if (link_layer->ncp.state != OPENED && value == 1)
                    EQ_Put_Event(IPCP_Event, (UNSIGNED)dev_ptr, NCP_OPEN_REQUEST);

                status = NU_SUCCESS;
            }
#endif
#if (INCLUDE_IPV6 == NU_TRUE)
            /* If the device is an IPv4 device. */
            if (dev_ptr->dev_flags & DV6_IPV6)
            {
                /* If NCP is open, it is ok to close it (state 2). Likewise, if NCP
                   is closed, it is ok to open it (state 1). Otherwise, an event
                   shouldn't be sent. */
                if (link_layer->ncp6.state == OPENED && value == 2)
                    EQ_Put_Event(IPV6CP_Event, (UNSIGNED)dev_ptr, NCP_CLOSE_REQUEST);

                else if (link_layer->ncp6.state != OPENED && value == 1)
                    EQ_Put_Event(IPV6CP_Event, (UNSIGNED)dev_ptr, NCP_OPEN_REQUEST);

                status = NU_SUCCESS;
            }
#endif
        }
    }

    return status;

} /* PMN_SetIpConfigAdminStatus */



/*************************************************************************
* FUNCTION
*
*        PMN_SetIpConfigCompression
*
* DESCRIPTION
*
*        Administrative set whether or not IP compression will be
*        used in the next IPCP link. Note: Since Nucleus PPP does not
*        currently support IP compression, the setting is ignored.
*
* INPUTS
*
*        *dev_ptr
*        value
*
* OUTPUTS
*
*        NU_SUCCESS
*        PM_ERROR
*
*************************************************************************/
STATUS PMN_SetIpConfigCompression(DV_DEVICE_ENTRY* dev_ptr, INT32 value)
{
    /* Check parameters. */
    if (value != 1 && value != 2)
        return PM_ERROR;

    /* Validate the device pointer and make sure it is a PPP device. */
    if (dev_ptr && dev_ptr->dev_type == DVT_PPP)
    {
        /* IP compression is currently not supported. Just return ok. */
        return NU_SUCCESS;
    }

    /* Something was wrong. Perhaps this isn't a PPP device. */
    return PM_ERROR;

} /* PMN_SetIpConfigCompression */
