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
*       pml.c
*
*   COMPONENT
*
*       PML - PPP MIB access to LCP
*
*   DESCRIPTION
*
*       Link Control MIB access functions for RFC 1471.
*
*   DATA STRUCTURES
*
*       None
*
*   FUNCTIONS
*
*       PML_GetPhysicalIndex
*       PML_GetBadAddresses
*       PML_GetBadControls
*       PML_GetPacketTooLongs
*       PML_GetBadFCSs
*       PML_GetLocalMRU
*       PML_GetRemoteMRU
*       PML_GetLocalToPeerACCMap
*       PML_GetPeerToLocalACCMap
*       PML_GetLocalToRemoteProtocolCompression
*       PML_GetRemoteToLocalProtocolCompression
*       PML_GetLocalToRemoteACCompression
*       PML_GetRemoteToLocalACCompression
*       PML_GetTransmitFcsSize
*       PML_GetReceiveFcsSize
*       PML_GetInitialMRU
*       PML_GetInitialReceiveACCMap
*       PML_GetInitialTransmitACCMap
*       PML_GetInitialMagicNumber
*       PML_GetInitialFcsSize
*       PML_SetInitialMRU
*       PML_SetInitialReceiveACCMap
*       PML_SetInitialTransmitACCMap
*       PML_SetInitialMagicNumber
*       PML_SetInitialFcsSize
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
*        PML_GetPhysicalIndex
*
* DESCRIPTION
*
*        Get the output interface for this device, if it is different
*        than this one.
*
* INPUTS
*
*        *dev_ptr
*
* OUTPUTS
*
*        Device index or PM_ERROR
*
*************************************************************************/
INT32 PML_GetPhysicalIndex(DV_DEVICE_ENTRY *dev_ptr)
{
    LINK_LAYER *link_layer;

    /* If the device is valid, find its output device. */
    if (dev_ptr && dev_ptr->dev_type == DVT_PPP &&
        dev_ptr->dev_link_layer != NU_NULL)
    {
        /* Access the PPP structure variable. */
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
		
        /* If we have a valid pointer to a hardware device, */
        if (link_layer->hwi.hdev_ptr != NU_NULL)
        {
            /* then return the index of the pointed-to hardware
               device ("1 based" per dts0100554595) */
            return link_layer->hwi.hdev_ptr->dev_index + 1;
        }

        else
        {
            /* otherwise, return the index of the current device
               device ("1 based" per dts0100554595).  For example,
               currently PPP0E doesn't set hdev_ptr. */
            return dev_ptr->dev_index + 1;
        }
    }

    /* An error must have occurred above. */
    return PM_ERROR;
}



/*************************************************************************
* FUNCTION
*
*        PML_GetBadAddresses
*
* DESCRIPTION
*
*        Returns the number of bad address fields received by this
*        PPP link. Returns an error value if LCP is not open.
*
* INPUTS
*
*        *dev_ptr
*        *value
*
* OUTPUTS
*
*        Error count or PM_ERROR
*
*************************************************************************/
STATUS PML_GetBadAddresses(DV_DEVICE_ENTRY *dev_ptr, UINT32 *value)
{
    LINK_LAYER       *link_layer;

    /* Validate the device pointer and make sure it is a PPP device. */
    if (dev_ptr && dev_ptr->dev_type == DVT_PPP)
    {
        /* Access the PPP structure variable. */
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

        if (link_layer != NU_NULL && link_layer->lcp.state == OPENED)
        {
            *value = (UINT32)link_layer->status.address_field_errors;
            return NU_SUCCESS;
        }
    }

    /* Something was wrong. Perhaps this isn't a PPP device or
       a link was not established. */
    return PM_ERROR;
}



