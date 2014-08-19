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
*       cans.c
*
* COMPONENT
*
*       Nucleus CAN
*
* DESCRIPTION
*
*       This file contains the support routines for Nucleus CAN
*       services.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       CANS_Get_CAN_CB                     Function to get Nucleus CAN
*                                           control block.
*
*       CANS_Copy_Msg                       Function to copy Nucleus CAN
*                                           packet from one location to
*                                           another.
*
*       CANS_Calculate_Port_Driver_ID       Function to calculate starting
*                                           device number of a Nucleus CAN
*                                           port.
*
*       CANS_Check_Init_Params              Function for checking
*                                           initialization parameters
*                                           of a Nucleus CAN device.
*
* DEPENDENCIES
*
*       can_extr.h                          Function prototypes for
*                                           Nucleus CAN services.
*
*************************************************************************/
#define     NU_CAN_SOURCE_FILE

#include    "connectivity/can_extr.h"

extern      CAN_CB      *CAN_Devs        [NU_CAN_MAX_DEV_COUNT];
extern      CAN_CB       CAN_Dev_Control_Blocks [NU_CAN_MAX_DEV_COUNT];

/*************************************************************************
* FUNCTION
*
*       CANS_Get_CAN_CB
*
* DESCRIPTION
*
*       This function gets the control block of the specified device.
*
* INPUTS
*
*       can_port_id                         Nucleus CAN port ID.
*
*       can_dev_id                          Nucleus CAN Device ID.
*
*     **can_cb                              Pointer to the location where
*                                           Nucleus CAN control block
*                                           pointer will be returned.
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
STATUS  CANS_Get_CAN_CB                 (UINT8 can_port_id,
                                         CAN_HANDLE can_dev_id,
                                         CAN_CB **can_cb)
{
    STATUS          status = NU_SUCCESS;

#if         (NU_CAN_OPERATING_MODE == NU_CAN_LOOPBACK)

    /* Make sure loopback device ID and port ID are set correctly. */

    can_dev_id  = CAN_LOOPBACK_DEVICE;
    (VOID)can_dev_id;

#endif      /* (NU_CAN_OPERATING_MODE == NU_CAN_LOOPBACK) */

#if         NU_CAN_MULTIPLE_PORTS_SUPPORT

    /* Check if the given port ID is valid. */
    if (can_port_id >= CAN_MAX_PORTS)
    {
        /* Set the status to indicate wrong port ID. */
        status = CAN_UNSUPPORTED_PORT;
    }

    /* Check if the given device ID is valid. */
    else if (can_dev_id >= CAN_Devs_In_Port[can_port_id])
    {
        /* Set the status to indicate wrong device ID. */
        status = CAN_UNSUPPORTED_CONTROLLER;
    }

#else

    /* Check if the given device ID is valid. */
    if (can_dev_id >= NU_CAN_MAX_DEV_COUNT)
    {
        /* Set the status to indicate wrong device ID. */
        status = CAN_UNSUPPORTED_CONTROLLER;
    }

#endif      /* NU_CAN_MULTIPLE_PORTS_SUPPORT */

    else
    {
        /* Get the CAN control block. */
        *can_cb = CAN_Devs[can_dev_id];

        /* Check if the control block is valid. */
        if (*can_cb == NU_NULL)
        {
            /* Set the status to indicate that the device is not
               initialized. */
            status = CAN_DEV_NOT_INIT;
        }
    }

    /* Return the status of the service. */
    return (status);
}

#if         (!NU_CAN_OPTIMIZE_FOR_SPEED)

/*************************************************************************
* FUNCTION
*
*       CANS_Copy_Msg
*
* DESCRIPTION
*
*       This function is used to copy a specified CAN packed to a
*       destination CAN packet.
*
* INPUTS
*
*      *src_msg                             Source CAN message pointer.
*
*      *dest_msg                            Destination CAN message
*                                           pointer.
*
* OUTPUTS
*
*       None
*
*************************************************************************/
VOID        CANS_Copy_Msg                   (const CAN_PACKET *src_msg,
                                                   CAN_PACKET *dest_msg)
{
    UNSIGNED_INT       *dest;
    UNSIGNED_INT       *src;
    UNSIGNED_INT        count;

    /* Check if given pointers are valid. */
    if ((src_msg != NU_NULL) && (dest_msg  != NU_NULL))
    {
        /* Save the source and destination addresses in unsigned
           pointers. */
        dest = (UNSIGNED_INT *)dest_msg;
        src  = (UNSIGNED_INT *)src_msg;

        /* Calculate the number of word-sized data chunks to copy. */
        count = sizeof(CAN_PACKET)/sizeof(UNSIGNED_INT);

        /* Copy the data one word at a time. */
        while (count--)
        {
            *(dest++) = *(src++);
        }
    }
}

#if         NU_CAN_MULTIPLE_PORTS_SUPPORT

