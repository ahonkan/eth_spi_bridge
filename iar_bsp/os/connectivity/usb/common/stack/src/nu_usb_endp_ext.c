/***********************************************************************
*
*           Copyright 2003 Mentor Graphics Corporation
*                         All Rights Reserved.
*
*           THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY
*             INFORMATION WHICH IS THE PROPERTY OF MENTOR
*             GRAPHICS CORPORATION OR ITS LICENSORS AND IS
*                      SUBJECT TO LICENSE TERMS.
*
************************************************************************

*************************************************************************
*
* FILE NAME
*
*        nu_usb_endp_ext.c
*
* COMPONENT
*
*        USB Base
*
* DESCRIPTION
*    This file contains implementation of the NU_USB_ENDP services.
*
*
* DATA STRUCTURES
*
*
* FUNCTIONS
*    NU_USB_ENDP_Get_Desc             - Returns pointer to endpoint
*                                       descriptor.
*    NU_USB_ENDP_Get_Number           - returns the endpoint number.
*    NU_USB_ENDP_Get_Direction        - returns the direction of the
*                                       endpoint.
*    NU_USB_ENDP_Get_Transfer_Type    - returns the transfer type of the
*                                       endpoint.
*    NU_USB_ENDP_Get_Sync_Type         - returns the sync type of the
*                                       endpoint.
*    NU_USB_ENDP_Get_Usage_Type       - returns the usage type of the
*                                       endpoint.
*    NU_USB_ENDP_Get_Max_Packet_Size  - returns the max packet size of the
*                                       endpoint.
*    NU_USB_ENDP_Get_Num_Transactions - returns the number of transaction
*                                       per micro frame thats supported by the
*                                       endpoint.
*    NU_USB_ENDP_Get_Class_Desc       - returns pointer to the class
*                                       specific endpoint descriptor.
*    NU_USB_ENDP_Get_Interval         - returns the interval of the
*                                       endpoint.
*    NU_USB_ENDP_Get_Status           - returns the status of the endpoint.
*    NU_USB_ENDP_Get_Pipe             - returns pointer to the control
*                                       block of the associated pipe.
*    NU_USB_ENDP_Get_Alt_Settg        - returns pointer to the control
*                                       block of the associated alt setting.
*    NU_USB_ENDP_Get_Device           - returns pointer to the control
*                                       block of the associated device.
*    NU_USB_ENDP_Get_Companion_Desc   - Returns the whole SuperSpeed
*                                       endpoint companion descriptor
*                                       associated with an endpoint.
*    NU_USB_ENDP_Get_MaxBurst         - This API returns the bMaxBurst
*                                       field of SuperSpeed endpoint
*                                       companion descriptor.
*    NU_USB_ENDP_Get_Bulk_MaxStreams  - Returns the maximum number of
*                                       Streams of SuperSpeed BULK
*                                       endpoint.
*    NU_USB_ENDP_Get_Iso_MaxPktPerIntrvl
*                                     - Returns the maximum number of
*                                       packets within a service interval
*                                       that this SuperSpeed ISOCHRONOUS
*                                       endpoint supports.
*    NU_USB_ENDP_Get_BytesPerInterval - Returns the bytes this periodic
*                                       (Interrupt and Isochronous)
*                                       endpoint can transfer in a single
*                                       service interval.
*    NU_USB_ENDP_Get_SSEPC_bmAttributes
*                                     - User calls this API to get
*                                       bmAttributes field of a SuperSpeed
*                                       endpoint companion descriptor for
*                                       the particular endpoint.
*
* DEPENDENCIES
*       nu_usb.h              All USB definitions
*
************************************************************************/
#ifndef USB_ENDP_EXT_C
#define	USB_ENDP_EXT_C

/* ======================  Include Files  ============================= */
#include "connectivity/nu_usb.h"