/*************************************************************************
* FUNCTION
*
*        PML_GetBadControls
*
* DESCRIPTION
*
*        Returns the number of bad control fields received by this
*        PPP link. Returns an error value if LCP is not open.
*
* INPUTS
*
*        *dev_ptr
*        *value
*
* OUTPUTS
*
*        Error count or PM_ERROR
*
*************************************************************************/
STATUS PML_GetBadControls(DV_DEVICE_ENTRY *dev_ptr, UINT32 *value)
{
    LINK_LAYER       *link_layer;

    /* Validate the device pointer and make sure it is a PPP device. */
    if (dev_ptr && dev_ptr->dev_type == DVT_PPP)
    {
        /* Access the PPP structure variable. */
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

        if (link_layer != NU_NULL && link_layer->lcp.state == OPENED)
        {
            *value = (UINT32)link_layer->status.control_field_errors;
            return NU_SUCCESS;
        }
    }

    /* Something was wrong. Perhaps this isn't a PPP device or
       a link was not established. */
    return PM_ERROR;
}



/*************************************************************************
* FUNCTION
*
*        PML_GetPacketTooLongs
*
* DESCRIPTION
*
*        Returns the number of packets that were too large to be
*        received by this PPP link. Returns an error value if LCP
*        is not open.
*
* INPUTS
*
*        *dev_ptr
*        *value
*
* OUTPUTS
*
*        Error count or PM_ERROR
*
*************************************************************************/
STATUS PML_GetPacketTooLongs(DV_DEVICE_ENTRY *dev_ptr, UINT32 *value)
{
    LINK_LAYER       *link_layer;

    /* Validate the device pointer and make sure it is a PPP device. */
    if (dev_ptr && dev_ptr->dev_type == DVT_PPP)
    {
        /* Access the PPP structure variable. */
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

        if (link_layer != NU_NULL && link_layer->lcp.state == OPENED)
        {
            *value = (UINT32)link_layer->status.overrun_errors;
            return NU_SUCCESS;
        }
    }

    /* Something was wrong. Perhaps this isn't a PPP device or
       a link was not established. */
    return PM_ERROR;
}



/*************************************************************************
* FUNCTION
*
*        PML_GetBadFCSs
*
* DESCRIPTION
*
*        Returns the number of packets with an incorrect FCS received
*        by this PPP link. Returns an error value if LCP is not open.
*
* INPUTS
*
*        *dev_ptr
*        *value
*
* OUTPUTS
*
*        Error count or PM_ERROR
*
*************************************************************************/
STATUS PML_GetBadFCSs(DV_DEVICE_ENTRY *dev_ptr, UINT32 *value)
{
    LINK_LAYER       *link_layer;

    /* Validate the device pointer and make sure it is a PPP device. */
    if (dev_ptr && dev_ptr->dev_type == DVT_PPP)
    {
        /* Access the PPP structure variable. */
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

        if (link_layer != NU_NULL && link_layer->lcp.state == OPENED)
        {
            *value = (UINT32)link_layer->status.fcs_errors;
            return NU_SUCCESS;
        }
    }

    /* Something was wrong. Perhaps this isn't a PPP device or
       a link was not established. */
    return PM_ERROR;
}



/*************************************************************************
* FUNCTION
*
*        PML_GetLocalMRU
*
* DESCRIPTION
*
*        Returns the local MRU value established for this
*        PPP link. Returns an error value if LCP is not open.
*
* INPUTS
*
*        *dev_ptr
*        *value
*
* OUTPUTS
*
*        Value or PM_ERROR
*
*************************************************************************/
STATUS PML_GetLocalMRU(DV_DEVICE_ENTRY *dev_ptr, INT32 *value)
{
    LINK_LAYER       *link_layer;

    /* Validate the device pointer and make sure it is a PPP device. */
    if (dev_ptr && dev_ptr->dev_type == DVT_PPP)
    {
        /* Access the PPP structure variable. */
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

        if (link_layer != NU_NULL && link_layer->lcp.state == OPENED)
        {
            *value = (INT32)link_layer->lcp.options.local.mru;
            return NU_SUCCESS;
        }
    }

    /* Something was wrong. Perhaps this isn't a PPP device or
       a link was not established. */
    return PM_ERROR;
}



