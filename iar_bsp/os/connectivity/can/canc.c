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
*       canc.c
*
* COMPONENT
*
*       Nucleus CAN
*
* DESCRIPTION
*
*       This file contains the core functions offered by Nucleus CAN.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       CANC_Receive_Data                   Function to get CAN message
*                                           from input queue.
*
*       CANC_Node_State                     Function to get the current
*                                           state of the node.
*
*       CANC_Sleep_Device                   Function to force the CAN
*                                           device to go to sleep mode.
*
*       CANC_Wakeup_Device                  Function to force the CAN
*                                           device to come out of sleep
*                                           mode.
*
*       CANC_Set_Baud_Rate                  Function to set baud rate of
*                                           a given CAN device.
*
*       CANC_Get_Baud_Rate                  Function to get current baud
*                                           rate of the node.
*
*       CANC_Close_Driver                   Function to close the driver
*                                           of a CAN device.
*
*       nu_os_conn_can_init                 CAN initialization routine.
*
* DEPENDENCIES
*
*       can_extr.h                          Function prototypes for
*                                           Nucleus CAN services.
*
*************************************************************************/
#define     NU_CAN_SOURCE_FILE

#include    "connectivity/can_extr.h"

extern      NU_MEMORY_POOL System_Memory;
extern      CAN_CB *CAN_Devs[NU_CAN_MAX_DEV_COUNT];
extern      STATUS CAN_Open_Device(DV_DEV_ID device_id, VOID *context);

/*************************************************************************
* FUNCTION
*
*       CANC_Receive_Data
*
* DESCRIPTION
*
*       This API function is used to get the data from the input queue
*       when data indication callback is sent to the user.
*
* INPUTS
*
*       *can_msg                            Reference where the received
*                                           CAN message will be returned.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       CAN_QUEUE_EMPTY                     Input queue has no received
*                                           CAN message.
*
*       CAN_UNSUPPORTED_PORT                Invalid CAN port ID given.
*
*       CAN_UNSUPPORTED_CONTROLLER          Invalid CAN device ID given.
*
*       CAN_DEV_NOT_INIT                    Specified CAN device is not
*                                           initialized.
*
*************************************************************************/
STATUS  CANC_Receive_Data    (CAN_PACKET *can_msg)
{
    CAN_CB         *can_cb = NU_NULL;
    CAN_PACKET      can_buffer;
    STATUS          status;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    if (can_msg != NU_NULL)
    {

#if         NU_CAN_MULTIPLE_PORTS_SUPPORT

        /* Get Nucleus CAN control block. */
        status = CANS_Get_CAN_CB(can_msg->can_port_id,
            can_msg->can_dev,
            &can_cb);
#else

        /* Get Nucleus CAN control block. */
        status = CANS_Get_CAN_CB(0, can_msg->can_dev, &can_cb);

#endif      /* NU_CAN_MULTIPLE_PORTS_SUPPORT */

    }
    else
    {
        /* Set status to indicate invalid parameter. */
        status = CAN_INVALID_PARAMETER;
    }

    /* Check if CAN control block is obtained successfully. */
    if (status == NU_SUCCESS)
    {
        /* Declare variable needed for critical section handling. */
        INT     old_level;

        /* Start critical section. */
        old_level = NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);

        /* Get the data from the queue. */
        CQM_Get_Rx_Queue(can_cb, &can_buffer, &status);

        /* Check if data has been received successfully. */
        if (status == NU_SUCCESS)
        {
            /* Copy the data to user buffer. */
            CANS_Copy_Msg(&can_buffer, can_msg);
        }

        /* End critical section. */
        NU_Local_Control_Interrupts(old_level);
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}


