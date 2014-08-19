/**************************************************************************
*
*               Copyright 2005 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
***************************************************************************

***************************************************************************
* FILE NAME
*
*       nu_usbf_eth_imp.c
*
* COMPONENT
*
*       Nucleus USB Function Software : Ethernet User Driver.
*
* DESCRIPTION
*
*       This file contains the internal implementation for Ethernet User.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       EF_Set_Multi_Filters_Rcvd           Set Multicast filters command
*                                           handling.
*       EF_Set_Power_Filters_Rcvd           Set Power filter command
*                                           handling.
*       EF_Set_Pkt_Filters_Rcvd             Set Packet filter command
*                                           handling.
*       EF_Find_Device                      Find pointer to the user device
*                                           corresponding to the handle passed
*                                           as an argument.
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions.
*
**************************************************************************/

/* ==============  USB Include Files =================================  */
#include "connectivity/nu_usb.h"

/* ==========================  Functions ============================== */

/*************************************************************************
*
* FUNCTION
*       EF_Set_Multi_Filters_Rcvd
*
* DESCRIPTION
*
*       This function handles EF_SET_ETHF_MULTICAST_FILTERS request from
*       Host.
*
* INPUTS
*
*       eth         Pointer to user driver control block.
*       pdev        Pointer to the ethernet device control block.
*       user_cmd    Pointer to command structure.
*
* OUTPUTS
*
*       NU_SUCCESS  Indicates successful completion.
*
*************************************************************************/

STATUS EF_Set_Multi_Filters_Rcvd(NU_USBF_ETH *eth,
								 NU_USBF_ETH_DEV *pdev,
                                 USBF_COMM_USER_CMD *user_cmd)
{
    INT i;
    ETHF_MULTICAST_FILTER *filter;

    /* Update driver data. */
    UINT8 num_filters = user_cmd->cmd_value;
    UINT8 *buffer = user_cmd->cmd_data;
    pdev->num_multicast_filters = num_filters;

    /* Initialize internal filter structure with received value. */
    for(i=0; i<num_filters; i++)
    {
        filter = &pdev->multicast_filter_array[i];
        filter->byte7 = *buffer++;
        filter->byte6 = *buffer++;
        filter->byte5 = *buffer++;
        filter->byte4 = *buffer++;
        filter->byte3 = *buffer++;
        filter->byte2 = *buffer++;
        filter->byte1 = *buffer++;
        filter->byte0 = *buffer++;
    }

    /* Pass this request to application. */
    if(eth->eth_ioctl)
    {
        eth->eth_ioctl((NU_USBF_USER *)eth,
                EF_SET_ETHF_MULTICAST_FILTERS,
                pdev->multicast_filter_array,
                num_filters);
    }
    return NU_SUCCESS;

}
/*************************************************************************
* FUNCTION
*       EF_Set_Power_Filters_Rcvd
*
* DESCRIPTION
*       This function handles EF_SET_ETH_PWR_MGT_PTRN_FILTER request from
*       Host.
*
* INPUTS
*       eth                    Pointer to user driver control block.
*       pdev                   Pointer to the ethernet device control block.
*       user_cmd               Pointer to command structure.
*
* OUTPUTS
*       NU_SUCCESS             Indicates successful completion.
*       NU_USB_MAX_EXCEEDED    Device does not have an empty slot
*                              for the power filter.
*
*************************************************************************/

STATUS EF_Set_Power_Filters_Rcvd(NU_USBF_ETH *eth,
								 NU_USBF_ETH_DEV *pdev,
                                 USBF_COMM_USER_CMD *user_cmd)
{
    INT i;
    STATUS status = NU_SUCCESS;
    ETHF_POWER_MNG_FILTER *filter = NU_NULL;
    UINT8 *buffer;

    /* Check if this filter is supported by user driver. */
    for(i=0; i<EF_MAX_POWER_PATTERN_FILTERS; i++)
    {
        filter = &pdev->power_pattern_filter_array[i];
        /* Find the empty slot. */
        if((filter->filter_status == EF_FALSE) &&
           (filter->filter_num == 0))
        {
            break;
        }
    }
    if(i==EF_MAX_POWER_PATTERN_FILTERS)
    {
        status = NU_USB_MAX_EXCEEDED;
    }
    else
    {
        /* Initialize internal filter structure with received value. */
        buffer = (UINT8 *)user_cmd->cmd_data;
        filter->filter_num = user_cmd->cmd_value;

        /* Copy the 2 bytes of mask_size. */
        filter->mask_size = LE16_2_HOST(*(UINT16 *)buffer);
		/* Make sure that mask size is within the defined limit. */
		if(filter->mask_size > EF_MAX_MASK_SIZE)
		{
			filter->mask_size = EF_MAX_MASK_SIZE;
		}
        buffer += sizeof(UINT16);

        /* Copy the mask byte of size equal to mask_size. */
        memcpy(filter->mask, buffer, filter->mask_size);
        buffer += filter->mask_size;

        /* Copy the filter pattern from remaining buffer. */
        memcpy(filter->pattern, buffer,
               (user_cmd->data_len - filter->mask_size - sizeof(UINT16)));

        filter->filter_status = EF_TRUE;

        /* Pass this command to application. */
        if(eth->eth_ioctl)
        {
            eth->eth_ioctl((NU_USBF_USER *)eth,
                    EF_SET_ETH_PWR_MGT_PTRN_FILTER,
                    filter,
                    0);
        }
    }

    return status;

}