/*************************************************************************
* FUNCTION
*
*        PML_GetRemoteMRU
*
* DESCRIPTION
*
*        Returns the remote MRU value established for this
*        PPP link. Returns an error value if LCP is not open.
*
* INPUTS
*
*        *dev_ptr
*        *value
*
* OUTPUTS
*
*        Value or PM_ERROR
*
*************************************************************************/
STATUS PML_GetRemoteMRU(DV_DEVICE_ENTRY *dev_ptr, INT32 *value)
{
    LINK_LAYER       *link_layer;

    /* Validate the device pointer and make sure it is a PPP device. */
    if (dev_ptr && dev_ptr->dev_type == DVT_PPP)
    {
        /* Access the PPP structure variable. */
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

        if (link_layer != NU_NULL && link_layer->lcp.state == OPENED)
        {
            *value = (INT32)link_layer->lcp.options.remote.mru;
            return NU_SUCCESS;
        }
    }

    /* Something was wrong. Perhaps this isn't a PPP device or
       a link was not established. */
    return PM_ERROR;
}



/*************************************************************************
* FUNCTION
*
*        PML_GetLocalToPeerACCMap
*
* DESCRIPTION
*
*        Returns the ACCM to be used when sending to the peer.
*        Returns an error value if LCP is not open.
*
* INPUTS
*
*        *dev_ptr
*        *value
*
* OUTPUTS
*
*        Value or PM_ERROR
*
*************************************************************************/
STATUS PML_GetLocalToPeerACCMap(DV_DEVICE_ENTRY *dev_ptr, UINT8 *value)
{
    LINK_LAYER       *link_layer;

    /* Validate the device pointer and make sure it is a PPP device. */
    if (dev_ptr && dev_ptr->dev_type == DVT_PPP)
    {
        /* Access the PPP structure variable. */
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

        if (link_layer != NU_NULL && link_layer->lcp.state == OPENED)
        {
            PUT32(value, 0, link_layer->lcp.options.remote.accm);
            return NU_SUCCESS;
        }
    }

    /* Something was wrong. Perhaps this isn't a PPP device or
       a link was not established. */
    return PM_ERROR;
}



/*************************************************************************
* FUNCTION
*
*        PML_GetPeerToLocalACCMap
*
* DESCRIPTION
*
*        Returns the ACCM to be used when receiving from the peer.
*        Returns an error value if LCP is not open.
*
* INPUTS
*
*        *dev_ptr
*        *value
*
* OUTPUTS
*
*        Value or PM_ERROR
*
*************************************************************************/
STATUS PML_GetPeerToLocalACCMap(DV_DEVICE_ENTRY *dev_ptr, UINT8 *value)
{
    LINK_LAYER       *link_layer;

    /* Validate the device pointer and make sure it is a PPP device. */
    if (dev_ptr && dev_ptr->dev_type == DVT_PPP)
    {
        /* Access the PPP structure variable. */
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

        if (link_layer != NU_NULL && link_layer->lcp.state == OPENED)
        {
            PUT32(value, 0, link_layer->lcp.options.local.accm);
            return NU_SUCCESS;
        }
    }

    /* Something was wrong. Perhaps this isn't a PPP device or
       a link was not established. */
    return PM_ERROR;
}



/*************************************************************************
* FUNCTION
*
*        PML_GetLocalToRemoteProtocolCompression
*
* DESCRIPTION
*
*        Returns whether or not PPP Protocol Compression is being used
*        on outgoing packets to the peer. Returns an error value if
*        LCP is not open.
*
* INPUTS
*
*        *dev_ptr
*        *value
*
* OUTPUTS
*
*        Value or PM_ERROR
*
*************************************************************************/
STATUS PML_GetLocalToRemoteProtocolCompression(DV_DEVICE_ENTRY *dev_ptr,
                                               INT32 *value)
{
    LINK_LAYER       *link_layer;

    /* Validate the device pointer and make sure it is a PPP device. */
    if (dev_ptr && dev_ptr->dev_type == DVT_PPP)
    {
        /* Access the PPP structure variable. */
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

        if (link_layer != NU_NULL && link_layer->lcp.state == OPENED)
        {
            if (link_layer->lcp.options.remote.flags & PPP_FLAG_PFC)
                *value = (INT32)1;
            else
                *value = (INT32)2;

            return NU_SUCCESS;
        }
    }

    /* Something was wrong. Perhaps this isn't a PPP device or
       a link was not established. */
    return PM_ERROR;
}



