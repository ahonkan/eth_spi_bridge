/**************************************************************************
*
*               Copyright 2005  Mentor Graphics Corporation
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
*       nu_usbh_com_user_imp.c
*
* COMPONENT
*
*       Nucleus USB host software : Communication class user driver.
*
* DESCRIPTION
*
*       This file contains internal routines for user driver.
*
* DATA STRUCTURES
*
*       None.
*
* FUNCTIONS
*
*       NU_USB_COM_User_Poll_Data           Incoming data poll function.
*
* DEPENDENCIES
*
*       nu_usb.h                            All USB definitions.
*
**************************************************************************/

/* ======================  USB Include Files ==========================  */
#include "connectivity/nu_usb.h"

/**************************************************************************
* FUNCTION
*     NU_USB_COM_User_Poll_Data
*
* DESCRIPTION
*     Communication device's incoming data poll routine.
*
* INPUTS
*     pcb_drvr              Pointer to user driver control block.
*     pcb_mdm_dev           Pointer to MOD device control block.
*
* OUTPUTS
*     None.
*
**************************************************************************/

VOID NU_USBH_COM_User_Poll_Data(
     UINT32 pcb_usr_drvr,
     VOID*  pcb_curr_dev)
{

    STATUS status = NU_SUCCESS;
    NU_USBH_COM_USER*       pcb_user_drvr =
                            (NU_USBH_COM_USER*)pcb_usr_drvr;
    NU_USBH_COM_USR_DEVICE* usr_device   =
                            (NU_USBH_COM_USR_DEVICE*)pcb_curr_dev;

    while(1)
    {
        status = NU_Obtain_Semaphore(&(usr_device->poll_task_sync),NU_SUSPEND);
        if ( status == NU_SUCCESS )
        {
            status = NU_USBH_COM_Transfer_In (usr_device->usb_device,
                                                  &usr_device->rx_xblock);

            if(status == NU_SUCCESS)
            {
                pcb_user_drvr->p_hndl_table->Data_Handler(pcb_curr_dev,
                                                 &usr_device->rx_xblock);
            }
            else
            {
                NU_Sleep(10);
            }

            NU_Release_Semaphore(&(usr_device->poll_task_sync));
        }
    }
}
/* ======================= Global data ================================  */

