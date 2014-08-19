/*************************************************************************
*
*               Copyright 2006 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
* FILE NAME
*
*       spi_init.c
*
* COMPONENT
*
*       Nucleus SPI Initialization
*
* DESCRIPTION
*
*       This file contains the initialization functions for Nucleus SPI.
*
* DATA STRUCTURES
*
*
* FUNCTIONS
*
*       nu_os_conn_spi_init                 SPI Middleware Entry Point
*
*       SPI_Start                           Initializes the specified
*                                           SPI device.
*
*       SPI_Open_Device                     This function opens a SPI
*                                           device.
*       SPI_Close_Device                    This function closes a SPI
*                                           device.
*
* DEPENDENCIES
*
*       spi_osal_extr.h                     Function prototypes for
*                                           Nucleus SPI OSAL component.
*
*       spi_init_extr.h                     Function prototypes for
*                                           Nucleus SPI Initialization
*                                           component.
*
*************************************************************************/
#define     NU_SPI_SOURCE_FILE

#include    "connectivity/spi_osal_extr.h"
#include    "services/nu_services.h"

/*********************************/
/* EXTERNAL VARIABLES            */
/*********************************/
extern NU_MEMORY_POOL  System_Memory;

/* Define to identify that the module is responsible for
   Nucleus SPI initialization. */
#define     NU_SPI_INITIALIZATION

#include    "connectivity/spi_init_extr.h"

/* Define external inner-component global data references. */
extern      SPI_CB         SPI_Devices[SPI_MAX_DEV_COUNT];

/* Define file static data */
static  CHAR    SPI_MW_Reg_Path[REG_MAX_KEY_LENGTH];

/* Internal Functions */
STATUS SPI_Open_Device (DV_DEV_ID spi_dev_id, VOID *context);
STATUS SPI_Close_Device (DV_DEV_ID spi_dev_id, VOID *context);
static STATUS SPI_Get_Registry_Info(SPI_CB* spi_cb);