/*************************************************************************
* FUNCTION
*
*        PML_GetRemoteToLocalProtocolCompression
*
* DESCRIPTION
*
*        Returns whether or not PPP Protocol Compression is being used
*        on incoming packets from the peer. Returns an error value if
*        LCP is not open.
*
* INPUTS
*
*        *dev_ptr
*        *value
*
* OUTPUTS
*
*        Value or PM_ERROR
*
*************************************************************************/
STATUS PML_GetRemoteToLocalProtocolCompression(DV_DEVICE_ENTRY *dev_ptr,
                                               INT32 *value)
{
    LINK_LAYER       *link_layer;

    /* Validate the device pointer and make sure it is a PPP device. */
    if (dev_ptr && dev_ptr->dev_type == DVT_PPP)
    {
        /* Access the PPP structure variable. */
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

        if (link_layer != NU_NULL && link_layer->lcp.state == OPENED)
        {
            if (link_layer->lcp.options.local.flags & PPP_FLAG_PFC)
                *value = (INT32)1;
            else
                *value = (INT32)2;

            return NU_SUCCESS;
        }
    }

    /* Something was wrong. Perhaps this isn't a PPP device or
       a link was not established. */
    return PM_ERROR;
}



/*************************************************************************
* FUNCTION
*
*        PML_GetLocalToRemoteACCompression
*
* DESCRIPTION
*
*        Returns whether or not HDLC Address and Control Compression is
*        being used on outgoing packets to the peer. Returns an error
*        value if LCP is not open.
*
* INPUTS
*
*        *dev_ptr
*        *value
*
* OUTPUTS
*
*        Value or PM_ERROR
*
*************************************************************************/
STATUS PML_GetLocalToRemoteACCompression(DV_DEVICE_ENTRY *dev_ptr,
                                         INT32 *value)
{
    LINK_LAYER       *link_layer;

    /* Validate the device pointer and make sure it is a PPP device. */
    if (dev_ptr && dev_ptr->dev_type == DVT_PPP)
    {
        /* Access the PPP structure variable. */
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

        if (link_layer != NU_NULL && link_layer->lcp.state == OPENED)
        {
            if (link_layer->lcp.options.remote.flags & PPP_FLAG_ACC)
                *value = (INT32)1;
            else
                *value = (INT32)2;

            return NU_SUCCESS;
        }
    }

    /* Something was wrong. Perhaps this isn't a PPP device or
       a link was not established. */
    return PM_ERROR;
}



/*************************************************************************
* FUNCTION
*
*        PML_GetRemoteToLocalACCompression
*
* DESCRIPTION
*
*        Returns whether or not HDLC Address and Control Compression is
*        being used on incoming packets from the peer. Returns an error
*        value if LCP is not open.
*
* INPUTS
*
*        *dev_ptr
*        *value
*
* OUTPUTS
*
*        Value or PM_ERROR
*
*************************************************************************/
STATUS PML_GetRemoteToLocalACCompression(DV_DEVICE_ENTRY *dev_ptr,
                                         INT32 *value)
{
    LINK_LAYER       *link_layer;

    /* Validate the device pointer and make sure it is a PPP device. */
    if (dev_ptr && dev_ptr->dev_type == DVT_PPP)
    {
        /* Access the PPP structure variable. */
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

        if (link_layer != NU_NULL && link_layer->lcp.state == OPENED)
        {
            if (link_layer->lcp.options.local.flags & PPP_FLAG_ACC)
                *value = (INT32)1;
            else
                *value = (INT32)2;

            return NU_SUCCESS;
        }
    }

    /* Something was wrong. Perhaps this isn't a PPP device or
       a link was not established. */
    return PM_ERROR;
}



/*************************************************************************
* FUNCTION
*
*        PML_GetTransmitFcsSize
*
* DESCRIPTION
*
*        Returns the number of bits used in calculating the Frame
*        Check Sequence for outgoing packets. Returns an error value
*        if LCP is not open.
*
* INPUTS
*
*        *dev_ptr
*        *value
*
* OUTPUTS
*
*        Value or PM_ERROR
*
*************************************************************************/
STATUS PML_GetTransmitFcsSize(DV_DEVICE_ENTRY *dev_ptr, INT32 *value)
{
    LINK_LAYER       *link_layer;

    /* Validate the device pointer and make sure it is a PPP device. */
    if (dev_ptr && dev_ptr->dev_type == DVT_PPP)
    {
        /* Access the PPP structure variable. */
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

        if (link_layer != NU_NULL && link_layer->lcp.state == OPENED)
        {
            *value = (INT32)(link_layer->lcp.options.remote.fcs_size);
            return NU_SUCCESS;
        }
    }

    /* Something was wrong. Perhaps this isn't a PPP device or
       a link was not established. */
    return PM_ERROR;
}



