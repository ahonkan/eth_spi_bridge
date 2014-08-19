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
*     nu_usb_stack_imp.c
*
* COMPONENT
*
*     Nucleus USB Software 
*
* DESCRIPTION
*       This file provides the implementation for the internal functions of
*       common base stack class.
*
* 
* DATA STRUCTURES
*
*
*	FUNCTIONS
*
*		USB_Parse_Descriptors
*						- Parses descriptors from raw bytes to
*                         NU_USB_CFG structure.
*
*		usb_verify_match_flag
*						- Verify the validity of match flag.
*
*   	USB_Parse_BOS_Descriptor
*						- used for parsing BOS descriptor
*						  and initializing ‘bos’ field of
*						  NU_USB_DEVICE control block.
*						  This API must be called once in
*						  both host and function mode
*						  before enumeration is complete.
*
*	DEPENDENCIES       
*
*		nu_usb.h		All USB definitions
*
************************************************************************/
#ifndef USB_STACK_IMP_C
#define	USB_STACK_IMP_C

/* ======================  Include Files  ============================= */
#include "connectivity/nu_usb.h"

/*************************************************************************
 * FUNCTION 
 *        USB_Parse_Descriptors
 *
 * DESCRIPTION
 *     Parses the descriptors in the raw byte form in to NU_USB_CFG structure
 * that holds them in terms of Config Descriptors, Interface Descriptors, 
 * Endpoint descriptors.
 * 
 * INPUTS 
 *    UINT8 *raw_cfg     Ptr to byte stream that holds the descriptors fetched
 *                         from the device.
 *    NU_USB_CFG *cfg    Ptr to the structure that is to store the parsed 
 *                         descriptors.
 *    UINT16 size        size of the byte stream pointed to by raw_cfg.
 *
 * OUTPUTS
 *      NU_SUCCESS          Indicates that the descriptors are well formed and
 *                          are parsed in to NU_USB_CFG structure.
 *      NU_INVALID_POINTER  Indicates that the first or second argument 
 *                          passed is NU_NULL.
 *      NU_USB_INVLD_DESC   Indicates that descriptors contained in
 *                          raw_cfg are ill formed.
 *      NU_USB_MAX_EXCEEDED Indicates that the number of IADs or interfaces
 *                          is more than related defined max value.
 *
 *************************************************************************/