/*************************************************************************
* FUNCTION
*
*       nu_os_conn_spi_init
*
* DESCRIPTION
*
*       This function initializes Nucleus SPI. It
*       also creates the notification handler thread for Nucleus SPI and
*       SPI controller discovery task.
*
* INPUTS
*
*       key                                 Registry Path
*       startorstop                         Start or Stop middleware services
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       SPI_REGISTRY_ERROR                  Registry error
*
*       Error from NU_Create_Task API call
*
*************************************************************************/
STATUS  nu_os_conn_spi_init(const CHAR * key, INT startorstop)
{
    STATUS          status = NU_SUCCESS;
    INT                 index;
    DV_DEV_LABEL        device_type_label[] = {{SPI_LABEL}};
    DV_LISTENER_HANDLE  listener_handle;

    if(startorstop == RUNLEVEL_START)
    {
        /* Save SPI MW registry Path */
        if((strlen(key)+1) <= REG_MAX_KEY_LENGTH)
        {
            strcpy(SPI_MW_Reg_Path,key);

            status = DVC_Reg_Change_Notify(device_type_label,
                                        DV_GET_LABEL_COUNT(device_type_label),
                                        SPI_Open_Device,
                                        SPI_Close_Device,
                                        NU_NULL,
                                        &listener_handle);
            if (status == NU_SUCCESS)
            {
                /* Create notification handler thread for Nucleus SPI. */
                status = SPI_OSAL_Create_Handler(&System_Memory);
            }
        }
        else
        {
            /* The registry path is small for the key */
            status = SPI_REGISTRY_ERROR;
        }
    }
    else if (startorstop == RUNLEVEL_STOP)
    {
        /* Delete the notification handler thread */
        SPI_OSAL_Delete_Handler();

        /* Close all opened devices and Deallocate resources */
        for(index=0; index<SPI_MAX_DEV_COUNT; index++)
        {
            if(SPI_Devices[index].init_flag == NU_TRUE)
            {
                (VOID)DVC_Dev_Close (SPI_Devices[index].spi_dev_handle);

                status = SPI_OSAL_Deallocate_Resources(&SPI_Devices[index]);
            }
        }

        /* Zeroize SPI CB list */
        ESAL_GE_MEM_Clear ((VOID*)SPI_Devices, (INT)sizeof(SPI_Devices));
    }

    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       SPI_Open_Device
*
* DESCRIPTION
*
*       This function opens an SPI device.
*
* INPUTS
*
*       DV_DEV_ID spi_dev_id                  - Device ID of the SPI controller
*
* OUTPUTS
*
*
*************************************************************************/
STATUS SPI_Open_Device (DV_DEV_ID spi_dev_id, VOID *context)
{
    DV_DEV_LABEL    device_type_label[] = {{SPI_LABEL}};
    INT             device_label_cnt = sizeof(device_type_label)/sizeof(DV_DEV_LABEL);
    STATUS          status = NU_SUCCESS;
    SPI_CB          *spi_cb;
    DV_IOCTL0_STRUCT ioctl0;
    INT             index;

    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Find an available CB */
    for(index = 0, spi_cb = NU_NULL; index<SPI_MAX_DEV_COUNT; index++)
    {
        if(SPI_Devices[index].init_flag != 1)
        {
            /* An un-used control block is available */
            spi_cb = &SPI_Devices[index];

            /* Initialize spi_cb */

            /* Set init flag */
            spi_cb->init_flag = NU_TRUE;

            spi_cb->spi_dev_id = spi_dev_id;

            break;
        }
    }

    /* If an un-used Control block is available */
    if(index < SPI_MAX_DEV_COUNT)
    {
        /* Open the device */
        status =  DVC_Dev_ID_Open (spi_dev_id,
                                   device_type_label, device_label_cnt,
                                   &(spi_cb->spi_dev_handle));

        if(status == NU_SUCCESS)
        {
            /* Do IOCTL0 */
            memcpy(&ioctl0.label, device_type_label, sizeof(DV_DEV_LABEL));
            status = DVC_Dev_Ioctl (spi_cb->spi_dev_handle, DV_IOCTL0, &ioctl0, sizeof(ioctl0));

            if(status == NU_SUCCESS)
            {
                /* Save SPI ioctl base in the SPI CB */
                spi_cb->spi_ioctl_base = ioctl0.base;

                /* Save SPI Middleware CB in Device Session. This is needed in Device ISR */
                status = DVC_Dev_Ioctl (spi_cb->spi_dev_handle, (spi_cb->spi_ioctl_base)+SPI_SET_CB,
                                        spi_cb, sizeof(*spi_cb));

                /* Get Slave devices count for this controller */
                status = DVC_Dev_Ioctl (spi_cb->spi_dev_handle, (spi_cb->spi_ioctl_base)+SPI_GET_SLAVE_CNT,
                                        &spi_cb->spi_slaves_count, sizeof(spi_cb->spi_slaves_count));

                /* Get the Device Driver Attributes */

                /* Get master/slave mode */
                status = DVC_Dev_Ioctl (spi_cb->spi_dev_handle, (spi_cb->spi_ioctl_base)+SPI_GET_MASTER_SLAVE_MODE,
                                        NU_NULL, NU_NULL);

                /* Set Simulate Transfer Attributes flag. */
                status = DVC_Dev_Ioctl (spi_cb->spi_dev_handle, (spi_cb->spi_ioctl_base)+SPI_GET_SIM_TX_ATTRIBS,
                                        NU_NULL, NU_NULL);

                /* Get interrupt/polled mode */
                status = DVC_Dev_Ioctl (spi_cb->spi_dev_handle, (spi_cb->spi_ioctl_base)+SPI_GET_INTR_POLL_MODE,
                                        NU_NULL, NU_NULL);

                /* Get the SPI middleware config registry path */
                status = DVC_Dev_Ioctl (spi_cb->spi_dev_handle, (spi_cb->spi_ioctl_base)+SPI_GET_MW_CONFIG_PATH,
                                        spi_cb->reg_config_path, sizeof(spi_cb->reg_config_path));

                /* Get middleware settings from device or middleware registry */
                status = SPI_Get_Registry_Info(spi_cb);

                if(status == NU_SUCCESS)
                {

                    /* Init memory pool */
                    spi_cb->spi_memory_pool = &System_Memory;

                    #if         (NU_SPI_USER_BUFFERING_ONLY)

                    /* If Nucleus SPI is configured to use only user buffering for
                       devices using interrupt driven mode then return error if user
                       buffering is not specified with interrupt driven driver mode
                       because now the code that handles internal buffering is not
                       included in compilation. */
                    if (spi_cb->spi_driver_mode == SPI_INTERRUPT_MODE)
                    {
                        /* Set status to indicate that specified driver mode is not
                           available in current configuration. */
                        status = SPI_DRIVER_MODE_UNAVAILABLE;
                    }

                    else

                    #endif      /* NU_SPI_USER_BUFFERING_ONLY */

                    #if         (NU_SPI_SUPPORT_POLLING_MODE == 0)

                    /* If Nucleus SPI is configured to exclude support of polling mode
                       then return error if polling mode is specified for a device
                       because now the code that handles polling mode is not included
                       in compilation. */
                    if ((spi_cb->spi_master_mode == NU_TRUE) &&
                        (spi_cb->spi_driver_mode == SPI_POLLING_MODE))
                    {
                        /* Set status to indicate that specified driver mode is not
                           available in current configuration. */
                        status = SPI_DRIVER_MODE_UNAVAILABLE;
                    }

                    #endif      /* NU_SPI_SUPPORT_POLLING_MODE == 0 */

                    if (status == NU_SUCCESS)
                    {
                        /* Allocate OS resources. */
                        status = SPI_OSAL_Allocate_Resources(spi_cb);

                        /* Check the result of previous operation. */
                        if (status == NU_SUCCESS)
                        {
                            /* ********** Initialize the queue. ************ */
                            /* Setup queue size. */
                            spi_cb->spi_queue.spi_qsize = spi_cb->spi_queue_size;

                            /* Initialize queue's starting read pointer. */
                            spi_cb->spi_queue.spi_qread =
                                spi_cb->spi_queue.spi_qstart;

                            /* Initialize queue's write pointer. */
                            spi_cb->spi_queue.spi_qwrite =
                                spi_cb->spi_queue.spi_qread;

                            /* Set output queue's end. */
                            spi_cb->spi_queue.spi_qend
                                = spi_cb->spi_queue.spi_qread +
                                 (spi_cb->spi_queue.spi_qsize - 1);

                            /* Reset queue count. */
                            spi_cb->spi_queue.spi_qcount = 0;

                            /* Set queue's notification pointer. */
                            spi_cb->spi_queue.spi_qnotify =
                                spi_cb->spi_queue.spi_qread;

                            /* Reset queue notification count. */
                            spi_cb->spi_queue.spi_qnotify_count = 0;

                            /* ******** Initialize the queue buffer. ******** */

                            spi_cb->spi_queue.spi_qbuffer.spi_buff_write =
                                (spi_cb->spi_queue.spi_qbuffer.spi_buff_read =
                                    spi_cb->spi_queue.spi_qbuffer.spi_buff_start);

                            spi_cb->spi_queue.spi_qbuffer.spi_buff_end =
                                spi_cb->spi_queue.spi_qbuffer.spi_buff_read +
                               (spi_cb->spi_buffer_size - 1UL);

                            spi_cb->spi_queue.spi_qbuffer.spi_buff_count = 0;

                            /* Reset active transfer indication flag to false. */
                            spi_cb->spi_transfer_active = NU_FALSE;

                            /* Reset the transfer started flag to false. */
                            spi_cb->spi_transfer_started = NU_FALSE;

                            /* Reset the pending notifications count. */
                            spi_cb->spi_notify_pending = 0;

                            /* Set the SPI device user count to zero */
                            spi_cb->spi_dev_user_count = 0;

                            /* Allocate resources for the transfer attributes
                               list. */
                            status = SPI_OSAL_Allocate_Attribute_List(spi_cb);

                            /* Check if the previous allocation was successful. */
                            if (status == NU_SUCCESS)
                            {
                                /* Allocate resources for the callback list. */
                                status = SPI_OSAL_Allocate_Callbacks_List(spi_cb);
                            }

                            if (status == NU_SUCCESS)
                            {
                                /* Request the driver to setup available
                                   slave addresses in the transfer attributes
                                   list. */
                                status = DVC_Dev_Ioctl (spi_cb->spi_dev_handle,
                                                       (spi_cb->spi_ioctl_base+SPI_SETUP_SLAVE_ADDRS),
                                                        NU_NULL, NU_NULL);

                                /* Enable the controller */
                                if (status == NU_SUCCESS)
                                {
                                    status = DVC_Dev_Ioctl (spi_cb->spi_dev_handle, (spi_cb->spi_ioctl_base)+SPI_ENABLE_DEVICE,
                                                            NU_NULL, NU_NULL);
                                }
                            }
                        } /* if spi resource allocation was successful. */
                    } /* If spi driver and master mode are valid */

                } /* If registry info was obtained */

            } /* ioctl0 was successful */

            /* If any of the IOCTL's failed, close device */
            if(status != NU_SUCCESS)
            {
                /* Close the device */
                status =  DVC_Dev_Close (spi_cb->spi_dev_handle);

                /* Zeroize SPI CB */
                ESAL_GE_MEM_Clear ((VOID*)spi_cb, (INT)sizeof(SPI_CB));
            }

        }
    }
    else
    {
        /* No more un-used CB structure */
    }

    return status;
}

/*************************************************************************
*
* FUNCTION
*
*       SPI_Close_Device
*
* DESCRIPTION
*
*       This function is called by device manager if an SPI hardware
*       driver is unregistered with DM. This function is left empty
*       intentionally as its implementation is not required.
*
* INPUTS
*
*       DV_DEV_ID spi_dev_id                  - Device ID of the SPI controller
*
* OUTPUTS
*
*
*************************************************************************/
STATUS SPI_Close_Device(DV_DEV_ID spi_dev_id, VOID *context)
{
    return ( NU_SUCCESS );
}

/*************************************************************************
* FUNCTION
*
*       SPI_Start
*
* DESCRIPTION
*
*       This function initializes the specified SPI device.
*
* INPUTS
*
*      *spi_dev                             Pointer to the location that
*                                           will get the handle to the
*                                           initialized Nucleus SPI
*                                           device.
*
*      *spi_init                            Pointer to Nucleus SPI device
*                                           initialization block structure.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       SPI_GENERIC_ERROR                   SPI generic error
*
*       SPI_INVALID_HANDLE_POINTER          Specified Nucleus SPI device
*                                           handle pointer is null.
*
*       SPI_NULL_GIVEN_FOR_INIT             Initialization structure
*                                           pointer is null.
*
*       SPI_NULL_GIVEN_FOR_MEM_POOL         Memory pool pointer is null.
*
*       SPI_DRIVER_REGISTER_FAILED          Nucleus SPI driver could not
*                                           be registered with Nucleus SPI.
*
*       SPI_INVALID_PORT_ID                 Specified port is not valid.
*
*       SPI_INVALID_DEVICE_ID               Specified device is not valid.
*
*       SPI_INVALID_DRIVER_MODE             Specified driver mode is not
*                                           valid.
*
*       SPI_INVALID_QUEUE_SIZE              Specified queue size is not
*                                           valid.
*
*       SPI_INVALID_BUFFER_SIZE             Specified buffer size is not
*                                           valid.
*
*       SPI_DRIVER_MODE_UNAVAILABLE         Specified driver mode is not
*                                           available in current
*                                           configuration.
*
*       SPI_DEV_ALREADY_INIT                The specified device has
*                                           already been initialized.
*
*       SPI_INVALID_MODE                    Specified SPI mode is not
*                                           valid.
*
*       SPI_INVALID_BIT_ORDER               Specified bit order is invalid.
*
*       SPI_UNSUPPORTED_BAUD_RATE           The specified baud rate is
*                                           not supported.
*
*       SPI_UNSUPPORTED_MODE                Specified SPI mode is not
*                                           supported.
*
*       SPI_UNSUPPORTED_BIT_ORDER           Specified bit order is not
*                                           supported.
*
*       SPI_UNSUPPORTED_TRANSFER_SIZE       Specified transfer size is
*                                           not supported.
*
*       SPI_MASTER_MODE_NOT_SUPPORTED       Specified SPI device cannot
*                                           function as an SPI master.
*
*       SPI_SLAVE_MODE_NOT_SUPPORTED        Specified SPI device cannot
*                                           function as an SPI slave.
*       SPI_DEVICE_ERROR                    SPI device with given label
*                                           is not found.
*
*************************************************************************/
STATUS  SPI_Start(SPI_HANDLE *spi_dev,  SPI_INIT *spi_init)
{
    SPI_CB             *spi_cb = NU_NULL;
    STATUS              status = NU_SUCCESS;
    SPI_DRV_IOCTL_DATA  spi_drv_ioctl_data;
    DV_DEV_ID           spi_ctlr_dev_id;
    INT                 dev_id_cnt = 1;
    INT                 index = 0;
    SPI_TRANSFER_CONFIG *config;
    SPI_APP_CALLBACKS   *callback;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();

    /* Check if the device handle pointer is valid. */
    if (spi_dev == NU_NULL)
    {
        /* Set status to indicate that the pointer given for returning
           device handle is null. */
        status = SPI_INVALID_HANDLE_POINTER;
    }

    /* Check the initialization control block pointer. */
    if (spi_init == NU_NULL)
    {
        /* Set status to indicate that initialization structure pointer is null. */
        status = SPI_NULL_GIVEN_FOR_INIT;
    }

    /* Check if parameters are valid. */
    if (status == NU_SUCCESS)
    {
        /* Get the control block of the specified device. */
        status = DVC_Dev_ID_Get (&(spi_init->spi_controller_label), 1, &spi_ctlr_dev_id, &dev_id_cnt);

        if(status == NU_SUCCESS && dev_id_cnt > 0)
        {
            /* Get the SPI CB for the controller */
            index = SPICS_Get_CB_Index(spi_ctlr_dev_id);

            if(index < SPI_MAX_DEV_COUNT)
            {
                spi_cb = &SPI_Devices[index];

                /* See if device is already opened or not. Open the device if
                   its not opened yet. */
                if(spi_cb->init_flag != NU_TRUE)
                {
                    /* Open SPI device. */
                    status = SPI_Open_Device(spi_ctlr_dev_id, NU_NULL);

                    /* See if the device was opened properly. */
                    if (status == NU_SUCCESS)
                    {
                        /* Get the SPI CB for the controller */
                        index = SPICS_Get_CB_Index(spi_ctlr_dev_id);

                        spi_cb = &SPI_Devices[index];
                    }
                }

                /* Check the initialization parameters for validity. */
                status = SPICS_Check_Init_Params(spi_init, spi_cb);
                if(status == NU_SUCCESS)
                {
                    /************** INIT BEGIN **********************/

                    /* Get the transfer attributes
                       configuration structure for the current
                       slave address. */
                    config = &(spi_cb->spi_slaves_attribs[spi_init->spi_address]);

                    /* Record the transfer attributes for the
                       current slave address. */
                    config->spi_baud_rate      =
                        spi_init->spi_baud_rate;
                    config->spi_clock_phase    =
                        spi_init->spi_clock_phase;
                    config->spi_clock_polarity =
                        spi_init->spi_clock_polarity;
                    config->spi_bit_order      =
                        spi_init->spi_bit_order;
                    config->spi_transfer_size  =
                        spi_init->spi_transfer_size;

                    /* Set the default slave-select polarity to low. */
                    config->spi_ss_polarity    = SPI_SSPOL_LOW;

                    /* Get the callback function structure for
                       the current slave address. */
                    callback = &(spi_cb->spi_ucb[spi_init->spi_address]);

                    /* Record the callback functions and the
                       slave address. */
                    callback->spi_address = config->spi_address;
                    callback->spi_error =
                        spi_init->spi_callbacks.spi_error;
                    callback->spi_transfer_complete =
                        spi_init->spi_callbacks.spi_transfer_complete;
                        
                    /* If device is not already started by any user then start it now. */
                    if(spi_cb->spi_dev_user_count == 0)
                    {
                        /* Init ioctl data */
                        spi_drv_ioctl_data.address = config->spi_address;
                        spi_drv_ioctl_data.xfer_attrs = config;

                        /* Check if the device is a master. */
                        if (spi_cb->spi_master_mode == NU_TRUE)
                        {
                            /* Call the driver service to set the
                               baud rate. */
                            status = DVC_Dev_Ioctl (spi_cb->spi_dev_handle,
                                                   (spi_cb->spi_ioctl_base+SPI_SET_BAUD_RATE),
                                                   &spi_drv_ioctl_data, sizeof(spi_drv_ioctl_data));
                        }

                        /* Check if previous service was
                           successful. */
                        if (status == NU_SUCCESS)
                        {
                            /* Call the driver service to set the
                               SPI mode. */
                            status = DVC_Dev_Ioctl (spi_cb->spi_dev_handle,
                                                   (spi_cb->spi_ioctl_base+SPI_SET_MODE),
                                                   &spi_drv_ioctl_data, sizeof(spi_drv_ioctl_data));
                        }

                        /* Check if previous service was
                           successful. */
                        if (status == NU_SUCCESS)
                        {
                            /* Call the driver service to set the
                               bit order. */
                            status = DVC_Dev_Ioctl (spi_cb->spi_dev_handle,
                                                   (spi_cb->spi_ioctl_base+SPI_SET_BIT_ORDER),
                                                   &spi_drv_ioctl_data, sizeof(spi_drv_ioctl_data));

                        }

                        /* Check if previous service was
                           successful. */
                        if (status == NU_SUCCESS)
                        {
                            /* Call the driver service to set the
                               transfer size. */
                            status = DVC_Dev_Ioctl (spi_cb->spi_dev_handle,
                                                   (spi_cb->spi_ioctl_base+SPI_SET_TRANSFER_SIZE),
                                                   &spi_drv_ioctl_data, sizeof(spi_drv_ioctl_data));
                        }

                        /* Check if previous service was
                           successful. */
                        if (status == NU_SUCCESS)
                        {
                            /* Call the driver service to set the
                               slave-select polarity. */
                            status = DVC_Dev_Ioctl (spi_cb->spi_dev_handle,
                                                   (spi_cb->spi_ioctl_base+SPI_SET_SS_POLARITY),
                                                   &spi_drv_ioctl_data, sizeof(spi_drv_ioctl_data));
                        }
                    }

                    if (status == NU_SUCCESS)
                    {
                        /* Increment the SPI device user count. */
                        spi_cb->spi_dev_user_count++;

                        /* Return the device handle information to the
                           caller. */
                        *spi_dev  = index;

                        /* Set status as success */
                        status = NU_SUCCESS;
                    }

                    /***********************INIT END ****************************/
                }
                else
                {
                    /* Invalid initialization parameters */
                    status = SPI_DEVICE_ERROR;
                }
            }
            else
            {
                /* No such SPI CB found; some device/driver error */
                status = SPI_DEVICE_ERROR;
            }
        }
        else
        {
            /* SPI controller with the given label is not registered. */
            status = SPI_DEVICE_ERROR;
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
*       SPI_Get_Registry_Info
*
* DESCRIPTION
*
*       This function gets SPI Middleware settings from System Registry
*
* INPUTS
*
*      *spi_cb                              Pointer to the SPI Middleware
*                                           Control Block
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*       SPI_REGISTRY_ERROR                  For any registry errors
*
*************************************************************************/
static STATUS SPI_Get_Registry_Info(SPI_CB* spi_cb)
{
    STATUS status = NU_SUCCESS;
    STATUS reg_status = NU_SUCCESS;
    CHAR mw_config_path[REG_MAX_KEY_LENGTH];


    /* If registry path has path information */
    if(REG_Has_Key(spi_cb->reg_config_path))
    {
        /* Get queue size */
        strncpy(mw_config_path, spi_cb->reg_config_path, sizeof(mw_config_path));
        if((strlen(mw_config_path) + strlen("/queue_size")+1) <= REG_MAX_KEY_LENGTH)
        {
            reg_status = REG_Get_UINT16 (strcat(mw_config_path,"/queue_size"), (UINT16*)&(spi_cb->spi_queue_size));

            if(reg_status == NU_SUCCESS)
            {
                /* Get buffer size */
                strncpy(mw_config_path, spi_cb->reg_config_path, sizeof(mw_config_path));
                if((strlen(mw_config_path) + strlen("/buffer_size")+1) <= REG_MAX_KEY_LENGTH)
                {
                    reg_status = REG_Get_UINT32 (strcat(mw_config_path,"/buffer_size"), (UINT32*)&(spi_cb->spi_buffer_size));
                }
                else
                {
                    status = SPI_REGISTRY_ERROR;
                }
            }
        }
        else
        {
            status = SPI_REGISTRY_ERROR;
        }

        if((reg_status == NU_SUCCESS) && (status == NU_SUCCESS))
        {
            status = NU_SUCCESS;
        }
        else
        {
            status = SPI_REGISTRY_ERROR;
        }
    }
    /* If mw config not obtained from driver metadata, get the MW defaults */
    else
    {
        strncpy(mw_config_path, SPI_MW_Reg_Path, REG_MAX_KEY_LENGTH);
        if((strlen(mw_config_path) + strlen("/queue_size")+1) <= REG_MAX_KEY_LENGTH)
        {
            reg_status = REG_Get_UINT16(strcat(mw_config_path,"/queue_size"), &(spi_cb->spi_queue_size));
            if(reg_status == NU_SUCCESS)
            {
                status = NU_SUCCESS;
            }
            else
            {
                status = SPI_REGISTRY_ERROR;
            }
        }

        strncpy(mw_config_path, SPI_MW_Reg_Path, REG_MAX_KEY_LENGTH);
        if((strlen(mw_config_path) + strlen("/buffer_size")+1) <= REG_MAX_KEY_LENGTH)
        {
            reg_status = REG_Get_UINT32(strcat(mw_config_path,"/buffer_size"), &(spi_cb->spi_buffer_size));
            if(reg_status == NU_SUCCESS)
            {
                status = NU_SUCCESS;
            }
            else
            {
                status = SPI_REGISTRY_ERROR;
            }
        }
    }

    return status;
}