/*************************************************************************
* FUNCTION
*
*       CANC_Node_State
*
* DESCRIPTION
*
*       This API function is called to query the updated state of the CAN
*       node.
*
* INPUTS
*
*       can_port_id                         Nucleus CAN port ID.
*
*       can_dev_id                          Nucleus CAN Device ID.
*
* OUTPUTS
*
*       CAN_UNSUPPORTED_PORT                Invalid CAN port ID given.
*
*       CAN_UNSUPPORTED_CONTROLLER          Invalid CAN device ID given.
*
*       CAN_DEV_NOT_INIT                    Specified CAN device is not
*                                           initialized.
*
*       CAN_ERROR_ACTIVE_STATE              CAN node in Error Active
*                                           state. This is normal state.
*
*       CAN_ERROR_PASSIVE_STATE             CAN node in Error Passive
*                                           state.
*
*       CAN_BUS_OFF_STATE                   CAN node in Buss Off state.
*
*       CAN_BIT_ERROR                       Bit error detected.
*
*       CAN_FRAME_ERROR                     Framing error detected.
*
*       CAN_ACK_ERROR                       Acknowledgment error detected.
*
*       CAN_CRC_ERROR                       CRC error detected.
*
*       CAN_ERROR_LIGHT                     The node is experiencing some
*                                           errors on the bus.
*
*       CAN_ERROR_HEAVY                     The node is experiencing too
*                                           much errors on the bus.
*
*       CAN_ERROR_FATAL                     The node has stopped taking
*                                           part in communication due to
*                                           excessive errors.
*
*       CAN_HW_OVERRUN                      Hardware overflow occurred.
*
*       CAN_GENERAL_HARDWARE_ERROR          An undocumented CAN controller
*                                           specific error occurred.
*
*************************************************************************/
STATUS  CANC_Node_State     (UINT8 can_port_id,
                             CAN_HANDLE can_dev_id)
{
    CAN_CB         *can_cb = NU_NULL;
    STATUS          status;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Get Nucleus CAN control block. */
    status = CANS_Get_CAN_CB(can_port_id, can_dev_id, &can_cb);

    /* Check if CAN control block is obtained successfully. */
    if (status == NU_SUCCESS)
    {
        /* Get the last reported state of the node. */
        status = can_cb->can_state;
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       CANC_Sleep_Device
*
* DESCRIPTION
*
*       This API function is called to make a CAN controller go to
*       sleep mode where it would shutdown its clocks and will not
*       transmit anything on the network. This service is hardware
*       dependent and some controller may not provide this.
*
* INPUTS
*
*       can_port_id                         Nucleus CAN port ID.
*
*       can_dev_id                          Nucleus CAN Device ID.
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
*************************************************************************/
STATUS  CANC_Sleep_Device   (UINT8 can_port_id,
                             CAN_HANDLE can_dev_id)
{
    CAN_CB         *can_cb = NU_NULL;
    STATUS          status;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Get Nucleus CAN control block. */
    status = CANS_Get_CAN_CB(can_port_id, can_dev_id, &can_cb);

    /* Check if CAN control block is obtained successfully. */
    if (status == NU_SUCCESS)
    {
        /* Call the driver service to sleep the specified
        CAN controller. */
        status = DVC_Dev_Ioctl (can_cb->can_dev_handle,
                               (can_cb->can_ioctl_base + NU_CAN_SLEEP_NODE),
                               &can_dev_id, sizeof(CAN_HANDLE));
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       CANC_Wakeup_Device
*
* DESCRIPTION
*
*       This API function is called to make a CAN controller come
*       out of sleep mode and resume its normal activity. This service
*       is hardware dependent and some controller may not provide this.
*
* INPUTS
*
*       can_port_id                         Nucleus CAN port ID.
*
*       can_dev_id                          Nucleus CAN Device ID.
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
*************************************************************************/
STATUS  CANC_Wakeup_Device  (UINT8 can_port_id,
                             CAN_HANDLE can_dev_id)
{
    CAN_CB         *can_cb = NU_NULL;
    STATUS          status;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Get Nucleus CAN control block. */
    status = CANS_Get_CAN_CB(can_port_id, can_dev_id, &can_cb);

    /* Check if CAN control block is obtained successfully. */
    if (status == NU_SUCCESS)
    {
        /* Call the driver service to wakeup the specified
        CAN controller. */
        status = DVC_Dev_Ioctl (can_cb->can_dev_handle,
                               (can_cb->can_ioctl_base + NU_CAN_WAKEUP_NODE),
                               &can_dev_id, sizeof(CAN_HANDLE));
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       CANC_Set_Baud_Rate
*
* DESCRIPTION
*
*       This function can be used to change the baud rate of the currently
*       active CAN driver.
*
* INPUTS
*
*       can_dev                             CAN device ID.
*
*       baud_rate                           The baud rate value to be set.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       CAN_INVALID_BAUD_RATE               Given baud rate is not
*                                           supported. CAN device set to
*                                           default baud rate.
*
*       CAN_UNSUPPORTED_PORT                Invalid CAN port ID given.
*
*       CAN_UNSUPPORTED_CONTROLLER          Invalid CAN device ID given.
*
*       CAN_DEV_NOT_INIT                    Specified CAN device is not
*                                           initialized.
*
*************************************************************************/
STATUS  CANC_Set_Baud_Rate  (UINT8  can_port_id,
                             CAN_HANDLE  can_dev_id,
                             UINT16 baud_rate)
{
    CAN_CB                  *can_cb;
    STATUS                  status;
    CAN_DRV_IOCTL_SET_BAUD  can_drv_ioctl_set_baud;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Get Nucleus CAN control block. */
    status = CANS_Get_CAN_CB(can_port_id, can_dev_id, &can_cb);

    /* Check if CAN control block is obtained successfully. */
    if (status == NU_SUCCESS)
    {
        if ((baud_rate <= CAN_MAX_BAUDRATE) && (baud_rate != 0))
        {
            can_drv_ioctl_set_baud.baud_rate = baud_rate;
            can_drv_ioctl_set_baud.can_dev_id = can_dev_id;

            status = DVC_Dev_Ioctl (can_cb->can_dev_handle,
                                   (can_cb->can_ioctl_base + NU_CAN_SET_BAUD_RATE),
                                   &can_drv_ioctl_set_baud, sizeof(CAN_DRV_IOCTL_SET_BAUD));
        }
        else
        {
            /* Set the status to indicate that the baud
               rate is not valid. */
            status = CAN_INVALID_BAUDRATE;
        }
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the node state/service status. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       CANC_Get_Baud_Rate
*
* DESCRIPTION
*
*       This API function is called to get the current baud rate value of
*       the node. If the given CAN device ID is not valid, then 0 is
*       returned instead of the baud rate.
*
* INPUTS
*
*       can_port_id                         Nucleus CAN port ID.
*
*       can_dev_id                          Nucleus CAN Device ID.
*
* OUTPUTS
*
*       current value of the baud rate or 0 as explained above.
*
*************************************************************************/
UINT16      CANC_Get_Baud_Rate  (UINT8 can_port_id,
                                 CAN_HANDLE can_dev_id)
{
    CAN_CB         *can_cb = NU_NULL;
    STATUS          status;
    UINT16          baud_rate;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Get Nucleus CAN control block. */
    status = CANS_Get_CAN_CB(can_port_id, can_dev_id, &can_cb);

    /* Check if CAN control block is obtained successfully. */
    if (status == NU_SUCCESS)
    {
        /* Get the current baud rate of the node. */
        baud_rate = can_cb->can_baud;
    }

    else
    {
        /* Set baud rate to '0'. */
        baud_rate = 0;
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the node baud rate. */
    return (baud_rate);
}

/*************************************************************************
* FUNCTION
*
*       CANC_Close_Driver
*
* DESCRIPTION
*
*       This API function is used to stop all the functionality of
*       the given CAN device.
*
* INPUTS
*
*       can_port_id                         Nucleus CAN port ID.
*
*       can_dev_id                          Nucleus CAN Device ID.
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
*************************************************************************/
STATUS  CANC_Close_Driver   (UINT8 can_port_id,
                             CAN_HANDLE can_dev_id)
{
    CAN_CB         *can_cb = NU_NULL;
    STATUS          status;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Get Nucleus CAN control block. */
    status = CANS_Get_CAN_CB(can_port_id, can_dev_id, &can_cb);

    /* Check if CAN control block is obtained successfully. */
    if ((status == NU_SUCCESS) &&
        (can_cb->can_dev_init == CAN_DEV_IS_INITIALIZED))
    {
        /* Call the driver to shutdown the controller functionality. */
        status = DVC_Dev_Close(can_cb->can_dev_handle);

        if (status == NU_SUCCESS)
        {
            /* Deallocate the OS resources. */
            status = CAN_OSAL_Deallocate_Resources(can_cb);

            /* Reset the device initialization flag. */
            can_cb->can_dev_init = NU_FALSE;

            /* Reset the device open status. */
            can_cb->is_opened = NU_FALSE;
        }

        else
        {
            /* Set the status to indicate a shutdown error. */
            status = CAN_SHUTDOWN_ERROR;
        }
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       nu_os_conn_can_init
*
* DESCRIPTION
*
*       This is the generic initialization callback function of Nucleus CAN
*       for usage with registry.
*
* INPUTS
*
*       key                                 Registry Path
*       startstop                           Start or Stop middleware services
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID    nu_os_conn_can_init(const CHAR * key, INT startstop)
{
    STATUS              status;
    DV_DEV_LABEL        can_label = {CAN_LABEL};
    DV_LISTENER_HANDLE  listener_id;

    if(startstop)
    {
        /* Initialize the Nucleus CAN. */
        status = (STATUS)CANC_Initialize(&System_Memory);

        if (status == NU_SUCCESS)
        {
            /* Register callback for CAN device. */
            status =  DVC_Reg_Change_Notify(&can_label,
                                            DV_GET_LABEL_COUNT(can_label),
                                            &CAN_Open_Device,
                                            NU_NULL,
                                            NU_NULL,
                                            &listener_id);
        }
    }

    /* Suppress harmless compiler warnings. */
    NU_UNUSED_PARAM(status);
}