/*************************************************************************
* FUNCTION
*               NU_USB_ENDP_Get_Desc
*
* DESCRIPTION
*     This function returns a pointer to the end point descriptor.
*
* INPUTS
*   cb             Pointer to endpoint control block.
*   endp_desc_out  Pointer to a memory location to hold the pointer to the
*                  endpoint descriptor.
*
* OUTPUTS
*    NU_SUCCESS         Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_ENDP_Get_Desc (NU_USB_ENDP * cb,
                             NU_USB_ENDP_DESC ** endp_desc_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(endp_desc_out);

    *endp_desc_out = cb->desc;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_ENDP_Get_Number
*
* DESCRIPTION
*   This function returns the endpoint number in the range of 1 to 15, in
* number_out.
*
* INPUTS
*   cb          Pointer to endpoint control block.
*   number_out  pointer to the variable to hold the endpoint number.
*
* OUTPUTS
*    NU_SUCCESS         Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_ENDP_Get_Number (NU_USB_ENDP * cb,
                               UINT8 *number_out)
{

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(number_out);
    NU_USB_PTRCHK(cb->desc);

    *number_out = (cb->desc->bEndpointAddress & 0x0F);

    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_ENDP_Get_Direction
*
* DESCRIPTION
*    This function returns the direction of the endpoint, in direction_out.
*
* INPUTS
*   cb             Pointer to endpoint control block.
*   direction_out  Pointer to the variable to hold the direction which can
*                  be either USB_DIR_IN or USB_DIR_OUT.
*
* OUTPUTS
*    NU_SUCCESS         Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_ENDP_Get_Direction (NU_USB_ENDP * cb,
                                  UINT8 *direction_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(direction_out);
    NU_USB_PTRCHK(cb->desc);

    *direction_out = (cb->desc->bEndpointAddress & 0x80);
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_ENDP_Get_Transfer_Type
*
* DESCRIPTION
*    This function returns the type of the transfer in transfer_type_out.
*
* INPUTS
*   cb                 Pointer to endpoint control block.
*   transfer_type_out  Pointer to the variable to hold transfer type, which
*                      is one of the - USB_EP_CTRL, USB_EP_ISO, USB_EP_BULK ,
*                      USB_EP_INTR.
* OUTPUTS
*    NU_SUCCESS         Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_ENDP_Get_Transfer_Type (NU_USB_ENDP * cb,
                                      UINT8 *transfer_type_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(transfer_type_out);
    NU_USB_PTRCHK(cb->desc);

    *transfer_type_out = (cb->desc->bmAttributes & 0x03);
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_ENDP_Get_Sync_Type
*
* DESCRIPTION
*    This function returns the sync type of the endpoint, in sync_type_out.
*
* INPUTS
*   cb                 Pointer to endpoint control block.
*   sync_type_out      Pointer to the variable to hold the sync type.
*
* OUTPUTS
*    NU_SUCCESS         Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_ENDP_Get_Sync_Type (NU_USB_ENDP * cb,
                                  UINT8 *sync_type_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(sync_type_out);
    NU_USB_PTRCHK(cb->desc);

    *sync_type_out = (cb->desc->bmAttributes & 0x0C);
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_ENDP_Get_Usage_Type
*
* DESCRIPTION
*    This function returns the usage type of the endpoint, in usage_type_out.
*
* INPUTS
*   cb                 Pointer to endpoint control block.
*   usage_type_out    Pointer to the variable to hold the usage type.
*
* OUTPUTS
*    NU_SUCCESS         Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_ENDP_Get_Usage_Type (NU_USB_ENDP * cb,
                                   UINT8 *usage_type_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(usage_type_out);
    NU_USB_PTRCHK(cb->desc);

    *usage_type_out = (cb->desc->bmAttributes & 0x30);
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_ENDP_Get_Max_Packet_Size
*
* DESCRIPTION
*    This function returns the max packet size of the endpoint, in
* max_packet_size_out.
*
* INPUTS
*    cb                  Pointer to the endpoint  control block.
*    max_packet_size_out Pointer to the variable to hold the max packet
*                        size of the endpoint.
*
* OUTPUTS
*    NU_SUCCESS         Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_ENDP_Get_Max_Packet_Size (NU_USB_ENDP * cb,
                                        UINT16 *max_packet_size_out)
{
    UINT16 maxPacket;
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(max_packet_size_out);
    NU_USB_PTRCHK(cb->desc);

    maxPacket = (cb->desc->wMaxPacketSize1 << 8) | cb->desc->wMaxPacketSize0;
    *max_packet_size_out = (maxPacket & 0x7FF);
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_ENDP_Get_Num_Transactions
*
* DESCRIPTION
*    The functions returns the number of transactions permitted per micro
* frame for this endpoint.
*
* INPUTS
*    cb                    Pointer to the endpoint control block.
*    num_transactions_out  Pointer to the variable to hold the value of the
*                          number of transactions.
*
* OUTPUTS
*    NU_SUCCESS         Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_ENDP_Get_Num_Transactions (NU_USB_ENDP * cb,
                                         UINT8 *num_transactions_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(num_transactions_out);
    NU_USB_PTRCHK(cb->desc);

    *num_transactions_out = (cb->desc->wMaxPacketSize1 & 0x18);
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_ENDP_Get_Class_Desc
*
* DESCRIPTION
*    This function returns pointer to the class specific endpoint
* descriptor, if any associated with the end point.
*
* INPUTS
*    cb              Pointer to the endpoint control block.
*    class_desc_out  Pointer to the memory location to hold the pointer to
*                    the class specific endpoint descriptor.
*    length_out      Pointer to the variable to hold the length of the
*                    class specific descriptor in bytes.
*
* OUTPUTS
*    NU_SUCCESS         Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_ENDP_Get_Class_Desc (NU_USB_ENDP * cb,
                                   UINT8 **class_desc_out,
                                   UINT32 *length_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(class_desc_out);
    NU_USB_PTRCHK(length_out);

    *class_desc_out = cb->class_specific;
    *length_out = cb->length;
    return (NU_SUCCESS);

}

/*************************************************************************
* FUNCTION
*               NU_USB_ENDP_Get_Interval
*
* DESCRIPTION
*    This function returns the interval of the endpoint, in interval_out.
*
* INPUTS
*    cb            pointer to the endpoint control block.
*    interval_out  pointer to the variable to hold the value of the
*                  endpoint interval.
*
* OUTPUTS
*    NU_SUCCESS         Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_ENDP_Get_Interval (NU_USB_ENDP * cb,
                                 UINT8 *interval_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(interval_out);
    NU_USB_PTRCHK(cb->desc);

    *interval_out = cb->desc->bInterval;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_ENDP_Get_Status
*
* DESCRIPTION
*   This function returns the status of the endpoint in status_out.
*
* INPUTS
*   cb           Pointer to the endpoint control block.
*   status_out   Pointer to the variable to hold the value of the endpoint
*                status.
*
* OUTPUTS
*    NU_SUCCESS         Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_ENDP_Get_Status (NU_USB_ENDP * cb,
                               UINT16 *status_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(status_out);
    NU_USB_PTRCHK(cb->device);

    /* Host and Function s/w have their own ways of fetching the status */
    return NU_USB_STACK_Get_Endpoint_Status (cb->device->stack, &cb->pipe,
                                             status_out);

}

