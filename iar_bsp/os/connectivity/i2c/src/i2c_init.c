/*************************************************************************
*
*                  Copyright 2006 Mentor Graphics Corporation
*                         All Rights Reserved.
*
* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS THE
* PROPERTY OF MENTOR GRAPHICS CORPORATION OR ITS LICENSORS AND IS SUBJECT
* TO LICENSE TERMS.
*
**************************************************************************

**************************************************************************
*
* FILE NAME
*
*       i2c_init.c
*
* COMPONENT
*
*       I2C - I2C Core
*
* DESCRIPTION
*
*       This file contains the initialization services for Nucleus I2C.
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       nu_os_conn_i2c_init                 Entery function of I2C run 
*                                           level in RS.
*       I2C_Open_Device                     I2C device registration callback 
*                                           function.
*       I2C_Close_Device                    I2C device un-registration callback 
*                                           function.
*       NU_I2C_Open                         Application interface for 
*                                           obtaining access for I2C 
*                                           operations of an interface 
*                                           from user space.
*       I2C_Initialize                      Performs initialization for 
*                                           I2C generic layer.
*       I2C_Get_CB                          Get pointer to I2C control 
*                                           block.
*       I2C_Obtain_Access                   Get the I2C control block 
*                                           corresponding to a device ID
*
* DEPENDENCIES
*
*       i2c_extr.h                          Function prototypes for
*                                           Nucleus I2C services.
*
*       i2c_osal_extr.h                     Function prototypes for
*                                           Nucleus I2C OS abstraction
*                                           services.
*
*       reg_api.h                           Contains registger API 
*                                           definitions.
*
*************************************************************************/


/*********************************/
/* INCLUDE FILES                 */
/*********************************/

#define     NU_I2C_SOURCE_FILE

/* Define to identify that the module is responsible for
   Nucleus I2C initialization. */
#define     NU_I2C_INITIALIZATION

#include    "services/reg_api.h"
#include    "services/runlevel_init.h"
#include    "connectivity/i2c_extr.h"
#include    "connectivity/i2c_osal_extr.h"

/*********************************/
/* MACROS                        */
/*********************************/
/* For device change event */
#define I2C_DEVICE_OPENED_BIT_MASK 1

/*********************************/
/* EXTERNAL VARIABLES            */
/*********************************/
extern NU_MEMORY_POOL  System_Memory;

/*******************************/
/* LOCAL VARIABLE DECLARATIONS */
/*******************************/
I2C_CB                      I2c_cbs[I2C_MAX_INSTANCES];

static  NU_SEMAPHORE        NU_I2C_Open_Semaphore;
static  EV_GCB              I2C_Device_Change_Event;

/*********************************/
/* FUNCTION PROTOTYPES           */
/*********************************/

/* Function prototype for generic Nucleus I2C initialization. */
static  STATUS  I2C_Initialize (NU_MEMORY_POOL *mem_pool);
static I2C_HANDLE I2C_Obtain_Access (DV_DEV_LABEL *name);

STATUS I2C_Open_Device (DV_DEV_ID i2c_dev_id, VOID *context);
STATUS I2C_Close_Device (DV_DEV_ID i2c_dev_id, VOID *context);

/* I2C Middleware init function */
STATUS nu_os_conn_i2c_init(const CHAR * key, INT startorstop);