/*************************************************************************
* FUNCTION
*       EF_Set_Pkt_Filters_Rcvd
*
* DESCRIPTION
*       This function handles EF_SET_ETHERNET_PACKET_FILTER request from
*       Host.
*
* INPUTS
*       eth         Pointer to user driver control block.
*       pdev        Pointer to the ethernet device control block.
*       user_cmd    Pointer to command structure.
*
* OUTPUTS
*       NU_SUCCESS  Indicates successful completion.
*
*************************************************************************/

STATUS EF_Set_Pkt_Filters_Rcvd(NU_USBF_ETH *eth,
							   NU_USBF_ETH_DEV *pdev,	
                               USBF_COMM_USER_CMD *user_cmd)
{
    ETHF_PKT_FILTER *filter = &pdev->packet_filter;

    /* Updating local variable with the filter bitmap. */
    UINT16 filter_bitmap = user_cmd->cmd_value;

    /* Initially setting all filter to NU_FALSE. */
    filter->packet_type_multicast     = NU_FALSE;
    filter->packet_type_broadcast     = NU_FALSE;
    filter->packet_type_directed      = NU_FALSE;
    filter->packet_type_all_multicast = NU_FALSE;
    filter->packet_type_promiscuous   = NU_FALSE;

    /* Check individual bits in bitmap and set the flag accordingly. */
    if(filter_bitmap & EF_MASK_BIT4)
    {
        filter->packet_type_multicast     = NU_TRUE;
    }
    if(filter_bitmap & EF_MASK_BIT3)
    {
        filter->packet_type_broadcast     = NU_TRUE;
    }
    if(filter_bitmap & EF_MASK_BIT2)
    {
        filter->packet_type_directed      = NU_TRUE;
    }
    if(filter_bitmap & EF_MASK_BIT1)
    {
        filter->packet_type_all_multicast = NU_TRUE;
    }
    if(filter_bitmap & EF_MASK_BIT0)
    {
        filter->packet_type_promiscuous   = NU_TRUE;
    }

    /* Pass this request to application. */
    if(eth->eth_ioctl)
    {
        eth->eth_ioctl((NU_USBF_USER *)eth,
                EF_SET_ETHERNET_PACKET_FILTER,
                filter,
                0);
    }
    return NU_SUCCESS;
}

/*************************************************************************
* FUNCTION
*
*        EF_Find_Device
*
* DESCRIPTION
*
*        Routine used to find pointer to the user device corresponding
*        to the handle provided by the Communication driver.
*
* INPUTS
*
*       cb               Pointer to user control block.
*       handle           handle to search.
*
* OUTPUTS
*
*       NU_NULL          Indicates device pointer not found.
*       NU_USBF_ETH_DEV* Pointer to the ethernet device.
*
*
*************************************************************************/
NU_USBF_ETH_DEV *EF_Find_Device (NU_USBF_ETH *cb,
                                 VOID *handle)
{

	NU_USBF_ETH_DEV *next;
	NU_USBF_ETH_DEV *eth_device = cb->eth_dev_list_head;

	/* Search for handle in the circular list of ethernet user
	 * instances.
	 */
	while (eth_device )
	{
		next = (NU_USBF_ETH_DEV*)(eth_device->node.cs_next);

		if (eth_device->handle == handle)
			return (eth_device);

		if ( (next == cb->eth_dev_list_head) ||
			(cb->eth_dev_list_head == NU_NULL))
			return (NU_NULL);
		else
			eth_device = next;						
	}	

	return (NU_NULL);
	
}
/* ======================  End Of File  =============================== */