/*************************************************************************
* FUNCTION
*               NU_USB_ENDP_Get_Pipe
*
* DESCRIPTION
*    This function returns a pointer to the control block of the associated
* pipe in pipe_out..
*
* INPUTS
*    cb        Pointer to the endpoint control block.
*    pipe_out  Pointer to memory location to hold the pointer to the
*              associated pipe control block.
*
* OUTPUTS
*    NU_SUCCESS         Indicates successful completion of the service.
*
*************************************************************************/
/* Services to access associated objects */
STATUS NU_USB_ENDP_Get_Pipe (NU_USB_ENDP * cb,
                             NU_USB_PIPE ** pipe_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(pipe_out);

    *pipe_out = &cb->pipe;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_ENDP_Get_Alt_Settg
*
* DESCRIPTION
*    This function returns a pointer to the control block of the associated
* alternate setting.
*
* INPUTS
*   cb               pointer to the endpoint control block.
*   alt_setting_out  pointer to the memory location to hold the pointer to
*                    the control block of the associated alternate setting.
*
* OUTPUTS
*    NU_SUCCESS         Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_ENDP_Get_Alt_Settg (NU_USB_ENDP * cb,
                                  NU_USB_ALT_SETTG ** alt_setting_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(alt_setting_out);

    *alt_setting_out = cb->alt_setting;
    return (NU_SUCCESS);
}

/*************************************************************************
* FUNCTION
*               NU_USB_ENDP_Get_Device
*
* DESCRIPTION
*    This function returns a pointer to the control block of the associated
* device.
*
* INPUTS
*   cb          pointer to the endpoint control block.
*   device_out  pointer to the memory location to hold the pointer to
*                    the control block of the associated device.
*
* OUTPUTS
*    NU_SUCCESS         Indicates successful completion of the service.
*
*************************************************************************/
STATUS NU_USB_ENDP_Get_Device (NU_USB_ENDP * cb,
                               NU_USB_DEVICE ** device_out)
{
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(device_out);

    *device_out = cb->device;
    return (NU_SUCCESS);
}

/* Following functions should only be visible when stack is configured
 * for Super Speed USB (USB 3.0). */

#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

/*************************************************************************
*
*   FUNCTION
*
*       NU_USB_ENDP_Get_Companion_Desc
*
*   DESCRIPTION
*
*       User can call this API to get SuperSpeed endpoint companion
*       descriptor associated with an endpoint. This API returns the whole
*       descriptor. To get individual fields of SuperSpeed endpoint
*       companion descriptor, the user should call respective API's.
*
*   INPUT
*
*       cb              - Pointer to NU_USB_ENDP control block
*
*       epcompanion_desc_out
*                       - Pointer to NU_USB_SSENDPCOMPANION_DESC. This
*                         points to a valid SuperSpeed endpoint companion
*                         descriptor or NU_NULL if descriptor is not
*                         present.
*
*   OUTPUT
*
*       NU_SUCCESS      - Indicates Operation Completed Successfully,
*                         ‘epcompanion_desc_out’ points to a valid
*                         SuperSpeed endpoint companion descriptor.
*
*       NU_USB_INVLD_ARG
*                       - Any of the input argument is invalid.
*
*       NU_NOT_PRESENT  - There is no SuperSpeed endpoint descriptor
*                         associated with this endpoint.
*
*************************************************************************/
STATUS NU_USB_ENDP_Get_Companion_Desc (
                    NU_USB_ENDP                   *cb,
                    NU_USB_SSEPCOMPANION_DESC    **epcompanion_desc_out)
{
    STATUS status;

    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(epcompanion_desc_out);

    /* Initialize output argument to default value. */
    *epcompanion_desc_out = NU_NULL;
	
    if (cb->epcompanion_desc != NU_NULL)
    {
        *epcompanion_desc_out = cb->epcompanion_desc;
        status = NU_SUCCESS;
    }
    else
    {
        status = NU_NOT_PRESENT;
    }

    return ( status );
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_USB_ENDP_Get_MaxBurst
*
*   DESCRIPTION
*
*       This API returns the bMaxBurst field of SuperSpeed endpoint
*       companion descriptor.
*
*   INPUT
*
*       cb              - Pointer to NU_USB_ENDP control block.
*
*       maxburst_out    - Pointer to UINT8. This contains the value of
*                         bMaxBurst field of SuperSpeed endpoint companion
*                         descriptor if function completes without an
*                         error.
*
*   OUTPUT
*
*       NU_SUCCESS      - Operation Completed Successfully, ‘maxburst_out’
*                         contains value of bMaxBurst field of SuperSpeed
*                         endpoint companion descriptor.
*
*       NU_USB_INVLD_ARG
*                       - Any of the input argument is invalid.
*
*       NU_NOT_PRESENT  - There is no SuperSpeed endpoint descriptor
*                         associated with this endpoint.
*
*************************************************************************/
STATUS NU_USB_ENDP_Get_MaxBurst (NU_USB_ENDP    *cb,
                                 UINT8          *maxburst_out)
{
    STATUS status;
    
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(maxburst_out);

    if (cb->epcompanion_desc != NU_NULL)
    {
        *maxburst_out = cb->epcompanion_desc->bMaxBurst;
        status = NU_SUCCESS;
    }
    else
    {
        status = NU_NOT_PRESENT;
    }

    return ( status );
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_USB_ENDP_Get_Bulk_MaxStreams
*
*   DESCRIPTION
*
*       User calls this API to get Max Streams of SuperSpeed BULK
*       endpoint. In case of a SuperSpeed BULK endpoint maximum number of
*       streams is encoded in Bit 4:0 of bmAttributes field of SuperSpeed
*       endpoint companion descriptor.
*
*   INPUT
*
*       cb              - Pointer to NU_USB_ENDP control block.
*
*       maxstreams_out  - Pointer to UINT16. This contains the value of Max
*                         streams (Bit 4-0 of bmAttributes field) supported
*                         by SuperSpeed BULK endpoint.
*
*   OUTPUT
*
*       NU_SUCCESS      - Operation Completed Successfully,
*                         ‘maxstreams_out’ contains maximum number of
*                         streams supported by SuperSpeed BULK endpoint.
*
*       NU_USB_NOT_SUPPORTED
*                       - Endpoint is not a BULK endpoint.
*
*       NU_USB_INVLD_ARG
*                       - Any of the input argument is invalid.
*
*       NU_NOT_PRESENT  - There is no SuperSpeed endpoint descriptor
*                         associated with this endpoint.
*
*************************************************************************/
STATUS NU_USB_ENDP_Get_Bulk_MaxStreams (NU_USB_ENDP     *cb,
                                        UINT32          *maxstreams_out)
{
    STATUS status;
	UINT8 transfer_type;
	UINT8 mult;
	
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(maxstreams_out);

    if ( cb->epcompanion_desc != NU_NULL )
    {
		status = NU_USB_ENDP_Get_Transfer_Type (cb, &transfer_type);
        if ((status == NU_SUCCESS) && (transfer_type == USB_EP_BULK))
        {
            mult = (cb->epcompanion_desc->bmAttributes &
                                            SSEPDESC_MAXSTREAM_MASK); 
			*maxstreams_out = 1;
            *maxstreams_out <<= mult;
            status = NU_SUCCESS;
        }
        else
        {
            status = NU_USB_NOT_SUPPORTED;
        }
    }
    else
    {
        status = NU_NOT_PRESENT;
    }

    return ( status );
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_USB_ENDP_Get_Iso_MaxPktPerIntrvl
*
*   DESCRIPTION
*
*       User calls this API to get the maximum number of packets within a
*       service interval that this SuperSpeed ISOCHRONOUS endpoint
*       supports. In case of a SuperSpeed ISOCHRONOUS endpoint this value
*       is encoded in Mult (Bit 1:0  of bmAttributes field of SuperSpeed
*       endpoint companion descriptor).
*       Maximum number of packets = bMaxBurst x (Mult + 1).
*
*   INPUT
*
*       cb              - Pointer to NU_USB_ENDP control block.
*
*       max_packets_out - Pointer to UINT8. This contains the max packets
*                         per service interval supported by SuperSpeed
*                         ISOCHRONOUS endpoint.
*
*   OUTPUT
*
*       NU_SUCCESS      - Operation Completed Successfully,
*                         ‘max_packets_out’ contains Mult value supported
*                         by SuperSpeed ISOCHRONOUS endpoint.
*
*       NU_USB_NOT_SUPPORTED
*                       - Endpoint is not a ISOCHRONOUS endpoint.
*
*       NU_USB_INVLD_ARG
*                       - Any of the input argument is invalid.
*
*       NU_NOT_PRESENT  - There is no SuperSpeed endpoint descriptor
*                         associated with this endpoint.
*
*************************************************************************/
STATUS NU_USB_ENDP_Get_Iso_MaxPktPerIntrvl (NU_USB_ENDP *cb,
                                            UINT8       *max_packets_out)
{
    STATUS status;
	UINT8 transfer_type, mult, max_burst;
	
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(max_packets_out);

    if ( cb->epcompanion_desc != NU_NULL )
    {
        status = NU_USB_ENDP_Get_Transfer_Type (cb, &transfer_type);
        if ((status == NU_SUCCESS) && (transfer_type == USB_EP_ISO))
        {
            mult = (cb->epcompanion_desc->bmAttributes &
                                                  SSEPDESC_MULT_MASK);
            status = NU_USB_ENDP_Get_MaxBurst(cb, &max_burst);

            if (status == NU_SUCCESS)
            {
                *max_packets_out = max_burst*(mult+1);
                status = NU_SUCCESS;
            }
        }
        else
        {
            status = NU_USB_NOT_SUPPORTED;
        }
    }
    else
    {
        status = NU_NOT_PRESENT;
    }

    return ( status );
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_USB_ENDP_Get_BytesPerInterval
*
*   DESCRIPTION
*
*       User calls this API to get wBytesPerInterval field of a SuperSpeed
*       periodic (Interrupt and Isochronous) endpoints. This filed actually
*       determines that how many bytes this endpoint can transfer in a
*       single service interval.
*
*   INPUT
*
*       cb              - Pointer to NU_USB_ENDP control block.
*
*       bytesperinterval_out
*                       - Pointer to UINT16. This contains the value of
*                         wBytesPerInterval by a periodic SuperSpeed
*                         endpoint.
*
*   OUTPUT
*
*       NU_SUCCESS      - Operation Completed Successfully,
*                         ‘bytesperinterval_out’ contains value of
*                         wBytesPerInterval supported by SuperSpeed
*                         periodic endpoint.
*
*       NU_USB_NOT_SUPPORTED
*                       - Endpoint is not a periodic (Interrupt or
*                         Isochronous).
*
*       NU_USB_INVLD_ARG
*                       - Any of the input argument is invalid.
*
*       NU_NOT_PRESENT  - There is no SuperSpeed endpoint descriptor
*                         associated with this endpoint.
*
*************************************************************************/
STATUS NU_USB_ENDP_Get_BytesPerInterval (NU_USB_ENDP *cb,
                                            UINT16   *bytesperinterval_out)
{
    STATUS status;
	UINT8 transfer_type;
	
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(bytesperinterval_out);

    if ( cb->epcompanion_desc != NU_NULL )
    {
		status = NU_USB_ENDP_Get_Transfer_Type (cb, &transfer_type);
        if ((status == NU_SUCCESS) &&
           ((transfer_type==USB_EP_ISO)||(transfer_type==USB_EP_INTR)))
        {
            *bytesperinterval_out =
                            cb->epcompanion_desc->wBytesPerInterval;
            status = NU_SUCCESS;
        }
        else
        {
            status = NU_USB_NOT_SUPPORTED;
        }
    }
    else
    {
        status = NU_NOT_PRESENT;
    }

    return ( status );
}

/*************************************************************************
*
*   FUNCTION
*
*       NU_USB_ENDP_Get_SSEPC_bmAttributes
*
*   DESCRIPTION
*
*       User calls this API to get bmAttributes field of a SuperSpeed
*       endpoint companion descriptor for the particular endpoint. 
*
*   INPUT
*
*       cb              - Pointer to NU_USB_ENDP control block.
*
*       bmAttributes_out
*                       - Pointer to UINT8. This contains the value of
*                         bmAttributes of a SuperSpeed endpoint.
*
*   OUTPUT
*
*       NU_SUCCESS      - Operation Completed Successfully,
*                         ‘bmAttributes_out’ contains value of
*                         bmAttributes of a SuperSpeed endpoint.
*
*       NU_USB_INVLD_ARG
*                       - Any of the input argument is invalid.
*
*       NU_NOT_PRESENT  - There is no SuperSpeed endpoint descriptor
*                         associated with this endpoint.
*
*************************************************************************/
STATUS NU_USB_ENDP_Get_SSEPC_bmAttributes (NU_USB_ENDP *cb,
                                         UINT8   *bmAttributes_out)
{
    STATUS status;
    
    NU_USB_PTRCHK(cb);
    NU_USB_PTRCHK(bmAttributes_out);

    if ( cb->epcompanion_desc != NU_NULL )
    {
        *bmAttributes_out = cb->epcompanion_desc->bmAttributes;
        status = NU_SUCCESS;
    }
    else
    {
        status = NU_NOT_PRESENT;
    }

    return ( status );
}

#endif /* (CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE ) */

#endif /* USB_ENDP_EXT_C */
/*************************** end of file ********************************/

