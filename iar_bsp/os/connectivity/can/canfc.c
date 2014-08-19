/****************************************************************************
*
*                  Copyright 2002 Mentor Graphics Corporation
*                             All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
****************************************************************************/

/**************************************************************************
*
* FILE NAME
*
*       canfc.c
*
* COMPONENT
*
*       Nucleus CAN
*
* DESCRIPTION
*
*       This file contains the routines for supporting CAN message
*       filter configuration. The services contained in this file
*       will not be available in loopback mode of Nucleus CAN.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       CANFC_Set_Mask                      Function to set the
*                                           reception mask for a single
*                                           message buffer or a group of
*                                           message buffers.
*
* DEPENDENCIES
*
*       can_extr.h                          Function prototypes for
*                                           Nucleus CAN services.
*
*************************************************************************/
#define     NU_CAN_SOURCE_FILE

#include    "connectivity/can_extr.h"

/*************************************************************************
* FUNCTION
*
*       CANFC_Set_Mask
*
* DESCRIPTION
*
*       This API function can be used to set the acceptance mask for a
*       buffer of the specified CAN device. The behavior of this service
*       is highly hardware dependent. Refer to your CAN controller manual
*       for detail of the possible values to the second parameter.
*
* INPUTS
*
*       can_port_id                         Nucleus CAN port ID.
*
*       can_dev                             CAN device ID.
*
*       buffer_no                           Buffer number for which
*                                           acceptance mask for received
*                                           messages is to be set.
*
*       mask_value                          The mask value to be set.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       CAN_UNSUPPORTED_PORT                Invalid CAN port ID given.
*
*       CAN_UNSUPPORTED_CONTROLLER          Invalid CAN device ID given.
*
*       CAN_DEV_NOT_INIT                    Specified CAN device is not
*                                           initialized.
*
*       CAN_NO_MASK_BUFF                    Specified mask buffer is not
*                                           supported by the device.
*
*       CAN_SERVICE_NOT_SUPPORTED           The requested operation can't
*                                           be performed as the service is
*                                           not supported by hardware
*                                           driver.
*
*************************************************************************/
STATUS  CANFC_Set_Mask   (UINT8  can_port_id,
                          CAN_HANDLE  can_dev_id,
                          UINT8  buffer_no,
                          UINT32 mask_value)
{
    CAN_CB                  *can_cb;
    STATUS                  status;
    CAN_DRV_IOCTL_ACP_MASK  can_drv_ioctl_acp_mask;
    
    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Get Nucleus CAN control block. */
    status = CANS_Get_CAN_CB(can_port_id, can_dev_id, &can_cb);

    /* Check if CAN control block is obtained successfully. */
    if (status == NU_SUCCESS)
    {
        can_drv_ioctl_acp_mask.buffer_no = buffer_no;
        can_drv_ioctl_acp_mask.can_dev_id = can_dev_id;
        can_drv_ioctl_acp_mask.mask_value = mask_value;
        
        status = DVC_Dev_Ioctl (can_cb->can_dev_handle,
                               (can_cb->can_ioctl_base + NU_CAN_SET_ACP_MASK),
                               &can_drv_ioctl_acp_mask, sizeof(CAN_DRV_IOCTL_ACP_MASK));
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}