STATUS USB_Parse_Descriptors (NU_USB_DEVICE * device,
                              UINT8 *raw_cfg,
                              NU_USB_CFG * cfg,
                              UINT16 size)
{
    INT i, j, k;
    UINT8 *buffer;
    NU_USB_HDR *hdr;
    INT context = 0;        /* 1 = std interface descriptor
                             * 2 = class specific interface descriptor
                             * 3 = std end point descriptor
                             * 4 = class specific end point descriptor */
    INT num_interfaces = 0;
    INT num_eps = 0;
    NU_USB_CFG_DESC *cd;
    NU_USB_IAD_DESC *iad_desc;
    NU_USB_ALT_SETTG *alt_set = NU_NULL;
    NU_USB_INTF_DESC *intf_descrptr = NU_NULL;
    NU_USB_ENDP_DESC *ep_descrpt = NU_NULL;
    NU_USB_OTG_DESC  *otg_desc;
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
    NU_USB_SSEPCOMPANION_DESC *ssepcompanion_desc = NU_NULL;
#endif

    if ((raw_cfg == NU_NULL) 
	    || (cfg == NU_NULL)
		|| (device == NU_NULL))
	{
        return NU_USB_INVLD_ARG;
	}

    buffer = raw_cfg;

    /* Do the contents of the buffer make a meaningful config descriptor ? */
    cd = (NU_USB_CFG_DESC *) buffer;
    if (cd->bLength != USB_CONFIG_DESCRPTR_SIZE)
    {
        return (NU_USB_INVLD_DESC);
    }

    if (cd->bNumInterfaces > NU_USB_MAX_INTERFACES)
    {
        return (NU_USB_INVLD_DESC);
    }

    /* Bus powered device cannot ask for more than 500mA */
    if ((cd->bMaxPower > 250) && ((cd->bmAttributes & 0x40) == 0))
    {
        return (NU_USB_INVLD_DESC);
    }

    /* Valid Config descriptor, place it in the cfg structure */
    cfg->desc = cd;
    cfg->load = 0;
    cfg->is_active = 0;
    cfg->device = device;
    cfg->otg_desc = NU_NULL;
    cfg->class_specific = NU_NULL;
    cfg->numIADs = 0;
    cfg->numIntfs = 0;

    for (i = 0; i < NU_USB_MAX_IADS; i++)
    {
        cfg->iad[i].first_intf = 0xFF;
        cfg->iad[i].last_intf = 0;
        cfg->iad[i].iad_desc = NU_NULL;
        cfg->iad[i].cfg = cfg;
        cfg->iad[i].device = device;
    }

    for (i = 0; i < NU_USB_MAX_INTERFACES; i++)
    {
        cfg->intf[i].current = NU_NULL;
        cfg->intf[i].driver = NU_NULL;
        cfg->intf[i].iad = NU_NULL;
        cfg->intf[i].load = 0;
        /* Cast to UINT8 to remove Lint warning. */
        cfg->intf[i].intf_num = (UINT8)i;
        cfg->intf[i].cfg = cfg;
        cfg->intf[i].device = device;
        for (j = 0; j < NU_USB_MAX_ALT_SETTINGS; j++)
        {
            cfg->intf[i].alt_settg[j].desc = NU_NULL;
            cfg->intf[i].alt_settg[j].length = 0;
            cfg->intf[i].alt_settg[j].class_specific = NU_NULL;
            cfg->intf[i].alt_settg[j].intf = &cfg->intf[i];
            cfg->intf[i].alt_settg[j].is_active = 0;
            cfg->intf[i].alt_settg[j].load = 0;
            for (k = 0; k < NU_USB_MAX_ENDPOINTS; k++)
            {
                cfg->intf[i].alt_settg[j].endp[k].desc = NU_NULL;
                cfg->intf[i].alt_settg[j].endp[k].class_specific = NU_NULL;
                cfg->intf[i].alt_settg[j].endp[k].length = 0;
                cfg->intf[i].alt_settg[j].endp[k].device = device;
                cfg->intf[i].alt_settg[j].endp[k].load = 0;
                cfg->intf[i].alt_settg[j].endp[k].alt_setting =
                    &cfg->intf[i].alt_settg[j];
            }
        }
    }

    buffer += USB_CONFIG_DESCRPTR_SIZE;
    size -= USB_CONFIG_DESCRPTR_SIZE;

    while (size > 2)
    {
        hdr = (NU_USB_HDR *) buffer;
        if ((hdr->bLength > size) || (hdr->bLength == 0))
            return (NU_USB_INVLD_DESC);

        /* If an IAD exists, its bFirstInterface field indicates the
         * first interface number of the association. The bInterfaceCount
         * field indicates the number of contiguous interfaces.
         */
        /* Every Alt setting starts with an exactly one USB_DT_INTERFACE, 
         * followed by zero or more Class specific interface descriptors
         * followed by zero or more USB_DT_ENDPOINT descriptors.
         * Each USB_DT_ENDPOINT is immediately followed by
         * zero or more class specific EP descriptors
         */
        switch (hdr->bDescriptorType)
        {
            case USB_DT_OTG :

                if(cfg->otg_desc != NU_NULL)
                    return (NU_USB_INVLD_DESC);

                otg_desc = (NU_USB_OTG_DESC *)buffer;

                /* error checking for length */

                if(otg_desc->bLength != 3)
                    return (NU_USB_INVLD_DESC);

                /* if HNP is supported, SRP must also be supported */

                if((otg_desc->bmAttributes & 0x02) && 
                        (!(otg_desc->bmAttributes & 0x01)))
                    return (NU_USB_INVLD_DESC);

                /* if device already has OTG support, via another
                 * configuration, both must state the same truth about its
                 * OTG support. */

                if (device->otg_status != 0)
                {
                    if((device->otg_status & 0x03) !=
                        (otg_desc->bmAttributes & 0x03))
                        return (NU_USB_INVLD_DESC);
                }

                cfg->otg_desc = otg_desc;
                device->otg_status |= (otg_desc->bmAttributes & 0x03);

                size -= hdr->bLength;
                buffer += hdr->bLength;
                break;

            case USB_DT_INTERFACE_ASSOC:
                /* At least one Interface Association Descriptor exists */
                /* Check for Multi-Interface Function Device Class code */
                if (device->device_descriptor.bDeviceClass != MULTI_INTERFACE_FUNC_CLASS ||
                    device->device_descriptor.bDeviceSubClass != MULTI_INTERFACE_FUNC_SUBCLASS ||
                    device->device_descriptor.bDeviceProtocol != MULTI_INTERFACE_FUNC_PROTOCOL)
                {
                    return (NU_USB_INVLD_DESC);
                }

                /* Check if number of IADs is over maximum limit */
                if (cfg->numIADs >= NU_USB_MAX_IADS)
                {
                    return NU_USB_MAX_EXCEEDED;
                }

                /* Do the contents of the buffer make a meaningful
                 * IAD descriptor?
                 */
                iad_desc = (NU_USB_IAD_DESC *) buffer; 

                /* make sure number of interface value is valid */
                if (iad_desc->bInterfaceCount > NU_USB_MAX_INTERFACES)
                {
                    return (NU_USB_INVLD_DESC);
                }

                if (iad_desc->bFirstInterface + iad_desc->bInterfaceCount >
                    NU_USB_MAX_INTERFACES)
                {
                    return (NU_USB_INVLD_DESC);
                }

                cfg->iad[cfg->numIADs].first_intf = iad_desc->bFirstInterface;
                cfg->iad[cfg->numIADs].last_intf = iad_desc->bFirstInterface +
                    iad_desc->bInterfaceCount - 1;
                cfg->iad[cfg->numIADs].iad_desc = iad_desc;
                cfg->numIADs++;
                size -= hdr->bLength;
                buffer += hdr->bLength;
                break;

            case USB_DT_INTERFACE:

                if ((alt_set) && (num_eps != (INT)alt_set->desc->bNumEndpoints))
                    return (NU_USB_INVLD_DESC);
                intf_descrptr = (NU_USB_INTF_DESC *) buffer;

                /* Do the contents of the buffer make a meaningful Intf descriptor ? */
                if (intf_descrptr->bInterfaceNumber >= NU_USB_MAX_INTERFACES)
                    return (NU_USB_INVLD_DESC);
                if (intf_descrptr->bAlternateSetting >= NU_USB_MAX_ALT_SETTINGS)
                    return (NU_USB_INVLD_DESC);

                /* Valid Intf descriptor, place it in the cfg structure */
                alt_set = &(cfg->intf[intf_descrptr->bInterfaceNumber].
                            alt_settg[intf_descrptr->bAlternateSetting]);
                if (alt_set->desc != NU_NULL)
                    return (NU_USB_INVLD_DESC);
                alt_set->desc = intf_descrptr;
                size -= hdr->bLength;
                buffer += hdr->bLength;
                if (intf_descrptr->bAlternateSetting == 0)
                {
                    num_interfaces++;
                    cfg->numIntfs++;
                }

                if (num_interfaces > (INT)cd->bNumInterfaces)
                    return (NU_USB_INVLD_DESC);

                /* Sub Class can be specified only if class is specified */
                if ((intf_descrptr->bInterfaceSubClass != 0) &&
                    (intf_descrptr->bInterfaceSubClass != 0xff) &&
                    (intf_descrptr->bInterfaceClass == 0))
                    return (NU_USB_INVLD_DESC);

                /* Check to see if this interface is part of an IAD */
                for (j=0; j<cfg->numIADs; j++)
                {
                    if (intf_descrptr->bInterfaceNumber >=
                            cfg->iad[j].first_intf &&
                        intf_descrptr->bInterfaceNumber <=
                            cfg->iad[j].last_intf)
                    {
                        cfg->intf[intf_descrptr->bInterfaceNumber].iad =
                            &cfg->iad[j];
                        break;
                    }
                }

                /* Std Interface descriptors */
                context = 1;
                num_eps = 0;
                break;

            case USB_DT_ENDPOINT:
                if (alt_set == NU_NULL)
                    return (NU_USB_INVLD_DESC);

                /* EP descriptors  must be preceded by std or class specific 
                 * interface descriptors
                 */
                context = 3;

                /* Do the contents of the buffer make a meaningful EP descriptor ? */
                ep_descrpt = (NU_USB_ENDP_DESC *) buffer;

                /* EP 0 should never have a EP descriptor */
                if ((ep_descrpt->bEndpointAddress & 0xf) == 0)
                    return (NU_USB_INVLD_DESC);

                /* Iso endpoint shouldn't be in alt setting 0 */
                if ((intf_descrptr->bAlternateSetting == 0) &&
                    ((ep_descrpt->bmAttributes & 0x3) == 1))
                    return (NU_USB_INVLD_DESC);

                if (num_eps >= NU_USB_MAX_ENDPOINTS)
                    return (NU_USB_INVLD_DESC);

                /* Valid EP descriptor, place it in the cfg structure */
                if (alt_set->endp[num_eps].desc != NU_NULL)
                    return (NU_USB_INVLD_DESC);
                alt_set->endp[num_eps].desc = ep_descrpt;

                alt_set->endp[num_eps].pipe.endpoint =
                    &(alt_set->endp[num_eps]);
                alt_set->endp[num_eps].pipe.device = device;

                size -= hdr->bLength;
                buffer += hdr->bLength;
                num_eps++;
                break;
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )
            case USB_DT_SSEPCOMPANION:
                if ((ep_descrpt != NU_NULL) && (alt_set != NU_NULL)
                    && (num_eps > 0))
                {
                    ssepcompanion_desc =
                                 (NU_USB_SSEPCOMPANION_DESC*)buffer;
                    alt_set->endp[num_eps - 1].epcompanion_desc =
                                                    ssepcompanion_desc;
                    size -= ssepcompanion_desc->bLength;
                    buffer += ssepcompanion_desc->bLength;
                }
                break;
#endif
            default:
                if (alt_set == NU_NULL)
                {
                    if(!cfg->class_specific)
                        cfg->class_specific =  buffer;

                    size -= hdr->bLength;
                    buffer += hdr->bLength;
                    cfg->length = (UINT32)(buffer - cfg->class_specific);
                    break;
                }
                if ((context == 1) || (context == 2))
                {
                    /* Class specific Interface Descriptors */
                    context = 2;
                    if (!alt_set->class_specific)
                        alt_set->class_specific = buffer;
                    size -= hdr->bLength;
                    buffer += hdr->bLength;
                    alt_set->length =
                        (UINT32) (buffer - alt_set->class_specific);
                    break;
                }
                if (((context == 3) || (context == 4))&&(num_eps > 0x00))
                {
                    /* Class specific End Point Descriptors */
                    context = 4;
                    if (!(alt_set->endp[num_eps - 1].class_specific))
                        alt_set->endp[num_eps - 1].class_specific = buffer;
                    size -= hdr->bLength;
                    buffer += hdr->bLength;
                    alt_set->endp[num_eps - 1].length =
                        (UINT32) (buffer -
                                  alt_set->endp[num_eps - 1].class_specific);
                    break;
                }
                /* fall through */
            case USB_DT_DEVICE:
            case USB_DT_CONFIG:
            case USB_DT_STRING:
                return (NU_USB_INVLD_DESC);
        }
    }
    if ((size != 0) || (num_interfaces != (INT)cd->bNumInterfaces))
        return (NU_USB_INVLD_DESC);
    if ((alt_set) && (num_eps != (INT)alt_set->desc->bNumEndpoints))
        return (NU_USB_INVLD_DESC);
    return (NU_SUCCESS);
}