/*************************************************************************
* FUNCTION
*
*        PML_GetReceiveFcsSize
*
* DESCRIPTION
*
*        Returns the number of octets used in calculating the Frame
*        Check Sequence for incoming packets. Returns an error value
*        if LCP is not open.
*
* INPUTS
*
*        *dev_ptr
*        *value
*
* OUTPUTS
*
*        Value or PM_ERROR
*
*************************************************************************/
STATUS PML_GetReceiveFcsSize(DV_DEVICE_ENTRY *dev_ptr, INT32 *value)
{
    LINK_LAYER       *link_layer;

    /* Validate the device pointer and make sure it is a PPP device. */
    if (dev_ptr && dev_ptr->dev_type == DVT_PPP)
    {
        /* Access the PPP structure variable. */
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

        if (link_layer != NU_NULL && link_layer->lcp.state == OPENED)
        {
            *value = (INT32)(link_layer->lcp.options.local.fcs_size);
            return NU_SUCCESS;
        }
    }

    /* Something was wrong. Perhaps this isn't a PPP device or
       a link was not established. */
    return PM_ERROR;
}



/*************************************************************************
* FUNCTION
*
*        PML_GetInitialMRU
*
* DESCRIPTION
*
*        Returns the initial local MRU settings that will be used when
*        negotiating a new PPP link.
*
* INPUTS
*
*        *dev_ptr
*        *value
*
* OUTPUTS
*
*        Value or PM_ERROR
*
*************************************************************************/
STATUS PML_GetInitialMRU(DV_DEVICE_ENTRY *dev_ptr, INT32 *value)
{
    LINK_LAYER       *link_layer;

    /* Validate the device pointer and make sure it is a PPP device. */
    if (dev_ptr && dev_ptr->dev_type == DVT_PPP)
    {
        /* Access the PPP structure variable. */
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

        if (link_layer != NU_NULL)
        {
            if (link_layer->lcp.options.local.default_flags & PPP_FLAG_MRU)
                *value = (INT32)link_layer->lcp.options.local.default_mru;
            else
                *value = 0;

            return NU_SUCCESS;
        }
    }

    /* Something was wrong. Perhaps this isn't a PPP device. */
    return PM_ERROR;
}



/*************************************************************************
* FUNCTION
*
*        PML_GetInitialReceiveACCMap
*
* DESCRIPTION
*
*        Returns the initial local ACCM settings that will be used when
*        negotiating a new PPP link.
*
* INPUTS
*
*        *dev_ptr
*        *value
*
* OUTPUTS
*
*        Value or PM_ERROR
*
*************************************************************************/
STATUS PML_GetInitialReceiveACCMap(DV_DEVICE_ENTRY *dev_ptr, UINT8 *value)
{
    LINK_LAYER       *link_layer;

    /* Validate the device pointer and make sure it is a PPP device. */
    if (dev_ptr && dev_ptr->dev_type == DVT_PPP)
    {
        /* Access the PPP structure variable. */
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

        if (link_layer != NU_NULL)
        {
            PUT32(value, 0, link_layer->lcp.options.local.default_accm);
            return NU_SUCCESS;
        }
    }

    /* Something was wrong. Perhaps this isn't a PPP device. */
    return PM_ERROR;
}



