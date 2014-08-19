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
*       iso11898.c
*
* COMPONENT
*
*       ISO11898 - Nucleus CAN implementation for ISO11898 core
*
* DESCRIPTION
*
*       This file contains the functions used support core services of
*       ISO11898 in Nucleus CAN.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       ISO11898_Data_Request               Function for CAN data message
*                                           transmission.
*
*       ISO11898_Remote_Request             Function for CAN RTR message
*                                           transmission.
*
*       ISO11898_Assign_Remote              Function for assigning
*                                           automatic response to
*                                           incoming matching RTR.
*
*       ISO11898_Clear_Remote               Function for clearing a
*                                           previously assigned automatic
*                                           RTR response.
*
*       ISO11898_Check_Parameters           Function for checking
*                                           parameters of the services.
*
* DEPENDENCIES
*
*       can_extr.h                          Function prototypes for
*                                           Nucleus CAN services.
*
*       cqm_extr.h                          Function prototypes and macros
*                                           for Nucleus CAN queue
*                                           management services.
*
*       iso11898_extr.h                     Function prototypes for
*                                           ISO11898 services.
*
*************************************************************************/
#define     NU_CAN_SOURCE_FILE

#include    "connectivity/can_extr.h"
#include    "connectivity/cqm_extr.h"
#include    "connectivity/iso11898_extr.h"

extern      CAN_CB      *CAN_Devs[NU_CAN_MAX_DEV_COUNT];