/*************************************************************************
* FUNCTION
*
*       CANS_Calculate_Port_Driver_ID
*
* DESCRIPTION
*
*       This function is used to calculate the driver ID w.r.t.
*       Nucleus CAN based upon Port ID and the driver ID within that port.
*       Internal driver ID of a port is calculated like following.
*
*        -------------------
*       | port 1 | driver#1 |
*       |        | driver#2 |
*        -------------------
*       | port 2 | driver#1 |
*        -------------------
*
*       So, internal ID for driver 1 of port 2 would be 3-1=2 as the
*       first driver of first port is numbered as 0.
*       If the given port ID is not valid the first driver of the first
*       port is returned.
*
* INPUTS
*
*       can_port_id                         CAN driver port ID.
*
*       can_dev_id                          CAN device ID within the port.
*
* OUTPUTS
*
*       can driver index
*
*************************************************************************/
INT         CANS_Calculate_Port_Driver_ID   (INT can_port_id,
                                             INT can_dev_id)
{
    INT   port_id;
    INT   prev_ports_drv_count = 0;

    /* Check if the specified CAN driver port ID is within valid range. */
    if (can_port_id < CAN_MAX_PORTS)
    {
        /* Get CAN driver port ID. */
        port_id = can_port_id;

        /* Sum up driver count for all ports till this port. */
        while (port_id--)
        {
            prev_ports_drv_count += CAN_Devs_In_Port[port_id];
        }
    }

    /* Return the final port driver ID for internal use of Nucleus CAN. */
    return (prev_ports_drv_count + can_dev_id);
}

#endif      /* NU_CAN_MULTIPLE_PORTS_SUPPORT */

#endif      /* (!NU_CAN_OPTIMIZE_FOR_SPEED) */

/*************************************************************************
* FUNCTION
*
*       CANS_Check_Init_Params
*
* DESCRIPTION
*
*       This function checks the initialization parameters of Nucleus CAN
*       and returns the pointer to the control block of the device which
*       is a candidate for initialization.
*
* INPUTS
*
*      *can_init                            Nucleus CAN initialization
*                                           structure.
*
*      *can_cb                              Pointer where Nucleus CAN
*                                           control block will be returned.
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
STATUS  CANS_Check_Init_Params(CAN_INIT *can_init, CAN_CB **can_cb)
{
    STATUS          status = NU_SUCCESS;
    
    /* Check if the parameter value is not null. */
    if (can_init != NU_NULL)
    {

#if         (!NU_CAN_MULTIPLE_PORTS_SUPPORT)

    /* Set port ID to zero to make sure valid port for single port. */
    can_init->can_port_id = 0;

#endif      /* NU_CAN_MULTIPLE_PORTS_SUPPORT */

        /* Check if the device is not already initialized. */
        if ((status == CAN_DEV_NOT_INIT) || (status == NU_SUCCESS))
        {
            /* Check if memory pool pointer is valid. */
            if (can_init->can_memory_pool == NU_NULL)
            {
                /* Set the status to indicate that the memory pool
                   pointer is null. */
                status = CAN_NULL_GIVEN_FOR_MEM_POOL;
            }

            else

            /* Check if given baud rate is not more than the
               maximum supported baud rate. */
            if ((can_init->can_baud > CAN_MAX_BAUDRATE) ||
                     (can_init->can_baud == 0))
            {
                /* Set the status to indicate invalid baud rate value. */
                status = CAN_INVALID_BAUDRATE;
            }
            else
            {
                /* Set the status to NU_SUCCESS. */
                status = NU_SUCCESS;
            }
        }
    }

    else
    {
        /* Set the status to indicate that null was passed as parameter
           to the API. */
        status = CAN_INVALID_PARAMETER;
    }

    /* Return the status of the service. */
    return(status);
}

/*************************************************************************
* FUNCTION
*
*       CANS_Get_Device_Index
*
* DESCRIPTION
*
*       This function gets the control block of the specified device.
*
* INPUTS
*
*       dev_id                              Device ID of controller.
*
* OUTPUTS
*
*       INDEX                               Index of device's control
*                                           block in the CAN_Devs array.
*       NU_SUCCESS                          Index lookup successful.
*       CAN_DEV_NOT_INIT                    Index lookup failed.
*
*************************************************************************/
STATUS CANS_Get_Device_Index (DV_DEV_ID dev_id, INT* index)
{
    INT     i = 0;
    STATUS  status = CAN_DEV_NOT_INIT;
    
    /* Loop through all the available control blocks to find one
     * with matching device ID. */
    for(i = 0; i<NU_CAN_MAX_DEV_COUNT; i++)
    {
        if(CAN_Devs[i]->can_dev_id == dev_id)
        {
            *index = i;
            status = NU_SUCCESS;
            break;
        }
    }
    
    return status;
}

