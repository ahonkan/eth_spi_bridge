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
*       can_init.c
*
* COMPONENT
*
*       CAN Init - Nucleus CAN Initialization component
*
* DESCRIPTION
*
*       This file contains the initialization service for Nucleus CAN.
*       It is also responsible for calling the driver initialization
*       service, whether for CAN loopback device or a CAN hardware
*       controller.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       CAN_Initialize                      Nucleus CAN initialization
*                                           function.
*
* DEPENDENCIES
*
*       can_extr.h                          Function prototypes for
*                                           Nucleus CAN services.
*
*************************************************************************/
#define     NU_CAN_SOURCE_FILE

/* Define to identify that the module is responsible for
   Nucleus CAN initialization. */
#define     NU_CAN_INITIALIZATION

#include    "connectivity/can_extr.h"

#if         (NU_CAN_OPERATING_MODE == NU_CAN_LOOPBACK)

extern      STATUS CAN_Loopback_Initialize(VOID);

#endif      /* (NU_CAN_OPERATING_MODE == NU_CAN_LOOPBACK) */

extern      CAN_CB  *CAN_Devs        [NU_CAN_MAX_DEV_COUNT];
extern      CAN_CB   CAN_Dev_Control_Blocks [NU_CAN_MAX_DEV_COUNT];

/* Functions hooks to be placed in CAN's control block. */
extern      STATUS  CAN_Tgt_Set_Baud_Rate   (UINT16 baud_rate,
                                             DV_DEV_ID can_dev);
extern      STATUS  CAN_Tgt_Terminate_Can   (DV_DEV_ID can_dev);
extern      STATUS  CAN_Tgt_Write_Driver    (DV_DEV_ID can_dev);
extern      STATUS  CAN_Tgt_Sleep_Node      (DV_DEV_ID can_dev);
extern      STATUS  CAN_Tgt_Wakeup_Node     (DV_DEV_ID can_dev);
extern      STATUS  CAN_Tgt_Set_AcpMask     (DV_DEV_ID can_dev,
                                             UINT8 mask_buf,
                                             UINT32 mask);

#if         (NU_CAN_AUTOMATIC_RTR_RESPONSE && NU_CAN_SUPPORTS_RTR)

extern      STATUS  CAN_Tgt_Assign_Msgbuff  (CAN_PACKET *can_msg);

extern      STATUS  CAN_Tgt_Release_Msgbuff (CAN_PACKET *can_msg);

#endif

#if         ((NU_CAN_OPERATING_MODE != NU_CAN_LOOPBACK))
extern      NU_SEMAPHORE    CAN_HW_Init_Semaphore;
#endif
STATUS      CAN_Open_Device(DV_DEV_ID device_id, VOID *context);

