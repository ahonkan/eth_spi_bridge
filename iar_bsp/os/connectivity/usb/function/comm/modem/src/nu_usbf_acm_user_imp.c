/**************************************************************************
*
*           Copyright 2005 Mentor Graphics Corporation
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
*       nu_usbf_acm_user_imp.c
*
*
* COMPONENT
*
*       Nucleus USB Function Software : ACM User Driver.
*
* DESCRIPTION
*
*       This file contains the Control Block and other internal data
*       structures and definitions for ACM User Driver.
*
*  DATA STRUCTURES
*
*       None.
*
*  FUNCTIONS
*
*       ACM_Send_Notification               Sends ACM notification.
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions.
*
*
**************************************************************************/
#include "connectivity/nu_usb.h"
/* ==================================================================== */

/*************************************************************************
* FUNCTION
*
*        ACM_Send_Notification
*
* DESCRIPTION
*
*        Routine used to send notification to Host.
*
* INPUTS
*       cb          Pointer to user control block.
*       pdev        Pointer to the user device
*       notif       Notification code for sending to Host.
*
* OUTPUTS
*       NU_SUCCESS          Indicates successful completion.
*
*
*************************************************************************/
STATUS ACM_Send_Notification(
       NU_USBF_ACM_USER*     cb,
       NU_USBF_ACM_DEV*      pdev,
       UINT8                 notif)
{
    STATUS status = NU_USB_INVLD_ARG;
    	

    USBF_COMM_USER_NOTIFICATION* user_notif;
    NU_ASSERT(cb);
		

	if (pdev)
	{
	    user_notif = &cb->user_notif;

	    /* If the notification is sent to the host that
	     * response to AT command is available
	     */
	    if(notif == ACM_RESPONSE_AVAILABLE)
	    {
	        pdev->acmf_curr_notif = notif;
	        user_notif->notification = notif;
	        user_notif->notif_value = ACMF_ZERO;
	        user_notif->data = NU_NULL;
	        user_notif->length = 0;
	    }
	    /* If the notification is sent to the host
	       regarding the serial state  */
	    else if (notif == ACM_SERIAL_STATE)
	    {
	        pdev->acmf_curr_notif = notif;
	        user_notif->notification = notif;
	        user_notif->notif_value = ACMF_ZERO ;
	        user_notif->data = pdev->acmf_notif_buff;
	        user_notif->length = 2;
	    }
	    /* Call the communication class driver's API to send notification. */
	    status = NU_USBF_COMM_Send_Notification(
	               ((NU_USBF_USER_COMM *)cb)->mng_drvr,
	               user_notif,pdev->handle);
	}
    return status;
}

/*************************************************************************
* FUNCTION
*
*        ACM_Find_Device
*
* DESCRIPTION
*
*        Routine used to find pointer to the user device corresponding
*        to the handle provided by the COMMs driver.
*
* INPUTS
*
*       cb          Pointer to user control block.
*       handle      handle to search.
*
* OUTPUTS
*
*       NU_USBF_ACM_DEV*         Pointer to the ACM device.
*       NU_NULL                  NULL pointer.
*
*
*************************************************************************/
NU_USBF_ACM_DEV *ACM_Find_Device (NU_USBF_ACM_USER *cb,
                                  VOID *handle)
{

	NU_USBF_ACM_DEV *next;
	NU_USBF_ACM_DEV *acm_device = cb->acm_list_head;

	/* Search for handle in the circular list of ACM user
	 * instances.
	 */
	while (acm_device )
	{
		next = (NU_USBF_ACM_DEV*)(acm_device->node.cs_next);

		if (acm_device->handle == handle)
			return (acm_device);

		if ( (next == cb->acm_list_head) ||
			(cb->acm_list_head == NU_NULL))
			return (NU_NULL);
		else
			acm_device = next;						
	}	

	return (NU_NULL);
	
}
/* ======================  End Of File  =============================== */