/*************************************************************************
* FUNCTION
*
*       nu_os_conn_i2c_init
*
* DESCRIPTION
*
*       This function initializes Nucleus I2C. It
*       also creates the notification handler thread for Nucleus I2C and 
*       I2C controller discovery task.
*
* INPUTS
*
*      *mem_pool                            Memory pool pointer.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       I2C_NULL_GIVEN_FOR_MEM_POOL         The specified memory pool
*                                           pointer is null.
*
*************************************************************************/
STATUS  nu_os_conn_i2c_init(const CHAR * key, INT compctrl)
{
    STATUS              status = NU_SUCCESS;
    DV_LISTENER_HANDLE  listener_handle;
    DV_DEV_LABEL        device_type_label[] = {{I2C_LABEL}};

    if(compctrl == RUNLEVEL_START)
    {
        /* One time initialization for Nucleus I2C. */
        status = I2C_Initialize(&System_Memory);
        if ( status == NU_SUCCESS )
        {
            /* Register a notification listener with device manager. */
            status = DVC_Reg_Change_Notify(device_type_label,
                                            DV_GET_LABEL_COUNT(device_type_label),
                                            I2C_Open_Device,
                                            I2C_Close_Device,
                                            NU_NULL,
                                            &listener_handle);
        }   
    }
    /* Return the completion status of the service. */
    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       I2c_Open_Device
*
* DESCRIPTION
*
*       This function opens a i2c device
*
* INPUTS
*
*       DV_DEV_ID i2c_dev_id                  - Device ID of the I2C controller
*
* OUTPUTS
*
*
*************************************************************************/
STATUS I2C_Open_Device (DV_DEV_ID i2c_dev_id, VOID *context)
{
    DV_DEV_LABEL    i2c_class_label[] = {{I2C_LABEL}};
    INT             cb_index;
    DV_IOCTL0_STRUCT ioctl0;
    DV_DEV_HANDLE   dev_handle;
    STATUS          status;

     /* Find an available session */
    for(cb_index=0; cb_index<I2C_MAX_INSTANCES; cb_index++)
    {
        if(I2c_cbs[cb_index].device_opened == NU_FALSE)
        {
            break;
        }
    }
            
    /* If a session is available */
    if (cb_index < I2C_MAX_INSTANCES)
    {
        I2c_cbs[cb_index].i2c_dv_id = i2c_dev_id;

        /* Open device in I2C mode */
        status = DVC_Dev_ID_Open (i2c_dev_id, i2c_class_label, 1, &dev_handle);
        if (status == NU_SUCCESS)
        {
            /* Save device information in session */
            I2c_cbs[cb_index].i2c_dv_handle = dev_handle;
            
            /* Init ioct0 structure with I2C 'mode' label */
            memcpy(&ioctl0.label, i2c_class_label, sizeof(DV_DEV_LABEL));
            
            status = DVC_Dev_Ioctl (dev_handle, DV_IOCTL0, &ioctl0, sizeof(DV_IOCTL0_STRUCT));
            if(status == NU_SUCCESS)             
            {
                I2c_cbs[cb_index].i2c_ioctl_base = ioctl0.base;
                I2c_cbs[cb_index].device_opened = NU_TRUE;

                /* Set an event flag to show a device has been opened. */
                status = NU_Set_Events(&I2C_Device_Change_Event, I2C_DEVICE_OPENED_BIT_MASK, 
                            (OPTION)NU_OR);
            }
            else
            {
                /* Close the device */
                status = DVC_Dev_Close(dev_handle);
            }
       }
    }
    else
    {
        /* ERROR No more sessions available */
        status = I2C_SESSION_UNAVAILABLE;
    }

    return (status);
}

/*************************************************************************
*
* FUNCTION
*
*       I2C_Close_Device
*
* DESCRIPTION
*
*       This function is called by device manager if an I2C hardware 
*       driver is unregistered with DM. This function is left empty 
*       intentionally as its implementation is not required.
*
* INPUTS
*
*       DV_DEV_ID spi_dev_id                  - Device ID of the I2C controller
*
* OUTPUTS
*
*
*************************************************************************/
STATUS I2C_Close_Device (DV_DEV_ID i2c_dev_id, VOID *context)
{
    return ( NU_SUCCESS );
}

/*************************************************************************
* FUNCTION
*
*       NU_I2C_Open
*
* DESCRIPTION
*
*       Open a I2C controller
*
* INPUTS
*
*       i2c_controller_name                 Label of the specific
*                                           I2C controller
*
*       *i2c_handle                         Pointer to Nucleus I2C
*                                           device handle.
*
*       *i2c_init                           Pointer to device
*                                           initialization block structure.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       I2C_DEV_ALREADY_INIT                The specified device has
*                                           already been initialized.
*
*       I2C_DRIVER_REGISTER_FAILED          Nucleus I2C driver could not
*                                           be registered with Nucleus I2C.
*
*       I2C_INVALID_ADDRESS_TYPE            Address type is not 7-bit or
*                                           10-bit.
*
*       I2C_INVALID_BAUDRATE                The specified baud rate is
*                                           not supported by the driver.
*
*       I2C_INVALID_DEVICE_ID               Specified device is not valid.
*
*       I2C_INVALID_DRIVER_MODE             Specified driver mode is not
*                                           valid.
*
*       I2C_INVALID_HANDLE_POINTER          Null given for returning the
*                                           I2C_HANDLE pointer.
*
*       I2C_INVALID_NODE_TYPE               Node type is not master/slave.
*
*       I2C_INVALID_PORT_ID                 Specified port is not valid.
*
*       I2C_INVALID_SLAVE_ADDRESS           Slave address is not valid as
*                                           it falls into the reserved
*                                           range or is more than the
*                                           maximum possible value.
*
*       I2C_NULL_GIVEN_FOR_INIT             Initialization structure
*                                           pointer is null.
*
*       I2C_NULL_GIVEN_FOR_MEM_POOL         Memory pool pointer is null.
*
*************************************************************************/
INT NU_I2C_Open (DV_DEV_LABEL *i2c_controller_name, I2C_HANDLE *p_i2c_handle, I2C_INIT  *i2c_init)
{
    I2C_CB          *i2c_cb   ;
    STATUS          status   = NU_SUCCESS;
    I2C_HANDLE      i2c_handle;
    CHAR            mw_config_path[REG_MAX_KEY_LENGTH];
    DV_DEV_LABEL    dev_label_list[] = {{I2C_LABEL}};
    DV_DEV_ID       dev_id_list[CFG_NU_OS_CONN_I2C_MAX_DEVS_SUPPORTED];
    INT             dev_id_cnt = CFG_NU_OS_CONN_I2C_MAX_DEVS_SUPPORTED;
    INT             i;

    /* Declare variable needed by mode switching macros. */
    NU_SUPERV_USER_VARIABLES

    /* Switch to supervisor mode. */
    NU_SUPERVISOR_MODE();
    
    /* Wait until the device was opened */
    *p_i2c_handle = i2c_handle = I2C_Obtain_Access (i2c_controller_name);

    if (i2c_handle == NU_NULL)
    {
        /* Find device id */
        status = DVC_Dev_ID_Get (dev_label_list, 1, dev_id_list, &dev_id_cnt);

        if (dev_id_cnt > 0)
        {
            for (i = 0; i < dev_id_cnt; i++)
            {
                /* Open device */
                status = I2C_Open_Device (dev_id_list[i], NU_NULL);

                if(status == NU_SUCCESS)
                {
                    /* Get device handle */
                    *p_i2c_handle = i2c_handle = I2C_Obtain_Access (i2c_controller_name);

                    break;
                }
            }
        }

    }

    if ((status == NU_SUCCESS) && (i2c_handle != NU_NULL))
    {
        /* Serialize access to this function */
        status = NU_Obtain_Semaphore(&NU_I2C_Open_Semaphore, NU_SUSPEND);
        if ( status == NU_SUCCESS )
        {
            I2CS_Get_CB (i2c_handle, &i2c_cb);

            /* Check the initialization parameters for validity. */
            status = I2CS_Check_Init_Params(i2c_init);
            if ( status == NU_SUCCESS )
            {
                /* Get the I2C middleware config registry path */
                status = DVC_Dev_Ioctl (i2c_cb->i2c_dv_handle,
                                        i2c_cb->i2c_ioctl_base + I2C_GET_MW_CONFIG_PATH,
                                        mw_config_path, sizeof(mw_config_path));

                /* If registry path has path information */
                if((status == NU_SUCCESS) && (mw_config_path[0] != 0))
                {
                    UINT32 data;

                    /* Get baud rate */
                    status = REG_Get_UINT32_Value (mw_config_path, "/baud", &data);
                    if(status == NU_SUCCESS)
                    {
                        i2c_init->i2c_baudrate = data;
                    }
                }

                /* Initialize the device, if it has not been initialized
                   already. */
                if (i2c_cb->is_opened == NU_FALSE)
                {
                    /* Allocate OS resources. */
                    status = I2C_OSAL_Allocate_Resources(i2c_init, i2c_cb);

                    /* Check if OS resources allocation was successful. */
                    if (status == NU_SUCCESS)
                    {
                        /* Set Nucleus I2C control block fields. */
                        i2c_cb->i2c_baudrate = i2c_init->i2c_baudrate;

                        /* Set the device user count to 1. */
                        i2c_cb->i2c_dev_user_count = 1;

                        /* Set the memory pool. */
                        i2c_cb->i2c_memory_pool = i2c_init->i2c_memory_pool;

                        /* Set the default callbacks. */

                        i2c_cb->i2c_ucb.callback->i2c_ack_indication =
                            i2c_init->i2c_app_cb.i2c_ack_indication;
                        i2c_cb->i2c_ucb.callback->i2c_data_indication =
                            i2c_init->i2c_app_cb.i2c_data_indication;
                        i2c_cb->i2c_ucb.callback->i2c_transmission_complete =
                            i2c_init->i2c_app_cb.i2c_transmission_complete;
                        i2c_cb->i2c_ucb.callback->i2c_address_indication =
                            i2c_init->i2c_app_cb.i2c_address_indication;
                        i2c_cb->i2c_ucb.callback->i2c_error =
                            i2c_init->i2c_app_cb.i2c_error;

                        /* Set the next pointer to null. */
                        i2c_cb->i2c_ucb.next     = NU_NULL;

                        i2c_cb->i2c_driver_mode = i2c_init->i2c_driver_mode;
                        i2c_cb->i2c_10bit_address2 = 0;

                        /* Set node type (master/slave) for the I2C device. */
                        i2c_cb->i2c_node_address.i2c_node_type =
                            i2c_init->i2c_node_address.i2c_node_type;

                        /* Set slave address for the node, if it provides the
                           functionality for that. */
                        if (i2c_cb->i2c_node_address.i2c_node_type &
                            I2C_SLAVE_NODE)
                        {
                            i2c_cb->i2c_node_address.i2c_slave_address =
                                i2c_init->i2c_node_address.i2c_slave_address;
                            i2c_cb->i2c_node_address.i2c_address_type =
                                i2c_init->i2c_node_address.i2c_address_type;
                            i2c_cb->i2c_ucb.callback->i2c_slave_address =
                                i2c_init->i2c_node_address.i2c_slave_address;
                        }

                        /* Set node address for general call response. */
                        else
                        {
                            i2c_cb->i2c_node_address.i2c_slave_address =
                                I2C_GENERAL_CALL_ADDRESS;
                            i2c_cb->i2c_node_address.i2c_address_type =
                                I2C_7BIT_ADDRESS;
                            i2c_cb->i2c_ucb.callback->i2c_slave_address =
                            I2C_DEFAULT_CBSLAVE_ADDRESS;
                        }

                        /* Set active mode for the node as master or slave. */
                        i2c_cb->i2c_active_mode = i2c_init->i2c_active_mode;

                        /* Set the size (in bytes) for transmission and
                           reception buffers. */
                        i2c_cb->i2c_io_buffer.i2cbm_tx_buffer_size
                            = i2c_init->i2c_tx_buffer_size;
                        i2c_cb->i2c_io_buffer.i2cbm_rx_buffer_size
                            = i2c_init->i2c_rx_buffer_size;

                        /* Reset data read/write pointers to start of I/O
                           buffers. */
                        i2c_cb->i2c_io_buffer.i2cbm_tx_buffer.i2cbm_data_read =
                         i2c_cb->i2c_io_buffer.i2cbm_tx_buffer.i2cbm_data_start;
                        i2c_cb->i2c_io_buffer.i2cbm_rx_buffer.i2cbm_data_read =
                         i2c_cb->i2c_io_buffer.i2cbm_rx_buffer.i2cbm_data_start;
                        i2c_cb->i2c_io_buffer.i2cbm_tx_buffer.i2cbm_data_write =
                         i2c_cb->i2c_io_buffer.i2cbm_tx_buffer.i2cbm_data_start;
                        i2c_cb->i2c_io_buffer.i2cbm_rx_buffer.i2cbm_data_write =
                         i2c_cb->i2c_io_buffer.i2cbm_rx_buffer.i2cbm_data_start;

                        /* Reset data byte count in the I/O buffers. */
                        i2c_cb->i2c_io_buffer.i2cbm_tx_buffer.i2cbm_count = 0;
                        i2c_cb->i2c_io_buffer.i2cbm_rx_buffer.i2cbm_count = 0;

                        /* Reset multi transfer queue structure. */
                        i2c_cb->i2c_mtr_queue.i2c_slave_address = NU_NULL;
                        i2c_cb->i2c_mtr_queue.i2c_slave_tx_data = NU_NULL;
                        i2c_cb->i2c_mtr_queue.i2c_slave_rx_data = NU_NULL;
                        i2c_cb->i2c_mtr_queue.i2c_data_length   = NU_NULL;
                        i2c_cb->i2c_mtr_queue.i2c_read_write    = NU_NULL;
                        i2c_cb->i2c_mtr_queue.i2c_slave_count   = 0;

                        /* Reset I2C handler type. */
                        i2c_cb->i2c_handler_type = 0;

                        /* Set Nucleus I2C node state to idle to indicate that
                           no transfer is in progress. */
                        i2c_cb->i2c_node_state = I2C_NODE_IDLE;

                        i2c_cb->is_opened = NU_TRUE;
                    }
                }

                /* Device is already initialized. */
                else
                {
                    /* Increment the device user count. */
                    i2c_cb->i2c_dev_user_count++;

                    /* Set the status to indicate that the device is already
                       initialized. */
                    status = I2C_DEV_ALREADY_INIT;
                }

                if ( status == NU_SUCCESS )
                {
                    status = I2CC_Ioctl_Driver (i2c_handle, I2C_SET_CONTROL_BLOCK, i2c_cb);
                }

#if ( !NU_I2C_SUPPORT_POLLING_MODE )
                if ( status == NU_SUCCESS )
                {
                    status = I2CC_Ioctl_Driver (i2c_handle, I2C_ENABLE_INTERRUPT, NU_NULL);
                }
#endif

                if ( status == NU_SUCCESS )
                {
                    /* Check if the node supports master functionality. */
                    if ((i2c_cb->i2c_node_address.i2c_node_type & I2C_MASTER_NODE) &&
                        (i2c_cb->i2c_active_mode == I2C_MASTER_NODE))
                    {
                        UINT16 baudrate = i2c_cb->i2c_baudrate;
                        status = I2CC_Ioctl_Driver (i2c_handle, I2C_SET_BAUDRATE, &baudrate);
                        if ( status == NU_SUCCESS )
                        {
                            status = I2CC_Ioctl_Driver (i2c_handle, I2C_SET_MASTER_MODE, NU_NULL);
                        }
                    }
                }

                if ( status == NU_SUCCESS )
                {
                    /* Check if the node supports slave mode? */
                    if((i2c_cb->i2c_node_address.i2c_node_type & I2C_SLAVE_NODE) &&
                        (i2c_cb->i2c_active_mode == I2C_SLAVE_NODE))
                    {
                        status = I2CC_Ioctl_Driver (i2c_handle, I2C_SET_SLAVE_MODE, i2c_cb);
                        if ( status == NU_SUCCESS )
                        {
                            status = I2CC_Ioctl_Driver (i2c_handle, I2C_SET_SLAVE_ADDRESS, i2c_cb);
                        }
                    }
                }

                if ( status == NU_SUCCESS )
                {
                    status = I2CC_Ioctl_Driver (i2c_handle, I2C_ENABLE_DEVICE, NU_NULL);
                }
            }
            
            /* Allow other threads to use this service */
            NU_Release_Semaphore(&NU_I2C_Open_Semaphore);
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
*       I2C_Initialize
*
* DESCRIPTION
*
*       This function initializes Nucleus I2C data structures.
*
* INPUTS
*
*      *mem_pool                            Memory pool pointer.
*
* OUTPUTS
*
*       NU_SUCCESS                          Service completed successfully.
*
*       I2C_NULL_GIVEN_FOR_MEM_POOL         Memory pool pointer is null.
*
*************************************************************************/
STATUS  I2C_Initialize (NU_MEMORY_POOL *mem_pool)
{
    STATUS          status;
    UINT8           rollback;

    /* Initialize status to a default error value. */
    status      = I2C_NULL_GIVEN_FOR_MEM_POOL;
    rollback    = 0;

    /* Check if the specified memory pool is valid. */
    if (mem_pool != NU_NULL)
    {
        do
        {
            /* Create notification handler threads for Nucleus I2C. */
            status = I2C_OSAL_Create_Handler(mem_pool);
            if ( status != NU_SUCCESS )
            {
                rollback = 0;
                break;
            }   
            
            /* Create the semaphore */
            status = NU_Create_Semaphore (&NU_I2C_Open_Semaphore, "i2c_sem",
                                          (UNSIGNED)1, (OPTION)NU_FIFO);
            if ( status != NU_SUCCESS )
            {
                rollback = 1;
                break;
            }
            
            /* Create an event group for the I2C device access system to use */
            status = NU_Create_Event_Group(&I2C_Device_Change_Event, "I2CEVT");
            if ( status != NU_SUCCESS )
            {
                rollback = 2;
                break;
            }       
        }while(0);

        /* If there was any error in execution then perform cleanup before exit. */
        switch(rollback)
        {
            case 2:
                NU_Delete_Semaphore(&NU_I2C_Open_Semaphore);
            case 1:
            default:
                break;
        }       
    }   

    /* Return the completion status of the service. */
    return (status);
}


/*************************************************************************
* FUNCTION
*
*       I2C_Get_CB
*
* DESCRIPTION
*
*       Get the I2C control block corresponding to a device ID
*
* INPUTS
*
*      dev_id                           Device ID of the I2C controller
*
* OUTPUTS
*
*       pointer to the control block, or NU_NULL if not found
*
*************************************************************************/
I2C_CB* I2C_Get_CB (DV_DEV_ID dev_id)
{
    int i;
    for (i=0;i < I2C_MAX_INSTANCES; i++)
        if (I2c_cbs[i].i2c_dv_id == dev_id)
            return &I2c_cbs[i];
    return NU_NULL;
}

/*************************************************************************
* FUNCTION
*
*       I2C_Obtain_Access
*
* DESCRIPTION
*
*       Get the I2C handle corresponding to a I2C controller label
*
* INPUTS
*
*      name                           Label of the I2C controller
*
* OUTPUTS
*
*      I2C handle corresponding to a I2C controller label 
*
*************************************************************************/
I2C_HANDLE I2C_Obtain_Access (DV_DEV_LABEL *name)
{
    DV_DEV_LABEL    i2c_label[] = {{I2C_LABEL}};
    DV_DEV_LABEL    labels[2];
    INT             label_cnt;
    DV_DEV_ID       i2c_dev_id_list[CFG_NU_OS_CONN_I2C_MAX_DEVS_SUPPORTED];
    INT             i, dev_id_cnt = CFG_NU_OS_CONN_I2C_MAX_DEVS_SUPPORTED;
    I2C_HANDLE      handle = NU_NULL;
    UNSIGNED        event_val;
    STATUS          status;

    /* Wait for device discovery. */
    status = NU_Retrieve_Events (&I2C_Device_Change_Event, 
                                I2C_DEVICE_OPENED_BIT_MASK,
                                NU_OR, 
                                &event_val, 
                                NU_SUSPEND);
    if ( (status == NU_SUCCESS) && (event_val & I2C_DEVICE_OPENED_BIT_MASK) )
    {
        /* Copy I2C label for use. */
        memcpy(&labels[0], &i2c_label[0], sizeof(DV_DEV_LABEL));

         /* If no name was passed-in, then we will look for one label only i.e. I2C */
        if(name == NU_NULL)
        {
            label_cnt = 1;
        }
        /* If a name was passed-in, copy that name as second entry in label list */
        else
        {
            /* Copy user label */
            memcpy(&labels[1], name, sizeof(DV_DEV_LABEL));
            label_cnt = 2;
        }

        /* Get all devices with these labels; <I2C ,name>  */
        status = DVC_Dev_ID_Get (labels, label_cnt, i2c_dev_id_list, &dev_id_cnt);
        if (status == NU_SUCCESS && dev_id_cnt > 0)
        {
            for (i=0; i<dev_id_cnt; i++)
            {
                if ((handle = (I2C_HANDLE)I2C_Get_CB(i2c_dev_id_list[i])) != NU_NULL)
                {
                    break;
                }
            }
        }
    }

    return handle;
}