/*************************************************************************
* FUNCTION
*
*       CAN_Initialize
*
* DESCRIPTION
*
*       This API function creates Nucleus CAN driver and initializes the
*       CAN module.
*
* INPUTS
*
*      *mem_pool                            Memory pool pointer.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       CAN_UNSUPPORTED_PORT                The specified port isn't
*                                           supported. Either it doesn't
*                                           exist or it is not integrated
*                                           properly with Nucleus CAN.
*
*       CAN_INVALID_PARAMETER               Invalid parameter value
*                                           passed to the API.
*
*       Error returned by call to Nucleus API.
*
*************************************************************************/
STATUS CANC_Initialize(NU_MEMORY_POOL *mem_pool)
{

    CAN_CB         *can_cb = NU_NULL;
    STATUS          status;
    INT             can_dev = 0;

#if         NU_CAN_MULTIPLE_PORTS_SUPPORT

    /* Set the number of devices in each CAN hardware driver port.
       'break' statement has been deliberately left in the all
       cases except the last. */
#if         (CAN_MAX_PORTS == 1)

    CAN_Devs_In_Port[CAN_PORT1] = CAN_PORT1_DRIVERS;
    CAN_Devs_In_Port[CAN_PORT2] = 0;
    CAN_Devs_In_Port[CAN_PORT3] = 0;
    CAN_Devs_In_Port[CAN_PORT4] = 0;

#elif       (CAN_MAX_PORTS == 2)

    CAN_Devs_In_Port[CAN_PORT1] = CAN_PORT1_DRIVERS;
    CAN_Devs_In_Port[CAN_PORT2] = CAN_PORT2_DRIVERS;
    CAN_Devs_In_Port[CAN_PORT3] = 0;
    CAN_Devs_In_Port[CAN_PORT4] = 0;


#elif       (CAN_MAX_PORTS == 3)

    CAN_Devs_In_Port[CAN_PORT1] = CAN_PORT1_DRIVERS;
    CAN_Devs_In_Port[CAN_PORT2] = CAN_PORT2_DRIVERS;
    CAN_Devs_In_Port[CAN_PORT3] = CAN_PORT3_DRIVERS;
    CAN_Devs_In_Port[CAN_PORT4] = 0;

#elif       (CAN_MAX_PORTS == 4)

    CAN_Devs_In_Port[CAN_PORT1] = CAN_PORT1_DRIVERS;
    CAN_Devs_In_Port[CAN_PORT2] = CAN_PORT2_DRIVERS;
    CAN_Devs_In_Port[CAN_PORT3] = CAN_PORT3_DRIVERS;
    CAN_Devs_In_Port[CAN_PORT4] = CAN_PORT4_DRIVERS;

#else

#error      "This value of CAN_MAX_PORTS is not supported by Nucleus CAN. "

#endif      /* CAN_MAX_PORTS */

#endif      /* NU_CAN_MULTIPLE_PORTS_SUPPORT */

    /* Initialize various fields of all CAN control blocks. */
    while (can_dev < NU_CAN_MAX_DEV_COUNT)
    {
        /* Get the control block pointer. */
        can_cb = &CAN_Dev_Control_Blocks[can_dev];

        /* Initialize various fields of the control block. */
        can_cb->can_status  = NU_SUCCESS;
        can_cb->can_state   = CAN_ERROR_ACTIVE_STATE;
        can_cb->can_queue.can_tx_queue.can_qsize  = CAN_OUTQUEUE_SIZE;
        can_cb->can_queue.can_rx_queue.can_qsize  = CAN_INQUEUE_SIZE;
        can_cb->can_queue.can_tx_queue.can_qcount = 0;
        can_cb->can_queue.can_rx_queue.can_qcount = 0;
        can_cb->can_handler_type                  = 0;

        can_cb->can_dev_init = NU_FALSE;
        can_cb->is_opened = NU_FALSE;

        /* Update the CAN_Devs pointer array to point to the respective
         * control block in the control blocks array. */
        CAN_Devs[can_dev] = &CAN_Dev_Control_Blocks[can_dev];

        /* Increment can_dev to point to the next CAN
           device control block. */
        can_dev++;
    }

    if (mem_pool != NU_NULL)
    {
        /* Create message handler threads for Nucleus CAN. */
        status = CAN_OSAL_Create_Handler(mem_pool);

#if         ((NU_CAN_OPERATING_MODE != NU_CAN_LOOPBACK))
        if(status == NU_SUCCESS)
        {
            /* Create semaphore which will be used to wait for
             * hardware initialization. */
            status = NU_Create_Semaphore(&CAN_HW_Init_Semaphore,
                                         "CANHW", 0, NU_PRIORITY);
        }
#endif

    }

    else
    {
        /* Set the status to indicate that the memory pool
           pointer is null. */
        status = CAN_NULL_GIVEN_FOR_MEM_POOL;
    }

    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
* FUNCTION
*
*       CANC_Start
*
* DESCRIPTION
*
*       This API function creates Nucleus CAN driver and initializes the
*       CAN module.
*
* INPUTS
*
*       *can_init                           Pointer to device
*                                           initialization block structure.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       CAN_INVALID_PARAMETER               Invalid parameter value
*                                           passed to the API.
*
*       CAN_UNSUPPORTED_PORT                The specified port isn't
*                                           supported. Either it doesn't
*                                           exist or it is not integrated
*                                           properly with Nucleus CAN.
*
*       CAN_DEV_NOT_INIT                    Invalid handle given.
*
*       Error returned by call to Nucleus API.
*
*************************************************************************/
STATUS CANC_Start(CAN_HANDLE *can_dev, CAN_INIT *can_init)
{
    CAN_CB             *can_cb;
    STATUS              status = NU_SUCCESS;
    DV_DEV_ID           can_ctlr_dev_id;
    INT                 dev_id_cnt = 1;
    INT                 index = 0;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Make sure that provided placeholder for device's handler
       is not NULL. */
    if (can_dev == NU_NULL)
    {
        status = CAN_INVALID_POINTER;
    }

    /* Proceed only if we have a valid placeholder. */
    if (status == NU_SUCCESS)
    {
        /* Get the control block of the specified device. */
        status = DVC_Dev_ID_Get (&(can_init->can_controller_label), 1, &can_ctlr_dev_id, &dev_id_cnt);

        /* Call the service to check Nucleus CAN initialization parameters. */
        if (status == NU_SUCCESS)
        {
            status = CANS_Check_Init_Params(can_init, &can_cb);
        }

        /* Check if parameters are valid. */
        if(status == NU_SUCCESS && dev_id_cnt > 0)
        {
            /* Get the index of CAN CB for the controller */
            status = CANS_Get_Device_Index(can_ctlr_dev_id, &index);

            if (status == NU_SUCCESS)
            {
                /* Get the control block associated with this CAN device ID. */
                can_cb = CAN_Devs[index];

                /* Open the CAN device if not already opened. */
                if (can_cb->is_opened == NU_FALSE)
                {
                    status = CAN_Open_Device(can_ctlr_dev_id, NU_NULL);

                    /* After we open the device, we need to refresh the control
                       block based on the new index alloted to this device ID in
                       the control blocks array. It can cause problems if this
                       step is skipped. */

                    /* Get the index of CAN CB for the controller */
                    status = CANS_Get_Device_Index(can_ctlr_dev_id, &index);

                    if (status == NU_SUCCESS)
                    {
                        /* Get the control block associated with this CAN device ID. */
                        can_cb = CAN_Devs[index];
                    }
                }

                /* Check if CAN device is already initialized. */
                if ((status == NU_SUCCESS) &&
                    (can_cb->can_dev_init != CAN_DEV_IS_INITIALIZED))
                {
                    /* Set CAN control block fields. */
                    can_cb->can_dev_id  = can_ctlr_dev_id;
                    can_cb->can_port_id = can_init->can_port_id;
                    can_cb->can_baud    = can_init->can_baud;

                    /* Set application callbacks. */
                    can_cb->can_ucb.can_error                 =
                        can_init->can_callbacks.can_error;
                    can_cb->can_ucb.can_data_indication       =
                        can_init->can_callbacks.can_data_indication;
                    can_cb->can_ucb.can_data_confirm          =
                        can_init->can_callbacks.can_data_confirm;
                    can_cb->can_ucb.can_rtr_indication        =
                        can_init->can_callbacks.can_rtr_indication;
                    can_cb->can_ucb.can_rtr_confirm           =
                        can_init->can_callbacks.can_rtr_confirm;

                    /* Initialize OS resources. */
                    status = CAN_OSAL_Allocate_Resources(can_cb,
                                        can_init->can_memory_pool);

                    /* Check if the resources are allocated successfully. */
                    if (status == NU_SUCCESS)
                    {
                        /* Initialize output queue's starting read pointer. */
                        can_cb->can_queue.can_tx_queue.can_qread =
                            can_cb->can_queue.can_buff_out;

                        /* Initialize output queue's write pointer. */
                        can_cb->can_queue.can_tx_queue.can_qwrite =
                            can_cb->can_queue.can_tx_queue.can_qread;

                        /* Set output queue's end. */
                        can_cb->can_queue.can_tx_queue.can_qend =
                            can_cb->can_queue.can_tx_queue.can_qread +
                            can_cb->can_queue.can_tx_queue.can_qsize;

                        /* Reset output queue message count. */
                        can_cb->can_queue.can_tx_queue.can_qcount = 0;

                        /* Initialize input queue's starting read pointer. */
                        can_cb->can_queue.can_rx_queue.can_qread =
                            can_cb->can_queue.can_buff_in;

                        /* Initialize input queue's write pointer. */
                        can_cb->can_queue.can_rx_queue.can_qwrite =
                            can_cb->can_queue.can_rx_queue.can_qread;

                        /* Set input queue's end. */
                        can_cb->can_queue.can_rx_queue.can_qend =
                            can_cb->can_queue.can_rx_queue.can_qread +
                            can_cb->can_queue.can_rx_queue.can_qsize;

                        /* Reset input queue message count. */
                        can_cb->can_queue.can_rx_queue.can_qcount = 0;

#if         ((NU_CAN_OPERATING_MODE == NU_CAN_LOOPBACK))

                        /* Initialize Nucleus CAN loopback device. */
                        status = CAN_Loopback_Initialize();

#endif      /* (NU_CAN_OPERATING_MODE == NU_CAN_LOOPBACK) */

                        /* Return the device handle information to the
                           caller. */
                        *can_dev  = index;

                        /* Update CAN device state. */
                        can_cb->can_dev_init = CAN_DEV_IS_INITIALIZED;
                        
                        if (status == NU_SUCCESS)
                        {
                            status = CANC_Set_Baud_Rate (can_cb->can_port_id, 
                                        index, can_cb->can_baud);
                        }
                    }
                }

                else
                {
                    /* Set the status to indicate that the device is already
                        initialized. */
                    status = CAN_DEV_ALREADY_INIT;
                }
            }
        }
    }

    /* Revert back to user mode. */
    NU_USER_MODE();

    /* Return the completion status of the service. */
    return (status);
}

/***********************************************************************
*
*   FUNCTION
*
*       CAN_Open_Device
*
*   DESCRIPTION
*
*       This function opens a CAN device.
*
*   INPUTS
*
*       device_id                   Device ID of newly registered CAN
*                                   device.
*       context                     Context information for this callback.
*                                   CAN_ID for this component.
*
*   OUTPUTS
*
*       NU_SUCCESS
*       CAN_SESSION_NOT_AVAILABLE
*
***********************************************************************/
STATUS CAN_Open_Device(DV_DEV_ID device_id, VOID *context)
{
    STATUS              status = NU_SUCCESS;
    DV_DEV_LABEL        can_label = {CAN_LABEL};
    DV_IOCTL0_STRUCT    dev_ioctl0;
    CAN_CB              *can_cb;
    INT                 index;

    /* Find an available CB which is not opened. */
    for(index = 0; index<NU_CAN_MAX_DEV_COUNT; index++)
    {
        if (CAN_Devs[index]->is_opened == NU_FALSE)
        {
            /* Point to the available control block. */
            can_cb = CAN_Devs[index];

            /* Assign the device ID to this control block. */
            can_cb->can_dev_id = device_id;

            /* Mark this control block as opened. */
            can_cb->is_opened = NU_TRUE;

            break;
        }
    }

    /* Check if we were able to find a valid control block. */
    if(index < NU_CAN_MAX_DEV_COUNT)
    {
        /* Open the CAN device. */
        status = DVC_Dev_ID_Open(device_id, &can_label, 1, &(can_cb->can_dev_handle));

        /* If device was opened successfully then initialize it. */
        if(status == NU_SUCCESS)
        {
            /* Init ioct0 structure with CAN 'mode' label */
            dev_ioctl0.label = can_label;

            status = DVC_Dev_Ioctl(can_cb->can_dev_handle, DV_IOCTL0, &dev_ioctl0, sizeof(DV_IOCTL0_STRUCT));

            if(status == NU_SUCCESS)
            {
                can_cb->can_ioctl_base = dev_ioctl0.base;
            }
            else
            {
                /* Close the device */
                status = DVC_Dev_Close(can_cb->can_dev_handle);
            }
        }
    }
    else
    {
        status = CAN_SESSION_NOT_AVAILABLE;
    }

    return status;
}