/*************************************************************************
* FUNCTION
*
*        PML_GetInitialTransmitACCMap
*
* DESCRIPTION
*
*        Returns the initial foreign ACCM settings that will be used when
*        negotiating a new PPP link.
*
* INPUTS
*
*        *dev_ptr
*        *value
*
* OUTPUTS
*
*        Value or PM_ERROR
*
*************************************************************************/
STATUS PML_GetInitialTransmitACCMap(DV_DEVICE_ENTRY *dev_ptr, UINT8 *value)
{
    LINK_LAYER       *link_layer;

    /* Validate the device pointer and make sure it is a PPP device. */
    if (dev_ptr && dev_ptr->dev_type == DVT_PPP)
    {
        /* Access the PPP structure variable. */
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

        if (link_layer != NU_NULL)
        {
            PUT32(value, 0, link_layer->lcp.options.remote.default_accm);
            return NU_SUCCESS;
        }
    }

    /* Something was wrong. Perhaps this isn't a PPP device. */
    return PM_ERROR;
}



/*************************************************************************
* FUNCTION
*
*        PML_GetInitialMagicNumber
*
* DESCRIPTION
*
*        Returns whether or not the magic number will be negotiated
*        for a new PPP link by default.
*
* INPUTS
*
*        *dev_ptr
*        *value
*
* OUTPUTS
*
*        Value or PM_ERROR
*
*************************************************************************/
STATUS PML_GetInitialMagicNumber(DV_DEVICE_ENTRY *dev_ptr, INT32 *value)
{
    LINK_LAYER       *link_layer;

    /* Validate the device pointer and make sure it is a PPP device. */
    if (dev_ptr && dev_ptr->dev_type == DVT_PPP)
    {
        /* Access the PPP structure variable. */
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
        if (link_layer != NU_NULL)
        {
            if (link_layer->lcp.options.local.default_flags & PPP_FLAG_MAGIC)
                *value = 2;
            else
                *value = 1;

            return NU_SUCCESS;
        }
    }

    /* Something was wrong. Perhaps this isn't a PPP device. */
    return PM_ERROR;
}



/*************************************************************************
* FUNCTION
*
*        PML_GetInitialFcsSize
*
* DESCRIPTION
*
*        Returns the initial FCS size setting that will be used when
*        negotiating a new PPP link.
*
* INPUTS
*
*        *dev_ptr
*        *value
*
* OUTPUTS
*
*        Value or PM_ERROR
*
*************************************************************************/
STATUS PML_GetInitialFcsSize(DV_DEVICE_ENTRY *dev_ptr, INT32 *value)
{
    LINK_LAYER       *link_layer;

    /* Validate the device pointer and make sure it is a PPP device. */
    if (dev_ptr && dev_ptr->dev_type == DVT_PPP)
    {
        /* Access the PPP structure variable. */
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

        if (link_layer != NU_NULL)
        {
            *value = (INT32)(link_layer->lcp.options.local.default_fcs_size);
            return NU_SUCCESS;
        }
    }

    /* Something was wrong. Perhaps this isn't a PPP device. */
    return PM_ERROR;
}



/*************************************************************************
* FUNCTION
*
*        PML_SetInitialMRU
*
* DESCRIPTION
*
*        Sets the initial local MRU value to be negotiated for the
*        next PPP link. The value does not affect a currently
*        established link.
*
* INPUTS
*
*        *dev_ptr
*        value
*
* OUTPUTS
*
*        Value or PM_ERROR
*
*************************************************************************/
STATUS PML_SetInitialMRU(DV_DEVICE_ENTRY *dev_ptr, INT32 value)
{
    LINK_LAYER       *link_layer;

    /* Validate the device pointer and make sure it is a PPP device. */
    if (dev_ptr && dev_ptr->dev_type == DVT_PPP)
    {
        /* Access the PPP structure variable. */
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;
        if (link_layer != NU_NULL)
        {
            /* A value of 0 means that the MRU will not be negotiated. */
            if (value == 0)
                link_layer->lcp.options.local.default_flags &= ~PPP_FLAG_MRU;

            /* Check parameters. PPP_MAX_RX_SIZE is the hardcoded size of the
               HDLC buffers, so this cannot be breached. */
            else if (value < 256 || value > PPP_MAX_RX_SIZE)
                return PM_ERROR;

            else
            {
                link_layer->lcp.options.local.default_mru = (UINT16)value;
                link_layer->lcp.options.local.default_flags |= PPP_FLAG_MRU;
            }

            return NU_SUCCESS;
        }
    }

    /* Something was wrong. Perhaps this isn't a PPP device. */
    return PM_ERROR;
}