/*************************************************************************
 *
 * FUNCTION
 *      usb_verify_match_flag
 *
 * DESCRIPTION
 *    Checks to see if the match flag is well formed.
 *
 * INPUTS
 *    UINT8 match_flag  match flag that is to be verified.
 *
 * OUTPUTS
 *   UINT8 0 if ill formed and 1 otherwise.
 * 
 *************************************************************************/
UINT8 usb_verify_match_flag (UINT8 match_flag)
{
    if (match_flag & USB_MATCH_PRDCT_ID)
        if (!(match_flag & USB_MATCH_VNDR_ID))
            return 0;
    if (match_flag & USB_MATCH_REL_NUM)
        if (!(match_flag & USB_MATCH_PRDCT_ID))
            return 0;
    if (match_flag & USB_MATCH_SUB_CLASS)
        if (!(match_flag & USB_MATCH_CLASS))
            return 0;
    if (match_flag & USB_MATCH_PROTOCOL)
        if (!(match_flag & USB_MATCH_SUB_CLASS))
            return 0;
    return 1;
}

/* Following functions should only be visible when stack is configured
 * for USB 3.0. */
#if ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE )

/*************************************************************************
*
* 	FUNCTION
*
*       USB_Parse_BOS_Descriptor
*
* 	DESCRIPTION
*
*		This function is used for parsing BOS descriptor and initializing
*       'bos' field of NU_USB_DEVICE control block. This API must be called
*       once in both host and function mode before enumeration is complete.
*
* 	INPUT
*
*       device          - Pointer to NU_USB_DEVICE control block.
*
*       raw_bos_desc	- Array of UINT8 containing raw BOS descriptor and
*						  device capability descriptors.
*
*       bos				- Pointer to NU_USB_BOS. Fields of this control
*						  block will be initialized when function returns.
*
*       size			- Total size of BOS descriptor and device
*						  capability descriptors.
*
* 	OUTPUT
*
*  		NU_SUCCESS		- Indicates Operation Completed Successfully.
*
*  		NU_USB_INVLD_ARG
*						- Indicates any of the input argument is invalid.
*
*************************************************************************/
STATUS USB_Parse_BOS_Descriptor(NU_USB_DEVICE   *device,
                                UINT8           *raw_bos_desc,
                                NU_USB_BOS      *bos,
                                UINT16          size)
{
    UINT8   *buffer;
    STATUS  status = NU_USB_INVLD_ARG;
    UINT8   cap_descriptors_parsed = 0;
    BOOLEAN is_cap_valid = NU_TRUE;
    
    NU_USB_BOS_DESC * bos_desc;
    NU_USB_DEVCAP_HDR *hdr;

    /* Check for valid control block pointers. */
    if ((device         != NU_NULL) &&
        (raw_bos_desc   != NU_NULL) &&
        (bos            != NU_NULL))
    {
        status      = NU_USB_INVLD_DESC;
        buffer      = raw_bos_desc;
        bos_desc    = (NU_USB_BOS_DESC*)buffer;

        /* Check for a valid BOS descriptor. */
        if ((bos_desc->bLength          == BOS_DESC_LENGTH) &&
            (bos_desc->bNumDeviceCaps   >= MIN_DEV_CAP)     &&
            (bos_desc->bNumDeviceCaps   <= MAX_DEV_CAP)     &&
            (bos_desc->bDescriptor      == USB_DT_BOS))
        {
            bos->bos_desc = bos_desc;
            size -= bos_desc->bLength;
            buffer = buffer + bos_desc->bLength;
    
            /* Parse all device capability descriptors. */
            while ((size > 2) &&
                     (cap_descriptors_parsed < bos_desc->bNumDeviceCaps))
            {
                hdr = (NU_USB_DEVCAP_HDR *)buffer;
                if ((hdr->bLength <= size) && (hdr->bLength != 0 ) &&
                    (hdr->bDescriptorType == USB_DT_DEVCAP) )
                {
                    switch(hdr->bDevCapabilityType)
                    {
                        case USB_DCT_USB2EXT:
                            bos->devcap_usb2ext_desc =
                               (NU_USB_DEVCAP_USB2EXT_DESC *) buffer;
                            break;
                        case USB_DCT_USBSS:
                            bos->devcap_ss_desc =
                               (NU_USB_DEVCAP_SUPERSPEED_DESC *) buffer;
                            break;
                        case USB_DCT_CONTID:
                            bos->devcap_cid_desc =
                               (NU_USB_DEVCAP_CONTAINERID_DESC *) buffer;
                            break;
                        default:
                            is_cap_valid = NU_FALSE;    
                            break;
                    }
                }
                else
                {
                    break;
                }

                /* Move ahead in buffer to next packet. */
                buffer = buffer + hdr->bLength;
                
                /* Decrement the buffer size by the length of the packet. */
                size = size - hdr->bLength;
                
                /* Increment parsed descriptor counter. */
                cap_descriptors_parsed++;
            }

            if ((size == 0) &&
                (cap_descriptors_parsed == bos_desc->bNumDeviceCaps) &&
                (is_cap_valid == NU_TRUE))
            {
                status = NU_SUCCESS;
            }
        }
    }
    
    return status;
}

#endif /* ( CFG_NU_OS_CONN_USB_COM_STACK_SS_ENABLE == NU_TRUE ) */

#endif /* USB_STACK_IMP_C */
/*************************** end of file ********************************/