/*************************************************************************
* FUNCTION
*
*       ISO11898_Data_Request
*
* DESCRIPTION
*
*       This API function is responsible for requesting the driver to
*       transmit the given data over CAN bus.
*
* INPUTS
*
*      *can_msg                             CAN message pointer.
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
*       CAN_INVALID_MSG_POINTER             The given CAN message pointer
*                                           is invalid.
*
*       CAN_ID_RANGE_ERROR                  CAN ID range is out of bound.
*
*       CAN_INVALID_DATA_LENGTH             Data length is invalid.
*                                           Set to maximum allowed.
*
*       CAN_NO_FREE_MB                      No transmission message buffer
*                                           is free.
*
*       CAN_INVALID_MSG_ID_TYPE             Message identifier is not
*                                           correctly specified. It should
*                                           be either CAN_STANDARD_ID or
*                                           CAN_EXTENDED_ID
*
*       CAN_QUEUE_FULL                      Output queue is full. No more
*                                           packet can be put into the
*                                           output queue.
*
*************************************************************************/
STATUS ISO11898_Data_Request    (CAN_PACKET *can_msg)
{
    CAN_CB         *can_cb;
    STATUS          status;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Check API parameters. */
    status = ISO11898_Check_Parameters(can_msg);

    /* Check if everything is O.K. till this point. */
    if (status == NU_SUCCESS)
    {
        /* Declare variable needed by mode switching macros. */
        INT     old_level;

        /* Set the message type for the CAN message. */
        can_msg->can_msg_type = CAN_DATA_MSG;

        /* Get the control block for the specified driver. */
        can_cb = CAN_Devs[can_msg->can_dev];

        /* Start critical section. */
        old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

        /* Put the data in the transmit queue. */
        CQM_Put_Tx_Queue(can_cb, can_msg, &status);

        /* End critical section. */
        NU_Local_Control_Interrupts(old_level);

        /* Check if data has been put in the transmit queue. */
        if (status == NU_SUCCESS)
        {
            /* Call the driver to send the data. */
            status = DVC_Dev_Ioctl (can_cb->can_dev_handle,
                                   (can_cb->can_ioctl_base + NU_CAN_DATA_REQUEST),
                                   (VOID*)can_msg, sizeof(CAN_PACKET*));
        }
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}

#if         (NU_CAN_SUPPORTS_RTR)

/*************************************************************************
* FUNCTION
*
*       ISO11898_Remote_Request
*
* DESCRIPTION
*
*       This API function provides the service for transmitting the RTR
*       (Remote Transmission Request) messages over CAN bus.
*
* INPUTS
*
*      *can_msg                             CAN message pointer.
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
*       CAN_INVALID_MSG_POINTER             The given CAN message pointer
*                                           is invalid.
*
*       CAN_ID_RANGE_ERROR                  CAN ID range is out of bound.
*
*       CAN_INVALID_DATA_LENGTH             Data length is invalid.
*                                           Set to maximum allowed.
*
*       CAN_NO_FREE_MB                      No transmission message buffer
*                                           is free.
*
*       CAN_INVALID_MSG_ID_TYPE             Message identifier is not
*                                           correctly specified. It should
*                                           be either CAN_STANDARD_ID or
*                                           CAN_EXTENDED_ID
*
*       CAN_QUEUE_EMPTY                     The output queue was empty,
*                                           while the driver tried to
*                                           fetch the packet for
*                                           transmission.
*
*       CAN_QUEUE_FULL                      Output queue is full. No more
*                                           packet can be put into the
*                                           output queue.
*
*************************************************************************/
STATUS ISO11898_Remote_Request  (CAN_PACKET *can_msg)
{
    CAN_CB         *can_cb;
    STATUS          status;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Check API parameters. */
    status = ISO11898_Check_Parameters(can_msg);

    /* Check if everything is O.K. till this point. */
    if (status == NU_SUCCESS)
    {
        /* Set the message type for the CAN message. */
        can_msg->can_msg_type = CAN_RTR_MSG;

        /* Get the control block for the specified driver. */
        can_cb = CAN_Devs[can_msg->can_dev];

        /* Put the data in the transmit queue. */
        CQM_Put_Tx_Queue(can_cb, can_msg, &status);

        /* Check if data has been put in the transmit queue. */
        if (status == NU_SUCCESS)
        {
            /* Call the driver to send the data. */
            status = DVC_Dev_Ioctl (can_cb->can_dev_handle,
                                   (can_cb->can_ioctl_base + NU_CAN_DATA_REQUEST),
                                   (VOID*)can_msg, sizeof(CAN_PACKET*));
        }
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}

#endif      /* NU_CAN_SUPPORTS_RTR */

#if         NU_CAN_AUTOMATIC_RTR_RESPONSE

/*************************************************************************
* FUNCTION
*
*       ISO11898_Assign_Remote
*
* DESCRIPTION
*
*       This API function allocates a response for handling RTRs.
*       If an incoming RTR message has the same ID as the ID
*       assigned by this service to a transmit-message buffer, the message
*       buffer will transmit the response message automatically.
*
* INPUTS
*
*      *can_msg                             CAN message pointer.
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
*       CAN_INVALID_MSG_POINTER             The given CAN message pointer
*                                           is invalid.
*
*       CAN_ID_RANGE_ERROR                  CAN ID range is out of bound.
*
*       CAN_INVALID_DATA_LENGTH             Data length is invalid.
*                                           Set to maximum allowed.
*
*       CAN_NO_FREE_MB                      No message buffer is free to
*                                           reserve for RTR response.
*
*       CAN_RTR_ALREADY_ASSIGNED            RTR message is already set
*                                           for response.
*
*************************************************************************/
STATUS ISO11898_Assign_Remote   (CAN_PACKET *can_msg)
{
    STATUS      status;
    CAN_CB      *can_cb;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Check API parameters. */
    status = ISO11898_Check_Parameters(can_msg);

    /* Check if everything is O.K. till this point. */
    if (status == NU_SUCCESS)
    {
        /* Set the message type to data message. */
        can_msg->can_msg_type = CAN_DATA_MSG;

        /* Get the control block for the specified driver. */
        can_cb = CAN_Devs[can_msg->can_dev];
        
        /* Call the driver to assign buffer for response to an
           incoming remote request. */
        status = DVC_Dev_Ioctl (can_cb->can_dev_handle,
                               (can_cb->can_ioctl_base + NU_CAN_ASSIGN_MSG_BUFFER),
                               (VOID*)can_msg, sizeof(CAN_PACKET*));
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       ISO11898_Clear_Remote
*
* DESCRIPTION
*
*       This API function clears the RTR response previously allocated for
*       handling external remote requests.
*
* INPUTS
*
*      *can_msg                             CAN message pointer.
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
*       CAN_INVALID_MSG_POINTER             The given CAN message pointer
*                                           is invalid.
*
*       CAN_ID_RANGE_ERROR                  CAN ID range is out of bound.
*
*       CAN_NO_ASSIGNED_MB                  No message buffer has been
*                                           assigned a response with the
*                                           specified message ID.
*
*************************************************************************/
STATUS ISO11898_Clear_Remote    (CAN_PACKET *can_msg)
{
    STATUS      status;
    CAN_CB      *can_cb;
    
    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Check API parameters. */
    status = ISO11898_Check_Parameters(can_msg);

    /* Check if necessary parameters are O.K. till this point. */
    if ((status == NU_SUCCESS) || (status == CAN_INVALID_DATA_LENGTH))
    {
        /* Get the control block for the specified driver. */
        can_cb = CAN_Devs[can_msg->can_dev];
        
        /* Call the driver to free the buffer assigned for RTR
           response. */
        status = DVC_Dev_Ioctl (can_cb->can_dev_handle,
                               (can_cb->can_ioctl_base + NU_CAN_RELEASE_MSG_BUFFER),
                               (VOID*)can_msg, sizeof(CAN_PACKET*));
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}

#endif      /* NU_CAN_AUTOMATIC_RTR_RESPONSE */

#if         (NU_CAN_DEBUG)

/*************************************************************************
* FUNCTION
*
*       ISO11898_Check_Parameters
*
* DESCRIPTION
*
*       This private function checks the validity of the parameters of an
*       API.
*
* INPUTS
*
*      *can_msg                             CAN message pointer.
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
*       CAN_INVALID_DATA_LENGTH             The specified length is invalid.
*                                           Length set to maximum CAN data
*                                           length.
*
*       CAN_INVALID_MSG_POINTER             The given CAN message pointer
*                                           is invalid.
*
*       CAN_ID_RANGE_ERROR                  CAN ID range is out of bound.
*
*************************************************************************/
STATUS ISO11898_Check_Parameters(CAN_PACKET *can_msg)
{
    CAN_CB         *can_cb = NU_NULL;
    STATUS          status = NU_SUCCESS;

    /* Check if the CAN message pointer is not null. */
    if (can_msg != NU_NULL)
    {

#if         NU_CAN_MULTIPLE_PORTS_SUPPORT

        /* Get Nucleus CAN control block. */
        status = CANS_Get_CAN_CB(can_msg->can_port_id,
                                 can_msg->can_dev,
                                 &can_cb);

#else

        /* Get Nucleus CAN control block. */
        status = CANS_Get_CAN_CB(0,
                                 can_msg->can_dev,
                                 &can_cb);

#endif      /* NU_CAN_MULTIPLE_PORTS_SUPPORT */

        /* Check if all the parameters are O.K. till this point. */
        if(status == NU_SUCCESS)
        {
            if ((can_msg->can_msg_id_type != CAN_STANDARD_ID)  &&
                (can_msg->can_msg_id_type != CAN_EXTENDED_ID))
            {
                /* Set the status to indicate wrong message ID type. */
                status = CAN_INVALID_MSG_ID_TYPE;
            }

            /* Check if the identifier is within valid range. */
            else if (((can_msg->can_msg_id_type == CAN_STANDARD_ID) &&
                (can_msg->can_msg_id > CAN_MAX_STANDARD_ID))    ||
                ((can_msg->can_msg_id_type == CAN_EXTENDED_ID) &&
                (can_msg->can_msg_id > CAN_MAX_EXTENDED_ID)))
            {
                /* Set the status to indicate ID range violation. */
                status = CAN_ID_RANGE_ERROR;
            }

            /* Check if message length is valid. */
            else if (can_msg->can_msg_length > CAN_MAX_DATA)
            {
                /* Set the status to indicate invalid data length. */
                status = CAN_INVALID_DATA_LENGTH;

                /* Limit the length to maximum data length allowed. */
                can_msg->can_msg_length = CAN_MAX_DATA;
            }
            else
            {
                /* Unused else. Put here for compliance with MISRA
                   rules. */
            }
        }
    }

    else
    {
        /* Set the status to indicate invalid CAN message pointer. */
        status = CAN_INVALID_MSG_POINTER;
    }

    /* Return the status of the service. */
    return (status);
}

#endif      /* NU_CAN_DEBUG */