/*************************************************************************
* FUNCTION
*
*        PML_SetInitialReceiveACCMap
*
* DESCRIPTION
*
*        Sets the initial local ACCM to be negotiated for the
*        next PPP link. The value does not affect a currently
*        established link.
*
* INPUTS
*
*        *dev_ptr
*        *value
*
* OUTPUTS
*
*        Value or PM_ERROR
*
*************************************************************************/
STATUS PML_SetInitialReceiveACCMap(DV_DEVICE_ENTRY *dev_ptr, UINT8 *value)
{
    LINK_LAYER       *link_layer;

    /* Validate the device pointer and make sure it is a PPP device. */
    if (dev_ptr && dev_ptr->dev_type == DVT_PPP)
    {
        /* Access the PPP structure variable. */
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

        if (link_layer != NU_NULL)
        {
            link_layer->lcp.options.local.default_accm = GET32(value, 0);
            return NU_SUCCESS;
        }
    }

    /* Something was wrong. Perhaps this isn't a PPP device. */
    return PM_ERROR;
}



/*************************************************************************
* FUNCTION
*
*        PML_SetInitialTransmitACCMap
*
* DESCRIPTION
*
*        Sets the initial foreign ACCM to be negotiated for the
*        next PPP link. The value does not affect a currently
*        established link.
*
* INPUTS
*
*        *dev_ptr
*        *value
*
* OUTPUTS
*
*        Value or PM_ERROR
*
*************************************************************************/
STATUS PML_SetInitialTransmitACCMap(DV_DEVICE_ENTRY *dev_ptr, UINT8 *value)
{
    LINK_LAYER       *link_layer;

    /* Validate the device pointer and make sure it is a PPP device. */
    if (dev_ptr && dev_ptr->dev_type == DVT_PPP)
    {
        /* Access the PPP structure variable. */
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

        if (link_layer != NU_NULL)
        {
            link_layer->lcp.options.remote.default_accm = GET32(value, 0);
            return NU_SUCCESS;
        }
    }

    /* Something was wrong. Perhaps this isn't a PPP device. */
    return PM_ERROR;
}



/*************************************************************************
* FUNCTION
*
*        PML_SetInitialMagicNumber
*
* DESCRIPTION
*
*        Sets whether or not the magic number will be negotiated for
*        the next link. The value does not affect a currently
*        established link.
*
* INPUTS
*
*        *dev_ptr
*        value
*
* OUTPUTS
*
*        Value or PM_ERROR
*
*************************************************************************/
STATUS PML_SetInitialMagicNumber(DV_DEVICE_ENTRY *dev_ptr, INT32 value)
{
    LINK_LAYER       *link_layer;

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
            if (value == 1)
                link_layer->lcp.options.local.default_flags &= ~PPP_FLAG_MAGIC;
            else
                link_layer->lcp.options.local.default_flags |= PPP_FLAG_MAGIC;

            return NU_SUCCESS;
        }
    }

    /* Something was wrong. Perhaps this isn't a PPP device. */
    return PM_ERROR;
}



/*************************************************************************
* FUNCTION
*
*        PML_SetInitialFcsSize
*
* DESCRIPTION
*
*        Sets the initial local FCS size (in bits) to be negotiated
*        for the next PPP link. The value does not affect a currently
*        established link.
*
* INPUTS
*
*        *dev_ptr
*        value
*
* OUTPUTS
*
*        Value or PM_ERROR
*
*************************************************************************/
STATUS PML_SetInitialFcsSize(DV_DEVICE_ENTRY *dev_ptr, INT32 value)
{
    LINK_LAYER       *link_layer;

    /* Check parameters. */
    if (value < 0 || value > 128)
        return PM_ERROR;

    /* Validate the device pointer and make sure it is a PPP device. */
    if (dev_ptr && dev_ptr->dev_type == DVT_PPP)
    {
        /* Access the PPP structure variable. */
        link_layer = (LINK_LAYER*)dev_ptr->dev_link_layer;

        if (link_layer != NU_NULL)
        {
            link_layer->lcp.options.local.default_fcs_size = (UINT16)value;
            return NU_SUCCESS;
        }
    }

    /* Something was wrong. Perhaps this isn't a PPP device. */
    return PM_ERROR;
}
